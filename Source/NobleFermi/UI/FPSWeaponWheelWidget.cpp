#include "UI/FPSWeaponWheelWidget.h"
#include "Character/FPSCharacter.h"
#include "Inventory/FPSInventoryComponent.h"
#include "Weapons/FPSWeaponBase.h"
#include "Weapons/FPSWeaponDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SCompoundWidget.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"

/**
 * Custom Slate Canvas Widget for Drawing the High-Tech Weapon Wheel
 */
class SFPSWeaponWheelCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSWeaponWheelCanvas) {}
		SLATE_ARGUMENT(UFPSWeaponWheelWidget*, OwningWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OwningWidget = InArgs._OwningWidget;
		SetCanTick(false);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		if (!OwningWidget || OwningWidget->OpenAnimationAlpha <= 0.001f)
		{
			return LayerId;
		}

		const FVector2f LocalSize = AllottedGeometry.GetLocalSize();
		const FVector2f Center = LocalSize * 0.5f;
		const float Alpha = OwningWidget->OpenAnimationAlpha;

		const FSlateBrush* WhiteBrush = FAppStyle::Get().GetBrush("WhiteBrush");
		const FSlateResourceHandle ResourceHandle = WhiteBrush->GetRenderingResource();
		const FSlateRenderTransform& RenderTransform = AllottedGeometry.ToPaintGeometry().GetAccumulatedRenderTransform();

		// Colors
		const FLinearColor Cyan = OwningWidget->VisorCyan * Alpha;
		const FLinearColor Amber = OwningWidget->VisorAmber * Alpha;
		const FLinearColor DarkBg = FLinearColor(0.012f, 0.016f, 0.024f, 0.82f * Alpha);
		const FLinearColor ActiveBg = FLinearColor(0.02f, 0.09f, 0.14f, 0.90f * Alpha);
		const FLinearColor MutedText = FLinearColor(0.6f, 0.7f, 0.8f, 0.8f * Alpha);

		const TArray<FFPSWeaponWheelEntry>& Entries = OwningWidget->GetArsenalEntries();
		const int32 NumSectors = Entries.Num();
		if (NumSectors == 0) return LayerId;

		const float OuterRadius = 260.0f * (0.85f + 0.15f * Alpha);
		const float InnerRadius = 120.0f;
		const float SectorAngle = 360.0f / NumSectors;
		const int32 HighlightedIdx = OwningWidget->GetHighlightedIndex();

		// 1. Draw Outer & Inner Wheel Rings with Segmented Accents
		const int32 CirclePoints = 72;
		TArray<FSlateVertex> RingVerts;
		TArray<SlateIndex> RingIndices;

		const FColor RingColor = (Cyan * 0.5f).ToFColor(true);

		for (int32 i = 0; i < CirclePoints; ++i)
		{
			float Angle0 = FMath::DegreesToRadians(i * (360.0f / CirclePoints));
			float Angle1 = FMath::DegreesToRadians((i + 1) * (360.0f / CirclePoints));

			FVector2f P0_Out = Center + FVector2f(FMath::Sin(Angle0), -FMath::Cos(Angle0)) * (OuterRadius + 8.0f);
			FVector2f P1_Out = Center + FVector2f(FMath::Sin(Angle1), -FMath::Cos(Angle1)) * (OuterRadius + 8.0f);
			FVector2f P0_In  = Center + FVector2f(FMath::Sin(Angle0), -FMath::Cos(Angle0)) * (OuterRadius + 4.0f);
			FVector2f P1_In  = Center + FVector2f(FMath::Sin(Angle1), -FMath::Cos(Angle1)) * (OuterRadius + 4.0f);

			int32 BaseIdx = RingVerts.Num();
			RingVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, P0_Out, FVector2f(0.0f, 0.0f), RingColor));
			RingVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, P1_Out, FVector2f(1.0f, 0.0f), RingColor));
			RingVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, P1_In,  FVector2f(1.0f, 1.0f), RingColor));
			RingVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, P0_In,  FVector2f(0.0f, 1.0f), RingColor));

			RingIndices.Add(BaseIdx + 0); RingIndices.Add(BaseIdx + 1); RingIndices.Add(BaseIdx + 2);
			RingIndices.Add(BaseIdx + 0); RingIndices.Add(BaseIdx + 2); RingIndices.Add(BaseIdx + 3);
		}

		if (RingVerts.Num() > 0)
		{
			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements,
				LayerId++,
				ResourceHandle,
				RingVerts,
				RingIndices,
				nullptr, 0, 0
			);
		}

		// 2. Draw Sectors
		for (int32 s = 0; s < NumSectors; ++s)
		{
			const bool bIsSelected = (s == HighlightedIdx);
			const float MidAngleDeg = s * SectorAngle;
			const float StartAngleDeg = MidAngleDeg - (SectorAngle * 0.5f) + 1.2f;
			const float EndAngleDeg   = MidAngleDeg + (SectorAngle * 0.5f) - 1.2f;

			const int32 ArcSteps = 10;
			TArray<FSlateVertex> SectorVerts;
			TArray<SlateIndex> SectorIndices;

			const FColor SectorBgFColor = (bIsSelected ? ActiveBg : DarkBg).ToFColor(true);
			const FLinearColor SectorBorderColor = bIsSelected ? Amber : (Cyan * 0.45f);

			for (int32 step = 0; step < ArcSteps; ++step)
			{
				float A0 = FMath::DegreesToRadians(FMath::Lerp(StartAngleDeg, EndAngleDeg, (float)step / ArcSteps));
				float A1 = FMath::DegreesToRadians(FMath::Lerp(StartAngleDeg, EndAngleDeg, (float)(step + 1) / ArcSteps));

				FVector2f S0_Out = Center + FVector2f(FMath::Sin(A0), -FMath::Cos(A0)) * (bIsSelected ? (OuterRadius + 14.0f) : OuterRadius);
				FVector2f S1_Out = Center + FVector2f(FMath::Sin(A1), -FMath::Cos(A1)) * (bIsSelected ? (OuterRadius + 14.0f) : OuterRadius);
				FVector2f S0_In  = Center + FVector2f(FMath::Sin(A0), -FMath::Cos(A0)) * InnerRadius;
				FVector2f S1_In  = Center + FVector2f(FMath::Sin(A1), -FMath::Cos(A1)) * InnerRadius;

				int32 BaseIdx = SectorVerts.Num();
				SectorVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, S0_Out, FVector2f(0.0f, 0.0f), SectorBgFColor));
				SectorVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, S1_Out, FVector2f(1.0f, 0.0f), SectorBgFColor));
				SectorVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, S1_In,  FVector2f(1.0f, 1.0f), SectorBgFColor));
				SectorVerts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RenderTransform, S0_In,  FVector2f(0.0f, 1.0f), SectorBgFColor));

				SectorIndices.Add(BaseIdx + 0); SectorIndices.Add(BaseIdx + 1); SectorIndices.Add(BaseIdx + 2);
				SectorIndices.Add(BaseIdx + 0); SectorIndices.Add(BaseIdx + 2); SectorIndices.Add(BaseIdx + 3);
			}

			if (SectorVerts.Num() > 0)
			{
				FSlateDrawElement::MakeCustomVerts(
					OutDrawElements,
					LayerId++,
					ResourceHandle,
					SectorVerts,
					SectorIndices,
					nullptr, 0, 0
				);
			}

			// Sector Perimeter Border
			float MidRad = FMath::DegreesToRadians(MidAngleDeg);
			float SRad_Start = FMath::DegreesToRadians(StartAngleDeg);
			float SRad_End   = FMath::DegreesToRadians(EndAngleDeg);

			FVector2f OutEdgeStart = Center + FVector2f(FMath::Sin(SRad_Start), -FMath::Cos(SRad_Start)) * (bIsSelected ? (OuterRadius + 14.0f) : OuterRadius);
			FVector2f OutEdgeEnd   = Center + FVector2f(FMath::Sin(SRad_End),   -FMath::Cos(SRad_End))   * (bIsSelected ? (OuterRadius + 14.0f) : OuterRadius);

			TArray<FVector2f> BorderPoints;
			BorderPoints.Add(OutEdgeStart);
			BorderPoints.Add(OutEdgeEnd);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), BorderPoints, ESlateDrawEffect::None, SectorBorderColor, true, bIsSelected ? 3.0f : 1.5f);

			// Sector Label Placement
			const float TextRadius = (InnerRadius + OuterRadius) * 0.5f + (bIsSelected ? 4.0f : 0.0f);
			const FVector2f TextPos = Center + FVector2f(FMath::Sin(MidRad), -FMath::Cos(MidRad)) * TextRadius;

			FSlateFontInfo FontBold = FCoreStyle::Get().GetFontStyle("Bold");
			FontBold.Size = bIsSelected ? 12 : 10;

			const FString SectorTitle = Entries[s].DisplayName.ToString();
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 2,
				AllottedGeometry.ToPaintGeometry(FVector2f(120.0f, 24.0f), FSlateLayoutTransform(TextPos - FVector2f(60.0f, 10.0f))),
				SectorTitle,
				FontBold,
				ESlateDrawEffect::None,
				bIsSelected ? Amber : FLinearColor(0.92f, 0.95f, 1.0f, 0.9f * Alpha)
			);
		}

		LayerId += 3;

		// 3. Center Hub Display (Showcases Selected Weapon Specs)
		if (Entries.IsValidIndex(HighlightedIdx))
		{
			const FFPSWeaponWheelEntry& Sel = Entries[HighlightedIdx];

			// Center Glowing Reticle
			FSlateFontInfo CenterTitleFont = FCoreStyle::Get().GetFontStyle("Bold");
			CenterTitleFont.Size = 14;

			FSlateFontInfo CenterSubFont = FCoreStyle::Get().GetFontStyle("Regular");
			CenterSubFont.Size = 9;

			// 3D Gun Model Holographic Viewport Box in Modal Hub
			const FVector2f ModelBoxSize(170.0f, 85.0f);
			const FVector2f ModelBoxTopLeft = Center + FVector2f(-ModelBoxSize.X * 0.5f, -74.0f);
			
			// Gun Viewport Background Card
			static FSlateBrush CardBrush;
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId++,
				AllottedGeometry.ToPaintGeometry(ModelBoxSize, FSlateLayoutTransform(ModelBoxTopLeft)),
				&CardBrush,
				ESlateDrawEffect::None,
				FLinearColor(0.015f, 0.035f, 0.055f, 0.75f * Alpha)
			);

			// Gun Viewport Tactical Cyan Corner Brackets
			TArray<FVector2f> BoxCorners;
			BoxCorners.Add(ModelBoxTopLeft);
			BoxCorners.Add(ModelBoxTopLeft + FVector2f(ModelBoxSize.X, 0.0f));
			BoxCorners.Add(ModelBoxTopLeft + ModelBoxSize);
			BoxCorners.Add(ModelBoxTopLeft + FVector2f(0.0f, ModelBoxSize.Y));
			BoxCorners.Add(ModelBoxTopLeft);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), BoxCorners, ESlateDrawEffect::None, Cyan * 0.5f, true, 1.0f);

			// 3D Weapon Model Image
			if (Sel.WeaponIcon)
			{
				FSlateBrush ModelBrush;
				ModelBrush.SetResourceObject(Sel.WeaponIcon);
				ModelBrush.ImageSize = FVector2D(ModelBoxSize.X, ModelBoxSize.Y);
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId++,
					AllottedGeometry.ToPaintGeometry(ModelBoxSize, FSlateLayoutTransform(ModelBoxTopLeft)),
					&ModelBrush,
					ESlateDrawEffect::None,
					FLinearColor(1.0f, 1.0f, 1.0f, Alpha)
				);
			}

			// Header Category
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(180.0f, 18.0f), FSlateLayoutTransform(Center + FVector2f(-90.0f, 14.0f))),
				Sel.CategoryName.ToString().ToUpper(),
				CenterSubFont,
				ESlateDrawEffect::None,
				Amber
			);

			// Weapon Name
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(190.0f, 22.0f), FSlateLayoutTransform(Center + FVector2f(-95.0f, 28.0f))),
				Sel.DisplayName.ToString(),
				CenterTitleFont,
				ESlateDrawEffect::None,
				FLinearColor(1.0f, 1.0f, 1.0f, Alpha)
			);

			// Caliber / Ammo Spec
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(180.0f, 16.0f), FSlateLayoutTransform(Center + FVector2f(-90.0f, 50.0f))),
				Sel.Caliber,
				CenterSubFont,
				ESlateDrawEffect::None,
				Cyan
			);

			// Selection Hint
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(180.0f, 16.0f), FSlateLayoutTransform(Center + FVector2f(-90.0f, 68.0f))),
				TEXT("[RELEASE KEY TO EQUIP]"),
				CenterSubFont,
				ESlateDrawEffect::None,
				MutedText
			);
		}

		// 4. Directional Gesture Pointer
		if (!OwningWidget->DirectionalVector.IsNearlyZero(0.1f))
		{
			FVector2f Dir = FVector2f(OwningWidget->DirectionalVector.X, OwningWidget->DirectionalVector.Y).GetSafeNormal();
			FVector2f ArrowTip = Center + Dir * (InnerRadius - 16.0f);
			FVector2f ArrowBase1 = Center + Dir * 20.0f + FVector2f(-Dir.Y, Dir.X) * 8.0f;
			FVector2f ArrowBase2 = Center + Dir * 20.0f + FVector2f(Dir.Y, -Dir.X) * 8.0f;

			TArray<FVector2f> ArrowLines;
			ArrowLines.Add(ArrowBase1);
			ArrowLines.Add(ArrowTip);
			ArrowLines.Add(ArrowBase2);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(), ArrowLines, ESlateDrawEffect::None, Amber, true, 2.5f);
		}

		return LayerId + 5;
	}

