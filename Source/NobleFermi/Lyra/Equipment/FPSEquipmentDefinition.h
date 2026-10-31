#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "FPSEquipmentDefinition.generated.h"

class UFPSEquipmentInstance;

/**
 * Specification for an actor spawned by equipment (weapon mesh, tactical accessory, armor plate)
 */
USTRUCT(BlueprintType)
struct NOBLEFERMI_API FFPSEquipmentActorToSpawn
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	FName AttachSocket;

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	FTransform AttachTransform;
};

/**
 * Definition of an equippable item piece (Lyra pattern)
 */
UCLASS(Blueprintable, Const, Abstract)
class NOBLEFERMI_API UFPSEquipmentDefinition : public UObject
{
	GENERATED_BODY()

public:
	UFPSEquipmentDefinition();

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<UFPSEquipmentInstance> InstanceType;

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TArray<FFPSEquipmentActorToSpawn> ActorsToSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	FGameplayTag EquipmentSlotTag;

	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	FGameplayTagContainer GrantedTags;
};
