#include "Lyra/Pawn/FPSPawnExtensionComponent.h"
#include "Lyra/AbilitySystem/FPSGameplayTags.h"
#include "Lyra/AbilitySystem/FPSAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"

UFPSPawnExtensionComponent::UFPSPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFPSPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFPSPawnExtensionComponent, PawnData);
}

void UFPSPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();

	const FFPSGameplayTags& Tags = FFPSGameplayTags::Get();
	CurrentInitState = Tags.InitState_Spawned;
}

void UFPSPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();

	CheckDefaultInitialization();
}

void UFPSPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAbilitySystem();
	Super::EndPlay(EndPlayReason);
}

void UFPSPawnExtensionComponent::SetPawnData(const UFPSPawnData* InPawnData)
{
	check(InPawnData);
	APawn* Pawn = GetPawnChecked<APawn>();

	if (Pawn->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		return;
	}

	PawnData = InPawnData;
	CheckDefaultInitialization();
}

void UFPSPawnExtensionComponent::OnRep_PawnData()
{
	CheckDefaultInitialization();
}

void UFPSPawnExtensionComponent::InitializeAbilitySystem(UFPSAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
	check(InASC);
	check(InOwnerActor);

	if (AbilitySystemComponent == InASC)
	{
		return;
	}

	if (AbilitySystemComponent)
	{
		UninitializeAbilitySystem();
	}

	APawn* Pawn = GetPawnChecked<APawn>();
	AbilitySystemComponent = InASC;
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);

	const FFPSGameplayTags& Tags = FFPSGameplayTags::Get();
	CurrentInitState = Tags.InitState_DataInitialized;

	CheckDefaultInitialization();
}

void UFPSPawnExtensionComponent::UninitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		AbilitySystemComponent->CancelAbilities();
		AbilitySystemComponent->ClearActorInfo();
	}

	AbilitySystemComponent = nullptr;
}

void UFPSPawnExtensionComponent::HandleControllerChanged()
{
	CheckDefaultInitialization();
}

void UFPSPawnExtensionComponent::HandlePlayerStateReplicated()
{
	CheckDefaultInitialization();
}

void UFPSPawnExtensionComponent::SetupPlayerInputComponent()
{
	CheckDefaultInitialization();
}

bool UFPSPawnExtensionComponent::HasFeatureReachedInitState(const FGameplayTag& DesiredState) const
{
	return CurrentInitState.MatchesTagExact(DesiredState);
}

void UFPSPawnExtensionComponent::CheckDefaultInitialization()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;

	const FFPSGameplayTags& Tags = FFPSGameplayTags::Get();

	// Step 1: Check Data Available (Controller & PlayerState present)
	if (CurrentInitState == Tags.InitState_Spawned)
	{
		const bool bHasController = (Pawn->GetController() != nullptr);
		const bool bHasPlayerState = (Pawn->GetPlayerState() != nullptr);

		if (bHasController && bHasPlayerState)
		{
			CurrentInitState = Tags.InitState_DataAvailable;
		}
	}

	// Step 2: Check Data Initialized (Ability System initialized)
	if (CurrentInitState == Tags.InitState_DataAvailable)
	{
		if (AbilitySystemComponent != nullptr)
		{
			CurrentInitState = Tags.InitState_DataInitialized;
		}
	}

	// Step 3: Check Gameplay Ready
	if (CurrentInitState == Tags.InitState_DataInitialized)
	{
		CurrentInitState = Tags.InitState_GameplayReady;
		OnPawnReadyToInitialize.Broadcast();
	}
}
