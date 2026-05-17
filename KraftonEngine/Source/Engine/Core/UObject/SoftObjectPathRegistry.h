#pragma once
#include "FSoftObjectPath.h"
#include "Core/Singleton.h"
#include <unordered_map>

class FSoftObjectPathRegistry : public TSingleton<FSoftObjectPathRegistry>
{
	friend class TSingleton<FSoftObjectPathRegistry>;

public:


private:
	TMap<FSoftObjectPath, UObject*> SoftObjectPathRegistry;

};
