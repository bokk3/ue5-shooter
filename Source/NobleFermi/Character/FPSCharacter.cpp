#include "Character/FPSCharacter.h"
#include "Character/FPSCharacterMovementComponent.h"
#include "Combat/FPSHealthComponent.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Combat/FPSEquipmentComponent.h"
#include "BodyMechanics/FPSBodyMechanicsComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "UI/FPSHUDWidget.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"

AFPSCharacter::AFPSCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	FPSMovementComponent = Cast<UFPSCharacterMovementComponent>(GetCharacterMovement());

	GetCapsuleComponent()->InitCapsuleSize(40.0f, 96.0f);

	// Primary 3rd-Person Tactical Camera Boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
	CameraBoom->TargetArmLength = DefaultArmLength;
	CameraBoom->SocketOffset = RightShoulderOffset;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12.0f;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 16.0f;
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

	static ConstructorHelpers::FObjectFinder<UPhysicsAsset> BaseMalePhysFinder(TEXT("/Game/Characters/BaseMale/SK_BaseMale_PhysicsAsset.SK_BaseMale_PhysicsAsset"));
	if (BaseMalePhysFinder.Succeeded())
	{
		GetMesh()->SetPhysicsAsset(BaseMalePhysFinder.Object);
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

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		TSubclassOf<UFPSHUDWidget> WidgetClass = HUDWidgetClass ? HUDWidgetClass : TSubclassOf<UFPSHUDWidget>(UFPSHUDWidget::StaticClass());
		ActiveHUDWidget = CreateWidget<UFPSHUDWidget>(PC, WidgetClass);
		if (ActiveHUDWidget)
		{
			ActiveHUDWidget->SetOwningCharacter(this);
			ActiveHUDWidget->AddToViewport(0);
		}
	}

	// Tactical Operator Material Configuration (Multi-zone Camo & Ripstop PBR)
	if (GetMesh())
	{
		UMaterialInstanceDynamic* BodyMID = GetMesh()->CreateDynamicMaterialInstance(0);
		if (BodyMID)
		{
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
	}
}

void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateThirdPersonCamera(DeltaTime);
	UpdateLocomotionAnimation(DeltaTime);

	// Update HUD Crosshair with current weapon spread
	const bool bIsAiming = (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights());
	if (ActiveHUDWidget && InventoryComponent)
	{
		AFPSWeaponBase* Weapon = InventoryComponent->GetActiveWeapon();
		if (Weapon)
		{
			// In ADS, crosshair collapses so operator aims via 3D reflex optic dot
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

	// Over-the-shoulder framing & smooth ADS zoom
	const float TargetArmLength = bIsAiming ? ADSArmLength : DefaultArmLength;
	const float TargetFOV = bIsAiming ? ADSFOV : DefaultFOV;
	const FVector TargetSocketOffset = bIsRightShoulder ?
		(bIsAiming ? ADSRightShoulderOffset : RightShoulderOffset) :
		(bIsAiming ? ADSLeftShoulderOffset : LeftShoulderOffset);

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, 14.0f);
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, 14.0f);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, 16.0f));
}

void AFPSCharacter::ToggleShoulderSwap()
{
	bIsRightShoulder = !bIsRightShoulder;
}

void AFPSCharacter::HandleOnDeath(AActor* KillerActor)
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

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

	if (FPSMovementComponent && FPSMovementComponent->IsTacticalSprinting() && Speed > 280.0f)
	{
		TargetAnim = AnimSprint ? AnimSprint : AnimRun;
		PlayRate = FMath::Clamp(Speed / 520.0f, 0.85f, 1.35f);
	}
	else if (Speed > 260.0f)
	{
		TargetAnim = AnimRun ? AnimRun : AnimWalk;
		PlayRate = FMath::Clamp(Speed / 340.0f, 0.85f, 1.25f);
	}
	else if (Speed > 15.0f)
	{
		TargetAnim = AnimWalk ? AnimWalk : AnimIdleADS;
		PlayRate = FMath::Clamp(Speed / 180.0f, 0.75f, 1.25f);
	}
	else
	{
		TargetAnim = AnimIdleADS;
		PlayRate = 1.0f;
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

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
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
}

void AFPSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AFPSCharacter::MoveRight(float Value)
{
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
	AddControllerYawInput(Rate * SensMod);
}

void AFPSCharacter::LookUp(float Rate)
{
	float SensMod = 1.0f;
	if (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights())
	{
		SensMod = 0.65f;
	}
	AddControllerPitchInput(Rate * SensMod);
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

void AFPSCharacter::StartCookingLethal()
{
	if (EquipmentComponent)
	{
		EquipmentComponent->StartCookingLethal();
	}
}

void AFPSCharacter::ReleaseLethal()
{
	if (EquipmentComponent)
	{
		EquipmentComponent->ReleaseAndThrowLethal();
	}
}

void AFPSCharacter::ThrowTactical()
{
	if (EquipmentComponent)
	{
		EquipmentComponent->ThrowTactical();
	}
}
