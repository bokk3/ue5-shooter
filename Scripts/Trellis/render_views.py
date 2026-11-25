"""Blender: render front / right / top orthographic views of each GLB listed in a file.

blender -b -P render_views.py -- <list.txt> <out_dir>
Views are in glTF terms: front looks at the model's +Z face, right looks at its +X face, top looks down -Y.
(Blender imports glTF +Z as -Y, +X as +X, +Y as +Z.)
"""
import math
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
paths = [l.strip() for l in open(args[0], encoding="utf-8") if l.strip()]
out_dir = args[1]
os.makedirs(out_dir, exist_ok=True)

scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "STUDIO"
scene.display.shading.color_type = "TEXTURE"
scene.display.shading.show_cavity = True
scene.render.resolution_x = scene.render.resolution_y = 320
scene.render.film_transparent = False
scene.world = scene.world or bpy.data.worlds.new("W")
scene.display.shading.background_type = "VIEWPORT"
scene.render.image_settings.file_format = "PNG"

VIEWS = {  # blender direction from the model to the camera, camera up
    "front": (Vector((0, -1, 0)), Vector((0, 0, 1))),
    "right": (Vector((1, 0, 0)), Vector((0, 0, 1))),
    "top": (Vector((0, 0, 1)), Vector((0, 1, 0))),
}

for path in paths:
    name = os.path.splitext(os.path.basename(path))[0]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    scene.display.shading.show_cavity = True
    scene.render.resolution_x = scene.render.resolution_y = 320
    scene.render.image_settings.file_format = "PNG"
    world = bpy.data.worlds.new("W")
    world.color = (0.35, 0.37, 0.40)
    scene.world = world
    bpy.ops.import_scene.gltf(filepath=path)
    meshes = [o for o in scene.objects if o.type == "MESH"]
    lo = Vector((1e9, 1e9, 1e9))
    hi = Vector((-1e9, -1e9, -1e9))
    for o in meshes:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            lo = Vector(map(min, lo, w))
            hi = Vector(map(max, hi, w))
    center = (lo + hi) / 2
    size = max(hi - lo)
    cam_data = bpy.data.cameras.new("C")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = size * 1.15
    cam = bpy.data.objects.new("C", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    for view, (direction, up) in VIEWS.items():
        cam.location = center + direction * size * 3
        forward = -direction
        right = forward.cross(up).normalized()
        true_up = right.cross(forward).normalized()
        from mathutils import Matrix
        rot = Matrix((right, true_up, -forward)).transposed()
        cam.rotation_euler = rot.to_euler()
        cam_data.clip_end = size * 10
        scene.render.filepath = os.path.join(out_dir, f"{name}__{view}.png")
        bpy.ops.render.render(write_still=True)
    print(f"RENDERED {name}")
