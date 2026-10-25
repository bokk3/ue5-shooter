import unreal

sm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_Soldier_Posed")
if sm:
    unreal.log_warning(f"[MESH_DIRECT] get_num_lods={sm.get_num_lods()}, get_num_sections(0)={sm.get_num_sections(0)}")
    for i in range(10):
        mat = sm.get_material(i)
        if mat:
            unreal.log_warning(f"  Slot {i}: {mat.get_name()}")
        else:
            break
