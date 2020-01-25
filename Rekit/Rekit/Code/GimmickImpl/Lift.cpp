#include "Lift.h"

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

struct ParamLift final : public Donya::Singleton<ParamLift>
{
	friend Donya::Singleton<ParamLift>;
public:
	struct Member
	{
		float	drawScale{ 1.0f };
		float	moveSpeed{};
		AABBEx	hitBox{};
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize ( Archive& archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP ( moveSpeed ),
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
	static constexpr const char* SERIAL_ID = "Lift";
	Member m;
private:
	ParamLift () : m () {}
public:
	~ParamLift () = default;
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
			if (ImGui::TreeNode ( u8"ギミック[Lift]・調整データ" ))
			{
				auto AdjustAABB = []( const std::string& prefix, AABBEx* pHitBox )
				{
					ImGui::DragFloat2 ( (prefix + u8"中心位置のオフセット").c_str (), &pHitBox->pos.x );
					ImGui::DragFloat2 ( (prefix + u8"サイズ（半分を指定）").c_str (), &pHitBox->size.x );
					ImGui::DragInt ( (prefix + u8"質量").c_str (), &pHitBox->mass, 1.0f, 0 );
					ImGui::Checkbox ( (prefix + u8"当たり判定は有効か").c_str (), &pHitBox->exist );
				};

				ImGui::DragFloat ( u8"描画スケール",				&m.drawScale, 0.1f );
				ImGui::DragFloat ( u8"エレベーターが動く速度",	&m.moveSpeed, 0.1f );

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
CEREAL_CLASS_VERSION ( ParamLift::Member, 1 )



void Lift::ParameterInit ()
{
	ParamLift::Get ().Init ();
}
#if USE_IMGUI
void Lift::UseParameterImGui ()
{
	ParamLift::Get ().UseImGui ();
}
#endif // USE_IMGUI

Lift::Lift () : GimmickBase (),
direction ( 0, 0, 0 ), moveAmount ( 0 ), maxMoveAmount ( 0 ), state ( 0 )
{}
Lift::Lift ( const Donya::Vector3& direction, float moveAmount ) : GimmickBase (),
direction ( direction ), moveAmount ( 0 ), maxMoveAmount ( moveAmount ), state ( 0 )
{}
Lift::~Lift () = default;

void Lift::Init ( int gimmickKind, float roll, const Donya::Vector3 & wsPos )
{
	kind = gimmickKind;
	rollDegree = roll;
	pos = wsPos;
	velocity = 0.0f;
	direction.Normalize ();
}
void Lift::Uninit ()
{
	// No op
}

void Lift::Update ( float elapsedTime )
{
	float speed;
	switch (state)
	{
	case 0:	// go
		moveAmount += ParamLift::Get ().Data ().moveSpeed;
		speed = ParamLift::Get ().Data ().moveSpeed;

		if (moveAmount >= maxMoveAmount)
		{
			speed = ParamLift::Get ().Data ().moveSpeed - (moveAmount - maxMoveAmount);
			moveAmount = maxMoveAmount;
			state++;
		}
		break;

	case 1:	// go back
		moveAmount -= ParamLift::Get ().Data ().moveSpeed;
		speed = ParamLift::Get ().Data ().moveSpeed * -1;

		if (moveAmount <= 0)
		{
			speed = ParamLift::Get ().Data ().moveSpeed - moveAmount * -1;
			moveAmount = 0;
			state--;
		}
		break;
	}

	velocity = direction * speed;
}
void Lift::PhysicUpdate ( const BoxEx & player, const BoxEx & accompanyBox, const std::vector<BoxEx> & terrains, bool collideToPlayer, bool ignoreHitBoxExist, bool allowCompress )
{
	pos += velocity;
}

void Lift::Draw ( const Donya::Vector4x4 & V, const Donya::Vector4x4 & P, const Donya::Vector4 & lightDir ) const
{
	Donya::Vector4x4 W = GetWorldMatrix ( /* useDrawing = */ true );
	Donya::Vector4x4 WVP = W * V * P;

	constexpr Donya::Vector4 colors[] = {
		{ 1.0f, 1.0f, 1.0f, 1.0f },		// Horizontal
		{ 1.0f, 1.0f, 1.0f, 1.0f }		// Vertical
	};

	BaseDraw ( WVP, W, lightDir, colors[scast<int> ( direction.x * direction.x )] );
}

void Lift::WakeUp ()
{
	// No op.
}

bool Lift::ShouldRemove () const
{
	return false;
}

Donya::Vector3 Lift::GetPosition () const
{
	return pos;
}
AABBEx Lift::GetHitBox () const
{
	const AABBEx hitBoxes = ParamLift::Get ().Data ().hitBox;

	AABBEx wsHitBox = hitBoxes;
	wsHitBox.pos += pos;
	wsHitBox.velocity = velocity;
	return wsHitBox;
}

Donya::Vector4x4 Lift::GetWorldMatrix ( bool useDrawing ) const
{
	auto wsBox = GetHitBox ();
	if (useDrawing)
	{
		// The AABB size is half, but drawing object's size is whole.
		// wsBox.size *= 2.0f;
	}

	Donya::Vector4x4 mat{};
	mat._11 =
	mat._22 =
	mat._33 = ParamLift::Get().Data().drawScale;
	mat._41 = wsBox.pos.x;
	mat._42 = wsBox.pos.y;
	mat._43 = wsBox.pos.z;
	return mat;
}

#if USE_IMGUI
void Lift::ShowImGuiNode ()
{
	ImGui::Text ( u8"種類：%d[Lift]", kind );
	ImGui::DragFloat3 ( u8"ワールド座標", &pos.x, 0.1f );
	ImGui::DragFloat3 ( u8"速度", &velocity.x, 0.01f );
}

#endif // USE_IMGUI
