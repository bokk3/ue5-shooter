#include "Character/FPSCharacter.h"
#include "Character/FPSCharacterMovementComponent.h"
#include "Combat/FPSHealthComponent.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Combat/FPSEquipmentComponent.h"
#include "BodyMechanics/FPSBodyMechanicsComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "UI/FPSHUDWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "GameFramework/PlayerController.h"

AFPSCharacter::AFPSCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	FPSMovementComponent = Cast<UFPSCharacterMovementComponent>(GetCharacterMovement());

	GetCapsuleComponent()->InitCapsuleSize(40.0f, 96.0f);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	FirstPersonCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	FirstPersonCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	FirstPersonCamera->PostProcessSettings.AutoExposureBias = 0.0f;
	FirstPersonCamera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	FirstPersonCamera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = 0;
	FirstPersonCamera->PostProcessSettings.bOverride_BloomIntensity = true;
	FirstPersonCamera->PostProcessSettings.BloomIntensity = 0.1f;

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
	Mesh1P->SetupAttachment(FirstPersonCamera);
	Mesh1P->SetCastShadow(false);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->SetOnlyOwnerSee(true);

	GetMesh()->SetOwnerNoSee(true);
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

	DefaultCameraPos = FirstPersonCamera->GetRelativeLocation();
	FirstPersonCamera->SetFieldOfView(95.0f);

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
}

void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateProceduralCameraBob(DeltaTime);
	UpdateWeaponSway(DeltaTime);

	// Smooth FOV interpolation for ADS tactical zoom
	const bool bIsAiming = (InventoryComponent && InventoryComponent->GetActiveWeapon() && InventoryComponent->GetActiveWeapon()->IsAimingDownSights());
	const float TargetFOV = bIsAiming ? 68.0f : 95.0f;
	FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaTime, 16.0f));

	// Update HUD Crosshair with current weapon spread
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

void AFPSCharacter::UpdateProceduralCameraBob(float DeltaTime)
{
	if (!FPSMovementComponent || !FPSMovementComponent->IsMovingOnGround())
	{
		// Smoothly return to default camera position
		FirstPersonCamera->SetRelativeLocation(FMath::VInterpTo(FirstPersonCamera->GetRelativeLocation(), DefaultCameraPos, DeltaTime, 8.0f));
		return;
	}

	float Speed = GetVelocity().Size2D();
	if (Speed > 10.0f)
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
	else
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
