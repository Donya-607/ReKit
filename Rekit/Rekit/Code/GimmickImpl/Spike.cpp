#include "Gimmicks.h"

#include <algorithm>		// Use std::max, min.
#include <string>

#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "FilePath.h"
#include "Music.h"

#undef max
#undef min

struct ParamSpikeBlock final : public Donya::Singleton<ParamSpikeBlock>
{
	friend Donya::Singleton<ParamSpikeBlock>;
public:
	struct Member
	{
		float	rotationSpeed{};
		AABBEx	hitBox{};		// Hit-Box of using to the collision to the stage.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( rotationSpeed ),
				CEREAL_NVP( hitBox )
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "SpikeBlock";
	Member m;
private:
	ParamSpikeBlock() : m() {}
public:
	~ParamSpikeBlock() = default;
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
			if ( ImGui::TreeNode( u8"ギミック[Spike]・調整データ" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt( ( prefix + u8"質量" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox( ( prefix + u8"当たり判定は有効か" ).c_str(), &pHitBox->exist );
				};

				ImGui::DragFloat( u8"回転速度", &m.rotationSpeed, ToRadian( 1.0f ) );

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
CEREAL_CLASS_VERSION( ParamSpikeBlock::Member, 0 )

void SpikeBlock::ParameterInit()
{
	ParamSpikeBlock::Get().Init();
}
#if USE_IMGUI
void SpikeBlock::UseParameterImGui()
{
	ParamSpikeBlock::Get().UseImGui();
}
#endif // USE_IMGUI

SpikeBlock::SpikeBlock() : GimmickBase(),
	radian()
{}
SpikeBlock::~SpikeBlock() = default;

void SpikeBlock::Init( int gimmickKind, float roll, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	rollDegree	= roll;
	pos			= wsPos;
	velocity	= 0.0f;
}
void SpikeBlock::Uninit()
{
	// No op.
}

void SpikeBlock::Update( float elapsedTime )
{
	radian += ParamSpikeBlock::Get().Data().rotationSpeed;
}
void SpikeBlock::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	// No op.
}

void SpikeBlock::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 color{ 1.0f, 0.1f, 0.0f, 0.8f };

	BaseDraw( WVP, W, lightDir, color );
}

void SpikeBlock::WakeUp()
{
	// No op.
}

bool SpikeBlock::ShouldRemove() const
{
	// Don't destroy.
	return false;
}

AABBEx SpikeBlock::GetHitBox() const
{
	AABBEx base = ParamSpikeBlock::Get().Data().hitBox;
	base.pos		+= pos;
	base.velocity	= velocity;
	base.attr		= kind;
	return base;
}

Donya::Vector4x4 SpikeBlock::GetWorldMatrix( bool useDrawing ) const
{
	auto wsBox = GetHitBox();
	if ( useDrawing )
	{
		// The AABB size is half, but drawing object's size is whole.
		// wsBox.size *= 2.0f;
	}

	const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Front(), radian + ToRadian( rollDegree ) );
	const Donya::Vector4x4 R = rotation.RequireRotationMatrix();
	const Donya::Vector4x4 S = Donya::Vector4x4::MakeScaling( wsBox.size );

	Donya::Vector4x4 mat = S * R;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

#if USE_IMGUI

void SpikeBlock::ShowImGuiNode()
{
	using namespace GimmickUtility;

	ImGui::Text( u8"種類：%d[%s]", kind, ToString( ToKind( kind ) ).c_str() );
	ImGui::DragFloat ( u8"Ｚ軸回転量",	&rollDegree,	1.0f	);
	ImGui::DragFloat3( u8"ワールド座標",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI
