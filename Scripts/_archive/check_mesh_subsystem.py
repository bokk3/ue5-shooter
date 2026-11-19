import unreal

sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
sm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_Soldier_Posed")
if sm:
    num_lods = sub.get_lod_count(sm)
    unreal.log_warning(f"[MESH_SUBSYS] SM_Soldier_Posed LODs: {num_lods}")
    num_verts = sub.get_number_verts(sm, 0)
    num_tris = sub.get_number_triangles(sm, 0)
    unreal.log_warning(f"  LOD 0: Vertices={num_verts}, Triangles={num_tris}")
    num_sections = sub.get_number_materials(sm)
    unreal.log_warning(f"  Materials Count={num_sections}")
    for i in range(num_sections):
        mat = sm.get_material(i)
        unreal.log_warning(f"    Slot {i}: {mat.get_name() if mat else 'NONE'}")
else:
    unreal.log_warning("[MESH_SUBSYS] Failed to load SM_Soldier_Posed")
