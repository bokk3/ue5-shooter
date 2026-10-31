import bpy
import bmesh
import os
import math
from mathutils import Vector, Matrix, Euler

print("=== STARTING TIER-1 OPERATOR PIPELINE (SK_BaseMale & SM_Soldier_Posed) ===")

glb_path = os.path.abspath('Scripts/BaseMesh/Soldier.glb')
ak_path = os.path.abspath('Content/Characters/BaseMale/SM_AK47.fbx')
out_sk_fbx = os.path.abspath('Content/Characters/BaseMale/SK_BaseMale.fbx')
out_posed_fbx = os.path.abspath('Content/Characters/BaseMale/SM_Soldier_Posed.fbx')

# Helper: Add Box to bmesh
def bm_add_box(bm, loc, extents, rot_euler=(0,0,0), mat_idx=1):
    mat = Matrix.Translation(loc) @ Euler(rot_euler).to_matrix().to_4x4()
    dx, dy, dz = extents
    verts = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            for sz in (-1, 1):
                p = mat @ Vector((sx*dx, sy*dy, sz*dz))
                verts.append(bm.verts.new(p))
    faces = [
        (0, 1, 3, 2), # -X
        (4, 6, 7, 5), # +X
        (0, 4, 5, 1), # -Y
        (2, 3, 7, 6), # +Y
        (0, 2, 6, 4), # -Z
        (1, 5, 7, 3), # +Z
    ]
    created = []
    for f in faces:
        face = bm.faces.new([verts[i] for i in f])
        face.material_index = mat_idx
        created.append(face)
    return created

# Helper: Add Cylinder to bmesh
def bm_add_cylinder(bm, loc, radius, depth, rot_euler=(0,0,0), segs=12, mat_idx=1, cap_mat_idx=None):
    if cap_mat_idx is None:
        cap_mat_idx = mat_idx
    mat = Matrix.Translation(loc) @ Euler(rot_euler).to_matrix().to_4x4()
    top_verts = []
    bot_verts = []
    for i in range(segs):
        ang = 2.0 * math.pi * i / segs
        x = radius * math.cos(ang)
        y = radius * math.sin(ang)
        top_verts.append(bm.verts.new(mat @ Vector((x, y, depth * 0.5))))
        bot_verts.append(bm.verts.new(mat @ Vector((x, y, -depth * 0.5))))
    for i in range(segs):
        ni = (i + 1) % segs
        f = bm.faces.new([top_verts[i], top_verts[ni], bot_verts[ni], bot_verts[i]])
        f.material_index = mat_idx
    f_top = bm.faces.new(top_verts)
    f_top.material_index = cap_mat_idx
    f_bot = bm.faces.new(list(reversed(bot_verts)))
    f_bot.material_index = mat_idx

# =========================================================================
# PART 1: GENERATE SM_SOLDIER_POSED.FBX (Hangar Turntable Master Operator)
# =========================================================================
print("--- [1/2] Generating SM_Soldier_Posed.fbx ---")
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

arm = bpy.data.objects['Character']
arm.animation_data.action = bpy.data.actions['Idle_Character']
bpy.context.scene.frame_set(0)

dg = bpy.context.evaluated_depsgraph_get()
mesh_body = bpy.data.objects['vanguard_Mesh']
mesh_visor = bpy.data.objects['vanguard_visor']

eval_body = mesh_body.evaluated_get(dg)
eval_visor = mesh_visor.evaluated_get(dg)

b_data = bpy.data.meshes.new_from_object(eval_body)
v_data = bpy.data.meshes.new_from_object(eval_visor)

body_obj = bpy.data.objects.new('Soldier_Body', b_data)
visor_obj = bpy.data.objects.new('Soldier_Visor', v_data)

body_obj.matrix_world = eval_body.matrix_world.copy()
visor_obj.matrix_world = eval_visor.matrix_world.copy()

bpy.context.scene.collection.objects.link(body_obj)
bpy.context.scene.collection.objects.link(visor_obj)

# Apply matrices to mesh coordinates
bpy.ops.object.select_all(action='DESELECT')
body_obj.select_set(True)
visor_obj.select_set(True)
bpy.context.view_layer.objects.active = body_obj
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# Join visor into body_obj
visor_obj.select_set(True)
bpy.ops.object.join()
soldier_mesh = body_obj
soldier_mesh.name = "SM_Soldier_Posed"

