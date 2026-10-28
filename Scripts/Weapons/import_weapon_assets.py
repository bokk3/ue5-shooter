import unreal
import json
import os
import glob

def log(msg):
    unreal.log_warning(f"[WEAPON_IMPORT] {msg}")

log("================================================================================")
log("  NOBLE FERMI // ARSENAL IMPORT PIPELINE (FBX, SOCKETS, PBR MATS, DATA ASSETS) ")
log("================================================================================")

PROCESSED_DIR = r"C:\Users\Boris\Documents\antigravity\noble-fermi\SourceArt\Weapons\Processed"
MESH_DEST_PATH = "/Game/Weapons/Meshes"
MAT_DEST_PATH = "/Game/Weapons/Materials"
DATA_DEST_PATH = "/Game/Weapons/Data"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset_lib = unreal.EditorAssetLibrary

# ------------------------------------------------------------------------------
# STEP 1: CREATE MASTER WEAPON PBR MATERIAL & INSTANCES
# ------------------------------------------------------------------------------
def get_or_create_master_material():
    master_mat_path = f"{MAT_DEST_PATH}/M_Weapon_Master"
    mat = editor_asset_lib.load_asset(master_mat_path)
    if mat:
        log(f"Found existing master material: {master_mat_path}")
        return mat

    log(f"Creating Master Material: {master_mat_path}")
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Weapon_Master", MAT_DEST_PATH, unreal.Material, mat_factory)
    if not mat:
        log("FAILED to create M_Weapon_Master")
        return None

    mat_editing = unreal.MaterialEditingLibrary

    # Base Color Parameter
    base_col = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -200)
    base_col.set_editor_property("parameter_name", "BaseColor")
    base_col.set_editor_property("default_value", unreal.LinearColor(0.08, 0.08, 0.09, 1.0))
    mat_editing.connect_material_property(base_col, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Metallic Parameter
    metal_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 100)
    metal_node.set_editor_property("parameter_name", "Metallic")
    metal_node.set_editor_property("default_value", 0.85)
    mat_editing.connect_material_property(metal_node, "", unreal.MaterialProperty.MP_METALLIC)

    # Roughness Parameter
    rough_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 250)
    rough_node.set_editor_property("parameter_name", "Roughness")
    rough_node.set_editor_property("default_value", 0.35)
    mat_editing.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mat_editing.recompile_material(mat)
    editor_asset_lib.save_loaded_asset(mat, only_if_is_dirty=False)
    log("M_Weapon_Master created and compiled successfully!")
    return mat

def create_material_instances(master_mat):
    instance_specs = {
        "MI_Weapon_Gunmetal": {"col": unreal.LinearColor(0.06, 0.06, 0.07, 1.0), "metal": 0.85, "rough": 0.32},
        "MI_Weapon_Polymer": {"col": unreal.LinearColor(0.03, 0.03, 0.03, 1.0), "metal": 0.00, "rough": 0.70},
        "MI_Weapon_Steel": {"col": unreal.LinearColor(0.12, 0.12, 0.13, 1.0), "metal": 0.90, "rough": 0.25},
        "MI_Weapon_Blade": {"col": unreal.LinearColor(0.35, 0.38, 0.40, 1.0), "metal": 0.95, "rough": 0.18},
        "MI_Weapon_OliveDrab": {"col": unreal.LinearColor(0.10, 0.12, 0.08, 1.0), "metal": 0.10, "rough": 0.65},
        "MI_Weapon_Silver": {"col": unreal.LinearColor(0.70, 0.70, 0.72, 1.0), "metal": 0.88, "rough": 0.22},
    }

    created_instances = {}
    for name, params in instance_specs.items():
        inst_path = f"{MAT_DEST_PATH}/{name}"
        inst = editor_asset_lib.load_asset(inst_path)
        if not inst:
            mic_factory = unreal.MaterialInstanceConstantFactoryNew()
            inst = asset_tools.create_asset(name, MAT_DEST_PATH, unreal.MaterialInstanceConstant, mic_factory)
            if inst:
                inst.set_editor_property("parent", master_mat)
                unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(inst, "BaseColor", params["col"])
                unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(inst, "Metallic", params["metal"])
                unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(inst, "Roughness", params["rough"])
                editor_asset_lib.save_loaded_asset(inst, only_if_is_dirty=False)
                log(f"Created Material Instance: {name}")
        created_instances[name] = inst
    return created_instances

