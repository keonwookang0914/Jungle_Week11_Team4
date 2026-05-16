#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Core/CoreTypes.h"

namespace json { class JSON; }

struct FArrayAccessor
{
	uint32	(*Num)(const void* ArrayPtr);
	void*	(*GetAt)(void* ArrayPtr, uint32 Index);
	void	(*AddDefault)(void* ArrayPtr);
	void	(*RemoveAt)(void* ArrayPtr, uint32 Index);
	void	(*Clear)(void* ArrayPtr);
	void	(*Assign)(void* DstArr, const void* SrcArr);
};

template <typename T>
inline FArrayAccessor* GetTArrayAccessor()
{
	static FArrayAccessor s = {
		+[](const void* A) -> uint32 { return (uint32)static_cast<const TArray<T>*>(A)->size(); },
		+[](void* A, uint32 i) -> void* { return &(*static_cast<TArray<T>*>(A))[i]; },
		+[](void* A) { static_cast<TArray<T>*>(A)->emplace_back(); },
		+[](void* A, uint32 i) { auto& V = *static_cast<TArray<T>*>(A); V.erase(V.begin() + i); },
		+[](void* A) { static_cast<TArray<T>*>(A)->clear(); },
		+[](void* D, const void* S) { *static_cast<TArray<T>*>(D) = *static_cast<const TArray<T>*>(S); },
	};
	return &s;
}

// Property dispatch tag used by the editor and serializers.
enum class EPropertyType : uint8_t
{
	Bool,
	ByteBool,
	Int,
	Float,
	Vec3,
	Vec4,
	Rotator,
	String,
	Name,
	SceneComponentRef,
	Color4,
	StaticMeshRef,
	SkeletalMeshRef,
	MaterialSlot,
	Enum,
	Struct,
	Script,
	Array,
};

enum EPropertyFlags : uint32 {
	CPF_None			= 0,
	CPF_Edit			= 1 << 1,
	CPF_FixedSize		= 1 << 2,
	CPF_Transient		= 1 << 3,
	CPF_Config			= 1 << 4,
};

struct FMaterialSlot
{
	std::string Path;
};

class FProperty;

using FStructPropertySchemaFn = const std::vector<FProperty*>& (*)();

class FProperty
{
public:
	virtual ~FProperty() = default;
	FProperty(const FProperty&) = delete;
	FProperty& operator=(const FProperty&) = delete;

	FString Name;
	FString Category;
	uint32 PropertyFlag = EPropertyFlags::CPF_None;
	uint32 ElementSize = 0;
	uint32 Offset_Internal = 0;

	virtual EPropertyType GetType() const = 0;
	virtual json::JSON Serialize(const void* Instance) const = 0;
	virtual void Deserialize(void* Instance, const json::JSON& Value) const = 0;

	void* ContainerPtrToValuePtr(void* Container) const
	{
		return static_cast<char*>(Container) + Offset_Internal;
	}

	const void* ContainerPtrToValuePtr(const void* Container) const
	{
		return static_cast<const char*>(Container) + Offset_Internal;
	}

protected:
	FProperty(
		const FString& InName,
		const FString& InCategory,
		uint32 InPropertyFlag,
		uint32 InOffset,
		uint32 InElementSize)
		: Name(InName)
		, Category(InCategory)
		, PropertyFlag(InPropertyFlag)
		, ElementSize(InElementSize)
		, Offset_Internal(InOffset)
	{}
};

// FProperty is pure schema. Instance addresses are derived from the owning container.
