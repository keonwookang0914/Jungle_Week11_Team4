#include "FSoftObjectProperty.h"
#include "Engine/Mesh/StaticMesh.h"
#include "Engine/Mesh/SkeletalMesh.h"
#include "Engine/Mesh/MeshManager.h"

UObject* FSoftObjectProperty::GetObjectPropertyValue(void* Addr) const
{
	const FString& Path = *static_cast<const FString*>(Addr);
	if (Path.empty() || Path == "None") return nullptr;
	if (PropertyClass == UStaticMesh::StaticClass())
		return FMeshManager::ResolveStaticMesh(Path);     // or whatever the API is
	if (PropertyClass == USkeletalMesh::StaticClass())
		return FMeshManager::ResolveSkeletalMesh(Path);
	return nullptr;
}

void FSoftObjectProperty::SetObjectPropertyValue(void* Addr, UObject* Value) const
{
	auto* Path = static_cast<FString*>(Addr);
	*Path = Value ? Value->GetAssetPathFileName() : FString();
}