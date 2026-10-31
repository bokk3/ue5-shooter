#include "Lyra/Inventory/FPSInventoryItemDefinition.h"

UFPSInventoryItemDefinition::UFPSInventoryItemDefinition()
{
}

const UFPSInventoryItemFragment* UFPSInventoryItemDefinition::FindFragmentByClass(TSubclassOf<UFPSInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass)
	{
		for (UFPSInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}
	return nullptr;
}
