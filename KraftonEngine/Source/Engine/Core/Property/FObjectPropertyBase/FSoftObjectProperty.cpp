#include "FSoftObjectProperty.h"
#include "Mesh/StaticMesh.h"
#include "Mesh/SkeletalMesh.h"
#include "Mesh/MeshManager.h"
#include "Core/UObject/TSoftObjectPtr.h"

namespace
{
	const FSoftObjectPath* GetSoftObjectPath(const void* Addr, const UClass* PropertyClass)
	{
		if (PropertyClass == UStaticMesh::StaticClass())
		{
			return &static_cast<const TSoftObjectPtr<UStaticMesh>*>(Addr)->GetPath();
		}
		if (PropertyClass == USkeletalMesh::StaticClass())
		{
			return &static_cast<const TSoftObjectPtr<USkeletalMesh>*>(Addr)->GetPath();
		}
		return nullptr;
	}

	void SetSoftObjectPath(void* Addr, const UClass* PropertyClass, const FString& Path)
	{
		if (PropertyClass == UStaticMesh::StaticClass())
		{
			static_cast<TSoftObjectPtr<UStaticMesh>*>(Addr)->SetPath(Path);
			return;
		}
		if (PropertyClass == USkeletalMesh::StaticClass())
		{
			static_cast<TSoftObjectPtr<USkeletalMesh>*>(Addr)->SetPath(Path);
		}
	}
}

UObject* FSoftObjectProperty::GetObjectPropertyValue(void* Addr) const
{
	//TSoftObjectPtr<T> &static_cast<const TSoftObjectPtr<UStaticMesh>*>(Addr)->GetPath();
	const FSoftObjectPath* Path = GetSoftObjectPath(Addr, PropertyClass);
	if (!Path || Path->IsNull()) return nullptr;
	if (PropertyClass == UStaticMesh::StaticClass())
	{
		auto* Ptr = static_cast<TSoftObjectPtr<UStaticMesh>*>(Addr);
		return Ptr->Get() ? Ptr->Get() : FMeshManager::FindStaticMesh(Path->ToString());
	}
	if (PropertyClass == USkeletalMesh::StaticClass())
	{
		auto* Ptr = static_cast<TSoftObjectPtr<USkeletalMesh>*>(Addr);
		return Ptr->Get() ? Ptr->Get() : FMeshManager::FindSkeletalMesh(Path->ToString());
	}
	return nullptr;
}

void FSoftObjectProperty::SetObjectPropertyValue(void* Addr, UObject* Value) const
{
	if (!Value)
	{
		if (PropertyClass == UStaticMesh::StaticClass())
		{
			static_cast<TSoftObjectPtr<UStaticMesh>*>(Addr)->Reset();
		}
		else if (PropertyClass == USkeletalMesh::StaticClass())
		{
			static_cast<TSoftObjectPtr<USkeletalMesh>*>(Addr)->Reset();
		}
		return;
	}

	if (PropertyClass == UStaticMesh::StaticClass())
	{
		auto* Ptr = static_cast<TSoftObjectPtr<UStaticMesh>*>(Addr);
		auto* Mesh = static_cast<UStaticMesh*>(Value);
		Ptr->SetPath(Mesh->GetAssetPathFileName());
		return;
	}

	if (PropertyClass == USkeletalMesh::StaticClass())
	{
		auto* Ptr = static_cast<TSoftObjectPtr<USkeletalMesh>*>(Addr);
		auto* Mesh = static_cast<USkeletalMesh*>(Value);
		Ptr->SetPath(Mesh->GetAssetPathFileName());
		return;
	}
}

json::JSON FSoftObjectProperty::Serialize(const void* Instance) const
{
	const auto* Path = GetSoftObjectPath(ContainerPtrToValuePtr(Instance), PropertyClass);
	return json::JSON(Path ? Path->ToString() : FString());
}

void FSoftObjectProperty::Deserialize(void* Instance, const json::JSON& Value) const
{
	SetSoftObjectPath(ContainerPtrToValuePtr(Instance), PropertyClass, Value.ToString());
}
