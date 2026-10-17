import unreal

unreal.log_warning("================= BAKING MATERIALS AND SOCKETS =================")

# 1. Setup Materials on SK_BaseMale
sk = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
op_mat = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody")

if sk and op_mat:
    mats = sk.get_editor_property("materials")
    unreal.log_warning(f"SK_BaseMale before: {len(mats)} materials, Slot 0 = {mats[0].get_editor_property('material_interface')}")
    
    # Properly assign material
    new_sk_mat = unreal.SkeletalMaterial()
    new_sk_mat.set_editor_property("material_interface", op_mat)
    new_sk_mat.set_editor_property("material_slot_name", "M_Operator_BaseBody")
    
    sk.set_editor_property("materials", [new_sk_mat])
    unreal.EditorAssetLibrary.save_loaded_asset(sk)
    
    # Verify reload
    sk_reloaded = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
    reloaded_mats = sk_reloaded.get_editor_property("materials")
    unreal.log_warning(f"SK_BaseMale after reload: Slot 0 = {reloaded_mats[0].get_editor_property('material_interface').get_path_name()}")
else:
    unreal.log_error("Could not load SK_BaseMale or M_Operator_BaseBody!")

# 2. Setup SM_AK47 Materials & Sockets
sm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_AK47")
mat_steel = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_AK47_Steel")
mat_wood = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_AK47_Wood")
mat_bake = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite")

if sm and mat_steel and mat_wood and mat_bake:
    sm.set_material(0, mat_steel)
    sm.set_material(1, mat_wood)
    sm.set_material(2, mat_bake)
    
    # Sockets definition
    desired_sockets = [
        ("weapon_l_ik", unreal.Vector(2.6, -17.5, -2.6), unreal.Rotator(0.0, 0.0, 0.0)),
        ("Muzzle_Flash_Socket", unreal.Vector(0.0, -52.0, 2.4), unreal.Rotator(0.0, -90.0, 0.0)),
        ("Ejection_Port_Socket", unreal.Vector(2.2, -4.0, 2.5), unreal.Rotator(0.0, 0.0, 0.0)),
    ]
    
    for sock_name, rel_loc, rel_rot in desired_sockets:
        existing_sock = sm.find_socket(sock_name)
        if existing_sock:
            unreal.log_warning(f"Found existing socket {sock_name}, removing to re-create with exact coordinates")
            sm.remove_socket(existing_sock)
            
        sock = unreal.StaticMeshSocket()
        sock.set_editor_property("socket_name", sock_name)
        sock.set_editor_property("relative_location", rel_loc)
        sock.set_editor_property("relative_rotation", rel_rot)
        sm.add_socket(sock)
        unreal.log_warning(f"Added socket {sock_name} at loc={rel_loc} rot={rel_rot}")
    
    unreal.EditorAssetLibrary.save_loaded_asset(sm)
    
    # Verify reload
    sm_reloaded = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_AK47")
    reloaded_sm_mats = sm_reloaded.get_editor_property("static_materials")
    for i, m in enumerate(reloaded_sm_mats):
        unreal.log_warning(f"SM_AK47 Slot {i}: {m.get_editor_property('material_interface').get_path_name()}")
    
    for sock_name, _, _ in desired_sockets:
        s = sm_reloaded.find_socket(sock_name)
        if s:
            unreal.log_warning(f"Verified SM_AK47 socket {sock_name} exists at {s.get_editor_property('relative_location')}")
        else:
            unreal.log_error(f"FAILED to find socket {sock_name} after reload!")
else:
    unreal.log_error("Could not load SM_AK47 or its materials!")

# 3. Check Texture References in M_Operator_BaseBody
tex_camo = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Textures/T_Camo_Digital")
tex_mask = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Textures/T_Operator_Body_Mask")
tex_norm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Textures/T_Ripstop_Normal")
tex_rough = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Textures/T_Ripstop_Roughness")

unreal.log_warning(f"Textures status: Camo={bool(tex_camo)}, Mask={bool(tex_mask)}, Norm={bool(tex_norm)}, Rough={bool(tex_rough)}")

# Save all assets in Characters directory
unreal.EditorAssetLibrary.save_directory("/Game/Characters/BaseMale", only_if_is_dirty=False, recursive=True)
unreal.log_warning("================= BAKING COMPLETED SUCCESSFULLY =================")
