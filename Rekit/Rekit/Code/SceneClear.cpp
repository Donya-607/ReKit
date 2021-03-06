#include "SceneClear.h"

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

SceneClear::SceneClear() :
	controller( Donya::Gamepad::PAD_1 )
{
}
SceneClear::~SceneClear() = default;

void SceneClear::Init()
{
	Donya::Sound::Play( Music::BGM_Clear );
}
void SceneClear::Uninit()
{
	Donya::Sound::Stop( Music::BGM_Clear );
}

Scene::Result SceneClear::Update( float elapsedTime )
{
#if USE_IMGUI

	UseImGui();

#endif // USE_IMGUI

	controller.Update();

#if DEBUG_MODE
	// Scene Transition Demo.
	{
		if ( Donya::Keyboard::Trigger( 'R' ) )
		{
			if ( !Fader::Get().IsExist() )
			{
				nextSceneType = Scene::Type::Game;
				StartFade();
			}
		}
		else if ( Donya::Keyboard::Trigger( 'T' ) )
		{
			if (!Fader::Get().IsExist())
			{
				nextSceneType = Scene::Type::Title;
				StartFade();
			}
		}
	}
#endif // DEBUG_MODE

	return ReturnResult();
}

void SceneClear::Draw( float elapsedTime )
{
	{
		constexpr FLOAT BG_COLOR[4]{ 0.8f, 0.8f, 0.8f, 1.0f };
		Donya::ClearViews( BG_COLOR );
	}
}

void SceneClear::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneClear::ReturnResult()
{
#if DEBUG_MODE

	bool pressCtrl =  Donya::Keyboard::Press( VK_LCONTROL ) || Donya::Keyboard::Press( VK_RCONTROL );
	if ( pressCtrl && Donya::Keyboard::Trigger( VK_RETURN ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::ItemDecision );

		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Title;
		return change;
	}
	// else

#endif // DEBUG_MODE

	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = nextSceneType;
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI

void SceneClear::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"クリア・設定" ) )
		{


			ImGui::TreePop();
		}

		ImGui::Text(u8"リトライ画面へ : <Press R>");
		ImGui::Text(u8"タイトル画面へ : <Press T>");

		ImGui::End();
	}
}

#endif // USE_IMGUI
