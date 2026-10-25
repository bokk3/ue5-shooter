import bpy

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath='Scripts/BaseMesh/Soldier.glb')

mesh = bpy.data.objects['vanguard_Mesh'].data
print("Active UV:", mesh.uv_layers.active.name)
for uv in mesh.uv_layers:
    vals = [uv.data[i].uv for i in range(min(5, len(uv.data)))]
    print(f"UV: {uv.name}, active_render: {uv.active_render}, sample: {vals}")
