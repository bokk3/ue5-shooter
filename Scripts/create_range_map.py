"""Build /Game/NobleFermi/Maps/L_Range: a small, cheap map for checking player mechanics.

Layout (X is forward from the spawn line, Y is right; 1 unit = 1 cm):
  * spawn line at X = -1500: four TeamAlpha player starts facing +X
  * firing lane (Y = 0): man-sized targets at 10, 25, 50 and 100 m, each with a distance label
  * traversal course (Y = -1500): Game Animation Sample traversable blocks, low hurdle to high mantle
  * cover course (Y = +1500): crouch-height and standing-height walls, pillars, a ramp
  * two TeamBravo starts far down range for bots (?Bots=N, default 2)
  * sun, sky light, sky atmosphere and height fog; no Lumen-heavy content, no landscape

Uses the sample's level-prototyping kit (grid material, traversable level blocks), so the sample content
must be imported (Scripts/ThirdPartyContent/README.md). The game mode treats any map named L_Range as
the Range theater (no procedural arena). Re-running rebuilds the map from scratch. Run headless:
    UnrealEditor-Cmd.exe NobleFermi.uproject -run=pythonscript -script=<this file> -unattended -nullrhi
"""
import unreal

MAP = "/Game/NobleFermi/Maps/L_Range"
GAME_MODE = "/Script/NobleFermi.FPSMultiplayerGameMode"
CUBE = "/Engine/BasicShapes/Cube"                          # 100 cm, centred pivot
GRID = "/Game/Levels/LevelPrototyping/Materials/M_Grid"
SOLID = "/Game/Levels/LevelPrototyping/Materials/MI_Solid_Blue"
TRAVERSABLE = "/Game/Levels/LevelPrototyping/LevelBlock_Traversable"
LEVEL_VISUALS = "/Game/Levels/LevelPrototyping/LevelVisuals"
SPAWN_X = -1500.0

log = unreal.log
eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

cube = unreal.load_asset(CUBE)
grid = unreal.load_asset(GRID)
solid = unreal.load_asset(SOLID)
traversable = unreal.load_asset(TRAVERSABLE)
log(f"[Range] traversable block asset: {traversable} ({type(traversable).__name__ if traversable else 'missing'})")

if eal.does_asset_exist(MAP):
    # Deleting a map from a commandlet doesn't stick; open it and start from an empty level instead.
    if not les.load_level(MAP):
        raise RuntimeError(f"could not open {MAP}")
    eas.destroy_actors(eas.get_all_level_actors())
elif not les.new_level(MAP):
    raise RuntimeError(f"could not create {MAP}")

V, R = unreal.Vector, unreal.Rotator


def label(actor, text, folder):
    actor.set_actor_label(text)
    actor.set_folder_path(folder)
    return actor


def ground(actor, x, y, z=0.0):
    """Moves an actor so the bottom of its bounds sits at height z, centred on (x, y)."""
    origin, extent = actor.get_actor_bounds(False)
    loc = actor.get_actor_location()
    actor.set_actor_location(V(loc.x + x - origin.x, loc.y + y - origin.y, loc.z + z - (origin.z - extent.z)), False, True)
    return actor


def box(name, folder, x, y, size, material=solid, yaw=0.0, pitch=0.0, z=0.0):
    """Static cube of size (cm) standing on the floor at (x, y)."""
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, V(x, y, 500), R(pitch=pitch, yaw=yaw, roll=0))
    smc = a.static_mesh_component
    smc.set_static_mesh(cube)
    smc.set_material(0, material)
    a.set_actor_scale3d(V(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    return label(ground(a, x, y, z), name, folder)


def block(name, folder, x, y, size):
    """Traversable level block (vault/hurdle/mantle) from the sample, scaled to size (cm)."""
    if traversable is None:
        return box(name, folder, x, y, size)
    a = eas.spawn_actor_from_class(traversable.generated_class(), V(x, y, 500), R(roll=0, pitch=0, yaw=0))
    origin, extent = a.get_actor_bounds(False)
    base = V(max(extent.x * 2, 1), max(extent.y * 2, 1), max(extent.z * 2, 1))
    scale = a.get_actor_scale3d()
    a.set_actor_scale3d(V(scale.x * size[0] / base.x, scale.y * size[1] / base.y, scale.z * size[2] / base.z))
    return label(ground(a, x, y), name, folder)


def text(name, folder, x, y, z, words, size=60.0):
    a = eas.spawn_actor_from_class(unreal.TextRenderActor, V(x, y, z), R(roll=0, pitch=0, yaw=180))  # faces the spawn line
    t = a.text_render
    t.set_text(words)
    t.set_world_size(size)
    t.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    t.set_text_render_color(unreal.Color(255, 220, 80, 255))
    return label(a, name, folder)


def start(name, x, y, yaw, tag):
    a = eas.spawn_actor_from_class(unreal.PlayerStart, V(x, y, 100), R(roll=0, pitch=0, yaw=yaw))
    a.set_editor_property("player_start_tag", tag)
    a.tags = [tag]
    return label(a, name, "Spawns")


# --- Lighting ----------------------------------------------------------------------------------
sun = label(eas.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 1000), R(pitch=-50, yaw=35, roll=0)), "Sun", "Lighting")
sun.light_component.set_editor_property("atmosphere_sun_light", True)
sun.light_component.set_intensity(8.0)
sky = label(eas.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 800), R(roll=0, pitch=0, yaw=0)), "SkyLight", "Lighting")
sky.light_component.set_editor_property("real_time_capture", True)
label(eas.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0), R(roll=0, pitch=0, yaw=0)), "SkyAtmosphere", "Lighting")
fog = label(eas.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, -200), R(roll=0, pitch=0, yaw=0)), "HeightFog", "Lighting")
fog.component.set_editor_property("fog_density", 0.004)

