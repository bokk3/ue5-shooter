#include "UI/FPSEscapeMenuWidget.h"
#include "Character/FPSCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"

UFPSEscapeMenuWidget::UFPSEscapeMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UFPSEscapeMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

void UFPSEscapeMenuWidget::SetOwningCharacter(AFPSCharacter* InCharacter)
{
	OwningFPSCharacter = InCharacter;
}

TSharedRef<SWidget> UFPSEscapeMenuWidget::RebuildWidget()
{
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
	const FSlateFontInfo SubTitleFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	const FSlateFontInfo ButtonFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);

	const FLinearColor AmberColor(1.0f, 0.70f, 0.0f, 1.0f);   // #FFB300
	const FLinearColor CyanColor(0.0f, 0.90f, 1.0f, 1.0f);    // #00E5FF
	const FLinearColor MutedColor(0.55f, 0.65f, 0.75f, 0.8f);

	auto CreateMenuButton = [this, ButtonFont, AmberColor](const FText& LabelText, FOnClicked OnClickedDelegate) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(44.0f)
			.Padding(FMargin(0.0f, 4.0f))
			[
				SNew(SButton)
				.OnClicked(OnClickedDelegate)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LabelText)
					.Font(ButtonFont)
					.ColorAndOpacity(AmberColor)
				]
			];
	};

	return SNew(SOverlay)
		// Fullscreen dark tactical backdrop
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.88f))
		]
		// Centered tactical command modal
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(460.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.04f, 0.07f, 0.11f, 0.95f))
				.Padding(FMargin(28.0f, 24.0f))
				[
					SNew(SVerticalBox)
					// Header
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("NOBLE FERMI // TACTICAL COMMAND")))
						.Font(TitleFont)
						.ColorAndOpacity(AmberColor)
					]
					// Subtitle telemetry
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.0f, 0.0f, 0.0f, 18.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("LIVE SERVER 128 HZ // NON-PAUSED SESSION")))
						.Font(SubTitleFont)
						.ColorAndOpacity(CyanColor)
					]
					// Button list
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						CreateMenuButton(
							FText::FromString(TEXT("RESUME OPERATION  [ESC / P]")),
							FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnResumeClicked)
						)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						CreateMenuButton(
							FText::FromString(TEXT("SETTINGS & CONTROLS")),
							FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnSettingsClicked)
						)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						CreateMenuButton(
							FText::FromString(TEXT("RETURN TO MAIN MENU")),
							FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnMainMenuClicked)
						)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						CreateMenuButton(
							FText::FromString(TEXT("QUIT TO DESKTOP")),
							FOnClicked::CreateUObject(this, &UFPSEscapeMenuWidget::OnExitClicked)
						)
					]
					// Footer note
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.0f, 16.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("PRESS [ESC] OR [P] TO TOGGLE COMMAND OVERLAY")))
						.Font(SubTitleFont)
						.ColorAndOpacity(MutedColor)
					]
				]
			]
		];
}

FReply UFPSEscapeMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::P)
	{
		ResumeOperation();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFPSEscapeMenuWidget::ResumeOperation()
{
	if (OwningFPSCharacter.IsValid())
	{
		OwningFPSCharacter->ToggleEscapeMenu();
	}
}

void UFPSEscapeMenuWidget::OpenSettings()
{
	// CommonUI / Slate settings toggle hook
	ResumeOperation();
}

void UFPSEscapeMenuWidget::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/MainMenuMap")));
}

void UFPSEscapeMenuWidget::ExitToDesktop()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

FReply UFPSEscapeMenuWidget::OnResumeClicked()
{
	ResumeOperation();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnSettingsClicked()
{
	OpenSettings();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnMainMenuClicked()
{
	ReturnToMainMenu();
	return FReply::Handled();
}

FReply UFPSEscapeMenuWidget::OnExitClicked()
{
	ExitToDesktop();
	return FReply::Handled();
}
