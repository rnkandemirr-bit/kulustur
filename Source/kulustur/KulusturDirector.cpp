#include "KulusturDirector.h"

#include "KulusturEnvironment.h"
#include "KulusturHUD.h"
#include "KulusturSave.h"
#include "KulusturTrack.h"
#include "KulusturTraffic.h"
#include "KulusturVehicle.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "KulusturPackage.h"
#include "InputAction.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogKulustur, Log, All);

#define LOCTEXT_NAMESPACE "Kulustur"

namespace
{
	const TCHAR* FxBurst = TEXT("/Game/3DPlatformerGameBundle/NiagaraSystems/Sphere_Burst_FXS.Sphere_Burst_FXS");
	const TCHAR* FxWave = TEXT("/Game/3DPlatformerGameBundle/NiagaraSystems/Circle_Wave_FXS.Circle_Wave_FXS");
	const TCHAR* FxSparks = TEXT("/Game/3DPlatformerGameBundle/NiagaraSystems/Sparks_FXS.Sparks_FXS");
	const TCHAR* FxSpeed = TEXT("/Game/3DPlatformerGameBundle/NiagaraSystems/SpeedLines_FXS.SpeedLines_FXS");

	void SpawnFx(UWorld* W, const TCHAR* Path, const FVector& Loc, float Scale = 1.f)
	{
		static TMap<FString, TWeakObjectPtr<UNiagaraSystem>> Cache;
		TWeakObjectPtr<UNiagaraSystem>& S = Cache.FindOrAdd(Path);
		if (!S.IsValid())
		{
			S = LoadObject<UNiagaraSystem>(nullptr, Path);
		}
		if (S.IsValid())
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, S.Get(), Loc, FRotator::ZeroRotator, FVector(Scale));
		}
	}

	const TCHAR* MusicFor(EKulusturBiome B)
	{
		switch (B)
		{
		case EKulusturBiome::Col: return TEXT("Apple_Cider_Cue");
		case EKulusturBiome::Yagmur:
		case EKulusturBiome::Sehir: return TEXT("Robotic_City_Cue");
		default: return TEXT("Flowerbed_fields_Cue");
		}
	}
	const TCHAR* EndlessMusic = TEXT("3xBlast_The_Best_Move_Is_To_Keep_Going_Looped_Cue");
}

AKulusturDirector::AKulusturDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	PlayerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PlayerLight"));
	PlayerLight->SetupAttachment(GetRootComponent());
	PlayerLight->SetIntensity(0.f);
	PlayerLight->SetAttenuationRadius(2600.f);
	PlayerLight->SetCastShadows(false);
	PlayerLight->SetLightColor(FLinearColor(1.f, 0.9f, 0.75f));
}

FString AKulusturDirector::FormatTime(float S)
{
	const int32 M = FMath::FloorToInt(S / 60.f);
	const float R = S - M * 60.f;
	return FString::Printf(TEXT("%d:%05.2f"), M, R);
}

ACharacter* AKulusturDirector::GetPlayer() const
{
	return Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

// ---------------------------------------------------------------------------------------------
// Kurulum

void AKulusturDirector::ReadOptions()
{
	const FString& Opt = GetWorld()->GetAuthGameMode() ? GetWorld()->GetAuthGameMode()->OptionsString : FString();
	const FString M = UGameplayStatics::ParseOption(Opt, TEXT("Mod"));
	if (M == TEXT("Level")) Mode = EKulusturMode::Level;
	else if (M == TEXT("Endless")) Mode = EKulusturMode::Endless;
	else Mode = EKulusturMode::Menu;

	// Komut satırından test: -KulusturMode=Level -KulusturWorld=2 ...
	FString CmdMode;
	if (FParse::Value(FCommandLine::Get(), TEXT("KulusturMode="), CmdMode) && M.IsEmpty())
	{
		if (CmdMode == TEXT("Level")) Mode = EKulusturMode::Level;
		if (CmdMode == TEXT("Endless")) Mode = EKulusturMode::Endless;
	}

	UKulusturSave* Save = UKulusturSave::Get();
	World = FMath::Clamp(UGameplayStatics::GetIntOption(Opt, TEXT("Dunya"), Save->LastWorld), 0, KulusturNumWorlds - 1);
	Level = FMath::Clamp(UGameplayStatics::GetIntOption(Opt, TEXT("Bolum"), 0), 0, KulusturLevelsPerWorld - 1);
	int32 CW = -1, CL = -1;
	if (FParse::Value(FCommandLine::Get(), TEXT("KulusturWorld="), CW) && !Opt.Contains(TEXT("Dunya"))) World = FMath::Clamp(CW, 0, 4);
	if (FParse::Value(FCommandLine::Get(), TEXT("KulusturLevel="), CL) && !Opt.Contains(TEXT("Bolum"))) Level = FMath::Clamp(CL, 0, 9);
	Deaths = UGameplayStatics::GetIntOption(Opt, TEXT("Olum"), 0);
	SelWorld = World;
	SelLevel = Level;

	FString Scr = UGameplayStatics::ParseOption(Opt, TEXT("Ekran"));
	if (Scr.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("KulusturScreen="), Scr);
	Screen = EKulusturScreen::Main;
	if (Scr == TEXT("Worlds")) Screen = EKulusturScreen::Worlds;
	if (Scr == TEXT("Levels")) Screen = EKulusturScreen::Levels;
	if (Scr == TEXT("Leaderboard")) Screen = EKulusturScreen::Leaderboard;
	if (Scr == TEXT("Settings")) Screen = EKulusturScreen::Settings;
	if (Scr == TEXT("Help")) Screen = EKulusturScreen::Help;
	if (Scr == TEXT("GameComplete")) Screen = EKulusturScreen::GameComplete;
}

void AKulusturDirector::BeginPlay()
{
	Super::BeginPlay();
	bTest = FParse::Param(FCommandLine::Get(), TEXT("KulusturTest"));
	bAuto = FParse::Param(FCommandLine::Get(), TEXT("KulusturAuto"));
	TestStart = 0.0;

	// Şablon haritanın zeminini kaldır; zemini biz kuruyoruz.
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	ReadOptions();

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Track = GetWorld()->SpawnActor<AKulusturTrack>(AKulusturTrack::StaticClass(), FTransform::Identity, P);
	Traffic = GetWorld()->SpawnActor<AKulusturTraffic>(AKulusturTraffic::StaticClass(), FTransform::Identity, P);
	Env = GetWorld()->SpawnActor<AKulusturEnvironment>(AKulusturEnvironment::StaticClass(), FTransform::Identity, P);

	Traffic->OnCrash = [this](const FVector& Loc, bool bExplosion)
	{
		const float D = FMath::Abs(Loc.X - PlayerX);
		if (bExplosion)
		{
			PlayAt(TEXT("Patlama"), Loc, 1.f);
			SpawnFx(GetWorld(), FxBurst, Loc, 4.f);
			SpawnFx(GetWorld(), FxWave, Loc, 3.f);
			if (ACharacter* C = GetPlayer())
			{
				const FVector Dd = C->GetActorLocation() - Loc;
				if (State == EKulusturState::Playing && Dd.Size() < 1500.f)
				{
					C->LaunchCharacter(Dd.GetSafeNormal2D() * 700.f + FVector(0.f, 0.f, 1500.f), false, true);
					Banner(TEXT("BOOM!"), TEXT("Tanker patladı"), FLinearColor(1.f, 0.5f, 0.1f), 1.2f);
				}
			}
		}
		else if (D < 9000.f)
		{
			PlayAt(TEXT("Carpisma"), Loc, 0.9f, FMath::FRandRange(0.85f, 1.15f));
		}
	};
	Env->OnThunder = [this]() { Play2D(TEXT("Gok"), 0.9f, FMath::FRandRange(0.8f, 1.1f)); };

	if (Mode == EKulusturMode::Menu)
	{
		SetupMenu();
	}
	else
	{
		SetupLevel();
	}
}

void AKulusturDirector::SetupMenu()
{
	UKulusturSave* Save = UKulusturSave::Get();
	Config = KulusturMakeLevelConfig(SelWorld, 3);
	Config.Seed = 777 + SelWorld;
	Config.Challenges = KChallenge::None;
	CurBiome = Config.Biome;
	Track->Init(EKulusturMode::Menu, Config);
	Traffic->Init(Track, EKulusturMode::Menu, Config, 0.f);
	Env->Init(CurBiome, 0);
	State = EKulusturState::Menu;
	// Video çekimi: menüde dolu bir kayıt görünsün
	if (FParse::Param(FCommandLine::Get(), TEXT("KulusturVideoSave")) && Save->PlayerName.IsEmpty())
	{
		Save->PlayerName = TEXT("Claude");
		for (int32 W = 0; W < 4; ++W)
		{
			for (int32 L = 0; L < (W < 3 ? 10 : 4); ++L)
			{
				Save->RecordLevel(W, L, 20.f + L * 3.f + W * 5.f, (L + W) % 4 == 3 ? 2 : 3);
			}
		}
		Save->RecordEndless(2140, 96);
		Save->Write();
	}
	if (Save->PlayerName.IsEmpty() && Screen == EKulusturScreen::Main)
	{
		Screen = EKulusturScreen::NameEntry;
	}

	FActorSpawnParameters P;
	MenuCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform(FVector(-1500.f, 2000.f, 900.f)), P);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetViewTarget(MenuCam);
		AddTickPrerequisiteActor(PC);
	}
	SetScreen(Screen);
	bMusicEndless = true;
	Music = Loop(EndlessMusic, 0.f);
	RumbleLoop = Loop(TEXT("Ugultu"), 0.f);
}

