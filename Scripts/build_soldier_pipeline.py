import bpy
import bmesh
import os
import math
from mathutils import Vector, Matrix, Euler

print("=== STARTING BUILD_SOLDIER_PIPELINE (Vanguard Soldier -> SK_BaseMale) ===")

glb_path = os.path.abspath('Scripts/BaseMesh/Soldier.glb')
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

arm_obj = bpy.data.objects.get('Character')
mesh_body = bpy.data.objects.get('vanguard_Mesh')
mesh_visor = bpy.data.objects.get('vanguard_visor')

if not arm_obj or not mesh_body or not mesh_visor:
    raise RuntimeError("Missing Character armature or Vanguard meshes!")

# Remove unneeded objects like Icosphere and mixamorig empty helpers
for obj in list(bpy.data.objects):
    if obj.name not in ['Character', 'vanguard_Mesh', 'vanguard_visor']:
        bpy.data.objects.remove(obj, do_unlink=True)

# 1. Join vanguard_visor into vanguard_Mesh so we have a single, unified skeletal mesh
bpy.ops.object.select_all(action='DESELECT')
mesh_visor.select_set(True)
mesh_body.select_set(True)
bpy.context.view_layer.objects.active = mesh_body
bpy.ops.object.join()
mesh_obj = mesh_body
mesh_obj.name = "SK_BaseMale"

# 2. Reset Armature pose to Rest Pose
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='POSE')
for pb in arm_obj.pose.bones:
    pb.matrix_basis = Matrix.Identity(4)
bpy.ops.object.mode_set(mode='OBJECT')

# Clear animation data so rest pose is exported cleanly
arm_obj.animation_data_clear()
mesh_obj.animation_data_clear()

# 3. Apply all transforms on mesh and armature
bpy.ops.object.select_all(action='DESELECT')
mesh_obj.select_set(True)
arm_obj.select_set(True)
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# 4. Bone renaming map (Mixamo -> Unreal Engine / NobleFermi standard skeleton)
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

# Add fingers
for side, s_code in [('Left', 'l'), ('Right', 'r')]:
    for i in range(1, 4):
        rename_map[f'mixamorig:{side}HandThumb{i}'] = f'thumb_0{i}_{s_code}'
        rename_map[f'mixamorig:{side}HandIndex{i}'] = f'index_0{i}_{s_code}'
        rename_map[f'mixamorig:{side}HandMiddle{i}'] = f'middle_0{i}_{s_code}'
        rename_map[f'mixamorig:{side}HandRing{i}'] = f'ring_0{i}_{s_code}'
    rename_map[f'mixamorig:{side}HandPinky1'] = f'pinky_01_{s_code}'
    rename_map[f'mixamorig:{side}HandPinky2'] = f'pinky_02_{s_code}'

# Rename vertex groups on mesh
for old_name, new_name in rename_map.items():
    vg = mesh_obj.vertex_groups.get(old_name)
    if vg:
        vg.name = new_name

# 5. Edit Armature: Rename bones and add root & weapon_r
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='EDIT')
ebones = arm_obj.data.edit_bones

# Rename bones
for old_name, new_name in rename_map.items():
    b = ebones.get(old_name)
    if b:
        b.name = new_name

# Add root bone at origin
root_bone = ebones.new('root')
root_bone.head = (0, 0, 0)
root_bone.tail = (0, 0.1, 0)
root_bone.roll = 0.0

pelvis_bone = ebones.get('pelvis')
if pelvis_bone:
    pelvis_bone.parent = root_bone

# Add weapon_r bone as child of hand_r
hand_r_bone = ebones.get('hand_r')
if hand_r_bone:
    weapon_bone = ebones.new('weapon_r')
    weapon_bone.head = hand_r_bone.head.copy()
    weapon_bone.tail = hand_r_bone.head + Vector((0, -0.15, 0)) # Points along forward weapon direction
    weapon_bone.parent = hand_r_bone
    weapon_bone.use_connect = False

bpy.ops.object.mode_set(mode='OBJECT')

# 6. Scale character to exact 180cm standing height
min_z = min(v.co.z for v in mesh_obj.data.vertices)
max_z = max(v.co.z for v in mesh_obj.data.vertices)
current_h = max_z - min_z
scale_to_180 = 1.80 / current_h
print(f"Current Height: {current_h:.4f}m -> Scaling to 1.80m (factor: {scale_to_180:.4f})")

arm_obj.scale = (scale_to_180, scale_to_180, scale_to_180)
mesh_obj.scale = (scale_to_180, scale_to_180, scale_to_180)

bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.select_all(action='DESELECT')
arm_obj.select_set(True)
mesh_obj.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# 7. Offset in Z so the boots sit exactly on ground plane Z = 0
min_z_after = min(v.co.z for v in mesh_obj.data.vertices)
print(f"Grounding character: min Z is {min_z_after:.4f}m")
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

final_min_z = min(v.co.z for v in mesh_obj.data.vertices)
final_max_z = max(v.co.z for v in mesh_obj.data.vertices)
print(f"Final Standing Z Range: [{final_min_z:.4f}m, {final_max_z:.4f}m] (Height: {final_max_z - final_min_z:.4f}m)")

# 8. Export textures
tex_dir = os.path.abspath('Content/Characters/BaseMale/Textures')
os.makedirs(tex_dir, exist_ok=True)

for img in bpy.data.images:
    if 'vanguard_diffuse' in img.name or 'file2' in img.name:
        dest_name = 'T_Soldier_Diffuse.png' if 'diffuse' in img.name else 'T_Soldier_Normal.png'
        out_p = os.path.join(tex_dir, dest_name)
        img.filepath_raw = out_p
        img.file_format = 'PNG'
        img.save()
        print(f"Saved texture {img.name} -> {out_p}")

# 9. Ensure outward smooth normals
bpy.context.view_layer.objects.active = mesh_obj
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.normals_make_consistent(inside=False)
bpy.ops.object.mode_set(mode='OBJECT')
for poly in mesh_obj.data.polygons:
    poly.use_smooth = True

# 10. Export to FBX
out_fbx = os.path.abspath('Content/Characters/BaseMale/SK_BaseMale.fbx')
bpy.ops.object.select_all(action='DESELECT')
arm_obj.select_set(True)
mesh_obj.select_set(True)
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
    bake_anim=False
)

print(f"SUCCESSFULLY EXPORTED SK_BaseMale.fbx: {out_fbx} ({os.path.getsize(out_fbx):,} bytes)")
