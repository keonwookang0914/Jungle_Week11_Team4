#include "Core/UObject/FSoftObjectPath.h"

#include "Serialization/Archive.h"

FArchive& operator<<(FArchive& Ar, FSoftObjectPath& Path)
{
	FString SerializedPath = Ar.IsSaving() ? Path.ToString() : FString();
	Ar << SerializedPath;

	if (Ar.IsLoading())
	{
		Path.SetPath(SerializedPath);
	}

	return Ar;
}
