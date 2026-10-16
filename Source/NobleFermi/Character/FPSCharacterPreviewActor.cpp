#include "Character/FPSCharacterPreviewActor.h"
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

AFPSCharacterPreviewActor::AFPSCharacterPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	// 1. Root Scene
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	// 2. Camera: Positioned in front-left looking directly at operator's torso/chest
	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(RootComponent);
	PreviewCamera->SetRelativeLocation(FVector(250.0f, -40.0f, 120.0f));
	PreviewCamera->SetRelativeRotation(FRotator(-3.4f, 171.0f, 0.0f));
	PreviewCamera->FieldOfView = 52.0f;
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

	// 3. Studio Lighting: 3-point tactical dynamic lighting (Movable, calibrated high-output studio lumens)
	StudioKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioKeyLight"));
	StudioKeyLight->SetupAttachment(RootComponent);
	StudioKeyLight->SetMobility(EComponentMobility::Movable);
	StudioKeyLight->SetRelativeLocation(FVector(190.0f, -130.0f, 220.0f));
	StudioKeyLight->SetRelativeRotation(FRotator(-40.0f, 145.0f, 0.0f));
	StudioKeyLight->SetIntensity(16000.0f);
	StudioKeyLight->SetLightColor(FLinearColor(1.0f, 0.96f, 0.90f));
	StudioKeyLight->SetAttenuationRadius(700.0f);
	StudioKeyLight->SetInnerConeAngle(25.0f);
	StudioKeyLight->SetOuterConeAngle(55.0f);

	StudioRimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("StudioRimLight"));
	StudioRimLight->SetupAttachment(RootComponent);
	StudioRimLight->SetMobility(EComponentMobility::Movable);
	StudioRimLight->SetRelativeLocation(FVector(-150.0f, 110.0f, 170.0f));
	StudioRimLight->SetRelativeRotation(FRotator(-25.0f, -35.0f, 0.0f));
	StudioRimLight->SetIntensity(10000.0f);
	StudioRimLight->SetLightColor(FLinearColor(0.25f, 0.55f, 1.0f)); // Tactical cyan-blue rim
	StudioRimLight->SetAttenuationRadius(600.0f);
	StudioRimLight->SetInnerConeAngle(30.0f);
	StudioRimLight->SetOuterConeAngle(65.0f);

	StudioFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioFillLight"));
	StudioFillLight->SetupAttachment(RootComponent);
	StudioFillLight->SetMobility(EComponentMobility::Movable);
	StudioFillLight->SetRelativeLocation(FVector(160.0f, 140.0f, 90.0f));
	StudioFillLight->SetIntensity(6000.0f);
	StudioFillLight->SetLightColor(FLinearColor(0.70f, 0.75f, 0.85f));
	StudioFillLight->SetAttenuationRadius(550.0f);

	StudioAmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StudioAmbientLight"));
	StudioAmbientLight->SetupAttachment(RootComponent);
	StudioAmbientLight->SetMobility(EComponentMobility::Movable);
	StudioAmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	StudioAmbientLight->SetIntensity(2500.0f);
	StudioAmbientLight->SetLightColor(FLinearColor(0.65f, 0.70f, 0.80f));
	StudioAmbientLight->SetAttenuationRadius(650.0f);
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
		UE_LOG(LogTemp, Log, TEXT("[PREVIEW_ACTOR] Constructor: BaseMaleFinder succeeded: %s"), *BaseMaleFinder.Object->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PREVIEW_ACTOR] Constructor: BaseMaleFinder FAILED"));
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Idle_ADS.Anim_BaseMale_Idle_ADS"));
	if (IdleAnimFinder.Succeeded())
	{
		AnimIdleADS = IdleAnimFinder.Object;
		BodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		BodyMesh->SetAnimation(AnimIdleADS);
		BodyMesh->Play(true);
		UE_LOG(LogTemp, Log, TEXT("[PREVIEW_ACTOR] Constructor: IdleAnimFinder succeeded: %s"), *IdleAnimFinder.Object->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PREVIEW_ACTOR] Constructor: IdleAnimFinder FAILED"));
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

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	if (BaseBodyMatFinder.Succeeded())
	{
		BodyMesh->SetMaterial(0, BaseBodyMatFinder.Object);
		UE_LOG(LogTemp, Log, TEXT("[PREVIEW_ACTOR] Constructor: BaseBodyMatFinder succeeded: %s"), *BaseBodyMatFinder.Object->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PREVIEW_ACTOR] Constructor: BaseBodyMatFinder FAILED"));
	}

	// 5.6 AK-47 Rifle Substitute Mesh (SM_AK47 attached directly to weapon_r bone socket on BodyMesh)
	AK47Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AK47Mesh"));
	AK47Mesh->SetupAttachment(BodyMesh, TEXT("weapon_r"));
	AK47Mesh->SetRelativeLocation(FVector::ZeroVector);
	AK47Mesh->SetRelativeRotation(FRotator::ZeroRotator);
	AK47Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AK47Mesh->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> AK47MeshFinder(TEXT("/Game/Characters/BaseMale/SM_AK47.SM_AK47"));
	if (AK47MeshFinder.Succeeded())
	{
		AK47Mesh->SetStaticMesh(AK47MeshFinder.Object);
		UE_LOG(LogTemp, Log, TEXT("[PREVIEW_ACTOR] Constructor: AK47MeshFinder succeeded: %s"), *AK47MeshFinder.Object->GetName());
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKSteelMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKWoodMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AKBakeliteMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite"));
	if (AKSteelMatFinder.Succeeded()) AK47Mesh->SetMaterial(0, AKSteelMatFinder.Object);
	if (AKWoodMatFinder.Succeeded()) AK47Mesh->SetMaterial(1, AKWoodMatFinder.Object);
	if (AKBakeliteMatFinder.Succeeded()) AK47Mesh->SetMaterial(2, AKBakeliteMatFinder.Object);

	// 6. Turntable Pedestal
	SetupMeshComp(PedestalMesh, TEXT("PedestalMesh"), CylinderMesh, FVector(0.0f, 0.0f, -2.0f), FRotator::ZeroRotator, FVector(0.9f, 0.9f, 0.04f));

	// 7. Human Male Anatomical Skeleton (180 cm anthropometry)
	// Pelvis (96cm above ground)
	SetupMeshComp(PelvisMesh, TEXT("PelvisMesh"), CubeMesh, FVector(0.0f, 0.0f, 96.0f), FRotator::ZeroRotator, FVector(0.24f, 0.32f, 0.16f));

	// Spine Lumbar (110cm) & Mid-Thoracic (124cm)
	SetupMeshComp(SpineLowerMesh, TEXT("SpineLowerMesh"), CylinderMesh, FVector(0.0f, 0.0f, 110.0f), FRotator::ZeroRotator, FVector(0.22f, 0.26f, 0.14f));
	SetupMeshComp(SpineUpperMesh, TEXT("SpineUpperMesh"), CylinderMesh, FVector(0.0f, 0.0f, 124.0f), FRotator::ZeroRotator, FVector(0.24f, 0.28f, 0.14f));

	// Upper Thoracic Chest & Tactical Vest
	SetupMeshComp(ChestMesh, TEXT("ChestMesh"), CubeMesh, FVector(0.0f, 0.0f, 138.0f), FRotator::ZeroRotator, FVector(0.26f, 0.38f, 0.16f));
	SetupMeshComp(VestMesh, TEXT("VestMesh"), CubeMesh, FVector(1.0f, 0.0f, 132.0f), FRotator::ZeroRotator, FVector(0.30f, 0.36f, 0.26f));

	// Neck (148cm) & Cranium (164cm)
	SetupMeshComp(NeckMesh, TEXT("NeckMesh"), CylinderMesh, FVector(0.0f, 0.0f, 148.0f), FRotator::ZeroRotator, FVector(0.12f, 0.12f, 0.10f));
	SetupMeshComp(HeadMesh, TEXT("HeadMesh"), SphereMesh, FVector(0.0f, 0.0f, 164.0f), FRotator::ZeroRotator, FVector(0.20f, 0.18f, 0.22f));

	// Headgear Variants
	SetupMeshComp(HelmetMesh, TEXT("HelmetMesh"), SphereMesh, FVector(0.0f, 0.0f, 167.0f), FRotator::ZeroRotator, FVector(0.23f, 0.21f, 0.20f));
	SetupMeshComp(NVGMesh, TEXT("NVGMesh"), CubeMesh, FVector(12.0f, 0.0f, 166.0f), FRotator::ZeroRotator, FVector(0.12f, 0.16f, 0.06f));
	SetupMeshComp(BalaclavaMesh, TEXT("BalaclavaMesh"), CylinderMesh, FVector(1.0f, 0.0f, 161.0f), FRotator::ZeroRotator, FVector(0.20f, 0.18f, 0.12f));
	SetupMeshComp(CapMesh, TEXT("CapMesh"), SphereMesh, FVector(0.0f, 0.0f, 168.0f), FRotator::ZeroRotator, FVector(0.21f, 0.20f, 0.12f));

	// Arms & Hands (Two-handed ADS Aiming Stance)
	// Right Arm (Trigger hand, stock against shoulder)
	SetupMeshComp(RightClavicleMesh, TEXT("RightClavicleMesh"), CylinderMesh, FVector(-2.0f, 16.0f, 142.0f), FRotator(0.0f, 0.0f, 30.0f), FVector(0.10f, 0.10f, 0.12f));
	SetupMeshComp(RightUpperArmMesh, TEXT("RightUpperArmMesh"), CylinderMesh, FVector(6.0f, 20.0f, 136.0f), FRotator(-35.0f, 15.0f, 0.0f), FVector(0.12f, 0.12f, 0.28f));
	SetupMeshComp(RightForearmMesh, TEXT("RightForearmMesh"), CylinderMesh, FVector(26.0f, 16.0f, 137.0f), FRotator(15.0f, -25.0f, 0.0f), FVector(0.11f, 0.11f, 0.26f));
	SetupMeshComp(RightHandMesh, TEXT("RightHandMesh"), SphereMesh, FVector(44.0f, 11.0f, 137.0f), FRotator::ZeroRotator, FVector(0.09f, 0.08f, 0.09f));

	// Left Arm (Support hand gripping rifle handguard)
	SetupMeshComp(LeftClavicleMesh, TEXT("LeftClavicleMesh"), CylinderMesh, FVector(-2.0f, -16.0f, 142.0f), FRotator(0.0f, 0.0f, -30.0f), FVector(0.10f, 0.10f, 0.12f));
	SetupMeshComp(LeftUpperArmMesh, TEXT("LeftUpperArmMesh"), CylinderMesh, FVector(14.0f, -14.0f, 134.0f), FRotator(-20.0f, -35.0f, 0.0f), FVector(0.12f, 0.12f, 0.28f));
	SetupMeshComp(LeftForearmMesh, TEXT("LeftForearmMesh"), CylinderMesh, FVector(40.0f, -4.0f, 138.0f), FRotator(15.0f, 45.0f, 0.0f), FVector(0.11f, 0.11f, 0.32f));
	SetupMeshComp(LeftHandMesh, TEXT("LeftHandMesh"), SphereMesh, FVector(62.0f, 5.0f, 140.0f), FRotator::ZeroRotator, FVector(0.09f, 0.08f, 0.09f));

	// Legs & Footwear
	SetupMeshComp(LeftThighMesh, TEXT("LeftThighMesh"), CylinderMesh, FVector(0.0f, -11.0f, 74.0f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.42f));
	SetupMeshComp(LeftCalfMesh, TEXT("LeftCalfMesh"), CylinderMesh, FVector(0.0f, -11.0f, 32.0f), FRotator::ZeroRotator, FVector(0.14f, 0.14f, 0.42f));
	SetupMeshComp(LeftBootMesh, TEXT("LeftBootMesh"), CubeMesh, FVector(4.0f, -11.0f, 5.0f), FRotator::ZeroRotator, FVector(0.24f, 0.12f, 0.10f));

	SetupMeshComp(RightThighMesh, TEXT("RightThighMesh"), CylinderMesh, FVector(0.0f, 11.0f, 74.0f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.42f));
	SetupMeshComp(RightCalfMesh, TEXT("RightCalfMesh"), CylinderMesh, FVector(0.0f, 11.0f, 32.0f), FRotator::ZeroRotator, FVector(0.14f, 0.14f, 0.42f));
	SetupMeshComp(RightBootMesh, TEXT("RightBootMesh"), CubeMesh, FVector(4.0f, 11.0f, 5.0f), FRotator::ZeroRotator, FVector(0.24f, 0.12f, 0.10f));

	// 8. Tactical Weapon Subsystem
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

	// Optional external weapon skeletal mesh
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(TurntableRoot);
	WeaponMesh->SetVisibility(false);
}

void AFPSCharacterPreviewActor::BeginPlay()
{
	Super::BeginPlay();

	// Create dynamic material instances for all visual components
	auto RegisterUniform = [this](UStaticMeshComponent* Comp)
	{
		if (Comp)
		{
			UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(0);
			if (MID) UniformMaterials.Add(MID);
		}
	};

	auto RegisterArmor = [this](UStaticMeshComponent* Comp)
	{
		if (Comp)
		{
			UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(0);
			if (MID) ArmorMaterials.Add(MID);
		}
	};

	auto RegisterWeapon = [this](UStaticMeshComponent* Comp)
	{
		if (Comp)
		{
			UMaterialInstanceDynamic* MID = Comp->CreateDynamicMaterialInstance(0);
			if (MID) WeaponMaterials.Add(MID);
		}
	};

	// Master Anatomical Player Body or Primitive Fallback
	if (BodyMesh && !BodyMesh->GetSkeletalMeshAsset())
	{
		USkeletalMesh* LoadedMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
		if (LoadedMesh)
		{
			BodyMesh->SetSkeletalMesh(LoadedMesh);
			UE_LOG(LogTemp, Log, TEXT("[PREVIEW_ACTOR] BeginPlay: Loaded SK_BaseMale via LoadObject fallback!"));
		}
	}

	if (!AnimIdleADS)
	{
		AnimIdleADS = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Idle_ADS.Anim_BaseMale_Idle_ADS"));
	}
	if (!AnimLowReady)
	{
		AnimLowReady = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_LowReady.Anim_BaseMale_LowReady"));
	}
	if (!AnimWalk)
	{
		AnimWalk = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Walk.Anim_BaseMale_Walk"));
	}

	if (BodyMesh && BodyMesh->GetSkeletalMeshAsset() && AnimIdleADS)
	{
		BodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		BodyMesh->SetAnimation(AnimIdleADS);
		BodyMesh->Play(true);
		UE_LOG(LogTemp, Log, TEXT("[PREVIEW_ACTOR] BeginPlay: Playing Anim_BaseMale_Idle_ADS on SK_BaseMale!"));
	}

	if (BodyMesh && !BodyMesh->GetMaterial(0))
	{
		UMaterialInterface* LoadedMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
		if (LoadedMat)
		{
			BodyMesh->SetMaterial(0, LoadedMat);
			UE_LOG(LogTemp, Log, TEXT("[PREVIEW_ACTOR] BeginPlay: Loaded M_Operator_BaseBody via LoadObject fallback!"));
		}
	}

	if (AK47Mesh && !AK47Mesh->GetStaticMesh())
	{
		UStaticMesh* LoadedAK = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SM_AK47.SM_AK47"));
		if (LoadedAK)
		{
			AK47Mesh->SetStaticMesh(LoadedAK);
			UE_LOG(LogTemp, Log, TEXT("[PREVIEW_ACTOR] BeginPlay: Loaded SM_AK47 via LoadObject fallback!"));
		}
	}

	if (AK47Mesh && AK47Mesh->GetStaticMesh() && BodyMesh)
	{
		AK47Mesh->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("weapon_r"));
		AK47Mesh->SetRelativeLocation(FVector::ZeroVector);
		AK47Mesh->SetRelativeRotation(FRotator::ZeroRotator);
		AK47Mesh->SetVisibility(true);

		if (!AK47Mesh->GetMaterial(0))
		{
			if (UMaterialInterface* M0 = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Steel.M_AK47_Steel"))) AK47Mesh->SetMaterial(0, M0);
		}
		if (!AK47Mesh->GetMaterial(1))
		{
			if (UMaterialInterface* M1 = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Wood.M_AK47_Wood"))) AK47Mesh->SetMaterial(1, M1);
		}
		if (!AK47Mesh->GetMaterial(2))
		{
			if (UMaterialInterface* M2 = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite.M_AK47_Bakelite"))) AK47Mesh->SetMaterial(2, M2);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[PREVIEW_ACTOR] BeginPlay: BodyMesh Valid=%d, SkeletalMeshAsset=%s, Visible=%d, AK47Mesh=%s"),
		BodyMesh != nullptr,
		(BodyMesh && BodyMesh->GetSkeletalMeshAsset()) ? *BodyMesh->GetSkeletalMeshAsset()->GetName() : TEXT("None"),
		BodyMesh ? (int32)BodyMesh->IsVisible() : -1,
		(AK47Mesh && AK47Mesh->GetStaticMesh()) ? *AK47Mesh->GetStaticMesh()->GetName() : TEXT("None"));

	if (BodyMesh && BodyMesh->GetSkeletalMeshAsset())
	{
		// Hide all legacy primitive blocks when the true connected anatomical body is present
		auto HideComp = [](UStaticMeshComponent* Comp) { if (Comp) Comp->SetVisibility(false); };
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

		if (AK47Mesh && AK47Mesh->GetStaticMesh())
		{
			HideComp(WeaponReceiverMesh);
			HideComp(WeaponBarrelMesh);
			HideComp(WeaponOpticMesh);
			HideComp(WeaponMagMesh);
			HideComp(WeaponStockMesh);
		}

		BodyMesh->SetVisibility(true);

		UMaterialInstanceDynamic* BodyMID = BodyMesh->CreateDynamicMaterialInstance(0);
		if (BodyMID)
		{
			UniformMaterials.Add(BodyMID);
		}
	}
	else
	{
		// Uniform meshes fallback
		RegisterUniform(PelvisMesh);
		RegisterUniform(SpineLowerMesh);
		RegisterUniform(SpineUpperMesh);
		RegisterUniform(ChestMesh);
		RegisterUniform(NeckMesh);
		RegisterUniform(HeadMesh);
		RegisterUniform(RightClavicleMesh);
		RegisterUniform(RightUpperArmMesh);
		RegisterUniform(RightForearmMesh);
		RegisterUniform(RightHandMesh);
		RegisterUniform(LeftClavicleMesh);
		RegisterUniform(LeftUpperArmMesh);
		RegisterUniform(LeftForearmMesh);
		RegisterUniform(LeftHandMesh);
		RegisterUniform(LeftThighMesh);
		RegisterUniform(LeftCalfMesh);
		RegisterUniform(RightThighMesh);
		RegisterUniform(RightCalfMesh);

		// Armor & Gear
		RegisterArmor(VestMesh);
		RegisterArmor(HelmetMesh);
		RegisterArmor(NVGMesh);
		RegisterArmor(BalaclavaMesh);
		RegisterArmor(CapMesh);
		RegisterArmor(LeftBootMesh);
		RegisterArmor(RightBootMesh);

		// Weapon
		RegisterWeapon(WeaponReceiverMesh);
		RegisterWeapon(WeaponBarrelMesh);
		RegisterWeapon(WeaponOpticMesh);
		RegisterWeapon(WeaponMagMesh);
		RegisterWeapon(WeaponStockMesh);
	}

	// Pedestal dark metallic material
	if (PedestalMesh)
	{
		UMaterialInstanceDynamic* PedestalMID = PedestalMesh->CreateDynamicMaterialInstance(0);
		if (PedestalMID)
		{
			PedestalMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.04f, 0.05f, 0.07f));
		}
	}

	ApplyCamoColors();
	ApplyHeadgearVisibility();
	ApplyVestConfiguration();
}

void AFPSCharacterPreviewActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Turntable 360-degree continuous rotation
	if (bIsRotating && TurntableRoot)
	{
		TurntableRoot->AddLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	}

	// 2. Procedural idle breathing and ADS aim motion
	UpdateAimingPose(DeltaTime);
}

