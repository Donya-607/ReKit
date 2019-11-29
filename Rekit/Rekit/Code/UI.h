#pragma once

#include "Header/Useful.h"

class UI
{
public:
	struct DrawData
	{
		Donya::Int2 texPos{};
		Donya::Int2 texSize{};
	};
	enum class Kind
	{
		Board,
		Numbers,
		Score,
		PromptInput,
		Time,
		Start,
		Stage,
		Pause,
		BackToTitle,
		ReTry,
		BackToGame,
		TimeJP,
		GoToNextStage,
		BestTime,
		DeathCount,
	};
public:
	static DrawData GetDrawData( Kind kind )
	{
		DrawData data{};

		switch ( kind )
		{
		case UI::Kind::Board:
			data.texPos = { 0, 0 };
			data.texSize = { 480, 96 };
			break;
		case UI::Kind::Numbers:
			data.texPos = { 0, 0 + 96 };
			data.texSize = { 44, 64 };
			break;
		case UI::Kind::Score:
			data.texPos = { 0, 96 + 64 };
			data.texSize = { 320, 64 };
			break;
		case UI::Kind::PromptInput:
			data.texPos = { 0, 160 + 64 };
			data.texSize = { 576, 64 };
			break;
		case UI::Kind::Time:
			data.texPos = { 0, 224 + 64 };
			data.texSize = { 256, 64 };
			break;
		case UI::Kind::Start:
			data.texPos = { 0, 288 + 64 };
			data.texSize = { 320, 64 };
			break;
		case UI::Kind::Stage:
			data.texPos = { 0, 352 + 64 };
			data.texSize = { 320, 64 };
			break;
		case UI::Kind::Pause:
			data.texPos = { 0, 416 + 64 };
			data.texSize = { 320, 64 };
			break;
		case UI::Kind::BackToTitle:
			data.texPos = { 0, 480 + 64 };
			data.texSize = { 448, 64 };
			break;
		case UI::Kind::ReTry:
			data.texPos = { 0, 544 + 64 };
			data.texSize = { 256, 64 };
			break;
		case UI::Kind::BackToGame:
			data.texPos = { 0, 608 + 64 };
			data.texSize = { 384, 64 };
			break;
		case UI::Kind::TimeJP:
			data.texPos = { 0, 672 + 64 };
			data.texSize = { 192, 64 };
			break;
		case UI::Kind::GoToNextStage:
			data.texPos = { 0, 736 + 64 };
			data.texSize = { 448, 64 };
			break;
		case UI::Kind::BestTime:
			data.texPos = { 0, 800 + 64 };
			data.texSize = { 384, 64 };
			break;
		case UI::Kind::DeathCount:
			data.texPos = { 0, 864 + 64 };
			data.texSize = { 320, 64 };
			break;
		default:
			data.texPos = { 0, 0 };
			data.texSize = { 0, 0 };
			break;
		}

		return data;
	}
};
