#include "Weapons/FPSWeaponBase.h"
#include "Combat/FPSHealthComponent.h"
#include "Combat/FPSBallisticTracer.h"
#include "Combat/FPSFermiTimeBubble.h"
#include "Combat/FPSFermiDecoyActor.h"
#include "BodyMechanics/FPSHumanBones.h"
#include "Weapons/FPSDilatedBulletActor.h"
#include "Audio/FPSAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Radar/FPSRadarSubsystem.h"
#include "Multiplayer/FPSPlayerState.h"
#include "Weapons/FPSGunsmithComponent.h"
#include "Character/FPSCharacter.h"
#include "UI/FPSHUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMeshSocket.h"
#include "ProceduralMeshComponent.h"

namespace
{
static void AddBoxToProcMeshBuffers(
	TArray<FVector>& Vertices,
	TArray<int32>& Triangles,
	TArray<FVector>& Normals,
	TArray<FVector2D>& UVs,
	TArray<FColor>& Colors,
	const FVector& Center,
	const FVector& HalfExtent,
	const FRotator& Rotation,
	const FColor& Color)
{
	const FQuat Q = Rotation.Quaternion();

	const FVector C[8] = {
		Q.RotateVector(FVector( HalfExtent.X, -HalfExtent.Y, -HalfExtent.Z)) + Center, // 0: Front Bottom Left
		Q.RotateVector(FVector( HalfExtent.X,  HalfExtent.Y, -HalfExtent.Z)) + Center, // 1: Front Bottom Right
		Q.RotateVector(FVector( HalfExtent.X,  HalfExtent.Y,  HalfExtent.Z)) + Center, // 2: Front Top Right
		Q.RotateVector(FVector( HalfExtent.X, -HalfExtent.Y,  HalfExtent.Z)) + Center, // 3: Front Top Left
		Q.RotateVector(FVector(-HalfExtent.X, -HalfExtent.Y, -HalfExtent.Z)) + Center, // 4: Back Bottom Left
		Q.RotateVector(FVector(-HalfExtent.X,  HalfExtent.Y, -HalfExtent.Z)) + Center, // 5: Back Bottom Right
		Q.RotateVector(FVector(-HalfExtent.X,  HalfExtent.Y,  HalfExtent.Z)) + Center, // 6: Back Top Right
		Q.RotateVector(FVector(-HalfExtent.X, -HalfExtent.Y,  HalfExtent.Z)) + Center  // 7: Back Top Left
	};

	auto AddQuad = [&](int32 i0, int32 i1, int32 i2, int32 i3, const FVector& Normal)
	{
		const int32 StartIdx = Vertices.Num();

		Vertices.Add(C[i0]);
		Vertices.Add(C[i1]);
		Vertices.Add(C[i2]);
		Vertices.Add(C[i3]);

		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);

		UVs.Add(FVector2D(0.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 1.0f));
		UVs.Add(FVector2D(0.0f, 1.0f));

		Colors.Add(Color);
		Colors.Add(Color);
		Colors.Add(Color);
		Colors.Add(Color);

		Triangles.Add(StartIdx + 0);
		Triangles.Add(StartIdx + 1);
		Triangles.Add(StartIdx + 2);

		Triangles.Add(StartIdx + 0);
		Triangles.Add(StartIdx + 2);
		Triangles.Add(StartIdx + 3);
	};

	AddQuad(0, 1, 2, 3, Q.RotateVector(FVector( 1.0f,  0.0f,  0.0f))); // Front (+X)
	AddQuad(5, 4, 7, 6, Q.RotateVector(FVector(-1.0f,  0.0f,  0.0f))); // Back (-X)
	AddQuad(1, 5, 6, 2, Q.RotateVector(FVector( 0.0f,  1.0f,  0.0f))); // Right (+Y)
	AddQuad(4, 0, 3, 7, Q.RotateVector(FVector( 0.0f, -1.0f,  0.0f))); // Left (-Y)
	AddQuad(3, 2, 6, 7, Q.RotateVector(FVector( 0.0f,  0.0f,  1.0f))); // Top (+Z)
	AddQuad(4, 5, 1, 0, Q.RotateVector(FVector( 0.0f,  0.0f, -1.0f))); // Bottom (-Z)
}
} // namespace