void AKulusturDirector::SetupLevel()
{
	if (Mode == EKulusturMode::Level)
	{
		Config = KulusturMakeLevelConfig(World, Level);
		UKulusturSave* Save = UKulusturSave::Get();
		Save->LastWorld = World;
	}
	else
	{
		Config = KulusturMakeEndlessConfig(0.f);
		Config.Seed = (int32)(FDateTime::Now().GetTicks() % 100000);
	}
	CurBiome = Mode == EKulusturMode::Endless ? KulusturEndlessBiomeAt(0.f) : Config.Biome;
	ActiveChallenges = Config.Challenges;
	WindStrength = Config.WindStrength;

	Track->Init(Mode, Config);
	Traffic->Init(Track, Mode, Config, 0.f);
	Env->Init(CurBiome, ActiveChallenges);

	State = EKulusturState::Playing;
	Screen = EKulusturScreen::None;
	SetScreen(EKulusturScreen::None);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		AddTickPrerequisiteActor(PC);
	}

	if (Mode == EKulusturMode::Level)
	{
		const FKulusturBiomeStyle& S = KulusturGetBiomeStyle(Config.Biome);
		Banner(FString::Printf(TEXT("%d-%d  %s"), World + 1, Level + 1, *Config.Title.ToString()), Config.Hint.ToString(), S.UIColor, 4.f);
		StartMusic(Config.Biome);
	}
	else
	{
		Banner(TEXT("SONSUZ MOD"), TEXT("Ne kadar uzağa gidebilirsin?"), FLinearColor(1.f, 0.8f, 0.2f), 3.5f);
		Music = Loop(EndlessMusic, 0.f);
		bMusicEndless = true;
	}

	RumbleLoop = Loop(TEXT("Ugultu"), 0.f);
	WindLoop = Loop(TEXT("Ruzgar"), 0.f);
	RainLoop = Loop(TEXT("Yagmur"), 0.f);
	CityLoop = Loop(TEXT("Sehir"), 0.f);
	LaserLoop = Loop(TEXT("Lazer"), 0.f);
}

void AKulusturDirector::PlacePlayer(ACharacter* C)
{
	AKulusturVehicle* V = Traffic ? Traffic->GetStartVehicle() : nullptr;
	if (!V) return;
	const float HalfH = C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Loc = V->RoofLocation() + FVector(-300.f, 0.f, HalfH + 30.f);
	C->SetActorLocationAndRotation(Loc, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* Ctrl = C->GetController())
	{
		Ctrl->SetControlRotation(FRotator(-12.f, 0.f, 0.f));
	}
	if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
	{
		Move->Velocity = FVector(V->Speed, 0.f, 0.f);
		// Araçlar karakterden önce hareket etsin ki üstlerinde titremeden dursun.
		Move->AddTickPrerequisiteActor(this);
		DefaultGroundFriction = Move->GroundFriction;
		DefaultBraking = Move->BrakingDecelerationWalking;
		if (FParse::Param(FCommandLine::Get(), TEXT("KulusturTestJump")))
		{
			UE_LOG(LogTemp, Warning, TEXT("DUMP grav=%.2f jumpZ=%.0f air=%.2f walk=%.0f acc=%.0f jumps=%d brakeFall=%.0f latFric=%.2f falling=%.0f"),
				Move->GravityScale, Move->JumpZVelocity, Move->AirControl, Move->MaxWalkSpeed, Move->MaxAcceleration, C->JumpMaxCount, Move->BrakingDecelerationFalling, Move->FallingLateralFriction, Move->MaxFlySpeed);
		}
	}
	StartX = Loc.X;
	LastPlayerLoc = Loc;
	bPlaced = true;
	SetupFirstPerson(C);
	RemovePackageWidgets();
}

// ---------------------------------------------------------------------------------------------
// Ses

USoundBase* AKulusturDirector::Snd(const TCHAR* Name)
{
	const FName Key(Name);
	if (TObjectPtr<USoundBase>* Found = SoundCache.Find(Key))
	{
		return *Found;
	}
	USoundBase* S = nullptr;
	const FString N(Name);
	const FString Paths[] = {
		FString::Printf(TEXT("/Game/Kulustur/Audio/%s.%s"), *N, *N),
		FString::Printf(TEXT("/Game/3DPlatformerGameBundle/Audio/Cues/%s.%s"), *N, *N),
		FString::Printf(TEXT("/Game/3DPlatformerGameBundle/Audio/WAV/%s.%s"), *N, *N) };
	for (const FString& P : Paths)
	{
		if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(P)))
		{
			S = LoadObject<USoundBase>(nullptr, *P);
			if (S) break;
		}
	}
	if (!S) UE_LOG(LogKulustur, Verbose, TEXT("Ses yok: %s"), Name);
	SoundCache.Add(Key, S);
	return S;
}

void AKulusturDirector::Play2D(const TCHAR* Name, float Volume, float Pitch)
{
	if (USoundBase* S = Snd(Name))
	{
		UGameplayStatics::PlaySound2D(this, S, Volume * UKulusturSave::Get()->MasterVolume, Pitch);
	}
}

void AKulusturDirector::PlayAt(const TCHAR* Name, const FVector& Loc, float Volume, float Pitch)
{
	if (USoundBase* S = Snd(Name))
	{
		// Mesafe zayıflatması paket seslerinde yok olabilir; elle kısıyoruz.
		const float D = FVector::Dist(Loc, LastPlayerLoc);
		const float Att = FMath::Clamp(1.f - D / 9000.f, 0.15f, 1.f);
		UGameplayStatics::PlaySound2D(this, S, Volume * Att * UKulusturSave::Get()->MasterVolume, Pitch);
	}
}

UAudioComponent* AKulusturDirector::Loop(const TCHAR* Name, float Volume)
{
	USoundBase* S = Snd(Name);
	if (!S) return nullptr;
	UAudioComponent* A = UGameplayStatics::CreateSound2D(this, S, FMath::Max(Volume, 0.001f), 1.f, 0.f, nullptr, false, false);
	if (A)
	{
		A->bAutoDestroy = false;
		A->Play();
	}
	return A;
}

void AKulusturDirector::StartMusic(EKulusturBiome Biome)
{
	if (Music) Music->Stop();
	MusicBiome = Biome;
	bMusicEndless = false;
	Music = Loop(MusicFor(Biome), 0.f);
}

