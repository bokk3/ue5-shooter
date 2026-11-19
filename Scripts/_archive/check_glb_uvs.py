import bpy

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath='Scripts/BaseMesh/Soldier.glb')

for obj in bpy.data.objects:
    if obj.type == 'MESH':
        print(f"Object: {obj.name}, UVs: {[uv.name for uv in obj.data.uv_layers]}")
        for m in obj.data.materials:
            print(f"  Material: {m.name}")
