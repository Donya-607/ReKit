#include "SceneChiProject.h"

#include <vector>

#include "Header/Camera.h"
#include "Header/CBuffer.h"
#include "Header/Constant.h"
#include "Header/Donya.h"		// Use GetFPS().
#include "Header/GeometricPrimitive.h"
#include "Header/Keyboard.h"
#include "Header/Mouse.h"
#include "Header/Quaternion.h"
#include "Header/Sprite.h"
#include "Header/Useful.h"
#include "Header/UseImgui.h"
#include "Header/Vector.h"

#include "ChiPlayer.h"
#include "Common.h"

using namespace DirectX;

struct SceneChiProject::Impl
{
public:
	struct Light
	{
		Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Donya::Vector4 direction{ 0.0f, 0.0f, 1.0f, 0.0f };
	};
public:
	Light							light;
	Donya::Vector4					mtlColor;

	ChiPlayer						player;

	std::shared_ptr<Donya::Camera>	pCamera;
public:
	Impl() :
		light(),
		mtlColor( 1.0f, 1.0f, 1.0f, 1.0f ),
		player(),
		pCamera( nullptr )
	{}
	~Impl()
	{
		pCamera.reset();
	}
public:
	void Init()
	{
		player.Init();

		pCamera = std::make_shared<Donya::Camera>();
		pCamera->Init
		(
			Common::ScreenWidthF(),
			Common::ScreenHeightF(),
			ToRadian( 30.0f ) // FOV
		);
		pCamera->SetFocusCoordinate( { 0.0f, 0.0f, 0.0f } );
		pCamera->SetPosition( { 0.0f, 32.0f, -64.0f } );
	}
	void Uninit()
	{
		player.Uninit();
	}

	void Update( float elapsedTime )
	{
		CameraUpdate();

		auto MakePlayerInput = []()->ChiPlayer::Input
		{
			ChiPlayer::Input input{};

			if ( Donya::Keyboard::Press( VK_UP    ) ) { input.moveVector.z = +1.0f; }
			if ( Donya::Keyboard::Press( VK_DOWN  ) ) { input.moveVector.z = -1.0f; }
			if ( Donya::Keyboard::Press( VK_LEFT  ) ) { input.moveVector.x = -1.0f; }
			if ( Donya::Keyboard::Press( VK_RIGHT ) ) { input.moveVector.x = +1.0f; }

			if ( Donya::Keyboard::Press( 'W' ) ) { input.moveVector.y = +1.0f; }
			if ( Donya::Keyboard::Press( 'S' ) ) { input.moveVector.y = -1.0f; }

			if ( Donya::Keyboard::Press( 'Z' ) ) { input.doDefend = true; }
			if ( Donya::Keyboard::Press( 'X' ) ) { input.doAttack = true; }

			return input;
		};
		player.Update( MakePlayerInput() );

	#if USE_IMGUI
		pCamera->ShowParametersToImGui();
		if ( ImGui::BeginIfAllowed() )
		{
			if ( ImGui::TreeNode( u8"ちー・テスト" ) )
			{
				ImGui::Text( "FPS[%f]", Donya::GetFPS() );
				ImGui::Text( "" );

				ImGui::SliderFloat3( u8"方向性ライト・向き", &light.direction.x, -1.0f, 1.0f );
				ImGui::ColorEdit4( u8"方向性ライト・カラー", &light.color.x );
				ImGui::ColorEdit4( u8"マテリアル・カラー", &mtlColor.x );

				ImGui::TreePop();
			}

			ImGui::End();
		}
	#endif // USE_IMGUI
	}

