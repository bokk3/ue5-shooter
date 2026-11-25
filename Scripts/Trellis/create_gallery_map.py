"""Build /Game/NobleFermi/Maps/L_AssetGallery: every imported Trellis mesh laid out for inspection.

One row per batch (01 Weapons nearest the spawn, 08 Range/Hangar furthest), rows running along Y, the player
spawning at X < 0 looking +X. Props face the player (yaw 180); weapons, attachments and magazines are turned
side-on with the muzzle to the player's right (yaw 90). Held items and pickups stand on 1 m pedestals. Each row
starts with a 180 cm reference post, and every item has a label with its name and size. Shot_<row>_<n> cameras
frame groups of items for capture_gallery.py.

Play it with the operator for scale (no arena is spawned with Theater=Range):
    UnrealEditor.exe NobleFermi.uproject /Game/NobleFermi/Maps/L_AssetGallery?Theater=Range?Bots=0 -game
Build headless (re-running rebuilds it):
    UnrealEditor-Cmd.exe NobleFermi.uproject -run=pythonscript -script=Scripts/Trellis/create_gallery_map.py -unattended -nullrhi
"""
import math
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from asset_specs import S  # noqa: E402

MAP = "/Game/NobleFermi/Maps/L_AssetGallery"
GENERATED = "/Game/NobleFermi/Generated"
GAME_MODE = "/Script/NobleFermi.FPSMultiplayerGameMode"
CUBE = "/Engine/BasicShapes/Cube"
GRID = "/Game/Levels/LevelPrototyping/Materials/M_Grid"
SOLID = "/Game/Levels/LevelPrototyping/Materials/MI_Solid_Blue"
SIDE_ON = {"Weapons", "Melee", "Magazines", "Optics", "Muzzle", "Underbarrel", "Rail", "IronSights"}
PEDESTAL_H = 100.0
GAP = 60.0          # between items in a row
ROW_GAP = 700.0     # walking space between rows
SHOT_GROUP = {"held": 4, "pickup": 4, "prop": 6, "big": 4}  # items per camera, by the row's main kind
CAMERA_FOV = 60.0

log = unreal.log
eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
V, R = unreal.Vector, unreal.Rotator

cube = unreal.load_asset(CUBE)
grid = unreal.load_asset(GRID)
solid = unreal.load_asset(SOLID)

if eal.does_asset_exist(MAP):
    if not les.load_level(MAP):
        raise RuntimeError(f"could not open {MAP}")
    eas.destroy_actors(eas.get_all_level_actors())
elif not les.new_level(MAP):
    raise RuntimeError(f"could not create {MAP}")


def label(actor, name, folder):
    actor.set_actor_label(name)
    actor.set_folder_path(folder)
    return actor


def bounds(actor):
    origin, extent = actor.get_actor_bounds(False)
    return origin, extent


def place(actor, x, y, z):
    """Moves an actor so its bounds are centred on (x, y) with the bottom at height z."""
    origin, extent = bounds(actor)
    loc = actor.get_actor_location()
    actor.set_actor_location(V(loc.x + x - origin.x, loc.y + y - origin.y, loc.z + z - (origin.z - extent.z)), False, True)


def box(name, folder, x, y, size, z=0.0, material=None):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, V(x, y, 1000), R(roll=0, pitch=0, yaw=0))
    smc = a.static_mesh_component
    smc.set_static_mesh(cube)
    if material:
        smc.set_material(0, material)
    a.set_actor_scale3d(V(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    place(a, x, y, z)
    return label(a, name, folder)


def text(name, folder, x, y, z, words, size):
    a = eas.spawn_actor_from_class(unreal.TextRenderActor, V(x, y, z), R(roll=0, pitch=0, yaw=180))
    t = a.text_render
    t.set_text(words)
    t.set_world_size(size)
    t.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    t.set_text_render_color(unreal.Color(255, 220, 80, 255))
    return label(a, name, folder)


def find_meshes():
    """(batch, category, name, mesh) for every spec that has an imported mesh, batch by batch."""
    rows = {}
    for path in eal.list_assets(GENERATED, recursive=True, include_folder=False):
        asset_name = path.rsplit("/", 1)[-1].split(".")[0]
        if not asset_name.startswith("SM_"):
            continue
        name = asset_name[3:]
        if name not in S:
            continue
        batch, category = path[len(GENERATED) + 1:].split("/")[:2]
        rows.setdefault(batch, []).append((category, name, unreal.load_asset(path)))
    order = list(S)
    return {b: sorted(items, key=lambda i: order.index(i[1])) for b, items in sorted(rows.items())}


# --- Lighting (as L_Range) -------------------------------------------------------------------------------
sun = label(eas.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 1000), R(pitch=-45, yaw=-30, roll=0)), "Sun", "Lighting")
sun.light_component.set_editor_property("atmosphere_sun_light", True)
sun.light_component.set_intensity(8.0)
sky = label(eas.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 800), R(roll=0, pitch=0, yaw=0)), "SkyLight", "Lighting")
sky.light_component.set_editor_property("real_time_capture", True)
label(eas.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0), R(roll=0, pitch=0, yaw=0)), "SkyAtmosphere", "Lighting")
fog = label(eas.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, -200), R(roll=0, pitch=0, yaw=0)), "HeightFog", "Lighting")
fog.component.set_editor_property("fog_density", 0.002)

