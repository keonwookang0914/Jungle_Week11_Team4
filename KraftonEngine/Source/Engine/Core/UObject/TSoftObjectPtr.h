#pragma once
#include "FSoftObjectPath.h"

template <typename T>
class TSoftObjectPtr
{
public:
	TSoftObjectPtr() = default;
	TSoftObjectPtr(const FSoftObjectPath& InPath) : Path(InPath) {}

	bool IsNull() const { return Path.IsNull(); }
	void Reset() { Path.Reset(); CachedObject = nullptr; }

	const FSoftObjectPath& GetPath() const { return Path; }


private:
	FSoftObjectPath Path;
	mutable T* CachedObject = nullptr;
};