#pragma once

#include "Donya/GamepadXInput.h"
#include "Donya/UseImgui.h"


#include "Animation.h"
#include "Scene.h"

class ScenePause : public Scene
{
private:
	enum class Choice
	{
		Nil = -1,
		Resume = 0,
		BackToTitle,
		Retry,

		ItemCount
	};
private:
	Choice			choice;
	SpriteSheet		sprBoard;
	SpriteSheet		sprSentences;
	Donya::XInput	controller;

	Scene::Type		nextSceneType;
public:
	ScenePause();
	~ScenePause();
public:
	void	Init();
	void	Uninit();

	Result	Update( float elapsedTime );

	void	Draw( float elapsedTime );
private:
	void	UpdateChooseItem();
	void	StartFade() const;

	Result	ReturnResult();

#if USE_IMGUI
	void UseImGui();
#endif
};
