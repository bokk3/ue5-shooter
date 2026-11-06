import unreal
import os

unreal.log_warning("========================================================")
unreal.log_warning(">>> IMPORTING CLEAN OPERATOR MESHES & PBR MATERIALS <<<")
unreal.log_warning("========================================================")

base_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Characters\BaseMale"
sk_fbx = os.path.join(base_dir, "SK_BaseMale.fbx")
posed_fbx = os.path.join(base_dir, "SM_Soldier_Posed.fbx")

materials_path = "/Game/Characters/BaseMale/Materials"
textures_path = "/Game/Characters/BaseMale/Textures"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_editing = unreal.MaterialEditingLibrary

tex_soldier_diffuse = unreal.EditorAssetLibrary.load_asset(f"{textures_path}/T_Soldier_Diffuse")
tex_soldier_normal = unreal.EditorAssetLibrary.load_asset(f"{textures_path}/T_Soldier_Normal")

# 1. Clean M_Operator_BaseBody (Slot 0: Full sculpted Vanguard body, armor, helmet & boots)
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

    if tex_soldier_diffuse:
        samp_diff = mat_editing.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -650, -200)
        samp_diff.set_editor_property("parameter_name", "Texture_Diffuse")
        samp_diff.set_editor_property("texture", tex_soldier_diffuse)

        col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -650, 40)
        col_param.set_editor_property("parameter_name", "CamoColorPrimary")
        col_param.set_editor_property("default_value", unreal.LinearColor(0.58, 0.62, 0.68, 1.0))

        mult_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, -100)
        mat_editing.connect_material_expressions(samp_diff, "", mult_node, "A")
        mat_editing.connect_material_expressions(col_param, "", mult_node, "B")
        mat_editing.connect_material_property(mult_node, "", unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -450, -200)
        col_param.set_editor_property("parameter_name", "CamoColorPrimary")
        col_param.set_editor_property("default_value", unreal.LinearColor(0.28, 0.32, 0.38, 1.0))
        mat_editing.connect_material_property(col_param, "", unreal.MaterialProperty.MP_BASE_COLOR)

    if tex_soldier_normal:
        samp_norm = mat_editing.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 220)
        samp_norm.set_editor_property("parameter_name", "Texture_Normal")
        samp_norm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        samp_norm.set_editor_property("texture", tex_soldier_normal)
        mat_editing.connect_material_property(samp_norm, "", unreal.MaterialProperty.MP_NORMAL)

    rough_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 60)
    rough_param.set_editor_property("parameter_name", "RoughnessBase")
    rough_param.set_editor_property("default_value", 0.55)
    mat_editing.connect_material_property(rough_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 140)
    metal_param.set_editor_property("parameter_name", "Metallic")
    metal_param.set_editor_property("default_value", 0.12)
    mat_editing.connect_material_property(metal_param, "", unreal.MaterialProperty.MP_METALLIC)

    # Zero out emissive so the operator is never washed out
    zero_emis = mat_editing.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 240)
    zero_emis.set_editor_property("r", 0.0)
    mat_editing.connect_material_property(zero_emis, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    unreal.log_warning(f"[MAT] Cleanly rebuilt {mat_name}")
    return mat

# 2. Clean M_Soldier_Visor (Slot 1: Ballistic glass visor mapped to T_Soldier_Diffuse visor UV island)
def create_visor_material(mat_name):
    full_path = f"{materials_path}/{mat_name}"
    mat = unreal.EditorAssetLibrary.load_asset(full_path)
    if not mat:
        mat_factory = unreal.MaterialFactoryNew()
        mat = asset_tools.create_asset(mat_name, materials_path, unreal.Material, mat_factory)

    try:
        mat.set_editor_property("used_with_skeletal_mesh", True)
        mat.set_editor_property("two_sided", False)
    except Exception:
        pass

    if tex_soldier_diffuse:
        samp_diff = mat_editing.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -650, -200)
        samp_diff.set_editor_property("parameter_name", "Texture_Diffuse")
        samp_diff.set_editor_property("texture", tex_soldier_diffuse)

        col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -650, 40)
        col_param.set_editor_property("parameter_name", "BaseColor")
        col_param.set_editor_property("default_value", unreal.LinearColor(0.65, 0.75, 0.85, 1.0))

        mult_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, -100)
        mat_editing.connect_material_expressions(samp_diff, "", mult_node, "A")
        mat_editing.connect_material_expressions(col_param, "", mult_node, "B")
        mat_editing.connect_material_property(mult_node, "", unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -450, -200)
        col_param.set_editor_property("parameter_name", "BaseColor")
        col_param.set_editor_property("default_value", unreal.LinearColor(0.02, 0.03, 0.05, 1.0))
        mat_editing.connect_material_property(col_param, "", unreal.MaterialProperty.MP_BASE_COLOR)

    if tex_soldier_normal:
        samp_norm = mat_editing.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 220)
        samp_norm.set_editor_property("parameter_name", "Texture_Normal")
        samp_norm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        samp_norm.set_editor_property("texture", tex_soldier_normal)
        mat_editing.connect_material_property(samp_norm, "", unreal.MaterialProperty.MP_NORMAL)

    rough_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 60)
    rough_param.set_editor_property("parameter_name", "RoughnessBase")
    rough_param.set_editor_property("default_value", 0.12)
    mat_editing.connect_material_property(rough_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 140)
    metal_param.set_editor_property("parameter_name", "Metallic")
    metal_param.set_editor_property("default_value", 0.78)
    mat_editing.connect_material_property(metal_param, "", unreal.MaterialProperty.MP_METALLIC)

    zero_emis = mat_editing.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 240)
    zero_emis.set_editor_property("r", 0.0)
    mat_editing.connect_material_property(zero_emis, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    unreal.log_warning(f"[MAT] Cleanly rebuilt {mat_name}")
    return mat

# 2.5 Clean M_Belt_Gear_PBR (Dedicated untextured PBR material for 3D procedural belt gear: STANAG mags, M67 Frag, M84 Flashbang)
def create_belt_gear_material(mat_name):
    full_path = f"{materials_path}/{mat_name}"
    mat = unreal.EditorAssetLibrary.load_asset(full_path)
    if not mat:
        mat_factory = unreal.MaterialFactoryNew()
        mat = asset_tools.create_asset(mat_name, materials_path, unreal.Material, mat_factory)
    else:
        mat_editing.delete_all_material_expressions(mat)

    try:
        mat.set_editor_property("used_with_skeletal_mesh", True)
        mat.set_editor_property("two_sided", True)
    except Exception:
        pass

    col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -450, -160)
    col_param.set_editor_property("parameter_name", "BaseColor")
    col_param.set_editor_property("default_value", unreal.LinearColor(0.18, 0.20, 0.22, 1.0))
    mat_editing.connect_material_property(col_param, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 40)
    rough_param.set_editor_property("parameter_name", "Roughness")
    rough_param.set_editor_property("default_value", 0.45)
    mat_editing.connect_material_property(rough_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 140)
    metal_param.set_editor_property("parameter_name", "Metallic")
    metal_param.set_editor_property("default_value", 0.50)
    mat_editing.connect_material_property(metal_param, "", unreal.MaterialProperty.MP_METALLIC)

    zero_emis = mat_editing.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 240)
    zero_emis.set_editor_property("r", 0.0)
    mat_editing.connect_material_property(zero_emis, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    unreal.log_warning(f"[MAT] Cleanly rebuilt {mat_name}")
    return mat

mat_uniform = create_uniform_material("M_Operator_BaseBody")
mat_armor = create_uniform_material("M_Operator_Armor")
mat_visor = create_visor_material("M_Soldier_Visor")
mat_belt_gear = create_belt_gear_material("M_Belt_Gear_PBR")

# 3. Import clean SM_Soldier_Posed
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

unreal.log_warning("Importing clean SM_Soldier_Posed.fbx...")
asset_tools.import_asset_tasks([posed_task])

sm_posed = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_Soldier_Posed")
if sm_posed:
    sm_posed.set_material(0, mat_uniform)
    sm_posed.set_material(1, mat_visor)
    unreal.EditorAssetLibrary.save_loaded_asset(sm_posed, only_if_is_dirty=False)
    unreal.log_warning("Successfully assigned clean materials [M_Operator_BaseBody, M_Soldier_Visor] to SM_Soldier_Posed!")

# 4. Delete old conflicting SK_BaseMale / SK_BaseMale_Skeleton and import clean SK_BaseMale.fbx with its native Vanguard rest pose
if unreal.EditorAssetLibrary.does_asset_exist("/Game/Characters/BaseMale/SK_BaseMale"):
    unreal.EditorAssetLibrary.delete_asset("/Game/Characters/BaseMale/SK_BaseMale")
if unreal.EditorAssetLibrary.does_asset_exist("/Game/Characters/BaseMale/SK_BaseMale_Skeleton"):
    unreal.EditorAssetLibrary.delete_asset("/Game/Characters/BaseMale/SK_BaseMale_Skeleton")

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
sk_opts.create_physics_asset = True
sk_opts.automated_import_should_detect_type = False
sk_opts.import_animations = False
sk_opts.import_materials = False
sk_opts.import_textures = False
sk_task.options = sk_opts

unreal.log_warning("Importing clean SK_BaseMale.fbx with native Vanguard Skeleton...")
asset_tools.import_asset_tasks([sk_task])

sk_base = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if sk_base:
    skeletal_mats = list(sk_base.get_editor_property("materials"))
    sk_assigned = [mat_uniform, mat_visor]
    new_mats = []
    for idx in range(len(skeletal_mats)):
        mat_slot = skeletal_mats[idx]
        if idx < len(sk_assigned):
            mat_slot.set_editor_property("material_interface", sk_assigned[idx])
        new_mats.append(mat_slot)
    sk_base.set_editor_property("materials", new_mats)
    unreal.EditorAssetLibrary.save_loaded_asset(sk_base, only_if_is_dirty=False)
    unreal.log_warning("Successfully assigned clean materials [M_Operator_BaseBody, M_Soldier_Visor] to SK_BaseMale!")

unreal.EditorAssetLibrary.save_directory("/Game/Characters/BaseMale", only_if_is_dirty=False, recursive=True)
unreal.log_warning("========================================================")
unreal.log_warning(">>> CLEAN OPERATOR MESHES & PBR MATERIALS COMPLETE <<<")
unreal.log_warning("========================================================")
