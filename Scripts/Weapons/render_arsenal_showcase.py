"""
Noble Fermi // Studio Showcase Lighting & Camera Multi-Angle Renderer
Renders high-fidelity showcase renders of all 11 procedural arsenal weapons on the tactical dark workbench.
"""
import bpy
import os
import math
from mathutils import Vector, Euler

bpy.ops.wm.read_factory_settings(use_empty=True)

scene = bpy.context.scene
scene.render.engine = 'BLENDER_EEVEE_NEXT' if hasattr(bpy.types, 'RenderSettings') and 'BLENDER_EEVEE_NEXT' in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties['engine'].enum_items] else 'BLENDER_EEVEE'
scene.render.resolution_x = 1920
scene.render.resolution_y = 1080
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'

if hasattr(scene, 'eevee'):
    scene.eevee.taa_render_samples = 32

# Tactical dark workbench world atmosphere
world = bpy.data.worlds.new("ArmoryWorld")
scene.world = world
world.use_nodes = True
bg_node = world.node_tree.nodes.get("Background")
if bg_node:
    bg_node.inputs[0].default_value = (0.015, 0.018, 0.024, 1.0)
    bg_node.inputs[1].default_value = 0.65

def create_mat(name, color, metallic, roughness):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs['Base Color'].default_value = color
        bsdf.inputs['Metallic'].default_value = metallic
        bsdf.inputs['Roughness'].default_value = roughness
    return mat

mat_table = create_mat("M_Table", (0.012, 0.014, 0.018, 1.0), 0.35, 0.35)

# Armory Table Base
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, -0.05))
table = bpy.context.active_object
table.scale = (4.4, 2.0, 0.1)
table.data.materials.append(mat_table)

# Back Wall with illuminated edge
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 1.0, 0.6))
wall = bpy.context.active_object
wall.scale = (4.6, 0.08, 1.4)
wall.data.materials.append(mat_table)

# Cyan edge strip
mat_accent = create_mat("M_Cyan", (0.0, 0.85, 1.0, 1.0), 0.0, 0.1)
if 'Emission Color' in mat_accent.node_tree.nodes.get("Principled BSDF").inputs:
    mat_accent.node_tree.nodes.get("Principled BSDF").inputs['Emission Color'].default_value = (0.0, 0.85, 1.0, 1.0)
    mat_accent.node_tree.nodes.get("Principled BSDF").inputs['Emission Strength'].default_value = 5.0

bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0.95, 0.005))
trim = bpy.context.active_object
trim.scale = (4.4, 0.015, 0.008)
trim.data.materials.append(mat_accent)

base_dir = os.path.abspath("SourceArt/Weapons/Processed")

def import_obj(fbx_name, loc, rot_deg):
    p = os.path.join(base_dir, fbx_name)
    if not os.path.exists(p):
        print(f"File not found: {p}")
        return None
    bpy.ops.import_scene.fbx(filepath=p)
    objs = bpy.context.selected_objects
    if not objs:
        return None
    o = objs[0]
    o.location = Vector(loc)
    o.rotation_euler = Euler((math.radians(rot_deg[0]), math.radians(rot_deg[1]), math.radians(rot_deg[2])), 'XYZ')
    return o

# --- ROW 1: PRIMARY WEAPONS (Y = 0.50) ---
# W01 NF-98 Carbine + STANAG Mag
import_obj("SM_W01_NF98_Receiver.fbx", (-1.35, 0.52, 0.04), (90, 0, -15))
import_obj("SM_W01_NF98_Mag.fbx",      (-1.58, 0.20, 0.02), (90, 0, 20))

# W02 VK-74 Vektor + Banana Mag
import_obj("SM_W02_VK74_Receiver.fbx", (-0.48, 0.55, 0.04), (90, 0, -15))
import_obj("SM_W02_VK74_Mag.fbx",      (-0.70, 0.20, 0.02), (90, 0, 25))

# W03 Falcon-50 DMR + 20rd Box Mag
import_obj("SM_W03_Falcon50_Receiver.fbx", (0.42, 0.58, 0.04), (90, 0, -15))
import_obj("SM_W03_Falcon50_Mag.fbx",      (0.16, 0.22, 0.02), (90, 0, 10))

# W04 Breacher-12 Shotgun + 12ga Shell
import_obj("SM_W04_Breacher12_Receiver.fbx", (1.32, 0.54, 0.04), (90, 0, -15))
import_obj("SM_W04_Breacher12_Mag.fbx",      (1.04, 0.22, 0.015), (0, 90, 30))


