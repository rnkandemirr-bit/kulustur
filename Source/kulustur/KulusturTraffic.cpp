#include "KulusturTraffic.h"

#include "KulusturPackage.h"
#include "KulusturTrack.h"
#include "KulusturVehicle.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogKulusturTraffic, Log, All);

AKulusturTraffic::AKulusturTraffic()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

float AKulusturTraffic::VehicleLength(EVehicleType Type, float& OutBack)
{
	switch (Type)
	{
	case EVehicleType::DevTir: OutBack = 1300.f; return 1300.f + 1620.f;
	case EVehicleType::Otobus: OutBack = 950.f; return 1900.f;
	case EVehicleType::Kamyonet: OutBack = 350.f; return 350.f + 625.f;
	case EVehicleType::Tanker: OutBack = 650.f; return 650.f + 970.f;
	default: OutBack = 700.f; return 700.f + 1020.f;
	}
}

EVehicleType AKulusturTraffic::PickType(float X)
{
	const FKulusturBiomeStyle& S = KulusturGetBiomeStyle(Track ? Track->BiomeAt(X) : Config.Biome);
	float Total = 0.f;
	for (const TPair<EVehicleType, float>& P : S.VehicleMix) Total += P.Value;
	if (Total <= 0.f) return EVehicleType::Tir;
	float R = Rng.FRandRange(0.f, Total);
	for (const TPair<EVehicleType, float>& P : S.VehicleMix)
	{
		R -= P.Value;
		if (R <= 0.f) return P.Key;
	}
	return S.VehicleMix.Last().Key;
}

FLinearColor AKulusturTraffic::PickColor(float X)
{
	const FKulusturBiomeStyle& S = KulusturGetBiomeStyle(Track ? Track->BiomeAt(X) : Config.Biome);
	if (S.TruckColors.Num() == 0) return FLinearColor(0.8f, 0.1f, 0.1f);
	return S.TruckColors[Rng.RandRange(0, S.TruckColors.Num() - 1)];
}

float AKulusturTraffic::LaneBaseSpeed(int32 Lane, float X) const
{
	float Base = Config.BaseSpeed;
	if (Mode == EKulusturMode::Endless)
	{
		Base = KulusturMakeEndlessConfig(X).BaseSpeed;
	}
	return Base + (LaneOffset.IsValidIndex(Lane) ? LaneOffset[Lane] : 0.f);
}

bool AKulusturTraffic::IsLaneFree(int32 Lane, float BackX, float FrontX, const AKulusturVehicle* Ignore) const
{
	for (const AKulusturVehicle* V : Vehicles)
	{
		if (!V || V == Ignore || V->IsCrashed()) continue;
		if (V->Lane != Lane && !(V->IsChangingLane() && V->PrevLane == Lane)) continue;
		if (V->FrontX() > BackX && V->BackX() < FrontX) return false;
	}
	return true;
}

bool AKulusturTraffic::IsBlockedByObstacle(int32 Lane, float BackX, float FrontX) const
{
	if (!Track) return false;
	for (const FKulusturObstacle& O : Track->GetObstacles())
	{
		if (!(O.LaneMask & (1u << Lane))) continue;
		if (FrontX > O.X - 200.f && BackX < O.X + O.Length + 200.f) return true;
		// Bitişten sonrası tamamen kapalı
		if (O.Kind == EObstacleKind::Finish && FrontX > O.X - 600.f) return true;
	}
	return false;
}

