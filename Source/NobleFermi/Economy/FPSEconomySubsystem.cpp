#include "Economy/FPSEconomySubsystem.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"

void UFPSEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	InitializeCatalog();
	InitializeWheelSlices();
	InitializeDailyBounties();

	// Default baseline unlocks
	UnlockedItemIDs.Add(TEXT("camo_digital"));
	UnlockedItemIDs.Add(TEXT("headgear_fast"));
	UnlockedItemIDs.Add(TEXT("vest_level4"));
	UnlockedItemIDs.Add(TEXT("weapon_steel"));

	LoadProfile();
}

void UFPSEconomySubsystem::InitializeCatalog()
{
	CatalogItems.Empty();

	auto AddItem = [this](const FString& ID, const FText& Name, const FString& Cat, int32 Gold, int32 Opal, bool bFree)
	{
		FFPSCatalogItem Item;
		Item.ItemID = ID;
		Item.DisplayName = Name;
		Item.Category = Cat;
		Item.GoldCost = Gold;
		Item.OpalCost = Opal;
		Item.bDefaultUnlocked = bFree;
		CatalogItems.Add(ID, Item);
	};

	// Camouflages
	AddItem(TEXT("camo_digital"), FText::FromString(TEXT("Urban Digital")), TEXT("camo"), 0, 0, true);
	AddItem(TEXT("camo_woodland"), FText::FromString(TEXT("Woodland Tactical")), TEXT("camo"), 2000, 30, false);
	AddItem(TEXT("camo_desert"), FText::FromString(TEXT("Desert Spec Ops")), TEXT("camo"), 2500, 40, false);
	AddItem(TEXT("camo_blackout"), FText::FromString(TEXT("Stealth Blackout")), TEXT("camo"), 3500, 60, false);

	// Headgear
	AddItem(TEXT("headgear_fast"), FText::FromString(TEXT("FAST Ballistic Helmet")), TEXT("headgear"), 0, 0, true);
	AddItem(TEXT("headgear_quadnods"), FText::FromString(TEXT("GPNVG-18 Quad-Nods")), TEXT("headgear"), 3000, 50, false);
	AddItem(TEXT("headgear_balaclava"), FText::FromString(TEXT("Spec Ops Balaclava")), TEXT("headgear"), 1800, 25, false);
	AddItem(TEXT("headgear_cap"), FText::FromString(TEXT("Tactical Cap & Headset")), TEXT("headgear"), 1200, 20, false);

	// Armor Rigs
	AddItem(TEXT("vest_level4"), FText::FromString(TEXT("Level IV Heavy Carrier")), TEXT("vest"), 0, 0, true);
	AddItem(TEXT("vest_jpc"), FText::FromString(TEXT("Lightweight JPC Vest")), TEXT("vest"), 2200, 35, false);
	AddItem(TEXT("vest_recon"), FText::FromString(TEXT("Recon Tactical Rig")), TEXT("vest"), 2800, 45, false);

	// NF-98 Weapon Finishes
	AddItem(TEXT("weapon_steel"), FText::FromString(TEXT("Factory Blued Steel")), TEXT("weapon"), 0, 0, true);
	AddItem(TEXT("weapon_cerakote"), FText::FromString(TEXT("Desert Tan Cerakote")), TEXT("weapon"), 1800, 30, false);
	AddItem(TEXT("weapon_carbon"), FText::FromString(TEXT("Carbon Fiber Spec-Ops")), TEXT("weapon"), 3200, 50, false);
	AddItem(TEXT("weapon_damascene"), FText::FromString(TEXT("24K Gold & Opal Damascene")), TEXT("weapon"), 6500, 90, false);
}

