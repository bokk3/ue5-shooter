import unreal
import os
import sys

print(">>> IMPORTING LOCOMOTION ANIMATIONS INTO UNREAL ENGINE <<<")

skeleton_path = "/Game/Characters/BaseMale/SK_BaseMale_Skeleton"
skeleton = unreal.EditorAssetLibrary.load_asset(skeleton_path)
if not skeleton:
    print(f"ERROR: Skeleton not found at {skeleton_path}")
    sys.exit(1)

print(f"Loaded Target Skeleton: {skeleton.get_path_name()}")

destination_path = "/Game/Characters/BaseMale/Animations"
anim_names = [
    "Anim_Operator_Idle_ADS",
    "Anim_Operator_Walk",
    "Anim_Operator_Run",
    "Anim_Operator_Sprint"
]

tasks = []
for anim_name in anim_names:
    fbx_file = os.path.abspath(f"Content/Characters/BaseMale/Animations/{anim_name}.fbx")
    if not os.path.exists(fbx_file):
        print(f"ERROR: FBX missing at {fbx_file}")
        continue
    
    task = unreal.AssetImportTask()
    task.filename = fbx_file
    task.destination_path = destination_path
    task.destination_name = anim_name
    task.replace_existing = True
    task.automated = True
    task.save = True
    
    options = unreal.FbxImportUI()
    options.import_mesh = False
    options.import_as_skeletal = False
    options.import_animations = True
    options.skeleton = skeleton
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    options.automated_import_should_detect_type = False
    
    task.options = options
    tasks.append(task)

print(f"Executing import tasks for {len(tasks)} animations...")
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_tools.import_asset_tasks(tasks)

# Save directory
unreal.EditorAssetLibrary.save_directory(destination_path, only_if_is_dirty=False, recursive=True)

imported_assets = unreal.EditorAssetLibrary.list_assets(destination_path, recursive=True)
print(f"Current Assets in {destination_path}:")
for a in imported_assets:
    print(f"  - {a}")

print(">>> ANIMATION IMPORT COMPLETED <<<")
sys.exit(0)
