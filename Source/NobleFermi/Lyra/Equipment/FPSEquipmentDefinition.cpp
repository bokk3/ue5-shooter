#include "Lyra/Equipment/FPSEquipmentDefinition.h"
#include "Lyra/Equipment/FPSEquipmentInstance.h"

UFPSEquipmentDefinition::UFPSEquipmentDefinition()
{
	InstanceType = UFPSEquipmentInstance::StaticClass();
}
