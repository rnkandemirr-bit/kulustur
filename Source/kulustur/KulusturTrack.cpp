#include "KulusturTrack.h"

#include "KulusturPackage.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogKulusturTrack, Log, All);

const FName AKulusturTrack::TagDeadly(TEXT("Olumcul"));
const FName AKulusturTrack::TagFinish(TEXT("Bitis"));

namespace
{
	constexpr float RampLength = 900.f;
	constexpr float RampHeight = 260.f;
	uint32 LaneBit(int32 L) { return 1u << L; }
	uint32 AllLanes() { return (1u << AKulusturTrack::NumLanes) - 1u; }
	int32 ChunkOf(float X) { return FMath::FloorToInt(X / AKulusturTrack::ChunkLength); }

	const FLinearColor Red(0.85f, 0.04f, 0.03f);
	const FLinearColor White(0.95f, 0.95f, 0.95f);
	const FLinearColor Black(0.02f, 0.02f, 0.02f);
	const FLinearColor Steel(0.45f, 0.47f, 0.50f);
	const FLinearColor Gold(1.f, 0.75f, 0.05f);
	const FLinearColor Bark(0.25f, 0.15f, 0.07f);
}

AKulusturTrack::AKulusturTrack()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	GetRootComponent()->SetMobility(EComponentMobility::Static);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphF(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeF(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatF(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Cube = CubeF.Object;
	Cylinder = CylF.Object;
	Sphere = SphF.Object;
	Cone = ConeF.Object;
	BaseMaterial = MatF.Object;
}

// ---------------------------------------------------------------------------------------------
// Yardımcılar

UMaterialInterface* AKulusturTrack::ColorMat(const FLinearColor& Color)
{
	const uint32 Key = Color.ToFColor(false).ToPackedRGBA();
	if (TObjectPtr<UMaterialInterface>* Found = MatCache.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	MatCache.Add(Key, MID);
	return MID;
}

void AKulusturTrack::Track(int32 Chunk, UActorComponent* C)
{
	Chunks.FindOrAdd(Chunk).Comps.Add(C);
}

void AKulusturTrack::Track(int32 Chunk, AActor* A)
{
	if (A) Chunks.FindOrAdd(Chunk).Actors.Add(A);
}

UStaticMeshComponent* AKulusturTrack::AddMesh(int32 Chunk, UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FLinearColor& Color, bool bCollide, FName Tag, const FRotator& Rot, UMaterialInterface* Mat)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetupAttachment(GetRootComponent());
	C->SetWorldLocationAndRotation(Loc, Rot);
	C->SetWorldScale3D(Scale);
	C->SetGenerateOverlapEvents(false);
	if (bCollide)
	{
		C->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (!Tag.IsNone()) C->ComponentTags.Add(Tag);
	C->RegisterComponent();
	C->SetMaterial(0, Mat ? Mat : ColorMat(Color));
	Track(Chunk, C);
	return C;
}

UStaticMeshComponent* AKulusturTrack::AddBox(int32 Chunk, const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollide, FName Tag, const FRotator& Rot)
{
	return AddMesh(Chunk, Cube, Center, Size / 100.f, Color, bCollide, Tag, Rot);
}

UInstancedStaticMeshComponent* AKulusturTrack::AddISM(int32 Chunk, UStaticMesh* Mesh, const FLinearColor& Color, UMaterialInterface* Mat)
{
	UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetupAttachment(GetRootComponent());
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetGenerateOverlapEvents(false);
	C->RegisterComponent();
	if (Mat || Mesh == Cube || Mesh == Cylinder || Mesh == Sphere || Mesh == Cone)
	{
		C->SetMaterial(0, Mat ? Mat : ColorMat(Color));
	}
	Track(Chunk, C);
	return C;
}

void AKulusturTrack::AddText(int32 Chunk, const FVector& Loc, const FString& Text, float Size, const FColor& Color)
{
	UTextRenderComponent* T = NewObject<UTextRenderComponent>(this);
	T->SetupAttachment(GetRootComponent());
	T->SetWorldLocationAndRotation(Loc, FRotator(0.f, 180.f, 0.f)); // oyuncuya (-X) baksın
	T->SetText(FText::FromString(Text));
	T->SetHorizontalAlignment(EHTA_Center);
	T->SetVerticalAlignment(EVRTA_TextCenter);
	T->SetWorldSize(Size);
	T->SetTextRenderColor(Color);
	T->RegisterComponent();
	Track(Chunk, T);
}

// ---------------------------------------------------------------------------------------------

EKulusturBiome AKulusturTrack::BiomeAt(float X) const
{
	return Mode == EKulusturMode::Endless ? KulusturEndlessBiomeAt(X) : Config.Biome;
}

void AKulusturTrack::Init(EKulusturMode InMode, const FKulusturLevelConfig& InConfig)
{
	Mode = InMode;
	Config = InConfig;
	Rng.Initialize(Config.Seed);

	CoinMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/3DPlatformerGameBundle/Meshes/Coin_SM.Coin_SM"));
	RingMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/3DPlatformerGameBundle/Meshes/Boost_Ring_SM.Boost_Ring_SM"));
	for (const TCHAR* P : { TEXT("/Game/3DPlatformerGameBundle/Meshes/Rock_TallA_SM.Rock_TallA_SM"), TEXT("/Game/3DPlatformerGameBundle/Meshes/Rock_TallB_SM.Rock_TallB_SM"),
		TEXT("/Game/3DPlatformerGameBundle/Meshes/Rock_TallE_SM.Rock_TallE_SM"), TEXT("/Game/3DPlatformerGameBundle/Meshes/Rock_TallH_SM.Rock_TallH_SM") })
	{
		if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, P)) Rocks.Add(M);
	}

	// Paketin grind ray sistemi haritada bir Pulley_BP arıyor; yoksa raylar çalışmıyor.
	PulleyHelper = KulusturPackage::SpawnConfigured(GetWorld(), KulusturPackage::Pulley, FTransform(FVector(0.f, 0.f, -30000.f)), {});

	if (Mode == EKulusturMode::Level)
	{
		PlanLevel();
		const int32 Last = ChunkOf(Config.RoadLength + 30000.f); // bitişten sonra da yol devam eder
		for (int32 i = -1; i <= Last; ++i)
		{
			BuildChunk(i);
		}
		MaxBuiltChunk = Last;
		BuildFinish();
	}
	else
	{
		UpdateStreaming(0.f);
	}
	UE_LOG(LogKulusturTrack, Log, TEXT("Pist hazir: parca=%d engel=%d lazer=%d halka=%d altin=%d"), Chunks.Num(), Obstacles.Num(), Lasers.Num(), Rings.Num(), TotalCoinsPlaced);
}

void AKulusturTrack::UpdateStreaming(float PlayerX)
{
	if (Mode == EKulusturMode::Level) return;

	const int32 Want = ChunkOf(PlayerX + 70000.f);
	if (MaxBuiltChunk < -1)
	{
		// İlk kurulum: hemen birkaç parça
		for (int32 i = -1; i <= ChunkOf(PlayerX + 30000.f); ++i)
		{
			PlanEndlessChunk(i);
			BuildChunk(i);
			MaxBuiltChunk = i;
		}
	}
	// Kare başına en fazla bir parça üret (takılma olmasın)
	if (MaxBuiltChunk < Want)
	{
		const int32 i = MaxBuiltChunk + 1;
		PlanEndlessChunk(i);
		BuildChunk(i);
		MaxBuiltChunk = i;
	}

	// Arkada kalanları sil
	TArray<int32> Old;
	for (const TPair<int32, FChunk>& C : Chunks)
	{
		if (C.Key < ChunkOf(PlayerX - 20000.f)) Old.Add(C.Key);
	}
	for (int32 i : Old) DestroyChunk(i);
}

void AKulusturTrack::DestroyChunk(int32 Index)
{
	if (FChunk* C = Chunks.Find(Index))
	{
		for (TWeakObjectPtr<UActorComponent>& Comp : C->Comps)
		{
			if (Comp.IsValid()) Comp->DestroyComponent();
		}
		for (TWeakObjectPtr<AActor>& A : C->Actors)
		{
			if (A.IsValid()) A->Destroy();
		}
		Chunks.Remove(Index);
	}
	Plan.Remove(Index);
	CoinISM.Remove(Index);
	Obstacles.RemoveAll([Index](const FKulusturObstacle& O) { return O.Chunk == Index; });
	Lasers.RemoveAll([Index](const FKulusturLaser& L) { return L.Chunk == Index; });
	Rings.RemoveAll([Index](const FKulusturRing& R) { return R.Chunk == Index; });
	Coins.RemoveAll([Index](const FKulusturCoin& C) { return C.Chunk == Index; });
}

// ---------------------------------------------------------------------------------------------
// Planlama

void AKulusturTrack::PlanLevel()
{
	const float MinX = 4500.f;
	const float MaxX = Config.RoadLength - 3500.f;
	TArray<float> HardX;
	TArray<float> FeatX;

	auto Pick = [&](TArray<float>& Used, float Spacing, float Lo, float Hi) -> float
	{
		for (int32 Try = 0; Try < 60; ++Try)
		{
			const float X = Rng.FRandRange(Lo, Hi);
			bool bOk = true;
			for (float U : Used)
			{
				if (FMath::Abs(U - X) < Spacing) { bOk = false; break; }
			}
			if (bOk) { Used.Add(X); return X; }
		}
		return -1.f;
	};
	auto Add = [&](FTrackItem::EKind K, float X, int32 Lane, int32 LaneB = 0, float Param = 0.f)
	{
		if (X < 0.f) return;
		FTrackItem I;
		I.Kind = K; I.X = X; I.Lane = Lane; I.LaneB = LaneB; I.Param = Param;
		Plan.FindOrAdd(ChunkOf(X)).Add(I);
	};

	const int32 StartLane = NumLanes / 2;
	// Başlangıç şeridine mutlaka bir engel: oyuncu tırdan tıra atlamak zorunda kalsın.
	{
		const float X = Config.RoadLength * Rng.FRandRange(0.28f, 0.4f);
		HardX.Add(X);
		if (Config.NumWalls > 0) Add(FTrackItem::Wall, X, StartLane, StartLane);
		else Add(FTrackItem::Ramp, X, StartLane);
	}

	for (int32 i = 0; i < Config.NumRamps; ++i) Add(FTrackItem::Ramp, Pick(HardX, 2400.f, MinX, MaxX), Rng.RandRange(0, NumLanes - 1));
	for (int32 i = 0; i < Config.NumWalls; ++i)
	{
		const int32 Span = Rng.RandRange(1, 2);
		const int32 L = Rng.RandRange(0, NumLanes - Span);
		Add(FTrackItem::Wall, Pick(HardX, 2400.f, MinX, MaxX), L, L + Span - 1);
	}
	for (int32 i = 0; i < Config.NumGapWalls; ++i)
	{
		// Açık şerit sayısı: ilk bölümlerde 2, sonra 1
		const int32 Open = (Config.Level < 6) ? 2 : 1;
		const int32 L = Rng.RandRange(0, NumLanes - Open);
		Add(FTrackItem::GapWall, Pick(HardX, 4000.f, MinX + 4000.f, MaxX), L, L + Open - 1);
	}
	for (int32 i = 0; i < Config.NumLasers; ++i)
	{
		const float Amp = (Config.Level >= 5 && Rng.FRand() < 0.6f) ? 180.f : 0.f;
		Add(FTrackItem::Laser, Pick(HardX, 2400.f, MinX, MaxX), 0, 0, Amp);
	}

	for (int32 i = 0; i < Config.NumRails; ++i)
	{
		const int32 L = Rng.RandRange(0, NumLanes - 1);
		const int32 LB = FMath::Clamp(L + Rng.RandRange(-2, 2), 0, NumLanes - 1);
		Add(FTrackItem::Rail, Pick(FeatX, 8000.f, MinX, MaxX - 7000.f), L, LB, Rng.FRandRange(5000.f, 7500.f));
	}
	for (int32 i = 0; i < Config.NumZiplines; ++i) Add(FTrackItem::Zipline, Pick(FeatX, 7000.f, MinX, MaxX - 6000.f), Rng.RandRange(0, NumLanes - 1));
	for (int32 i = 0; i < Config.NumPulleys; ++i) Add(FTrackItem::Pulley, Pick(FeatX, 3000.f, MinX, MaxX - 4000.f), Rng.RandRange(0, NumLanes - 1));
	for (int32 i = 0; i < Config.NumRings; ++i) Add(FTrackItem::Ring, Rng.FRandRange(MinX, MaxX), Rng.RandRange(0, NumLanes - 1), 0, Rng.FRandRange(800.f, 1200.f));
	for (int32 i = 0; i < Config.NumCoinLines; ++i) Add(FTrackItem::CoinLine, Rng.FRandRange(2000.f, MaxX), Rng.RandRange(0, NumLanes - 1));
}

void AKulusturTrack::PlanEndlessChunk(int32 Index)
{
	if (Plan.Contains(Index)) return;
	TArray<FTrackItem>& Items = Plan.Add(Index);
	const float X0 = Index * ChunkLength;
	if (X0 < 8000.f) return; // başlangıç alanı boş

	FRandomStream R(Config.Seed + Index * 131);
	const FKulusturLevelConfig C = (Mode == EKulusturMode::Menu) ? Config : KulusturMakeEndlessConfig(X0);
	const int32 Diff = C.Level;

	auto Add = [&](FTrackItem::EKind K, float X, int32 Lane, int32 LaneB = 0, float Param = 0.f)
	{
		FTrackItem I;
		I.Kind = K; I.X = X; I.Lane = Lane; I.LaneB = LaneB; I.Param = Param;
		Items.Add(I);
	};

	// Biyom sınırının hemen yanına zor engel koyma
	const float InBiome = FMath::Fmod(X0, KulusturEndlessBiomeLength);
	const int32 NumHard = (Mode == EKulusturMode::Menu) ? 1 : 1 + (Diff >= 4) + (Diff >= 8);
	for (int32 h = 0; h < NumHard; ++h)
	{
		const float X = X0 + 1500.f + h * (7000.f / NumHard) + R.FRandRange(0.f, 1500.f);
		TArray<FTrackItem::EKind> Allowed = { FTrackItem::Ramp, FTrackItem::Ramp };
		if (Diff >= 1) Allowed.Add(FTrackItem::Wall);
		if ((C.Challenges & KChallenge::GapWalls) && InBiome > 10000.f && Mode != EKulusturMode::Menu) Allowed.Add(FTrackItem::GapWall);
		if ((C.Challenges & KChallenge::Lasers) && Mode != EKulusturMode::Menu) Allowed.Add(FTrackItem::Laser);
		const FTrackItem::EKind K = Allowed[R.RandRange(0, Allowed.Num() - 1)];
		if (K == FTrackItem::Wall)
		{
			const int32 Span = R.RandRange(1, 2);
			const int32 L = R.RandRange(0, NumLanes - Span);
			Add(K, X, L, L + Span - 1);
		}
		else if (K == FTrackItem::GapWall)
		{
			const int32 Open = Diff < 6 ? 2 : 1;
			const int32 L = R.RandRange(0, NumLanes - Open);
			Add(K, X, L, L + Open - 1);
		}
		else
		{
			Add(K, X, R.RandRange(0, NumLanes - 1), 0, (K == FTrackItem::Laser && R.FRand() < 0.5f) ? 180.f : 0.f);
		}
	}

	const float F = R.FRand();
	if (F < 0.35f)
	{
		const int32 L = R.RandRange(0, NumLanes - 1);
		Add(FTrackItem::Rail, X0 + R.FRandRange(500.f, 2500.f), L, FMath::Clamp(L + R.RandRange(-2, 2), 0, NumLanes - 1), R.FRandRange(5000.f, 7000.f));
	}
	else if (F < 0.55f)
	{
		Add(FTrackItem::Pulley, X0 + R.FRandRange(1000.f, 8000.f), R.RandRange(0, NumLanes - 1));
	}
	else if (F < 0.7f)
	{
		Add(FTrackItem::Zipline, X0 + R.FRandRange(500.f, 3000.f), R.RandRange(0, NumLanes - 1));
	}
	if (R.FRand() < 0.5f) Add(FTrackItem::Ring, X0 + R.FRandRange(0.f, ChunkLength), R.RandRange(0, NumLanes - 1), 0, R.FRandRange(800.f, 1200.f));
	for (int32 i = 0; i < 2; ++i) Add(FTrackItem::CoinLine, X0 + R.FRandRange(0.f, ChunkLength - 2000.f), R.RandRange(0, NumLanes - 1));
}

// ---------------------------------------------------------------------------------------------
// Kurulum

void AKulusturTrack::BuildChunk(int32 Index)
{
	const float X0 = Index * ChunkLength;
	const float X1 = X0 + ChunkLength;
	const float XM = (X0 + X1) * 0.5f;
	const EKulusturBiome Biome = BiomeAt(XM);
	const FKulusturBiomeStyle& S = KulusturGetBiomeStyle(Biome);
	const float RW = RoadHalfWidth();

	Chunks.FindOrAdd(Index);

	// Zemin: değersen ölürsün.
	AddBox(Index, FVector(XM, 0.f, -100.f), FVector(ChunkLength + 4.f, 90000.f, 200.f), S.Ground, true, TagDeadly);
	// Yol
	AddBox(Index, FVector(XM, 0.f, 3.f), FVector(ChunkLength + 2.f, RW * 2.f, 4.f), S.Road, false);
	// Kenar çizgileri
	for (float Side : { -1.f, 1.f })
	{
		AddBox(Index, FVector(XM, Side * (RW - 40.f), 6.f), FVector(ChunkLength, 24.f, 4.f), S.RoadLine, false);
	}
	// Kesikli şerit çizgileri
	{
		UInstancedStaticMeshComponent* Dashes = AddISM(Index, Cube, S.RoadLine);
		for (int32 L = 1; L < NumLanes; ++L)
		{
			const float Y = LaneY(L) - LaneSpacing * 0.5f;
			for (float X = X0 + 200.f; X < X1; X += 900.f)
			{
				Dashes->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, 6.f), FVector(4.5f, 0.18f, 0.04f)));
			}
		}
	}

	FRandomStream R(Config.Seed * 7 + Index * 977);
	BuildDecor(Index, X0, X1, Biome, R);

	if (TArray<FTrackItem>* Items = Plan.Find(Index))
	{
		for (const FTrackItem& I : *Items)
		{
			BuildItem(I, Index);
		}
	}
}

