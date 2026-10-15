import bpy
import math
import os
import sys

print(">>> GENERATING 4 TACTICAL LOCOMOTION ANIMATIONS FROM SK_BASEMALE FBX <<<")

fbx_source = os.path.abspath("Content/Characters/BaseMale/SK_BaseMale.fbx")
out_dir = os.path.abspath("Content/Characters/BaseMale/Animations")
os.makedirs(out_dir, exist_ok=True)

def load_base_fbx():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=fbx_source)
    arm = [o for o in bpy.data.objects if o.type == 'ARMATURE'][0]
    mesh = [o for o in bpy.data.objects if o.type == 'MESH'][0]
    
    # Ensure XYZ Euler rotation mode on all pose bones
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode='POSE')
    for pb in arm.pose.bones:
        pb.rotation_mode = 'XYZ'
        pb.location = (0, 0, 0)
        pb.rotation_euler = (0, 0, 0)
        pb.scale = (1, 1, 1)
    bpy.ops.object.mode_set(mode='OBJECT')
    return arm, mesh

def export_fbx(arm, mesh, fbx_filename):
    out_fbx = os.path.join(out_dir, fbx_filename)
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    mesh.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    
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
    print(f"Exported: {out_fbx} ({os.path.getsize(out_fbx):,} bytes)")

def set_upper_body_ready(pb, forward_lean=6.0):
    pb['spine_01'].rotation_euler = (math.radians(forward_lean), 0.0, math.radians(-2.5))
    pb['spine_02'].rotation_euler = (math.radians(forward_lean * 0.8), 0.0, math.radians(-1.5))
    pb['spine_03'].rotation_euler = (math.radians(forward_lean * 0.6), 0.0, math.radians(-1.0))
    pb['clavicle_r'].rotation_euler = (math.radians(5.0), math.radians(5.0), math.radians(-10.0))
    pb['upperarm_r'].rotation_euler = (math.radians(-65.0), math.radians(20.0), math.radians(-25.0))
    pb['lowerarm_r'].rotation_euler = (math.radians(85.0), math.radians(-35.0), math.radians(15.0))
    pb['hand_r'].rotation_euler = (math.radians(10.0), math.radians(5.0), math.radians(-15.0))
    pb['clavicle_l'].rotation_euler = (math.radians(-5.0), math.radians(-8.0), math.radians(12.0))
    pb['upperarm_l'].rotation_euler = (math.radians(-50.0), math.radians(-35.0), math.radians(35.0))
    pb['lowerarm_l'].rotation_euler = (math.radians(70.0), math.radians(45.0), math.radians(-20.0))
    pb['hand_l'].rotation_euler = (math.radians(-15.0), math.radians(10.0), math.radians(20.0))
    pb['neck_01'].rotation_euler = (math.radians(-4.0), 0.0, 0.0)
    pb['head'].rotation_euler = (math.radians(-2.0), 0.0, 0.0)


# =========================================================================
# 1. Anim_Operator_Idle_ADS (60 frames @ 30fps) - Breathing & Micro-sway
# =========================================================================
print("--- Generating Anim_Operator_Idle_ADS ---")
arm, mesh = load_base_fbx()
bpy.context.view_layer.objects.active = arm
bpy.ops.object.mode_set(mode='POSE')
pb = arm.pose.bones

action_idle = bpy.data.actions.new("Anim_Operator_Idle_ADS")
arm.animation_data_create()
arm.animation_data.action = action_idle

set_upper_body_ready(pb, forward_lean=6.0)
pb['thigh_l'].rotation_euler = (math.radians(-8.0), math.radians(-5.0), math.radians(-6.0))
pb['calf_l'].rotation_euler = (math.radians(14.0), 0.0, 0.0)
pb['foot_l'].rotation_euler = (math.radians(-6.0), math.radians(5.0), 0.0)
pb['thigh_r'].rotation_euler = (math.radians(10.0), math.radians(6.0), math.radians(8.0))
pb['calf_r'].rotation_euler = (math.radians(16.0), 0.0, 0.0)
pb['foot_r'].rotation_euler = (math.radians(-10.0), math.radians(-6.0), 0.0)

for b in pb:
    b.keyframe_insert(data_path="rotation_euler", frame=1)
    b.keyframe_insert(data_path="location", frame=1)