# --- Rows ---------------------------------------------------------------------------------------------
row_x = 0.0
max_half_width = 0.0
shots = 0
placed = 0
for batch, items in find_meshes().items():
    folder = f"Gallery/{batch}"
    spawned = []
    for category, name, mesh in items:
        kind = S[name]["kind"]
        yaw = 90.0 if category in SIDE_ON else 180.0
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, V(0, 0, 5000), R(roll=0, pitch=0, yaw=yaw))
        a.static_mesh_component.set_static_mesh(mesh)
        label(a, name, folder)
        _, ext = bounds(a)
        spawned.append((name, kind, a, ext))
    depth = max(e.x for _, _, _, e in spawned) * 2
    width = sum(max(e.y * 2, 30.0) for _, _, _, e in spawned) + GAP * (len(spawned) + 1) + 100
    centre_x = row_x + depth / 2
    y = -width / 2
    box(f"Ref180_{batch}", folder, centre_x, y + 20, (40, 40, 180), material=solid)
    text(f"Label_Ref180_{batch}", folder, centre_x - 40, y + 20, 200, "180 cm", 24)
    text(f"Title_{batch}", folder, row_x - 150, 0, 10, batch.replace("_", " "), 80)
    y += 100 + GAP
    group = []
    kinds = [k for _, k, _, _ in spawned]
    per_shot = SHOT_GROUP[max(set(kinds), key=kinds.count)]
    for name, kind, a, ext in spawned:
        w = max(ext.y * 2, 30.0)
        cy = y + w / 2
        base = 0.0
        if kind in ("held", "pickup"):
            box(f"Pedestal_{name}", folder, centre_x, cy, (max(ext.x * 2 + 10, 30), w + 10, PEDESTAL_H), material=grid)
            base = PEDESTAL_H
        place(a, centre_x, cy, base)
        size = f"{ext.x * 2:.0f} x {ext.y * 2:.0f} x {ext.z * 2:.0f} cm"
        text_size = max(6.0, min(40.0, w * 0.12))
        text(f"Label_{name}", folder, centre_x - ext.x - 15, cy, base + ext.z * 2 + text_size * 2.5 + 5,
             f"{name}\n{size}", text_size)
        group.append((cy, w, base + ext.z * 2, ext.x, base))
        y += w + GAP
        placed += 1
        if len(group) == per_shot or (name, kind, a, ext) == spawned[-1]:
            lo = group[0][0] - group[0][1] / 2
            hi = group[-1][0] + group[-1][1] / 2
            bottom = min(g[4] for g in group)
            top = max(g[2] for g in group)
            # Frame the group's width (16:9) and the height from its pedestals to its tallest item.
            half = max((hi - lo) / 2 * 1.15, (top - bottom) * 0.9)
            dist = half / math.tan(math.radians(CAMERA_FOV / 2)) + max(g[3] for g in group)
            pitch = 12.0
            aim_z = (bottom + top) / 2
            cam = eas.spawn_actor_from_class(unreal.CameraActor,
                                             V(centre_x - dist, (lo + hi) / 2, aim_z + dist * math.tan(math.radians(pitch))),
                                             R(roll=0, pitch=-pitch, yaw=0))
            cam.camera_component.set_editor_property("field_of_view", CAMERA_FOV)
            label(cam, f"Shot_{batch}_{shots:02d}", "Gallery/Shots")
            shots += 1
            group = []
    max_half_width = max(max_half_width, width / 2)
    row_x += depth + ROW_GAP

# --- Overview from above the spawn ---------------------------------------------------------------------
overview = eas.spawn_actor_from_class(unreal.CameraActor, V(-2500, 0, 3500), R(roll=0, pitch=-35, yaw=0))
overview.camera_component.set_editor_property("field_of_view", 75.0)
label(overview, "Shot_00_Overview", "Gallery/Shots")

# --- Floor and spawn -----------------------------------------------------------------------------------
floor_len = row_x + 1500
box("Floor", "Geometry", floor_len / 2 - 1000, 0, (floor_len, max_half_width * 2 + 2000, 100), z=-100, material=grid)
start = label(eas.spawn_actor_from_class(unreal.PlayerStart, V(-600, 0, 100), R(roll=0, pitch=0, yaw=0)), "PlayerStart_Alpha_0", "Spawns")
start.set_editor_property("player_start_tag", "TeamAlpha")
start.tags = ["TeamAlpha"]

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, GAME_MODE))
les.save_current_level()
log(f"[Gallery] GALLERY_DONE {MAP}: {placed} meshes, {shots} cameras, {row_x / 100:.0f} m deep, {max_half_width * 2 / 100:.0f} m wide")
