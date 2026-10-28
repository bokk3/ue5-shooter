#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeaponBase.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class USceneComponent;
class USoundBase;
class UParticleSystem;
class UCurveFloat;
class UProceduralMeshComponent;
class UFPSWeaponDataAsset;

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
UCLASS(BlueprintType, Blueprintable)
class NOBLEFERMI_API AFPSWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AFPSWeaponBase();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	/** Embedded Data Asset defining PBR meshes, detached magazine, sockets, and combat ballistics */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Data")
	TObjectPtr<class UFPSWeaponDataAsset> WeaponDataAsset;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Data")
	void ApplyWeaponDataAsset(class UFPSWeaponDataAsset* InDataAsset);

	UFUNCTION(BlueprintPure, Category = "Weapon|Transforms")
	FTransform GetRightHandGripTransform() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Transforms")
	FTransform GetLeftHandGripTransform() const;

	/** Authoritative 3D Procedural NF-98 Carbine Geometry Component (Guaranteed visibility in packaged builds) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Procedural")
	TObjectPtr<UProceduralMeshComponent> CarbineProcMesh;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Procedural")
	void BuildProceduralCarbineGeometry();

	// Procedural 3D Weapon Model Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> AK47StaticMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> ReceiverMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> TopRailMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> HandguardMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> BarrelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> MuzzleCompMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> GripMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> StockMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> BufferTubeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<USceneComponent> MagazineRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> MagazineMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> SightBaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> SightHoodMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> ReticleDotMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> FrontSightPostMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Model")
	TObjectPtr<UStaticMeshComponent> FrontSightDotMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	TObjectPtr<class UFPSGunsmithComponent> GunsmithComponent;

	UFUNCTION(BlueprintPure, Category = "Weapon|Gunsmith")
	class UFPSGunsmithComponent* GetGunsmithComponent() const { return GunsmithComponent; }

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

	UFUNCTION(BlueprintCallable, Category = "Weapon|Cosmetics")
	void ApplyLoadoutCosmetics(int32 CamoIndex, int32 HeadgearIndex, int32 ArmorIndex, int32 WeaponFinishIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Actions")
	void RefillBotAmmo();

	// Getters
	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	FString GetWeaponName() const { return WeaponDisplayName.ToString(); }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	int32 GetCurrentClipAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	float GetCurrentSpread() const { return CurrentSpread; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	bool IsAimingDownSights() const { return bIsAimingDownSights; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	bool IsReloading() const { return CurrentState == EFPSWeaponState::Reloading; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	float GetReloadProgress() const { return (CurrentState == EFPSWeaponState::Reloading && ReloadDuration > 0.01f) ? FMath::Clamp(ReloadElapsedTime / ReloadDuration, 0.0f, 1.0f) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	int32 GetMagazineCapacity() const { return MagazineCapacity; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Stats")
	float GetADSAlpha() const { return CurrentADSAlpha; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Transforms")
	FVector GetHipfireLocation() const { return HipfireRelativeLocation; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Transforms")
	FRotator GetHipfireRotation() const { return HipfireRelativeRotation; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Transforms")
	FVector GetADSLocation() const { return ADSRelativeLocation; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Transforms")
	FRotator GetADSRotation() const { return ADSRelativeRotation; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Transforms")
	FVector GetThirdPersonLocation() const { return ThirdPersonRelativeLocation; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Transforms")
	FRotator GetThirdPersonRotation() const { return ThirdPersonRelativeRotation; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Transforms")
	FVector GetForegripWorldLocation() const;

	// Weapon Transforms
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Transforms")
	FVector HipfireRelativeLocation = FVector(26.0f, 14.0f, -13.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Transforms")
	FRotator HipfireRelativeRotation = FRotator(0.5f, -1.0f, -1.5f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Transforms")
	FVector ADSRelativeLocation = FVector(20.0f, 0.0f, -6.8f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Transforms")
	FRotator ADSRelativeRotation = FRotator(0.0f, 0.0f, 0.0f);

	// 3rd Person Tactical Mesh Attachment Transforms (relative to hand_r / weapon_r)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Transforms")
	FVector ThirdPersonRelativeLocation = FVector(-4.0f, -2.0f, -6.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Transforms")
	FRotator ThirdPersonRelativeRotation = FRotator(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Transforms")
	float ADSSpeed = 16.0f;

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

	// Recoil & Bloom Tuning (Calibrated to AAA tactical shooter handling with spring-back recovery)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilPitchKick = 0.32f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilYawSpread = 0.10f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilRecoverySpeed = 16.0f;

	UPROPERTY(Transient)
	float AccumulatedPitchRecoil = 0.0f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TObjectPtr<USoundBase> ShellDropSound;

	/** Calibrated subtle brass casing volume (not deafening, realistic metallic clink) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	float ShellDropBaseVolume = 0.24f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	float ShellDropMinDistance = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	float ShellDropMaxDistance = 1400.0f; // 14 meters max audible distance

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	float GunshotMaxAudibleDistance = 22000.0f; // 220 meters full combat radius

	UFUNCTION()
	void PlayDelayedShellDrop(FVector DropLocation);

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

	// Procedural Animation State
	float CurrentADSAlpha = 0.0f;
	FVector RecoilOffsetLoc = FVector::ZeroVector;
	FRotator RecoilOffsetRot = FRotator::ZeroRotator;
	FRotator CurrentSwayRot = FRotator::ZeroRotator;
	FRotator LastControlRot = FRotator::ZeroRotator;

	bool bIsEmptyReload = false;
	float ReloadDuration = 2.4f;
	float ReloadElapsedTime = 0.0f;
	FVector MagazineBaseLoc = FVector(8.0f, 0.0f, -2.8f);

private:
	void FinishReload();
	float GetTimeBetweenShots() const { return 60.0f / FMath::Max(1.0f, FireRateRPM); }
};
