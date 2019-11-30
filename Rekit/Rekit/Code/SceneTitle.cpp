#include "SceneTitle.h"

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

SceneTitle::SceneTitle() :
	controller( Donya::Gamepad::PAD_1 )
{}
SceneTitle::~SceneTitle() = default;

void SceneTitle::Init()
{
	Donya::Sound::Play( Music::BGM_Title );
}
void SceneTitle::Uninit()
{
	Donya::Sound::Stop( Music::BGM_Title );
}

Scene::Result SceneTitle::Update( float elapsedTime )
{
#if USE_IMGUI

	UseImGui();

#endif // USE_IMGUI

	controller.Update();

#if DEBUG_MODE
	// Scene Transition Demo.
	{
		if ( Donya::Keyboard::Trigger( VK_RETURN ) || controller.Trigger( Donya::Gamepad::Button::A ) || controller.Trigger( Donya::Gamepad::Button::START ) )
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

void SceneTitle::Draw( float elapsedTime )
{
	{
		constexpr FLOAT BG_COLOR[4]{ 0.3f, 1.0f, 0.7f, 1.0f };
		Donya::ClearViews( BG_COLOR );
	}
}

void SceneTitle::StartFade() const
{
	Fader::Configuration config{};
	config.type			= Fader::Type::Gradually;
	config.closeFrame	= Fader::GetDefaultCloseFrame();;
	config.SetColor( Donya::Color::Code::BLACK );
	Fader::Get().StartFadeOut( config );
}

Scene::Result SceneTitle::ReturnResult()
{
#if DEBUG_MODE

	bool pressCtrl =  Donya::Keyboard::Press( VK_LCONTROL ) || Donya::Keyboard::Press( VK_RCONTROL );
	if ( pressCtrl && Donya::Keyboard::Trigger( VK_RETURN ) && !Fader::Get().IsExist() )
	{
		Donya::Sound::Play( Music::ItemDecision );

		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Game;
		return change;
	}
	// else

#endif // DEBUG_MODE

	if ( Fader::Get().IsClosed() )
	{
		Scene::Result change{};
		change.AddRequest( Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ME );
		change.sceneType = Scene::Type::Game;
		return change;
	}
	// else

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI

void SceneTitle::UseImGui()
{
	if ( ImGui::BeginIfAllowed() )
	{
		if ( ImGui::TreeNode( u8"タイトル・設定" ) )
		{


			ImGui::TreePop();
		}

		ImGui::End();
	}
}

#endif // USE_IMGUI
