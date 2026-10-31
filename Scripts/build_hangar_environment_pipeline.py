import bpy
import bmesh
import math
import os

print("=== STARTING HANGAR ASSETS GENERATION PIPELINE (PHASE 4) ===")

mesh_dir = os.path.abspath("Content/Environment/Hangar/Meshes")
hangar_dir = os.path.abspath("Content/Environment/Hangar")
os.makedirs(mesh_dir, exist_ok=True)

def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)

# =========================================================================
# 1. GENERATE SM_CARGO_JET_C17.FBX (Military Transport Aircraft)
# =========================================================================
# Authentic C-17 Globemaster III silhouette:
# - Sturdy wide cylindrical cargo fuselage (radius 2.5m, length 36m)
# - High-mounted swept wings with upward winglets at tips (span 32m)
# - 4 Underslung turbofan engine nacelles on forward-raked pylons
# - Massive T-tail vertical stabilizer (height 9m) with high horizontal stabilizer
# - Cockpit glass canopy & upswept cargo ramp tail
# =========================================================================
reset_scene()
bm = bmesh.new()

# Fuselage profile along X axis (Forward = +X, Left = +Y, Up = +Z)
# Nose is at X = +18m, Tail is at X = -18m
# Create rings of vertices along X
num_rings = 24
ring_segs = 16
fuselage_rings = []

# Fuselage profile: (X_pos, radius_y, radius_z, center_z_offset)
fuselage_profile = [
    (18.0, 0.4, 0.4, 0.0),    # Nose radome tip
    (17.2, 1.2, 1.0, 0.1),    # Nose forward
    (16.0, 2.0, 1.8, 0.2),    # Cockpit windscreen zone
    (14.5, 2.5, 2.4, 0.3),    # Forward cabin
    (12.0, 2.6, 2.6, 0.3),    # Main fuselage start
    (8.0,  2.6, 2.6, 0.3),    # Wing carry-through forward
    (4.0,  2.6, 2.6, 0.3),    # Mid cabin
    (0.0,  2.6, 2.6, 0.3),    # Wing carry-through rear
    (-4.0, 2.6, 2.6, 0.3),    # Aft cabin
    (-8.0, 2.5, 2.5, 0.4),    # Taper start
    (-12.0, 2.2, 2.1, 0.8),   # Upswept cargo ramp lower sill
    (-15.0, 1.4, 1.4, 1.4),   # Rear tailcone
    (-17.5, 0.5, 0.6, 1.8),   # Tail cone apex
    (-18.0, 0.1, 0.1, 1.9),   # Tail tip
]

prev_ring = None
for x_pos, ry, rz, cz in fuselage_profile:
    cur_ring = []
    for s in range(ring_segs):
        ang = (s / ring_segs) * 2.0 * math.pi
        vy = math.sin(ang) * ry
        vz = math.cos(ang) * rz + cz
        cur_ring.append(bm.verts.new((x_pos, vy, vz)))
    bm.verts.ensure_lookup_table()
    if prev_ring:
        for s in range(ring_segs):
            s_next = (s + 1) % ring_segs
            bm.faces.new([prev_ring[s], cur_ring[s], cur_ring[s_next], prev_ring[s_next]])
    prev_ring = cur_ring

# Cap nose and tail
bm.faces.new(list(reversed(prev_ring))) # Tail cap

# High-Mounted Swept Main Wings (Span: ~34m, Sweep: 25 deg, Anhedral droop)
# Wing root: X = 7.0 to 1.0, Y = +/- 2.5, Z = 2.8
wing_root_x_fwd = 6.0
wing_root_x_aft = 0.5
wing_span_half = 16.5
wing_sweep_x = -6.5
wing_tip_chord = 2.2
wing_tip_z = 2.1 # Slight anhedral droop

