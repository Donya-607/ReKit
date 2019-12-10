#include "Gimmicks.h"

#include "Donya/GeometricPrimitive.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "Common.h"
#include "FilePath.h"

#pragma region HeavyBlock

struct ParamHeavyBlock final : public Donya::Singleton<ParamHeavyBlock>
{
	friend Donya::Singleton<ParamHeavyBlock>;
public:
	struct Member
	{
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
				// CEREAL_NVP( x )
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

HeavyBlock::HeavyBlock() :
	pos()
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

}

void HeavyBlock::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static auto cube = Donya::Geometric::CreateCube();

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

Donya::AABB HeavyBlock::GetHitBox() const
{
	Donya::AABB base = ParamHeavyBlock::Get().Data().hitBox;
	base.pos += pos;
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


#if USE_IMGUI

void HeavyBlock::ShowImGuiNode()
{
	ImGui::DragFloat3( u8"ワールド座標", &pos.x );
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

void Gimmick::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	for ( const auto &it : heavyBlocks )
	{
		it.Draw( V, P, lightDir );
	}
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