void UFPSEconomySubsystem::InitializeDailyBounties()
{
	ActiveBounties.Empty();

	// Contract 1: ChronoDiveKill -> +1,200 GOLD
	{
		FFPSTacticalBounty B1;
		B1.BountyID = TEXT("ChronoDiveKill");
		B1.EventTag = FName(TEXT("ChronoDiveKill"));
		B1.SecondaryEventTag = NAME_None;
		B1.Title = FText::FromString(TEXT("CHRONO BREACHER"));
		B1.Description = FText::FromString(TEXT("Eliminate 3 enemies while diving inside a Fermi Time Bubble"));
		B1.CurrentProgress = 0;
		B1.TargetProgress = 3;
		B1.GoldReward = 1200;
		B1.OpalReward = 0;
		B1.bCompleted = false;
		ActiveBounties.Add(B1);
	}

	// Contract 2: FermiSwapKill -> +1,500 GOLD + 5 OPALS
	{
		FFPSTacticalBounty B2;
		B2.BountyID = TEXT("FermiSwapKill");
		B2.EventTag = FName(TEXT("FermiSwapKill"));
		B2.SecondaryEventTag = NAME_None;
		B2.Title = FText::FromString(TEXT("PHANTOM OPERATOR"));
		B2.Description = FText::FromString(TEXT("Eliminate 3 enemies after executing a Q Fermi Swap"));
		B2.CurrentProgress = 0;
		B2.TargetProgress = 3;
		B2.GoldReward = 1500;
		B2.OpalReward = 5;
		B2.bCompleted = false;
		ActiveBounties.Add(B2);
	}

	// Contract 3: BulletDodge or PlateShatter -> +1,000 GOLD + 3 OPALS
	{
		FFPSTacticalBounty B3;
		B3.BountyID = TEXT("BulletDodge");
		B3.EventTag = FName(TEXT("BulletDodge"));
		B3.SecondaryEventTag = FName(TEXT("PlateShatter"));
		B3.Title = FText::FromString(TEXT("KINETIC MATRIX"));
		B3.Description = FText::FromString(TEXT("Dodge 5 slow-mo bullets OR shatter 3 Level IV Ceramic Plates"));
		B3.CurrentProgress = 0;
		B3.TargetProgress = 5;
		B3.GoldReward = 1000;
		B3.OpalReward = 3;
		B3.bCompleted = false;
		ActiveBounties.Add(B3);
	}
}

void UFPSEconomySubsystem::InitializeWheelSlices()
{
	WheelSlices.Empty();

	auto AddSlice = [this](const FText& Label, EWheelRewardType Type, int32 Amount, float Weight, const FLinearColor& Color)
	{
		FFPSWheelSlice S;
		S.Label = Label;
		S.RewardType = Type;
		S.Amount = Amount;
		S.Weight = Weight;
		S.SliceColor = Color;
		WheelSlices.Add(S);
	};

	// 8 Slices matching the specification:
	// 0: 250 GOLD (26% weight)
	AddSlice(FText::FromString(TEXT("250 GOLD")), EWheelRewardType::Gold, 250, 26.0f, FLinearColor(0.05f, 0.08f, 0.12f, 1.0f));
	// 1: 500 GOLD (24% weight)
	AddSlice(FText::FromString(TEXT("500 GOLD")), EWheelRewardType::Gold, 500, 24.0f, FLinearColor(0.08f, 0.12f, 0.18f, 1.0f));
	// 2: 750 GOLD (18% weight)
	AddSlice(FText::FromString(TEXT("750 GOLD")), EWheelRewardType::Gold, 750, 18.0f, FLinearColor(0.05f, 0.08f, 0.12f, 1.0f));
	// 3: 1,250 GOLD (12% weight)
	AddSlice(FText::FromString(TEXT("1,250 GOLD")), EWheelRewardType::Gold, 1250, 12.0f, FLinearColor(0.08f, 0.12f, 0.18f, 1.0f));
	// 4: 2,500 GOLD JACKPOT (6% weight)
	AddSlice(FText::FromString(TEXT("2,500 GOLD JACKPOT")), EWheelRewardType::Gold, 2500, 6.0f, FLinearColor(0.85f, 0.55f, 0.0f, 1.0f));
	// 5: BONUS DART TOKEN (9% weight)
	AddSlice(FText::FromString(TEXT("+1 DART TOKEN")), EWheelRewardType::DartToken, 1, 9.0f, FLinearColor(0.0f, 0.65f, 0.75f, 1.0f));
	// 6: 5 RARE OPALS (4% weight)
	AddSlice(FText::FromString(TEXT("💎 5 RARE OPALS")), EWheelRewardType::Opals, 5, 4.0f, FLinearColor(0.0f, 0.85f, 0.95f, 1.0f));
	// 7: 25 ULTRA OPALS (1% weight)
	AddSlice(FText::FromString(TEXT("💎 25 ULTRA OPALS")), EWheelRewardType::Opals, 25, 1.0f, FLinearColor(0.95f, 0.20f, 0.55f, 1.0f));
}