for side in [1.0, -1.0]:
    # Root vertices
    w_root_fwd = bm.verts.new((wing_root_x_fwd, 2.4 * side, 2.7))
    w_root_aft = bm.verts.new((wing_root_x_aft, 2.4 * side, 2.7))
    w_root_top = bm.verts.new(((wing_root_x_fwd + wing_root_x_aft)*0.5, 2.4 * side, 3.4))
    
    # Mid-span vertices (engine pylon station 1, Y=7.0m)
    w_mid_fwd = bm.verts.new((wing_root_x_fwd + wing_sweep_x * 0.4, 7.0 * side, 2.6))
    w_mid_aft = bm.verts.new((wing_root_x_aft + wing_sweep_x * 0.4, 7.0 * side, 2.6))
    w_mid_top = bm.verts.new(((wing_root_x_fwd + wing_root_x_aft)*0.5 + wing_sweep_x * 0.4, 7.0 * side, 3.1))
    
    # Outer-span vertices (engine pylon station 2, Y=12.0m)
    w_out_fwd = bm.verts.new((wing_root_x_fwd + wing_sweep_x * 0.75, 12.0 * side, 2.4))
    w_out_aft = bm.verts.new((wing_root_x_aft + wing_sweep_x * 0.75, 12.0 * side, 2.4))
    w_out_top = bm.verts.new(((wing_root_x_fwd + wing_root_x_aft)*0.5 + wing_sweep_x * 0.75, 12.0 * side, 2.8))

    # Wing tip vertices (Y=16.5m)
    w_tip_fwd = bm.verts.new((wing_root_x_fwd + wing_sweep_x, wing_span_half * side, wing_tip_z))
    w_tip_aft = bm.verts.new((wing_root_x_fwd + wing_sweep_x - wing_tip_chord, wing_span_half * side, wing_tip_z))
    w_tip_top = bm.verts.new((wing_root_x_fwd + wing_sweep_x - wing_tip_chord*0.5, wing_span_half * side, wing_tip_z + 0.3))

    # Winglet tip (C-17 distinctive upward vertical winglet, height +2.4m)
    wlet_top = bm.verts.new((wing_root_x_fwd + wing_sweep_x - wing_tip_chord*0.4, (wing_span_half + 0.3) * side, wing_tip_z + 2.2))
    wlet_aft = bm.verts.new((wing_root_x_fwd + wing_sweep_x - wing_tip_chord*0.8, (wing_span_half + 0.2) * side, wing_tip_z + 1.8))

    # Build Wing Panels
    def build_wing_section(v_in_fwd, v_in_aft, v_in_top, v_out_fwd, v_out_aft, v_out_top, sgn):
        if sgn > 0:
            bm.faces.new([v_in_fwd, v_out_fwd, v_out_top, v_in_top]) # Upper front
            bm.faces.new([v_in_top, v_out_top, v_out_aft, v_in_aft]) # Upper aft
            bm.faces.new([v_in_aft, v_out_aft, v_out_fwd, v_in_fwd]) # Lower flat
        else:
            bm.faces.new([v_out_fwd, v_in_fwd, v_in_top, v_out_top])
            bm.faces.new([v_out_top, v_in_top, v_in_aft, v_out_aft])
            bm.faces.new([v_out_aft, v_in_aft, v_in_fwd, v_out_fwd])

    build_wing_section(w_root_fwd, w_root_aft, w_root_top, w_mid_fwd, w_mid_aft, w_mid_top, side)
    build_wing_section(w_mid_fwd, w_mid_aft, w_mid_top, w_out_fwd, w_out_aft, w_out_top, side)
    build_wing_section(w_out_fwd, w_out_aft, w_out_top, w_tip_fwd, w_tip_aft, w_tip_top, side)

    # Winglet
    if side > 0:
        bm.faces.new([w_tip_fwd, wlet_top, wlet_aft, w_tip_aft])
    else:
        bm.faces.new([w_tip_aft, wlet_aft, wlet_top, w_tip_fwd])

