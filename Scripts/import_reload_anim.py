import unreal
import os
import sys

print(">>> IMPORTING RELOAD ANIMATION INTO UNREAL ENGINE 5.8 <<<")

skeleton_path = "/Game/Characters/BaseMale/SK_BaseMale_Skeleton"
skeleton = unreal.EditorAssetLibrary.load_asset(skeleton_path)
if not skeleton:
    print(f"ERROR: Skeleton not found at {skeleton_path}")
    sys.exit(1)

print(f"Loaded Target Skeleton: {skeleton.get_path_name()}")

destination_path = "/Game/Characters/BaseMale/Animations"
anim_names = [
    "Anim_BaseMale_Reload",
    "Anim_Operator_Reload"
]

anims_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Characters\BaseMale\Animations"
tasks = []
for anim_name in anim_names:
    fbx_file = os.path.join(anims_dir, f"{anim_name}.fbx")
    if not os.path.exists(fbx_file):
        unreal.log_warning(f"ERROR: FBX missing at {fbx_file}")
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

unreal.log_warning(f"Executing import tasks for {len(tasks)} animations...")
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_tools.import_asset_tasks(tasks)

unreal.EditorAssetLibrary.save_directory(destination_path, only_if_is_dirty=False, recursive=True)
unreal.log_warning(">>> RELOAD ANIMATION IMPORT COMPLETED <<<")
sys.exit(0)
