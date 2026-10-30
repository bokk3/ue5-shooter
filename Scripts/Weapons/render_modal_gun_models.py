"""
Render clean isometric 3D weapon models with transparent background and military cyan/amber wireframe/shaded lighting for the Weapon Selection Wheel modal.
"""
import bpy
import os
import math
from mathutils import Vector, Euler

weapons = [
    {
        "id": "W01_NF98",
        "rcv": "SM_W01_NF98_Receiver.fbx",
        "mag": "SM_W01_NF98_Mag.fbx",
        "scale": 1.0,
        "rot": (0, 0, -15)
    },
    {
        "id": "W02_VK74",
        "rcv": "SM_W02_VK74_Receiver.fbx",
        "mag": "SM_W02_VK74_Mag.fbx",
        "scale": 1.0,
        "rot": (0, 0, -15)
    },
    {
        "id": "W03_Falcon50",
        "rcv": "SM_W03_Falcon50_Receiver.fbx",
        "mag": "SM_W03_Falcon50_Mag.fbx",
        "scale": 0.85,
        "rot": (0, 0, -15)
    },
    {
        "id": "W04_Breacher12",
        "rcv": "SM_W04_Breacher12_Receiver.fbx",
        "mag": "SM_W04_Breacher12_Mag.fbx",
        "scale": 1.0,
        "rot": (0, 0, -15)
    },
    {
        "id": "W05_Apex9",
        "rcv": "SM_W05_Apex9_Receiver.fbx",
        "mag": "SM_W05_Apex9_Mag.fbx",
        "scale": 1.25,
        "rot": (0, 0, -15)
    },
    {
        "id": "W06_Spectre45",
        "rcv": "SM_W06_Spectre45_Receiver.fbx",
        "mag": "SM_W06_Spectre45_Mag.fbx",
        "scale": 1.25,
        "rot": (0, 0, -15)
    },
    {
        "id": "W07_Kestrel357",
        "rcv": "SM_W07_Kestrel357_Receiver.fbx",
        "mag": "SM_W07_Kestrel357_Mag.fbx",
        "scale": 1.4,
        "rot": (0, 0, -15)
    },
    {
        "id": "W08_GrenadeM67",
        "rcv": "SM_W08_GrenadeM67_Receiver.fbx",
        "mag": None,
        "scale": 2.2,
        "rot": (15, 20, -15)
    },
    {
        "id": "W09_Stun9",
        "rcv": "SM_W09_Stun9_Receiver.fbx",
        "mag": None,
        "scale": 2.2,
        "rot": (15, 20, -15)
    },
    {
        "id": "W10_Machete",
        "rcv": "SM_W10_Machete_Receiver.fbx",
        "mag": None,
        "scale": 1.1,
        "rot": (0, 0, -25)
    },
    {
        "id": "W11_Stiletto",
        "rcv": "SM_W11_Stiletto_Receiver.fbx",
        "mag": None,
        "scale": 1.5,
        "rot": (0, 0, -25)
    }
]

proc_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi\SourceArt\Weapons\Processed"
out_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Weapons\Thumbnails"
os.makedirs(out_dir, exist_ok=True)

for w in weapons:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.resolution_x = 512
    scene.render.resolution_y = 256
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.render.film_transparent = True

    # Camera
    cam_data = bpy.data.cameras.new(name="Cam")
    cam_obj = bpy.data.objects.new("CamObj", cam_data)
    bpy.context.collection.objects.link(cam_obj)
    scene.camera = cam_obj
    cam_obj.location = (0, -1.8, 0.25)
    cam_obj.rotation_euler = (math.radians(82), 0, 0)
    cam_data.lens = 65

    # Tactical Lighting (Cyan key, Amber rim light, fill)
    l_key = bpy.data.lights.new(name="LightKey", type='POINT')
    l_key.energy = 25.0
    l_key.color = (0.2, 0.85, 1.0)
    o_key = bpy.data.objects.new("ObjLightKey", l_key)
    o_key.location = (-0.8, -1.2, 1.0)
    bpy.context.collection.objects.link(o_key)

    l_rim = bpy.data.lights.new(name="LightRim", type='POINT')
    l_rim.energy = 40.0
    l_rim.color = (1.0, 0.75, 0.2)
    o_rim = bpy.data.objects.new("ObjLightRim", l_rim)
    o_rim.location = (1.2, 0.8, 0.9)
    bpy.context.collection.objects.link(o_rim)

    l_fill = bpy.data.lights.new(name="LightFill", type='POINT')
    l_fill.energy = 15.0
    l_fill.color = (0.8, 0.9, 1.0)
    o_fill = bpy.data.objects.new("ObjLightFill", l_fill)
    o_fill.location = (0.0, -1.5, -0.6)
    bpy.context.collection.objects.link(o_fill)

    # Tactical Gun Metal Material
    mat_metal = bpy.data.materials.new("M_TacticalMetal")
    mat_metal.use_nodes = True
    bsdf = mat_metal.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs['Base Color'].default_value = (0.10, 0.13, 0.16, 1.0)
        bsdf.inputs['Metallic'].default_value = 0.85
        bsdf.inputs['Roughness'].default_value = 0.35

    # Import Receiver
    rcv_f = os.path.join(proc_dir, w["rcv"])
    if os.path.exists(rcv_f):
        bpy.ops.import_scene.fbx(filepath=rcv_f)
    
    # Import Mag if available
    if w["mag"]:
        mag_f = os.path.join(proc_dir, w["mag"])
        if os.path.exists(mag_f):
            bpy.ops.import_scene.fbx(filepath=mag_f)

    # Gather mesh objects and center them
    mesh_objs = [o for o in bpy.data.objects if o.type == 'MESH']
    
    if mesh_objs:
        # Assign material & apply rotation/scale
        for obj in mesh_objs:
            obj.data.materials.clear()
            obj.data.materials.append(mat_metal)

        # Calculate bounding center
        all_coords = []
        for obj in mesh_objs:
            matrix = obj.matrix_world
            for v in obj.data.vertices:
                all_coords.append(matrix @ v.co)

        if all_coords:
            min_x = min(c.x for c in all_coords)
            max_x = max(c.x for c in all_coords)
            min_y = min(c.y for c in all_coords)
            max_y = max(c.y for c in all_coords)
            min_z = min(c.z for c in all_coords)
            max_z = max(c.z for c in all_coords)
            center = Vector(((min_x + max_x) * 0.5, (min_y + max_y) * 0.5, (min_z + max_z) * 0.5))

            # Shift all to origin
            for obj in mesh_objs:
                obj.location -= center
                obj.scale *= w["scale"]
                obj.rotation_euler.z += math.radians(w["rot"][2])
                obj.rotation_euler.x += math.radians(w["rot"][0])
                obj.rotation_euler.y += math.radians(w["rot"][1])

    out_file = os.path.join(out_dir, f"{w['id']}.png")
    scene.render.filepath = out_file
    bpy.ops.render.render(write_still=True)
    print(f"Rendered {w['id']} -> {out_file}")

print("ALL WEAPON 3D MODAL RENDERS COMPLETE!")
