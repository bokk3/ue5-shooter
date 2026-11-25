"""Build /Game/NobleFermi/Maps/L_Range: a small, cheap map for checking player mechanics.

Layout (X is forward from the spawn line, Y is right; 1 unit = 1 cm):
  * spawn line at X = -1500: four TeamAlpha player starts facing +X
  * firing lane (Y = 0): man-sized targets at 10, 25, 50 and 100 m, each with a distance label
  * traversal course (Y = -1500): Game Animation Sample traversable blocks, low hurdle to high mantle
  * cover course (Y = +1500): crouch-height and standing-height walls, pillars, a ramp
  * two TeamBravo starts far down range for bots (?Bots=N, default 2)
  * sun, sky light, sky atmosphere and height fog; no Lumen-heavy content, no landscape
  * dressing from the Trellis props (Content/NobleFermi/Generated, see Scripts/Trellis): a ready room behind
    the spawn (lockers, racks, briefing table, workbench, supplies), real targets, distance markers and a
    backstop on the lane, a row of real cover (Y = +2300), a panel shoot house (Y = -2450) and parked vehicles.
    Props that aren't imported are skipped, so the map still builds without them.

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
GENERATED = "/Game/NobleFermi/Generated"                   # Trellis props (Scripts/Trellis); optional
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


def find_generated_meshes():
    """Trellis props imported by Scripts/Trellis/import_assets.py, by name (SM_<Name> -> Name)."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    found = {}
    for data in registry.get_assets_by_path(GENERATED, recursive=True):
        name = str(data.asset_name)
        if str(data.asset_class_path.asset_name) == "StaticMesh" and name.startswith("SM_"):
            found[name[3:]] = str(data.package_name)
    log(f"[Range] {len(found)} generated props under {GENERATED}")
    return found


generated = find_generated_meshes()
missing_props = set()


def prop(name, folder, mesh_name, x, y, yaw=0.0, z=0.0):
    """Generated prop standing at height z, centred on (x, y). The props face +X; yaw 180 faces the spawn line.
    Returns None (and the map still builds) when the prop isn't imported."""
    path = generated.get(mesh_name)
    mesh = unreal.load_asset(path) if path else None
    if mesh is None:
        missing_props.add(mesh_name)
        return None
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, V(x, y, 500), R(roll=0, pitch=0, yaw=yaw))
    a.static_mesh_component.set_static_mesh(mesh)
    return label(ground(a, x, y, z), name, folder)


def top_of(actor):
    """Height of an actor's top, for setting props on tables and crates."""
    origin, extent = actor.get_actor_bounds(False)
    return origin.z + extent.z


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
    if not prop(f"Target_{metres}m", "FiringLane", "Target_Silhouette", x, 0, yaw=180):
        box(f"Target_{metres}m", "FiringLane", x, 0, (30, 60, 180))
    text(f"Label_{metres}m", "FiringLane", x, 0, 230, f"{metres} m")
    prop(f"DistanceMarker_{metres}m", "FiringLane", "DistanceMarker", x, 210, yaw=180)
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

# --- Dressing: Trellis props (Content/NobleFermi/Generated). Each is skipped if not imported. ---------

# Ready room behind the spawn line (X < -1800): armory wall, briefing table, workbench, supplies.
prop("Whiteboard_Map", "ReadyRoom", "Whiteboard_Map", -2450, 0)
for i, y in enumerate((-400, -540, -680)):
    prop(f"LoadoutLocker_{i}", "ReadyRoom", "LoadoutLocker", -2400, y)
prop("ArmorMannequin", "ReadyRoom", "ArmorMannequinStand", -2380, -860)
for i, y in enumerate((300, 390, 480)):
    prop(f"WeaponRack_{i}", "ReadyRoom", "WeaponRack", -2420, y)
for i, y in enumerate((650, 750)):
    prop(f"WeaponDisplay_{i}", "ReadyRoom", "WeaponDisplayStand", -2200, y)
table = prop("BriefingTable", "ReadyRoom", "TacticalTable", -2050, -550)
if table:
    tabletop = top_of(table)
    for name, mesh, x, y in (("Laptop", "RuggedLaptop", -2070, -590), ("Radio", "FieldRadioStation", -2065, -500),
                             ("Claymore", "Claymore", -2005, -610), ("C4", "C4Charge", -2005, -575),
                             ("Smoke", "SmokeGrenade", -2000, -545), ("TimeBubble", "TimeBubbleGrenade", -2000, -525),
                             ("TeleportBeacon", "TeleportBeacon", -2010, -495)):
        prop(f"Table_{name}", "ReadyRoom", mesh, x, y, z=tabletop)
prop("Chair_0", "ReadyRoom", "FoldingChair", -2160, -550)
prop("Chair_1", "ReadyRoom", "FoldingChair", -1940, -550, yaw=180)
prop("Workbench", "ReadyRoom", "Workbench", -2420, 900)
prop("ToolChest", "ReadyRoom", "ToolChest", -2420, 1080)
prop("JerryCan", "ReadyRoom", "JerryCan", -2300, 1090)
for i, y in enumerate((950, 1010)):
    prop(f"WeaponCases_{i}", "ReadyRoom", "WeaponCases", -2200, y)
