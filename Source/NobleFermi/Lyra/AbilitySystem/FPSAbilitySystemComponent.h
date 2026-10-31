#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Lyra/AbilitySystem/FPSGameplayTags.h"
#include "FPSAbilitySystemComponent.generated.h"

/**
 * Lyra Replicated Ability System Component for Noble Fermi
 */
UCLASS()
class NOBLEFERMI_API UFPSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UFPSAbilitySystemComponent();

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	/** Input Tag bindings (Enhanced Input -> GAS) */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	/** Fast Tag Query */
	UFUNCTION(BlueprintCallable, Category = "FPS|Abilities")
	bool HasActiveTag(const FGameplayTag& TagToCheck) const;

	/** Server-Authoritative Tag Toggle */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FPS|Abilities")
	void AddTag(const FGameplayTag& TagToAdd);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FPS|Abilities")
	void RemoveTag(const FGameplayTag& TagToRemove);

protected:
	// Local input caching for predictive execution
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
