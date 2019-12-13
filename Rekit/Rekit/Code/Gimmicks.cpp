#include "Gimmicks.h"

#include <algorithm>		// Use std::remove_if.

#include "Donya/GeometricPrimitive.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.
#include "Donya/Sound.h"

#include "Common.h"
#include "FilePath.h"
#include "Music.h"

#undef max
#undef min

#pragma region FragileBlock

struct ParamHeavyBlock final : public Donya::Singleton<ParamHeavyBlock>
{
	friend Donya::Singleton<ParamHeavyBlock>;
public:
	struct Member
	{
		float	gravity{};
		float	maxFallSpeed{};
		AABBEx	hitBox{};	// Hit-Box of using to the collision to the stage.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( hitBox )
			);
			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( gravity ),
					CEREAL_NVP( maxFallSpeed )
				);
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "FragileBlock";
	Member m;
private:
	ParamHeavyBlock() : m() {}
public:
	~ParamHeavyBlock() = default;
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
			if ( ImGui::TreeNode( u8"ブロック・調整データ" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"質量" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox  ( ( prefix + u8"当たり判定は有効か" ).c_str(), &pHitBox->exist );
				};

				ImGui::DragFloat( u8"重力加速度", &m.gravity );
				ImGui::DragFloat( u8"最大落下速度", &m.maxFallSpeed );

				AdjustAABB( u8"当たり判定", &m.hitBox );

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
CEREAL_CLASS_VERSION( ParamHeavyBlock::Member, 1 )

FragileBlock::FragileBlock() :
	pos(), velocity(),
	wasBroken( false )
{}
FragileBlock::~FragileBlock() = default;

void FragileBlock::Init( const Donya::Vector3 &wsPos )
{
	pos = wsPos;
}
void FragileBlock::Uninit()
{

}

void FragileBlock::Update( float elapsedTime )
{
	Fall( elapsedTime );
}
void FragileBlock::PhysicUpdate( const std::vector<BoxEx> &terrains )
{
	AssignVelocity( terrains );
}

void FragileBlock::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static Donya::Geometric::Cube cube = Donya::Geometric::CreateCube();

		constexpr Donya::Vector4 color{ 0.3f, 0.3f, 0.3f, 0.8f };
		cube.Render
		(
			nullptr,
			/* useDefaultShading	= */ true,
			/* isEnableFill			= */ true,
			WVP, W, lightDir, color
		);
	}
#endif // DEBUG_MODE
}

bool FragileBlock::ShouldRemove() const
{
	return wasBroken;
}

Donya::Vector3 FragileBlock::GetPosition() const
{
	return pos;
}
AABBEx FragileBlock::GetHitBox() const
{
	AABBEx base = ParamHeavyBlock::Get().Data().hitBox;
	base.pos		+= pos;
	base.velocity	=  velocity;
	return base;
}