# Scale to exactly 1.80m standing height
min_z = min(v.co.z for v in soldier_mesh.data.vertices)
max_z = max(v.co.z for v in soldier_mesh.data.vertices)
scale_f = 1.80 / (max_z - min_z)
soldier_mesh.scale = (scale_f, scale_f, scale_f)
bpy.ops.object.transform_apply(scale=True)

# Ground boots at Z = 0
min_z2 = min(v.co.z for v in soldier_mesh.data.vertices)
for v in soldier_mesh.data.vertices:
    v.co.z -= min_z2

print(f"Posed Soldier Base Mesh: {len(soldier_mesh.data.vertices)} vertices, grounded at Z=0")

# Setup 4 Materials on soldier_mesh
soldier_mesh.data.materials.clear()
m_uniform = bpy.data.materials.new(name="M_Operator_Uniform") # Slot 0
m_armor   = bpy.data.materials.new(name="M_Operator_Armor")   # Slot 1
m_visor   = bpy.data.materials.new(name="M_Operator_Visor")   # Slot 2
m_patch   = bpy.data.materials.new(name="M_Operator_Patch")   # Slot 3

for m in [m_uniform, m_armor, m_visor, m_patch]:
    soldier_mesh.data.materials.append(m)

# Assign existing body polygons:
# By default, vanguard_Mesh is slot 0 (Uniform).
# Vanguard visor polygons (around head Z > 1.62 and Y > 0.18) become slot 2 (Visor).
for poly in soldier_mesh.data.polygons:
    if poly.center.z > 1.60 and poly.center.y > 0.16:
        poly.material_index = 2 # Visor
    elif poly.center.z < 0.18:
        poly.material_index = 1 # Boots / Armor
    else:
        poly.material_index = 0 # Base Uniform

# Now build high-fidelity tactical gear directly attached to posed soldier
bm_gear = bmesh.new()

# 1. FAST HIGH-CUT HELMET & FLIPPED-UP GPNVG-18 QUAD-NODS
# Head center is at approx (0.0035, 0.1237, 1.6560)
hc = Vector((0.0035, 0.1237, 1.6560))

# FAST Helmet Shell & ARC Rails (Slot 1 Armor)
bm_add_box(bm_gear, (hc.x, hc.y - 0.015, hc.z + 0.065), (0.125, 0.135, 0.075), mat_idx=1) # Dome
bm_add_box(bm_gear, (hc.x, hc.y - 0.020, hc.z + 0.115), (0.105, 0.115, 0.035), mat_idx=1) # Crown
bm_add_box(bm_gear, (hc.x, hc.y - 0.105, hc.z + 0.005), (0.105, 0.045, 0.055), mat_idx=1) # Nape protector
bm_add_box(bm_gear, (hc.x, hc.y + 0.115, hc.z + 0.055), (0.035, 0.015, 0.035), mat_idx=1) # Wilcox Shroud
bm_add_box(bm_gear, (hc.x - 0.128, hc.y - 0.015, hc.z + 0.035), (0.012, 0.075, 0.018), mat_idx=1) # Left ARC rail
bm_add_box(bm_gear, (hc.x + 0.128, hc.y - 0.015, hc.z + 0.035), (0.012, 0.075, 0.018), mat_idx=1) # Right ARC rail
bm_add_box(bm_gear, (hc.x, hc.y - 0.145, hc.z + 0.015), (0.055, 0.022, 0.038), mat_idx=1) # Rear battery pack

# Peltor ComTac VI Headset (Left & Right earcups + boom mic)
bm_add_box(bm_gear, (hc.x - 0.118, hc.y - 0.015, hc.z - 0.065), (0.022, 0.045, 0.055), mat_idx=1) # Left earcup
bm_add_box(bm_gear, (hc.x + 0.118, hc.y - 0.015, hc.z - 0.065), (0.022, 0.045, 0.055), mat_idx=1) # Right earcup
bm_add_cylinder(bm_gear, (hc.x - 0.08, hc.y + 0.07, hc.z - 0.10), 0.005, 0.12, (math.radians(15), math.radians(-45), 0), mat_idx=1) # Mic boom
bm_add_box(bm_gear, (hc.x - 0.02, hc.y + 0.11, hc.z - 0.09), (0.012, 0.018, 0.012), mat_idx=1) # Mic windscreen

