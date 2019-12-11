#include "Gimmicks.h"

#include "Donya/GeometricPrimitive.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "Common.h"
#include "FilePath.h"

#undef max
#undef min

#pragma region HeavyBlock

struct ParamHeavyBlock final : public Donya::Singleton<ParamHeavyBlock>
{
	friend Donya::Singleton<ParamHeavyBlock>;
public:
	struct Member
	{
		float		gravity{};
		float		maxFallSpeed{};
		Donya::AABB	hitBox{};	// Hit-Box of using to the collision to the stage.
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
	static constexpr const char *SERIAL_ID = "HeavyBlock";
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
				auto AdjustAABB = []( const std::string &prefix, Donya::AABB *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::Checkbox( ( prefix + u8"当たり判定は有効か" ).c_str(), &pHitBox->exist );
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

HeavyBlock::HeavyBlock() :
	pos(), velocity()
{}
HeavyBlock::~HeavyBlock() = default;

void HeavyBlock::Init( const Donya::Vector3 &wsPos )
{
	pos = wsPos;
}
void HeavyBlock::Uninit()
{

}

void HeavyBlock::Update( float elapsedTime )
{
	Fall( elapsedTime );
}
void HeavyBlock::PhysicUpdate( const std::vector<Donya::Box> &terrains )
{
	AssignVelocity( terrains );
}

void HeavyBlock::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static Donya::Geometric::Cube cube = Donya::Geometric::CreateCube();

		constexpr Donya::Vector4 color{ 0.8f, 0.8f, 0.0f, 0.8f };
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

Donya::Vector3 HeavyBlock::GetPosition() const
{
	return pos;
}
Donya::AABB HeavyBlock::GetHitBox() const
{
	Donya::AABB base = ParamHeavyBlock::Get().Data().hitBox;
	base.pos		+= pos;
	base.velocity	=  velocity;
	return base;
}

Donya::Vector4x4 HeavyBlock::GetWorldMatrix( bool useDrawing ) const
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

void HeavyBlock::Fall( float elapsedTime )
{
	const auto DATA = ParamHeavyBlock::Get().Data();
	velocity.y -= DATA.gravity * elapsedTime;
	velocity.y =  std::max( DATA.maxFallSpeed, velocity.y );
}

void HeavyBlock::AssignVelocity( const std::vector<Donya::Box> &terrains )
{
	/// <summary>
	/// The "x Axis" is specify moving axis. please only set to { 1, 0 } or { 0, 1 }. This function  to be able to handle any axis.
	/// </summary>
	auto MoveSpecifiedAxis = [&]( Donya::Vector2 xyNAxis, float moveSpeed, const Donya::AABB &baseHitBox )->bool
	{
		bool corrected = false;

		Donya::Box previousXYBody{}; // Use for check "a wall is myself?".
		{
			previousXYBody.pos.x		= baseHitBox.pos.x;
			previousXYBody.pos.y		= baseHitBox.pos.y;
			previousXYBody.size.x		= baseHitBox.size.x;
			previousXYBody.size.y		= baseHitBox.size.y;
			previousXYBody.velocity.x	= baseHitBox.velocity.x;
			previousXYBody.velocity.y	= baseHitBox.velocity.y;
			previousXYBody.exist		= true;
		}

		// Only either X or Y is valid.
		const Donya::Vector2 xyVelocity = xyNAxis * moveSpeed;
		pos.x += xyVelocity.x;
		pos.y += xyVelocity.y;

		// Take a value of +1 or -1.
		float moveSign = scast<float>( Donya::SignBit( xyVelocity.x ) + Donya::SignBit( xyVelocity.y ) );

		// This process require the current move velocity(because using to calculate the repulse direction).
		if ( ZeroEqual( moveSign ) ) { return corrected; }
		// else

		// The player's hit box of stage is circle, but doing with rectangle for easily correction.
		Donya::Box xyBody{};
		{
			xyBody.pos.x  = GetPosition().x;
			xyBody.pos.y  = GetPosition().y;
			xyBody.size.x = baseHitBox.size.x;
			xyBody.size.y = baseHitBox.size.y;
			xyBody.exist  = true;
		}
		Donya::Vector2 xyBodyCenter = xyBody.pos;
		Donya::Vector2 bodySize{ xyBody.size.x * xyNAxis.x, xyBody.size.y * xyNAxis.y }; // Only either X or Y is valid.
		const float bodyWidth = bodySize.Length(); // Extract valid member by Length().

		for ( const auto &wall : terrains )
		{
			if ( !Donya::Box::IsHitBox( xyBody, wall ) ) { continue; }
			if ( previousXYBody == wall ) { continue; } // The terrains contain also myself.
			// else

			Donya::Vector2 xyWallCenter = wall.pos;
			Donya::Vector2 wallSize{ wall.size.x * xyNAxis.x, wall.size.y * xyNAxis.y }; // Only either X or Y is valid.
			float wallWidth = wallSize.Length(); // Extract valid member by Length().

			// Calculate colliding length.
			// First, calculate body's edge of moving side.
			// Then, calculate wall's edge of inverse moving side.
			// After that, calculate colliding length from two edges.
			// Finally, correct the position to inverse moving side only that length.

			Donya::Vector2 bodyEdge	= xyBodyCenter + ( xyNAxis * bodyWidth *  moveSign );
			Donya::Vector2 wallEdge	= xyWallCenter + ( xyNAxis * wallWidth * -moveSign );
			Donya::Vector2 diff		= bodyEdge - wallEdge;
			Donya::Vector2 axisDiff{ diff.x * xyNAxis.x, diff.y * xyNAxis.y };
			float collidingLength	= axisDiff.Length();
		
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

	const Donya::AABB actualBody = GetHitBox();

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

void HeavyBlock::ShowImGuiNode()
{
	ImGui::DragFloat3( u8"ワールド座標",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI

// region HeavyBlock
#pragma endregion

#pragma region Gimmick

Gimmick::Gimmick() :
	stageNo(), heavyBlocks()
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
	heavyBlocks.clear();
}

void Gimmick::Update( float elapsedTime )
{
#if USE_IMGUI
	UseImGui();
#endif // USE_IMGUI

	for ( auto &it : heavyBlocks )
	{
		it.Update( elapsedTime );
	}
}
void Gimmick::PhysicUpdate( const std::vector<Donya::Box> &terrains )
{
	//for ( auto &it : heavyBlocks )
	//{
	//	it.PhysicUpdate( terrains );
	//}

	const size_t blockCount = heavyBlocks.size();

	auto ToBox = []( const Donya::AABB &aabb )
	{
		Donya::Box box{};
		box.pos.x		= aabb.pos.x;
		box.pos.y		= aabb.pos.y;
		box.size.x		= aabb.size.x;
		box.size.y		= aabb.size.y;
		box.velocity.x	= aabb.velocity.x;
		box.velocity.y	= aabb.velocity.y;
		box.exist		= aabb.exist;
		return box;
	};

	// The "heavyBlocks" will update at PhysicUpdate().
	// So I prepare a temporary vector of terrains and update this every time update elements.
	std::vector<Donya::Box> boxes{ blockCount }; // Necessary for AABB to Box.
	for ( size_t i = 0; i < blockCount; ++i )
	{
		boxes[i] = ToBox( heavyBlocks[i].GetHitBox() );
	}

	std::vector<Donya::Box> allTerrains = boxes; // [blocks][terrains]
	allTerrains.insert( allTerrains.end(), terrains.begin(), terrains.end() );

	for ( size_t i = 0; i < blockCount; ++i )
	{
		heavyBlocks[i].PhysicUpdate( allTerrains );

		allTerrains[i] = ToBox( heavyBlocks[i].GetHitBox() );
	}
}

void Gimmick::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	for ( const auto &it : heavyBlocks )
	{
		it.Draw( V, P, lightDir );
	}
}

std::vector<Donya::AABB> Gimmick::RequireHitBoxes() const
{
	std::vector<Donya::AABB> boxes{};
	for ( const auto &it : heavyBlocks )
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
					heavyBlocks.push_back( {} );
					heavyBlocks.back().Init( Donya::Vector3::Zero() );
				}
				if ( heavyBlocks.empty() )
				{
					// Align a line.
					ImGui::Text( "" );
				}
				else if ( ImGui::Button( u8"末尾のブロック削除" ) )
				{
					heavyBlocks.pop_back();
				}
			}

			int i = 0;
			std::string caption{};
			for ( auto &it : heavyBlocks )
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
