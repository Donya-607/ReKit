#include "StorageForScene.h"

static StorageForScene instance{};
static bool isValidStorage = false;

bool HasStorage()
{
	return isValidStorage;
}

void SetStorage( StorageForScene newData )
{
	instance = newData;
	isValidStorage = true;
}
const StorageForScene *GetStorageOrNull()
{
	if ( !HasStorage() ) { return nullptr; }
	// else

	isValidStorage = false;
	return &instance;
}
