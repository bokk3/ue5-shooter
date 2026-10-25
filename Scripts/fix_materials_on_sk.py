import unreal
import sys

print(">>> ASSIGNING MATERIALS TO SK_BASEMALE <<<")

sk_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if not sk_mesh:
    print("ERROR: Could not load SK_BaseMale!")
    sys.exit(1)

body_mat = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody")
visor_mat = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_Soldier_Visor")

if not body_mat:
    print("ERROR: Could not load M_Operator_BaseBody!")
    sys.exit(1)

if not visor_mat:
    print("ERROR: Could not load M_Soldier_Visor!")
    sys.exit(1)

# Ensure materials have used_with_skeletal_mesh = True
body_mat.set_editor_property("used_with_skeletal_mesh", True)
visor_mat.set_editor_property("used_with_skeletal_mesh", True)
unreal.EditorAssetLibrary.save_loaded_asset(body_mat, only_if_is_dirty=False)
unreal.EditorAssetLibrary.save_loaded_asset(visor_mat, only_if_is_dirty=False)
print("Ensured used_with_skeletal_mesh = True on both materials")

# Assign materials to slots
try:
    sk_mesh.set_material(0, body_mat)
    print("Called set_material(0, body_mat)")
except Exception as e:
    print("set_material(0) failed:", e)

try:
    sk_mesh.set_material(1, visor_mat)
    print("Called set_material(1, visor_mat)")
except Exception as e:
    print("set_material(1) failed:", e)

# Also update the materials array directly
mats = sk_mesh.get_editor_property("materials")
if len(mats) > 0:
    m0 = mats[0]
    m0.material_interface = body_mat
    mats[0] = m0
if len(mats) > 1:
    m1 = mats[1]
    m1.material_interface = visor_mat
    mats[1] = m1
sk_mesh.set_editor_property("materials", mats)

unreal.EditorAssetLibrary.save_loaded_asset(sk_mesh, only_if_is_dirty=False)
print("Saved SK_BaseMale successfully!")

# Verify
mats_after = sk_mesh.get_editor_property("materials")
for i, m in enumerate(mats_after):
    print(f"Slot {i}: {m.material_slot_name} -> {m.material_interface.get_name() if m.material_interface else 'None'}")

out_path = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Intermediate\mat_assign_verify.txt"
with open(out_path, "w") as f:
    for i, m in enumerate(mats_after):
        f.write(f"Slot {i}: {m.material_slot_name} -> {m.material_interface.get_name() if m.material_interface else 'None'}\n")

sys.exit(0)