void AKulusturDirector::TickAudio(float Dt, ACharacter* C)
{
	UKulusturSave* Save = UKulusturSave::Get();
	const float Master = Save->MasterVolume;
	auto Fade = [Dt](UAudioComponent* A, float Target)
	{
		if (!A) return;
		const float Cur = A->VolumeMultiplier;
		A->SetVolumeMultiplier(FMath::Max(0.001f, FMath::FInterpTo(Cur, Target, Dt, 1.5f)));
	};

	// Müzik (biterse yeniden başlat)
	if (Music)
	{
		Fade(Music, Save->MusicVolume * Master * (State == EKulusturState::Paused ? 0.4f : 1.f));
		if (!Music->IsPlaying()) Music->Play();
	}

	const EKulusturBiome B = CurBiome;
	const float InAir = (C && C->GetCharacterMovement() && C->GetCharacterMovement()->IsFalling()) ? 1.f : 0.f;
	float Wind = 0.12f;
	if (B == EKulusturBiome::Col) Wind = 0.45f;
	if (B == EKulusturBiome::Kar) Wind = 0.4f;
	if (B == EKulusturBiome::Yagmur) Wind = 0.3f;
	if (ActiveChallenges & KChallenge::Wind) Wind += 0.25f;
	Wind += InAir * 0.15f;
	const bool bMenu = Mode == EKulusturMode::Menu;
	Fade(WindLoop, Wind * Master * (bMenu ? 0.5f : 1.f));
	Fade(RainLoop, (B == EKulusturBiome::Yagmur ? 0.7f : 0.f) * Master);
	Fade(CityLoop, (B == EKulusturBiome::Sehir ? 0.45f : 0.f) * Master);
	Fade(RumbleLoop, 0.35f * Master);
	Fade(LaserLoop, (LaserDist > 0.f ? FMath::Clamp(1.f - LaserDist / 5000.f, 0.f, 1.f) * 0.6f : 0.f) * Master);
	for (UAudioComponent* A : { WindLoop.Get(), RainLoop.Get(), CityLoop.Get(), RumbleLoop.Get(), LaserLoop.Get() })
	{
		if (A && !A->IsPlaying()) A->Play();
	}

	// Ara sıra korna
	static float HornTimer = 6.f;
	HornTimer -= Dt;
	if (HornTimer <= 0.f && Traffic)
	{
		HornTimer = FMath::FRandRange(5.f, 13.f);
		for (const AKulusturVehicle* V : Traffic->GetVehicles())
		{
			if (V && !V->IsCrashed() && FMath::Abs(V->GetActorLocation().X - PlayerX) < 4000.f && FMath::FRand() < 0.3f)
			{
				PlayAt(TEXT("Korna"), V->GetActorLocation(), 0.5f, FMath::FRandRange(0.85f, 1.2f));
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Ekranlar ve tuşlar

void AKulusturDirector::SetScreen(EKulusturScreen S)
{
	if (S != Screen) PrevScreen = Screen;
	Screen = S;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;
	const bool bUI = S != EKulusturScreen::None;
	PC->bShowMouseCursor = bUI;
	if (bUI)
	{
		FInputModeGameAndUI M;
		M.SetHideCursorDuringCapture(false);
		PC->SetInputMode(M);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
	if (S == EKulusturScreen::NameEntry)
	{
		NameBuffer = UKulusturSave::Get()->PlayerName;
	}
}

void AKulusturDirector::SetPaused(bool bPause)
{
	if (bPause && State == EKulusturState::Playing)
	{
		State = EKulusturState::Paused;
		UGameplayStatics::SetGamePaused(this, true);
		SetScreen(EKulusturScreen::Pause);
	}
	else if (!bPause && State == EKulusturState::Paused)
	{
		State = EKulusturState::Playing;
		UGameplayStatics::SetGamePaused(this, false);
		SetScreen(EKulusturScreen::None);
	}
}

void AKulusturDirector::OpenMode(EKulusturMode NewMode, int32 W, int32 L, EKulusturScreen MenuScreen)
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	UGameplayStatics::SetGamePaused(this, false);
	UKulusturSave::Get()->Write();
	FString Opt;
	switch (NewMode)
	{
	case EKulusturMode::Level:
		Opt = FString::Printf(TEXT("Mod=Level?Dunya=%d?Bolum=%d?Olum=%d"), W, L, (W == World && L == Level && Mode == EKulusturMode::Level) ? Deaths : 0);
		break;
	case EKulusturMode::Endless:
		Opt = TEXT("Mod=Endless");
		break;
	default:
	{
		const TCHAR* Scr = TEXT("Main");
		if (MenuScreen == EKulusturScreen::Worlds) Scr = TEXT("Worlds");
		if (MenuScreen == EKulusturScreen::Levels) Scr = TEXT("Levels");
		if (MenuScreen == EKulusturScreen::Leaderboard) Scr = TEXT("Leaderboard");
		Opt = FString::Printf(TEXT("Mod=Menu?Dunya=%d?Bolum=%d?Ekran=%s"), W, L, Scr);
		break;
	}
	}
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)), true, Opt);
}

void AKulusturDirector::OnButton(FName Id)
{
	UKulusturSave* Save = UKulusturSave::Get();
	const FString S = Id.ToString();
	Play2D(TEXT("UI_Sound_2_Cue"), 0.6f);

	if (S == TEXT("Play")) { SelWorld = FMath::Clamp(Save->LastWorld, 0, 4); SetScreen(EKulusturScreen::Worlds); return; }
	if (S == TEXT("Endless")) { OpenMode(EKulusturMode::Endless, 0, 0); return; }
	if (S == TEXT("Leaderboard")) { SetScreen(EKulusturScreen::Leaderboard); return; }
	if (S == TEXT("Settings")) { SetScreen(EKulusturScreen::Settings); return; }
	if (S == TEXT("Help")) { SetScreen(EKulusturScreen::Help); return; }
	if (S == TEXT("Name")) { SetScreen(EKulusturScreen::NameEntry); return; }
	if (S == TEXT("Quit")) { Save->Write(); if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->ConsoleCommand(TEXT("quit")); return; }
	if (S == TEXT("NameOK"))
	{
		NameBuffer.TrimStartAndEndInline();
		if (!NameBuffer.IsEmpty())
		{
			Save->PlayerName = NameBuffer;
			Save->Write();
			SetScreen(EKulusturScreen::Main);
		}
		return;
	}
	if (S == TEXT("Back"))
	{
		switch (Screen)
		{
		case EKulusturScreen::Levels: SetScreen(EKulusturScreen::Worlds); break;
		case EKulusturScreen::Pause: SetPaused(false); break;
		default: SetScreen(EKulusturScreen::Main); break;
		}
		return;
	}
	if (S.StartsWith(TEXT("World")))
	{
		const int32 W = FCString::Atoi(*S.RightChop(5));
		if (Save->IsWorldUnlocked(W))
		{
			SelWorld = W;
			SelLevel = 0;
			for (int32 L = 0; L < KulusturLevelsPerWorld; ++L)
			{
				if (Save->IsLevelUnlocked(W, L)) SelLevel = L;
			}
			SetScreen(EKulusturScreen::Levels);
		}
		else
		{
			Play2D(TEXT("Beep_03_Cue"), 0.7f);
		}
		return;
	}
	if (S.StartsWith(TEXT("Level")))
	{
		const int32 L = FCString::Atoi(*S.RightChop(5));
		if (Save->IsLevelUnlocked(SelWorld, L)) SelLevel = L;
		return;
	}
	if (S == TEXT("Start")) { OpenMode(EKulusturMode::Level, SelWorld, SelLevel); return; }
	if (S == TEXT("Resume")) { SetPaused(false); return; }
	if (S == TEXT("Retry"))
	{
		if (Mode == EKulusturMode::Endless) OpenMode(EKulusturMode::Endless, 0, 0);
		else OpenMode(EKulusturMode::Level, World, Level);
		return;
	}
	if (S == TEXT("Next"))
	{
		int32 W = World, L = Level + 1;
		if (L >= KulusturLevelsPerWorld) { L = 0; ++W; }
		if (W >= KulusturNumWorlds) { SetScreen(EKulusturScreen::GameComplete); return; }
		OpenMode(EKulusturMode::Level, W, L);
		return;
	}
	if (S == TEXT("Map")) { OpenMode(EKulusturMode::Menu, World, Level, EKulusturScreen::Levels); return; }
	if (S == TEXT("Menu")) { OpenMode(EKulusturMode::Menu, World, Level, EKulusturScreen::Main); return; }
	if (S == TEXT("Replay")) { OpenMode(EKulusturMode::Level, 0, 0); return; }
	if (S == TEXT("Scores")) { OpenMode(EKulusturMode::Menu, World, Level, EKulusturScreen::Leaderboard); return; }

	auto Adjust = [&](float& V, float D) { V = FMath::Clamp(FMath::RoundToFloat((V + D) * 10.f) / 10.f, 0.f, 1.f); Save->Write(); };
	if (S == TEXT("VolUp")) { Adjust(Save->MasterVolume, 0.1f); return; }
	if (S == TEXT("VolDown")) { Adjust(Save->MasterVolume, -0.1f); return; }
	if (S == TEXT("MusUp")) { Adjust(Save->MusicVolume, 0.1f); return; }
	if (S == TEXT("MusDown")) { Adjust(Save->MusicVolume, -0.1f); return; }
}

void AKulusturDirector::HandleKeys()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// Fare tıklaması: HUD düğmeleri
	if (Screen != EKulusturScreen::None && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		float MX = 0.f, MY = 0.f;
		if (PC->GetMousePosition(MX, MY))
		{
			if (AKulusturHUD* H = Cast<AKulusturHUD>(PC->GetHUD()))
			{
				const FName Id = H->HitTest(FVector2D(MX, MY));
				if (!Id.IsNone())
				{
					OnButton(Id);
					return;
				}
			}
		}
	}

	if (Screen == EKulusturScreen::NameEntry)
	{
		static TArray<TPair<FKey, TCHAR>> Keys;
		if (Keys.Num() == 0)
		{
			for (TCHAR Ch = 'A'; Ch <= 'Z'; ++Ch) Keys.Add({ FKey(FName(*FString::Chr(Ch))), Ch });
			const FKey Digits[] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
			for (int32 i = 0; i < 10; ++i) Keys.Add({ Digits[i], TCHAR('0' + i) });
			Keys.Add({ EKeys::SpaceBar, ' ' });
		}
		for (const TPair<FKey, TCHAR>& K : Keys)
		{
			if (PC->WasInputKeyJustPressed(K.Key) && NameBuffer.Len() < 12)
			{
				NameBuffer.AppendChar(K.Value);
				Play2D(TEXT("Beep_Cue"), 0.25f, 1.5f);
			}
		}
		if (PC->WasInputKeyJustPressed(EKeys::BackSpace) && NameBuffer.Len() > 0)
		{
			NameBuffer.LeftChopInline(1);
		}
		if (PC->WasInputKeyJustPressed(EKeys::Enter)) OnButton(TEXT("NameOK"));
		if (PC->WasInputKeyJustPressed(EKeys::Escape) && !UKulusturSave::Get()->PlayerName.IsEmpty()) SetScreen(EKulusturScreen::Main);
		return;
	}

	const bool bEsc = PC->WasInputKeyJustPressed(EKeys::Escape) || PC->WasInputKeyJustPressed(EKeys::P);
	const bool bEnter = PC->WasInputKeyJustPressed(EKeys::Enter);

	switch (Screen)
	{
	case EKulusturScreen::None:
		if (bEsc && State == EKulusturState::Playing) SetPaused(true);
		if (PC->WasInputKeyJustPressed(EKeys::R) && (State == EKulusturState::Playing || State == EKulusturState::Dead))
		{
			if (Mode == EKulusturMode::Level) { ++Deaths; OpenMode(EKulusturMode::Level, World, Level); }
		}
		break;
	case EKulusturScreen::Pause:
		if (bEsc) SetPaused(false);
		if (PC->WasInputKeyJustPressed(EKeys::R)) OnButton(TEXT("Retry"));
		break;
	case EKulusturScreen::Main:
		if (bEnter) OnButton(TEXT("Play"));
		break;
	case EKulusturScreen::Worlds:
		if (bEsc) SetScreen(EKulusturScreen::Main);
		if (PC->WasInputKeyJustPressed(EKeys::Right) || PC->WasInputKeyJustPressed(EKeys::D)) SelWorld = FMath::Min(SelWorld + 1, 4);
		if (PC->WasInputKeyJustPressed(EKeys::Left) || PC->WasInputKeyJustPressed(EKeys::A)) SelWorld = FMath::Max(SelWorld - 1, 0);
		if (bEnter) OnButton(*FString::Printf(TEXT("World%d"), SelWorld));
		break;
	case EKulusturScreen::Levels:
		if (bEsc) SetScreen(EKulusturScreen::Worlds);
		if ((PC->WasInputKeyJustPressed(EKeys::Right) || PC->WasInputKeyJustPressed(EKeys::D)) && UKulusturSave::Get()->IsLevelUnlocked(SelWorld, SelLevel + 1)) SelLevel = FMath::Min(SelLevel + 1, 9);
		if (PC->WasInputKeyJustPressed(EKeys::Left) || PC->WasInputKeyJustPressed(EKeys::A)) SelLevel = FMath::Max(SelLevel - 1, 0);
		if (bEnter) OnButton(TEXT("Start"));
		break;
	case EKulusturScreen::Results:
		if (bEnter) OnButton(TEXT("Next"));
		if (PC->WasInputKeyJustPressed(EKeys::R)) OnButton(TEXT("Retry"));
		if (bEsc) OnButton(TEXT("Map"));
		break;
	case EKulusturScreen::EndlessOver:
		if (bEnter || PC->WasInputKeyJustPressed(EKeys::R)) OnButton(TEXT("Retry"));
		if (bEsc) OnButton(TEXT("Menu"));
		break;
	case EKulusturScreen::GameComplete:
		if (bEsc) OnButton(TEXT("Menu"));
		break;
	default:
		if (bEsc) OnButton(TEXT("Back"));
		break;
	}
}

