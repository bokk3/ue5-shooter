import unreal
import os
import sys

print(">>> IMPORTING 5 IK-BAKED TACTICAL STANCES & LOCOMOTION ANIMATIONS INTO UNREAL ENGINE 5.8 <<<")

skeleton_path = "/Game/Characters/BaseMale/SK_BaseMale_Skeleton"
skeleton = unreal.EditorAssetLibrary.load_asset(skeleton_path)
if not skeleton:
    print(f"ERROR: Skeleton not found at {skeleton_path}")
    sys.exit(1)

print(f"Loaded Target Skeleton: {skeleton.get_path_name()}")

destination_path = "/Game/Characters/BaseMale/Animations"
anim_names = [
    "Anim_BaseMale_Idle_ADS",
    "Anim_BaseMale_LowReady",
    "Anim_BaseMale_Walk",
    "Anim_BaseMale_Run",
    "Anim_BaseMale_Sprint",
    "Anim_BaseMale_Jump_Loop",
    "Anim_BaseMale_Crouch_Idle",
    "Anim_BaseMale_Crouch_Walk",
    "Anim_BaseMale_Slide",
    "Anim_BaseMale_Walk_Backwards",
    "Anim_Operator_Idle_ADS",
    "Anim_Operator_LowReady",
    "Anim_Operator_Walk",
    "Anim_Operator_Run",
    "Anim_Operator_Sprint",
    "Anim_Operator_Jump_Loop",
    "Anim_Operator_Crouch_Idle",
    "Anim_Operator_Crouch_Walk",
    "Anim_Operator_Slide",
    "Anim_Operator_Walk_Backwards"
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

# Verify weapon_r socket transform in Unreal Engine component space!
try:
    sk_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
    ak_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_AK47")
    if sk_mesh:
        sk_comp = unreal.SkeletalMeshComponent()
        sk_comp.set_skeletal_mesh_asset(sk_mesh)
        for bname in ["root", "pelvis", "hand_r", "hand_l", "weapon_r"]:
            t_comp = sk_comp.get_socket_transform(bname, unreal.RelativeTransformSpace.RTS_COMPONENT)
            unreal.log_warning(f"[UE_SOCKET_CHECK] {bname} (Component Space): Loc={t_comp.translation}, Rot={t_comp.rotation.rotator()}")
    if ak_mesh:
        b = ak_mesh.get_bounds()
        unreal.log_warning(f"[UE_AK47_BOUNDS] Origin={b.origin}, BoxExtent={b.box_extent}")
except Exception as e:
    unreal.log_warning(f"[UE_SOCKET_CHECK] Note: {e}")

unreal.log_warning(">>> ANIMATION IMPORT COMPLETED <<<")
sys.exit(0)
