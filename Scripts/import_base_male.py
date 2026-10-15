import unreal
import os
import sys

print(">>> IMPORTING BASE MALE FBX AND CREATING PHYSICS ASSET <<<")

fbx_path = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Characters\BaseMale\SK_BaseMale.fbx"
destination_path = "/Game/Characters/BaseMale"

if not os.path.exists(fbx_path):
    print(f"ERROR: FBX not found at {fbx_path}")
    sys.exit(1)

print(f"FBX confirmed at: {fbx_path} (size: {os.path.getsize(fbx_path)} bytes)")

# Configure Import Task
task = unreal.AssetImportTask()
task.filename = fbx_path
task.destination_path = destination_path
task.destination_name = "SK_BaseMale"
task.replace_existing = True
task.automated = True
task.save = True

# FBX Import UI settings
options = unreal.FbxImportUI()
options.import_mesh = True
options.import_as_skeletal = True
options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
options.create_physics_asset = True
options.automated_import_should_detect_type = False
options.import_animations = False
options.import_materials = False
options.import_textures = False

task.options = options

print("Starting asset import...")
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_tools.import_asset_tasks([task])

print(">>> IMPORT TASK COMPLETED <<<")
print(f"Imported objects count: {len(task.imported_object_paths)}")
for p in task.imported_object_paths:
    print(f" - Imported: {p}")

# Force save all assets in destination
unreal.EditorAssetLibrary.save_directory(destination_path, only_if_is_dirty=False, recursive=True)

# List all assets currently in destination_path
assets = unreal.EditorAssetLibrary.list_assets(destination_path, recursive=True)
print(f"Assets in {destination_path}:")
for a in assets:
    print(f"  Asset: {a}")

print("SUCCESSFULLY COMPLETED SCRIPT")
sys.exit(0)
