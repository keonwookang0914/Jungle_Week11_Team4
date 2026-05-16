#pragma once
#include "Core/Property/PropertyTypes.h"

class FFloatProperty final : public FProperty
{
public:
	FFloatProperty(
		const FString& InName,
		const FString& InCategory,
		uint32 InPropertyFlag,
		uint32 InOffset,
		uint32 InElementSize,
		float InMin = 0.0f,
		float InMax = 0.0f,
		float InSpeed = 0.1f)
		: FProperty(InName, InCategory, InPropertyFlag, InOffset, InElementSize)
		, Min(InMin)
		, Max(InMax)
		, Speed(InSpeed)
	{
	}

	float Min = 0.0f;
	float Max = 0.0f;
	float Speed = 0.1f;

	EPropertyType GetType() const override { return EPropertyType::Float; }
	json::JSON Serialize(const void* Instance) const override;
	void Deserialize(void* Instance, const json::JSON& Value) const override;
};