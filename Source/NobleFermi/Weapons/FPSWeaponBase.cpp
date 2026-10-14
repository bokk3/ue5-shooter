#include "Weapons/FPSWeaponBase.h"
#include "Combat/FPSHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

AFPSWeaponBase::AFPSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
}

void AFPSWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmmo = MagazineCapacity;
	ReserveAmmo = MaxReserveAmmo;
	CurrentSpread = HipfireMinSpread;
	CurrentState = EFPSWeaponState::Idle;
}

void AFPSWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Spread recovery over time towards target min spread
	float TargetSpread = bIsAimingDownSights ? ADSMinSpread : HipfireMinSpread;
	CurrentSpread = FMath::FInterpTo(CurrentSpread, TargetSpread, DeltaTime, RecoilRecoverySpeed);
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

	// Weapon Audio
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}

	// Trace line calculation from Owner Camera
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (PawnOwner)
	{
		APlayerController* PC = Cast<APlayerController>(PawnOwner->GetController());
		if (PC && PC->PlayerCameraManager)
		{
			FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
			FRotator CameraRot = PC->PlayerCameraManager->GetCameraRotation();

			// Add dynamic spread bloom
			CurrentSpread = FMath::Min(MaxSpread, CurrentSpread + SpreadPerShot);
			float SpreadHalfAngleRad = FMath::DegreesToRadians(CurrentSpread * 0.5f);
			FVector ShotDirection = FMath::VRandCone(CameraRot.Vector(), SpreadHalfAngleRad);

			FVector TraceEnd = CameraLoc + (ShotDirection * MaxRange);

			FHitResult HitResult;
			FCollisionQueryParams TraceParams;
			TraceParams.AddIgnoredActor(this);
			TraceParams.AddIgnoredActor(PawnOwner);
			TraceParams.bReturnPhysicalMaterial = true;

			UWorld* World = GetWorld();
			if (World && World->LineTraceSingleByChannel(HitResult, CameraLoc, TraceEnd, ECC_Visibility, TraceParams))
			{
				AActor* HitActor = HitResult.GetActor();
				if (HitActor)
				{
					UFPSHealthComponent* HealthComp = HitActor->FindComponentByClass<UFPSHealthComponent>();
					if (HealthComp)
					{
						HealthComp->ApplyDamage(BaseDamage, PawnOwner, HitResult);
					}
				}
			}

			// Apply camera recoil kick
			float PitchKick = -RecoilPitchKick * (bIsAimingDownSights ? 0.6f : 1.0f);
			float YawKick = FMath::RandRange(-RecoilYawSpread, RecoilYawSpread);
			PC->AddPitchInput(PitchKick);
			PC->AddYawInput(YawKick);
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

	if (ReloadSound)
	{
		UGameplayStatics::PlaySound2D(this, ReloadSound);
	}

	float Duration = (CurrentAmmo == 0) ? EmptyReloadDuration : TacticalReloadDuration;
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AFPSWeaponBase::FinishReload, Duration, false);
}

void AFPSWeaponBase::FinishReload()
{
	int32 NeededAmmo = MagazineCapacity - CurrentAmmo;
	int32 AmmoToLoad = FMath::Min(NeededAmmo, ReserveAmmo);

	CurrentAmmo += AmmoToLoad;
	ReserveAmmo -= AmmoToLoad;

	CurrentState = EFPSWeaponState::Idle;
	OnAmmoChanged.Broadcast(CurrentAmmo, ReserveAmmo);
}

void AFPSWeaponBase::SetADS(bool bInADS)
{
	bIsAimingDownSights = bInADS;
}
