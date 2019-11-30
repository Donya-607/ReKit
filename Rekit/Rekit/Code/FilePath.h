#pragma once

#include <string>

#include "Donya/Serializer.h"

std::string GenerateSerializePath( std::string identifier, Donya::Serializer::Extension extension );

int CalcStageCount();

std::string GetTerrainPath( int stageNumber );
std::string GetEnemyPath( int stageNumber );
std::string GetObjectPath( int stageNumber );
std::string GetConfigPath( int stageNumber );

enum class SpriteAttribute
{
	FMODLogoBlack,
	FMODLogoWhite,
	Player,
	PlayerEffects,
	MapChip,
	Enemy,
	Object,
	TitleLogo,
	SelectableStage,
	Usage,
	BG_Title,
	BG_1,
	BG_2,
	BG_3,
	UI,
	ScoreBoard,

	TestFont,

	SP_BG,
	SP_FireA,
	SP_FireB,
};

std::wstring GetSpritePath( SpriteAttribute spriteAttribute );

enum class ModelAttribute
{
	ChiPlayer,
};

std::string GetModelPath( ModelAttribute modelAttribute );
