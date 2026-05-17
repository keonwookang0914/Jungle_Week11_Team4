#pragma once
#include "Core/Property/PropertyTypes.h"

class UClass;
class UObject;

class FObjectPropertyBase : public FProperty {
public:
	virtual EPropertyType GetType() const													= 0;
	virtual json::JSON Serialize(const void* Instance) const								= 0;
	virtual void Deserialize(void* Instance, const json::JSON& Value) const					= 0;

	// Purpose:	Reads the raw `UObject*` stored at that memory address.
	// Usage:	Fast and direct, but only works if the property is a "hard" object pointer.
	virtual UObject* GetObjectPropertyValue(void* PropertyMemoryAddress) const				= 0;
	virtual void SetObjectPropertyValue(void* PropertyMemoryAddress, UObject* Value) const	= 0;

public:
	UClass* PropertyClass = nullptr;

protected:
	FObjectPropertyBase(const FString& InName, const FString& InCategory,
		uint32 InFlag, uint32 InOffset, uint32 InSize,
		UClass* InPropertyClass) : FProperty(InName, InCategory, InFlag, InOffset, InSize) {}

	virtual ~FObjectPropertyBase() = default;
};