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
