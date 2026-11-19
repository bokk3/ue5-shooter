import unreal

sk = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if not sk:
    print("Could not load SK_BaseMale")
    quit()

skel = sk.get_editor_property("skeleton")
print(f"Loaded SK_BaseMale, Skeleton: {skel.get_name() if skel else 'None'}")

# We can inspect bone names and positions from the reference skeleton using Python
# Let's inspect using AnimationLibrary if available
num_bones = unreal.SkeletonModifier.get_num_bones(sk) if hasattr(unreal, "SkeletonModifier") else 0
print(f"Num bones: {num_bones}")
