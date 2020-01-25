#include "FlammableBlock.h"

#include <algorithm>			// Use std::max, min.
#include <string>

#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"		// Use convert string functions.

#include "FilePath.h"
#include "Music.h"
#include "GimmickImpl/Bomb.h"	// Use for confirming to "is the box Bomb?".

#undef max
#undef min

struct ParamFlammableBlock final : public Donya::Singleton<ParamFlammableBlock>
{
	friend Donya::Singleton<ParamFlammableBlock>;
public:
	struct Member
	{
		float	drawScale{ 1.0f };
		AABBEx	hitBox{};			// Hit-Box of using to the collision to the stage.
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
				archive( CEREAL_NVP( drawScale ) );
			}
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "FlammableBlock";
	Member m;
private:
	ParamFlammableBlock() : m() {}
public:
	~ParamFlammableBlock() = default;
public:
	void Init()
	{
		LoadParameter();
	}
	void Uninit()
	{
		// No op.
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
			if ( ImGui::TreeNode( u8"ギミック[Flammable]・調整データ" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"質量" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox  ( ( prefix + u8"当たり判定は有効か" ).c_str(), &pHitBox->exist );
				};

				ImGui::DragFloat( u8"描画スケール", &m.drawScale, 0.1f );
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
CEREAL_CLASS_VERSION( ParamFlammableBlock::Member, 1 )

void FlammableBlock::ParameterInit()
{
	ParamFlammableBlock::Get().Init();
}
#if USE_IMGUI
void FlammableBlock::UseParameterImGui()
{
	ParamFlammableBlock::Get().UseImGui();
}
#endif // USE_IMGUI

FlammableBlock::FlammableBlock() : GimmickBase(),
	wasFlamed( false )
{}
FlammableBlock::~FlammableBlock() = default;

void FlammableBlock::Init( int gimmickKind, float roll, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	rollDegree	= roll;
	pos			= wsPos;
	velocity	= 0.0f;
}
void FlammableBlock::Uninit()
{
	// No op.
}

void FlammableBlock::Update( float elapsedTime )
{

}
void FlammableBlock::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	if ( wasFlamed ) { return; }
	// else

	for ( const auto &it : terrains )
	{
		if ( !Bomb::IsExplosionBox( it ) ) { continue; }
		// else

		if ( Donya::Box::IsHitBox( it, GetHitBox().Get2D(), /* ignoreExistFlag = */ true ) )
		{
			wasFlamed = true;
			return;
		}
	}
}

void FlammableBlock::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

	BaseDraw( WVP, W, lightDir, color );
}

bool FlammableBlock::ShouldRemove() const
{
	return wasFlamed;
}

AABBEx FlammableBlock::GetHitBox() const
{
	if ( wasFlamed ) { return AABBEx::Nil(); }
	// else

	AABBEx base = ParamFlammableBlock::Get().Data().hitBox;
	base.pos		+= pos;
	base.velocity	=  velocity;
	base.attr		=  kind;
	return base;
}

Donya::Vector4x4 FlammableBlock::GetWorldMatrix( bool useDrawing ) const
{
	auto wsBox = GetHitBox();
	if ( useDrawing )
	{
		// The AABB size is half, but drawing object's size is whole.
		// wsBox.size *= 2.0f;
	}

	const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Front(), ToRadian( rollDegree ) );
	const Donya::Vector4x4 R = rotation.RequireRotationMatrix();
	Donya::Vector4x4 mat{};
	mat._11 =
	mat._22 =
	mat._33 = ParamFlammableBlock::Get().Data().drawScale;
	mat *= R;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

#if USE_IMGUI

void FlammableBlock::ShowImGuiNode()
{
	ImGui::Text( u8"種類：%d[FlammableBlock]", kind );
	ImGui::DragFloat ( u8"Ｚ軸回転量",	&rollDegree,	1.0f	);
	ImGui::DragFloat3( u8"ワールド座標",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI
