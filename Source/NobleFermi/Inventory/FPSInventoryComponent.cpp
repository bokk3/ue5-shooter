#include "Inventory/FPSInventoryComponent.h"
#include "Weapons/FPSWeaponBase.h"
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

	// Attach weapon to character's first-person hand socket if available
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (Char && Char->GetMesh())
	{
		ActiveWeapon->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("GripPoint"));
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