Donya::Vector4x4 FragileBlock::GetWorldMatrix( bool useDrawing ) const
{
	auto wsBox = GetHitBox();
	if ( useDrawing )
	{
		// The AABB size is half, but drawing object's size is whole.
		wsBox.size *= 2.0f;
	}

	Donya::Vector4x4 mat{};
	mat._11 = wsBox.size.x;
	mat._22 = wsBox.size.y;
	mat._33 = wsBox.size.z;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

void FragileBlock::Fall( float elapsedTime )
{
	const auto DATA = ParamHeavyBlock::Get().Data();
	velocity.y -= DATA.gravity * elapsedTime;
	velocity.y =  std::max( DATA.maxFallSpeed, velocity.y );
}

void FragileBlock::AssignVelocity( const std::vector<BoxEx> &terrains )
{
	/// <summary>
	/// The "x Axis" is specify moving axis. please only set to { 1, 0 } or { 0, 1 }. This function  to be able to handle any axis.
	/// </summary>
	auto MoveSpecifiedAxis = [&]( Donya::Vector2 xyNAxis, float moveSpeed, const AABBEx &baseHitBox )->bool
	{
		bool corrected = false;

		BoxEx previousXYBody{}; // Use for check "a wall is myself?".
		{
			previousXYBody.pos.x		= baseHitBox.pos.x;
			previousXYBody.pos.y		= baseHitBox.pos.y;
			previousXYBody.size.x		= baseHitBox.size.x;
			previousXYBody.size.y		= baseHitBox.size.y;
			previousXYBody.velocity.x	= baseHitBox.velocity.x;
			previousXYBody.velocity.y	= baseHitBox.velocity.y;
			previousXYBody.exist		= true;
			previousXYBody.mass			= baseHitBox.mass;
		}

		// Only either X or Y is valid.
		const Donya::Vector2 xyVelocity = xyNAxis * moveSpeed;
		pos.x += xyVelocity.x;
		pos.y += xyVelocity.y;

		// The player's hit box of stage is circle, but doing with rectangle for easily correction.
		BoxEx xyBody{};
		{
			xyBody.pos.x	= GetPosition().x;
			xyBody.pos.y	= GetPosition().y;
			xyBody.size.x	= baseHitBox.size.x;
			xyBody.size.y	= baseHitBox.size.y;
			xyBody.exist	= true;
			xyBody.mass		= baseHitBox.mass;
		}
		Donya::Vector2 xyBodyCenter = xyBody.pos;
		Donya::Vector2 bodySize{ xyBody.size.x * xyNAxis.x, xyBody.size.y * xyNAxis.y }; // Only either X or Y is valid.
		const float bodyWidth = bodySize.Length(); // Extract valid member by Length().

		// The moving direction of myself. Take a value of +1 or -1.
		float moveSign{};
		bool  pushedNow = false;
		Donya::Vector2 pushedDirection{}; // Store a vector of [wall->myself].
		const size_t wallCount = terrains.size();
		for ( size_t i = 0; i < wallCount; ++i )
		{
			const BoxEx &wall = terrains[i];

			if ( previousXYBody == wall ) { continue; } // The terrains contain also myself.
			if ( !Donya::Box::IsHitBox( xyBody, wall ) ) { continue; }
			// else

			moveSign = scast<float>( Donya::SignBit( xyVelocity.x ) + Donya::SignBit( xyVelocity.y ) );

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
				if ( ZeroEqual( wallSpeed )  )
				{
					// Only store a direction of [wall->myself].
					pushedDirection = ( xyBody.pos - wall.pos ).Normalized();
					continue;
				}
				// else

				moveSign  = scast<float>( Donya::SignBit( wallVelocity.x ) + Donya::SignBit( wallVelocity.y ) );
				moveSign *= -1.0f; // This "moveSign" represent the moving direction of myself, so I should reverse.
				moveSpeed = wallSpeed * -moveSign;
			}

			// Calculate the myself will complessed?
			if ( xyBody.mass <= wall.mass )
			{
				if ( pushedNow )
				{
					Donya::Vector2 currentPushedDir = ( xyBody.pos - wall.pos ).Normalized();

					float angle = Donya::Vector2::Dot( pushedDirection, currentPushedDir );
					if (  angle < 0.0f ) // If these direction is against.
					{

						Donya::Sound::Play( Music::Insert );
						wasBroken = true;
						break;
					}
				}
				else
				{
					pushedDirection = ( xyBody.pos - wall.pos ).Normalized();
					pushedNow = true;
				}
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

			// Recheck from the beginning with updated position.
			i = 0;
		}

		return corrected;
	};

	const AABBEx actualBody = GetHitBox();

	// Move to X-axis with collision.
	MoveSpecifiedAxis( Donya::Vector2{ 1.0f, 0.0f }, velocity.x, actualBody );

	// Move to Y-axis with collision.
	bool wasCorrected = MoveSpecifiedAxis( Donya::Vector2{ 0.0f, 1.0f }, velocity.y, actualBody );
	if ( wasCorrected )
	{
		velocity.y = 0.0f;
	}

	// Move to Z-axis only.
	pos.z += velocity.z;
}

#if USE_IMGUI

void FragileBlock::ShowImGuiNode()
{
	ImGui::DragFloat3( u8"ワールド座標",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI

// region FragileBlock
#pragma endregion

#pragma region Gimmick

Gimmick::Gimmick() :
	stageNo(), fragileBlocks()
{}
Gimmick::~Gimmick() = default;

void Gimmick::Init( int stageNumber )
{
	ParamHeavyBlock::Get().Init();

	LoadParameter();

	stageNo = stageNumber;
}
void Gimmick::Uninit()
{
	fragileBlocks.clear();
}

void Gimmick::Update( float elapsedTime )
{
#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

	for ( auto &it : fragileBlocks )
	{
		it.Update( elapsedTime );
	}
}
void Gimmick::PhysicUpdate( const std::vector<BoxEx> &terrains )
{
	const size_t blockCount = fragileBlocks.size();

	auto ToBox = []( const AABBEx &aabb )
	{
		BoxEx box{};
		box.pos.x		= aabb.pos.x;
		box.pos.y		= aabb.pos.y;
		box.size.x		= aabb.size.x;
		box.size.y		= aabb.size.y;
		box.velocity.x	= aabb.velocity.x;
		box.velocity.y	= aabb.velocity.y;
		box.exist		= aabb.exist;
		box.mass		= aabb.mass;
		return box;
	};

	// The "fragileBlocks" will update at PhysicUpdate().
	// So I prepare a temporary vector of terrains and update this every time update elements.
	std::vector<BoxEx> boxes{ blockCount }; // Necessary for AABB to Box.
	for ( size_t i = 0; i < blockCount; ++i )
	{
		boxes[i] = ToBox( fragileBlocks[i].GetHitBox() );
	}

	std::vector<BoxEx> allTerrains = boxes; // [blocks][terrains]
	allTerrains.insert( allTerrains.end(), terrains.begin(), terrains.end() );

	for ( size_t i = 0; i < blockCount; ++i )
	{
		fragileBlocks[i].PhysicUpdate( allTerrains );

		allTerrains[i] = ToBox( fragileBlocks[i].GetHitBox() );
	}

	// Erase the should remove blocks.
	{
		auto itr = std::remove_if
		(
			fragileBlocks.begin(), fragileBlocks.end(),
			[]( FragileBlock &element )
			{
				return element.ShouldRemove();
			}
		);
		fragileBlocks.erase( itr, fragileBlocks.end() );
	}
}

void Gimmick::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	for ( const auto &it : fragileBlocks )
	{
		it.Draw( V, P, lightDir );
	}
}

std::vector<AABBEx> Gimmick::RequireHitBoxes() const
{
	std::vector<AABBEx> boxes{};
	for ( const auto &it : fragileBlocks )
	{
		boxes.emplace_back( it.GetHitBox() );
	}
	return boxes;
}

void Gimmick::LoadParameter( bool fromBinary )
{
	std::string filePath = GenerateSerializePath( SERIAL_ID, fromBinary );
	Donya::Serializer::Load( *this, filePath.c_str(), SERIAL_ID, fromBinary );
}

#if USE_IMGUI

void Gimmick::SaveParameter()
{
	bool useBinary = true;
	std::string filePath{};

	filePath = GenerateSerializePath( SERIAL_ID, useBinary );
	Donya::Serializer::Save( *this, filePath.c_str(), SERIAL_ID, useBinary );

	useBinary = false;

	filePath = GenerateSerializePath( SERIAL_ID, useBinary );
	Donya::Serializer::Save( *this, filePath.c_str(), SERIAL_ID, useBinary );
}

void Gimmick::UseImGui()
{
	ParamHeavyBlock::Get().UseImGui();

	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"ギミック" ) )
		{
			ImGui::Text( u8"アルファ向けのデバッグ用です" );
			ImGui::Text( "" );
		
			// Resizing.
			{
				if ( ImGui::Button( u8"末尾にブロック追加" ) )
				{
					fragileBlocks.push_back( {} );
					fragileBlocks.back().Init( Donya::Vector3::Zero() );
				}
				if ( fragileBlocks.empty() )
				{
					// Align a line.
					ImGui::Text( "" );
				}
				else if ( ImGui::Button( u8"末尾のブロック削除" ) )
				{
					fragileBlocks.pop_back();
				}
			}

			int i = 0;
			std::string caption{};
			for ( auto &it : fragileBlocks )
			{
				caption = "Block[" + std::to_string( i++ ) + "]";
				if ( ImGui::TreeNode( caption.c_str() ) )
				{
					it.ShowImGuiNode();
					ImGui::TreePop();
				}
			}

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

// region Gimmick
#pragma endregion
