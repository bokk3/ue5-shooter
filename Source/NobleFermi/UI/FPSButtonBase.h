#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "FPSButtonBase.generated.h"

class USoundBase;
class UTextBlock;

/**
 * AAA FPS Styled Menu Button
 * Supports CommonUI input routing, gamepad focus, custom sounds, and dynamic text styling.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NOBLEFERMI_API UFPSButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UFPSButtonBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Button|Styling")
	FText ButtonLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Button|Audio")
	TObjectPtr<USoundBase> HoverSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Button|Audio")
	TObjectPtr<USoundBase> ClickSound;

	UFUNCTION(BlueprintCallable, Category = "FPS Button")
	void SetButtonLabel(const FText& InLabel);

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS Button")
	void OnButtonFocusedStateChanged(bool bIsFocused);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnCurrentTextStyleChanged() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnClicked() override;
};