private:
	UFPSWeaponWheelWidget* OwningWidget = nullptr;
};

UFPSWeaponWheelWidget::UFPSWeaponWheelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
	PopulateArsenalEntries();
}

void UFPSWeaponWheelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PopulateArsenalEntries();
}

TSharedRef<SWidget> UFPSWeaponWheelWidget::RebuildWidget()
{
	return SNew(SFPSWeaponWheelCanvas)
		.OwningWidget(this);
}

void UFPSWeaponWheelWidget::PopulateArsenalEntries()
{
	ArsenalEntries.Empty();

	auto AddEntry = [this](const FString& ID, const FString& Name, const FString& Cat, const FString& Cal, EFPSWeaponArchetype Arch, const FString& Path, int32 SlotNum)
	{
		FFPSWeaponWheelEntry E;
		E.WeaponID = ID;
		E.DisplayName = FText::FromString(Name);
		E.CategoryName = FText::FromString(Cat);
		E.Caliber = Cal;
		E.Archetype = Arch;
		E.AssetPath = Path;
		E.SlotIndex = SlotNum;

		// Load Weapon Data Asset to retrieve Gun Model Icon
		UFPSWeaponDataAsset* LoadedAsset = LoadObject<UFPSWeaponDataAsset>(nullptr, *Path);
		if (LoadedAsset && LoadedAsset->WeaponIcon)
		{
			E.WeaponIcon = LoadedAsset->WeaponIcon;
		}
		else
		{
			// Fallback to direct Texture2D thumbnail asset
			FString TexPath = FString::Printf(TEXT("/Game/Weapons/Thumbnails/%s.%s"), *ID, *ID);
			E.WeaponIcon = LoadObject<UTexture2D>(nullptr, *TexPath);
		}

		ArsenalEntries.Add(E);
	};

	AddEntry(TEXT("W01_NF98"),       TEXT("NF-98 Carbine"),      TEXT("Assault Rifle"), TEXT("5.56x45mm NATO"),    EFPSWeaponArchetype::AssaultRifle, TEXT("/Game/Weapons/Data/DA_W01_NF98.DA_W01_NF98"), 0);
	AddEntry(TEXT("W02_VK74"),       TEXT("VK-74 Vektor"),       TEXT("Assault Rifle"), TEXT("7.62x39mm Soviet"),  EFPSWeaponArchetype::AssaultRifle, TEXT("/Game/Weapons/Data/DA_W02_VK74.DA_W02_VK74"), 1);
	AddEntry(TEXT("W03_Falcon50"),   TEXT("Falcon-50 DMR"),      TEXT("Marksman Rifle"),TEXT("7.62x51mm NATO"),    EFPSWeaponArchetype::BattleRifle,  TEXT("/Game/Weapons/Data/DA_W03_Falcon50.DA_W03_Falcon50"), 2);
	AddEntry(TEXT("W04_Breacher12"), TEXT("Breacher-12"),        TEXT("Combat Shotgun"),TEXT("12-Gauge Buckshot"), EFPSWeaponArchetype::Shotgun,      TEXT("/Game/Weapons/Data/DA_W04_Breacher12.DA_W04_Breacher12"), 3);
	AddEntry(TEXT("W05_Apex9"),      TEXT("Apex-9"),             TEXT("Tactical Pistol"),TEXT("9x19mm Parabellum"),EFPSWeaponArchetype::Pistol,       TEXT("/Game/Weapons/Data/DA_W05_Apex9.DA_W05_Apex9"), 4);
	AddEntry(TEXT("W06_Spectre45"),  TEXT("Spectre-45"),         TEXT("Heavy Sidearm"), TEXT(".45 ACP Tactical"),  EFPSWeaponArchetype::Pistol,       TEXT("/Game/Weapons/Data/DA_W06_Spectre45.DA_W06_Spectre45"), 5);
	AddEntry(TEXT("W07_Kestrel357"), TEXT("Kestrel-357"),        TEXT("Magnum Revolver"),TEXT(".357 Magnum"),      EFPSWeaponArchetype::Pistol,       TEXT("/Game/Weapons/Data/DA_W07_Kestrel357.DA_W07_Kestrel357"), 6);
	AddEntry(TEXT("W08_M67"),        TEXT("M67 Frag"),           TEXT("Lethal Grenade"),TEXT("High Explosive"),    EFPSWeaponArchetype::Throwable,    TEXT("/Game/Weapons/Data/DA_W08_GrenadeM67.DA_W08_GrenadeM67"), 7);
	AddEntry(TEXT("W09_Stun9"),      TEXT("Stun-9 Flashbang"),   TEXT("Tactical Stun"), TEXT("Pyrotechnic Flash"), EFPSWeaponArchetype::Throwable,    TEXT("/Game/Weapons/Data/DA_W09_Stun9.DA_W09_Stun9"), 8);
	AddEntry(TEXT("W10_Machete"),    TEXT("Favela Bolo Machete"),TEXT("Heavy Melee"),   TEXT("Carbon Steel"),      EFPSWeaponArchetype::Melee,        TEXT("/Game/Weapons/Data/DA_W10_Machete.DA_W10_Machete"), 9);
	AddEntry(TEXT("W11_Stiletto"),   TEXT("Shadow Stiletto"),    TEXT("Combat Blade"),  TEXT("440C Stainless"),    EFPSWeaponArchetype::Melee,        TEXT("/Game/Weapons/Data/DA_W11_Stiletto.DA_W11_Stiletto"), 10);
}

