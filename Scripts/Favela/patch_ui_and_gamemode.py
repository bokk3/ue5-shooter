import os

source_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Source\NobleFermi"

# 1. Patch FPSMultiplayerGameMode.cpp
gm_cpp = os.path.join(source_dir, "Multiplayer", "FPSMultiplayerGameMode.cpp")
with open(gm_cpp, "r", encoding="utf-8") as f:
    gm_txt = f.read()

old_theater_parse = """\tif (TheaterOption.Equals(TEXT("Coastal"), ESearchCase::IgnoreCase))
\t{
\t\tActiveTheaterIndex = 1;
\t}
\telse if (TheaterOption.Equals(TEXT("Rust"), ESearchCase::IgnoreCase))
\t{
\t\tActiveTheaterIndex = 0;
\t}"""

new_theater_parse = """\tif (TheaterOption.Equals(TEXT("Coastal"), ESearchCase::IgnoreCase) ||
\t\tTheaterOption.Equals(TEXT("BaiaBrava"), ESearchCase::IgnoreCase) ||
\t\tTheaterOption.Equals(TEXT("Baia"), ESearchCase::IgnoreCase))
\t{
\t\tActiveTheaterIndex = 1;
\t}
\telse if (TheaterOption.Equals(TEXT("Rust"), ESearchCase::IgnoreCase) ||
\t\tTheaterOption.Equals(TEXT("RochaVelha"), ESearchCase::IgnoreCase) ||
\t\tTheaterOption.Equals(TEXT("Favela"), ESearchCase::IgnoreCase))
\t{
\t\tActiveTheaterIndex = 0;
\t}"""

assert old_theater_parse in gm_txt, "old_theater_parse not found"
gm_txt = gm_txt.replace(old_theater_parse, new_theater_parse, 1)

gm_txt = gm_txt.replace(
    'ActiveTheaterIndex == 1 ? TEXT("Coastal Operations") : TEXT("Rust Tactical")',
    'ActiveTheaterIndex == 1 ? TEXT("Favela // Baía Brava") : TEXT("Favela // Rocha Velha")'
)

with open(gm_cpp, "w", encoding="utf-8", newline="\r\n") as f:
    f.write(gm_txt.replace("\r\n", "\n"))
print("Updated FPSMultiplayerGameMode.cpp")

# 2. Patch FPSHUDWidget.cpp
hud_cpp = os.path.join(source_dir, "UI", "FPSHUDWidget.cpp")
with open(hud_cpp, "r", encoding="utf-8") as f:
    hud_txt = f.read()

old_hud_theater = """\t\t// Detect Active Theater (0 = Rust Tactical, 1 = Coastal Operations)
\t\tint32 ActiveTheaterIdx = 0;
\t\tif (UWorld* HudWorld = OwningHUD->GetWorld())
\t\t{
\t\t\tif (UGameInstance* GI = HudWorld->GetGameInstance())
\t\t\t{
\t\t\t\tif (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
\t\t\t\t{
\t\t\t\t\tActiveTheaterIdx = Economy->GetSelectedTheaterIndex();
\t\t\t\t}
\t\t\t}
\t\t}
\t\tconst bool bIsCoastalOps = (ActiveTheaterIdx == 1);"""

new_hud_theater = """\t\t// Detect Active Theater (0 = Favela // Rocha Velha, 1 = Favela // Baía Brava)
\t\tint32 ActiveTheaterIdx = 0;
\t\tif (UWorld* HudWorld = OwningHUD->GetWorld())
\t\t{
\t\t\tif (UGameplayStatics::GetActorOfClass(HudWorld, AFPSCoastalOperationsMapActor::StaticClass()) != nullptr)
\t\t\t{
\t\t\t\tActiveTheaterIdx = 1;
\t\t\t}
\t\t\telse if (UGameInstance* GI = HudWorld->GetGameInstance())
\t\t\t{
\t\t\t\tif (UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>())
\t\t\t\t{
\t\t\t\t\tActiveTheaterIdx = Economy->GetSelectedTheaterIndex();
\t\t\t\t}
\t\t\t}
\t\t}
\t\tconst bool bIsCoastalOps = (ActiveTheaterIdx == 1);"""

assert old_hud_theater in hud_txt, "old_hud_theater not found"
hud_txt = hud_txt.replace(old_hud_theater, new_hud_theater, 1)

old_hud_callout = 'bIsCoastalOps ? TEXT("COASTAL OPS") : TEXT("RUST TACTICAL")'
new_hud_callout = 'bIsCoastalOps ? TEXT("FAVELA // BAÍA BRAVA") : TEXT("FAVELA // ROCHA VELHA")'
assert old_hud_callout in hud_txt, "old_hud_callout not found"
hud_txt = hud_txt.replace(old_hud_callout, new_hud_callout, 1)

# Ensure #include "Map/FPSCoastalOperationsMapActor.h" is present in FPSHUDWidget.cpp
if '#include "Map/FPSCoastalOperationsMapActor.h"' not in hud_txt:
    hud_txt = hud_txt.replace('#include "UI/FPSHUDWidget.h"', '#include "UI/FPSHUDWidget.h"\n#include "Map/FPSCoastalOperationsMapActor.h"', 1)

with open(hud_cpp, "w", encoding="utf-8", newline="\r\n") as f:
    f.write(hud_txt.replace("\r\n", "\n"))
print("Updated FPSHUDWidget.cpp")

# 3. Patch MainMenuWidget.cpp
menu_cpp = os.path.join(source_dir, "UI", "MainMenuWidget.cpp")
with open(menu_cpp, "r", encoding="utf-8") as f:
    menu_txt = f.read()

menu_txt = menu_txt.replace(
    'TEXT("OPERATION: COASTAL OPERATIONS")\n\t\t\t\t\t\t\t\t\t: TEXT("OPERATION: RUST TACTICAL")',
    'TEXT("FAVELA: BAÍA BRAVA")\n\t\t\t\t\t\t\t\t\t: TEXT("FAVELA: ROCHA VELHA")'
)

menu_txt = menu_txt.replace(
    'TEXT("OPERATION: COASTAL OPS")\n\t\t\t\t\t\t\t\t\t: TEXT("OPERATION: RUST TACTICAL")',
    'TEXT("FAVELA: BAÍA BRAVA")\n\t\t\t\t\t\t\t\t\t: TEXT("FAVELA: ROCHA VELHA")'
)

menu_txt = menu_txt.replace(
    'TEXT("North Sea Littoral / Dike & Pier")\n\t\t\t\t\t\t\t\t\t: TEXT("Industrial Combat / High Intensity")',
    'TEXT("120m Coastal Hillside & Waterfront Pier")\n\t\t\t\t\t\t\t\t\t: TEXT("60m Hillside Shanty Town & Laje Watchtower")'
)

menu_txt = menu_txt.replace(
    'const FString TheaterOpt = (SelectedTheaterIndex == 1) ? TEXT("Coastal") : TEXT("Rust");',
    'const FString TheaterOpt = (SelectedTheaterIndex == 1) ? TEXT("BaiaBrava") : TEXT("RochaVelha");'
)

menu_txt = menu_txt.replace(
    'Map: Coastal Operations',
    'Map: Favela Baía Brava'
)
menu_txt = menu_txt.replace(
    'Map: Rust Tactical',
    'Map: Favela Rocha Velha'
)

with open(menu_cpp, "w", encoding="utf-8", newline="\r\n") as f:
    f.write(menu_txt.replace("\r\n", "\n"))
print("Updated MainMenuWidget.cpp")