# The sample's level blocks look up its LevelVisuals actor (colour style) in their construction script.
visuals = unreal.load_asset(LEVEL_VISUALS)
if visuals:
    label(eas.spawn_actor_from_class(visuals.generated_class(), V(SPAWN_X - 400, 0, 0), R(roll=0, pitch=0, yaw=0)), "LevelVisuals", "Lighting")
else:
    log(f"[Range] {LEVEL_VISUALS} missing; level blocks will warn in their construction script")

# --- Floor: 140 m x 60 m grid, top at z = 0 ------------------------------------------------------
box("Floor", "Geometry", 4000, 0, (14000, 6000, 100), material=grid, z=-100)

# --- Spawns --------------------------------------------------------------------------------------
for i, y in enumerate((-300, -100, 100, 300)):
    start(f"PlayerStart_Alpha_{i}", SPAWN_X, y, 0, "TeamAlpha")
for i, y in enumerate((-800, 800)):
    start(f"BotStart_Bravo_{i}", 6000, y, 180, "TeamBravo")

# --- Firing lane (Y = 0): man-sized targets ---------------------------------------------------------
for metres in (10, 25, 50, 100):
    x = SPAWN_X + metres * 100
    box(f"Target_{metres}m", "FiringLane", x, 0, (30, 60, 180))
    text(f"Label_{metres}m", "FiringLane", x, 0, 230, f"{metres} m")
box("Lane_Left", "FiringLane", SPAWN_X + 5500, -260, (11000, 20, 40))
box("Lane_Right", "FiringLane", SPAWN_X + 5500, 260, (11000, 20, 40))

# --- Traversal course (Y = -1500): thin obstacles to vault/hurdle, platforms to mantle ------------
course = [  # (name, x, size, caption)
    ("Hurdle_60", -600, (40, 300, 60), "hurdle 60"),
    ("Vault_100", 0, (40, 300, 100), "vault 100"),
    ("Vault_130", 600, (40, 300, 130), "vault 130"),
    ("Mantle_100", 1300, (300, 300, 100), "mantle 100"),
    ("Mantle_160", 2000, (300, 300, 160), "mantle 160"),
    ("Mantle_220", 2700, (300, 300, 220), "mantle 220"),
]
for name, x, size, caption in course:
    block(name, "TraversalCourse", x, -1500, size)
    text(f"Label_{name}", "TraversalCourse", x, -1500, size[2] + 60, caption, 40)

# --- Cover course (Y = +1500): crouch and standing cover, pillars, a ramp ----------------------------
for i, x in enumerate((-600, 400)):
    box(f"Cover_Crouch_{i}", "CoverCourse", x, 1500, (40, 300, 100))
for i, x in enumerate((1400, 2400)):
    box(f"Cover_Stand_{i}", "CoverCourse", x, 1500, (40, 300, 200))
for i, (x, y) in enumerate(((3200, 1300), (3200, 1700))):
    box(f"Pillar_{i}", "CoverCourse", x, y, (80, 80, 300))
box("Ramp", "CoverCourse", 4300, 1500, (800, 300, 20), pitch=15)
text("Label_Cover", "CoverCourse", -600, 1500, 170, "crouch cover", 40)
text("Label_Stand", "CoverCourse", 1400, 1500, 270, "standing cover", 40)

# --- World settings and save ------------------------------------------------------------------------
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, GAME_MODE))
les.save_current_level()
log(f"[Range] RANGE_MAP_DONE {MAP} with {len(eas.get_all_level_actors())} actors")
