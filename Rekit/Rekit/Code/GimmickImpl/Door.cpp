#include "Door.h"

#include <algorithm>		// Use std::max, min.
#include <string>

#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.
#include "Donya/Sound.h"
#include "Donya/Keyboard.h"

#include "FilePath.h"
#include "GimmickUtil.h"	// Use for the GimmickKind, a namespaces.
#include "Music.h"

#undef max
#undef min

struct ParamDoor final : public Donya::Singleton<ParamDoor>
{
	friend Donya::Singleton<ParamDoor>;
public:
	struct Member
	{
		float	drawScale{ 1.0f };
		float	openSpeed{};
		AABBEx	hitBox{};
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize ( Archive& archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP ( openSpeed ),
				CEREAL_NVP ( hitBox )
			);
			if ( 1 <= version )
			{
				archive( CEREAL_NVP( drawScale ) );
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	static constexpr const char* SERIAL_ID = "Door";
	Member m;
private:
	ParamDoor () : m () {}
public:
	~ParamDoor () = default;
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
			if (ImGui::TreeNode ( u8"ギミック[Door]・調整データ" ))
			{
				auto AdjustAABB = []( const std::string& prefix, AABBEx* pHitBox )
				{
					ImGui::DragFloat2 ( (prefix + u8"中心位置のオフセット").c_str (), &pHitBox->pos.x );
					ImGui::DragFloat2 ( (prefix + u8"サイズ（半分を指定）").c_str (), &pHitBox->size.x );
					ImGui::DragInt ( (prefix + u8"質量").c_str (), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox ( (prefix + u8"当たり判定は有効か").c_str (), &pHitBox->exist );
				};

				ImGui::DragFloat ( u8"描画スケール",		&m.drawScale, 0.1f );
				ImGui::DragFloat ( u8"ドアが開く速度",	&m.openSpeed, 0.1f );

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
CEREAL_CLASS_VERSION ( ParamDoor::Member, 1 )



void Door::ParameterInit ()
{
	ParamDoor::Get ().Init ();
}
#if USE_IMGUI
void Door::UseParameterImGui ()
{
	ParamDoor::Get ().UseImGui ();
}
#endif // USE_IMGUI

Door::Door () : GimmickBase (),
id ( -1 ), direction ( 0, 0, 0 ), movedWidth ( 0 ), state ( Door::DoorState::Wait )
{}
Door::Door ( int id, const Donya::Vector3& direction ) : GimmickBase (),
id ( id ), direction ( direction ), movedWidth ( 0 ), state ( Door::DoorState::Wait )
{}
Door::~Door () = default;

void Door::Init ( int gimmickKind, float roll, const Donya::Vector3 & wsPos )
{
	kind = gimmickKind;
	rollDegree = roll;
	pos = wsPos;
	velocity = 0.0f;
}
void Door::Uninit ()
{
	// No op.
}

void Door::Update ( float elapsedTime )
{
	switch (state)
	{
	case DoorState::Wait:
	#if DEBUG_MODE
		if (Donya::Keyboard::Trigger ( 'K' ))
		{
			GimmickStatus::Register( id, true );
		}
	#endif // DEBUG_MODE

		if (GimmickStatus::Refer( id ))
		{
			state = DoorState::Open;
			Donya::Sound::Play( Music::DoorOpenOrClose );
		}

		break;

	case DoorState::Open:
		// 正方形なのでxでもyでもどっちでも良いのではないか説
		if (movedWidth >= ParamDoor::Get ().Data ().hitBox.size.x * 2)
		{
			velocity = 0;
			// GimmickStatus::Remove ( id ); // Prevent to reset when the scene initialization.
			state = DoorState::Opened;
			break;
		}

		velocity = direction * ParamDoor::Get ().Data ().openSpeed;
		movedWidth += ParamDoor::Get ().Data ().openSpeed;
		break;

	case DoorState::Opened:
		velocity = 0;
		break;
	}
}
void Door::PhysicUpdate ( const BoxEx & player, const BoxEx & accompanyBox, const std::vector<BoxEx> & terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	pos += velocity;
}

void Door::Draw ( const Donya::Vector4x4 & V, const Donya::Vector4x4 & P, const Donya::Vector4 & lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix ( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

	BaseDraw ( WVP, W, lightDir, color );
}

void Door::WakeUp ()
{
	// No op.
}

bool Door::ShouldRemove () const
{
	return false;
}

Donya::Vector3 Door::GetPosition () const
{
	return pos;
}
AABBEx Door::GetHitBox () const
{
	const AABBEx hitBoxes = ParamDoor::Get ().Data ().hitBox;

	AABBEx wsHitBox = hitBoxes;
	wsHitBox.pos += pos;
	wsHitBox.velocity = velocity;
	wsHitBox.attr = kind;
	return wsHitBox;
}

Donya::Vector4x4 Door::GetWorldMatrix ( bool useDrawing ) const
{
	auto wsBox = GetHitBox ();
	if (useDrawing)
	{
		// The AABB size is half, but drawing object's size is whole.
		// wsBox.size *= 2.0f;
	}

	const Donya::Quaternion rotation = Donya::Quaternion::Make ( Donya::Vector3::Front (), ToRadian ( rollDegree ) );
	const Donya::Vector4x4 R = rotation.RequireRotationMatrix ();
	Donya::Vector4x4 mat{};
	mat._11 =
	mat._22 =
	mat._33 = ParamDoor::Get().Data().drawScale;
	mat *= R;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

#if USE_IMGUI

void Door::ShowImGuiNode ()
{
	ImGui::Text ( u8"種類：%d[Door]", kind );
	ImGui::DragFloat ( u8"Ｚ軸回転量", &rollDegree, 1.0f );
	ImGui::DragFloat3 ( u8"ワールド座標", &pos.x, 0.1f );
	ImGui::DragFloat3 ( u8"速度", &velocity.x, 0.01f );
}

#endif // USE_IMGUI
