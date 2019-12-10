#include "SceneGame.h"

#include <vector>

#include "Donya/Camera.h"
#include "Donya/CBuffer.h"
#include "Donya/Constant.h"
#include "Donya/Donya.h"		// Use GetFPS().
#include "Donya/GeometricPrimitive.h"
#include "Donya/Keyboard.h"
#include "Donya/Mouse.h"
#include "Donya/Quaternion.h"
#include "Donya/Sound.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Common.h"
#include "Fader.h"
#include "Music.h"

using namespace DirectX;

#if DEBUG_MODE
namespace
{
	static std::vector<Donya::Box> debugTestTerrains{};
}
#endif // DEBUG_MODE

SceneGame::SceneGame() :
	dirLight(),
	player(), gimmicks(),
	iCamera(),
	controller( Donya::Gamepad::PAD_1 )
{}
SceneGame::~SceneGame() = default;

void SceneGame::Init()
{
	Donya::Sound::Play( Music::BGM_Game );

	CameraInit();

#if DEBUG_MODE
	gimmicks.Init( NULL );
#endif // DEBUG_MODE

	player.Init();
//	hook.Init();
}
void SceneGame::Uninit()
{
	Donya::Sound::Stop( Music::BGM_Game );

	gimmicks.Uninit();

	player.Uninit();
//	hook.Uninit();
}

Scene::Result SceneGame::Update( float elapsedTime )
{
#if USE_IMGUI

	UseImGui();

#endif // USE_IMGUI

	controller.Update();

	gimmicks.Update( elapsedTime );

	// This update does not call PhysicUpdate().
	PlayerUpdate( elapsedTime );

#if DEBUG_MODE
	// Collision Test.
	{
		constexpr float OFFSET	= 15.0f;
		constexpr float SIZE	= 128.0f;

		const std::vector<Donya::Box> terrainList
		{
			Donya::Box{ 0.0f,-OFFSET, SIZE, 1.0f, true },
			Donya::Box{ 0.0f, OFFSET, SIZE, 1.0f, true },
			Donya::Box{-OFFSET, 0.0f, 1.0f, SIZE, true },
			Donya::Box{ OFFSET, 0.0f, 1.0f, SIZE, true },
			Donya::Box{ 0.0f,   0.0f, 1.0f, 1.0f, true },
			Donya::Box{ 2.0f,  -2.0f, 1.0f, 1.0f, true },
			Donya::Box{ 4.0f,  -4.0f, 1.0f, 1.0f, true },
		};
		debugTestTerrains = terrainList;

		static Donya::Box changeable{ -4.0f, 0.0f, 2.0f, 2.0f, true };
	#if USE_IMGUI
		if ( ImGui::BeginIfAllowed() )
		{
			if ( ImGui::TreeNode( u8"当たり判定テスト" ) )
			{
				ImGui::DragFloat2( u8"位置", &changeable.pos.x );
				ImGui::DragFloat2( u8"半分のサイズ", &changeable.size.x );

				ImGui::TreePop();
			}
			ImGui::End();
		}
	#endif // USE_IMGUI
		debugTestTerrains.emplace_back( changeable );

		// Add the gimmicks block.
		{
			auto ToBox = []( const Donya::AABB &aabb )
			{
				Donya::Box box{};
				box.pos.x  = aabb.pos.x;
				box.pos.y  = aabb.pos.y;
				box.size.x = aabb.size.x;
				box.size.y = aabb.size.y;
				box.exist  = aabb.exist;
				return box;
			};

			const auto boxes = gimmicks.RequireHItBoxes();
			for ( const auto &it : boxes )
			{
				debugTestTerrains.emplace_back( ToBox( it ) );
			}
		}

		player.PhysicUpdate( debugTestTerrains );
//		hook.PhysicUpdate( debugTestTerrains );
	}
#endif // DEBUG_MODE

	CameraUpdate();

#if DEBUG_MODE
	// Scene Transition Demo.
	{
		bool pressCtrl = Donya::Keyboard::Press( VK_LCONTROL ) || Donya::Keyboard::Press( VK_RCONTROL );
		bool triggerDebugButton = ( Donya::Keyboard::Trigger( VK_RETURN ) || controller.Trigger( Donya::Gamepad::Button::A ) || controller.Trigger( Donya::Gamepad::Button::START ) );
		if ( pressCtrl && triggerDebugButton )
		{
			if ( !Fader::Get().IsExist() )
			{
				StartFade();
			}
		}
	}
#endif // DEBUG_MODE

	return ReturnResult();
}

