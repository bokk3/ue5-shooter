import unreal
import sys

unreal.log_warning(">>> CREATING BS_BaseMale_Locomotion <<<")

skeleton_path = "/Game/Characters/BaseMale/SK_BaseMale_Skeleton"
skeleton = unreal.EditorAssetLibrary.load_asset(skeleton_path)
mesh_path = "/Game/Characters/BaseMale/SK_BaseMale"
mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)

if not skeleton or not mesh:
    unreal.log_error("Could not load skeleton or mesh!")
    sys.exit(1)

# Check if BlendSpace already exists
bs_path = "/Game/Characters/BaseMale/BS_BaseMale_Locomotion"
bs = unreal.EditorAssetLibrary.load_asset(bs_path)
if not bs:
    factory = unreal.BlendSpaceFactoryNew()
    factory.set_editor_property("target_skeleton", skeleton)
    factory.set_editor_property("preview_skeletal_mesh", mesh)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    bs = asset_tools.create_asset("BS_BaseMale_Locomotion", "/Game/Characters/BaseMale", unreal.BlendSpace, factory)

unreal.log_warning(f"BlendSpace created/loaded: {bs}")

# Load animations
anim_idle = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Animations/Anim_Operator_LowReady")
anim_walk = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Animations/Anim_Operator_Walk")
anim_run = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Animations/Anim_Operator_Run")
anim_sprint = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Animations/Anim_Operator_Sprint")
anim_back = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Animations/Anim_Operator_Walk_Backwards")
anim_crouch_walk = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Animations/Anim_Operator_Crouch_Walk")

unreal.log_warning(f"Animations: idle={anim_idle}, walk={anim_walk}, run={anim_run}, sprint={anim_sprint}, back={anim_back}")

unreal.EditorAssetLibrary.save_loaded_asset(bs)
unreal.log_warning("Saved BS_BaseMale_Locomotion successfully!")
sys.exit(0)
