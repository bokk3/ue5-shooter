import bpy
import os
import math
from mathutils import Vector, Euler, Matrix

print("=== STARTING GENERATE_ALL_MENU_ASSETS.PY ===")

os.makedirs('Content/Characters/BaseMale', exist_ok=True)
os.makedirs('Content/Environment/Hangar', exist_ok=True)

# -------------------------------------------------------------------------
# 1. GENERATE SM_SOLDIER_POSED.FBX
# -------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
glb_path = os.path.abspath('Scripts/BaseMesh/Soldier.glb')
bpy.ops.import_scene.gltf(filepath=glb_path)

arm = bpy.data.objects['Character']
arm.animation_data.action = bpy.data.actions['Idle_Character']
bpy.context.scene.frame_set(0)

# Evaluate deformed meshes in world space
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

# Apply matrices to mesh vertex coordinates
bpy.ops.object.select_all(action='DESELECT')
body_obj.select_set(True)
visor_obj.select_set(True)
bpy.context.view_layer.objects.active = body_obj
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# Import AK47
ak_path = os.path.abspath('Content/Characters/BaseMale/SM_AK47.fbx')
bpy.ops.import_scene.fbx(filepath=ak_path)
ak_obj = bpy.data.objects['SM_AK47']

# Position AK47 across the chest in tactical Low-Ready sling carry
ak_obj.location = (0.02, 0.24, 1.08)
ak_obj.rotation_euler = (math.radians(-38), math.radians(28), math.radians(-50))
ak_obj.scale = (1.0, 1.0, 1.0)
bpy.ops.object.select_all(action='DESELECT')
ak_obj.select_set(True)
bpy.context.view_layer.objects.active = ak_obj
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# Join visor and AK47 into body_obj
bpy.ops.object.select_all(action='DESELECT')
visor_obj.select_set(True)
ak_obj.select_set(True)
body_obj.select_set(True)
bpy.context.view_layer.objects.active = body_obj
bpy.ops.object.join()
soldier_mesh = body_obj
soldier_mesh.name = 'SM_Soldier_Posed'

# Ensure exact height 1.80m and ground boots at Z = 0
min_z = min(v.co.z for v in soldier_mesh.data.vertices)
max_z = max(v.co.z for v in soldier_mesh.data.vertices)
curr_h = max_z - min_z
scale_f = 1.80 / curr_h
print(f"Soldier Raw Height: {curr_h:.3f}m -> Scaling to 1.80m (factor: {scale_f:.4f})")

soldier_mesh.scale = (scale_f, scale_f, scale_f)
bpy.ops.object.transform_apply(scale=True)

min_z2 = min(v.co.z for v in soldier_mesh.data.vertices)
for v in soldier_mesh.data.vertices:
    v.co.z -= min_z2

# Rotate so front of soldier (+Y in Blender) faces +X in Unreal (+X is forward)
# A -90 deg rotation around Z turns +Y to +X
soldier_mesh.rotation_euler = (0, 0, math.radians(-90))
bpy.ops.object.transform_apply(rotation=True)

# Material slot names
mat_names = [m.name for m in soldier_mesh.data.materials]
print("SM_Soldier_Posed Materials:", mat_names)

# Export to FBX
soldier_fbx = os.path.abspath('Content/Characters/BaseMale/SM_Soldier_Posed.fbx')
bpy.ops.object.select_all(action='DESELECT')
soldier_mesh.select_set(True)
bpy.context.view_layer.objects.active = soldier_mesh
bpy.ops.export_scene.fbx(
    filepath=soldier_fbx,
    use_selection=True,
    global_scale=1.0, # 1 Blender meter translates to 100 Unreal centimeters via apply_unit_scale
    apply_unit_scale=True,
    axis_forward='X',
    axis_up='Z',
    object_types={'MESH'}
)
print(f"Exported SM_Soldier_Posed.fbx: {soldier_fbx} ({os.path.getsize(soldier_fbx):,} bytes)")

# -------------------------------------------------------------------------
# 2. GENERATE SM_HANGAR_PLINTH.FBX (Radius 1.35m, Height 0.14m)
# -------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.mesh.primitive_cylinder_add(radius=1.35, depth=0.14, vertices=64, location=(0, 0, -0.07))
plinth = bpy.context.active_object
plinth.name = 'SM_Hangar_Plinth'
bev = plinth.modifiers.new('Bevel', 'BEVEL')
bev.width = 0.02
bev.segments = 3
bpy.ops.object.modifier_apply(modifier='Bevel')

# Base material
mat_base = bpy.data.materials.new('Plinth_Base')
plinth.data.materials.append(mat_base)
mat_ring = bpy.data.materials.new('Plinth_Ring')
plinth.data.materials.append(mat_ring)

plinth_fbx = os.path.abspath('Content/Environment/Hangar/SM_Hangar_Plinth.fbx')
bpy.ops.export_scene.fbx(
    filepath=plinth_fbx,
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    axis_forward='X',
    axis_up='Z',
    object_types={'MESH'}
)
print(f"Exported SM_Hangar_Plinth.fbx: {plinth_fbx}")

# -------------------------------------------------------------------------
# 3. GENERATE SM_HANGAR_FLOOR.FBX (32m x 32m)
# -------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.mesh.primitive_plane_add(size=32.0, location=(0, 0, -0.14))
floor = bpy.context.active_object
floor.name = 'SM_Hangar_Floor'
mat_floor = bpy.data.materials.new('Floor_Mat')
floor.data.materials.append(mat_floor)

floor_fbx = os.path.abspath('Content/Environment/Hangar/SM_Hangar_Floor.fbx')
bpy.ops.export_scene.fbx(
    filepath=floor_fbx,
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    axis_forward='X',
    axis_up='Z',
    object_types={'MESH'}
)
print(f"Exported SM_Hangar_Floor.fbx: {floor_fbx}")

# -------------------------------------------------------------------------
# 4. GENERATE SM_HANGAR_WALL.FBX (Industrial Column 8m high)
# -------------------------------------------------------------------------
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 4.0))
wall = bpy.context.active_object
wall.name = 'SM_Hangar_Wall'
wall.scale = (0.6, 0.6, 8.0)
bpy.ops.object.transform_apply(scale=True)
mat_wall = bpy.data.materials.new('Wall_Mat')
wall.data.materials.append(mat_wall)

wall_fbx = os.path.abspath('Content/Environment/Hangar/SM_Hangar_Wall.fbx')
bpy.ops.export_scene.fbx(
    filepath=wall_fbx,
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    axis_forward='X',
    axis_up='Z',
    object_types={'MESH'}
)
print(f"Exported SM_Hangar_Wall.fbx: {wall_fbx}")

print("=== ALL FBX ASSETS SUCCESSFULLY GENERATED ===")