void AKulusturDirector::Banner(const FString& Text, const FString& Sub, const FLinearColor& Color, float Duration)
{
	for (FKulusturBanner& B : Banners)
	{
		if (B.Text == Text) { B.Time = 0.f; return; }
	}
	FKulusturBanner B;
	B.Text = Text; B.Sub = Sub; B.Color = Color; B.Duration = Duration;
	Banners.Add(B);
}

// ---------------------------------------------------------------------------------------------
// Ana döngü

void AKulusturDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Real = FMath::Min((float)FApp::GetDeltaTime(), 0.1f);
	StateTime += Real;

	for (int32 i = Banners.Num() - 1; i >= 0; --i)
	{
		Banners[i].Time += Real;
		if (Banners[i].Time > Banners[i].Duration) Banners.RemoveAt(i);
	}

	HandleKeys();

	if (State == EKulusturState::Paused)
	{
		TickAudio(Real, GetPlayer());
		return;
	}

	ACharacter* C = (Mode == EKulusturMode::Menu) ? nullptr : GetPlayer();
	if (C)
	{
		PlayerX = C->GetActorLocation().X;
		PlayerY = C->GetActorLocation().Y;
	}
	else if (Mode == EKulusturMode::Menu)
	{
		PlayerX = MenuCamX;
	}

	if (Track) Track->UpdateStreaming(PlayerX);

	const UPrimitiveComponent* Base = C ? C->GetMovementBase() : nullptr;
	if (Traffic) Traffic->Update(DeltaSeconds, PlayerX, Base ? Base->GetOwner() : nullptr);

	FVector CamLoc(PlayerX, 0.f, 600.f), CamVel(Config.BaseSpeed, 0.f, 0.f);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager) CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	}
	if (Env) Env->UpdateEnv(DeltaSeconds, CamLoc, CamVel);

	if (Mode == EKulusturMode::Menu)
	{
		TickMenuCamera(DeltaSeconds);
	}
	else if (C)
	{
		if (!bPlaced)
		{
			PlacePlayer(C);
		}
		else
		{
			TickPlaying(DeltaSeconds, C);
		}
	}

	if (State == EKulusturState::Dead && StateTime > 1.3f)
	{
		if (Mode == EKulusturMode::Level)
		{
			OpenMode(EKulusturMode::Level, World, Level);
			return;
		}
	}
	if (State == EKulusturState::EndlessOver && StateTime > 1.2f)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	}
	if (State == EKulusturState::Won && StateTime > 1.6f && Screen == EKulusturScreen::None)
	{
		const bool bLast = World == KulusturNumWorlds - 1 && Level == KulusturLevelsPerWorld - 1;
		SetScreen(bLast ? EKulusturScreen::GameComplete : EKulusturScreen::Results);
		if (bLast) Play2D(TEXT("Spacey_1Up_Power_Up_WAV"), 1.f, 0.8f);
	}

	TickAudio(Real, C);

	// --- Test modu ---
	if (bTest)
	{
		TestTimer += Real;
		if (TestTimer >= 1.f)
		{
			TestTimer = 0.f;
			int32 Crashed = 0, Num = 0;
			if (Traffic) for (const AKulusturVehicle* V : Traffic->GetVehicles()) { if (V) { ++Num; if (V->IsCrashed()) ++Crashed; } }
			UE_LOG(LogKulustur, Log, TEXT("TEST t=%.1f mod=%d durum=%d ekran=%d oyuncu=%s taban=%s hiz=%.0f arac=%d devrik=%d altin=%d metre=%d biyom=%d"),
				Elapsed, (int32)Mode, (int32)State, (int32)Screen, C ? *C->GetActorLocation().ToCompactString() : TEXT("yok"),
				Base ? *GetNameSafe(Base->GetOwner()) : TEXT("yok"), SpeedKmh, Num, Crashed, Coins, GetMeters(), (int32)CurBiome);
		}
		// Bölüm yeniden yüklense de test süresi ve görüntü sayısı sürsün.
		static double GTestStart = 0.0;
		static int32 GTestShots = 0;
		if (GTestStart <= 0.0) GTestStart = FPlatformTime::Seconds();
		TestStart = GTestStart;
		const double T = FPlatformTime::Seconds() - TestStart;
		if (FParse::Param(FCommandLine::Get(), TEXT("KulusturShots")))
		{
			const double ShotTimes[] = { 6.0, 12.0, 18.0, 24.0 };
			if (GTestShots < 4 && T > ShotTimes[GTestShots])
			{
				++GTestShots;
				if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->ConsoleCommand(TEXT("shot"));
				UE_LOG(LogKulustur, Log, TEXT("TEST ekran goruntusu %d"), GTestShots);
			}
		}
		float Limit = 40.f;
		FParse::Value(FCommandLine::Get(), TEXT("KulusturTestTime="), Limit);
		// Video çekimi: kare sayısıyla bitir (her kare diske yazıldığından gerçek zaman yavaş akar)
		static int32 GFrames = 0;
		int32 FrameLimit = 0;
		const bool bFrameMode = FParse::Value(FCommandLine::Get(), TEXT("KulusturFrames="), FrameLimit);
		if (bFrameMode) Limit = 1e9f;
		if (bFrameMode && ++GFrames >= FrameLimit)
		{
			UE_LOG(LogKulustur, Log, TEXT("TEST bitti (%d kare)"), GFrames);
			FPlatformMisc::RequestExit(false);
		}
		if (T > Limit)
		{
			UE_LOG(LogKulustur, Log, TEXT("TEST bitti"));
			FPlatformMisc::RequestExit(false);
		}
	}
}

void AKulusturDirector::TickMenuCamera(float Dt)
{
	MenuCamT += Dt;
	MenuCamX += Config.BaseSpeed * Dt;
	if (!MenuCam) return;
	const bool bCity = CurBiome == EKulusturBiome::Sehir;
	const float Side = (bCity ? 700.f : 2600.f) + FMath::Sin(MenuCamT * 0.15f) * (bCity ? 400.f : 500.f);
	const FVector Loc(MenuCamX - 1800.f, Side, 1500.f + FMath::Sin(MenuCamT * 0.21f) * 250.f);
	const FVector Look(MenuCamX + 2500.f, 0.f, 250.f);
	MenuCam->SetActorLocationAndRotation(Loc, (Look - Loc).Rotation());
	LastPlayerLoc = Loc;
}

