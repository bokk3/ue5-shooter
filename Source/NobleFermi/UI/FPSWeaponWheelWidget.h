#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Weapons/FPSWeaponDataAsset.h"
#include "FPSWeaponWheelWidget.generated.h"

class AFPSCharacter;
class SWidget;

USTRUCT(BlueprintType)
struct FFPSWeaponWheelEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
	FString WeaponID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
	FText CategoryName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
	FString Caliber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
	EFPSWeaponArchetype Archetype = EFPSWeaponArchetype::AssaultRifle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
	FString AssetPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
	int32 SlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
	TObjectPtr<class UTexture2D> WeaponIcon = nullptr;
};

/**
 * AAA Military Visor Weapon Selection Wheel Widget
 * Renders an 8-segment or 11-segment radial tactical wheel in Slate.
 * Active while selection key (default 'TAB' or '4') is held down.
 * Supports:
 * - Direction keys (WASD, Arrow keys) for sector highlighting
 * - Mouse cursor deflection angle & distance for seamless gesture selection
 * - Real-time switching on release
 */
UCLASS(BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSWeaponWheelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFPSWeaponWheelWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	void SetOwningCharacter(AFPSCharacter* InCharacter);

	UFUNCTION(BlueprintCallable, Category = "FPS Weapon Wheel")
	void ShowWeaponWheel();

	UFUNCTION(BlueprintCallable, Category = "FPS Weapon Wheel")
	void HideAndSelectWeapon();

	UFUNCTION(BlueprintPure, Category = "FPS Weapon Wheel")
	bool IsWheelOpen() const { return bIsOpen; }

	UFUNCTION(BlueprintCallable, Category = "FPS Weapon Wheel")
	void SetDirectionalInput(FVector2D Direction);

	UFUNCTION(BlueprintCallable, Category = "FPS Weapon Wheel")
	void SetHighlightedIndex(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category = "FPS Weapon Wheel")
	int32 GetHighlightedIndex() const { return HighlightedIndex; }

	UFUNCTION(BlueprintPure, Category = "FPS Weapon Wheel")
	const TArray<FFPSWeaponWheelEntry>& GetArsenalEntries() const { return ArsenalEntries; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Weapon Wheel|Visuals")
	FLinearColor VisorCyan = FLinearColor(0.0f, 0.90f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Weapon Wheel|Visuals")
	FLinearColor VisorAmber = FLinearColor(1.0f, 0.70f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Weapon Wheel|Visuals")
	FLinearColor VisorDarkBg = FLinearColor(0.012f, 0.016f, 0.024f, 0.88f);

	// Public access for Slate renderer
	float OpenAnimationAlpha = 0.0f;
	FVector2D RawMousePos = FVector2D::ZeroVector;
	FVector2D DirectionalVector = FVector2D::ZeroVector;

private:
	UPROPERTY()
	TWeakObjectPtr<AFPSCharacter> OwningCharacter;

	UPROPERTY()
	TArray<FFPSWeaponWheelEntry> ArsenalEntries;

	bool bIsOpen = false;
	int32 HighlightedIndex = 0;
	int32 PreviousHighlightedIndex = -1;

	void PopulateArsenalEntries();
};
