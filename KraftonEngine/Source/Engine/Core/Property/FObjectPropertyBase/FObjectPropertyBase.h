#pragma once
#include "Core/Property/PropertyTypes.h"

class UClass;
class UObject;

class FObjectPropertyBase : public FProperty {
public:
	virtual EPropertyType GetType() const													= 0;
	virtual json::JSON Serialize(const void* Instance) const								= 0;
	virtual void Deserialize(void* Instance, const json::JSON& Value) const					= 0;

	// -----------------------------------------------------------------------------------------
	// GetObjectPropertyValue()
	// Purpose:	Reads the raw `UObject*` stored at that memory address.
	// Usage:	Fast and direct, but only works if the property is a "hard" object pointer.
	// _________________________________________________________________________________________
	// 
	// Returns the UObject currently represented by this property value.
	// Hard object properties read the stored pointer directly.
	// Soft object properties may resolve a loaded object from an indirect reference
	// and return nullptr when the referenced object is not currently loaded.
	// -----------------------------------------------------------------------------------------
	virtual UObject* GetObjectPropertyValue(void* PropertyMemoryAddress) const				= 0;

	// -----------------------------------------------------------------------------------------
	// SetObjectPropertyValue()
	// _________________________________________________________________________________________
	// 
	// Updates the property value so it refers to Value.
	// The concrete property decides whether that means storing a direct pointer,
	// a path, or another reference representation.
	// -----------------------------------------------------------------------------------------
	virtual void SetObjectPropertyValue(void* PropertyMemoryAddress, UObject* Value) const	= 0;

public:
	UClass* PropertyClass = nullptr;

protected:
	FObjectPropertyBase(const FString& InName, const FString& InCategory,
		uint32 InFlag, uint32 InOffset, uint32 InSize,
		UClass* InPropertyClass) : FProperty(InName, InCategory, InFlag, InOffset, InSize) 
	{
		Name = InName; Category = InCategory; PropertyFlag = InFlag;
		Offset_Internal = InOffset; ElementSize = InSize;
		PropertyClass = InPropertyClass;
	}

	virtual ~FObjectPropertyBase() = default;
};