AFPSWeaponBase::AFPSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// Guaranteed 3D Procedural Carbine Geometry (100% immune to missing uncooked uasset meshes in packaged builds)
	CarbineProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CarbineProcMesh"));
	CarbineProcMesh->SetupAttachment(WeaponMesh);
	CarbineProcMesh->SetMobility(EComponentMobility::Movable);
	CarbineProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CarbineProcMesh->SetCastShadow(true);
	CarbineProcMesh->bCastDynamicShadow = true;
	CarbineProcMesh->SetOwnerNoSee(false);
	CarbineProcMesh->SetOnlyOwnerSee(false);
	CarbineProcMesh->SetHiddenInGame(false);
	CarbineProcMesh->SetVisibility(true, true);

	GunsmithComponent = CreateDefaultSubobject<UFPSGunsmithComponent>(TEXT("GunsmithComponent"));

	// Find standard basic shape assets for procedural 3D model
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
	UStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;
	UMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;

	auto SetupMeshComp = [this, BaseMat](TObjectPtr<UStaticMeshComponent>& Comp, const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(WeaponMesh);
		Comp->SetMobility(EComponentMobility::Movable);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
	};

	// 1. Lower & Upper Receiver (22cm x 3.8cm x 5.6cm)
	SetupMeshComp(ReceiverMesh, TEXT("ReceiverMesh"), CubeMesh, FVector(4.0f, 0.0f, 0.0f), FRotator::ZeroRotator, FVector(0.22f, 0.038f, 0.056f));

	// 2. Picatinny Top Rail (36cm x 2.4cm x 0.8cm)
	SetupMeshComp(TopRailMesh, TEXT("TopRailMesh"), CubeMesh, FVector(10.0f, 0.0f, 3.2f), FRotator::ZeroRotator, FVector(0.36f, 0.024f, 0.008f));

	// 3. Handguard (20cm x 4.2cm x 4.6cm)
	SetupMeshComp(HandguardMesh, TEXT("HandguardMesh"), CubeMesh, FVector(25.0f, 0.0f, 0.2f), FRotator::ZeroRotator, FVector(0.20f, 0.042f, 0.046f));

	// 4. Barrel (38cm long, cylinder aligned along X axis)
	SetupMeshComp(BarrelMesh, TEXT("BarrelMesh"), CylinderMesh, FVector(28.0f, 0.0f, 0.3f), FRotator(90.0f, 0.0f, 0.0f), FVector(0.016f, 0.016f, 0.38f));

	// 5. Muzzle Compensator / Birdcage Flash Hider (4cm long, 2.4cm diameter)
	SetupMeshComp(MuzzleCompMesh, TEXT("MuzzleCompMesh"), CylinderMesh, FVector(48.5f, 0.0f, 0.3f), FRotator(90.0f, 0.0f, 0.0f), FVector(0.024f, 0.024f, 0.04f));

	// 6. Pistol Grip (Angled back 18 deg)
	SetupMeshComp(GripMesh, TEXT("GripMesh"), CubeMesh, FVector(-1.5f, 0.0f, -8.0f), FRotator(-18.0f, 0.0f, 0.0f), FVector(0.042f, 0.032f, 0.12f));

	// 7. Buffer Tube & Buttstock
	SetupMeshComp(BufferTubeMesh, TEXT("BufferTubeMesh"), CylinderMesh, FVector(-15.0f, 0.0f, 0.8f), FRotator(90.0f, 0.0f, 0.0f), FVector(0.026f, 0.026f, 0.16f));
	SetupMeshComp(StockMesh, TEXT("StockMesh"), CubeMesh, FVector(-20.0f, 0.0f, -1.0f), FRotator(-4.0f, 0.0f, 0.0f), FVector(0.12f, 0.036f, 0.13f));

	// 8. Magazine Assembly (Root scene component + Mesh for procedural reload translation)
	MagazineBaseLoc = FVector(8.0f, 0.0f, -2.8f);
	MagazineRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MagazineRoot"));
	MagazineRoot->SetupAttachment(WeaponMesh);
	MagazineRoot->SetRelativeLocation(MagazineBaseLoc);

	MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
	MagazineMesh->SetupAttachment(MagazineRoot);
	MagazineMesh->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) MagazineMesh->SetStaticMesh(CubeMesh);
	if (BaseMat) MagazineMesh->SetMaterial(0, BaseMat);
	MagazineMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -7.5f));
	MagazineMesh->SetRelativeRotation(FRotator(8.0f, 0.0f, 0.0f));
	MagazineMesh->SetRelativeScale3D(FVector(0.065f, 0.025f, 0.16f));
	MagazineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MagazineMesh->SetCastShadow(true);

	// 9. Reflex Optic Sight (Mount + Frame Hood + Glowing Reticle Dot at Z = +6.8cm)
	SetupMeshComp(SightBaseMesh, TEXT("SightBaseMesh"), CubeMesh, FVector(5.0f, 0.0f, 4.2f), FRotator::ZeroRotator, FVector(0.07f, 0.026f, 0.012f));
	SetupMeshComp(SightHoodMesh, TEXT("SightHoodMesh"), CubeMesh, FVector(5.0f, 0.0f, 8.8f), FRotator::ZeroRotator, FVector(0.05f, 0.036f, 0.008f));
	SetupMeshComp(ReticleDotMesh, TEXT("ReticleDotMesh"), SphereMesh, FVector(5.0f, 0.0f, 6.8f), FRotator::ZeroRotator, FVector(0.007f, 0.007f, 0.007f));

	// 10. Front Iron Sight Post & Tritium Pip (Co-witnessed at Z = +6.2cm)
	SetupMeshComp(FrontSightPostMesh, TEXT("FrontSightPostMesh"), CubeMesh, FVector(33.0f, 0.0f, 4.6f), FRotator::ZeroRotator, FVector(0.015f, 0.012f, 0.030f));
	SetupMeshComp(FrontSightDotMesh, TEXT("FrontSightDotMesh"), SphereMesh, FVector(33.0f, 0.0f, 6.2f), FRotator::ZeroRotator, FVector(0.005f, 0.005f, 0.005f));

	// 11. 3D AK-47 Rifle Mesh (aligned 1:1 with weapon_r bone socket on SK_BaseMale)
	AK47StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AK47StaticMesh"));
	AK47StaticMesh->SetupAttachment(WeaponMesh);
	AK47StaticMesh->SetMobility(EComponentMobility::Movable);
	AK47StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AK47StaticMesh->SetCastShadow(true);
	AK47StaticMesh->SetRelativeLocation(FVector::ZeroVector);
	AK47StaticMesh->SetRelativeRotation(FRotator::ZeroRotator);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> AK47MeshFinder(TEXT("/Game/Characters/BaseMale/SM_AK47.SM_AK47"));
	if (AK47MeshFinder.Succeeded())
	{
		AK47StaticMesh->SetStaticMesh(AK47MeshFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKSteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKWoodMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKBakeMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite"));
	if (AKSteelMatFinder.Succeeded()) AK47StaticMesh->SetMaterial(0, AKSteelMatFinder.Object);
	if (AKWoodMatFinder.Succeeded()) AK47StaticMesh->SetMaterial(1, AKWoodMatFinder.Object);
	if (AKBakeMatFinder.Succeeded()) AK47StaticMesh->SetMaterial(2, AKBakeMatFinder.Object);
}

void AFPSWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// Construct guaranteed procedural 3D NF-98 carbine geometry
	BuildProceduralCarbineGeometry();

	if (AK47StaticMesh && !AK47StaticMesh->GetStaticMesh())
	{
		UStaticMesh* LoadedAK47 = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SM_AK47.SM_AK47"));
		if (LoadedAK47)
		{
			AK47StaticMesh->SetStaticMesh(LoadedAK47);
		}
	}

	const bool bIsAK47 = (WeaponID == FName(TEXT("AK47")));
	if (bIsAK47 && AK47StaticMesh && AK47StaticMesh->GetStaticMesh())
	{
		UMaterialInterface* SteelMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
		UMaterialInterface* WoodMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood"));
		UMaterialInterface* BakeMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite"));
		if (SteelMat) AK47StaticMesh->SetMaterial(0, SteelMat);
		if (WoodMat) AK47StaticMesh->SetMaterial(1, WoodMat);
		if (BakeMat) AK47StaticMesh->SetMaterial(2, BakeMat);

		// Ensure sockets exist on SM_AK47 static mesh at runtime
		if (UStaticMesh* SM = AK47StaticMesh->GetStaticMesh())
		{
			auto EnsureStaticSocket = [SM](FName SocketName, const FVector& Loc, const FRotator& Rot)
			{
				if (!SM->FindSocket(SocketName))
				{
					UStaticMeshSocket* NewSocket = NewObject<UStaticMeshSocket>(SM);
					if (NewSocket)
					{
						NewSocket->SocketName = SocketName;
						NewSocket->RelativeLocation = Loc;
						NewSocket->RelativeRotation = Rot;
						NewSocket->SetFlags(RF_Public | RF_Standalone);
						NewSocket->AddToRoot();
						SM->AddSocket(NewSocket);
					}
				}
			};

			EnsureStaticSocket(FName(TEXT("weapon_l_ik")), FVector(2.6f, -17.5f, -2.6f), FRotator::ZeroRotator);
			EnsureStaticSocket(FName(TEXT("Muzzle_Flash_Socket")), FVector(0.0f, -52.0f, 2.4f), FRotator(0.0f, -90.0f, 0.0f));
			EnsureStaticSocket(FName(TEXT("Ejection_Port_Socket")), FVector(2.2f, -4.0f, 2.5f), FRotator::ZeroRotator);
		}

		AK47StaticMesh->SetVisibility(true);
		if (CarbineProcMesh) CarbineProcMesh->SetVisibility(false);
	}
	else
	{
		if (AK47StaticMesh)
		{
			AK47StaticMesh->SetVisibility(false);
		}
		if (CarbineProcMesh)
		{
			CarbineProcMesh->SetVisibility(true, true);
			CarbineProcMesh->SetHiddenInGame(false);
			CarbineProcMesh->SetOnlyOwnerSee(false);
			CarbineProcMesh->SetOwnerNoSee(false);
			CarbineProcMesh->SetCastShadow(true);
		}
	}

	// Hide legacy basic shape static mesh components to avoid rendering duplicate or empty shapes
	TArray<UStaticMeshComponent*> LegacyBasicParts = {
		ReceiverMesh, TopRailMesh, HandguardMesh, BarrelMesh, MuzzleCompMesh,
		GripMesh, BufferTubeMesh, StockMesh, MagazineMesh, SightBaseMesh,
		SightHoodMesh, ReticleDotMesh, FrontSightPostMesh, FrontSightDotMesh
	};
	for (UStaticMeshComponent* Comp : LegacyBasicParts)
	{
		if (Comp) Comp->SetVisibility(false);
	}

	// Initialize tactical dynamic material finishes
	auto ApplyCompColor = [](UStaticMeshComponent* Comp, const FLinearColor& Color)
	{
		if (Comp)
		{
			UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(0);
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("Color"), Color);
			}
		}
	};

	ApplyCompColor(ReceiverMesh, FLinearColor(0.06f, 0.065f, 0.075f));
	ApplyCompColor(TopRailMesh, FLinearColor(0.035f, 0.035f, 0.04f));
	ApplyCompColor(BufferTubeMesh, FLinearColor(0.035f, 0.035f, 0.04f));
	ApplyCompColor(BarrelMesh, FLinearColor(0.03f, 0.03f, 0.035f));
	ApplyCompColor(MuzzleCompMesh, FLinearColor(0.025f, 0.025f, 0.03f));
	ApplyCompColor(HandguardMesh, FLinearColor(0.05f, 0.052f, 0.055f));
	ApplyCompColor(GripMesh, FLinearColor(0.04f, 0.04f, 0.045f));
	ApplyCompColor(StockMesh, FLinearColor(0.045f, 0.045f, 0.05f));
	ApplyCompColor(MagazineMesh, FLinearColor(0.035f, 0.035f, 0.038f));
	ApplyCompColor(SightBaseMesh, FLinearColor(0.03f, 0.03f, 0.035f));
	ApplyCompColor(SightHoodMesh, FLinearColor(0.03f, 0.03f, 0.035f));
	ApplyCompColor(FrontSightPostMesh, FLinearColor(0.035f, 0.035f, 0.04f));

	// High-intensity emissive reflex red dot reticle (creates vibrant optic bloom)
	ApplyCompColor(ReticleDotMesh, FLinearColor(15.0f, 0.02f, 0.02f));
	// Glowing green tritium night-sight pip
	ApplyCompColor(FrontSightDotMesh, FLinearColor(0.1f, 12.0f, 0.1f));

	if (GunsmithComponent)
	{
		GunsmithComponent->ApplyModifiersToWeapon(this);
		MagazineCapacity += GunsmithComponent->GetTotalMagazineCapacityBonus();
		TacticalReloadDuration *= GunsmithComponent->GetTotalReloadDurationMultiplier();
		EmptyReloadDuration *= GunsmithComponent->GetTotalReloadDurationMultiplier();
	}

	CurrentAmmo = MagazineCapacity;
	ReserveAmmo = MaxReserveAmmo;
	CurrentSpread = HipfireMinSpread;
	CurrentState = EFPSWeaponState::Idle;
	CurrentADSAlpha = 0.0f;

	// Load authentic audio defaults if not overridden in blueprint
	if (!FireSound)
	{
		FireSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Weapons/Fire/A_Carbine_556_Single_Shot.A_Carbine_556_Single_Shot"));
		if (!FireSound)
		{
			FireSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Weapons/Fire/A_Carbine_556_Fire_Unsuppressed.A_Carbine_556_Fire_Unsuppressed"));
		}
	}
	if (!ShellDropSound)
	{
		ShellDropSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Combat/A_Shell_Casing_Drop.A_Shell_Casing_Drop"));
	}
	if (!ReloadSound)
	{
		ReloadSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Weapons/Reloads/A_Reload_Phase2_MagRelease_TopCover.A_Reload_Phase2_MagRelease_TopCover"));
	}
	if (!DryFireSound)
	{
		DryFireSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Weapons/Reloads/A_Weapon_DryFire_Click.A_Weapon_DryFire_Click"));
	}

	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (PawnOwner)
	{
		LastControlRot = PawnOwner->GetControlRotation();
	}
}

