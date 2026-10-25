import unreal
import sys

print(">>> ASSIGNING SOLDIER MATERIALS TO SK_BASEMALE <<<")

sk_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if not sk_mesh:
    print("ERROR: Could not load SK_BaseMale!")
    sys.exit(1)

body_mat = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody")
visor_mat = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_Soldier_Visor")

skeletal_mats = sk_mesh.get_editor_property("materials")
print(f"Material slots count: {len(skeletal_mats)}")
for i, m in enumerate(skeletal_mats):
    print(f"Slot {i}: {m.material_slot_name} -> {m.material_interface.get_name() if m.material_interface else 'None'}")

if len(skeletal_mats) > 0 and body_mat:
    skeletal_mats[0].material_interface = body_mat
    print("Assigned body_mat to slot 0")

if len(skeletal_mats) > 1 and visor_mat:
    skeletal_mats[1].material_interface = visor_mat
    print("Assigned visor_mat to slot 1")

sk_mesh.set_editor_property("materials", skeletal_mats)
unreal.EditorAssetLibrary.save_loaded_asset(sk_mesh, only_if_is_dirty=False)
print("SUCCESSFULLY ASSIGNED MATERIALS AND SAVED SK_BASEMALE")
sys.exit(0)
