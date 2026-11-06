#include "KulusturVehicle.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor Dark(0.02f, 0.02f, 0.025f);
	const FLinearColor CabWhite(0.88f, 0.88f, 0.86f);
	const FLinearColor Glass(0.04f, 0.08f, 0.14f);
	const FLinearColor Chrome(0.55f, 0.55f, 0.58f);
	constexpr float LaneChangeSpeed = 420.f; // cm/sn yana
}

AKulusturVehicle::AKulusturVehicle()
{
	PrimaryActorTick.bCanEverTick = false; // trafik yöneticisi hareket ettiriyor

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = CubeFinder.Object;
	CylinderMesh = CylFinder.Object;
	BaseMaterial = MatFinder.Object;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetStaticMesh(CubeMesh);
	Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetGenerateOverlapEvents(false);
	Body->BodyInstance.bUseCCD = true;
	SetRootComponent(Body);

	Wheels = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Wheels"));
	Wheels->SetupAttachment(Body);
	Wheels->SetStaticMesh(CylFinder.Object);
	Wheels->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wheels->SetUsingAbsoluteScale(true);
	Wheels->SetMobility(EComponentMobility::Movable);
	Wheels->SetCastShadow(false);
}

float AKulusturVehicle::CenterHeightFor(EVehicleType Type)
{
	switch (Type)
	{
	case EVehicleType::Kamyonet: return 210.f;
	case EVehicleType::Tanker: return 140.f;
	default: return 270.f;
	}
}

void AKulusturVehicle::Paint(UStaticMeshComponent* C, const FLinearColor& Color)
{
	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		C->SetMaterial(0, MID);
	}
}

UStaticMeshComponent* AKulusturVehicle::AddPart(UStaticMesh* Mesh, const FVector& Loc, const FVector& Size, const FLinearColor& Color, bool bCollide, const FRotator& Rot)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetStaticMesh(Mesh);
	Part->SetMobility(EComponentMobility::Movable);
	Part->SetupAttachment(Body);
	// Gövde ölçekli; parçalar mutlak ölçek kullanır, konumları gövde ölçeğine bölünür.
	Part->SetUsingAbsoluteScale(true);
	Part->SetRelativeLocation(Loc / BodyScale);
	Part->SetRelativeRotation(Rot);
	Part->SetWorldScale3D(Size / 100.f);
	Part->SetGenerateOverlapEvents(false);
	if (bCollide)
	{
		Part->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	}
	else
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
	}
	Part->RegisterComponent();
	Paint(Part, Color);
	Parts.Add(Part);
	return Part;
}

void AKulusturVehicle::AddWheel(const FVector& Loc, float Diameter, float Width)
{
	for (float Side : { -1.f, 1.f })
	{
		const FVector P(Loc.X, Side * Loc.Y, Loc.Z);
		// ISM örnekleri bileşen uzayında; bileşen mutlak ölçekli olduğu için düz konum verilir.
		Wheels->AddInstance(FTransform(FRotator(0.f, 0.f, 90.f), P / BodyScale * BodyScale, FVector(Diameter / 100.f, Diameter / 100.f, Width / 100.f)));
	}
}