void AFPSWeaponBase::BuildProceduralCarbineGeometry()
{
	if (!CarbineProcMesh) return;

	CarbineProcMesh->ClearAllMeshSections();

	// Section 0: Main Carbine Metal & Polymer Components
	TArray<FVector> Vertices0;
	TArray<int32> Triangles0;
	TArray<FVector> Normals0;
	TArray<FVector2D> UVs0;
	TArray<FColor> Colors0;
	TArray<FProcMeshTangent> Tangents0;

	// 1. Lower & Upper Receiver (22cm x 3.8cm x 5.6cm)
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(4.0f, 0.0f, 0.0f), FVector(11.0f, 1.9f, 2.8f), FRotator::ZeroRotator, FColor(45, 48, 52, 255));

	// 2. Picatinny Top Rail (36cm x 2.4cm x 0.8cm)
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(10.0f, 0.0f, 3.2f), FVector(18.0f, 1.2f, 0.4f), FRotator::ZeroRotator, FColor(28, 28, 30, 255));

	// 3. M-LOK Handguard (20cm x 4.2cm x 4.6cm)
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(25.0f, 0.0f, 0.2f), FVector(10.0f, 2.1f, 2.3f), FRotator::ZeroRotator, FColor(38, 40, 42, 255));

	// 4. Barrel (38cm long x 1.6cm diameter)
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(28.0f, 0.0f, 0.3f), FVector(19.0f, 0.8f, 0.8f), FRotator::ZeroRotator, FColor(32, 34, 38, 255));

	// 5. Muzzle Brake / Compensator (4cm x 2.4cm x 2.4cm)
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(48.5f, 0.0f, 0.3f), FVector(2.0f, 1.2f, 1.2f), FRotator::ZeroRotator, FColor(20, 20, 22, 255));

	// 6. Ergonomic Pistol Grip (angled -18 deg)
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(-1.5f, 0.0f, -6.5f), FVector(2.1f, 1.6f, 6.0f), FRotator(-18.0f, 0.0f, 0.0f), FColor(30, 32, 34, 255));

	// 7. Buffer Tube & Buttstock
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(-15.0f, 0.0f, 0.8f), FVector(8.0f, 1.3f, 1.3f), FRotator::ZeroRotator, FColor(28, 28, 30, 255));
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(-20.0f, 0.0f, -1.0f), FVector(6.0f, 1.8f, 6.5f), FRotator(-4.0f, 0.0f, 0.0f), FColor(32, 32, 35, 255));

	// 8. Curved 30-round Banana Magazine
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(8.0f, 0.0f, -6.0f), FVector(3.2f, 1.25f, 4.0f), FRotator(4.0f, 0.0f, 0.0f), FColor(30, 30, 32, 255));
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(8.6f, 0.0f, -13.0f), FVector(3.2f, 1.2f, 4.5f), FRotator(12.0f, 0.0f, 0.0f), FColor(26, 26, 28, 255));
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(9.8f, 0.0f, -17.5f), FVector(3.6f, 1.35f, 0.5f), FRotator(12.0f, 0.0f, 0.0f), FColor(18, 18, 20, 255));

	// 9. Reflex Sight Mount & Hood
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(5.0f, 0.0f, 4.2f), FVector(3.5f, 1.3f, 0.6f), FRotator::ZeroRotator, FColor(24, 24, 26, 255));
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(5.0f, -1.5f, 6.8f), FVector(2.5f, 0.3f, 2.0f), FRotator::ZeroRotator, FColor(20, 20, 22, 255));
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(5.0f, 1.5f, 6.8f), FVector(2.5f, 0.3f, 2.0f), FRotator::ZeroRotator, FColor(20, 20, 22, 255));
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(5.0f, 0.0f, 8.8f), FVector(2.5f, 1.8f, 0.4f), FRotator::ZeroRotator, FColor(20, 20, 22, 255));

	// 10. Front Iron Sight Post
	AddBoxToProcMeshBuffers(Vertices0, Triangles0, Normals0, UVs0, Colors0,
		FVector(33.0f, 0.0f, 4.6f), FVector(0.8f, 0.6f, 1.5f), FRotator::ZeroRotator, FColor(24, 24, 26, 255));

	CarbineProcMesh->CreateMeshSection(0, Vertices0, Triangles0, Normals0, UVs0, Colors0, Tangents0, false);

	// Section 1: Emissive Optics (Red Dot & Tritium Night-Sight)
	TArray<FVector> Vertices1;
	TArray<int32> Triangles1;
	TArray<FVector> Normals1;
	TArray<FVector2D> UVs1;
	TArray<FColor> Colors1;
	TArray<FProcMeshTangent> Tangents1;

	// Reflex Red Dot (Center of hood at Z = 6.8cm)
	AddBoxToProcMeshBuffers(Vertices1, Triangles1, Normals1, UVs1, Colors1,
		FVector(5.0f, 0.0f, 6.8f), FVector(0.4f, 0.4f, 0.4f), FRotator::ZeroRotator, FColor(255, 30, 30, 255));

	// Tritium Pip on front sight (Z = 6.2cm)
	AddBoxToProcMeshBuffers(Vertices1, Triangles1, Normals1, UVs1, Colors1,
		FVector(33.0f, 0.0f, 6.2f), FVector(0.3f, 0.3f, 0.3f), FRotator::ZeroRotator, FColor(50, 255, 50, 255));

	CarbineProcMesh->CreateMeshSection(1, Vertices1, Triangles1, Normals1, UVs1, Colors1, Tangents1, false);

	// Apply Materials
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BaseMat)
	{
		BaseMat = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	if (BaseMat)
	{
		UMaterialInstanceDynamic* BodyMID = CarbineProcMesh->CreateDynamicMaterialInstance(0, BaseMat);
		if (BodyMID)
		{
			BodyMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.06f, 0.065f, 0.075f, 1.0f));
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.06f, 0.065f, 0.075f, 1.0f));
			BodyMID->SetScalarParameterValue(TEXT("Roughness"), 0.45f);
			BodyMID->SetScalarParameterValue(TEXT("Metallic"), 0.75f);
		}

		UMaterialInstanceDynamic* DotMID = CarbineProcMesh->CreateDynamicMaterialInstance(1, BaseMat);
		if (DotMID)
		{
			DotMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(15.0f, 0.05f, 0.05f, 1.0f));
			DotMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(15.0f, 0.05f, 0.05f, 1.0f));
			DotMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(15.0f, 0.05f, 0.05f, 1.0f));
			DotMID->SetScalarParameterValue(TEXT("Emissive"), 10.0f);
		}
	}
}

void AFPSWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Dynamic spread recovery towards target min spread
	const float TargetSpread = bIsAimingDownSights ? ADSMinSpread : HipfireMinSpread;
	CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetSpread, DeltaTime, RecoilRecoverySpeed);

	// 2. Smooth ADS Alpha interpolation
	const float TargetADSAlpha = bIsAimingDownSights ? 1.0f : 0.0f;
	CurrentADSAlpha = FMath::FInterpTo(CurrentADSAlpha, TargetADSAlpha, DeltaTime, ADSSpeed);

	// 3. Physical recoil kick recovery
	RecoilOffsetLoc = FMath::VInterpTo(RecoilOffsetLoc, FVector::ZeroVector, DeltaTime, 22.0f);
	RecoilOffsetRot = FMath::RInterpTo(RecoilOffsetRot, FRotator::ZeroRotator, DeltaTime, 22.0f);

	// 4. Mouse look sway inertia
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (PawnOwner)
	{
		const FRotator CurrentControlRot = PawnOwner->GetControlRotation();
		FRotator DeltaRot = CurrentControlRot - LastControlRot;
		DeltaRot.Normalize();
		LastControlRot = CurrentControlRot;

		// Dampen weapon sway by 85% when aiming down sights
		const float SwayDampener = FMath::Lerp(1.0f, 0.15f, CurrentADSAlpha);
		const FRotator TargetSway = FRotator(-DeltaRot.Pitch * 0.7f * SwayDampener, DeltaRot.Yaw * 0.7f * SwayDampener, DeltaRot.Yaw * 0.4f * SwayDampener);
		CurrentSwayRot = FMath::RInterpTo(CurrentSwayRot, TargetSway, DeltaTime, 12.0f);
	}

	// 5. Multi-Phase Procedural Reload Animation
	FVector ReloadOffsetLoc = FVector::ZeroVector;
	FRotator ReloadOffsetRot = FRotator::ZeroRotator;

	if (CurrentState == EFPSWeaponState::Reloading)
	{
		ReloadElapsedTime += DeltaTime;
		const float Alpha = FMath::Clamp(ReloadElapsedTime / ReloadDuration, 0.0f, 1.0f);

		// Weapon Cant & Tilt (Rolls 22 deg outward so magwell faces operator)
		const FVector CantLoc = FVector(-3.5f, 2.0f, -3.0f);
		const FRotator CantRot = FRotator(6.0f, -4.0f, 22.0f);

		float CantWeight = 0.0f;
		if (Alpha < 0.20f)
		{
			CantWeight = FMath::SmoothStep(0.0f, 0.20f, Alpha);
		}
		else if (Alpha <= 0.78f)
		{
			CantWeight = 1.0f;
		}
		else
		{
			CantWeight = 1.0f - FMath::SmoothStep(0.78f, 1.0f, Alpha);
		}
		ReloadOffsetLoc = CantLoc * CantWeight;
		ReloadOffsetRot = CantRot * CantWeight;

		// Magazine Movement
		if (MagazineRoot)
		{
			FVector MagOffset = FVector::ZeroVector;
			FRotator MagRot = FRotator::ZeroRotator;

			if (Alpha >= 0.20f && Alpha < 0.42f)
			{
				// Phase 2: Mag Drop (ejects downward out of magwell)
				const float DropT = (Alpha - 0.20f) / (0.42f - 0.20f);
				MagOffset = FVector(-DropT * 2.0f, 0.0f, -DropT * 28.0f);
				MagRot = FRotator(DropT * 18.0f, 0.0f, 0.0f);
			}
			else if (Alpha >= 0.42f && Alpha < 0.70f)
			{
				// Phase 3: Fresh Mag Slide-In (slides up into magwell)
				const float InsertT = (Alpha - 0.42f) / (0.70f - 0.42f);
				MagOffset = FVector(-(1.0f - InsertT) * 1.5f, 0.0f, -(1.0f - InsertT) * 24.0f);
				MagRot = FRotator((1.0f - InsertT) * 12.0f, 0.0f, 0.0f);
			}
			else if (Alpha >= 0.70f && Alpha < 0.76f)
			{
				// Palm Slap Bump
				const float SlapT = (Alpha - 0.70f) / 0.06f;
				MagOffset = FVector(0.0f, 0.0f, FMath::Sin(SlapT * PI) * 0.6f);
			}

			MagazineRoot->SetRelativeLocation(MagazineBaseLoc + MagOffset);
			MagazineRoot->SetRelativeRotation(MagRot);
		}

		// Phase 4: Bolt Rack Snap (if empty reload)
		if (bIsEmptyReload && Alpha >= 0.77f && Alpha <= 0.86f)
		{
			const float BoltT = (Alpha - 0.77f) / 0.09f;
			const float BoltPulse = FMath::Sin(BoltT * PI);
			ReloadOffsetLoc += FVector(-1.2f, 0.0f, -0.4f) * BoltPulse;
			ReloadOffsetRot += FRotator(1.8f, 0.0f, -1.2f) * BoltPulse;
		}

		if (ReloadElapsedTime >= ReloadDuration)
		{
			FinishReload();
		}
	}

	// 6. Blend Final Weapon Relative Transform (Hipfire <-> ADS + Recoil + Reload + Sway)
	USceneComponent* AttachParentComp = GetRootComponent() ? GetRootComponent()->GetAttachParent() : nullptr;
	const bool bIsAttachedToMesh = (AttachParentComp && AttachParentComp->IsA<USkeletalMeshComponent>());

	FVector BaseLoc;
	FRotator BaseRot;

	if (bIsAttachedToMesh)
	{
		BaseLoc = ThirdPersonRelativeLocation;
		BaseRot = ThirdPersonRelativeRotation;
	}
	else
	{
		BaseLoc = FMath::Lerp(HipfireRelativeLocation, ADSRelativeLocation, CurrentADSAlpha);
		BaseRot = FMath::Lerp(HipfireRelativeRotation, ADSRelativeRotation, CurrentADSAlpha);
	}

	const FVector FinalLoc = BaseLoc + RecoilOffsetLoc + ReloadOffsetLoc;
	const FRotator FinalRot = BaseRot + RecoilOffsetRot + ReloadOffsetRot + (bIsAttachedToMesh ? FRotator::ZeroRotator : CurrentSwayRot);

	SetActorRelativeLocation(FinalLoc);
	SetActorRelativeRotation(FinalRot);
}

