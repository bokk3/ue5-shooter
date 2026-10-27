#include "Character/FPSCharacter.h"
#include "Character/FPSOperatorGearComponent.h"
#include "Character/FPSAnimInstance.h"
#include "Character/FPSCharacterMovementComponent.h"
#include "Combat/FPSHealthComponent.h"
#include "Combat/FPSFermiTimeBubble.h"
#include "Combat/FPSFermiDecoyActor.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Combat/FPSEquipmentComponent.h"
#include "BodyMechanics/FPSBodyMechanicsComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "Multiplayer/FPSPlayerState.h"
#include "Economy/FPSEconomySubsystem.h"
#include "Radar/FPSRadarSubsystem.h"
#include "Map/FPSCoastalSplineActor.h"
#include "UI/FPSHUDWidget.h"
#include "UI/FPSEscapeMenuWidget.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "BodyMechanics/FPSHumanBones.h"
#include "Multiplayer/FPSMultiplayerGameMode.h"
#include "Combat/FPSBallisticTracer.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Audio/FPSAudioSubsystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "TimerManager.h"

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

AFPSCharacter::AFPSCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	FPSMovementComponent = Cast<UFPSCharacterMovementComponent>(GetCharacterMovement());

	GetCapsuleComponent()->InitCapsuleSize(40.0f, 96.0f);

	// Primary 3rd-Person Tactical Camera Boom (Phase 8A OTS Spec)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
	CameraBoom->TargetArmLength = DefaultArmLength;
	CameraBoom->SocketOffset = HipfireShoulderOffset;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12.0f;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 18.0f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 24.0f;

	// Primary 3rd-Person Tactical Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(DefaultFOV);
	FollowCamera->PostProcessBlendWeight = 1.0f;

	// Exposure Lock (AEM_Manual, no blinding auto-exposure adjustments)
	FollowCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	FollowCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	FollowCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	FollowCamera->PostProcessSettings.AutoExposureBias = 0.0f;
	FollowCamera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	FollowCamera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = 0;

	// Controlled HDR Bloom (Zero Sand/Wall Glow in Rust Tactical)
	FollowCamera->PostProcessSettings.bOverride_BloomMethod = true;
	FollowCamera->PostProcessSettings.BloomMethod = EBloomMethod::BM_SOG;
	FollowCamera->PostProcessSettings.bOverride_BloomIntensity = true;
	FollowCamera->PostProcessSettings.BloomIntensity = 0.32f; // HARD CAP: Never exceed 0.40f
	FollowCamera->PostProcessSettings.bOverride_BloomThreshold = true;
	FollowCamera->PostProcessSettings.BloomThreshold = 1.35f; // CRITICAL: Desert sand/concrete reflections will NOT bloom

	// Micro-Capped Tactical Motion Blur (Flick-Safe)
	FollowCamera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	FollowCamera->PostProcessSettings.MotionBlurAmount = 0.18f; // Max 0.28f during Sprint/Slide/Dive
	FollowCamera->PostProcessSettings.bOverride_MotionBlurMax = true;
	FollowCamera->PostProcessSettings.MotionBlurMax = 2.2f; // HARD CEILING: Never exceed 2.5f to prevent mouse flick smearing
	FollowCamera->PostProcessSettings.bOverride_MotionBlurTargetFPS = true;
	FollowCamera->PostProcessSettings.MotionBlurTargetFPS = 120;
	FollowCamera->PostProcessSettings.bOverride_MotionBlurPerObjectSize = true;
	FollowCamera->PostProcessSettings.MotionBlurPerObjectSize = 0.5f;

	// Depth of Field (Sharp Hipfire, Subtle ADS Only)
	FollowCamera->PostProcessSettings.bOverride_DepthOfFieldFstop = true;
	FollowCamera->PostProcessSettings.DepthOfFieldFstop = 32.0f;
	FollowCamera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = true;
	FollowCamera->PostProcessSettings.DepthOfFieldFocalDistance = 1000.0f;

	FollowCamera->SetActive(true);

	// First-Person Camera (Inactive fallback)
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = false;
	FirstPersonCamera->SetActive(false);

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
	Mesh1P->SetupAttachment(FirstPersonCamera);
	Mesh1P->SetCastShadow(false);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->SetVisibility(false);
	Mesh1P->SetOwnerNoSee(true);
	Mesh1P->SetComponentTickEnabled(false);

	// Tactical 3rd Person Character Controls: Orient character to controller yaw
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Full tactical operator mesh: visible to local owner
	GetMesh()->SetOwnerNoSee(false);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetCastShadow(true);
	GetMesh()->bCastDynamicShadow = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BaseMaleMeshFinder(TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
	if (BaseMaleMeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(BaseMaleMeshFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseBodyMatFinder(TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
	if (BaseBodyMatFinder.Succeeded())
	{
		GetMesh()->SetMaterial(0, BaseBodyMatFinder.Object);
	}

	// Ballistic collision routing: movement capsule blocks visibility trace so bullet line traces register reliably
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Tactical Locomotion Animations
	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Idle_ADS.Anim_BaseMale_Idle_ADS"));
	if (IdleAnimFinder.Succeeded())
	{
		AnimIdleADS = IdleAnimFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Walk.Anim_BaseMale_Walk"));
	if (WalkAnimFinder.Succeeded())
	{
		AnimWalk = WalkAnimFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Run.Anim_BaseMale_Run"));
	if (RunAnimFinder.Succeeded())
	{
		AnimRun = RunAnimFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> SprintAnimFinder(TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Sprint.Anim_BaseMale_Sprint"));
	if (SprintAnimFinder.Succeeded())
	{
		AnimSprint = SprintAnimFinder.Object;
	}

	// Tactical Locomotion Animation Blueprint / Native C++ AnimInstance
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBPFinder(TEXT("/Game/Characters/BaseMale/ABP_BaseMale.ABP_BaseMale_C"));
	if (AnimBPFinder.Succeeded() && AnimBPFinder.Class)
	{
		AnimBlueprintClass = AnimBPFinder.Class;
		GetMesh()->SetAnimInstanceClass(AnimBlueprintClass);
	}
	else
	{
		GetMesh()->SetAnimInstanceClass(UFPSAnimInstance::StaticClass());
	}
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	HealthComponent = CreateDefaultSubobject<UFPSHealthComponent>(TEXT("HealthComponent"));
	InventoryComponent = CreateDefaultSubobject<UFPSInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->DefaultPrimaryClass = AFPSWeaponBase::StaticClass();
	EquipmentComponent = CreateDefaultSubobject<UFPSEquipmentComponent>(TEXT("EquipmentComponent"));
	BodyMechanicsComponent = CreateDefaultSubobject<UFPSBodyMechanicsComponent>(TEXT("BodyMechanicsComponent"));
	OperatorGearComponent = CreateDefaultSubobject<UFPSOperatorGearComponent>(TEXT("OperatorGearComponent"));
	HUDWidgetClass = UFPSHUDWidget::StaticClass();

	// Authoritative 3D Procedural Level IV Ceramic Plate Carrier (Attached to Capsule so it NEVER inherits 100x FBX bone scale!)
	PlateCarrierProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("PlateCarrierProcMesh"));
	PlateCarrierProcMesh->SetupAttachment(GetCapsuleComponent());
	PlateCarrierProcMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 28.0f));
	PlateCarrierProcMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PlateCarrierProcMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	PlateCarrierProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlateCarrierProcMesh->SetCastShadow(true);
	PlateCarrierProcMesh->bCastDynamicShadow = true;
	PlateCarrierProcMesh->SetOwnerNoSee(false);
	PlateCarrierProcMesh->SetOnlyOwnerSee(false);
	PlateCarrierProcMesh->SetHiddenInGame(false);
	PlateCarrierProcMesh->SetVisibility(true, true);

	// Authoritative 3D Procedural Tactical FAST Ballistic Helmet (Attached to Capsule so it NEVER inherits 100x FBX bone scale!)
	BallisticHelmetProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BallisticHelmetProcMesh"));
	BallisticHelmetProcMesh->SetupAttachment(GetCapsuleComponent());
	BallisticHelmetProcMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	BallisticHelmetProcMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	BallisticHelmetProcMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	BallisticHelmetProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BallisticHelmetProcMesh->SetCastShadow(true);
	BallisticHelmetProcMesh->bCastDynamicShadow = true;
	BallisticHelmetProcMesh->SetOwnerNoSee(false);
	BallisticHelmetProcMesh->SetOnlyOwnerSee(false);
	BallisticHelmetProcMesh->SetHiddenInGame(false);
	BallisticHelmetProcMesh->SetVisibility(true, true);

	// Legacy basic shape static meshes (Kept hidden; ProceduralMesh is authoritative)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	ChestCarrierMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestCarrierMesh"));
	ChestCarrierMesh->SetupAttachment(GetCapsuleComponent());
	ChestCarrierMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChestCarrierMesh->SetCastShadow(false);
	ChestCarrierMesh->SetHiddenInGame(true);
	ChestCarrierMesh->SetVisibility(false, true);

	BallisticHelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallisticHelmetMesh"));
	BallisticHelmetMesh->SetupAttachment(GetCapsuleComponent());
	BallisticHelmetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BallisticHelmetMesh->SetCastShadow(false);
	BallisticHelmetMesh->SetHiddenInGame(true);
	BallisticHelmetMesh->SetVisibility(false, true);

	EjectedPlateShardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EjectedPlateShardMesh"));
	EjectedPlateShardMesh->SetupAttachment(GetRootComponent());
	EjectedPlateShardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EjectedPlateShardMesh->SetCastShadow(false);
	EjectedPlateShardMesh->SetVisibility(false);
	if (CubeMeshFinder.Succeeded())
	{
		EjectedPlateShardMesh->SetStaticMesh(CubeMeshFinder.Object);
	}
	if (BasicMatFinder.Succeeded())
	{
		EjectedPlateShardMesh->SetMaterial(0, BasicMatFinder.Object);
	}
	EjectedPlateShardMesh->SetRelativeScale3D(FVector(0.18f, 0.04f, 0.22f));
}

void AFPSCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Bridge 1: Sync 3rd-person cosmetic loadout in construction/editor
	if (!bIsTacticalBot)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				ApplyLoadoutCosmetics(
					Economy->GetEquippedCamoIndex(),
					Economy->GetEquippedHeadgearIndex(),
					Economy->GetEquippedArmorIndex(),
					Economy->GetEquippedWeaponFinishIndex()
				);
			}
		}
	}
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultCameraPos = FVector(0.0f, 0.0f, 65.0f);
	if (FollowCamera)
	{
		FollowCamera->SetFieldOfView(DefaultFOV);
	}

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AFPSCharacter::HandleOnDeath);
		HealthComponent->OnDamageTaken.AddDynamic(this, &AFPSCharacter::HandleOnDamageTaken);
	}

	InitializeHUD();

	// Tactical Operator Mesh & Animation Runtime Fallbacks + Material Configuration
	if (GetMesh())
	{
		if (!GetMesh()->GetSkeletalMeshAsset())
		{
			USkeletalMesh* LoadedBaseMale = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/BaseMale/SK_BaseMale.SK_BaseMale"));
			if (LoadedBaseMale)
			{
				GetMesh()->SetSkeletalMesh(LoadedBaseMale);
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
		if (!AnimRun)
		{
			AnimRun = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Run.Anim_BaseMale_Run"));
		}
		if (!AnimSprint)
		{
			AnimSprint = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Sprint.Anim_BaseMale_Sprint"));
		}
		if (!AnimJumpLoop)
		{
			AnimJumpLoop = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Jump_Loop.Anim_BaseMale_Jump_Loop"));
		}
		if (!AnimCrouchIdle)
		{
			AnimCrouchIdle = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Crouch_Idle.Anim_BaseMale_Crouch_Idle"));
		}
		if (!AnimCrouchWalk)
		{
			AnimCrouchWalk = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Crouch_Walk.Anim_BaseMale_Crouch_Walk"));
		}
		if (!AnimSlide)
		{
			AnimSlide = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Slide.Anim_BaseMale_Slide"));
		}
		if (!AnimWalkBackwards)
		{
			AnimWalkBackwards = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/BaseMale/Animations/Anim_BaseMale_Walk_Backwards.Anim_BaseMale_Walk_Backwards"));
		}

		// Runtime fallback for Animation Blueprint mode / Authoritative C++ AnimInstance
		if (!AnimBlueprintClass)
		{
			AnimBlueprintClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/BaseMale/ABP_BaseMale.ABP_BaseMale_C"));
		}

		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (AnimLowReady)
		{
			GetMesh()->SetAnimation(AnimLowReady);
			GetMesh()->Play(true);
		}

		UMaterialInterface* CurrentMat = GetMesh()->GetMaterial(0);
		if (!CurrentMat || !CurrentMat->GetName().Contains(TEXT("M_Operator_BaseBody")))
		{
			UMaterialInterface* BaseBodyMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody.M_Operator_BaseBody"));
			if (BaseBodyMat)
			{
				GetMesh()->SetMaterial(0, BaseBodyMat);
			}
		}

		UMaterialInstanceDynamic* BodyMID = GetMesh()->CreateDynamicMaterialInstance(0);
		if (BodyMID)
		{
			if (UTexture* CamoTex = LoadObject<UTexture>(nullptr, TEXT("/Game/Characters/BaseMale/Textures/T_Camo_Digital.T_Camo_Digital")))
			{
				BodyMID->SetTextureParameterValue(TEXT("Texture_Camo"), CamoTex);
			}
			if (UTexture* MaskTex = LoadObject<UTexture>(nullptr, TEXT("/Game/Characters/BaseMale/Textures/T_Operator_Body_Mask.T_Operator_Body_Mask")))
			{
				BodyMID->SetTextureParameterValue(TEXT("Texture_BodyMask"), MaskTex);
			}
			if (UTexture* NormTex = LoadObject<UTexture>(nullptr, TEXT("/Game/Characters/BaseMale/Textures/T_Ripstop_Normal.T_Ripstop_Normal")))
			{
				BodyMID->SetTextureParameterValue(TEXT("Texture_RipstopNormal"), NormTex);
			}
			if (UTexture* RoughTex = LoadObject<UTexture>(nullptr, TEXT("/Game/Characters/BaseMale/Textures/T_Ripstop_Roughness.T_Ripstop_Roughness")))
			{
				BodyMID->SetTextureParameterValue(TEXT("Texture_RipstopRoughness"), RoughTex);
			}

			BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), FLinearColor(0.25f, 0.28f, 0.32f, 1.0f));
			BodyMID->SetVectorParameterValue(TEXT("CamoColorSecondary"), FLinearColor(0.12f, 0.14f, 0.17f, 1.0f));
			BodyMID->SetVectorParameterValue(TEXT("CamoColorTertiary"), FLinearColor(0.42f, 0.44f, 0.46f, 1.0f));
			BodyMID->SetVectorParameterValue(TEXT("TorsoColor"), FLinearColor(0.18f, 0.20f, 0.23f, 1.0f));
			BodyMID->SetVectorParameterValue(TEXT("SkinTone"), FLinearColor(0.82f, 0.62f, 0.50f, 1.0f));
			BodyMID->SetVectorParameterValue(TEXT("GearColor"), FLinearColor(0.05f, 0.06f, 0.07f, 1.0f));
			BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), 0.75f);
			BodyMID->SetScalarParameterValue(TEXT("SubsurfaceIntensity"), 0.40f);
			BodyMID->SetScalarParameterValue(TEXT("CamoTiling"), 6.0f);
			BodyMID->SetScalarParameterValue(TEXT("RipstopTiling"), 24.0f);
		}

		// Build and attach Authoritative 3D Procedural Level IV Ceramic Plate Carrier & FAST Helmet
		BuildProceduralTacticalGear();

		if (OperatorGearComponent)
		{
			OperatorGearComponent->InitializeGearRig(GetMesh(), GetCapsuleComponent(), false);
		}

		if (PlateCarrierProcMesh)
		{
			PlateCarrierProcMesh->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			PlateCarrierProcMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 28.0f));
			PlateCarrierProcMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
			PlateCarrierProcMesh->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
			PlateCarrierProcMesh->SetCastShadow(false);
			PlateCarrierProcMesh->SetHiddenInGame(true);
			PlateCarrierProcMesh->SetVisibility(false, true);
		}

		if (BallisticHelmetProcMesh)
		{
			BallisticHelmetProcMesh->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			BallisticHelmetProcMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
			BallisticHelmetProcMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
			BallisticHelmetProcMesh->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
			BallisticHelmetProcMesh->SetCastShadow(false);
			BallisticHelmetProcMesh->SetHiddenInGame(true);
			BallisticHelmetProcMesh->SetVisibility(false, true);
		}

		// Hide legacy basic shape static meshes permanently
		if (ChestCarrierMesh) { ChestCarrierMesh->SetVisibility(false); ChestCarrierMesh->SetHiddenInGame(true); }
		if (BallisticHelmetMesh) { BallisticHelmetMesh->SetVisibility(false); BallisticHelmetMesh->SetHiddenInGame(true); }

		if (EjectedPlateShardMesh)
		{
			if (UMaterialInstanceDynamic* ShardMID = EjectedPlateShardMesh->CreateDynamicMaterialInstance(0))
			{
				ShardMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.55f, 0.95f, 1.0f));
				ShardMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.12f, 0.55f, 0.95f, 1.0f));
			}
		}
	}

	// Load authentic physical surface footstep sounds if not set in blueprint
	auto LoadStepSound = [](const TCHAR* Path) -> USoundBase*
	{
		return LoadObject<USoundBase>(nullptr, Path);
	};

	if (FootstepConcreteSounds.Num() == 0)
	{
		if (USoundBase* S1 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Concrete/FS_Concrete_01.FS_Concrete_01"))) FootstepConcreteSounds.Add(S1);
		if (USoundBase* S2 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Concrete/FS_Concrete_02.FS_Concrete_02"))) FootstepConcreteSounds.Add(S2);
	}
	if (FootstepWoodSounds.Num() == 0)
	{
		if (USoundBase* S1 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Wood/FS_Wood_01.FS_Wood_01"))) FootstepWoodSounds.Add(S1);
		if (USoundBase* S2 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Wood/FS_Wood_02.FS_Wood_02"))) FootstepWoodSounds.Add(S2);
	}
	if (FootstepSandSounds.Num() == 0)
	{
		if (USoundBase* S1 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Sand/FS_Sand_01.FS_Sand_01"))) FootstepSandSounds.Add(S1);
		if (USoundBase* S2 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Sand/FS_Sand_02.FS_Sand_02"))) FootstepSandSounds.Add(S2);
	}
	if (FootstepWaterSounds.Num() == 0)
	{
		if (USoundBase* S1 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Water/FS_Water_01.FS_Water_01"))) FootstepWaterSounds.Add(S1);
		if (USoundBase* S2 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Water/FS_Water_02.FS_Water_02"))) FootstepWaterSounds.Add(S2);
		if (USoundBase* S3 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Water/FS_Water_03.FS_Water_03"))) FootstepWaterSounds.Add(S3);
	}
	if (FootstepMetalSounds.Num() == 0)
	{
		if (USoundBase* S1 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Metal/FS_Metal_01.FS_Metal_01"))) FootstepMetalSounds.Add(S1);
		if (USoundBase* S2 = LoadStepSound(TEXT("/Game/Audio/Footsteps/Metal/FS_Metal_02.FS_Metal_02"))) FootstepMetalSounds.Add(S2);
	}

	// Bridge 1: In-Game 3rd-Person Cosmetic Loadout Sync from economy subsystem
	if (!bIsTacticalBot && (IsLocallyControlled() || GetNetMode() == NM_Standalone))
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				ApplyLoadoutCosmetics(
					Economy->GetEquippedCamoIndex(),
					Economy->GetEquippedHeadgearIndex(),
					Economy->GetEquippedArmorIndex(),
					Economy->GetEquippedWeaponFinishIndex()
				);
			}
		}
	}
}

void AFPSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeHUD();

	if (!bIsTacticalBot)
	{
		if (APlayerController* PC = Cast<APlayerController>(NewController))
		{
			if (PC->IsLocalController())
			{
				if (UGameInstance* GI = GetGameInstance())
				{
					if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
					{
						ApplyLoadoutCosmetics(
							Economy->GetEquippedCamoIndex(),
							Economy->GetEquippedHeadgearIndex(),
							Economy->GetEquippedArmorIndex(),
							Economy->GetEquippedWeaponFinishIndex()
						);
					}
				}
			}
		}
	}
}

void AFPSCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeHUD();

	if (!bIsTacticalBot && IsLocallyControlled())
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				ApplyLoadoutCosmetics(
					Economy->GetEquippedCamoIndex(),
					Economy->GetEquippedHeadgearIndex(),
					Economy->GetEquippedArmorIndex(),
					Economy->GetEquippedWeaponFinishIndex()
				);
			}
		}
	}
}

void AFPSCharacter::InitializeHUD()
{
	if (ActiveHUDWidget)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && PC->IsLocalController())
	{
		TSubclassOf<UFPSHUDWidget> WidgetClass = HUDWidgetClass ? HUDWidgetClass : TSubclassOf<UFPSHUDWidget>(UFPSHUDWidget::StaticClass());
		ActiveHUDWidget = CreateWidget<UFPSHUDWidget>(PC, WidgetClass);
		if (ActiveHUDWidget)
		{
			ActiveHUDWidget->SetOwningCharacter(this);
			ActiveHUDWidget->AddToViewport(0);
			UE_LOG(LogTemp, Warning, TEXT("[FPSCharacter] InitializeHUD: Created and added UFPSHUDWidget to viewport for %s"), *GetNameSafe(this));

			if (FParse::Param(FCommandLine::Get(), TEXT("CaptureHUD")))
			{
				bShouldCaptureHUD = true;
				CaptureHUDFrameCounter = 0;
				UE_LOG(LogTemp, Warning, TEXT("[CaptureHUD] CaptureHUD enabled on local player pawn. Will capture at frame 60."));
			}
		}
	}
}

