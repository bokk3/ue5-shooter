import bpy
import bmesh
import os
import math
from mathutils import Vector, Matrix, Quaternion

print("=== STARTING CLEAN HIGH-FIDELITY OPERATOR PIPELINE (SK_BaseMale & SM_Soldier_Posed) ===")

glb_path = os.path.abspath('Scripts/BaseMesh/Soldier.glb')
out_sk_fbx = os.path.abspath('Content/Characters/BaseMale/SK_BaseMale.fbx')
out_posed_fbx = os.path.abspath('Content/Characters/BaseMale/SM_Soldier_Posed.fbx')


def strip_belt_pouch_islands(mesh_obj):
    """
    Separates and removes the baked-in boxy belt pouches, clips, and studs from vanguard_Mesh
    while preserving the continuous waist belt strap ring (ISL023/ISL035) and front buckle plate (ISL055/ISL107).
    """
    me = mesh_obj.data
    vg_names = {g.index: g.name for g in mesh_obj.vertex_groups}

    bm = bmesh.new()
    bm.from_mesh(me)
    bm.verts.ensure_lookup_table()

    seen = set()
    islands = []
    for v in bm.verts:
        if v.index in seen:
            continue
        stack = [v]
        isl = []
        seen.add(v.index)
        while stack:
            c = stack.pop()
            isl.append(c.index)
            for e in c.link_edges:
                n = e.other_vert(c)
                if n.index not in seen:
                    seen.add(n.index)
                    stack.append(n)
        islands.append(isl)

    verts_to_remove = set()
    removed_islands = 0

    for isl in islands:
        if len(isl) > 200:
            continue
        # Check if island belongs exclusively to mixamorig:Hips
        hips_count = 0
        other_bone_count = 0
        for vid in isl:
            for g in me.vertices[vid].groups:
                if g.weight > 0.25:
                    bname = vg_names.get(g.group, '')
                    if bname == 'mixamorig:Hips':
                        hips_count += 1
                    else:
                        other_bone_count += 1
        if hips_count == 0 or other_bone_count > 0:
            continue

        pts = [mesh_obj.matrix_world @ me.vertices[vid].co for vid in isl]
        cx = sum(p.x for p in pts) / len(pts)
        cy = sum(p.y for p in pts) / len(pts)
        cz = sum(p.z for p in pts) / len(pts)
        ex = max(p.x for p in pts) - min(p.x for p in pts)
        ey = max(p.y for p in pts) - min(p.y for p in pts)
        ez = max(p.z for p in pts) - min(p.z for p in pts)

        # Keep the main waist belt strap ring (ex ~ 0.39-0.41m)
        if ex > 0.25:
            continue
        # Keep the flat central front belt buckle plate (cx ~ 0, cy > 0.12, ey < 0.025)
        if abs(cx) < 0.025 and cy > 0.12 and ey < 0.025:
            continue

        # Remove boxy belt pouch / clip / stud island
        removed_islands += 1
        for vid in isl:
            verts_to_remove.add(vid)

    if verts_to_remove:
        bmesh.ops.delete(bm, geom=[bm.verts[i] for i in verts_to_remove], context='VERTS')
        bm.to_mesh(me)
        me.update()

    bm.free()
    print(f"[BeltStrip] Stripped {removed_islands} baked belt pouch/clip islands ({len(verts_to_remove)} vertices removed, {len(me.vertices)} vertices remaining)")


# =========================================================================
# PART 1: GENERATE SM_SOLDIER_POSED.FBX (Hangar Turntable Master Operator)
# Pure high-res Vanguard Spec-Ops Soldier with 100% intact UVs & zero box clutter
# =========================================================================
print("--- [1/2] Generating Clean SM_Soldier_Posed.fbx ---")
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

mesh_body = bpy.data.objects['vanguard_Mesh']
strip_belt_pouch_islands(mesh_body)

arm = bpy.data.objects['Character']
arm.animation_data.action = bpy.data.actions['Idle_Character']
bpy.context.scene.frame_set(0)
bpy.context.view_layer.update()

