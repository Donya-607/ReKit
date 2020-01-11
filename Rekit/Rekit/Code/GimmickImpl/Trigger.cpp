#include "Gimmicks.h"

#include <algorithm>		// Use std::max, min.
#include <string>

#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.
#include "Donya/Sound.h"

#include "FilePath.h"
#include "Music.h"

#undef max
#undef min

struct ParamTrigger final : public Donya::Singleton<ParamTrigger>
{
	friend Donya::Singleton<ParamTrigger>;
public:
	struct Member
	{
		struct KeyMember
		{
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				/*archive
				(
					CEREAL_NVP( stretchMax )
				);*/
				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct SwitchMember
		{
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				/*archive
				(
					CEREAL_NVP( stretchMax )
				);*/
				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		struct PullMember
		{
			float stretchMax;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( stretchMax )
				);
				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		KeyMember		mKey;
		SwitchMember	mSwitch;
		PullMember		mPull;

		AABBEx	hitBoxKey{};
		AABBEx	hitBoxSwitch{};
		AABBEx	hitBoxPull{};
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( hitBoxKey ),
				CEREAL_NVP( hitBoxSwitch ),
				CEREAL_NVP( hitBoxPull )
			);
			if ( 1 <= version )
			{
				archive
				(
					CEREAL_NVP( mKey ),
					CEREAL_NVP( mSwitch ),
					CEREAL_NVP( mPull )
				);
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "Trigger";
	Member m;
private:
	ParamTrigger() : m() {}
public:
	~ParamTrigger() = default;
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
	Member Data() const { return m; }
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
			if ( ImGui::TreeNode( u8"ギミック[Trigger]・調整データ" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt( ( prefix + u8"質量" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox( ( prefix + u8"当たり判定は有効か" ).c_str(), &pHitBox->exist );
				};

				if ( ImGui::TreeNode( u8"カギ" ) )
				{
					ImGui::TreePop();
				}
				if ( ImGui::TreeNode( u8"スイッチ" ) )
				{
					ImGui::TreePop();
				}
				if ( ImGui::TreeNode( u8"引き" ) )
				{
					ImGui::DragFloat( u8"引っ張る長さ", &m.mPull.stretchMax, 1.0f, 0.0f );

					ImGui::TreePop();
				}

				AdjustAABB( u8"当たり判定・鍵",		&m.hitBoxKey	);
				AdjustAABB( u8"当たり判定・スイッチ",	&m.hitBoxSwitch	);
				AdjustAABB( u8"当たり判定・引き",		&m.hitBoxPull	);

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
CEREAL_CLASS_VERSION( ParamTrigger::Member, 1 )



void Trigger::ParameterInit()
{
	ParamTrigger::Get().Init();
}
#if USE_IMGUI
void Trigger::UseParameterImGui()
{
	ParamTrigger::Get().UseImGui();
}
#endif // USE_IMGUI

Trigger::Trigger() : GimmickBase(),
	ID( 0 ), enable( false ),
	mKey(), mSwitch(), mPull()
{}
Trigger::Trigger( int id, bool enable ) : GimmickBase(),
	ID( id ), enable( enable ),
	mKey(), mSwitch(), mPull()
{}
Trigger::~Trigger() = default;

void Trigger::Init( int gimmickKind, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	pos			= wsPos;
	velocity	= 0.0f;

	switch ( kind )
	{
	case scast<int>( GimmickKind::TriggerKey ) :
		InitKey();
		break;
	case scast<int>( GimmickKind::TriggerSwitch ) :
		InitSwitch();
		break;
	case scast<int>( GimmickKind::TriggerPull ) :
		InitPull();
		break;
	default: return;
	}
}
void Trigger::Uninit()
{
	switch ( kind )
	{
	case scast<int>( GimmickKind::TriggerKey ) :
		UninitKey();
		break;
	case scast<int>( GimmickKind::TriggerSwitch ) :
		UninitSwitch();
		break;
	case scast<int>( GimmickKind::TriggerPull ) :
		UninitPull();
		break;
	default: return;
	}
}

void Trigger::Update( float elapsedTime )
{
	switch ( kind )
	{
	case scast<int>( GimmickKind::TriggerKey ) :
		UpdateKey( elapsedTime );
		break;
	case scast<int>( GimmickKind::TriggerSwitch ) :
		UpdateSwitch( elapsedTime );
		break;
	case scast<int>( GimmickKind::TriggerPull ) :
		UpdatePull( elapsedTime );
		break;
	default: return;
	}
}
void Trigger::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist )
{
	switch ( kind )
	{
	case scast<int>( GimmickKind::TriggerKey ) :
		PhysicUpdateKey( player, accompanyBox, terrains );
		break;
	case scast<int>( GimmickKind::TriggerSwitch ) :
		PhysicUpdateSwitch( player, accompanyBox, terrains );
		break;
	case scast<int>( GimmickKind::TriggerPull ) :
		PhysicUpdatePull( player, accompanyBox, terrains );
		break;
	default: return;
	}
}

void Trigger::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 colors[]
	{
		{ 0.8f, 1.0f, 0.0f, 0.8f },		// Key
		{ 1.0f, 0.0f, 0.8f, 0.8f },		// Switch
		{ 0.0f, 0.8f, 1.0f, 0.8f }		// Pull
	};
	constexpr Donya::Vector4 lightenFactors[]
	{
		{ 0.2f, 0.0f, 1.0f, 0.0f },		// Key
		{ 0.0f, 1.0f, 0.2f, 0.0f },		// Switch
		{ 1.0f, 0.2f, 0.0f, 0.0f }		// Pull
	};
	const int kindIndex = GetTriggerKindIndex();

	Donya::Vector4 color = colors[kindIndex];
	if ( enable ) { color += lightenFactors[kindIndex]; }

	BaseDraw( WVP, W, lightDir, color );
}

void Trigger::WakeUp()
{
	// No op.
}

bool Trigger::ShouldRemove() const
{
	return false;
}

Donya::Vector3 Trigger::GetPosition() const
{
	return pos;
}
AABBEx Trigger::GetHitBox() const
{
	const AABBEx hitBoxes[]
	{
		ParamTrigger::Get().Data().hitBoxKey,
		ParamTrigger::Get().Data().hitBoxSwitch,
		ParamTrigger::Get().Data().hitBoxPull
	};
	const bool hitBoxExists[]
	{
		false,					// Key
		true,					// Switch
		false					// Pull
	};
	const int kindIndex =  GetTriggerKindIndex();

	AABBEx wsHitBox		=  hitBoxes[kindIndex];
	wsHitBox.pos		+= pos;
	wsHitBox.velocity	=  velocity;
	wsHitBox.exist		=  hitBoxExists[kindIndex];
	wsHitBox.attr		= kind;
	return wsHitBox;
}

int Trigger::GetTriggerKindIndex() const
{
	const int kindIndex = kind - scast<int>( GimmickKind::TriggerKey );
	_ASSERT_EXPR( 0 <= kindIndex, L"Error : A trigger's kind is invalid!" );
	return kindIndex;
}

Donya::Vector4x4 Trigger::GetWorldMatrix( bool useDrawing ) const
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

void Trigger::InitKey()
{

}
void Trigger::InitSwitch()
{

}
void Trigger::InitPull()
{
	mPull.initPos = pos;
}

void Trigger::UninitKey()
{

}
void Trigger::UninitSwitch()
{

}
void Trigger::UninitPull()
{

}

void Trigger::UpdateKey( float elapsedTime )
{

}
void Trigger::UpdateSwitch( float elapsedTime )
{

}
void Trigger::UpdatePull( float elapsedTime )
{

}

void Trigger::PhysicUpdateKey( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains )
{
	GimmickBase::PhysicUpdate( player, accompanyBox, terrains, /* collideToPlayer = */ false, /* ignoreHitBoxExist = */ true );

	if ( !enable && Donya::Box::IsHitBox( player, GetHitBox().Get2D(), /* ignoreHitBoxExist = */ true ) )
	{
		enable = true;
	}
}
void Trigger::PhysicUpdateSwitch( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains )
{
	GimmickBase::PhysicUpdate( player, accompanyBox, terrains );
}
void Trigger::PhysicUpdatePull( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains )
{
	GimmickBase::PhysicUpdate( player, accompanyBox, terrains, /* collideToPlayer = */ false, /* ignoreHitBoxExist = */ true );

	// Restrict the direction of stretch.
	pos.x = mPull.initPos.x;

	const Donya::Vector3 stretched = pos - mPull.initPos;
	const float stretchMax = ParamTrigger::Get().Data().mPull.stretchMax;
	if ( stretchMax < stretched.Length() )
	{
		pos = mPull.initPos + ( stretched.Normalized() * stretchMax );
		enable = true;
	}
}

#if USE_IMGUI

void Trigger::ShowImGuiNode()
{
	using namespace GimmickUtility;

	ImGui::Text( u8"種類：%d[%s]", kind, ToString( ToKind( kind ) ).c_str() );
	ImGui::DragFloat3( u8"ワールド座標", &pos.x, 0.1f );
	ImGui::DragFloat3( u8"速度", &velocity.x, 0.01f );
}

#endif // USE_IMGUI