void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShouldCaptureHUD)
	{
		CaptureHUDFrameCounter++;
		if (CaptureHUDFrameCounter == 60)
		{
			FString ScreenshotPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("HUD_In_Action.png"));
			FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
			UE_LOG(LogTemp, Warning, TEXT("[CaptureHUD] Screenshot requested at frame 60 to: %s"), *ScreenshotPath);
		}
		else if (CaptureHUDFrameCounter >= 120)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CaptureHUD] Completed capture! Requesting clean exit at frame %d"), CaptureHUDFrameCounter);
			bShouldCaptureHUD = false;
			FGenericPlatformMisc::RequestExit(false, TEXT("CaptureHUD Complete"));
		}
	}

	// Track real-world cooldowns independent of CustomTimeDilation
	const float RealDeltaTime = DeltaTime / FMath::Max(0.05f, CustomTimeDilation);
	if (FermiDecoyCooldownRemaining > 0.0f)
	{
		FermiDecoyCooldownRemaining = FMath::Max(0.0f, FermiDecoyCooldownRemaining - RealDeltaTime);
	}
	if (QuantumSwapFlashTimer > 0.0f)
	{
		QuantumSwapFlashTimer = FMath::Max(0.0f, QuantumSwapFlashTimer - RealDeltaTime);
	}
	if (FOVPunchTimer > 0.0f)
	{
		FOVPunchTimer = FMath::Max(0.0f, FOVPunchTimer - RealDeltaTime);
	}
	if (QuantumSwapKillWindowRemaining > 0.0f)
	{
		QuantumSwapKillWindowRemaining = FMath::Max(0.0f, QuantumSwapKillWindowRemaining - RealDeltaTime);
	}

	UpdatePhysicalArmorAndHelmetDebris(RealDeltaTime);

	if (bIsTacticalBot)
	{
		TickTacticalBotAI(DeltaTime);
	}
	else
	{
		UpdateThirdPersonCamera(RealDeltaTime);
	}

	UpdateBallisticDivePosture(RealDeltaTime);
	UpdateLocomotionAnimation(DeltaTime);

	// Physical Surface Footstep Cadence Tracking
	if (GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround() && !bIsDead && !IsBallisticDiving())
	{
		const float GroundSpeed = GetVelocity().Size2D();
		if (GroundSpeed > 25.0f)
		{
			float StrideThreshold = 145.0f; // Walk stride (1.45m)
			float StepVolume = 0.42f;

			if (FPSMovementComponent && FPSMovementComponent->IsTacticalSprinting() && GroundSpeed > 380.0f)
			{
				StrideThreshold = 215.0f; // Sprint stride (2.15m)
				StepVolume = 0.82f;
			}
			else if (GroundSpeed > 280.0f)
			{
				StrideThreshold = 175.0f; // Jog/Run stride (1.75m)
				StepVolume = 0.62f;
			}
			else if (bIsCrouched || IsAiming())
			{
				StrideThreshold = 115.0f; // Stealth crouch stride (1.15m)
				StepVolume = 0.22f;
			}

			FootstepDistanceTraveled += GroundSpeed * DeltaTime;
			if (FootstepDistanceTraveled >= StrideThreshold)
			{
				TriggerSurfaceFootstep(StepVolume);
				FootstepDistanceTraveled = 0.0f;
			}
		}
		else
		{
			FootstepDistanceTraveled = FMath::Min(FootstepDistanceTraveled, 35.0f);
		}
	}

	// Update HUD Crosshair with current weapon spread
	const bool bIsAiming = (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights());
	if (ActiveHUDWidget && InventoryComponent)
	{
		AFPSWeaponBase* Weapon = InventoryComponent->GetActiveWeapon();
		if (Weapon)
		{
			// In ADS, crosshair collapses so operator aims via 3D optic / tight OTS sightline
			const float Spread = bIsAiming ? 0.0f : Weapon->GetCurrentSpread();
			ActiveHUDWidget->UpdateCrosshairSpread(Spread);
		}
	}

	// Authoritative 3D Weapon & Tactical Gear Pinning (100% visible & protected from 100x FBX bone scale!)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (InventoryComponent)
		{
			if (AFPSWeaponBase* Weapon = InventoryComponent->GetActiveWeapon())
			{
				Weapon->SetActorHiddenInGame(false);
				Weapon->SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));

				const FVector HandR_Pos = MeshComp->GetBoneLocation(FName(TEXT("hand_r")), EBoneSpaces::WorldSpace);
				const FVector HandL_Pos = MeshComp->GetBoneLocation(FName(TEXT("hand_l")), EBoneSpaces::WorldSpace);
				FVector BarrelDir = (HandL_Pos - HandR_Pos).GetSafeNormal();
				const float HandDist = FVector::Dist(HandR_Pos, HandL_Pos);
				if (HandDist < 15.0f || HandDist > 55.0f || BarrelDir.IsNearlyZero())
				{
					BarrelDir = GetActorForwardVector();
				}
				const FVector CharUp = GetActorUpVector();
				FVector RifleRight = FVector::CrossProduct(CharUp, BarrelDir).GetSafeNormal();
				if (RifleRight.IsNearlyZero())
				{
					RifleRight = GetActorRightVector();
				}
				const FVector RifleUp = FVector::CrossProduct(BarrelDir, RifleRight).GetSafeNormal();
				const FQuat RifleWorldQuat = FRotationMatrix::MakeFromXZ(BarrelDir, RifleUp).ToQuat();

				Weapon->SetActorLocationAndRotation(HandR_Pos, RifleWorldQuat);
			}
		}

		if (OperatorGearComponent && !bIsDead)
		{
			OperatorGearComponent->UpdateGearTransforms();
		}

		const FQuat GearWorldQuat = MeshComp->GetComponentQuat();
		if (PlateCarrierProcMesh && !bIsDead)
		{
			const FVector SpineWorldLoc = MeshComp->GetBoneLocation(FName(TEXT("spine_03")), EBoneSpaces::WorldSpace);
			PlateCarrierProcMesh->SetWorldLocationAndRotation(SpineWorldLoc, GearWorldQuat);
			PlateCarrierProcMesh->SetWorldScale3D(bIsArmorPlateShattered ? FVector(0.92f, 0.80f, 0.92f) : FVector(1.0f, 1.0f, 1.0f));
		}
		if (BallisticHelmetProcMesh && !bIsHelmetPopped && !bIsDead)
		{
			const FVector HeadWorldLoc = MeshComp->GetBoneLocation(FName(TEXT("head")), EBoneSpaces::WorldSpace);
			BallisticHelmetProcMesh->SetWorldLocationAndRotation(HeadWorldLoc, GearWorldQuat);
			BallisticHelmetProcMesh->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
		}
	}
}