void UFPSWeaponWheelWidget::SetOwningCharacter(AFPSCharacter* InCharacter)
{
	OwningCharacter = InCharacter;
}

void UFPSWeaponWheelWidget::ShowWeaponWheel()
{
	bIsOpen = true;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// Sync current active weapon as starting highlighted index
	if (OwningCharacter.IsValid())
	{
		AFPSWeaponBase* CurWep = OwningCharacter->GetCurrentWeapon();
		if (CurWep && CurWep->WeaponDataAsset)
		{
			for (int32 i = 0; i < ArsenalEntries.Num(); ++i)
			{
				if (ArsenalEntries[i].WeaponID == CurWep->WeaponDataAsset->WeaponID)
				{
					HighlightedIndex = i;
					break;
				}
			}
		}
	}
}

void UFPSWeaponWheelWidget::HideAndSelectWeapon()
{
	if (!bIsOpen) return;
	bIsOpen = false;

	// Execute Weapon Switch in Player Inventory
	if (OwningCharacter.IsValid() && ArsenalEntries.IsValidIndex(HighlightedIndex))
	{
		const FFPSWeaponWheelEntry& SelectedEntry = ArsenalEntries[HighlightedIndex];

		if (UFPSInventoryComponent* Inv = OwningCharacter->GetInventoryComponent())
		{
			// Load Weapon Data Asset
			UFPSWeaponDataAsset* LoadedAsset = LoadObject<UFPSWeaponDataAsset>(nullptr, *SelectedEntry.AssetPath);
			if (LoadedAsset)
			{
				AFPSWeaponBase* ActiveWep = Inv->GetActiveWeapon();
				if (ActiveWep)
				{
					ActiveWep->ApplyWeaponDataAsset(LoadedAsset);
				}
			}
		}
	}
}