master_mat = get_or_create_master_material()
mat_instances = create_material_instances(master_mat)

# ------------------------------------------------------------------------------
# STEP 2: IMPORT ALL FBX MODELS (RECEIVERS & DETACHABLE MAGAZINES) INDIVIDUALLY
# ------------------------------------------------------------------------------
fbx_files = sorted(glob.glob(os.path.join(PROCESSED_DIR, "*.fbx")))
log(f"Found {len(fbx_files)} FBX models to import into {MESH_DEST_PATH}...")

for fbx_path in fbx_files:
    mesh_name = os.path.splitext(os.path.basename(fbx_path))[0]
    task = unreal.AssetImportTask()
    task.filename = os.path.abspath(fbx_path)
    task.destination_path = MESH_DEST_PATH
    task.destination_name = mesh_name
    task.replace_existing = True
    task.automated = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.automated_import_should_detect_type = False
    options.import_animations = False
    options.import_materials = False
    options.import_textures = False

    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.generate_lightmap_u_vs = True
    task.options = options
    
    try:
        asset_tools.import_asset_tasks([task])
        imported_mesh = editor_asset_lib.load_asset(f"{MESH_DEST_PATH}/{mesh_name}")
        if imported_mesh:
            b = imported_mesh.get_bounds()
            log(f"  [IMPORT OK] {mesh_name} | Extent=X:{b.box_extent.x:.1f},Y:{b.box_extent.y:.1f},Z:{b.box_extent.z:.1f} | Origin={b.origin.x:.1f},{b.origin.y:.1f},{b.origin.z:.1f}")
        else:
            log(f"  [IMPORT FAIL] Could not load {mesh_name}")
    except Exception as e:
        log(f"  [IMPORT EXCEPTION] {mesh_name}: {e}")

log("FBX Sequential Import complete!")

# ------------------------------------------------------------------------------
# STEP 3: CONFIGURE STATIC MESH SOCKETS & MATERIALS
# ------------------------------------------------------------------------------
def assign_mesh_material(mesh, mat_instance):
    if not mesh or not mat_instance:
        return
    num_mats = mesh.get_num_sections(0) if hasattr(mesh, "get_num_sections") else 1
    for slot_idx in range(max(1, num_mats)):
        mesh.set_material(slot_idx, mat_instance)

def setup_mesh_sockets(mesh, socket_dict):
    if not mesh:
        return
    for sock_name, sock_data in socket_dict.items():
        fname = unreal.Name(sock_name)
        existing = mesh.find_socket(fname)
        if existing:
            mesh.remove_socket(existing)
        
        loc = sock_data.get("loc", [0.0, 0.0, 0.0])
        rot = sock_data.get("rot", [0.0, 0.0, 0.0])
        
        sock = unreal.StaticMeshSocket()
        sock.set_editor_property("socket_name", fname)
        sock.set_editor_property("relative_location", unreal.Vector(loc[0], loc[1], loc[2]))
        sock.set_editor_property("relative_rotation", unreal.Rotator(rot[0], rot[1], rot[2]))
        mesh.add_socket(sock)

# ------------------------------------------------------------------------------
# STEP 4: CREATE AUTHORITATIVE UFPSWeaponDataAsset INSTANCES
# ------------------------------------------------------------------------------
data_asset_class = unreal.load_class(None, "/Script/NobleFermi.FPSWeaponDataAsset")
data_factory = unreal.DataAssetFactory()
data_factory.set_editor_property("data_asset_class", data_asset_class)

meta_files = sorted(glob.glob(os.path.join(PROCESSED_DIR, "*.meta.json")))
log(f"Processing {len(meta_files)} weapon meta definitions...")

archetype_map = {
    "AssaultRifle": unreal.FPSWeaponArchetype.ASSAULT_RIFLE,
    "BattleRifle": unreal.FPSWeaponArchetype.BATTLE_RIFLE,
    "Shotgun": unreal.FPSWeaponArchetype.SHOTGUN,
    "Pistol": unreal.FPSWeaponArchetype.PISTOL,
    "Throwable": unreal.FPSWeaponArchetype.THROWABLE,
    "Melee": unreal.FPSWeaponArchetype.MELEE,
}

created_data_assets = []

for meta_path in meta_files:
    with open(meta_path, "r", encoding="utf-8") as f:
        meta = json.load(f)

    wid = meta["id"]
    wname = meta["name"]
    archetype_str = meta["archetype"]
    caliber = meta.get("caliber", "")
    stats = meta.get("stats", {})
    sockets = meta.get("sockets", {})

    receiver_mesh_name = os.path.splitext(meta["receiver_mesh"])[0]
    receiver_path = f"{MESH_DEST_PATH}/{receiver_mesh_name}"
    receiver_mesh = editor_asset_lib.load_asset(receiver_path)

    mag_mesh = None
    if meta.get("magazine_mesh"):
        mag_mesh_name = os.path.splitext(meta["magazine_mesh"])[0]
        mag_path = f"{MESH_DEST_PATH}/{mag_mesh_name}"
        mag_mesh = editor_asset_lib.load_asset(mag_path)

    # Determine PBR material flavor
    if archetype_str == "Melee":
        mat_inst = mat_instances.get("MI_Weapon_Blade")
    elif "Grenade" in wid:
        mat_inst = mat_instances.get("MI_Weapon_OliveDrab")
    elif "Stun" in wid:
        mat_inst = mat_instances.get("MI_Weapon_Silver")
    elif wid in ("W02_VK74", "W07_Kestrel357"):
        mat_inst = mat_instances.get("MI_Weapon_Steel")
    else:
        mat_inst = mat_instances.get("MI_Weapon_Gunmetal")

    # Configure receiver mesh
    if receiver_mesh:
        assign_mesh_material(receiver_mesh, mat_inst)
        setup_mesh_sockets(receiver_mesh, sockets)
        editor_asset_lib.save_loaded_asset(receiver_mesh, only_if_is_dirty=False)
        log(f"Configured receiver: {receiver_mesh_name} with {len(sockets)} sockets")
    else:
        log(f"Receiver mesh not loaded: {receiver_path}")

    # Configure mag mesh if present
    if mag_mesh:
        mag_mat = mat_instances.get("MI_Weapon_Polymer") if "W01" in wid or "W05" in wid else mat_inst
        assign_mesh_material(mag_mesh, mag_mat)
        setup_mesh_sockets(mag_mesh, {"Socket_FeedLip": {"loc": [0.0, 0.0, 0.0], "rot": [0.0, 0.0, 0.0]}})
        editor_asset_lib.save_loaded_asset(mag_mesh, only_if_is_dirty=False)
        log(f"Configured magazine: {mag_path}")

    # Create or update UFPSWeaponDataAsset
    da_asset_name = f"DA_{wid}"
    da_asset_path = f"{DATA_DEST_PATH}/{da_asset_name}"
    da = editor_asset_lib.load_asset(da_asset_path)
    if not da:
        da = asset_tools.create_asset(da_asset_name, DATA_DEST_PATH, None, data_factory)
        log(f"Created new UFPSWeaponDataAsset: {da_asset_name}")

    if da:
        da.set_editor_property("weapon_id", wid)
        da.set_editor_property("weapon_display_name", unreal.Text(wname))
        da.set_editor_property("archetype", archetype_map.get(archetype_str, unreal.FPSWeaponArchetype.ASSAULT_RIFLE))
        da.set_editor_property("caliber", caliber)
        
        if receiver_mesh:
            da.set_editor_property("receiver_mesh", receiver_mesh)
        if mag_mesh:
            da.set_editor_property("magazine_mesh", mag_mesh)

        da.set_editor_property("magazine_capacity", stats.get("mag_capacity", 30))
        da.set_editor_property("reserve_ammo", stats.get("reserve_ammo", 120))
        da.set_editor_property("base_damage", float(stats.get("base_damage", 30.0)))
        da.set_editor_property("fire_rate_rpm", float(stats.get("fire_rate_rpm", 600.0)))
        da.set_editor_property("effective_range_meters", float(stats.get("effective_range_m", 100.0)))
        da.set_editor_property("reload_time_seconds", float(stats.get("reload_time_sec", 2.2)))
        da.set_editor_property("empty_reload_time_seconds", float(stats.get("empty_reload_time_sec", 2.7)))
        da.set_editor_property("recoil_pitch", float(stats.get("recoil_pitch", 0.35)))
        da.set_editor_property("recoil_yaw_spread", float(stats.get("recoil_yaw_spread", 0.12)))

        # Sockets in DataAsset
        rh = sockets.get("Socket_RightHand", {"loc": [0,0,0], "rot": [0,0,0]})
        lh = sockets.get("Socket_LeftHand", {"loc": [0,0,0], "rot": [0,0,0]})
        mz = sockets.get("Socket_Muzzle", {"loc": [0,0,0]})
        mw = sockets.get("Socket_Magwell", {"loc": [0,0,0]})
        op = sockets.get("Socket_Optic", {"loc": [0,0,0]})
        ej = sockets.get("Socket_Eject", {"loc": [0,0,0]})

        da.set_editor_property("right_hand_grip_transform", unreal.Transform(
            location=unreal.Vector(rh["loc"][0], rh["loc"][1], rh["loc"][2]),
            rotation=unreal.Rotator(rh["rot"][0], rh["rot"][1], rh["rot"][2]),
            scale=unreal.Vector(1,1,1)
        ))
        da.set_editor_property("left_hand_grip_transform", unreal.Transform(
            location=unreal.Vector(lh["loc"][0], lh["loc"][1], lh["loc"][2]),
            rotation=unreal.Rotator(lh["rot"][0], lh["rot"][1], lh["rot"][2]),
            scale=unreal.Vector(1,1,1)
        ))
        da.set_editor_property("muzzle_socket_offset", unreal.Vector(mz["loc"][0], mz["loc"][1], mz["loc"][2]))
        da.set_editor_property("mag_well_socket_offset", unreal.Vector(mw["loc"][0], mw["loc"][1], mw["loc"][2]))
        da.set_editor_property("optic_socket_offset", unreal.Vector(op["loc"][0], op["loc"][1], op["loc"][2]))
        da.set_editor_property("ejection_port_offset", unreal.Vector(ej["loc"][0], ej["loc"][1], ej["loc"][2]))

        editor_asset_lib.save_loaded_asset(da, only_if_is_dirty=False)
        created_data_assets.append(da)

# ------------------------------------------------------------------------------
# STEP 5: SAVE ALL DIRECTORIES AND PRINT SUMMARY
# ------------------------------------------------------------------------------
editor_asset_lib.save_directory(MESH_DEST_PATH, only_if_is_dirty=False, recursive=True)
editor_asset_lib.save_directory(MAT_DEST_PATH, only_if_is_dirty=False, recursive=True)
editor_asset_lib.save_directory(DATA_DEST_PATH, only_if_is_dirty=False, recursive=True)

log("================================================================================")
log(f"  ARSENAL IMPORT COMPLETED: {len(created_data_assets)} / {len(meta_files)} WEAPONS READY")
log("================================================================================")
