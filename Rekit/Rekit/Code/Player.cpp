#include "Player.h"

#include <array>
#include <algorithm>			// Use std::min(), max().
#include <vector>

#include "Donya/Constant.h"		// Use DEBUG_MODE, scast macros.
#include "Donya/Loader.h"
#include "Donya/Sound.h"
#include "Donya/Template.h"		
#include "Donya/Useful.h"		// Use convert string functions.
#include "Donya/Sprite.h"

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#endif // DEBUG_MODE

#include "Common.h"
#include "FilePath.h"
#include "GimmickUtil.h"		// Use for confirming to slip ground.
#include "GimmickImpl/Bomb.h"	// Use for confirming to "is the attribute danger?".
#include "Music.h"
#include "SceneGame.h"

#undef max
#undef min

using namespace GimmickUtility;

class PlayerParam final : public Donya::Singleton<PlayerParam>
{
	friend Donya::Singleton<PlayerParam>;
public:
	struct Member
	{
		int		maxJumpCount{};		// 0 is can not jump, 1 ~ is can jump.
		float	moveSpeed{};		// Use for a horizontal move. It will influenced by "elapsedTime".
		float	brakeSpeed{};		// Use for represent a slipping. It will influenced by "elapsedTime".
		float	jumpPower{};		// Use for a just moment of using a jump.
		float	maxFallSpeed{};		// Use for a limit of falling speed.
		float	gravity{};			// Always use to fall. It will influenced by "elapsedTime".
		float	drawScale{ 1.0f };

		AABBEx	hitBoxPhysic{};	// Hit-Box of using to the collision to the stage.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( maxJumpCount ),
				CEREAL_NVP( moveSpeed ),
				CEREAL_NVP( jumpPower ),
				CEREAL_NVP( maxFallSpeed ),
				CEREAL_NVP( gravity ),
				CEREAL_NVP( hitBoxPhysic )
			);
			if ( 1 <= version )
			{
				archive( CEREAL_NVP( brakeSpeed ) );
			}
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( drawScale ) );
			}
			if ( 3 <= version )
			{
				// archive( CEREAL_NVP( x ) )
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "Player";
	Member m;
private:
	PlayerParam() : m() {}
public:
	~PlayerParam() = default;
public:
	void Init()
	{
		LoadParameter();
	}
	void Uninit()
	{

	}
public:
	Member Data() const
	{
		return m;
	}
private:
	void LoadParameter( bool fromBinary = true )
	{
		std::string filePath = GenerateSerializePath( SERIAL_ID, fromBinary );
		Donya::Serializer::Load( m, filePath.c_str(), SERIAL_ID, fromBinary );
	}

#if USE_IMGUI

	void SaveParameter()
	{
		bool useBinary = true;
		std::string filePath{};

		filePath = GenerateSerializePath( SERIAL_ID, useBinary );
		Donya::Serializer::Save( m, filePath.c_str(), SERIAL_ID, useBinary );

		useBinary = false;
		
		filePath = GenerateSerializePath( SERIAL_ID, useBinary );
		Donya::Serializer::Save( m, filePath.c_str(), SERIAL_ID, useBinary );
	}

public:
	void UseImGui()
	{
		if ( ImGui::BeginIfAllowed() )
		{
			if ( ImGui::TreeNode( u8"プレイヤー・調整データ" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x  );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"質量"                ).c_str(), &pHitBox->mass   );
					ImGui::Checkbox  ( ( prefix + u8"当たり判定は有効か"   ).c_str(), &pHitBox->exist  );
				};

				ImGui::DragInt( u8"最大ジャンプ回数",		&m.maxJumpCount,	1.0f, 0		);
				ImGui::DragFloat( u8"横移動速度",		&m.moveSpeed,		1.0f, 0.0f	);
				ImGui::DragFloat( u8"減速力（氷床）",		&m.brakeSpeed,		1.0f, 0.0f	);
				ImGui::DragFloat( u8"ジャンプ初速",		&m.jumpPower,		1.0f, 0.0f	);
				ImGui::DragFloat( u8"最大落下速度",		&m.maxFallSpeed,	1.0f, 0.0f	);
				ImGui::DragFloat( u8"重力",				&m.gravity,			1.0f, 0.0f	);
				ImGui::DragFloat( u8"描画スケール",		&m.drawScale,		0.1f		);

				AdjustAABB( u8"当たり判定：ＶＳ地形", &m.hitBoxPhysic );

				if ( ImGui::TreeNode( u8"ファイル" ) )
				{
					static bool isBinary = true;
					if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true; }
					if ( ImGui::RadioButton( "JSON", !isBinary ) ) { isBinary = false; }
					std::string loadStr{ "読み込み " };
					loadStr += ( isBinary ) ? "Binary" : "JSON";

					if ( ImGui::Button( u8"保存" ) )
					{
						SaveParameter();
					}
					if ( ImGui::Button( Donya::MultiToUTF8( loadStr ).c_str() ) )
					{
						LoadParameter( isBinary );
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			ImGui::End();
		}
	}

