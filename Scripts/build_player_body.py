import bpy
import bmesh
import os
import math
from mathutils import Vector, Matrix, Euler

print("=== STARTING BUILD_PLAYER_BODY PIPELINE (V3: IK-SOLVED AK-47 STANCE & WEAPON_R SOCKET) ===")

blend_path = os.path.abspath('Scripts/BaseMesh/Male_Human.blend')
bpy.ops.wm.read_factory_settings(use_empty=True)

with bpy.data.libraries.load(blend_path) as (data_from, data_to):
    data_to.objects = ['T_Male_04_Deform', 'T_Metarig_Finger']

mesh_obj = [o for o in data_to.objects if o.type == 'MESH'][0]
arm_obj = [o for o in data_to.objects if o.type == 'ARMATURE'][0]

bpy.context.scene.collection.objects.link(mesh_obj)
bpy.context.scene.collection.objects.link(arm_obj)

# 0. Cleanly unparent and zero world location to (0, 0, 0)
bpy.ops.object.select_all(action='DESELECT')
mesh_obj.select_set(True)
bpy.context.view_layer.objects.active = mesh_obj
bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')

arm_obj.location = (0, 0, 0)
mesh_obj.location = (0, 0, 0)

bpy.ops.object.select_all(action='DESELECT')
mesh_obj.select_set(True)
arm_obj.select_set(True)
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

mesh_obj.parent = arm_obj
mesh_obj.matrix_parent_inverse = arm_obj.matrix_world.inverted()

# 1. Clean Vertex Group Merging
def merge_vg_clean(source_name, target_name):
    src_vg = mesh_obj.vertex_groups.get(source_name)
    tgt_vg = mesh_obj.vertex_groups.get(target_name)
    if not src_vg or not tgt_vg:
        return
    src_idx = src_vg.index
    for v in mesh_obj.data.vertices:
        for g in v.groups:
            if g.group == src_idx:
                tgt_vg.add([v.index], g.weight, 'ADD')
                break
    mesh_obj.vertex_groups.remove(src_vg)

merge_vg_clean('spine.006', 'spine.005')
merge_vg_clean('pelvis.L', 'spine')
merge_vg_clean('pelvis.R', 'spine')
merge_vg_clean('heel.02.L', 'foot.L')
merge_vg_clean('heel.02.R', 'foot.R')

for side in ['L', 'R']:
    hand = f'hand.{side}'
    for i in range(1, 5):
        palm = f'palm.0{i}.{side}'
        merge_vg_clean(palm, hand)

rename_map = {
    'spine': 'pelvis',
    'spine.001': 'spine_01',
    'spine.002': 'spine_02',
    'spine.003': 'spine_03',
    'spine.004': 'neck_01',
    'spine.005': 'head',
    'shoulder.L': 'clavicle_l',
    'shoulder.R': 'clavicle_r',
    'upper_arm.L': 'upperarm_l',
    'upper_arm.R': 'upperarm_r',
    'forearm.L': 'lowerarm_l',
    'forearm.R': 'lowerarm_r',
    'hand.L': 'hand_l',
    'hand.R': 'hand_r',
    'thigh.L': 'thigh_l',
    'thigh.R': 'thigh_r',
    'shin.L': 'calf_l',
    'shin.R': 'calf_r',
    'foot.L': 'foot_l',
    'foot.R': 'foot_r',
    'toe.L': 'toes_l',
    'toe.R': 'toes_r',
}

for side, s_code in [('L', 'l'), ('R', 'r')]:
    for i in range(1, 4):
        rename_map[f'f_index.0{i}.{side}'] = f'index_0{i}_{s_code}'
        rename_map[f'thumb.0{i}.{side}'] = f'thumb_0{i}_{s_code}'
        rename_map[f'f_middle.0{i}.{side}'] = f'middle_0{i}_{s_code}'
        rename_map[f'f_ring.0{i}.{side}'] = f'ring_0{i}_{s_code}'
        rename_map[f'f_pinky.0{i}.{side}'] = f'pinky_0{i}_{s_code}'

