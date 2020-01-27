#pragma once

#include <string>

#include "Donya/Serializer.h"

/// <summary>
/// If set false to "useBinaryExtension", returns JSON extension.
/// </summary>
std::string GenerateSerializePath( std::string identifier, bool useBinaryExtension );

enum class SpriteAttribute
{
	FMODLogoBlack,
	FMODLogoWhite,
	TitleText,
	TitleGear,
	Tutorial,
	Gear,
	Emergerncy,
	EmergerncySoloFrame,
	Mission,
	Complete,
	PauseBoard,
	PauseSentences,
};

std::wstring GetSpritePath( SpriteAttribute spriteAttribute );

enum class ModelAttribute
{
	Player,
	Hook,
};

std::string GetModelPath( ModelAttribute modelAttribute );