#endif // USE_IMGUI
};

CEREAL_CLASS_VERSION( PlayerParam::Member, 2 )

Donya::StaticMesh	Player::drawModel{};
bool				Player::wasLoaded{ false };

Player::Player() :
	status(State::Normal),
	collideKeyCounter(0), remainJumpCount(1),
	drawAlpha(1.0f),
	pos(), velocity(),
	aboveSlipGround(false),
	seeRight(true),
	viewOpenCount(0),
	isCatchKey(false),
	idOpenDoor(0)
{}
Player::~Player() = default;

void Player::Init( const Donya::Vector3 &wsInitPos )
{
	PlayerParam::Get().Init();

	LoadModel();

	pos = wsInitPos;

	collideKeyCounter = 0;
	viewOpenCount = 0;
	idOpenDoor = Donya::Sprite::Load(L"Data/Images/Door_Open.png");
}
void Player::Uninit()
{
	PlayerParam::Get().Uninit();
}

void Player::Update( float elapsedTime, Input controller )
{
#if USE_IMGUI

	PlayerParam::Get().UseImGui();
	UseImGui();

#endif // USE_IMGUI

	switch ( status )
	{
	case Player::State::Normal:
		NormalUpdate( elapsedTime, controller );
		return;
	case Player::State::Dead:
		DeadUpdate( elapsedTime, controller );
		return;
	default: return;
	}
}