for old_name, new_name in rename_map.items():
    vg = mesh_obj.vertex_groups.get(old_name)
    if vg:
        vg.name = new_name

# 2. Edit Armature: Rename bones, remove unneeded bones, add root bone
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='EDIT')
ebones = arm_obj.data.edit_bones

for b_del in ['spine.006', 'pelvis.L', 'pelvis.R', 'heel.02.L', 'heel.02.R']:
    b = ebones.get(b_del)
    if b:
        ebones.remove(b)

for side in ['L', 'R']:
    for i in range(1, 5):
        b = ebones.get(f'palm.0{i}.{side}')
        if b:
            hand_bone = ebones.get(f'hand.{side}')
            for child in list(b.children):
                child.parent = hand_bone
            ebones.remove(b)

root_bone = ebones.new('root')
root_bone.head = (0, 0, 0)
root_bone.tail = (0, 0.1, 0)
root_bone.roll = 0.0

pelvis_bone = ebones.get('spine')
if pelvis_bone:
    pelvis_bone.parent = root_bone

for old_name, new_name in rename_map.items():
    b = ebones.get(old_name)
    if b:
        b.name = new_name

bpy.ops.object.mode_set(mode='OBJECT')

# 3. Scaling to exact 180cm standing height
min_z = min(v.co.z for v in mesh_obj.data.vertices)
max_z = max(v.co.z for v in mesh_obj.data.vertices)
current_h = max_z - min_z
scale_to_180 = 1.80 / current_h

print(f"Mesh Z Range before: [{min_z:.4f}, {max_z:.4f}], Height: {current_h:.4f}m -> Scaling by {scale_to_180:.4f}")

arm_obj.scale = (scale_to_180, scale_to_180, scale_to_180)
mesh_obj.scale = (scale_to_180, scale_to_180, scale_to_180)

bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.select_all(action='DESELECT')
arm_obj.select_set(True)
mesh_obj.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# Offset to sit exactly on ground plane Z = 0
min_z_after = min(v.co.z for v in mesh_obj.data.vertices)
if abs(min_z_after) > 0.001:
    print(f"Grounding character: offset by {-min_z_after:.4f}m in Z")
    for v in mesh_obj.data.vertices:
        v.co.z -= min_z_after
    bpy.context.view_layer.objects.active = arm_obj
    bpy.ops.object.mode_set(mode='EDIT')
    for eb in arm_obj.data.edit_bones:
        if eb.name != 'root':
            eb.head.z -= min_z_after
            eb.tail.z -= min_z_after
    bpy.ops.object.mode_set(mode='OBJECT')

# Add weapon_r bone (child of hand_r) with identity rest orientation at (0,0,0)->(0,0.1,0)
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='EDIT')
weapon_bone = arm_obj.data.edit_bones.new('weapon_r')
weapon_bone.head = (0.0, 0.0, 0.0)
weapon_bone.tail = (0.0, 0.10, 0.0)
weapon_bone.roll = 0.0
weapon_bone.parent = arm_obj.data.edit_bones['hand_r']
weapon_bone.use_connect = False
bpy.ops.object.mode_set(mode='OBJECT')

final_min_z = min(v.co.z for v in mesh_obj.data.vertices)
final_max_z = max(v.co.z for v in mesh_obj.data.vertices)
print(f"Final Calibrated Z Range: [{final_min_z:.4f}m, {final_max_z:.4f}m] (Height = {final_max_z - final_min_z:.4f}m)")

# Ensure all face normals are consistently facing outward & smooth shaded
bpy.context.view_layer.objects.active = mesh_obj
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.normals_make_consistent(inside=False)
bpy.ops.object.mode_set(mode='OBJECT')
for poly in mesh_obj.data.polygons:
    poly.use_smooth = True

