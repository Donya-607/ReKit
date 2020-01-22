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

struct ParamOneWayBlock final : public Donya::Singleton<ParamOneWayBlock>
{
	friend Donya::Singleton<ParamOneWayBlock>;
public:
	struct Member
	{
		float	openAmount{ 1.0f };	// Constant value.
		float	closeSpeed{ 1.0f };	// Acceleration.
		AABBEx	hitBox{};			// Hit-Box of using to the collision to the stage.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( openAmount ),
				CEREAL_NVP( closeSpeed ),
				CEREAL_NVP( hitBox )
			);
			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "OneWayBlock";
	Member m;
private:
	ParamOneWayBlock() : m() {}
public:
	~ParamOneWayBlock() = default;
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
			if ( ImGui::TreeNode( u8"ギミック[OneWayBlock]・調整データ" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"質量" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox  ( ( prefix + u8"当たり判定は有効か" ).c_str(), &pHitBox->exist );
				};

				ImGui::DragFloat( u8"開く量",		&m.openAmount, 0.1f, 0.001f );
				ImGui::DragFloat( u8"閉まる速度",	&m.closeSpeed, 1.0f, 0.001f );

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
CEREAL_CLASS_VERSION( ParamOneWayBlock::Member, 0 )

void OneWayBlock::ParameterInit()
{
	ParamOneWayBlock::Get().Init();
}
#if USE_IMGUI
void OneWayBlock::UseParameterImGui()
{
	ParamOneWayBlock::Get().UseImGui();
}
#endif // USE_IMGUI

OneWayBlock::OneWayBlock() : GimmickBase(),
	openDirection( 0.0f, 1.0f, 0.0f ), velocity()
{}
OneWayBlock::OneWayBlock( const Donya::Vector3 &openDirection ) : GimmickBase(),
	openDirection( openDirection.Normalized() ), velocity()
{}
OneWayBlock::~OneWayBlock() = default;

void OneWayBlock::Init( int gimmickKind, float roll, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	rollDegree	= roll;
	pos			= wsPos;
	velocity	= 0.0f;

	initPos		= pos;
	velocity	= Donya::Vector3::Zero();
}
void OneWayBlock::Uninit()
{
	// No op.
}

void OneWayBlock::Update( float elapsedTime )
{
	Close( elapsedTime );
}
void OneWayBlock::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	// This block collide to only player.

	pos += velocity;

	const BoxEx wsMovedBody = GetHitBox().Get2D();
	if ( Donya::Box::IsHitBox( wsMovedBody, player ) )
	{
		Open();
	}
}

void OneWayBlock::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 color{ 0.8f, 0.9f, 1.0f, 0.9f };

	BaseDraw( WVP, W, lightDir, color );
}

bool OneWayBlock::ShouldRemove() const
{
	// Don't destroy.
	return false;
}

AABBEx OneWayBlock::GetHitBox() const
{
	AABBEx base = ParamOneWayBlock::Get().Data().hitBox;
	base.pos		+= pos;
	base.velocity	=  velocity;
	base.attr		=  kind;
	return base;
}

void OneWayBlock::Close( float elapsedTime )
{
	const auto param = ParamOneWayBlock::Get().Data();

	const Donya::Vector3 destVec	= initPos - pos;
	const Donya::Vector3 closeVec	= destVec.Normalized() * param.closeSpeed * elapsedTime;

	velocity += closeVec;

	if ( destVec.LengthSq() <= velocity.LengthSq() )
	{
		// Prevent pass over the "initPos".
		velocity = destVec;
	}
}
void OneWayBlock::Open()
{
	velocity = -openDirection;
	velocity *= ParamOneWayBlock::Get().Data().openAmount;
}

Donya::Vector4x4 OneWayBlock::GetWorldMatrix( bool useDrawing ) const
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
	mat._11 = wsBox.size.x;
	mat._22 = wsBox.size.y;
	mat._33 = wsBox.size.z;
	mat *= R;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

#if USE_IMGUI

void OneWayBlock::ShowImGuiNode()
{
	using namespace GimmickUtility;

	ImGui::Text( u8"種類：%d[%s]", kind, ToString( ToKind( kind ) ).c_str() );
	ImGui::DragFloat ( u8"Ｚ軸回転量",	&rollDegree,	1.0f	);
	ImGui::DragFloat3( u8"ワールド座標",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI
