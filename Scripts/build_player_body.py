import bpy
import os
import math

print("=== STARTING BUILD_PLAYER_BODY PIPELINE (V2 with Tactical ADS Pose) ===")

blend_path = os.path.abspath('Scripts/BaseMesh/Male_Human.blend')
bpy.ops.wm.read_factory_settings(use_empty=True)

with bpy.data.libraries.load(blend_path) as (data_from, data_to):
    data_to.objects = ['T_Male_04_Deform', 'T_Metarig_Finger']

mesh_obj = [o for o in data_to.objects if o.type == 'MESH'][0]
arm_obj = [o for o in data_to.objects if o.type == 'ARMATURE'][0]

bpy.context.scene.collection.objects.link(mesh_obj)
bpy.context.scene.collection.objects.link(arm_obj)

# 1. Clean Vertex Group Merging
def merge_vg_clean(source_name, target_name):
    src_vg = mesh_obj.vertex_groups.get(source_name)
    tgt_vg = mesh_obj.vertex_groups.get(target_name)
    if not src_vg or not tgt_vg:
        return
    src_idx = src_vg.index
    tgt_idx = tgt_vg.index
    for v in mesh_obj.data.vertices:
        for g in v.groups:
            if g.group == src_idx:
                tgt_vg.add([v.index], g.weight, 'ADD')
                break
    mesh_obj.vertex_groups.remove(src_vg)

# Merge secondary groups
merge_vg_clean('spine.006', 'spine.005') # top of head into head
merge_vg_clean('pelvis.L', 'spine')
merge_vg_clean('pelvis.R', 'spine')
merge_vg_clean('heel.02.L', 'foot.L')
merge_vg_clean('heel.02.R', 'foot.R')

# Merge palms into hands
for side in ['L', 'R']:
    hand = f'hand.{side}'
    for i in range(1, 5):
        palm = f'palm.0{i}.{side}'
        merge_vg_clean(palm, hand)

# Rename mapping
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

# Add fingers
for side, s_code in [('L', 'l'), ('R', 'r')]:
    for i in range(1, 4):
        rename_map[f'f_index.0{i}.{side}'] = f'index_0{i}_{s_code}'
        rename_map[f'thumb.0{i}.{side}'] = f'thumb_0{i}_{s_code}'
        rename_map[f'f_middle.0{i}.{side}'] = f'middle_0{i}_{s_code}'
        rename_map[f'f_ring.0{i}.{side}'] = f'ring_0{i}_{s_code}'
        rename_map[f'f_pinky.0{i}.{side}'] = f'pinky_0{i}_{s_code}'

# Apply renaming to vertex groups
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

pelvis_bone = ebones.get('spine')
if pelvis_bone:
    pelvis_bone.parent = root_bone

for old_name, new_name in rename_map.items():
    b = ebones.get(old_name)
    if b:
        b.name = new_name

bpy.ops.object.mode_set(mode='OBJECT')

# 3. Scaling to exact 180cm standing height
# Calibrate so top vertex of head is at Z = 1.80m and lowest vertex of feet is at Z = 0.00m
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
    arm_obj.location.z -= min_z_after
    mesh_obj.location.z -= min_z_after
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

final_min_z = min(v.co.z for v in mesh_obj.data.vertices)
final_max_z = max(v.co.z for v in mesh_obj.data.vertices)
print(f"Final Calibrated Z Range: [{final_min_z:.4f}m, {final_max_z:.4f}m] (Height = {final_max_z - final_min_z:.4f}m)")

# 4. Author ADS Aiming Stance Pose
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='POSE')
pbones = arm_obj.pose.bones

# Create ADS Aiming Action
ads_action = bpy.data.actions.new(name="Anim_Operator_ADS")
arm_obj.animation_data_create()
arm_obj.animation_data.action = ads_action

# Tactical stance rotations (radians)
# Bladed stance: pelvis turned slightly right
if pbones.get('pelvis'):
    pbones['pelvis'].rotation_mode = 'XYZ'
    pbones['pelvis'].rotation_euler = (math.radians(-4.0), math.radians(2.0), math.radians(12.0))

# Upper body leans forward into recoil
if pbones.get('spine_01'):
    pbones['spine_01'].rotation_mode = 'XYZ'
    pbones['spine_01'].rotation_euler = (math.radians(6.0), 0.0, math.radians(-4.0))

if pbones.get('spine_02'):
    pbones['spine_02'].rotation_mode = 'XYZ'
    pbones['spine_02'].rotation_euler = (math.radians(5.0), 0.0, math.radians(-3.0))

if pbones.get('spine_03'):
    pbones['spine_03'].rotation_mode = 'XYZ'
    pbones['spine_03'].rotation_euler = (math.radians(4.0), 0.0, math.radians(-2.0))

# Right arm: trigger hand in shooting pocket, elbow tucked
if pbones.get('clavicle_r'):
    pbones['clavicle_r'].rotation_mode = 'XYZ'
    pbones['clavicle_r'].rotation_euler = (math.radians(5.0), math.radians(5.0), math.radians(-10.0))

