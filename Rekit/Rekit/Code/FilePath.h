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
	Gear,
};

std::wstring GetSpritePath( SpriteAttribute spriteAttribute );

enum class ModelAttribute
{
	Player,
	Hook,
};

std::string GetModelPath( ModelAttribute modelAttribute );
