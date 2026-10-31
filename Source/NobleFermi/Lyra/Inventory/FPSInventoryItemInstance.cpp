#include "Lyra/Inventory/FPSInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"

UFPSInventoryItemInstance::UFPSInventoryItemInstance()
{
}

void UFPSInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFPSInventoryItemInstance, ItemDef);
	DOREPLIFETIME(UFPSInventoryItemInstance, StatTags);
}

void UFPSInventoryItemInstance::SetItemDef(TSubclassOf<UFPSInventoryItemDefinition> InDef)
{
	ItemDef = InDef;
}

const UFPSInventoryItemFragment* UFPSInventoryItemInstance::FindFragmentByClass(TSubclassOf<UFPSInventoryItemFragment> FragmentClass) const
{
	if (ItemDef)
	{
		return GetDefault<UFPSInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}

void UFPSInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid() || StackCount <= 0) return;

	for (FFPSTagStack& Stack : StatTags)
	{
		if (Stack.Tag == Tag)
		{
			Stack.StackCount += StackCount;
			return;
		}
	}
	StatTags.Add(FFPSTagStack(Tag, StackCount));
}

void UFPSInventoryItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid() || StackCount <= 0) return;

	for (int32 i = 0; i < StatTags.Num(); ++i)
	{
		if (StatTags[i].Tag == Tag)
		{
			StatTags[i].StackCount = FMath::Max(0, StatTags[i].StackCount - StackCount);
			if (StatTags[i].StackCount == 0)
			{
				StatTags.RemoveAt(i);
			}
			return;
		}
	}
}

int32 UFPSInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	for (const FFPSTagStack& Stack : StatTags)
	{
		if (Stack.Tag == Tag)
		{
			return Stack.StackCount;
		}
	}
	return 0;
}

bool UFPSInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	for (const FFPSTagStack& Stack : StatTags)
	{
		if (Stack.Tag == Tag && Stack.StackCount > 0)
		{
			return true;
		}
	}
	return false;
}
