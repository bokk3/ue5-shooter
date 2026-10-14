#include "UI/FPSButtonBase.h"
#include "Kismet/GameplayStatics.h"

UFPSButtonBase::UFPSButtonBase()
{
	bIsFocusable = true;
	ButtonLabel = FText::FromString(TEXT("MENU ITEM"));
}

void UFPSButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetButtonLabel(ButtonLabel);
}

void UFPSButtonBase::SetButtonLabel(const FText& InLabel)
{
	ButtonLabel = InLabel;
}

void UFPSButtonBase::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();
}

void UFPSButtonBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (HoverSound)
	{
		UGameplayStatics::PlaySound2D(this, HoverSound);
	}
}

void UFPSButtonBase::NativeOnClicked()
{
	Super::NativeOnClicked();

	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}
}