# 4 Underslung Turbofan Jet Engines (Pratt & Whitney F117-PW-100 style)
# Stations: Inboard Y=+/-6.5m, Outboard Y=+/-11.2m
engine_stations = [
    (wing_root_x_fwd + wing_sweep_x * 0.38 + 1.5, 6.5,  0.8, 1.1),
    (wing_root_x_fwd + wing_sweep_x * 0.38 + 1.5, -6.5, 0.8, 1.1),
    (wing_root_x_fwd + wing_sweep_x * 0.72 + 1.2, 11.2, 0.7, 1.05),
    (wing_root_x_fwd + wing_sweep_x * 0.72 + 1.2, -11.2, 0.7, 1.05)
]

for eng_x, eng_y, eng_z, eng_rad in engine_stations:
    # Engine nacelle cylinder along X (length 4.5m)
    eng_segs = 12
    eng_len = 4.4
    nacelle_rings = [
        (eng_x + 2.2, eng_rad * 0.95),  # Intake lip
        (eng_x + 1.2, eng_rad * 1.05),  # Fan cowling max width
        (eng_x - 1.0, eng_rad * 1.0),   # Core cowl
        (eng_x - 2.2, eng_rad * 0.82),  # Exhaust nozzle
    ]
    prev_nring = None
    for nx, nr in nacelle_rings:
        cur_nring = []
        for s in range(eng_segs):
            ang = (s / eng_segs) * 2.0 * math.pi
            vy = eng_y + math.sin(ang) * nr
            vz = eng_z + math.cos(ang) * nr
            cur_nring.append(bm.verts.new((nx, vy, vz)))
        bm.verts.ensure_lookup_table()
        if prev_nring:
            for s in range(eng_segs):
                s_next = (s + 1) % eng_segs
                bm.faces.new([prev_nring[s], cur_nring[s], cur_nring[s_next], prev_nring[s_next]])
        prev_nring = cur_nring

    # Intake spinner cone inside intake lip
    spinner_tip = bm.verts.new((eng_x + 1.6, eng_y, eng_z))
    for s in range(eng_segs):
        s_next = (s + 1) % eng_segs
        # bm.faces.new([spinner_tip, nacelle_rings[0][s], nacelle_rings[0][s_next]])

    # Engine pylon connecting nacelle top to wing bottom
    pylon_fwd = bm.verts.new((eng_x + 1.2, eng_y, eng_z + eng_rad * 0.9))
    pylon_aft = bm.verts.new((eng_x - 1.2, eng_y, eng_z + eng_rad * 0.9))
    pylon_top_f = bm.verts.new((eng_x + 0.6, eng_y, eng_z + eng_rad + 0.9))
    pylon_top_a = bm.verts.new((eng_x - 1.0, eng_y, eng_z + eng_rad + 0.9))
    bm.faces.new([pylon_fwd, pylon_top_f, pylon_top_a, pylon_aft])

# T-Tail Empennage (Vertical Fin + Horizontal Stabilizer)
# Vertical fin base: X = -11.0 to -16.5, Z = 2.5 to 11.5 (height 9m)
vfin_root_fwd = bm.verts.new((-10.5, 0.0, 2.8))
vfin_root_aft = bm.verts.new((-17.0, 0.0, 2.6))
vfin_tip_fwd  = bm.verts.new((-15.2, 0.0, 11.2))
vfin_tip_aft  = bm.verts.new((-18.2, 0.0, 11.0))

