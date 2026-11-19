import unreal

sk = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if not sk:
    print("Could not load SK_BaseMale")
    quit()

skel = sk.get_editor_property("skeleton")
print(f"Skeleton: {skel.get_name()}")

# Let's inspect bone hierarchy and rest pose transforms
# In UE, we can inspect reference skeleton bones
num_bones = unreal.SkeletonModifier.get_num_bones(sk) if hasattr(unreal, "SkeletonModifier") else 0
print(f"Num bones: {num_bones}")

# Print bone names
for bone_name in ["hand_r", "index_01_r", "index_02_r", "middle_01_r", "thumb_01_r", "hand_l", "index_01_l"]:
    # Let's check if we can get bone transform or socket
    print(f"Bone: {bone_name}")
