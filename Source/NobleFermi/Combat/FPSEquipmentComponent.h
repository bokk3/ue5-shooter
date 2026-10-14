#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/FPSProjectileBase.h"
#include "FPSEquipmentComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentCountChangedSignature, int32, LethalCount, int32, TacticalCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFragCookingTickSignature, float, CookTimeRemaining);

/**
 * AAA Tactical Equipment Component
 * Manages player lethal equipment (frag cooking, Semtex, throwing knife) and tactical gear (flashbangs, smoke).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSEquipmentComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Equipment|Events")
	FOnEquipmentCountChangedSignature OnEquipmentCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS Equipment|Events")
	FOnFragCookingTickSignature OnFragCookingTick;

	// Equipment Setup
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Equipment|Loadout")
	EFPSProjectileType EquippedLethal = EFPSProjectileType::FragGrenade;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Equipment|Loadout")
	EFPSProjectileType EquippedTactical = EFPSProjectileType::Flashbang;

	UPROPERTY(EditDefaultsOnly, Category = "FPS Equipment|Spawning")
	TSubclassOf<AFPSProjectileBase> ProjectileClass;

	// Player Input Actions
	UFUNCTION(BlueprintCallable, Category = "FPS Equipment|Actions")
	void StartCookingLethal();

	UFUNCTION(BlueprintCallable, Category = "FPS Equipment|Actions")
	void ReleaseAndThrowLethal();

	UFUNCTION(BlueprintCallable, Category = "FPS Equipment|Actions")
	void ThrowTactical();

	// Scavenger perk resupply
	UFUNCTION(BlueprintCallable, Category = "FPS Equipment|Actions")
	void ResupplyEquipment(int32 LethalsToAdd = 1, int32 TacticalsToAdd = 1);

	// Getters
	UFUNCTION(BlueprintPure, Category = "FPS Equipment|State")
	int32 GetLethalCount() const { return CurrentLethalCount; }

	UFUNCTION(BlueprintPure, Category = "FPS Equipment|State")
	int32 GetTacticalCount() const { return CurrentTacticalCount; }

	UFUNCTION(BlueprintPure, Category = "FPS Equipment|State")
	bool IsCookingFrag() const { return bIsCookingFrag; }

	UFUNCTION(BlueprintPure, Category = "FPS Equipment|State")
	float GetFragCookElapsed() const { return FragCookElapsed; }

	// Server RPCs
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ThrowLethal(FVector SpawnLocation, FVector LaunchVelocity, float CustomFuse);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ThrowTactical(FVector SpawnLocation, FVector LaunchVelocity);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_EquipmentCounts, VisibleInstanceOnly, Category = "FPS Equipment|State")
	int32 CurrentLethalCount = 1;

	UPROPERTY(ReplicatedUsing = OnRep_EquipmentCounts, VisibleInstanceOnly, Category = "FPS Equipment|State")
	int32 CurrentTacticalCount = 2;

	UPROPERTY(EditDefaultsOnly, Category = "FPS Equipment|Tuning")
	float MaxFragFuseTime = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "FPS Equipment|Tuning")
	float ThrowImpulseSpeed = 1800.0f;

	UFUNCTION()
	void OnRep_EquipmentCounts();

private:
	bool bIsCookingFrag = false;
	float FragCookElapsed = 0.0f;

	void DetonateFragInHand();
};
