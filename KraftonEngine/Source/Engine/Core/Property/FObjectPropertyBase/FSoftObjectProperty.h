#pragma once  
#include "FObjectPropertyBase.h"  

class FSoftObjectProperty final : public FObjectPropertyBase  
{  
public:  
	FSoftObjectProperty(const FString& InName, const FString& InCategory,  
		uint32 InFlag, uint32 InOffset, uint32 InSize,  
		UClass* InPropertyClass)  
		: FObjectPropertyBase(InName, InCategory, InFlag, InOffset, InSize, InPropertyClass) // Explicitly call the default constructor of the base class  
	{  
		Name = InName; Category = InCategory; PropertyFlag = InFlag;  
		Offset_Internal = InOffset; ElementSize = InSize;  
		PropertyClass = InPropertyClass;  
	}  

	EPropertyType GetType() const override { return EPropertyType::SoftObject; }  
	json::JSON Serialize(const void* Instance) const override;        // FString path to JSON  
	void Deserialize(void* Instance, const json::JSON& Value) const override;  

	UObject* GetObjectPropertyValue(void* PropertyMemoryAddress) const override;  
	void SetObjectPropertyValue(void* PropertyMemoryAddress, UObject* Value) const override;  
};