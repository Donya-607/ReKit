#include "ScenePause.h"

#include <algorithm>

#include "Donya/Constant.h"
#include "Donya/Keyboard.h"
#include "Donya/Sprite.h"
#include "Donya/Sound.h"
#include "Donya/Vector.h"

#include "Common.h"
#include "Fader.h"
#include "FilePath.h"
#include "Music.h"

#undef max
#undef min

ScenePause::ScenePause() :
	choice( Choice::Resume ),
	sprUI(),
	nextSceneType(),
	controller( Donya::XInput::PadNumber::PAD_1 )
{}
ScenePause::~ScenePause() = default;

void ScenePause::Init()
{
	// sprUI.LoadSheet( GetSpritePath( SpriteAttribute::UI ), 256U );
}

void ScenePause::Uninit()
{

}

Scene::Result ScenePause::Update( float elapsedTime )
{
	controller.Update();

	UpdateChooseItem();

#if DEBUG_MODE
	// Scene Transition Demo.
	{
		if (Donya::Keyboard::Trigger('R'))
		{
			if (!Fader::Get().IsExist())
			{
				nextSceneType = Scene::Type::Game;
				StartFade();
			}
		}
		else if (Donya::Keyboard::Trigger('T'))
		{
			if (!Fader::Get().IsExist())
			{
				nextSceneType = Scene::Type::Title;
				StartFade();
			}
		}
	}
#endif // DEBUG_MODE

#if USE_IMGUI
	UseImGui();
#endif

	return ReturnResult();
}

void ScenePause::Draw( float elapsedTime )
{
	
}

void ScenePause::UpdateChooseItem()
{
	bool left{}, right{};
	left  = controller.Trigger( Donya::Gamepad::Button::LEFT );
	right = controller.Trigger( Donya::Gamepad::Button::RIGHT );
	if ( !left )
	{
		left  = controller.TriggerStick( Donya::Gamepad::StickDirection::LEFT );
	}
	if ( !right )
	{
		right = controller.TriggerStick( Donya::Gamepad::StickDirection::RIGHT );
	}

	int index = scast<int>( choice );
	int oldIndex = index;

	if ( left ) { index--; }
	if ( right ) { index++; }

	index = std::max( 0, std::min( scast<int>( Choice::ReTry ), index ) );

	if ( index != oldIndex )
	{
		Donya::Sound::Play( Music::ItemChoose );
	}

	choice = scast<Choice>( index );
}

void ScenePause::StartFade() const
{
	Fader::Configuration config{};
	config.type = Fader::Type::Gradually;
	config.closeFrame = Fader::GetDefaultCloseFrame();
	config.SetColor(Donya::Color::Code::BLACK);
	Fader::Get().StartFadeOut(config);
}

Scene::Result ScenePause::ReturnResult()
{
	// ポーズ解除
	if ( Donya::Keyboard::Trigger( 'P' ) || controller.Trigger( Donya::Gamepad::Button::START ) )
	{
		Donya::Sound::Play( Music::ItemDecision );

		Scene::Result change{};
		change.AddRequest( Scene::Request::REMOVE_ME );
		return change;
	}

	// シーン変更
	Donya::Sound::Play(Music::ItemDecision);

	if (Fader::Get().IsClosed())
	{
		Scene::Result change{};
		change.sceneType = nextSceneType;
		change.AddRequest(Scene::Request::ADD_SCENE, Scene::Request::REMOVE_ALL);
		return change;
	}

	Scene::Result noop{ Scene::Request::NONE, Scene::Type::Null };
	return noop;
}

#if USE_IMGUI
void ScenePause::UseImGui()
{
	if (ImGui::BeginIfAllowed())
	{
		if (ImGui::TreeNode(u8"ポーズ・設定"))
		{

			ImGui::TreePop();
		}
		ImGui::Text(u8"ゲーム画面へ : <Press P>");
		ImGui::Text(u8"リトライへ : <Press R>");
		ImGui::Text(u8"タイトルへ : <Press T>");
		ImGui::End();
	}
}
#endif