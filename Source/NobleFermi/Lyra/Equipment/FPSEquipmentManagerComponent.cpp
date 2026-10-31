#include "Lyra/Equipment/FPSEquipmentManagerComponent.h"
#include "Lyra/Equipment/FPSEquipmentDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

//////////////////////////////////////////////////////////////////////
// FFPSAppliedEquipmentEntry

void FFPSAppliedEquipmentEntry::PreReplicatedRemove(const FFPSEquipmentList& InArraySerializer)
{
	if (Instance)
	{
		Instance->OnUnequipped();
		Instance->DestroyEquipmentActors();
	}
}

void FFPSAppliedEquipmentEntry::PostReplicatedAdd(const FFPSEquipmentList& InArraySerializer)
{
	if (Instance && EquipmentDefinition)
	{
		const UFPSEquipmentDefinition* DefCDO = GetDefault<UFPSEquipmentDefinition>(EquipmentDefinition);
		Instance->SpawnEquipmentActors(DefCDO->ActorsToSpawn);
		Instance->OnEquipped();
	}
}

void FFPSAppliedEquipmentEntry::PostReplicatedChange(const FFPSEquipmentList& InArraySerializer)
{
}

//////////////////////////////////////////////////////////////////////
// FFPSEquipmentList

UFPSEquipmentInstance* FFPSEquipmentList::AddEntry(TSubclassOf<UFPSEquipmentDefinition> EquipmentDef)
{
	if (!EquipmentDef) return nullptr;

	check(OwnerComponent);
	AActor* OwnerActor = OwnerComponent->GetOwner();
	check(OwnerActor->HasAuthority());

	const UFPSEquipmentDefinition* DefCDO = GetDefault<UFPSEquipmentDefinition>(EquipmentDef);
	TSubclassOf<UFPSEquipmentInstance> InstanceType = DefCDO->InstanceType;
	if (!InstanceType)
	{
		InstanceType = UFPSEquipmentInstance::StaticClass();
	}

	FFPSAppliedEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.EquipmentDefinition = EquipmentDef;
	NewEntry.Instance = NewObject<UFPSEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);

	NewEntry.Instance->SpawnEquipmentActors(DefCDO->ActorsToSpawn);
	NewEntry.Instance->OnEquipped();

	MarkItemDirty(NewEntry);
	return NewEntry.Instance;
}

void FFPSEquipmentList::RemoveEntry(UFPSEquipmentInstance* InInstance)
{
	for (auto It = Entries.CreateIterator(); It; ++It)
	{
		FFPSAppliedEquipmentEntry& Entry = *It;
		if (Entry.Instance == InInstance)
		{
			if (Entry.Instance)
			{
				Entry.Instance->OnUnequipped();
				Entry.Instance->DestroyEquipmentActors();
			}
			It.RemoveCurrent();
			MarkArrayDirty();
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// UFPSEquipmentManagerComponent

UFPSEquipmentManagerComponent::UFPSEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EquipmentList(this)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UFPSEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFPSEquipmentManagerComponent, EquipmentList);
}

void UFPSEquipmentManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	if (IsUsingRegisteredSubObjectList())
	{
		for (const FFPSAppliedEquipmentEntry& Entry : EquipmentList.Entries)
		{
			if (Entry.Instance)
			{
				AddReplicatedSubObject(Entry.Instance);
			}
		}
	}
}

bool UFPSEquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FFPSAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance)
		{
			WroteSomething |= Channel->ReplicateSubobject(Entry.Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UFPSEquipmentManagerComponent::UninitializeComponent()
{
	TArray<UFPSEquipmentInstance*> AllInstances;
	for (const FFPSAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance)
		{
			AllInstances.Add(Entry.Instance);
		}
	}

	for (UFPSEquipmentInstance* Inst : AllInstances)
	{
		UnequipItem(Inst);
	}

	Super::UninitializeComponent();
}

UFPSEquipmentInstance* UFPSEquipmentManagerComponent::EquipItem(TSubclassOf<UFPSEquipmentDefinition> EquipmentDefinition)
{
	UFPSEquipmentInstance* Result = nullptr;
	if (EquipmentDefinition)
	{
		Result = EquipmentList.AddEntry(EquipmentDefinition);
		if (Result)
		{
			if (IsUsingRegisteredSubObjectList())
			{
				AddReplicatedSubObject(Result);
			}
		}
	}
	return Result;
}

void UFPSEquipmentManagerComponent::UnequipItem(UFPSEquipmentInstance* ItemInstance)
{
	if (ItemInstance)
	{
		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(ItemInstance);
		}
		EquipmentList.RemoveEntry(ItemInstance);
	}
}

UFPSEquipmentInstance* UFPSEquipmentManagerComponent::GetFirstInstanceOfType(TSubclassOf<UFPSEquipmentInstance> InstanceType)
{
	for (const FFPSAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance && Entry.Instance->IsA(InstanceType))
		{
			return Entry.Instance;
		}
	}
	return nullptr;
}