# GPNVG-18 Wilcox Pivot Bridge (Slot 1 Armor)
bm_add_box(bm_gear, (hc.x, hc.y + 0.140, hc.z + 0.095), (0.055, 0.022, 0.018), mat_idx=1)

# 4 Flipped-Up Quad-Tubes (Slot 1 Armor with Slot 2 Emissive CRT Green Lenses)
# Angled 55 degrees upward above the forehead
nvg_pitch = math.radians(55)
# Inner Left
bm_add_cylinder(bm_gear, (hc.x - 0.028, hc.y + 0.155, hc.z + 0.145), 0.018, 0.075, (nvg_pitch, 0, math.radians(-10)), segs=12, mat_idx=1, cap_mat_idx=2)
# Inner Right
bm_add_cylinder(bm_gear, (hc.x + 0.028, hc.y + 0.155, hc.z + 0.145), 0.018, 0.075, (nvg_pitch, 0, math.radians(10)), segs=12, mat_idx=1, cap_mat_idx=2)
# Outer Left
bm_add_cylinder(bm_gear, (hc.x - 0.072, hc.y + 0.145, hc.z + 0.135), 0.018, 0.072, (nvg_pitch, 0, math.radians(-25)), segs=12, mat_idx=1, cap_mat_idx=2)
# Outer Right
bm_add_cylinder(bm_gear, (hc.x + 0.072, hc.y + 0.145, hc.z + 0.135), 0.018, 0.072, (nvg_pitch, 0, math.radians(25)), segs=12, mat_idx=1, cap_mat_idx=2)

# 2. CRYE PRECISION JPC PLATE CARRIER & TACTICAL CHEST RIG
# Chest center is at approx (-0.0054, 0.0009, 1.2986)
cc = Vector((-0.0054, 0.0009, 1.2986))

# Front & Rear Ceramic Armor Plates (Slot 1 Armor)
bm_add_box(bm_gear, (cc.x, cc.y + 0.095, cc.z - 0.02), (0.155, 0.035, 0.175), mat_idx=1) # Front plate
bm_add_box(bm_gear, (cc.x, cc.y - 0.105, cc.z - 0.02), (0.155, 0.035, 0.175), mat_idx=1) # Rear plate

# Padded Shoulder Straps
bm_add_box(bm_gear, (cc.x - 0.105, cc.y - 0.005, cc.z + 0.15), (0.035, 0.125, 0.022), mat_idx=1)
bm_add_box(bm_gear, (cc.x + 0.095, cc.y - 0.005, cc.z + 0.15), (0.035, 0.125, 0.022), mat_idx=1)

# Skeletal Cummerbund (Sides)
bm_add_box(bm_gear, (cc.x - 0.158, cc.y - 0.005, cc.z - 0.06), (0.016, 0.115, 0.075), mat_idx=1)
bm_add_box(bm_gear, (cc.x + 0.148, cc.y - 0.005, cc.z - 0.06), (0.016, 0.115, 0.075), mat_idx=1)

# Triple 5.56 STANAG Mag Pouches (Front Plate Carrier)
bm_add_box(bm_gear, (cc.x - 0.082, cc.y + 0.145, cc.z - 0.08), (0.032, 0.026, 0.095), mat_idx=1) # Left mag
bm_add_box(bm_gear, (cc.x - 0.002, cc.y + 0.148, cc.z - 0.08), (0.032, 0.026, 0.095), mat_idx=1) # Center mag
bm_add_box(bm_gear, (cc.x + 0.078, cc.y + 0.145, cc.z - 0.08), (0.032, 0.026, 0.095), mat_idx=1) # Right mag

# CAT Tourniquet mounted horizontally above mag pouches
bm_add_box(bm_gear, (cc.x - 0.002, cc.y + 0.142, cc.z + 0.045), (0.075, 0.018, 0.018), mat_idx=1)