# Inhale at frame 30: thoracic expansion & clavicle elevation
orig_s3 = pb['spine_03'].rotation_euler.copy()
orig_cr = pb['clavicle_r'].rotation_euler.copy()
orig_cl = pb['clavicle_l'].rotation_euler.copy()

pb['spine_03'].rotation_euler = (orig_s3.x + math.radians(1.6), orig_s3.y, orig_s3.z)
pb['clavicle_r'].rotation_euler = (orig_cr.x, orig_cr.y + math.radians(1.0), orig_cr.z)
pb['clavicle_l'].rotation_euler = (orig_cl.x, orig_cl.y - math.radians(1.0), orig_cl.z)
pb['pelvis'].location = (0.002, 0.0, 0.003)

pb['spine_03'].keyframe_insert(data_path="rotation_euler", frame=30)
pb['clavicle_r'].keyframe_insert(data_path="rotation_euler", frame=30)
pb['clavicle_l'].keyframe_insert(data_path="rotation_euler", frame=30)
pb['pelvis'].keyframe_insert(data_path="location", frame=30)

# Return at frame 60
pb['spine_03'].rotation_euler = orig_s3
pb['clavicle_r'].rotation_euler = orig_cr
pb['clavicle_l'].rotation_euler = orig_cl
pb['pelvis'].location = (0, 0, 0)

for b in pb:
    b.keyframe_insert(data_path="rotation_euler", frame=60)
    b.keyframe_insert(data_path="location", frame=60)

bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 60
export_fbx(arm, mesh, "Anim_Operator_Idle_ADS.fbx")


# =========================================================================
# 2. Anim_Operator_Walk (30 frames @ 30fps) - Tactical Combat Walk
# =========================================================================
print("--- Generating Anim_Operator_Walk ---")
arm, mesh = load_base_fbx()
bpy.context.view_layer.objects.active = arm
bpy.ops.object.mode_set(mode='POSE')
pb = arm.pose.bones

action_walk = bpy.data.actions.new("Anim_Operator_Walk")
arm.animation_data_create()
arm.animation_data.action = action_walk

set_upper_body_ready(pb, forward_lean=7.0)

for b in pb:
    b.keyframe_insert(data_path="rotation_euler", frame=1)
    b.keyframe_insert(data_path="rotation_euler", frame=30)

walk_keyframes = {
    1: {  # Left Contact
        'pelvis_loc': (0.0, 0.0, -0.015),
        'pelvis_rot': (0.0, math.radians(2.0), math.radians(3.0)),
        'thigh_l': (math.radians(-22.0), 0.0, math.radians(-2.0)),
        'calf_l': (math.radians(8.0), 0.0, 0.0),
        'foot_l': (math.radians(-12.0), 0.0, 0.0),
        'thigh_r': (math.radians(18.0), 0.0, math.radians(2.0)),
        'calf_r': (math.radians(35.0), 0.0, 0.0),
        'foot_r': (math.radians(16.0), 0.0, 0.0),
    },
    8: {  # Left Passing
        'pelvis_loc': (-0.012, 0.0, 0.010),
        'pelvis_rot': (math.radians(1.0), 0.0, 0.0),
        'thigh_l': (math.radians(-4.0), 0.0, 0.0),
        'calf_l': (math.radians(6.0), 0.0, 0.0),
        'foot_l': (math.radians(0.0), 0.0, 0.0),
        'thigh_r': (math.radians(-2.0), 0.0, math.radians(3.0)),
        'calf_r': (math.radians(42.0), 0.0, 0.0),
        'foot_r': (math.radians(-8.0), 0.0, 0.0),
    },
    15: { # Right Contact
        'pelvis_loc': (0.0, 0.0, -0.015),
        'pelvis_rot': (0.0, math.radians(-2.0), math.radians(-3.0)),
        'thigh_l': (math.radians(18.0), 0.0, math.radians(-2.0)),
        'calf_l': (math.radians(35.0), 0.0, 0.0),
        'foot_l': (math.radians(16.0), 0.0, 0.0),
        'thigh_r': (math.radians(-22.0), 0.0, math.radians(2.0)),
        'calf_r': (math.radians(8.0), 0.0, 0.0),
        'foot_r': (math.radians(-12.0), 0.0, 0.0),
    },
    23: { # Right Passing
        'pelvis_loc': (0.012, 0.0, 0.010),
        'pelvis_rot': (math.radians(1.0), 0.0, 0.0),
        'thigh_l': (math.radians(-2.0), 0.0, math.radians(-3.0)),
        'calf_l': (math.radians(42.0), 0.0, 0.0),
        'foot_l': (math.radians(-8.0), 0.0, 0.0),
        'thigh_r': (math.radians(-4.0), 0.0, 0.0),
        'calf_r': (math.radians(6.0), 0.0, 0.0),
        'foot_r': (math.radians(0.0), 0.0, 0.0),
    },
    30: { # Loop End
        'pelvis_loc': (0.0, 0.0, -0.015),
        'pelvis_rot': (0.0, math.radians(2.0), math.radians(3.0)),
        'thigh_l': (math.radians(-22.0), 0.0, math.radians(-2.0)),
        'calf_l': (math.radians(8.0), 0.0, 0.0),
        'foot_l': (math.radians(-12.0), 0.0, 0.0),
        'thigh_r': (math.radians(18.0), 0.0, math.radians(2.0)),
        'calf_r': (math.radians(35.0), 0.0, 0.0),
        'foot_r': (math.radians(16.0), 0.0, 0.0),
    }
}