void AFPSWeaponBase::StartFire()
{
	if (CurrentState == EFPSWeaponState::Reloading) return;

	if (CurrentAmmo <= 0)
	{
		if (DryFireSound)
		{
			UGameplayStatics::PlaySound2D(this, DryFireSound);
		}
		Reload();
		return;
	}

	CurrentState = EFPSWeaponState::Firing;
	ShotsFiredInBurst = 0;

	FireShot();

	if (FireMode == EFPSFireMode::FullAuto)
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AFPSWeaponBase::FireShot, GetTimeBetweenShots(), true);
	}
}

void AFPSWeaponBase::StopFire()
{
	if (CurrentState == EFPSWeaponState::Firing)
	{
		CurrentState = EFPSWeaponState::Idle;
	}
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void AFPSWeaponBase::FireShot()
{
	if (CurrentAmmo <= 0)
	{
		StopFire();
		Reload();
		return;
	}

	CurrentAmmo--;
	ShotsFiredInBurst++;
	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);
	OnWeaponFired.Broadcast();

	// Weapon Gunshot Audio (Strictly 1 single shot per trigger cycle with dynamic distance attenuation)
	FVector MuzzleWorldLoc = GetActorLocation() + GetActorForwardVector() * 48.0f + FVector(0.0f, 0.0f, 4.0f);
	if (AK47StaticMesh && AK47StaticMesh->DoesSocketExist(TEXT("Muzzle_Flash_Socket")))
	{
		MuzzleWorldLoc = AK47StaticMesh->GetSocketLocation(TEXT("Muzzle_Flash_Socket"));
	}
	if (FireSound)
	{
		UFPSAudioSubsystem::PlaySpatialSoundWithFalloff(
			this,
			FireSound,
			MuzzleWorldLoc,
			bIsSuppressed ? 0.45f : 0.90f,
			250.0f,
			GunshotMaxAudibleDistance,
			FMath::RandRange(0.985f, 1.015f),
			EFPSAudioChannel::SFX
		);
	}

	APawn* PawnOwner = Cast<APawn>(GetOwner());

	// Deaf Accessibility / Visual Audio: Notify local player's HUD if gunfire originated from another pawn
	if (UWorld* ShotWorld = GetWorld())
	{
		if (APlayerController* FirstPC = ShotWorld->GetFirstPlayerController())
		{
			APawn* LocalPawn = FirstPC->GetPawn();
			if (LocalPawn && LocalPawn != PawnOwner)
			{
				if (AFPSCharacter* LocalChar = Cast<AFPSCharacter>(LocalPawn))
				{
					if (UFPSHUDWidget* LocalHUD = LocalChar->GetActiveHUDWidget())
					{
						const float ShotDist = FVector::Dist(MuzzleWorldLoc, LocalChar->GetActorLocation());
						if (ShotDist <= GunshotMaxAudibleDistance)
						{
							const float Attenuation = FMath::Clamp(1.0f - (ShotDist / GunshotMaxAudibleDistance), 0.25f, 1.0f);
							const float GunfireIntensity = (bIsSuppressed ? 0.75f : 1.5f) * Attenuation;
							LocalHUD->AddDirectionalIndicator(MuzzleWorldLoc, GunfireIntensity, EThreatIndicatorType::Gunfire);
						}
					}
				}
			}
		}
	}

	// Dynamic Shell Casing Drop Audio (Attenuated brass bounce after mechanical ejection)
	if (ShellDropSound)
	{
		const FVector EjectionLoc = GetActorLocation() + (GetActorRightVector() * 20.0f) + FVector(0.0f, 0.0f, 6.0f);
		FTimerHandle ShellDropTimer;
		FTimerDelegate ShellDropDelegate = FTimerDelegate::CreateUObject(this, &AFPSWeaponBase::PlayDelayedShellDrop, EjectionLoc);
		GetWorldTimerManager().SetTimer(ShellDropTimer, ShellDropDelegate, FMath::RandRange(0.24f, 0.32f), false);
	}

	// Trace line calculation from Owner Camera or Tactical Bot Aim
	if (PawnOwner)
	{
		AFPSCharacter* ShooterFPSChar = Cast<AFPSCharacter>(PawnOwner);
		APlayerController* PC = Cast<APlayerController>(PawnOwner->GetController());

		FVector CameraLoc = FVector::ZeroVector;
		FRotator CameraRot = FRotator::ZeroRotator;
		bool bHasValidAimOrigin = false;

		if (PC && PC->PlayerCameraManager)
		{
			CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
			CameraRot = PC->PlayerCameraManager->GetCameraRotation();
			bHasValidAimOrigin = true;
		}
		else if (ShooterFPSChar && ShooterFPSChar->IsTacticalBot())
		{
			CameraLoc = ShooterFPSChar->GetBotEyeLocation();
			CameraRot = ShooterFPSChar->GetBotAimRotation();
			bHasValidAimOrigin = true;
		}

		if (bHasValidAimOrigin)
		{
			// Dynamic spread bloom (reduced by 55% during Max Payne Ballistic Dive for crisp mid-air marksmanship)
			const bool bIsDiving = (ShooterFPSChar && ShooterFPSChar->IsBallisticDiving());
			const float DiveSpreadScale = bIsDiving ? 0.45f : 1.0f;

			CurrentSpread = FMath::Min(MaxSpread * DiveSpreadScale, CurrentSpread + SpreadPerShot * DiveSpreadScale);
			float SpreadHalfAngleRad = FMath::DegreesToRadians((ShooterFPSChar && ShooterFPSChar->IsTacticalBot()) ? 2.4f : (CurrentSpread * 0.5f));
			FVector ShotDirection = FMath::VRandCone(CameraRot.Vector(), SpreadHalfAngleRad);

			FVector TraceEnd = CameraLoc + (ShotDirection * MaxRange);

			FHitResult HitResult;
			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(this);
			TraceParams.AddIgnoredActor(PawnOwner);
			TraceParams.bReturnPhysicalMaterial = true;

			UWorld* World = GetWorld();
			if (World)
			{
				UFPSRadarSubsystem* RadarSubsystem = World->GetSubsystem<UFPSRadarSubsystem>();
				if (RadarSubsystem)
				{
					EFPSPlayerTeam ShooterTeam = EFPSPlayerTeam::None;
					AFPSPlayerState* PS = PawnOwner->GetPlayerState<AFPSPlayerState>();
					if (PS)
					{
						ShooterTeam = PS->GetPlayerTeam();
					}
					else if (ShooterFPSChar)
					{
						ShooterTeam = ShooterFPSChar->GetBotTeam();
					}
					RadarSubsystem->ReportGunfire(PawnOwner, PawnOwner->GetActorLocation(), ShooterTeam, bIsSuppressed);
				}

				const bool bShooterInTimeBubble = bIsDiving
					|| (ShooterFPSChar && ShooterFPSChar->IsFermiTimeBubbleActive())
					|| AFPSFermiTimeBubble::IsLocationInsideAnyBubble(World, CameraLoc)
					|| AFPSFermiTimeBubble::IsLocationInsideAnyBubble(World, MuzzleWorldLoc);

				if (bShooterInTimeBubble)
				{
					// Case 1: Shooter is inside an active Fermi Bubble
					// Convert hitscan immediately into a physical 3D dilated slug starting at muzzle/eye location
					AFPSDilatedBulletActor* DilatedBullet = World->SpawnActor<AFPSDilatedBulletActor>(AFPSDilatedBulletActor::StaticClass(), MuzzleWorldLoc, ShotDirection.Rotation());
					if (DilatedBullet)
					{
						DilatedBullet->InitializeSlug(MuzzleWorldLoc, ShotDirection, BaseDamage, MaxRange, PawnOwner);
					}
				}
				else
				{
					// Case 2: Shooter is outside. Check if hitscan ray intersects any active Fermi Bubble
					// First, test world geometry line trace to find hit distance
					const bool bHitWorld = World->LineTraceSingleByChannel(HitResult, CameraLoc, TraceEnd, ECC_Visibility, TraceParams);
					const float WorldHitDistance = bHitWorld ? HitResult.Distance : MaxRange;

					FVector BubbleEntryLoc = FVector::ZeroVector;
					float BubbleEntryDist = 0.0f;
					const bool bEntersBubble = AFPSFermiTimeBubble::FindEarliestBubbleEntry(World, CameraLoc, TraceEnd, WorldHitDistance, BubbleEntryLoc, BubbleEntryDist);

					if (bEntersBubble && BubbleEntryDist < WorldHitDistance)
					{
						// Ray encounters bubble boundary BEFORE hitting world geometry!
						// Terminate ray at boundary and spawn physical dilated slug cruising inward
						AFPSBallisticTracer* LeadTracer = World->SpawnActor<AFPSBallisticTracer>(AFPSBallisticTracer::StaticClass(), MuzzleWorldLoc, ShotDirection.Rotation());
						if (LeadTracer)
						{
							LeadTracer->InitializeTracer(MuzzleWorldLoc, BubbleEntryLoc, false, false);
						}

						const float RemainingDist = FMath::Max(100.0f, MaxRange - BubbleEntryDist);
						AFPSDilatedBulletActor* DilatedBullet = World->SpawnActor<AFPSDilatedBulletActor>(AFPSDilatedBulletActor::StaticClass(), BubbleEntryLoc, ShotDirection.Rotation());
						if (DilatedBullet)
						{
							DilatedBullet->InitializeSlug(BubbleEntryLoc, ShotDirection, BaseDamage, RemainingDist, PawnOwner);
						}
					}
					else
					{
						// Ray does NOT enter any bubble before hitting geometry (or at all). Standard instantaneous hitscan resolution!
						FVector TracerImpactPoint = TraceEnd;
						if (bHitWorld)
						{
							TracerImpactPoint = HitResult.ImpactPoint;
							AActor* HitActor = HitResult.GetActor();
							if (HitActor)
							{
								if (AFPSFermiDecoyActor* Decoy = Cast<AFPSFermiDecoyActor>(HitActor))
								{
									Decoy->OnDecoyHit(PawnOwner, HitResult.ImpactPoint);
								}
								else
								{
									UFPSHealthComponent* HealthComp = HitActor->FindComponentByClass<UFPSHealthComponent>();
									if (HealthComp)
									{
										const bool bIsHead = (HitResult.BoneName == FName(TEXT("head")));
										const bool bIsArmorProtected = FFPSHumanSkeleton::IsBoneArmorProtected(HitResult.BoneName);
										const float PreHealth = HealthComp->GetCurrentHealth();
										const float PreShield = HealthComp->GetCurrentShield();

										HealthComp->ApplyDamage(BaseDamage, PawnOwner, HitResult);

										const bool bKilled = HealthComp->IsDead() || (PreHealth > 0.0f && HealthComp->GetCurrentHealth() <= 0.0f);
										const bool bShieldBroken = (PreShield > 0.0f && HealthComp->GetCurrentShield() <= 0.0f) || (bIsArmorProtected && PreShield > 0.0f);

										if (ShooterFPSChar)
										{
											ShooterFPSChar->NotifyEnemyHit(HitActor, HitResult, bIsHead, bShieldBroken, bKilled);
										}
									}
								}
							}
						}

						// Visual tracer streak
						AFPSBallisticTracer* Tracer = World->SpawnActor<AFPSBallisticTracer>(AFPSBallisticTracer::StaticClass(), MuzzleWorldLoc, ShotDirection.Rotation());
						if (Tracer)
						{
							Tracer->InitializeTracer(MuzzleWorldLoc, TracerImpactPoint, false, false);
						}

						// Deaf Accessibility / Bullet Flyby: Detect supersonic crack passing near local player
						if (APlayerController* FirstPC = World->GetFirstPlayerController())
						{
							APawn* LocalPawn = FirstPC->GetPawn();
							if (LocalPawn && LocalPawn != PawnOwner)
							{
								if (AFPSCharacter* LocalChar = Cast<AFPSCharacter>(LocalPawn))
								{
									if (UFPSHUDWidget* LocalHUD = LocalChar->GetActiveHUDWidget())
									{
										const FVector LocalPlayerHead = LocalChar->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
										const FVector ClosestPt = FMath::ClosestPointOnSegment(LocalPlayerHead, MuzzleWorldLoc, TracerImpactPoint);
										const float FlybyDist = FVector::Dist(ClosestPt, LocalPlayerHead);

										if (FlybyDist <= 450.0f) // Within 4.5 meters
										{
											const float FlybyIntensity = FMath::Clamp(1.8f - (FlybyDist / 450.0f) * 0.8f, 1.0f, 2.0f);
											LocalHUD->AddDirectionalIndicator(MuzzleWorldLoc, FlybyIntensity, EThreatIndicatorType::Gunfire);
										}
									}
								}
							}
						}
					}
				}
			}

			// Apply camera recoil kick (scaled by Gunsmith attachment modifiers)
			float RecoilMod = GunsmithComponent ? GunsmithComponent->GetTotalRecoilMultiplier() : 1.0f;
			float PitchKick = -RecoilPitchKick * (bIsAimingDownSights ? 0.6f : 1.0f) * RecoilMod;
			float YawKick = FMath::RandRange(-RecoilYawSpread, RecoilYawSpread) * RecoilMod;
			if (PC)
			{
				PC->AddPitchInput(PitchKick);
				PC->AddYawInput(YawKick);
			}

			// Procedural physical weapon recoil kick
			const float ADSRecoilDamp = bIsAimingDownSights ? 0.6f : 1.0f;
			RecoilOffsetLoc += FVector(-2.0f * ADSRecoilDamp * RecoilMod, FMath::RandRange(-0.15f, 0.15f) * RecoilMod, 0.5f * ADSRecoilDamp * RecoilMod);
			RecoilOffsetRot += FRotator(FMath::RandRange(1.8f, 2.8f) * ADSRecoilDamp * RecoilMod, FMath::RandRange(-0.6f, 0.6f) * RecoilMod, FMath::RandRange(-0.8f, 0.8f) * RecoilMod);
		}
	}

	// Burst mode cutoff check
	if (FireMode == EFPSFireMode::Burst && ShotsFiredInBurst >= 3)
	{
		StopFire();
	}
}