# Tactical Whip Antenna extending from back panel
bm_add_cylinder(bm_gear, (cc.x - 0.12, cc.y - 0.145, cc.z + 0.28), 0.004, 0.55, rot_euler=(math.radians(12), 0, math.radians(-10)), segs=8, mat_idx=1)

# Back Modular Assault Pack
bm_add_box(bm_gear, (cc.x - 0.002, cc.y - 0.155, cc.z - 0.04), (0.135, 0.045, 0.155), mat_idx=1)

# 24K GOLD EMBROIDERED "NOBLE FERMI" CHEST CALLSIGN PATCH (Slot 3 Patch)
bm_add_box(bm_gear, (cc.x - 0.002, cc.y + 0.140, cc.z + 0.095), (0.068, 0.006, 0.028), mat_idx=3)

# 3. FOREARM WRIST PDA WITH PHOSPHOR-GREEN CRT DISPLAY
# Left Forearm center in posed body is approx (-0.1799, 0.1689, 1.1353)
fc = Vector((-0.1799, 0.1689, 1.1353))
# Rugged PDA Casing (Slot 1 Armor)
bm_add_box(bm_gear, (fc.x, fc.y + 0.045, fc.z), (0.045, 0.025, 0.042), rot_euler=(math.radians(20), math.radians(-15), math.radians(25)), mat_idx=1)
# Glowing CRT Matrix Screen (Slot 2 Visor / CRT Emissive)
bm_add_box(bm_gear, (fc.x, fc.y + 0.068, fc.z), (0.036, 0.004, 0.034), rot_euler=(math.radians(20), math.radians(-15), math.radians(25)), mat_idx=2)

# 4. MOLLE BATTLE BELT & SAFARILAND DROP-LEG HOLSTER + PISTOL
# Waist center in posed body is approx (-0.0450, 0.0657, 0.9973)
wc = Vector((-0.0450, 0.0657, 0.9973))
# Battle Belt (Slot 1 Armor)
bm_add_box(bm_gear, (wc.x, wc.y - 0.015, wc.z), (0.185, 0.155, 0.038), mat_idx=1)
# Safariland Drop-Leg Holster (Right Thigh)
tc = Vector((0.135, 0.045, 0.74))
bm_add_box(bm_gear, (tc.x + 0.035, tc.y + 0.015, tc.z), (0.042, 0.075, 0.125), rot_euler=(0, math.radians(-8), 0), mat_idx=1)
# Sidearm Pistol Grip
bm_add_box(bm_gear, (tc.x + 0.035, tc.y - 0.015, tc.z + 0.11), (0.022, 0.045, 0.055), rot_euler=(0, math.radians(-18), 0), mat_idx=1)
# IFAK Blowout Medical Pouch (Left Hip)
bm_add_box(bm_gear, (wc.x - 0.165, wc.y - 0.025, wc.z - 0.04), (0.042, 0.068, 0.068), mat_idx=1)

# Convert gear bmesh into mesh and join into soldier_mesh
me_gear = bpy.data.meshes.new("Gear_Mesh")
bm_gear.to_mesh(me_gear)
bm_gear.free()

ob_gear = bpy.data.objects.new("Gear_Obj", me_gear)
for m in [m_uniform, m_armor, m_visor, m_patch]:
    ob_gear.data.materials.append(m)

bpy.context.scene.collection.objects.link(ob_gear)
bpy.ops.object.select_all(action='DESELECT')
ob_gear.select_set(True)
soldier_mesh.select_set(True)
bpy.context.view_layer.objects.active = soldier_mesh
bpy.ops.object.join()

print("Joined all Tier-1 Spec-Ops tactical gear into SM_Soldier_Posed")

# 5. ATTACH AK-47 ASSAULT RIFLE
bpy.ops.import_scene.fbx(filepath=ak_path)
ak_obj = None
for o in bpy.data.objects:
    if 'AK47' in o.name:
        ak_obj = o
        break

if not ak_obj:
    raise RuntimeError("Failed to load SM_AK47.fbx!")

