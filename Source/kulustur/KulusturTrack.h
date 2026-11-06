#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KulusturTypes.h"
#include "KulusturTrack.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

enum class EObstacleKind : uint8
{
	Ramp,
	Wall,
	GapWall,
	Finish
};

struct FKulusturObstacle
{
	float X = 0.f;
	float Length = 150.f;
	uint32 LaneMask = 0; // hangi şeritleri kapatıyor
	EObstacleKind Kind = EObstacleKind::Wall;
	int32 Chunk = 0;
};

struct FKulusturLaser
{
	float X = 0.f;
	float BaseZ = 520.f;
	float Amp = 0.f;
	float Speed = 1.f;
	float Phase = 0.f;
	float Z = 520.f;
	int32 Chunk = 0;
	TWeakObjectPtr<UStaticMeshComponent> Beam;
};

struct FKulusturRing
{
	FVector Loc;
	int32 Chunk = 0;
	float Cooldown = 0.f;
	TWeakObjectPtr<UStaticMeshComponent> Mesh;
};

struct FKulusturCoin
{
	FVector Loc;
	int32 Chunk = 0;
	int32 Instance = 0;
	bool bTaken = false;
};

/** Plan öğesi: bölüm başında (ya da sonsuz modda parça doğarken) neyin nereye konacağı. */
struct FTrackItem
{
	enum EKind : uint8 { Ramp, Wall, GapWall, Laser, Rail, Zipline, Pulley, Ring, CoinLine };
	EKind Kind = Wall;
	float X = 0.f;
	int32 Lane = 0;
	int32 LaneB = 0;
	float Param = 0.f;
};

/**
 * Pist: zemin, yol, dekor, engeller, paket parçaları (ray, halat, makara), halkalar ve altınlar.
 * Bölüm modunda tamamı baştan kurulur; sonsuz modda oyuncunun önünde parça parça üretilir.
 */
UCLASS()
class KULUSTUR_API AKulusturTrack : public AActor
{
	GENERATED_BODY()

public:
	AKulusturTrack();

	static constexpr int32 NumLanes = 5;
	static constexpr float LaneSpacing = 460.f;
	static constexpr float ChunkLength = 10000.f;
	static constexpr float FinishLength = 5000.f;
	static constexpr float FinishTop = 380.f;
	static constexpr float RoofZ = 430.f;
	static float LaneY(int32 Lane) { return (Lane - (NumLanes - 1) * 0.5f) * LaneSpacing; }
	static float RoadHalfWidth() { return NumLanes * LaneSpacing * 0.5f + 150.f; }

	static const FName TagDeadly;
	static const FName TagFinish;

	void Init(EKulusturMode InMode, const FKulusturLevelConfig& InConfig);

	/** Sonsuz mod: oyuncunun önünü üretir, arkasını siler. */
	void UpdateStreaming(float PlayerX);

	virtual void Tick(float DeltaSeconds) override;

	EKulusturBiome BiomeAt(float X) const;
	bool HasFinish() const { return Mode == EKulusturMode::Level; }
	float GetFinishX() const { return Config.RoadLength; }
	const TArray<FKulusturObstacle>& GetObstacles() const { return Obstacles; }

	/** Oyuncunun kapsülü bir lazere değiyor mu? */
	bool CheckLaserHit(const FVector& Center, float HalfHeight, float Radius) const;
	/** Önümüzdeki en yakın lazerin uzaklığı (uyarı için), yoksa -1. */
	float NextLaserDistance(float PlayerX) const;

	/** Yakındaki altınları toplar, toplanan sayıyı döner. */
	int32 CollectCoins(const FVector& P, float Radius);
	/** Halkadan geçtiyse true. */
	bool CheckRing(const FVector& P, float Radius);

	int32 GetTotalCoins() const { return TotalCoinsPlaced; }

	/** Yere zorunlu şerit uyarısı gibi şeyler için bilgi: oyuncunun önündeki ilk boşluklu duvar. */
	const FKulusturObstacle* NextGapWall(float PlayerX) const;

private:
	struct FChunk
	{
		TArray<TWeakObjectPtr<UActorComponent>> Comps;
		TArray<TWeakObjectPtr<AActor>> Actors;
	};

	void PlanLevel();
	void PlanEndlessChunk(int32 Index);
	void BuildChunk(int32 Index);
	void DestroyChunk(int32 Index);
	void BuildItem(const FTrackItem& Item, int32 Chunk);
	void BuildDecor(int32 Chunk, float X0, float X1, EKulusturBiome Biome, FRandomStream& R);
	void BuildFinish();

	UStaticMeshComponent* AddMesh(int32 Chunk, UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FLinearColor& Color, bool bCollide, FName Tag = NAME_None, const FRotator& Rot = FRotator::ZeroRotator, UMaterialInterface* Mat = nullptr);
	UStaticMeshComponent* AddBox(int32 Chunk, const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollide, FName Tag = NAME_None, const FRotator& Rot = FRotator::ZeroRotator);
	UInstancedStaticMeshComponent* AddISM(int32 Chunk, UStaticMesh* Mesh, const FLinearColor& Color, UMaterialInterface* Mat = nullptr);
	void AddText(int32 Chunk, const FVector& Loc, const FString& Text, float Size, const FColor& Color);
	UMaterialInterface* ColorMat(const FLinearColor& Color);
	void Track(int32 Chunk, UActorComponent* C);
	void Track(int32 Chunk, AActor* A);

	EKulusturMode Mode = EKulusturMode::Level;
	FKulusturLevelConfig Config;
	FRandomStream Rng;

	TMap<int32, FChunk> Chunks;
	TMap<int32, TArray<FTrackItem>> Plan;
	int32 MaxBuiltChunk = -10;

	TArray<FKulusturObstacle> Obstacles;
	TArray<FKulusturLaser> Lasers;
	TArray<FKulusturRing> Rings;
	TArray<FKulusturCoin> Coins;
	TMap<int32, TWeakObjectPtr<UInstancedStaticMeshComponent>> CoinISM;
	int32 TotalCoinsPlaced = 0;
	float Time = 0.f;

	UPROPERTY()
	TMap<uint32, TObjectPtr<UMaterialInterface>> MatCache;

	UPROPERTY() TObjectPtr<UStaticMesh> Cube;
	UPROPERTY() TObjectPtr<UStaticMesh> Cylinder;
	UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
	UPROPERTY() TObjectPtr<UStaticMesh> Cone;
	UPROPERTY() TObjectPtr<UStaticMesh> CoinMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> RingMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
	UPROPERTY() TArray<TObjectPtr<UStaticMesh>> Rocks;
	UPROPERTY() TObjectPtr<AActor> PulleyHelper;
};
