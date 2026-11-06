#include "KulusturGameMode.h"

#include "KulusturDirector.h"
#include "KulusturHUD.h"
#include "KulusturDump.h"
#include "KulusturPackage.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Components/SplineComponent.h"
#include "Misc/CommandLine.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

AKulusturGameMode::AKulusturGameMode()
{
	// Paketteki hazır karakter: zıplama, homing attack, ray ve zipline bileşenleri takılı.
	static ConstructorHelpers::FClassFinder<APawn> PawnFinder(TEXT("/Game/3DPlatformerGameBundle/Demo/ThirdPersonAssets/Blueprints/ThirdPersonCharacter_BP"));
	if (PawnFinder.Succeeded())
	{
		DefaultPawnClass = PawnFinder.Class;
	}
	HUDClass = AKulusturHUD::StaticClass();
}

void AKulusturGameMode::StartPlay()
{
	if (FParse::Param(FCommandLine::Get(), TEXT("KulusturDump")))
	{
		KulusturDump::DumpWorld(GetWorld());
		FPlatformMisc::RequestExit(false);
		Super::StartPlay();
		return;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("KulusturExampleTest")))
	{
		UWorld* W = GetWorld();
		FTimerHandle H;
		int32* Step = new int32(0);
		W->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([W, Step]()
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(W, 0);
			++(*Step);
			if (*Step == 3 && Pawn)
			{
				FVector Target = FVector::ZeroVector;
				for (TActorIterator<AActor> It(W); It; ++It)
				{
					const FString CN = It->GetClass()->GetName();
					if (CN.StartsWith(TEXT("Spline_Mesh_BP")))
					{
						const USplineComponent* Sp = It->FindComponentByClass<USplineComponent>();
						Target = Sp->GetLocationAtDistanceAlongSpline(Sp->GetSplineLength() * 0.3f, ESplineCoordinateSpace::World) + FVector(0, 0, 200);
						UE_LOG(LogTemp, Log, TEXT("EXTEST ray %s"), *It->GetName());
						break;
					}
					if (CN.StartsWith(TEXT("Jump_Pad_A_BP")))
					{
						for (UActorComponent* C : It->GetComponents())
						{
							if (C->GetName() == TEXT("Box")) Target = Cast<USceneComponent>(C)->GetComponentLocation() + FVector(0, 0, 150);
						}
						UE_LOG(LogTemp, Log, TEXT("EXTEST pad %s"), *It->GetName());
						break;
					}
				}
				Pawn->SetActorLocation(Target);
			}
			UE_LOG(LogTemp, Log, TEXT("EXTEST step=%d pawn=%s %s"), *Step, Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("-"), Pawn ? *Pawn->GetClass()->GetName() : TEXT(""));
			if (*Step > 16) FPlatformMisc::RequestExit(false);
		}), 0.5f, true);
		Super::StartPlay();
		return;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("KulusturPkgTest")))
	{
		using namespace KulusturPackage;
		UWorld* W = GetWorld();
		if (FParse::Param(FCommandLine::Get(), TEXT("WithPulley")))
		{
			AActor* Pul = SpawnConfigured(W, TEXT("/Game/3DPlatformerGameBundle/Blueprints/GrindRailActors/Pulley_BP.Pulley_BP_C"), FTransform(FVector(0, 0, -5000)), {});
			UE_LOG(LogTemp, Log, TEXT("PKGTEST pulley=%s"), *GetNameSafe(Pul));
		}
		AActor* R = SpawnRail(W, { FVector(600, 0, 300), FVector(4000, 0, 700), FVector(9000, 0, 400) }, false, { { TEXT("OverrideRailSpeed"), TEXT("0") } });
		AActor* Z = SpawnRail(W, { FVector(0, 1500, 1500), FVector(7000, 1500, 500) }, true, {});
		AActor* P = SpawnLauncher(W, JumpPadA, FVector(500, -1500, 135), FVector(4000, -1500, 0));
		AActor* Q = SpawnConfigured(W, PulleyZipline, FTransform(FVector(3000, 3000, 2500)), { { TEXT("PulleyLocation"), TEXT("(X=0,Y=0,Z=-1800)") }, { TEXT("CableLength"), TEXT("18") }, { TEXT("PulleyStartLocation"), Vec(FVector(3000, 3000, 700)) } });
		AActor* S = SpawnConfigured(W, UpSpringA, FTransform(FVector(1500, -3000, 100)), { { TEXT("LaunchHeight"), TEXT("900") } });
		for (AActor* A : { R, Z, P, Q, S })
		{
			if (!A) { UE_LOG(LogTemp, Warning, TEXT("PKGTEST spawn basarisiz")); continue; }
			TArray<UActorComponent*> Comps; A->GetComponents(Comps);
			const USplineComponent* Sp = A->FindComponentByClass<USplineComponent>();
			UE_LOG(LogTemp, Log, TEXT("PKGTEST %s comps=%d splineLen=%.0f"), *A->GetName(), Comps.Num(), Sp ? Sp->GetSplineLength() : -1.f);
		}
		FTimerHandle H;
		int32* Step = new int32(0);
		W->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([W, Step]()
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(W, 0);
			APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0);
			++(*Step);
			const bool bV2 = FParse::Param(FCommandLine::Get(), TEXT("PawnVer2"));
			const bool bComb = FParse::Param(FCommandLine::Get(), TEXT("PawnCombined"));
			if (*Step == 1 && (bV2 || bComb) && PC)
			{
				UClass* V2 = LoadClass<APawn>(nullptr, bV2 ? TEXT("/Game/3DPlatformerGameBundle/Demo/ThirdPersonAssets/Blueprints/GrindRailCharacterBP/Ver_2_ThirdPersonCharacter_BP.Ver_2_ThirdPersonCharacter_BP_C") : TEXT("/Game/3DPlatformerGameBundle/Demo/ThirdPersonAssets/Blueprints/ThirdPersonCharacter_BP.ThirdPersonCharacter_BP_C"));
				APawn* NP = W->SpawnActor<APawn>(V2, FTransform(FVector(-500, 0, 200)));
				if (Pawn) Pawn->Destroy();
				PC->Possess(NP);
				Pawn = NP;
			}
			FString Only; FParse::Value(FCommandLine::Get(), TEXT("Only="), Only);
			if (*Step == 3 && Pawn && Only.IsEmpty()) { Pawn->SetActorLocation(FVector(1500, 0, 900)); }
			if (*Step == 3 && Pawn && Only == TEXT("Spring")) { Pawn->SetActorLocation(FVector(1500, -3000, 400)); }
			if (*Step == 3 && Pawn && Only == TEXT("Pulley")) { Pawn->SetActorLocation(FVector(3000, 3000, 2500 - 1800 + 50)); }
			if (*Step == (Only == TEXT("Pad") ? 3 : 12) && Pawn && (Only.IsEmpty() || Only == TEXT("Pad")))
			{
				for (TActorIterator<AActor> It(W); It; ++It)
				{
					if (It->GetClass()->GetName().StartsWith(TEXT("Jump_Pad_A")))
					{
						for (UActorComponent* C : It->GetComponents())
						{
							if (C->GetName() == TEXT("Box")) { Pawn->SetActorLocation(Cast<USceneComponent>(C)->GetComponentLocation() + FVector(0, 0, 150)); }
						}
					}
				}
			}
			UE_LOG(LogTemp, Log, TEXT("PKGTEST step=%d pawn=%s"), *Step, Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("-"));
			if ((*Step == 4 || *Step == 14) && PC) PC->ConsoleCommand(TEXT("shot"));
			if (*Step == 5 && FParse::Param(FCommandLine::Get(), TEXT("DumpToo"))) KulusturDump::DumpWorld(W);
			if (*Step > 22) FPlatformMisc::RequestExit(false);
		}), 0.5f, true);
		Super::StartPlay();
		return;
	}

	// Haritada yönetici yoksa bölümü biz kuralım.
	if (!TActorIterator<AKulusturDirector>(GetWorld()))
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AKulusturDirector>(AKulusturDirector::StaticClass(), FTransform::Identity, P);
	}
	Super::StartPlay();
}

APawn* AKulusturGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	FString M = UGameplayStatics::ParseOption(OptionsString, TEXT("Mod"));
	if (M.IsEmpty())
	{
		FParse::Value(FCommandLine::Get(), TEXT("KulusturMode="), M);
	}
	// Eski paket test bayrakları karakter ister.
	const bool bPkgTest = FParse::Param(FCommandLine::Get(), TEXT("KulusturPkgTest")) || FParse::Param(FCommandLine::Get(), TEXT("KulusturExampleTest"));
	if (!bPkgTest && M != TEXT("Level") && M != TEXT("Endless"))
	{
		return nullptr;
	}
	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}