void AFPSCharacterPreviewActor::UpdateAimingPose(float DeltaTime)
{
	AimBreathTimer += DeltaTime;

	// Procedural subtle breathing sway for fallback primitive meshes
	const float BreathFreq = 1.6f;
	const float BreathZ = FMath::Sin(AimBreathTimer * BreathFreq) * 0.40f;
	const float BreathPitch = FMath::Sin(AimBreathTimer * BreathFreq) * 0.35f;

	if (WeaponRoot)
	{
		if (bIsAiming)
		{
			WeaponRoot->SetRelativeLocation(BaseWeaponLoc + FVector(0.0f, 0.0f, BreathZ));
			WeaponRoot->SetRelativeRotation(BaseWeaponRot + FRotator(BreathPitch, 0.0f, 0.0f));
		}
		else
		{
			// Low-ready lowered stance
			WeaponRoot->SetRelativeLocation(BaseWeaponLoc + FVector(-8.0f, 2.0f, -22.0f + BreathZ * 0.5f));
			WeaponRoot->SetRelativeRotation(FRotator(-22.0f + BreathPitch, 0.0f, 0.0f));
		}
	}

	if (ChestMesh)
	{
		const float Expansion = 1.0f + 0.015f * FMath::Sin(AimBreathTimer * BreathFreq);
		ChestMesh->SetRelativeScale3D(FVector(0.26f * Expansion, 0.38f * Expansion, 0.16f));
	}
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

void AFPSCharacterPreviewActor::SetPreviewStance(int32 StanceIndex)
{
	CurrentStanceIndex = FMath::Clamp(StanceIndex, 0, 2);
	bIsAiming = (CurrentStanceIndex == 0);

	if (BodyMesh && BodyMesh->GetSkeletalMeshAsset())
	{
		UAnimSequence* TargetAnim = AnimIdleADS;
		if (CurrentStanceIndex == 1 && AnimLowReady)
		{
			TargetAnim = AnimLowReady;
		}
		else if (CurrentStanceIndex == 2 && AnimWalk)
		{
			TargetAnim = AnimWalk;
		}

		if (TargetAnim)
		{
			BodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			BodyMesh->SetAnimation(TargetAnim);
			BodyMesh->Play(true);
		}
	}
}

void AFPSCharacterPreviewActor::SetAimingStance(bool bInAiming)
{
	// Cycle through the 3 stances (0 = Aimed ADS, 1 = Low-Ready, 2 = Walk Cycle)
	SetPreviewStance((CurrentStanceIndex + 1) % 3);

	if (RightForearmMesh && LeftForearmMesh)
	{
		if (bIsAiming)
		{
			RightForearmMesh->SetRelativeRotation(FRotator(15.0f, -25.0f, 0.0f));
			LeftForearmMesh->SetRelativeRotation(FRotator(15.0f, 45.0f, 0.0f));
			HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 164.0f));
		}
		else
		{
			// Lower arms in low ready
			RightForearmMesh->SetRelativeRotation(FRotator(-15.0f, -15.0f, 0.0f));
			LeftForearmMesh->SetRelativeRotation(FRotator(-10.0f, 30.0f, 0.0f));
			HeadMesh->SetRelativeLocation(FVector(-2.0f, 0.0f, 165.0f));
		}
	}
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
	FLinearColor UniformColor = FLinearColor(0.22f, 0.25f, 0.30f);
	FLinearColor ArmorColor = FLinearColor(0.12f, 0.14f, 0.18f);
	FLinearColor WeaponColor = FLinearColor(0.06f, 0.07f, 0.08f);

	// Multi-zone Tactical Camo & Body Parameters
	FLinearColor CamoPrimary = FLinearColor(0.25f, 0.28f, 0.32f, 1.0f);
	FLinearColor CamoSecondary = FLinearColor(0.12f, 0.14f, 0.17f, 1.0f);
	FLinearColor CamoTertiary = FLinearColor(0.42f, 0.44f, 0.46f, 1.0f);
	FLinearColor TorsoColor = FLinearColor(0.18f, 0.20f, 0.23f, 1.0f);
	FLinearColor SkinTone = FLinearColor(0.82f, 0.62f, 0.50f, 1.0f);
	FLinearColor GearColor = FLinearColor(0.05f, 0.06f, 0.07f, 1.0f);
	float RoughnessBase = 0.75f;
	float SubsurfaceIntensity = 0.40f;
	float CamoTiling = 6.0f;
	float RipstopTiling = 24.0f;

	switch (CurrentCamo)
	{
	case EOperatorCamo::UrbanDigital:
		UniformColor = FLinearColor(0.22f, 0.25f, 0.30f); // Slate gray
		ArmorColor = FLinearColor(0.12f, 0.14f, 0.18f);   // Dark charcoal
		WeaponColor = FLinearColor(0.06f, 0.07f, 0.08f);  // Matte tactical black

		CamoPrimary = FLinearColor(0.25f, 0.28f, 0.32f, 1.0f);
		CamoSecondary = FLinearColor(0.12f, 0.14f, 0.17f, 1.0f);
		CamoTertiary = FLinearColor(0.42f, 0.44f, 0.46f, 1.0f);
		TorsoColor = FLinearColor(0.18f, 0.20f, 0.23f, 1.0f);
		GearColor = FLinearColor(0.05f, 0.06f, 0.07f, 1.0f);
		RoughnessBase = 0.76f;
		break;

	case EOperatorCamo::WoodlandTactical:
		UniformColor = FLinearColor(0.18f, 0.24f, 0.15f); // Olive drab
		ArmorColor = FLinearColor(0.24f, 0.22f, 0.16f);   // Ranger khaki
		WeaponColor = FLinearColor(0.11f, 0.10f, 0.09f);  // Anodized dark bronze

		CamoPrimary = FLinearColor(0.18f, 0.24f, 0.15f, 1.0f);
		CamoSecondary = FLinearColor(0.09f, 0.13f, 0.08f, 1.0f);
		CamoTertiary = FLinearColor(0.30f, 0.27f, 0.18f, 1.0f);
		TorsoColor = FLinearColor(0.16f, 0.21f, 0.14f, 1.0f);
		GearColor = FLinearColor(0.12f, 0.11f, 0.10f, 1.0f);
		RoughnessBase = 0.78f;
		break;

	case EOperatorCamo::DesertSpecOps:
		UniformColor = FLinearColor(0.48f, 0.42f, 0.32f); // Coyote tan
		ArmorColor = FLinearColor(0.55f, 0.49f, 0.38f);   // Desert khaki
		WeaponColor = FLinearColor(0.38f, 0.34f, 0.26f);  // FDE (Flat Dark Earth)

		CamoPrimary = FLinearColor(0.48f, 0.42f, 0.32f, 1.0f);
		CamoSecondary = FLinearColor(0.34f, 0.29f, 0.22f, 1.0f);
		CamoTertiary = FLinearColor(0.58f, 0.53f, 0.42f, 1.0f);
		TorsoColor = FLinearColor(0.40f, 0.35f, 0.27f, 1.0f);
		GearColor = FLinearColor(0.25f, 0.22f, 0.18f, 1.0f);
		RoughnessBase = 0.82f;
		break;

	case EOperatorCamo::StealthBlackout:
		UniformColor = FLinearColor(0.04f, 0.04f, 0.05f); // Midnight black
		ArmorColor = FLinearColor(0.07f, 0.07f, 0.08f);   // Carbon kevlar
		WeaponColor = FLinearColor(0.03f, 0.03f, 0.04f);  // Stealth gunmetal

		CamoPrimary = FLinearColor(0.04f, 0.04f, 0.05f, 1.0f);
		CamoSecondary = FLinearColor(0.07f, 0.07f, 0.08f, 1.0f);
		CamoTertiary = FLinearColor(0.09f, 0.09f, 0.11f, 1.0f);
		TorsoColor = FLinearColor(0.03f, 0.03f, 0.04f, 1.0f);
		GearColor = FLinearColor(0.02f, 0.02f, 0.03f, 1.0f);
		RoughnessBase = 0.72f;
		break;
	}

	for (UMaterialInstanceDynamic* MID : UniformMaterials)
	{
		if (MID)
		{
			// Flat fallback parameters
			MID->SetVectorParameterValue(TEXT("Color"), UniformColor);
			MID->SetVectorParameterValue(TEXT("BaseColor"), UniformColor);

			// Master Operator PBR Multi-Zone & Ripstop Parameters
			MID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoPrimary);
			MID->SetVectorParameterValue(TEXT("CamoColorSecondary"), CamoSecondary);
			MID->SetVectorParameterValue(TEXT("CamoColorTertiary"), CamoTertiary);
			MID->SetVectorParameterValue(TEXT("TorsoColor"), TorsoColor);
			MID->SetVectorParameterValue(TEXT("SkinTone"), SkinTone);
			MID->SetVectorParameterValue(TEXT("GearColor"), GearColor);
			MID->SetScalarParameterValue(TEXT("RoughnessBase"), RoughnessBase);
			MID->SetScalarParameterValue(TEXT("SubsurfaceIntensity"), SubsurfaceIntensity);
			MID->SetScalarParameterValue(TEXT("CamoTiling"), CamoTiling);
			MID->SetScalarParameterValue(TEXT("RipstopTiling"), RipstopTiling);
		}
	}

	for (UMaterialInstanceDynamic* MID : ArmorMaterials)
	{
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), ArmorColor);
			MID->SetVectorParameterValue(TEXT("BaseColor"), ArmorColor);
		}
	}

	for (UMaterialInstanceDynamic* MID : WeaponMaterials)
	{
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), WeaponColor);
			MID->SetVectorParameterValue(TEXT("BaseColor"), WeaponColor);
		}
	}

	// Glowing optic reticle
	if (WeaponOpticMesh)
	{
		UMaterialInstanceDynamic* OpticMID = WeaponOpticMesh->CreateDynamicMaterialInstance(0);
		if (OpticMID)
		{
			OpticMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.2f, 0.05f)); // Amber/Red Holo Reticle
		}
	}
}