void AKulusturDirector::TickPlaying(float Dt, ACharacter* C)
{
	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	const FVector Loc = C->GetActorLocation();
	const UPrimitiveComponent* Base = C->GetMovementBase();
	const bool bOnGround = Move && Move->IsMovingOnGround() && Base;

	if (Dt > KINDA_SMALL_NUMBER && PlacedFrames > 1)
	{
		const float Inst = FVector::Dist2D(Loc, LastPlayerLoc) / Dt * 0.036f;
		SpeedKmh = FMath::FInterpTo(SpeedKmh, Inst, Dt, 6.f);
	}
	LastPlayerLoc = Loc;
	++PlacedFrames;

	// Paket kendi kamerasını geri açarsa yine kapat; paket yazıları çıkarsa kaldır.
	for (const TWeakObjectPtr<UCameraComponent>& Cam : OtherCams)
	{
		if (Cam.IsValid() && Cam->IsActive()) Cam->SetActive(false);
	}
	if (FpsCam && !FpsCam->IsActive()) FpsCam->SetActive(true);
	if (Move && !FMath::IsNearlyEqual(Move->JumpZVelocity, 1050.f)) TuneMovement(Move); // paket değiştirirse geri al
	if (Move && Move->MaxWalkSpeed < 800.f) Move->MaxWalkSpeed = 800.f; // tır üstünde daha çevik koşu
	WidgetSweep -= Dt;
	if (WidgetSweep <= 0.f)
	{
		WidgetSweep = 0.5f;
		RemovePackageWidgets();
	}

	// Karanlıkta oyuncunun etrafı aydınlık
	PlayerLight->SetWorldLocation(Loc + FVector(400.f, 0.f, 500.f));
	PlayerLight->SetIntensity((ActiveChallenges & KChallenge::Darkness) ? 60000.f : 0.f);

	if (State != EKulusturState::Playing)
	{
		return;
	}

	// Ağır çekimde süre gerçek zamanla sayılır (yıldız süresi hilesi olmasın)
	Elapsed += bSlowMo ? FMath::Min((float)FApp::GetDeltaTime(), 0.1f) : Dt;
	LaunchCooldown -= Dt;

	// Işınlanmadan sonraki eski "yerdeyim" bilgisi yüzünden ölmeyelim.
	if (!bSafeSeen)
	{
		if (PlacedFrames > 3 && (!bOnGround || !Base->ComponentHasTag(AKulusturTrack::TagDeadly)))
		{
			bSafeSeen = true;
		}
		return;
	}

	// Sonsuz mod: dünya değişimi ve zorlaşma
	if (Mode == EKulusturMode::Endless)
	{
		const EKulusturBiome NB = KulusturEndlessBiomeAt(Loc.X);
		if (NB != CurBiome)
		{
			CurBiome = NB;
			Env->SetBiome(NB);
			const FKulusturBiomeStyle& S = KulusturGetBiomeStyle(NB);
			Banner(FString::Printf(TEXT("YENİ BÖLGE: %s"), *S.Name.ToString()), S.Subtitle.ToString(), S.UIColor, 3.5f);
			Play2D(TEXT("SpeedUp_Cue"), 0.8f);
		}
		static int32 LastMilestone = 0;
		const int32 Ms = GetMeters() / 500;
		if (PlacedFrames < 10) LastMilestone = 0;
		if (Ms > LastMilestone)
		{
			LastMilestone = Ms;
			Banner(FString::Printf(TEXT("%d METRE!"), Ms * 500), UKulusturSave::Get()->BestEndless() > 0 ? FString::Printf(TEXT("Rekor: %d m"), UKulusturSave::Get()->BestEndless()) : FString(), FLinearColor(1.f, 0.85f, 0.2f), 1.6f);
			Play2D(TEXT("Grab_Coin_Cue"), 0.7f, 0.7f);
		}
		static float CfgTimer = 0.f;
		CfgTimer += Dt;
		if (CfgTimer > 1.f)
		{
			CfgTimer = 0.f;
			ApplyEndlessConfig(false);
		}
	}

	TickChallenges(Dt, C);

	// Altın
	if (const int32 Got = Track->CollectCoins(Loc, 170.f))
	{
		Coins += Got;
		Play2D(TEXT("Grab_Coin_Cue"), 0.6f, FMath::FRandRange(0.95f, 1.1f));
	}
	// Video çekimi için oyunu kendi oynayan pilot
	if (bAuto) AutoPilot(Dt, C, bOnGround, Base);
	// Zıplama yardımları, yetenekler, akrobasi
	TickMoves(Dt, C, bOnGround, Base);
	if (State != EKulusturState::Playing) return;
	// Halka
	if (Track->CheckRing(Loc, 380.f))
	{
		C->LaunchCharacter(FVector(FMath::Max(C->GetVelocity().X, 1500.f) + 1300.f, 0.f, 650.f), true, true);
		Play2D(TEXT("Ring_Boing_Cue"), 0.9f);
		Play2D(TEXT("SpeedUp_Cue"), 0.6f);
		SpawnFx(GetWorld(), FxWave, Loc, 2.f);
		Banner(TEXT("HIZ HALKASI!"), FString(), FLinearColor(0.3f, 0.9f, 1.f), 0.9f);
	}
	// Fırlatıcı araç
	if (bOnGround && LaunchCooldown <= 0.f)
	{
		if (const AKulusturVehicle* V = Cast<AKulusturVehicle>(Base->GetOwner()))
		{
			const FVector Feet = Loc - FVector(0.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			if (V->Type == EVehicleType::Firlatici && V->IsOnLaunchPad(Feet))
			{
				LaunchCooldown = 0.6f;
				// Araçtan ayrılırken aracın hızı zaten eklenir; burada sadece ek itiş.
				C->LaunchCharacter(FVector(1700.f, 0.f, 1500.f), true, true);
				Play2D(TEXT("Spring_Hit_Cue"), 1.f);
				Play2D(TEXT("WhooshSpace_shipWhizWipe_WAV"), 0.6f);
				SpawnFx(GetWorld(), FxSparks, Feet, 1.5f);
				SpawnFx(GetWorld(), FxSpeed, Loc, 1.f);
			}
		}
	}

	// Oyuncu sesleri: zıplama, iniş, adım
	{
		const FVector V = Move ? Move->Velocity : FVector::ZeroVector;
		if (bWasOnGround && !bOnGround && Move && Move->IsFalling() && V.Z > 250.f)
		{
			Play2D(TEXT("Jump_Cue"), 0.7f);
		}
		if (!bWasOnGround && bOnGround)
		{
			if (LastZVel < -350.f) Play2D(TEXT("Jumpland_Cue"), FMath::Clamp(-LastZVel / 1500.f, 0.3f, 1.f));
			FootstepTimer = 0.1f;
		}
		if (bOnGround && V.Size2D() > 200.f)
		{
			FootstepTimer -= Dt;
			if (FootstepTimer <= 0.f)
			{
				FootstepTimer = FMath::Clamp(140.f / V.Size2D(), 0.22f, 0.45f);
				Play2D(TEXT("Footsteps_Cue"), 0.45f);
			}
		}
		LastZVel = V.Z;
		bWasOnGround = bOnGround;
	}

	// Ölüm / kazanma
	if (Track->CheckLaserHit(Loc, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), C->GetCapsuleComponent()->GetScaledCapsuleRadius()))
	{
		SpawnFx(GetWorld(), FxSparks, Loc, 2.f);
		Die(TEXT("lazer"));
		return;
	}
	if (bOnGround)
	{
		if (Base->ComponentHasTag(AKulusturTrack::TagDeadly))
		{
			Die(TEXT("yere degdi"));
			return;
		}
		if (Base->ComponentHasTag(AKulusturTrack::TagFinish))
		{
			Win();
			return;
		}
	}
	if (Loc.Z < -300.f)
	{
		Die(TEXT("dustu"));
		return;
	}
	// Bitiş kapısından canlı geçtin mi?
	if (Mode == EKulusturMode::Level && Loc.X >= Track->GetFinishX())
	{
		Win();
		return;
	}

	if (bTest)
	{
		static float JumpT = 0.f;
		JumpT += Dt;
		if (JumpT > 2.5f && bOnGround && !bAuto && !FParse::Param(FCommandLine::Get(), TEXT("KulusturTestJump")))
		{
			JumpT = 0.f;
			C->Jump();
		}
		// Zıplama testi: Space basışını taklit et (yerde bas, havada tekrar bas), hızı logla
		static float JumpTestT = -1.f;
		if (FParse::Param(FCommandLine::Get(), TEXT("KulusturTestJump")) && Elapsed > 3.f)
		{
			static UInputAction* JumpIA = LoadObject<UInputAction>(nullptr, TEXT("/Game/3DPlatformerGameBundle/Demo/ThirdPersonAssets/Input/Actions/Jump_IA.Jump_IA"));
			const APlayerController* PC = Cast<APlayerController>(C->GetController());
			UEnhancedInputLocalPlayerSubsystem* Sub = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
			if (JumpTestT < 0.f) JumpTestT = 0.f;
			const float Prev = JumpTestT;
			JumpTestT += Dt;
			auto Cross = [&](float T) { return Prev < T && JumpTestT >= T; };
			if (Sub && JumpIA)
			{
				if (Cross(0.f + KINDA_SMALL_NUMBER) || Cross(0.6f) || Cross(4.f) || Cross(4.6f)) Sub->StartContinuousInputInjectionForAction(JumpIA, FInputActionValue(true), {}, {});
				if (Cross(0.6f) || Cross(4.6f)) TryAirJump(C); // gerçek oyunda Space tuşu tetikler
				if (Cross(1.0f) && FParse::Param(FCommandLine::Get(), TEXT("KulusturTestDash"))) UE_LOG(LogKulustur, Log, TEXT("TEST atilma=%d"), TryDash(C) ? 1 : 0);
				if (Cross(2.0f)) bTestSlow = true;
				if (Cross(2.6f)) bTestSlow = false;
				if (Cross(4.8f)) UE_LOG(LogKulustur, Log, TEXT("TEST cakilma=%d"), TrySlam(C) ? 1 : 0);
				if (Cross(0.15f) || Cross(0.75f) || Cross(4.15f) || Cross(4.75f)) Sub->StopContinuousInputInjectionForAction(JumpIA);
			}
			if (JumpTestT < 7.f && FMath::FloorToInt(Prev * 10.f) != FMath::FloorToInt(JumpTestT * 10.f))
			{
				UE_LOG(LogKulustur, Log, TEXT("TEST zipla t=%.1f enerji=%.0f yavas=%d fov=%.0f hiz=%s z=%.0f dusuyor=%d taban=%s girdiYok=%d"), JumpTestT, Energy, bSlowMo ? 1 : 0, FpsCam ? FpsCam->FieldOfView : 0.f, *Move->Velocity.ToCompactString(), Loc.Z,
					Move->IsFalling() ? 1 : 0, Base ? *GetNameSafe(Base->GetOwner()) : TEXT("-"), C->GetController() && C->GetController()->IsMoveInputIgnored() ? 1 : 0);
			}
		}
		// Fırlatıcı testi: 3. saniyede en yakın fırlatıcının rampasına ışınla
		static int32 LaunchTestStep = 0;
		if (FParse::Param(FCommandLine::Get(), TEXT("KulusturTestLaunch")) && Elapsed > 3.f && LaunchTestStep == 0)
		{
			for (const AKulusturVehicle* V : Traffic->GetVehicles())
			{
				if (V && V->Type == EVehicleType::Firlatici && !V->IsCrashed())
				{
					LaunchTestStep = 1;
					const FVector P = V->GetActorTransform().TransformPositionNoScale(FVector(380.f, 0.f, 340.f));
					C->SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);
					Move->Velocity = FVector(V->Speed, 0.f, 0.f);
					UE_LOG(LogKulustur, Log, TEXT("TEST firlatici rampasina isinlandi: %s"), *P.ToCompactString());
					break;
				}
			}
		}
		if (LaunchTestStep >= 1 && LaunchTestStep < 150)
		{
			++LaunchTestStep;
			if (LaunchTestStep % 10 == 0)
			{
				UE_LOG(LogKulustur, Log, TEXT("TEST firlatma hiz=%s yerde=%d taban=%s"), *Move->Velocity.ToCompactString(), bOnGround ? 1 : 0, Base ? *GetNameSafe(Base->GetOwner()) : TEXT("-"));
			}
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("KulusturTestWin")) && Mode == EKulusturMode::Level && Elapsed > 3.f && Loc.X < Track->GetFinishX())
		{
			C->SetActorLocation(FVector(Track->GetFinishX() + 100.f, Loc.Y, Loc.Z + 200.f), false, nullptr, ETeleportType::TeleportPhysics);
			Move->Velocity = FVector::ZeroVector;
		}
	}
}

