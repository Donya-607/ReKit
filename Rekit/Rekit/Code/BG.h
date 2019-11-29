#pragma once

#include "Animation.h"
#include "Header/Vector.h"

/// <summary>
/// Scrolling sprite.
/// </summary>
class BG
{
private:
	int stageNo;
	SpriteSheet sprite;
public:
	BG();
public:
	void Init( int stageNumber );

	void Draw( Donya::Vector2 scroll );
};
