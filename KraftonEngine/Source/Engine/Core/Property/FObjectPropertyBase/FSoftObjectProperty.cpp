#include "FSoftObjectProperty.h"
#include "Mesh/StaticMesh.h"
#include "Mesh/SkeletalMesh.h"
#include "Mesh/MeshManager.h"
#include "Core/UObject/FSoftObjectPath.h"

UObject* FSoftObjectProperty::GetObjectPropertyValue(void* Addr) const
{
	const FSoftObjectPath& Path = *static_cast<const FSoftObjectPath*>(Addr);
	if (Path.IsNull()) return nullptr;
	if (PropertyClass == UStaticMesh::StaticClass())
		return FMeshManager::FindStaticMesh(Path.ToString());
	if (PropertyClass == USkeletalMesh::StaticClass())
		return FMeshManager::FindSkeletalMesh(Path.ToString());
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

json::JSON FSoftObjectProperty::Serialize(const void* Instance) const
{
	const auto* Path = static_cast<const FSoftObjectPath*>(
		ContainerPtrToValuePtr(Instance));
	return json::JSON(Path->ToString());
}

void FSoftObjectProperty::Deserialize(void* Instance, const json::JSON& Value) const
{
	auto* Path = static_cast<FSoftObjectPath*>(
		ContainerPtrToValuePtr(Instance));
	Path->SetPath(Value.ToString());
}