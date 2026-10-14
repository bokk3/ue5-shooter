#include "Character/FPSCharacter.h"
#include "Character/FPSCharacterMovementComponent.h"
#include "Combat/FPSHealthComponent.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "UI/FPSHUDWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
	Mesh1P->SetupAttachment(FirstPersonCamera);
	Mesh1P->SetCastShadow(false);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->SetOnlyOwnerSee(true);

	GetMesh()->SetOwnerNoSee(true);

	HealthComponent = CreateDefaultSubobject<UFPSHealthComponent>(TEXT("HealthComponent"));
	InventoryComponent = CreateDefaultSubobject<UFPSInventoryComponent>(TEXT("InventoryComponent"));
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultCameraPos = FirstPersonCamera->GetRelativeLocation();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && HUDWidgetClass)
	{
		ActiveHUDWidget = CreateWidget<UFPSHUDWidget>(PC, HUDWidgetClass);
		if (ActiveHUDWidget)
		{
			ActiveHUDWidget->AddToViewport();
		}
	}
}

void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateProceduralCameraBob(DeltaTime);
	UpdateWeaponSway(DeltaTime);

	// Update HUD Crosshair with current weapon spread
	if (ActiveHUDWidget && InventoryComponent)
	{
		AFPSWeaponBase* Weapon = InventoryComponent->GetActiveWeapon();
		if (Weapon)
		{
			ActiveHUDWidget->UpdateCrosshairSpread(Weapon->GetCurrentSpread());
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

	// Sway lag
	FRotator TargetSway = FRotator(-DeltaRot.Pitch * 1.5f, DeltaRot.Yaw * 1.5f, DeltaRot.Yaw * 0.8f);
	CurrentWeaponSwayRot = FMath::RInterpTo(CurrentWeaponSwayRot, TargetSway, DeltaTime, 10.0f);

	Mesh1P->SetRelativeRotation(CurrentWeaponSwayRot);
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
	AddControllerYawInput(Rate);
}

void AFPSCharacter::LookUp(float Rate)
{
	AddControllerPitchInput(Rate);
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
		FirstPersonCamera->SetFieldOfView(80.0f); // Zoom in
	}
}

void AFPSCharacter::StopADS()
{
	if (InventoryComponent && InventoryComponent->GetActiveWeapon())
	{
		InventoryComponent->GetActiveWeapon()->SetADS(false);
		FirstPersonCamera->SetFieldOfView(103.0f); // Default FOV
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
