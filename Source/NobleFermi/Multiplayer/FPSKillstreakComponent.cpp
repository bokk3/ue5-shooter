#include "Multiplayer/FPSKillstreakComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

UFPSKillstreakComponent::UFPSKillstreakComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Classic MW2 Loadout: UAV (3), Predator Missile (5), Tactical Nuke (25)
	EquippedStreaks.Add({ EFPSKillstreakType::UAV, FText::FromString("UAV Recon"), 3, TEXT("Friendly UAV online, scanning AO.") });
	EquippedStreaks.Add({ EFPSKillstreakType::PredatorMissile, FText::FromString("Predator Missile"), 5, TEXT("Predator missile awaiting your coordinates.") });
	EquippedStreaks.Add({ EFPSKillstreakType::TacticalNuke, FText::FromString("Tactical Nuke"), 25, TEXT("TACTICAL NUKE READY! TURN KEY TO DETONATE!") });
}

void UFPSKillstreakComponent::BeginPlay()
{
	Super::BeginPlay();
	AvailableStreaks.Empty();
}

void UFPSKillstreakComponent::NotifyKillRecorded(int32 CurrentStreak)
{
	for (const FFPSKillstreakDef& Def : EquippedStreaks)
	{
		if (Def.RequiredKills == CurrentStreak)
		{
			AvailableStreaks.AddUnique(Def.Type);
			OnKillstreakEarned.Broadcast(Def.Type);
			break;
		}
	}
}

void UFPSKillstreakComponent::NotifyPlayerDied()
{
	// In classic MW2, earned streaks remain in inventory upon respawn, but the streak counter itself resets.
}

bool UFPSKillstreakComponent::HasStreakReady(EFPSKillstreakType StreakType) const
{
	return AvailableStreaks.Contains(StreakType);
}

bool UFPSKillstreakComponent::ActivateStreak(EFPSKillstreakType StreakType)
{
	if (!HasStreakReady(StreakType)) return false;

	AvailableStreaks.Remove(StreakType);
	OnKillstreakActivated.Broadcast(StreakType);

	switch (StreakType)
	{
	case EFPSKillstreakType::UAV:
		ExecuteUAV();
		break;
	case EFPSKillstreakType::PredatorMissile:
		ExecutePredatorMissile();
		break;
	case EFPSKillstreakType::TacticalNuke:
		ExecuteTacticalNuke();
		break;
	default:
		break;
	}

	return true;
}

void UFPSKillstreakComponent::ExecuteUAV()
{
	// Sweeps minimap, exposes all enemy locations for 30 seconds
}

void UFPSKillstreakComponent::ExecutePredatorMissile()
{
	// Player enters rugged military laptop thermal view, piloting a missile from high altitude down to the ground
}

void UFPSKillstreakComponent::ExecuteTacticalNuke()
{
	// Starts the 10 second sirens, red flashing alert, and terminates the match with a win
	OnNukeCountdownStarted.Broadcast(10.0f);
}