void AFPSCharacter::UpdateThirdPersonCamera(float DeltaTime)
{
	if (!FollowCamera || !CameraBoom || bIsDead)
	{
		return;
	}

	const bool bIsAiming = (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights());

	// Smoothly interpolate ShoulderSideMultiplier between +1.0f (Right Shoulder) and -1.0f (Left Shoulder) at speed 10.0f
	const float TargetShoulderSide = bIsRightShoulder ? 1.0f : -1.0f;
	ShoulderSideMultiplier = FMath::FInterpTo(ShoulderSideMultiplier, TargetShoulderSide, DeltaTime, 10.0f);

	float TargetArmLength = DefaultArmLength;
	float TargetFOV = DefaultFOV;
	FVector TargetSocketOffset = FVector(
		HipfireShoulderOffset.X,
		HipfireShoulderOffset.Y * ShoulderSideMultiplier,
		HipfireShoulderOffset.Z
	);

	if (bIsAiming)
	{
		if (bUseFirstPersonOpticADS && !IsBallisticDiving())
		{
			// Hybrid 1st-Person True Optic Red-Dot ADS (TargetArmLength = 0.0f)
			TargetArmLength = 0.0f;
			TargetFOV = 55.0f;
			TargetSocketOffset = FVector(28.0f, 12.0f * ShoulderSideMultiplier, 4.0f);
		}
		else
		{
			// Tight Over-The-Shoulder ADS Zoom (ArmLength = 95.0f, Offset = (0, 34*Side, 16), FOV = 62.0f)
			TargetArmLength = ADSArmLength;
			TargetFOV = ADSFOV;
			TargetSocketOffset = FVector(
				ADSShoulderOffset.X,
				ADSShoulderOffset.Y * ShoulderSideMultiplier,
				ADSShoulderOffset.Z
			);
		}
	}
	else if (IsBallisticDiving())
	{
		// Slightly wider cinematic framing during Max Payne ballistic dive
		TargetArmLength = 225.0f;
		TargetFOV = 94.0f;
	}

	// Apply crisp 180ms FOV micro-punch (+8.0 deg smoothly snapping back) on Quantum Swap
	if (FOVPunchTimer > 0.0f)
	{
		const float PunchAlpha = FOVPunchTimer / 0.18f;
		const float MicroPunch = 8.0f * FMath::Sin(PunchAlpha * PI);
		TargetFOV += MicroPunch;
	}

	// CQB Culvert Sluice Camera Framing: Short upward trace (+270.0f cm) above the character
	bool bInCulvertSluice = false;
	FHitResult OverheadHit;
	FCollisionQueryParams OverheadParams(TEXT("CulvertOverheadTrace"), false, this);
	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart + FVector(0.0f, 0.0f, 270.0f);
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(OverheadHit, TraceStart, TraceEnd, ECC_Visibility, OverheadParams))
	{
		UPrimitiveComponent* HitComp = OverheadHit.GetComponent();
		AActor* HitActor = OverheadHit.GetActor();
		if ((HitComp && (HitComp->ComponentTags.Contains(FName("Culvert_Ceiling")) ||
		                 HitComp->ComponentTags.Contains(FName("Culvert_Sluice")) ||
		                 HitComp->ComponentTags.Contains(FName("Sluice_Steel")) ||
		                 HitComp->ComponentTags.Contains(FName("Culvert_Wall"))))
			|| (HitActor && (HitActor->IsA(AFPSCoastalSplineActor::StaticClass()) || HitActor->GetName().Contains(TEXT("CoastalSpline")))))
		{
			bInCulvertSluice = true;
		}
	}

	if (bInCulvertSluice && !bIsAiming && !IsBallisticDiving())
	{
		// Smoothly tighten TargetArmLength to 115.0f so OTS camera never clips the culvert roof
		TargetArmLength = 115.0f;
	}

	// Prevent camera probe clipping hitch when transitioning near 0.0f arm length (1st person optic)
	if (CameraBoom)
	{
		CameraBoom->bDoCollisionTest = (CameraBoom->TargetArmLength > 45.0f);
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, 14.0f);
		CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, 14.0f);
	}
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, 18.0f));

	// Competitive Tactical Post-Process Management (Depth of Field & Motion Blur)
	if (bIsAiming)
	{
		// Subtle ADS Depth of Field: Forward camera raycast to dynamically track reticle focus distance
		FHitResult FocusHit;
		FCollisionQueryParams FocusParams;
		FocusParams.AddIgnoredActor(this);
		const FVector CamLoc = FollowCamera->GetComponentLocation();
		const FVector CamFwd = FollowCamera->GetForwardVector();
		float HitDistance = 2000.0f;
		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(FocusHit, CamLoc, CamLoc + (CamFwd * 50000.0f), ECC_Visibility, FocusParams))
		{
			HitDistance = FocusHit.Distance;
		}
		const float TargetFocalDist = FMath::Clamp(HitDistance, 400.0f, 10000.0f);
		FollowCamera->PostProcessSettings.DepthOfFieldFocalDistance = FMath::FInterpTo(
			FollowCamera->PostProcessSettings.DepthOfFieldFocalDistance,
			TargetFocalDist,
			DeltaTime,
			15.0f
		);
		FollowCamera->PostProcessSettings.DepthOfFieldFstop = 9.0f; // Subtle peripheral softness; NEVER drop below 8.0f
	}
	else
	{
		// Hipfire / OTS: 100% sharp from operator to horizon
		FollowCamera->PostProcessSettings.DepthOfFieldFstop = 32.0f;
	}

	// Micro-Capped Tactical Motion Blur (0.18f standard, 0.28f during Sprint/Slide/Dive)
	const bool bHighSpeedMotion = (FPSMovementComponent && (FPSMovementComponent->IsTacticalSprinting() || FPSMovementComponent->IsSliding())) || IsBallisticDiving();
	const float TargetMotionBlur = bHighSpeedMotion ? 0.28f : 0.18f;
	FollowCamera->PostProcessSettings.MotionBlurAmount = FMath::FInterpTo(
		FollowCamera->PostProcessSettings.MotionBlurAmount,
		TargetMotionBlur,
		DeltaTime,
		12.0f
	);

	// Safety Clamps in Tick()
	FollowCamera->PostProcessSettings.BloomIntensity = FMath::Clamp(FollowCamera->PostProcessSettings.BloomIntensity, 0.0f, 0.40f);
	FollowCamera->PostProcessSettings.MotionBlurAmount = FMath::Clamp(FollowCamera->PostProcessSettings.MotionBlurAmount, 0.0f, 0.28f);
	FollowCamera->PostProcessSettings.MotionBlurMax = FMath::Clamp(FollowCamera->PostProcessSettings.MotionBlurMax, 0.0f, 2.5f);
	FollowCamera->PostProcessSettings.DepthOfFieldFstop = FMath::Clamp(FollowCamera->PostProcessSettings.DepthOfFieldFstop, 8.0f, 32.0f);
}

void AFPSCharacter::UpdateBallisticDivePosture(float DeltaTime)
{
	if (!GetMesh() || bIsDead)
	{
		return;
	}

	FRotator TargetMeshRot = FRotator(0.0f, -90.0f, 0.0f);
	float TargetZOffset = -96.0f;

	if (FPSMovementComponent && FPSMovementComponent->IsBallisticDiving())
	{
		const FVector DiveDir2D = FPSMovementComponent->GetBallisticDiveDirection().GetSafeNormal2D();
		const FVector CharForward2D = GetActorForwardVector().GetSafeNormal2D();
		const FVector CharRight2D = GetActorRightVector().GetSafeNormal2D();

		const float ForwardDot = FVector::DotProduct(DiveDir2D, CharForward2D);
		const float RightDot = FVector::DotProduct(DiveDir2D, CharRight2D);

		const float DiveTiltDeg = FPSMovementComponent->IsInDiveLandingSlide() ? 78.0f : 72.0f;
		const float TargetRoll = -ForwardDot * DiveTiltDeg;
		const float TargetPitch = RightDot * DiveTiltDeg;

		TargetMeshRot = FRotator(TargetPitch, -90.0f, TargetRoll);
		TargetZOffset = FPSMovementComponent->IsInDiveLandingSlide() ? -68.0f : -58.0f;
	}
	else if (FPSMovementComponent && FPSMovementComponent->IsSliding())
	{
		const float AimPitch = FRotator::NormalizeAxis(GetControlRotation().Pitch);
		TargetMeshRot = FRotator(0.0f, -90.0f, 18.0f + FMath::Clamp(AimPitch * 0.25f, -15.0f, 15.0f));
		TargetZOffset = -42.0f;
	}
	else if (FPSMovementComponent && FPSMovementComponent->IsCrouching())
	{
		const float AimPitch = FRotator::NormalizeAxis(GetControlRotation().Pitch);
		TargetMeshRot = FRotator(0.0f, -90.0f, -6.0f + FMath::Clamp(AimPitch * 0.35f, -20.0f, 20.0f));
		TargetZOffset = -48.0f;
	}
	else
	{
		// Standing posture: Forward yaw = +90 deg strictly maintained
		const float RawPitch = bIsTacticalBot ? BotAimRotation.Pitch : GetControlRotation().Pitch;
		const float AimPitch = FRotator::NormalizeAxis(RawPitch);
		TargetMeshRot = FRotator(0.0f, -90.0f, FMath::Clamp(AimPitch * 0.55f, -42.0f, 42.0f));
		TargetZOffset = -96.0f;
	}

	if (LimbTraumaFlinchTimer > 0.0f)
	{
		const float TraumaAlpha = FMath::Clamp(LimbTraumaFlinchTimer / 0.35f, 0.0f, 1.0f);
		TargetMeshRot += LimbTraumaRotOffset * TraumaAlpha;
	}

	CurrentDiveMeshRotation = FMath::RInterpTo(CurrentDiveMeshRotation, TargetMeshRot, DeltaTime, 14.0f);
	CurrentDiveMeshZOffset = FMath::FInterpTo(CurrentDiveMeshZOffset, TargetZOffset, DeltaTime, 14.0f);

	GetMesh()->SetRelativeRotation(CurrentDiveMeshRotation);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, CurrentDiveMeshZOffset));
}

void AFPSCharacter::ToggleShoulderSwap()
{
	bIsRightShoulder = !bIsRightShoulder;
}

void AFPSCharacter::ToggleOpticADSMode()
{
	bUseFirstPersonOpticADS = !bUseFirstPersonOpticADS;
}

bool AFPSCharacter::IsAiming() const
{
	return InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights();
}

void AFPSCharacter::OnMiddleMouseButtonPressed()
{
	if (IsAiming())
	{
		ToggleOpticADSMode();
	}
	else
	{
		StartCookingLethal();
	}
}

void AFPSCharacter::OnMiddleMouseButtonReleased()
{
	if (!IsAiming())
	{
		ReleaseLethal();
	}
}



bool AFPSCharacter::IsBallisticDiving() const
{
	return FPSMovementComponent && FPSMovementComponent->IsBallisticDiving();
}

bool AFPSCharacter::IsFermiTimeBubbleActive() const
{
	return ActiveTimeBubble && IsValid(ActiveTimeBubble) && ActiveTimeBubble->GetRemainingTime() > 0.0f;
}

float AFPSCharacter::GetFermiTimeBubbleRemaining() const
{
	return (ActiveTimeBubble && IsValid(ActiveTimeBubble)) ? ActiveTimeBubble->GetRemainingTime() : 0.0f;
}

bool AFPSCharacter::IsFermiDecoyActive() const
{
	return ActiveFermiDecoy && IsValid(ActiveFermiDecoy) && ActiveFermiDecoy->GetRemainingLifetime() > 0.0f;
}

float AFPSCharacter::GetFermiDecoyRemaining() const
{
	return (ActiveFermiDecoy && IsValid(ActiveFermiDecoy)) ? ActiveFermiDecoy->GetRemainingLifetime() : 0.0f;
}