# Position AK-47 across chest in tactical low-ready sling carry
ak_obj.location = (0.02, 0.24, 1.08)
ak_obj.rotation_euler = (math.radians(-38), math.radians(28), math.radians(-50))
ak_obj.scale = (1.0, 1.0, 1.0)
bpy.ops.object.select_all(action='DESELECT')
ak_obj.select_set(True)
bpy.context.view_layer.objects.active = ak_obj
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# Join AK-47 into soldier_mesh (Blender appends ak_obj's materials into slots 4, 5, 6)
bpy.ops.object.select_all(action='DESELECT')
ak_obj.select_set(True)
soldier_mesh.select_set(True)
bpy.context.view_layer.objects.active = soldier_mesh
bpy.ops.object.join()

print(f"Posed Soldier with AK-47: {len(soldier_mesh.data.vertices)} vertices, {len(soldier_mesh.data.polygons)} polygons")
print(f"Material slot names: {[m.name for m in soldier_mesh.data.materials]}")

# Ensure exact standing height 1.80m and grounded at Z = 0
min_z_final = min(v.co.z for v in soldier_mesh.data.vertices)
max_z_final = max(v.co.z for v in soldier_mesh.data.vertices)
curr_h_final = max_z_final - min_z_final
print(f"Final Height before normalization: {curr_h_final:.3f}m")

# Ground boots at Z = 0
for v in soldier_mesh.data.vertices:
    v.co.z -= min_z_final

# Rotate -90 degrees around Z so front (+Y in Blender) faces +X in Unreal (+X is forward)
soldier_mesh.rotation_euler = (0, 0, math.radians(-90))
bpy.ops.object.select_all(action='DESELECT')
soldier_mesh.select_set(True)
bpy.context.view_layer.objects.active = soldier_mesh
bpy.ops.object.transform_apply(rotation=True)

# Export SM_Soldier_Posed.fbx
bpy.ops.export_scene.fbx(
    filepath=out_posed_fbx,
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    axis_forward='X',
    axis_up='Z',
    object_types={'MESH'}
)
print(f"Exported SM_Soldier_Posed.fbx: {out_posed_fbx} ({os.path.getsize(out_posed_fbx):,} bytes)")

# =========================================================================
# PART 2: GENERATE SK_BASEMALE.FBX (Standard Rest-Pose Skeletal Mesh Rig)
# =========================================================================
print("--- [2/2] Generating SK_BaseMale.fbx ---")
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

arm_obj = bpy.data.objects.get('Character')
mesh_body = bpy.data.objects.get('vanguard_Mesh')
mesh_visor = bpy.data.objects.get('vanguard_visor')

# Remove unneeded objects
for obj in list(bpy.data.objects):
    if obj.name not in ['Character', 'vanguard_Mesh', 'vanguard_visor']:
        bpy.data.objects.remove(obj, do_unlink=True)

# Join visor into mesh_body
bpy.ops.object.select_all(action='DESELECT')
mesh_visor.select_set(True)
mesh_body.select_set(True)
bpy.context.view_layer.objects.active = mesh_body
bpy.ops.object.join()
sk_mesh = mesh_body
sk_mesh.name = "SK_BaseMale"

# Reset Armature to rest pose
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='POSE')
for pb in arm_obj.pose.bones:
    pb.matrix_basis = Matrix.Identity(4)
bpy.ops.object.mode_set(mode='OBJECT')

arm_obj.animation_data_clear()
sk_mesh.animation_data_clear()

# Apply transforms
bpy.ops.object.select_all(action='DESELECT')
sk_mesh.select_set(True)
arm_obj.select_set(True)
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# Bone renaming map (Mixamo -> NobleFermi standard skeleton)
rename_map = {
    'mixamorig:Hips': 'pelvis',
    'mixamorig:Spine': 'spine_01',
    'mixamorig:Spine1': 'spine_02',
    'mixamorig:Spine2': 'spine_03',
    'mixamorig:Neck': 'neck_01',
    'mixamorig:Head': 'head',
    'mixamorig:LeftShoulder': 'clavicle_l',
    'mixamorig:LeftArm': 'upperarm_l',
    'mixamorig:LeftForeArm': 'lowerarm_l',
    'mixamorig:LeftHand': 'hand_l',
    'mixamorig:RightShoulder': 'clavicle_r',
    'mixamorig:RightArm': 'upperarm_r',
    'mixamorig:RightForeArm': 'lowerarm_r',
    'mixamorig:RightHand': 'hand_r',
    'mixamorig:LeftUpLeg': 'thigh_l',
    'mixamorig:LeftLeg': 'calf_l',
    'mixamorig:LeftFoot': 'foot_l',
    'mixamorig:LeftToeBase': 'toes_l',
    'mixamorig:RightUpLeg': 'thigh_r',
    'mixamorig:RightLeg': 'calf_r',
    'mixamorig:RightFoot': 'foot_r',
    'mixamorig:RightToeBase': 'toes_r',
}