FString UFPSEconomySubsystem::GetProfileSaveFilePath() const
{
	return FPaths::ProjectSavedDir() / TEXT("NobleFermiProfile.json");
}

void UFPSEconomySubsystem::AddGold(int32 Amount, const FString& Reason)
{
	if (Amount <= 0) return;
	GoldBalance += Amount;
	SaveProfile();
	OnCurrencyChanged.Broadcast(GoldBalance, OpalBalance);
}

void UFPSEconomySubsystem::AddOpals(int32 Amount, const FString& Reason)
{
	if (Amount <= 0) return;
	OpalBalance += Amount;
	SaveProfile();
	OnCurrencyChanged.Broadcast(GoldBalance, OpalBalance);
}

bool UFPSEconomySubsystem::TrySpendGold(int32 Amount)
{
	if (Amount <= 0 || GoldBalance < Amount) return false;
	GoldBalance -= Amount;
	SaveProfile();
	OnCurrencyChanged.Broadcast(GoldBalance, OpalBalance);
	return true;
}

bool UFPSEconomySubsystem::TrySpendOpals(int32 Amount)
{
	if (Amount <= 0 || OpalBalance < Amount) return false;
	OpalBalance -= Amount;
	SaveProfile();
	OnCurrencyChanged.Broadcast(GoldBalance, OpalBalance);
	return true;
}

void UFPSEconomySubsystem::SetEquippedCamoIndex(int32 NewIndex)
{
	EquippedCamoIndex = FMath::Clamp(NewIndex, 0, 3);
	SaveProfile();
	OnEquippedLoadoutChanged.Broadcast(EquippedCamoIndex, EquippedHeadgearIndex, EquippedArmorIndex, EquippedWeaponFinishIndex);
}

void UFPSEconomySubsystem::SetEquippedHeadgearIndex(int32 NewIndex)
{
	EquippedHeadgearIndex = FMath::Clamp(NewIndex, 0, 3);
	SaveProfile();
	OnEquippedLoadoutChanged.Broadcast(EquippedCamoIndex, EquippedHeadgearIndex, EquippedArmorIndex, EquippedWeaponFinishIndex);
}

void UFPSEconomySubsystem::SetEquippedArmorIndex(int32 NewIndex)
{
	EquippedArmorIndex = FMath::Clamp(NewIndex, 0, 2);
	SaveProfile();
	OnEquippedLoadoutChanged.Broadcast(EquippedCamoIndex, EquippedHeadgearIndex, EquippedArmorIndex, EquippedWeaponFinishIndex);
}

void UFPSEconomySubsystem::SetEquippedWeaponFinishIndex(int32 NewIndex)
{
	EquippedWeaponFinishIndex = FMath::Clamp(NewIndex, 0, 3);
	SaveProfile();
	OnEquippedLoadoutChanged.Broadcast(EquippedCamoIndex, EquippedHeadgearIndex, EquippedArmorIndex, EquippedWeaponFinishIndex);
}

void UFPSEconomySubsystem::SetEquippedLoadout(int32 InCamoIdx, int32 InHeadgearIdx, int32 InArmorIdx, int32 InWeaponFinishIdx)
{
	EquippedCamoIndex = FMath::Clamp(InCamoIdx, 0, 3);
	EquippedHeadgearIndex = FMath::Clamp(InHeadgearIdx, 0, 3);
	EquippedArmorIndex = FMath::Clamp(InArmorIdx, 0, 2);
	EquippedWeaponFinishIndex = FMath::Clamp(InWeaponFinishIdx, 0, 3);
	SaveProfile();
	OnEquippedLoadoutChanged.Broadcast(EquippedCamoIndex, EquippedHeadgearIndex, EquippedArmorIndex, EquippedWeaponFinishIndex);
}