void AKulusturDirector::ApplyEndlessConfig(bool bInstant)
{
	const FKulusturLevelConfig C = KulusturMakeEndlessConfig(PlayerX);
	Config.BaseSpeed = C.BaseSpeed;
	if (ActiveChallenges != C.Challenges)
	{
		const uint32 Added = C.Challenges & ~ActiveChallenges;
		ActiveChallenges = C.Challenges;
		Env->SetChallenges(ActiveChallenges);
		Traffic->bLaneSwerve = (ActiveChallenges & KChallenge::LaneSwerve) != 0;
		if (Added && !bInstant)
		{
			Banner(TEXT("YENİ ZORLUK"), KulusturChallengeText(Added).ToString(), FLinearColor(1.f, 0.3f, 0.2f), 3.f);
		}
	}
	WindStrength = C.WindStrength;
}

void AKulusturDirector::TickChallenges(float Dt, ACharacter* C)
{
	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	const FVector Loc = C->GetActorLocation();

	// Ters kontrol: 8 sn normal, 1.5 sn uyarı, 5 sn ters
	if (ActiveChallenges & KChallenge::InvertControls)
	{
		InvertTimer += Dt;
		const float Cycle = 14.5f;
		const float T = FMath::Fmod(InvertTimer, Cycle);
		const bool bNowInv = T >= 9.5f;
		InvertWarn = (T >= 8.f && T < 9.5f) ? (9.5f - T) : 0.f;
		if (bNowInv != bInverted)
		{
			bInverted = bNowInv;
			Play2D(TEXT("Swish_Cue"), 0.9f, bInverted ? 0.8f : 1.2f);
			Banner(bInverted ? TEXT("KONTROLLER TERS!") : TEXT("KONTROLLER NORMAL"), bInverted ? TEXT("Sol sağ, sağ sol oldu") : FString(),
				bInverted ? FLinearColor(1.f, 0.2f, 0.6f) : FLinearColor(0.4f, 1.f, 0.5f), 1.5f);
		}
		if (InvertWarn > 0.f && FMath::Fmod(InvertWarn, 0.5f) < Dt) Play2D(TEXT("Beep_Cue"), 0.4f);
		if (bInverted)
		{
			// Girişi kameranın sağ eksenine göre aynala: sol/sağ yer değiştirir.
			const FVector In = C->Internal_ConsumeMovementInputVector();
			const FRotator CR = C->GetControlRotation();
			const FVector Right = FRotationMatrix(FRotator(0.f, CR.Yaw, 0.f)).GetUnitAxis(EAxis::Y);
			const FVector Out = In - 2.f * FVector::DotProduct(In, Right) * Right;
			C->Internal_AddMovementInput(Out, true);
		}
	}
	else
	{
		bInverted = false;
		InvertWarn = 0.f;
	}

	// Rüzgar: havadayken yana iter
	if ((ActiveChallenges & KChallenge::Wind) && Move)
	{
		WindTimer -= Dt;
		if (WindTimer <= 0.f)
		{
			WindTimer = FMath::FRandRange(5.f, 9.f);
			const float NewDir = FMath::FRand() < 0.5f ? -1.f : 1.f;
			if (NewDir != WindDir)
			{
				WindDir = NewDir;
				Banner(WindDir > 0.f ? TEXT("RÜZGAR SAĞA ESİYOR  >") : TEXT("<  RÜZGAR SOLA ESİYOR"), FString(), FLinearColor(0.7f, 0.9f, 1.f), 1.5f);
			}
		}
		if (Move->IsFalling())
		{
			// Y ekseni: +Y oyuncunun sağı (ileri +X bakarken)
			// Yana itiş sınırlı: rüzgar zorlasın ama oyuncuyu yoldan atmasın.
			if (Move->Velocity.Y * WindDir < 650.f)
			{
				Move->Velocity.Y += WindDir * WindStrength * 0.6f * Dt;
			}
		}
	}

	// Buz: tavanlar kaygan
	const bool bIce = (ActiveChallenges & KChallenge::Ice) != 0 && Move && Move->IsMovingOnGround();
	if (Move && bIce != bIceApplied)
	{
		bIceApplied = bIce;
		Move->GroundFriction = bIce ? 0.5f : DefaultGroundFriction;
		Move->BrakingDecelerationWalking = bIce ? 180.f : DefaultBraking;
	}

	// Gökten araç
	if (ActiveChallenges & KChallenge::FallingTrucks)
	{
		FallTimer -= Dt;
		if (FallTimer <= 0.f)
		{
			FallTimer = FMath::FRandRange(3.5f, 6.5f);
			const int32 Lane = FMath::RandRange(0, AKulusturTrack::NumLanes - 1);
			Traffic->SpawnFallingVehicle(Loc.X + FMath::FRandRange(2500.f, 6000.f), Lane);
			Banner(TEXT("GÖKTEN ARAÇ YAĞIYOR!"), FString(), FLinearColor(1.f, 0.4f, 0.2f), 1.2f);
			Play2D(TEXT("Uyari"), 0.5f);
		}
	}

	// Ani hızlanma
	if (ActiveChallenges & KChallenge::SpeedBurst)
	{
		BurstTimer -= Dt;
		if (BurstTimer <= 0.f)
		{
			BurstTimer = FMath::FRandRange(9.f, 14.f);
			Traffic->SpeedBurst(1.4f, 3.f);
			Banner(TEXT("TRAFİK HIZLANIYOR!"), FString(), FLinearColor(1.f, 0.8f, 0.1f), 1.5f);
			Play2D(TEXT("SpeedUp_Cue"), 0.8f, 0.8f);
		}
	}

	// Lazer uyarısı
	LaserDist = Track->NextLaserDistance(Loc.X);
	if (LaserDist > 0.f && LaserDist < 3500.f)
	{
		LaserBeep -= Dt;
		if (LaserBeep <= 0.f)
		{
			LaserBeep = FMath::Clamp(LaserDist / 3500.f, 0.15f, 1.f) * 0.6f;
			Play2D(TEXT("Beep_03_Cue"), 0.35f, 1.3f);
		}
	}
}

float AKulusturDirector::GetGapWallInfo(float& OutOpenY) const
{
	if (!Track) return -1.f;
	const FKulusturObstacle* O = Track->NextGapWall(PlayerX);
	if (!O) return -1.f;
	float Sum = 0.f;
	int32 N = 0;
	for (int32 L = 0; L < AKulusturTrack::NumLanes; ++L)
	{
		if (!(O->LaneMask & (1u << L))) { Sum += AKulusturTrack::LaneY(L); ++N; }
	}
	OutOpenY = N > 0 ? Sum / N : 0.f;
	return O->X - PlayerX;
}

void AKulusturDirector::Die(const TCHAR* Reason)
{
	if (State != EKulusturState::Playing) return;
	UE_LOG(LogKulustur, Log, TEXT("OLUM: %s (t=%.1f, x=%.0f)"), Reason, Elapsed, PlayerX);
	// Test: ölümsüz mod (ekran görüntüsü almak için)
	if (bTest && FParse::Param(FCommandLine::Get(), TEXT("KulusturGod")) && FCString::Strcmp(Reason, TEXT("dustu")) != 0)
	{
		return;
	}
	StateTime = 0.f;
	++Deaths;
	Play2D(TEXT("Thwack_05_Cue"), 1.f);
	Play2D(TEXT("Hit_2_Cue"), 0.8f, 0.7f);
	bSlowMo = false;
	if (Music) Music->SetPitchMultiplier(1.f);
	UGameplayStatics::SetGlobalTimeDilation(this, 0.25f);

	if (Mode == EKulusturMode::Endless)
	{
		State = EKulusturState::EndlessOver;
		UKulusturSave* Save = UKulusturSave::Get();
		const int32 Prev = Save->BestEndless();
		EndlessRank = Save->RecordEndless(GetMeters(), Coins);
		bNewRecord = GetMeters() > Prev;
		Save->TotalCoins += Coins;
		Save->Write();
		SetScreen(EKulusturScreen::EndlessOver);
	}
	else
	{
		State = EKulusturState::Dead;
	}
}

void AKulusturDirector::Win()
{
	if (State != EKulusturState::Playing) return;
	SetSlowMo(false);
	State = EKulusturState::Won;
	StateTime = 0.f;
	const int32 Total = Track ? Track->GetTotalCoins() : 0;
	ResultStars = 1 + (Elapsed <= Config.ParTime * 0.95f ? 1 : 0) + ((Total == 0 || Coins * 2 >= Total) ? 1 : 0);
	UKulusturSave* Save = UKulusturSave::Get();
	Save->TotalCoins += Coins;
	bNewRecord = Save->GetBestTime(World, Level) <= 0.f || Elapsed < Save->GetBestTime(World, Level);
	bNewWorld = Save->RecordLevel(World, Level, Elapsed, ResultStars);
	Save->Write();
	UE_LOG(LogKulustur, Log, TEXT("KAZANDI: %d-%d sure %.2f yildiz %d altin %d/%d"), World + 1, Level + 1, Elapsed, ResultStars, Coins, Total);
	Play2D(TEXT("Spacey_1Up_Power_Up_WAV"), 1.f);
	SpawnFx(GetWorld(), FxBurst, LastPlayerLoc, 3.f);
	if (Music) Music->SetVolumeMultiplier(0.2f);
}