bpy.context.view_layer.objects.active = arm
bpy.ops.object.mode_set(mode='POSE')
leg_bones_posed = [
    'mixamorig:LeftUpLeg', 'mixamorig:LeftLeg', 'mixamorig:LeftFoot', 'mixamorig:LeftToeBase',
    'mixamorig:RightUpLeg', 'mixamorig:RightLeg', 'mixamorig:RightFoot', 'mixamorig:RightToeBase',
    'mixamorig:Hips', 'mixamorig:Spine', 'mixamorig:Spine1', 'mixamorig:Spine2'
]
for bname in leg_bones_posed:
    pb = arm.pose.bones.get(bname)
    if pb:
        pb.rotation_mode = 'QUATERNION'
        if 'Hips' in bname:
            pb.rotation_quaternion = pb.rotation_quaternion.slerp(Quaternion((1.0, 0.0, 0.0, 0.0)), 0.55)
        elif 'Spine' in bname:
            pb.rotation_quaternion = pb.rotation_quaternion.slerp(Quaternion((1.0, 0.0, 0.0, 0.0)), 0.50)
        else:
            pb.rotation_quaternion = pb.rotation_quaternion.slerp(Quaternion((1.0, 0.0, 0.0, 0.0)), 0.72)
bpy.ops.object.mode_set(mode='OBJECT')
bpy.context.view_layer.update()

dg = bpy.context.evaluated_depsgraph_get()
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

# Remove all other scene objects before exporting posed static mesh
for obj in list(bpy.data.objects):
    if obj not in (body_obj, visor_obj):
        bpy.data.objects.remove(obj, do_unlink=True)

# Apply world matrices to mesh coordinates
bpy.ops.object.select_all(action='DESELECT')
body_obj.select_set(True)
visor_obj.select_set(True)
bpy.context.view_layer.objects.active = body_obj
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# Assign clean material slots BEFORE joining so 100% of body/helmet/boots stay on Slot 0
# and 100% of the visor stays on Slot 1
m_uniform = bpy.data.materials.new(name="M_Operator_BaseBody") # Slot 0
m_visor   = bpy.data.materials.new(name="M_Soldier_Visor")     # Slot 1

body_obj.data.materials.clear()
body_obj.data.materials.append(m_uniform)
body_obj.data.materials.append(m_visor)
for poly in body_obj.data.polygons:
    poly.material_index = 0

visor_obj.data.materials.clear()
visor_obj.data.materials.append(m_uniform)
visor_obj.data.materials.append(m_visor)
for poly in visor_obj.data.polygons:
    poly.material_index = 1

# Join visor into body_obj
bpy.ops.object.select_all(action='DESELECT')
visor_obj.select_set(True)
body_obj.select_set(True)
bpy.context.view_layer.objects.active = body_obj
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

# Rotate -90 degrees around Z so front (+Y in Blender) faces +X in Unreal (+X is forward)
soldier_mesh.rotation_euler = (0, 0, math.radians(-90))
bpy.ops.object.select_all(action='DESELECT')
soldier_mesh.select_set(True)
bpy.context.view_layer.objects.active = soldier_mesh
bpy.ops.object.transform_apply(rotation=True)

# Ensure smooth normals
for poly in soldier_mesh.data.polygons:
    poly.use_smooth = True

print(f"Clean Posed Soldier: {len(soldier_mesh.data.vertices)} vertices, {len(soldier_mesh.data.polygons)} polygons, grounded at Z=0")
print(f"Material slots: {[m.name for m in soldier_mesh.data.materials]}")

# Export SM_Soldier_Posed.fbx
bpy.ops.export_scene.fbx(
    filepath=out_posed_fbx,
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    axis_forward='X',
    axis_up='Z',
    object_types={'MESH'},
    mesh_smooth_type='FACE'
)
print(f"Exported clean SM_Soldier_Posed.fbx: {out_posed_fbx} ({os.path.getsize(out_posed_fbx):,} bytes)")

# =========================================================================
# PART 2: GENERATE SK_BASEMALE.FBX (Clean Rest-Pose Skeletal Mesh Rig)
# =========================================================================
print("--- [2/2] Generating Clean SK_BaseMale.fbx ---")
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb_path)

