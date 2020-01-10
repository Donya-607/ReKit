#include "Player.h"

#include <array>
#include <algorithm>		// Use std::min(), max().
#include <vector>

#include "Donya/Constant.h"	// Use DEBUG_MODE, scast macros.
#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#endif // DEBUG_MODE

#include "FilePath.h"
#include "Music.h"

#undef max
#undef min

class PlayerParam final : public Donya::Singleton<PlayerParam>
{
	friend Donya::Singleton<PlayerParam>;
public:
	struct Member
	{
		int		maxJumpCount{};	// 0 is can not jump, 1 ~ is can jump.
		float	moveSpeed{};	// Use for a horizontal move. It will influenced by "elapsedTime".
		float	jumpPower{};	// Use for a just moment of using a jump.
		float	maxFallSpeed{};	// Use for a limit of falling speed.
		float	gravity{};		// Always use to fall. It will influenced by "elapsedTime".

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
				// CEREAL_NVP( x )
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
			if ( ImGui::TreeNode( u8"�v���C���[�E�����f�[�^" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"���S�ʒu�̃I�t�Z�b�g" ).c_str(), &pHitBox->pos.x  );
					ImGui::DragFloat2( ( prefix + u8"�T�C�Y�i�������w��j" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"����"                ).c_str(), &pHitBox->mass   );
					ImGui::Checkbox  ( ( prefix + u8"�����蔻��͗L����"   ).c_str(), &pHitBox->exist  );
				};

				ImGui::DragInt( u8"�ő�W�����v��",		&m.maxJumpCount,	1.0f, 0		);
				ImGui::DragFloat( u8"���ړ����x",		&m.moveSpeed,		1.0f, 0.0f	);
				ImGui::DragFloat( u8"�W�����v����",		&m.jumpPower,		1.0f, 0.0f	);
				ImGui::DragFloat( u8"�ő嗎�����x",		&m.maxFallSpeed,	1.0f, 0.0f	);
				ImGui::DragFloat( u8"�d��",				&m.gravity,			1.0f, 0.0f	);

				AdjustAABB( u8"�����蔻��F�u�r�n�`", &m.hitBoxPhysic );

				if ( ImGui::TreeNode( u8"�t�@�C��" ) )
				{
					static bool isBinary = true;
					if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true; }
					if ( ImGui::RadioButton( "JSON", !isBinary ) ) { isBinary = false; }
					std::string loadStr{ "�ǂݍ��� " };
					loadStr += ( isBinary ) ? "Binary" : "JSON";

					if ( ImGui::Button( u8"�ۑ�" ) )
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

CEREAL_CLASS_VERSION( PlayerParam::Member, 0 )

Player::Player() :
	remainJumpCount( 1 ),
	pos(), velocity(),
	drawModel( Donya::Geometric::CreateSphere() ), cbuffer(), VSDemo(), PSDemo()
{}
Player::~Player() = default;

void Player::Init( const Donya::Vector3 &wsInitPos )
{
	PlayerParam::Get().Init();

	CreateRenderingObjects();

	pos = wsInitPos;
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

#if DEBUG_MODE
	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Keyboard::Trigger( 'J' ) )
		{
			remainJumpCount = 65535;
		}
	}
#endif // DEBUG_MODE

	Move( elapsedTime, controller );

	Fall( elapsedTime, controller );
	JumpIfUsed( elapsedTime, controller );
}

void Player::PhysicUpdate( const std::vector<BoxEx> &terrains )
{
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

	Version_3();
}

void Player::Draw( const Donya::Vector4x4 &matViewProjection, const Donya::Vector4 &lightDirection, const Donya::Vector4 &lightColor ) const
{
	Donya::Vector4x4 T = Donya::Vector4x4::MakeTranslation( GetPosition() );
	Donya::Vector4x4 S = Donya::Vector4x4::MakeScaling( PlayerParam::Get().Data().hitBoxPhysic.size * 2.0f/* Half size to Whole size */ );
	Donya::Vector4x4 W = S * T;

	cbuffer.data.world					= W.XMFloat();
	cbuffer.data.worldViewProjection	= ( W * matViewProjection ).XMFloat();
	cbuffer.data.lightDirection			= lightDirection;
	cbuffer.data.lightColor				= lightColor;
	cbuffer.data.materialColor			= Donya::Vector4{ 1.0f, 0.6f, 0.8f, 1.0f };

	cbuffer.Activate( 0, /* setVS = */ true, /* setPS = */ true );
	VSDemo.Activate();
	PSDemo.Activate();

	drawModel.Render( nullptr, /* useDefaultShading = */ false );
	
	PSDemo.Deactivate();
	VSDemo.Deactivate();
	cbuffer.Deactivate();
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
	return wsAABB;
}

void Player::CreateRenderingObjects()
{
	cbuffer.Create();

	constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputElements
	{
		D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	// The function requires argument is std::vector, so convert.
	const std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementsVector{ inputElements.begin(), inputElements.end() };
	VSDemo.CreateByCSO( GetShaderPath( ShaderAttribute::Demo, /* wantVS */ true  ), inputElementsVector );
	PSDemo.CreateByCSO( GetShaderPath( ShaderAttribute::Demo, /* wantVS */ false ) );
}

void Player::Move( float elapsedTime, Input controller )
{
	const float moveSpeed = PlayerParam::Get().Data().moveSpeed;
	velocity.x = controller.moveVelocity.x * moveSpeed * elapsedTime;
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

#if USE_IMGUI

void Player::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"�v���C���[�E���̃f�[�^" ) )
		{
			ImGui::DragInt( u8"�̂���W�����v��",	&remainJumpCount	);
			ImGui::DragFloat3( u8"���[���h���W",		&pos.x				);
			ImGui::DragFloat3( u8"�ړ����x",			&velocity.x			);

			ImGui::TreePop();
		}

		ImGui::End();
	}
}

#endif // USE_IMGUI
