import unreal
import os
import sys

print(">>> IMPORTING SK_BASEMALE & SM_AK47 FBX ASSETS INTO UNREAL ENGINE 5.8 <<<")

base_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Characters\BaseMale"
sk_fbx_path = os.path.join(base_dir, "SK_BaseMale.fbx")
ak_fbx_path = os.path.join(base_dir, "SM_AK47.fbx")
destination_path = "/Game/Characters/BaseMale"
materials_path = "/Game/Characters/BaseMale/Materials"
textures_path = "/Game/Characters/BaseMale/Textures"

if not os.path.exists(sk_fbx_path):
    print(f"ERROR: SK_BaseMale.fbx not found at {sk_fbx_path}")
    sys.exit(1)

if not os.path.exists(ak_fbx_path):
    print(f"ERROR: SM_AK47.fbx not found at {ak_fbx_path}")
    sys.exit(1)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_editing = unreal.MaterialEditingLibrary

# 1. Import Soldier Textures
def import_texture(tex_filename, is_normal, srgb):
    tex_path = os.path.join(base_dir, "Textures", tex_filename)
    if not os.path.exists(tex_path):
        print(f"Texture file not found: {tex_path}")
        return None
    
    tex_name = os.path.splitext(tex_filename)[0]
    task = unreal.AssetImportTask()
    task.filename = tex_path
    task.destination_path = textures_path
    task.destination_name = tex_name
    task.replace_existing = True
    task.automated = True
    task.save = True
    
    options = unreal.TextureFactory()
    task.factory = options
    asset_tools.import_asset_tasks([task])
    
    tex_asset = unreal.EditorAssetLibrary.load_asset(f"{textures_path}/{tex_name}")
    if tex_asset:
        if is_normal:
            tex_asset.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            try:
                tex_asset.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
            except Exception:
                pass
        tex_asset.set_editor_property("srgb", srgb)
        unreal.EditorAssetLibrary.save_loaded_asset(tex_asset)
        print(f"Imported texture: {tex_name}")
    return tex_asset

tex_diffuse = import_texture("T_Soldier_Diffuse.png", False, True)
tex_normal = import_texture("T_Soldier_Normal.png", True, False)

# 2. Helper to load or create PBR materials safely
def get_or_create_pbr_material(mat_name, base_color, roughness, metallic):
    full_path = f"{materials_path}/{mat_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        return unreal.EditorAssetLibrary.load_asset(full_path)

    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(mat_name, materials_path, unreal.Material, factory)
    try:
        mat.set_editor_property("used_with_static_lighting", False)
        mat.set_editor_property("used_with_skeletal_mesh", True)
    except Exception as e:
        print(f"Material property note: {e}")

    col_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -300, -100)
    col_node.set_editor_property("parameter_name", "BaseColor")
    col_node.set_editor_property("default_value", unreal.LinearColor(base_color[0], base_color[1], base_color[2], 1.0))
    mat_editing.connect_material_property(col_node, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 100)
    rough_node.set_editor_property("r", roughness)
    mat_editing.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
    metal_node.set_editor_property("r", metallic)
    mat_editing.connect_material_property(metal_node, "", unreal.MaterialProperty.MP_METALLIC)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat

mat_steel = get_or_create_pbr_material("M_AK47_Steel", (0.12, 0.13, 0.15), 0.35, 0.85)
mat_wood = get_or_create_pbr_material("M_AK47_Wood", (0.42, 0.22, 0.09), 0.55, 0.0)
mat_bakelite = get_or_create_pbr_material("M_AK47_Bakelite", (0.52, 0.18, 0.06), 0.40, 0.05)

# 3. Create Soldier Body and Visor Materials
def build_soldier_material(mat_name, b_is_visor):
    full_path = f"{materials_path}/{mat_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        unreal.EditorAssetLibrary.delete_asset(full_path)

    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(mat_name, materials_path, unreal.Material, factory)
    try:
        mat.set_editor_property("used_with_skeletal_mesh", True)
    except Exception:
        pass

    # Diffuse Texture Sample Parameter
    samp_diff = mat_editing.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, -200)
    samp_diff.set_editor_property("parameter_name", "Texture_Diffuse")
    if tex_diffuse:
        samp_diff.set_editor_property("texture", tex_diffuse)

    # Tint Parameter
    tint_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -400)
    tint_node.set_editor_property("parameter_name", "CamoColorPrimary")
    tint_node.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    # Multiply Diffuse * Tint
    mult_col = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, -300)
    mat_editing.connect_material_expressions(samp_diff, "", mult_col, "A")
    mat_editing.connect_material_expressions(tint_node, "", mult_col, "B")
    mat_editing.connect_material_property(mult_col, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Normal Map Sample Parameter
    if not b_is_visor and tex_normal:
        samp_norm = mat_editing.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 200)
        samp_norm.set_editor_property("parameter_name", "Texture_Normal")
        samp_norm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        samp_norm.set_editor_property("texture", tex_normal)
        mat_editing.connect_material_property(samp_norm, "", unreal.MaterialProperty.MP_NORMAL)

    # Roughness
    rough_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 50)
    rough_node.set_editor_property("parameter_name", "RoughnessBase")
    rough_node.set_editor_property("default_value", 0.08 if b_is_visor else 0.55)
    mat_editing.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Metallic
    metal_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 150)
    metal_node.set_editor_property("parameter_name", "Metallic")
    metal_node.set_editor_property("default_value", 0.95 if b_is_visor else 0.15)
    mat_editing.connect_material_property(metal_node, "", unreal.MaterialProperty.MP_METALLIC)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    print(f"Created Soldier Material: {mat_name}")
    return mat

