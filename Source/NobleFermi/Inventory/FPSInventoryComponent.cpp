#include "Inventory/FPSInventoryComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "Character/FPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

UFPSInventoryComponent::UFPSInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	WeaponSlots.SetNum(3);
}

void UFPSInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!World || !OwnerActor) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);

	// Spawn Default Primary
	if (DefaultPrimaryClass)
	{
		AFPSWeaponBase* Primary = World->SpawnActor<AFPSWeaponBase>(DefaultPrimaryClass, SpawnParams);
		if (Primary)
		{
			WeaponSlots[0] = Primary;
			Primary->SetActorHiddenInGame(true);
		}
	}

	// Spawn Default Secondary
	if (DefaultSecondaryClass)
	{
		AFPSWeaponBase* Secondary = World->SpawnActor<AFPSWeaponBase>(DefaultSecondaryClass, SpawnParams);
		if (Secondary)
		{
			WeaponSlots[1] = Secondary;
			Secondary->SetActorHiddenInGame(true);
		}
	}

	// Equip primary by default
	EquipSlot(0);
}

void UFPSInventoryComponent::EquipSlot(int32 SlotIndex)
{
	if (!WeaponSlots.IsValidIndex(SlotIndex)) return;

	AFPSWeaponBase* TargetWeapon = WeaponSlots[SlotIndex];
	if (!TargetWeapon || TargetWeapon == ActiveWeapon) return;

	// Holster active weapon
	if (ActiveWeapon)
	{
		ActiveWeapon->StopFire();
		ActiveWeapon->SetActorHiddenInGame(true);
	}

	// Unholster new weapon
	CurrentSlotIndex = SlotIndex;
	ActiveWeapon = TargetWeapon;
	ActiveWeapon->SetActorHiddenInGame(false);

	// Primary 3rd-person tactical weapon attachment: attach authoritatively to hand_r / weapon_r
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (Char && Char->GetMesh())
	{
		USkeletalMeshComponent* MeshComp = Char->GetMesh();
		FName SocketName = FName(TEXT("hand_r"));

		// If a calibrated weapon_r socket exists and is properly co-located near the hand (not at origin 0,0,0)
		if (MeshComp->DoesSocketExist(TEXT("weapon_r")))
		{
			const FVector WeaponRSocketLoc = MeshComp->GetSocketLocation(TEXT("weapon_r"));
			const FVector HandRLoc = MeshComp->GetSocketLocation(TEXT("hand_r"));
			if (FVector::DistSquared(WeaponRSocketLoc, HandRLoc) < 1225.0f) // within 35cm
			{
				SocketName = FName(TEXT("weapon_r"));
			}
		}

		ActiveWeapon->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		ActiveWeapon->SetActorRelativeLocation(ActiveWeapon->GetThirdPersonLocation());
		ActiveWeapon->SetActorRelativeRotation(ActiveWeapon->GetThirdPersonRotation());
		ActiveWeapon->SetActorHiddenInGame(false);
	}
	else
	{
		AFPSCharacter* FPSChar = Cast<AFPSCharacter>(GetOwner());
		if (FPSChar && FPSChar->FirstPersonCamera)
		{
			ActiveWeapon->AttachToComponent(FPSChar->FirstPersonCamera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			ActiveWeapon->SetActorRelativeLocation(ActiveWeapon->GetHipfireLocation());
			ActiveWeapon->SetActorRelativeRotation(ActiveWeapon->GetHipfireRotation());
		}
	}

	OnActiveWeaponChanged.Broadcast(ActiveWeapon);
}

void UFPSInventoryComponent::CycleNextWeapon()
{
	int32 NextSlot = (CurrentSlotIndex + 1) % WeaponSlots.Num();
	// Find next valid weapon slot
	for (int32 i = 0; i < WeaponSlots.Num(); ++i)
	{
		if (WeaponSlots[NextSlot])
		{
			EquipSlot(NextSlot);
			return;
		}
		NextSlot = (NextSlot + 1) % WeaponSlots.Num();
	}
}
