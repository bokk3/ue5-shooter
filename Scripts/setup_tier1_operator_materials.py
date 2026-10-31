import unreal
import os
import sys

unreal.log_warning("==================================================")
unreal.log_warning(">>> SETTING UP TIER-1 OPERATOR MATERIALS & RIG <<<")
unreal.log_warning("==================================================")

base_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Characters\BaseMale"
sk_fbx = os.path.join(base_dir, "SK_BaseMale.fbx")
posed_fbx = os.path.join(base_dir, "SM_Soldier_Posed.fbx")

materials_path = "/Game/Characters/BaseMale/Materials"
textures_path = "/Game/Characters/BaseMale/Textures"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_editing = unreal.MaterialEditingLibrary

# Load textures
tex_soldier_diffuse = unreal.EditorAssetLibrary.load_asset(f"{textures_path}/T_Soldier_Diffuse")
tex_soldier_normal = unreal.EditorAssetLibrary.load_asset(f"{textures_path}/T_Soldier_Normal")
tex_ripstop_normal = unreal.EditorAssetLibrary.load_asset(f"{textures_path}/T_Ripstop_Normal")
tex_ripstop_rough = unreal.EditorAssetLibrary.load_asset(f"{textures_path}/T_Ripstop_Roughness")

# 1. Setup or update M_Operator_BaseBody (Uniform Slot 0)
def create_uniform_material(mat_name):
    full_path = f"{materials_path}/{mat_name}"
    mat = unreal.EditorAssetLibrary.load_asset(full_path)
    if not mat:
        mat_factory = unreal.MaterialFactoryNew()
        mat = asset_tools.create_asset(mat_name, materials_path, unreal.Material, mat_factory)

    try:
        mat.set_editor_property("used_with_skeletal_mesh", True)
        mat.set_editor_property("two_sided", False)
    except Exception as e:
        unreal.log_warning(f"Material property notice: {e}")

    # Primary Tactical Crye Uniform Texture Sample
    if tex_soldier_diffuse:
        samp_diff = mat_editing.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, -200)
        samp_diff.set_editor_property("parameter_name", "Texture_Diffuse")
        samp_diff.set_editor_property("texture", tex_soldier_diffuse)

        col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, 50)
        col_param.set_editor_property("parameter_name", "BaseColor")
        col_param.set_editor_property("default_value", unreal.LinearColor(0.85, 0.90, 0.95, 1.0)) # Crisp Tactical Slate Tint

        mult_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, -100)
        mat_editing.connect_material_expressions(samp_diff, "", mult_node, "A")
        mat_editing.connect_material_expressions(col_param, "", mult_node, "B")
        mat_editing.connect_material_property(mult_node, "", unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -200)
        col_param.set_editor_property("parameter_name", "BaseColor")
        col_param.set_editor_property("default_value", unreal.LinearColor(0.18, 0.22, 0.28, 1.0))
        mat_editing.connect_material_property(col_param, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Normal Map
    active_norm = tex_soldier_normal if tex_soldier_normal else tex_ripstop_normal
    if active_norm:
        samp_norm = mat_editing.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 200)
        samp_norm.set_editor_property("parameter_name", "Texture_Normal")
        samp_norm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        samp_norm.set_editor_property("texture", active_norm)
        mat_editing.connect_material_property(samp_norm, "", unreal.MaterialProperty.MP_NORMAL)

    # Roughness
    rough_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 50)
    rough_param.set_editor_property("parameter_name", "RoughnessBase")
    rough_param.set_editor_property("default_value", 0.65)
    mat_editing.connect_material_property(rough_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Metallic
    metal_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 120)
    metal_param.set_editor_property("parameter_name", "Metallic")
    metal_param.set_editor_property("default_value", 0.05)
    mat_editing.connect_material_property(metal_param, "", unreal.MaterialProperty.MP_METALLIC)

    # Ambient Fill Emissive (guarantees uniform detail & camo legibility under any lighting)
    amb_scale = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 200)
    amb_scale.set_editor_property("parameter_name", "AmbientFillScale")
    amb_scale.set_editor_property("default_value", 0.22)

    amb_mult = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 200)
    if tex_soldier_diffuse:
        mat_editing.connect_material_expressions(mult_node, "", amb_mult, "A")
    else:
        mat_editing.connect_material_expressions(col_param, "", amb_mult, "A")
    mat_editing.connect_material_expressions(amb_scale, "", amb_mult, "B")
    mat_editing.connect_material_property(amb_mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    unreal.log_warning(f"[MAT] Successfully configured {mat_name}")
    return mat

# 2. Setup M_Operator_Armor (FAST Helmet, NVGs, Earcups, Holster Slot 1)
def create_armor_material(mat_name):
    full_path = f"{materials_path}/{mat_name}"
    mat = unreal.EditorAssetLibrary.load_asset(full_path)
    if not mat:
        mat_factory = unreal.MaterialFactoryNew()
        mat = asset_tools.create_asset(mat_name, materials_path, unreal.Material, mat_factory)

    try:
        mat.set_editor_property("used_with_skeletal_mesh", True)
    except Exception as e:
        pass

    col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -200)
    col_param.set_editor_property("parameter_name", "BaseColor")
    col_param.set_editor_property("default_value", unreal.LinearColor(0.12, 0.14, 0.17, 1.0)) # Crisp Matte Charcoal Kevlar
    mat_editing.connect_material_property(col_param, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 50)
    rough_param.set_editor_property("parameter_name", "RoughnessBase")
    rough_param.set_editor_property("default_value", 0.45)
    mat_editing.connect_material_property(rough_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 120)
    metal_param.set_editor_property("parameter_name", "Metallic")
    metal_param.set_editor_property("default_value", 0.25)
    mat_editing.connect_material_property(metal_param, "", unreal.MaterialProperty.MP_METALLIC)

    # Ambient Fill Emissive
    amb_scale = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 200)
    amb_scale.set_editor_property("parameter_name", "AmbientFillScale")
    amb_scale.set_editor_property("default_value", 0.18)

    amb_mult = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 200)
    mat_editing.connect_material_expressions(col_param, "", amb_mult, "A")
    mat_editing.connect_material_expressions(amb_scale, "", amb_mult, "B")
    mat_editing.connect_material_property(amb_mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    unreal.log_warning(f"[MAT] Successfully configured {mat_name}")
    return mat

# 3. Setup M_Soldier_Visor (Ballistic Shades & PDA Display Slot 2)
def create_visor_material(mat_name):
    full_path = f"{materials_path}/{mat_name}"
    mat = unreal.EditorAssetLibrary.load_asset(full_path)
    if not mat:
        mat_factory = unreal.MaterialFactoryNew()
        mat = asset_tools.create_asset(mat_name, materials_path, unreal.Material, mat_factory)

    try:
        mat.set_editor_property("used_with_skeletal_mesh", True)
    except Exception as e:
        pass

    col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -200)
    col_param.set_editor_property("parameter_name", "BaseColor")
    col_param.set_editor_property("default_value", unreal.LinearColor(0.01, 0.02, 0.03, 1.0)) # Deep Obsidian
    mat_editing.connect_material_property(col_param, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 50)
    rough_param.set_editor_property("parameter_name", "RoughnessBase")
    rough_param.set_editor_property("default_value", 0.06)
    mat_editing.connect_material_property(rough_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 120)
    metal_param.set_editor_property("parameter_name", "Metallic")
    metal_param.set_editor_property("default_value", 0.92)
    mat_editing.connect_material_property(metal_param, "", unreal.MaterialProperty.MP_METALLIC)

    # Green CRT Matrix Emissive Display
    emis_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 200)
    emis_param.set_editor_property("parameter_name", "EmissiveColor")
    emis_param.set_editor_property("default_value", unreal.LinearColor(0.0, 1.6, 0.45, 1.0)) # Phosphor Green
    mat_editing.connect_material_property(emis_param, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    unreal.log_warning(f"[MAT] Successfully configured {mat_name}")
    return mat

# 4. Setup M_Operator_Patch (Gold NOBLE FERMI Callsign Patch Slot 3)
def create_patch_material(mat_name):
    full_path = f"{materials_path}/{mat_name}"
    mat = unreal.EditorAssetLibrary.load_asset(full_path)
    if not mat:
        mat_factory = unreal.MaterialFactoryNew()
        mat = asset_tools.create_asset(mat_name, materials_path, unreal.Material, mat_factory)

    try:
        mat.set_editor_property("used_with_skeletal_mesh", True)
    except Exception as e:
        pass

    # Metallic 24K Gold Embroidery
    col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -200)
    col_param.set_editor_property("parameter_name", "BaseColor")
    col_param.set_editor_property("default_value", unreal.LinearColor(0.95, 0.78, 0.28, 1.0)) # Gold #E5C747
    mat_editing.connect_material_property(col_param, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 50)
    rough_param.set_editor_property("parameter_name", "RoughnessBase")
    rough_param.set_editor_property("default_value", 0.32)
    mat_editing.connect_material_property(rough_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 120)
    metal_param.set_editor_property("parameter_name", "Metallic")
    metal_param.set_editor_property("default_value", 0.85)
    mat_editing.connect_material_property(metal_param, "", unreal.MaterialProperty.MP_METALLIC)

    # Ambient Gold Luster Emissive
    emis_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 200)
    emis_param.set_editor_property("parameter_name", "EmissiveColor")
    emis_param.set_editor_property("default_value", unreal.LinearColor(0.42, 0.32, 0.10, 1.0)) # Warm Gold Ambient Glow
    mat_editing.connect_material_property(emis_param, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    unreal.log_warning(f"[MAT] Successfully configured {mat_name}")
    return mat

mat_uniform = create_uniform_material("M_Operator_BaseBody")
mat_armor = create_armor_material("M_Operator_Armor")
mat_visor = create_visor_material("M_Soldier_Visor")
mat_patch = create_patch_material("M_Operator_Patch")

mat_steel = unreal.EditorAssetLibrary.load_asset(f"{materials_path}/M_AK47_Steel")
mat_wood = unreal.EditorAssetLibrary.load_asset(f"{materials_path}/M_AK47_Wood")
mat_bake = unreal.EditorAssetLibrary.load_asset(f"{materials_path}/M_AK47_Bakelite")

# 5. Import SM_Soldier_Posed
posed_task = unreal.AssetImportTask()
posed_task.filename = posed_fbx
posed_task.destination_path = "/Game/Characters/BaseMale"
posed_task.destination_name = "SM_Soldier_Posed"
posed_task.replace_existing = True
posed_task.automated = True
posed_task.save = True

posed_opts = unreal.FbxImportUI()
posed_opts.import_mesh = True
posed_opts.import_as_skeletal = False
posed_opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
posed_opts.automated_import_should_detect_type = False
posed_opts.import_animations = False
posed_opts.import_materials = False
posed_opts.import_textures = False
posed_opts.static_mesh_import_data.combine_meshes = True
posed_opts.static_mesh_import_data.generate_lightmap_u_vs = True
posed_task.options = posed_opts

unreal.log_warning("Importing SM_Soldier_Posed.fbx...")
asset_tools.import_asset_tasks([posed_task])

sm_posed = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_Soldier_Posed")
if sm_posed:
    materials_list = [mat_uniform, mat_armor, mat_visor, mat_patch, mat_steel, mat_wood, mat_bake]
    for idx, m in enumerate(materials_list):
        if m:
            sm_posed.set_material(idx, m)
            unreal.log_warning(f"  SM_Soldier_Posed Slot {idx} -> {m.get_name()}")
    unreal.EditorAssetLibrary.save_loaded_asset(sm_posed, only_if_is_dirty=False)
    unreal.log_warning("Successfully updated SM_Soldier_Posed materials!")

# 6. Reimport SK_BaseMale (target existing SK_BaseMale_Skeleton)
sk_skeleton = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale_Skeleton")

sk_task = unreal.AssetImportTask()
sk_task.filename = sk_fbx
sk_task.destination_path = "/Game/Characters/BaseMale"
sk_task.destination_name = "SK_BaseMale"
sk_task.replace_existing = True
sk_task.automated = True
sk_task.save = True

sk_opts = unreal.FbxImportUI()
sk_opts.import_mesh = True
sk_opts.import_as_skeletal = True
sk_opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
sk_opts.create_physics_asset = False
sk_opts.automated_import_should_detect_type = False
sk_opts.import_animations = False
sk_opts.import_materials = False
sk_opts.import_textures = False
if sk_skeleton:
    sk_opts.skeleton = sk_skeleton
sk_task.options = sk_opts

unreal.log_warning("Importing SK_BaseMale.fbx onto SK_BaseMale_Skeleton...")
asset_tools.import_asset_tasks([sk_task])

sk_base = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if sk_base:
    skeletal_mats = sk_base.get_editor_property("materials")
    unreal.log_warning(f"SK_BaseMale imported with {len(skeletal_mats)} material slots")
    sk_assigned = [mat_uniform, mat_armor, mat_visor, mat_patch]
    for idx in range(min(len(skeletal_mats), len(sk_assigned))):
        skeletal_mats[idx].material_interface = sk_assigned[idx]
        unreal.log_warning(f"  SK_BaseMale Slot {idx} -> {sk_assigned[idx].get_name()}")
    sk_base.set_editor_property("materials", skeletal_mats)
    unreal.EditorAssetLibrary.save_loaded_asset(sk_base, only_if_is_dirty=False)
    unreal.log_warning("Successfully assigned materials to SK_BaseMale!")

unreal.EditorAssetLibrary.save_directory("/Game/Characters/BaseMale", only_if_is_dirty=False, recursive=True)
unreal.log_warning("==================================================")
unreal.log_warning(">>> TIER-1 OPERATOR RIG & ASSETS COMPLETE <<<")
unreal.log_warning("==================================================")
