"""List the Lyra Starter Game assets Noble Fermi uses: rifle, pistol and shotgun animations on the
UE5 Mannequin, hit reacts and deaths, the weapon meshes, and everything they hard-reference.

Run headless inside the Lyra project (see README.md). Writes JSON to $LYRA_DEPS_OUT.
Blueprints (Lyra's anim layer BPs etc.) are excluded: they depend on Lyra's C++ module.
"""
import json
import os

import unreal

OUT = os.environ.get("LYRA_DEPS_OUT", os.path.join(os.environ.get("TEMP", "."), "lyra_deps.json"))

ANIM_ROOT = "/Game/Characters/Heroes/Mannequin/Animations"
FOLDERS = [
    f"{ANIM_ROOT}/Locomotion/Rifle",
    f"{ANIM_ROOT}/Locomotion/Pistol",
    f"{ANIM_ROOT}/Locomotion/Shotgun",
    f"{ANIM_ROOT}/AimOffsets",
    f"{ANIM_ROOT}/Actions",
    "/Game/Weapons/Rifle",
    "/Game/Weapons/Pistol",
    "/Game/Weapons/Shotgun",
]
EXTRA = [
    "/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny",
    "/Game/Characters/Heroes/Mannequin/Rig/IK_Mannequin",
]
SKIP_CLASSES = {"AnimBlueprint", "Blueprint", "WidgetBlueprint"}

ar = unreal.AssetRegistryHelpers.get_asset_registry()
ar.wait_for_completion()

roots, skipped = [], []
for folder in FOLDERS:
    for data in ar.get_assets_by_path(folder, recursive=True):
        name = str(data.asset_name)
        cls = str(data.asset_class_path.asset_name)
        pkg = str(data.package_name)
        # Feminine (MF_) variants and emotes aren't needed: one operator body.
        if name.startswith(("MF_", "Mf_", "AO_MF_", "BS_MF_")) or "Emote" in name or "Feminine" in name:
            skipped.append(pkg)
            continue
        if cls in SKIP_CLASSES:
            skipped.append(pkg)
            continue
        roots.append(pkg)
roots += EXTRA

opts = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=False, include_hard_package_references=True,
    include_searchable_names=False, include_soft_management_references=False,
    include_hard_management_references=False)

seen, stack, script_deps = set(), list(roots), {}
while stack:
    p = stack.pop()
    if p in seen:
        continue
    seen.add(p)
    for d in ar.get_dependencies(p, opts) or []:
        d = str(d)
        if d.startswith("/Game/"):
            if d not in seen:
                stack.append(d)
        elif d.startswith("/Script/Lyra"):
            script_deps.setdefault(p, []).append(d)

classes = {}
for p in seen:
    for data in ar.get_assets_by_package_name(p):
        classes[p] = str(data.asset_class_path.asset_name)

json.dump({"roots": sorted(roots), "hard": sorted(seen), "skipped": sorted(set(skipped)),
           "lyra_script_deps": script_deps, "classes": classes}, open(OUT, "w"), indent=1)
print("LYRA_DEPS_DONE", len(roots), len(seen), "lyra-script:", len(script_deps))