for side, s_code in [('Left', 'l'), ('Right', 'r')]:
    for i in range(1, 4):
        rename_map[f'mixamorig:{side}HandThumb{i}'] = f'thumb_0{i}_{s_code}'
        rename_map[f'mixamorig:{side}HandIndex{i}'] = f'index_0{i}_{s_code}'
        rename_map[f'mixamorig:{side}HandMiddle{i}'] = f'middle_0{i}_{s_code}'
        rename_map[f'mixamorig:{side}HandRing{i}'] = f'ring_0{i}_{s_code}'
    rename_map[f'mixamorig:{side}HandPinky1'] = f'pinky_01_{s_code}'
    rename_map[f'mixamorig:{side}HandPinky2'] = f'pinky_02_{s_code}'

# Rename vertex groups on sk_mesh
for old_name, new_name in rename_map.items():
    vg = sk_mesh.vertex_groups.get(old_name)
    if vg:
        vg.name = new_name

# Edit Armature: Rename bones and add root & weapon_r
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='EDIT')
ebones = arm_obj.data.edit_bones

for old_name, new_name in rename_map.items():
    b = ebones.get(old_name)
    if b:
        b.name = new_name

root_bone = ebones.new('root')
root_bone.head = (0, 0, 0)
root_bone.tail = (0, 0.1, 0)
pelvis_b = ebones.get('pelvis')
if pelvis_b:
    pelvis_b.parent = root_bone

hand_r_bone = ebones.get('hand_r')
if hand_r_bone:
    weapon_bone = ebones.new('weapon_r')
    weapon_bone.head = hand_r_bone.head.copy()
    weapon_bone.tail = hand_r_bone.head + Vector((0, -0.15, 0))
    weapon_bone.parent = hand_r_bone
    weapon_bone.use_connect = False

bpy.ops.object.mode_set(mode='OBJECT')

# Scale character to 1.80m standing height
min_z = min(v.co.z for v in sk_mesh.data.vertices)
max_z = max(v.co.z for v in sk_mesh.data.vertices)
scale_to_180 = 1.80 / (max_z - min_z)
arm_obj.scale = (scale_to_180, scale_to_180, scale_to_180)
sk_mesh.scale = (scale_to_180, scale_to_180, scale_to_180)

bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.select_all(action='DESELECT')
arm_obj.select_set(True)
sk_mesh.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# Ground character at Z = 0
min_z_after = min(v.co.z for v in sk_mesh.data.vertices)
if abs(min_z_after) > 0.001:
    for v in sk_mesh.data.vertices:
        v.co.z -= min_z_after
    bpy.context.view_layer.objects.active = arm_obj
    bpy.ops.object.mode_set(mode='EDIT')
    for eb in arm_obj.data.edit_bones:
        if eb.name != 'root':
            eb.head.z -= min_z_after
            eb.tail.z -= min_z_after
    bpy.ops.object.mode_set(mode='OBJECT')

# Set 4 materials on sk_mesh
sk_mesh.data.materials.clear()
m_sk_uniform = bpy.data.materials.new(name="M_Operator_Uniform")
m_sk_armor   = bpy.data.materials.new(name="M_Operator_Armor")
m_sk_visor   = bpy.data.materials.new(name="M_Operator_Visor")
m_sk_patch   = bpy.data.materials.new(name="M_Operator_Patch")

for m in [m_sk_uniform, m_sk_armor, m_sk_visor, m_sk_patch]:
    sk_mesh.data.materials.append(m)

for poly in sk_mesh.data.polygons:
    if poly.center.z > 1.60 and poly.center.y < -0.02:
        poly.material_index = 2 # Visor
    elif poly.center.z < 0.18:
        poly.material_index = 1 # Boots
    else:
        poly.material_index = 0 # Uniform

