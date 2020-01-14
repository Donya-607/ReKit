#include "Hook.h"

#include <array>
#include <algorithm>		// Use std::min(), max().
#include <vector>

#include "Donya/Easing.h"
#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "FilePath.h"
#include "Music.h"

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

//		AABBEx		appearanceBox{};// Apparent size of box.
		AABBEx		hitBoxPhysic{};	// Hit-Box of using to the collision to the stage.
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
			if (ImGui::TreeNode(u8"�t�b�N�E�����f�[�^"))
			{
				auto AdjustAABB = [](const std::string& prefix, AABBEx* pHitBox)
				{
					ImGui::DragFloat2((prefix + u8"���S�ʒu�̃I�t�Z�b�g").c_str(), &pHitBox->pos.x);
					ImGui::DragFloat2((prefix + u8"�T�C�Y�i�������w��j").c_str(), &pHitBox->size.x);
					ImGui::DragInt( ( prefix + u8"����" ).c_str(), &pHitBox->mass );
					ImGui::Checkbox((prefix + u8"�����蔻��͗L����").c_str(), &pHitBox->exist);
				};

				ImGui::DragFloat(u8"�����̌��E�l",					&m.lengthLimit,	1.0f, 0.0f);
				ImGui::DragFloat(u8"������ꂽ���̑��x",				&m.throwSpeed,	1.0f, 0.0f);
				ImGui::DragFloat(u8"�����ꂽ���̏���",				&m.pullSpeed,	1.0f, 0.0f);
				ImGui::DragFloat(u8"�����ꂽ����Easing�̃t���[����",	&m.pullTime,	1.0f, 0.0f);
				ImGui::DragFloat(u8"Player�Ƃ̓����蔻��̔��a",		&m.hitRadius,	1.0f, 0.0f);

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

//				AdjustAABB(u8"�݂Ă���̃T�C�Y",		&m.appearanceBox, 1.0f, 0.0f);
				AdjustAABB(u8"�����蔻��F�u�r�n�`",	&m.hitBoxPhysic);

				if (ImGui::TreeNode(u8"�t�@�C��"))
				{
					static bool isBinary = true;
					if (ImGui::RadioButton("Binary", isBinary)) { isBinary = true; }
					if (ImGui::RadioButton("JSON", !isBinary)) { isBinary = false; }
					std::string loadStr{ "�ǂݍ��� " };
					loadStr += (isBinary) ? "Binary" : "JSON";

					if (ImGui::Button(u8"�ۑ�"))
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

Donya::Geometric::Cube			Hook::drawModel{};
Donya::CBuffer<Hook::Constants>	Hook::cbuffer{};
Donya::VertexShader				Hook::VSDemo{};
Donya::PixelShader				Hook::PSDemo{};

Hook::Hook(const Donya::Vector3& playerPos) :
	pos(playerPos), velocity(), direction(), state(ActionState::Throw),
	easingTime(0), distance(0), momentPullDist(0),
	prevPress(false), exist(true), placeablePoint(true)
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
		if (ImGui::TreeNode(u8"�t�b�N�E���̃f�[�^"))
		{
			ImGui::DragFloat3(u8"���[���h���W", &pos.x);
			ImGui::DragFloat3(u8"�ړ����x", &velocity.x);

			ImGui::TreePop();
		}

		ImGui::End();
	}
//
#endif // USE_IMGUI

	switch (state)
	{
	case ActionState::Throw:
		ThrowUpdate(elapsedTime,controller);
		if (controller.currPress)
		{
			state = ( placeablePoint ) ? ActionState::Stay : ActionState::Erase;
			Donya::Sound::Play( Music::Appearance );
		}
		break;

	case ActionState::Stay:
		velocity = 0.0f;
		if (controller.currPress)
		{
			state = ActionState::Pull;
			momentPullDist;
			Donya::Sound::Play( Music::Pull );
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

void Hook::PhysicUpdate(const std::vector<BoxEx>& terrains, const Donya::Vector3& playerPos)
{
	if ( state == ActionState::Throw )
	{
		pos.x = direction.x * distance + playerPos.x;
		pos.y = direction.y * distance + playerPos.y;

		placeablePoint = true;
		const auto wsAABB = GetHitBox();
		BoxEx xyBody = wsAABB.Get2D();
		xyBody.exist = true;
		for ( const auto &it : terrains )
		{
			if ( Donya::Box::IsHitBox( it, xyBody ) )
			{
				placeablePoint = false;
				break;
			}
		}

		return;
	}
	// else

	Donya::Sphere hookBody{};
	{
		hookBody.pos.x			= GetPosition().x;
		hookBody.pos.y			= GetPosition().y;
		hookBody.velocity.x		= GetVelocity().x;
		hookBody.velocity.y		= GetVelocity().y;
		hookBody.radius			= HookParam::Get().Data().hitRadius;
		hookBody.exist			= true;
	}
	Donya::Sphere playerBody{};
	{
		playerBody.pos.x		= playerPos.x;
		playerBody.pos.y		= playerPos.y;
		playerBody.velocity.x	= 0.0f;
		playerBody.velocity.y	= 0.0f;
		playerBody.radius		= 1.0f;
		playerBody.exist		= true;
	}
	if ( Donya::Sphere::IsHitSphere( hookBody, playerBody ) )
	{
		state = ActionState::End;
		return;
	}
	// else

	auto CalcCollidingBox = [&]( const BoxEx &myself, const BoxEx &previousMyself )->BoxEx
	{
		for ( const auto &it : terrains )
		{
			if ( it.mass < myself.mass ) { continue; }
			if ( it == previousMyself  ) { continue; }
			// else

			if ( Donya::Box::IsHitBox( it, myself ) )
			{
				return it;
			}
		}

		return BoxEx::Nil();
	};

	const AABBEx actualBody		= GetHitBox();
	const BoxEx  previousXYBody	= actualBody.Get2D();

	Donya::Vector2 xyVelocity{ velocity.x, velocity.y };
	Donya::Vector2 moveSign // The moving direction of myself. Take a value of +1.0f or -1.0f.
	{
		scast<float>( Donya::SignBit( xyVelocity.x ) ),
		scast<float>( Donya::SignBit( xyVelocity.y ) )
	};

	BoxEx movedXYBody = previousXYBody;
	movedXYBody.pos  += xyVelocity;

	BoxEx other{};
	bool wasCollided = false;

	constexpr unsigned int MAX_LOOP_COUNT = 1000U;
	unsigned int loopCount{};
	while ( ++loopCount < MAX_LOOP_COUNT )
	{
		other = CalcCollidingBox( movedXYBody, previousXYBody );
		if ( other == BoxEx::Nil() ) { break; } // Does not detected a collision.
		// else

		if ( other.mass < movedXYBody.mass ) { continue; }
		// else

		if ( ZeroEqual( moveSign.x ) && !ZeroEqual( other.velocity.x ) )
		{
			// The myself's moving direction is considered the inverse of other's moving direction.
			moveSign.x = scast<float>( Donya::SignBit( -other.velocity.x ) );
		}
		if ( ZeroEqual( moveSign.y ) && !ZeroEqual( other.velocity.y ) )
		{
			// The myself's moving direction is considered the inverse of other's moving direction.
			moveSign.y = scast<float>( Donya::SignBit( -other.velocity.y ) );
		}

		if ( moveSign.IsZero() ) { continue; } // Each other does not move, so collide is no possible.
		// else

		Donya::Vector2 penetration{}; // Store absolute value.
		Donya::Vector2 plusPenetration
		{
			fabsf( ( movedXYBody.pos.x + movedXYBody.size.x ) - ( other.pos.x - other.size.x ) ),
			fabsf( ( movedXYBody.pos.y + movedXYBody.size.y ) - ( other.pos.y - other.size.y ) )
		};
		Donya::Vector2 minusPenetration
		{
			fabsf( ( movedXYBody.pos.x - movedXYBody.size.x ) - ( other.pos.x + other.size.x ) ),
			fabsf( ( movedXYBody.pos.y - movedXYBody.size.y ) - ( other.pos.y + other.size.y ) )
		};
		penetration.x
			= ( moveSign.x < 0.0f ) ? minusPenetration.x
			: ( moveSign.x > 0.0f ) ? plusPenetration.x
			: 0.0f;
		penetration.y
			= ( moveSign.y < 0.0f ) ? minusPenetration.y
			: ( moveSign.y > 0.0f ) ? plusPenetration.y
			: 0.0f;

		constexpr float ERROR_MARGIN = 0.0001f; // Prevent the two edges onto same place(the collision detective allows same(equal) value).

		Donya::Vector2 resolver
		{
			( penetration.x + ERROR_MARGIN ) * -moveSign.x,
			( penetration.y + ERROR_MARGIN ) * -moveSign.y
		};

		// Repulse to the more little(but greater than zero) axis side of penetration.
		if ( penetration.y < penetration.x || ZeroEqual( penetration.x ) )
		{
			movedXYBody.pos.y += resolver.y;
			velocity.y = 0.0f;
			moveSign.y = scast<float>( Donya::SignBit( resolver.y ) );
		}
		else // if ( !ZeroEqual( penetration.x ) ) is same as above this : " || ZeroEqual( penetration.x ) "
		{
			movedXYBody.pos.x += resolver.x;
			velocity.x = 0.0f;
			moveSign.x = scast<float>( Donya::SignBit( resolver.x ) );
		}

		wasCollided = true;
	}

	pos.x = movedXYBody.pos.x;
	pos.y = movedXYBody.pos.y;
	pos.z += velocity.z;

	if ( wasCollided )
	{
		state = ActionState::Stay;
	}
}

void Hook::Draw(const Donya::Vector4x4& matViewProjection, const Donya::Vector4& lightDirection, const Donya::Vector4& lightColor) const
{
	const AABBEx wsHitBox = GetHitBox();
	Donya::Vector4x4 T = Donya::Vector4x4::MakeTranslation( wsHitBox.pos );
	Donya::Vector4x4 S = Donya::Vector4x4::MakeScaling( wsHitBox.size * 2.0f/* Half size to Whole size */ );
	Donya::Vector4x4 W = S * T;

	cbuffer.data.world					= W.XMFloat();
	cbuffer.data.worldViewProjection	= (W * matViewProjection).XMFloat();
	cbuffer.data.lightDirection			= lightDirection;
	cbuffer.data.lightColor				= lightColor;
	cbuffer.data.materialColor			= ( placeablePoint )
										? Donya::Vector4{ 0.4f, 1.0f, 0.6f, 1.0f }
										: Donya::Vector4{ 0.8f, 0.0f, 0.6f, 1.0f };

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

AABBEx Hook::GetHitBox() const
{
	AABBEx wsBox	= HookParam::Get().Data().hitBoxPhysic;
	wsBox.pos		+= GetPosition();
	wsBox.velocity	= GetVelocity();
	wsBox.exist		= ( state == ActionState::Stay || state == ActionState::Pull )
					? true
					: false;
	return wsBox;
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
//	if ( controller.stickVec.IsZero() ) { return; }
	// else
	if (!controller.stickVec.IsZero ())
	{
		direction.x = controller.stickVec.x;
		direction.y = controller.stickVec.y;
	}

	const float moveSpeed = HookParam::Get().Data().throwSpeed * elapsedTime;
	if (controller.extend) { distance += moveSpeed; }
	if (controller.shrink) { distance -= moveSpeed; }
	
	if ( HookParam::Get().Data().lengthLimit < distance )
	{
		distance = HookParam::Get().Data().lengthLimit;
	}
	if (0 >= distance)
	{
		distance = 0;
	}
}

void Hook::PullUpdate(float elapsedTime, Input controller)
{
	float speed = HookParam::Get().Data().pullSpeed * (1 - Ease(Donya::Easing::Kind::Circular, Donya::Easing::Type::Out, easingTime));
	easingTime += ( 1.0f / HookParam::Get().Data().pullTime ) * elapsedTime;
	if (easingTime >= 1.0f) { easingTime = 1.0f; }

	Donya::Vector3 calcVec(controller.playerPos - pos);
	calcVec.Normalize();
	velocity = calcVec * speed;
}

// �����鎞�̃A�j���[�V�����p�֐�
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
	//	if (ImGui::TreeNode(u8"�t�b�N�E���̃f�[�^"))
	//	{
	//		ImGui::DragFloat3(u8"���[���h���W", &pos.x);
	//		ImGui::DragFloat3(u8"�ړ����x", &velocity.x);

	//		ImGui::TreePop();
	//	}

	//	ImGui::End();
	//}
}

#endif // USE_IMGUI