void Player::PhysicUpdate( const std::vector<BoxEx> &terrains )
{
	if ( IsDead() ) { return; }
	// else

	// VS Key.
	{
		BoxEx movedBody = GetHitBox().Get2D();
		movedBody.pos += movedBody.velocity;

		bool nowHit = false;
		for ( const auto &it : terrains )
		{
			if ( !GimmickUtility::HasAttribute( GimmickKind::TriggerKey, it ) ) { continue; }
			// else

			if ( Donya::Box::IsHitBox( movedBody, it, /* ignoreExistFlag = */ true ) )
			{
				nowHit = true;
				break;
			}
		}

		if ( nowHit )
		{
			collideKeyCounter++;
		}
		else
		{
			collideKeyCounter = 0;
		}
	}

	/// <summary>
	/// Support an attribute.
	/// </summary>
	auto Version_4 = [&]()
	{
		aboveSlipGround = false; // This flag must be reset before collision.

		auto CalcCollidingBox = [&]( const BoxEx &myself, const BoxEx &previousMyself )->BoxEx
		{
			for ( const auto &it : terrains )
			{
				if ( it == previousMyself ) { continue; }
				// else

				if ( !it.exist )
				{
					if ( Bomb::IsExplosionBox( it ) && Donya::Box::IsHitBox( it, myself, /* ignoreExistFlag = */ true ) )
					{
						return it;
					}
					// else
					continue;
				}
				// else

				if ( Donya::Box::IsHitBox( it, myself ) )
				{
					return it;
				}
			}

			return BoxEx::Nil();
		};

		const AABBEx actualBody		= GetHitBox();
		const BoxEx  previousXYBody	= actualBody.Get2D();

		Donya::Vector2 xyVelocity{ velocity.x, velocity.y };
		Donya::Vector2 moveSign // The moving direction of myself. Take a value of +1.0f or -1.0f.
		{
			scast<float>( Donya::SignBit( xyVelocity.x ) ),
			scast<float>( Donya::SignBit( xyVelocity.y ) )
		};

		Donya::Vector2	lastResolver{};
		BoxEx			lastHitOther{};

		BoxEx movedXYBody = previousXYBody;
		movedXYBody.pos  += xyVelocity;

		BoxEx other{};

		constexpr unsigned int MAX_LOOP_COUNT = 1000U;
		unsigned int loopCount{};
		while ( ++loopCount < MAX_LOOP_COUNT )
		{
			other = CalcCollidingBox( movedXYBody, previousXYBody );
			if ( other == BoxEx::Nil() ) { break; } // Does not detected a collision.
			// else

			if ( Bomb::IsExplosionBox( other ) || HasDangerAttribute( other ) )
			{
				KillMe();
				return;
			}
			// else

			if ( ZeroEqual( moveSign.x ) && !ZeroEqual( other.velocity.x ) )
			{
				// The myself's moving direction is considered the inverse of other's moving direction.
				moveSign.x = scast<float>( Donya::SignBit( -other.velocity.x ) );
			}
			if ( ZeroEqual( moveSign.y ) && !ZeroEqual( other.velocity.y ) )
			{
				// The myself's moving direction is considered the inverse of other's moving direction.
				moveSign.y = scast<float>( Donya::SignBit( -other.velocity.y ) );
			}

			if ( moveSign.IsZero() ) { continue; } // Each other does not move, so collide is no possible.
			// else

			Donya::Vector2 penetration{}; // Store absolute value.
			Donya::Vector2 plusPenetration
			{
				fabsf( ( movedXYBody.pos.x + movedXYBody.size.x ) - ( other.pos.x - other.size.x ) ),
				fabsf( ( movedXYBody.pos.y + movedXYBody.size.y ) - ( other.pos.y - other.size.y ) )
			};
			Donya::Vector2 minusPenetration
			{
				fabsf( ( movedXYBody.pos.x - movedXYBody.size.x ) - ( other.pos.x + other.size.x ) ),
				fabsf( ( movedXYBody.pos.y - movedXYBody.size.y ) - ( other.pos.y + other.size.y ) )
			};
			penetration.x
				= ( moveSign.x < 0.0f ) ? minusPenetration.x
				: ( moveSign.x > 0.0f ) ? plusPenetration.x
				: 0.0f;
			penetration.y
				= ( moveSign.y < 0.0f ) ? minusPenetration.y
				: ( moveSign.y > 0.0f ) ? plusPenetration.y
				: 0.0f;

			constexpr float ERROR_MARGIN = 0.0001f; // Prevent the two edges onto same place(the collision detective allows same(equal) value).

			Donya::Vector2 resolver
			{
				( penetration.x + ERROR_MARGIN ) * -moveSign.x,
				( penetration.y + ERROR_MARGIN ) * -moveSign.y
			};

			lastResolver = resolver;
			lastHitOther = other;

			// Repulse to the more little(but greater than zero) axis side of penetration.
			if ( ( penetration.y < penetration.x && !ZeroEqual( penetration.y ) ) || ZeroEqual( penetration.x ) )
			// if ( penetration.y < penetration.x || ZeroEqual( penetration.x ) )
			{
				Donya::Vector2 influence{};
				enum Dir { Up = 1, Down = -1 };
				int  verticalSign =  Donya::SignBit( moveSign.y ); // Represent a direction that was collided to other.
				if ( verticalSign == Down )
				{
					influence = HasInfluence( other );
				}

				movedXYBody.pos.y	+= resolver.y;
				moveSign.y			=  scast<float>( Donya::SignBit( resolver.y ) );
				lastResolver.x		=  0.0f;

				if ( !influence.IsZero() )
				{
					movedXYBody.pos += influence;
					const Donya::Int2 signs
					{
						Donya::SignBit( influence.x ),
						Donya::SignBit( influence.y )
					};
					if ( signs.x != 0 ) { moveSign.x = scast<float>( signs.x ); }
					if ( signs.y != 0 ) { moveSign.y = scast<float>( signs.y ); }
				}
			}
			// else // if ( !ZeroEqual( penetration.x ) ) is same as above this : " || ZeroEqual( penetration.x ) "
			else if ( !ZeroEqual( penetration.x ) )
			{
				movedXYBody.pos.x	+= resolver.x;
				moveSign.x			=  scast<float>( Donya::SignBit( resolver.x ) );
				lastResolver.y		=  0.0f;
			}
		}

		pos.x =  movedXYBody.pos.x;
		pos.y =  movedXYBody.pos.y;
		pos.z += velocity.z;

		enum Dir { Up = 1, Down = -1 };
		int  verticalSign = Donya::SignBit( -lastResolver.y ); // Represent the last direction that was collided to other.
		if ( verticalSign == Down )
		{
			Landing();

			aboveSlipGround = HasSlipAttribute( lastHitOther );
		}

		if ( Donya::SignBit( lastResolver.x ) != 0 )
		{
			velocity.x = 0.0f;
		}
		if ( Donya::SignBit( lastResolver.y ) != 0 )
		{
			velocity.y = 0.0f;
		}

		// Check the foot for landing.
		if ( !ZeroEqual( velocity.y ) )
		{
			constexpr float	slightOffset = 0.0001f; // This value used for the check to "was resolved in that direction?", so should be greater than zero and smaller than large.
			const     int	sign = Donya::SignBit( velocity.y );
			movedXYBody.pos.y += slightOffset * sign;

			other = CalcCollidingBox( movedXYBody, previousXYBody );
			if ( other != BoxEx::Nil() )
			{
				if ( sign == Down )
				{
					Landing();
					aboveSlipGround = HasSlipAttribute( other );
				}
				else
				{
					velocity.y = 0.0f;
				}
			}
		}
	};

	/// <summary>
	/// The Version retrieved from a gimmick.
	/// </summary>
	auto Version_3 = [&]()
	{
		auto CalcCollidingBox = [&]( const BoxEx &myself, const BoxEx &previousMyself )->BoxEx
		{
			for ( const auto &it : terrains )
			{
				// if ( it.mass < myself.mass ) { continue; }
				if ( it == previousMyself ) { continue; }
				// else

				if ( Donya::Box::IsHitBox( it, myself ) )
				{
					return it;
				}
			}

			return BoxEx::Nil();
		};

		const AABBEx actualBody		= GetHitBox();
		const BoxEx  previousXYBody	= actualBody.Get2D();

		Donya::Vector2 xyVelocity{ velocity.x, velocity.y };
		Donya::Vector2 moveSign // The moving direction of myself. Take a value of +1.0f or -1.0f.
		{
			scast<float>( Donya::SignBit( xyVelocity.x ) ),
			scast<float>( Donya::SignBit( xyVelocity.y ) )
		};

		BoxEx movedXYBody = previousXYBody;
		movedXYBody.pos  += xyVelocity;

		BoxEx other{};

		constexpr unsigned int MAX_LOOP_COUNT = 1000U;
		unsigned int loopCount{};
		while ( ++loopCount < MAX_LOOP_COUNT )
		{
			other = CalcCollidingBox( movedXYBody, previousXYBody );
			if ( other == BoxEx::Nil() ) { break; } // Does not detected a collision.
			// else

			// if ( other.mass < movedXYBody.mass ) { continue; }
			// else

			if ( ZeroEqual( moveSign.x ) && !ZeroEqual( other.velocity.x ) )
			{
				// The myself's moving direction is considered the inverse of other's moving direction.
				moveSign.x = scast<float>( Donya::SignBit( -other.velocity.x ) );
			}
			if ( ZeroEqual( moveSign.y ) && !ZeroEqual( other.velocity.y ) )
			{
				// The myself's moving direction is considered the inverse of other's moving direction.
				moveSign.y = scast<float>( Donya::SignBit( -other.velocity.y ) );
			}

			if ( moveSign.IsZero() ) { continue; } // Each other does not move, so collide is no possible.
			// else

			Donya::Vector2 penetration{}; // Store absolute value.
			Donya::Vector2 plusPenetration
			{
				fabsf( ( movedXYBody.pos.x + movedXYBody.size.x ) - ( other.pos.x - other.size.x ) ),
				fabsf( ( movedXYBody.pos.y + movedXYBody.size.y ) - ( other.pos.y - other.size.y ) )
			};
			Donya::Vector2 minusPenetration
			{
				fabsf( ( movedXYBody.pos.x - movedXYBody.size.x ) - ( other.pos.x + other.size.x ) ),
				fabsf( ( movedXYBody.pos.y - movedXYBody.size.y ) - ( other.pos.y + other.size.y ) )
			};
			penetration.x
				= ( moveSign.x < 0.0f ) ? minusPenetration.x
				: ( moveSign.x > 0.0f ) ? plusPenetration.x
				: 0.0f;
			penetration.y
				= ( moveSign.y < 0.0f ) ? minusPenetration.y
				: ( moveSign.y > 0.0f ) ? plusPenetration.y
				: 0.0f;

			constexpr float ERROR_MARGIN = 0.0001f; // Prevent the two edges onto same place(the collision detective allows same(equal) value).

			Donya::Vector2 resolver
			{
				( penetration.x + ERROR_MARGIN ) * -moveSign.x,
				( penetration.y + ERROR_MARGIN ) * -moveSign.y
			};

			// Repulse to the more little(but greater than zero) axis side of penetration.
			if ( penetration.y < penetration.x || ZeroEqual( penetration.x ) )
			{
				enum Dir { Up = 1, Down = -1 };
				int  verticalSign =  Donya::SignBit( velocity.y );
				if ( verticalSign == Down )
				{
					Landing();
				}	

				movedXYBody.pos.y += resolver.y;
				velocity.y = 0.0f;
				moveSign.y = scast<float>( Donya::SignBit( resolver.y ) );
			}
			else // if ( !ZeroEqual( penetration.x ) ) is same as above this : " || ZeroEqual( penetration.x ) "
			{
				movedXYBody.pos.x += resolver.x;
				velocity.x = 0.0f;
				moveSign.x = scast<float>( Donya::SignBit( resolver.x ) );
			}
		}

		pos.x =  movedXYBody.pos.x;
		pos.y =  movedXYBody.pos.y;
		pos.z += velocity.z;
	};

	/// <summary>
	/// The collision detective and resolving a penetrate process.
	/// </summary>
	auto Version_2 = [&]()
	{
		/// <summary>
		/// The "x Axis" is specify moving axis. please only set to { 1, 0 } or { 0, 1 }. This function  to be able to handle any axis.
		/// </summary>
		auto MoveSpecifiedAxis = [&]( Donya::Vector2 xyNAxis, float moveSpeed )->bool
		{
			bool corrected = false;

			// Only either X or Y is valid.
			const Donya::Vector2 xyVelocity = xyNAxis * moveSpeed;
			pos.x += xyVelocity.x;
			pos.y += xyVelocity.y;

			const auto actualBody = PlayerParam::Get().Data().hitBoxPhysic;

			// A moveing direction of myself. Take a value of +1.0f or -1.0f.
			float moveSign = scast<float>( Donya::SignBit( xyVelocity.x ) + Donya::SignBit( xyVelocity.y ) );

			// The player's hit box of stage is circle, but doing with rectangle for easily correction.
			BoxEx xyBody{};
			{
				xyBody.pos.x	= GetPosition().x;
				xyBody.pos.y	= GetPosition().y;
				xyBody.size.x	= actualBody.size.x;// * xyNAxis.x; // Only either X or Y is valid.
				xyBody.size.y	= actualBody.size.y;// * xyNAxis.y; // Only either X or Y is valid.
				xyBody.exist	= true;
				xyBody.mass		= actualBody.mass;
			}
			Donya::Vector2 xyBodyCenter = xyBody.pos;
			Donya::Vector2 bodySize{ xyBody.size.x * xyNAxis.x, xyBody.size.y * xyNAxis.y }; // Only either X or Y is valid.
			const float bodyWidth = bodySize.Length(); // Extract valid member by Length().

			for ( const auto &wall : terrains )
			{
				if ( !Donya::Box::IsHitBox( xyBody, wall ) ) { continue; }
				// else

				Donya::Vector2 xyWallCenter = wall.pos;
				Donya::Vector2 wallSize{ wall.size.x * xyNAxis.x, wall.size.y * xyNAxis.y }; // Only either X or Y is valid.
				Donya::Vector2 wallVelocity{ wall.velocity.x * xyNAxis.x, wall.velocity.y * xyNAxis.y }; // Only either X or Y is valid.
				float wallWidth = wallSize.Length();		// Extract valid member by Length().
				float wallSpeed = wallVelocity.Length();	// Extract valid member by Length().

				if ( ZeroEqual( moveSign ) )
				{
					// Usually I decide a repulse direction by moving direction("moveSign") of myself.
					// But I can not decide a repulse direction when the myself does not moving.
					// So use the other("wall")'s moving direction.

					// If the myself is heavier than wall, so the myself does not repulse.
					if ( wall.mass < xyBody.mass ) { continue; }
					// else
					
					// Each other does not move, it is not colliding movement of now axis.
					if ( ZeroEqual( wallSpeed ) ) { continue; }
					// else

					moveSign  = scast<float>( Donya::SignBit( wallVelocity.x ) + Donya::SignBit( wallVelocity.y ) );
					moveSign *= -1.0f;		// This "moveSign" represent the moving direction of myself, so I should reverse.
					moveSpeed = wallSpeed * moveSign;
				}

				// Calculate colliding length.
				// First, calculate body's edge of moving side.
				// Then, calculate wall's edge of inverse moving side.
				// After that, calculate colliding length from two edges.
				// Finally, correct the position to inverse moving side only that length.

				Donya::Vector2 bodyEdge	= xyBodyCenter + ( xyNAxis * bodyWidth *  moveSign );
				Donya::Vector2 wallEdge	= xyWallCenter + ( xyNAxis * wallWidth * -moveSign );
				Donya::Vector2 diff		= bodyEdge - wallEdge;
				Donya::Vector2 axisDiff{ diff.x * xyNAxis.x, diff.y * xyNAxis.y };
				float collidingLength	= axisDiff.Length(); // Extract valid member by Length().

				Donya::Vector2 xyCorrection = xyNAxis * ( collidingLength * -moveSign );
				pos.x += xyCorrection.x;
				pos.y += xyCorrection.y;

				// Prevent the two edges onto same place(the collision detective allows same(equal) value).
				pos.x += 0.0001f * scast<float>( Donya::SignBit( xyCorrection.x ) );
				pos.y += 0.0001f * scast<float>( Donya::SignBit( xyCorrection.y ) );

				// We must apply the repulsed position to hit-box for next collision.
				xyBody.pos.x = GetPosition().x;
				xyBody.pos.y = GetPosition().y;

				corrected = true;
			}

			return corrected;
		};

		// Move to X-axis with collision.
		MoveSpecifiedAxis( Donya::Vector2{ 1.0f, 0.0f }, velocity.x );
		// Move to Y-axis with collision.
		bool wasCorrected = MoveSpecifiedAxis( Donya::Vector2{ 0.0f, 1.0f }, velocity.y );
		if ( wasCorrected )
		{
			enum Dir { Up = 1, Down = -1 };
			int  moveSign =  Donya::SignBit( velocity.y );
			if ( moveSign == Up )
			{
				velocity.y = 0.0f;
			}
			else
			if ( moveSign == Down )
			{
				Landing();
			}
		}
		// Move to Z-axis only.
		pos.z += velocity.z;
	};
	/// <summary>
	/// The collision detective and resolving a penetrate process.
	/// </summary>
	auto Version_1 = [&]()
	{
		/// <summary>
		/// The "x Axis" is specify moving axis. please only set to { 1, 0 } or { 0, 1 }. This function  to be able to handle any axis.
		/// </summary>
		auto MoveSpecifiedAxis = [&]( Donya::Vector2 xyNAxis, float moveSpeed )->bool
		{
			bool corrected = false;

			// Only either X or Y is valid.
			const Donya::Vector2 xyVelocity = xyNAxis * moveSpeed;
			pos.x += xyVelocity.x;
			pos.y += xyVelocity.y;

			const auto actualBody = PlayerParam::Get().Data().hitBoxPhysic;

			// Take a value of +1 or -1.
			float moveSign = scast<float>( Donya::SignBit( xyVelocity.x ) + Donya::SignBit( xyVelocity.y ) );

			/*
			// This process require the current move velocity(because using to calculate the repulse direction).
			if ( ZeroEqual( moveSign ) ) { return corrected; }
			// else
			*/

			// The player's hit box of stage is circle, but doing with rectangle for easily correction.
			Donya::Box xyBody{};
			{
				xyBody.pos.x	= GetPosition().x;
				xyBody.pos.y	= GetPosition().y;
				xyBody.size.x	= actualBody.size.x * xyNAxis.x; // Only either X or Y is valid.
				xyBody.size.y	= actualBody.size.y * xyNAxis.y; // Only either X or Y is valid.
				xyBody.exist	= true;
			}
			Donya::Vector2 xyBodyCenter = xyBody.pos;
			const float bodyWidth = xyBody.size.Length(); // Extract valid member by Length().

			for ( const auto &wall : terrains )
			{
				if ( !Donya::Box::IsHitBox( xyBody, wall ) ) { continue; }
				// else

				Donya::Vector2 xyWallCenter = wall.pos;
				Donya::Vector2 wallSize{ wall.size.x * xyNAxis.x, wall.size.y * xyNAxis.y }; // Only either X or Y is valid.
				float wallWidth = wallSize.Length(); // Extract valid member by Length().

				if ( ZeroEqual( moveSign ) )
				{
					// Correct to nearest side.

					Donya::Vector2 diff = wall.pos - xyBody.pos;
					float diffSignX = diff.x * xyNAxis.x; // Only either X or Y is valid.
					float diffSignY = diff.y * xyNAxis.y; // Only either X or Y is valid.

					if ( !ZeroEqual( diffSignX ) ) { moveSign = scast<float>( Donya::SignBit( diffSignX ) ); }
					else
					if ( !ZeroEqual( diffSignY ) ) { moveSign = scast<float>( Donya::SignBit( diffSignY ) ); }
					else { continue; }

					moveSpeed = EPSILON; // For add the error used for preventing the two edges onto the same place(the collision detective allows the same(equal) value).
				}

				// Calculate colliding length.
				// First, calculate body's edge of moving side.
				// Then, calculate wall's edge of inverse moving side.
				// After that, calculate colliding length from two edges.
				// Finally, correct the position to inverse moving side only that length.

				Donya::Vector2 bodyEdge	= xyBodyCenter + ( xyNAxis * bodyWidth *  moveSign );
				Donya::Vector2 wallEdge	= xyWallCenter + ( xyNAxis * wallWidth * -moveSign );
				Donya::Vector2 diff		= bodyEdge - wallEdge;
				Donya::Vector2 axisDiff{ diff.x * xyNAxis.x, diff.y * xyNAxis.y };
				float collidingLength = axisDiff.Length();
				collidingLength += fabsf( moveSpeed ) * 0.1f; // Prevent the two edges onto same place(the collision detective allows same(equal) value).

				Donya::Vector2 xyCorrection
				{
					xyNAxis.x * ( collidingLength * -moveSign ),
					xyNAxis.y * ( collidingLength * -moveSign )
				};
				pos.x += xyCorrection.x;
				pos.y += xyCorrection.y;

				// We must apply the repulsed position to hit-box for next collision.
				xyBody.pos.x = GetPosition().x;
				xyBody.pos.y = GetPosition().y;

				corrected = true;
			}

			return corrected;
		};

		// Move to X-axis with collision.
		MoveSpecifiedAxis( Donya::Vector2{ 1.0f, 0.0f }, velocity.x );
		// Move to Y-axis with collision.
		bool wasCorrected = MoveSpecifiedAxis( Donya::Vector2{ 0.0f, 1.0f }, velocity.y );
		if ( wasCorrected )
		{
			enum Dir { Up = 1, Down = -1 };
			int  moveSign =  Donya::SignBit( velocity.y );
			if ( moveSign == Up )
			{
				velocity.y = 0.0f;
			}
			else
			if ( moveSign == Down )
			{
				Landing();
			}
		}
		// Move to Z-axis only.
		pos.z += velocity.z;
	};

	Version_4();
}