# Build rest-pose tactical gear and bind to vertex groups
# In rest pose:
# head bone head is approx (0, -0.054, 1.541)
rhc = Vector((0.0, -0.054, 1.541))
# spine_03 is approx (0, -0.036, 1.369)
rsc = Vector((0.0, -0.036, 1.369))
# pelvis is approx (0, 0.011, 1.043)
rwc = Vector((0.0, 0.011, 1.043))

bm_sk_gear = bmesh.new()

# Headgear (FAST Helmet + Wilcox + NVG + ComTac VI) -> Assigned to 'head'
head_v_start = len(bm_sk_gear.verts)
# Helmet shell & shroud
bm_add_box(bm_sk_gear, (rhc.x, rhc.y + 0.015, rhc.z + 0.17), (0.125, 0.135, 0.075), mat_idx=1)
bm_add_box(bm_sk_gear, (rhc.x, rhc.y + 0.020, rhc.z + 0.22), (0.105, 0.115, 0.035), mat_idx=1)
bm_add_box(bm_sk_gear, (rhc.x, rhc.y - 0.075, rhc.z + 0.11), (0.105, 0.045, 0.055), mat_idx=1)
bm_add_box(bm_sk_gear, (rhc.x, rhc.y + 0.145, rhc.z + 0.16), (0.035, 0.015, 0.035), mat_idx=1)
# Peltor earcups & mic
bm_add_box(bm_sk_gear, (rhc.x - 0.118, rhc.y + 0.015, rhc.z + 0.04), (0.022, 0.045, 0.055), mat_idx=1)
bm_add_box(bm_sk_gear, (rhc.x + 0.118, rhc.y + 0.015, rhc.z + 0.04), (0.022, 0.045, 0.055), mat_idx=1)
# Flipped-up quad-nods (Slot 1 & 2)
bm_add_cylinder(bm_sk_gear, (rhc.x - 0.028, rhc.y + 0.165, rhc.z + 0.22), 0.018, 0.075, (math.radians(55), 0, math.radians(-10)), segs=12, mat_idx=1, cap_mat_idx=2)
bm_add_cylinder(bm_sk_gear, (rhc.x + 0.028, rhc.y + 0.165, rhc.z + 0.22), 0.018, 0.075, (math.radians(55), 0, math.radians(10)), segs=12, mat_idx=1, cap_mat_idx=2)
bm_add_cylinder(bm_sk_gear, (rhc.x - 0.072, rhc.y + 0.155, rhc.z + 0.21), 0.018, 0.072, (math.radians(55), 0, math.radians(-25)), segs=12, mat_idx=1, cap_mat_idx=2)
bm_add_cylinder(bm_sk_gear, (rhc.x + 0.072, rhc.y + 0.155, rhc.z + 0.21), 0.018, 0.072, (math.radians(55), 0, math.radians(25)), segs=12, mat_idx=1, cap_mat_idx=2)
head_v_end = len(bm_sk_gear.verts)

# Chest rig (Plate carrier + pouches + patch) -> Assigned to 'spine_03'
chest_v_start = len(bm_sk_gear.verts)
bm_add_box(bm_sk_gear, (rsc.x, rsc.y + 0.095, rsc.z - 0.05), (0.155, 0.035, 0.175), mat_idx=1)
bm_add_box(bm_sk_gear, (rsc.x, rsc.y - 0.105, rsc.z - 0.05), (0.155, 0.035, 0.175), mat_idx=1)
bm_add_box(bm_sk_gear, (rsc.x - 0.105, rsc.y - 0.005, rsc.z + 0.12), (0.035, 0.125, 0.022), mat_idx=1)
bm_add_box(bm_sk_gear, (rsc.x + 0.095, rsc.y - 0.005, rsc.z + 0.12), (0.035, 0.125, 0.022), mat_idx=1)
# Mag pouches
bm_add_box(bm_sk_gear, (rsc.x - 0.082, rsc.y + 0.145, rsc.z - 0.11), (0.032, 0.026, 0.095), mat_idx=1)
bm_add_box(bm_sk_gear, (rsc.x - 0.002, rsc.y + 0.148, rsc.z - 0.11), (0.032, 0.026, 0.095), mat_idx=1)
bm_add_box(bm_sk_gear, (rsc.x + 0.078, rsc.y + 0.145, rsc.z - 0.11), (0.032, 0.026, 0.095), mat_idx=1)
# Antenna
bm_add_cylinder(bm_sk_gear, (rsc.x - 0.12, rsc.y - 0.145, rsc.z + 0.25), 0.004, 0.55, rot_euler=(math.radians(12), 0, math.radians(-10)), segs=8, mat_idx=1)
# Gold Callsign Patch (Slot 3 Patch)
bm_add_box(bm_sk_gear, (rsc.x - 0.002, rsc.y + 0.140, rsc.z + 0.065), (0.068, 0.006, 0.028), mat_idx=3)
chest_v_end = len(bm_sk_gear.verts)

