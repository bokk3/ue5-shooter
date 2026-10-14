#include "UI/FPSKillfeedWidget.h"

void UFPSKillfeedWidget::AddKillfeedEntry(const FFPSKillfeedEntry& NewEntry)
{
	OnKillfeedEntryAdded(NewEntry);
}

void UFPSKillfeedWidget::TriggerAccolade(const FText& AccoladeTitle, int32 BonusScore)
{
	OnAccoladePopped(AccoladeTitle, BonusScore);
}