void AFPSCharacter::TriggerBallisticDive()
{
	if (!FPSMovementComponent || bIsDead)
	{
		return;
	}

	// Determine desired world dive vector from WASD input (or forward vector if neutral)
	FVector DesiredDiveDir = (GetActorForwardVector() * CurrentMoveInput.X) + (GetActorRightVector() * CurrentMoveInput.Y);
	if (DesiredDiveDir.SizeSquared2D() < 0.05f)
	{
		DesiredDiveDir = GetVelocity().Size2D() > 30.0f ? GetVelocity().GetSafeNormal2D() : GetActorForwardVector();
	}

	if (FPSMovementComponent->TryStartBallisticDive(DesiredDiveDir))
	{
		UWorld* World = GetWorld();
		if (World)
		{
			if (ActiveTimeBubble && IsValid(ActiveTimeBubble))
			{
				ActiveTimeBubble->Destroy();
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = this;
			ActiveTimeBubble = World->SpawnActor<AFPSFermiTimeBubble>(AFPSFermiTimeBubble::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
			if (ActiveTimeBubble)
			{
				ActiveTimeBubble->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}

			// If an Acoustic Fermi Echo decoy is active on the field, expand a secondary Fermi Time Bubble around the decoy too!
			if (IsFermiDecoyActive())
			{
				ActiveFermiDecoy->SpawnSecondaryTimeBubble();
			}
		}
	}
}

void AFPSCharacter::TriggerFermiEchoOrQuantumSwap()
{
	if (bIsDead)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// CASE 1: Active Fermi Echo exists -> Execute Instant Quantum Phase-Swap!
	if (IsFermiDecoyActive())
	{
		const FVector PlayerOldLoc = GetActorLocation();
		const FRotator PlayerControlRot = GetControlRotation();
		const FVector DecoyLoc = ActiveFermiDecoy->GetActorLocation();

		// Preserve ballistic dive velocity magnitude and re-project along current aim yaw if diving, or keep velocity
		const FVector OldVelocity = GetVelocity();
		const float HorizSpeed = OldVelocity.Size2D();

		// Swap world coordinates
		SetActorLocation(DecoyLoc, false, nullptr, ETeleportType::TeleportPhysics);
		ActiveFermiDecoy->SetActorLocation(PlayerOldLoc, false, nullptr, ETeleportType::TeleportPhysics);

		// Reverse decoy forward run direction so it sprints away from swap point
		ActiveFermiDecoy->SetSprintDirection(-PlayerControlRot.Vector().GetSafeNormal2D());

		// Bring active Fermi Time Bubble along with the player to slow down the baited enemy!
		if (ActiveTimeBubble && IsValid(ActiveTimeBubble))
		{
			ActiveTimeBubble->SetActorLocation(DecoyLoc, false, nullptr, ETeleportType::TeleportPhysics);
			ActiveTimeBubble->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}

		if (FPSMovementComponent)
		{
			if (FPSMovementComponent->IsBallisticDiving())
			{
				const FVector NewDiveDir = PlayerControlRot.Vector().GetSafeNormal2D();
				FPSMovementComponent->Velocity = NewDiveDir * FMath::Max(HorizSpeed, 680.0f) + FVector(0.0f, 0.0f, FMath::Max(OldVelocity.Z, 180.0f));
			}
			else
			{
				FPSMovementComponent->Velocity = OldVelocity;
			}
		}

		// Apply crisp 180ms FOV micro-punch (+8.0 deg smoothly snapping back)
		FOVPunchTimer = 0.18f;

		// 3.0s window for FERMI PHANTOM accolade and FermiSwapKill bounty on subsequent elimination
		QuantumSwapKillWindowRemaining = 3.0f;

		// Trigger Quantum Phase-Swap visor flash on HUD
		QuantumSwapFlashTimer = 0.35f;
		return;
	}

	// CASE 2: No active Fermi Echo -> Deploy Autonomous Acoustic Hologram Clone (if off cooldown)
	if (FermiDecoyCooldownRemaining > 0.0f)
	{
		return;
	}

	FVector DeployDir = GetControlRotation().Vector().GetSafeNormal2D();
	if (DeployDir.IsNearlyZero())
	{
		DeployDir = GetActorForwardVector().GetSafeNormal2D();
	}

	// Offset slightly toward the active shoulder side so the decoy sprints out past cover
	const FVector SpawnLoc = GetActorLocation() + DeployDir * 65.0f + GetActorRightVector() * (35.0f * ShoulderSideMultiplier);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ActiveFermiDecoy = World->SpawnActor<AFPSFermiDecoyActor>(AFPSFermiDecoyActor::StaticClass(), SpawnLoc, DeployDir.Rotation(), SpawnParams);
	if (ActiveFermiDecoy)
	{
		EFPSPlayerTeam MyTeam = EFPSPlayerTeam::Alpha;
		if (AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>())
		{
			MyTeam = PS->GetPlayerTeam();
		}
		ActiveFermiDecoy->InitializeDecoy(DeployDir, MyTeam);
		FermiDecoyCooldownRemaining = 9.0f; // 9.0 second cooldown

		// If the player is already mid-dive when deploying the decoy, immediately project its secondary time bubble
		if (IsBallisticDiving())
		{
			ActiveFermiDecoy->SpawnSecondaryTimeBubble();
		}
	}
}

void AFPSCharacter::HandleOnDeath(AActor* KillerActor)
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	StopFire();
	bBotIsFiringBurst = false;

	CustomTimeDilation = 1.0f;
	if (ActiveTimeBubble && IsValid(ActiveTimeBubble))
	{
		ActiveTimeBubble->Destroy();
		ActiveTimeBubble = nullptr;
	}
	if (ActiveFermiDecoy && IsValid(ActiveFermiDecoy))
	{
		ActiveFermiDecoy->Destroy();
		ActiveFermiDecoy = nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		DisableInput(PC);
	}

	if (FPSMovementComponent)
	{
		FPSMovementComponent->DisableMovement();
		FPSMovementComponent->StopMovementImmediately();
	}

	// Smoothly attach CameraBoom to pelvis to track ragdoll death without jarring snap
	if (!bIsTacticalBot && CameraBoom && GetMesh())
	{
		CameraBoom->bEnableCameraLag = true;
		CameraBoom->CameraLagSpeed = 6.0f;
		CameraBoom->TargetArmLength = 240.0f;
		CameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepWorldTransform, TEXT("pelvis"));
	}

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (GetMesh())
	{
		GetMesh()->SetOwnerNoSee(false);
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetAllBodiesSimulatePhysics(true);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;
	}

	// Notify GameMode for scoring, killfeed, and bot/player respawn scheduling
	if (UWorld* World = GetWorld())
	{
		if (AFPSMultiplayerGameMode* GM = World->GetAuthGameMode<AFPSMultiplayerGameMode>())
		{
			if (bIsTacticalBot)
			{
				GM->HandleBotKilled(this, KillerActor);
			}
			else
			{
				AController* KillerCtrl = nullptr;
				if (APawn* KillerPawn = Cast<APawn>(KillerActor))
				{
					KillerCtrl = KillerPawn->GetController();
				}
				GM->NotifyPlayerKilled(KillerCtrl, GetController(), TEXT("NF-98 CARBINE"), false);
			}
		}
	}
}

void AFPSCharacter::HandleOnDamageTaken(float Damage, AActor* Attacker, const FHitResult& HitResult, bool bIsHeadshot, bool bShieldBroken)
{
	if (ActiveHUDWidget)
	{
		FVector ThreatSource = Attacker ? Attacker->GetActorLocation() : FVector(HitResult.ImpactPoint);
		if (ThreatSource.IsNearlyZero())
		{
			ThreatSource = FVector(HitResult.TraceStart);
		}
		if (ThreatSource.IsNearlyZero())
		{
			ThreatSource = GetActorLocation() - GetControlRotation().Vector() * 500.0f;
		}

		ActiveHUDWidget->AddDirectionalIndicator(ThreatSource, FMath::Clamp(Damage / 25.0f, 1.0f, 2.5f), EThreatIndicatorType::Damage);
	}

	FVector ShotDir = (HitResult.TraceEnd - HitResult.TraceStart).GetSafeNormal();
	if (ShotDir.IsNearlyZero() && Attacker)
	{
		ShotDir = (GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal();
	}

	// 1. Level IV Ceramic Plate Shatter (spine_02 / spine_03 shield depleted from >0 to 0)
	if (bShieldBroken && !bIsArmorPlateShattered)
	{
		const FVector ImpactLoc = HitResult.ImpactPoint.IsNearlyZero()
			? (GetActorLocation() + FVector(0.0f, 0.0f, 35.0f))
			: FVector(HitResult.ImpactPoint);
		MulticastTriggerArmorPlateShatter(ShotDir, ImpactLoc);

		if (AFPSCharacter* AttackerChar = Cast<AFPSCharacter>(Attacker))
		{
			AttackerChar->AwardCombatAccolade(TEXT("PLATE SHATTERED (+75 GOLD)"), 75, FLinearColor(0.0f, 0.65f, 1.0f));
			AttackerChar->NotifyBountyEvent(FName(TEXT("PlateShatter")), 1);
		}
	}

	// 2. Lethal Headshot Ballistic Helmet Pop
	if (bIsHeadshot && HealthComponent && HealthComponent->IsDead() && !bIsHelmetPopped)
	{
		MulticastPopBallisticHelmet(ShotDir);
	}

	// 3. Limb Trauma Flinch Read
	const EFPSBodyDamageZone Zone = FFPSHumanSkeleton::GetDamageZoneForBone(HitResult.BoneName);
	if (Zone == EFPSBodyDamageZone::UpperArms || Zone == EFPSBodyDamageZone::LowerArms || Zone == EFPSBodyDamageZone::Hands ||
	    Zone == EFPSBodyDamageZone::Thighs || Zone == EFPSBodyDamageZone::Calves || Zone == EFPSBodyDamageZone::Feet)
	{
		LimbTraumaFlinchTimer = 0.35f;
		LimbTraumaRotOffset = FRotator(FMath::RandRange(-14.0f, 14.0f), 0.0f, FMath::RandRange(-16.0f, 16.0f));
	}
}

void AFPSCharacter::ToggleEscapeMenu()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (!bIsEscapeMenuOpen)
	{
		// === OPEN ESCAPE MENU ===
		bIsEscapeMenuOpen = true;

		// 1. Immediately halt weapon fire
		StopFire();

		// 2. Instantiate or show Escape Menu Widget
		if (!ActiveEscapeMenuWidget)
		{
			TSubclassOf<UFPSEscapeMenuWidget> WidgetClass = EscapeMenuWidgetClass;
			if (!WidgetClass)
			{
				WidgetClass = UFPSEscapeMenuWidget::StaticClass();
			}
			ActiveEscapeMenuWidget = CreateWidget<UFPSEscapeMenuWidget>(PC, WidgetClass);
			if (ActiveEscapeMenuWidget)
			{
				ActiveEscapeMenuWidget->SetOwningCharacter(this);
				ActiveEscapeMenuWidget->AddToViewport(100);
			}
		}
		else
		{
			ActiveEscapeMenuWidget->SetVisibility(ESlateVisibility::Visible);
		}

		// 3. Enable mouse cursor and block character look & move input
		PC->bShowMouseCursor = true;
		PC->SetIgnoreLookInput(true);
		PC->SetIgnoreMoveInput(true);

		// 4. Set input mode to Game & UI without pausing the game clock
		FInputModeGameAndUI InputMode;
		if (ActiveEscapeMenuWidget)
		{
			InputMode.SetWidgetToFocus(ActiveEscapeMenuWidget->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
	else
	{
		// === CLOSE ESCAPE MENU ===
		bIsEscapeMenuOpen = false;

		if (ActiveEscapeMenuWidget)
		{
			ActiveEscapeMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		PC->bShowMouseCursor = false;
		PC->ResetIgnoreLookInput();
		PC->ResetIgnoreMoveInput();

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

void AFPSCharacter::UpdateProceduralCameraBob(float DeltaTime)
{
	if (!FPSMovementComponent || !FPSMovementComponent->IsMovingOnGround())
	{
		if (FirstPersonCamera)
		{
			FirstPersonCamera->SetRelativeLocation(FMath::VInterpTo(FirstPersonCamera->GetRelativeLocation(), DefaultCameraPos, DeltaTime, 8.0f));
		}
		return;
	}

	float Speed = GetVelocity().Size2D();
	if (Speed > 10.0f && FirstPersonCamera)
	{
		float Frequency = (FPSMovementComponent->IsTacticalSprinting() ? 12.0f : 8.0f);
		float AmplitudeZ = (FPSMovementComponent->IsTacticalSprinting() ? 3.5f : 1.8f);
		float AmplitudeY = (FPSMovementComponent->IsTacticalSprinting() ? 2.5f : 1.2f);

		BobTimer += DeltaTime * Frequency;
		float BobZ = FMath::Sin(BobTimer) * AmplitudeZ;
		float BobY = FMath::Cos(BobTimer * 0.5f) * AmplitudeY;

		FVector TargetPos = DefaultCameraPos + FVector(0.0f, BobY, BobZ);
		FirstPersonCamera->SetRelativeLocation(FMath::VInterpTo(FirstPersonCamera->GetRelativeLocation(), TargetPos, DeltaTime, 12.0f));
	}
	else if (FirstPersonCamera)
	{
		FirstPersonCamera->SetRelativeLocation(FMath::VInterpTo(FirstPersonCamera->GetRelativeLocation(), DefaultCameraPos, DeltaTime, 8.0f));
	}
}

void AFPSCharacter::UpdateWeaponSway(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !Mesh1P) return;

	FRotator CurrentRot = PC->GetControlRotation();
	FRotator DeltaRot = CurrentRot - LastControlRotation;
	DeltaRot.Normalize();
	LastControlRotation = CurrentRot;

	// Scale sway amplitude if arms are injured
	const float ArmTraumaMultiplier = (BodyMechanicsComponent && BodyMechanicsComponent->IsArmInjured()) ? 1.75f : 1.0f;

	// Sway lag
	FRotator TargetSway = FRotator(-DeltaRot.Pitch * 1.5f * ArmTraumaMultiplier, DeltaRot.Yaw * 1.5f * ArmTraumaMultiplier, DeltaRot.Yaw * 0.8f);
	CurrentWeaponSwayRot = FMath::RInterpTo(CurrentWeaponSwayRot, TargetSway, DeltaTime, 10.0f);

	// Blend weapon sway with flinch aim punch and procedural spine lean roll
	const FRotator AimPunch = BodyMechanicsComponent ? BodyMechanicsComponent->GetCurrentAimPunch() : FRotator::ZeroRotator;
	const FRotator SpineLean = BodyMechanicsComponent ? BodyMechanicsComponent->GetProceduralSpineRotation() : FRotator::ZeroRotator;

	Mesh1P->SetRelativeRotation(CurrentWeaponSwayRot + AimPunch + FRotator(0.0f, 0.0f, SpineLean.Roll * 0.4f));
}

void AFPSCharacter::UpdateLocomotionAnimation(float DeltaTime)
{
	if (!GetMesh())
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	UAnimSequence* TargetAnim = AnimIdleADS;
	float PlayRate = 1.0f;

	// Evaluate directional heading relative to character forward facing
	const FVector Velocity2D = GetVelocity().GetSafeNormal2D();
	const FVector Forward2D = GetActorForwardVector().GetSafeNormal2D();
	const float ForwardDot = Velocity2D.IsNearlyZero() ? 1.0f : FVector::DotProduct(Velocity2D, Forward2D);
	const bool bIsMovingBackwards = (ForwardDot < -0.35f);

	if (IsBallisticDiving())
	{
		// Lock upper body and hands into high-ready aimed posture during mid-air ballistic dive
		TargetAnim = AnimIdleADS;
		PlayRate = 0.5f;
	}
	else if (FPSMovementComponent && FPSMovementComponent->IsFalling())
	{
		// Airborne jump tuck / apex suspension holding weapon stabilized
		TargetAnim = AnimJumpLoop ? AnimJumpLoop : AnimIdleADS;
		PlayRate = 1.0f;
	}
	else if (FPSMovementComponent && FPSMovementComponent->IsSliding())
	{
		// High-speed tactical knee slide
		TargetAnim = AnimSlide ? AnimSlide : AnimCrouchIdle;
		PlayRate = FMath::Clamp(Speed / 550.0f, 0.85f, 1.45f);
	}
	else if (FPSMovementComponent && FPSMovementComponent->IsCrouching())
	{
		if (Speed > 15.0f)
		{
			TargetAnim = AnimCrouchWalk ? AnimCrouchWalk : (AnimWalk ? AnimWalk : AnimIdleADS);
			PlayRate = FMath::Clamp(Speed / 220.0f, 0.75f, 1.35f);
		}
		else
		{
			TargetAnim = AnimCrouchIdle ? AnimCrouchIdle : (AnimLowReady ? AnimLowReady : AnimIdleADS);
			PlayRate = 1.0f;
		}
	}
	else if (FPSMovementComponent && FPSMovementComponent->IsTacticalSprinting() && Speed > 280.0f)
	{
		TargetAnim = AnimSprint ? AnimSprint : AnimRun;
		PlayRate = FMath::Clamp(Speed / 520.0f, 0.85f, 1.35f);
	}
	else if (Speed > 260.0f)
	{
		if (bIsMovingBackwards)
		{
			if (AnimWalkBackwards)
			{
				TargetAnim = AnimWalkBackwards;
				PlayRate = FMath::Clamp(Speed / 260.0f, 0.85f, 1.35f);
			}
			else
			{
				TargetAnim = AnimRun ? AnimRun : AnimWalk;
				PlayRate = -FMath::Clamp(Speed / 260.0f, 0.75f, 1.25f);
			}
		}
		else
		{
			TargetAnim = AnimRun ? AnimRun : AnimWalk;
			PlayRate = FMath::Clamp(Speed / 340.0f, 0.85f, 1.25f);
		}
	}
	else if (Speed > 15.0f)
	{
		if (bIsMovingBackwards)
		{
			if (AnimWalkBackwards)
			{
				TargetAnim = AnimWalkBackwards;
				PlayRate = FMath::Clamp(Speed / 180.0f, 0.75f, 1.25f);
			}
			else
			{
				TargetAnim = AnimWalk ? AnimWalk : AnimIdleADS;
				PlayRate = -FMath::Clamp(Speed / 180.0f, 0.75f, 1.25f);
			}
		}
		else
		{
			TargetAnim = AnimWalk ? AnimWalk : (IsAiming() ? AnimIdleADS : (AnimLowReady ? AnimLowReady : AnimIdleADS));
			PlayRate = FMath::Clamp(Speed / 180.0f, 0.75f, 1.25f);
		}
	}
	else
	{
		// Stationary stance: Aimed ADS when aiming, Low Ready when idle/hipfire
		if (IsAiming())
		{
			TargetAnim = AnimIdleADS;
			PlayRate = 1.0f;
		}
		else
		{
			TargetAnim = AnimLowReady ? AnimLowReady : AnimIdleADS;
			PlayRate = 1.0f;
		}
	}

	// Authoritative high-quality mocap locomotion animation playback
	if (TargetAnim && TargetAnim != CurrentLocomotionAnim)
	{
		CurrentLocomotionAnim = TargetAnim;
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->SetAnimation(TargetAnim);
		GetMesh()->SetPlayRate(PlayRate);
		GetMesh()->Play(true);
	}
	else if (CurrentLocomotionAnim)
	{
		GetMesh()->SetPlayRate(PlayRate);
	}
}

UFPSAnimInstance* AFPSCharacter::GetFPSAnimInstance() const
{
	return GetMesh() ? Cast<UFPSAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
}

AFPSWeaponBase* AFPSCharacter::GetCurrentWeapon() const
{
	return InventoryComponent ? InventoryComponent->GetActiveWeapon() : nullptr;
}

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AFPSCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AFPSCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AFPSCharacter::LookUp);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AFPSCharacter::OnJumpOrDivePressed);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AFPSCharacter::StartFire);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AFPSCharacter::StopFire);

	PlayerInputComponent->BindAction(TEXT("ADS"), IE_Pressed, this, &AFPSCharacter::StartADS);
	PlayerInputComponent->BindAction(TEXT("ADS"), IE_Released, this, &AFPSCharacter::StopADS);

	PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AFPSCharacter::Reload);

	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AFPSCharacter::StartSprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AFPSCharacter::StopSprint);

	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AFPSCharacter::StartCrouchOrSlide);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Released, this, &AFPSCharacter::StopCrouchOrSlide);

	PlayerInputComponent->BindAction(TEXT("ShoulderSwap"), IE_Pressed, this, &AFPSCharacter::ToggleShoulderSwap);

	// The Fermi Paradox Engine Bindings
	PlayerInputComponent->BindAction(TEXT("BallisticDive"), IE_Pressed, this, &AFPSCharacter::TriggerBallisticDive);
	PlayerInputComponent->BindKey(EKeys::LeftAlt, IE_Pressed, this, &AFPSCharacter::TriggerBallisticDive);
	PlayerInputComponent->BindAction(TEXT("Tactical"), IE_Pressed, this, &AFPSCharacter::TriggerFermiEchoOrQuantumSwap);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AFPSCharacter::TriggerFermiEchoOrQuantumSwap);
	PlayerInputComponent->BindAction(TEXT("Lethal"), IE_Pressed, this, &AFPSCharacter::StartCookingLethal);
	PlayerInputComponent->BindAction(TEXT("Lethal"), IE_Released, this, &AFPSCharacter::ReleaseLethal);
	PlayerInputComponent->BindAction(TEXT("ToggleOpticADS"), IE_Pressed, this, &AFPSCharacter::OnMiddleMouseButtonPressed);
	PlayerInputComponent->BindAction(TEXT("ToggleOpticADS"), IE_Released, this, &AFPSCharacter::OnMiddleMouseButtonReleased);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AFPSCharacter::OnMiddleMouseButtonPressed);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AFPSCharacter::OnMiddleMouseButtonReleased);

	// Live Escape Menu Input Bindings (Escape and P for Editor PIE)
	PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AFPSCharacter::ToggleEscapeMenu);
	PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &AFPSCharacter::ToggleEscapeMenu);
	PlayerInputComponent->BindAction(TEXT("ToggleEscapeMenu"), IE_Pressed, this, &AFPSCharacter::ToggleEscapeMenu);
	PlayerInputComponent->BindAction(TEXT("Pause"), IE_Pressed, this, &AFPSCharacter::ToggleEscapeMenu);
}

