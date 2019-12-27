#pragma once

#include "Donya/Camera.h"
#include "Donya/GamepadXInput.h"
#include "Donya/Vector.h"
#include "Donya/UseImGui.h"

#include "Gimmicks.h"
#include "Player.h"
#include "Scene.h"

class SceneEditor : public Scene
{
private:
	Donya::XInput			controller;
	Donya::ICamera			iCamera;

	Player					player;
	Gimmick					gimmicks;

	Scene::Type				nextSceneType;

public:
	SceneEditor();
	~SceneEditor();
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;

private:
	void	StartFade() const;

private:
	void	CameraInit();
	void	CameraUpdate();

private:
	Result	ReturnResult();
#if USE_IMGUI

	void	UseImGui();

#endif // USE_IMGUI
};

