#pragma once

#include <memory>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/GamepadXInput.h"
#include "Donya/Vector.h"
#include "Donya/UseImGui.h"

#include "Gimmicks.h"
#include "Hook.h"
#include "Player.h"
#include "Scene.h"

class SceneGame : public Scene
{
public:
	struct DirectionalLight
	{
		Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Donya::Vector4 dir	{ 0.0f,-1.0f, 1.0f, 0.0f };
	};
private:
	int						stageCount;		// 1-based.
	int						currentStageNo;	// 0-based.

	DirectionalLight		dirLight;
	Donya::ICamera			iCamera;
	Donya::XInput			controller;
	Donya::Vector2			roomOriginPos;	// Center. Screen space.

	Player					player;
	std::vector<Gimmick>	gimmicks;

	std::unique_ptr<Hook>	pHook;

	bool					useCushion;		// Use for digest an elapsedTime when after initialize.
public:
	SceneGame();
	~SceneGame();
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	void	LoadAllStages();

	// The Y axis is screen space.
	Donya::Int2 CalcRoomIndex( int stageNo ) const;

	void	CameraInit();
	void	CameraUpdate();

	void	PlayerUpdate( float elapsedTime );
	bool	IsPlayerOutFromRoom() const;
	void	UpdateCurrentStage();
	void	HookUpdate( float elapsedTime );

	bool	DetectClearMoment() const;

	void	StartFade() const;
private:
	Result	ReturnResult();
private:
#if USE_IMGUI

	void	UseImGui();

#endif // USE_IMGUI
};
