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
};

std::wstring GetSpritePath( SpriteAttribute spriteAttribute );

enum class ModelAttribute
{
	Demo, // For copy. Can erase this.
};

std::string GetModelPath( ModelAttribute modelAttribute );

enum class ShaderAttribute
{
	Demo,
};

std::string GetShaderPath( ShaderAttribute shaderAttribute, bool wantVS );