void AFPSCharacter::MoveForward(float Value)
{
	CurrentMoveInput.X = Value;
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AFPSCharacter::MoveRight(float Value)
{
	CurrentMoveInput.Y = Value;
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AFPSCharacter::Turn(float Rate)
{
	float SensMod = MouseSensitivityMultiplier;
	if (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights())
	{
		SensMod *= 0.65f;
	}
	// Preserve crisp 1:1 mouse aiming responsiveness even when CustomTimeDilation = 0.55f inside the Fermi Time Bubble
	const float DilationComp = 1.0f / FMath::Max(0.15f, CustomTimeDilation);
	AddControllerYawInput(Rate * SensMod * DilationComp);
}

void AFPSCharacter::LookUp(float Rate)
{
	float SensMod = MouseSensitivityMultiplier;
	if (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights())
	{
		SensMod *= 0.65f;
	}
	const float DilationComp = 1.0f / FMath::Max(0.15f, CustomTimeDilation);
	AddControllerPitchInput(Rate * SensMod * DilationComp);
}

void AFPSCharacter::StartFire()
{
	if (InventoryComponent && InventoryComponent->GetActiveWeapon())
	{
		InventoryComponent->GetActiveWeapon()->StartFire();
	}
}

void AFPSCharacter::StopFire()
{
	if (InventoryComponent && InventoryComponent->GetActiveWeapon())
	{
		InventoryComponent->GetActiveWeapon()->StopFire();
	}
}

void AFPSCharacter::StartADS()
{
	if (InventoryComponent && InventoryComponent->GetActiveWeapon())
	{
		InventoryComponent->GetActiveWeapon()->SetADS(true);
	}
}

void AFPSCharacter::StopADS()
{
	if (InventoryComponent && InventoryComponent->GetActiveWeapon())
	{
		InventoryComponent->GetActiveWeapon()->SetADS(false);
	}
}

void AFPSCharacter::Reload()
{
	if (InventoryComponent && InventoryComponent->GetActiveWeapon())
	{
		InventoryComponent->GetActiveWeapon()->Reload();
	}
}

void AFPSCharacter::StartSprint()
{
	if (BodyMechanicsComponent && BodyMechanicsComponent->IsLegInjured())
	{
		// Severely injured legs prevent sprint
		return;
	}

	if (FPSMovementComponent)
	{
		FPSMovementComponent->SetWantsToSprint(true);
	}
}

void AFPSCharacter::StopSprint()
{
	if (FPSMovementComponent)
	{
		FPSMovementComponent->SetWantsToSprint(false);
	}
}

void AFPSCharacter::StartCrouchOrSlide()
{
	bIsHoldingCrouchKey = true;

	UWorld* World = GetWorld();
	const float Now = World ? World->GetRealTimeSeconds() : 0.0f;
	const bool bIsDoubleTapCrouch = (Now - LastCrouchPressTime) <= 0.32f;
	LastCrouchPressTime = Now;

	// Double-tap C while moving in any WASD direction triggers Omnidirectional Ballistic Dive!
	if (bIsDoubleTapCrouch && (CurrentMoveInput.SizeSquared() > 0.05f || GetVelocity().Size2D() > 120.0f))
	{
		TriggerBallisticDive();
		return;
	}

	if (FPSMovementComponent)
	{
		if (FPSMovementComponent->IsTacticalSprinting())
		{
			FPSMovementComponent->StartSlide();
		}
		else
		{
			Crouch();
		}
	}
}

void AFPSCharacter::StopCrouchOrSlide()
{
	bIsHoldingCrouchKey = false;
	if (FPSMovementComponent)
	{
		if (FPSMovementComponent->IsSliding())
		{
			FPSMovementComponent->StopSlide();
		}
		else
		{
			UnCrouch();
		}
	}
}

void AFPSCharacter::OnJumpOrDivePressed()
{
	// Pressing Spacebar while holding LeftControl/C and moving triggers Max Payne Omnidirectional Dive
	if (bIsHoldingCrouchKey && (CurrentMoveInput.SizeSquared() > 0.05f || GetVelocity().Size2D() > 80.0f))
	{
		TriggerBallisticDive();
		return;
	}
	Jump();
}

void AFPSCharacter::StartCookingLethal()
{
	// Optional Toggle from Prompt A: Pressing MiddleMouseButton while aiming toggles between 3rd-Person Tight OTS ADS and 1st-Person True Optic ADS
	const bool bIsAiming = (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights());
	if (bIsAiming)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC && PC->WasInputKeyJustPressed(EKeys::MiddleMouseButton))
		{
			bUseFirstPersonOpticADS = !bUseFirstPersonOpticADS;
			return;
		}
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->StartCookingLethal();
	}
}

void AFPSCharacter::ReleaseLethal()
{
	if (EquipmentComponent && EquipmentComponent->IsCookingFrag())
	{
		EquipmentComponent->ReleaseAndThrowLethal();
	}
}

void AFPSCharacter::ThrowTactical()
{
	TriggerFermiEchoOrQuantumSwap();
}

