#pragma once

#include "CoreMinimal.h"
#include "Weapons/FPSAttachmentData.h"
#include "Combat/FPSProjectileBase.h"
#include "Multiplayer/FPSPerkComponent.h"
#include "FPSLoadoutTypes.generated.h"

class AFPSWeaponBase;

/**
 * Complete Create-A-Class Loadout Data Structure
 * Encapsulates primary & secondary weapons with attachments, lethal/tactical equipment, and Pick-3 perks.
 */
USTRUCT(BlueprintType)
struct FFPSCustomClassLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	FString ClassName = TEXT("Assault");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout|Primary")
	TSubclassOf<AFPSWeaponBase> PrimaryWeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout|Primary")
	TArray<FFPSAttachmentDefinition> PrimaryAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout|Secondary")
	TSubclassOf<AFPSWeaponBase> SecondaryWeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout|Secondary")
	TArray<FFPSAttachmentDefinition> SecondaryAttachments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout|Equipment")
	EFPSProjectileType LethalEquipment = EFPSProjectileType::Semtex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout|Equipment")
	EFPSProjectileType TacticalEquipment = EFPSProjectileType::Flashbang;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout|Perks")
	EFPSPerkSlot1 PerkSlot1 = EFPSPerkSlot1::SleightOfHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout|Perks")
	EFPSPerkSlot2 PerkSlot2 = EFPSPerkSlot2::StoppingPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout|Perks")
	EFPSPerkSlot3 PerkSlot3 = EFPSPerkSlot3::Ninja;
};