if pbones.get('upperarm_r'):
    pbones['upperarm_r'].rotation_mode = 'XYZ'
    pbones['upperarm_r'].rotation_euler = (math.radians(-65.0), math.radians(20.0), math.radians(-25.0))

if pbones.get('lowerarm_r'):
    pbones['lowerarm_r'].rotation_mode = 'XYZ'
    pbones['lowerarm_r'].rotation_euler = (math.radians(85.0), math.radians(-35.0), math.radians(15.0))

if pbones.get('hand_r'):
    pbones['hand_r'].rotation_mode = 'XYZ'
    pbones['hand_r'].rotation_euler = (math.radians(10.0), math.radians(5.0), math.radians(-15.0))

# Left arm: support hand on rifle handguard
if pbones.get('clavicle_l'):
    pbones['clavicle_l'].rotation_mode = 'XYZ'
    pbones['clavicle_l'].rotation_euler = (math.radians(-5.0), math.radians(-8.0), math.radians(12.0))

if pbones.get('upperarm_l'):
    pbones['upperarm_l'].rotation_mode = 'XYZ'
    pbones['upperarm_l'].rotation_euler = (math.radians(-50.0), math.radians(-35.0), math.radians(35.0))

if pbones.get('lowerarm_l'):
    pbones['lowerarm_l'].rotation_mode = 'XYZ'
    pbones['lowerarm_l'].rotation_euler = (math.radians(70.0), math.radians(45.0), math.radians(-20.0))

if pbones.get('hand_l'):
    pbones['hand_l'].rotation_mode = 'XYZ'
    pbones['hand_l'].rotation_euler = (math.radians(-15.0), math.radians(10.0), math.radians(20.0))

# Head: looking straight down sightline
if pbones.get('neck_01'):
    pbones['neck_01'].rotation_mode = 'XYZ'
    pbones['neck_01'].rotation_euler = (math.radians(-5.0), math.radians(-2.0), math.radians(-4.0))

if pbones.get('head'):
    pbones['head'].rotation_mode = 'XYZ'
    pbones['head'].rotation_euler = (math.radians(-3.0), math.radians(2.0), math.radians(-3.0))

# Legs: athletic ready stance, knees slightly flexed
if pbones.get('thigh_l'):
    pbones['thigh_l'].rotation_mode = 'XYZ'
    pbones['thigh_l'].rotation_euler = (math.radians(-8.0), math.radians(-5.0), math.radians(-6.0))

if pbones.get('calf_l'):
    pbones['calf_l'].rotation_mode = 'XYZ'
    pbones['calf_l'].rotation_euler = (math.radians(14.0), 0.0, 0.0)

if pbones.get('foot_l'):
    pbones['foot_l'].rotation_mode = 'XYZ'
    pbones['foot_l'].rotation_euler = (math.radians(-6.0), math.radians(5.0), 0.0)

if pbones.get('thigh_r'):
    pbones['thigh_r'].rotation_mode = 'XYZ'
    pbones['thigh_r'].rotation_euler = (math.radians(10.0), math.radians(6.0), math.radians(8.0))

if pbones.get('calf_r'):
    pbones['calf_r'].rotation_mode = 'XYZ'
    pbones['calf_r'].rotation_euler = (math.radians(16.0), 0.0, 0.0)

if pbones.get('foot_r'):
    pbones['foot_r'].rotation_mode = 'XYZ'
    pbones['foot_r'].rotation_euler = (math.radians(-10.0), math.radians(-6.0), 0.0)

# Keyframe at frame 1 and 60 with subtle breathing curve
for pbone in pbones:
    pbone.keyframe_insert(data_path="rotation_euler", frame=1)
    # Subtle breathing on spine_03 at frame 30
    if pbone.name == 'spine_03':
        orig_rot = pbone.rotation_euler.copy()
        pbone.rotation_euler = (orig_rot.x + math.radians(1.2), orig_rot.y, orig_rot.z)
        pbone.keyframe_insert(data_path="rotation_euler", frame=30)
        pbone.rotation_euler = orig_rot
    pbone.keyframe_insert(data_path="rotation_euler", frame=60)

bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 60

bpy.ops.object.mode_set(mode='OBJECT')

# 5. Clean Material Setup
mat = bpy.data.materials.new(name="M_Operator_BaseBody")
mat.use_nodes = True
bsdf = mat.node_tree.nodes.get("Principled BSDF")
if bsdf:
    # Olive drab tactical under-suit / base athletic operator body
    bsdf.inputs['Base Color'].default_value = (0.24, 0.26, 0.22, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.60

if mesh_obj.data.materials:
    mesh_obj.data.materials[0] = mat
else:
    mesh_obj.data.materials.append(mat)

# 6. Export to FBX
out_dir = os.path.abspath('Content/Characters/BaseMale')
os.makedirs(out_dir, exist_ok=True)
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
print("=== PIPELINE V2 COMPLETE ===")
