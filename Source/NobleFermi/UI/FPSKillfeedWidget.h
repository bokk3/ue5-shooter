#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSKillfeedWidget.generated.h"

USTRUCT(BlueprintType)
struct FFPSKillfeedEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Killfeed")
	FString KillerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Killfeed")
	bool bKillerIsFriendly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Killfeed")
	FString VictimName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Killfeed")
	bool bVictimIsFriendly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Killfeed")
	FString WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Killfeed")
	bool bIsHeadshot = false;
};

/**
 * Classic MW2 Style Multiplayer Killfeed & Accolade Widget
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSKillfeedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FPS Killfeed")
	void AddKillfeedEntry(const FFPSKillfeedEntry& NewEntry);

	UFUNCTION(BlueprintCallable, Category = "FPS Killfeed")
	void TriggerAccolade(const FText& AccoladeTitle, int32 BonusScore);

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS Killfeed")
	void OnKillfeedEntryAdded(const FFPSKillfeedEntry& Entry);

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS Killfeed")
	void OnAccoladePopped(const FText& AccoladeTitle, int32 BonusScore);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Killfeed")
	int32 MaxVisibleEntries = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Killfeed")
	float EntryLifetimeSeconds = 5.0f;
};
