#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Multiplayer/FPSLoadoutTypes.h"
#include "FPSLoadoutSelectWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClassSelectedSignature, int32, ClassIndex);

/**
 * AAA MW2 Style Create-A-Class In-Game Loadout Selector Widget
 */
UCLASS()
class NOBLEFERMI_API UFPSLoadoutSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintAssignable, Category = "FPS Loadout")
	FOnClassSelectedSignature OnClassSelected;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Loadout")
	TArray<FFPSCustomClassLoadout> AvailableClasses;

	UFUNCTION(BlueprintCallable, Category = "FPS Loadout")
	void SelectClass(int32 ClassIndex);

	UFUNCTION(BlueprintCallable, Category = "FPS Loadout")
	FFPSCustomClassLoadout GetSelectedClass() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS Loadout")
	void OnLoadoutMenuRefreshed(const TArray<FFPSCustomClassLoadout>& Classes, int32 CurrentSelectedIndex);

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FPS Loadout")
	int32 SelectedClassIndex = 0;

private:
	void InitializeDefaultClasses();
};
