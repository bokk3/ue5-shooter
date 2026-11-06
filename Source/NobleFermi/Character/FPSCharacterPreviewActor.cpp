#include "Character/FPSCharacterPreviewActor.h"
#include "Character/FPSOperatorGearComponent.h"
#include "Character/FPSBeltGearComponent.h"
#include "Character/FPSBreathVaporComponent.h"
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
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/GameInstance.h"
#include "Economy/FPSEconomySubsystem.h"

AFPSCharacterPreviewActor::AFPSCharacterPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	// 1. Root Scene
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootScene->SetMobility(EComponentMobility::Movable);
	RootComponent = RootScene;

	// 2. Camera: Calibrated to frame the full Task Force Echo Operator, Turntable & Hangar Armory Table
	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(RootComponent);
	PreviewCamera->SetRelativeLocation(FVector(435.0f, 0.0f, 96.0f));
	PreviewCamera->SetRelativeRotation(FRotator(-1.0f, 180.0f, 0.0f));
	PreviewCamera->FieldOfView = 52.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	PreviewCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	PreviewCamera->PostProcessSettings.AutoExposureBias = -0.35f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	PreviewCamera->PostProcessSettings.AutoExposureMinBrightness = 0.40f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	PreviewCamera->PostProcessSettings.AutoExposureMaxBrightness = 2.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureSpeedUp = true;
	PreviewCamera->PostProcessSettings.AutoExposureSpeedUp = 20.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureSpeedDown = true;
	PreviewCamera->PostProcessSettings.AutoExposureSpeedDown = 20.0f;
	PreviewCamera->PostProcessSettings.bOverride_BloomIntensity = true;
	PreviewCamera->PostProcessSettings.BloomIntensity = 0.25f;

	// 3. Studio Lighting: 3-point tactical dynamic lighting
	StudioKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioKeyLight"));
	StudioKeyLight->SetupAttachment(RootComponent);
	StudioKeyLight->SetMobility(EComponentMobility::Movable);
	StudioKeyLight->SetRelativeLocation(FVector(190.0f, -120.0f, 210.0f));
	StudioKeyLight->SetRelativeRotation(FRotator(-35.0f, 145.0f, 0.0f));
	StudioKeyLight->SetIntensity(1800.0f);
	StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.40f)); // Warm amber key light
	StudioKeyLight->SetAttenuationRadius(1500.0f);
	StudioKeyLight->SetInnerConeAngle(25.0f);
	StudioKeyLight->SetOuterConeAngle(65.0f);

	StudioRimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioRimLight"));
	StudioRimLight->SetupAttachment(RootComponent);
	StudioRimLight->SetMobility(EComponentMobility::Movable);
	StudioRimLight->SetRelativeLocation(FVector(-160.0f, 120.0f, 175.0f));
	StudioRimLight->SetRelativeRotation(FRotator(-25.0f, -35.0f, 0.0f));
	StudioRimLight->SetIntensity(2200.0f);
	StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical cyan rim light (#00E5FF)
	StudioRimLight->SetAttenuationRadius(1400.0f);
	StudioRimLight->SetInnerConeAngle(30.0f);
	StudioRimLight->SetOuterConeAngle(70.0f);
	StudioRimLight->SetCastShadows(false);

	StudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
	StudioFillLight->SetupAttachment(RootComponent);
	StudioFillLight->SetMobility(EComponentMobility::Movable);
	StudioFillLight->SetRelativeLocation(FVector(150.0f, 0.0f, 120.0f));
	StudioFillLight->SetIntensity(1100.0f);
	StudioFillLight->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f)); // Clean fill
	StudioFillLight->SetAttenuationRadius(1500.0f);
	StudioFillLight->SetCastShadows(false);

	StudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
	StudioAmbientLight->SetupAttachment(RootComponent);
	StudioAmbientLight->SetMobility(EComponentMobility::Movable);
	StudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	StudioAmbientLight->SetIntensity(500.0f);
	StudioAmbientLight->SetLightColor(FLinearColor(0.35f, 0.45f, 0.55f));
	StudioAmbientLight->SetAttenuationRadius(1500.0f);
	StudioAmbientLight->SetCastShadows(false);

	StudioSunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("StudioSunLight"));
	StudioSunLight->SetupAttachment(RootComponent);
	StudioSunLight->SetMobility(EComponentMobility::Movable);
	StudioSunLight->SetRelativeRotation(FRotator(-30.0f, 165.0f, 0.0f));
	StudioSunLight->SetIntensity(0.0f);
	StudioSunLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.88f));
	StudioSunLight->SetCastShadows(false);

	// 4. Turntable Root (Rotates in Tick)
	TurntableRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TurntableRoot"));
	TurntableRoot->SetupAttachment(RootComponent);
	TurntableRoot->SetRelativeLocation(FVector::ZeroVector);

	// 5. Basic Shape & Operator Resources
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

	// 5. Tactical Turntable Pedestal (Radius 135cm, Height 11cm with beveled rim)
	PedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PedestalMesh"));
	PedestalMesh->SetupAttachment(TurntableRoot);
	PedestalMesh->SetMobility(EComponentMobility::Movable);
	PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -11.0f));
	PedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
	PedestalMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	PedestalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PedestalMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlinthMeshFinder(TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth"));
	if (PlinthMeshFinder.Succeeded())
	{
		PedestalMesh->SetStaticMesh(PlinthMeshFinder.Object);
	}
	else if (CylinderMesh)
	{
		PedestalMesh->SetStaticMesh(CylinderMesh);
		PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));
		PedestalMesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.04f));
	}

	// 5.5 Static Posed Operator Fallback (kept hidden when live skinned BodyMesh is active)
	OperatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OperatorMesh"));
	OperatorMesh->SetupAttachment(TurntableRoot);
	OperatorMesh->SetMobility(EComponentMobility::Movable);
	OperatorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	OperatorMesh->SetRelativeRotation(FRotator(0.0f, 15.0f, 0.0f));
	OperatorMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	OperatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OperatorMesh->SetCastShadow(true);
	OperatorMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PosedSoldierFinder(TEXT("/Game/Characters/BaseMale/SM_Soldier_Posed.SM_Soldier_Posed"));
	if (PosedSoldierFinder.Succeeded())
	{
		OperatorMesh->SetStaticMesh(PosedSoldierFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkelSoldierFinder(TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArmorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_Armor.M_Operator_Armor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VisorMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKSteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));

	if (BaseBodyMatFinder.Succeeded()) OperatorMesh->SetMaterial(0, BaseBodyMatFinder.Object);
	if (VisorMatFinder.Succeeded()) OperatorMesh->SetMaterial(1, VisorMatFinder.Object);

	if (AKSteelMatFinder.Succeeded()) PedestalMesh->SetMaterial(0, AKSteelMatFinder.Object);
	if (VisorMatFinder.Succeeded()) PedestalMesh->SetMaterial(1, VisorMatFinder.Object);

	// 6. Live Animated Skeletal Operator (SK_BaseMale driven by UFPSAnimInstance)
	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(TurntableRoot);
	BodyMesh->SetMobility(EComponentMobility::Movable);
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	BodyMesh->SetRelativeRotation(FRotator(0.0f, 105.0f, 0.0f)); // Faces +X camera with heroic 15-deg turn
	BodyMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->bAlwaysCreatePhysicsState = false;
	BodyMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	BodyMesh->SetCastShadow(true);
	BodyMesh->SetVisibility(true);

	if (SkelSoldierFinder.Succeeded())
	{
		BodyMesh->SetSkeletalMesh(SkelSoldierFinder.Object);
	}
	if (BaseBodyMatFinder.Succeeded())
	{
		BodyMesh->SetMaterial(0, BaseBodyMatFinder.Object);
	}
	if (VisorMatFinder.Succeeded())
	{
		BodyMesh->SetMaterial(1, VisorMatFinder.Object);
	}
	BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	BodyMesh->SetAnimInstanceClass(UFPSAnimInstance::StaticClass());

	// 6.2 Modular 3D Belt Gear Kit (STANAG Fast-Mag Pouches, M67 Frag, M84 Flashbang)
	BeltGearComponent = CreateDefaultSubobject<UFPSBeltGearComponent>(TEXT("BeltGearComponent"));
	BreathVaporComponent = CreateDefaultSubobject<UFPSBreathVaporComponent>(TEXT("BreathVaporComponent"));
	OperatorGearComponent = CreateDefaultSubobject<UFPSOperatorGearComponent>(TEXT("OperatorGearComponent"));

	// 6.5 Hangar Armory Weapon Table & Loadout Rifle Display (Screen Left next to Turntable)
	TableWeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TableWeaponRoot"));
	TableWeaponRoot->SetupAttachment(RootComponent);
	TableWeaponRoot->SetMobility(EComponentMobility::Movable);
	TableWeaponRoot->SetRelativeLocation(FVector(25.0f, 82.0f, 0.0f));
	TableWeaponRoot->SetRelativeRotation(FRotator(0.0f, -24.0f, 0.0f));

	TablePedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TablePedestalMesh"));
	TablePedestalMesh->SetupAttachment(TableWeaponRoot);
	TablePedestalMesh->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) TablePedestalMesh->SetStaticMesh(CubeMesh);
	if (AKSteelMatFinder.Succeeded()) TablePedestalMesh->SetMaterial(0, AKSteelMatFinder.Object);
	TablePedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 24.0f));
	TablePedestalMesh->SetRelativeScale3D(FVector(0.42f, 0.78f, 0.48f));
	TablePedestalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TablePedestalMesh->SetCastShadow(true);

	TableRackStandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableRackStandMesh"));
	TableRackStandMesh->SetupAttachment(TableWeaponRoot);
	TableRackStandMesh->SetMobility(EComponentMobility::Movable);
	if (CubeMesh) TableRackStandMesh->SetStaticMesh(CubeMesh);
	if (AKSteelMatFinder.Succeeded()) TableRackStandMesh->SetMaterial(0, AKSteelMatFinder.Object);
	TableRackStandMesh->SetRelativeLocation(FVector(2.0f, 0.0f, 50.0f));
	TableRackStandMesh->SetRelativeScale3D(FVector(0.14f, 0.44f, 0.04f));
	TableRackStandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TableRackStandMesh->SetCastShadow(true);

	TableRifleReceiverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableRifleReceiverMesh"));
	TableRifleReceiverMesh->SetupAttachment(TableWeaponRoot);
	TableRifleReceiverMesh->SetMobility(EComponentMobility::Movable);
	TableRifleReceiverMesh->SetRelativeLocation(FVector(4.0f, 0.0f, 60.0f));
	TableRifleReceiverMesh->SetRelativeRotation(FRotator(6.0f, -90.0f, -16.0f));
	TableRifleReceiverMesh->SetRelativeScale3D(FVector(0.88f, 0.88f, 0.88f));
	TableRifleReceiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TableRifleReceiverMesh->SetCastShadow(true);

	TableRifleMagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableRifleMagMesh"));
	TableRifleMagMesh->SetupAttachment(TableRifleReceiverMesh);
	TableRifleMagMesh->SetMobility(EComponentMobility::Movable);
	TableRifleMagMesh->SetRelativeLocation(FVector(6.0f, 0.0f, -1.5f));
	TableRifleMagMesh->SetRelativeRotation(FRotator::ZeroRotator);
	TableRifleMagMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	TableRifleMagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TableRifleMagMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultRecFinder(TEXT("/Game/Weapons/Meshes/SM_W01_NF98_Receiver.SM_W01_NF98_Receiver"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMagFinder(TEXT("/Game/Weapons/Meshes/SM_W01_NF98_Mag.SM_W01_NF98_Mag"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WeaponGunmetalFinder(TEXT("/Game/Weapons/Materials/MI_Weapon_Gunmetal.MI_Weapon_Gunmetal"));
	if (DefaultRecFinder.Succeeded())
	{
		TableRifleReceiverMesh->SetStaticMesh(DefaultRecFinder.Object);
	}
	if (DefaultMagFinder.Succeeded())
	{
		TableRifleMagMesh->SetStaticMesh(DefaultMagFinder.Object);
	}
	if (WeaponGunmetalFinder.Succeeded())
	{
		TableRifleReceiverMesh->SetMaterial(0, WeaponGunmetalFinder.Object);
		TableRifleMagMesh->SetMaterial(0, WeaponGunmetalFinder.Object);
	}

	TableWeaponKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("TableWeaponKeyLight"));
	TableWeaponKeyLight->SetupAttachment(TableWeaponRoot);
	TableWeaponKeyLight->SetMobility(EComponentMobility::Movable);
	TableWeaponKeyLight->SetRelativeLocation(FVector(65.0f, -25.0f, 145.0f));
	TableWeaponKeyLight->SetRelativeRotation(FRotator(-42.0f, 165.0f, 0.0f));
	TableWeaponKeyLight->SetIntensity(1400.0f);
	TableWeaponKeyLight->SetLightColor(FLinearColor(0.85f, 0.94f, 1.0f));
	TableWeaponKeyLight->SetAttenuationRadius(320.0f);
	TableWeaponKeyLight->SetInnerConeAngle(22.0f);
	TableWeaponKeyLight->SetOuterConeAngle(48.0f);
	TableWeaponKeyLight->SetCastShadows(false);

	// Hand-held rifle meshes (shown when user clicks HIGH ADS in Operator Customization)
	HeldRifleReceiverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldRifleReceiverMesh"));
	HeldRifleReceiverMesh->SetupAttachment(TurntableRoot);
	HeldRifleReceiverMesh->SetMobility(EComponentMobility::Movable);
	HeldRifleReceiverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldRifleReceiverMesh->SetCastShadow(true);
	HeldRifleReceiverMesh->SetVisibility(false);

	HeldRifleMagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldRifleMagMesh"));
	HeldRifleMagMesh->SetupAttachment(HeldRifleReceiverMesh);
	HeldRifleMagMesh->SetMobility(EComponentMobility::Movable);
	HeldRifleMagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldRifleMagMesh->SetCastShadow(true);
	HeldRifleMagMesh->SetVisibility(false);

	if (DefaultRecFinder.Succeeded()) HeldRifleReceiverMesh->SetStaticMesh(DefaultRecFinder.Object);
	if (DefaultMagFinder.Succeeded()) HeldRifleMagMesh->SetStaticMesh(DefaultMagFinder.Object);
	if (WeaponGunmetalFinder.Succeeded())
	{
		HeldRifleReceiverMesh->SetMaterial(0, WeaponGunmetalFinder.Object);
		HeldRifleMagMesh->SetMaterial(0, WeaponGunmetalFinder.Object);
	}

	AK47Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AK47Mesh"));
	AK47Mesh->SetupAttachment(TurntableRoot);
	AK47Mesh->SetVisibility(false);

	// 7. Human Male Anatomical Skeleton (180 cm anthropometry fallback, kept hidden)
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

	// 1. Activate Live Skinned BodyMesh (`SK_BaseMale` driven by `UFPSAnimInstance`)
	bool bUsingLiveSkinnedBody = false;
	if (BodyMesh)
	{
		if (!BodyMesh->GetSkeletalMeshAsset())
		{
			if (USkeletalMesh* LoadedSkel = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale")))
			{
				BodyMesh->SetSkeletalMesh(LoadedSkel);
			}
		}

		if (BodyMesh->GetSkeletalMeshAsset())
		{
			bUsingLiveSkinnedBody = true;
			BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
			BodyMesh->SetRelativeRotation(FRotator(0.0f, 105.0f, 0.0f));
			BodyMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			BodyMesh->SetVisibility(true, false);
			BodyMesh->SetHiddenInGame(false, false);

			if (UMaterialInterface* BodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
			{
				BodyMesh->SetMaterial(0, BodyMat);
			}
			if (UMaterialInterface* VisorMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Soldier_Visor.M_Soldier_Visor")))
			{
				BodyMesh->SetMaterial(1, VisorMat);
			}

			BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			BodyMesh->SetAnimInstanceClass(UFPSAnimInstance::StaticClass());
			BodyMesh->InitAnim(true);

			// Initialize modular 3D belt gear on the Hangar operator (`BeltMagComp_1`, `BeltMagComp_2`, `BeltFragComp`, `BeltFlashComp`)
			if (BeltGearComponent)
			{
				BeltGearComponent->InitializeBeltKit(BodyMesh);
			}
			if (BreathVaporComponent)
			{
				BreathVaporComponent->InitializeBreathVapor(BodyMesh);
			}
		}
	}

	// 2. Fallback to static OperatorMesh only if SK_BaseMale is unavailable
	if (OperatorMesh)
	{
		if (!bUsingLiveSkinnedBody)
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
		}
		else
		{
			OperatorMesh->SetVisibility(false, true);
			OperatorMesh->SetHiddenInGame(true, true);
		}
	}

	// 3. Ensure PedestalMesh uses low-profile SM_Hangar_Plinth at Z=-11cm (top deck flush at Z=0 under boots)
	if (PedestalMesh)
	{
		if (UStaticMesh* LoadedPlinth = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Hangar/SM_Hangar_Plinth.SM_Hangar_Plinth")))
		{
			PedestalMesh->SetStaticMesh(LoadedPlinth);
			PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -11.0f));
			PedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
			PedestalMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		}
		else
		{
			PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));
			PedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
			PedestalMesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.04f));
		}
		PedestalMesh->SetVisibility(true, true);
		PedestalMesh->SetHiddenInGame(false, true);

		if (UMaterialInterface* RimSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_Steel.M_Turntable_Steel")))
			PedestalMesh->SetMaterial(0, RimSteel);
		if (UMaterialInterface* CyanNeon = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_CyanNeon.M_Turntable_CyanNeon")))
			PedestalMesh->SetMaterial(1, CyanNeon);
		if (UMaterialInterface* GripDeck = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_Deck.M_Turntable_Deck")))
			PedestalMesh->SetMaterial(2, GripDeck);
	}

	// Style Hangar Armory Table pedestal & stand
	if (TablePedestalMesh)
	{
		if (UMaterialInterface* RimSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_Steel.M_Turntable_Steel")))
		{
			TablePedestalMesh->SetMaterial(0, RimSteel);
		}
	}
	if (TableRackStandMesh)
	{
		if (UMaterialInterface* RimSteel = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_Steel.M_Turntable_Steel")))
		{
			TableRackStandMesh->SetMaterial(0, RimSteel);
		}
	}

	// 4. Hide all legacy primitive boxes
	auto HideComp = [](UPrimitiveComponent* Comp)
	{
		if (Comp)
		{
			Comp->SetVisibility(false);
			Comp->SetHiddenInGame(true);
		}
	};
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

	// 5. Sync persistent equipped loadout from UFPSEconomySubsystem if available
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

	int32 CmdStance = CurrentStanceIndex;
	if (FParse::Value(FCommandLine::Get(), TEXT("PreviewStance="), CmdStance))
	{
		CurrentStanceIndex = FMath::Clamp(CmdStance, 0, 2);
	}

	SetEquippedWeaponArchetype(CurrentWeaponArchetypeIndex);
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
}

void AFPSCharacterPreviewActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Turntable 360-degree continuous rotation
	if (bIsRotating && TurntableRoot)
	{
		TurntableRoot->AddLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	}

	// 2. When in Stance 1 (High-Ready ADS preview), pin HeldRifleReceiverMesh between hand_r and hand_l
	if (BodyMesh && HeldRifleReceiverMesh && HeldRifleReceiverMesh->IsVisible())
	{
		const FVector HandRPos = BodyMesh->GetBoneLocation(FName(TEXT("hand_r")), EBoneSpaces::WorldSpace);
		const FVector HandLPos = BodyMesh->GetBoneLocation(FName(TEXT("hand_l")), EBoneSpaces::WorldSpace);
		const float HandDist = FVector::Dist(HandRPos, HandLPos);

		FVector BarrelDir;
		if (HandDist >= 15.0f && HandDist <= 65.0f)
		{
			BarrelDir = (HandLPos - HandRPos).GetSafeNormal();
		}
		else
		{
			BarrelDir = BodyMesh->GetComponentQuat().GetForwardVector();
		}

		if (BarrelDir.ContainsNaN() || BarrelDir.IsNearlyZero(1e-3f))
		{
			BarrelDir = BodyMesh->GetComponentQuat().GetForwardVector();
		}

		const FVector UpVec = FVector::UpVector;
		FVector RightVec = FVector::CrossProduct(UpVec, BarrelDir).GetSafeNormal();
		if (RightVec.ContainsNaN() || RightVec.IsNearlyZero(1e-3f))
		{
			RightVec = BodyMesh->GetComponentQuat().GetRightVector();
		}

		const FVector OrthoUp = FVector::CrossProduct(BarrelDir, RightVec).GetSafeNormal();
		const FQuat RifleQuat = FRotationMatrix::MakeFromXZ(BarrelDir, OrthoUp).ToQuat();

		if (!RifleQuat.ContainsNaN() && !HandRPos.ContainsNaN())
		{
			// Offset by grip socket location so the operator's right hand holds the actual pistol grip
			const FVector GripOffset(10.56f, 0.0f, 7.04f);
			HeldRifleReceiverMesh->SetWorldLocationAndRotation(HandRPos + RifleQuat.RotateVector(GripOffset), RifleQuat);
		}
	}

	// 3. Subtle hover/presentation animation on the Hangar Table Rifle
	UpdateAimingPose(DeltaTime);
	if (TableRifleReceiverMesh && TableRifleReceiverMesh->IsVisible())
	{
		const float HoverZ = 60.0f + FMath::Sin(AimBreathTimer * 1.6f) * 0.4f;
		TableRifleReceiverMesh->SetRelativeLocation(FVector(4.0f, 0.0f, HoverZ));
	}
}

void AFPSCharacterPreviewActor::UpdateAimingPose(float DeltaTime)
{
	AimBreathTimer += DeltaTime;
}

void AFPSCharacterPreviewActor::SetEquippedWeaponArchetype(int32 ArchetypeIndex)
{
	CurrentWeaponArchetypeIndex = FMath::Clamp(ArchetypeIndex, 0, 4);

	const TCHAR* ReceiverPaths[5] = {
		TEXT("/Game/Weapons/Meshes/SM_W01_NF98_Receiver.SM_W01_NF98_Receiver"),
		TEXT("/Game/Weapons/Meshes/SM_W06_Spectre45_Receiver.SM_W06_Spectre45_Receiver"),
		TEXT("/Game/Weapons/Meshes/SM_W02_VK74_Receiver.SM_W02_VK74_Receiver"),
		TEXT("/Game/Weapons/Meshes/SM_W03_Falcon50_Receiver.SM_W03_Falcon50_Receiver"),
		TEXT("/Game/Weapons/Meshes/SM_W04_Breacher12_Receiver.SM_W04_Breacher12_Receiver")
	};

	const TCHAR* MagPaths[5] = {
		TEXT("/Game/Weapons/Meshes/SM_W01_NF98_Mag.SM_W01_NF98_Mag"),
		TEXT("/Game/Weapons/Meshes/SM_W06_Spectre45_Mag.SM_W06_Spectre45_Mag"),
		TEXT("/Game/Weapons/Meshes/SM_W02_VK74_Mag.SM_W02_VK74_Mag"),
		TEXT("/Game/Weapons/Meshes/SM_W03_Falcon50_Mag.SM_W03_Falcon50_Mag"),
		TEXT("/Game/Weapons/Meshes/SM_W04_Breacher12_Mag.SM_W04_Breacher12_Mag")
	};

	const FVector MagOffsets[5] = {
		FVector(6.0f, 0.0f, -1.5f),
		FVector(4.5f, 0.0f, -1.2f),
		FVector(7.5f, 0.0f, -1.8f),
		FVector(6.5f, 0.0f, -1.6f),
		FVector(5.0f, 0.0f, -1.4f)
	};

	UStaticMesh* RecMesh = LoadObject<UStaticMesh>(nullptr, ReceiverPaths[CurrentWeaponArchetypeIndex]);
	UStaticMesh* MagMesh = LoadObject<UStaticMesh>(nullptr, MagPaths[CurrentWeaponArchetypeIndex]);

	if (TableRifleReceiverMesh && RecMesh)
	{
		TableRifleReceiverMesh->SetStaticMesh(RecMesh);
	}
	if (TableRifleMagMesh && MagMesh)
	{
		TableRifleMagMesh->SetStaticMesh(MagMesh);
		TableRifleMagMesh->SetRelativeLocation(MagOffsets[CurrentWeaponArchetypeIndex]);
	}
	if (HeldRifleReceiverMesh && RecMesh)
	{
		HeldRifleReceiverMesh->SetStaticMesh(RecMesh);
	}
	if (HeldRifleMagMesh && MagMesh)
	{
		HeldRifleMagMesh->SetStaticMesh(MagMesh);
		HeldRifleMagMesh->SetRelativeLocation(MagOffsets[CurrentWeaponArchetypeIndex]);
	}

	ApplyWeaponFinish();
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
	// 0 = Relaxed Tactical Idle (rifle displayed on Hangar table), 1 = High-Ready ADS, 2 = Patrol Walk
	CurrentStanceIndex = FMath::Clamp(StanceIndex, 0, 2);
	bIsAiming = (CurrentStanceIndex == 1);

	if (BodyMesh)
	{
		if (UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(BodyMesh->GetAnimInstance()))
		{
			FPSAnim->bIsMenuPreview = true;
			FPSAnim->MenuPoseIndex = CurrentStanceIndex;
			FPSAnim->bUnarmedRelaxedIdle = (CurrentStanceIndex != 1);
			FPSAnim->bForceHighReady = (CurrentStanceIndex == 1);
			FPSAnim->bIsADS = (CurrentStanceIndex == 1);
		}
	}

	// Show rifle in hands when previewing High-Ready ADS (Stance 1), or on the Hangar Armory Table in Stance 0 & 2
	const bool bShowHeldRifle = (CurrentStanceIndex == 1);
	if (HeldRifleReceiverMesh)
	{
		HeldRifleReceiverMesh->SetVisibility(bShowHeldRifle, true);
		HeldRifleReceiverMesh->SetHiddenInGame(!bShowHeldRifle, true);
	}
	if (TableRifleReceiverMesh)
	{
		TableRifleReceiverMesh->SetVisibility(!bShowHeldRifle, true);
		TableRifleReceiverMesh->SetHiddenInGame(bShowHeldRifle, true);
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
}

void AFPSCharacterPreviewActor::ApplyCamoColors()
{
	// Balanced natural tactical tints for Vanguard Spec-Ops Soldier (no overexposure)
	FLinearColor CamoTint = FLinearColor(0.55f, 0.58f, 0.64f, 1.0f); // UrbanDigital (Task Force Echo Slate)
	if (CurrentCamo == EOperatorCamo::WoodlandTactical)
	{
		CamoTint = FLinearColor(0.46f, 0.58f, 0.42f, 1.0f); // Tactical Woodland Olive
	}
	else if (CurrentCamo == EOperatorCamo::DesertSpecOps)
	{
		CamoTint = FLinearColor(0.74f, 0.66f, 0.52f, 1.0f); // Desert Sand SpecOps
	}
	else if (CurrentCamo == EOperatorCamo::StealthBlackout)
	{
		CamoTint = FLinearColor(0.24f, 0.26f, 0.30f, 1.0f); // Stealth Blackout Dark Obsidian
	}

	auto ConfigureMeshMaterials = [&](UMeshComponent* MeshComp)
	{
		if (!MeshComp) return;

		UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(0));
		if (!BodyMID)
		{
			BodyMID = MeshComp->CreateDynamicMaterialInstance(0);
		}
		if (BodyMID)
		{
			BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoTint);
			BodyMID->SetVectorParameterValue(TEXT("BaseColor"), CamoTint);
			BodyMID->SetScalarParameterValue(TEXT("AmbientFillScale"), 0.0f);
			BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.55f);
		}

		UMaterialInstanceDynamic* VisorMID = Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(1));
		if (!VisorMID)
		{
			VisorMID = MeshComp->CreateDynamicMaterialInstance(1);
		}
		if (VisorMID)
		{
			VisorMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.65f, 0.75f, 0.85f, 1.0f));
			VisorMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
			VisorMID->SetScalarParameterValue(TEXT("EmissiveIntensity"), 0.0f);
		}
	};

	ConfigureMeshMaterials(BodyMesh);
	ConfigureMeshMaterials(OperatorMesh);
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
	FLinearColor FinishColor(0.14f, 0.16f, 0.19f, 1.0f);
	float FinishRoughness = 0.35f;
	float FinishMetallic = 0.82f;

	if (CurrentWeaponFinish == EOperatorWeaponFinish::DesertTanCerakote)
	{
		FinishColor = FLinearColor(0.62f, 0.52f, 0.38f, 1.0f);
		FinishRoughness = 0.52f;
		FinishMetallic = 0.25f;
	}
	else if (CurrentWeaponFinish == EOperatorWeaponFinish::CarbonFiberSpecOps)
	{
		FinishColor = FLinearColor(0.05f, 0.06f, 0.07f, 1.0f);
		FinishRoughness = 0.28f;
		FinishMetallic = 0.45f;
	}
	else if (CurrentWeaponFinish == EOperatorWeaponFinish::GoldOpalDamascene)
	{
		FinishColor = FLinearColor(0.85f, 0.64f, 0.18f, 1.0f);
		FinishRoughness = 0.18f;
		FinishMetallic = 0.94f;
	}

	auto ApplyToWeaponMesh = [&](UStaticMeshComponent* WeapComp, const FLinearColor& ColorOverride)
	{
		if (!WeapComp) return;
		UMaterialInterface* WeaponMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weapons/Materials/MI_Weapon_Gunmetal.MI_Weapon_Gunmetal"));
		if (!WeaponMat)
		{
			WeaponMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Belt_Gear_PBR.M_Belt_Gear_PBR"));
		}
		if (!WeaponMat)
		{
			WeaponMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Hangar/Materials/M_Turntable_Steel.M_Turntable_Steel"));
		}
		const int32 NumMats = FMath::Max(1, WeapComp->GetNumMaterials());
		for (int32 i = 0; i < NumMats; ++i)
		{
			UMaterialInstanceDynamic* MID = WeapComp->CreateDynamicMaterialInstance(i, WeaponMat ? WeaponMat : WeapComp->GetMaterial(i));
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("BaseColor"), ColorOverride);
				MID->SetVectorParameterValue(TEXT("CamoColorPrimary"), ColorOverride);
				MID->SetVectorParameterValue(TEXT("Color"), ColorOverride);
				MID->SetScalarParameterValue(TEXT("Roughness"), FinishRoughness);
				MID->SetScalarParameterValue(TEXT("RoughnessBase"), FinishRoughness);
				MID->SetScalarParameterValue(TEXT("Metallic"), FinishMetallic);
			}
		}
	};

	const FLinearColor MagColor = (CurrentWeaponFinish == EOperatorWeaponFinish::DesertTanCerakote)
		? FLinearColor(0.42f, 0.35f, 0.25f, 1.0f)
		: FLinearColor(0.08f, 0.09f, 0.11f, 1.0f);

	ApplyToWeaponMesh(TableRifleReceiverMesh, FinishColor);
	ApplyToWeaponMesh(TableRifleMagMesh, MagColor);
	ApplyToWeaponMesh(HeldRifleReceiverMesh, FinishColor);
	ApplyToWeaponMesh(HeldRifleMagMesh, MagColor);
}
