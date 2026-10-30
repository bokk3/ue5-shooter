// =========================================================================================
// NOBLE FERMI // TACTICAL WEAPON DATA ASSET & EMBEDDED METADATA REGISTRY
// Primary Data Asset encapsulating PBR meshes, detachable magazines, ergonomic grip sockets,
// and authoritative weapon performance parameters (damage, fire rate, recoil, reload).
// =========================================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FPSWeaponDataAsset.generated.h"

class UStaticMesh;
class USoundBase;

UENUM(BlueprintType)
enum class EFPSWeaponArchetype : uint8
{
	AssaultRifle  UMETA(DisplayName = "Assault Rifle / Tactical Carbine"),
	BattleRifle   UMETA(DisplayName = "Battle Rifle / Designated Marksman Rifle"),
	Shotgun       UMETA(DisplayName = "Combat Shotgun"),
	Pistol        UMETA(DisplayName = "Tactical Sidearm"),
	Throwable     UMETA(DisplayName = "Tactical Throwable (Frag / Flash)"),
	Melee         UMETA(DisplayName = "Tactical Melee Blade")
};

UCLASS(BlueprintType)
class NOBLEFERMI_API UFPSWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Identification & Visual Trade Dress
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
	FString WeaponID = TEXT("W01_NF98");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
	FText WeaponDisplayName = FText::FromString(TEXT("NF-98 Carbine"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
	EFPSWeaponArchetype Archetype = EFPSWeaponArchetype::AssaultRifle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
	FString Caliber = TEXT("5.56x45mm NATO");

	// 2D Tactical Preview Icon / Model Render for UI and Weapon Wheel Modal
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
	TObjectPtr<class UTexture2D> WeaponIcon = nullptr;

	// 3D Modular Meshes (Discrete Receiver & Detachable Magazine)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Meshes")
	TObjectPtr<UStaticMesh> ReceiverMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Meshes")
	TObjectPtr<UStaticMesh> MagazineMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Meshes")
	TObjectPtr<UStaticMesh> SecondaryPartMesh = nullptr; // e.g. Grenade Pin or Spoon

	// Combat Ballistics & Capacities
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ballistics")
	int32 MagazineCapacity = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ballistics")
	int32 ReserveAmmo = 180;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ballistics")
	float BaseDamage = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ballistics")
	float FireRateRPM = 750.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ballistics")
	float EffectiveRangeMeters = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ballistics")
	float ReloadTimeSeconds = 2.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ballistics")
	float EmptyReloadTimeSeconds = 2.6f;

	// Recoil & Handling Dynamics
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilPitch = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilYawSpread = 0.12f;

	// Ergonomic Hand Grip Sockets (Computed geometrically from mesh)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FTransform RightHandGripTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FTransform LeftHandGripTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FVector MuzzleSocketOffset = FVector(42.0f, 0.0f, 2.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FVector MagWellSocketOffset = FVector(6.0f, 0.0f, -1.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FVector OpticSocketOffset = FVector(4.0f, 0.0f, 6.2f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Sockets")
	FVector EjectionPortOffset = FVector(2.0f, 2.2f, 3.0f);
};
