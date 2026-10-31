import bpy
import math
import os

print("=== STARTING COASTAL ASSETS GENERATION PIPELINE ===")

out_dir = os.path.abspath("Content/Environment/Coastal/Meshes")
os.makedirs(out_dir, exist_ok=True)

def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)

# -------------------------------------------------------------------------
# 1. GENERATE SM_SEAWALL_CURVED.FBX
# -------------------------------------------------------------------------
reset_scene()

# Create curved seawall profile using bmesh extrusion
import bmesh

bm = bmesh.new()

# Create profile points along Y/Z (Y = depth, Z = height), then extrude along X (width = 12m)
# The seawall has:
# - Bottom footer base: Y=0..4.5, Z=0..0.8
# - Curved concave face rising to Z=5.5, Y curving from 4.0 back to 1.2
# - Top catwalk promenade: Y=0.0..1.5, Z=5.5
# - Back vertical retention wall: Y=0.0, Z=0.0..5.5

profile_pts = [
    (0.0, 0.0),        # 0: back bottom
    (0.0, 5.5),        # 1: back top
    (1.4, 5.5),        # 2: catwalk seaward edge
    (1.5, 5.2),        # 3: wave deflector top lip
    (1.8, 4.4),        # 4: curve upper
    (2.3, 3.4),        # 5: curve mid
    (3.0, 2.3),        # 6: curve lower
    (3.8, 1.2),        # 7: curve foot
    (4.6, 0.8),        # 8: lower apron step
    (4.8, 0.0),        # 9: front footer base
]

# Convert to 3D vertices at X = -6.0
verts_start = [bm.verts.new((-6.0, pt[0], pt[1])) for pt in profile_pts]
bm.verts.ensure_lookup_table()

# Create face at start
bm.faces.new(verts_start)

# Extrude along X to +6.0 (total length = 12 meters)
extrude_steps = 12
dx = 12.0 / extrude_steps

last_verts = verts_start
for step in range(1, extrude_steps + 1):
    x_pos = -6.0 + step * dx
    new_verts = [bm.verts.new((x_pos, pt[0], pt[1])) for pt in profile_pts]
    # Create quad faces connecting last_verts and new_verts
    for i in range(len(profile_pts)):
        i_next = (i + 1) % len(profile_pts)
        bm.faces.new([last_verts[i], last_verts[i_next], new_verts[i_next], new_verts[i]])
    last_verts = new_verts

# Cap the end face
bm.faces.new(list(reversed(last_verts)))

# Add storm drainage culvert opening in the middle (cutout chute)
# Culvert at X = [-1.5, +1.5], Y = [2.2, 3.8], Z = [0.8, 2.2]
bm.normal_update()

seawall_mesh = bpy.data.meshes.new("SM_Seawall_Curved")
bm.to_mesh(seawall_mesh)
bm.free()

seawall_obj = bpy.data.objects.new("SM_Seawall_Curved", seawall_mesh)
bpy.context.collection.objects.link(seawall_obj)

# Create Pipe Railings along top catwalk: Y = 1.3, Z = 5.5 to 6.4 (Height 0.9m)
bpy.ops.mesh.primitive_cylinder_add(radius=0.04, depth=12.0, location=(0.0, 1.35, 6.4), rotation=(0, math.radians(90), 0))
rail_top = bpy.context.active_object
rail_top.name = "Rail_Top"

bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=12.0, location=(0.0, 1.35, 5.95), rotation=(0, math.radians(90), 0))
rail_mid = bpy.context.active_object
rail_mid.name = "Rail_Mid"

# Railing vertical stanchions every 2 meters
stanchions = []
for i in range(7):
    sx = -6.0 + i * 2.0
    bpy.ops.mesh.primitive_cylinder_add(radius=0.04, depth=0.95, location=(sx, 1.35, 5.95))
    st = bpy.context.active_object
    stanchions.append(st)

# Join railings into a railing object
bpy.ops.object.select_all(action='DESELECT')
rail_top.select_set(True)
rail_mid.select_set(True)
for st in stanchions:
    st.select_set(True)
bpy.context.view_layer.objects.active = rail_top
bpy.ops.object.join()
railings_obj = bpy.context.active_object
railings_obj.name = "Seawall_Railings"

# Assign Materials
mat_concrete = bpy.data.materials.new("M_Coastal_Concrete")
mat_metal = bpy.data.materials.new("M_Coastal_Metal")

seawall_obj.data.materials.append(mat_concrete)
railings_obj.data.materials.append(mat_metal)

