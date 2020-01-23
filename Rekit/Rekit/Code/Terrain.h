#pragma once

#include <vector>

#include "Donya/Vector.h"

#include "DerivedCollision.h"

/// <summary>
/// This class have simple terrains(represent by hit-box).
/// </summary>
class Terrain
{
public:
	static bool LoadModel();
private:
	Donya::Vector3		worldOffset{};
	std::vector<BoxEx>	source{};		// Constant.
	std::vector<BoxEx>	boxes{};		// Editable.
public:
	void Init( const Donya::Vector3 &wsRoomOriginPos, const std::vector<BoxEx> &sourceTerrain );
	void Uninit();

	void Draw( const Donya::Vector4x4 &matVP, const Donya::Vector4 &lightDir, bool drawEditableBoxes = false ) const;
public:
	/// <summary>
	/// Reset the edited hit-boxes to be initial state.
	/// </summary>
	void Reset();

	/// <summary>
	/// Returns current edited hit-boxes.
	/// </summary>
	std::vector<BoxEx> Acquire() const;

	/// <summary>
	/// Append the terrain to current editable hit-boxes.
	/// </summary>
	void Append( const std::vector<BoxEx> &terrain );
};
