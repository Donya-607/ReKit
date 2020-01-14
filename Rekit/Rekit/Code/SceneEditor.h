#pragma once


#include "Donya/Camera.h"
#include "Donya/GamepadXInput.h"
#include "Donya/Vector.h"
#include "Donya/UseImGui.h"

#include "Gimmicks.h"
#include "Player.h"
#include "Scene.h"

enum class SelectGimmick
{
	Normal = 0,
	Fragile,
	Hard,
	TriggerKey,
	TriggerSwitch,
	TriggerPull,
	Ice,

	GimmicksCount
};

class SceneEditor : public Scene
{
private:
	struct DirectionalLight
	{
		Donya::Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Donya::Vector4 dir{ 0.0f,-1.0f, 1.0f, 0.0f };
	};

private:
	Donya::XInput			controller;
	Donya::ICamera			iCamera;
	DirectionalLight		dirLight;

	Player					player;
	Gimmick					gimmicks;

	Donya::Geometric::Line	line;
	Donya::Vector2			mousePos;


	Scene::Type				nextSceneType;



	bool					isPressG;

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

	// Edit Function
	void GenerateBlockIfCleck();
	void EraseBlockIfRightCleck();
	void CorrectionGridCursor();
	void SaveEditParameter();

private:
	Result	ReturnResult();

	Donya::Vector3* CalcScreenToWorld(
		Donya::Vector3* pout,
		int Sx, int Sy,
		float fZ,
		int screenWidth, int screenHeight,
		Donya::Vector4x4 view,
		Donya::Vector4x4 projection
		);
	Donya::Vector3* CalcScreenToXY(
		Donya::Vector3* pout,
		int Sx, int Sy,
		int screenWidth, int screenHeight,
		Donya::Vector4x4 view,
		Donya::Vector4x4 projection
	);

#if USE_IMGUI

	void	UseImGui();

#endif // USE_IMGUI
};

