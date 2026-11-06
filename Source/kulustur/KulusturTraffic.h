#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KulusturTypes.h"
#include "KulusturTraffic.generated.h"

class AKulusturVehicle;
class AKulusturTrack;

/**
 * Yoldaki tüm araçlar: oyuncunun çevresinde araç doğurur/siler, şerit değiştirir,
 * arkadan çarpışmaları, engellere çarpmayı ve tanker patlamalarını yönetir.
 * Oyun yöneticisi her kare Update çağırır (karakterden önce).
 */
UCLASS()
class KULUSTUR_API AKulusturTraffic : public AActor
{
	GENERATED_BODY()

public:
	AKulusturTraffic();

	void Init(AKulusturTrack* InTrack, EKulusturMode InMode, const FKulusturLevelConfig& InConfig, float StartX);
	void Update(float DeltaSeconds, float PlayerX, const AActor* PlayerBaseActor);

	AKulusturVehicle* GetStartVehicle() const { return StartVehicle; }
	const TArray<TObjectPtr<AKulusturVehicle>>& GetVehicles() const { return Vehicles; }

	/** Gökten araç düşür (X, şerit). */
	AKulusturVehicle* SpawnFallingVehicle(float X, int32 Lane);

	/** Tüm trafik kısa süre hızlansın. */
	void SpeedBurst(float Mul, float Duration);

	/** Rastgele bir aracın ani şerit değiştirmesi (oyuncunun önünde). */
	void RandomSwerve(float PlayerX);

	/** Çarpma olunca (ses/efekt için). bExplosion: tanker patladı. */
	TFunction<void(const FVector& Loc, bool bExplosion)> OnCrash;

	bool bLaneSwerve = false;

private:
	AKulusturVehicle* SpawnVehicle(EVehicleType Type, int32 Lane, float CenterX, float Speed);
	EVehicleType PickType(float X);
	FLinearColor PickColor(float X);
	float LaneBaseSpeed(int32 Lane, float X) const;
	bool IsLaneFree(int32 Lane, float BackX, float FrontX, const AKulusturVehicle* Ignore) const;
	bool IsBlockedByObstacle(int32 Lane, float BackX, float FrontX) const;
	void Explode(AKulusturVehicle* V);
	static float VehicleLength(EVehicleType Type, float& OutBack);

	UPROPERTY()
	TArray<TObjectPtr<AKulusturVehicle>> Vehicles;

	UPROPERTY()
	TObjectPtr<AKulusturVehicle> StartVehicle;

	UPROPERTY()
	TObjectPtr<AKulusturTrack> Track;

	EKulusturMode Mode = EKulusturMode::Level;
	FKulusturLevelConfig Config;
	FRandomStream Rng;
	TArray<float> LaneOffset;
	float BrakeTimer = 0.f;
	float SwerveTimer = 0.f;
	float BurstMul = 1.f;
	float BurstTime = 0.f;
	float CrashSoundCooldown = 0.f;

public:
	static constexpr float AheadDist = 20000.f;
	static constexpr float BehindDist = 6000.f;
};