void AFPSCharacter::TriggerSurfaceFootstep(float VolumeScale)
{
	if (bIsDead || !GetWorld()) return;

	FHitResult GroundHit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	TraceParams.bReturnPhysicalMaterial = true;

	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 130.0f);

	FVector FootstepLocation = TraceStart - FVector(0.0f, 0.0f, 90.0f);
	FString MatName = TEXT("");

	if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
	{
		FootstepLocation = GroundHit.ImpactPoint;
		if (GroundHit.PhysMaterial.IsValid())
		{
			MatName = GroundHit.PhysMaterial->GetName().ToLower();
		}
		if (GroundHit.Component.IsValid())
		{
			if (GroundHit.Component->ComponentTags.Contains(TEXT("Wood"))) MatName = TEXT("wood");
			else if (GroundHit.Component->ComponentTags.Contains(TEXT("Sand"))) MatName = TEXT("sand");
			else if (GroundHit.Component->ComponentTags.Contains(TEXT("Water"))) MatName = TEXT("water");
			else if (GroundHit.Component->ComponentTags.Contains(TEXT("Metal"))) MatName = TEXT("metal");
			else if (GroundHit.Component->ComponentTags.Contains(TEXT("Concrete"))) MatName = TEXT("concrete");
			else if (UMaterialInterface* HitMat = GroundHit.Component->GetMaterial(0))
			{
				MatName = HitMat->GetName().ToLower();
			}
		}
	}

	// Select surface sound array based on traced physical material & texture names
	TArray<TObjectPtr<USoundBase>>* SelectedSounds = &FootstepConcreteSounds;

	if (MatName.Contains(TEXT("wood")) || MatName.Contains(TEXT("plank")) || MatName.Contains(TEXT("timber")) || MatName.Contains(TEXT("board")))
	{
		SelectedSounds = &FootstepWoodSounds;
	}
	else if (MatName.Contains(TEXT("sand")) || MatName.Contains(TEXT("dirt")) || MatName.Contains(TEXT("ground")) || MatName.Contains(TEXT("gravel")) || MatName.Contains(TEXT("mud")) || MatName.Contains(TEXT("rock")) || MatName.Contains(TEXT("grass")))
	{
		SelectedSounds = &FootstepSandSounds;
	}
	else if (MatName.Contains(TEXT("water")) || MatName.Contains(TEXT("puddle")) || MatName.Contains(TEXT("pond")) || MatName.Contains(TEXT("river")) || MatName.Contains(TEXT("ocean")) || MatName.Contains(TEXT("wet")))
	{
		SelectedSounds = &FootstepWaterSounds;
	}
	else if (MatName.Contains(TEXT("metal")) || MatName.Contains(TEXT("iron")) || MatName.Contains(TEXT("steel")) || MatName.Contains(TEXT("grate")) || MatName.Contains(TEXT("vent")))
	{
		SelectedSounds = &FootstepMetalSounds;
	}

	// Fallback to concrete if chosen category is empty
	if (SelectedSounds->Num() == 0)
	{
		SelectedSounds = &FootstepConcreteSounds;
	}

	if (SelectedSounds->Num() > 0)
	{
		FootstepAlternator = (FootstepAlternator + 1) % SelectedSounds->Num();
		USoundBase* SoundToPlay = (*SelectedSounds)[FootstepAlternator];

		if (SoundToPlay)
		{
			// Spatialized footstep with dynamic distance falloff (audible up to 24 meters)
			UFPSAudioSubsystem::PlaySpatialSoundWithFalloff(
				this,
				SoundToPlay,
				FootstepLocation,
				VolumeScale,
				70.0f,
				2400.0f,
				FMath::RandRange(0.96f, 1.04f),
				EFPSAudioChannel::SFX
			);

			// Deaf Accessibility / Visual Audio: Notify local player if footstep originated from another character
			if (UWorld* World = GetWorld())
			{
				if (APlayerController* PC = World->GetFirstPlayerController())
				{
					if (PC->GetPawn() != this)
					{
						if (AFPSCharacter* LocalChar = Cast<AFPSCharacter>(PC->GetPawn()))
						{
							if (UFPSHUDWidget* LocalHUD = LocalChar->GetActiveHUDWidget())
							{
								const float StepDist = FVector::Dist(FootstepLocation, LocalChar->GetActorLocation());
								if (StepDist <= 2400.0f)
								{
									const float StepIntensity = FMath::Clamp(1.0f - (StepDist / 2400.0f), 0.4f, 1.4f) * VolumeScale;
									LocalHUD->AddDirectionalIndicator(FootstepLocation, StepIntensity, EThreatIndicatorType::Footstep);
								}
							}
						}
					}
				}
			}
		}

		// Surface-Aware Acoustic Radar Footstep Detection
		if (GroundHit.Component.IsValid())
		{
			const bool bIsPierWood = GroundHit.Component->ComponentTags.Contains(FName("Surface_Pier_Wood")) ||
			                         GroundHit.Component->ComponentTags.Contains(FName("Pier_Deck")) ||
			                         GroundHit.Component->ComponentTags.Contains(FName("Pier_THead"));
			const bool bIsShallowWater = GroundHit.Component->ComponentTags.Contains(FName("Surface_Shallow_Water"));
			const bool bIsSandDune = GroundHit.Component->ComponentTags.Contains(FName("Surface_Sand_Dune")) ||
			                         GroundHit.Component->ComponentTags.Contains(FName("Dune_SlipFace"));

			const bool bIsSprinting = FPSMovementComponent && FPSMovementComponent->IsTacticalSprinting();

			if (bIsSandDune)
			{
				// Sand dunes completely suppress footstep radar pings (0% acoustic footprint)
				// Backcountry stealth flanking route - no acoustic radar blip emitted!
			}
			else if (bIsSprinting && (bIsPierWood || bIsShallowWater))
			{
				// Sprinting across wooden pier planks or shallow water emits low-radius (1,500cm) acoustic radar ping
				if (UWorld* World = GetWorld())
				{
					if (UFPSRadarSubsystem* Radar = World->GetSubsystem<UFPSRadarSubsystem>())
					{
						EFPSPlayerTeam SourceTeam = EFPSPlayerTeam::Alpha;
						if (AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>())
						{
							SourceTeam = PS->GetPlayerTeam();
						}
						else if (IsTacticalBot())
						{
							SourceTeam = EFPSPlayerTeam::Bravo;
						}
						Radar->ReportAcousticPing(this, FootstepLocation, SourceTeam, 1500.0f, 1.8f);
					}
				}
			}
		}
	}
}

void AFPSCharacter::NotifyEnemyHit(AActor* Victim, const FHitResult& HitInfo, bool bIsHeadshot, bool bShieldBroken, bool bKilled)
{
	if (!ActiveHUDWidget) return;

	// 1. Differentiated Hitmarkers
	if (bKilled)
	{
		ActiveHUDWidget->TriggerHitmarker(EFPSHitmarkerType::Kill);
	}
	else if (bIsHeadshot)
	{
		ActiveHUDWidget->TriggerHitmarker(EFPSHitmarkerType::Headshot);
	}
	else if (bShieldBroken)
	{
		ActiveHUDWidget->TriggerHitmarker(EFPSHitmarkerType::ShieldBreak);
	}
	else
	{
		ActiveHUDWidget->TriggerHitmarker(EFPSHitmarkerType::Standard);
	}

	// 2. Accolades & Combo Registering
	if (bShieldBroken)
	{
		AwardCombatAccolade(TEXT("ANATOMICAL CRITICAL (+125 GOLD)"), 125, FLinearColor(0.0f, 0.65f, 1.0f));
	}
	else if (bIsHeadshot && !bKilled)
	{
		AwardCombatAccolade(TEXT("ANATOMICAL CRITICAL (+125 GOLD)"), 125, FLinearColor(1.0f, 0.702f, 0.0f));
	}

	if (bKilled)
	{
		if (ActiveHUDWidget)
		{
			ActiveHUDWidget->RegisterKillForCombo();
		}

		// Hostile Elimination Accolade & Direct Gold Credit (+100 Gold)
		AwardCombatAccolade(TEXT("HOSTILE ELIMINATED (+100 GOLD)"), 100, FLinearColor(1.0f, 0.84f, 0.0f));

		// Accolade: Chrono Intercept (Killing enemy while inside Fermi Bubble)
		const bool bInBubble = IsFermiTimeBubbleActive() || (GetWorld() && AFPSFermiTimeBubble::IsLocationInsideAnyBubble(GetWorld(), GetActorLocation()));
		if (bInBubble)
		{
			AwardCombatAccolade(TEXT("CHRONO INTERCEPT (+100 GOLD)"), 100, FLinearColor(0.0f, 0.90f, 1.0f));
		}

		// Accolade: Fermi Phantom (Killing enemy within 3.0s of executing Q Fermi Swap)
		if (QuantumSwapKillWindowRemaining > 0.0f)
		{
			AwardCombatAccolade(TEXT("FERMI PHANTOM (+150 GOLD)"), 150, FLinearColor(0.40f, 1.0f, 0.60f));
			NotifyBountyEvent(FName(TEXT("FermiSwapKill")), 1);
		}

		// Chrono-Dive Kill Bounty Hook
		if (IsBallisticDiving())
		{
			AwardCombatAccolade(TEXT("CHRONO DIVE LETHAL (+125 GOLD)"), 125, FLinearColor(0.0f, 0.90f, 1.0f));
			NotifyBountyEvent(FName(TEXT("ChronoDiveKill")), 1);
		}

		// Accolade: Headshot Critical Kill
		if (bIsHeadshot)
		{
			AwardCombatAccolade(TEXT("ANATOMICAL CRITICAL (+125 GOLD)"), 125, FLinearColor(1.0f, 0.18f, 0.32f));
		}
	}
}

void AFPSCharacter::HandleBulletGraze(float Distance)
{
	// Refund 20 Stamina
	if (FPSMovementComponent)
	{
		FPSMovementComponent->AddStamina(20.0f);
	}

	// Trigger HUD Callout and Dodge Accolade
	if (ActiveHUDWidget)
	{
		ActiveHUDWidget->TriggerAccolade(TEXT("NEAR MISS // +20 STAMINA"), 0, FLinearColor(0.0f, 0.90f, 1.0f));
	}

	AwardCombatAccolade(TEXT("BULLET DODGE (+50 GOLD)"), 50, FLinearColor(0.20f, 1.0f, 0.60f));
	NotifyBountyEvent(FName(TEXT("BulletDodge")), 1);
}

void AFPSCharacter::AwardCombatAccolade(const FString& AccoladeName, int32 GoldReward, const FLinearColor& AccentColor)
{
	if (ActiveHUDWidget)
	{
		ActiveHUDWidget->TriggerAccolade(AccoladeName, GoldReward, AccentColor);
	}

	if (AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>())
	{
		PS->AwardCombatAccolade(AccoladeName, GoldReward);
	}

	// Combat-to-Economy Integration Bridge: Credit persistent profile wallet immediately
	if (GoldReward > 0)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
			{
				Economy->AddGold(GoldReward, AccoladeName);
			}
		}
	}

	OnCombatAccoladeEarned.Broadcast(AccoladeName, GoldReward);
}

void AFPSCharacter::NotifyBountyEvent(FName EventTag, int32 Count)
{
	if (Count <= 0) return;

	OnBountyEvent.Broadcast(EventTag, Count);

	// Bridge 2: Connect combat events directly to daily bounties
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			Economy->RecordBountyProgress(EventTag, Count);
		}
	}

	if (ActiveHUDWidget)
	{
		FString BountyTitle = TEXT("BOUNTY PROGRESS");
		if (EventTag == FName(TEXT("BulletDodge")))
		{
			BountyTitle = TEXT("BOUNTY // MATRIX DODGE +1");
		}
		else if (EventTag == FName(TEXT("FermiSwapKill")))
		{
			BountyTitle = TEXT("BOUNTY // FERMI PHANTOM +1");
		}
		else if (EventTag == FName(TEXT("ChronoDiveKill")))
		{
			BountyTitle = TEXT("BOUNTY // CHRONO DIVER +1");
		}
		else if (EventTag == FName(TEXT("PlateShatter")))
		{
			BountyTitle = TEXT("BOUNTY // ARMOR BREAKER +1");
		}
		ActiveHUDWidget->TriggerAccolade(BountyTitle, 0, FLinearColor(0.0f, 0.90f, 0.46f));
	}
}

void AFPSCharacter::ApplyLoadoutCosmetics(int32 CamoIndex, int32 HeadgearIndex, int32 ArmorIndex, int32 WeaponFinishIndex)
{
	SetOperatorCamo(static_cast<uint8>(FMath::Clamp(CamoIndex, 0, 3)));

	if (OperatorGearComponent)
	{
		OperatorGearComponent->ApplyCamoPreset(CamoIndex);
		OperatorGearComponent->ApplyHeadgearVariant(HeadgearIndex);
		OperatorGearComponent->ApplyVestVariant(ArmorIndex);
		OperatorGearComponent->ApplyWeaponFinishVariant(WeaponFinishIndex);
	}

	// Headgear cosmetic styling (0: Matte Olive, 1: Spec-Ops Night Vision Slate, 2: Desert Tan, 3: Gold Command Trim)
	FLinearColor HelmetColor(0.09f, 0.11f, 0.13f, 1.0f);
	if (HeadgearIndex == 1) HelmetColor = FLinearColor(0.03f, 0.05f, 0.07f, 1.0f);
	else if (HeadgearIndex == 2) HelmetColor = FLinearColor(0.42f, 0.36f, 0.26f, 1.0f);
	else if (HeadgearIndex == 3) HelmetColor = FLinearColor(0.65f, 0.48f, 0.12f, 1.0f);

	if (BallisticHelmetProcMesh && !bIsHelmetPopped)
	{
		if (UMaterialInstanceDynamic* HelmetMID = Cast<UMaterialInstanceDynamic>(BallisticHelmetProcMesh->GetMaterial(0)))
		{
			HelmetMID->SetVectorParameterValue(TEXT("BaseColor"), HelmetColor);
			HelmetMID->SetVectorParameterValue(TEXT("Color"), HelmetColor);
		}
	}

	if (BallisticHelmetMesh && !bIsHelmetPopped)
	{
		UMaterialInstanceDynamic* HelmetMID = Cast<UMaterialInstanceDynamic>(BallisticHelmetMesh->GetMaterial(0));
		if (!HelmetMID)
		{
			HelmetMID = BallisticHelmetMesh->CreateDynamicMaterialInstance(0);
		}
		if (HelmetMID)
		{
			HelmetMID->SetVectorParameterValue(TEXT("BaseColor"), HelmetColor);
			HelmetMID->SetVectorParameterValue(TEXT("Color"), HelmetColor);
		}
	}

	// Armor Carrier cosmetic styling (0: Tactical Slate, 1: Heavy Olive, 2: Coyote Tan, 3: Crimson Spec-Ops)
	FLinearColor CarrierColor(0.12f, 0.15f, 0.18f, 1.0f);
	if (ArmorIndex == 1) CarrierColor = FLinearColor(0.14f, 0.19f, 0.12f, 1.0f);
	else if (ArmorIndex == 2) CarrierColor = FLinearColor(0.44f, 0.37f, 0.26f, 1.0f);
	else if (ArmorIndex == 3) CarrierColor = FLinearColor(0.32f, 0.06f, 0.09f, 1.0f);

	if (PlateCarrierProcMesh && !bIsArmorPlateShattered)
	{
		if (UMaterialInstanceDynamic* CarrierMID = Cast<UMaterialInstanceDynamic>(PlateCarrierProcMesh->GetMaterial(0)))
		{
			CarrierMID->SetVectorParameterValue(TEXT("BaseColor"), CarrierColor);
			CarrierMID->SetVectorParameterValue(TEXT("Color"), CarrierColor);
		}
	}

	if (ChestCarrierMesh && !bIsArmorPlateShattered)
	{
		UMaterialInstanceDynamic* CarrierMID = Cast<UMaterialInstanceDynamic>(ChestCarrierMesh->GetMaterial(0));
		if (!CarrierMID)
		{
			CarrierMID = ChestCarrierMesh->CreateDynamicMaterialInstance(0);
		}
		if (CarrierMID)
		{
			CarrierMID->SetVectorParameterValue(TEXT("BaseColor"), CarrierColor);
			CarrierMID->SetVectorParameterValue(TEXT("Color"), CarrierColor);
		}
	}

	// Propagate weapon finish to equipped primary/secondary weapons
	if (InventoryComponent)
	{
		if (AFPSWeaponBase* PrimaryWep = InventoryComponent->GetPrimaryWeapon())
		{
			PrimaryWep->ApplyLoadoutCosmetics(CamoIndex, HeadgearIndex, ArmorIndex, WeaponFinishIndex);
		}
		if (AFPSWeaponBase* SecondaryWep = InventoryComponent->GetSecondaryWeapon())
		{
			SecondaryWep->ApplyLoadoutCosmetics(CamoIndex, HeadgearIndex, ArmorIndex, WeaponFinishIndex);
		}
	}
}

