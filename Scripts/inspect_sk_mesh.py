import unreal
import os

out_path = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Intermediate\sk_inspect.txt"
lines = []

sk = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if sk:
    lines.append("========================================")
    lines.append("SK_BaseMale successfully loaded!")
    try:
        lines.append(f"Num LODs: {sk.get_num_lods()}")
    except Exception as e:
        lines.append(f"Num LODs error: {e}")
    bounds = sk.get_bounds()
    lines.append(f"Bounds: Origin={bounds.origin}, Extent={bounds.box_extent}, Radius={bounds.sphere_radius}")
    mats = sk.get_editor_property("materials")
    lines.append(f"Num materials: {len(mats)}")
    for i, m in enumerate(mats):
        lines.append(f"  Slot {i}: {m.material_slot_name} -> {m.material_interface.get_name() if m.material_interface else 'None'}")
    skeleton = sk.get_editor_property("skeleton")
    lines.append(f"Skeleton: {skeleton.get_name() if skeleton else 'None'}")
    lines.append("========================================")
else:
    lines.append("ERROR: Could not load /Game/Characters/BaseMale/SK_BaseMale!")

with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")

unreal.log_warning("\n".join(lines))
