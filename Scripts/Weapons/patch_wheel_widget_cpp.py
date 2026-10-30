with open('Source/NobleFermi/UI/FPSWeaponWheelWidget.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Update center Slate rendering to show the weapon model preview box and weapon model render
old_center_draw = '''\t\t\t// Header Category
\t\t\tFSlateDrawElement::MakeText(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId,
\t\t\t\tAllottedGeometry.ToPaintGeometry(FVector2f(160.0f, 20.0f), FSlateLayoutTransform(Center + FVector2f(-80.0f, -42.0f))),
\t\t\t\tSel.CategoryName.ToString().ToUpper(),
\t\t\t\tCenterSubFont,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tAmber
\t\t\t);

\t\t\t// Weapon Name
\t\t\tFSlateDrawElement::MakeText(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId,
\t\t\t\tAllottedGeometry.ToPaintGeometry(FVector2f(180.0f, 24.0f), FSlateLayoutTransform(Center + FVector2f(-90.0f, -22.0f))),
\t\t\t\tSel.DisplayName.ToString(),
\t\t\t\tCenterTitleFont,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tFLinearColor(1.0f, 1.0f, 1.0f, Alpha)
\t\t\t);

\t\t\t// Caliber / Ammo Spec
\t\t\tFSlateDrawElement::MakeText(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId,
\t\t\t\tAllottedGeometry.ToPaintGeometry(FVector2f(160.0f, 18.0f), FSlateLayoutTransform(Center + FVector2f(-80.0f, 4.0f))),
\t\t\t\tSel.Caliber,
\t\t\t\tCenterSubFont,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tCyan
\t\t\t);

\t\t\t// Selection Hint
\t\t\tFSlateDrawElement::MakeText(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId,
\t\t\t\tAllottedGeometry.ToPaintGeometry(FVector2f(160.0f, 18.0f), FSlateLayoutTransform(Center + FVector2f(-80.0f, 24.0f))),
\t\t\t\tTEXT("[RELEASE KEY TO EQUIP]"),
\t\t\t\tCenterSubFont,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tMutedText
\t\t\t);'''

new_center_draw = '''\t\t\t// 3D Gun Model Holographic Viewport Box in Modal Hub
\t\t\tconst FVector2f ModelBoxSize(170.0f, 85.0f);
\t\t\tconst FVector2f ModelBoxTopLeft = Center + FVector2f(-ModelBoxSize.X * 0.5f, -74.0f);
\t\t\t
\t\t\t// Gun Viewport Background Card
\t\t\tFSlateDrawElement::MakeBox(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId++,
\t\t\t\tAllottedGeometry.ToPaintGeometry(ModelBoxSize, FSlateLayoutTransform(ModelBoxTopLeft)),
\t\t\t\t&Brush,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tFLinearColor(0.015f, 0.035f, 0.055f, 0.75f * Alpha)
\t\t\t);

\t\t\t// Gun Viewport Tactical Cyan Corner Brackets
\t\t\tTArray<FVector2f> BoxCorners;
\t\t\tBoxCorners.Add(ModelBoxTopLeft);
\t\t\tBoxCorners.Add(ModelBoxTopLeft + FVector2f(ModelBoxSize.X, 0.0f));
\t\t\tBoxCorners.Add(ModelBoxTopLeft + ModelBoxSize);
\t\t\tBoxCorners.Add(ModelBoxTopLeft + FVector2f(0.0f, ModelBoxSize.Y));
\t\t\tBoxCorners.Add(ModelBoxTopLeft);
\t\t\tFSlateDrawElement::MakeLines(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), BoxCorners, ESlateDrawEffect::None, Cyan * 0.5f, true, 1.0f);

\t\t\t// 3D Weapon Model Image
\t\t\tif (Sel.WeaponIcon)
\t\t\t{
\t\t\t\tFSlateBrush ModelBrush;
\t\t\t\tModelBrush.SetResourceObject(Sel.WeaponIcon);
\t\t\t\tModelBrush.ImageSize = FVector2D(ModelBoxSize.X, ModelBoxSize.Y);
\t\t\t\tFSlateDrawElement::MakeBox(
\t\t\t\t\tOutDrawElements,
\t\t\t\t\tLayerId++,
\t\t\t\t\tAllottedGeometry.ToPaintGeometry(ModelBoxSize, FSlateLayoutTransform(ModelBoxTopLeft)),
\t\t\t\t\t&ModelBrush,
\t\t\t\t\tESlateDrawEffect::None,
\t\t\t\t\tFLinearColor(1.0f, 1.0f, 1.0f, Alpha)
\t\t\t\t);
\t\t\t}

\t\t\t// Header Category
\t\t\tFSlateDrawElement::MakeText(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId,
\t\t\t\tAllottedGeometry.ToPaintGeometry(FVector2f(180.0f, 18.0f), FSlateLayoutTransform(Center + FVector2f(-90.0f, 14.0f))),
\t\t\t\tSel.CategoryName.ToString().ToUpper(),
\t\t\t\tCenterSubFont,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tAmber
\t\t\t);

\t\t\t// Weapon Name
\t\t\tFSlateDrawElement::MakeText(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId,
\t\t\t\tAllottedGeometry.ToPaintGeometry(FVector2f(190.0f, 22.0f), FSlateLayoutTransform(Center + FVector2f(-95.0f, 28.0f))),
\t\t\t\tSel.DisplayName.ToString(),
\t\t\t\tCenterTitleFont,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tFLinearColor(1.0f, 1.0f, 1.0f, Alpha)
\t\t\t);

\t\t\t// Caliber / Ammo Spec
\t\t\tFSlateDrawElement::MakeText(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId,
\t\t\t\tAllottedGeometry.ToPaintGeometry(FVector2f(180.0f, 16.0f), FSlateLayoutTransform(Center + FVector2f(-90.0f, 50.0f))),
\t\t\t\tSel.Caliber,
\t\t\t\tCenterSubFont,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tCyan
\t\t\t);

\t\t\t// Selection Hint
\t\t\tFSlateDrawElement::MakeText(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId,
\t\t\t\tAllottedGeometry.ToPaintGeometry(FVector2f(180.0f, 16.0f), FSlateLayoutTransform(Center + FVector2f(-90.0f, 68.0f))),
\t\t\t\tTEXT("[RELEASE KEY TO EQUIP]"),
\t\t\t\tCenterSubFont,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tMutedText
\t\t\t);'''

if old_center_draw in text:
    text = text.replace(old_center_draw, new_center_draw, 1)
    print("Replaced center draw successfully")
else:
    print("Old center draw not found")

# 2. Update PopulateArsenalEntries to load WeaponIcon from data asset or direct path
old_pop = '''\tauto AddEntry = [this](const FString& ID, const FString& Name, const FString& Cat, const FString& Cal, EFPSWeaponArchetype Arch, const FString& Path, int32 SlotNum)
\t{
\t\tFFPSWeaponWheelEntry E;
\t\tE.WeaponID = ID;
\t\tE.DisplayName = FText::FromString(Name);
\t\tE.CategoryName = FText::FromString(Cat);
\t\tE.Caliber = Cal;
\t\tE.Archetype = Arch;
\t\tE.AssetPath = Path;
\t\tE.SlotIndex = SlotNum;
\t\tArsenalEntries.Add(E);
\t};'''

new_pop = '''\tauto AddEntry = [this](const FString& ID, const FString& Name, const FString& Cat, const FString& Cal, EFPSWeaponArchetype Arch, const FString& Path, int32 SlotNum)
\t{
\t\tFFPSWeaponWheelEntry E;
\t\tE.WeaponID = ID;
\t\tE.DisplayName = FText::FromString(Name);
\t\tE.CategoryName = FText::FromString(Cat);
\t\tE.Caliber = Cal;
\t\tE.Archetype = Arch;
\t\tE.AssetPath = Path;
\t\tE.SlotIndex = SlotNum;

\t\t// Load Weapon Data Asset to retrieve Gun Model Icon
\t\tUFPSWeaponDataAsset* LoadedAsset = LoadObject<UFPSWeaponDataAsset>(nullptr, *Path);
\t\tif (LoadedAsset && LoadedAsset->WeaponIcon)
\t\t{
\t\t\tE.WeaponIcon = LoadedAsset->WeaponIcon;
\t\t}
\t\telse
\t\t{
\t\t\t// Fallback to direct Texture2D thumbnail asset
\t\t\tFString TexPath = FString::Printf(TEXT("/Game/Weapons/Thumbnails/%s.%s"), *ID, *ID);
\t\t\tE.WeaponIcon = LoadObject<UTexture2D>(nullptr, *TexPath);
\t\t}

\t\tArsenalEntries.Add(E);
\t};'''

if old_pop in text:
    text = text.replace(old_pop, new_pop, 1)
    print("Replaced populate entries successfully")
else:
    print("Old populate entries not found")

with open('Source/NobleFermi/UI/FPSWeaponWheelWidget.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
    f.write(text)