mat_soldier_body = build_soldier_material("M_Operator_BaseBody", False)
mat_soldier_visor = build_soldier_material("M_Soldier_Visor", True)

# 4. Clean existing conflicting skeleton / mesh before importing new skeletal rig
if unreal.EditorAssetLibrary.does_asset_exist("/Game/Characters/BaseMale/SK_BaseMale"):
    unreal.EditorAssetLibrary.delete_asset("/Game/Characters/BaseMale/SK_BaseMale")
if unreal.EditorAssetLibrary.does_asset_exist("/Game/Characters/BaseMale/SK_BaseMale_Skeleton"):
    unreal.EditorAssetLibrary.delete_asset("/Game/Characters/BaseMale/SK_BaseMale_Skeleton")

# 5. Import SK_BaseMale.fbx
sk_task = unreal.AssetImportTask()
sk_task.filename = sk_fbx_path
sk_task.destination_path = destination_path
sk_task.destination_name = "SK_BaseMale"
sk_task.replace_existing = True
sk_task.automated = True
sk_task.save = True

sk_options = unreal.FbxImportUI()
sk_options.import_mesh = True
sk_options.import_as_skeletal = True
sk_options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
sk_options.create_physics_asset = True
sk_options.automated_import_should_detect_type = False
sk_options.import_animations = False
sk_options.import_materials = False
sk_options.import_textures = False
sk_task.options = sk_options

# 6. Import SM_AK47.fbx
ak_task = unreal.AssetImportTask()
ak_task.filename = ak_fbx_path
ak_task.destination_path = destination_path
ak_task.destination_name = "SM_AK47"
ak_task.replace_existing = True
ak_task.automated = True
ak_task.save = True

ak_options = unreal.FbxImportUI()
ak_options.import_mesh = True
ak_options.import_as_skeletal = False
ak_options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
ak_options.automated_import_should_detect_type = False
ak_options.import_animations = False
ak_options.import_materials = False
ak_options.import_textures = False
ak_options.static_mesh_import_data.combine_meshes = True
ak_task.options = ak_options

print("Executing import tasks for SK_BaseMale and SM_AK47...")
asset_tools.import_asset_tasks([sk_task, ak_task])

# 7. Assign Materials to SK_BaseMale slots
sk_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if sk_mesh:
    skeletal_mats = sk_mesh.get_editor_property("materials")
    print(f"SK_BaseMale material slots count: {len(skeletal_mats)}")
    if len(skeletal_mats) > 0 and mat_soldier_body:
        skeletal_mats[0].material_interface = mat_soldier_body
    if len(skeletal_mats) > 1 and mat_soldier_visor:
        skeletal_mats[1].material_interface = mat_soldier_visor
    sk_mesh.set_editor_property("materials", skeletal_mats)
    unreal.EditorAssetLibrary.save_loaded_asset(sk_mesh, only_if_is_dirty=False)
    print("Assigned M_Operator_BaseBody and M_Soldier_Visor to SK_BaseMale")

# 8. Assign Materials to SM_AK47
ak_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_AK47")
if ak_mesh:
    ak_mesh.set_material(0, mat_steel)
    ak_mesh.set_material(1, mat_wood)
    ak_mesh.set_material(2, mat_bakelite)
    unreal.EditorAssetLibrary.save_loaded_asset(ak_mesh, only_if_is_dirty=False)
    print("Assigned M_AK47_Steel, M_AK47_Wood, M_AK47_Bakelite to SM_AK47")

unreal.EditorAssetLibrary.save_directory(destination_path, only_if_is_dirty=False, recursive=True)
print("SUCCESSFULLY COMPLETED SK_BASEMALE & SM_AK47 IMPORT")
sys.exit(0)