void AFPSWeaponBase::Reload()
{
	if (CurrentState == EFPSWeaponState::Reloading) return;
	if (CurrentAmmo >= MagazineCapacity || ReserveAmmo <= 0) return;

	CurrentState = EFPSWeaponState::Reloading;
	StopFire();
	bIsAimingDownSights = false; // Drop ADS when reloading

	bIsEmptyReload = (CurrentAmmo == 0);
	ReloadDuration = bIsEmptyReload ? EmptyReloadDuration : TacticalReloadDuration;
	ReloadElapsedTime = 0.0f;

	if (ReloadSound)
	{
		UGameplayStatics::PlaySound2D(this, ReloadSound);
	}
}

void AFPSWeaponBase::FinishReload()
{
	int32 NeededAmmo = MagazineCapacity - CurrentAmmo;
	int32 AmmoToLoad = FMath::Min(NeededAmmo, ReserveAmmo);

	CurrentAmmo += AmmoToLoad;
	ReserveAmmo -= AmmoToLoad;

	CurrentState = EFPSWeaponState::Idle;
	if (MagazineRoot)
	{
		MagazineRoot->SetRelativeLocation(MagazineBaseLoc);
		MagazineRoot->SetRelativeRotation(FRotator::ZeroRotator);
	}

	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);
}

