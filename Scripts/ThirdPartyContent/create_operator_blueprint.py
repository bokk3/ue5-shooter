"""Create /Game/NobleFermi/Characters/BP_FPSOperator.

The Blueprint is a copy of the Game Animation Sample's SandboxCharacter_CMC re-parented onto our C++
AFPSCharacter. The sample Blueprint keeps owning locomotion (motion-matched AnimBP, Enhanced Input
movement, gait speeds, camera); AFPSCharacter adds combat, health, netcode and Fermi abilities.

Run headless:
    UnrealEditor-Cmd.exe NobleFermi.uproject -run=pythonscript -script=<this file> -unattended -nullrhi
"""
import unreal

SOURCE = "/Game/Blueprints/SandboxCharacter_CMC"
DEST_DIR = "/Game/NobleFermi/Characters"
DEST_NAME = "BP_FPSOperator"
DEST = f"{DEST_DIR}/{DEST_NAME}"

log = unreal.log
eal = unreal.EditorAssetLibrary
bel = unreal.BlueprintEditorLibrary

if eal.does_asset_exist(DEST):
    log(f"[Operator] {DEST} already exists; re-parenting and updating it in place")
    bp = unreal.load_asset(DEST)
else:
    if not eal.does_asset_exist(SOURCE):
        raise RuntimeError(f"{SOURCE} is missing; import the Game Animation Sample content first")
    bp = eal.duplicate_asset(SOURCE, DEST)
    log(f"[Operator] Duplicated {SOURCE} -> {DEST}")

parent = unreal.load_class(None, "/Script/NobleFermi.FPSCharacter")
if parent is None:
    raise RuntimeError("AFPSCharacter class not found; build the NobleFermiEditor target first")

bel.reparent_blueprint(bp, parent)
log(f"[Operator] Parent class is now {bel.get_blueprint_parent_class(bp).get_name()}")

bel.compile_blueprint(bp)

cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("bUseSampleLocomotion", True)

mesh = cdo.get_editor_property("mesh")
anim_class = mesh.get_editor_property("anim_class") if mesh else None
log("[Operator] Mesh asset: %s" % (mesh.get_editor_property("skeletal_mesh_asset").get_path_name() if mesh and mesh.get_editor_property("skeletal_mesh_asset") else "None"))
log("[Operator] Anim class: %s" % (anim_class.get_path_name() if anim_class else "None"))
log("[Operator] bUseSampleLocomotion: %s" % cdo.get_editor_property("bUseSampleLocomotion"))

bel.compile_blueprint(bp)
eal.save_asset(DEST, only_if_is_dirty=False)
log("[Operator] OPERATOR_BP_DONE")
