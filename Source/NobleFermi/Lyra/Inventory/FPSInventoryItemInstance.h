#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "Lyra/Inventory/FPSInventoryItemDefinition.h"
#include "FPSInventoryItemInstance.generated.h"

USTRUCT(BlueprintType)
struct NOBLEFERMI_API FFPSTagStack
{
	GENERATED_BODY()

	FFPSTagStack() {}
	FFPSTagStack(FGameplayTag InTag, int32 InCount) : Tag(InTag), StackCount(InCount) {}

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGameplayTag Tag;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 StackCount = 0;
};

/**
 * Replicated Inventory Item Instance (Lyra pattern)
 */
UCLASS(BlueprintType)
class NOBLEFERMI_API UFPSInventoryItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UFPSInventoryItemInstance();

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetItemDef(TSubclassOf<UFPSInventoryItemDefinition> InDef);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TSubclassOf<UFPSInventoryItemDefinition> GetItemDef() const { return ItemDef; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const UFPSInventoryItemFragment* FindFragmentByClass(TSubclassOf<UFPSInventoryItemFragment> FragmentClass) const;

	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return Cast<ResultClass>(FindFragmentByClass(ResultClass::StaticClass()));
	}

	// Dynamic Stat Tracking (e.g. Current Ammo in Magazine)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasStatTag(FGameplayTag Tag) const;

protected:
	UPROPERTY(Replicated)
	TSubclassOf<UFPSInventoryItemDefinition> ItemDef;

	UPROPERTY(Replicated)
	TArray<FFPSTagStack> StatTags;
};
