#include "Inventory/FPSInventoryComponent.h"
#include "Weapons/FPSWeaponDataAsset.h"
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

	// Spawn Default Primary (Assault Rifle DA_W01_NF98)
	TSubclassOf<AFPSWeaponBase> PrimaryClass = DefaultPrimaryClass ? DefaultPrimaryClass : TSubclassOf<AFPSWeaponBase>(AFPSWeaponBase::StaticClass());
	AFPSWeaponBase* Primary = World->SpawnActor<AFPSWeaponBase>(PrimaryClass, SpawnParams);
	if (Primary)
	{
		UFPSWeaponDataAsset* PrimaryDA = LoadObject<UFPSWeaponDataAsset>(nullptr, TEXT("/Game/Weapons/Data/DA_W01_NF98.DA_W01_NF98"));
		if (PrimaryDA)
		{
			Primary->ApplyWeaponDataAsset(PrimaryDA);
		}
		WeaponSlots[0] = Primary;
		Primary->SetActorHiddenInGame(true);
	}

	// Spawn Default Secondary (SMG DA_W05_Apex9)
	TSubclassOf<AFPSWeaponBase> SecondaryClass = DefaultSecondaryClass ? DefaultSecondaryClass : TSubclassOf<AFPSWeaponBase>(AFPSWeaponBase::StaticClass());
	AFPSWeaponBase* Secondary = World->SpawnActor<AFPSWeaponBase>(SecondaryClass, SpawnParams);
	if (Secondary)
	{
		UFPSWeaponDataAsset* SecondaryDA = LoadObject<UFPSWeaponDataAsset>(nullptr, TEXT("/Game/Weapons/Data/DA_W05_Apex9.DA_W05_Apex9"));
		if (SecondaryDA)
		{
			Secondary->ApplyWeaponDataAsset(SecondaryDA);
		}
		WeaponSlots[1] = Secondary;
		Secondary->SetActorHiddenInGame(true);
	}

	// Spawn Default Sidearm (Tactical Pistol DA_W06_Spectre45)
	TSubclassOf<AFPSWeaponBase> SidearmClass = DefaultSidearmClass ? DefaultSidearmClass : TSubclassOf<AFPSWeaponBase>(AFPSWeaponBase::StaticClass());
	AFPSWeaponBase* Sidearm = World->SpawnActor<AFPSWeaponBase>(SidearmClass, SpawnParams);
	if (Sidearm)
	{
		UFPSWeaponDataAsset* SidearmDA = LoadObject<UFPSWeaponDataAsset>(nullptr, TEXT("/Game/Weapons/Data/DA_W06_Spectre45.DA_W06_Spectre45"));
		if (SidearmDA)
		{
			Sidearm->ApplyWeaponDataAsset(SidearmDA);
		}
		WeaponSlots[2] = Sidearm;
		Sidearm->SetActorHiddenInGame(true);
	}

	// Equip primary by default
	EquipSlot(0);
}

void UFPSInventoryComponent::EquipSlot(int32 SlotIndex)
{
	if (!WeaponSlots.IsValidIndex(SlotIndex)) return;

	AFPSWeaponBase* TargetWeapon = WeaponSlots[SlotIndex];
	if (!TargetWeapon || TargetWeapon == ActiveWeapon) return;

	// Holster active weapon cleanly aborting any in-progress reload
	if (ActiveWeapon)
	{
		ActiveWeapon->StopFire();
		ActiveWeapon->AbortReload();
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
	for (int32 i = 1; i <= WeaponSlots.Num(); ++i)
	{
		int32 NextSlot = (CurrentSlotIndex + i) % WeaponSlots.Num();
		if (WeaponSlots[NextSlot] && WeaponSlots[NextSlot] != ActiveWeapon)
		{
			EquipSlot(NextSlot);
			return;
		}
	}
}

void UFPSInventoryComponent::CyclePreviousWeapon()
{
	for (int32 i = 1; i <= WeaponSlots.Num(); ++i)
	{
		int32 PrevSlot = (CurrentSlotIndex - i + WeaponSlots.Num()) % WeaponSlots.Num();
		if (WeaponSlots[PrevSlot] && WeaponSlots[PrevSlot] != ActiveWeapon)
		{
			EquipSlot(PrevSlot);
			return;
		}
	}
}
