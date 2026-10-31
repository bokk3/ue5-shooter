#include "Lyra/Pawn/FPSHeroComponent.h"
#include "Lyra/Pawn/FPSPawnExtensionComponent.h"
#include "Lyra/Camera/FPSCameraComponent.h"
#include "Lyra/AbilitySystem/FPSAbilitySystemComponent.h"
#include "Lyra/AbilitySystem/FPSGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "Components/InputComponent.h"

UFPSHeroComponent::UFPSHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
	DefaultCameraMode = UFPSCameraMode_ThirdPersonOTS::StaticClass();
	ADSCameraMode = UFPSCameraMode_ADS::StaticClass();
	OpticCameraMode = UFPSCameraMode_FirstPersonOptic::StaticClass();
}

void UFPSHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (UFPSPawnExtensionComponent* PawnExt = Pawn->FindComponentByClass<UFPSPawnExtensionComponent>())
		{
			PawnExt->OnPawnReadyToInitialize.AddDynamic(this, &UFPSHeroComponent::OnPawnReadyToInitialize);
		}
	}
}

void UFPSHeroComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UFPSHeroComponent::OnPawnReadyToInitialize()
{
	UpdateCameraMode();
}

void UFPSHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	// Binds input components and initializes hero state
	UpdateCameraMode();
}

void UFPSHeroComponent::SetAimingState(bool bInAiming, bool bInOptic)
{
	bIsAimingState = bInAiming;
	bIsOpticMode = bInOptic;

	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (UFPSPawnExtensionComponent* PawnExt = Pawn->FindComponentByClass<UFPSPawnExtensionComponent>())
		{
			if (UFPSAbilitySystemComponent* ASC = PawnExt->GetFPSAbilitySystemComponent())
			{
				const FFPSGameplayTags& Tags = FFPSGameplayTags::Get();
				if (bIsAimingState)
				{
					ASC->AddTag(Tags.State_Combat_Aiming);
					if (bIsOpticMode)
					{
						ASC->AddTag(Tags.State_Combat_FirstPersonOptic);
					}
					else
					{
						ASC->RemoveTag(Tags.State_Combat_FirstPersonOptic);
					}
				}
				else
				{
					ASC->RemoveTag(Tags.State_Combat_Aiming);
					ASC->RemoveTag(Tags.State_Combat_FirstPersonOptic);
				}
			}
		}
	}

	UpdateCameraMode();
}

void UFPSHeroComponent::UpdateCameraMode()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	UFPSCameraComponent* CameraComp = Pawn->FindComponentByClass<UFPSCameraComponent>();
	if (!CameraComp) return;

	if (bIsAimingState)
	{
		if (bIsOpticMode && OpticCameraMode)
		{
			CameraComp->SetDesiredCameraMode(OpticCameraMode);
		}
		else if (ADSCameraMode)
		{
			CameraComp->SetDesiredCameraMode(ADSCameraMode);
		}
	}
	else if (DefaultCameraMode)
	{
		CameraComp->SetDesiredCameraMode(DefaultCameraMode);
	}
}
