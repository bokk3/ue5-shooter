#include "Character/FPSCharacterPreviewActor.h"
#include "Character/FPSOperatorGearComponent.h"
#include "Character/FPSAnimInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
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

	// 2. Camera: Calibrated to frame the full Task Force Echo Operator (mid-shin Z=20cm to ~19cm above helmet crown Z=202cm)
	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(RootComponent);
	PreviewCamera->SetRelativeLocation(FVector(335.0f, 0.0f, 126.0f));
	PreviewCamera->SetRelativeRotation(FRotator(-1.6f, 180.0f, 0.0f));
	PreviewCamera->FieldOfView = 54.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	PreviewCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	PreviewCamera->PostProcessSettings.AutoExposureBias = 0.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PreviewCamera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = 0;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	PreviewCamera->PostProcessSettings.AutoExposureMinBrightness = 1.0f;
	PreviewCamera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	PreviewCamera->PostProcessSettings.AutoExposureMaxBrightness = 1.0f;
	PreviewCamera->PostProcessSettings.bOverride_BloomIntensity = true;
	PreviewCamera->PostProcessSettings.BloomIntensity = 0.1f;

	// 3. Studio Lighting: 3-point tactical dynamic lighting (Warm amber key light, cool cyan rim light)
	StudioKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioKeyLight"));
	StudioKeyLight->SetupAttachment(RootComponent);
	StudioKeyLight->SetMobility(EComponentMobility::Movable);
	StudioKeyLight->SetRelativeLocation(FVector(190.0f, -120.0f, 210.0f));
	StudioKeyLight->SetRelativeRotation(FRotator(-35.0f, 145.0f, 0.0f));
	StudioKeyLight->SetIntensity(22000.0f);
	StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.40f)); // Warm amber key light
	StudioKeyLight->SetAttenuationRadius(750.0f);
	StudioKeyLight->SetInnerConeAngle(25.0f);
	StudioKeyLight->SetOuterConeAngle(55.0f);

	StudioRimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioRimLight"));
	StudioRimLight->SetupAttachment(RootComponent);
	StudioRimLight->SetMobility(EComponentMobility::Movable);
	StudioRimLight->SetRelativeLocation(FVector(-160.0f, 120.0f, 175.0f));
	StudioRimLight->SetRelativeRotation(FRotator(-25.0f, -35.0f, 0.0f));
	StudioRimLight->SetIntensity(18000.0f);
	StudioRimLight->SetLightColor(FLinearColor(0.0f, 0.898f, 1.0f)); // Tactical cyan rim light (#00E5FF)
	StudioRimLight->SetAttenuationRadius(650.0f);
	StudioRimLight->SetInnerConeAngle(30.0f);
	StudioRimLight->SetOuterConeAngle(65.0f);
	StudioRimLight->SetCastShadows(false);

	StudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
	StudioFillLight->SetupAttachment(RootComponent);
	StudioFillLight->SetMobility(EComponentMobility::Movable);
	StudioFillLight->SetRelativeLocation(FVector(160.0f, 140.0f, 90.0f));
	StudioFillLight->SetIntensity(7000.0f);
	StudioFillLight->SetLightColor(FLinearColor(0.12f, 0.22f, 0.35f)); // Deep blue-slate fill
	StudioFillLight->SetAttenuationRadius(600.0f);
	StudioFillLight->SetCastShadows(false);

	StudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
	StudioAmbientLight->SetupAttachment(RootComponent);
	StudioAmbientLight->SetMobility(EComponentMobility::Movable);
	StudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	StudioAmbientLight->SetIntensity(3000.0f);
	StudioAmbientLight->SetLightColor(FLinearColor(0.08f, 0.12f, 0.18f));
	StudioAmbientLight->SetAttenuationRadius(700.0f);
	StudioAmbientLight->SetCastShadows(false);

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

	// 5.5 Master Anatomical Player Body (SK_BaseMale with connected limbs & realistic muscle deformation)
	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(TurntableRoot);
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	BodyMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BaseMaleFinder(TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
	if (BaseMaleFinder.Succeeded())
	{
		BodyMesh->SetSkeletalMesh(BaseMaleFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Idle_ADS.Anim_BaseMale_Idle_ADS"));
	if (IdleAnimFinder.Succeeded())
	{
		AnimIdleADS = IdleAnimFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> LowReadyAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_LowReady.Anim_BaseMale_LowReady"));
	if (LowReadyAnimFinder.Succeeded())
	{
		AnimLowReady = LowReadyAnimFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Walk.Anim_BaseMale_Walk"));
	if (WalkAnimFinder.Succeeded())
	{
		AnimWalk = WalkAnimFinder.Object;
	}

	// Use UFPSAnimInstance C++ 21-Bone Procedural Low-Ready & Two-Bone IK Solver on SK_BaseMale
	BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	BodyMesh->SetAnimInstanceClass(UFPSAnimInstance::StaticClass());

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	if (BaseBodyMatFinder.Succeeded())
	{
		BodyMesh->SetMaterial(0, BaseBodyMatFinder.Object);
	}

	// 5.6 7-Zone Socket-Driven 3D Tactical Gear & Carbine Rig Component
	OperatorGearComponent = CreateDefaultSubobject<UFPSOperatorGearComponent>(TEXT("OperatorGearComponent"));

	// 5.7 Legacy AK47Mesh (Attached to TurntableRoot and hidden because OperatorGearComponent->CarbineProcMesh renders the 1:1 Spec-Ops Carbine)
	AK47Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AK47Mesh"));
	AK47Mesh->SetupAttachment(TurntableRoot);
	AK47Mesh->SetRelativeLocation(FVector::ZeroVector);
	AK47Mesh->SetRelativeRotation(FRotator::ZeroRotator);
	AK47Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AK47Mesh->SetCastShadow(false);
	AK47Mesh->SetVisibility(false);

	// 6. Turntable Pedestal
	SetupMeshComp(PedestalMesh, TEXT("PedestalMesh"), CylinderMesh, FVector(0.0f, 0.0f, -2.0f), FRotator::ZeroRotator, FVector(0.9f, 0.9f, 0.04f));

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

	// Ensure SK_BaseMale is loaded
	if (BodyMesh && !BodyMesh->GetSkeletalMeshAsset())
	{
		if (USkeletalMesh* LoadedMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale")))
		{
			BodyMesh->SetSkeletalMesh(LoadedMesh);
		}
	}

	if (BodyMesh)
	{
		UMaterialInterface* CurrentMat = BodyMesh->GetMaterial(0);
		if (!CurrentMat || !CurrentMat->GetName().Contains(TEXT("M_Operator_BaseBody")))
		{
			if (UMaterialInterface* LoadedMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody")))
			{
				BodyMesh->SetMaterial(0, LoadedMat);
			}
		}

		// Run C++ UFPSAnimInstance (21-bone Low-Ready Carbine + Two-Bone IK solver)
		BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		BodyMesh->SetAnimInstanceClass(UFPSAnimInstance::StaticClass());
		BodyMesh->InitAnim(true);

		if (UFPSAnimInstance* FPSAnim = Cast<UFPSAnimInstance>(BodyMesh->GetAnimInstance()))
		{
			FPSAnim->bIsMenuPreview = true;
			FPSAnim->MenuPoseIndex = CurrentStanceIndex; // 0 = Low-Ready Carbine
		}
	}

	if (BodyMesh && BodyMesh->GetSkeletalMeshAsset())
	{
		// Hide all legacy primitive blocks when SK_BaseMale + UFPSOperatorGearComponent are active
		auto HideComp = [](UStaticMeshComponent* Comp)
		{
			if (Comp)
			{
				Comp->SetVisibility(false);
				Comp->SetHiddenInGame(true);
			}
		};
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
		HideComp(AK47Mesh);

		BodyMesh->SetVisibility(true);
	}

	// Pedestal dark metallic material
	if (PedestalMesh)
	{
		if (UMaterialInstanceDynamic* PedestalMID = PedestalMesh->CreateDynamicMaterialInstance(0))
		{
			PedestalMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.04f, 0.06f));
		}
	}

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

	// Initialize the 7-Zone Socket-Driven 3D Tactical Gear & Carbine Rig
	if (OperatorGearComponent && BodyMesh)
	{
		OperatorGearComponent->InitializeGearRig(BodyMesh, TurntableRoot, true);
	}

	ApplyCamoColors();
	ApplyHeadgearVisibility();
	ApplyVestConfiguration();
	ApplyWeaponFinish();
	SetPreviewStance(CurrentStanceIndex);
}

void AFPSCharacterPreviewActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Turntable 360-degree continuous rotation
	if (bIsRotating && TurntableRoot)
	{
		TurntableRoot->AddLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	}

	// 2. Pin 7-Zone Tactical Armor & Carbine to animated SK_BaseMale bones
	if (OperatorGearComponent)
	{
		OperatorGearComponent->UpdateGearTransforms();
	}

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
	if (OperatorGearComponent)
	{
		OperatorGearComponent->ApplyCamoPreset(static_cast<int32>(CurrentCamo));
	}
}

void AFPSCharacterPreviewActor::ApplyHeadgearVisibility()
{
	if (OperatorGearComponent)
	{
		OperatorGearComponent->ApplyHeadgearVariant(static_cast<int32>(CurrentHeadgear));
	}
}

void AFPSCharacterPreviewActor::ApplyVestConfiguration()
{
	if (OperatorGearComponent)
	{
		OperatorGearComponent->ApplyVestVariant(static_cast<int32>(CurrentVest));
	}
}

void AFPSCharacterPreviewActor::ApplyWeaponFinish()
{
	if (OperatorGearComponent)
	{
		OperatorGearComponent->ApplyWeaponFinishVariant(static_cast<int32>(CurrentWeaponFinish));
	}
}
