#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSInventoryComponent.generated.h"

class AFPSWeaponBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveWeaponChangedSignature, AFPSWeaponBase*, NewWeapon);

/**
 * AAA FPS Loadout & Inventory Component
 * Manages active weapon slots, holster/draw transitions, and reserve ammo pools.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "FPS Inventory|Events")
	FOnActiveWeaponChangedSignature OnActiveWeaponChanged;

	UPROPERTY(EditDefaultsOnly, Category = "FPS Inventory|Loadout")
	TSubclassOf<AFPSWeaponBase> DefaultPrimaryClass;

	UPROPERTY(EditDefaultsOnly, Category = "FPS Inventory|Loadout")
	TSubclassOf<AFPSWeaponBase> DefaultSecondaryClass;

	UFUNCTION(BlueprintCallable, Category = "FPS Inventory")
	void EquipSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "FPS Inventory")
	void CycleNextWeapon();

	UFUNCTION(BlueprintPure, Category = "FPS Inventory")
	AFPSWeaponBase* GetActiveWeapon() const { return ActiveWeapon; }

	UFUNCTION(BlueprintPure, Category = "FPS Inventory")
	AFPSWeaponBase* GetPrimaryWeapon() const { return WeaponSlots.IsValidIndex(0) ? WeaponSlots[0].Get() : nullptr; }

	UFUNCTION(BlueprintPure, Category = "FPS Inventory")
	AFPSWeaponBase* GetSecondaryWeapon() const { return WeaponSlots.IsValidIndex(1) ? WeaponSlots[1].Get() : nullptr; }

	UFUNCTION(BlueprintPure, Category = "FPS Inventory")
	int32 GetActiveSlotIndex() const { return CurrentSlotIndex; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<TObjectPtr<AFPSWeaponBase>> WeaponSlots;

	UPROPERTY()
	TObjectPtr<AFPSWeaponBase> ActiveWeapon;

	int32 CurrentSlotIndex = 0;
};
