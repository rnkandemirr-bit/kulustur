#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KulusturTypes.h"
#include "KulusturEnvironment.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class APostProcessVolume;
class UInstancedStaticMeshComponent;

/**
 * Dünyanın havası: güneş, gök ışığı, sis, renk ayarı, yağmur/kar/kum taneleri ve şimşek.
 * Sonsuz modda dünya değişince yumuşakça geçiş yapar.
 */
UCLASS()
class KULUSTUR_API AKulusturEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AKulusturEnvironment();

	void Init(EKulusturBiome Biome, uint32 Challenges);
	void SetBiome(EKulusturBiome Biome, bool bInstant = false);
	void SetChallenges(uint32 InChallenges) { Challenges = InChallenges; }

	/** Kamera konumu (hava taneleri kameranın çevresinde döner). */
	void UpdateEnv(float DeltaSeconds, const FVector& CameraLoc, const FVector& CameraVel);

	/** Şimşek çakınca çağrılır (gök gürültüsü sesi için gecikmeli). */
	TFunction<void()> OnThunder;

	float GetLightningFlash() const { return Flash; }

private:
	void Apply();

	UPROPERTY() TObjectPtr<ADirectionalLight> Sun;
	UPROPERTY() TObjectPtr<ASkyLight> Sky;
	UPROPERTY() TObjectPtr<AExponentialHeightFog> Fog;
	UPROPERTY() TObjectPtr<APostProcessVolume> PPV;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Particles;
	UPROPERTY() TObjectPtr<UMaterialInterface> ParticleMat;

	EKulusturBiome Target = EKulusturBiome::Gunesli;
	uint32 Challenges = 0;
	float Blend = 1.f; // 0 -> eski, 1 -> hedef
	FKulusturBiomeStyle From;
	FKulusturBiomeStyle Cur;

	struct FDrop { FVector Pos; float Speed; };
	TArray<FDrop> Drops;
	EKulusturWeather ActiveWeather = EKulusturWeather::None;
	float Flash = 0.f;
	float NextLightning = 5.f;
	float ThunderDelay = -1.f;
	float SkyRecapture = 0.f;
	FRandomStream Rng;
};