void AFPSWeaponBase::SetADS(bool bInADS)
{
	if (bInADS && CurrentState == EFPSWeaponState::Reloading)
	{
		return;
	}
	bIsAimingDownSights = bInADS;
}

void AFPSWeaponBase::ApplyLoadoutCosmetics(int32 CamoIndex, int32 HeadgearIndex, int32 ArmorIndex, int32 WeaponFinishIndex)
{
	FLinearColor SteelColor(0.18f, 0.20f, 0.22f, 1.0f);
	FLinearColor WoodOrGuardColor(0.32f, 0.14f, 0.06f, 1.0f);
	FLinearColor MagColor(0.42f, 0.18f, 0.05f, 1.0f);
	float MetallicVal = 0.85f;
	float RoughnessVal = 0.35f;

	switch (WeaponFinishIndex)
	{
	case 1: // Tactical Blackout Polymer
		SteelColor = FLinearColor(0.04f, 0.04f, 0.05f, 1.0f);
		WoodOrGuardColor = FLinearColor(0.06f, 0.07f, 0.08f, 1.0f);
		MagColor = FLinearColor(0.05f, 0.05f, 0.06f, 1.0f);
		MetallicVal = 0.75f;
		RoughnessVal = 0.42f;
		break;
	case 2: // Desert Sand Cerakote
		SteelColor = FLinearColor(0.42f, 0.36f, 0.26f, 1.0f);
		WoodOrGuardColor = FLinearColor(0.54f, 0.47f, 0.34f, 1.0f);
		MagColor = FLinearColor(0.34f, 0.29f, 0.20f, 1.0f);
		MetallicVal = 0.45f;
		RoughnessVal = 0.55f;
		break;
	case 3: // 24K Gold & Opal Prestige
		SteelColor = FLinearColor(0.95f, 0.72f, 0.18f, 1.0f);
		WoodOrGuardColor = FLinearColor(0.08f, 0.10f, 0.14f, 1.0f);
		MagColor = FLinearColor(0.0f, 0.85f, 0.95f, 1.0f);
		MetallicVal = 0.98f;
		RoughnessVal = 0.18f;
		break;
	default: // 0: Classic Blued Steel & Russian Redwood
		break;
	}

	auto ApplyTintToMesh = [MetallicVal, RoughnessVal](UStaticMeshComponent* Comp, int32 MatIdx, const FLinearColor& Tint)
	{
		if (!Comp) return;
		if (UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(MatIdx))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Tint);
			MID->SetVectorParameterValue(TEXT("BaseColor"), Tint);
			MID->SetScalarParameterValue(TEXT("Metallic"), MetallicVal);
			MID->SetScalarParameterValue(TEXT("Roughness"), RoughnessVal);
		}
	};

	if (AK47StaticMesh)
	{
		ApplyTintToMesh(AK47StaticMesh, 0, SteelColor);
		ApplyTintToMesh(AK47StaticMesh, 1, WoodOrGuardColor);
		ApplyTintToMesh(AK47StaticMesh, 2, MagColor);
	}

	if (CarbineProcMesh)
	{
		if (UMaterialInstanceDynamic* MID = CarbineProcMesh->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), SteelColor);
			MID->SetVectorParameterValue(TEXT("BaseColor"), SteelColor);
			MID->SetScalarParameterValue(TEXT("Metallic"), MetallicVal);
			MID->SetScalarParameterValue(TEXT("Roughness"), RoughnessVal);
		}
	}

	ApplyTintToMesh(ReceiverMesh, 0, SteelColor);
	ApplyTintToMesh(BarrelMesh, 0, SteelColor);
	ApplyTintToMesh(HandguardMesh, 0, WoodOrGuardColor);
	ApplyTintToMesh(StockMesh, 0, WoodOrGuardColor);
	ApplyTintToMesh(MagazineMesh, 0, MagColor);
}