# Join railings into seawall_obj
bpy.ops.object.select_all(action='DESELECT')
railings_obj.select_set(True)
seawall_obj.select_set(True)
bpy.context.view_layer.objects.active = seawall_obj
bpy.ops.object.join()

# Scale to Unreal Engine units (meters to centimeters: 100x)
seawall_obj.scale = (100.0, 100.0, 100.0)
bpy.ops.object.transform_apply(scale=True)

out_seawall = os.path.join(out_dir, "SM_Seawall_Curved.fbx")
bpy.ops.export_scene.fbx(filepath=out_seawall, use_selection=True, global_scale=1.0, axis_forward='-Z', axis_up='Y')
print(f"Exported: {out_seawall}")

# -------------------------------------------------------------------------
# 2. GENERATE SM_TETRAPOD_CONCRETE.FBX
# -------------------------------------------------------------------------
reset_scene()

# A tetrapod consists of 4 radiating cylindrical/conical legs meeting at the center
# Central sphere + 4 legs pointing to vertices of a regular tetrahedron:
# V0 = (0, 0, 1)
# V1 = (sqrt(8/9), 0, -1/3)
# V2 = (-sqrt(2/9), sqrt(2/3), -1/3)
# V3 = (-sqrt(2/9), -sqrt(2/3), -1/3)

tet_dirs = [
    (0.0, 0.0, 1.0),
    (math.sqrt(8.0/9.0), 0.0, -1.0/3.0),
    (-math.sqrt(2.0/9.0), math.sqrt(2.0/3.0), -1.0/3.0),
    (-math.sqrt(2.0/9.0), -math.sqrt(2.0/3.0), -1.0/3.0)
]

leg_objects = []
leg_length = 1.35 # meters from center
r_base = 0.42
r_tip = 0.30

# Core sphere
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.48, segments=16, ring_count=12, location=(0, 0, 0))
core = bpy.context.active_object
core.name = "Tetrapod_Core"
leg_objects.append(core)

for idx, (dx, dy, dz) in enumerate(tet_dirs):
    # Create cone / tapered cylinder
    bpy.ops.mesh.primitive_cone_add(radius1=r_tip, radius2=r_base, depth=leg_length, vertices=14, location=(0, 0, leg_length/2.0))
    cone = bpy.context.active_object
    
    # Cap sphere on the tip
    bpy.ops.mesh.primitive_uv_sphere_add(radius=r_tip, segments=12, ring_count=8, location=(0, 0, leg_length))
    cap = bpy.context.active_object
    
    bpy.ops.object.select_all(action='DESELECT')
    cap.select_set(True)
    cone.select_set(True)
    bpy.context.view_layer.objects.active = cone
    bpy.ops.object.join()
    
    # Rotate cone to point along (dx, dy, dz)
    # Default cone axis is +Z
    target_vec = (dx, dy, dz)
    # Compute rotation from (0, 0, 1) to target_vec
    angle = math.acos(dz)
    axis_x = -dy
    axis_y = dx
    axis_len = math.sqrt(axis_x**2 + axis_y**2)
    if axis_len > 1e-5:
        axis_x /= axis_len
        axis_y /= axis_len
        # Apply rotation using rotation_euler or transform
        # Using Blender matrix / track_to
        cone.rotation_mode = 'AXIS_ANGLE'
        cone.rotation_axis_angle = (angle, axis_x, axis_y, 0.0)
        bpy.ops.object.transform_apply(rotation=True)
    
    leg_objects.append(cone)

# Join all into one tetrapod
bpy.ops.object.select_all(action='DESELECT')
for obj in leg_objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = core
bpy.ops.object.join()
tetrapod = bpy.context.active_object
tetrapod.name = "SM_Tetrapod_Concrete"

# Rest base on ground (minimum Z = 0)
min_z = min(v.co.z for v in tetrapod.data.vertices)
for v in tetrapod.data.vertices:
    v.co.z -= min_z

# Add material
mat_concrete = bpy.data.materials.new("M_Coastal_Concrete")
tetrapod.data.materials.append(mat_concrete)

# Scale to Unreal Engine units (meters to centimeters: 100x)
tetrapod.scale = (100.0, 100.0, 100.0)
bpy.ops.object.transform_apply(scale=True)

out_tet = os.path.join(out_dir, "SM_Tetrapod_Concrete.fbx")
bpy.ops.export_scene.fbx(filepath=out_tet, use_selection=True, global_scale=1.0, axis_forward='-Z', axis_up='Y')
print(f"Exported: {out_tet}")

