#include "Gimmicks.h"

#include <algorithm>		// Use std::max, min.
#include <string>

#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.
#include "Donya/Sound.h"
#include "Donya/Keyboard.h"

#include "FilePath.h"
#include "Music.h"

#undef max
#undef min

struct ParamElevator final : public Donya::Singleton<ParamElevator>
{
	friend Donya::Singleton<ParamElevator>;
public:
	struct Member
	{
		float	moveSpeed{};
		float	waitTime{};
		AABBEx	hitBox{};
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize ( Archive& archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP ( moveSpeed ),
				CEREAL_NVP ( waitTime ),
				CEREAL_NVP ( hitBox )
			);
			if (1 <= version)
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char* SERIAL_ID = "Elevator";
	Member m;
private:
	ParamElevator () : m () {}
public:
	~ParamElevator () = default;
public:
	void Init ()
	{
		LoadParameter ();
	}
	void Uninit ()
	{
		// No op.
	}
public:
	Member Data () const { return m; }
private:
	void LoadParameter ( bool fromBinary = true )
	{
		std::string filePath = GenerateSerializePath ( SERIAL_ID, fromBinary );
		Donya::Serializer::Load ( m, filePath.c_str (), SERIAL_ID, fromBinary );
	}

#if USE_IMGUI

