#include "KulusturEnvironment.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FLinearColor LerpC(const FLinearColor& A, const FLinearColor& B, float T) { return A + (B - A) * T; }
}

AKulusturEnvironment::AKulusturEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatF(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Particles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Weather"));
	Particles->SetupAttachment(GetRootComponent());
	Particles->SetStaticMesh(CubeF.Object);
	Particles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Particles->SetCastShadow(false);
	Particles->SetMobility(EComponentMobility::Movable);
	ParticleMat = MatF.Object;
	Rng.Initialize(1234);
}

void AKulusturEnvironment::Init(EKulusturBiome Biome, uint32 InChallenges)
{
	Challenges = InChallenges;
	UWorld* W = GetWorld();
	for (TActorIterator<ADirectionalLight> It(W); It; ++It) { Sun = *It; break; }
	for (TActorIterator<ASkyLight> It(W); It; ++It) { Sky = *It; break; }
	for (TActorIterator<AExponentialHeightFog> It(W); It; ++It) { Fog = *It; break; }

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PPV = W->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform::Identity, P);
	if (PPV)
	{
		PPV->bUnbound = true;
		PPV->Priority = 10.f;
		FPostProcessSettings& S = PPV->Settings;
		S.bOverride_MotionBlurAmount = true;
		S.MotionBlurAmount = 0.f;
		S.bOverride_AutoExposureSpeedUp = true;
		S.AutoExposureSpeedUp = 6.f;
		S.bOverride_AutoExposureSpeedDown = true;
		S.AutoExposureSpeedDown = 3.f;
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = 0.35f;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 0.5f;
	}

	SetBiome(Biome, true);
}

void AKulusturEnvironment::SetBiome(EKulusturBiome Biome, bool bInstant)
{
	if (!bInstant && Biome == Target && Blend >= 1.f) return;
	From = bInstant ? KulusturGetBiomeStyle(Biome) : Cur;
	Target = Biome;
	Blend = bInstant ? 1.f : 0.f;
	Cur = KulusturGetBiomeStyle(Biome);
	if (bInstant)
	{
		From = Cur;
	}
	Apply();
	if (Sky && Sky->GetLightComponent()) Sky->GetLightComponent()->RecaptureSky();
}

void AKulusturEnvironment::Apply()
{
	const FKulusturBiomeStyle& T = KulusturGetBiomeStyle(Target);
	const float B = FMath::SmoothStep(0.f, 1.f, Blend);

	Cur.SunPitch = FMath::Lerp(From.SunPitch, T.SunPitch, B);
	Cur.SunYaw = FMath::Lerp(From.SunYaw, T.SunYaw, B);
	Cur.SunIntensity = FMath::Lerp(From.SunIntensity, T.SunIntensity, B);
	Cur.SunColor = LerpC(From.SunColor, T.SunColor, B);
	Cur.SkyIntensity = FMath::Lerp(From.SkyIntensity, T.SkyIntensity, B);
	Cur.FogDensity = FMath::Lerp(From.FogDensity, T.FogDensity, B);
	Cur.FogFalloff = FMath::Lerp(From.FogFalloff, T.FogFalloff, B);
	Cur.FogColor = LerpC(From.FogColor, T.FogColor, B);
	Cur.Saturation = FMath::Lerp(From.Saturation, T.Saturation, B);
	Cur.Tint = LerpC(From.Tint, T.Tint, B);
	Cur.ExposureBias = FMath::Lerp(From.ExposureBias, T.ExposureBias, B);

	const bool bDark = (Challenges & KChallenge::Darkness) != 0;
	const bool bFog = (Challenges & KChallenge::Fog) != 0;

	if (Sun)
	{
		Sun->SetActorRotation(FRotator(Cur.SunPitch, Cur.SunYaw, 0.f));
		if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			L->SetIntensity(Cur.SunIntensity * (bDark ? 0.12f : 1.f) + Flash * 25.f);
			L->SetLightColor(Flash > 0.05f ? FLinearColor(0.85f, 0.9f, 1.f) : Cur.SunColor);
		}
	}
	if (Sky && Sky->GetLightComponent())
	{
		Sky->GetLightComponent()->SetIntensity(Cur.SkyIntensity * (bDark ? 0.3f : 1.f) + Flash * 2.f);
	}
	if (Fog)
	{
		if (UExponentialHeightFogComponent* F = Fog->GetComponent())
		{
			F->SetFogDensity(Cur.FogDensity * (bFog ? 4.f : 1.f));
			F->SetFogHeightFalloff(Cur.FogFalloff);
			F->SetFogInscatteringColor(bDark ? Cur.FogColor * 0.2f : Cur.FogColor);
		}
	}
	if (PPV)
	{
		FPostProcessSettings& S = PPV->Settings;
		S.bOverride_ColorSaturation = true;
		S.ColorSaturation = FVector4(Cur.Saturation, Cur.Saturation, Cur.Saturation, 1.f);
		S.bOverride_SceneColorTint = true;
		S.SceneColorTint = Cur.Tint;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = Cur.ExposureBias + (bDark ? -1.0f : 0.f);
		S.VignetteIntensity = bDark ? 0.8f : 0.35f;
	}
}

