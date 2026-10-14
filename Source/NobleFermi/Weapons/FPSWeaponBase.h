#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeaponBase.generated.h"

class USkeletalMeshComponent;
class USoundBase;
class UParticleSystem;
class UCurveFloat;

UENUM(BlueprintType)
enum class EFPSFireMode : uint8
{
	FullAuto,
	Burst,
	SemiAuto
};

UENUM(BlueprintType)
enum class EFPSWeaponState : uint8
{
	Idle,
	Firing,
	Reloading,
	Holstered
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedSignature, int32, CurrentAmmo, int32, ReserveAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFiredSignature);

/**
 * AAA FPS Weapon Base Class
 * Features procedural recoil curves, dynamic spread bloom, ADS alignment, and surface penetration traces.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API AFPSWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AFPSWeaponBase();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnAmmoChangedSignature OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponFiredSignature OnWeaponFired;

	// Core Firing Actions
	UFUNCTION(BlueprintCallable, Category = "Weapon|Actions")
	virtual void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Actions")
	virtual void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Actions")
	virtual void Reload();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Actions")
	void SetADS(bool bInADS);

	// Getters
	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	float GetCurrentSpread() const { return CurrentSpread; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	bool IsAimingDownSights() const { return bIsAimingDownSights; }

	// Weapon Attributes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	FName WeaponID = "NF98_Carbine";

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	FText WeaponDisplayName = FText::FromString("NF-98 Carbine");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Mechanics")
	EFPSFireMode FireMode = EFPSFireMode::FullAuto;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Mechanics")
	float FireRateRPM = 750.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Mechanics")
	float BaseDamage = 32.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Mechanics")
	float MaxRange = 10000.0f; // 100 meters

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 MagazineCapacity = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 MaxReserveAmmo = 180;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	float TacticalReloadDuration = 2.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	float EmptyReloadDuration = 2.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stealth")
	bool bIsSuppressed = false;

	// Recoil & Bloom Tuning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilPitchKick = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilYawSpread = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilRecoverySpeed = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Bloom")
	float HipfireMinSpread = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Bloom")
	float ADSMinSpread = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Bloom")
	float MaxSpread = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Bloom")
	float SpreadPerShot = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> ReloadSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> DryFireSound;

protected:
	virtual void BeginPlay() override;
	virtual void FireShot();

	UPROPERTY()
	EFPSWeaponState CurrentState = EFPSWeaponState::Idle;

	UPROPERTY()
	int32 CurrentAmmo = 30;

	UPROPERTY()
	int32 ReserveAmmo = 120;

	UPROPERTY()
	bool bIsAimingDownSights = false;

	float CurrentSpread = 1.5f;
	int32 ShotsFiredInBurst = 0;
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;

private:
	void FinishReload();
	float GetTimeBetweenShots() const { return 60.0f / FMath::Max(1.0f, FireRateRPM); }
};
