#pragma once

// ---------------------------------------------------------------------------
// Unreal-style annotation markers parsed by the codegen (Scripts/GenerateCode.py).
// These macros are intentionally empty at compile time — they exist only so
// the parser can locate UCLASS / UPROPERTY / UFUNCTION sites in headers.
//
// Codegen emits two files per annotated header:
//   <Foo>.generated.h   — defines KE_GENERATED_BODY_<ClassName>()
//   <Foo>.gen.cpp       — UClass static, property table, Lua usertype binding
//
// Author-side usage:
//
//   #include "Foo.generated.h"
//
//   UCLASS()
//   class UFoo : public UBar
//   {
//       GENERATED_BODY(UFoo)
//
//       UPROPERTY(Edit, Category="Movement", Min=0, Max=200, Speed=0.5)
//       float MaxSpeed = 50.f;
//
//       UFUNCTION(Lua)
//       void Possess(AActor* Target);
//   };
//
// GENERATED_BODY(ClassName) token-pastes to the per-class macro defined in
// the matching .generated.h, so multiple UCLASSes can coexist in one header
// without UE's __LINE__ tracking.
//
// v1 scope: UENUM/USTRUCT are recognized as markers but do NOT trigger
// names-table or DescribeProperties emission. Enum/struct UPROPERTY sites
// must still pass EnumNames=... / StructFunc=... explicitly until v2.
// ---------------------------------------------------------------------------

#define UCLASS(...)
#define UPROPERTY(...)
#define UFUNCTION(...)
#define UENUM(...)
#define USTRUCT(...)

#define GENERATED_BODY(ClassName) KE_GENERATED_BODY_##ClassName()