AKulusturVehicle* AKulusturTraffic::SpawnVehicle(EVehicleType Type, int32 Lane, float CenterX, float Speed)
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Loc(CenterX, AKulusturTrack::LaneY(Lane), AKulusturVehicle::CenterHeightFor(Type));
	AKulusturVehicle* V = GetWorld()->SpawnActor<AKulusturVehicle>(AKulusturVehicle::StaticClass(), Loc, FRotator::ZeroRotator, P);
	if (!V) return nullptr;
	V->Setup(Type, PickColor(CenterX), Speed, Lane);
	V->NextLaneChangeTime = Rng.FRandRange(1.5f, 4.f);
	Vehicles.Add(V);

	// Homing Attack ile kilitlenilebilsin diye paketin gizli hedefi tavana
	// (Kapalı: havada hedefe kilitlenmek ileri hızı öldürüyordu, oynanışı zorlaştırıyordu.)
	if (false)
	{
		if (AActor* T = KulusturPackage::SpawnConfigured(GetWorld(), KulusturPackage::HiddenTarget, FTransform(V->RoofLocation() + FVector(0.f, 0.f, 60.f)), {}))
		{
			V->AddAttachment(T);
		}
	}
	// Yaylı tır: paketin yukarı fırlatan yayı tavanda
	if (Type == EVehicleType::YayliTir)
	{
		const FVector SpringLoc = V->RoofLocation() + FVector(-250.f, 0.f, 2.f);
		if (AActor* S = KulusturPackage::SpawnConfigured(GetWorld(), KulusturPackage::UpSpringB, FTransform(SpringLoc), {
			{ TEXT("LaunchHeight"), TEXT("1100") },
			{ TEXT("SnapSpringToFloor?"), TEXT("False") } }))
		{
			V->AddAttachment(S);
		}
	}
	return V;
}

void AKulusturTraffic::Init(AKulusturTrack* InTrack, EKulusturMode InMode, const FKulusturLevelConfig& InConfig, float StartX)
{
	Track = InTrack;
	Mode = InMode;
	Config = InConfig;
	Rng.Initialize(Config.Seed * 31 + 7);
	bLaneSwerve = (Config.Challenges & KChallenge::LaneSwerve) != 0;

	const int32 N = AKulusturTrack::NumLanes;
	const int32 StartLane = N / 2;
	LaneOffset.SetNum(N);
	for (int32 L = 0; L < N; ++L)
	{
		LaneOffset[L] = (L == StartLane) ? 0.f : Rng.FRandRange(-Config.SpeedVariation, Config.SpeedVariation);
	}

	float Front = StartX + AheadDist;
	if (Mode == EKulusturMode::Level && Track) Front = FMath::Min(Front, Track->GetFinishX() + 25000.f);

	for (int32 L = 0; L < N; ++L)
	{
		// Başlangıç şeridinde oyuncunun tırı tam StartX'te
		float Cursor = (L == StartLane) ? StartX : StartX + Rng.FRandRange(-900.f, 900.f);
		bool bFirst = true;
		// İleri
		// BackX: sıradaki aracın arka ucu (araç boyları farklı, üst üste binmesinler)
		float BackX = Cursor - 700.f;
		while (BackX < Front)
		{
			const EVehicleType T = (bFirst && L == StartLane) ? EVehicleType::Tir : PickType(BackX);
			float Back = 0.f;
			const float Len = VehicleLength(T, Back);
			if (!IsBlockedByObstacle(L, BackX, BackX + Len) || (bFirst && L == StartLane))
			{
				AKulusturVehicle* V = SpawnVehicle(T, L, BackX + Back, LaneBaseSpeed(L, BackX));
				if (bFirst && L == StartLane) StartVehicle = V;
			}
			bFirst = false;
			BackX += Len + Rng.FRandRange(450.f, 1500.f);
		}
		// Geri
		for (float C = Cursor - 2400.f - Rng.FRandRange(400.f, 1500.f); C > StartX - BehindDist; C -= 2400.f + Rng.FRandRange(400.f, 1500.f))
		{
			const EVehicleType T = PickType(C);
			float Back = 0.f;
			const float Len = VehicleLength(T, Back);
			if (IsLaneFree(L, C - Back - 100.f, C - Back + Len + 100.f, nullptr))
			{
				SpawnVehicle(T, L, C, LaneBaseSpeed(L, C) - Rng.FRandRange(0.f, 30.f));
			}
		}
	}
	UE_LOG(LogKulusturTraffic, Log, TEXT("Trafik hazir: %d arac"), Vehicles.Num());
}