#if DEBUG_MODE
#include "Donya/GeometricPrimitive.h"
#endif // DEBUG_MODE
void Player::Draw( const Donya::Vector4x4 &matViewProjection, const Donya::Vector4 &lightDirection, const Donya::Vector4 &lightColor ) const
{
	Donya::Vector4x4 S = Donya::Vector4x4::MakeScaling( PlayerParam::Get().Data().drawScale );
	Donya::Vector4x4 R = Donya::Vector4x4::Identity();
	if ( !seeRight )
	{
		Donya::Quaternion halfRot = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( 180.0f ) );
		R = halfRot.RequireRotationMatrix();
	}
	Donya::Vector4x4 T = Donya::Vector4x4::MakeTranslation( GetPosition() );
	
	Donya::Vector4x4 W = S * R * T;

	drawModel.Render
	(
		nullptr,
		/* useDefaultShading	= */ true,
		/* isEnableFill			= */ true,
		W * matViewProjection, W,
		lightDirection, Donya::Vector4{ 1.0f, 1.0f, 1.0f, drawAlpha }
	);

	DrawOfOpenDoor(matViewProjection);

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static Donya::Geometric::Cube cube = Donya::Geometric::CreateCube();
		
		const auto wsBody = GetHitBox();
		T = Donya::Vector4x4::MakeTranslation( wsBody.pos );
		S = Donya::Vector4x4::MakeScaling( wsBody.size );
		W = S * T;

		cube.Render
		(
			nullptr,
			true, true,
			W * matViewProjection, W,
			lightDirection, Donya::Vector4{ 0.6f, 1.0f, 0.6f, 0.5f }
		);
	}