void UFPSEconomySubsystem::GetEquippedLoadout(int32& OutCamoIdx, int32& OutHeadgearIdx, int32& OutArmorIdx, int32& OutWeaponFinishIdx) const
{
	OutCamoIdx = EquippedCamoIndex;
	OutHeadgearIdx = EquippedHeadgearIndex;
	OutArmorIdx = EquippedArmorIndex;
	OutWeaponFinishIdx = EquippedWeaponFinishIndex;
}

void UFPSEconomySubsystem::RecordBountyProgress(FName EventTag, int32 Delta)
{
	if (Delta <= 0 || EventTag.IsNone()) return;

	bool bAnyUpdated = false;
	for (FFPSTacticalBounty& Bounty : ActiveBounties)
	{
		if (Bounty.bCompleted)
		{
			continue;
		}

		const bool bMatchesPrimary = (Bounty.EventTag == EventTag);
		const bool bMatchesSecondary = (!Bounty.SecondaryEventTag.IsNone() && Bounty.SecondaryEventTag == EventTag);

		if (bMatchesPrimary || bMatchesSecondary)
		{
			// For Contract 3 ("BulletDodge" target 5 OR "PlateShatter" target 3),
			// shattering 3 plates scales to reach the 5 progress threshold cleanly
			const int32 EffectiveDelta = (bMatchesSecondary && EventTag == FName(TEXT("PlateShatter"))) ? FMath::Max(Delta * 2, 2) : Delta;
			Bounty.CurrentProgress = FMath::Clamp(Bounty.CurrentProgress + EffectiveDelta, 0, Bounty.TargetProgress);
			bAnyUpdated = true;

			if (Bounty.CurrentProgress >= Bounty.TargetProgress)
			{
				Bounty.bCompleted = true;
				GoldBalance += Bounty.GoldReward;
				OpalBalance += Bounty.OpalReward;
				OnBountyCompleted.Broadcast(Bounty);
				OnCurrencyChanged.Broadcast(GoldBalance, OpalBalance);
			}
		}
	}

	if (bAnyUpdated)
	{
		SaveProfile();
	}
}

bool UFPSEconomySubsystem::RerollDailyBounties()
{
	if (!TrySpendGold(250))
	{
		return false;
	}

	BountyRerollSeed++;
	ResetDailyBounties();
	return true;
}

void UFPSEconomySubsystem::ResetDailyBounties()
{
	InitializeDailyBounties();
	SaveProfile();
}

void UFPSEconomySubsystem::GetDailyRewardPreview(int32 DayIndex, int32& OutGold, int32& OutOpals) const
{
	const int32 ClampedDay = FMath::Clamp(DayIndex, 1, 7);
	switch (ClampedDay)
	{
	case 1: OutGold = 250;  OutOpals = 0;  break;
	case 2: OutGold = 500;  OutOpals = 0;  break;
	case 3: OutGold = 750;  OutOpals = 0;  break;
	case 4: OutGold = 1000; OutOpals = 0;  break;
	case 5: OutGold = 1250; OutOpals = 0;  break;
	case 6: OutGold = 1500; OutOpals = 0;  break;
	case 7: OutGold = 2000; OutOpals = 15; break;
	default: OutGold = 250; OutOpals = 0;  break;
	}
}

bool UFPSEconomySubsystem::ClaimDailyLoginReward(int32& OutGoldAwarded, int32& OutOpalsAwarded)
{
	OutGoldAwarded = 0;
	OutOpalsAwarded = 0;

	if (bDailyRewardClaimedToday)
	{
		return false;
	}

	GetDailyRewardPreview(ConsecutiveLoginDays, OutGoldAwarded, OutOpalsAwarded);

	GoldBalance += OutGoldAwarded;
	OpalBalance += OutOpalsAwarded;
	bDailyRewardClaimedToday = true;

	SaveProfile();
	OnCurrencyChanged.Broadcast(GoldBalance, OpalBalance);
	return true;
}

void UFPSEconomySubsystem::SimulateNextDay()
{
	if (bDailyRewardClaimedToday)
	{
		ConsecutiveLoginDays = (ConsecutiveLoginDays % 7) + 1;
	}
	bDailyRewardClaimedToday = false;
	FreeSpinsRemaining = FMath::Max(FreeSpinsRemaining + 1, 1);
	DartTokens = FMath::Min(DartTokens + 2, 10);

	// Refresh daily tactical contracts on new day
	InitializeDailyBounties();

	SaveProfile();
	OnCurrencyChanged.Broadcast(GoldBalance, OpalBalance);
}

