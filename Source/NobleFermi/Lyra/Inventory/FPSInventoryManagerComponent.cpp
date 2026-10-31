#include "Lyra/Inventory/FPSInventoryManagerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

//////////////////////////////////////////////////////////////////////
// FFPSInventoryEntry

void FFPSInventoryEntry::PreReplicatedRemove(const FFPSInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		UFPSInventoryManagerComponent* Manager = Cast<UFPSInventoryManagerComponent>(InArraySerializer.OwnerComponent);
		if (Manager && Instance)
		{
			Manager->OnItemRemoved.Broadcast(Instance);
		}
	}
}

void FFPSInventoryEntry::PostReplicatedAdd(const FFPSInventoryList& InArraySerializer)
{
	if (InArraySerializer.OwnerComponent)
	{
		UFPSInventoryManagerComponent* Manager = Cast<UFPSInventoryManagerComponent>(InArraySerializer.OwnerComponent);
		if (Manager && Instance)
		{
			Manager->OnItemAdded.Broadcast(Instance);
		}
	}
}

void FFPSInventoryEntry::PostReplicatedChange(const FFPSInventoryList& InArraySerializer)
{
}

//////////////////////////////////////////////////////////////////////
// FFPSInventoryList

TArray<UFPSInventoryItemInstance*> FFPSInventoryList::GetAllAvailableItems() const
{
	TArray<UFPSInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const FFPSInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance != nullptr)
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}

UFPSInventoryItemInstance* FFPSInventoryList::AddEntry(TSubclassOf<UFPSInventoryItemDefinition> ItemClass, int32 StackCount)
{
	if (!ItemClass) return nullptr;

	check(OwnerComponent);
	AActor* OwnerActor = OwnerComponent->GetOwner();
	check(OwnerActor->HasAuthority());

	FFPSInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UFPSInventoryItemInstance>(OwnerComponent->GetOwner());
	NewEntry.Instance->SetItemDef(ItemClass);
	NewEntry.StackCount = StackCount;

	const UFPSInventoryItemDefinition* ItemCDO = GetDefault<UFPSInventoryItemDefinition>(ItemClass);
	for (UFPSInventoryItemFragment* Fragment : ItemCDO->Fragments)
	{
		if (Fragment)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}

	MarkItemDirty(NewEntry);
	return NewEntry.Instance;
}

void FFPSInventoryList::RemoveEntry(UFPSInventoryItemInstance* InInstance)
{
	for (auto It = Entries.CreateIterator(); It; ++It)
	{
		FFPSInventoryEntry& Entry = *It;
		if (Entry.Instance == InInstance)
		{
			It.RemoveCurrent();
			MarkArrayDirty();
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// UFPSInventoryManagerComponent

UFPSInventoryManagerComponent::UFPSInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UFPSInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFPSInventoryManagerComponent, InventoryList);
}

void UFPSInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing subobjects for network replication
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FFPSInventoryEntry& Entry : InventoryList.Entries)
		{
			if (Entry.Instance)
			{
				AddReplicatedSubObject(Entry.Instance);
			}
		}
	}
}

bool UFPSInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FFPSInventoryEntry& Entry : InventoryList.Entries)
	{
		if (Entry.Instance)
		{
			WroteSomething |= Channel->ReplicateSubobject(Entry.Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

UFPSInventoryItemInstance* UFPSInventoryManagerComponent::AddItemDefinition(TSubclassOf<UFPSInventoryItemDefinition> ItemDef, int32 StackCount)
{
	UFPSInventoryItemInstance* Result = nullptr;
	if (ItemDef)
	{
		Result = InventoryList.AddEntry(ItemDef, StackCount);
		if (Result)
		{
			if (IsUsingRegisteredSubObjectList())
			{
				AddReplicatedSubObject(Result);
			}
			OnItemAdded.Broadcast(Result);
		}
	}
	return Result;
}

void UFPSInventoryManagerComponent::RemoveItemInstance(UFPSInventoryItemInstance* ItemInstance)
{
	if (ItemInstance)
	{
		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(ItemInstance);
		}
		InventoryList.RemoveEntry(ItemInstance);
		OnItemRemoved.Broadcast(ItemInstance);
	}
}

TArray<UFPSInventoryItemInstance*> UFPSInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllAvailableItems();
}

UFPSInventoryItemInstance* UFPSInventoryManagerComponent::FindFirstItemStackByDefinition(TSubclassOf<UFPSInventoryItemDefinition> ItemDef) const
{
	for (const FFPSInventoryEntry& Entry : InventoryList.Entries)
	{
		if (Entry.Instance && Entry.Instance->GetItemDef() == ItemDef)
		{
			return Entry.Instance;
		}
	}
	return nullptr;
}
