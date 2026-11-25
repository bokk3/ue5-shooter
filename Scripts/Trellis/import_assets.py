"""UE (headless): import the cleaned Trellis models (clean_assets.py output) as static meshes.

  set NF_TRELLIS_CLEAN=D:\\epuc\\Projects\\noble-fermi\\SourceArt\\Trellis\\_Clean
  [set NF_TRELLIS_ONLY=NF98_Carbine,Pallet]
  UnrealEditor-Cmd.exe NobleFermi.uproject -run=pythonscript -script=Scripts/Trellis/import_assets.py -unattended -nullrhi

Each <batch>/<category>/<Name>.glb lands in /Game/NobleFermi/Generated/<batch>/<category>/ as SM_<Name> (Interchange
puts it in <Name>/StaticMeshes with its materials and textures beside it). The models are already at real scale
with forward +X and up +Z, so the default glTF pipeline imports them as they are. Per asset_specs kind:
  held   - no collision, no Nanite, textures capped at 1K
  pickup - box collision, no Nanite, 1K
  prop   - convex-hull collision, Nanite, 2K
  big    - complex-as-simple collision, Nanite, 2K
Logs [Trellis] lines with each mesh's bounds in cm.
"""
import glob
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from asset_specs import S  # noqa: E402

CLEAN = os.environ.get("NF_TRELLIS_CLEAN", "")
ONLY = {n for n in os.environ.get("NF_TRELLIS_ONLY", "").split(",") if n}
DEST_ROOT = "/Game/NobleFermi/Generated"
TEXTURE_CAP = {"held": 1024, "pickup": 1024, "prop": 2048, "big": 2048}
log = unreal.log

CONFIGURE_ONLY = os.environ.get("NF_TRELLIS_CONFIGURE_ONLY") == "1"  # re-apply settings to already imported meshes
# The commandlet has no editor subsystems registered, and the deprecated EditorStaticMeshLibrary forwards to
# that missing subsystem and does nothing; a directly created instance works.
mesh_lib = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.StaticMeshEditorSubsystem()
asset_lib = unreal.EditorAssetLibrary


def import_glb(path, dest, name):
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = dest
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return [unreal.load_asset(p) for p in task.get_editor_property("imported_object_paths") or []]


def set_nanite(mesh, enabled):
    settings = mesh.get_editor_property("nanite_settings")
    settings.set_editor_property("enabled", enabled)
    mesh_lib.set_nanite_settings(mesh, settings, apply_changes=True)


def set_collision(mesh, kind):
    mesh_lib.remove_collisions(mesh)
    body = mesh.get_editor_property("body_setup")
    if kind == "held":
        return "none"
    if kind == "pickup":
        mesh_lib.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
        return "box"
    if kind == "prop":
        mesh_lib.set_convex_decomposition_collisions(mesh, 8, 24, 300000)
        return "convex x8"
    if body:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    return "complex"


def cap_textures(folder, cap):
    n = 0
    for p in asset_lib.list_assets(folder, recursive=True, include_folder=False):
        tex = unreal.load_asset(p)
        if isinstance(tex, unreal.Texture2D):
            tex.set_editor_property("max_texture_size", cap)
            n += 1
    return n


def main():
    if not os.path.isdir(CLEAN):
        raise RuntimeError(f"NF_TRELLIS_CLEAN not found: '{CLEAN}'")
    done, failed = 0, []
    for path in sorted(glob.glob(os.path.join(CLEAN, "0*", "*", "*.glb"))):
        name = os.path.splitext(os.path.basename(path))[0]
        sp = S.get(name)
        if not sp or (ONLY and name not in ONLY):
            continue
        batch, category = os.path.relpath(path, CLEAN).replace("\\", "/").split("/")[:2]
        dest = f"{DEST_ROOT}/{batch}/{category}"
        try:
            if CONFIGURE_ONLY:
                meshes = [unreal.load_asset(f"{dest}/{name}/StaticMeshes/SM_{name}")]
            else:
                meshes = import_glb(path, dest, f"SM_{name}")
            meshes = [a for a in meshes if isinstance(a, unreal.StaticMesh)]
            if not meshes:
                raise RuntimeError("no static mesh imported")
            mesh = meshes[0]
            kind = sp["kind"]
            set_nanite(mesh, kind in ("prop", "big"))
            collision = set_collision(mesh, kind)
            folder = mesh.get_path_name().rsplit("/", 2)[0]  # .../<Name>/StaticMeshes/SM_x -> .../<Name>
            textures = cap_textures(folder, TEXTURE_CAP[kind])
            asset_lib.save_directory(folder, only_if_is_dirty=False, recursive=True)
            box = mesh.get_bounding_box()
            size = box.max - box.min
            collision += f" ({mesh_lib.get_simple_collision_count(mesh)} simple, {mesh_lib.get_convex_collision_count(mesh)} convex)"
            log(f"[Trellis] {name}: {mesh.get_path_name()} size ({size.x:.1f}, {size.y:.1f}, {size.z:.1f}) cm "
                f"min z {box.min.z:.1f} kind {kind} collision {collision} textures {textures}@{TEXTURE_CAP[kind]} "
                f"nanite {mesh.get_editor_property('nanite_settings').enabled}")
            done += 1
        except Exception as e:  # noqa: BLE001  keep going; report it
            failed.append(name)
            unreal.log_error(f"[Trellis] {name} failed: {e}")
    log(f"[Trellis] TRELLIS_DONE imported {done}, failed {len(failed)}: {failed}")


main()
