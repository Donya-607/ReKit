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
	std::vector<BoxEx> source{};	// Constant.
	std::vector<BoxEx> boxes{};		// Editable.
public:
	void Init( const std::vector<BoxEx> &sourceTerrain );
	void Uninit();

	/// <summary>
	/// Draw the current editable hit-boxes.
	/// </summary>
	void Draw( const Donya::Vector4x4 &matVP, const Donya::Vector4 &lightDir ) const;
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
