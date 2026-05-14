#include "FName.h"
#include <algorithm>
#include <cctype>

// ============================================================
// FNamePool
// ============================================================
uint32 FNamePool::Store(const FString& InString)
{
	auto It = LookupMap.find(InString);
	if (It != LookupMap.end())
	{
		return It->second;
	}
	uint32 Index = static_cast<uint32>(Entries.size());
	Entries.push_back(InString);
	LookupMap[InString] = Index;
	return Index;
}

const FString& FNamePool::Resolve(uint32 Index) const
{
	static const FString Empty;
	if (Index < static_cast<uint32>(Entries.size()))
	{
		return Entries[Index];
	}
	return Empty;
}

// ============================================================
// FName
// ============================================================
static void ToLower(FString& Str)
{
    for (char& C : Str)
        C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
}

const FName FName::None = "Name_None";

FName::FName()
	: ComparisonIndex(0)
	, DisplayIndex(0)
{
}

FName::FName(const char* InName) : FName(InName ? FString(InName) : FString())
{
}

FName::FName(const FString& InName)
{
	if (InName.empty())
	{
		ComparisonIndex = 0;
		DisplayIndex = 0;
		return;
	}

	FNamePool& Pool = FNamePool::Get();
	DisplayIndex = Pool.Store(InName);

	FString LowerStr = InName;
	ToLower(LowerStr);

	ComparisonIndex = Pool.Store(LowerStr);
}

bool FName::operator==(const FName& Other) const
{
	return ComparisonIndex == Other.ComparisonIndex;
}

bool FName::operator!=(const FName& Other) const
{
	return ComparisonIndex != Other.ComparisonIndex;
}

size_t FName::Hash::operator()(const FName& Name) const
{
	return std::hash<uint32>()(Name.ComparisonIndex);
}

FString FName::ToString() const
{
	return FNamePool::Get().Resolve(DisplayIndex);
}

bool FName::IsValid() const
{
	return DisplayIndex != 0 || ComparisonIndex != 0;
}

FString FName::NameToDisplayString(const FString& InName, bool bIsBool)
{
	FString Name = InName;
	// If the variable is a bool, drop the leading 'b'
	if (bIsBool && Name.length() >= 2)
	{
		const char First = Name[0];
		const char Second = Name[1];

		if (First == 'b' && std::isupper(static_cast<unsigned char>(Second)))
		{
			Name.erase(0, 1);
		}
	}
	FString OutName;

	// Add space between camelcases. 
	OutName.reserve(Name.length());

	// If the variable is a bool, drop 'b' in the beginning of the name


	return OutName;
}