void AKulusturTrack::BuildDecor(int32 Chunk, float X0, float X1, EKulusturBiome Biome, FRandomStream& R)
{
	const FKulusturBiomeStyle& S = KulusturGetBiomeStyle(Biome);
	const float RW = RoadHalfWidth();
	auto SideY = [&](float Min, float Max) { return (R.FRand() < 0.5f ? -1.f : 1.f) * R.FRandRange(Min, Max); };

	// Zemin lekeleri
	{
		UInstancedStaticMeshComponent* Patches = AddISM(Chunk, Cube, S.GroundAlt);
		for (int32 i = 0; i < 18; ++i)
		{
			Patches->AddInstance(FTransform(FRotator(0.f, R.FRandRange(0.f, 90.f), 0.f), FVector(R.FRandRange(X0, X1), SideY(RW + 400.f, 20000.f), 1.f),
				FVector(R.FRandRange(6.f, 25.f), R.FRandRange(6.f, 25.f), 0.03f)));
		}
	}

	// Yol kenarı bariyer (şehir hariç)
	if (Biome != EKulusturBiome::Sehir)
	{
		UInstancedStaticMeshComponent* Posts = AddISM(Chunk, Cube, Steel);
		for (float Side : { -1.f, 1.f })
		{
			AddBox(Chunk, FVector((X0 + X1) * 0.5f, Side * (RW + 150.f), 70.f), FVector(ChunkLength, 12.f, 40.f), Steel, false);
			for (float X = X0; X < X1; X += 500.f)
			{
				Posts->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Side * (RW + 150.f), 40.f), FVector(0.12f, 0.12f, 0.8f)));
			}
		}
	}

	// Uzak dağlar / tepeler
	{
		FLinearColor MC = S.GroundAlt * 0.8f;
		UStaticMesh* MM = Cone;
		if (Biome == EKulusturBiome::Gunesli || Biome == EKulusturBiome::Yagmur) MM = Sphere;
		if (Biome == EKulusturBiome::Col) { MM = Cylinder; MC = FLinearColor(0.65f, 0.35f, 0.18f); }
		if (Biome == EKulusturBiome::Kar) MC = FLinearColor(0.85f, 0.88f, 0.95f);
		if (Biome != EKulusturBiome::Sehir)
		{
			UInstancedStaticMeshComponent* Mounts = AddISM(Chunk, MM, MC);
			for (int32 i = 0; i < 4; ++i)
			{
				const float Sc = R.FRandRange(150.f, 300.f);
				const float H = (MM == Cylinder) ? R.FRandRange(20.f, 50.f) : R.FRandRange(60.f, 120.f);
				Mounts->AddInstance(FTransform(FRotator::ZeroRotator, FVector(R.FRandRange(X0, X1), SideY(28000.f, 40000.f), (MM == Sphere) ? -H * 30.f : H * 50.f),
					FVector(Sc, Sc, H)));
			}
		}
	}

	switch (Biome)
	{
	case EKulusturBiome::Gunesli:
	case EKulusturBiome::Yagmur:
	{
		const bool bRain = Biome == EKulusturBiome::Yagmur;
		UInstancedStaticMeshComponent* Trunks = AddISM(Chunk, Cylinder, Bark);
		UInstancedStaticMeshComponent* Crowns = AddISM(Chunk, Sphere, bRain ? FLinearColor(0.05f, 0.18f, 0.06f) : FLinearColor(0.12f, 0.45f, 0.08f));
		for (int32 i = 0; i < 28; ++i)
		{
			const FVector P(R.FRandRange(X0, X1), SideY(RW + 700.f, 14000.f), 0.f);
			const float H = R.FRandRange(2.5f, 4.5f);
			Trunks->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0.f, 0.f, H * 50.f), FVector(0.5f, 0.5f, H)));
			const float C = R.FRandRange(3.f, 5.5f);
			Crowns->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0.f, 0.f, H * 100.f + C * 30.f), FVector(C, C, C * 1.1f)));
		}
		if (bRain)
		{
			// Elektrik direkleri ve su birikintileri
			UInstancedStaticMeshComponent* Poles = AddISM(Chunk, Cylinder, FLinearColor(0.2f, 0.15f, 0.1f));
			UInstancedStaticMeshComponent* Bars = AddISM(Chunk, Cube, FLinearColor(0.2f, 0.15f, 0.1f));
			for (float X = X0 + 500.f; X < X1; X += 2500.f)
			{
				for (float Side : { -1.f, 1.f })
				{
					Poles->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Side * (RW + 500.f), 600.f), FVector(0.3f, 0.3f, 12.f)));
					Bars->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Side * (RW + 500.f), 1150.f), FVector(0.2f, 3.f, 0.2f)));
				}
			}
			UInstancedStaticMeshComponent* Puddles = AddISM(Chunk, Cylinder, FLinearColor(0.08f, 0.1f, 0.14f));
			for (int32 i = 0; i < 10; ++i)
			{
				Puddles->AddInstance(FTransform(FRotator::ZeroRotator, FVector(R.FRandRange(X0, X1), SideY(RW + 300.f, 6000.f), 2.f), FVector(R.FRandRange(4.f, 10.f), R.FRandRange(4.f, 10.f), 0.02f)));
			}
		}
		break;
	}
	case EKulusturBiome::Col:
	{
		UInstancedStaticMeshComponent* Cacti = AddISM(Chunk, Cylinder, FLinearColor(0.15f, 0.40f, 0.12f));
		for (int32 i = 0; i < 22; ++i)
		{
			const FVector P(R.FRandRange(X0, X1), SideY(RW + 600.f, 15000.f), 0.f);
			const float H = R.FRandRange(2.f, 4.f);
			Cacti->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0.f, 0.f, H * 50.f), FVector(0.5f, 0.5f, H)));
			Cacti->AddInstance(FTransform(FRotator(0.f, 0.f, 0.f), P + FVector(0.f, 45.f, H * 60.f), FVector(0.3f, 0.3f, 1.2f)));
			Cacti->AddInstance(FTransform(FRotator(0.f, 0.f, 0.f), P + FVector(0.f, -45.f, H * 40.f), FVector(0.3f, 0.3f, 1.f)));
		}
		UInstancedStaticMeshComponent* Dunes = AddISM(Chunk, Sphere, S.GroundAlt);
		for (int32 i = 0; i < 6; ++i)
		{
			Dunes->AddInstance(FTransform(FRotator(0.f, R.FRandRange(0.f, 180.f), 0.f), FVector(R.FRandRange(X0, X1), SideY(RW + 3000.f, 25000.f), -150.f),
				FVector(R.FRandRange(30.f, 60.f), R.FRandRange(15.f, 30.f), R.FRandRange(4.f, 9.f))));
		}
		break;
	}
	case EKulusturBiome::Kar:
	{
		UInstancedStaticMeshComponent* Trunks = AddISM(Chunk, Cylinder, Bark);
		UInstancedStaticMeshComponent* Pines = AddISM(Chunk, Cone, FLinearColor(0.04f, 0.20f, 0.10f));
		UInstancedStaticMeshComponent* Snow = AddISM(Chunk, Cone, FLinearColor(0.92f, 0.94f, 0.98f));
		for (int32 i = 0; i < 36; ++i)
		{
			const FVector P(R.FRandRange(X0, X1), SideY(RW + 600.f, 15000.f), 0.f);
			const float H = R.FRandRange(4.f, 8.f);
			Trunks->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0.f, 0.f, 60.f), FVector(0.4f, 0.4f, 1.2f)));
			Pines->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0.f, 0.f, 100.f + H * 50.f), FVector(H * 0.5f, H * 0.5f, H)));
			Snow->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0.f, 0.f, 100.f + H * 75.f), FVector(H * 0.27f, H * 0.27f, H * 0.5f)));
		}
		UInstancedStaticMeshComponent* Mounds = AddISM(Chunk, Sphere, FLinearColor(0.95f, 0.96f, 1.f));
		for (int32 i = 0; i < 8; ++i)
		{
			Mounds->AddInstance(FTransform(FRotator::ZeroRotator, FVector(R.FRandRange(X0, X1), SideY(RW + 400.f, 18000.f), -60.f),
				FVector(R.FRandRange(10.f, 30.f), R.FRandRange(10.f, 25.f), R.FRandRange(2.f, 4.f))));
		}
		break;
	}
	case EKulusturBiome::Sehir:
	{
		// Kaldırım
		for (float Side : { -1.f, 1.f })
		{
			AddBox(Chunk, FVector((X0 + X1) * 0.5f, Side * (RW + 350.f), 15.f), FVector(ChunkLength, 700.f, 30.f), FLinearColor(0.45f, 0.45f, 0.48f), false);
		}
		const FLinearColor BColors[] = { FLinearColor(0.25f, 0.27f, 0.32f), FLinearColor(0.15f, 0.17f, 0.25f), FLinearColor(0.40f, 0.36f, 0.33f) };
		UInstancedStaticMeshComponent* B[3] = { AddISM(Chunk, Cube, BColors[0]), AddISM(Chunk, Cube, BColors[1]), AddISM(Chunk, Cube, BColors[2]) };
		UInstancedStaticMeshComponent* Windows = AddISM(Chunk, Cube, FLinearColor(1.f, 0.85f, 0.45f));
		UInstancedStaticMeshComponent* Neon = AddISM(Chunk, Cube, FLinearColor(1.f, 0.1f, 0.7f));
		UInstancedStaticMeshComponent* Neon2 = AddISM(Chunk, Cube, FLinearColor(0.1f, 0.9f, 1.f));
		for (float Side : { -1.f, 1.f })
		{
			for (float X = X0 + R.FRandRange(0.f, 400.f); X < X1; X += R.FRandRange(1300.f, 1800.f))
			{
				const float W = R.FRandRange(900.f, 1200.f);
				const float D = R.FRandRange(1000.f, 2500.f);
				const float H = R.FRandRange(1500.f, 9000.f);
				const float Y = Side * (RW + 900.f + D * 0.5f);
				B[R.RandRange(0, 2)]->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Y, H * 0.5f), FVector(W, D, H) / 100.f));
				const float FaceY = Side * (RW + 900.f) - Side * 6.f;
				for (float Z = 300.f; Z < H - 150.f; Z += 420.f)
				{
					if (R.FRand() < 0.85f)
					{
						Windows->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, FaceY, Z), FVector((W - 160.f) / 100.f, 0.1f, 1.4f)));
					}
				}
				if (R.FRand() < 0.4f)
				{
					((R.FRand() < 0.5f) ? Neon : Neon2)->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, FaceY - Side * 10.f, R.FRandRange(600.f, 1400.f)), FVector(W / 150.f, 0.15f, 1.6f)));
				}
			}
		}
		// Sokak lambaları
		UInstancedStaticMeshComponent* Poles = AddISM(Chunk, Cylinder, Steel);
		UInstancedStaticMeshComponent* Heads = AddISM(Chunk, Cube, FLinearColor(1.f, 0.9f, 0.6f));
		for (float X = X0; X < X1; X += 2000.f)
		{
			for (float Side : { -1.f, 1.f })
			{
				Poles->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Side * (RW + 250.f), 450.f), FVector(0.25f, 0.25f, 9.f)));
				Heads->AddInstance(FTransform(FRotator::ZeroRotator, FVector(X, Side * (RW + 120.f), 900.f), FVector(1.f, 0.6f, 0.2f)));
			}
		}
		break;
	}
	}

	// Kayalar (paketten)
	if (Rocks.Num() > 0 && Biome != EKulusturBiome::Sehir)
	{
		UInstancedStaticMeshComponent* RockISM = AddISM(Chunk, Rocks[R.RandRange(0, Rocks.Num() - 1)], FLinearColor::White);
		for (int32 i = 0; i < 6; ++i)
		{
			RockISM->AddInstance(FTransform(FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f), FVector(R.FRandRange(X0, X1), SideY(RW + 1500.f, 18000.f), -40.f),
				FVector(R.FRandRange(3.f, 8.f))));
		}
	}
}

