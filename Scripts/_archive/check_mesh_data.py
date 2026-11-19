import unreal

sm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_Soldier_Posed")
if sm:
    num_lods = unreal.EditorStaticMeshLibrary.get_lod_count(sm)
    unreal.log_warning(f"[MESH_DATA] SM_Soldier_Posed LODs: {num_lods}")
    for lod in range(num_lods):
        num_sections = unreal.EditorStaticMeshLibrary.get_lod_section_count(sm, lod)
        unreal.log_warning(f"  LOD {lod} Sections: {num_sections}")
        for sec in range(num_sections):
            mat_idx = unreal.EditorStaticMeshLibrary.get_lod_section_material_index(sm, lod, sec)
            mat = sm.get_material(mat_idx)
            mat_name = mat.get_name() if mat else "NONE"
            unreal.log_warning(f"    Sec {sec}: MaterialIndex={mat_idx}, Material={mat_name}")
else:
    unreal.log_warning("[MESH_DATA] Failed to load SM_Soldier_Posed")
