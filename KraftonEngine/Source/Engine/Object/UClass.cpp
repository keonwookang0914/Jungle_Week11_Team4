#include "UClass.h"

void UClass::HideInheritedProperty(FString InName)
{
	for (uint32 i = 0; i < Properties.size(); i++)
	{
		if (!Properties[i]) continue;
		FProperty* Property = Properties[i];
		if (Property->Name != InName) continue;
		Property->PropertyFlag &= ~EPropertyFlags::CPF_Edit;
		break;
	}
}

bool UClass::IsPropertyHidden(FString InName) const
{
	for (uint32 i = 0; i < Properties.size(); i++)
	{
		if (!Properties[i]) continue;
		FProperty* Property = Properties[i];
		if (Property->Name != InName) continue;
		return (Property->PropertyFlag & EPropertyFlags::CPF_Edit) == 0;
	}

	return false;
}

void UClass::GetEditableProperties(TArray<FProperty>& OutProps) const
{
	if (SuperClass) SuperClass->GetEditableProperties(OutProps);

	for (uint32 i = 0; i < Properties.size(); i++)
	{
		if (!Properties[i]) continue;
		FProperty* Property = Properties[i];
		if (Property->PropertyFlag & EPropertyFlags::CPF_Edit) OutProps.push_back(*Property);
	}
}

void UClass::GetNonTransientProperties(TArray<FProperty>& OutProps) const
{
	if (SuperClass) SuperClass->GetNonTransientProperties(OutProps);

	for (uint32 i = 0; i < Properties.size(); i++)
	{
		if (!Properties[i]) continue;
		FProperty* Property = Properties[i];
		if ((Property->PropertyFlag & EPropertyFlags::CPF_Transient) == 0) OutProps.push_back(*Property);
	}
}