bool UFPSEconomySubsystem::CanSpinWheel() const
{
	return FreeSpinsRemaining > 0 || GoldBalance >= 250;
}

int32 UFPSEconomySubsystem::RollWheelSliceIndex()
{
	if (WheelSlices.Num() == 0) return 0;

	float TotalWeight = 0.0f;
	for (const FFPSWheelSlice& S : WheelSlices)
	{
		TotalWeight += S.Weight;
	}

	const float Roll = FMath::FRandRange(0.0f, TotalWeight);
	float Acc = 0.0f;
	for (int32 i = 0; i < WheelSlices.Num(); ++i)
	{
		Acc += WheelSlices[i].Weight;
		if (Roll <= Acc)
		{
			return i;
		}
	}
	return 0;
}

void UFPSEconomySubsystem::AwardWheelSliceReward(int32 SliceIndex, FText& OutMessage, int32& OutRewardAmount, EWheelRewardType& OutType)
{
	if (!WheelSlices.IsValidIndex(SliceIndex))
	{
		SliceIndex = 0;
	}

	// Consume spin: free spin first, or 250 gold
	if (FreeSpinsRemaining > 0)
	{
		FreeSpinsRemaining--;
	}
	else
	{
		SpendGold(250);
	}

	const FFPSWheelSlice& WonSlice = WheelSlices[SliceIndex];
	OutType = WonSlice.RewardType;
	OutRewardAmount = WonSlice.Amount;

	switch (WonSlice.RewardType)
	{
	case EWheelRewardType::Gold:
		GoldBalance += WonSlice.Amount;
		OutMessage = FText::FromString(FString::Printf(TEXT("WHEEL VICTORY: +%d GOLD!"), WonSlice.Amount));
		break;
	case EWheelRewardType::Opals:
		OpalBalance += WonSlice.Amount;
		OutMessage = FText::FromString(FString::Printf(TEXT("JACKPOT DROP: +%d RARE OPALS!"), WonSlice.Amount));
		break;
	case EWheelRewardType::DartToken:
		DartTokens += WonSlice.Amount;
		OutMessage = FText::FromString(TEXT("ARMORY BONUS: +1 TACTICAL DART TOKEN!"));
		break;
	}

	SaveProfile();
	OnCurrencyChanged.Broadcast(GoldBalance, OpalBalance);
}

bool UFPSEconomySubsystem::CanThrowDart() const
{
	return DartTokens > 0;
}

void UFPSEconomySubsystem::AwardDartThrowReward(float DistanceFromCenterNormalized, FText& OutMessage, int32& OutGoldAwarded, int32& OutOpalsAwarded, bool& bOutIsBullseye)
{
	if (DartTokens > 0)
	{
		DartTokens--;
	}

	bOutIsBullseye = (DistanceFromCenterNormalized <= 0.04f);

	if (bOutIsBullseye)
	{
		// Dead-Center Bullseye (<4% radius): 1,800 Gold + 10 Rare Opals!
		OutGoldAwarded = 1800;
		OutOpalsAwarded = 10;
		GoldBalance += OutGoldAwarded;
		OpalBalance += OutOpalsAwarded;
		OutMessage = FText::FromString(TEXT("🎯 DEAD-CENTER BULLSEYE! +1,800 GOLD & +10 RARE OPALS!"));
	}
	else if (DistanceFromCenterNormalized <= 0.38f)
	{
		// Inner Tactical Ring: 600 to 1,000 Gold
		OutGoldAwarded = 750 + FMath::RandRange(0, 250);
		OutOpalsAwarded = 0;
		GoldBalance += OutGoldAwarded;
		OutMessage = FText::FromString(FString::Printf(TEXT("🎯 INNER TACTICAL HIT! +%d GOLD"), OutGoldAwarded));
	}
	else
	{
		// Outer Ring: 150 to 350 Gold
		OutGoldAwarded = 250 + FMath::RandRange(-50, 100);
		OutOpalsAwarded = 0;
		GoldBalance += OutGoldAwarded;
		OutMessage = FText::FromString(FString::Printf(TEXT("🎯 PERIMETER HIT: +%d GOLD"), OutGoldAwarded));
	}

	SaveProfile();
	OnCurrencyChanged.Broadcast(GoldBalance, OpalBalance);
}

