with open('Source/NobleFermi/Weapons/FPSWeaponDataAsset.h', 'r', encoding='utf-8') as f:
    text = f.read()

target = '''\tUPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
\tFString Caliber = TEXT("5.56x45mm NATO");

\t// 3D Modular Meshes (Discrete Receiver & Detachable Magazine)'''

replacement = '''\tUPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
\tFString Caliber = TEXT("5.56x45mm NATO");

\t// 2D Tactical Preview Icon / Model Render for UI and Weapon Wheel Modal
\tUPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Identity")
\tTObjectPtr<class UTexture2D> WeaponIcon = nullptr;

\t// 3D Modular Meshes (Discrete Receiver & Detachable Magazine)'''

if target in text:
    text = text.replace(target, replacement, 1)
    with open('Source/NobleFermi/Weapons/FPSWeaponDataAsset.h', 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(text)
    print('Updated FPSWeaponDataAsset.h successfully')
else:
    print('Target not found')