float AKulusturDirector::GetProgress() const
{
	if (Mode != EKulusturMode::Level || !Track) return 0.f;
	return FMath::Clamp(PlayerX / Track->GetFinishX(), 0.f, 1.f);
}

int32 AKulusturDirector::GetTotalCoins() const
{
	return Track ? Track->GetTotalCoins() : 0;
}

float AKulusturDirector::GetFlash() const
{
	return Env ? Env->GetLightningFlash() : 0.f;
}

void AKulusturDirector::EndPlay(const EEndPlayReason::Type Reason)
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	Super::EndPlay(Reason);
}

#undef LOCTEXT_NAMESPACE

// ---------------------------------------------------------------------------------------------
// Birinci şahıs kamera ve paket yazıları

void AKulusturDirector::SetupFirstPerson(ACharacter* C)
{
	// Paketin üçüncü şahıs kameralarını kapat, göz hizasına kendi kameramızı koy.
	TArray<UCameraComponent*> Cams;
	C->GetComponents(Cams);
	for (UCameraComponent* Cam : Cams)
	{
		if (Cam && Cam != FpsCam)
		{
			Cam->SetActive(false);
			OtherCams.Add(Cam);
		}
	}
	if (!FpsCam)
	{
		FpsCam = NewObject<UCameraComponent>(C, TEXT("KulusturFPSKamera"));
		FpsCam->SetupAttachment(C->GetCapsuleComponent());
		FpsCam->SetRelativeLocation(FVector(10.f, 0.f, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.75f));
		FpsCam->bUsePawnControlRotation = true;
		FpsCam->SetFieldOfView(100.f);
		FpsCam->RegisterComponent();
	}
	FpsCam->SetActive(true);

	// Kendi gövdeni görme (gölgesi kalsın)
	if (USkeletalMeshComponent* M = C->GetMesh())
	{
		M->SetOwnerNoSee(true);
		M->bCastHiddenShadow = true;
	}
	// Karakter kameranın baktığı yöne dönsün
	C->bUseControllerRotationYaw = true;
	if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = false;
		TuneMovement(Move);
	}
	// Paketin "havada hedefe atılma"sı ileri hızı sıfırlıyor; tır kaçıyor, oyuncu düşüyor. Kapat.
	TArray<UActorComponent*> Comps;
	C->GetComponents(Comps);
	for (UActorComponent* Comp : Comps)
	{
		const FString N = Comp ? Comp->GetClass()->GetName() : FString();
		if (N.StartsWith(TEXT("Homing_Attack_Component")))
		{
			KulusturPackage::SetProp(Comp, TEXT("CanSearchTargets?"), TEXT("False"));
			KulusturPackage::SetProp(Comp, TEXT("TargetMaxDistance"), TEXT("0"));
		}
		else if (N.StartsWith(TEXT("Platformer_Component")))
		{
			// Düşüş ayarlarını paket değil biz belirleyelim
			KulusturPackage::SetProp(Comp, TEXT("AdjustFallingParameters?"), TEXT("False"));
		}
	}
}

void AKulusturDirector::TryAirJump(ACharacter* C)
{
	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	if (!Move || !Move->IsFalling() || AirJumps <= 0) return;
	--AirJumps;
	Move->Velocity.Z = FMath::Max(Move->Velocity.Z, 950.f);
	Play2D(TEXT("Mid_Air_Jump_Cue"), 0.7f);
	// Akrobasi: kamera öne takla atar
	FlipT = 0.f;
	++Tricks;
	TrickNames.AddUnique(TEXT("TAKLA"));
}

bool AKulusturDirector::TryDash(ACharacter* C)
{
	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	if (!Move || bDashUsed || Energy < DashCost || DashCooldown > 0.f) return false;
	if (!Move->IsFalling() && !Move->IsMovingOnGround()) return false; // rayda/halatta değil
	// Yön: tuşlarla gidilen yön, yoksa kameranın baktığı yön (yatay)
	FVector Dir = C->GetLastMovementInputVector().GetSafeNormal2D();
	if (Dir.IsNearlyZero())
	{
		const FRotator R = C->GetControlRotation();
		Dir = FRotator(0.f, R.Yaw, 0.f).Vector();
	}
	const bool bAir = Move->IsFalling();
	const float Up = bAir ? FMath::Max(0.f, 300.f - Move->Velocity.Z) : 450.f;
	C->LaunchCharacter(Dir * 1400.f + FVector(0.f, 0.f, Up), false, false);
	Energy -= DashCost;
	DashCooldown = 0.35f;
	if (bAir)
	{
		bDashUsed = true;
		++Tricks;
		TrickNames.AddUnique(TEXT("ATILMA"));
	}
	FovKick = 18.f;
	Play2D(TEXT("SpeedUp_Cue"), 0.8f, 1.2f);
	SpawnFx(GetWorld(), FxWave, C->GetActorLocation(), 1.2f);
	return true;
}

bool AKulusturDirector::TrySlam(ACharacter* C)
{
	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	if (!Move || !Move->IsFalling() || bSlamming) return false;
	bSlamming = true;
	Move->Velocity = FVector(Move->Velocity.X * 0.85f, Move->Velocity.Y * 0.85f, -2600.f);
	++Tricks;
	TrickNames.AddUnique(TEXT("ÇAKILMA"));
	Play2D(TEXT("Mid_Air_Jump_Cue"), 0.7f, 0.6f);
	return true;
}

void AKulusturDirector::SetSlowMo(bool bOn)
{
	if (bOn == bSlowMo) return;
	bSlowMo = bOn;
	UGameplayStatics::SetGlobalTimeDilation(this, bOn ? 0.3f : 1.f);
	Play2D(TEXT("SpeedUp_Cue"), 0.6f, bOn ? 0.5f : 0.8f);
	if (Music) Music->SetPitchMultiplier(bOn ? 0.8f : 1.f);
}

void AKulusturDirector::TickMoves(float Dt, ACharacter* C, bool bOnGround, const UPrimitiveComponent* Base)
{
	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	const APlayerController* PC = Cast<APlayerController>(C->GetController());
	const float Real = FMath::Min((float)FApp::GetDeltaTime(), 0.1f);
	auto Pressed = [PC](const FKey& K) { return PC && PC->WasInputKeyJustPressed(K); };
	auto Down = [PC](const FKey& K) { return PC && PC->IsInputKeyDown(K); };
	const bool bSpace = Pressed(EKeys::SpaceBar) || Pressed(EKeys::Gamepad_FaceButton_Bottom);
	const bool bFalling = Move && Move->IsFalling();
	DashCooldown -= Dt;

	if (bOnGround)
	{
		if (!bPrevGround) OnLanded(C, Base);
		AirJumps = 1;
		bDashUsed = false;
		bLeftByJump = false;
		CoyoteT = 0.15f;
		// İnmeden hemen önce basılan zıplama unutulmasın
		if (JumpBuffer > 0.f)
		{
			JumpBuffer = 0.f;
			bLeftByJump = true;
			C->LaunchCharacter(FVector(0.f, 0.f, Move->JumpZVelocity), false, true);
		}
	}
	else if (bFalling)
	{
		if (bPrevGround && Move->Velocity.Z > 250.f) bLeftByJump = true;
		CoyoteT -= Dt;
		JumpBuffer -= Dt;
		if (bSpace)
		{
			if (CoyoteT > 0.f && !bLeftByJump)
			{
				// Kenardan düştükten hemen sonra hâlâ normal zıplayabilirsin
				bLeftByJump = true;
				CoyoteT = 0.f;
				Move->Velocity.Z = Move->JumpZVelocity;
			}
			else if (AirJumps > 0)
			{
				TryAirJump(C);
			}
			else
			{
				JumpBuffer = 0.15f;
			}
		}
	}
	bPrevGround = bOnGround;

	// Yetenekler
	if (Pressed(EKeys::E) || Pressed(EKeys::Gamepad_FaceButton_Left)) TryDash(C);
	if (Pressed(EKeys::LeftControl) || Pressed(EKeys::C) || Pressed(EKeys::Gamepad_FaceButton_Right)) TrySlam(C);
	const bool bWantSlow = Down(EKeys::Q) || Down(EKeys::Gamepad_LeftTrigger) || bTestSlow;
	if (bWantSlow && !bSlowMo && Energy >= 15.f) SetSlowMo(true);
	else if (bSlowMo && (!bWantSlow || Energy <= 0.f)) SetSlowMo(false);

	// Enerji (gerçek zamanla): ağır çekim harcar, yoksa yavaşça dolar
	Energy = FMath::Clamp(Energy + (bSlowMo ? -38.f : 7.f) * Real, 0.f, 100.f);

	// Kamera: takla, atılmada FOV darbesi, hızlandıkça geniş açı
	if (FpsCam)
	{
		FovKick = FMath::FInterpTo(FovKick, 0.f, Real, 5.f);
		const float Target = 100.f + FMath::Clamp((SpeedKmh - 60.f) / 4.f, 0.f, 12.f) + FovKick;
		FpsCam->SetFieldOfView(FMath::FInterpTo(FpsCam->FieldOfView, Target, Real, 6.f));
		if (FlipT >= 0.f)
		{
			FlipT += Real;
			const float A = FMath::Clamp(FlipT / 0.5f, 0.f, 1.f);
			const float Ease = A * A * (3.f - 2.f * A);
			if (A >= 1.f || bOnGround)
			{
				FlipT = -1.f;
				FpsCam->bUsePawnControlRotation = true;
			}
			else
			{
				FpsCam->bUsePawnControlRotation = false;
				const FQuat Look = FRotator(C->GetControlRotation().Pitch, 0.f, 0.f).Quaternion();
				FpsCam->SetRelativeRotation(Look * FRotator(-360.f * Ease, 0.f, 0.f).Quaternion());
			}
		}
	}
}

