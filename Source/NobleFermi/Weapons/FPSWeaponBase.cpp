#include "Weapons/FPSWeaponBase.h"
#include "Combat/FPSHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Radar/FPSRadarSubsystem.h"
#include "Multiplayer/FPSPlayerState.h"
#include "Weapons/FPSGunsmithComponent.h"

AFPSWeaponBase::AFPSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));

	GunsmithComponent = CreateDefaultSubobject<UFPSGunsmithComponent>(TEXT("GunsmithComponent"));
}

void AFPSWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (GunsmithComponent)
	{
		GunsmithComponent->ApplyModifiersToWeapon(this);
		MagazineCapacity += GunsmithComponent->GetTotalMagazineCapacityBonus();
		TacticalReloadDuration *= GunsmithComponent->GetTotalReloadDurationMultiplier();
		EmptyReloadDuration *= GunsmithComponent->GetTotalReloadDurationMultiplier();
	}

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
			if (World)
			{
				UFPSRadarSubsystem* RadarSubsystem = World->GetSubsystem<UFPSRadarSubsystem>();
				if (RadarSubsystem)
				{
					EFPSPlayerTeam ShooterTeam = EFPSPlayerTeam::None;
					AFPSPlayerState* PS = PawnOwner->GetPlayerState<AFPSPlayerState>();
					if (PS)
					{
						ShooterTeam = PS->GetPlayerTeam();
					}
					RadarSubsystem->ReportGunfire(PawnOwner, PawnOwner->GetActorLocation(), ShooterTeam, bIsSuppressed);
				}

				if (World->LineTraceSingleByChannel(HitResult, CameraLoc, TraceEnd, ECC_Visibility, TraceParams))
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
			}

			// Apply camera recoil kick (scaled by Gunsmith attachment modifiers)
			float RecoilMod = GunsmithComponent ? GunsmithComponent->GetTotalRecoilMultiplier() : 1.0f;
			float PitchKick = -RecoilPitchKick * (bIsAimingDownSights ? 0.6f : 1.0f) * RecoilMod;
			float YawKick = FMath::RandRange(-RecoilYawSpread, RecoilYawSpread) * RecoilMod;
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
