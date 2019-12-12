#include "Hook.h"

#include <array>
#include <algorithm>		// Use std::min(), max().
#include <vector>

#include "Donya/Easing.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "FilePath.h"

#undef max
#undef min

class HookParam final : public Donya::Singleton<HookParam>
{
	friend Donya::Singleton<HookParam>;
public:
	struct Member
	{
		float		lengthLimit{};	// Limit value of length that can extend hook.
		float		throwSpeed{};	// 
		float		pullSpeed{};	// 
		float		pullTime{};		// 
		float		hitRadius{};	// Hit-Sphere of using to the collision to player.

//		Donya::AABB	appearanceBox{};// Apparent size of box.
		Donya::AABB	hitBoxPhysic{};	// Hit-Box of using to the collision to the stage.
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& archive, std::uint32_t version)
		{
			archive
			(
				CEREAL_NVP(lengthLimit),
				CEREAL_NVP(throwSpeed),
				CEREAL_NVP(pullSpeed),
				CEREAL_NVP(pullTime),
				CEREAL_NVP(hitRadius),
//				CEREAL_NVP(appearanceBox),
				CEREAL_NVP(hitBoxPhysic)
			);
			if (1 <= version)
			{
				// CEREAL_NVP( x )
			}
		}
	};
private:
	static constexpr const char* SERIAL_ID = "Hook";
	Member m;
private:
	HookParam() : m() {}
public:
	~HookParam() = default;
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
	void LoadParameter(bool fromBinary = true)
	{
		std::string filePath = GenerateSerializePath(SERIAL_ID, fromBinary);
		Donya::Serializer::Load(m, filePath.c_str(), SERIAL_ID, fromBinary);
	}

#if USE_IMGUI

	void SaveParameter()
	{
		bool useBinary = true;
		std::string filePath{};

		filePath = GenerateSerializePath(SERIAL_ID, useBinary);
		Donya::Serializer::Save(m, filePath.c_str(), SERIAL_ID, useBinary);

		useBinary = false;

		filePath = GenerateSerializePath(SERIAL_ID, useBinary);
		Donya::Serializer::Save(m, filePath.c_str(), SERIAL_ID, useBinary);
	}

