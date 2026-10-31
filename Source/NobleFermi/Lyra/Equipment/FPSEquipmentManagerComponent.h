#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Lyra/Equipment/FPSEquipmentInstance.h"
#include "FPSEquipmentManagerComponent.generated.h"

class UFPSEquipmentDefinition;
class UFPSEquipmentManagerComponent;

/**
 * A single entry of active applied equipment (Lyra pattern)
 */
USTRUCT(BlueprintType)
struct NOBLEFERMI_API FFPSAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FFPSAppliedEquipmentEntry() {}

	UPROPERTY()
	TSubclassOf<UFPSEquipmentDefinition> EquipmentDefinition;

	UPROPERTY()
	TObjectPtr<UFPSEquipmentInstance> Instance = nullptr;

	void PreReplicatedRemove(const struct FFPSEquipmentList& InArraySerializer);
	void PostReplicatedAdd(const struct FFPSEquipmentList& InArraySerializer);
	void PostReplicatedChange(const struct FFPSEquipmentList& InArraySerializer);
};

/**
 * Fast-Array replicated list of applied equipment (Lyra pattern)
 */
USTRUCT(BlueprintType)
struct NOBLEFERMI_API FFPSEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FFPSEquipmentList()
		: OwnerComponent(nullptr)
	{
	}

	FFPSEquipmentList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	UFPSEquipmentInstance* AddEntry(TSubclassOf<UFPSEquipmentDefinition> EquipmentDef);
	void RemoveEntry(UFPSEquipmentInstance* Instance);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FFPSAppliedEquipmentEntry, FFPSEquipmentList>(Entries, DeltaParms, *this);
	}

private:
	friend class UFPSEquipmentManagerComponent;
	friend struct FFPSAppliedEquipmentEntry;

	UPROPERTY()
	TArray<FFPSAppliedEquipmentEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FFPSEquipmentList> : public TStructOpsTypeTraitsBase2<FFPSEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * Replicated Equipment Manager Component (Lyra Pattern)
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UFPSEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;
	virtual void UninitializeComponent() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	UFPSEquipmentInstance* EquipItem(TSubclassOf<UFPSEquipmentDefinition> EquipmentDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void UnequipItem(UFPSEquipmentInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UFPSEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UFPSEquipmentInstance> InstanceType);

	template <typename ResultClass>
	ResultClass* GetFirstInstanceOfType()
	{
		return Cast<ResultClass>(GetFirstInstanceOfType(ResultClass::StaticClass()));
	}

protected:
	UPROPERTY(Replicated)
	FFPSEquipmentList EquipmentList;
};
