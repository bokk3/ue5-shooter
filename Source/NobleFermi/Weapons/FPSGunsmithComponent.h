#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/FPSAttachmentData.h"
#include "FPSGunsmithComponent.generated.h"

class AFPSWeaponBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttachmentModifiedSignature, EFPSAttachmentSlot, Slot, const FFPSAttachmentDefinition&, Attachment);

/**
 * AAA Gunsmith Component
 * Handles modular weapon attachments, dynamic socket attachment, stat modification, and network replication.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOBLEFERMI_API UFPSGunsmithComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSGunsmithComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Gunsmith|Events")
	FOnAttachmentModifiedSignature OnAttachmentEquipped;

	UPROPERTY(BlueprintAssignable, Category = "FPS Gunsmith|Events")
	FOnAttachmentModifiedSignature OnAttachmentRemoved;

	// Client/Server attachment actions
	UFUNCTION(BlueprintCallable, Category = "FPS Gunsmith")
	void EquipAttachment(const FFPSAttachmentDefinition& AttachmentDef);

	UFUNCTION(BlueprintCallable, Category = "FPS Gunsmith")
	void RemoveAttachment(EFPSAttachmentSlot Slot);

	UFUNCTION(BlueprintPure, Category = "FPS Gunsmith")
	bool HasAttachmentInSlot(EFPSAttachmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "FPS Gunsmith")
	FFPSAttachmentDefinition GetAttachmentInSlot(EFPSAttachmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "FPS Gunsmith")
	float GetTotalRecoilMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "FPS Gunsmith")
	float GetTotalRangeMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "FPS Gunsmith")
	float GetTotalAdsFovMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "FPS Gunsmith")
	float GetTotalAdsSpeedModifier() const;

	UFUNCTION(BlueprintPure, Category = "FPS Gunsmith")
	int32 GetTotalMagazineCapacityBonus() const;

	UFUNCTION(BlueprintPure, Category = "FPS Gunsmith")
	float GetTotalReloadDurationMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "FPS Gunsmith")
	bool IsWeaponSilenced() const;

	UFUNCTION(BlueprintCallable, Category = "FPS Gunsmith")
	void ApplyModifiersToWeapon(AFPSWeaponBase* Weapon);

	// Network RPCs
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EquipAttachment(const FFPSAttachmentDefinition& AttachmentDef);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RemoveAttachment(EFPSAttachmentSlot Slot);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedAttachments, VisibleInstanceOnly, Category = "FPS Gunsmith|State")
	TArray<FFPSAttachmentDefinition> EquippedAttachments;

	UFUNCTION()
	void OnRep_EquippedAttachments();

	void RefreshAttachedMeshes();

private:
	UPROPERTY()
	TMap<EFPSAttachmentSlot, TObjectPtr<UStaticMeshComponent>> SpawnedAttachmentMeshes;
};
