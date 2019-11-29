#pragma once

struct StorageForScene
{
	int selectedStageNumber{};
};

bool HasStorage();

void SetStorage( StorageForScene newData );
const StorageForScene *GetStorageOrNull();
