#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Lyra/Inventory/FPSInventoryItemInstance.h"
#include "FPSInventoryManagerComponent.generated.h"

class UFPSInventoryManagerComponent;
class UFPSInventoryItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemChangedSignature, UFPSInventoryItemInstance*, ItemInstance);

/**
 * A single entry in an inventory (Lyra pattern)
 */
USTRUCT(BlueprintType)
struct NOBLEFERMI_API FFPSInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FFPSInventoryEntry() {}

	UPROPERTY()
	TObjectPtr<UFPSInventoryItemInstance> Instance = nullptr;

	UPROPERTY()
	int32 StackCount = 0;

	void PreReplicatedRemove(const struct FFPSInventoryList& InArraySerializer);
	void PostReplicatedAdd(const struct FFPSInventoryList& InArraySerializer);
	void PostReplicatedChange(const struct FFPSInventoryList& InArraySerializer);
};

/**
 * Fast-Array replicated list of inventory items (Lyra pattern)
 */
USTRUCT(BlueprintType)
struct NOBLEFERMI_API FFPSInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FFPSInventoryList()
		: OwnerComponent(nullptr)
	{
	}

	FFPSInventoryList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	TArray<UFPSInventoryItemInstance*> GetAllAvailableItems() const;

	UFPSInventoryItemInstance* AddEntry(TSubclassOf<UFPSInventoryItemDefinition> ItemClass, int32 StackCount);
	void RemoveEntry(UFPSInventoryItemInstance* Instance);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FFPSInventoryEntry, FFPSInventoryList>(Entries, DeltaParms, *this);
	}

private:
	friend class UFPSInventoryManagerComponent;
	friend struct FFPSInventoryEntry;

	UPROPERTY()
	TArray<FFPSInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FFPSInventoryList> : public TStructOpsTypeTraitsBase2<FFPSInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * Replicated Inventory Manager Component (Lyra Pattern)
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemChangedSignature OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemChangedSignature OnItemRemoved;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	UFPSInventoryItemInstance* AddItemDefinition(TSubclassOf<UFPSInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void RemoveItemInstance(UFPSInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<UFPSInventoryItemInstance*> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UFPSInventoryItemInstance* FindFirstItemStackByDefinition(TSubclassOf<UFPSInventoryItemDefinition> ItemDef) const;

protected:
	UPROPERTY(Replicated)
	FFPSInventoryList InventoryList;
};
