import unreal

sm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_Soldier_Posed")
if sm:
    print(f"Asset: {sm.get_name()}")
    num_lods = sm.get_num_lods()
    print(f"Num LODs: {num_lods}")
    for lod in range(num_lods):
        num_sections = sm.get_num_sections(lod)
        print(f"  LOD {lod}: {num_sections} sections")
        for s in range(num_sections):
            mat = sm.get_material(s)
            mat_name = mat.get_name() if mat else "NONE"
            print(f"    Section {s}: Material = {mat_name}")
    bounds = sm.get_bounds()
    print(f"Bounds: Origin={bounds.origin}, Extent={bounds.box_extent}, Radius={bounds.sphere_radius}")
else:
    print("Failed to load SM_Soldier_Posed")
