#include "Lyra/AbilitySystem/FPSGameplayTags.h"
#include "GameplayTagsManager.h"

FFPSGameplayTags FFPSGameplayTags::GameplayTags;

void FFPSGameplayTags::InitializeNativeGameplayTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	GameplayTags.InitState_Spawned = Manager.AddNativeGameplayTag(
		FName("InitState.Spawned"),
		FString("Actor has spawned in the world.")
	);
	GameplayTags.InitState_DataAvailable = Manager.AddNativeGameplayTag(
		FName("InitState.DataAvailable"),
		FString("PlayerState and Controller have replicated and are available.")
	);
	GameplayTags.InitState_DataInitialized = Manager.AddNativeGameplayTag(
		FName("InitState.DataInitialized"),
		FString("PawnData and AbilitySystemComponent have been initialized.")
	);
	GameplayTags.InitState_GameplayReady = Manager.AddNativeGameplayTag(
		FName("InitState.GameplayReady"),
		FString("Pawn is fully initialized and ready for player input & gameplay.")
	);

	// Movement
	GameplayTags.State_Movement_Idle = Manager.AddNativeGameplayTag(FName("State.Movement.Idle"));
	GameplayTags.State_Movement_Walking = Manager.AddNativeGameplayTag(FName("State.Movement.Walking"));
	GameplayTags.State_Movement_Running = Manager.AddNativeGameplayTag(FName("State.Movement.Running"));
	GameplayTags.State_Movement_Sprinting = Manager.AddNativeGameplayTag(FName("State.Movement.Sprinting"));
	GameplayTags.State_Movement_Crouching = Manager.AddNativeGameplayTag(FName("State.Movement.Crouching"));
	GameplayTags.State_Movement_Sliding = Manager.AddNativeGameplayTag(FName("State.Movement.Sliding"));
	GameplayTags.State_Movement_InAir = Manager.AddNativeGameplayTag(FName("State.Movement.InAir"));

	// Combat
	GameplayTags.State_Combat_Aiming = Manager.AddNativeGameplayTag(FName("State.Combat.Aiming"));
	GameplayTags.State_Combat_FirstPersonOptic = Manager.AddNativeGameplayTag(FName("State.Combat.FirstPersonOptic"));
	GameplayTags.State_Combat_Firing = Manager.AddNativeGameplayTag(FName("State.Combat.Firing"));
	GameplayTags.State_Combat_Reloading = Manager.AddNativeGameplayTag(FName("State.Combat.Reloading"));
	GameplayTags.State_Combat_Dead = Manager.AddNativeGameplayTag(FName("State.Combat.Dead"));

	// Equipment
	GameplayTags.Equipment_Slot_Primary = Manager.AddNativeGameplayTag(FName("Equipment.Slot.Primary"));
	GameplayTags.Equipment_Slot_Secondary = Manager.AddNativeGameplayTag(FName("Equipment.Slot.Secondary"));
	GameplayTags.Equipment_Slot_Tactical = Manager.AddNativeGameplayTag(FName("Equipment.Slot.Tactical"));
	GameplayTags.Equipment_Slot_Armor = Manager.AddNativeGameplayTag(FName("Equipment.Slot.Armor"));

	// Inputs
	GameplayTags.InputTag_PrimaryFire = Manager.AddNativeGameplayTag(FName("InputTag.PrimaryFire"));
	GameplayTags.InputTag_SecondaryFire = Manager.AddNativeGameplayTag(FName("InputTag.SecondaryFire"));
	GameplayTags.InputTag_ADS = Manager.AddNativeGameplayTag(FName("InputTag.ADS"));
	GameplayTags.InputTag_Reload = Manager.AddNativeGameplayTag(FName("InputTag.Reload"));
	GameplayTags.InputTag_Sprint = Manager.AddNativeGameplayTag(FName("InputTag.Sprint"));
	GameplayTags.InputTag_Crouch = Manager.AddNativeGameplayTag(FName("InputTag.Crouch"));
	GameplayTags.InputTag_Slide = Manager.AddNativeGameplayTag(FName("InputTag.Slide"));
	GameplayTags.InputTag_Equipment_Lethal = Manager.AddNativeGameplayTag(FName("InputTag.Equipment.Lethal"));
	GameplayTags.InputTag_Equipment_Tactical = Manager.AddNativeGameplayTag(FName("InputTag.Equipment.Tactical"));
}
