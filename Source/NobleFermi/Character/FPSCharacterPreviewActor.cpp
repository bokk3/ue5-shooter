#include "Character/FPSCharacterPreviewActor.h"
#include "Character/FPSOperatorGearComponent.h"
#include "Character/FPSAnimInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/GameInstance.h"
#include "Economy/FPSEconomySubsystem.h"

AFPSCharacterPreviewActor::AFPSCharacterPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	// 1. Root Scene
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	// 2. Camera: Calibrated to frame the full Task Force Echo Operator & Hangar stage
	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(RootComponent);
	PreviewCamera->SetRelativeLocation(FVector(410.0f, 0.0f, 66.0f));
	PreviewCamera->SetRelativeRotation(FRotator(2.0f, 180.0f, 0.0f));
	PreviewCamera->FieldOfView = 50.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	PreviewCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	PreviewCamera->PostProcessSettings.AutoExposureBias = 0.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMinBrightness = false;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = false;
	PreviewCamera->PostProcessSettings.bOverride_BloomIntensity = true;
	PreviewCamera->PostProcessSettings.BloomIntensity = 0.25f;

	// 3. Studio Lighting: 3-point tactical dynamic lighting
	StudioKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioKeyLight"));
	StudioKeyLight->SetupAttachment(RootComponent);
	StudioKeyLight->SetMobility(EComponentMobility::Movable);
	StudioKeyLight->SetRelativeLocation(FVector(190.0f, -120.0f, 210.0f));
	StudioKeyLight->SetRelativeRotation(FRotator(-35.0f, 145.0f, 0.0f));
	StudioKeyLight->SetIntensity(1500.0f);
	StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.40f)); // Warm amber key light
	StudioKeyLight->SetAttenuationRadius(1500.0f);
	StudioKeyLight->SetInnerConeAngle(25.0f);
	StudioKeyLight->SetOuterConeAngle(65.0f);

	StudioRimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioRimLight"));
	StudioRimLight->SetupAttachment(RootComponent);
	StudioRimLight->SetMobility(EComponentMobility::Movable);
	StudioRimLight->SetRelativeLocation(FVector(-160.0f, 120.0f, 175.0f));
	StudioRimLight->SetRelativeRotation(FRotator(-25.0f, -35.0f, 0.0f));
	StudioRimLight->SetIntensity(1800.0f);
	StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical cyan rim light (#00E5FF)
	StudioRimLight->SetAttenuationRadius(1400.0f);
	StudioRimLight->SetInnerConeAngle(30.0f);
	StudioRimLight->SetOuterConeAngle(70.0f);
	StudioRimLight->SetCastShadows(false);

	StudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
	StudioFillLight->SetupAttachment(RootComponent);
	StudioFillLight->SetMobility(EComponentMobility::Movable);
	StudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 120.0f));
	StudioFillLight->SetIntensity(600.0f);
	StudioFillLight->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f)); // Clean fill
	StudioFillLight->SetAttenuationRadius(1500.0f);
	StudioFillLight->SetCastShadows(false);

	StudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
	StudioAmbientLight->SetupAttachment(RootComponent);
	StudioAmbientLight->SetMobility(EComponentMobility::Movable);
	StudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	StudioAmbientLight->SetIntensity(200.0f);
	StudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
	StudioAmbientLight->SetAttenuationRadius(1500.0f);
	StudioAmbientLight->SetCastShadows(false);

	StudioSunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StudioSunLight"));
	StudioSunLight->SetupAttachment(RootComponent);
	StudioSunLight->SetMobility(EComponentMobility::Movable);
	StudioSunLight->SetRelativeRotation(FRotator(-30.0f, 165.0f, 0.0f));
	StudioSunLight->SetIntensity(0.0f); // Disabled inside indoor hangar stage
	StudioSunLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.88f));
	StudioSunLight->SetCastShadows(false);
	// 4. Turntable Root (Rotates in Tick)
	TurntableRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TurntableRoot"));
	TurntableRoot->SetupAttachment(RootComponent);
	TurntableRoot->SetRelativeLocation(FVector::ZeroVector);

	// 5. Basic Shape Resources
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	UStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;
	UMaterialInterface* BaseMat = BaseMatFinder.Succeeded() ? BaseMatFinder.Object : nullptr;

	auto SetupMeshComp = [this, BaseMat](TObjectPtr<UStaticMeshComponent>& Comp, const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(TurntableRoot);
		Comp->SetMobility(EComponentMobility::Movable);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
	};

	bIsRotating = false;

	// 5. Tactical Turntable Pedestal (Radius 135cm, Height 14cm with beveled rim)
	PedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PedestalMesh"));
	PedestalMesh->SetupAttachment(TurntableRoot);
	PedestalMesh->SetMobility(EComponentMobility::Movable);
	PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	PedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
	PedestalMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	PedestalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PedestalMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlinthFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth"));
	if (PlinthFinder.Succeeded())
	{
		PedestalMesh->SetStaticMesh(PlinthFinder.Object);
	}

	// 5.5 Master High-Fidelity Posed Operator (Vanguard Spec-Ops Soldier with 7-Zone Tactical Armor, Visor, Boots & AK-47)
	OperatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OperatorMesh"));
	OperatorMesh->SetupAttachment(TurntableRoot);
	OperatorMesh->SetMobility(EComponentMobility::Movable);
	OperatorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	OperatorMesh->SetRelativeRotation(FRotator(0.0f, 15.0f, 0.0f)); // Heroic 15-degree angle facing camera
	OperatorMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	OperatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OperatorMesh->SetCastShadow(true);
	OperatorMesh->SetVisibility(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PosedSoldierFinder(TEXT("/Game/Characters/BaseMale/SM_Soldier_Posed.SM_Soldier_Posed"));
	if (PosedSoldierFinder.Succeeded())
	{
		OperatorMesh->SetStaticMesh(PosedSoldierFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VisorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKSteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKWoodMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKBakeMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite"));

	if (BaseBodyMatFinder.Succeeded()) OperatorMesh->SetMaterial(0, BaseBodyMatFinder.Object);
	if (VisorMatFinder.Succeeded()) OperatorMesh->SetMaterial(1, VisorMatFinder.Object);
	if (AKSteelMatFinder.Succeeded()) OperatorMesh->SetMaterial(2, AKSteelMatFinder.Object);
	if (AKWoodMatFinder.Succeeded()) OperatorMesh->SetMaterial(3, AKWoodMatFinder.Object);
	if (AKBakeMatFinder.Succeeded()) OperatorMesh->SetMaterial(4, AKBakeMatFinder.Object);

	if (AKSteelMatFinder.Succeeded()) PedestalMesh->SetMaterial(0, AKSteelMatFinder.Object);
	if (VisorMatFinder.Succeeded()) PedestalMesh->SetMaterial(1, VisorMatFinder.Object);

	// Skeletal BodyMesh kept as inactive fallback
	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(TurntableRoot);
	BodyMesh->SetVisibility(false);

	OperatorGearComponent = CreateDefaultSubobject<UFPSOperatorGearComponent>(TEXT("OperatorGearComponent"));

	AK47Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AK47Mesh"));
	AK47Mesh->SetupAttachment(TurntableRoot);
	AK47Mesh->SetVisibility(false);

	// 7. Human Male Anatomical Skeleton (180 cm anthropometry fallback)
	SetupMeshComp(PelvisMesh, TEXT("PelvisMesh"), CubeMesh, FVector(0.0f, 0.0f, 96.0f), FRotator::ZeroRotator, FVector(0.24f, 0.32f, 0.16f));
	SetupMeshComp(SpineLowerMesh, TEXT("SpineLowerMesh"), CylinderMesh, FVector(0.0f, 0.0f, 110.0f), FRotator::ZeroRotator, FVector(0.22f, 0.26f, 0.14f));
	SetupMeshComp(SpineUpperMesh, TEXT("SpineUpperMesh"), CylinderMesh, FVector(0.0f, 0.0f, 124.0f), FRotator::ZeroRotator, FVector(0.24f, 0.28f, 0.14f));
	SetupMeshComp(ChestMesh, TEXT("ChestMesh"), CubeMesh, FVector(0.0f, 0.0f, 138.0f), FRotator::ZeroRotator, FVector(0.26f, 0.38f, 0.16f));
	SetupMeshComp(VestMesh, TEXT("VestMesh"), CubeMesh, FVector(1.0f, 0.0f, 132.0f), FRotator::ZeroRotator, FVector(0.30f, 0.36f, 0.26f));
	SetupMeshComp(NeckMesh, TEXT("NeckMesh"), CylinderMesh, FVector(0.0f, 0.0f, 148.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 0.10f));
	SetupMeshComp(HeadMesh, TEXT("HeadMesh"), SphereMesh, FVector(0.0f, 0.0f, 164.0f), FRotator::ZeroRotator, FVector(0.20f, 0.18f, 0.22f));

	SetupMeshComp(HelmetMesh, TEXT("HelmetMesh"), SphereMesh, FVector(0.0f, 0.0f, 167.0f), FRotator::ZeroRotator, FVector(0.23f, 0.21f, 0.20f));
	SetupMeshComp(NVGMesh, TEXT("NVGMesh"), CubeMesh, FVector(12.0f, 0.0f, 166.0f), FRotator::ZeroRotator, FVector(0.12f, 0.16f, 0.06f));
	SetupMeshComp(BalaclavaMesh, TEXT("BalaclavaMesh"), CylinderMesh, FVector(1.0f, 0.0f, 161.0f), FRotator::ZeroRotator, FVector(0.20f, 0.18f, 0.12f));
	SetupMeshComp(CapMesh, TEXT("CapMesh"), SphereMesh, FVector(0.0f, 0.0f, 168.0f), FRotator::ZeroRotator, FVector(0.21f, 0.20f, 0.12f));

	SetupMeshComp(RightClavicleMesh, TEXT("RightClavicleMesh"), CylinderMesh, FVector(-2.0f, 16.0f, 142.0f), FRotator(0.0f, 0.0f, 30.0f), FVector(0.10f, 0.10f, 0.12f));
	SetupMeshComp(RightUpperArmMesh, TEXT("RightUpperArmMesh"), CylinderMesh, FVector(6.0f, 20.0f, 136.0f), FRotator(-35.0f, 15.0f, 0.0f), FVector(0.12f, 0.12f, 0.28f));
	SetupMeshComp(RightForearmMesh, TEXT("RightForearmMesh"), CylinderMesh, FVector(26.0f, 16.0f, 137.0f), FRotator(15.0f, -25.0f, 0.0f), FVector(0.11f, 0.11f, 0.26f));
	SetupMeshComp(RightHandMesh, TEXT("RightHandMesh"), SphereMesh, FVector(44.0f, 11.0f, 137.0f), FRotator::ZeroRotator, FVector(0.09f, 0.08f, 0.09f));

	SetupMeshComp(LeftClavicleMesh, TEXT("LeftClavicleMesh"), CylinderMesh, FVector(-2.0f, -16.0f, 142.0f), FRotator(0.0f, 0.0f, -30.0f), FVector(0.10f, 0.10f, 0.12f));
	SetupMeshComp(LeftUpperArmMesh, TEXT("LeftUpperArmMesh"), CylinderMesh, FVector(14.0f, -14.0f, 134.0f), FRotator(-20.0f, -35.0f, 0.0f), FVector(0.12f, 0.12f, 0.28f));
	SetupMeshComp(LeftForearmMesh, TEXT("LeftForearmMesh"), CylinderMesh, FVector(40.0f, -4.0f, 138.0f), FRotator(15.0f, 45.0f, 0.0f), FVector(0.11f, 0.11f, 0.32f));
	SetupMeshComp(LeftHandMesh, TEXT("LeftHandMesh"), SphereMesh, FVector(62.0f, 5.0f, 140.0f), FRotator::ZeroRotator, FVector(0.09f, 0.08f, 0.09f));

	SetupMeshComp(LeftThighMesh, TEXT("LeftThighMesh"), CylinderMesh, FVector(0.0f, -11.0f, 74.0f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.42f));
	SetupMeshComp(LeftCalfMesh, TEXT("LeftCalfMesh"), CylinderMesh, FVector(0.0f, -11.0f, 32.0f), FRotator::ZeroRotator, FVector(0.14f, 0.14f, 0.42f));
	SetupMeshComp(LeftBootMesh, TEXT("LeftBootMesh"), CubeMesh, FVector(4.0f, -11.0f, 5.0f), FRotator::ZeroRotator, FVector(0.24f, 0.12f, 0.10f));

	SetupMeshComp(RightThighMesh, TEXT("RightThighMesh"), CylinderMesh, FVector(0.0f, 11.0f, 74.0f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.42f));
	SetupMeshComp(RightCalfMesh, TEXT("RightCalfMesh"), CylinderMesh, FVector(0.0f, 11.0f, 32.0f), FRotator::ZeroRotator, FVector(0.14f, 0.14f, 0.42f));
	SetupMeshComp(RightBootMesh, TEXT("RightBootMesh"), CubeMesh, FVector(4.0f, 11.0f, 5.0f), FRotator::ZeroRotator, FVector(0.24f, 0.12f, 0.10f));

	// 8. Tactical Weapon Subsystem (Fallback)
	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	WeaponRoot->SetupAttachment(TurntableRoot);
	WeaponRoot->SetRelativeLocation(BaseWeaponLoc);
	WeaponRoot->SetRelativeRotation(BaseWeaponRot);

	auto SetupWeaponPart = [this, BaseMat](TObjectPtr<UStaticMeshComponent>& Comp, const TCHAR* Name, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(WeaponRoot);
		if (Mesh) Comp->SetStaticMesh(Mesh);
		if (BaseMat) Comp->SetMaterial(0, BaseMat);
		Comp->SetRelativeLocation(Loc);
		Comp->SetRelativeRotation(Rot);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(true);
	};

	SetupWeaponPart(WeaponReceiverMesh, TEXT("WeaponReceiverMesh"), CubeMesh, FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, FVector(0.26f, 0.06f, 0.09f));
	SetupWeaponPart(WeaponBarrelMesh, TEXT("WeaponBarrelMesh"), CylinderMesh, FVector(24.0f, 0.0f, 1.0f), FRotator(0.0f, 90.0f, 0.0f), FVector(0.03f, 0.03f, 0.36f));
	SetupWeaponPart(WeaponOpticMesh, TEXT("WeaponOpticMesh"), CubeMesh, FVector(-2.0f, 0.0f, 7.0f), FRotator::ZeroRotator, FVector(0.09f, 0.05f, 0.05f));
	SetupWeaponPart(WeaponMagMesh, TEXT("WeaponMagMesh"), CubeMesh, FVector(3.0f, 0.0f, -9.0f), FRotator(0.0f, 12.0f, 0.0f), FVector(0.08f, 0.04f, 0.18f));
	SetupWeaponPart(WeaponStockMesh, TEXT("WeaponStockMesh"), CubeMesh, FVector(-18.0f, 0.0f, -1.0f), FRotator::ZeroRotator, FVector(0.16f, 0.05f, 0.08f));

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(TurntableRoot);
	WeaponMesh->SetVisibility(false);
}

void AFPSCharacterPreviewActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[PREVIEW] AFPSCharacterPreviewActor::BeginPlay() running. PreviewCamera: %s at %s"),
		PreviewCamera ? TEXT("VALID") : TEXT("NULL"),
		PreviewCamera ? *PreviewCamera->GetRelativeLocation().ToString() : TEXT("NONE"));

	// Ensure OperatorMesh is loaded and visible
	if (OperatorMesh)
	{
		if (!OperatorMesh->GetStaticMesh())
		{
			if (UStaticMesh* LoadedPosed = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SM_Soldier_Posed.SM_Soldier_Posed")))
			{
				OperatorMesh->SetStaticMesh(LoadedPosed);
			}
		}

		OperatorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		OperatorMesh->SetRelativeRotation(FRotator(0.0f, 15.0f, 0.0f));
		OperatorMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		OperatorMesh->SetVisibility(true, true);
		OperatorMesh->SetHiddenInGame(false, true);

		if (UMaterialInterface* BodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
			OperatorMesh->SetMaterial(0, BodyMat);
		if (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
			OperatorMesh->SetMaterial(1, VisorMat);
		if (UMaterialInterface* AKSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel")))
			OperatorMesh->SetMaterial(2, AKSteel);
		if (UMaterialInterface* AKWood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood")))
			OperatorMesh->SetMaterial(3, AKWood);
		if (UMaterialInterface* AKBake = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite")))
			OperatorMesh->SetMaterial(4, AKBake);
	}

	// Ensure PedestalMesh is loaded and visible
	if (PedestalMesh)
	{
		if (!PedestalMesh->GetStaticMesh())
		{
			if (UStaticMesh* LoadedPlinth = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth")))
			{
				PedestalMesh->SetStaticMesh(LoadedPlinth);
			}
		}

		PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		PedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
		PedestalMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		PedestalMesh->SetVisibility(true, true);
		PedestalMesh->SetHiddenInGame(false, true);

		if (UMaterialInterface* AKSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel")))
			PedestalMesh->SetMaterial(0, AKSteel);
		if (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
			PedestalMesh->SetMaterial(1, VisorMat);
	}

	// Hide all legacy primitive boxes and inactive BodyMesh
	auto HideComp = [](UPrimitiveComponent* Comp)
	{
		if (Comp)
		{
			Comp->SetVisibility(false);
			Comp->SetHiddenInGame(true);
		}
	};
	HideComp(BodyMesh);
	HideComp(AK47Mesh);
	HideComp(PelvisMesh);
	HideComp(SpineLowerMesh);
	HideComp(SpineUpperMesh);
	HideComp(ChestMesh);
	HideComp(NeckMesh);
	HideComp(HeadMesh);
	HideComp(RightClavicleMesh);
	HideComp(RightUpperArmMesh);
	HideComp(RightForearmMesh);
	HideComp(RightHandMesh);
	HideComp(LeftClavicleMesh);
	HideComp(LeftUpperArmMesh);
	HideComp(LeftForearmMesh);
	HideComp(LeftHandMesh);
	HideComp(LeftThighMesh);
	HideComp(LeftCalfMesh);
	HideComp(RightThighMesh);
	HideComp(RightCalfMesh);
	HideComp(LeftBootMesh);
	HideComp(RightBootMesh);
	HideComp(VestMesh);
	HideComp(HelmetMesh);
	HideComp(NVGMesh);
	HideComp(BalaclavaMesh);
	HideComp(CapMesh);
	HideComp(WeaponReceiverMesh);
	HideComp(WeaponBarrelMesh);
	HideComp(WeaponOpticMesh);
	HideComp(WeaponMagMesh);
	HideComp(WeaponStockMesh);

	UE_LOG(LogTemp, Warning, TEXT("[PREVIEW_DIAG] OperatorMesh valid=%d, Asset=%s, Vis=%d, Hidden=%d, Loc=%s, Pedestal=%s"),
		OperatorMesh != nullptr,
		(OperatorMesh && OperatorMesh->GetStaticMesh()) ? *OperatorMesh->GetStaticMesh()->GetName() : TEXT("NULL"),
		OperatorMesh ? OperatorMesh->IsVisible() : 0,
		OperatorMesh ? OperatorMesh->bHiddenInGame : 0,
		OperatorMesh ? *OperatorMesh->GetComponentLocation().ToString() : TEXT("NONE"),
		(PedestalMesh && PedestalMesh->GetStaticMesh()) ? *PedestalMesh->GetStaticMesh()->GetName() : TEXT("NULL"));

	// Sync persistent equipped loadout from UFPSEconomySubsystem if available
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			CurrentCamo = static_cast<EOperatorCamo>(FMath::Clamp(Economy->GetEquippedCamoIndex(), 0, 3));
			CurrentHeadgear = static_cast<EOperatorHeadgear>(FMath::Clamp(Economy->GetEquippedHeadgearIndex(), 0, 3));
			CurrentVest = static_cast<EOperatorArmorVest>(FMath::Clamp(Economy->GetEquippedArmorIndex(), 0, 2));
			CurrentWeaponFinish = static_cast<EOperatorWeaponFinish>(FMath::Clamp(Economy->GetEquippedWeaponFinishIndex(), 0, 3));
		}
	}

	// The high-fidelity SK_BaseMale model contains full integrated tactical armor, helmet, visor, and boots.
	// OperatorGearComponent procedural box rig is disabled to keep the clean textured mesh.

	ApplyCamoColors();
	ApplyHeadgearVisibility();
	ApplyVestConfiguration();
	ApplyWeaponFinish();
	SetPreviewStance(CurrentStanceIndex);
}

void AFPSCharacterPreviewActor::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult)
{
	if (PreviewCamera)
	{
		PreviewCamera->GetCameraView(DeltaTime, OutResult);
	}
	else
	{
		Super::CalcCamera(DeltaTime, OutResult);
	}

	static bool bLoggedCameraOnce = false;
	if (!bLoggedCameraOnce)
	{
		bLoggedCameraOnce = true;
		UE_LOG(LogTemp, Warning, TEXT("[CALC_CAMERA] PreviewCamera active! POV Loc=%s, Rot=%s, FOV=%.1f"),
			*OutResult.Location.ToString(), *OutResult.Rotation.ToString(), OutResult.FOV);
	}
}

void AFPSCharacterPreviewActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Turntable 360-degree continuous rotation
	if (bIsRotating && TurntableRoot)
	{
		TurntableRoot->AddLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	}

	// 2. Tactical weapon socket stays locked to animated weapon_r bone

	// 3. Fallback procedural idle breathing
	UpdateAimingPose(DeltaTime);
}

void AFPSCharacterPreviewActor::UpdateAimingPose(float DeltaTime)
{
	AimBreathTimer += DeltaTime;
}

void AFPSCharacterPreviewActor::SetCamo(EOperatorCamo NewCamo)
{
	CurrentCamo = NewCamo;
	ApplyCamoColors();
}

void AFPSCharacterPreviewActor::SetHeadgear(EOperatorHeadgear NewHeadgear)
{
	CurrentHeadgear = NewHeadgear;
	ApplyHeadgearVisibility();
}

void AFPSCharacterPreviewActor::SetArmorVest(EOperatorArmorVest NewVest)
{
	CurrentVest = NewVest;
	ApplyVestConfiguration();
}

void AFPSCharacterPreviewActor::SetWeaponFinish(EOperatorWeaponFinish NewFinish)
{
	CurrentWeaponFinish = NewFinish;
	ApplyWeaponFinish();
}

void AFPSCharacterPreviewActor::SetPreviewStance(int32 StanceIndex)
{
	// 0 = Low-Ready Carbine (Default matching menu-prop.jpeg), 1 = High-Ready ADS, 2 = Patrol Walk
	CurrentStanceIndex = FMath::Clamp(StanceIndex, 0, 2);
	bIsAiming = (CurrentStanceIndex == 1);

	if (BodyMesh)
	{
		if (UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(BodyMesh->GetAnimInstance()))
		{
			FPSAnim->bIsMenuPreview = true;
			FPSAnim->MenuPoseIndex = CurrentStanceIndex;
		}
	}

	if (OperatorGearComponent)
	{
		OperatorGearComponent->UpdateGearTransforms();
	}
}

void AFPSCharacterPreviewActor::SetAimingStance(bool bInAiming)
{
	SetPreviewStance((CurrentStanceIndex + 1) % 3);
}

void AFPSCharacterPreviewActor::ResetFacing()
{
	if (TurntableRoot)
	{
		TurntableRoot->SetRelativeRotation(FRotator::ZeroRotator);
	}
	if (OperatorGearComponent)
	{
		OperatorGearComponent->UpdateGearTransforms();
	}
}

void AFPSCharacterPreviewActor::ApplyCamoColors()
{
	if (OperatorMesh)
	{
		UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(0));
		if (!BodyMID)
		{
			BodyMID = OperatorMesh->CreateDynamicMaterialInstance(0);
		}
		if (BodyMID)
		{
			// Clean tactical tints for Vanguard Spec-Ops Soldier
			FLinearColor CamoTint = FLinearColor(0.28f, 0.32f, 0.38f, 1.0f); // UrbanDigital (Task Force Echo Dark Slate / Default)
			if (CurrentCamo == EOperatorCamo::WoodlandTactical)
			{
				CamoTint = FLinearColor(0.85f, 1.05f, 0.85f, 1.0f); // Tactical Woodland Olive
			}
			else if (CurrentCamo == EOperatorCamo::DesertSpecOps)
			{
				CamoTint = FLinearColor(1.15f, 1.05f, 0.90f, 1.0f); // Desert Sand SpecOps
			}
			else if (CurrentCamo == EOperatorCamo::StealthBlackout)
			{
				CamoTint = FLinearColor(0.70f, 0.72f, 0.76f, 1.0f); // Stealth Blackout Dark Slate
			}
			BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoTint);
			BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.45f);
		}

		if (UMaterialInstanceDynamic* VisorMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(1)))
		{
			VisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.01f, 0.08f, 0.12f));
			VisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.898f, 1.0f));
			VisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 1.6f);
		}
		else if (UMaterialInstanceDynamic* NewVisorMID = OperatorMesh->CreateDynamicMaterialInstance(1))
		{
			NewVisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.01f, 0.08f, 0.12f));
			NewVisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.898f, 1.0f));
			NewVisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 1.6f);
		}
	}
}

