#include "KulusturPackage.h"

#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

DEFINE_LOG_CATEGORY_STATIC(LogKulusturPkg, Log, All);

namespace KulusturPackage
{
	const TCHAR* JumpPadA = TEXT("/Game/3DPlatformerGameBundle/Blueprints/PlatformerActors/Jump_Pad_A_BP.Jump_Pad_A_BP_C");
	const TCHAR* JumpPadB = TEXT("/Game/3DPlatformerGameBundle/Blueprints/PlatformerActors/Jump_Pad_B_BP.Jump_Pad_B_BP_C");
	const TCHAR* BoostRingA = TEXT("/Game/3DPlatformerGameBundle/Blueprints/PlatformerActors/Boost_Ring_A_BP.Boost_Ring_A_BP_C");
	const TCHAR* BoostRingB = TEXT("/Game/3DPlatformerGameBundle/Blueprints/PlatformerActors/Boost_Ring_B_BP.Boost_Ring_B_BP_C");
	const TCHAR* ArcSpring = TEXT("/Game/3DPlatformerGameBundle/Blueprints/PlatformerActors/Arc_Spring_BP.Arc_Spring_BP_C");
	const TCHAR* UpSpringA = TEXT("/Game/3DPlatformerGameBundle/Blueprints/PlatformerActors/Jump_Up_Spring_A_BP.Jump_Up_Spring_A_BP_C");
	const TCHAR* UpSpringB = TEXT("/Game/3DPlatformerGameBundle/Blueprints/PlatformerActors/Jump_Up_Spring_B_BP.Jump_Up_Spring_B_BP_C");
	const TCHAR* PulleyZipline = TEXT("/Game/3DPlatformerGameBundle/Blueprints/PlatformerActors/Zipline_BP.Zipline_BP_C");
	const TCHAR* Rail = TEXT("/Game/3DPlatformerGameBundle/Blueprints/GrindRailActors/Spline_Mesh_BP.Spline_Mesh_BP_C");
	const TCHAR* Coin = TEXT("/Game/3DPlatformerGameBundle/Blueprints/GrindRailActors/Coin_BP.Coin_BP_C");
	const TCHAR* SpeedUp = TEXT("/Game/3DPlatformerGameBundle/Blueprints/GrindRailActors/Speed_Up_Item_BP.Speed_Up_Item_BP_C");
	const TCHAR* Pulley = TEXT("/Game/3DPlatformerGameBundle/Blueprints/GrindRailActors/Pulley_BP.Pulley_BP_C");
	const TCHAR* HiddenTarget = TEXT("/Game/3DPlatformerGameBundle/Blueprints/HomingAttackActors/Hidden_Target_BP.Hidden_Target_BP_C");

	UClass* GetClass(const TCHAR* Path)
	{
		static TMap<FString, TWeakObjectPtr<UClass>> Cache;
		if (TWeakObjectPtr<UClass>* Found = Cache.Find(Path))
		{
			if (Found->IsValid()) return Found->Get();
		}
		UClass* C = LoadClass<AActor>(nullptr, Path);
		if (!C)
		{
			UE_LOG(LogKulusturPkg, Warning, TEXT("Paket sinifi yuklenemedi: %s"), Path);
		}
		Cache.Add(Path, C);
		return C;
	}

	bool SetProp(UObject* Obj, FName Name, const FString& Value)
	{
		if (!Obj) return false;
		FProperty* P = FindFProperty<FProperty>(Obj->GetClass(), Name);
		if (!P)
		{
			UE_LOG(LogKulusturPkg, Warning, TEXT("%s icinde '%s' degiskeni yok"), *Obj->GetClass()->GetName(), *Name.ToString());
			return false;
		}
		const TCHAR* Result = P->ImportText_Direct(*Value, P->ContainerPtrToValuePtr<void>(Obj), Obj, PPF_None);
		if (!Result)
		{
			UE_LOG(LogKulusturPkg, Warning, TEXT("'%s' = '%s' yazilamadi"), *Name.ToString(), *Value);
			return false;
		}
		return true;
	}

