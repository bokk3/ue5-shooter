import bpy
import bmesh
import math
import os
import shutil
from mathutils import Vector, Matrix, Euler

print(">>> GENERATING TACTICAL RELOAD ANIMATION FOR NOBLE FERMI <<<")

blend_path = os.path.abspath('Scripts/BaseMesh/Male_Human.blend')
out_dir = os.path.abspath("Content/Characters/BaseMale/Animations")
os.makedirs(out_dir, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)

with bpy.data.libraries.load(blend_path) as (data_from, data_to):
    data_to.objects = ['T_Male_04_Deform', 'T_Metarig_Finger']

mesh_obj = [o for o in data_to.objects if o.type == 'MESH'][0]
arm_obj = [o for o in data_to.objects if o.type == 'ARMATURE'][0]

bpy.context.scene.collection.objects.link(mesh_obj)
bpy.context.scene.collection.objects.link(arm_obj)

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
merge_vg_clean('f_pinky.03.L', 'f_pinky.02.L')
merge_vg_clean('f_pinky.03.R', 'f_pinky.02.R')

for side in ['L', 'R']:
    hand = f'hand.{side}'
    for i in range(1, 5):
        merge_vg_clean(f'palm.0{i}.{side}', hand)

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
    for i in range(1, 3):
        rename_map[f'f_pinky.0{i}.{side}'] = f'pinky_0{i}_{s_code}'

for old_name, new_name in rename_map.items():
    vg = mesh_obj.vertex_groups.get(old_name)
    if vg:
        vg.name = new_name

bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='EDIT')
ebones = arm_obj.data.edit_bones
for b_del in ['spine.006', 'pelvis.L', 'pelvis.R', 'heel.02.L', 'heel.02.R', 'f_pinky.03.L', 'f_pinky.03.R']:
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

# Scale to 1.80m
min_z = min(v.co.z for v in mesh_obj.data.vertices)
max_z = max(v.co.z for v in mesh_obj.data.vertices)
scale_to_180 = 1.80 / (max_z - min_z)
arm_obj.scale = (scale_to_180, scale_to_180, scale_to_180)
mesh_obj.scale = (scale_to_180, scale_to_180, scale_to_180)
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.select_all(action='DESELECT')
arm_obj.select_set(True)
mesh_obj.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

min_z_after = min(v.co.z for v in mesh_obj.data.vertices)
if abs(min_z_after) > 0.001:
    for v in mesh_obj.data.vertices:
        v.co.z -= min_z_after
    bpy.context.view_layer.objects.active = arm_obj
    bpy.ops.object.mode_set(mode='EDIT')
    for eb in arm_obj.data.edit_bones:
        if eb.name != 'root':
            eb.head.z -= min_z_after
            eb.tail.z -= min_z_after
    bpy.ops.object.mode_set(mode='OBJECT')

# Add weapon_r bone
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='EDIT')
weapon_bone = arm_obj.data.edit_bones.new('weapon_r')
weapon_bone.head = (0.0, 0.0, 0.0)
weapon_bone.tail = (0.0, 0.10, 0.0)
weapon_bone.roll = 0.0
weapon_bone.parent = arm_obj.data.edit_bones['hand_r']
weapon_bone.use_connect = False
bpy.ops.object.mode_set(mode='OBJECT')

# Weapon & IK Empties
ak_obj = bpy.data.objects.new("AK47_Proxy", None)
bpy.context.scene.collection.objects.link(ak_obj)

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

rest_pose_mats = {b.name: b.bone.matrix_local.copy() for b in pb}

R_ak_ads_inv = Euler((0.0, 0.0, math.radians(-5.0)), 'XYZ').to_matrix().inverted()
y_r_local = R_ak_ads_inv @ Vector((0.28, -0.95, 0.12)).normalized()
z_r_local = R_ak_ads_inv @ Vector((-0.55, -0.05, 0.83)).normalized()

y_l_world_ads = Vector((-0.62, -0.77, 0.14)).normalized()
x_l_palm_ads = Vector((-0.35, 0.05, 0.93)).normalized()
z_l_world_ads = x_l_palm_ads.cross(y_l_world_ads).normalized()
y_l_local = R_ak_ads_inv @ y_l_world_ads
z_l_local = R_ak_ads_inv @ z_l_world_ads

def reset_pose():
    for b in pb:
        b.location = (0, 0, 0)
        b.rotation_euler = (0, 0, 0)
        b.rotation_quaternion = (1, 0, 0, 0)
        b.scale = (1, 1, 1)
    bpy.context.view_layer.update()

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