void AKulusturEnvironment::UpdateEnv(float DeltaSeconds, const FVector& CameraLoc, const FVector& CameraVel)
{
	if (Blend < 1.f)
	{
		Blend = FMath::Min(1.f, Blend + DeltaSeconds / 6.f);
		SkyRecapture -= DeltaSeconds;
		if (SkyRecapture <= 0.f || Blend >= 1.f)
		{
			SkyRecapture = 1.5f;
			if (Sky && Sky->GetLightComponent()) Sky->GetLightComponent()->RecaptureSky();
		}
	}

	// Şimşek
	const FKulusturBiomeStyle& T = KulusturGetBiomeStyle(Target);
	if (T.bLightning && Blend > 0.5f)
	{
		NextLightning -= DeltaSeconds;
		if (NextLightning <= 0.f)
		{
			NextLightning = Rng.FRandRange(5.f, 11.f);
			Flash = 1.f;
			ThunderDelay = Rng.FRandRange(0.3f, 1.2f);
		}
	}
	if (Flash > 0.f)
	{
		Flash = FMath::Max(0.f, Flash - DeltaSeconds * 3.5f);
		// titreme
		if (Flash > 0.3f && Rng.FRand() < 0.25f) Flash *= 0.5f;
	}
	if (ThunderDelay > 0.f)
	{
		ThunderDelay -= DeltaSeconds;
		if (ThunderDelay <= 0.f && OnThunder) OnThunder();
	}
	Apply();

	// Hava taneleri
	const EKulusturWeather Want = (Blend > 0.5f) ? T.Weather : From.Weather;
	const float Amount = (Blend > 0.5f) ? T.WeatherAmount : From.WeatherAmount;
	if (Want != ActiveWeather)
	{
		ActiveWeather = Want;
		Particles->ClearInstances();
		Drops.Reset();
		if (Want != EKulusturWeather::None)
		{
			FLinearColor C(0.75f, 0.8f, 0.9f);
			int32 N = 900;
			if (Want == EKulusturWeather::Snow) { C = FLinearColor(1.f, 1.f, 1.f); N = 800; }
			if (Want == EKulusturWeather::Sand) { C = FLinearColor(0.85f, 0.65f, 0.4f); N = 600; }
			N = FMath::RoundToInt(N * FMath::Max(0.3f, Amount));
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ParticleMat, this);
			MID->SetVectorParameterValue(TEXT("Color"), C);
			Particles->SetMaterial(0, MID);
			TArray<FTransform> Init;
			for (int32 i = 0; i < N; ++i)
			{
				FDrop D;
				D.Pos = CameraLoc + FVector(Rng.FRandRange(-1500.f, 3500.f), Rng.FRandRange(-2500.f, 2500.f), Rng.FRandRange(-1200.f, 1800.f));
				D.Speed = Rng.FRandRange(0.8f, 1.2f);
				Drops.Add(D);
				Init.Add(FTransform(D.Pos));
			}
			Particles->AddInstances(Init, false, true);
		}
	}
	if (Drops.Num() == 0) return;

	FVector Vel(0.f, 0.f, -2200.f);
	FVector Scale(0.015f, 0.015f, 0.7f);
	FRotator Rot = FRotator::ZeroRotator;
	if (ActiveWeather == EKulusturWeather::Snow) { Vel = FVector(-150.f, 120.f, -260.f); Scale = FVector(0.07f); }
	if (ActiveWeather == EKulusturWeather::Sand) { Vel = FVector(-900.f, 1400.f, -80.f); Scale = FVector(0.05f, 0.05f, 0.03f); }
	if (ActiveWeather == EKulusturWeather::Rain)
	{
		// Hızla ileri giderken yağmur çizgileri eğik görünsün
		const FVector Rel = Vel - FVector(CameraVel.X, CameraVel.Y, 0.f) * 0.3f;
		Rot = FRotationMatrix::MakeFromZ(-Rel).Rotator();
	}

	TArray<FTransform> Xf;
	Xf.SetNumUninitialized(Drops.Num());
	const float Time = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < Drops.Num(); ++i)
	{
		FDrop& D = Drops[i];
		FVector V = Vel * D.Speed;
		if (ActiveWeather == EKulusturWeather::Snow)
		{
			V.Y += FMath::Sin(Time * 1.3f + i) * 80.f;
		}
		D.Pos += V * DeltaSeconds;
		const FVector L = D.Pos - CameraLoc;
		if (L.Z < -1200.f) D.Pos.Z += 3000.f;
		if (L.Z > 1800.f) D.Pos.Z -= 3000.f;
		if (L.X < -1500.f) D.Pos.X += 5000.f;
		if (L.X > 3500.f) D.Pos.X -= 5000.f;
		if (L.Y < -2500.f) D.Pos.Y += 5000.f;
		if (L.Y > 2500.f) D.Pos.Y -= 5000.f;
		Xf[i] = FTransform(Rot, D.Pos, Scale);
	}
	Particles->BatchUpdateInstancesTransforms(0, Xf, true, true, false);
}