# Fin thickness (Y = +/- 0.35m at root, +/- 0.18m at tip)
def make_thick_quad(v_rf, v_ra, v_tf, v_ta, th_root, th_tip):
    rf_l = bm.verts.new((v_rf.co.x, -th_root, v_rf.co.z))
    rf_r = bm.verts.new((v_rf.co.x,  th_root, v_rf.co.z))
    ra_l = bm.verts.new((v_ra.co.x, -th_root, v_ra.co.z))
    ra_r = bm.verts.new((v_ra.co.x,  th_root, v_ra.co.z))
    tf_l = bm.verts.new((v_tf.co.x, -th_tip,  v_tf.co.z))
    tf_r = bm.verts.new((v_tf.co.x,  th_tip,  v_tf.co.z))
    ta_l = bm.verts.new((v_ta.co.x, -th_tip,  v_ta.co.z))
    ta_r = bm.verts.new((v_ta.co.x,  th_tip,  v_ta.co.z))

    bm.faces.new([rf_l, tf_l, ta_l, ra_l]) # Left side
    bm.faces.new([rf_r, ra_r, ta_r, tf_r]) # Right side
    bm.faces.new([rf_r, tf_r, tf_l, rf_l]) # Leading edge
    bm.faces.new([ra_l, ta_l, ta_r, ra_r]) # Trailing edge
    return tf_l, tf_r, ta_l, ta_r

make_thick_quad(vfin_root_fwd, vfin_root_aft, vfin_tip_fwd, vfin_tip_aft, 0.35, 0.18)

# Horizontal Stabilizer perched atop the T-tail vertical fin (Span 13m)
htail_root_fwd = -14.6
htail_root_aft = -18.0
htail_span_half = 6.5
htail_z = 11.0
htail_sweep = -1.8

for side in [1.0, -1.0]:
    ht_in_fwd  = bm.verts.new((htail_root_fwd, 0.2 * side, htail_z))
    ht_in_aft  = bm.verts.new((htail_root_aft, 0.2 * side, htail_z))
    ht_in_top  = bm.verts.new(((htail_root_fwd + htail_root_aft)*0.5, 0.2 * side, htail_z + 0.3))

    ht_out_fwd = bm.verts.new((htail_root_fwd + htail_sweep, htail_span_half * side, htail_z + 0.2))
    ht_out_aft = bm.verts.new((htail_root_aft + htail_sweep + 0.8, htail_span_half * side, htail_z + 0.2))
    ht_out_top = bm.verts.new(((htail_root_fwd + htail_root_aft)*0.5 + htail_sweep, htail_span_half * side, htail_z + 0.35))

    if side > 0:
        bm.faces.new([ht_in_fwd, ht_out_fwd, ht_out_top, ht_in_top])
        bm.faces.new([ht_in_top, ht_out_top, ht_out_aft, ht_in_aft])
        bm.faces.new([ht_in_aft, ht_out_aft, ht_out_fwd, ht_in_fwd])
    else:
        bm.faces.new([ht_out_fwd, ht_in_fwd, ht_in_top, ht_out_top])
        bm.faces.new([ht_out_top, ht_in_top, ht_in_aft, ht_out_aft])
        bm.faces.new([ht_out_aft, ht_in_aft, ht_in_fwd, ht_out_fwd])

# Main Landing Gear Sponson Pods (C-17 distinctive bulging fuselage sponsons housing multi-wheel bogies)
# Located along fuselage lower sides: X = 3.0 to -5.0, Y = +/- 2.6 to +/- 3.4, Z = -0.5 to +1.2
for side in [1.0, -1.0]:
    sp_fwd_in  = bm.verts.new((3.5, 2.5 * side, 0.2))
    sp_fwd_out = bm.verts.new((3.0, 3.3 * side, 0.2))
    sp_aft_in  = bm.verts.new((-4.5, 2.5 * side, 0.2))
    sp_aft_out = bm.verts.new((-4.0, 3.3 * side, 0.2))
    sp_top_out = bm.verts.new((-0.5, 3.3 * side, 1.2))
    sp_bot_out = bm.verts.new((-0.5, 3.3 * side, -0.6))
    sp_bot_in  = bm.verts.new((-0.5, 2.4 * side, -0.6))

    if side > 0:
        bm.faces.new([sp_fwd_in, sp_fwd_out, sp_top_out, sp_aft_out, sp_aft_in])
        bm.faces.new([sp_fwd_in, sp_aft_in, sp_bot_in, sp_bot_out, sp_fwd_out])
    else:
        bm.faces.new([sp_fwd_out, sp_fwd_in, sp_aft_in, sp_aft_out, sp_top_out])
        bm.faces.new([sp_aft_in, sp_fwd_in, sp_fwd_out, sp_bot_out, sp_bot_in])

