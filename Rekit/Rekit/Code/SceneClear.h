#pragma once

#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"

#include "Scene.h"

class SceneClear : public Scene
{
private:
	Donya::XInput	controller;

	Scene::Type		nextSceneType;
public:
	SceneClear();
	~SceneClear();
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI

	void	UseImGui();

#endif // USE_IMGUI
};