void AKulusturTrack::BuildItem(const FTrackItem& I, int32 Chunk)
{
	const float RW = RoadHalfWidth();
	UWorld* W = GetWorld();

	switch (I.Kind)
	{
	case FTrackItem::Ramp:
	{
		const float Angle = FMath::RadiansToDegrees(FMath::Atan2(RampHeight, RampLength));
		const float Slope = FMath::Sqrt(RampLength * RampLength + RampHeight * RampHeight);
		const float Y = LaneY(I.Lane);
		AddBox(Chunk, FVector(I.X + RampLength * 0.5f, Y, RampHeight * 0.5f - 20.f), FVector(Slope, LaneSpacing - 40.f, 40.f), FLinearColor(0.95f, 0.45f, 0.05f), true, TagDeadly, FRotator(Angle, 0.f, 0.f));
		AddBox(Chunk, FVector(I.X + RampLength - 30.f, Y, RampHeight * 0.5f), FVector(60.f, LaneSpacing - 60.f, RampHeight), FLinearColor(0.3f, 0.3f, 0.32f), true, TagDeadly);
		FKulusturObstacle O;
		O.X = I.X; O.Length = RampLength; O.LaneMask = LaneBit(I.Lane); O.Kind = EObstacleKind::Ramp; O.Chunk = Chunk;
		Obstacles.Add(O);
		break;
	}
	case FTrackItem::Wall:
	{
		const int32 Span = I.LaneB - I.Lane + 1;
		const float CY = (LaneY(I.Lane) + LaneY(I.LaneB)) * 0.5f;
		AddBox(Chunk, FVector(I.X + 75.f, CY, 260.f), FVector(150.f, Span * LaneSpacing, 520.f), Red, true, TagDeadly);
		for (int32 s = 0; s < 3; ++s)
		{
			AddBox(Chunk, FVector(I.X - 2.f, CY, 80.f + s * 170.f), FVector(4.f, Span * LaneSpacing, 50.f), White, false);
		}
		FKulusturObstacle O;
		O.X = I.X; O.LaneMask = 0;
		for (int32 L = I.Lane; L <= I.LaneB; ++L) O.LaneMask |= LaneBit(L);
		O.Kind = EObstacleKind::Wall; O.Chunk = Chunk;
		Obstacles.Add(O);
		break;
	}
	case FTrackItem::GapWall:
	{
		FKulusturObstacle O;
		O.X = I.X; O.Kind = EObstacleKind::GapWall; O.Chunk = Chunk; O.LaneMask = 0;
		for (int32 L = 0; L < NumLanes; ++L)
		{
			const bool bOpen = L >= I.Lane && L <= I.LaneB;
			if (!bOpen)
			{
				O.LaneMask |= LaneBit(L);
				AddBox(Chunk, FVector(I.X + 75.f, LaneY(L), 550.f), FVector(150.f, LaneSpacing + 2.f, 1100.f), Red, true, TagDeadly);
				for (int32 s = 0; s < 5; ++s)
				{
					AddBox(Chunk, FVector(I.X - 2.f, LaneY(L), 120.f + s * 220.f), FVector(4.f, LaneSpacing, 60.f), White, false, NAME_None, FRotator(0.f, 0.f, 20.f));
				}
			}
			else
			{
				// Açık şerit: yeşil çerçeve
				AddBox(Chunk, FVector(I.X + 75.f, LaneY(L), 1060.f), FVector(150.f, LaneSpacing, 80.f), FLinearColor(0.1f, 0.9f, 0.3f), false);
			}
		}
		// Yolun iki kenarında da duvar: etrafından dolanılamasın
		for (float Side : { -1.f, 1.f })
		{
			AddBox(Chunk, FVector(I.X + 75.f, Side * (RW + 400.f), 550.f), FVector(150.f, 800.f, 1100.f), Red, true, TagDeadly);
		}
		Obstacles.Add(O);

		// 70 m önce uyarı tabelası: hangi yöne geçmeli
		const float GapY = (LaneY(I.Lane) + LaneY(I.LaneB)) * 0.5f;
		const float SignX = I.X - 7000.f;
		const int32 SignChunk = Chunk; // aynı parçaya bağla (silinme sırası için yeterli)
		for (float Side : { -1.f, 1.f })
		{
			AddBox(SignChunk, FVector(SignX, Side * (RW + 100.f), 800.f), FVector(60.f, 60.f, 1600.f), Steel, false);
		}
		AddBox(SignChunk, FVector(SignX, 0.f, 1500.f), FVector(40.f, RW * 2.f + 200.f, 360.f), FLinearColor(0.05f, 0.35f, 0.12f), false);
		const FString Arrow = (GapY < -10.f) ? TEXT("< SOLA GEÇ") : (GapY > 10.f ? TEXT("SAĞA GEÇ >") : TEXT("v ORTADAN GEÇ v"));
		AddText(SignChunk, FVector(SignX - 30.f, 0.f, 1500.f), Arrow, 220.f, FColor::White);
		// Yere açık şeride doğru oklar
		for (int32 k = 0; k < 4; ++k)
		{
			AddBox(Chunk, FVector(I.X - 1500.f - k * 900.f, GapY, 8.f), FVector(500.f, 120.f, 4.f), FLinearColor(0.1f, 0.9f, 0.3f), false, NAME_None, FRotator(0.f, 45.f, 0.f));
		}
		break;
	}
	case FTrackItem::Laser:
	{
		FKulusturLaser L;
		L.X = I.X; L.Amp = I.Param; L.BaseZ = RoofZ + 95.f + (L.Amp > 0.f ? 60.f : 0.f); L.Z = L.BaseZ;
		L.Speed = Rng.FRandRange(1.2f, 2.2f); L.Phase = Rng.FRandRange(0.f, 6.f); L.Chunk = Chunk;
		for (float Side : { -1.f, 1.f })
		{
			AddMesh(Chunk, Cylinder, FVector(I.X, Side * (RW + 60.f), 700.f), FVector(0.6f, 0.6f, 14.f), Black, false);
			AddMesh(Chunk, Cylinder, FVector(I.X, Side * (RW + 60.f), 1420.f), FVector(0.9f, 0.9f, 0.4f), Red, false);
		}
		UStaticMeshComponent* Beam = AddMesh(Chunk, Cylinder, FVector(I.X, 0.f, L.Z), FVector(0.14f, 0.14f, (RW * 2.f + 120.f) / 100.f), FLinearColor(1.f, 0.05f, 0.02f), false, NAME_None, FRotator(0.f, 0.f, 90.f));
		Beam->SetCastShadow(false);
		L.Beam = Beam;
		Lasers.Add(L);
		break;
	}
	case FTrackItem::Rail:
	{
		const float Len = I.Param;
		const float Y0 = LaneY(I.Lane);
		const float Y1 = LaneY(I.LaneB);
		TArray<FVector> Pts = {
			FVector(I.X, Y0, RoofZ + 110.f),
			FVector(I.X + Len * 0.3f, FMath::Lerp(Y0, Y1, 0.3f), 900.f),
			FVector(I.X + Len * 0.7f, FMath::Lerp(Y0, Y1, 0.7f), 900.f),
			FVector(I.X + Len, Y1, RoofZ + 140.f) };
		AActor* Rail = KulusturPackage::SpawnRail(W, Pts, false, {
			{ TEXT("CreateSupports?"), TEXT("False") },
			{ TEXT("CreateZiplineSupports?"), TEXT("False") } });
		Track(Chunk, Rail);
		// Rayın üstünde altınlar
		UInstancedStaticMeshComponent*& ISM = reinterpret_cast<UInstancedStaticMeshComponent*&>(CoinISM.FindOrAdd(Chunk));
		if (!ISM && CoinMesh) ISM = AddISM(Chunk, CoinMesh, FLinearColor::White);
		if (ISM)
		{
			for (int32 k = 1; k < 12; ++k)
			{
				const float t = k / 12.f;
				// Basit eğri yaklaşımı
				const FVector P = (t < 0.3f) ? FMath::Lerp(Pts[0], Pts[1], t / 0.3f) : (t < 0.7f ? FMath::Lerp(Pts[1], Pts[2], (t - 0.3f) / 0.4f) : FMath::Lerp(Pts[2], Pts[3], (t - 0.7f) / 0.3f));
				FKulusturCoin C; C.Loc = P + FVector(0.f, 0.f, 140.f); C.Chunk = Chunk;
				C.Instance = ISM->AddInstance(FTransform(FRotator(0.f, 90.f, 0.f), C.Loc, FVector(1.6f)));
				Coins.Add(C);
				++TotalCoinsPlaced;
			}
		}
		break;
	}
	case FTrackItem::Zipline:
	{
		const float Y = LaneY(I.Lane);
		const float Len = 6500.f;
		const FVector A(I.X, Y, 760.f);
		const FVector B(I.X + Len, Y, 560.f);
		AActor* Z = KulusturPackage::SpawnRail(W, { A, B }, true, {
			{ TEXT("CreateSupports?"), TEXT("False") },
			{ TEXT("CreateZiplineSupports?"), TEXT("False") } });
		Track(Chunk, Z);
		// İki uçta kapı direkleri (görsel)
		for (const FVector& E : { A, B })
		{
			for (float Side : { -1.f, 1.f })
			{
				AddBox(Chunk, FVector(E.X, Side * (RW + 100.f), (E.Z + 100.f) * 0.5f), FVector(60.f, 60.f, E.Z + 100.f), Steel, false);
			}
			AddBox(Chunk, FVector(E.X, 0.f, E.Z + 100.f), FVector(50.f, RW * 2.f + 260.f, 50.f), Steel, false);
		}
		break;
	}
	case FTrackItem::Pulley:
	{
		const float Y = LaneY(I.Lane);
		const float TopZ = 2600.f;
		const float HangZ = RoofZ + 230.f;
		AActor* P = KulusturPackage::SpawnConfigured(W, KulusturPackage::PulleyZipline, FTransform(FVector(I.X, Y, TopZ)), {
			{ TEXT("PulleyLocation"), KulusturPackage::Vec(FVector(0.f, 0.f, HangZ - TopZ)) },
			{ TEXT("CableLength"), FString::SanitizeFloat((TopZ - HangZ) / 100.f) },
			{ TEXT("PulleyStartLocation"), KulusturPackage::Vec(FVector(I.X, Y, HangZ)) },
			{ TEXT("AddBaseSupports?"), TEXT("False") },
			{ TEXT("ForwardLaunchVelocity"), TEXT("2300") },
			{ TEXT("UpwardsLaunchVelocity"), TEXT("900") } });
		Track(Chunk, P);
		// Üstte taşıyıcı köprü
		for (float Side : { -1.f, 1.f })
		{
			AddBox(Chunk, FVector(I.X, Side * (RW + 100.f), TopZ * 0.5f), FVector(80.f, 80.f, TopZ + 100.f), FLinearColor(1.f, 0.8f, 0.1f), false);
		}
		AddBox(Chunk, FVector(I.X, 0.f, TopZ + 80.f), FVector(80.f, RW * 2.f + 280.f, 80.f), FLinearColor(1.f, 0.8f, 0.1f), false);
		break;
	}
	case FTrackItem::Ring:
	{
		if (!RingMesh) break;
		FKulusturRing Rg;
		Rg.Loc = FVector(I.X, LaneY(I.Lane), I.Param);
		Rg.Chunk = Chunk;
		UStaticMeshComponent* M = AddMesh(Chunk, RingMesh, Rg.Loc, FVector(3.f), FLinearColor::White, false, NAME_None, FRotator::ZeroRotator, RingMesh->GetMaterial(0));
		Rg.Mesh = M;
		Rings.Add(Rg);
		break;
	}
	case FTrackItem::CoinLine:
	{
		if (!CoinMesh) break;
		UInstancedStaticMeshComponent*& ISM = reinterpret_cast<UInstancedStaticMeshComponent*&>(CoinISM.FindOrAdd(Chunk));
		if (!ISM) ISM = AddISM(Chunk, CoinMesh, FLinearColor::White);
		const float Y = LaneY(I.Lane);
		for (int32 k = 0; k < 8; ++k)
		{
			FKulusturCoin C;
			C.Loc = FVector(I.X + k * 260.f, Y, RoofZ + 170.f + FMath::Sin(k / 7.f * PI) * 150.f);
			C.Chunk = Chunk;
			C.Instance = ISM->AddInstance(FTransform(FRotator(0.f, 90.f, 0.f), C.Loc, FVector(1.6f)));
			Coins.Add(C);
			++TotalCoinsPlaced;
		}
		break;
	}
	}
}

