#include "Weapons/FPSGunsmithComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

UFPSGunsmithComponent::UFPSGunsmithComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UFPSGunsmithComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFPSGunsmithComponent, EquippedAttachments);
}

void UFPSGunsmithComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshAttachedMeshes();
}

void UFPSGunsmithComponent::EquipAttachment(const FFPSAttachmentDefinition& AttachmentDef)
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && !OwnerActor->HasAuthority())
	{
		Server_EquipAttachment(AttachmentDef);
		return;
	}

	// Remove any existing attachment in this slot
	for (int32 i = EquippedAttachments.Num() - 1; i >= 0; --i)
	{
		if (EquippedAttachments[i].Slot == AttachmentDef.Slot)
		{
			EquippedAttachments.RemoveAt(i);
		}
	}

	EquippedAttachments.Add(AttachmentDef);
	OnRep_EquippedAttachments();
	OnAttachmentEquipped.Broadcast(AttachmentDef.Slot, AttachmentDef);
}

void UFPSGunsmithComponent::RemoveAttachment(EFPSAttachmentSlot Slot)
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && !OwnerActor->HasAuthority())
	{
		Server_RemoveAttachment(Slot);
		return;
	}

	for (int32 i = EquippedAttachments.Num() - 1; i >= 0; --i)
	{
		if (EquippedAttachments[i].Slot == Slot)
		{
			FFPSAttachmentDefinition Removed = EquippedAttachments[i];
			EquippedAttachments.RemoveAt(i);
			OnRep_EquippedAttachments();
			OnAttachmentRemoved.Broadcast(Slot, Removed);
			break;
		}
	}
}

bool UFPSGunsmithComponent::HasAttachmentInSlot(EFPSAttachmentSlot Slot) const
{
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		if (Att.Slot == Slot) return true;
	}
	return false;
}

FFPSAttachmentDefinition UFPSGunsmithComponent::GetAttachmentInSlot(EFPSAttachmentSlot Slot) const
{
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		if (Att.Slot == Slot) return Att;
	}
	return FFPSAttachmentDefinition();
}

float UFPSGunsmithComponent::GetTotalRecoilMultiplier() const
{
	float Multiplier = 1.0f;
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		Multiplier *= Att.RecoilMultiplier;
	}
	return FMath::Max(0.2f, Multiplier);
}

float UFPSGunsmithComponent::GetTotalRangeMultiplier() const
{
	float Multiplier = 1.0f;
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		Multiplier *= Att.RangeMultiplier;
	}
	return FMath::Max(0.3f, Multiplier);
}

float UFPSGunsmithComponent::GetTotalAdsFovMultiplier() const
{
	float Multiplier = 1.0f;
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		Multiplier *= Att.AdsFovMultiplier;
	}
	return FMath::Clamp(Multiplier, 0.2f, 1.2f);
}

float UFPSGunsmithComponent::GetTotalAdsSpeedModifier() const
{
	float Modifier = 1.0f;
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		Modifier *= Att.AdsSpeedModifier;
	}
	return FMath::Clamp(Modifier, 0.5f, 2.0f);
}

int32 UFPSGunsmithComponent::GetTotalMagazineCapacityBonus() const
{
	int32 Bonus = 0;
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		Bonus += Att.MagazineCapacityBonus;
	}
	return Bonus;
}

float UFPSGunsmithComponent::GetTotalReloadDurationMultiplier() const
{
	float Multiplier = 1.0f;
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		Multiplier *= Att.ReloadDurationMultiplier;
	}
	return FMath::Clamp(Multiplier, 0.4f, 1.8f);
}

bool UFPSGunsmithComponent::IsWeaponSilenced() const
{
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		if (Att.bSilenced) return true;
	}
	return false;
}

void UFPSGunsmithComponent::ApplyModifiersToWeapon(AFPSWeaponBase* Weapon)
{
	if (!Weapon) return;

	Weapon->bIsSuppressed = IsWeaponSilenced();
}

void UFPSGunsmithComponent::OnRep_EquippedAttachments()
{
	RefreshAttachedMeshes();
}

void UFPSGunsmithComponent::RefreshAttachedMeshes()
{
	AFPSWeaponBase* Weapon = Cast<AFPSWeaponBase>(GetOwner());
	if (!Weapon || !Weapon->WeaponMesh) return;

	// Clean up old meshes
	for (auto& Pair : SpawnedAttachmentMeshes)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}
	SpawnedAttachmentMeshes.Empty();

	// Attach newly equipped meshes
	for (const FFPSAttachmentDefinition& Att : EquippedAttachments)
	{
		UStaticMesh* LoadedMesh = Att.AttachmentMesh.LoadSynchronous();
		if (!LoadedMesh) continue;

		UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(Weapon);
		if (MeshComp)
		{
			MeshComp->SetStaticMesh(LoadedMesh);
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MeshComp->RegisterComponent();
			MeshComp->AttachToComponent(Weapon->WeaponMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Att.SocketName);

			SpawnedAttachmentMeshes.Add(Att.Slot, MeshComp);
		}
	}

	ApplyModifiersToWeapon(Weapon);
}

void UFPSGunsmithComponent::Server_EquipAttachment_Implementation(const FFPSAttachmentDefinition& AttachmentDef)
{
	EquipAttachment(AttachmentDef);
}

bool UFPSGunsmithComponent::Server_EquipAttachment_Validate(const FFPSAttachmentDefinition& AttachmentDef)
{
	return true;
}

void UFPSGunsmithComponent::Server_RemoveAttachment_Implementation(EFPSAttachmentSlot Slot)
{
	RemoveAttachment(Slot);
}

bool UFPSGunsmithComponent::Server_RemoveAttachment_Validate(EFPSAttachmentSlot Slot)
{
	return true;
}