#endif // DEBUG_MODE
}

Donya::Vector3 Player::GetPosition() const
{
	return pos;
}
AABBEx Player::GetHitBox() const
{
	AABBEx wsAABB	=  PlayerParam::Get().Data().hitBoxPhysic;
	wsAABB.pos		+= GetPosition();
	wsAABB.velocity	=  velocity;
	wsAABB.exist	=  ( status == State::Dead ) ? false : true;
	return wsAABB;
}

bool Player::IsCatchKey() const
{
	return ( collideKeyCounter == 1 ) ? true : false;
}

bool Player::IsDead() const
{
	return ( status == State::Dead && drawAlpha <= 0.0f ) ? true : false;
}

void Player::LoadModel()
{
	if ( wasLoaded ) { return; }
	// else

	Donya::Loader loader{};
	bool  succeeded = loader.Load( GetModelPath( ModelAttribute::Player ), nullptr );
	if ( !succeeded )
	{
		_ASSERT_EXPR( 0, L"Failed : Load the Player's model." );
		return;
	}

	succeeded = Donya::StaticMesh::Create( loader, drawModel );
	if ( !succeeded )
	{
		_ASSERT_EXPR( 0, L"Failed : Create the Player's model." );
		return;
	}

	wasLoaded = true;
}

void Player::NormalUpdate( float elapsedTime, Input controller )
{
	Move( elapsedTime, controller );

	Fall( elapsedTime, controller );
	JumpIfUsed( elapsedTime, controller );

	UpdateOpenDoor(elapsedTime);
}
void Player::DeadUpdate( float elapsedTime, Input controller )
{
	constexpr float TO_OPAQUE_SPEED	= 5.0f;
	drawAlpha -= TO_OPAQUE_SPEED * elapsedTime;
}

