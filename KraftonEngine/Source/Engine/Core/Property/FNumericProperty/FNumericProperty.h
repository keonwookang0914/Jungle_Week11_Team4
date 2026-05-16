#pragma once

#include "Core/Property/PropertyTypes.h"

class FNumericProperty : public FProperty
{
public:
	float Min = 0.0f;
	float Max = 0.0f;
	float Speed = 0.1f;

protected:
	FNumericProperty(
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
};