# 4. Build Procedural AK-47 Mesh (SM_AK47) and Export StaticMesh FBX
def create_ak47_mesh():
    bm = bmesh.new()
    def add_box(center, size, rot_x_deg=0.0, mat_idx=0):
        ret = bmesh.ops.create_cube(bm, size=1.0)
        verts = ret['verts']
        bmesh.ops.scale(bm, vec=size, verts=verts)
        if abs(rot_x_deg) > 0.01:
            bmesh.ops.rotate(bm, cent=(0,0,0), matrix=Matrix.Rotation(math.radians(rot_x_deg), 3, 'X'), verts=verts)
        bmesh.ops.translate(bm, vec=center, verts=verts)
        for f in bm.faces:
            if all(v in verts for v in f.verts):
                f.material_index = mat_idx
        return verts

    # Mat 0 = Gunmetal Steel, Mat 1 = Walnut Wood, Mat 2 = Bakelite Magazine
    # 1. Stamped Steel Receiver (24cm long, 3.4cm wide, 6.0cm tall)
    add_box((0.0, -0.02, 0.012), (0.034, 0.24, 0.060), mat_idx=0)
    # 2. Ribbed Dust Cover
    add_box((0.0, -0.01, 0.048), (0.030, 0.22, 0.020), mat_idx=0)
    # 3. Walnut Wooden Buttstock (extending back +Y from +0.10 to +0.31)
    add_box((0.0, 0.205, -0.018), (0.032, 0.21, 0.072), rot_x_deg=-8.0, mat_idx=1)
    # Steel Buttplate
    add_box((0.0, 0.31, -0.032), (0.034, 0.018, 0.102), mat_idx=0)
    # 4. Ergonomic AK-47 Pistol Grip / Handle -> CONTACT POINT 1 (Right Hand)
    add_box((0.0, 0.055, -0.070), (0.026, 0.040, 0.115), rot_x_deg=18.0, mat_idx=1)
    # Trigger Guard & Trigger
    add_box((0.0, 0.008, -0.032), (0.016, 0.045, 0.024), mat_idx=0)
    add_box((0.0, 0.016, -0.030), (0.006, 0.010, 0.022), rot_x_deg=15.0, mat_idx=0)
    # 5. Lower & Upper Wooden Handguard (Y from -0.14 to -0.32) -> CONTACT POINT 2 (Left Hand)
    add_box((0.0, -0.230, 0.004), (0.038, 0.18, 0.044), mat_idx=1)
    add_box((0.0, -0.225, 0.038), (0.030, 0.16, 0.024), mat_idx=1)
    # 6. Rifled Barrel (Y from -0.32 to -0.58, Z=0.016)
    add_box((0.0, -0.450, 0.016), (0.016, 0.28, 0.016), mat_idx=0)
    # Muzzle Slant Brake
    add_box((0.0, -0.595, 0.016), (0.020, 0.03, 0.020), mat_idx=0)
    # 7. Gas Tube & Gas Block
    add_box((0.0, -0.360, 0.038), (0.015, 0.13, 0.015), mat_idx=0)
    add_box((0.0, -0.420, 0.027), (0.018, 0.03, 0.038), mat_idx=0)
    # 8. Iconic AK-47 Triangular Front Sight Post & Rear Sight Block
    add_box((0.0, -0.555, 0.044), (0.010, 0.032, 0.052), rot_x_deg=-12.0, mat_idx=0)
    add_box((0.0, -0.12, 0.060), (0.022, 0.042, 0.016), mat_idx=0)
    # 9. Curved 30-Round "Banana" Magazine
    add_box((0.0, -0.075, -0.052), (0.024, 0.058, 0.085), rot_x_deg=-10.0, mat_idx=2)
    add_box((0.0, -0.092, -0.125), (0.024, 0.058, 0.085), rot_x_deg=-24.0, mat_idx=2)
    add_box((0.0, -0.122, -0.188), (0.024, 0.058, 0.075), rot_x_deg=-38.0, mat_idx=2)

    mesh_data = bpy.data.meshes.new("SM_AK47_Mesh")
    bm.to_mesh(mesh_data)
    bm.free()
    ak_obj = bpy.data.objects.new("SM_AK47", mesh_data)
    bpy.context.scene.collection.objects.link(ak_obj)
    
    def make_mat(name, color):
        m = bpy.data.materials.new(name)
        m.use_nodes = True
        m.node_tree.nodes["Principled BSDF"].inputs['Base Color'].default_value = color
        m.diffuse_color = color
        return m

    ak_obj.data.materials.append(make_mat("M_AK47_Steel", (0.12, 0.13, 0.15, 1.0)))
    ak_obj.data.materials.append(make_mat("M_AK47_Wood", (0.42, 0.22, 0.09, 1.0)))
    ak_obj.data.materials.append(make_mat("M_AK47_Bakelite", (0.52, 0.18, 0.06, 1.0)))
    return ak_obj