void UFPSWeaponWheelWidget::SetDirectionalInput(FVector2D Direction)
{
	DirectionalVector = Direction;
	if (Direction.IsNearlyZero(0.15f)) return;

	// Direction.X = Right/Left (+1 to -1)
	// Direction.Y = Up/Down (+1 for Up, -1 for Down)
	// Compute sector angle: 0 deg = North (Up), 90 deg = East (Right)
	float AngleRad = FMath::Atan2(Direction.X, Direction.Y);
	float AngleDeg = FMath::RadiansToDegrees(AngleRad);
	if (AngleDeg < 0.0f) AngleDeg += 360.0f;

	const int32 NumSectors = ArsenalEntries.Num();
	if (NumSectors > 0)
	{
		const float SectorSize = 360.0f / NumSectors;
		int32 SectorIdx = FMath::RoundToInt(AngleDeg / SectorSize) % NumSectors;
		SetHighlightedIndex(SectorIdx);
	}
}

void UFPSWeaponWheelWidget::SetHighlightedIndex(int32 NewIndex)
{
	if (ArsenalEntries.IsValidIndex(NewIndex) && HighlightedIndex != NewIndex)
	{
		HighlightedIndex = NewIndex;
		// Play tactical tick sound
		if (OwningCharacter.IsValid())
		{
			UGameplayStatics::PlaySound2D(OwningCharacter.Get(), LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Weapons/Reloads/A_Weapon_DryFire_Click.A_Weapon_DryFire_Click")), 0.6f, 1.25f);
		}
	}
}

void UFPSWeaponWheelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Smooth open/close alpha interpolation
	const float TargetAlpha = bIsOpen ? 1.0f : 0.0f;
	OpenAnimationAlpha = FMath::FInterpTo(OpenAnimationAlpha, TargetAlpha, InDeltaTime, 16.0f);

	if (!bIsOpen && OpenAnimationAlpha <= 0.01f)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// Track Mouse Movement if active
	if (bIsOpen && OwningCharacter.IsValid())
	{
		APlayerController* PC = Cast<APlayerController>(OwningCharacter->GetController());
		if (PC)
		{
			float MouseX = 0.0f;
			float MouseY = 0.0f;
			PC->GetInputMouseDelta(MouseX, MouseY);

			if (FMath::Abs(MouseX) > 0.05f || FMath::Abs(MouseY) > 0.05f)
			{
				RawMousePos += FVector2D(MouseX, -MouseY) * 2.5f;
				if (RawMousePos.Size() > 180.0f)
				{
					RawMousePos = RawMousePos.GetSafeNormal() * 180.0f;
				}

				if (RawMousePos.Size() > 25.0f)
				{
					SetDirectionalInput(RawMousePos.GetSafeNormal());
				}
			}
		}
	}
}
