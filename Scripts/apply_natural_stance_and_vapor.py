import os

# 1. Update FPSCharacterPreviewActor.h
path_preview_h = "Source/NobleFermi/Character/FPSCharacterPreviewActor.h"
with open(path_preview_h, "r", encoding="utf-8") as f:
    content = f.read()

if "UFPSBreathVaporComponent" not in content:
    content = content.replace(
        "class UFPSBeltGearComponent;",
        "class UFPSBeltGearComponent;\nclass UFPSBreathVaporComponent;"
    )
    content = content.replace(
        "TObjectPtr<UFPSBeltGearComponent> BeltGearComponent;",
        "TObjectPtr<UFPSBeltGearComponent> BeltGearComponent;\n\n\t// Procedural Cold Breath Vapor Component\n\tUPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"FPS Preview|Effects\")\n\tTObjectPtr<UFPSBreathVaporComponent> BreathVaporComponent;"
    )
    with open(path_preview_h, "w", encoding="utf-8") as f:
        f.write(content)
    print("Updated FPSCharacterPreviewActor.h")
else:
    print("FPSCharacterPreviewActor.h already updated")

# 2. Update FPSCharacterPreviewActor.cpp
path_preview_cpp = "Source/NobleFermi/Character/FPSCharacterPreviewActor.cpp"
with open(path_preview_cpp, "r", encoding="utf-8") as f:
    content = f.read()

if "FPSBreathVaporComponent.h" not in content:
    content = content.replace(
        '#include "Character/FPSBeltGearComponent.h"',
        '#include "Character/FPSBeltGearComponent.h"\n#include "Character/FPSBreathVaporComponent.h"'
    )
    content = content.replace(
        'BeltGearComponent = CreateDefaultSubobject<UFPSBeltGearComponent>(TEXT("BeltGearComponent"));',
        'BeltGearComponent = CreateDefaultSubobject<UFPSBeltGearComponent>(TEXT("BeltGearComponent"));\n\tBreathVaporComponent = CreateDefaultSubobject<UFPSBreathVaporComponent>(TEXT("BreathVaporComponent"));'
    )
    content = content.replace(
        'BeltGearComponent->InitializeBeltKit(BodyMesh);\n\t\t\t}',
        'BeltGearComponent->InitializeBeltKit(BodyMesh);\n\t\t\t}\n\t\t\tif (BreathVaporComponent)\n\t\t\t{\n\t\t\t\tBreathVaporComponent->InitializeBreathVapor(BodyMesh);\n\t\t\t}'
    )
    with open(path_preview_cpp, "w", encoding="utf-8") as f:
        f.write(content)
    print("Updated FPSCharacterPreviewActor.cpp")
else:
    print("FPSCharacterPreviewActor.cpp already updated")

# 3. Update FPSCharacter.h
path_char_h = "Source/NobleFermi/Character/FPSCharacter.h"
with open(path_char_h, "r", encoding="utf-8") as f:
    content = f.read()

if "UFPSBreathVaporComponent" not in content:
    content = content.replace(
        "class UFPSBeltGearComponent* GetBeltGearComponent() const { return BeltGearComponent; }",
        "class UFPSBeltGearComponent* GetBeltGearComponent() const { return BeltGearComponent; }\n\n\tUFUNCTION(BlueprintPure, Category = \"FPS Character|Effects\")\n\tclass UFPSBreathVaporComponent* GetBreathVaporComponent() const { return BreathVaporComponent; }"
    )
    content = content.replace(
        "TObjectPtr<class UFPSBeltGearComponent> BeltGearComponent;",
        "TObjectPtr<class UFPSBeltGearComponent> BeltGearComponent;\n\n\tUPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = \"FPS Character|Effects\")\n\tTObjectPtr<class UFPSBreathVaporComponent> BreathVaporComponent;"
    )
    with open(path_char_h, "w", encoding="utf-8") as f:
        f.write(content)
    print("Updated FPSCharacter.h")
else:
    print("FPSCharacter.h already updated")

# 4. Update FPSCharacter.cpp
path_char_cpp = "Source/NobleFermi/Character/FPSCharacter.cpp"
with open(path_char_cpp, "r", encoding="utf-8") as f:
    content = f.read()

if "FPSBreathVaporComponent.h" not in content:
    content = content.replace(
        '#include "Character/FPSBeltGearComponent.h"',
        '#include "Character/FPSBeltGearComponent.h"\n#include "Character/FPSBreathVaporComponent.h"'
    )
    content = content.replace(
        'BeltGearComponent = CreateDefaultSubobject<UFPSBeltGearComponent>(TEXT("BeltGearComponent"));',
        'BeltGearComponent = CreateDefaultSubobject<UFPSBeltGearComponent>(TEXT("BeltGearComponent"));\n\tBreathVaporComponent = CreateDefaultSubobject<UFPSBreathVaporComponent>(TEXT("BreathVaporComponent"));'
    )
    content = content.replace(
        'BeltGearComponent->InitializeBeltKit(GetMesh());\n\t\t}',
        'BeltGearComponent->InitializeBeltKit(GetMesh());\n\t\t}\n\t\tif (BreathVaporComponent)\n\t\t{\n\t\t\tBreathVaporComponent->InitializeBreathVapor(GetMesh());\n\t\t}'
    )
    with open(path_char_cpp, "w", encoding="utf-8") as f:
        f.write(content)
    print("Updated FPSCharacter.cpp")
else:
    print("FPSCharacter.cpp already updated")