	void SaveParameter ()
	{
		bool useBinary = true;
		std::string filePath{};

		filePath = GenerateSerializePath ( SERIAL_ID, useBinary );
		Donya::Serializer::Save ( m, filePath.c_str (), SERIAL_ID, useBinary );

		useBinary = false;

		filePath = GenerateSerializePath ( SERIAL_ID, useBinary );
		Donya::Serializer::Save ( m, filePath.c_str (), SERIAL_ID, useBinary );
	}

public:
	void UseImGui ()
	{
		if (ImGui::BeginIfAllowed ())
		{
			if (ImGui::TreeNode ( u8"ギミック[Elevator]・調整データ" ))
			{
				auto AdjustAABB = []( const std::string& prefix, AABBEx* pHitBox )
				{
					ImGui::DragFloat2 ( (prefix + u8"中心位置のオフセット").c_str (), &pHitBox->pos.x );
					ImGui::DragFloat2 ( (prefix + u8"サイズ（半分を指定）").c_str (), &pHitBox->size.x );
					ImGui::DragInt ( (prefix + u8"質量").c_str (), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox ( (prefix + u8"当たり判定は有効か").c_str (), &pHitBox->exist );
				};

				ImGui::DragFloat ( u8"エレベーターが動く速度", &m.moveSpeed, 0.1f );
				ImGui::DragFloat ( u8"エレベーターの待機フレーム", &m.waitTime, 0.1f );

				AdjustAABB ( u8"当たり判定", &m.hitBox );

				if (ImGui::TreeNode ( u8"ファイル" ))
				{
					static bool isBinary = true;
					if (ImGui::RadioButton ( "Binary", isBinary )) { isBinary = true; }
					if (ImGui::RadioButton ( "JSON", !isBinary )) { isBinary = false; }
					std::string loadStr{ "読み込み " };
					loadStr += (isBinary) ? "Binary" : "JSON";

					if (ImGui::Button ( u8"保存" ))
					{
						SaveParameter ();
					}
					if (ImGui::Button ( Donya::MultiToUTF8 ( loadStr ).c_str () ))
					{
						LoadParameter ( isBinary );
					}

					ImGui::TreePop ();
				}

				ImGui::TreePop ();
			}

			ImGui::End ();
		}
	}

#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION ( ParamElevator::Member, 0 )



void Elevator::ParameterInit ()
{
	ParamElevator::Get ().Init ();
}
#if USE_IMGUI
void Elevator::UseParameterImGui ()
{
	ParamElevator::Get ().UseImGui ();
}
#endif // USE_IMGUI

Elevator::Elevator () : GimmickBase (),
id ( -1 ), direction ( 0, 0, 0 ), moveAmount ( 0 ), maxMoveAmount ( 0 ), waitCount ( 0 ), state ( Elevator::ElevatorState::Stay )
{}
Elevator::Elevator ( int id, const Donya::Vector3& direction, float moveAmount ) : GimmickBase (),
id ( id ), direction ( direction ), moveAmount ( 0 ), maxMoveAmount ( moveAmount ), waitCount ( 0 ), state ( Elevator::ElevatorState::Stay )
{}
Elevator::~Elevator () = default;

void Elevator::Init ( int gimmickKind, float roll, const Donya::Vector3& wsPos )
{
	kind = gimmickKind;
	rollDegree = roll;
	pos = wsPos;
	velocity = 0.0f;
	direction.Normalize ();
}
void Elevator::Uninit ()
{
	// No op                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         .
}

void Elevator::Update ( float elapsedTime )
{
	switch (state)
	{
	case ElevatorState::Stay:
		// debug用なので消してね-----------------------------------
		if (Donya::Keyboard::Trigger ( 'Q' ))
		{
			GimmickStatus::Register ( id, true );
		}
		//---------------------------------------------------------

		velocity = 0;
		if (GimmickStatus::Refer ( id ))
		{
			GimmickStatus::Remove ( id );
			state = ElevatorState::Go;
		}
		break;

	case ElevatorState::Go:
		velocity = direction * ParamElevator::Get ().Data ().moveSpeed;
		moveAmount += ParamElevator::Get ().Data ().moveSpeed;

		if (moveAmount >= maxMoveAmount)
		{
			float cma = ParamElevator::Get ().Data ().moveSpeed - (moveAmount - maxMoveAmount);		// Current move amount.
			velocity = direction * cma;
			moveAmount = maxMoveAmount;
			state = ElevatorState::Wait;
		}
		break;

	case ElevatorState::Wait:
		velocity = 0;
		if (waitCount++ >= ParamElevator::Get ().Data ().waitTime)
		{
			waitCount = 0;
			state = ElevatorState::GoBack;
		}
		break;

	case ElevatorState::GoBack:
		velocity = direction * ParamElevator::Get ().Data ().moveSpeed * -1;
		moveAmount -= ParamElevator::Get ().Data ().moveSpeed;

		if (moveAmount <= 0)
		{
			float cma = ParamElevator::Get ().Data ().moveSpeed - moveAmount;		// Current move amount.
			velocity = direction * cma * -1;
			state = ElevatorState::Stay;
		}
		break;
		
	default:
		break;
	}
}
void Elevator::PhysicUpdate ( const BoxEx & player, const BoxEx & accompanyBox, const std::vector<BoxEx> & terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	pos += velocity;
}

void Elevator::Draw ( const Donya::Vector4x4 & V, const Donya::Vector4x4 & P, const Donya::Vector4 & lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix ( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 colors = { 1.0f, 1.0f, 0.0f, 0.8f };

	BaseDraw ( WVP, W, lightDir, colors );
}

void Elevator::WakeUp ()
{
	// No op.
}

bool Elevator::ShouldRemove () const
{
	return false;
}

Donya::Vector3 Elevator::GetPosition () const
{
	return pos;
}
AABBEx Elevator::GetHitBox () const
{
	const AABBEx hitBoxes = ParamElevator::Get ().Data ().hitBox;

	AABBEx wsHitBox = hitBoxes;
	wsHitBox.pos += pos;
	wsHitBox.velocity = velocity;
	return wsHitBox;
}

Donya::Vector4x4 Elevator::GetWorldMatrix ( bool useDrawing ) const
{
	auto wsBox = GetHitBox ();
	if (useDrawing)
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

void Elevator::ShowImGuiNode ()
{
	using namespace GimmickUtility;

	ImGui::Text ( u8"種類：%d[%s]", kind, ToString ( ToKind ( kind ) ).c_str () );
	ImGui::DragFloat3 ( u8"ワールド座標", &pos.x, 0.1f );
	ImGui::DragFloat3 ( u8"速度", &velocity.x, 0.01f );
}

#endif // USE_IMGUI
