#pragma once
#include "PropertyTypes.h"

class FByteBoolProperty final : public FProperty
{
public:
	FByteBoolProperty(const FString& InName, const FString& InCategory, uint32 InPropertyFlag, uint32 InOffset, uint32 InElementSize)
		: FProperty(InName, InCategory, InPropertyFlag, InOffset, InElementSize)
	{
	}

	EPropertyType GetType() const override { return EPropertyType::ByteBool; }
	json::JSON Serialize(const void* Instance) const override;
	void Deserialize(void* Instance, const json::JSON& Value) const override;
};