	void Draw( float elapsedTime )
	{
		// Back-ground.
		{
			Donya::Sprite::SetDrawDepth( 1.0f );
			Donya::Sprite::DrawRect
			(
				Common::HalfScreenWidthF(),
				Common::HalfScreenHeightF(),
				Common::ScreenWidthF(),
				Common::ScreenHeightF(),
				Donya::Color::Code::GRAY, 1.0f
			);
		}

		Donya::Vector4x4 V = Donya::Vector4x4::FromMatrix( pCamera->CalcViewMatrix() );
		Donya::Vector4x4 P = Donya::Vector4x4::FromMatrix( pCamera->GetProjectionMatrix() );

		Donya::Vector4 cameraPos{ pCamera->GetPos(), 1.0f };

		player.Draw( V, P, cameraPos, light.direction, light.color, mtlColor );

		{
			static auto					textureBoard = Donya::Geometric::CreateTextureBoard( L"./Data/Images/Rights/FMOD Logo White - Black Background.png" );
			static Donya::Vector2		texPos{};
			static Donya::Vector2		texSize{ 728.0f, 192.0f };

			static Donya::Vector3		boardScale{ 1.0f, 1.0f, 1.0f };
			static Donya::Vector3		boardPos{};
			static Donya::Quaternion	boardPose{};

		#if USE_IMGUI

			if ( ImGui::BeginIfAllowed() )
			{
				if ( ImGui::TreeNode( u8"板ポリ描画テスト" ) )
				{
					ImGui::DragFloat2( u8"切り取り位置・左上",	&texPos.x		);
					ImGui::DragFloat2( u8"切り取りサイズ・全体",	&texSize.x		);
					ImGui::Text( "" );

					ImGui::DragFloat3( u8"スケール",		&boardScale.x			);
					ImGui::DragFloat3( u8"ワールド位置",	&boardPos.x				);

					static Donya::Vector3 look{ 0.0f, 0.0f, 1.0f };
					static bool freezeY = true;
					ImGui::SliderFloat3( u8"向き",		&look.x	, -1.0f, 1.0f	);
					ImGui::Checkbox( u8"Ｙ軸固定",		&freezeY );

					boardPose = boardPose.LookAt
					(
						look.Normalized(),
						( freezeY )
						? Donya::Quaternion::Freeze::Up
						: Donya::Quaternion::Freeze::None
					);
					boardPose.Normalize();
					ImGui::Text( "[X:%5.2f][Y:%5.2f][Z:%5.2f][W:%5.2f]", boardPose.x, boardPose.y, boardPose.z, boardPose.w );
					if ( ImGui::Button( u8"姿勢リセット" ) )
					{
						boardPose = {};
					}

					ImGui::TreePop();
				}

				ImGui::End();
			}

		#endif // USE_IMGUI

			Donya::Vector4x4 TB_S = Donya::Vector4x4::MakeScaling( boardScale );
			Donya::Vector4x4 TB_R = boardPose.RequireRotationMatrix();
			Donya::Vector4x4 TB_T = Donya::Vector4x4::MakeTranslation( boardPos );
			Donya::Vector4x4 TB_W = TB_S * TB_R * TB_T;

			textureBoard.RenderPart
			(
				texPos, texSize,
				nullptr, true, true,
				( TB_W * V * P ), TB_W,
				light.direction, mtlColor
			);
		}
	}
private:
	void CameraUpdate()
	{
		auto MakeControlStructWithMouse = []()
		{
			if ( !Donya::Keyboard::Press( VK_MENU ) )
			{
				Donya::Camera::Controller noop{};
				noop.SetNoOperation();
				return noop;
			}

			static Donya::Int2 prevMouse{};
			static Donya::Int2 currMouse{};

			prevMouse = currMouse;

			auto nowMouse = Donya::Mouse::Coordinate();
			currMouse.x = scast<int>( nowMouse.x );
			currMouse.y = scast<int>( nowMouse.y );

			bool isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
			bool isDriveMouse = ( prevMouse != currMouse ) || Donya::Mouse::WheelRot() || isInputMouseButton;
			if ( !isDriveMouse )
			{
				Donya::Camera::Controller noop{};
				noop.SetNoOperation();
				return noop;
			}

			Donya::Vector3 diff{};
			{
				Donya::Vector2 vec2 = ( currMouse - prevMouse ).Float();

				diff.x = vec2.x;
				diff.y = vec2.y;
			}

			Donya::Vector3 movement{};
			Donya::Vector3 rotation{};

			if ( Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) )
			{
				constexpr float ROT_AMOUNT = ToRadian( 1.0f );
				rotation.x = diff.x * ROT_AMOUNT;
				rotation.y = diff.y * ROT_AMOUNT;
			}
			else
			if ( Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) )
			{
				constexpr float MOVE_SPEED = 0.1f;
				movement.x = diff.x * MOVE_SPEED;
				movement.y = -diff.y * MOVE_SPEED;
			}