void Player::Move( float elapsedTime, Input controller )
{
	const float moveSpeed = PlayerParam::Get().Data().moveSpeed * elapsedTime;

	auto AssignMoveSpeed = [&]()
	{
		if ( controller.moveVelocity.x > 0 )
		{
			seeRight = true;
		}
		else
		if ( controller.moveVelocity.x < 0 )
		{
			seeRight = false;
		}
		velocity.x = controller.moveVelocity.x * moveSpeed;
	};

	if ( aboveSlipGround )
	{
		// This slipping behavior referred to the Megaman.

		const Donya::Int2 moveSign
		{
			Donya::SignBit( velocity.x ),
			Donya::SignBit( velocity.y )
		};
		const Donya::Int2 inputSign
		{
			Donya::SignBit( controller.moveVelocity.x ),
			Donya::SignBit( controller.moveVelocity.y )
		};

		auto Brake = [&]()
		{
			const float brakeSpeed = PlayerParam::Get().Data().brakeSpeed * elapsedTime;

			if ( fabsf( velocity.x ) <= brakeSpeed )
			{
				velocity.x = 0.0f;
			}
			else
			{
				velocity.x -= brakeSpeed * moveSign.x;
			}
		};

		if ( !inputSign.x )
		{
			if ( moveSign.x != 0 )
			{
				Brake();
			}

			return;
		}
		// else

		// Player can move as usual if the input direction is same as the slipping direction.
		if ( inputSign.x == moveSign.x || !moveSign.x )
		{
			AssignMoveSpeed();
		}
		else
		{
			Brake();
		}

		return;
	}
	// else

	AssignMoveSpeed();
}

