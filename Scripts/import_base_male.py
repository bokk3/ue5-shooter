import unreal
import os
import sys

print(">>> IMPORTING SK_BASEMALE & SM_AK47 FBX ASSETS INTO UNREAL ENGINE 5.8 <<<")

base_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Characters\BaseMale"
sk_fbx_path = os.path.join(base_dir, "SK_BaseMale.fbx")
ak_fbx_path = os.path.join(base_dir, "SM_AK47.fbx")
destination_path = "/Game/Characters/BaseMale"
materials_path = "/Game/Characters/BaseMale/Materials"

if not os.path.exists(sk_fbx_path):
    print(f"ERROR: SK_BaseMale.fbx not found at {sk_fbx_path}")
    sys.exit(1)

if not os.path.exists(ak_fbx_path):
    print(f"ERROR: SM_AK47.fbx not found at {ak_fbx_path}")
    sys.exit(1)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_editing = unreal.MaterialEditingLibrary

# 1. Create / Update AK-47 PBR Materials (Steel, Walnut Wood, Bakelite)
def create_pbr_material(mat_name, base_color, roughness, metallic):
    full_path = f"{materials_path}/{mat_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        mat = unreal.EditorAssetLibrary.load_asset(full_path)
        mat_editing.delete_all_material_expressions(mat)
    else:
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

mat_steel = create_pbr_material("M_AK47_Steel", (0.10, 0.11, 0.13), 0.35, 0.85)
mat_wood = create_pbr_material("M_AK47_Wood", (0.38, 0.18, 0.07), 0.55, 0.0)
mat_bakelite = create_pbr_material("M_AK47_Bakelite", (0.48, 0.15, 0.04), 0.40, 0.05)

# 2. Import SK_BaseMale.fbx
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

# 4. Import SM_AK47.fbx
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

# 5. Assign Materials to SK_BaseMale and SM_AK47
sk_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
body_mat = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody")
if sk_mesh and body_mat:
    skeletal_mats = sk_mesh.get_editor_property("materials")
    if len(skeletal_mats) > 0:
        skeletal_mats[0].material_interface = body_mat
    else:
        skeletal_mats.append(unreal.SkeletalMaterial(material_interface=body_mat, material_slot_name="M_Operator_BaseBody"))
    sk_mesh.set_editor_property("materials", skeletal_mats)
    unreal.EditorAssetLibrary.save_loaded_asset(sk_mesh, only_if_is_dirty=False)
    print("Assigned M_Operator_BaseBody to SK_BaseMale slot 0")

ak_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_AK47")
if ak_mesh:
    ak_mesh.set_material(0, mat_steel)
    ak_mesh.set_material(1, mat_wood)
    ak_mesh.set_material(2, mat_bakelite)
    unreal.EditorAssetLibrary.save_loaded_asset(ak_mesh, only_if_is_dirty=False)
    print("Assigned M_AK47_Steel, M_AK47_Wood, M_AK47_Bakelite to SM_AK47 slots 0, 1, 2")

unreal.EditorAssetLibrary.save_directory(destination_path, only_if_is_dirty=False, recursive=True)
print("SUCCESSFULLY COMPLETED SK_BASEMALE & SM_AK47 IMPORT")
sys.exit(0)
