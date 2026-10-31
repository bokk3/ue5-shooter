#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Lyra Native Gameplay Tags for Noble Fermi Tactical Framework
 */
struct NOBLEFERMI_API FFPSGameplayTags
{
public:
	static const FFPSGameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();

	// Init States
	FGameplayTag InitState_Spawned;
	FGameplayTag InitState_DataAvailable;
	FGameplayTag InitState_DataInitialized;
	FGameplayTag InitState_GameplayReady;

	// Character States
	FGameplayTag State_Movement_Idle;
	FGameplayTag State_Movement_Walking;
	FGameplayTag State_Movement_Running;
	FGameplayTag State_Movement_Sprinting;
	FGameplayTag State_Movement_Crouching;
	FGameplayTag State_Movement_Sliding;
	FGameplayTag State_Movement_InAir;

	// Combat & Aiming States
	FGameplayTag State_Combat_Aiming;
	FGameplayTag State_Combat_FirstPersonOptic;
	FGameplayTag State_Combat_Firing;
	FGameplayTag State_Combat_Reloading;
	FGameplayTag State_Combat_Dead;

	// Equipment Slots
	FGameplayTag Equipment_Slot_Primary;
	FGameplayTag Equipment_Slot_Secondary;
	FGameplayTag Equipment_Slot_Tactical;
	FGameplayTag Equipment_Slot_Armor;

	// Ability Input Tags
	FGameplayTag InputTag_PrimaryFire;
	FGameplayTag InputTag_SecondaryFire;
	FGameplayTag InputTag_ADS;
	FGameplayTag InputTag_Reload;
	FGameplayTag InputTag_Sprint;
	FGameplayTag InputTag_Crouch;
	FGameplayTag InputTag_Slide;
	FGameplayTag InputTag_Equipment_Lethal;
	FGameplayTag InputTag_Equipment_Tactical;

private:
	static FFPSGameplayTags GameplayTags;
};
