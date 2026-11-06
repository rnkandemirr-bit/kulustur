#include "KulusturDump.h"

#include "Components/SplineComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogKulusturDump, Log, All);

namespace
{
	void DumpProps(const UObject* Obj, const UObject* Defaults, bool bOnlyChanged, const FString& Indent)
	{
		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			const FProperty* P = *It;
			// Sadece Blueprint'te tanımlı değişkenler
			if (!P->GetOwnerClass() || !Cast<UBlueprintGeneratedClass>(P->GetOwnerClass()))
			{
				continue;
			}
			FString Value;
			P->ExportTextItem_Direct(Value, P->ContainerPtrToValuePtr<void>(Obj), nullptr, nullptr, PPF_None);
			FString Def;
			if (Defaults)
			{
				P->ExportTextItem_Direct(Def, P->ContainerPtrToValuePtr<void>(Defaults), nullptr, nullptr, PPF_None);
			}
			const bool bChanged = Defaults && Value != Def;
			if (bOnlyChanged && !bChanged) continue;
			UE_LOG(LogKulusturDump, Log, TEXT("%s%s%s (%s) = %s"), *Indent, bChanged ? TEXT("* ") : TEXT(""), *P->GetName(), *P->GetCPPType(), *Value.Left(300));
		}
	}
}

void KulusturDump::DumpWorld(UWorld* World)
{
	TSet<UClass*> Seen;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* A = *It;
		UClass* C = A->GetClass();
		if (!Cast<UBlueprintGeneratedClass>(C) || !C->GetPathName().Contains(TEXT("3DPlatformerGameBundle")))
		{
			continue;
		}
		const bool bFirst = !Seen.Contains(C);
		Seen.Add(C);
		UE_LOG(LogKulusturDump, Log, TEXT("ACTOR %s class=%s loc=%s rot=%s scale=%s"), *A->GetName(), *C->GetName(),
			*A->GetActorLocation().ToCompactString(), *A->GetActorRotation().ToCompactString(), *A->GetActorScale3D().ToCompactString());

		TArray<UActorComponent*> Comps;
		A->GetComponents(Comps);
		for (UActorComponent* Comp : Comps)
		{
			FString Extra;
			if (const USceneComponent* SC = Cast<USceneComponent>(Comp))
			{
				Extra = FString::Printf(TEXT(" rel=%s"), *SC->GetRelativeLocation().ToCompactString());
			}
			if (const USplineComponent* Spline = Cast<USplineComponent>(Comp))
			{
				Extra += FString::Printf(TEXT(" SPLINE points=%d len=%.0f closed=%d"), Spline->GetNumberOfSplinePoints(), Spline->GetSplineLength(), Spline->IsClosedLoop());
				for (int32 i = 0; i < Spline->GetNumberOfSplinePoints(); ++i)
				{
					Extra += FString::Printf(TEXT(" [%s t=%s]"), *Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local).ToCompactString(),
						*Spline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local).ToCompactString());
				}
			}
			UE_LOG(LogKulusturDump, Log, TEXT("   comp %s (%s)%s"), *Comp->GetName(), *Comp->GetClass()->GetName(), *Extra);
			if (Cast<UBlueprintGeneratedClass>(Comp->GetClass()))
			{
				DumpProps(Comp, Comp->GetClass()->GetDefaultObject(), false, TEXT("      "));
			}
		}
		DumpProps(A, C->GetDefaultObject(), !bFirst, TEXT("   "));
	}
	UE_LOG(LogKulusturDump, Log, TEXT("DUMP bitti"));
}
