#include "Character/FPSCharacter.h"
#include "Character/FPSCharacterMovementComponent.h"
#include "Combat/FPSHealthComponent.h"
#include "Combat/FPSFermiTimeBubble.h"
#include "Combat/FPSFermiDecoyActor.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Combat/FPSEquipmentComponent.h"
#include "BodyMechanics/FPSBodyMechanicsComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "Multiplayer/FPSPlayerState.h"
#include "UI/FPSHUDWidget.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"
#include "Audio/FPSAudioSubsystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "TimerManager.h"

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
	FollowCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	FollowCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	FollowCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	FollowCamera->PostProcessSettings.AutoExposureBias = 0.0f;
	FollowCamera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	FollowCamera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = 0;
	FollowCamera->PostProcessSettings.bOverride_BloomIntensity = true;
	FollowCamera->PostProcessSettings.BloomIntensity = 0.1f;
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

	// Ballistic collision routing: root movement capsule ignores visibility trace so bullet line traces strike anatomical bone capsules directly
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

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

	// Default to tactical high-ready idle animation with natural thoracic breathing
	if (AnimIdleADS)
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->SetAnimation(AnimIdleADS);
		CurrentLocomotionAnim = AnimIdleADS;
		GetMesh()->Play(true);
	}

	HealthComponent = CreateDefaultSubobject<UFPSHealthComponent>(TEXT("HealthComponent"));
	InventoryComponent = CreateDefaultSubobject<UFPSInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->DefaultPrimaryClass = AFPSWeaponBase::StaticClass();
	EquipmentComponent = CreateDefaultSubobject<UFPSEquipmentComponent>(TEXT("EquipmentComponent"));
	BodyMechanicsComponent = CreateDefaultSubobject<UFPSBodyMechanicsComponent>(TEXT("BodyMechanicsComponent"));
	HUDWidgetClass = UFPSHUDWidget::StaticClass();
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

		UAnimSequence* InitialAnim = AnimLowReady ? AnimLowReady : AnimIdleADS;
		if (InitialAnim && !CurrentLocomotionAnim)
		{
			GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			GetMesh()->SetAnimation(InitialAnim);
			CurrentLocomotionAnim = InitialAnim;
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

		// Ensure weapon_r socket exists on right hand for weapon rigging
		if (USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(GetMesh()->GetSkeletalMeshAsset()))
		{
			bool bSocketFound = false;
			for (const USkeletalMeshSocket* S : SkelMesh->GetMeshOnlySocketList())
			{
				if (S && S->SocketName == FName(TEXT("weapon_r")))
				{
					bSocketFound = true;
					break;
				}
			}
			if (!bSocketFound)
			{
				USkeletalMeshSocket* WeaponRSocket = NewObject<USkeletalMeshSocket>(SkelMesh);
				if (WeaponRSocket)
				{
					WeaponRSocket->SocketName = FName(TEXT("weapon_r"));
					WeaponRSocket->BoneName = FName(TEXT("hand_r"));
					WeaponRSocket->RelativeLocation = FVector::ZeroVector;
					WeaponRSocket->RelativeRotation = FRotator::ZeroRotator;
					SkelMesh->GetMeshOnlySocketList().Add(WeaponRSocket);
				}
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
}

void AFPSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeHUD();
}

void AFPSCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeHUD();
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

	UpdateThirdPersonCamera(RealDeltaTime);
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

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, 14.0f);
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, 14.0f);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, 16.0f));
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
		// Compute dive direction relative to current character aim yaw so the 21-bone mesh tilts 75 deg into the world dive vector
		// while allowing full 360-degree aiming & firing mid-air!
		const FVector DiveDir2D = FPSMovementComponent->GetBallisticDiveDirection().GetSafeNormal2D();
		const FVector CharForward2D = GetActorForwardVector().GetSafeNormal2D();
		const FVector CharRight2D = GetActorRightVector().GetSafeNormal2D();

		const float ForwardDot = FVector::DotProduct(DiveDir2D, CharForward2D);
		const float RightDot = FVector::DotProduct(DiveDir2D, CharRight2D);

		// Note: Mesh has a base Yaw of -90 deg (so local +Y is character Forward, local +X is character Right)
		// In Unreal FRotator(Pitch, Yaw, Roll) with Yaw = -90:
		// Roll tilts the character forward/backward along their facing axis, Pitch tilts them side-to-side!
		const float DiveTiltDeg = FPSMovementComponent->IsInDiveLandingSlide() ? 78.0f : 72.0f;
		const float TargetRoll = ForwardDot * DiveTiltDeg;
		const float TargetPitch = -RightDot * DiveTiltDeg;

		TargetMeshRot = FRotator(TargetPitch, -90.0f, TargetRoll);
		TargetZOffset = FPSMovementComponent->IsInDiveLandingSlide() ? -68.0f : -58.0f;
	}
	else if (FPSMovementComponent && FPSMovementComponent->IsSliding())
	{
		// Tactical knee slide: Reclined torso, mesh aligned with reduced slide capsule
		const float AimPitch = FRotator::NormalizeAxis(GetControlRotation().Pitch);
		TargetMeshRot = FRotator(0.0f, -90.0f, -18.0f + FMath::Clamp(-AimPitch * 0.25f, -15.0f, 15.0f));
		TargetZOffset = -42.0f;
	}
	else if (FPSMovementComponent && FPSMovementComponent->IsCrouching())
	{
		// Crouched: Capsule half-height drops to 48 cm, mesh Z-offset -48 cm maintains ground contact
		const float AimPitch = FRotator::NormalizeAxis(GetControlRotation().Pitch);
		TargetMeshRot = FRotator(0.0f, -90.0f, 6.0f + FMath::Clamp(-AimPitch * 0.35f, -20.0f, 20.0f));
		TargetZOffset = -48.0f;
	}
	else
	{
		// Standing posture: Full upper-torso pitch aiming
		const float AimPitch = FRotator::NormalizeAxis(GetControlRotation().Pitch);
		TargetMeshRot = FRotator(0.0f, -90.0f, FMath::Clamp(-AimPitch * 0.55f, -42.0f, 42.0f));
		TargetZOffset = -96.0f;
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

		if (FPSMovementComponent)
		{
			if (FPSMovementComponent->IsBallisticDiving())
			{
				const FVector NewDiveDir = PlayerControlRot.Vector().GetSafeNormal2D();
				FPSMovementComponent->Velocity = NewDiveDir * FMath::Max(HorizSpeed, 620.0f) + FVector(0.0f, 0.0f, FMath::Max(OldVelocity.Z, 180.0f));
			}
			else
			{
				FPSMovementComponent->Velocity = OldVelocity;
			}
		}

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
		FermiDecoyCooldownRemaining = 8.0f;

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
	if (CameraBoom && GetMesh())
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

	if (TargetAnim && TargetAnim != CurrentLocomotionAnim)
	{
		CurrentLocomotionAnim = TargetAnim;
		GetMesh()->SetAnimation(TargetAnim);
		GetMesh()->SetPlayRate(PlayRate);
		GetMesh()->Play(true);
	}
	else if (CurrentLocomotionAnim)
	{
		GetMesh()->SetPlayRate(PlayRate);
	}
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
	PlayerInputComponent->BindAction(TEXT("Lethal"), IE_Pressed, this, &AFPSCharacter::StartCookingLethal);
	PlayerInputComponent->BindAction(TEXT("Lethal"), IE_Released, this, &AFPSCharacter::ReleaseLethal);
	PlayerInputComponent->BindAction(TEXT("ToggleOpticADS"), IE_Pressed, this, &AFPSCharacter::OnMiddleMouseButtonPressed);
	PlayerInputComponent->BindAction(TEXT("ToggleOpticADS"), IE_Released, this, &AFPSCharacter::OnMiddleMouseButtonReleased);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AFPSCharacter::OnMiddleMouseButtonPressed);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AFPSCharacter::OnMiddleMouseButtonReleased);
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
	float SensMod = 1.0f;
	if (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights())
	{
		SensMod = 0.65f;
	}
	// Preserve crisp 1:1 mouse aiming responsiveness even when CustomTimeDilation = 0.55f inside the Fermi Time Bubble
	const float DilationComp = 1.0f / FMath::Max(0.15f, CustomTimeDilation);
	AddControllerYawInput(Rate * SensMod * DilationComp);
}

void AFPSCharacter::LookUp(float Rate)
{
	float SensMod = 1.0f;
	if (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights())
	{
		SensMod = 0.65f;
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
		}
	}
}
