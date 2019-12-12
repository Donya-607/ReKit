#pragma once

#include <vector>

#include "Donya/CBuffer.h"
#include "Donya/Collision.h"
#include "Donya/GeometricPrimitive.h"
#include "Donya/Shader.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "DerivedCollision.h"

class Hook
{
public:
	struct Input
	{
		Donya::Vector3	playerPos{};	// World space.
		Donya::Vector2	stickVec{};		// スティック前に倒したら上行く(Y+ == ↑)
		bool			currPress{};	// Current button state : [TRUE:Pressed] [FALSE:Not pressed]
	};
private:
	struct Constants
	{
		DirectX::XMFLOAT4X4	worldViewProjection;
		DirectX::XMFLOAT4X4	world;
		DirectX::XMFLOAT4	lightDirection;
		DirectX::XMFLOAT4	lightColor;
		DirectX::XMFLOAT4	materialColor;
	};
private:
	enum ActionState
	{
		Throw = 0,		// 投げる
		Stay,			// 待機
		Pull,			// 引き戻す
		Erase,			// 引っかかっている場合、消す
		End				// delete
	};

	Donya::Vector3						pos;				// World space.
	Donya::Vector3						velocity;
	Donya::Vector3						direction;
	ActionState							state;

	float								easingTime;
	float								distance;			// playerとhookの距離
	float								momentPullDist;		// 引かれた瞬間のplayerとhookの距離
	bool								prevPress;			// Previous button state : [TRUE:Pressed] [FALSE:Not pressed]
	bool								exist;

	static Donya::Geometric::Cube		drawModel;
	static Donya::CBuffer<Constants>	cbuffer;
	static Donya::VertexShader			VSDemo;
	static Donya::PixelShader			PSDemo;
public:
	Hook(const Donya::Vector3& playerPos);
	~Hook();
public:
	static void Init();
	static void Uninit();

	void Update(float elpasedTime, Input controller);
	void PhysicUpdate(const std::vector<BoxEx>& terrains, const Donya::Vector3& playerPos);

	void Draw(const Donya::Vector4x4& matViewProjection, const Donya::Vector4& lightDirection, const Donya::Vector4& lightColor) const;
public:
	bool IsExist() const { return exist; }

	/// <summary>
	/// Returns position is world space.
	/// </summary>
	Donya::Vector3 GetPosition() const;
	Donya::Vector3 GetVelocity() const;
private:
	static void CreateRenderingObjects();

	void ThrowUpdate(float elapsedTime, Input controller);
	void PullUpdate(float elapsedTime, Input controller);
	void EraseUpdate(float elapsedTime, Input controller);

#if USE_IMGUI
public:

	static void UseImGui();

#endif // USE_IMGUI
};
