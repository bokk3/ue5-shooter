with open('Source/NobleFermi/UI/FPSWeaponWheelWidget.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

target = '''\t\t\t// Gun Viewport Background Card
\t\t\tFSlateDrawElement::MakeBox(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId++,
\t\t\t\tAllottedGeometry.ToPaintGeometry(ModelBoxSize, FSlateLayoutTransform(ModelBoxTopLeft)),
\t\t\t\t&Brush,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tFLinearColor(0.015f, 0.035f, 0.055f, 0.75f * Alpha)
\t\t\t);'''

replacement = '''\t\t\t// Gun Viewport Background Card
\t\t\tstatic FSlateBrush CardBrush;
\t\t\tFSlateDrawElement::MakeBox(
\t\t\t\tOutDrawElements,
\t\t\t\tLayerId++,
\t\t\t\tAllottedGeometry.ToPaintGeometry(ModelBoxSize, FSlateLayoutTransform(ModelBoxTopLeft)),
\t\t\t\t&CardBrush,
\t\t\t\tESlateDrawEffect::None,
\t\t\t\tFLinearColor(0.015f, 0.035f, 0.055f, 0.75f * Alpha)
\t\t\t);'''

if target in text:
    text = text.replace(target, replacement, 1)
    with open('Source/NobleFermi/UI/FPSWeaponWheelWidget.cpp', 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(text)
    print("Fixed CardBrush successfully")
else:
    print("Target not found")
