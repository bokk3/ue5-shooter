#include "Lyra/Equipment/FPSEquipmentInstance.h"
#include "Lyra/Equipment/FPSEquipmentDefinition.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UFPSEquipmentInstance::UFPSEquipmentInstance()
{
}

void UFPSEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFPSEquipmentInstance, SpawnedActors);
	DOREPLIFETIME(UFPSEquipmentInstance, Instigator);
}

APawn* UFPSEquipmentInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

void UFPSEquipmentInstance::OnEquipped()
{
}

void UFPSEquipmentInstance::OnUnequipped()
{
}

void UFPSEquipmentInstance::SpawnEquipmentActors(const TArray<FFPSEquipmentActorToSpawn>& ActorsToSpawn)
{
	APawn* OwningPawn = GetPawn();
	if (!OwningPawn) return;

	USceneComponent* AttachTarget = OwningPawn->GetRootComponent();
	if (ACharacter* Char = Cast<ACharacter>(OwningPawn))
	{
		AttachTarget = Char->GetMesh();
	}

	for (const FFPSEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
	{
		if (SpawnInfo.ActorToSpawn)
		{
			AActor* NewActor = OwningPawn->GetWorld()->SpawnActorDeferred<AActor>(
				SpawnInfo.ActorToSpawn,
				FTransform::Identity,
				OwningPawn,
				OwningPawn,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn
			);

			if (NewActor)
			{
				NewActor->FinishSpawning(FTransform::Identity, true);
				NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetIncludingScale, SpawnInfo.AttachSocket);
				NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
				SpawnedActors.Add(NewActor);
			}
		}
	}
}

void UFPSEquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();
}