void AKulusturDirector::OnLanded(ACharacter* C, const UPrimitiveComponent* Base)
{
	const bool bOnVehicle = Base && Base->GetOwner() && Base->GetOwner()->IsA<AKulusturVehicle>();
	if (bSlamming)
	{
		bSlamming = false;
		Play2D(TEXT("Thwack_05_Cue"), 0.6f, 1.3f);
		SpawnFx(GetWorld(), FxWave, C->GetActorLocation(), 1.6f);
		FovKick = -8.f;
	}
	if (bOnVehicle && Tricks > 0)
	{
		const float Gain = 10.f * Tricks;
		Energy = FMath::Min(100.f, Energy + Gain);
		if (Tricks >= 2)
		{
			Banner(FString::Printf(TEXT("AKROBASİ x%d!"), Tricks), FString::Join(TrickNames, TEXT(" + ")) + FString::Printf(TEXT("   +%d enerji"), FMath::RoundToInt(Gain)),
				FLinearColor(1.f, 0.45f, 0.9f), 1.4f);
			Play2D(TEXT("Grab_Coin_Cue"), 0.7f, 1.4f);
		}
	}
	Tricks = 0;
	TrickNames.Reset();
}

// ---------------------------------------------------------------------------------------------
// Otomatik pilot (-KulusturAuto): video görüntüsü için tırdan tıra atlar, yetenekleri kullanır.

static float AutoVehSpeed(const AKulusturVehicle* V) { return V->Speed * V->SpeedMul; }
static float AutoRoofCenterX(const AKulusturVehicle* V) { return V->GetActorLocation().X + (V->FrontOffset - V->BackOffset) * 0.5f; }

/** Verilen yükseklik farkına düşme süresi (zıplama hızı Vz, yerçekimi G). */
static float AutoFallTime(float Vz, float Dz, float G)
{
	const float Disc = Vz * Vz - 2.f * G * Dz;
	if (Disc < 0.f) return -1.f;
	return (Vz + FMath::Sqrt(Disc)) / G;
}

AKulusturVehicle* AKulusturDirector::PickAutoTarget(const FVector& Loc, const AKulusturVehicle* Cur) const
{
	AKulusturVehicle* Best = nullptr;
	float BestCost = TNumericLimits<float>::Max();
	const float CurRoof = Cur ? Cur->RoofLocation().Z : Loc.Z - 100.f;
	for (AKulusturVehicle* V : Traffic->GetVehicles())
	{
		if (!V || V == Cur || V->IsCrashed()) continue;
		const float Dx = V->BackX() - Loc.X;
		const float Dy = FMath::Abs(V->GetActorLocation().Y - Loc.Y);
		const float Dz = V->RoofLocation().Z - CurRoof;
		if (Dx < -200.f || Dx > 2600.f || Dy > 1000.f || Dz > 350.f) continue;
		const float Cost = FMath::Abs(Dx - 500.f) + Dy * 1.6f + FMath::Max(0.f, Dz) * 2.f;
		if (Cost < BestCost) { BestCost = Cost; Best = V; }
	}
	return Best;
}

void AKulusturDirector::AutoPilot(float Dt, ACharacter* C, bool bOnGround, const UPrimitiveComponent* Base)
{
	UCharacterMovementComponent* Move = C->GetCharacterMovement();
	APlayerController* PC = Cast<APlayerController>(C->GetController());
	if (!Move || !PC || !Traffic) return;
	const float Real = FMath::Min((float)FApp::GetDeltaTime(), 0.1f);
	const FVector Loc = C->GetActorLocation();
	const float G = FMath::Abs(Move->GetGravityZ());
	AutoClock += Real;
	if (AutoSlowT > 0.f) { AutoSlowT -= Real; if (AutoSlowT <= 0.f) bTestSlow = false; }

	// Ray/halat gibi paket hareketlerinde sadece ileri bak
	FRotator Want(-12.f, 0.f, 0.f);
	if (Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling)
	{
		AKulusturVehicle* Cur = Base ? Cast<AKulusturVehicle>(Base->GetOwner()) : nullptr;
		AKulusturVehicle* T = AutoTarget.Get();
		if (bOnGround || !T || T->IsCrashed())
		{
			T = PickAutoTarget(Loc, Cur);
			AutoTarget = T;
		}

		FVector Input(1.f, 0.f, 0.f);
		if (bOnGround)
		{
			const float CurSpeed = Cur ? AutoVehSpeed(Cur) : 0.f;
			if (T)
			{
				// Zıplarsam nereye düşerim? Hedefin üstüne denk geliyorsa zıpla.
				const float Dz = T->RoofLocation().Z - (Loc.Z - C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
				const float Tf = AutoFallTime(Move->JumpZVelocity, Dz + 20.f, G);
				const float Vx = CurSpeed + Move->Velocity.X;
				const float LandX = Loc.X + Vx * Tf;
				const float TX = T->GetActorLocation().X + AutoVehSpeed(T) * Tf;
				const float Rel = LandX - TX;
				const bool bFits = Tf > 0.f && Rel > -T->BackOffset + 250.f && Rel < T->FrontOffset - 350.f;
				const bool bEdge = Cur && Loc.X > Cur->FrontX() - 160.f;
				const float Dy = T->GetActorLocation().Y - Loc.Y;
				if (Cur && FMath::Abs(Loc.Y - Cur->GetActorLocation().Y) < 70.f) Input.Y = FMath::Clamp(Dy / 500.f, -0.7f, 0.7f);
				if (Rel > T->FrontOffset - 350.f) Input.X = -0.6f; // hedef geride kaldı: biraz bekle
				if (bFits || bEdge || !Cur)
				{
					C->Jump();
					// Ara sıra ağır çekimle atla (videoda yeteneği göstermek için)
					if (AutoClock - AutoLastSlow > 16.f && Energy > 60.f)
					{
						AutoLastSlow = AutoClock;
						bTestSlow = true;
						AutoSlowT = 1.4f;
					}
				}
			}
			else if (Cur && Loc.X > Cur->FrontX() - 350.f)
			{
				Input.X = -0.5f; // hedef yok: tırın üstünde bekle
			}
		}
		else if (T)
		{
			// Havada: hedefin tavanına doğru süzül
			const float Dz = T->RoofLocation().Z - (Loc.Z - C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			float Tf = AutoFallTime(Move->Velocity.Z, Dz, G);
			const bool bAbove = Tf > 0.f;
			if (!bAbove) Tf = 0.3f;
			const FVector P = Loc + FVector(Move->Velocity.X, Move->Velocity.Y, 0.f) * Tf;
			const FVector Tc(AutoRoofCenterX(T) + AutoVehSpeed(T) * Tf, T->GetActorLocation().Y, Loc.Z);
			const FVector Err = Tc - P;
			Input = FVector(Err.X, Err.Y, 0.f).GetClampedToMaxSize(400.f) / 400.f;
			const float Half = (T->FrontOffset + T->BackOffset) * 0.5f;
			const bool bShort = !bAbove || Err.X > Half - 150.f;
			const bool bLong = Err.X < -(Half - 200.f);
			if (bShort && Move->Velocity.Z < 150.f && AirJumps > 0)
			{
				TryAirJump(C);
			}
			else if (bShort && AirJumps == 0 && Move->Velocity.Z < -200.f && CanDash() && Err.Size2D() > 350.f)
			{
				C->AddMovementInput(FVector(Err.X, Err.Y, 0.f).GetSafeNormal());
				TryDash(C);
			}
			else if (bLong && FMath::Abs(Loc.X - AutoRoofCenterX(T) - AutoVehSpeed(T) * 0.f) < Half && Loc.Z > T->RoofLocation().Z + 300.f)
			{
				TrySlam(C);
			}
		}
		C->AddMovementInput(Input.GetSafeNormal2D(), FMath::Min(1.f, Input.Size2D()));
		if (T)
		{
			const FVector To = T->GetActorLocation() - Loc;
			Want.Yaw = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(To.Y, FMath::Max(To.X, 600.f))), -30.f, 30.f);
		}
		if (!bOnGround) Want.Pitch = -18.f;
	}
	PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(), Want, Real, 3.f));
}

void AKulusturDirector::TuneMovement(UCharacterMovementComponent* Move)
{
	// ClusterTruck hissi: yüksek ve uzun zıplama, havada rahat yön verme
	Move->JumpZVelocity = 1050.f;
	Move->GravityScale = 1.6f;
	Move->AirControl = 0.9f;
	Move->AirControlBoostMultiplier = 1.f;
	Move->BrakingDecelerationFalling = 0.f;
	Move->FallingLateralFriction = 0.f;
}

void AKulusturDirector::RemovePackageWidgets()
{
	// Paketin İngilizce yardım ekranı (UMG) yerine kendi Türkçe arayüzümüz var.
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), true);
	for (UUserWidget* W : Widgets)
	{
		if (W && W->IsInViewport())
		{
			W->RemoveFromParent();
		}
	}
}
