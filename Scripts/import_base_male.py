import unreal
import os

print("=== STARTING SK_BASEMALE IMPORT SCRIPT ===")

project_dir = unreal.SystemLibrary.get_project_directory()
fbx_path = os.path.normpath(os.path.join(project_dir, "Content/Characters/BaseMale/SK_BaseMale.fbx"))
dest_path = "/Game/Characters/BaseMale"
asset_name = "SK_BaseMale"

print(f"Target FBX: {fbx_path}")
print(f"Destination: {dest_path}/{asset_name}")

if not os.path.exists(fbx_path):
    print(f"ERROR: FBX does not exist at {fbx_path}")
else:
    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = dest_path
    task.destination_name = asset_name
    task.replace_existing = True
    task.automated = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = True
    options.import_materials = True
    options.import_textures = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH

    # Skeletal mesh import data
    sk_data = unreal.FbxSkeletalMeshImportData()
    sk_data.import_morph_targets = False
    sk_data.use_t0_as_ref_pose = True
    sk_data.preserve_smoothing_groups = True
    sk_data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS
    sk_data.convert_scene = True

    options.skeletal_mesh_import_data = sk_data
    options.import_animations = True
    
    anim_data = unreal.FbxAnimSequenceImportData()
    anim_data.animation_length = unreal.FBXAnimationLengthImportType.FBXAIT_EXPORTED_TIME
    options.anim_sequence_import_data = anim_data

    task.options = options

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_tools.import_asset_tasks([task])

    # Save imported assets
    unreal.EditorAssetLibrary.save_directory(dest_path)
    print(f"Successfully imported and saved assets in {dest_path}")

print("=== SK_BASEMALE IMPORT SCRIPT FINISHED ===")
