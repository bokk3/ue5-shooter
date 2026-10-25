import bpy

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath='Scripts/BaseMesh/Soldier.glb')

mat = bpy.data.materials.get('VanguardBodyMat')
if mat and mat.node_tree:
    for node in mat.node_tree.nodes:
        print(f"Node: {node.name} ({node.type})")
        if node.type == 'UVMAP':
            print(f"  UV Map name: {node.uv_map}")
        if node.type == 'TEX_IMAGE':
            print(f"  Image: {node.image.name if node.image else 'None'}")
            for inp in node.inputs:
                for link in inp.links:
                    print(f"    Input {inp.name} linked from {link.from_node.name} ({link.from_node.type})")