bool UFPSEconomySubsystem::IsItemUnlocked(const FString& ItemID) const
{
	if (const FFPSCatalogItem* Found = CatalogItems.Find(ItemID))
	{
		if (Found->bDefaultUnlocked) return true;
	}
	return UnlockedItemIDs.Contains(ItemID);
}

bool UFPSEconomySubsystem::UnlockItemWithGold(const FString& ItemID)
{
	const FFPSCatalogItem* Item = CatalogItems.Find(ItemID);
	if (!Item) return false;
	if (IsItemUnlocked(ItemID)) return true;

	if (TrySpendGold(Item->GoldCost))
	{
		UnlockedItemIDs.Add(ItemID);
		SaveProfile();
		OnItemUnlocked.Broadcast(ItemID);
		return true;
	}
	return false;
}

bool UFPSEconomySubsystem::UnlockItemWithOpals(const FString& ItemID)
{
	const FFPSCatalogItem* Item = CatalogItems.Find(ItemID);
	if (!Item) return false;
	if (IsItemUnlocked(ItemID)) return true;

	if (TrySpendOpals(Item->OpalCost))
	{
		UnlockedItemIDs.Add(ItemID);
		SaveProfile();
		OnItemUnlocked.Broadcast(ItemID);
		return true;
	}
	return false;
}

bool UFPSEconomySubsystem::GetCatalogItem(const FString& ItemID, FFPSCatalogItem& OutItem) const
{
	if (const FFPSCatalogItem* Found = CatalogItems.Find(ItemID))
	{
		OutItem = *Found;
		return true;
	}
	return false;
}

void UFPSEconomySubsystem::ClaimOpalSupplyDrop(int32 PackIndex, FText& OutMessage, int32& OutOpalsAdded)
{
	switch (PackIndex)
	{
	case 0:
		OutOpalsAdded = 100;
		OutMessage = FText::FromString(TEXT("SUPPLY DROP // TACTICAL POUCH: +100 OPALS ADDED"));
		break;
	case 1:
		OutOpalsAdded = 300;
		OutMessage = FText::FromString(TEXT("SUPPLY DROP // OPERATOR CRATE: +300 OPALS ADDED"));
		break;
	case 2:
	default:
		OutOpalsAdded = 750;
		OutMessage = FText::FromString(TEXT("SUPPLY DROP // SPEC OPS VAULT: +750 OPALS ADDED"));
		break;
	}

	AddOpals(OutOpalsAdded);
}