void AFPSCharacterPreviewActor::ApplyHeadgearVisibility()
{
	// Integrated into master soldier mesh
}

void AFPSCharacterPreviewActor::ApplyVestConfiguration()
{
	// Integrated into master soldier mesh
}

void AFPSCharacterPreviewActor::ApplyWeaponFinish()
{
	if (OperatorMesh)
	{
		UMaterialInstanceDynamic* SteelMID = Cast<UMaterialInstanceDynamic>(OperatorMesh->GetMaterial(2));
		if (!SteelMID)
		{
			SteelMID = OperatorMesh->CreateDynamicMaterialInstance(2);
		}
		if (SteelMID)
		{
			FLinearColor SteelTint(0.12f, 0.13f, 0.15f, 1.0f);
			if (CurrentWeaponFinish == EOperatorWeaponFinish::DesertTanCerakote)
			{
				SteelTint = FLinearColor(0.65f, 0.55f, 0.40f, 1.0f);
			}
			else if (CurrentWeaponFinish == EOperatorWeaponFinish::CarbonFiberSpecOps)
			{
				SteelTint = FLinearColor(0.04f, 0.04f, 0.05f, 1.0f);
			}
			else if (CurrentWeaponFinish == EOperatorWeaponFinish::GoldOpalDamascene)
			{
				SteelTint = FLinearColor(0.08f, 0.10f, 0.12f, 1.0f);
			}
			SteelMID->SetVectorParameterValue(TEXT("BaseColor"), SteelTint);
		}
	}
}