	FString Vec(const FVector& V)
	{
		return FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), V.X, V.Y, V.Z);
	}

	AActor* SpawnConfigured(UWorld* World, const TCHAR* ClassPath, const FTransform& Transform, const TMap<FName, FString>& Props)
	{
		UClass* C = GetClass(ClassPath);
		if (!World || !C) return nullptr;
		AActor* A = World->SpawnActorDeferred<AActor>(C, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!A) return nullptr;
		for (const TPair<FName, FString>& P : Props)
		{
			SetProp(A, P.Key, P.Value);
		}
		// Fırlatma yolunu hesaplarken aktör kendine çarpmasın (paket örneklerinde de böyle).
		if (FArrayProperty* Arr = FindFProperty<FArrayProperty>(C, TEXT("ActorsToIgnore (TracePath)")))
		{
			if (FObjectPropertyBase* Inner = CastField<FObjectPropertyBase>(Arr->Inner))
			{
				FScriptArrayHelper Helper(Arr, Arr->ContainerPtrToValuePtr<void>(A));
				const int32 Idx = Helper.AddValue();
				Inner->SetObjectPropertyValue(Helper.GetRawPtr(Idx), A);
			}
		}
		A->FinishSpawning(Transform);
		return A;
	}

	AActor* SpawnRail(UWorld* World, const TArray<FVector>& Points, bool bZipline, const TMap<FName, FString>& Props)
	{
		if (Points.Num() < 2) return nullptr;
		UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(GetClass(Rail));
		if (!BPGC) return nullptr;

		// Rayın şekli kurulum sırasında Blueprint şablonundaki spline'dan okunur.
		// Bu yüzden her raydan önce şablonun noktalarını değiştiriyoruz.
		USplineComponent* Template = nullptr;
		for (UBlueprintGeneratedClass* C = BPGC; C && !Template; C = Cast<UBlueprintGeneratedClass>(C->GetSuperClass()))
		{
			if (!C->SimpleConstructionScript) continue;
			for (USCS_Node* Node : C->SimpleConstructionScript->GetAllNodes())
			{
				if (USplineComponent* S = Cast<USplineComponent>(Node ? Node->GetActualComponentTemplate(BPGC) : nullptr))
				{
					Template = S;
					break;
				}
			}
		}
		if (!Template)
		{
			UE_LOG(LogKulusturPkg, Warning, TEXT("Ray sablonunda spline bulunamadi"));
			return nullptr;
		}
		TArray<FVector> Local;
		for (const FVector& P : Points) Local.Add(P - Points[0]);
		Template->SetSplinePoints(Local, ESplineCoordinateSpace::Local, true);

		TMap<FName, FString> All = Props;
		All.Add(TEXT("CreateZipline?"), bZipline ? TEXT("True") : TEXT("False"));
		return SpawnConfigured(World, Rail, FTransform(Points[0]), All);
	}

	AActor* SpawnLauncher(UWorld* World, const TCHAR* ClassPath, const FVector& LaunchFrom, const FVector& LandAt, const TMap<FName, FString>& ExtraProps)
	{
		TMap<FName, FString> Props = ExtraProps;
		const FString Rel = Vec(LaunchFrom - LandAt);
		UClass* C = GetClass(ClassPath);
		if (!C) return nullptr;
		// Her fırlatıcının "kendi konumu" değişkeninin adı farklı
		for (const TCHAR* Name : { TEXT("LaunchLocation"), TEXT("RingLocation"), TEXT("SpringLocation") })
		{
			if (FindFProperty<FProperty>(C, Name))
			{
				Props.Add(Name, Rel);
			}
		}
		return SpawnConfigured(World, ClassPath, FTransform(LandAt), Props);
	}
}
