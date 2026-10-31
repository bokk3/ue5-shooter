#include "Lyra/AbilitySystem/FPSAbilitySystemComponent.h"

UFPSAbilitySystemComponent::UFPSAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	ReplicationMode = EGameplayEffectReplicationMode::Mixed;
}

void UFPSAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
}

void UFPSAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.AddUnique(Spec.Handle);
		}
	}
}

void UFPSAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.Remove(Spec.Handle);
		}
	}
}

void UFPSAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	if (bGamePaused) return;

	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate = InputPressedSpecHandles;
	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesToActivate)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability)
			{
				Spec->InputPressed = true;
				if (Spec->IsActive())
				{
					AbilitySpecInputPressed(*Spec);
				}
				else
				{
					TryActivateAbility(Spec->Handle);
				}
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (Spec->Ability)
			{
				Spec->InputPressed = false;
				if (Spec->IsActive())
				{
					AbilitySpecInputReleased(*Spec);
				}
			}
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

bool UFPSAbilitySystemComponent::HasActiveTag(const FGameplayTag& TagToCheck) const
{
	return HasMatchingGameplayTag(TagToCheck);
}

void UFPSAbilitySystemComponent::AddTag(const FGameplayTag& TagToAdd)
{
	if (TagToAdd.IsValid())
	{
		Super::AddLooseGameplayTag(TagToAdd, 1);
	}
}

void UFPSAbilitySystemComponent::RemoveTag(const FGameplayTag& TagToRemove)
{
	if (TagToRemove.IsValid())
	{
		Super::RemoveLooseGameplayTag(TagToRemove, 1);
	}
}