# --- ROW 2: SIDEARMS & TACTICAL MELEE (Y = -0.30) ---
# W05 Apex-9 Pistol + Mag
import_obj("SM_W05_Apex9_Receiver.fbx", (-1.40, -0.28, 0.03), (90, 0, -10))
import_obj("SM_W05_Apex9_Mag.fbx",      (-1.54, -0.44, 0.015), (90, 0, 30))

# W06 Spectre-45 Pistol + Mag
import_obj("SM_W06_Spectre45_Receiver.fbx", (-0.95, -0.28, 0.03), (90, 0, -10))
import_obj("SM_W06_Spectre45_Mag.fbx",      (-1.08, -0.44, 0.015), (90, 0, 30))

# W07 Kestrel-357 Revolver + Speedloader
import_obj("SM_W07_Kestrel357_Receiver.fbx", (-0.50, -0.28, 0.03), (90, 0, -10))
import_obj("SM_W07_Kestrel357_Mag.fbx",      (-0.64, -0.42, 0.03), (0, 0, 0))

# W08 M67 Frag Grenade (Textured Sphere + Fuse Assembly)
import_obj("SM_W08_GrenadeM67_Receiver.fbx", (-0.08, -0.32, 0.045), (0, 0, 0))

# W09 Stun-9 Flashbang (Canister + Ring)
import_obj("SM_W09_Stun9_Receiver.fbx",      (0.18, -0.32, 0.03), (0, 90, 25))

# W10 Favela Bolo Machete
import_obj("SM_W10_Machete_Receiver.fbx",    (0.68, -0.24, 0.02), (90, 0, -35))

# W11 Shadow Stiletto
import_obj("SM_W11_Stiletto_Receiver.fbx",   (1.22, -0.26, 0.02), (90, 0, -35))


# --- LIGHTING ---
# Main Overhead Key Light
bpy.ops.object.light_add(type='AREA', location=(-1.2, -1.2, 2.5))
l_key = bpy.context.active_object
l_key.data.energy = 950.0
l_key.data.size = 3.6
l_key.data.color = (1.0, 0.98, 0.95)
l_key.rotation_euler = Euler((math.radians(45), math.radians(-20), math.radians(15)), 'XYZ')

# Cool Cyan Fill
bpy.ops.object.light_add(type='AREA', location=(1.8, -1.0, 2.2))
l_fill = bpy.context.active_object
l_fill.data.energy = 500.0
l_fill.data.size = 3.6
l_fill.data.color = (0.7, 0.88, 1.0)
l_fill.rotation_euler = Euler((math.radians(40), math.radians(30), math.radians(-20)), 'XYZ')

# Back Top Rim
bpy.ops.object.light_add(type='AREA', location=(0.0, 1.2, 1.8))
l_rim = bpy.context.active_object
l_rim.data.energy = 900.0
l_rim.data.size = 4.2
l_rim.data.color = (0.9, 0.96, 1.0)
l_rim.rotation_euler = Euler((math.radians(-55), 0, 0), 'XYZ')


# --- CAMERAS & FRAMING ---
cam_data = bpy.data.cameras.new(name="MainCam")
cam_obj = bpy.data.objects.new("MainCamObj", cam_data)
bpy.context.collection.objects.link(cam_obj)
scene.camera = cam_obj

# Row 1 (Rifles): Y ~ 0.38 - 0.58. Looking from Y=-0.80, Z=1.30 with rotX=50 deg gives target Y around 0.45.
# Row 2 (Sidearms): Y ~ -0.30. Looking from Y=-1.20, Z=1.00 with rotX=50 deg gives target Y around -0.32.
shots = [
    {
        "name": "arsenal_showcase_wide.png",
        "loc": (0.0, -1.90, 1.78),
        "rot": (math.radians(47), 0, 0),
        "lens": 29
    },
    {
        "name": "arsenal_rifles_detail.png",
        "loc": (0.0, -0.80, 1.30),
        "rot": (math.radians(50), 0, 0),
        "lens": 28
    },
    {
        "name": "arsenal_sidearms_detail.png",
        "loc": (-0.05, -1.25, 0.95),
        "rot": (math.radians(49), 0, 0),
        "lens": 26
    }
]

out_dir = os.path.abspath("Docs/Screenshots")
os.makedirs(out_dir, exist_ok=True)

for shot in shots:
    cam_obj.location = shot["loc"]
    cam_obj.rotation_euler = Euler(shot["rot"], 'XYZ')
    cam_data.lens = shot["lens"]
    
    out_path = os.path.join(out_dir, shot["name"])
    scene.render.filepath = out_path
    print(f"Rendering {shot['name']}...")
    bpy.ops.render.render(write_still=True)

print("Render complete!")
