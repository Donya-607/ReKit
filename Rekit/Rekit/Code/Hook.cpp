#include "Hook.h"

#include <array>
#include <algorithm>		// Use std::min(), max().
#include <vector>

#include "Donya/Easing.h"
#include "Donya/Loader.h"
#include "Donya/Sound.h"
#include "Donya/Template.h"
#include "Donya/Useful.h"	// Use convert string functions.

#include "FilePath.h"
#include "GimmickUtil.h"
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
		float		slerpSpeed{};	// 

//		AABBEx		appearanceBox{};// Apparent size of box.
		AABBEx		hitBoxPhysic{};	// Hit-Box of using to the collision to the stage.
		AABBEx		hitBoxVacuum{};	// Hit-Box of using to the vacuum a some gimmicks.
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
				CEREAL_NVP( slerpSpeed ),
				CEREAL_NVP(hitBoxPhysic)
			);
			if (1 <= version)
			{
				archive( CEREAL_NVP( hitBoxVacuum ) );
			}
			if (2 <= version)
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
				auto AdjustAABB = [](const std::string& prefix, AABBEx* pHitBox)
				{
					ImGui::DragFloat2((prefix + u8"中心位置のオフセット").c_str(), &pHitBox->pos.x);
					ImGui::DragFloat2((prefix + u8"サイズ（半分を指定）").c_str(), &pHitBox->size.x);
					ImGui::DragInt( ( prefix + u8"質量" ).c_str(), &pHitBox->mass );
					ImGui::Checkbox((prefix + u8"当たり判定は有効か").c_str(), &pHitBox->exist);
				};

				ImGui::DragFloat(u8"長さの限界値",						&m.lengthLimit,	1.0f, 0.0f);
				ImGui::DragFloat(u8"投げられた時の速度",				&m.throwSpeed,	1.0f, 0.0f);
				ImGui::DragFloat(u8"引かれた時の初速",					&m.pullSpeed,	1.0f, 0.0f);
				ImGui::DragFloat(u8"引かれた時のEasingのフレーム数",	&m.pullTime,	1.0f, 0.0f);
				ImGui::DragFloat(u8"Playerとの当たり判定の半径",		&m.hitRadius,	1.0f, 0.0f);
				ImGui::SliderFloat(u8"stick倒した時の回転速度",			&m.slerpSpeed,	0.0f, 360.0f);

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
				AdjustAABB(u8"当たり判定：ＶＳ地形・",	&m.hitBoxPhysic);
				AdjustAABB(u8"当たり判定：引き寄せる・",	&m.hitBoxVacuum);

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

CEREAL_CLASS_VERSION(HookParam::Member, 1)

Donya::StaticMesh	Hook::drawModel{};
bool				Hook::wasLoaded{};

Hook::Hook(const Donya::Vector3& playerPos) :
	pos(playerPos), velocity(), state(Hook::ActionState::Throw),
	interval(0), easingTime(0), distance(0), momentPullDist(0),
	prevPress(false), exist(true), isHitCheckEnable(), placeablePoint(true)
{}
Hook::~Hook() = default;

void Hook::Init()
{
	HookParam::Get().Init();

	if ( !wasLoaded )
	{
		Donya::Loader loader{};
		bool  succeeded = loader.Load( GetModelPath( ModelAttribute::Hook ), nullptr );
		if ( !succeeded )
		{
			_ASSERT_EXPR( 0, L"Failed : Load the Hook's model." );
			return;
		}
		// else

		succeeded = Donya::StaticMesh::Create( loader, drawModel );
		if ( !succeeded )
		{
			_ASSERT_EXPR( 0, L"Failed : Create the Hook's model." );
			return;
		}
		// else

		wasLoaded = true;
	}
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
	auto IsHitToJammer = [&]()->bool
	{
		const BoxEx wsBody = GetHitBox().Get2D();
		
		for ( const auto &it : terrains )
		{
			if ( !GimmickUtility::HasAttribute( GimmickKind::JammerArea, it ) ) { continue; }
			// else

			if ( Donya::Box::IsHitBox( it, wsBody, /* ignoreExistFlag = */ true ) )
			{
				return true;
			}
		}

		return false;
	};

	if ( state == ActionState::Throw )
	{
		pos += velocity;

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

		if ( IsHitToJammer() )
		{
			placeablePoint = false;
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
	if ( IsHitToJammer() )
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

#if DEBUG_MODE
#include "Donya/GeometricPrimitive.h"
#include "Common.h"
#endif // DEBUG_MODE
void Hook::Draw(const Donya::Vector4x4& matViewProjection, const Donya::Vector4& lightDirection, const Donya::Vector4& lightColor) const
{
	const Donya::Vector3 drawOffset{ 0.0f, 0.0f, -2.5f }; // For near.

	const AABBEx wsHitBox = GetHitBox();
	Donya::Vector4x4 T = Donya::Vector4x4::MakeTranslation( wsHitBox.pos + drawOffset );
	Donya::Vector4x4 S = Donya::Vector4x4::MakeScaling( wsHitBox.size );
	Donya::Vector4x4 W = S * T;

	Donya::Vector4 color	= ( placeablePoint )
							? Donya::Vector4{ 0.8f, 0.0f, 0.6f, 1.0f }
							: Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };

	if ( state == ActionState::Throw )
	{
		color.w = 0.5f;
	}

	drawModel.Render
	(
		nullptr,
		/* useDefaultShading	= */ true,
		/* isEnableFill			= */ true,
		W * matViewProjection, W,
		lightDirection, color
	);

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		static Donya::Geometric::Cube cube = Donya::Geometric::CreateCube();

		const auto wsBody = GetVacuumHitBox();
		T = Donya::Vector4x4::MakeTranslation( wsBody.pos );
		S = Donya::Vector4x4::MakeScaling( wsBody.size * 2.0f );
		W = S * T;

		cube.Render
		(
			nullptr,
			true, true,
			W * matViewProjection, W,
			lightDirection, Donya::Vector4{ 1.0f, 1.0f, 1.0f, 0.5f }
		);
	}
#endif // DEBUG_MODE
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
	AABBEx wsBox	=  HookParam::Get().Data().hitBoxPhysic;
	wsBox.pos		+= GetPosition();
	wsBox.velocity	=  GetVelocity();
	wsBox.exist		=  ( state == ActionState::Stay || state == ActionState::Pull )
					?  true
					:  false;
	return wsBox;
}
AABBEx Hook::GetVacuumHitBox() const
{
	AABBEx wsBox	=  HookParam::Get().Data().hitBoxVacuum;
	wsBox.pos		+= GetPosition();
	wsBox.velocity	=  GetVelocity();
	wsBox.exist		=  ( state == ActionState::Stay || state == ActionState::Pull )
					?  true
					:  false;
	return wsBox;
}

void Hook::ThrowUpdate(float elapsedTime, Input controller)
{
	float speed = HookParam::Get ().Data ().throwSpeed * elapsedTime;
	velocity.x = controller.stickVec.x * speed;
	velocity.y = controller.stickVec.y * speed;
	Donya::Vector3 dir ( pos + velocity - controller.playerPos );

	float distance = dir.Length ();
	dir.Normalize ();

	if (HookParam::Get ().Data ().lengthLimit < distance)
	{
		velocity = 0;
//		speed = speed - (distance - HookParam::Get ().Data ().lengthLimit);
//		velocity = dir * speed;
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