void AFPSCharacter::MulticastTriggerArmorPlateShatter_Implementation(FVector ShotDirection, FVector ImpactPoint)
{
	if (bIsArmorPlateShattered) return;
	bIsArmorPlateShattered = true;

	if (OperatorGearComponent)
	{
		OperatorGearComponent->SetPlateCarrierShattered(true);
	}

	// 1. Update 3rd-person Chest Carrier Mesh to battle-damaged / shredded state
	if (PlateCarrierProcMesh)
	{
		PlateCarrierProcMesh->SetWorldScale3D(FVector(0.92f, 0.80f, 0.92f));
	}
	if (ChestCarrierMesh)
	{
		ChestCarrierMesh->SetRelativeScale3D(FVector(0.09f, 0.25f, 0.24f));
		if (UMaterialInstanceDynamic* CarrierMID = ChestCarrierMesh->CreateDynamicMaterialInstance(0))
		{
			CarrierMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.03f, 0.03f, 0.04f, 1.0f));
			CarrierMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.03f, 0.04f, 1.0f));
			CarrierMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.95f, 0.35f, 0.05f, 1.0f));
		}
	}

	// 2. Eject physical procedural ceramic plate shard from chest carrier that tumbles to the ground
	if (EjectedPlateShardMesh)
	{
		EjectedPlateShardMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		const FVector SpawnPos = !ImpactPoint.IsNearlyZero() ? ImpactPoint : (GetActorLocation() + FVector(0.0f, 0.0f, 42.0f));
		EjectedPlateShardMesh->SetWorldLocation(SpawnPos);
		EjectedPlateShardMesh->SetWorldRotation(GetActorRotation());
		EjectedPlateShardMesh->SetVisibility(true);

		const FVector OutwardDir = (-ShotDirection.GetSafeNormal() + FVector(FMath::FRandRange(-0.35f, 0.35f), FMath::FRandRange(-0.35f, 0.35f), 0.65f)).GetSafeNormal();
		ShardVelocity = OutwardDir * 440.0f;
		ShardAngularVelocity = FRotator(FMath::FRandRange(-720.0f, 720.0f), FMath::FRandRange(-720.0f, 720.0f), FMath::FRandRange(-540.0f, 540.0f));
		ShardFlightTimer = 2.6f;

		if (UMaterialInstanceDynamic* ShardMID = EjectedPlateShardMesh->CreateDynamicMaterialInstance(0))
		{
			ShardMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.22f, 0.25f, 0.28f, 1.0f));
			ShardMID->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.0f, 0.65f, 1.0f, 1.0f));
		}
	}

	// 3. Spawn bright ceramic spark / dust burst tracers at chest impact point
	if (UWorld* World = GetWorld())
	{
		const FVector SparkOrigin = !ImpactPoint.IsNearlyZero() ? ImpactPoint : (GetActorLocation() + FVector(0.0f, 0.0f, 42.0f));
		for (int32 i = 0; i < 5; ++i)
		{
			FActorSpawnParameters SparkParams;
			SparkParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AFPSBallisticTracer* Spark = World->SpawnActor<AFPSBallisticTracer>(SparkOrigin, FRotator::ZeroRotator, SparkParams))
			{
				const FVector SparkEnd = SparkOrigin + FVector(FMath::FRandRange(-120.0f, 120.0f), FMath::FRandRange(-120.0f, 120.0f), FMath::FRandRange(-40.0f, 130.0f));
				Spark->InitializeTracer(SparkOrigin, SparkEnd, false, false);
			}
		}
	}
}

void AFPSCharacter::MulticastPopBallisticHelmet_Implementation(FVector ShotDirection)
{
	if (bIsHelmetPopped) return;
	bIsHelmetPopped = true;

	if (OperatorGearComponent)
	{
		OperatorGearComponent->SetHelmetVisible(false);
	}
	if (BallisticHelmetProcMesh)
	{
		BallisticHelmetProcMesh->SetHiddenInGame(false);
		BallisticHelmetProcMesh->SetVisibility(true, true);
		BallisticHelmetProcMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
	if (BallisticHelmetMesh)
	{
		BallisticHelmetMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
	const FVector LaunchDir = (ShotDirection.GetSafeNormal() * 0.45f + FVector(FMath::FRandRange(-0.2f, 0.2f), FMath::FRandRange(-0.2f, 0.2f), 1.15f)).GetSafeNormal();
	HelmetVelocity = LaunchDir * 580.0f;
	HelmetAngularVelocity = FRotator(FMath::FRandRange(-820.0f, 820.0f), FMath::FRandRange(-640.0f, 640.0f), FMath::FRandRange(-720.0f, 720.0f));
	HelmetFlightTimer = 3.0f;
}

void AFPSCharacter::UpdatePhysicalArmorAndHelmetDebris(float DeltaTime)
{
	if (LimbTraumaFlinchTimer > 0.0f)
	{
		LimbTraumaFlinchTimer = FMath::Max(0.0f, LimbTraumaFlinchTimer - DeltaTime);
	}

	// Simulate tumbling ceramic armor shard physics & ground bounce
	if (ShardFlightTimer > 0.0f && EjectedPlateShardMesh)
	{
		ShardFlightTimer -= DeltaTime;
		ShardVelocity.Z -= 1450.0f * DeltaTime;
		FVector NewLoc = EjectedPlateShardMesh->GetComponentLocation() + ShardVelocity * DeltaTime;
		if (NewLoc.Z < 6.0f)
		{
			NewLoc.Z = 6.0f;
			ShardVelocity.Z = FMath::Abs(ShardVelocity.Z) * 0.35f;
			ShardVelocity.X *= 0.65f;
			ShardVelocity.Y *= 0.65f;
		}
		EjectedPlateShardMesh->SetWorldLocation(NewLoc);
		EjectedPlateShardMesh->AddWorldRotation(ShardAngularVelocity * DeltaTime);

		if (ShardFlightTimer <= 0.0f)
		{
			EjectedPlateShardMesh->SetVisibility(false);
		}
	}

	// Simulate popped ballistic helmet trajectory & bounce
	if (HelmetFlightTimer > 0.0f)
	{
		HelmetFlightTimer -= DeltaTime;
		HelmetVelocity.Z -= 1380.0f * DeltaTime;

		FVector BaseLoc = BallisticHelmetProcMesh ? BallisticHelmetProcMesh->GetComponentLocation() : (BallisticHelmetMesh ? BallisticHelmetMesh->GetComponentLocation() : FVector::ZeroVector);
		FVector NewLoc = BaseLoc + HelmetVelocity * DeltaTime;
		if (NewLoc.Z < 12.0f)
		{
			NewLoc.Z = 12.0f;
			HelmetVelocity.Z = FMath::Abs(HelmetVelocity.Z) * 0.42f;
			HelmetVelocity.X *= 0.70f;
			HelmetVelocity.Y *= 0.70f;
		}
		if (BallisticHelmetProcMesh)
		{
			BallisticHelmetProcMesh->SetWorldLocation(NewLoc);
			BallisticHelmetProcMesh->AddWorldRotation(HelmetAngularVelocity * DeltaTime);
		}
		if (BallisticHelmetMesh)
		{
			BallisticHelmetMesh->SetWorldLocation(NewLoc);
			BallisticHelmetMesh->AddWorldRotation(HelmetAngularVelocity * DeltaTime);
		}
	}
}

void AFPSCharacter::InitializeAsTacticalBot(int32 InBotIndex, const FString& InCallsign, EFPSPlayerTeam InTeam)
{
	bIsTacticalBot = true;
	BotLaneIndex = InBotIndex % 3;
	BotWaypointIndex = 0;
	BotCallsign = InCallsign;
	BotTeam = InTeam;
	BotAimRotation = GetActorRotation();
	BaitedRemainingTimer = 0.0f;
	BotBurstFireTimer = 0.0f;
	BotBurstCooldownTimer = FMath::FRandRange(0.25f, 0.75f);
	BotStrafeTimer = FMath::FRandRange(0.8f, 1.6f);
	BotStrafeSign = (InBotIndex % 2 == 0) ? 1.0f : -1.0f;

	// Give bots distinct Spec-Ops / Crimson-Slate camo so they read clearly downrange
	ApplyLoadoutCosmetics((InBotIndex % 3) + 1, InBotIndex % 4, 3, InBotIndex % 4);
}

void AFPSCharacter::NotifyBaitedByDecoy(AFPSFermiDecoyActor* InDecoy, float BaitDuration)
{
	if (!bIsTacticalBot || bIsDead || !InDecoy) return;

	BaitedDecoyTarget = InDecoy;
	BaitedRemainingTimer = FMath::Max(BaitedRemainingTimer, BaitDuration);

	// Immediately trigger a burst toward the holographic decoy
	BotBurstCooldownTimer = 0.0f;
}

bool AFPSCharacter::IsBaitedByDecoy() const
{
	return bIsTacticalBot && !bIsDead && BaitedRemainingTimer > 0.0f && BaitedDecoyTarget.IsValid();
}

void AFPSCharacter::TickTacticalBotAI(float DeltaTime)
{
	if (!bIsTacticalBot || bIsDead) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Keep bot magazines topped up so firefights never stall
	if (AFPSWeaponBase* Weapon = GetCurrentWeapon())
	{
		if (Weapon->GetCurrentClipAmmo() <= 2)
		{
			Weapon->RefillBotAmmo();
		}
	}

	// Update Acoustic Decoy Bait state
	if (BaitedRemainingTimer > 0.0f)
	{
		BaitedRemainingTimer = FMath::Max(0.0f, BaitedRemainingTimer - DeltaTime);
		if (!BaitedDecoyTarget.IsValid())
		{
			BaitedRemainingTimer = 0.0f;
		}
	}

	// Rust Tactical 3-Lane Waypoints (Flag A Container Yard, Flag B Center Derrick, Flag C Pipeline Bunker)
	static const FVector RustLaneWaypoints[3][3] = {
		{ FVector(-1400.0f, -1000.0f, 110.0f), FVector(-520.0f, -750.0f, 110.0f), FVector(650.0f, -850.0f, 110.0f) },
		{ FVector(950.0f, -320.0f, 110.0f),   FVector(360.0f, -260.0f, 110.0f), FVector(-360.0f, 260.0f, 110.0f) },
		{ FVector(1400.0f, 1000.0f, 110.0f),  FVector(480.0f, 820.0f, 110.0f),  FVector(-680.0f, 880.0f, 110.0f) }
	};

	// Coastal Operations 3-Lane Waypoints (West Stair Flank, Central Sluice Culvert / Dike Crest Flag B, East Stair / Dune Basin Flag A)
	static const FVector CoastalLaneWaypoints[3][3] = {
		{ FVector(-3600.0f, -1900.0f, 110.0f), FVector(-3600.0f, 950.0f, 110.0f), FVector(-1400.0f, 3400.0f, 110.0f) },
		{ FVector(0.0f, -1900.0f, 110.0f),     FVector(0.0f, 1450.0f, 110.0f),    FVector(0.0f, 3600.0f, 110.0f) },
		{ FVector(2400.0f, -1900.0f, 110.0f),  FVector(1200.0f, -500.0f, 430.0f), FVector(1400.0f, 3400.0f, 110.0f) }
	};

	bool bIsCoastalTheater = (FMath::Abs(GetActorLocation().Y) > 1950.0f);
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
		{
			bIsCoastalTheater = (Economy->GetSelectedTheaterIndex() == 1);
		}
	}

	const FVector MyLoc = GetActorLocation();
	const FVector MyEye = GetBotEyeLocation();

	// 1. Target Acquisition: Prioritize Active Hologram Decoy when Baited!
	AActor* ActiveTarget = nullptr;
	FVector TargetAimPoint = FVector::ZeroVector;
	bool bHasLineOfSight = false;

	if (IsBaitedByDecoy())
	{
		ActiveTarget = BaitedDecoyTarget.Get();
		TargetAimPoint = ActiveTarget->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
		bHasLineOfSight = true;
	}
	else
	{
		// Scan for hostile human player or opposing team character
		float BestDistSq = FMath::Square(4800.0f);
		for (TActorIterator<AFPSCharacter> It(World); It; ++It)
		{
			AFPSCharacter* Candidate = *It;
			if (!Candidate || Candidate == this) continue;
			if (Candidate->GetHealthComponent() && Candidate->GetHealthComponent()->IsDead()) continue;

			EFPSPlayerTeam CandidateTeam = Candidate->IsTacticalBot() ? Candidate->GetBotTeam() : EFPSPlayerTeam::Alpha;
			if (AFPSPlayerState* CandidatePS = Candidate->GetPlayerState<AFPSPlayerState>())
			{
				CandidateTeam = CandidatePS->GetPlayerTeam();
			}
			if (CandidateTeam == BotTeam) continue;

			const float DistSq = FVector::DistSquared(MyLoc, Candidate->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				const FVector CandChest = Candidate->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
				FHitResult Hit;
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(this);
				QueryParams.AddIgnoredActor(Candidate);
				if (AFPSWeaponBase* MyWep = GetCurrentWeapon())
				{
					QueryParams.AddIgnoredActor(MyWep);
				}

				const bool bBlocked = World->LineTraceSingleByChannel(Hit, MyEye, CandChest, ECC_Visibility, QueryParams);
				if (!bBlocked || DistSq < FMath::Square(950.0f))
				{
					BestDistSq = DistSq;
					ActiveTarget = Candidate;
					TargetAimPoint = CandChest;
					bHasLineOfSight = true;
				}
			}
		}
	}

	// 2. Movement: Patrol between Rust Tactical lanes & strafe into cover during firefights
	const int32 SafeLane = FMath::Clamp(BotLaneIndex, 0, 2);
	const int32 SafeWP = FMath::Clamp(BotWaypointIndex, 0, 2);
	const FVector GoalPoint = bIsCoastalTheater ? CoastalLaneWaypoints[SafeLane][SafeWP] : RustLaneWaypoints[SafeLane][SafeWP];

	FVector ToGoal = GoalPoint - MyLoc;
	ToGoal.Z = 0.0f;
	if (ToGoal.SizeSquared2D() < FMath::Square(220.0f) || BotWaypointStuckTimer > 4.5f)
	{
		BotWaypointIndex = (BotWaypointIndex + 1) % 3;
		BotWaypointStuckTimer = 0.0f;
	}
	else
	{
		BotWaypointStuckTimer += DeltaTime;
	}

	BotStrafeTimer -= DeltaTime;
	if (BotStrafeTimer <= 0.0f)
	{
		BotStrafeTimer = FMath::FRandRange(0.85f, 1.75f);
		BotStrafeSign = -BotStrafeSign;
	}

	if (ActiveTarget && bHasLineOfSight)
	{
		// Combat Strafe + Lane Spacing
		FVector ToTarget = TargetAimPoint - MyLoc;
		const float Dist2D = ToTarget.Size2D();
		FVector DirToTarget2D = ToTarget.GetSafeNormal2D();
		FVector RightStrafeDir = FVector::CrossProduct(FVector::UpVector, DirToTarget2D) * BotStrafeSign;

		FVector DesiredMoveDir = RightStrafeDir * 0.78f;
		if (Dist2D > 1650.0f)
		{
			DesiredMoveDir += DirToTarget2D * 0.55f;
		}
		else if (Dist2D < 680.0f)
		{
			DesiredMoveDir -= DirToTarget2D * 0.55f;
		}

		AddMovementInput(DesiredMoveDir.GetSafeNormal2D(), 0.85f);
		CurrentMoveInput = FVector2D(0.35f, BotStrafeSign * 0.85f);

		// Smoothly track target or decoy
		const FRotator DesiredAim = (TargetAimPoint - MyEye).Rotation();
		const float TurnSpeed = IsBaitedByDecoy() ? 14.0f : 8.5f;
		BotAimRotation = FMath::RInterpTo(BotAimRotation, DesiredAim, DeltaTime, TurnSpeed);
		SetActorRotation(FRotator(0.0f, BotAimRotation.Yaw, 0.0f));

		// Controlled tactical burst fire (populates AFPSDilatedBulletActor inside Fermi Time Bubbles!)
		if (bBotIsFiringBurst)
		{
			BotBurstFireTimer -= DeltaTime;
			if (BotBurstFireTimer <= 0.0f)
			{
				bBotIsFiringBurst = false;
				StopFire();
				BotBurstCooldownTimer = IsBaitedByDecoy() ? FMath::FRandRange(0.22f, 0.45f) : FMath::FRandRange(0.55f, 1.15f);
			}
		}
		else
		{
			BotBurstCooldownTimer -= DeltaTime;
			if (BotBurstCooldownTimer <= 0.0f)
			{
				bBotIsFiringBurst = true;
				BotBurstFireTimer = IsBaitedByDecoy() ? FMath::FRandRange(0.45f, 0.75f) : FMath::FRandRange(0.28f, 0.52f);
				StartFire();
			}
		}
	}
	else
	{
		if (bBotIsFiringBurst)
		{
			bBotIsFiringBurst = false;
			StopFire();
		}

		const FVector MoveDir = ToGoal.GetSafeNormal2D();
		AddMovementInput(MoveDir, 1.0f);
		CurrentMoveInput = FVector2D(1.0f, 0.0f);

		if (!MoveDir.IsNearlyZero())
		{
			const FRotator DesiredPatrolRot = MoveDir.Rotation();
			BotAimRotation = FMath::RInterpTo(BotAimRotation, DesiredPatrolRot, DeltaTime, 6.0f);
			SetActorRotation(FRotator(0.0f, BotAimRotation.Yaw, 0.0f));
		}
	}
}

void AFPSCharacter::SetFieldOfView(float NewFOV)
{
	DefaultFOV = FMath::Clamp(NewFOV, 75.0f, 110.0f);
}

void AFPSCharacter::SetOperatorCamo(uint8 CamoIndex)
{
	if (!GetMesh()) return;

	UMaterialInstanceDynamic* BodyMID = Cast<UMaterialInstanceDynamic>(GetMesh()->GetMaterial(0));
	if (!BodyMID)
	{
		BodyMID = GetMesh()->CreateDynamicMaterialInstance(0);
	}
	if (!BodyMID) return;

	FLinearColor CamoPrimary = FLinearColor(0.25f, 0.28f, 0.32f, 1.0f);
	FLinearColor CamoSecondary = FLinearColor(0.12f, 0.14f, 0.17f, 1.0f);
	FLinearColor CamoTertiary = FLinearColor(0.42f, 0.44f, 0.46f, 1.0f);
	FLinearColor TorsoColor = FLinearColor(0.18f, 0.20f, 0.23f, 1.0f);
	float Roughness = 0.76f;

	if (CamoIndex == 1) // Woodland
	{
		CamoPrimary = FLinearColor(0.18f, 0.24f, 0.15f, 1.0f);
		CamoSecondary = FLinearColor(0.09f, 0.13f, 0.08f, 1.0f);
		CamoTertiary = FLinearColor(0.30f, 0.27f, 0.18f, 1.0f);
		TorsoColor = FLinearColor(0.16f, 0.21f, 0.14f, 1.0f);
		Roughness = 0.78f;
	}
	else if (CamoIndex == 2) // Desert
	{
		CamoPrimary = FLinearColor(0.48f, 0.42f, 0.32f, 1.0f);
		CamoSecondary = FLinearColor(0.34f, 0.29f, 0.22f, 1.0f);
		CamoTertiary = FLinearColor(0.58f, 0.53f, 0.42f, 1.0f);
		TorsoColor = FLinearColor(0.40f, 0.35f, 0.27f, 1.0f);
		Roughness = 0.82f;
	}
	else if (CamoIndex == 3) // Blackout
	{
		CamoPrimary = FLinearColor(0.04f, 0.04f, 0.05f, 1.0f);
		CamoSecondary = FLinearColor(0.07f, 0.07f, 0.08f, 1.0f);
		CamoTertiary = FLinearColor(0.09f, 0.09f, 0.11f, 1.0f);
		TorsoColor = FLinearColor(0.03f, 0.03f, 0.04f, 1.0f);
		Roughness = 0.72f;
	}

	BodyMID->SetVectorParameterValue(TEXT("CamoColorPrimary"), CamoPrimary);
	BodyMID->SetVectorParameterValue(TEXT("CamoColorSecondary"), CamoSecondary);
	BodyMID->SetVectorParameterValue(TEXT("CamoColorTertiary"), CamoTertiary);
	BodyMID->SetVectorParameterValue(TEXT("TorsoColor"), TorsoColor);
	BodyMID->SetScalarParameterValue(TEXT("RoughnessBase"), Roughness);
}

void AFPSCharacter::RedeployOperator()
{
	if (HealthComponent)
	{
		HealthComponent->Heal(HealthComponent->MaxHealth);
		HealthComponent->RechargeShield(HealthComponent->MaxShield);
	}
	if (FPSMovementComponent)
	{
		FPSMovementComponent->AddStamina(100.0f);
		FPSMovementComponent->StopBallisticDive();
	}

	bIsArmorPlateShattered = false;
	bIsHelmetPopped = false;
	ShardFlightTimer = 0.0f;
	HelmetFlightTimer = 0.0f;

	if (OperatorGearComponent)
	{
		OperatorGearComponent->SetPlateCarrierShattered(false);
		OperatorGearComponent->SetHelmetVisible(true);
	}

	if (PlateCarrierProcMesh && GetCapsuleComponent())
	{
		PlateCarrierProcMesh->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		PlateCarrierProcMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 28.0f));
		PlateCarrierProcMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
		PlateCarrierProcMesh->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
		PlateCarrierProcMesh->SetVisibility(true, true);
	}
	if (BallisticHelmetProcMesh && GetCapsuleComponent())
	{
		BallisticHelmetProcMesh->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		BallisticHelmetProcMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
		BallisticHelmetProcMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
		BallisticHelmetProcMesh->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
		BallisticHelmetProcMesh->SetVisibility(true, true);
	}
	if (ChestCarrierMesh) { ChestCarrierMesh->SetVisibility(false); ChestCarrierMesh->SetHiddenInGame(true); }
	if (BallisticHelmetMesh) { BallisticHelmetMesh->SetVisibility(false); BallisticHelmetMesh->SetHiddenInGame(true); }
	if (EjectedPlateShardMesh)
	{
		EjectedPlateShardMesh->SetVisibility(false);
	}

	SetActorLocation(FVector(0.0f, 0.0f, 120.0f), false, nullptr, ETeleportType::ResetPhysics);
}