void AKulusturTrack::BuildFinish()
{
	const float X = Config.RoadLength;
	const int32 Chunk = ChunkOf(X);
	const float RW = RoadHalfWidth();
	const float Width = RW * 2.f + 800.f;
	const FKulusturBiomeStyle& S = KulusturGetBiomeStyle(Config.Biome);

	// Bitiş bir kapı: araçlar altından geçip yola devam eder.
	// Oyuncu bu çizgiyi canlı geçince bölüm biter (yöneticide X kontrolü).
	const int32 Checks = 14;
	for (int32 i = 0; i < Checks; ++i)
	{
		for (int32 Row = 0; Row < 3; ++Row)
		{
			const float Y = -RW + (i + 0.5f) * (RW * 2.f / Checks);
			AddBox(Chunk, FVector(X + 100.f + Row * 200.f, Y, 7.f), FVector(200.f, RW * 2.f / Checks, 4.f), ((i + Row) % 2) ? Black : White, false);
		}
	}
	const float GateH = 1500.f;   // araçların ve tavandaki oyuncunun rahat geçeceği yükseklik
	for (float Side : { -1.f, 1.f })
	{
		AddBox(Chunk, FVector(X, Side * (Width * 0.5f - 100.f), GateH * 0.5f), FVector(200.f, 200.f, GateH), Gold, false);
		// Yan direklerde bayraklar
		AddBox(Chunk, FVector(X - 150.f, Side * (Width * 0.5f - 100.f), GateH - 150.f), FVector(300.f, 20.f, 200.f), S.UIColor, false);
	}
	AddBox(Chunk, FVector(X, 0.f, GateH + 120.f), FVector(200.f, Width, 240.f), S.UIColor, false);
	for (int32 i = 0; i < 16; ++i)
	{
		AddBox(Chunk, FVector(X - 102.f, -Width * 0.5f + (i + 0.5f) * (Width / 16.f), GateH + 20.f), FVector(4.f, Width / 16.f, 40.f), (i % 2) ? Black : White, false);
	}
	AddText(Chunk, FVector(X - 110.f, 0.f, GateH + 140.f), TEXT("BİTİŞ"), 240.f, FColor::White);

	// Uzaktan görünsün diye yolun iki yanında yüksek ışık direkleri
	for (int32 k = 1; k <= 3; ++k)
	{
		for (float Side : { -1.f, 1.f })
		{
			AddBox(Chunk, FVector(X - k * 1500.f, Side * (RW + 300.f), 900.f), FVector(80.f, 80.f, 1800.f), (k % 2) ? Gold : White, false);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Oyun sırasında

void AKulusturTrack::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	for (FKulusturLaser& L : Lasers)
	{
		if (L.Amp > 0.f)
		{
			L.Z = L.BaseZ + L.Amp * FMath::Sin(Time * L.Speed + L.Phase);
			if (L.Beam.IsValid())
			{
				L.Beam->SetWorldLocation(FVector(L.X, 0.f, L.Z));
			}
		}
	}
	for (FKulusturRing& R : Rings)
	{
		R.Cooldown = FMath::Max(0.f, R.Cooldown - DeltaSeconds);
		if (R.Mesh.IsValid())
		{
			R.Mesh->SetWorldRotation(FRotator(0.f, 0.f, Time * 90.f));
		}
	}
}

bool AKulusturTrack::CheckLaserHit(const FVector& Center, float HalfHeight, float Radius) const
{
	if (FMath::Abs(Center.Y) > RoadHalfWidth() + 300.f) return false;
	for (const FKulusturLaser& L : Lasers)
	{
		if (FMath::Abs(Center.X - L.X) < Radius + 12.f && L.Z > Center.Z - HalfHeight - 12.f && L.Z < Center.Z + HalfHeight + 12.f)
		{
			return true;
		}
	}
	return false;
}

float AKulusturTrack::NextLaserDistance(float PlayerX) const
{
	float Best = -1.f;
	for (const FKulusturLaser& L : Lasers)
	{
		const float D = L.X - PlayerX;
		if (D > 0.f && D < 8000.f && (Best < 0.f || D < Best)) Best = D;
	}
	return Best;
}

const FKulusturObstacle* AKulusturTrack::NextGapWall(float PlayerX) const
{
	const FKulusturObstacle* Best = nullptr;
	for (const FKulusturObstacle& O : Obstacles)
	{
		if (O.Kind == EObstacleKind::GapWall && O.X > PlayerX && O.X - PlayerX < 12000.f && (!Best || O.X < Best->X))
		{
			Best = &O;
		}
	}
	return Best;
}

int32 AKulusturTrack::CollectCoins(const FVector& P, float Radius)
{
	int32 N = 0;
	const float R2 = Radius * Radius;
	for (FKulusturCoin& C : Coins)
	{
		if (C.bTaken || FMath::Abs(C.Loc.X - P.X) > Radius) continue;
		if (FVector::DistSquared(C.Loc, P) < R2)
		{
			C.bTaken = true;
			++N;
			if (TWeakObjectPtr<UInstancedStaticMeshComponent>* ISM = CoinISM.Find(C.Chunk))
			{
				if (ISM->IsValid())
				{
					(*ISM)->UpdateInstanceTransform(C.Instance, FTransform(FRotator::ZeroRotator, C.Loc - FVector(0.f, 0.f, 100000.f), FVector(0.01f)), true, true, true);
				}
			}
		}
	}
	return N;
}

bool AKulusturTrack::CheckRing(const FVector& P, float Radius)
{
	for (FKulusturRing& R : Rings)
	{
		if (R.Cooldown <= 0.f && FVector::Dist(R.Loc, P) < Radius)
		{
			R.Cooldown = 1.5f;
			return true;
		}
	}
	return false;
}