void AKulusturVehicle::Setup(EVehicleType InType, const FLinearColor& Color, float InSpeed, int32 InLane)
{
	Type = InType;
	Speed = InSpeed;
	Lane = PrevLane = InLane;
	BaseZ = GetActorLocation().Z;
	TargetY = GetActorLocation().Y;

	FVector BodySize(1400.f, 300.f, 320.f);
	switch (Type)
	{
	case EVehicleType::DevTir: BodySize = FVector(2600.f, 300.f, 320.f); break;
	case EVehicleType::Otobus: BodySize = FVector(1900.f, 300.f, 330.f); break;
	case EVehicleType::Kamyonet: BodySize = FVector(700.f, 260.f, 200.f); break;
	case EVehicleType::Tanker: BodySize = FVector(1300.f, 280.f, 60.f); break;
	default: break;
	}
	BodyScale = BodySize / 100.f;
	Body->SetWorldScale3D(BodyScale);
	Paint(Body, Color);

	auto StandardCab = [&](float FrontOfBody, float CabCenterZ)
	{
		const float CabX = FrontOfBody + 20.f + 150.f;
		AddPart(CubeMesh, FVector(CabX, 0.f, CabCenterZ), FVector(300.f, 280.f, 260.f), CabWhite, true);
		AddPart(CubeMesh, FVector(CabX + 147.f, 0.f, CabCenterZ + 70.f), FVector(10.f, 240.f, 90.f), Glass, false);
		AddPart(CubeMesh, FVector(CabX + 152.f, 0.f, CabCenterZ - 90.f), FVector(10.f, 260.f, 40.f), Chrome, false);
		AddPart(CubeMesh, FVector(FrontOfBody + 10.f, 0.f, CabCenterZ - 100.f), FVector(40.f, 140.f, 30.f), Dark, false);
		return CabX + 150.f;
	};

	switch (Type)
	{
	case EVehicleType::Otobus:
	{
		FrontOffset = BackOffset = 950.f;
		RoofOffset = 165.f;
		for (float Side : { -1.f, 1.f })
		{
			AddPart(CubeMesh, FVector(20.f, Side * 151.f, 45.f), FVector(1700.f, 4.f, 85.f), Glass, false);
			AddPart(CubeMesh, FVector(0.f, Side * 151.f, -60.f), FVector(1880.f, 4.f, 18.f), FLinearColor::White, false);
		}
		AddPart(CubeMesh, FVector(951.f, 0.f, 30.f), FVector(4.f, 260.f, 130.f), Glass, false);
		AddPart(CubeMesh, FVector(0.f, 0.f, 166.f), FVector(1600.f, 200.f, 4.f), FLinearColor(0.85f, 0.85f, 0.85f), false);
		for (float X : { -650.f, -480.f, 650.f }) AddWheel(FVector(X, 145.f, -215.f));
		break;
	}
	case EVehicleType::Kamyonet:
	{
		BackOffset = 350.f;
		RoofOffset = 100.f;
		const float CabX = 350.f + 15.f + 130.f;
		AddPart(CubeMesh, FVector(CabX, 0.f, -10.f), FVector(260.f, 250.f, 220.f), CabWhite, true);
		AddPart(CubeMesh, FVector(CabX + 127.f, 0.f, 40.f), FVector(10.f, 220.f, 80.f), Glass, false);
		FrontOffset = CabX + 130.f;
		for (float X : { -220.f, 450.f }) AddWheel(FVector(X, 125.f, -155.f), 100.f, 30.f);
		break;
	}
	case EVehicleType::Tanker:
	{
		BackOffset = 650.f;
		RoofOffset = 310.f;
		// Silindir tank (X ekseni boyunca yatık)
		AddPart(CylinderMesh, FVector(0.f, 0.f, 170.f), FVector(280.f, 280.f, 1240.f), Color, true, FRotator(90.f, 0.f, 0.f));
		AddPart(CylinderMesh, FVector(0.f, 0.f, 170.f), FVector(290.f, 290.f, 40.f), Chrome, false, FRotator(90.f, 0.f, 0.f));
		AddPart(CubeMesh, FVector(0.f, 0.f, 312.f), FVector(200.f, 60.f, 10.f), FLinearColor(0.9f, 0.1f, 0.05f), false);
		Paint(Body, Dark);
		FrontOffset = StandardCab(650.f, 80.f);
		for (float X : { -500.f, -340.f, 400.f }) AddWheel(FVector(X, 135.f, -85.f));
		AddWheel(FVector(830.f, 135.f, -85.f));
		break;
	}
	case EVehicleType::DevTir:
	{
		BackOffset = 1300.f;
		RoofOffset = 160.f;
		AddPart(CubeMesh, FVector(0.f, 0.f, 0.f), FVector(40.f, 304.f, 324.f), Dark, false); // römork arası
		AddPart(CubeMesh, FVector(-650.f, 0.f, 162.f), FVector(1200.f, 80.f, 4.f), FLinearColor::White, false);
		AddPart(CubeMesh, FVector(650.f, 0.f, 162.f), FVector(1200.f, 80.f, 4.f), FLinearColor::White, false);
		FrontOffset = StandardCab(1300.f, -50.f);
		for (float X : { -1150.f, -990.f, -200.f, -40.f, 1050.f }) AddWheel(FVector(X, 145.f, -215.f));
		AddWheel(FVector(1470.f, 145.f, -215.f));
		break;
	}
	default:
	{
		// Tır ve türevleri
		BackOffset = 700.f;
		RoofOffset = 160.f;
		FrontOffset = StandardCab(700.f, -50.f);
		for (float X : { -560.f, -400.f, 450.f }) AddWheel(FVector(X, 145.f, -215.f));
		AddWheel(FVector(870.f, 145.f, -215.f));

		if (Type == EVehicleType::Tir)
		{
			AddPart(CubeMesh, FVector(0.f, 0.f, 162.f), FVector(1300.f, 80.f, 4.f), FLinearColor::White, false);
		}
		else if (Type == EVehicleType::YayliTir)
		{
			// Sarı-siyah uyarı şeritleri (yay paketten takılacak)
			for (int32 i = 0; i < 7; ++i)
			{
				for (float Side : { -1.f, 1.f })
				{
					AddPart(CubeMesh, FVector(-600.f + i * 200.f, Side * 151.f, 100.f), FVector(100.f, 4.f, 60.f), (i % 2) ? Dark : FLinearColor(1.f, 0.8f, 0.f), false);
				}
			}
		}
		else if (Type == EVehicleType::Firlatici)
		{
			// Ön tarafta yeşil rampa: üstüne basınca ileri fırlatır
			AddPart(CubeMesh, FVector(420.f, 0.f, 200.f), FVector(560.f, 280.f, 30.f), FLinearColor(0.1f, 0.9f, 0.3f), true, FRotator(14.f, 0.f, 0.f));
			for (int32 i = 0; i < 3; ++i)
			{
				AddPart(CubeMesh, FVector(-500.f + i * 220.f, 0.f, 163.f), FVector(120.f, 160.f, 4.f), FLinearColor(0.1f, 0.9f, 0.3f), false, FRotator(0.f, 45.f, 0.f));
			}
			LaunchPadBox = FBox(FVector(150.f, -150.f, 150.f), FVector(720.f, 150.f, 420.f));
		}
		else if (Type == EVehicleType::Hoplayan)
		{
			AddPart(CubeMesh, FVector(0.f, 0.f, -175.f), FVector(1300.f, 200.f, 30.f), FLinearColor(0.9f, 0.9f, 0.1f), false); // süspansiyon
			for (float Side : { -1.f, 1.f })
			{
				AddPart(CubeMesh, FVector(0.f, Side * 151.f, 0.f), FVector(1300.f, 4.f, 40.f), FLinearColor(1.f, 0.9f, 0.1f), false);
			}
			NextHop = 1.5f + FMath::FRand() * 2.f;
		}
		else if (Type == EVehicleType::Zigzag)
		{
			for (int32 i = 0; i < 6; ++i)
			{
				for (float Side : { -1.f, 1.f })
				{
					AddPart(CubeMesh, FVector(-550.f + i * 220.f, Side * 151.f, 0.f), FVector(80.f, 4.f, 300.f), Dark, false, FRotator(0.f, 0.f, 0.f));
				}
			}
		}
		break;
	}
	}

	if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Dark);
		Wheels->SetMaterial(0, MID);
	}
}

