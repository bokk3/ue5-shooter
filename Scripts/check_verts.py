import unreal

print(">>> CHECKING VERTEX & TRIANGLE COUNTS <<<")
for path in [
    "/Game/Characters/BaseMale/SM_Soldier_Posed",
    "/Game/Environment/Hangar/SM_Hangar_Plinth",
    "/Game/Environment/Hangar/SM_Hangar_Floor"
]:
    sm = unreal.EditorAssetLibrary.load_asset(path)
    if sm:
        num_lods = sm.get_num_lods()
        num_sections = sm.get_num_sections(0)
        print(f"Mesh: {path}")
        print(f"  Num LODs: {num_lods}, Num Sections in LOD 0: {num_sections}")
        for s in range(num_sections):
            mat = sm.get_material(s)
            print(f"    Section {s}: Material={mat.get_name() if mat else 'NONE'}")
    else:
        print(f"Failed to load: {path}")

print(">>> CHECK COMPLETE <<<")
