#pragma once

#include <memory>
#include <vector>

#include "Donya/Camera.h"
#include "Donya/GamepadXInput.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "BG.h"
#include "DerivedCollision.h"
#include "Gimmicks.h"
#include "Hook.h"
#include "Player.h"
#include "Scene.h"
#include "Terrain.h"
#include "Alert.h"

/// <summary>
/// The "Stage" and "room" means is same.
/// </summary>

class SceneGame : public Scene
{
private:
	enum TutorialState
	{
		Jump = 0,
		Extend,
		Make,
		Pull,
		Erase,
	};

private:
	int						stageCount;		// 1-based.
	int						currentStageNo;	// 0-based.

	Donya::ICamera			iCamera;
	Donya::XInput			controller;
	Donya::Vector2			roomOriginPos;	// Center. Screen space.
	
	size_t					idMission;		// Sprite.
	size_t					idComplete;		// Sprite.
	size_t					idTitleText;	// Sprite.
	size_t					idTitleGear;	// Sprite.
	size_t					idTutorial;		// Sprite.

	BG						bg;
	Player					player;
	Alert					alert;
	std::unique_ptr<Hook>	pHook;

	std::vector<Terrain>	terrains;		// The terrains per room.
	std::vector<Gimmick>	gimmicks;		// The gimmicks per room.

	std::vector<int>		elevatorRoomIndices; // Cache the indices of room that has the elevator.

	TutorialState			tutorialState;	// This variable controll drawing texts of tutorial.
	bool					nowTutorial;	// Do you doing tutorial now?
	bool					enableAlert;	// Will be true when the player ariived at the last room.
	bool					nowCleared;
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

	std::vector<BoxEx> FetchElevatorHitBoxes() const;

	// The Y axis is screen space.
	Donya::Int2 CalcRoomIndex( int stageNo ) const;

	void	CameraInit();
	void	CameraUpdate();
	void	MoveCamera();

	void	PlayerUpdate( float elapsedTime );
	void	PlayerPhysicUpdate( const std::vector<BoxEx> &hitBoxes );

	bool	IsPlayerOutFromRoom() const;
	void	UpdateCurrentStage();

	bool	InLastStage() const;

	void	HookUpdate( float elapsedTime );

	bool	DetectClearMoment() const;

	void	StartFade() const;
	void	PrepareGoToTitle();

	void	UpdateOfTutorial();
	void	DrawOfTutorial();
private:
	Result	ReturnResult();
private:
#if USE_IMGUI

	void	UseImGui();

#endif // USE_IMGUI
};