void SceneGame::Draw( float elapsedTime )
{
	{
		constexpr FLOAT BG_COLOR[4]{ 0.4f, 0.4f, 0.4f, 1.0f };
		Donya::ClearViews( BG_COLOR );
	}

	const Donya::Vector4x4 V = iCamera.CalcViewMatrix();
	const Donya::Vector4x4 P = iCamera.GetProjectionMatrix();
	const Donya::Vector4 cameraPos{ iCamera.GetPosition(), 1.0f };

	gimmicks.Draw( V, P, dirLight.dir );

	player.Draw( V * P, dirLight.dir, dirLight.color );
//	hook.Draw( V * P, dirLight.dir, dirLight.color );

#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		// Drawing Test Terrains that use to player's collision.
		{
			static auto cube = Donya::Geometric::CreateCube();

			constexpr Donya::Vector4 cubeColor{ 1.0f, 0.8f, 0.0f, 0.6f };
			Donya::Vector4x4 cubeT{};
			Donya::Vector4x4 cubeS{};
			Donya::Vector4x4 cubeW{};
			for ( const auto &it : debugTestTerrains )
			{
				// The drawing size is whole size.
				// But a collision class's size is half size.
				// So we should to double it size.

				cubeT = Donya::Vector4x4::MakeTranslation( Donya::Vector3{ it.pos, 0.0f } );
				cubeS = Donya::Vector4x4::MakeScaling( Donya::Vector3{ it.size * 2.0f, 1.0f } );
				cubeW = cubeS * cubeT;

				cube.Render
				(
					nullptr,
					/* useDefaultShading	= */ true,
					/* isEnableFill			= */ true,
					( cubeW * V * P ), cubeW,
					dirLight.dir,
					cubeColor
				);
			}
		}

		// Drawing TextureBoard Demo.
		{
			constexpr const wchar_t *texturePath	= L"./Data/Images/Rights/FMOD Logo White - Black Background.png";
			static Donya::Geometric::TextureBoard	texBoard = Donya::Geometric::CreateTextureBoard( texturePath );
			static Donya::Vector2	texPos{};
			static Donya::Vector2	texSize{ 728.0f, 192.0f };

			static Donya::Vector3	boardScale{ 1.0f, 1.0f, 1.0f };
			static Donya::Vector3	boardPos{};
			static float			boardRadian{};

			static Donya::Vector4	boardColor{ 1.0f, 1.0f, 1.0f, 1.0f };
			static Donya::Vector4	lightDir  { 0.0f,-1.0f, 1.0f, 0.0f };

		#if USE_IMGUI

			if ( ImGui::BeginIfAllowed() )
			{
				if ( ImGui::TreeNode( u8"板ポリ描画テスト" ) )
				{
					ImGui::DragFloat2( u8"切り取り位置・左上", &texPos.x );
					ImGui::DragFloat2( u8"切り取りサイズ・全体", &texSize.x );
					ImGui::Text( "" );
					ImGui::DragFloat3( u8"スケール", &boardScale.x );
					ImGui::DragFloat3( u8"ワールド位置", &boardPos.x );
					ImGui::DragFloat( u8"Z回転", &boardRadian, ToRadian( 10.0f ) );
					ImGui::Text( "" );
					ImGui::ColorEdit4( u8"ブレンド色", &boardColor.x );
					ImGui::SliderFloat3( u8"板ポリのライト方向", &lightDir.x, -1.0f, 1.0f );
					ImGui::Text( "" );

					ImGui::TreePop();
				}

				ImGui::End();
			}

		#endif // USE_IMGUI

			Donya::Vector4x4 TB_S = Donya::Vector4x4::MakeScaling( boardScale );
			Donya::Vector4x4 TB_R = texBoard.CalcBillboardRotation( ( iCamera.GetPosition() - boardPos ).Normalized(), boardRadian );
			Donya::Vector4x4 TB_T = Donya::Vector4x4::MakeTranslation( boardPos );
			Donya::Vector4x4 TB_W = TB_S * TB_R * TB_T;

			texBoard.RenderPart
			(
				texPos, texSize,
				nullptr, // Specify use library's device-context.
				/* useDefaultShading = */ true,
				/* isEnableFill      = */ true,
				( TB_W * V * P ), TB_W,
				lightDir, boardColor
			);
		}
	}
#endif // DEBUG_MODE
}

