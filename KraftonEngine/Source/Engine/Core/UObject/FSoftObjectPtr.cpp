#include "FSoftObjectPtr.h"
#include "Serialization/Archive.h"

FArchive& operator<<(FArchive& Ar, FSoftObjectPtr& Ptr)
{
	FSoftObjectPath Path = Ptr.GetPath();
	FString SerializedPath = Ar.IsSaving() ? Path.ToString() : FString();
	Ar << SerializedPath;

	if (Ar.IsLoading())
	{
		Path.SetPath(SerializedPath);
	}

	return Ar;
}