ak_obj = create_ak47_mesh()

out_dir = os.path.abspath('Content/Characters/BaseMale')
os.makedirs(out_dir, exist_ok=True)
ak_fbx_path = os.path.join(out_dir, 'SM_AK47.fbx')

# Export SM_AK47 at origin (0,0,0) with rotation (0,0,0)
ak_obj.location = (0.0, 0.0, 0.0)
ak_obj.rotation_euler = (0.0, 0.0, 0.0)
bpy.ops.object.select_all(action='DESELECT')
ak_obj.select_set(True)
bpy.context.view_layer.objects.active = ak_obj
bpy.ops.export_scene.fbx(
    filepath=ak_fbx_path,
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_ALL',
    axis_forward='-Z',
    axis_up='Y',
    object_types={'MESH'},
    use_mesh_modifiers=True,
    mesh_smooth_type='FACE',
    bake_anim=False
)
print(f"Exported AK-47 Static Mesh: {ak_fbx_path} ({os.path.getsize(ak_fbx_path):,} bytes)")

# 5. Set up IK Contact Targets on AK-47 and Solve Default Aimed Stance
emp_r = bpy.data.objects.new("IK_Target_RightWrist", None)
bpy.context.scene.collection.objects.link(emp_r)
emp_r.parent = ak_obj
emp_r.location = (-0.030, 0.118, -0.076)

emp_l = bpy.data.objects.new("IK_Target_LeftWrist", None)
bpy.context.scene.collection.objects.link(emp_l)
emp_l.parent = ak_obj
emp_l.location = (0.026, -0.175, -0.026)

pole_r = bpy.data.objects.new("IK_Pole_RightElbow", None)
bpy.context.scene.collection.objects.link(pole_r)
pole_r.location = (-0.32, 0.05, 1.10)

pole_l = bpy.data.objects.new("IK_Pole_LeftElbow", None)
bpy.context.scene.collection.objects.link(pole_l)
pole_l.location = (0.18, -0.18, 1.02)

bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='POSE')
pb = arm_obj.pose.bones
for b in pb:
    b.rotation_mode = 'XYZ'

ik_r = pb['lowerarm_r'].constraints.new('IK')
ik_r.target = emp_r
ik_r.pole_target = pole_r
ik_r.pole_angle = math.radians(-90.0)
ik_r.chain_count = 2

ik_l = pb['lowerarm_l'].constraints.new('IK')
ik_l.target = emp_l
ik_l.pole_target = pole_l
ik_l.pole_angle = math.radians(90.0)
ik_l.chain_count = 2

def rotate_bone_world(pbone, euler_world):
    R_world = euler_world.to_matrix().to_4x4()
    loc = pbone.matrix.to_translation()
    rot = pbone.matrix.to_3x3().to_4x4()
    pbone.matrix = Matrix.Translation(loc) @ R_world @ rot
    bpy.context.view_layer.update()

def set_bone_world_axes(pbone, y_dir_world, z_dir_world):
    y_w = y_dir_world.normalized()
    x_w = y_w.cross(z_dir_world.normalized()).normalized()
    z_w = x_w.cross(y_w).normalized()
    rot_3x3 = Matrix((
        (x_w.x, y_w.x, z_w.x),
        (x_w.y, y_w.y, z_w.y),
        (x_w.z, y_w.z, z_w.z),
    ))
    loc = pbone.matrix.to_translation()
    pbone.matrix = Matrix.Translation(loc) @ rot_3x3.to_4x4()
    bpy.context.view_layer.update()

