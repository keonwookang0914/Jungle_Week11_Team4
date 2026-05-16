#pragma once

#include "Core/Property/FNumericProperty/FNumericProperty.h"

class FIntProperty final : public FNumericProperty
{
public:
	FIntProperty(
		const FString& InName,
		const FString& InCategory,
		uint32 InPropertyFlag,
		uint32 InOffset,
		uint32 InElementSize,
		float InMin = 0.0f,
		float InMax = 0.0f,
		float InSpeed = 0.1f)
		: FNumericProperty(InName, InCategory, InPropertyFlag, InOffset, InElementSize, InMin, InMax, InSpeed)
	{
	}

	EPropertyType GetType() const override { return EPropertyType::Int; }
	json::JSON Serialize(const void* Instance) const override;
	void Deserialize(void* Instance, const json::JSON& Value) const override;
};
