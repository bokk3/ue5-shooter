#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Lyra/Camera/FPSCameraMode.h"
#include "Lyra/Inventory/FPSInventoryItemDefinition.h"
#include "FPSPawnData.generated.h"

class APawn;
class UFPSCameraMode;

/**
 * Primary Data Asset holding initialization data for a Lyra pawn (Lyra pattern)
 */
UCLASS(BlueprintType, Const)
class NOBLEFERMI_API UFPSPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFPSPawnData();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pawn")
	TSubclassOf<APawn> PawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TSubclassOf<UFPSCameraMode> DefaultCameraMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TArray<TSubclassOf<UFPSInventoryItemDefinition>> InitialInventoryItems;
};