for f, data in walk_keyframes.items():
    pb['pelvis'].location = data['pelvis_loc']
    pb['pelvis'].rotation_euler = data['pelvis_rot']
    pb['thigh_l'].rotation_euler = data['thigh_l']
    pb['calf_l'].rotation_euler = data['calf_l']
    pb['foot_l'].rotation_euler = data['foot_l']
    pb['thigh_r'].rotation_euler = data['thigh_r']
    pb['calf_r'].rotation_euler = data['calf_r']
    pb['foot_r'].rotation_euler = data['foot_r']
    
    pb['pelvis'].keyframe_insert(data_path="location", frame=f)
    pb['pelvis'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['thigh_l'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['calf_l'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['foot_l'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['thigh_r'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['calf_r'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['foot_r'].keyframe_insert(data_path="rotation_euler", frame=f)

bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 30
export_fbx(arm, mesh, "Anim_Operator_Walk.fbx")


# =========================================================================
# 3. Anim_Operator_Run (24 frames @ 30fps) - Athletic Combat Jog
# =========================================================================
print("--- Generating Anim_Operator_Run ---")
arm, mesh = load_base_fbx()
bpy.context.view_layer.objects.active = arm
bpy.ops.object.mode_set(mode='POSE')
pb = arm.pose.bones

action_run = bpy.data.actions.new("Anim_Operator_Run")
arm.animation_data_create()
arm.animation_data.action = action_run

set_upper_body_ready(pb, forward_lean=10.0)

for b in pb:
    b.keyframe_insert(data_path="rotation_euler", frame=1)
    b.keyframe_insert(data_path="rotation_euler", frame=24)

run_keyframes = {
    1: {  # Left Strike
        'pelvis_loc': (0.0, 0.0, -0.030),
        'thigh_l': (math.radians(-36.0), 0.0, math.radians(-3.0)),
        'calf_l': (math.radians(12.0), 0.0, 0.0),
        'foot_l': (math.radians(-16.0), 0.0, 0.0),
        'thigh_r': (math.radians(26.0), 0.0, math.radians(3.0)),
        'calf_r': (math.radians(55.0), 0.0, 0.0),
        'foot_r': (math.radians(22.0), 0.0, 0.0),
    },
    7: {  # Left Float / Push
        'pelvis_loc': (-0.015, 0.0, 0.020),
        'thigh_l': (math.radians(5.0), 0.0, 0.0),
        'calf_l': (math.radians(15.0), 0.0, 0.0),
        'foot_l': (math.radians(8.0), 0.0, 0.0),
        'thigh_r': (math.radians(-28.0), 0.0, math.radians(4.0)),
        'calf_r': (math.radians(65.0), 0.0, 0.0),
        'foot_r': (math.radians(-10.0), 0.0, 0.0),
    },
    13: { # Right Strike
        'pelvis_loc': (0.0, 0.0, -0.030),
        'thigh_l': (math.radians(26.0), 0.0, math.radians(-3.0)),
        'calf_l': (math.radians(55.0), 0.0, 0.0),
        'foot_l': (math.radians(22.0), 0.0, 0.0),
        'thigh_r': (math.radians(-36.0), 0.0, math.radians(3.0)),
        'calf_r': (math.radians(12.0), 0.0, 0.0),
        'foot_r': (math.radians(-16.0), 0.0, 0.0),
    },
    19: { # Right Float / Push
        'pelvis_loc': (0.015, 0.0, 0.020),
        'thigh_l': (math.radians(-28.0), 0.0, math.radians(-4.0)),
        'calf_l': (math.radians(65.0), 0.0, 0.0),
        'foot_l': (math.radians(-10.0), 0.0, 0.0),
        'thigh_r': (math.radians(5.0), 0.0, 0.0),
        'calf_r': (math.radians(15.0), 0.0, 0.0),
        'foot_r': (math.radians(8.0), 0.0, 0.0),
    },
    24: { # Loop end
        'pelvis_loc': (0.0, 0.0, -0.030),
        'thigh_l': (math.radians(-36.0), 0.0, math.radians(-3.0)),
        'calf_l': (math.radians(12.0), 0.0, 0.0),
        'foot_l': (math.radians(-16.0), 0.0, 0.0),
        'thigh_r': (math.radians(26.0), 0.0, math.radians(3.0)),
        'calf_r': (math.radians(55.0), 0.0, 0.0),
        'foot_r': (math.radians(22.0), 0.0, 0.0),
    }
}

for f, data in run_keyframes.items():
    pb['pelvis'].location = data['pelvis_loc']
    pb['thigh_l'].rotation_euler = data['thigh_l']
    pb['calf_l'].rotation_euler = data['calf_l']
    pb['foot_l'].rotation_euler = data['foot_l']
    pb['thigh_r'].rotation_euler = data['thigh_r']
    pb['calf_r'].rotation_euler = data['calf_r']
    pb['foot_r'].rotation_euler = data['foot_r']
    
    pb['pelvis'].keyframe_insert(data_path="location", frame=f)
    pb['thigh_l'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['calf_l'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['foot_l'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['thigh_r'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['calf_r'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['foot_r'].keyframe_insert(data_path="rotation_euler", frame=f)

bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 24
export_fbx(arm, mesh, "Anim_Operator_Run.fbx")


# =========================================================================
# 4. Anim_Operator_Sprint (18 frames @ 30fps) - Tactical Sprint
# =========================================================================
print("--- Generating Anim_Operator_Sprint ---")
arm, mesh = load_base_fbx()
bpy.context.view_layer.objects.active = arm
bpy.ops.object.mode_set(mode='POSE')
pb = arm.pose.bones

action_sprint = bpy.data.actions.new("Anim_Operator_Sprint")
arm.animation_data_create()
arm.animation_data.action = action_sprint

# Sprint upper body tuck (forward lean 16°, rifle held close)
pb['spine_01'].rotation_euler = (math.radians(16.0), 0.0, math.radians(-3.0))
pb['spine_02'].rotation_euler = (math.radians(14.0), 0.0, math.radians(-2.0))
pb['spine_03'].rotation_euler = (math.radians(12.0), 0.0, math.radians(-2.0))
pb['clavicle_r'].rotation_euler = (math.radians(10.0), math.radians(12.0), math.radians(-15.0))
pb['upperarm_r'].rotation_euler = (math.radians(-45.0), math.radians(25.0), math.radians(-35.0))
pb['lowerarm_r'].rotation_euler = (math.radians(95.0), math.radians(-40.0), math.radians(25.0))
pb['hand_r'].rotation_euler = (math.radians(15.0), math.radians(8.0), math.radians(-20.0))
pb['clavicle_l'].rotation_euler = (math.radians(-8.0), math.radians(-12.0), math.radians(16.0))
pb['upperarm_l'].rotation_euler = (math.radians(-40.0), math.radians(-45.0), math.radians(45.0))
pb['lowerarm_l'].rotation_euler = (math.radians(85.0), math.radians(55.0), math.radians(-25.0))
pb['hand_l'].rotation_euler = (math.radians(-20.0), math.radians(12.0), math.radians(25.0))
pb['neck_01'].rotation_euler = (math.radians(-10.0), 0.0, 0.0)
pb['head'].rotation_euler = (math.radians(-6.0), 0.0, 0.0)

for b in pb:
    b.keyframe_insert(data_path="rotation_euler", frame=1)
    b.keyframe_insert(data_path="rotation_euler", frame=18)

sprint_keyframes = {
    1: {  # Left high knee strike
        'pelvis_loc': (0.0, 0.0, -0.040),
        'thigh_l': (math.radians(-48.0), 0.0, math.radians(-4.0)),
        'calf_l': (math.radians(18.0), 0.0, 0.0),
        'foot_l': (math.radians(-20.0), 0.0, 0.0),
        'thigh_r': (math.radians(34.0), 0.0, math.radians(4.0)),
        'calf_r': (math.radians(85.0), 0.0, 0.0),
        'foot_r': (math.radians(30.0), 0.0, 0.0),
    },
    5: {  # Turnover float
        'pelvis_loc': (-0.02, 0.0, 0.025),
        'thigh_l': (math.radians(8.0), 0.0, 0.0),
        'calf_l': (math.radians(20.0), 0.0, 0.0),
        'foot_l': (math.radians(12.0), 0.0, 0.0),
        'thigh_r': (math.radians(-40.0), 0.0, math.radians(5.0)),
        'calf_r': (math.radians(95.0), 0.0, 0.0),
        'foot_r': (math.radians(-15.0), 0.0, 0.0),
    },
    10: { # Right high knee strike
        'pelvis_loc': (0.0, 0.0, -0.040),
        'thigh_l': (math.radians(34.0), 0.0, math.radians(-4.0)),
        'calf_l': (math.radians(85.0), 0.0, 0.0),
        'foot_l': (math.radians(30.0), 0.0, 0.0),
        'thigh_r': (math.radians(-48.0), 0.0, math.radians(4.0)),
        'calf_r': (math.radians(18.0), 0.0, 0.0),
        'foot_r': (math.radians(-20.0), 0.0, 0.0),
    },
    14: { # Turnover float
        'pelvis_loc': (0.02, 0.0, 0.025),
        'thigh_l': (math.radians(-40.0), 0.0, math.radians(-5.0)),
        'calf_l': (math.radians(95.0), 0.0, 0.0),
        'foot_l': (math.radians(-15.0), 0.0, 0.0),
        'thigh_r': (math.radians(8.0), 0.0, 0.0),
        'calf_r': (math.radians(20.0), 0.0, 0.0),
        'foot_r': (math.radians(12.0), 0.0, 0.0),
    },
    18: { # Loop end
        'pelvis_loc': (0.0, 0.0, -0.040),
        'thigh_l': (math.radians(-48.0), 0.0, math.radians(-4.0)),
        'calf_l': (math.radians(18.0), 0.0, 0.0),
        'foot_l': (math.radians(-20.0), 0.0, 0.0),
        'thigh_r': (math.radians(34.0), 0.0, math.radians(4.0)),
        'calf_r': (math.radians(85.0), 0.0, 0.0),
        'foot_r': (math.radians(30.0), 0.0, 0.0),
    }
}

for f, data in sprint_keyframes.items():
    pb['pelvis'].location = data['pelvis_loc']
    pb['thigh_l'].rotation_euler = data['thigh_l']
    pb['calf_l'].rotation_euler = data['calf_l']
    pb['foot_l'].rotation_euler = data['foot_l']
    pb['thigh_r'].rotation_euler = data['thigh_r']
    pb['calf_r'].rotation_euler = data['calf_r']
    pb['foot_r'].rotation_euler = data['foot_r']
    
    pb['pelvis'].keyframe_insert(data_path="location", frame=f)
    pb['thigh_l'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['calf_l'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['foot_l'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['thigh_r'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['calf_r'].keyframe_insert(data_path="rotation_euler", frame=f)
    pb['foot_r'].keyframe_insert(data_path="rotation_euler", frame=f)

bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 18
export_fbx(arm, mesh, "Anim_Operator_Sprint.fbx")

print(">>> ALL 4 LOCOMOTION ANIMATIONS COMPLETED AND EXPORTED <<<")
sys.exit(0)
