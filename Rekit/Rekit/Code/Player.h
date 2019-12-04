#pragma once

#include "Donya/Collision.h"
#include "Donya/Vector.h"
#include "Donya/UseImGui.h"

class Player
{
public:
	struct Input
	{
		Donya::Vector3 moveVelocity{};
		bool useJump{ false };
		bool useHook{ false };
	};
private:
	int				remainJumpCount{};	// 0 is can not jump, 1 ~ is can jump.
	Donya::Vector3	pos;				// World space.
	Donya::Vector3	velocity;
public:
	Player();
	~Player();
public:
	void Init();
	void Uninit();

	void Update( float elpasedTime, Input controller );

	void Draw() const;

#if USE_IMGUI
private:

	void UseImGui();

#endif // USE_IMGUI
};