			constexpr float FRONT_SPEED = 3.5f;
			movement.z = FRONT_SPEED * scast<float>( Donya::Mouse::WheelRot() );

			Donya::Quaternion rotYaw = Donya::Quaternion::Make( Donya::Vector3::Up(), rotation.x );

			Donya::Vector3 right = Donya::Vector3::Right();
			right = rotYaw.RotateVector( right );
			Donya::Quaternion rotPitch = Donya::Quaternion::Make( right, rotation.y );

			Donya::Quaternion rotQ = rotYaw * rotPitch;

			static Donya::Vector3 front = Donya::Vector3::Front();

			if ( !rotation.IsZero() )
			{
				front = rotQ.RotateVector( front );
				front.Normalize();
			}

			Donya::Camera::Controller ctrl{};
			ctrl.moveVelocity = movement;
			ctrl.rotation = rotation;
			ctrl.slerpPercent = 1.0f;
			ctrl.moveAtLocalSpace = true;

			return ctrl;
		};
		pCamera->Update( MakeControlStructWithMouse() );
		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			pCamera->SetPosition( { 0.0f, 360.0f, -512.0f } );
		}
	}
};

SceneChiProject::SceneChiProject() : pImpl( std::make_unique<Impl>() )
{
}
SceneChiProject::~SceneChiProject()
{
	pImpl.reset( nullptr );
}

void SceneChiProject::Init()
{
	pImpl->Init();
}
void SceneChiProject::Uninit()
{
	pImpl->Uninit();
}

Scene::Result SceneChiProject::Update( float elapsedTime )
{
	pImpl->Update( elapsedTime );

	return ReturnResult();
}

void SceneChiProject::Draw( float elapsedTime )
{
	pImpl->Draw( elapsedTime );
}

Scene::Result SceneChiProject::ReturnResult()
{
	if ( Donya::Keyboard::Press( VK_RSHIFT ) && Donya::Keyboard::Trigger( VK_RETURN ) )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::ChiProject;
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}


#define FOR_PRESENTATION_AMUSEMENT ( false )
#if FOR_PRESENTATION_AMUSEMENT

#include "Header/Vector.h"
#include "Header/Quaternion.h"

constexpr Donya::Vector3 Mul( const Donya::Vector3 &V, const Donya::Vector4x4 &M )
{
	return Donya::Vector3
	{
		( V.x * M._11 ) + ( V.y * M._21 ) + ( V.z * M._31 ),
		( V.x * M._12 ) + ( V.y * M._22 ) + ( V.z * M._32 ),
		( V.x * M._13 ) + ( V.y * M._23 ) + ( V.z * M._33 )
	};
}

void TestPlace()
{











	constexpr Donya::Quaternion	semiRotationY{ 0.0f, 1.0f, 0.0f, 0.0f };
	constexpr Donya::Vector3	front{ 0.0f, 0.0f, 1.0f };

	constexpr Donya::Vector3	rotated = semiRotationY.RotateVector( front );







	constexpr Donya::Vector4x4	rotationMatrix = semiRotationY.RequireRotationMatrix();

	constexpr Donya::Vector3	position{ 15.0f, 10.0f, 5.0f };
	constexpr Donya::Vector3	multiplied = Mul( position, rotationMatrix );























}

#endif // FOR_PRESENTATION_AMUSEMENT
