"""Add Noble Fermi's weapon layer to the Game Animation Sample's anim blueprint and use it on the operator.

The sample's SandboxCharacter_CMC_ABP is edited in place (it is third-party content that this script
regenerates, see README.md). It has to stay the same class: the sample's chooser tables select the
motion-matching databases with the ABP as context object and only accept SandboxCharacter_CMC_ABP_C,
so a renamed copy loses its locomotion. The ABP is re-parented onto UFPSOperatorAnimInstance and a
weapon layer is inserted before its Output Pose by the NobleFermiEditor module
(UFPSAnimGraphTools::AddWeaponUpperBodyLayer):

    sample locomotion ---------------------------------------------+
    Lyra hold pose -> aim offset -> [NF_UpperBody slot] -> layered blend from spine_01 -> [NF_FullBody slot] -> output
    (rifle, or pistol when bUsePistolPose)

Needs the sample and Lyra content and a built NobleFermiEditor target. Re-running replaces the layer.
    UnrealEditor-Cmd.exe NobleFermi.uproject -run=pythonscript -script=<this file> -unattended -nullrhi
Set NF_ANIM_DUMP=<file> to write a text dump of the graph before and after.
"""
import os

import unreal

ABP = "/Game/Blueprints/SandboxCharacter_CMC_ABP"
OLD_COPY = "/Game/NobleFermi/Characters/ABP_FPSOperator"  # earlier approach; removed
OPERATOR_BP = "/Game/NobleFermi/Characters/BP_FPSOperator"
PARENT = "/Script/NobleFermi.FPSOperatorAnimInstance"
LYRA = "/Game/Characters/UEFN_Mannequin/Animations/Lyra"

log = unreal.log
eal = unreal.EditorAssetLibrary
bel = unreal.BlueprintEditorLibrary
tools = unreal.FPSAnimGraphTools
dump_path = os.environ.get("NF_ANIM_DUMP")


def load(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"{path} is missing; see Scripts/ThirdPartyContent/README.md")
    return asset


abp = load(ABP)
if dump_path:
    with open(dump_path, "w", encoding="utf-8") as f:
        f.write(tools.dump_anim_graph(abp))

parent = unreal.load_class(None, PARENT)
if parent is None:
    raise RuntimeError("UFPSOperatorAnimInstance not found; build the NobleFermiEditor target first")
if bel.get_blueprint_parent_class(abp) != parent:
    bel.reparent_blueprint(abp, parent)
    log(f"[OperatorABP] parent is now {bel.get_blueprint_parent_class(abp).get_name()}")

settings = unreal.FPSWeaponLayerSettings()
settings.rifle_pose = load(f"{LYRA}/Locomotion/Rifle/MM_Rifle_Idle_Hipfire")
settings.rifle_aim_offset = load(f"{LYRA}/AimOffsets/AO_MM_Rifle_Idle_Hipfire")
settings.pistol_pose = load(f"{LYRA}/Locomotion/Pistol/MM_Pistol_Idle_Hipfire")
settings.pistol_aim_offset = load(f"{LYRA}/AimOffsets/AO_MM_Pistol_Idle_ADS")

# Unreal's Python returns only the out-param for a bool function, and None when it returns false.
layer_log = tools.add_weapon_upper_body_layer(abp, settings)
ok = layer_log is not None
layer_log = layer_log or "add_weapon_upper_body_layer returned false (details in LogPython above)"
for line in layer_log.splitlines():
    log(f"[OperatorABP] {line}")
if dump_path:
    with open(dump_path, "a", encoding="utf-8") as f:
        f.write("\n==== after ====\n" + tools.dump_anim_graph(abp) + "\n==== log ====\n" + layer_log)
if not ok:
    raise RuntimeError("weapon layer failed; see the log above")

eal.save_asset(ABP, only_if_is_dirty=False)
skeleton = abp.get_editor_property("target_skeleton")
if skeleton:
    eal.save_asset(skeleton.get_path_name().split(".")[0], only_if_is_dirty=False)

# Point the operator at the sample ABP again (an earlier version of this script used a copy).
operator = load(OPERATOR_BP)
cdo = unreal.get_default_object(operator.generated_class())
mesh = cdo.get_editor_property("mesh")
mesh.set_editor_property("anim_class", abp.generated_class())
bel.compile_blueprint(operator)
eal.save_asset(OPERATOR_BP, only_if_is_dirty=False)
log(f"[OperatorABP] {OPERATOR_BP} anim class: {mesh.get_editor_property('anim_class').get_path_name()}")
if eal.does_asset_exist(OLD_COPY):
    log(f"[OperatorABP] removing the old copy {OLD_COPY}: {eal.delete_asset(OLD_COPY)}")
log("[OperatorABP] OPERATOR_ABP_DONE")
