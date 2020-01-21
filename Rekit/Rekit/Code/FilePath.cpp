#include "FilePath.h"

#include "Donya/Constant.h"	// Use DEBUG_MODE.
#include "Donya/Useful.h"	// Use IsExistFile().

std::string GenerateSerializePath( std::string identifier, bool useBinary )
{
	const std::string EXT = ( useBinary ) ? ".bin" : ".json";

	return "./Data/Parameters/" + identifier + EXT;
}

std::wstring GetSpritePath( SpriteAttribute sprAttribute )
{
	switch ( sprAttribute )
	{
	case SpriteAttribute::FMODLogoBlack:
		return L"./Data/Images/Rights/FMOD Logo Black - White Background.png";	// break;
	case SpriteAttribute::FMODLogoWhite:
		return L"./Data/Images/Rights/FMOD Logo White - Black Background.png";	// break;
	case SpriteAttribute::Gear:
		return L"./Data/Images/BG/Gear.png";									// break;
	default:
		assert( !"Error : Specified unexpect sprite type." ); break;
	}

	return L"ERROR_ATTRIBUTE";
}

std::string GetModelPath( ModelAttribute modelAttribute )
{
	switch ( modelAttribute )
	{
	case ModelAttribute::Demo:
		return "./Foo/Bar/Anonymous.bin"; // break;
	default:
		assert( !"Error : Specified unexpect model type." ); break;
	}

	return "ERROR_ATTRIBUTE";
}

std::string GetShaderPath( ShaderAttribute shaderAttribute, bool wantVS )
{
	auto AddPostfix = []( const std::string &filePath, bool isVS )->std::string
	{
		const std::string type = ( isVS ) ? "VS" : "PS";
		return filePath + type + ".cso";
	};

	std::string fileName{};
	switch ( shaderAttribute )
	{
	case ShaderAttribute::Demo:
		return AddPostfix( "./Data/Shaders/DemoShader", wantVS ); // break;
	default:
		assert( !"Error : Specified unexpect shader type." ); break;
	}

	return "ERROR_ATTRIBUTE";
}
