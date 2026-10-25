// =========================================================================================
// NOBLE FERMI // FREE-TO-PLAY DUAL-CURRENCY ECONOMY & PERSISTENT PROFILE SUBSYSTEM
// Hand-off Notice for Agent 3 (Build & Link Engineer):
// - This subsystem uses FFileHelper (Core) for robust, self-contained JSON serialization to
//   FPaths::ProjectSavedDir() / "NobleFermiProfile.json" with ZERO external library overhead.
// - If Agent 3 chooses to swap to FJsonObject / FJsonSerializer, ensure "Json" and
//   "JsonUtilities" are added to PublicDependencyModuleNames in NobleFermi.Build.cs.
//   Otherwise, this subsystem links cleanly with Core, CoreUObject, and Engine!
// =========================================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FPSEconomySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrencyChangedSignature, int32, NewGold, int32, NewOpals);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUnlockedSignature, const FString&, ItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnEquippedLoadoutChangedSignature, int32, CamoIdx, int32, HeadgearIdx, int32, ArmorIdx, int32, WeaponFinishIdx);

UENUM(BlueprintType)
enum class EWheelRewardType : uint8
{
	Gold,
	Opals,
	DartToken
};

USTRUCT(BlueprintType)
struct FFPSWheelSlice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	EWheelRewardType RewardType = EWheelRewardType::Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	int32 Amount = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	float Weight = 15.0f; // Relative probability weight

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	FLinearColor SliceColor = FLinearColor(0.08f, 0.12f, 0.18f, 1.0f);
};

USTRUCT(BlueprintType)
struct FFPSCatalogItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	FString ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	FString Category; // "camo", "headgear", "vest", "weapon"

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	int32 GoldCost = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	int32 OpalCost = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy")
	bool bDefaultUnlocked = false;
};

USTRUCT(BlueprintType)
struct FFPSTacticalBounty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	FString BountyID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	FName EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	FName SecondaryEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	int32 CurrentProgress = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	int32 TargetProgress = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	int32 GoldReward = 1200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	int32 OpalReward = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Economy|Bounties")
	bool bCompleted = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBountyCompletedSignature, const FFPSTacticalBounty&, CompletedBounty);

/**
 * AAA Free-to-Play Dual-Currency Economy & Persistent Profile Subsystem
 * Manages Gold, rare Opals, daily streak progression, Fortune Wheel, Tactical Dartboard,
 * 3-Slot Daily Tactical Contracts (Bounties), and persistent 3rd-person equipped cosmetics.
 */