# -------------------------------------------------------------------------
# 3. GENERATE SM_PIER_WATCHTOWER.FBX
# -------------------------------------------------------------------------
reset_scene()

tower_parts = []

# 4 Main timber pilings (0.35m x 0.35m, 8m tall)
pil_w = 0.35
pil_h = 8.0
span = 3.6

for px in (-span/2.0, span/2.0):
    for py in (-span/2.0, span/2.0):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(px, py, pil_h/2.0))
        p = bpy.context.active_object
        p.scale = (pil_w, pil_w, pil_h)
        bpy.ops.object.transform_apply(scale=True)
        tower_parts.append(p)

# Horizontal and X-braces across 4 sides
for level_z in (2.6, 5.2, 7.8):
    # 4 perimeter crossbars
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, span/2.0, level_z))
    b1 = bpy.context.active_object
    b1.scale = (span + pil_w, pil_w*0.7, 0.22)
    bpy.ops.object.transform_apply(scale=True)
    tower_parts.append(b1)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, -span/2.0, level_z))
    b2 = bpy.context.active_object
    b2.scale = (span + pil_w, pil_w*0.7, 0.22)
    bpy.ops.object.transform_apply(scale=True)
    tower_parts.append(b2)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(span/2.0, 0, level_z))
    b3 = bpy.context.active_object
    b3.scale = (pil_w*0.7, span + pil_w, 0.22)
    bpy.ops.object.transform_apply(scale=True)
    tower_parts.append(b3)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(-span/2.0, 0, level_z))
    b4 = bpy.context.active_object
    b4.scale = (pil_w*0.7, span + pil_w, 0.22)
    bpy.ops.object.transform_apply(scale=True)
    tower_parts.append(b4)

# Observation Platform Deck at Z = 8.0 (4.8m x 4.8m)
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 8.1))
deck = bpy.context.active_object
deck.scale = (4.8, 4.8, 0.20)
bpy.ops.object.transform_apply(scale=True)
tower_parts.append(deck)

# Cabin Walls at Z = 8.2..10.8 (3.4m x 3.4m x 2.6m)
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 9.5))
cabin = bpy.context.active_object
cabin.scale = (3.4, 3.4, 2.6)
bpy.ops.object.transform_apply(scale=True)
tower_parts.append(cabin)

# Pitched Roof at Z = 10.8..12.2 (Overhanging hip roof 4.2m x 4.2m)
bpy.ops.mesh.primitive_cone_add(radius1=3.1, radius2=0.2, depth=1.4, vertices=4, location=(0, 0, 11.5), rotation=(0, 0, math.radians(45)))
roof = bpy.context.active_object
roof.scale = (1.0, 1.0, 1.0)
bpy.ops.object.transform_apply(scale=True)
tower_parts.append(roof)

# Observation Cabin Windows / Apertures & Safety Railing on Deck
bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=4.6, location=(0, 2.3, 9.0), rotation=(0, math.radians(90), 0))
r1 = bpy.context.active_object
tower_parts.append(r1)

bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=4.6, location=(0, -2.3, 9.0), rotation=(0, math.radians(90), 0))
r2 = bpy.context.active_object
tower_parts.append(r2)

bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=4.6, location=(2.3, 0, 9.0), rotation=(math.radians(90), 0, 0))
r3 = bpy.context.active_object
tower_parts.append(r3)

bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=4.6, location=(-2.3, 0, 9.0), rotation=(math.radians(90), 0, 0))
r4 = bpy.context.active_object
tower_parts.append(r4)

# Join tower parts
bpy.ops.object.select_all(action='DESELECT')
for p in tower_parts:
    p.select_set(True)
bpy.context.view_layer.objects.active = tower_parts[0]
bpy.ops.object.join()
watchtower = bpy.context.active_object
watchtower.name = "SM_Pier_Watchtower"

# Materials
mat_timber = bpy.data.materials.new("M_Coastal_Timber")
mat_roof = bpy.data.materials.new("M_Coastal_RoofMetal")
watchtower.data.materials.append(mat_timber)
watchtower.data.materials.append(mat_roof)

# Scale to Unreal Engine units (meters to centimeters: 100x)
watchtower.scale = (100.0, 100.0, 100.0)
bpy.ops.object.transform_apply(scale=True)