def bake_reload_animation():
    num_frames = 60 # 2.0 seconds @ 30fps
    print(f"--- Baking & Exporting Reload Animation ({num_frames} frames) ---")
    bpy.context.view_layer.objects.active = arm_obj
    bpy.ops.object.mode_set(mode='POSE')

    if arm_obj.animation_data:
        arm_obj.animation_data_clear()
    for act in list(bpy.data.actions):
        bpy.data.actions.remove(act)

    action = bpy.data.actions.new(name="Anim_BaseMale_Reload")
    arm_obj.animation_data_create()
    arm_obj.animation_data.action = action

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

    pelvis_rest_loc = rest_pose_mats['pelvis'].to_translation()

    foregrip_local = Vector((0.026, -0.175, -0.026))
    magwell_local = Vector((0.015, -0.06, -0.08))
    mag_drop_local = Vector((0.015, -0.06, -0.25))
    chest_rig_local = Vector((-0.08, 0.08, -0.30))

    for f in range(1, num_frames + 1):
        bpy.context.scene.frame_set(f)
        ik_r.mute = False
        ik_l.mute = False
        reset_pose()

        t = (f - 1) / float(num_frames - 1) # 0.0 -> 1.0

        # Spine & stance posture: solid tactical stance, slightly leaned forward
        pb['pelvis'].matrix.translation = pelvis_rest_loc + Vector((-0.01, 0.0, -0.015))
        bpy.context.view_layer.update()

        rotate_bone_world(pb['pelvis'], Euler((0.0, 0.0, math.radians(-12.0)), 'XYZ'))
        rotate_bone_world(pb['thigh_l'], Euler((math.radians(10.0), 0.0, math.radians(14.0)), 'XYZ'))
        rotate_bone_world(pb['calf_l'], Euler((math.radians(12.0), 0.0, 0.0), 'XYZ'))
        rotate_bone_world(pb['thigh_r'], Euler((math.radians(-14.0), 0.0, math.radians(-12.0)), 'XYZ'))
        rotate_bone_world(pb['calf_r'], Euler((math.radians(14.0), 0.0, 0.0), 'XYZ'))

        # Spine orientation: subtle breathe & lean
        rotate_bone_world(pb['spine_01'], Euler((math.radians(4.0), 0.0, math.radians(4.0)), 'XYZ'))
        rotate_bone_world(pb['spine_02'], Euler((math.radians(4.0), 0.0, math.radians(4.0)), 'XYZ'))
        rotate_bone_world(pb['spine_03'], Euler((math.radians(4.0), 0.0, math.radians(4.0)), 'XYZ'))

        # Weapon cant: 22 deg roll during reload
        cant_weight = 0.0
        if t < 0.20:
            cant_weight = math.sin((t / 0.20) * (math.pi / 2.0))
        elif t <= 0.80:
            cant_weight = 1.0
        else:
            cant_weight = math.cos(((t - 0.80) / 0.20) * (math.pi / 2.0))

        ak_x = -0.080 - 0.035 * cant_weight
        ak_y = -0.34 + 0.02 * cant_weight
        ak_z = 1.455 - 0.03 * cant_weight
        ak_pitch = 2.0 + 4.0 * cant_weight
        ak_roll = -22.0 * cant_weight
        ak_yaw = -5.0 - 4.0 * cant_weight

        ak_obj.location = (ak_x, ak_y, ak_z)
        ak_obj.rotation_euler = (math.radians(ak_pitch), math.radians(ak_roll), math.radians(ak_yaw))
        bpy.context.view_layer.update()

        # Head tracks weapon reload action subtly
        head_pitch = 12.0 * cant_weight
        head_yaw = -6.0 * cant_weight
        rotate_bone_world(pb['neck_01'], Euler((math.radians(head_pitch * 0.4), 0.0, math.radians(head_yaw * 0.4)), 'XYZ'))
        rotate_bone_world(pb['head'], Euler((math.radians(head_pitch * 0.6), 0.0, math.radians(head_yaw * 0.6)), 'XYZ'))

        # Left hand IK Target Trajectory (Reload Motion Phases):
        if t < 0.20:
            alpha = t / 0.20
            emp_l.location = foregrip_local.lerp(magwell_local, math.sin(alpha * math.pi / 2.0))
        elif t < 0.40:
            alpha = (t - 0.20) / 0.20
            emp_l.location = magwell_local.lerp(mag_drop_local, math.sin(alpha * math.pi / 2.0))
        elif t < 0.56:
            alpha = (t - 0.40) / 0.16
            emp_l.location = mag_drop_local.lerp(chest_rig_local, math.sin(alpha * math.pi / 2.0))
        elif t < 0.72:
            alpha = (t - 0.56) / 0.16
            emp_l.location = chest_rig_local.lerp(magwell_local, math.sin(alpha * math.pi / 2.0))
        elif t < 0.82:
            slap_t = (t - 0.72) / 0.10
            bump_z = math.sin(slap_t * math.pi) * 0.02
            emp_l.location = magwell_local + Vector((0.0, 0.0, bump_z))
        else:
            alpha = (t - 0.82) / 0.18
            emp_l.location = magwell_local.lerp(foregrip_local, math.sin(alpha * math.pi / 2.0))

        bpy.context.view_layer.update()

        # Apply hand rotation and finger posing
        R_ak = ak_obj.matrix_world.to_3x3()
        set_bone_world_axes(pb['hand_r'], R_ak @ y_r_local, R_ak @ z_r_local)

        if t < 0.20 or t > 0.85:
            set_bone_world_axes(pb['hand_l'], R_ak @ y_l_local, R_ak @ z_l_local)
        elif t >= 0.72 and t <= 0.82:
            slap_y = Vector((0.0, 0.2, 0.98)).normalized()
            slap_z = Vector((0.0, -0.98, 0.2)).normalized()
            set_bone_world_axes(pb['hand_l'], R_ak @ slap_y, R_ak @ slap_z)
        else:
            mag_y = Vector((0.0, -0.8, -0.6)).normalized()
            mag_z = Vector((-0.8, 0.0, 0.6)).normalized()
            set_bone_world_axes(pb['hand_l'], R_ak @ mag_y, R_ak @ mag_z)

        # Finger curl
        for f_prefix in ['index', 'middle', 'ring', 'pinky']:
            for i in range(1, 4):
                b_r = pb.get(f'{f_prefix}_0{i}_r')
                if b_r:
                    curl = 45.0 if f_prefix == 'index' else 65.0
                    b_r.rotation_euler = (math.radians(curl), 0.0, 0.0)

                b_l = pb.get(f'{f_prefix}_0{i}_l')
                if b_l:
                    if t >= 0.72 and t <= 0.82:
                        curl_l = 10.0 # open palm slap
                    elif t < 0.20 or t > 0.85:
                        curl_l = 65.0 # foregrip wrap
                    else:
                        curl_l = 48.0 # mag grasp
                    b_l.rotation_euler = (math.radians(curl_l), 0.0, 0.0)

        pb['weapon_r'].matrix = ak_obj.matrix_world.copy()
        bpy.context.view_layer.update()

        solved_mats = {b.name: b.matrix.copy() for b in pb}

        ik_r.mute = True
        ik_l.mute = True
        bpy.context.view_layer.update()

        for bone in arm_obj.data.bones:
            pbone = pb[bone.name]
            pbone.matrix = solved_mats[bone.name]
            bpy.context.view_layer.update()
            pbone.keyframe_insert(data_path="rotation_euler", frame=f)
            if bone.name in ('pelvis', 'weapon_r', 'root'):
                pbone.keyframe_insert(data_path="location", frame=f)

    pb['lowerarm_r'].constraints.remove(ik_r)
    pb['lowerarm_l'].constraints.remove(ik_l)
    bpy.context.scene.frame_start = 1
    bpy.context.scene.frame_end = num_frames

    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    mesh_obj.select_set(True)
    arm_obj.select_set(True)
    bpy.context.view_layer.objects.active = arm_obj

    fbx_primary = os.path.join(out_dir, "Anim_BaseMale_Reload.fbx")
    fbx_alias = os.path.join(out_dir, "Anim_Operator_Reload.fbx")

    bpy.ops.export_scene.fbx(
        filepath=fbx_primary,
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
        bake_anim_use_all_actions=False,
        bake_anim_use_nla_strips=False,
        bake_anim_step=1.0,
        bake_anim_simplify_factor=0.0
    )
    shutil.copyfile(fbx_primary, fbx_alias)
    print(f"Exported: {fbx_primary}")
    print(f"Exported: {fbx_alias}")

bake_reload_animation()
print(">>> RELOAD ANIMATION EXPORT COMPLETE! <<<")