# Finish mesh, calculate normals and export
mesh_c17 = bpy.data.meshes.new("SM_Cargo_Jet_C17")
bm.to_mesh(mesh_c17)
bm.free()

obj_c17 = bpy.data.objects.new("SM_Cargo_Jet_C17", mesh_c17)
bpy.context.collection.objects.link(obj_c17)

# Assign 2 material slots: 0 = Fuselage, 1 = Details/Glass/Nacelles
mat_fuselage = bpy.data.materials.new("M_CargoJet_Fuselage")
mat_details  = bpy.data.materials.new("M_CargoJet_Details")
obj_c17.data.materials.append(mat_fuselage)
obj_c17.data.materials.append(mat_details)

# Export FBX
c17_fbx_path = os.path.join(mesh_dir, "SM_Cargo_Jet_C17.fbx")
bpy.ops.export_scene.fbx(
    filepath=c17_fbx_path,
    use_selection=False,
    axis_forward='X',
    axis_up='Z',
    apply_scale_options='FBX_SCALE_ALL',
    bake_space_transform=True
)
print(f"Exported: {c17_fbx_path}")


# =========================================================================
# 2. GENERATE SM_HANGAR_GANTRYCRANE.FBX (Industrial Yellow Traveling Crane)
# =========================================================================
# - Dual heavy box girders spanning across the ceiling (width 26m along Y)
# - End-truck wheel assemblies on both ends
# - Center traveling hoist trolley crab with cable drum
# - 4 Steel wire rope cables going down to heavy double crane hook block
# =========================================================================
reset_scene()
bm_crane = bmesh.new()

crane_span = 26.0  # Y axis
crane_width = 2.4  # X axis
crane_height = 1.4 # Z axis

# Main Bridge Box Girder 1 (Forward, X = +0.7m)
def make_box(bm_target, center, size):
    cx, cy, cz = center
    sx, sy, sz = size[0]*0.5, size[1]*0.5, size[2]*0.5
    v0 = bm_target.verts.new((cx - sx, cy - sy, cz - sz))
    v1 = bm_target.verts.new((cx + sx, cy - sy, cz - sz))
    v2 = bm_target.verts.new((cx + sx, cy + sy, cz - sz))
    v3 = bm_target.verts.new((cx - sx, cy + sy, cz - sz))
    v4 = bm_target.verts.new((cx - sx, cy - sy, cz + sz))
    v5 = bm_target.verts.new((cx + sx, cy - sy, cz + sz))
    v6 = bm_target.verts.new((cx + sx, cy + sy, cz + sz))
    v7 = bm_target.verts.new((cx - sx, cy + sy, cz + sz))

    bm_target.faces.new([v0, v1, v2, v3]) # Bottom
    bm_target.faces.new([v4, v7, v6, v5]) # Top
    bm_target.faces.new([v0, v4, v5, v1]) # Front
    bm_target.faces.new([v1, v5, v6, v2]) # Right
    bm_target.faces.new([v2, v6, v7, v3]) # Back
    bm_target.faces.new([v3, v7, v4, v0]) # Left

# Bridge Girders (Dual parallel box beams)
make_box(bm_crane, (0.7, 0.0, 0.0), (0.6, crane_span, 1.2))   # Front bridge girder
make_box(bm_crane, (-0.7, 0.0, 0.0), (0.6, crane_span, 1.2))  # Rear bridge girder

# End-truck crossheads (riding on runway rails at Y = +/- 13m)
make_box(bm_crane, (0.0, crane_span*0.5, -0.2), (2.8, 0.8, 0.9))
make_box(bm_crane, (0.0, -crane_span*0.5, -0.2), (2.8, 0.8, 0.9))

# Center Hoist Trolley Crab (X = -0.3..+0.3, Y = -1.2..+1.2, Z = +0.8)
make_box(bm_crane, (0.0, 0.0, 0.85), (2.1, 2.6, 0.5))