void AFPSCharacter::BuildProceduralTacticalGear()
{
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BaseMat)
	{
		BaseMat = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	// 1. Build Level IV Ceramic Plate Carrier
	if (PlateCarrierProcMesh)
	{
		PlateCarrierProcMesh->ClearAllMeshSections();

		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<FProcMeshTangent> Tangents;

		// Front Plate Bag (15 x 5 x 16)
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(0.0f, 9.0f, 0.0f), FVector(15.0f, 5.0f, 16.0f), FRotator::ZeroRotator, FColor(24, 26, 28, 255));

		// Back Plate Bag (14 x 4.5 x 15)
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(0.0f, -8.5f, 0.0f), FVector(14.0f, 4.5f, 15.0f), FRotator::ZeroRotator, FColor(22, 24, 26, 255));

		// Left & Right Shoulder Straps (3 x 9 x 1.5)
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(-9.0f, 0.0f, 16.5f), FVector(3.0f, 9.0f, 1.5f), FRotator::ZeroRotator, FColor(18, 20, 22, 255));
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(9.0f, 0.0f, 16.5f), FVector(3.0f, 9.0f, 1.5f), FRotator::ZeroRotator, FColor(18, 20, 22, 255));

		// Left & Right Cummerbund Flanks (2 x 9 x 6)
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(-14.0f, 0.0f, -2.0f), FVector(2.0f, 9.0f, 6.0f), FRotator::ZeroRotator, FColor(20, 22, 24, 255));
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(14.0f, 0.0f, -2.0f), FVector(2.0f, 9.0f, 6.0f), FRotator::ZeroRotator, FColor(20, 22, 24, 255));

		// 3 Kangaroo Mag Pouches on Front Chest
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(-8.0f, 15.0f, -4.0f), FVector(3.8f, 2.5f, 7.5f), FRotator::ZeroRotator, FColor(26, 28, 30, 255));
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(0.0f, 15.0f, -4.0f), FVector(3.8f, 2.5f, 7.5f), FRotator::ZeroRotator, FColor(26, 28, 30, 255));
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(8.0f, 15.0f, -4.0f), FVector(3.8f, 2.5f, 7.5f), FRotator::ZeroRotator, FColor(26, 28, 30, 255));

		PlateCarrierProcMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* CarrierMID = PlateCarrierProcMesh->CreateDynamicMaterialInstance(0, BaseMat))
			{
				CarrierMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.09f, 0.11f, 1.0f));
				CarrierMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.08f, 0.09f, 0.11f, 1.0f));
				CarrierMID->SetScalarParameterValue(TEXT("Roughness"), 0.70f);
				CarrierMID->SetScalarParameterValue(TEXT("Metallic"), 0.10f);
			}
		}
	}

	// 2. Build FAST Ballistic Helmet
	if (BallisticHelmetProcMesh)
	{
		BallisticHelmetProcMesh->ClearAllMeshSections();

		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<FProcMeshTangent> Tangents;

		// High-Cut FAST Helmet Shell (11.5 x 12.5 x 9)
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(0.0f, 1.0f, 8.5f), FVector(11.5f, 12.5f, 9.0f), FRotator::ZeroRotator, FColor(22, 22, 24, 255));

		// Crown Ridge (4 x 11 x 1.5)
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(0.0f, 0.5f, 18.0f), FVector(4.0f, 11.0f, 1.5f), FRotator::ZeroRotator, FColor(20, 20, 22, 255));

		// Forehead NVG Shroud (2.5 x 1 x 2.5)
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(0.0f, 13.0f, 8.0f), FVector(2.5f, 1.0f, 2.5f), FRotator::ZeroRotator, FColor(45, 45, 48, 255));

		// Left & Right ARC Accessory Rails (1.2 x 7 x 1.8)
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(-12.5f, 1.0f, 7.0f), FVector(1.2f, 7.0f, 1.8f), FRotator::ZeroRotator, FColor(18, 18, 20, 255));
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(12.5f, 1.0f, 7.0f), FVector(1.2f, 7.0f, 1.8f), FRotator::ZeroRotator, FColor(18, 18, 20, 255));

		// Velcro Top Patch Panel (4 x 5 x 0.3)
		AddBoxToProcMeshBuffers(Vertices, Triangles, Normals, UVs, Colors,
			FVector(0.0f, -2.0f, 18.0f), FVector(4.0f, 5.0f, 0.3f), FRotator::ZeroRotator, FColor(35, 35, 38, 255));

		BallisticHelmetProcMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* HelmetMID = BallisticHelmetProcMesh->CreateDynamicMaterialInstance(0, BaseMat))
			{
				HelmetMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.06f, 0.07f, 0.08f, 1.0f));
				HelmetMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.06f, 0.07f, 0.08f, 1.0f));
				HelmetMID->SetScalarParameterValue(TEXT("Roughness"), 0.65f);
				HelmetMID->SetScalarParameterValue(TEXT("Metallic"), 0.15f);
			}
		}
	}
}

float AFPSCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (HealthComponent && !bIsDead && ActualDamage > 0.0f)
	{
		FHitResult DummyHit;
		DummyHit.ImpactPoint = GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
		DummyHit.BoneName = NAME_None;
		AActor* InstigatorPawn = EventInstigator ? EventInstigator->GetPawn() : DamageCauser;
		HealthComponent->ApplyDamage(ActualDamage, InstigatorPawn, DummyHit);
	}
	return ActualDamage;
}