void AFPSCharacterPreviewActor::ApplyHeadgearVisibility()
{
	if (BodyMesh && BodyMesh->GetSkeletalMeshAsset())
	{
		if (HelmetMesh) HelmetMesh->SetVisibility(false);
		if (NVGMesh) NVGMesh->SetVisibility(false);
		if (BalaclavaMesh) BalaclavaMesh->SetVisibility(false);
		if (CapMesh) CapMesh->SetVisibility(false);
		return;
	}

	const bool bHelmet = (CurrentHeadgear == EOperatorHeadgear::BallisticHelmet || CurrentHeadgear == EOperatorHeadgear::NightVisionGoggles);
	const bool bNVG = (CurrentHeadgear == EOperatorHeadgear::NightVisionGoggles);
	const bool bBalaclava = (CurrentHeadgear == EOperatorHeadgear::SpecOpsBalaclava);
	const bool bCap = (CurrentHeadgear == EOperatorHeadgear::CommCap);

	if (HelmetMesh) HelmetMesh->SetVisibility(bHelmet);
	if (NVGMesh)
	{
		NVGMesh->SetVisibility(bNVG);
		if (bNVG)
		{
			UMaterialInstanceDynamic* NVGMID = NVGMesh->CreateDynamicMaterialInstance(0);
			if (NVGMID)
			{
				NVGMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 0.9f, 0.3f)); // Phosphor green lens glow
			}
		}
	}
	if (BalaclavaMesh) BalaclavaMesh->SetVisibility(bBalaclava);
	if (CapMesh) CapMesh->SetVisibility(bCap);
}

void AFPSCharacterPreviewActor::ApplyVestConfiguration()
{
	if (!VestMesh) return;

	if (BodyMesh && BodyMesh->GetSkeletalMeshAsset())
	{
		VestMesh->SetVisibility(false);
		return;
	}

	switch (CurrentVest)
	{
	case EOperatorArmorVest::HeavyPlateCarrier:
		// Bulkier Level IV plate carrier with side plates
		VestMesh->SetRelativeScale3D(FVector(0.32f, 0.38f, 0.28f));
		VestMesh->SetVisibility(true);
		break;

	case EOperatorArmorVest::LightweightJPC:
		// Minimalist jumpable plate carrier
		VestMesh->SetRelativeScale3D(FVector(0.28f, 0.33f, 0.24f));
		VestMesh->SetVisibility(true);
		break;

	case EOperatorArmorVest::ReconChestRig:
		// Lightweight ammo rig
		VestMesh->SetRelativeScale3D(FVector(0.26f, 0.32f, 0.18f));
		VestMesh->SetVisibility(true);
		break;
	}
}