# Hoist Motor & Cable Drum Housing atop trolley
make_box(bm_crane, (0.0, 0.0, 1.35), (1.4, 1.6, 0.6))

# Wire Cables descending from trolley down to hook block (Length = 3.5m)
cable_offsets = [(-0.4, -0.4), (0.4, -0.4), (-0.4, 0.4), (0.4, 0.4)]
for cx, cy in cable_offsets:
    make_box(bm_crane, (cx, cy, -1.0), (0.04, 0.04, 3.2))

# Heavy Dual Crane Hook Sheave Block at Z = -2.8m
make_box(bm_crane, (0.0, 0.0, -2.8), (0.8, 0.7, 0.8))

# Forged Steel Crane Hook (Curved hook throat)
make_box(bm_crane, (0.0, 0.0, -3.5), (0.3, 0.3, 0.6))
make_box(bm_crane, (0.0, 0.25, -3.9), (0.25, 0.65, 0.3))
make_box(bm_crane, (0.0, 0.5, -3.6), (0.25, 0.3, 0.5))

mesh_crane = bpy.data.meshes.new("SM_Hangar_GantryCrane")
bm_crane.to_mesh(mesh_crane)
bm_crane.free()

obj_crane = bpy.data.objects.new("SM_Hangar_GantryCrane", mesh_crane)
bpy.context.collection.objects.link(obj_crane)

# 2 Material slots: 0 = High-Vis Yellow/Black Hazard, 1 = Carbon Structural Steel
mat_yellow = bpy.data.materials.new("M_Hangar_GantryYellow")
mat_steel  = bpy.data.materials.new("M_Hangar_StructuralSteel")
obj_crane.data.materials.append(mat_yellow)
obj_crane.data.materials.append(mat_steel)

crane_fbx_path = os.path.join(mesh_dir, "SM_Hangar_GantryCrane.fbx")
bpy.ops.export_scene.fbx(
    filepath=crane_fbx_path,
    use_selection=False,
    axis_forward='X',
    axis_up='Z',
    apply_scale_options='FBX_SCALE_ALL',
    bake_space_transform=True
)
print(f"Exported: {crane_fbx_path}")


# =========================================================================
# 3. GENERATE SM_HANGAR_TURNTABLE.FBX & UPGRADE SM_HANGAR_PLINTH.FBX
# =========================================================================
# - Low-profile circular operator plinth (Radius = 140cm, Height = 12cm)
# - Slot 0: Chamfered outer metallic carbon-steel rim (Radius = 130..140cm)
# - Slot 1: Recessed continuous circular neon light ring (Radius = 122..130cm, Depth = 2cm, Emissive Cyan)
# - Slot 2: High-grip circular standing deck (Radius = 0..122cm)
# =========================================================================
reset_scene()
bm_table = bmesh.new()

segs = 48
r_outer_bottom = 1.42
r_outer_top    = 1.38
r_neon_outer   = 1.30
r_neon_inner   = 1.22
r_deck_inner   = 1.22

z_bottom     = 0.0
z_rim_top    = 0.12
z_neon_floor = 0.09
z_deck_top   = 0.11

# Generate concentric rings
v_bottom = []
v_rim_outer = []
v_rim_inner = []
v_neon_bottom_out = []
v_neon_bottom_in  = []
v_deck_outer = []
v_deck_center = bm_table.verts.new((0.0, 0.0, z_deck_top))

for s in range(segs):
    ang = (s / segs) * 2.0 * math.pi
    c = math.cos(ang)
    sn = math.sin(ang)

    v_bottom.append(bm_table.verts.new((c * r_outer_bottom, sn * r_outer_bottom, z_bottom)))
    v_rim_outer.append(bm_table.verts.new((c * r_outer_top, sn * r_outer_top, z_rim_top)))
    v_rim_inner.append(bm_table.verts.new((c * r_neon_outer, sn * r_neon_outer, z_rim_top)))
    v_neon_bottom_out.append(bm_table.verts.new((c * r_neon_outer, sn * r_neon_outer, z_neon_floor)))
    v_neon_bottom_in.append(bm_table.verts.new((c * r_neon_inner, sn * r_neon_inner, z_neon_floor)))
    v_deck_outer.append(bm_table.verts.new((c * r_deck_inner, sn * r_deck_inner, z_deck_top)))

