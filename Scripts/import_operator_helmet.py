"""Import a combat helmet model (textured glTF/GLB, e.g. from Trellis) at real-world scale for the operator.

Generated models come normalized to about a 1 m cube, so the import scales the mesh until its longest side
is NF_HELMET_LENGTH_CM (default 29 cm, the front-to-back length of a typical ballistic helmet including the
brim) and turns it to face +X (NF_HELMET_YAW, default -90 for glTF's +Z front). AFPSCharacter attaches it
to the head bone without collision (OperatorHelmetMesh / OperatorHelmetOnHead).

Also logs the operator's head (bone position in a standing pose and the mesh top) to place the helmet.

Run headless:
  set NF_HELMET_SOURCE=C:\\path\\to\\helmet.glb
  UnrealEditor-Cmd.exe NobleFermi.uproject -run=pythonscript -script=Scripts/import_operator_helmet.py -unattended -nullrhi
"""
import os

import unreal

SOURCE = os.environ.get("NF_HELMET_SOURCE", "")
DEST = os.environ.get("NF_HELMET_DEST", "/Game/NobleFermi/Characters/Gear/Helmet")
NAME = os.environ.get("NF_HELMET_NAME", "SM_CombatHelmet")
LENGTH_CM = float(os.environ.get("NF_HELMET_LENGTH_CM", "29"))
OPERATOR_BP = "/Game/NobleFermi/Characters/BP_FPSOperator"
STANDING_POSE = "/Game/Characters/UEFN_Mannequin/Animations/Lyra/Locomotion/Rifle/MM_Rifle_Idle_Hipfire"
log = unreal.log


def fmt(v):
    return f"({v.x:.2f}, {v.y:.2f}, {v.z:.2f})"


GLTF_ASSETS_PIPELINE = "/Interchange/Pipelines/DefaultGLTFAssetsPipeline.DefaultGLTFAssetsPipeline"
GLTF_PIPELINE = "/Interchange/Pipelines/DefaultGLTFPipeline.DefaultGLTFPipeline"
# glTF models face +Z; Interchange brings that to +Y. UE assets face +X.
IMPORT_ROTATION = unreal.Rotator(roll=0.0, pitch=0.0, yaw=float(os.environ.get("NF_HELMET_YAW", "-90")))


def source_longest_side_m():
    """Longest side of the model's bounds, from the GLB's POSITION accessors (metres)."""
    import json
    import struct
    with open(SOURCE, "rb") as f:
        data = f.read()
    length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20 + length])
    lo, hi = [1e9] * 3, [-1e9] * 3
    for mesh in gltf["meshes"]:
        for prim in mesh["primitives"]:
            acc = gltf["accessors"][prim["attributes"]["POSITION"]]
            lo = [min(a, b) for a, b in zip(lo, acc["min"])]
            hi = [max(a, b) for a, b in zip(hi, acc["max"])]
    return max(h - l for h, l in zip(hi, lo))


def import_helmet():
    if not os.path.isfile(SOURCE):
        raise RuntimeError(f"NF_HELMET_SOURCE not found: '{SOURCE}'")
    scale = LENGTH_CM / (source_longest_side_m() * 100.0)

    # The project's default glTF pipelines, with the scale and facing applied at import. The pipeline asset is
    # only changed in memory (Interchange works on a copy); nothing under /Interchange is saved.
    assets_pipeline = unreal.load_asset(GLTF_ASSETS_PIPELINE)
    assets_pipeline.set_editor_property("import_offset_uniform_scale", scale)
    assets_pipeline.set_editor_property("import_offset_rotation", IMPORT_ROTATION)
    stack = unreal.InterchangePipelineStackOverride()
    stack.add_pipeline(assets_pipeline)
    stack.add_pipeline(unreal.load_asset(GLTF_PIPELINE))

    task = unreal.AssetImportTask()
    task.filename = SOURCE
    task.destination_path = DEST
    task.destination_name = NAME
    task.automated = True
    task.replace_existing = True
    task.save = False
    task.options = stack
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported = list(task.get_editor_property("imported_object_paths") or [])
    log(f"[Helmet] imported with scale {scale:.4f}, rotation {IMPORT_ROTATION}: {imported}")
    meshes = [m for m in (unreal.load_asset(p) for p in imported) if isinstance(m, unreal.StaticMesh)]
    if not meshes:
        raise RuntimeError("no static mesh imported")
    mesh = meshes[0]
    box = mesh.get_bounding_box()
    log(f"[Helmet] bounds min {fmt(box.min)} max {fmt(box.max)} size {fmt(box.max - box.min)} (X forward, Z up)")
    return mesh


def measure_head():
    alib = unreal.AnimationLibrary
    anim = unreal.load_asset(STANDING_POSE)
    if not anim:
        log("[Helmet] standing pose not found; skipping head measurement")
        return

    def component_space(bone):
        path = list(alib.find_bone_path_to_root(anim, bone))
        xf = None
        for name in path:
            local = alib.get_bone_pose_for_frame(anim, name, 0, False)
            xf = local if xf is None else xf.multiply(local)
        return xf

    head = component_space("head")
    neck = component_space("neck_01")
    log(f"[Helmet] head bone (component space, standing): {fmt(head.translation)} rot {head.rotation.rotator()}")
    log(f"[Helmet] neck_01: {fmt(neck.translation)}")
    bp = unreal.load_asset(OPERATOR_BP)
    cdo = unreal.get_default_object(bp.generated_class()) if bp else None
    mesh_comp = cdo.get_editor_property("mesh") if cdo else None
    skm = mesh_comp.get_editor_property("skeletal_mesh_asset") if mesh_comp else None
    if skm:
        b = skm.get_bounds()
        log(f"[Helmet] operator mesh {skm.get_name()}: bounds origin {fmt(b.origin)} extent {fmt(b.box_extent)} (top z {b.origin.z + b.box_extent.z:.2f})")


mesh = import_helmet()
for i, slot in enumerate(mesh.get_editor_property("static_materials")):
    log(f"[Helmet] material {i}: {slot.get_editor_property('material_interface')}")
unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)
log(f"[Helmet] saved {mesh.get_path_name()}")
try:
    measure_head()
except Exception as e:  # measurement is informational
    log(f"[Helmet] head measurement failed: {e}")
log("[Helmet] HELMET_DONE")
