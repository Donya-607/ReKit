#pragma once

#include <vector>

#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

class BG
{
public:
	/// <summary>
	/// Please call when a scene initialize.
	/// </summary>
	static void ParameterInit ();
#if USE_IMGUI
	/// <summary>
	/// Please call every frame.
	/// </summary>
	static void UseParameterImGui ();
#endif // USE_IMGUI
public:
	static constexpr int GEAR_NUM = 3;
private:
	size_t gear;

	Donya::Vector2				pos[GEAR_NUM];				// Screen space.
	float						degree[GEAR_NUM];
public:
	BG ();
	~BG ();
public:
	void Init ();
	void Uninit ();

	void Update ( float elpasedTime );

	void Draw () const;

#if USE_IMGUI
private:

	void ShowImGuiNode ();

#endif // USE_IMGUI
};
