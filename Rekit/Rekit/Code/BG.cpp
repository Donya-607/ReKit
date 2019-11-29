#include "BG.h"

#include "Header/Constant.h"
#include "FilePath.h"
#include "Header/Sprite.h"

BG::BG() : stageNo( 0 ), sprite()
{

}

void BG::Init( int stageNumber )
{
	stageNo = stageNumber;

	SpriteAttribute attrName{};
	switch ( stageNo )
	{
	case -1: attrName = SpriteAttribute::BG_Title; break;
	case 0: attrName = SpriteAttribute::BG_1; break;
	case 1: attrName = SpriteAttribute::BG_2; break;
	case 2: attrName = SpriteAttribute::BG_2; break;
	case 3: attrName = SpriteAttribute::BG_3; break;
	case 4: attrName = SpriteAttribute::BG_3; break;
	default: assert( !"BG : Unexpected stage number." ); return;
	}

	sprite.Init();
	sprite.LoadSheet( GetSpritePath( attrName ), 9U );
	sprite.SetPartCount( { 1, 1 } );
	sprite.SetPartSize( sprite.GetSheetSize() );
}

void BG::Draw( Donya::Vector2 scroll )
{
	Donya::Vector2 fSize = sprite.GetSheetSize().Float();

	// Wrap scroll.
	while ( fSize.x < scroll.x ) { scroll.x -= fSize.x; }
	while ( fSize.y < scroll.y ) { scroll.y -= fSize.y; }
	while ( scroll.x < 0.0f ) { scroll.x += fSize.x; }
	while ( scroll.y < 0.0f ) { scroll.y += fSize.y; }

	auto DrawBG = [&fSize]( size_t ID, float x, float y )
	{
		using Coord = Donya::Sprite::Origin;

		Donya::Sprite::Draw
		(
			ID,
			x,
			y + ( fSize.y * 0.9f/*Adjust to left-bottom*/ ),
			0.0f,
			Coord::X_LEFT | Coord::Y_BOTTOM
		);
	};

	// Drawing list:
	// Left-Top,	Top,	Right-Top,
	// Left-Center,	Center,	Right-Center,
	// Left-Bottom,	Bottom,	Right-Bottom.

	size_t ID = sprite.GetSpriteIdentifier();
	Donya::Vector2 center
	{
		( stageNo == -1 ) ? fSize.x * 0.1f : fSize.x * 0.5f,
		( stageNo == -1 ) ? fSize.y * 0.1f : fSize.y * 0.5f
	};

	for ( int signY = -1; signY <= 1; ++signY )
	{
		for ( int signX = -1; signX <= 1; ++signX )
		{
			DrawBG
			(
				ID,
				center.x - scroll.x + ( fSize.x * signX ),
				center.y - scroll.y + ( fSize.y * signY )
			);
		}
	}
}