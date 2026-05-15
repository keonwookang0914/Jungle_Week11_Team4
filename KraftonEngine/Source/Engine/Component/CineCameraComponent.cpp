#include "Component/CineCameraComponent.h"

#include "Object/ObjectFactory.h"
#include "Serialization/Archive.h"

IMPLEMENT_CLASS(UCineCameraComponent, UCameraComponent)

BEGIN_CLASS_PROPERTIES(UCineCameraComponent)
	PROPERTY_BOOL_NESTED_NAMED(Letterbox, FCineLetterboxSettings, bEnabled, "Enable Letterbox", "Enable Letterbox", "Cinematic", CPF_Edit)
	PROPERTY_FLOAT_NESTED_NAMED(Letterbox, FCineLetterboxSettings, Amount, "Letterbox Amount", "Letterbox Amount", "Cinematic", 0.0f, 1.0f, 0.01f, CPF_Edit)
	PROPERTY_FLOAT_NESTED_NAMED(Letterbox, FCineLetterboxSettings, Thickness, "Letterbox Thickness", "Letterbox Thickness", "Cinematic", 0.0f, 0.5f, 0.01f, CPF_Edit)
	REGISTER_PROPERTY_NESTED_NAMED(Letterbox, FCineLetterboxSettings, Color, "Letterbox Color", "Letterbox Color", EPropertyType::Color4, "Cinematic", CPF_Edit)
END_CLASS_PROPERTIES(UCineCameraComponent)

void UCineCameraComponent::Serialize(FArchive& Ar)
{
	UCameraComponent::Serialize(Ar);
	Ar << Letterbox.bEnabled;
	Ar << Letterbox.Amount;
	Ar << Letterbox.Thickness;
	Ar << Letterbox.Color;
}

void UCineCameraComponent::GetEditableProperties(TArray<FProperty>& OutProps)
{
	UCameraComponent::GetEditableProperties(OutProps);
}
