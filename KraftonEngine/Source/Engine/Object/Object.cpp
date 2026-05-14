#include "Object.h"
#include "UUIDGenerator.h"
#include "Serialization/Archive.h"
#include "Serialization/MemoryArchive.h"
#include "Object/ObjectFactory.h"

TArray<UObject*> GUObjectArray;
TSet<UObject*> GUObjectSet;

UObject::UObject()
{
	UUID = UUIDGenerator::GenUUID();
	InternalIndex = static_cast<uint32>(GUObjectArray.size());
	GUObjectArray.push_back(this);
	GUObjectSet.insert(this);
}

UObject::~UObject()
{
	GUObjectSet.erase(this);

	uint32 LastIndex = static_cast<uint32>(GUObjectArray.size() - 1);

	if (InternalIndex != LastIndex)
	{
		UObject* LastObject = GUObjectArray[LastIndex];
		GUObjectArray[InternalIndex] = LastObject;
		LastObject->InternalIndex = InternalIndex;
	}

	GUObjectArray.pop_back();
}

UObject* UObject::Duplicate(UObject* NewOuter) const
{
	// FObjectFactory 기반 같은 타입 인스턴스 생성 → Serialize 왕복 → PostDuplicate.
	// UUID/Name은 생성자에서 새로 발급되며, Serialize에서 덮어쓰지 않는 것이 규칙이다.
	// NewOuter가 nullptr이면 원본의 Outer를 그대로 승계.
	UObject* EffectiveOuter = NewOuter ? NewOuter : Outer;
	UObject* Dup = FObjectFactory::Get().Create(GetClass()->GetName(), EffectiveOuter);
	if (!Dup)
	{
		return nullptr;
	}

	FMemoryArchive Writer(/*bIsSaving=*/true);
	const_cast<UObject*>(this)->Serialize(Writer);

	FMemoryArchive Reader(Writer.GetBuffer(), /*bIsSaving=*/false);
	Dup->Serialize(Reader);

	Dup->PostDuplicate();
	return Dup;
}

void UObject::Serialize(FArchive& Ar)
{
	// 기본 UObject는 직렬화할 상태 없음.
	// UUID/InternalIndex/Name은 직렬화 금지 (복제 시 새로 발급).
	Ar << ObjectName;
}

void UObject::GetEditableProperties(TArray<FProperty>& /*OutProps*/)
{
	// 점진적 마이그레이션 동안 base 는 no-op. 각 reflected-class 는 자신의 override 에서
	//   Super::GetEditableProperties(OutProps);
	//   AppendReflectedProperties(this, StaticClass(), OutProps);
	// 패턴으로 OWN-class 프로퍼티만 인스턴스 바인딩해 추가한다.
	// 체인 전체를 한 번에 walk 하지 않는 이유: 아직 마이그레이션이 끝나지 않은 base 가
	// 자신의 매뉴얼 GetEditableProperties 에서 UObject base 를 호출하면, 그 자리에서
	// most-derived 클래스의 reflected 가 추가되어 후속 override 의 helper 호출과 중복돼버린다.
	// 모든 클래스가 마이그레이션되면 이 base 를 다시 GetAllProperties walk 로 바꿔도 안전.
}

void UObject::PostEditProperty(const char* /*PropertyName*/)
{
	// 기본 UObject는 편집 후 추가 작업 없음.
}

UClass UObject::StaticClassInstance("UObject", nullptr, sizeof(UObject), CF_None);