R_ak_ads_inv = Euler((0.0, 0.0, math.radians(-5.0)), 'XYZ').to_matrix().inverted()
y_r_local = R_ak_ads_inv @ Vector((0.28, -0.95, 0.12)).normalized()
z_r_local = R_ak_ads_inv @ Vector((-0.55, -0.05, 0.83)).normalized()

y_l_world_ads = Vector((-0.62, -0.77, 0.14)).normalized()
x_l_palm_ads = Vector((-0.35, 0.05, 0.93)).normalized()
z_l_world_ads = x_l_palm_ads.cross(y_l_world_ads).normalized()
y_l_local = R_ak_ads_inv @ y_l_world_ads
z_l_local = R_ak_ads_inv @ z_l_world_ads

# Position AK-47 in Aimed ADS Stance
ak_obj.location = (-0.085, -0.34, 1.465)
ak_obj.rotation_euler = (0.0, 0.0, math.radians(-5.0))
bpy.context.view_layer.update()

# Pose body for Aimed ADS
pelvis_rest_loc = pb['pelvis'].matrix.to_translation()
pb['pelvis'].matrix.translation = pelvis_rest_loc + Vector((0.0, 0.0, -0.02))
bpy.context.view_layer.update()
rotate_bone_world(pb['pelvis'], Euler((math.radians(-2.0), 0.0, math.radians(-10.0)), 'XYZ'))

rotate_bone_world(pb['spine_01'], Euler((math.radians(5.0), 0.0, math.radians(3.0)), 'XYZ'))
rotate_bone_world(pb['spine_02'], Euler((math.radians(5.0), 0.0, math.radians(3.0)), 'XYZ'))
rotate_bone_world(pb['spine_03'], Euler((math.radians(4.0), 0.0, math.radians(4.0)), 'XYZ'))

rotate_bone_world(pb['clavicle_r'], Euler((0.0, 0.0, math.radians(8.0)), 'XYZ'))
rotate_bone_world(pb['clavicle_l'], Euler((0.0, math.radians(-5.0), math.radians(-22.0)), 'XYZ'))

rotate_bone_world(pb['neck_01'], Euler((math.radians(4.0), math.radians(4.0), math.radians(-2.0)), 'XYZ'))
rotate_bone_world(pb['head'], Euler((math.radians(6.0), math.radians(5.0), math.radians(-2.0)), 'XYZ'))

rotate_bone_world(pb['thigh_l'], Euler((math.radians(-12.0), 0.0, math.radians(4.0)), 'XYZ'))
rotate_bone_world(pb['calf_l'], Euler((math.radians(18.0), 0.0, 0.0), 'XYZ'))
rotate_bone_world(pb['foot_l'], Euler((math.radians(-6.0), 0.0, 0.0), 'XYZ'))

rotate_bone_world(pb['thigh_r'], Euler((math.radians(10.0), 0.0, math.radians(-6.0)), 'XYZ'))
rotate_bone_world(pb['calf_r'], Euler((math.radians(16.0), 0.0, 0.0), 'XYZ'))
rotate_bone_world(pb['foot_r'], Euler((math.radians(-18.0), 0.0, math.radians(-6.0)), 'XYZ'))

R_ak = ak_obj.matrix_world.to_3x3()
set_bone_world_axes(pb['hand_r'], R_ak @ y_r_local, R_ak @ z_r_local)
set_bone_world_axes(pb['hand_l'], R_ak @ y_l_local, R_ak @ z_l_local)

for f_prefix in ['index', 'middle', 'ring', 'pinky']:
    for i in range(1, 4):
        b_r = pb.get(f'{f_prefix}_0{i}_r')
        if b_r:
            curl = (10.0 if i == 1 else (55.0 if i == 2 else 45.0)) if f_prefix == 'index' else (45.0 if i == 1 else (80.0 if i == 2 else 65.0))
            b_r.rotation_euler = (math.radians(curl), 0.0, 0.0)
        b_l = pb.get(f'{f_prefix}_0{i}_l')
        if b_l:
            curl_l = 55.0 if i == 1 else (80.0 if i == 2 else 65.0)
            b_l.rotation_euler = (math.radians(curl_l), 0.0, 0.0)

