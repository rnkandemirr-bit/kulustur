#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KulusturTypes.h"
#include "KulusturDirector.generated.h"

class AKulusturTrack;
class AKulusturTraffic;
class AKulusturEnvironment;
class AKulusturVehicle;
class ACharacter;
class ACameraActor;
class UAudioComponent;
class USoundBase;
class UPointLightComponent;

UENUM()
enum class EKulusturState : uint8
{
	Menu,
	Playing,
	Paused,
	Dead,
	Won,
	EndlessOver
};

UENUM()
enum class EKulusturScreen : uint8
{
	None,
	Main,
	Worlds,
	Levels,
	Leaderboard,
	Settings,
	NameEntry,
	Pause,
	Results,
	GameComplete,
	EndlessOver,
	Help
};

/** Ekranda kısa süre görünen uyarı/duyuru. */
struct FKulusturBanner
{
	FString Text;
	FString Sub;
	FLinearColor Color = FLinearColor::White;
	float Time = 0.f;
	float Duration = 2.f;
};

/**
 * Oyunun yöneticisi: menü, bölüm ve sonsuz mod akışı; ölüm/kazanma; zorluklar;
 * sesler ve müzik; kayıt. Pist, trafik ve ortamı kurar ve her kare günceller.
 */
UCLASS()
class KULUSTUR_API AKulusturDirector : public AActor
{
	GENERATED_BODY()

public:
	AKulusturDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// --- HUD için ---
	EKulusturMode GetMode() const { return Mode; }
	EKulusturState GetState() const { return State; }
	EKulusturScreen GetScreen() const { return Screen; }
	const FKulusturLevelConfig& GetConfig() const { return Config; }
	int32 GetWorldIndex() const { return World; }
	int32 GetLevelIndex() const { return Level; }
	int32 GetSelectedWorld() const { return SelWorld; }
	int32 GetSelectedLevel() const { return SelLevel; }
	float GetElapsed() const { return Elapsed; }
	float GetProgress() const;
	float GetSpeedKmh() const { return SpeedKmh; }
	int32 GetDeaths() const { return Deaths; }
	int32 GetCoins() const { return Coins; }
	int32 GetTotalCoins() const;
	int32 GetMeters() const { return FMath::Max(0, FMath::FloorToInt((PlayerX - StartX) / 100.f)); }
	int32 GetResultStars() const { return ResultStars; }
	bool WasNewWorldUnlocked() const { return bNewWorld; }
	bool WasNewRecord() const { return bNewRecord; }
	int32 GetEndlessRank() const { return EndlessRank; }
	EKulusturBiome GetCurrentBiome() const { return CurBiome; }
	uint32 GetActiveChallenges() const { return ActiveChallenges; }
	bool IsControlsInverted() const { return bInverted; }
	float GetInvertWarning() const { return InvertWarn; }
	float GetWindDir() const { return WindDir; }
	float GetWindStrength() const { return ActiveChallenges & KChallenge::Wind ? WindStrength : 0.f; }
	float GetLaserWarning() const { return LaserDist; }
	float GetGapWallInfo(float& OutOpenY) const;
	const TArray<FKulusturBanner>& GetBanners() const { return Banners; }
	const FString& GetNameBuffer() const { return NameBuffer; }
	float GetFlash() const;
	float GetStateTime() const { return StateTime; }
	float GetPlayerY() const { return PlayerY; }
	float GetEnergy() const { return Energy; }
	bool IsSlowMo() const { return bSlowMo; }
	bool CanDash() const { return !bDashUsed && Energy >= DashCost; }

	/** HUD düğmesine tıklandı. */
	void OnButton(FName Id);

	static FString FormatTime(float Seconds);

private:
	void ReadOptions();
	void SetupLevel();
	void SetupMenu();
	void PlacePlayer(ACharacter* C);

	void TickPlaying(float Dt, ACharacter* C);
	void TickMenuCamera(float Dt);
	void TickChallenges(float Dt, ACharacter* C);
	void TickAudio(float Dt, ACharacter* C);
	void HandleKeys();

	void Die(const TCHAR* Reason);
	void Win();
	void OpenMode(EKulusturMode NewMode, int32 W, int32 L, EKulusturScreen MenuScreen = EKulusturScreen::Main);
	void SetScreen(EKulusturScreen S);
	void SetPaused(bool bPause);
	void ApplyEndlessConfig(bool bInstant);
	void Banner(const FString& Text, const FString& Sub, const FLinearColor& Color, float Duration = 2.2f);

	// Ses
	USoundBase* Snd(const TCHAR* Name);
	void Play2D(const TCHAR* Name, float Volume = 1.f, float Pitch = 1.f);
	void PlayAt(const TCHAR* Name, const FVector& Loc, float Volume = 1.f, float Pitch = 1.f);
	UAudioComponent* Loop(const TCHAR* Name, float Volume);
	void StartMusic(EKulusturBiome Biome);

	ACharacter* GetPlayer() const;

