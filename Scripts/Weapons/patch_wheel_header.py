with open('Source/NobleFermi/UI/FPSWeaponWheelWidget.h', 'r', encoding='utf-8') as f:
    text = f.read()

target = '''\tUPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
\tFString AssetPath;

\tUPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
\tint32 SlotIndex = 0;
};'''

replacement = '''\tUPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
\tFString AssetPath;

\tUPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
\tint32 SlotIndex = 0;

\tUPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Wheel")
\tTObjectPtr<class UTexture2D> WeaponIcon = nullptr;
};'''

if target in text:
    text = text.replace(target, replacement, 1)
    with open('Source/NobleFermi/UI/FPSWeaponWheelWidget.h', 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(text)
    print('Updated FPSWeaponWheelWidget.h successfully')
else:
    print('Target not found')
