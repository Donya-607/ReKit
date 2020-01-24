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
	case SpriteAttribute::Emergerncy:
		return L"./Data/Images/UI/emergency.png";								// break;
	case SpriteAttribute::EmergerncySoloFrame:
		return L"./Data/Images/UI/Emergency_SoloFrame.png";						// break;
	case SpriteAttribute::Mission:
		return L"./Data/Images/Clear/MISSION.png";								// break;
	case SpriteAttribute::Complete:
		return L"./Data/Images/Clear/COMPLETE.png";								// break;
	case SpriteAttribute::KeyGet:
		return L"./Data/Images/Teacher/KeyGet.png";								// break;
	case SpriteAttribute::KeyNothing:
		return L"./Data/Images/Teacher/LockDoor.png";							// break;
	case SpriteAttribute::CommentaryBomb:
		return L"./Data/Images/Teacher/BombWall.png";							// break;
	case SpriteAttribute::CommentaryInset:
		return L"./Data/Images/Teacher/StageB1.png";							// break;
	default:
		assert( !"Error : Specified unexpect sprite type." ); break;
	}

	return L"ERROR_ATTRIBUTE";
}

std::string GetModelPath( ModelAttribute modelAttribute )
{
	switch ( modelAttribute )
	{
	case ModelAttribute::Player:
		return "./Data/Models/Player.bin"; // break;
	case ModelAttribute::Hook:
		return "./Data/Models/Hook.bin"; // break;
	default:
		assert( !"Error : Specified unexpect model type." ); break;
	}

	return "ERROR_ATTRIBUTE";
}
