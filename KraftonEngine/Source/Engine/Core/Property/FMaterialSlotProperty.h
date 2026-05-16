#pragma once

#include "PropertyTypes.h"

class FMaterialSlotProperty final : public FProperty
{
public:
	FMaterialSlotProperty(const FString& InName, const FString& InCategory, uint32 InPropertyFlag, uint32 InOffset, uint32 InElementSize)
		: FProperty(InName, InCategory, InPropertyFlag, InOffset, InElementSize)
	{
	}

	EPropertyType GetType() const override { return EPropertyType::MaterialSlot; }
	json::JSON Serialize(const void* Instance) const override;
	void Deserialize(void* Instance, const json::JSON& Value) const override;
};