void AKulusturVehicle::StartLaneChange(int32 NewLane, float NewY)
{
	if (bCrashed || NewLane == Lane) return;
	PrevLane = Lane;
	Lane = NewLane;
	TargetY = NewY;
	bChangingLane = true;
}

void AKulusturVehicle::Drive(float DeltaSeconds)
{
	LifeTime += DeltaSeconds;
	if (bCrashed)
	{
		CrashTime += DeltaSeconds;
		return;
	}

	FVector Loc = GetActorLocation();
	const float S = Speed * SpeedMul;
	FVector Vel(S, 0.f, 0.f);

	if (bChangingLane)
	{
		const float Dy = TargetY - Loc.Y;
		const float Step = LaneChangeSpeed * DeltaSeconds;
		if (FMath::Abs(Dy) <= Step)
		{
			Loc.Y = TargetY;
			bChangingLane = false;
			PrevLane = Lane;
		}
		else
		{
			Loc.Y += FMath::Sign(Dy) * Step;
			Vel.Y = FMath::Sign(Dy) * LaneChangeSpeed;
		}
		SetActorRotation(FRotator(0.f, bChangingLane ? FMath::Sign(Dy) * 6.f : 0.f, 0.f));
	}

	if (Type == EVehicleType::Hoplayan)
	{
		HopClock += DeltaSeconds;
		float Z = BaseZ;
		if (HopClock >= NextHop)
		{
			const float T = HopClock - NextHop;
			const float Dur = 0.9f;
			if (T < Dur)
			{
				Z = BaseZ + 320.f * FMath::Sin(PI * T / Dur);
				Vel.Z = 320.f * PI / Dur * FMath::Cos(PI * T / Dur);
			}
			else
			{
				HopClock = 0.f;
				NextHop = 2.f + FMath::FRand() * 2.5f;
			}
		}
		Loc.Z = Z;
	}

	Loc.X += S * DeltaSeconds;
	// Teleport yok: fizik bunu kinematik hareket sayar ve devrik araçları iter.
	SetActorLocation(Loc, false, nullptr, ETeleportType::None);
	// Karakter zıplarken aracın hızını alsın diye.
	Body->ComponentVelocity = Vel;
	for (UStaticMeshComponent* P : Parts)
	{
		if (P) P->ComponentVelocity = Vel;
	}
}

void AKulusturVehicle::Crash(const FVector& ExtraVelocity, const FVector& SpinDegPerSec)
{
	if (bCrashed)
	{
		return;
	}
	bCrashed = true;
	bChangingLane = false;
	CrashTime = 0.f;

	Body->SetMassOverrideInKg(NAME_None, 9000.f, true);
	Body->SetSimulatePhysics(true);
	Body->SetPhysicsLinearVelocity(FVector(Speed * SpeedMul, 0.f, 0.f) + ExtraVelocity);
	Body->SetPhysicsAngularVelocityInDegrees(SpinDegPerSec);
	Body->ComponentVelocity = FVector::ZeroVector;
}

bool AKulusturVehicle::IsOnLaunchPad(const FVector& WorldPoint) const
{
	if (!LaunchPadBox.IsValid || bCrashed) return false;
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(WorldPoint);
	return LaunchPadBox.IsInside(Local);
}

void AKulusturVehicle::AddAttachment(AActor* A)
{
	if (A)
	{
		A->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		Attachments.Add(A);
	}
}

void AKulusturVehicle::DestroyWithAttachments()
{
	for (TWeakObjectPtr<AActor>& A : Attachments)
	{
		if (A.IsValid()) A->Destroy();
	}
	Attachments.Empty();
	Destroy();
}
