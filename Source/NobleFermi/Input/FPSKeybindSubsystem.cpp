#include "Input/FPSKeybindSubsystem.h"
#include "Misc/ConfigCacheIni.h"

void UFPSKeybindSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SetupDefaultBindings();
	LoadSavedKeybinds();
}

void UFPSKeybindSubsystem::SetupDefaultBindings()
{
	ActionBindings.Empty();

	ActionBindings.Add({ "MoveForward", FText::FromString("Move Forward"), EKeys::W, FKey(), EKeys::Gamepad_LeftStick_Up });
	ActionBindings.Add({ "MoveBackward", FText::FromString("Move Backward"), EKeys::S, FKey(), EKeys::Gamepad_LeftStick_Down });
	ActionBindings.Add({ "MoveLeft", FText::FromString("Strafe Left"), EKeys::A, FKey(), EKeys::Gamepad_LeftStick_Left });
	ActionBindings.Add({ "MoveRight", FText::FromString("Strafe Right"), EKeys::D, FKey(), EKeys::Gamepad_LeftStick_Right });
	ActionBindings.Add({ "Jump", FText::FromString("Jump / Mantle"), EKeys::SpaceBar, FKey(), EKeys::Gamepad_FaceButton_Bottom });
	ActionBindings.Add({ "Crouch", FText::FromString("Crouch / Slide"), EKeys::LeftControl, EKeys::C, EKeys::Gamepad_FaceButton_Right });
	ActionBindings.Add({ "Sprint", FText::FromString("Tactical Sprint"), EKeys::LeftShift, FKey(), EKeys::Gamepad_LeftThumbstick });
	ActionBindings.Add({ "Fire", FText::FromString("Fire Weapon"), EKeys::LeftMouseButton, FKey(), EKeys::Gamepad_RightTrigger });
	ActionBindings.Add({ "ADS", FText::FromString("Aim Down Sights"), EKeys::RightMouseButton, FKey(), EKeys::Gamepad_LeftTrigger });
	ActionBindings.Add({ "Reload", FText::FromString("Reload Weapon"), EKeys::R, FKey(), EKeys::Gamepad_FaceButton_Left });
	ActionBindings.Add({ "Melee", FText::FromString("Quick Melee"), EKeys::V, EKeys::ThumbMouseButton, EKeys::Gamepad_RightThumbstick });
	ActionBindings.Add({ "Tactical", FText::FromString("Tactical Equipment"), EKeys::Q, FKey(), EKeys::Gamepad_LeftShoulder });
	ActionBindings.Add({ "Lethal", FText::FromString("Lethal Grenade"), EKeys::G, EKeys::ThumbMouseButton2, EKeys::Gamepad_RightShoulder });
	ActionBindings.Add({ "Interact", FText::FromString("Interact / Use"), EKeys::F, EKeys::E, EKeys::Gamepad_FaceButton_Top });
}

TArray<FFPSActionBinding> UFPSKeybindSubsystem::GetAllBindings() const
{
	return ActionBindings;
}

FName UFPSKeybindSubsystem::FindConflict(FKey KeyToCheck, FName IgnoreActionName) const
{
	if (!KeyToCheck.IsValid()) return NAME_None;

	for (const FFPSActionBinding& Binding : ActionBindings)
	{
		if (Binding.ActionName == IgnoreActionName) continue;

		if (Binding.PrimaryKey == KeyToCheck || Binding.SecondaryKey == KeyToCheck)
		{
			return Binding.ActionName;
		}
	}
	return NAME_None;
}

bool UFPSKeybindSubsystem::RemapActionKey(FName ActionName, FKey NewKey, bool bIsSecondary)
{
	for (FFPSActionBinding& Binding : ActionBindings)
	{
		if (Binding.ActionName == ActionName)
		{
			if (bIsSecondary)
			{
				Binding.SecondaryKey = NewKey;
			}
			else
			{
				Binding.PrimaryKey = NewKey;
			}
			SaveKeybinds();
			OnKeybindsUpdated.Broadcast();
			return true;
		}
	}
	return false;
}

void UFPSKeybindSubsystem::ResetAllToDefaults()
{
	SetupDefaultBindings();
	SaveKeybinds();
	OnKeybindsUpdated.Broadcast();
}

void UFPSKeybindSubsystem::SaveKeybinds()
{
	if (!GConfig) return;

	FString ConfigPath = GGameUserSettingsIni;
	for (const FFPSActionBinding& Binding : ActionBindings)
	{
		FString PriKeyName = FString::Printf(TEXT("Key_%s_Primary"), *Binding.ActionName.ToString());
		FString SecKeyName = FString::Printf(TEXT("Key_%s_Secondary"), *Binding.ActionName.ToString());
		GConfig->SetString(TEXT("FPSKeybinds"), *PriKeyName, *Binding.PrimaryKey.ToString(), ConfigPath);
		GConfig->SetString(TEXT("FPSKeybinds"), *SecKeyName, *Binding.SecondaryKey.ToString(), ConfigPath);
	}
	GConfig->Flush(false, ConfigPath);
}

void UFPSKeybindSubsystem::LoadSavedKeybinds()
{
	if (!GConfig) return;

	FString ConfigPath = GGameUserSettingsIni;
	for (FFPSActionBinding& Binding : ActionBindings)
	{
		FString PriKeyName = FString::Printf(TEXT("Key_%s_Primary"), *Binding.ActionName.ToString());
		FString SecKeyName = FString::Printf(TEXT("Key_%s_Secondary"), *Binding.ActionName.ToString());

		FString SavedPri, SavedSec;
		if (GConfig->GetString(TEXT("FPSKeybinds"), *PriKeyName, SavedPri, ConfigPath) && !SavedPri.IsEmpty())
		{
			Binding.PrimaryKey = FKey(*SavedPri);
		}
		if (GConfig->GetString(TEXT("FPSKeybinds"), *SecKeyName, SavedSec, ConfigPath) && !SavedSec.IsEmpty())
		{
			Binding.SecondaryKey = FKey(*SavedSec);
		}
	}
}