UCLASS()
class NOBLEFERMI_API UFPSEconomySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Economy")
	FOnCurrencyChangedSignature OnCurrencyChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS Economy")
	FOnItemUnlockedSignature OnItemUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "FPS Economy|Loadout")
	FOnEquippedLoadoutChangedSignature OnEquippedLoadoutChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS Economy|Bounties")
	FOnBountyCompletedSignature OnBountyCompleted;

	// === CURRENCY BALANCES & TRANSACTIONS ===
	UFUNCTION(BlueprintPure, Category = "FPS Economy")
	int32 GetGoldBalance() const { return GoldBalance; }

	UFUNCTION(BlueprintPure, Category = "FPS Economy")
	int32 GetOpalBalance() const { return OpalBalance; }

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void AddGold(int32 Amount, const FString& Reason = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void AddOpals(int32 Amount, const FString& Reason = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	bool TrySpendGold(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	bool SpendGold(int32 Amount) { return TrySpendGold(Amount); }

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	bool TrySpendOpals(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	bool SpendOpals(int32 Amount) { return TrySpendOpals(Amount); }

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void SetBalances(int32 InGold, int32 InOpals);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void UnlockItemDirectly(const FString& ItemID);

	const TSet<FString>& GetUnlockedItemIDs() const { return UnlockedItemIDs; }

	// === PERSISTENT EQUIPPED 3RD-PERSON LOADOUT ===
	UFUNCTION(BlueprintPure, Category = "FPS Economy|Loadout")
	int32 GetEquippedCamoIndex() const { return EquippedCamoIndex; }

	UFUNCTION(BlueprintPure, Category = "FPS Economy|Loadout")
	int32 GetEquippedHeadgearIndex() const { return EquippedHeadgearIndex; }

	UFUNCTION(BlueprintPure, Category = "FPS Economy|Loadout")
	int32 GetEquippedArmorIndex() const { return EquippedArmorIndex; }

	UFUNCTION(BlueprintPure, Category = "FPS Economy|Loadout")
	int32 GetEquippedWeaponFinishIndex() const { return EquippedWeaponFinishIndex; }

	UFUNCTION(BlueprintCallable, Category = "FPS Economy|Loadout")
	void SetEquippedCamoIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy|Loadout")
	void SetEquippedHeadgearIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy|Loadout")
	void SetEquippedArmorIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy|Loadout")
	void SetEquippedWeaponFinishIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy|Loadout")
	void SetEquippedLoadout(int32 InCamoIdx, int32 InHeadgearIdx, int32 InArmorIdx, int32 InWeaponFinishIdx);

	UFUNCTION(BlueprintPure, Category = "FPS Economy|Loadout")
	void GetEquippedLoadout(int32& OutCamoIdx, int32& OutHeadgearIdx, int32& OutArmorIdx, int32& OutWeaponFinishIdx) const;

	// === PERSISTENT COMBAT THEATER (MAP) SELECTION ===
	UFUNCTION(BlueprintPure, Category = "FPS Economy|Theater")
	int32 GetSelectedTheaterIndex() const { return SelectedTheaterIndex; }

	UFUNCTION(BlueprintCallable, Category = "FPS Economy|Theater")
	void SetSelectedTheaterIndex(int32 NewIndex);

	// === 3-SLOT DAILY TACTICAL CONTRACTS (BOUNTIES) ===
	UFUNCTION(BlueprintCallable, Category = "FPS Economy|Bounties")
	void RecordBountyProgress(FName EventTag, int32 Delta = 1);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy|Bounties")
	bool RerollDailyBounties();

	UFUNCTION(BlueprintCallable, Category = "FPS Economy|Bounties")
	void ResetDailyBounties();

	const TArray<FFPSTacticalBounty>& GetActiveBounties() const { return ActiveBounties; }

	// === DAILY LOGIN STREAK ===
	UFUNCTION(BlueprintPure, Category = "FPS Economy")
	int32 GetConsecutiveLoginDays() const { return ConsecutiveLoginDays; }

	UFUNCTION(BlueprintPure, Category = "FPS Economy")
	bool HasClaimedDailyRewardToday() const { return bDailyRewardClaimedToday; }

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	bool ClaimDailyLoginReward(int32& OutGoldAwarded, int32& OutOpalsAwarded);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void SimulateNextDay();

	UFUNCTION(BlueprintPure, Category = "FPS Economy")
	void GetDailyRewardPreview(int32 DayIndex, int32& OutGold, int32& OutOpals) const;

	// === MINIGAME: FORTUNE WHEEL ===
	UFUNCTION(BlueprintPure, Category = "FPS Economy")
	int32 GetFreeSpinsRemaining() const { return FreeSpinsRemaining; }

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	bool CanSpinWheel() const;

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	int32 RollWheelSliceIndex();

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void AwardWheelSliceReward(int32 SliceIndex, FText& OutMessage, int32& OutRewardAmount, EWheelRewardType& OutType);

	const TArray<FFPSWheelSlice>& GetWheelSlices() const { return WheelSlices; }

	// === MINIGAME: TACTICAL DARTBOARD ===
	UFUNCTION(BlueprintPure, Category = "FPS Economy")
	int32 GetDartTokens() const { return DartTokens; }

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	bool CanThrowDart() const;

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void AwardDartThrowReward(float DistanceFromCenterNormalized, FText& OutMessage, int32& OutGoldAwarded, int32& OutOpalsAwarded, bool& bOutIsBullseye);

	// === CUSTOMIZATION CATALOG & PURCHASING ===
	UFUNCTION(BlueprintPure, Category = "FPS Economy")
	bool IsItemUnlocked(const FString& ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	bool UnlockItemWithGold(const FString& ItemID);

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	bool UnlockItemWithOpals(const FString& ItemID);

	UFUNCTION(BlueprintPure, Category = "FPS Economy")
	bool GetCatalogItem(const FString& ItemID, FFPSCatalogItem& OutItem) const;

	// === OPAL SUPPLY DROP SIMULATOR ===
	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void ClaimOpalSupplyDrop(int32 PackIndex, FText& OutMessage, int32& OutOpalsAdded);

	// === PERSISTENCE ===
	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void SaveProfile();

	UFUNCTION(BlueprintCallable, Category = "FPS Economy")
	void LoadProfile();

private:
	UPROPERTY()
	int32 GoldBalance = 1500;

	UPROPERTY()
	int32 OpalBalance = 50;

	UPROPERTY()
	int32 ConsecutiveLoginDays = 1;

	UPROPERTY()
	FString LastLoginDateString;

	UPROPERTY()
	bool bDailyRewardClaimedToday = false;

	UPROPERTY()
	int32 DartTokens = 3;

	UPROPERTY()
	int32 FreeSpinsRemaining = 1;

	// Persistent Equipped Loadout Indices
	UPROPERTY()
	int32 EquippedCamoIndex = 0;

	UPROPERTY()
	int32 EquippedHeadgearIndex = 0;

	UPROPERTY()
	int32 EquippedArmorIndex = 0;

	UPROPERTY()
	int32 EquippedWeaponFinishIndex = 0;

	// Persistent Selected Combat Theater (0 = Rust Tactical [60x60m], 1 = Coastal Operations [120x120m])
	UPROPERTY()
	int32 SelectedTheaterIndex = 0;

	UPROPERTY()
	int32 BountyRerollSeed = 0;

	TSet<FString> UnlockedItemIDs;
	TMap<FString, FFPSCatalogItem> CatalogItems;
	TArray<FFPSWheelSlice> WheelSlices;
	TArray<FFPSTacticalBounty> ActiveBounties;

	void InitializeCatalog();
	void InitializeWheelSlices();
	void InitializeDailyBounties();
	FString GetProfileSaveFilePath() const;
};