void UFPSEconomySubsystem::SaveProfile()
{
	FString JsonOut = TEXT("{\n");
	JsonOut += FString::Printf(TEXT("\t\"GoldBalance\": %d,\n"), GoldBalance);
	JsonOut += FString::Printf(TEXT("\t\"OpalBalance\": %d,\n"), OpalBalance);
	JsonOut += FString::Printf(TEXT("\t\"ConsecutiveLoginDays\": %d,\n"), ConsecutiveLoginDays);
	JsonOut += FString::Printf(TEXT("\t\"bDailyRewardClaimedToday\": %s,\n"), bDailyRewardClaimedToday ? TEXT("true") : TEXT("false"));
	JsonOut += FString::Printf(TEXT("\t\"DartTokens\": %d,\n"), DartTokens);
	JsonOut += FString::Printf(TEXT("\t\"FreeSpinsRemaining\": %d,\n"), FreeSpinsRemaining);

	// Persistent 3rd-Person Equipped Loadout Indices
	JsonOut += FString::Printf(TEXT("\t\"EquippedCamoIndex\": %d,\n"), EquippedCamoIndex);
	JsonOut += FString::Printf(TEXT("\t\"EquippedHeadgearIndex\": %d,\n"), EquippedHeadgearIndex);
	JsonOut += FString::Printf(TEXT("\t\"EquippedArmorIndex\": %d,\n"), EquippedArmorIndex);
	JsonOut += FString::Printf(TEXT("\t\"EquippedWeaponFinishIndex\": %d,\n"), EquippedWeaponFinishIndex);

	// Daily Tactical Bounty State
	for (int32 i = 0; i < ActiveBounties.Num(); ++i)
	{
		JsonOut += FString::Printf(TEXT("\t\"Bounty%d_Progress\": %d,\n"), i, ActiveBounties[i].CurrentProgress);
		JsonOut += FString::Printf(TEXT("\t\"Bounty%d_Completed\": %s,\n"), i, ActiveBounties[i].bCompleted ? TEXT("true") : TEXT("false"));
	}

	JsonOut += TEXT("\t\"ActiveBounties\": [\n");
	for (int32 i = 0; i < ActiveBounties.Num(); ++i)
	{
		const FFPSTacticalBounty& B = ActiveBounties[i];
		JsonOut += FString::Printf(
			TEXT("\t\t{ \"BountyID\": \"%s\", \"CurrentProgress\": %d, \"TargetProgress\": %d, \"GoldReward\": %d, \"OpalReward\": %d, \"bCompleted\": %s }%s\n"),
			*B.BountyID, B.CurrentProgress, B.TargetProgress, B.GoldReward, B.OpalReward,
			B.bCompleted ? TEXT("true") : TEXT("false"),
			(i == ActiveBounties.Num() - 1) ? TEXT("") : TEXT(",")
		);
	}
	JsonOut += TEXT("\t],\n");

	// Unlocked items array
	JsonOut += TEXT("\t\"UnlockedItemIDs\": [\n");
	int32 ItemIdx = 0;
	for (const FString& ID : UnlockedItemIDs)
	{
		JsonOut += FString::Printf(TEXT("\t\t\"%s\"%s\n"), *ID, (ItemIdx == UnlockedItemIDs.Num() - 1) ? TEXT("") : TEXT(","));
		ItemIdx++;
	}
	JsonOut += TEXT("\t]\n");
	JsonOut += TEXT("}\n");

	const FString SavePath = GetProfileSaveFilePath();
	FFileHelper::SaveStringToFile(JsonOut, *SavePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void UFPSEconomySubsystem::LoadProfile()
{
	const FString SavePath = GetProfileSaveFilePath();
	FString JsonIn;
	if (!FFileHelper::LoadFileToString(JsonIn, *SavePath) || JsonIn.IsEmpty())
	{
		// Default profile
		GoldBalance = 1500;
		OpalBalance = 50;
		ConsecutiveLoginDays = 1;
		bDailyRewardClaimedToday = false;
		DartTokens = 3;
		FreeSpinsRemaining = 1;
		EquippedCamoIndex = 0;
		EquippedHeadgearIndex = 0;
		EquippedArmorIndex = 0;
		EquippedWeaponFinishIndex = 0;
		InitializeDailyBounties();
		SaveProfile();
		return;
	}

	// Simple and robust JSON parser
	auto ExtractInt = [&JsonIn](const FString& Key, int32 DefaultVal) -> int32
	{
		const FString Pattern = FString::Printf(TEXT("\"%s\":"), *Key);
		int32 FoundIdx = JsonIn.Find(Pattern);
		if (FoundIdx == INDEX_NONE) return DefaultVal;

		int32 ValStart = FoundIdx + Pattern.Len();
		while (ValStart < JsonIn.Len() && (JsonIn[ValStart] == ' ' || JsonIn[ValStart] == '\t'))
		{
			ValStart++;
		}
		int32 ValEnd = ValStart;
		while (ValEnd < JsonIn.Len() && (FChar::IsDigit(JsonIn[ValEnd]) || JsonIn[ValEnd] == '-'))
		{
			ValEnd++;
		}
		if (ValEnd > ValStart)
		{
			return FCString::Atoi(*JsonIn.Mid(ValStart, ValEnd - ValStart));
		}
		return DefaultVal;
	};

	auto ExtractBool = [&JsonIn](const FString& Key, bool DefaultVal) -> bool
	{
		const FString Pattern = FString::Printf(TEXT("\"%s\":"), *Key);
		int32 FoundIdx = JsonIn.Find(Pattern);
		if (FoundIdx == INDEX_NONE) return DefaultVal;

		const FString Rest = JsonIn.Mid(FoundIdx + Pattern.Len(), 15).ToLower();
		if (Rest.Contains(TEXT("true"))) return true;
		if (Rest.Contains(TEXT("false"))) return false;
		return DefaultVal;
	};

	GoldBalance = FMath::Max(0, ExtractInt(TEXT("GoldBalance"), 1500));
	OpalBalance = FMath::Max(0, ExtractInt(TEXT("OpalBalance"), 50));
	ConsecutiveLoginDays = FMath::Clamp(ExtractInt(TEXT("ConsecutiveLoginDays"), 1), 1, 7);
	bDailyRewardClaimedToday = ExtractBool(TEXT("bDailyRewardClaimedToday"), false);
	DartTokens = FMath::Max(0, ExtractInt(TEXT("DartTokens"), 3));
	FreeSpinsRemaining = FMath::Max(0, ExtractInt(TEXT("FreeSpinsRemaining"), 1));

	// Load Equipped 3rd-Person Loadout Indices
	EquippedCamoIndex = FMath::Clamp(ExtractInt(TEXT("EquippedCamoIndex"), 0), 0, 3);
	EquippedHeadgearIndex = FMath::Clamp(ExtractInt(TEXT("EquippedHeadgearIndex"), 0), 0, 3);
	EquippedArmorIndex = FMath::Clamp(ExtractInt(TEXT("EquippedArmorIndex"), 0), 0, 2);
	EquippedWeaponFinishIndex = FMath::Clamp(ExtractInt(TEXT("EquippedWeaponFinishIndex"), 0), 0, 3);

	// Load Daily Bounties
	if (ActiveBounties.Num() != 3)
	{
		InitializeDailyBounties();
	}
	for (int32 i = 0; i < ActiveBounties.Num(); ++i)
	{
		const FString ProgKey = FString::Printf(TEXT("Bounty%d_Progress"), i);
		const FString CompKey = FString::Printf(TEXT("Bounty%d_Completed"), i);
		ActiveBounties[i].CurrentProgress = FMath::Clamp(ExtractInt(ProgKey, 0), 0, ActiveBounties[i].TargetProgress);
		ActiveBounties[i].bCompleted = ExtractBool(CompKey, false);
		if (ActiveBounties[i].bCompleted)
		{
			ActiveBounties[i].CurrentProgress = ActiveBounties[i].TargetProgress;
		}
	}

	// Parse UnlockedItemIDs array
	const FString ArrayKey = TEXT("\"UnlockedItemIDs\":");
	int32 ArrayKeyIdx = JsonIn.Find(ArrayKey);
	if (ArrayKeyIdx != INDEX_NONE)
	{
		int32 OpenBracket = JsonIn.Find(TEXT("["), ESearchCase::IgnoreCase, ESearchDir::FromStart, ArrayKeyIdx);
		int32 CloseBracket = JsonIn.Find(TEXT("]"), ESearchCase::IgnoreCase, ESearchDir::FromStart, OpenBracket);
		if (OpenBracket != INDEX_NONE && CloseBracket != INDEX_NONE && CloseBracket > OpenBracket)
		{
			const FString ArrayBody = JsonIn.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1);
			TArray<FString> Lines;
			ArrayBody.ParseIntoArray(Lines, TEXT(","), true);
			for (FString& Line : Lines)
			{
				Line = Line.TrimStartAndEnd();
				int32 Q1 = Line.Find(TEXT("\""));
				int32 Q2 = Line.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, Q1 + 1);
				if (Q1 != INDEX_NONE && Q2 != INDEX_NONE && Q2 > Q1)
				{
					const FString ParsedID = Line.Mid(Q1 + 1, Q2 - Q1 - 1);
					if (!ParsedID.IsEmpty())
					{
						UnlockedItemIDs.Add(ParsedID);
					}
				}
			}
		}
	}

	// Always guarantee default items
	UnlockedItemIDs.Add(TEXT("camo_digital"));
	UnlockedItemIDs.Add(TEXT("headgear_fast"));
	UnlockedItemIDs.Add(TEXT("vest_level4"));
	UnlockedItemIDs.Add(TEXT("weapon_steel"));
}
