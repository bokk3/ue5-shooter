#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "FPSInventoryItemDefinition.generated.h"

class UFPSInventoryItemInstance;

/**
 * Base fragment for defining modular metadata on inventory items (Lyra pattern)
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class NOBLEFERMI_API UFPSInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UFPSInventoryItemInstance* Instance) const {}
};

/**
 * Definition of an inventory item template (Lyra pattern)
 */
UCLASS(Blueprintable, Const, Abstract)
class NOBLEFERMI_API UFPSInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UFPSInventoryItemDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FGameplayTag ItemTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	int32 MaxStack = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Fragments")
	TArray<TObjectPtr<UFPSInventoryItemFragment>> Fragments;

	const UFPSInventoryItemFragment* FindFragmentByClass(TSubclassOf<UFPSInventoryItemFragment> FragmentClass) const;
};
