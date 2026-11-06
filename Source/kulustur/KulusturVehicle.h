#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KulusturTypes.h"
#include "KulusturVehicle.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;

/**
 * Yoldaki araç. Normalde kodla düz gider (karakter üstünde titremeden dursun diye),
 * çarpışınca fiziğe geçip devrilir / uçar. Tipine göre şekli ve davranışı değişir.
 */
UCLASS()
class KULUSTUR_API AKulusturVehicle : public AActor
{
	GENERATED_BODY()

public:
	AKulusturVehicle();

	void Setup(EVehicleType InType, const FLinearColor& Color, float InSpeed, int32 InLane);

	/** Trafik yöneticisi her kare çağırır. */
	void Drive(float DeltaSeconds);

	/** Fiziğe geçir ve verilen ek hız / dönüşle savur. */
	void Crash(const FVector& ExtraVelocity, const FVector& SpinDegPerSec);

	void StartLaneChange(int32 NewLane, float NewY);
	bool IsChangingLane() const { return bChangingLane; }

	bool IsCrashed() const { return bCrashed; }
	float GetCrashTime() const { return CrashTime; }
	float FrontX() const { return GetActorLocation().X + FrontOffset; }
	float BackX() const { return GetActorLocation().X - BackOffset; }
	float Length() const { return FrontOffset + BackOffset; }
	FVector RoofLocation() const { return GetActorLocation() + FVector(0.f, 0.f, RoofOffset); }

	/** Fırlatıcı tırın rampası dünya uzayında bu noktayı içeriyor mu? */
	bool IsOnLaunchPad(const FVector& WorldPoint) const;

	/** Aracın üstüne takılan paket aktörleri (yay, hedef). Araçla birlikte silinir. */
	void AddAttachment(AActor* A);
	void DestroyWithAttachments();

	static float CenterHeightFor(EVehicleType Type);

	EVehicleType Type = EVehicleType::Tir;
	float Speed = 2000.f;
	/** Ani hızlanma zorluğu için trafik yöneticisinin verdiği çarpan. */
	float SpeedMul = 1.f;
	int32 Lane = 0;
	int32 PrevLane = 0;
	float FrontOffset = 1020.f;
	float BackOffset = 700.f;
	float RoofOffset = 160.f;

	bool bExploded = false;
	float NextLaneChangeTime = 0.f;
	float LifeTime = 0.f;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Wheels;

private:
	UStaticMeshComponent* AddPart(UStaticMesh* Mesh, const FVector& Loc, const FVector& Size, const FLinearColor& Color, bool bCollide, const FRotator& Rot = FRotator::ZeroRotator);
	void AddWheel(const FVector& Loc, float Diameter = 110.f, float Width = 35.f);
	void Paint(UStaticMeshComponent* C, const FLinearColor& Color);

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	TArray<TWeakObjectPtr<AActor>> Attachments;

	FVector BodyScale = FVector::OneVector;
	bool bCrashed = false;
	float CrashTime = 0.f;

	bool bChangingLane = false;
	float TargetY = 0.f;

	// Hoplayan araç
	float BaseZ = 0.f;
	float HopClock = 0.f;
	float NextHop = 3.f;

	FBox LaunchPadBox = FBox(ForceInit);
};
