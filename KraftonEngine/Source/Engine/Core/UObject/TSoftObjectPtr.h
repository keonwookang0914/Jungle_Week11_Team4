#pragma once
#include "FSoftObjectPath.h"
#include "Serialization/Archive.h"

template <typename T>
class TSoftObjectPtr
{
public:
	TSoftObjectPtr() = default;
	TSoftObjectPtr(const FSoftObjectPath& InPath) : Path(InPath) {}

	bool IsNull() const { return Path.IsNull(); }
	void Reset() { Path.Reset(); CachedObject = nullptr; }

	const FSoftObjectPath& GetPath() const { return Path; }
	void SetPath(const FSoftObjectPath& InPath) { CachedObject = nullptr; Path = InPath; }

	T* Get() const { return CachedObject; }
	void Set(T* InObject) { CachedObject = InObject; } // Later: derive Path from object through a generic asset-path API

private:
	FSoftObjectPath Path;
	mutable T* CachedObject = nullptr;
};

template <typename T>
FArchive& operator<<(FArchive& Ar, TSoftObjectPtr<T>& Ptr)
{
	const auto& Path = Ptr.GetPath();
	FString SerializedPath = Ar.IsSaving() ? Path.ToString() : FString();
	Ar << SerializedPath;

	if (Ar.IsLoading())
	{
		Ptr.SetPath(SerializedPath);
	}

	return Ar;
}