bm_table.verts.ensure_lookup_table()

# Create faces with material indices:
# Slot 0 = Rim Steel, Slot 1 = Neon Cyan Ring, Slot 2 = Center Deck
for s in range(segs):
    s_next = (s + 1) % segs

    # Outer chamfered wall (Slot 0)
    f_wall = bm_table.faces.new([v_bottom[s], v_bottom[s_next], v_rim_outer[s_next], v_rim_outer[s]])
    f_wall.material_index = 0

    # Top outer rim flat (Slot 0)
    f_rim = bm_table.faces.new([v_rim_outer[s], v_rim_outer[s_next], v_rim_inner[s_next], v_rim_inner[s]])
    f_rim.material_index = 0

    # Outer wall of neon trough dropping down (Slot 1)
    f_ndrop1 = bm_table.faces.new([v_rim_inner[s], v_rim_inner[s_next], v_neon_bottom_out[s_next], v_neon_bottom_out[s]])
    f_ndrop1.material_index = 1

    # Floor of neon trough (Slot 1 - Emissive Cyan)
    f_nfloor = bm_table.faces.new([v_neon_bottom_out[s], v_neon_bottom_out[s_next], v_neon_bottom_in[s_next], v_neon_bottom_in[s]])
    f_nfloor.material_index = 1

    # Inner wall of neon trough rising up to deck (Slot 1)
    f_ndrop2 = bm_table.faces.new([v_neon_bottom_in[s], v_neon_bottom_in[s_next], v_deck_outer[s_next], v_deck_outer[s]])
    f_ndrop2.material_index = 1

    # Center standing deck pie wedge (Slot 2)
    f_deck = bm_table.faces.new([v_deck_outer[s], v_deck_outer[s_next], v_deck_center])
    f_deck.material_index = 2

# Bottom cap
bm_table.faces.new(list(reversed(v_bottom)))

mesh_table = bpy.data.meshes.new("SM_Hangar_Turntable")
bm_table.to_mesh(mesh_table)
bm_table.free()

obj_table = bpy.data.objects.new("SM_Hangar_Turntable", mesh_table)
bpy.context.collection.objects.link(obj_table)

mat_rim  = bpy.data.materials.new("M_Turntable_Steel")
mat_neon = bpy.data.materials.new("M_Turntable_CyanNeon")
mat_deck = bpy.data.materials.new("M_Turntable_Deck")
obj_table.data.materials.append(mat_rim)
obj_table.data.materials.append(mat_neon)
obj_table.data.materials.append(mat_deck)

# Export both to Meshes/SM_Hangar_Turntable.fbx and root Content/Environment/Hangar/SM_Hangar_Plinth.fbx
turntable_fbx_path = os.path.join(mesh_dir, "SM_Hangar_Turntable.fbx")
plinth_fbx_path = os.path.join(hangar_dir, "SM_Hangar_Plinth.fbx")

bpy.ops.export_scene.fbx(filepath=turntable_fbx_path, use_selection=False, axis_forward='X', axis_up='Z', apply_scale_options='FBX_SCALE_ALL', bake_space_transform=True)
bpy.ops.export_scene.fbx(filepath=plinth_fbx_path, use_selection=False, axis_forward='X', axis_up='Z', apply_scale_options='FBX_SCALE_ALL', bake_space_transform=True)
print(f"Exported Turntable: {turntable_fbx_path} and upgraded {plinth_fbx_path}")

print("=== HANGAR 3D ASSETS PIPELINE COMPLETED SUCCESSFULLY ===")