AKulusturVehicle* AKulusturTraffic::SpawnFallingVehicle(float X, int32 Lane)
{
	const EVehicleType T = Rng.FRand() < 0.5f ? EVehicleType::Kamyonet : EVehicleType::Tir;
	AKulusturVehicle* V = SpawnVehicle(T, Lane, X, LaneBaseSpeed(Lane, X));
	if (!V) return nullptr;
	V->SetActorLocation(FVector(X, AKulusturTrack::LaneY(Lane), 3500.f));
	V->SetActorRotation(FRotator(0.f, Rng.FRandRange(-30.f, 30.f), Rng.FRandRange(-20.f, 20.f)));
	V->Crash(FVector(0.f, 0.f, -1200.f), FVector(Rng.FRandRange(-40.f, 40.f), Rng.FRandRange(-40.f, 40.f), Rng.FRandRange(-60.f, 60.f)));
	return V;
}

void AKulusturTraffic::SpeedBurst(float Mul, float Duration)
{
	BurstMul = Mul;
	BurstTime = Duration;
}

void AKulusturTraffic::RandomSwerve(float PlayerX)
{
	TArray<AKulusturVehicle*> Cands;
	for (AKulusturVehicle* V : Vehicles)
	{
		if (V && !V->IsCrashed() && !V->IsChangingLane())
		{
			const float X = V->GetActorLocation().X;
			if (X > PlayerX - 500.f && X < PlayerX + 9000.f) Cands.Add(V);
		}
	}
	for (int32 Try = 0; Try < 4 && Cands.Num() > 0; ++Try)
	{
		AKulusturVehicle* V = Cands[Rng.RandRange(0, Cands.Num() - 1)];
		const int32 Dir = Rng.FRand() < 0.5f ? -1 : 1;
		for (int32 D : { Dir, -Dir })
		{
			const int32 NL = V->Lane + D;
			if (NL < 0 || NL >= AKulusturTrack::NumLanes) continue;
			if (IsLaneFree(NL, V->BackX() - 300.f, V->FrontX() + 300.f, V) && !IsBlockedByObstacle(NL, V->BackX(), V->FrontX() + 2500.f))
			{
				V->StartLaneChange(NL, AKulusturTrack::LaneY(NL));
				return;
			}
		}
	}
}

void AKulusturTraffic::Explode(AKulusturVehicle* V)
{
	if (!V || V->bExploded) return;
	V->bExploded = true;
	const FVector C = V->GetActorLocation();
	for (AKulusturVehicle* O : Vehicles)
	{
		if (!O || O == V) continue;
		const FVector D = O->GetActorLocation() - C;
		const float Dist = D.Size();
		if (Dist < 1100.f)
		{
			const FVector Dir = D.GetSafeNormal2D();
			O->Crash(Dir * 900.f + FVector(0.f, 0.f, 1100.f), FVector(Rng.FRandRange(-120.f, 120.f), Rng.FRandRange(-120.f, 120.f), 0.f));
		}
	}
	if (V->Body && V->Body->IsSimulatingPhysics())
	{
		V->Body->AddImpulse(FVector(0.f, 0.f, 1300.f), NAME_None, true);
	}
	if (OnCrash) OnCrash(C, true);
}

