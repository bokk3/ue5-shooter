#include "UI/FPSLoadoutSelectWidget.h"

void UFPSLoadoutSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AvailableClasses.Num() == 0)
	{
		InitializeDefaultClasses();
	}

	OnLoadoutMenuRefreshed(AvailableClasses, SelectedClassIndex);
}

void UFPSLoadoutSelectWidget::SelectClass(int32 ClassIndex)
{
	if (AvailableClasses.IsValidIndex(ClassIndex))
	{
		SelectedClassIndex = ClassIndex;
		OnClassSelected.Broadcast(ClassIndex);
		OnLoadoutMenuRefreshed(AvailableClasses, SelectedClassIndex);
	}
}

FFPSCustomClassLoadout UFPSLoadoutSelectWidget::GetSelectedClass() const
{
	if (AvailableClasses.IsValidIndex(SelectedClassIndex))
	{
		return AvailableClasses[SelectedClassIndex];
	}
	return FFPSCustomClassLoadout();
}

void UFPSLoadoutSelectWidget::InitializeDefaultClasses()
{
	AvailableClasses.Empty();

	// 1. Assault
	FFPSCustomClassLoadout AssaultClass;
	AssaultClass.ClassName = TEXT("Custom 1: Assault (M4A1)");
	AssaultClass.PerkSlot1 = EFPSPerkSlot1::SleightOfHand;
	AssaultClass.PerkSlot2 = EFPSPerkSlot2::StoppingPower;
	AssaultClass.PerkSlot3 = EFPSPerkSlot3::Ninja;
	AssaultClass.LethalEquipment = EFPSProjectileType::Semtex;
	AssaultClass.TacticalEquipment = EFPSProjectileType::Flashbang;
	AvailableClasses.Add(AssaultClass);

	// 2. Spec Ops Stealth
	FFPSCustomClassLoadout StealthClass;
	StealthClass.ClassName = TEXT("Custom 2: Stealth Ops (Suppressed)");
	StealthClass.PerkSlot1 = EFPSPerkSlot1::Scavenger;
	StealthClass.PerkSlot2 = EFPSPerkSlot2::ColdBlooded;
	StealthClass.PerkSlot3 = EFPSPerkSlot3::Ninja;
	StealthClass.LethalEquipment = EFPSProjectileType::FragGrenade;
	StealthClass.TacticalEquipment = EFPSProjectileType::SmokeGrenade;
	AvailableClasses.Add(StealthClass);

	// 3. Heavy Gunner
	FFPSCustomClassLoadout HeavyClass;
	HeavyClass.ClassName = TEXT("Custom 3: Heavy Gunner");
	HeavyClass.PerkSlot1 = EFPSPerkSlot1::SleightOfHand;
	HeavyClass.PerkSlot2 = EFPSPerkSlot2::StoppingPower;
	HeavyClass.PerkSlot3 = EFPSPerkSlot3::SteadyAim;
	HeavyClass.LethalEquipment = EFPSProjectileType::FragGrenade;
	HeavyClass.TacticalEquipment = EFPSProjectileType::Flashbang;
	AvailableClasses.Add(HeavyClass);

	// 4. Recon Sniper
	FFPSCustomClassLoadout SniperClass;
	SniperClass.ClassName = TEXT("Custom 4: Sniper");
	SniperClass.PerkSlot1 = EFPSPerkSlot1::SleightOfHand;
	SniperClass.PerkSlot2 = EFPSPerkSlot2::StoppingPower;
	SniperClass.PerkSlot3 = EFPSPerkSlot3::Commando;
	SniperClass.LethalEquipment = EFPSProjectileType::ThrowingKnife;
	SniperClass.TacticalEquipment = EFPSProjectileType::SmokeGrenade;
	AvailableClasses.Add(SniperClass);

	// 5. Run & Gun Rusher
	FFPSCustomClassLoadout RusherClass;
	RusherClass.ClassName = TEXT("Custom 5: Rusher");
	RusherClass.PerkSlot1 = EFPSPerkSlot1::Marathon;
	RusherClass.PerkSlot2 = EFPSPerkSlot2::Lightweight;
	RusherClass.PerkSlot3 = EFPSPerkSlot3::Commando;
	RusherClass.LethalEquipment = EFPSProjectileType::Semtex;
	RusherClass.TacticalEquipment = EFPSProjectileType::Flashbang;
	AvailableClasses.Add(RusherClass);
}