for side in ['r', 'l']:
    for i in range(1, 4):
        tb = pb.get(f'thumb_0{i}_{side}')
        if tb:
            tb.rotation_euler = (math.radians(35.0), 0.0, math.radians(-20.0 if side=='r' else 20.0))

pb['weapon_r'].matrix = ak_obj.matrix_world.copy()
bpy.context.view_layer.update()

# Bake IK-solved matrices directly onto FK bone transforms and remove IK constraints!
solved_mats = {b.name: b.matrix.copy() for b in pb}
pb['lowerarm_r'].constraints.remove(ik_r)
pb['lowerarm_l'].constraints.remove(ik_l)
bpy.context.view_layer.update()

for bone in arm_obj.data.bones:
    pbone = pb[bone.name]
    pbone.matrix = solved_mats[bone.name]
    bpy.context.view_layer.update()

max_err = max((pb[b.name].matrix.to_translation() - solved_mats[b.name].to_translation()).length for b in pb)
print(f"Max IK->FK bake error across all bones: {max_err:.8f}m")

ads_action = bpy.data.actions.new(name="Anim_Operator_ADS")
arm_obj.animation_data_create()
arm_obj.animation_data.action = ads_action
for pbone in pb:
    pbone.keyframe_insert(data_path="rotation_euler", frame=1)
    pbone.keyframe_insert(data_path="location", frame=1)
bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 1

bpy.ops.object.mode_set(mode='OBJECT')

# Clear residual vertex color layers
while mesh_obj.data.color_attributes:
    mesh_obj.data.color_attributes.remove(mesh_obj.data.color_attributes[0])

# 6. Clean Material Setup
mat = bpy.data.materials.new(name="M_Operator_BaseBody")
mat.use_nodes = True
bsdf = mat.node_tree.nodes.get("Principled BSDF")
if bsdf:
    bsdf.inputs['Base Color'].default_value = (0.24, 0.26, 0.22, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.60
mat.diffuse_color = (0.45, 0.50, 0.42, 1.0)

if mesh_obj.data.materials:
    mesh_obj.data.materials[0] = mat
else:
    mesh_obj.data.materials.append(mat)

# 7. Export SK_BaseMale.fbx
out_fbx = os.path.join(out_dir, 'SK_BaseMale.fbx')

bpy.ops.object.select_all(action='DESELECT')
mesh_obj.select_set(True)
arm_obj.select_set(True)
bpy.context.view_layer.objects.active = arm_obj

bpy.ops.export_scene.fbx(
    filepath=out_fbx,
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
    bake_anim=True,
    bake_anim_use_all_actions=True,
    bake_anim_step=1.0,
    bake_anim_simplify_factor=0.0
)

print(f"Successfully exported: {out_fbx} (Size: {os.path.getsize(out_fbx):,} bytes)")

# 8. Render Preview_BaseMale.png (with baked FK pose + AK-47)
bpy.context.scene.render.engine = 'BLENDER_WORKBENCH'
bpy.context.scene.display.shading.light = 'STUDIO'
bpy.context.scene.display.shading.color_type = 'MATERIAL'
bpy.context.scene.display.shading.show_cavity = True

cam_data = bpy.data.cameras.new("PreviewCam")
cam_data.lens = 52
cam_obj = bpy.data.objects.new("PreviewCam", cam_data)
bpy.context.scene.collection.objects.link(cam_obj)
bpy.context.scene.camera = cam_obj
cam_obj.location = (-1.65, -1.75, 1.15)
direction = Vector((0.0, -0.15, 0.98)) - cam_obj.location
cam_obj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
bpy.context.scene.render.resolution_x = 900
bpy.context.scene.render.resolution_y = 900
preview_path = os.path.join(out_dir, 'Preview_BaseMale.png')
bpy.context.scene.render.filepath = preview_path
bpy.ops.render.render(write_still=True)
print(f"Rendered updated Preview_BaseMale.png: {preview_path}")
print("=== PIPELINE V3 COMPLETE ===")