	UPROPERTY() TObjectPtr<AKulusturTrack> Track;
	UPROPERTY() TObjectPtr<AKulusturTraffic> Traffic;
	UPROPERTY() TObjectPtr<AKulusturEnvironment> Env;
	UPROPERTY() TObjectPtr<ACameraActor> MenuCam;
	UPROPERTY() TObjectPtr<UPointLightComponent> PlayerLight;
	UPROPERTY() TObjectPtr<class UCameraComponent> FpsCam;
	TArray<TWeakObjectPtr<class UCameraComponent>> OtherCams;
	float WidgetSweep = 0.f;
	void SetupFirstPerson(ACharacter* C);
	void TuneMovement(class UCharacterMovementComponent* Move);
	void TryAirJump(ACharacter* C);
	bool TryDash(ACharacter* C);
	bool TrySlam(ACharacter* C);
	void SetSlowMo(bool bOn);
	void TickMoves(float Dt, ACharacter* C, bool bOnGround, const UPrimitiveComponent* Base);
	void OnLanded(ACharacter* C, const UPrimitiveComponent* Base);
	void AutoPilot(float Dt, ACharacter* C, bool bOnGround, const UPrimitiveComponent* Base);
	AKulusturVehicle* PickAutoTarget(const FVector& Loc, const AKulusturVehicle* Cur) const;
	bool bAuto = false;
	TWeakObjectPtr<AKulusturVehicle> AutoTarget;
	float AutoClock = 0.f;
	float AutoLastSlow = 0.f;
	float AutoSlowT = 0.f;

	// Hareket ve yetenekler
	static constexpr float DashCost = 30.f;
	int32 AirJumps = 1;
	bool bPrevGround = true;
	bool bLeftByJump = false;
	float CoyoteT = 0.f;
	float JumpBuffer = 0.f;
	bool bDashUsed = false;
	float DashCooldown = 0.f;
	bool bSlamming = false;
	bool bSlowMo = false;
	bool bTestSlow = false;
	float Energy = 100.f;
	float FlipT = -1.f;
	float FovKick = 0.f;
	int32 Tricks = 0;
	TArray<FString> TrickNames;
	void RemovePackageWidgets();

	UPROPERTY() TMap<FName, TObjectPtr<USoundBase>> SoundCache;
	UPROPERTY() TObjectPtr<UAudioComponent> Music;
	UPROPERTY() TObjectPtr<UAudioComponent> WindLoop;
	UPROPERTY() TObjectPtr<UAudioComponent> RainLoop;
	UPROPERTY() TObjectPtr<UAudioComponent> CityLoop;
	UPROPERTY() TObjectPtr<UAudioComponent> RumbleLoop;
	UPROPERTY() TObjectPtr<UAudioComponent> LaserLoop;

	EKulusturMode Mode = EKulusturMode::Menu;
	EKulusturState State = EKulusturState::Menu;
	EKulusturScreen Screen = EKulusturScreen::Main;
	EKulusturScreen PrevScreen = EKulusturScreen::Main;
	FKulusturLevelConfig Config;
	int32 World = 0;
	int32 Level = 0;
	int32 SelWorld = 0;
	int32 SelLevel = 0;
	int32 Deaths = 0;
	EKulusturBiome CurBiome = EKulusturBiome::Gunesli;
	EKulusturBiome MusicBiome = EKulusturBiome::Gunesli;
	bool bMusicEndless = false;

	// Oyun durumu
	float Elapsed = 0.f;
	float StateTime = 0.f;   // gerçek zaman
	float SpeedKmh = 0.f;
	float StartX = 0.f;
	float PlayerX = 0.f;
	float PlayerY = 0.f;
	FVector LastPlayerLoc = FVector::ZeroVector;
	bool bPlaced = false;
	bool bSafeSeen = false;
	int32 PlacedFrames = 0;
	int32 Coins = 0;
	int32 ResultStars = 0;
	bool bNewWorld = false;
	bool bNewRecord = false;
	int32 EndlessRank = -1;

	// Oyuncu durum geçişleri (ses için)
	bool bWasOnGround = true;
	float FootstepTimer = 0.f;
	float LastZVel = 0.f;
	const AActor* LastBaseActor = nullptr;

	// Zorluklar
	uint32 ActiveChallenges = 0;
	bool bInverted = false;
	float InvertTimer = 0.f;
	float InvertWarn = 0.f;
	float WindStrength = 0.f;
	float WindDir = 1.f;
	float WindTimer = 0.f;
	float FallTimer = 4.f;
	float BurstTimer = 8.f;
	float LaserDist = -1.f;
	float LaserBeep = 0.f;
	float DefaultGroundFriction = 8.f;
	float DefaultBraking = 2048.f;
	bool bIceApplied = false;
	float LaunchCooldown = 0.f;

	// Menü kamerası
	float MenuCamX = 0.f;
	float MenuCamT = 0.f;

	TArray<FKulusturBanner> Banners;
	FString NameBuffer;

	// Test
	bool bTest = false;
	float TestTimer = 0.f;
	int32 TestShots = 0;
	double TestStart = 0.0;
};
