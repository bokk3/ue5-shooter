#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSPerkComponent.generated.h"

UENUM(BlueprintType)
enum class EFPSPerkSlot1 : uint8
{
	None,
	Marathon,       // Unlimited tactical sprint duration
	SleightOfHand,  // 50% faster reload speed & faster weapon swap
	Scavenger       // Resupply ammo bags from fallen enemies
};

UENUM(BlueprintType)
enum class EFPSPerkSlot2 : uint8
{
	None,
	StoppingPower,  // +40% bullet damage
	Lightweight,    // +10% faster overall movement speed
	Hardline,       // Killstreaks require 1 less kill (e.g. UAV at 2 kills)
	ColdBlooded     // Undetectable by enemy radar & thermal optics
};

UENUM(BlueprintType)
enum class EFPSPerkSlot3 : uint8
{
	None,
	Ninja,          // Completely silent footsteps
	Commando,       // Increased melee reach
	SteadyAim       // 35% tighter hipfire spread
};

/**
 * Classic MW2 Pick-3 Perk Component
 * Modifies weapon damage, reload rates, sprint limits, and detection states.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSPerkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSPerkComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Perks|Loadout")
	EFPSPerkSlot1 PerkSlot1 = EFPSPerkSlot1::SleightOfHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Perks|Loadout")
	EFPSPerkSlot2 PerkSlot2 = EFPSPerkSlot2::StoppingPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Perks|Loadout")
	EFPSPerkSlot3 PerkSlot3 = EFPSPerkSlot3::Ninja;

	// Query perk buffs
	UFUNCTION(BlueprintPure, Category = "FPS Perks")
	float GetDamageMultiplier() const { return PerkSlot2 == EFPSPerkSlot2::StoppingPower ? 1.4f : 1.0f; }

	UFUNCTION(BlueprintPure, Category = "FPS Perks")
	float GetReloadSpeedMultiplier() const { return PerkSlot1 == EFPSPerkSlot1::SleightOfHand ? 0.5f : 1.0f; }

	UFUNCTION(BlueprintPure, Category = "FPS Perks")
	float GetMovementSpeedMultiplier() const { return PerkSlot2 == EFPSPerkSlot2::Lightweight ? 1.10f : 1.0f; }

	UFUNCTION(BlueprintPure, Category = "FPS Perks")
	float GetHipfireSpreadMultiplier() const { return PerkSlot3 == EFPSPerkSlot3::SteadyAim ? 0.65f : 1.0f; }

	UFUNCTION(BlueprintPure, Category = "FPS Perks")
	int32 GetKillstreakRequirementDiscount() const { return PerkSlot2 == EFPSPerkSlot2::Hardline ? 1 : 0; }

	UFUNCTION(BlueprintPure, Category = "FPS Perks")
	bool HasSilentFootsteps() const { return PerkSlot3 == EFPSPerkSlot3::Ninja; }

	UFUNCTION(BlueprintPure, Category = "FPS Perks")
	bool IsHiddenFromRadar() const { return PerkSlot2 == EFPSPerkSlot2::ColdBlooded; }
};
