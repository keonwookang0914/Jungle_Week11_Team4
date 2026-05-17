#pragma once
#include "FSoftObjectPath.h"
#include "Serialization/Archive.h"

template <typename T>
class TSoftObjectPtr
{
public:
	TSoftObjectPtr() = default;
	TSoftObjectPtr(const FSoftObjectPath& InPath) : Path(InPath) { CachedObject = Path.ResolveObject(); }

	bool IsNull() const { return Path.IsNull(); }
	void Reset() { Path.Reset(); CachedObject = nullptr; }

	const FSoftObjectPath& GetPath() const { return Path; }
	FSoftObjectPath& GetMutablePath() { return Path; }
	void SetPath(const FSoftObjectPath& InPath) 
	{ 
		Path = InPath;
		CachedObject = Path.ResolveObject();
	}
	void SetPath(const FString& InPath) { SetPath(FSoftObjectPath(InPath)); }

	T* Get() const { return CachedObject; }
	//void Set(T* InObject) { CachedObject = InObject; } // Later: derive Path from object through a generic asset-path API

	TSoftObjectPtr& operator=(T* InObject)
	{
		Set(InObject);
		return *this;
	}

	explicit operator bool() const { return CachedObject != nullptr; }
	T* operator->() const { return CachedObject; }
	operator T*() const { return CachedObject; }

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
