#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FPSEquipmentInstance.generated.h"

class AActor;
class APawn;

/**
 * Replicated instance of an active piece of equipment (Lyra pattern)
 */
UCLASS(BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	UFPSEquipmentInstance();

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	APawn* GetPawn() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	virtual void OnEquipped();
	virtual void OnUnequipped();

	void SpawnEquipmentActors(const TArray<struct FFPSEquipmentActorToSpawn>& ActorsToSpawn);
	void DestroyEquipmentActors();

protected:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SpawnedActors;

	UPROPERTY(Replicated)
	TObjectPtr<UObject> Instigator;
};
