import unreal

assets = [
    "/Game/Characters/BaseMale/SM_Soldier_Posed",
    "/Game/Environment/Hangar/SM_Hangar_Plinth",
    "/Game/Environment/Hangar/SM_Hangar_Floor",
    "/Game/Environment/Hangar/SM_Hangar_Wall"
]

for p in assets:
    m = unreal.EditorAssetLibrary.load_asset(p)
    if m:
        b = m.get_bounds()
        unreal.log_warning(f"[BOUNDS_CHECK] Asset={p}, Origin={b.origin}, Extent={b.box_extent}, Radius={b.sphere_radius}")
        # Also check materials
        for i in range(10):
            mat = m.get_material(i)
            if mat:
                unreal.log_warning(f"  Slot {i}: {mat.get_name()}")
            else:
                break
    else:
        unreal.log_warning(f"[BOUNDS_CHECK] Failed to load {p}")
