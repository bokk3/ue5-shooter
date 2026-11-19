import bpy
import os

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath='Content/Characters/BaseMale/SM_Soldier_Posed.fbx')

obj = bpy.data.objects['SM_Soldier_Posed']
print("Mesh name:", obj.name)
print("Vertex count:", len(obj.data.vertices))
print("UV layers:", [uv.name for uv in obj.data.uv_layers])
if obj.data.uv_layers:
    uv_layer = obj.data.uv_layers.active.data
    uvs = [uv_layer[i].uv for i in range(min(10, len(uv_layer)))]
    print("Sample UVs:", uvs)
else:
    print("WARNING: NO UV LAYERS FOUND!")

print("Materials:", [m.name for m in obj.data.materials])
