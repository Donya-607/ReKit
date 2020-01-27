#include "SwitchBlock.h"

#include <algorithm>		// Use std::max, min.
#include <string>

#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "FilePath.h"
#include "GimmickUtil.h"	// Use for the GimmickKind, a namespaces.
#include "Music.h"

#undef max
#undef min

struct ParamSwitchBlock final : public Donya::Singleton<ParamSwitchBlock>
{
	friend Donya::Singleton<ParamSwitchBlock>;
public:
	struct Member
	{
		float	drawScale{ 1.0f };
		float	gravity{};
		float	maxFallSpeed{};
		float	brakeSpeed{};		// Affect to inverse speed of current velocity(only X-axis).
		float	stopThreshold{};	// The threshold of a judge to stop instead of the brake.
		float	gatherSpeed{};		// Gather to switch.
		float	scalingSpeed{ 1.0f };// Use for the performance of appear.
		AABBEx	hitBox{};			// Hit-Box of using to the collision to the stage.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( gravity ),
				CEREAL_NVP( maxFallSpeed ),
				CEREAL_NVP( brakeSpeed ),
				CEREAL_NVP( stopThreshold ),
				CEREAL_NVP( gatherSpeed ),
				CEREAL_NVP( hitBox )
			);
			if ( 1 <= version )
			{
				archive( CEREAL_NVP( scalingSpeed ) );
			}
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( drawScale ) );
			}
			if ( 3 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char *SERIAL_ID = "SwitchBlock";
	Member m;
private:
	ParamSwitchBlock() : m() {}
public:
	~ParamSwitchBlock() = default;
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
			if ( ImGui::TreeNode( u8"ギミック[SwitchBlock]・調整データ" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"中心位置のオフセット" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"サイズ（半分を指定）" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"質量" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox  ( ( prefix + u8"当たり判定は有効か" ).c_str(), &pHitBox->exist );
				};

				ImGui::DragFloat( u8"描画スケール",			&m.drawScale,		0.1f	);
				ImGui::DragFloat( u8"重力加速度",			&m.gravity,			0.1f	);
				ImGui::DragFloat( u8"最大落下速度",			&m.maxFallSpeed,	0.1f	);
				ImGui::DragFloat( u8"ブレーキ速度（Ｘ軸）",	&m.brakeSpeed,		0.5f	);
				ImGui::DragFloat( u8"停止する閾値（Ｘ軸）",	&m.stopThreshold,	0.1f	);
				ImGui::DragFloat( u8"スイッチに近づく速度",	&m.gatherSpeed,		0.1f	);
				ImGui::Text( "" );
				ImGui::DragFloat( u8"大きくなる早さ",			&m.scalingSpeed,	0.1f	);

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
CEREAL_CLASS_VERSION( ParamSwitchBlock::Member, 2 )

void SwitchBlock::ParameterInit()
{
	ParamSwitchBlock::Get().Init();
}
#if USE_IMGUI
void SwitchBlock::UseParameterImGui()
{
	ParamSwitchBlock::Get().UseImGui();
}
#endif // USE_IMGUI

SwitchBlock::SwitchBlock() : GimmickBase(),
	wasBroken( false ),
	scale( 1.0f ),
	initPos()
{}
SwitchBlock::~SwitchBlock() = default;

void SwitchBlock::Init( int gimmickKind, float roll, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	rollDegree	= roll;
	pos			= wsPos;
	velocity	= 0.0f;

	initPos		= pos;
}
void SwitchBlock::AddOffset( const Donya::Vector3 &worldOffset )
{
	GimmickBase::AddOffset( worldOffset );

	initPos = pos;
}
void SwitchBlock::Uninit()
{
	// No op.
}

void SwitchBlock::Update( float elapsedTime )
{
	if ( WasBroken() )
	{
		Respawn();
	}

	Fall( elapsedTime );

	Brake( elapsedTime );

	Scale( elapsedTime );
}
void SwitchBlock::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	if ( WasBroken() ) { return; }
	// else

	// VS Spike.
	{
		constexpr GimmickKind igniteKind = GimmickKind::Spike;
		BoxEx movedBody = GetHitBox().Get2D();
		movedBody.pos.x += velocity.x; // Move temporally.
		movedBody.pos.y += velocity.y; // Move temporally.

		for ( const auto &it : terrains )
		{
			if ( !GimmickUtility::HasAttribute( igniteKind, it ) ) { continue; }
			// else

			if ( Donya::Box::IsHitBox( it, movedBody ) )
			{
				BreakMe();
				return;
			}
		}
	}

	// Returns true if hit to gathering target.
	if ( GatherToTheTarget( terrains ) )
	{
		pos += velocity;
		return;
	}
	// else

	GimmickBase::PhysicUpdate( player, accompanyBox, terrains, /* collideToPlayer = */ true, /* ignoreHitBoxExist = */ false, /* allowCompress = */ false );
}

void SwitchBlock::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	if ( WasBroken() ) { return; }
	// else

	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

	BaseDraw( WVP, W, lightDir, color );
}

