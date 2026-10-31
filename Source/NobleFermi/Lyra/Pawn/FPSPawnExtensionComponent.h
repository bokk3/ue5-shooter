#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "Lyra/Pawn/FPSPawnData.h"
#include "FPSPawnExtensionComponent.generated.h"

class UFPSAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPawnReadyToInitializeSignature);

/**
 * Coordinates modular pawn initialization lifecycle across network replication (Lyra pattern)
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSPawnExtensionComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UFPSPawnExtensionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Pawn")
	FOnPawnReadyToInitializeSignature OnPawnReadyToInitialize;

	void SetPawnData(const UFPSPawnData* InPawnData);

	UFUNCTION(BlueprintPure, Category = "Pawn")
	const UFPSPawnData* GetPawnData() const { return PawnData; }

	UFUNCTION(BlueprintPure, Category = "Pawn")
	UFPSAbilitySystemComponent* GetFPSAbilitySystemComponent() const { return AbilitySystemComponent; }

	void InitializeAbilitySystem(UFPSAbilitySystemComponent* InASC, AActor* InOwnerActor);
	void UninitializeAbilitySystem();

	void HandleControllerChanged();
	void HandlePlayerStateReplicated();
	void SetupPlayerInputComponent();

	UFUNCTION(BlueprintPure, Category = "Pawn")
	FGameplayTag GetCurrentInitState() const { return CurrentInitState; }

	UFUNCTION(BlueprintPure, Category = "Pawn")
	bool HasFeatureReachedInitState(const FGameplayTag& DesiredState) const;

	void CheckDefaultInitialization();

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditInstanceOnly, ReplicatedUsing = OnRep_PawnData, Category = "Pawn")
	TObjectPtr<const UFPSPawnData> PawnData;

	UPROPERTY(Transient)
	TObjectPtr<UFPSAbilitySystemComponent> AbilitySystemComponent;

	FGameplayTag CurrentInitState;

	UFUNCTION()
	void OnRep_PawnData();
};
