#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InputCoreTypes.h"
#include "FPSKeybindSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FFPSActionBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keybind")
	FName ActionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keybind")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keybind")
	FKey PrimaryKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keybind")
	FKey SecondaryKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keybind")
	FKey GamepadKey;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKeybindsUpdatedSignature);

/**
 * AAA FPS Keybinding Subsystem
 * Manages player customizable key bindings, conflict detection, and persistence.
 */
UCLASS()
class NOBLEFERMI_API UFPSKeybindSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "Keybinds")
	FOnKeybindsUpdatedSignature OnKeybindsUpdated;

	UFUNCTION(BlueprintPure, Category = "Keybinds")
	TArray<FFPSActionBinding> GetAllBindings() const;

	UFUNCTION(BlueprintCallable, Category = "Keybinds")
	bool RemapActionKey(FName ActionName, FKey NewKey, bool bIsSecondary = false);

	UFUNCTION(BlueprintPure, Category = "Keybinds")
	FName FindConflict(FKey KeyToCheck, FName IgnoreActionName) const;

	UFUNCTION(BlueprintCallable, Category = "Keybinds")
	void ResetAllToDefaults();

	UFUNCTION(BlueprintCallable, Category = "Keybinds")
	void SaveKeybinds();

private:
	UPROPERTY()
	TArray<FFPSActionBinding> ActionBindings;

	void SetupDefaultBindings();
	void LoadSavedKeybinds();
};
