#include "FSoftObjectProperty.h"
#include "Mesh/StaticMesh.h"
#include "Mesh/SkeletalMesh.h"
#include "Mesh/MeshManager.h"

UObject* FSoftObjectProperty::GetObjectPropertyValue(void* Addr) const
{
	const FString& Path = *static_cast<const FString*>(Addr);
	if (Path.empty() || Path == "None") return nullptr;
	if (PropertyClass == UStaticMesh::StaticClass())
		return FMeshManager::FindStaticMesh(Path);
	if (PropertyClass == USkeletalMesh::StaticClass())
		return FMeshManager::FindSkeletalMesh(Path);
	return nullptr;
}

void FSoftObjectProperty::SetObjectPropertyValue(void* Addr, UObject* Value) const
{
	auto* Path = static_cast<FString*>(Addr);

	if (!Value)
	{
		*Path = "None";
		return;
	}

	if (PropertyClass == UStaticMesh::StaticClass())
	{
		*Path = static_cast<UStaticMesh*>(Value)->GetAssetPathFileName();
		return;
	}

	if (PropertyClass == USkeletalMesh::StaticClass())
	{
		*Path = static_cast<USkeletalMesh*>(Value)->GetAssetPathFileName();
		return;
	}

	*Path = "None";
}
