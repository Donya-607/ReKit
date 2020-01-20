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

struct ParamSwitchBlock final : public Donya::Singleton<ParamSwitchBlock>
{
	friend Donya::Singleton<ParamSwitchBlock>;
public:
	struct Member
	{
		float	gravity{};
		float	maxFallSpeed{};
		float	brakeSpeed{};		// Affect to inverse speed of current velocity(only X-axis).
		float	stopThreshold{};	// The threshold of a judge to stop instead of the brake.
		float	gatherSpeed{};		// Gather to switch.
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
			if ( ImGui::TreeNode( u8"�M�~�b�N[SwitchBlock]�E�����f�[�^" ) )
			{
				auto AdjustAABB = []( const std::string &prefix, AABBEx *pHitBox )
				{
					ImGui::DragFloat2( ( prefix + u8"���S�ʒu�̃I�t�Z�b�g" ).c_str(), &pHitBox->pos.x );
					ImGui::DragFloat2( ( prefix + u8"�T�C�Y�i�������w��j" ).c_str(), &pHitBox->size.x );
					ImGui::DragInt   ( ( prefix + u8"����" ).c_str(), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox  ( ( prefix + u8"�����蔻��͗L����" ).c_str(), &pHitBox->exist );
				};

				ImGui::DragFloat( u8"�d�͉����x",			&m.gravity,			0.1f	);
				ImGui::DragFloat( u8"�ő嗎�����x",			&m.maxFallSpeed,	0.1f	);
				ImGui::DragFloat( u8"�u���[�L���x�i�w���j",	&m.brakeSpeed,		0.5f	);
				ImGui::DragFloat( u8"��~����臒l�i�w���j",	&m.stopThreshold,	0.1f	);
				ImGui::DragFloat( u8"�X�C�b�`�ɋ߂Â����x",	&m.gatherSpeed,		0.1f	);

				AdjustAABB( u8"�����蔻��", &m.hitBox );

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
CEREAL_CLASS_VERSION( ParamSwitchBlock::Member, 0 )

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

SwitchBlock::SwitchBlock() : GimmickBase()
{}
SwitchBlock::~SwitchBlock() = default;

void SwitchBlock::Init( int gimmickKind, float roll, const Donya::Vector3 &wsPos )
{
	kind		= gimmickKind;
	rollDegree	= roll;
	pos			= wsPos;
	velocity	= 0.0f;
}
void SwitchBlock::Uninit()
{
	// No op.
}

void SwitchBlock::Update( float elapsedTime )
{
	Fall( elapsedTime );

	Brake( elapsedTime );
}
void SwitchBlock::PhysicUpdate( const BoxEx &player, const BoxEx &accompanyBox, const std::vector<BoxEx> &terrains, bool collideToPlayer, bool ignoreHitBoxExist )
{
	GatherToTheTarget( terrains );

	GimmickBase::PhysicUpdate( player, accompanyBox, terrains );
}

void SwitchBlock::Draw( const Donya::Vector4x4 &V, const Donya::Vector4x4 &P, const Donya::Vector4 &lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 color{ 0.7f, 0.7f, 0.7f, 0.8f };

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
	mat._11 = wsBox.size.x;
	mat._22 = wsBox.size.y;
	mat._33 = wsBox.size.z;
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

void SwitchBlock::GatherToTheTarget( const std::vector<BoxEx> &terrains )
{
	for ( const auto &it : terrains )
	{
		if ( !Gimmick::HasGatherAttribute( it ) ) { continue; }
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
		break; // I expect don't collide at the same time to or-more-two the destination.
	}
}

#if USE_IMGUI

void SwitchBlock::ShowImGuiNode()
{
	using namespace GimmickUtility;

	ImGui::Text( u8"��ށF%d[%s]", kind, ToString( ToKind( kind ) ).c_str() );
	ImGui::DragFloat ( u8"�y����]��",	&rollDegree,	1.0f	);
	ImGui::DragFloat3( u8"���[���h���W",	&pos.x,			0.1f	);
	ImGui::DragFloat3( u8"���x",			&velocity.x,	0.01f	);
}

#endif // USE_IMGUI