crate = prop("AmmoCrate", "ReadyRoom", "AmmoCrate", -2000, 450)
if crate:
    prop("AmmoCan", "ReadyRoom", "AmmoCan", -2010, 430, z=top_of(crate))
    prop("FirstAidKit", "ReadyRoom", "FirstAidKit", -1990, 480, z=top_of(crate))
prop("ArmorPlateBag", "ReadyRoom", "ArmorPlateBag", -2000, 560)
prop("Generator", "ReadyRoom", "Generator", -2400, -1150, yaw=90)
prop("Floodlight", "ReadyRoom", "Floodlight", -2200, -1150)
prop("Jeep", "Vehicles", "MilitaryJeep", -2100, 1700)

# Firing lane extras: bench at the line, poppers at 15 m along the lane edges, paper targets at 25 m, backstop.
prop("ShootingBench", "FiringLane", "ShootingBench", SPAWN_X + 200, 0)
for i, y in enumerate((-200, -130, 130, 200)):
    prop(f"Popper_15m_{i}", "FiringLane", "Target_Popper", SPAWN_X + 1500, y, yaw=180)
for i, y in enumerate((-150, 150)):
    prop(f"PaperTarget_25m_{i}", "FiringLane", "Target_Paper", SPAWN_X + 2500, y, yaw=180)
for i, y in enumerate((-163, 0, 163)):
    prop(f"Backstop_Hesco_{i}", "FiringLane", "HescoBarrier", SPAWN_X + 10600, y)
for i, y in enumerate((-305, 305)):
    prop(f"Backstop_Container_{i}", "FiringLane", "Container20_Red", SPAWN_X + 11000, y, yaw=90)

# Real cover pieces (Y = +2300), next to the block cover course, low to high.
real_cover = [  # (name, mesh, x, yaw, caption)
    ("JerseyBarrier", "JerseyBarrier", -600, 0, "jersey barrier"),
    ("Hesco", "HescoBarrier", 400, 0, "hesco"),
    ("Tires", "TireBarricade", 1400, 90, "tire barricade"),
    ("Shield", "DeployableShield", 2400, 0, "deployable shield"),
    ("Panel", "ShootHouse_Panel", 3200, 0, "shoot house panel"),
    ("Container", "Container20_Red", 4400, 90, "container"),
    ("VaultBox", "VaultBox", 5400, 0, "vault box"),
]
for name, mesh, x, yaw, caption in real_cover:
    piece = prop(f"RealCover_{name}", "RealCover", mesh, x, 2300, yaw=yaw)
    if piece:
        text(f"Label_RealCover_{name}", "RealCover", x, 2300, top_of(piece) + 50, caption, 40)

# Shoot house (Y = -2450): a 7.4 m room of panels, door in the west and north walls, a half divider, targets.
PANEL = 247.0
HX, HY = 4300.0, -2450.0
house = [  # (x, y, yaw): yaw 0 walls run along Y, yaw 90 along X
    (HX - 1.5 * PANEL, HY - PANEL, 0), (HX - 1.5 * PANEL, HY, 0),                      # west, door at north end
    (HX + 1.5 * PANEL, HY - PANEL, 0), (HX + 1.5 * PANEL, HY, 0), (HX + 1.5 * PANEL, HY + PANEL, 0),  # east
    (HX - PANEL, HY - 1.5 * PANEL, 90), (HX, HY - 1.5 * PANEL, 90), (HX + PANEL, HY - 1.5 * PANEL, 90),  # south
    (HX - PANEL, HY + 1.5 * PANEL, 90), (HX + PANEL, HY + 1.5 * PANEL, 90),           # north, door in the middle
    (HX, HY - PANEL, 0),                                                              # half divider from the south
]
for i, (x, y, yaw) in enumerate(house):
    prop(f"ShootHouse_Panel_{i}", "ShootHouse", "ShootHouse_Panel", x, y, yaw=yaw)
prop("ShootHouse_Target_0", "ShootHouse", "Target_Silhouette", HX + 250, HY + 150, yaw=180)
prop("ShootHouse_Target_1", "ShootHouse", "Target_Silhouette", HX + 250, HY - 250, yaw=180)
prop("ShootHouse_Popper", "ShootHouse", "Target_Popper", HX - 150, HY - 250, yaw=90)
text("Label_ShootHouse", "ShootHouse", HX - 1.5 * PANEL - 100, HY + PANEL, 280, "shoot house", 50)

# Down range: helicopter pad, forklift and pallets by the backstop.
prop("Helicopter", "Vehicles", "TransportHelicopter", 7600, -1900, yaw=200)
prop("Forklift", "Vehicles", "Forklift", 8700, 900, yaw=90)
for i, y in enumerate((1400, 1550)):
    pallet = prop(f"Pallet_{i}", "DownRange", "Pallet", 8800, y)
    if pallet:
        prop(f"Pallet_AmmoCrate_{i}", "DownRange", "AmmoCrate", 8800, y, z=top_of(pallet))
prop("AirdropCrate", "DownRange", "AirdropCrate", 8600, -700)

if missing_props:
    log(f"[Range] props not imported, skipped: {', '.join(sorted(missing_props))}")

# --- World settings and save ------------------------------------------------------------------------
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, GAME_MODE))
les.save_current_level()
log(f"[Range] RANGE_MAP_DONE {MAP} with {len(eas.get_all_level_actors())} actors")