void SceneGame::CameraInit()
{
	iCamera.Init( Donya::ICamera::Mode::Look );
	iCamera.SetZRange( 0.1f, 1000.0f );
	iCamera.SetFOV( ToRadian( 30.0f ) );
	iCamera.SetScreenSize( { Common::ScreenWidthF(), Common::ScreenHeightF() } );
	iCamera.SetPosition( { 0.0f, 0.0f, -48.0f } );
	iCamera.SetFocusPoint( { 0.0f, 0.0f, 0.0f } );
	iCamera.SetProjectionPerspective();

	// I can setting a configuration,
	// but current data is not changed by this immediately.
	// So update here.

	Donya::ICamera::Controller moveInitPoint{};
	moveInitPoint.SetNoOperation();
	moveInitPoint.slerpPercent = 1.0f;
	iCamera.Update( moveInitPoint );
}
void SceneGame::CameraUpdate()
{
	auto MakeControlStructWithMouse = []()
	{
		constexpr float SLERP_FACTOR = 0.2f;

		auto NoOperation = [&SLERP_FACTOR]()
		{
			Donya::ICamera::Controller noop{};
			noop.SetNoOperation();
			noop.slerpPercent = SLERP_FACTOR;
			return noop;
		};

		if ( !Donya::Keyboard::Press( VK_MENU ) ) { return NoOperation(); }
		// else

		static Donya::Int2 prevMouse{};
		static Donya::Int2 currMouse{};

		prevMouse = currMouse;

		auto nowMouse = Donya::Mouse::Coordinate();
		currMouse.x = scast<int>( nowMouse.x );
		currMouse.y = scast<int>( nowMouse.y );

		bool isInputMouseButton = Donya::Mouse::Press( Donya::Mouse::Kind::LEFT ) || Donya::Mouse::Press( Donya::Mouse::Kind::MIDDLE ) || Donya::Mouse::Press( Donya::Mouse::Kind::RIGHT );
		bool isDriveMouse = ( prevMouse != currMouse ) || Donya::Mouse::WheelRot() || isInputMouseButton;
		if ( !isDriveMouse ) { return NoOperation(); }
		// else

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

		Donya::ICamera::Controller ctrl{};
		ctrl.moveVelocity		= movement;
		ctrl.yaw				= rotation.x;
		ctrl.pitch				= rotation.y;
		ctrl.roll				= 0.0f;
		ctrl.slerpPercent		= SLERP_FACTOR;
		ctrl.moveInLocalSpace	= true;

		return ctrl;
	};
	iCamera.Update( MakeControlStructWithMouse() );

#if DEBUG_MODE
	if ( Donya::Keyboard::Press( VK_MENU ) )
	{
		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			iCamera.SetPosition( { 0.0f, 0.0f, -5.0f } );
		}
	}
#endif // DEBUG_MODE
}

void SceneGame::PlayerUpdate( float elapsedTime )
{
	Player::Input input{};

	bool moveLeft	= false;
	bool moveRight	= false;
	bool useJump	= false;

	if ( controller.IsConnected() )
	{
		using Pad  = Donya::Gamepad;

		bool left  = controller.Press( Pad::LEFT  ) || controller.PressStick( Pad::StickDirection::LEFT,  /* leftStick = */ true );
		bool right = controller.Press( Pad::RIGHT ) || controller.PressStick( Pad::StickDirection::RIGHT, /* leftStick = */ true );

		if ( left  ) { moveLeft  = true; }
		if ( right ) { moveRight = true; }

		if ( controller.Trigger( Pad::A  ) ) { useJump = true; }
	}
	else
	{
		if ( Donya::Keyboard::Press( VK_LEFT  ) )		{ moveLeft  = true; }
		if ( Donya::Keyboard::Press( VK_RIGHT ) )		{ moveRight = true; }
		
		if ( Donya::Keyboard::Trigger( VK_LSHIFT ) )	{ useJump = true; }
	}

	if ( moveLeft  ) { input.moveVelocity.x -= 1.0f; }
	if ( moveRight ) { input.moveVelocity.x += 1.0f; }
	if ( useJump   ) { input.useJump = true; }

	player.Update( elapsedTime, input );
}

void SceneGame::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneGame::ReturnResult()
{
	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Clear;
		return change;
	}

	bool requestPause	= controller.Trigger( Donya::Gamepad::Button::START ) || controller.Trigger( Donya::Gamepad::Button::SELECT ) || Donya::Keyboard::Trigger( 'P' );
	bool allowPause		= !Fader::Get().IsExist();
	if ( requestPause && allowPause )
	{
		Donya::Sound::Play( Music::ItemDecision );

		Scene::Result pause{};
		pause.AddRequest( Scene::Request::ADD_SCENE );
		pause.sceneType = Scene::Type::Pause;
		return pause;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI

void SceneGame::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"ゲーム・設定" ) )
		{
			ImGui::SliderFloat3( u8"方向性ライト・向き", &dirLight.dir.x, -1.0f, 1.0f );
			ImGui::ColorEdit4( u8"方向性ライト・カラー", &dirLight.color.x );

			ImGui::TreePop();
		}
		ImGui::Text(u8"ポーズ画面へ : <Press P>");
		ImGui::Text(u8"クリア画面へ : <Press Ctrl + Enter>");
		ImGui::End();
	}
}

#endif // USE_IMGUI