arm_obj = bpy.data.objects.get('Character')
mesh_body = bpy.data.objects.get('vanguard_Mesh')
mesh_visor = bpy.data.objects.get('vanguard_visor')

strip_belt_pouch_islands(mesh_body)

# Remove unneeded objects
for obj in list(bpy.data.objects):
    if obj.name not in ['Character', 'vanguard_Mesh', 'vanguard_visor']:
        bpy.data.objects.remove(obj, do_unlink=True)

# Assign clean materials BEFORE joining
m_sk_uniform = bpy.data.materials.new(name="M_Operator_BaseBody")
m_sk_visor   = bpy.data.materials.new(name="M_Soldier_Visor")

mesh_body.data.materials.clear()
mesh_body.data.materials.append(m_sk_uniform)
mesh_body.data.materials.append(m_sk_visor)
for poly in mesh_body.data.polygons:
    poly.material_index = 0

mesh_visor.data.materials.clear()
mesh_visor.data.materials.append(m_sk_uniform)
mesh_visor.data.materials.append(m_sk_visor)
for poly in mesh_visor.data.polygons:
    poly.material_index = 1

# Join visor into mesh_body
bpy.ops.object.select_all(action='DESELECT')
mesh_visor.select_set(True)
mesh_body.select_set(True)
bpy.context.view_layer.objects.active = mesh_body
bpy.ops.object.join()
sk_mesh = mesh_body
sk_mesh.name = "SK_BaseMale"

# Bake Upright, Grounded 180cm Tactical Resting Pose from Idle_Character as Rest Pose
# (Preserves authentic Mixamo shoulder-pauldron & helmet bone rolls while straightening knees!)
arm_obj.animation_data.action = bpy.data.actions['Idle_Character']
bpy.context.scene.frame_set(0)
bpy.context.view_layer.update()

bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='POSE')

# Straighten knees by blending leg bones 72% toward upright standing rest pose
leg_bones = [
    'mixamorig:LeftUpLeg', 'mixamorig:LeftLeg', 'mixamorig:LeftFoot', 'mixamorig:LeftToeBase',
    'mixamorig:RightUpLeg', 'mixamorig:RightLeg', 'mixamorig:RightFoot', 'mixamorig:RightToeBase',
    'mixamorig:Hips', 'mixamorig:Spine', 'mixamorig:Spine1', 'mixamorig:Spine2'
]
for bname in leg_bones:
    pb = arm_obj.pose.bones.get(bname)
    if pb:
        pb.rotation_mode = 'QUATERNION'
        if 'Hips' in bname:
            pb.rotation_quaternion = pb.rotation_quaternion.slerp(Quaternion((1.0, 0.0, 0.0, 0.0)), 0.55)
        elif 'Spine' in bname:
            pb.rotation_quaternion = pb.rotation_quaternion.slerp(Quaternion((1.0, 0.0, 0.0, 0.0)), 0.50)
        else:
            pb.rotation_quaternion = pb.rotation_quaternion.slerp(Quaternion((1.0, 0.0, 0.0, 0.0)), 0.72)

bpy.ops.object.mode_set(mode='OBJECT')
bpy.context.view_layer.update()

# Apply current Armature modifier on sk_mesh so mesh vertices match the upright tactical pose
bpy.context.view_layer.objects.active = sk_mesh
for mod in list(sk_mesh.modifiers):
    if mod.type == 'ARMATURE':
        bpy.ops.object.modifier_apply(modifier=mod.name)

# Apply pose as the new Rest Pose of the Armature
bpy.context.view_layer.objects.active = arm_obj
bpy.ops.object.mode_set(mode='POSE')
bpy.ops.pose.armature_apply(selected=False)
bpy.ops.object.mode_set(mode='OBJECT')

# Re-add Armature modifier binding sk_mesh to arm_obj
arm_mod = sk_mesh.modifiers.new(name='Armature', type='ARMATURE')
arm_mod.object = arm_obj

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

# Ensure smooth normals
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
print(f"Exported clean SK_BaseMale.fbx: {out_sk_fbx} ({os.path.getsize(out_sk_fbx):,} bytes)")
print("=== CLEAN OPERATOR RIG & POSED STATIC MESH GENERATED SUCCESSFULLY ===")