void Player::Fall( float elapsedTime, Input controller )
{
	velocity.y -= PlayerParam::Get().Data().gravity * elapsedTime;
	velocity.y =  std::max( -PlayerParam::Get().Data().maxFallSpeed, velocity.y );
}
void Player::JumpIfUsed( float elapsedTime, Input controller )
{
	if ( !controller.useJump || remainJumpCount <= 0 ) { return; }
	// else

	remainJumpCount--;
	velocity.y = PlayerParam::Get().Data().jumpPower;
	
	Donya::Sound::Play( Music::Jump );
}

void Player::Landing()
{
	remainJumpCount = PlayerParam::Get().Data().maxJumpCount;
	velocity.y = 0.0f;
}

void Player::KillMe()
{
	status			= State::Dead;
	velocity		= 0.0f;
	remainJumpCount	= 0;
	drawAlpha		= 1.0f;
}

void Player::UpdateOpenDoor(float elapsedTime)
{
	if (IsCatchKey())
	{
		isCatchKey = true;
	}

	if (isCatchKey)
	{
		viewOpenCount += elapsedTime;
		if (viewOpenCount >= 4.0f)
		{
			isCatchKey = false;
			viewOpenCount = 0;
		}
	}
}

void Player::DrawOfOpenDoor(const Donya::Vector4x4& matViewProjection)const
{
	if (!isCatchKey)return;

	auto ConvertionScreenToWorld = [&](DirectX::XMFLOAT3 worldPos, Donya::Vector4x4 _ViewProj)
	{
		using namespace DirectX;

		XMVECTOR worldPos_v = XMLoadFloat3(&worldPos);

		float w = Common::HalfScreenWidthF();
		float h = Common::HalfScreenHeightF();

		XMMATRIX ViewProjection = {
			_ViewProj.m[0][0],_ViewProj.m[0][1],_ViewProj.m[0][2],_ViewProj.m[0][3],
			_ViewProj.m[1][0],_ViewProj.m[1][1],_ViewProj.m[1][2],_ViewProj.m[1][3],
			_ViewProj.m[2][0],_ViewProj.m[2][1],_ViewProj.m[2][2],_ViewProj.m[2][3],
			_ViewProj.m[3][0],_ViewProj.m[3][1],_ViewProj.m[3][2],_ViewProj.m[3][3],
		};

		XMMATRIX Vp = {
			w, 0, 0, 0,
			0, -h, 0, 0,
			0, 0, 1, 0,
			w, h, 0, 1,
		};

		worldPos_v = XMVector3Transform(worldPos_v, ViewProjection);

		XMFLOAT3 tmp;
		XMStoreFloat3(&tmp, worldPos_v);

		XMVECTOR viewVec = XMVectorSet(tmp.x / tmp.z, tmp.y / tmp.z, 1.0f, 1.0f);
		viewVec = XMVector3Transform(viewVec, Vp);
		XMFLOAT2 ans;
		XMStoreFloat2(&ans, viewVec);
		return ans;
	};



	DirectX::XMFLOAT3 playerPos{ pos };
	auto pos = ConvertionScreenToWorld(playerPos, matViewProjection);

	Donya::Sprite::SetDrawDepth(0.0f);

	Donya::Sprite::DrawPartExt(idOpenDoor, pos.x + 50, pos.y - 200, 0.0f, 0.0f, 640.0f,512.0f, 0.3f, 0.3f);

}

#if USE_IMGUI

void Player::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"プレイヤー・今のデータ" ) )
		{
			ImGui::Text( u8"キーに触れている時間[%d]", collideKeyCounter	);
			ImGui::DragInt( u8"のこりジャンプ回数",	&remainJumpCount	);
			ImGui::DragFloat3( u8"ワールド座標",		&pos.x				);
			ImGui::DragFloat3( u8"移動速度",			&velocity.x			);
			ImGui::Checkbox( u8"氷床の上にいる？",	&aboveSlipGround	);

			ImGui::TreePop();
		}

		ImGui::End();
	}
}

#endif // USE_IMGUI
