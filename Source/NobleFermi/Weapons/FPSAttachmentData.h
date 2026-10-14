#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FPSAttachmentData.generated.h"

class UStaticMesh;
class USoundBase;

UENUM(BlueprintType)
enum class EFPSAttachmentSlot : uint8
{
	Optic        UMETA(DisplayName = "Optic / Sight"),
	Muzzle       UMETA(DisplayName = "Muzzle Device"),
	Underbarrel  UMETA(DisplayName = "Underbarrel Grip / Launcher"),
	Magazine     UMETA(DisplayName = "Magazine & Caliber"),
	Stock        UMETA(DisplayName = "Stock & Buffer Tube")
};

/**
 * Data structure defining an attachment in the Gunsmith system
 */
USTRUCT(BlueprintType)
struct FFPSAttachmentDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Identity")
	FName AttachmentID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Identity")
	EFPSAttachmentSlot Slot = EFPSAttachmentSlot::Optic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Identity")
	FText DisplayName = FText::FromString("Attachment");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Visuals")
	TSoftObjectPtr<UStaticMesh> AttachmentMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Visuals")
	FName SocketName = "OpticSocket";

	// Recoil: Multiplier applied to Pitch kick and Yaw spread (< 1.0 reduces recoil)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Modifiers")
	float RecoilMultiplier = 1.0f;

	// Range: Multiplier applied to damage falloff distance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Modifiers")
	float RangeMultiplier = 1.0f;

	// ADS FOV Zoom: Multiplier applied to base camera FOV (< 1.0 zooms in, e.g. 0.4 for 4x ACOG)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Modifiers")
	float AdsFovMultiplier = 1.0f;

	// ADS Transition Time Modifier (< 1.0 speeds up ADS, > 1.0 slows down)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Modifiers")
	float AdsSpeedModifier = 1.0f;

	// Additional ammo added to weapon magazine capacity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Modifiers")
	int32 MagazineCapacityBonus = 0;

	// Reload duration multiplier (< 1.0 = faster reload)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Modifiers")
	float ReloadDurationMultiplier = 1.0f;

	// Stealth: Eliminates gunfire pings from enemy minimap & silences audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Stealth")
	bool bSilenced = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment|Stealth")
	TSoftObjectPtr<USoundBase> SuppressedFireSound;
};