void AFPSWeaponBase::RefillBotAmmo()
{
	ReserveAmmo = FMath::Max(ReserveAmmo, MagazineCapacity * 4);
	if (CurrentAmmo <= 0)
	{
		CurrentAmmo = MagazineCapacity;
	}
}

void AFPSWeaponBase::PlayDelayedShellDrop(FVector DropLocation)
{
	if (!ShellDropSound || !GetWorld()) return;

	FHitResult FloorHit;
	FCollisionQueryParams FloorParams;
	FloorParams.AddIgnoredActor(this);
	if (GetOwner()) FloorParams.AddIgnoredActor(GetOwner());

	FVector ContactLoc = DropLocation - FVector(0.0f, 0.0f, 85.0f);
	if (GetWorld()->LineTraceSingleByChannel(FloorHit, DropLocation, DropLocation - FVector(0.0f, 0.0f, 250.0f), ECC_Visibility, FloorParams))
	{
		ContactLoc = FloorHit.ImpactPoint;
	}

	// Dynamic distance-attenuated shell drop audio (calibrated subtle metallic bounce)
	UFPSAudioSubsystem::PlaySpatialSoundWithFalloff(
		this,
		ShellDropSound,
		ContactLoc,
		ShellDropBaseVolume,
		ShellDropMinDistance,
		ShellDropMaxDistance,
		FMath::RandRange(0.94f, 1.06f),
		EFPSAudioChannel::SFX
	);
}

FVector AFPSWeaponBase::GetForegripWorldLocation() const
{
	if (CarbineProcMesh && CarbineProcMesh->IsVisible())
	{
		return CarbineProcMesh->GetComponentTransform().TransformPosition(FVector(25.0f, 0.0f, 0.2f));
	}
	if (HandguardMesh && HandguardMesh->IsVisible())
	{
		return HandguardMesh->GetComponentLocation();
	}
	if (ReceiverMesh)
	{
		return ReceiverMesh->GetComponentTransform().TransformPosition(FVector(25.0f, 0.0f, 0.2f));
	}
	return GetActorLocation() + GetActorForwardVector() * 30.0f;
}