public:
	void UseImGui()
	{
		if (ImGui::BeginIfAllowed())
		{
			if (ImGui::TreeNode(u8"フック・調整データ"))
			{
				auto AdjustAABB = [](const std::string& prefix, Donya::AABB* pHitBox)
				{
					ImGui::DragFloat2((prefix + u8"中心位置のオフセット").c_str(), &pHitBox->pos.x);
					ImGui::DragFloat2((prefix + u8"サイズ（半分を指定）").c_str(), &pHitBox->size.x);
					ImGui::Checkbox((prefix + u8"当たり判定は有効か").c_str(), &pHitBox->exist);
				};

				ImGui::DragFloat(u8"長さの限界値",					&m.lengthLimit, 1.0f, 0);
				ImGui::DragFloat(u8"投げられた時の速度",			&m.throwSpeed, 1.0f, 0.0f);
				ImGui::DragFloat(u8"引かれた時の初速",				&m.pullSpeed, 1.0f, 0.0f);
				ImGui::DragFloat(u8"引かれた時のEasingのフレーム数",&m.pullTime, 1.0f, 0.0f);
				ImGui::DragFloat(u8"Playerとの当たり判定の半径",	&m.hitRadius, 1.0f, 0.0f);

//				static Donya::Easing::Type type = Donya::Easing::Type::In;
//				{
//					int iType = scast<int>(type);
//
//					std::string caption = "Type : ";
//					if (type == Donya::Easing::Type::In) { caption += "In"; }
//					if (type == Donya::Easing::Type::Out) { caption += "Out"; }
//					if (type == Donya::Easing::Type::InOut) { caption += "InOut"; }
//
//					ImGui::SliderInt(caption.c_str(), &iType, 0, 2);
//
//					type = scast<Donya::Easing::Type>(iType);
//				}
//				static Donya::Easing::Kind type = Donya::Easing::Kind::Linear;
//				{
//					int iType = scast<int>(type);
//
//					std::string caption = "Kind : ";
//					if (type == Donya::Easing::Kind::In) { caption += "In"; }
//					if (type == Donya::Easing::Kind::Out) { caption += "Out"; }
//					if (type == Donya::Easing::Kind::InOut) { caption += "InOut"; }
//
//					ImGui::SliderInt(caption.c_str(), &iType, 0, 2);
//
//					type = scast<Donya::Easing::Type>(iType);
//				}

//				AdjustAABB(u8"みてくれのサイズ",		&m.appearanceBox, 1.0f, 0.0f);
				AdjustAABB(u8"当たり判定：ＶＳ地形",	&m.hitBoxPhysic);

				if (ImGui::TreeNode(u8"ファイル"))
				{
					static bool isBinary = true;
					if (ImGui::RadioButton("Binary", isBinary)) { isBinary = true; }
					if (ImGui::RadioButton("JSON", !isBinary)) { isBinary = false; }
					std::string loadStr{ "読み込み " };
					loadStr += (isBinary) ? "Binary" : "JSON";

					if (ImGui::Button(u8"保存"))
					{
						SaveParameter();
					}
					if (ImGui::Button(Donya::MultiToUTF8(loadStr).c_str()))
					{
						LoadParameter(isBinary);
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

CEREAL_CLASS_VERSION(HookParam::Member, 0)

Donya::Geometric::Cube			Hook::drawModel;
Donya::CBuffer<Hook::Constants>	Hook::cbuffer;
Donya::VertexShader				Hook::VSDemo;
Donya::PixelShader				Hook::PSDemo;

Hook::Hook(const Donya::Vector3& playerPos) :
	pos(playerPos), velocity(), direction(), state(ActionState::Throw), exist(true),
	easingTime(0), prevPress(false), momentPullDist(0), distance(0)
{}
Hook::~Hook() = default;

void Hook::Init()
{
	HookParam::Get().Init();

	drawModel = { Donya::Geometric::CreateCube() };
	CreateRenderingObjects();
}
void Hook::Uninit()
{
	HookParam::Get().Uninit();
}

void Hook::Update(float elapsedTime, Input controller)
{
#if USE_IMGUI
//
//	HookParam::Get().UseImGui();
	if (ImGui::BeginIfAllowed())
	{
		if (ImGui::TreeNode(u8"フック・今のデータ"))
		{
			ImGui::DragFloat3(u8"ワールド座標", &pos.x);
			ImGui::DragFloat3(u8"移動速度", &velocity.x);

			ImGui::TreePop();
		}

		ImGui::End();
	}
//
#endif // USE_IMGUI

	// Calc velocity.

#if DEBUG_MODE

	{
		const float throwSpeed = HookParam::Get().Data().throwSpeed;
//		velocity = controller.moveVelocity * throwSpeed * elapsedTime;
	}

#endif // DEBUG_MODE

	switch (state)
	{
	case ActionState::Throw:
		ThrowUpdate(elapsedTime,controller);
		if (prevPress && !controller.currPress)
		{
			state = ActionState::Stay;
		}
		break;

	case ActionState::Stay:
		velocity = { 0.0f };
		if (controller.currPress)
		{
			state = ActionState::Pull;
			momentPullDist;
		}
		break;

	case ActionState::Pull:
		PullUpdate(elapsedTime, controller);
		break;

	case ActionState::Erase:
		EraseUpdate(elapsedTime, controller);
		if (controller.currPress)
		{
			state = ActionState::End;
		}
		break;

	case ActionState::End:
		exist = false;
		break;

	default:
		break;
	}
	prevPress = controller.currPress;
}

void Hook::PhysicUpdate(const std::vector<Donya::Box>& terrains, const Donya::Vector3& playerPos)
{

	if (state == ActionState::Throw)
	{
		if (HookParam::Get().Data().lengthLimit < distance)
		{
			distance = HookParam::Get().Data().lengthLimit;
		}
		pos.x = direction.x * distance + playerPos.x;
		pos.y = direction.y * distance + playerPos.y;

		return;
	}

	Donya::Sphere hookBody{};
	{
		hookBody.pos.x = GetPosition().x;
		hookBody.pos.y = GetPosition().y;
		hookBody.velocity.x = GetVelocity().x;
		hookBody.velocity.y = GetVelocity().y;
		hookBody.radius = HookParam::Get().Data().hitRadius;
		hookBody.exist = true;
	}
	Donya::Sphere playerBody{};
	{
		playerBody.pos.x = playerPos.x;
		playerBody.pos.y = playerPos.y;
		playerBody.velocity.x = 0.0f;
		playerBody.velocity.y = 0.0f;
		playerBody.radius = 1.0f;
		playerBody.exist = true;
	}
	if(Donya::Sphere::IsHitSphere(hookBody, playerBody))
	{
		state = ActionState::End;
	}

	/// <summary>
/// The "x Axis" is specify moving axis. please only set to { 1, 0 } or { 0, 1 }. This function  to be able to handle any axis.
/// </summary>
	auto MoveSpecifiedAxis = [&](Donya::Vector2 xyNAxis, float moveSpeed)->void
	{
		// Only either X or Y is valid.
		const Donya::Vector2 xyVelocity = xyNAxis * moveSpeed;
		pos.x += xyVelocity.x;
		pos.y += xyVelocity.y;

		// Take a value of +1 or -1.
		const float moveSign = scast<float>(Donya::SignBit(xyVelocity.x) + Donya::SignBit(xyVelocity.y));
		const auto  actualBody = HookParam::Get().Data().hitBoxPhysic;

		// This process require the current move velocity(because using to calculate the repulse direction).
		if (ZeroEqual(moveSign)) { return; }
		// else

		// The hook's hit box of stage is circle, but doing with rectangle for easily correction.
		Donya::Box xyBody{};
		{
			xyBody.pos.x = GetPosition().x;
			xyBody.pos.y = GetPosition().y;
			xyBody.size.x = actualBody.size.x * xyNAxis.x; // Only either X or Y is valid.
			xyBody.size.y = actualBody.size.y * xyNAxis.y; // Only either X or Y is valid.
			xyBody.velocity.x = GetVelocity().x;
			xyBody.velocity.y = GetVelocity().y;
			if (state == ActionState::Stay || state == ActionState::Pull)	{ xyBody.exist = true; }
			else															{ xyBody.exist = false; }
		}
		Donya::Vector2 xyBodyCenter = xyBody.pos;
		const float bodyWidth = xyBody.size.Length(); // Extract valid member by Length().

		for (const auto& wall : terrains)
		{
			if (!Donya::Box::IsHitBox(xyBody, wall)) { continue; }
			// else

			Donya::Vector2 xyWallCenter = wall.pos;
			Donya::Vector2 wallSize{ wall.size.x * xyNAxis.x, wall.size.y * xyNAxis.y }; // Only either X or Y is valid.
			float wallWidth = wallSize.Length(); // Extract valid member by Length().

			// Calculate colliding length.
			// First, calculate body's edge of moving side.
			// Then, calculate wall's edge of inverse moving side.
			// After that, calculate colliding length from two edges.
			// Finally, correct the position to inverse moving side only that length.

			Donya::Vector2 bodyEdge = xyBodyCenter + (xyNAxis * bodyWidth * moveSign);
			Donya::Vector2 wallEdge = xyWallCenter + (xyNAxis * wallWidth * -moveSign);
			Donya::Vector2 diff = bodyEdge - wallEdge;
			Donya::Vector2 axisDiff{ diff.x * xyNAxis.x, diff.y * xyNAxis.y };
			float collidingLength = axisDiff.Length();
			collidingLength += fabsf(moveSpeed) * 0.1f; // Prevent the two edges onto same place(the collision detective allows same(equal) value).

			Donya::Vector2 xyCorrection
			{
				xyNAxis.x * (collidingLength * -moveSign),
				xyNAxis.y * (collidingLength * -moveSign)
			};
			pos.x += xyCorrection.x;
			pos.y += xyCorrection.y;

			// We must apply the repulsed position to hit-box for next collision.
			xyBody.pos.x = GetPosition().x;
			xyBody.pos.y = GetPosition().y;
		}
	};

	// Move to X-axis with collision.
	MoveSpecifiedAxis(Donya::Vector2{ 1.0f, 0.0f }, velocity.x);
	// Move to Y-axis with collision.
	MoveSpecifiedAxis(Donya::Vector2{ 0.0f, 1.0f }, velocity.y);
	// Move to Z-axis only.
	pos.z += velocity.z;
}

void Hook::Draw(const Donya::Vector4x4& matViewProjection, const Donya::Vector4& lightDirection, const Donya::Vector4& lightColor) const
{
	Donya::Vector4x4 T = Donya::Vector4x4::MakeTranslation(GetPosition());
	Donya::Vector4x4 S = Donya::Vector4x4::MakeScaling(HookParam::Get().Data().hitBoxPhysic.size * 2.0f/* Half size to Whole size */);
	Donya::Vector4x4 W = S * T;

	cbuffer.data.world = W.XMFloat();
	cbuffer.data.worldViewProjection = (W * matViewProjection).XMFloat();
	cbuffer.data.lightDirection = lightDirection;
	cbuffer.data.lightColor = lightColor;
	cbuffer.data.materialColor = Donya::Vector4{ 1.0f, 0.6f, 0.8f, 1.0f };

	cbuffer.Activate(0, /* setVS = */ true, /* setPS = */ true);
	VSDemo.Activate();
	PSDemo.Activate();

	drawModel.Render(nullptr, /* useDefaultShading = */ false);

	PSDemo.Deactivate();
	VSDemo.Deactivate();
	cbuffer.Deactivate();
}

Donya::Vector3 Hook::GetPosition() const
{
	return pos;
}
Donya::Vector3 Hook::GetVelocity() const
{
	return velocity;
}


void Hook::CreateRenderingObjects()
{
	cbuffer.Create();

	constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> inputElements
	{
		D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	// The function requires argument is std::vector, so convert.
	const std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementsVector{ inputElements.begin(), inputElements.end() };
	VSDemo.CreateByCSO(GetShaderPath(ShaderAttribute::Demo, /* wantVS */ true), inputElementsVector);
	PSDemo.CreateByCSO(GetShaderPath(ShaderAttribute::Demo, /* wantVS */ false));
}

void Hook::ThrowUpdate(float elapsedTime, Input controller)
{
	const float moveSpeed = HookParam::Get().Data().throwSpeed * elapsedTime;
	if (controller.stickVec.x != 0.0f || controller.stickVec.y != 0.0f)
	{
		direction.x = controller.stickVec.x;
		direction.y = controller.stickVec.y;

		distance += moveSpeed;
	}
}

void Hook::PullUpdate(float elapsedTime, Input controller)
{
	// Calc speed
	float speed = HookParam::Get().Data().pullSpeed * (1 - Ease(Donya::Easing::Kind::Circular, Donya::Easing::Type::Out, easingTime));
	easingTime += 1 / HookParam::Get().Data().pullTime * elapsedTime;
	if (easingTime >= 1) { easingTime = 1.0f; }

	Donya::Vector3 calcVec(controller.playerPos - pos);
	calcVec.Normalize();
	velocity = calcVec * speed;
}

// 消える時のアニメーション用関数
void Hook::EraseUpdate(float elapsedTime, Input controller)
{
	velocity = { 0.0f };
	state = ActionState::End;
}

#if USE_IMGUI

void Hook::UseImGui()
{
	HookParam::Get().UseImGui();

	//if (ImGui::BeginIfAllowed())
	//{
	//	if (ImGui::TreeNode(u8"フック・今のデータ"))
	//	{
	//		ImGui::DragFloat3(u8"ワールド座標", &pos.x);
	//		ImGui::DragFloat3(u8"移動速度", &velocity.x);

	//		ImGui::TreePop();
	//	}

	//	ImGui::End();
	//}
}

#endif // USE_IMGUI