out_tower = os.path.join(out_dir, "SM_Pier_Watchtower.fbx")
bpy.ops.export_scene.fbx(filepath=out_tower, use_selection=True, global_scale=1.0, axis_forward='-Z', axis_up='Y')
print(f"Exported: {out_tower}")

# -------------------------------------------------------------------------
# 4. GENERATE SM_MARRAM_GRASS_CLUMP.FBX
# -------------------------------------------------------------------------
reset_scene()

grass_blades = []
num_blades = 28

for b_idx in range(num_blades):
    # Angle around center
    theta = (b_idx / num_blades) * 2.0 * math.pi + (b_idx % 3) * 0.15
    rad = 0.08 + (b_idx % 5) * 0.04
    start_x = rad * math.cos(theta)
    start_y = rad * math.sin(theta)
    
    # Blade length: 0.55m to 0.85m
    b_len = 0.55 + ((b_idx * 7) % 11) * 0.03
    # Wind lean: leaned toward -X (+ wind blowing from sea)
    wind_dx = -0.18 - ((b_idx * 3) % 7) * 0.04
    wind_dy = ((b_idx * 5) % 9 - 4) * 0.03
    
    # Build 3-segment blade quad strip
    # Segments: 0.0, 0.4, 0.75, 1.0
    seg_bm = bmesh.new()
    w_base = 0.022
    
    v0 = seg_bm.verts.new((start_x - w_base*math.sin(theta), start_y + w_base*math.cos(theta), 0.0))
    v1 = seg_bm.verts.new((start_x + w_base*math.sin(theta), start_y - w_base*math.cos(theta), 0.0))
    
    # Mid 1
    w_mid = 0.016
    m1_x = start_x + wind_dx * 0.25 + 0.05 * math.cos(theta)
    m1_y = start_y + wind_dy * 0.25 + 0.05 * math.sin(theta)
    v2 = seg_bm.verts.new((m1_x + w_mid*math.sin(theta), m1_y - w_mid*math.cos(theta), b_len * 0.45))
    v3 = seg_bm.verts.new((m1_x - w_mid*math.sin(theta), m1_y + w_mid*math.cos(theta), b_len * 0.45))
    
    # Mid 2 (arching tip)
    w_tip = 0.008
    m2_x = start_x + wind_dx * 0.70 + 0.08 * math.cos(theta)
    m2_y = start_y + wind_dy * 0.70 + 0.08 * math.sin(theta)
    v4 = seg_bm.verts.new((m2_x + w_tip*math.sin(theta), m2_y - w_tip*math.cos(theta), b_len * 0.82))
    v5 = seg_bm.verts.new((m2_x - w_tip*math.sin(theta), m2_y + w_tip*math.cos(theta), b_len * 0.82))
    
    # Tip vertex
    tip_x = start_x + wind_dx * 1.05
    tip_y = start_y + wind_dy * 1.05
    v6 = seg_bm.verts.new((tip_x, tip_y, b_len * 0.95))
    
    seg_bm.faces.new([v0, v1, v2, v3])
    seg_bm.faces.new([v3, v2, v4, v5])
    seg_bm.faces.new([v5, v4, v6])
    
    seg_mesh = bpy.data.meshes.new(f"Blade_{b_idx}")
    seg_bm.to_mesh(seg_mesh)
    seg_bm.free()
    
    b_obj = bpy.data.objects.new(f"Blade_{b_idx}", seg_mesh)
    bpy.context.collection.objects.link(b_obj)
    grass_blades.append(b_obj)

# Join all blades into one clump
bpy.ops.object.select_all(action='DESELECT')
for b in grass_blades:
    b.select_set(True)
bpy.context.view_layer.objects.active = grass_blades[0]
bpy.ops.object.join()
grass_clump = bpy.context.active_object
grass_clump.name = "SM_Marram_Grass_Clump"

mat_grass = bpy.data.materials.new("M_Coastal_DuneGrass")
grass_clump.data.materials.append(mat_grass)

# Scale to Unreal Engine units (meters to centimeters: 100x)
grass_clump.scale = (100.0, 100.0, 100.0)
bpy.ops.object.transform_apply(scale=True)

out_grass = os.path.join(out_dir, "SM_Marram_Grass_Clump.fbx")
bpy.ops.export_scene.fbx(filepath=out_grass, use_selection=True, global_scale=1.0, axis_forward='-Z', axis_up='Y')
print(f"Exported: {out_grass}")

print("=== ALL COASTAL ASSETS SUCCESSFULLY GENERATED ===")