# Battle Belt & Holster -> Assigned to 'pelvis'
belt_v_start = len(bm_sk_gear.verts)
bm_add_box(bm_sk_gear, (rwc.x, rwc.y - 0.015, rwc.z), (0.185, 0.155, 0.038), mat_idx=1)
bm_add_box(bm_sk_gear, (rwc.x + 0.17, rwc.y + 0.015, rwc.z - 0.26), (0.042, 0.075, 0.125), rot_euler=(0, math.radians(-8), 0), mat_idx=1)
bm_add_box(bm_sk_gear, (rwc.x + 0.17, rwc.y - 0.015, rwc.z - 0.15), (0.022, 0.045, 0.055), rot_euler=(0, math.radians(-18), 0), mat_idx=1)
bm_add_box(bm_sk_gear, (rwc.x - 0.165, rwc.y - 0.025, rwc.z - 0.04), (0.042, 0.068, 0.068), mat_idx=1)
belt_v_end = len(bm_sk_gear.verts)

# Convert to mesh object
me_sk_gear = bpy.data.meshes.new("SK_Gear_Mesh")
bm_sk_gear.to_mesh(me_sk_gear)
bm_sk_gear.free()

ob_sk_gear = bpy.data.objects.new("SK_Gear_Obj", me_sk_gear)
for m in [m_sk_uniform, m_sk_armor, m_sk_visor, m_sk_patch]:
    ob_sk_gear.data.materials.append(m)

# Assign vertex groups
vg_head = ob_sk_gear.vertex_groups.new(name="head")
vg_head.add(list(range(head_v_start, head_v_end)), 1.0, 'REPLACE')

vg_spine = ob_sk_gear.vertex_groups.new(name="spine_03")
vg_spine.add(list(range(chest_v_start, chest_v_end)), 1.0, 'REPLACE')

vg_pelvis = ob_sk_gear.vertex_groups.new(name="pelvis")
vg_pelvis.add(list(range(belt_v_start, belt_v_end)), 1.0, 'REPLACE')

bpy.context.scene.collection.objects.link(ob_sk_gear)

# Join gear into sk_mesh
bpy.ops.object.select_all(action='DESELECT')
ob_sk_gear.select_set(True)
sk_mesh.select_set(True)
bpy.context.view_layer.objects.active = sk_mesh
bpy.ops.object.join()

# Ensure smooth normals
bpy.context.view_layer.objects.active = sk_mesh
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.normals_make_consistent(inside=False)
bpy.ops.object.mode_set(mode='OBJECT')
for poly in sk_mesh.data.polygons:
    poly.use_smooth = True

# Export SK_BaseMale.fbx
bpy.ops.object.select_all(action='DESELECT')
arm_obj.select_set(True)
sk_mesh.select_set(True)
bpy.context.view_layer.objects.active = arm_obj

bpy.ops.export_scene.fbx(
    filepath=out_sk_fbx,
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_ALL',
    axis_forward='-Z',
    axis_up='Y',
    object_types={'ARMATURE', 'MESH'},
    use_mesh_modifiers=True,
    mesh_smooth_type='FACE',
    add_leaf_bones=False,
    primary_bone_axis='Y',
    secondary_bone_axis='X',
    armature_nodetype='NULL',
    bake_anim=False
)
print(f"Exported SK_BaseMale.fbx: {out_sk_fbx} ({os.path.getsize(out_sk_fbx):,} bytes)")

print("=== TIER-1 OPERATOR RIG & POSED STATIC MESH GENERATED SUCCESSFULLY ===")