void AKulusturTraffic::Update(float DeltaSeconds, float PlayerX, const AActor* PlayerBaseActor)
{
	DeltaSeconds = FMath::Min(DeltaSeconds, 0.05f);
	CrashSoundCooldown -= DeltaSeconds;

	// Hızlanma zorluğu
	float Mul = 1.f;
	if (BurstTime > 0.f)
	{
		BurstTime -= DeltaSeconds;
		Mul = BurstMul;
	}

	for (AKulusturVehicle* V : Vehicles)
	{
		if (!V) continue;
		V->SpeedMul = FMath::FInterpTo(V->SpeedMul, Mul, DeltaSeconds, 2.f);
		V->Drive(DeltaSeconds);
	}

	// Rastgele fren: arkadakiler çarpsın diye
	BrakeTimer += DeltaSeconds;
	if (BrakeTimer >= Config.BrakeInterval && Mode != EKulusturMode::Menu)
	{
		BrakeTimer = Rng.FRandRange(-1.f, 1.f);
		TArray<AKulusturVehicle*> Cands;
		for (AKulusturVehicle* V : Vehicles)
		{
			if (V && !V->IsCrashed() && V->GetActorLocation().X > PlayerX + 2500.f && V->GetActorLocation().X < PlayerX + 12000.f) Cands.Add(V);
		}
		if (Cands.Num() > 0)
		{
			Cands[Rng.RandRange(0, Cands.Num() - 1)]->Speed *= 0.65f;
		}
	}

	// Şerit değiştirme: zigzag araçlar sürekli, zorlukta herkes ara ara
	for (AKulusturVehicle* V : Vehicles)
	{
		if (!V || V->IsCrashed() || V->IsChangingLane() || V->Type != EVehicleType::Zigzag) continue;
		V->NextLaneChangeTime -= DeltaSeconds;
		if (V->NextLaneChangeTime <= 0.f)
		{
			V->NextLaneChangeTime = Rng.FRandRange(2.f, 4.f);
			const int32 Dir = Rng.FRand() < 0.5f ? -1 : 1;
			for (int32 D : { Dir, -Dir })
			{
				const int32 NL = V->Lane + D;
				if (NL >= 0 && NL < AKulusturTrack::NumLanes && IsLaneFree(NL, V->BackX() - 300.f, V->FrontX() + 300.f, V))
				{
					V->StartLaneChange(NL, AKulusturTrack::LaneY(NL));
					break;
				}
			}
		}
	}
	if (bLaneSwerve && Mode != EKulusturMode::Menu)
	{
		SwerveTimer += DeltaSeconds;
		if (SwerveTimer > 2.2f)
		{
			SwerveTimer = Rng.FRandRange(-0.8f, 0.5f);
			RandomSwerve(PlayerX);
		}
	}

	// Arkadan çarpma (şerit değiştiren iki şeritte de sayılır)
	for (int32 L = 0; L < AKulusturTrack::NumLanes; ++L)
	{
		TArray<AKulusturVehicle*> InLane;
		for (AKulusturVehicle* V : Vehicles)
		{
			if (V && !V->IsCrashed() && (V->Lane == L || (V->IsChangingLane() && V->PrevLane == L))) InLane.Add(V);
		}
		InLane.Sort([](const AKulusturVehicle& A, const AKulusturVehicle& B) { return A.GetActorLocation().X < B.GetActorLocation().X; });
		for (int32 i = 0; i + 1 < InLane.Num(); ++i)
		{
			AKulusturVehicle* Rear = InLane[i];
			AKulusturVehicle* Front = InLane[i + 1];
			if (Rear->IsCrashed() || Front->IsCrashed()) continue;
			if (Rear->FrontX() >= Front->BackX() - 10.f)
			{
				const float Rel = FMath::Max(150.f, Rear->Speed - Front->Speed);
				Rear->Crash(FVector(-Rel * 0.4f, Rng.FRandRange(-250.f, 250.f), 350.f + Rel * 0.3f), FVector(Rng.FRandRange(-40.f, 40.f), -50.f, Rng.FRandRange(-30.f, 30.f)));
				Front->Crash(FVector(Rel * 0.3f, Rng.FRandRange(-150.f, 150.f), 150.f), FVector(Rng.FRandRange(-20.f, 20.f), 25.f, 0.f));
				if (OnCrash) OnCrash(Front->GetActorLocation(), false);
			}
		}
	}

	// Engeller
	if (Track)
	{
		for (AKulusturVehicle* V : Vehicles)
		{
			if (!V || V->IsCrashed()) continue;
			const float F = V->FrontX();
			for (const FKulusturObstacle& O : Track->GetObstacles())
			{
				const bool bInLane = (O.LaneMask & (1u << V->Lane)) || (V->IsChangingLane() && (O.LaneMask & (1u << V->PrevLane)));
				if (!bInLane) continue;
				if (O.Kind == EObstacleKind::Ramp)
				{
					if (F >= O.X + 60.f && F < O.X + O.Length)
					{
						V->Crash(FVector(0.f, 0.f, 750.f), FVector(Rng.FRandRange(-15.f, 15.f), -20.f, 0.f));
						if (OnCrash) OnCrash(V->GetActorLocation(), false);
						break;
					}
				}
				else if (F >= O.X - 15.f && F < O.X + 300.f)
				{
					V->Crash(FVector(-V->Speed * 0.6f, Rng.FRandRange(-250.f, 250.f), 700.f), FVector(Rng.FRandRange(-60.f, 60.f), 160.f, Rng.FRandRange(-40.f, 40.f)));
					if (OnCrash) OnCrash(V->GetActorLocation(), false);
					break;
				}
			}
		}
	}

	// Tanker patlaması
	for (int32 i = 0; i < Vehicles.Num(); ++i)
	{
		AKulusturVehicle* V = Vehicles[i];
		if (V && V->Type == EVehicleType::Tanker && V->IsCrashed() && !V->bExploded && V->GetCrashTime() > 0.5f)
		{
			Explode(V);
		}
	}

	// Doğurma: her şeritte oyuncunun önünü ve arkasını dolu tut
	float FrontLimit = PlayerX + AheadDist;
	if (Mode == EKulusturMode::Level && Track) FrontLimit = FMath::Min(FrontLimit, Track->GetFinishX() + 25000.f);
	const float RearLimit = PlayerX - BehindDist;
	int32 Spawned = 0;
	for (int32 L = 0; L < AKulusturTrack::NumLanes && Spawned < 2 && Vehicles.Num() < 110; ++L)
	{
		float MaxFront = -1e9f, MinBack = 1e9f;
		for (const AKulusturVehicle* V : Vehicles)
		{
			if (!V || V->IsCrashed() || V->Lane != L) continue;
			MaxFront = FMath::Max(MaxFront, V->FrontX());
			MinBack = FMath::Min(MinBack, V->BackX());
		}
		if (MaxFront < -1e8f)
		{
			MaxFront = PlayerX + 3000.f;
			MinBack = MaxFront;
		}
		// Önüne
		{
			const float Gap = Rng.FRandRange(500.f, 1600.f);
			const EVehicleType T = PickType(MaxFront + Gap);
			float Back = 0.f;
			const float Len = VehicleLength(T, Back);
			float BackX = MaxFront + Gap;
			// Engelin içine düşüyorsa engelin arkasına at
			if (Track)
			{
				for (const FKulusturObstacle& O : Track->GetObstacles())
				{
					if (O.Kind != EObstacleKind::Finish && (O.LaneMask & (1u << L)) && BackX + Len > O.X - 200.f && BackX < O.X + O.Length + 200.f)
					{
						BackX = O.X + O.Length + 400.f;
					}
				}
			}
			if (BackX + Len < FrontLimit && !IsBlockedByObstacle(L, BackX, BackX + Len) && IsLaneFree(L, BackX - 200.f, BackX + Len + 200.f, nullptr))
			{
				SpawnVehicle(T, L, BackX + Back, LaneBaseSpeed(L, BackX) + Rng.FRandRange(0.f, 25.f));
				++Spawned;
			}
		}
		// Arkasına
		{
			const float Gap = Rng.FRandRange(500.f, 1600.f);
			const EVehicleType T = PickType(MinBack);
			float Back = 0.f;
			const float Len = VehicleLength(T, Back);
			const float FrontX = MinBack - Gap;
			if (FrontX - Len > RearLimit && !IsBlockedByObstacle(L, FrontX - Len, FrontX) && IsLaneFree(L, FrontX - Len - 200.f, FrontX + 200.f, nullptr))
			{
				// Arkadakiler biraz yavaş olsun ki hemen çarpışmasınlar
				SpawnVehicle(T, L, FrontX - Len + Back, LaneBaseSpeed(L, FrontX) - Rng.FRandRange(0.f, 60.f));
				++Spawned;
			}
		}
	}

	// Temizlik
	for (int32 i = Vehicles.Num() - 1; i >= 0; --i)
	{
		AKulusturVehicle* V = Vehicles[i];
		if (!IsValid(V))
		{
			Vehicles.RemoveAtSwap(i);
			continue;
		}
		const bool bUnderPlayer = PlayerBaseActor == V;
		const FVector Loc = V->GetActorLocation();
		const bool bOld = V->IsCrashed() && V->GetCrashTime() > 12.f && !bUnderPlayer;
		const bool bFarBehind = Loc.X < PlayerX - BehindDist - 6000.f && !bUnderPlayer;
		const bool bFarAhead = Loc.X > PlayerX + AheadDist + 15000.f;
		if (bOld || bFarBehind || bFarAhead || Loc.Z < -3000.f)
		{
			if (V == StartVehicle) StartVehicle = nullptr;
			V->DestroyWithAttachments();
			Vehicles.RemoveAtSwap(i);
		}
	}
}