void SwitchBlock::WakeUp()
{
	// No op.
}

bool SwitchBlock::ShouldRemove() const
{
	// Don't destroy.
	return false;
}

AABBEx SwitchBlock::GetHitBox() const
{
	AABBEx base = ParamSwitchBlock::Get().Data().hitBox;
	base.pos		+= pos;
	base.size		*= scale;
	base.velocity	=  velocity;
	base.attr		=  kind;
	return base;
}

Donya::Vector4x4 SwitchBlock::GetWorldMatrix( bool useDrawing ) const
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
	mat._33 = ParamSwitchBlock::Get().Data().drawScale * scale;
	mat *= R;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

void SwitchBlock::Fall( float elapsedTime )
{
	const auto DATA = ParamSwitchBlock::Get().Data();
	velocity.y -= DATA.gravity * elapsedTime;
	velocity.y =  std::max( DATA.maxFallSpeed, velocity.y );
}

void SwitchBlock::Brake( float elapsedTime )
{
	const float moveSign = scast<float>( Donya::SignBit( velocity.x ) );
	if ( ZeroEqual( moveSign ) ) { return; }
	// else

	const float nowSpeed = fabsf( velocity.x );
	if ( nowSpeed <= ParamSwitchBlock::Get().Data().stopThreshold )
	{
		velocity.x = 0.0f;
		return;
	}
	// else

	const float brakeSpeed = std::min( nowSpeed, ParamSwitchBlock::Get().Data().brakeSpeed );
	velocity.x -= brakeSpeed * moveSign;
}

bool SwitchBlock::WasBroken() const
{
	return ( wasBroken ) ? true : false;
}
void SwitchBlock::BreakMe()
{
	velocity		= 0.0f;
	wasCompressed	= false;

	wasBroken		= true;
	scale			= 0.0f;
}

void SwitchBlock::Scale( float elapsedTime )
{
	scale += ParamSwitchBlock::Get().Data().scalingSpeed * elapsedTime;
	scale =  std::min( 1.0f, scale );
}
void SwitchBlock::Respawn()
{
	pos				= initPos;
	velocity		= 0.0f;
	wasCompressed	= false;

	wasBroken		= false;
	scale			= 0.0f;
}

bool SwitchBlock::GatherToTheTarget( const std::vector<BoxEx> &terrains )
{
	for ( const auto &it : terrains )
	{
		if ( !GimmickUtility::HasGatherAttribute( it ) ) { continue; }
		// else

		if ( !Donya::Box::IsHitBox( it, GetHitBox().Get2D(), /* ignoreExistFlag = */ true ) ) { continue; }
		// else

		const Donya::Vector3 otherPos  = Donya::Vector3{ it.pos, pos.z };
		const Donya::Vector3 vecToDest = otherPos - pos;
		const float currentSpeed = velocity.Length();
		const float gatherSpeed	 = std::max( currentSpeed, ParamSwitchBlock::Get().Data().gatherSpeed );

		velocity =	( vecToDest.Length() < gatherSpeed )
					? vecToDest
					: vecToDest.Normalized() * gatherSpeed;
		return true; // I expect don't collide at the same time to or-more-two the destination.
	}
	return false;
}

#if USE_IMGUI

void SwitchBlock::ShowImGuiNode()
{
	ImGui::Text( u8"種類：%d[SwitchBlock]", kind );
	ImGui::DragFloat ( u8"Ｚ軸回転量",	&rollDegree,	1.0f	);
	ImGui::DragFloat3( u8"ワールド座標",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"速度",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI
