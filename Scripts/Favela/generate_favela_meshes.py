"""
NOBLE FERMI // FAVELA MESH KIT GENERATOR (Blender 4.2 headless)
Run: blender.exe -b --python Scripts/Favela/generate_favela_meshes.py

Exports to SourceArt/Favela/Meshes/*.fbx (1 Blender metre == 100 Unreal cm):
  SM_Favela_Cable      Unit catenary carrier tube along +X (0..1 m). UV.x = position along span.
                       Sag + wind sway are applied on the GPU (material WorldPositionOffset).
  SM_Favela_RoofSheet  2.4 m x 1.1 m corrugated sine-profile zinc sheet (76 mm pitch, 25 mm depth).
  SM_Favela_Weeds      Crossed-card grass/weed clump (UV v=0 at blade tips -> wind weight).
  SM_Favela_Ivy        Wall-hugging ivy card cluster facing +X.
  SM_Favela_Banana     Banana plant: stem + 7 arching torn leaves.
  SM_Favela_Tire       Discarded car tyre (roof hold-down weight).
  SM_Favela_WaterTank  Ribbed 1000 L polyethylene water tank (caixa d'agua) with lid.
"""
import bpy
import bmesh
import math
import os
from mathutils import Vector

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT = os.path.join(ROOT, "SourceArt", "Favela", "Meshes")
os.makedirs(OUT, exist_ok=True)


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def new_obj(name, bm, mat_name, smooth=True):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    mat = bpy.data.materials.new(mat_name)
    obj.data.materials.append(mat)
    for p in obj.data.polygons:
        p.use_smooth = smooth
    return obj


def export(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUT, obj.name + ".fbx")
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=True,
        global_scale=1.0,
        apply_unit_scale=True,
        axis_forward="X",
        axis_up="Z",
        object_types={"MESH"},
        mesh_smooth_type="FACE",
    )
    print("Exported", path, len(obj.data.polygons), "faces")


def add_quad(bm, uv_layer, verts_uv):
    """verts_uv: list of 4 ((x,y,z),(u,v)) counter-clockwise."""
    vs = [bm.verts.new(p) for p, _ in verts_uv]
    f = bm.faces.new(vs)
    for loop, (_, uv) in zip(f.loops, verts_uv):
        loop[uv_layer].uv = uv
    return f


# -----------------------------------------------------------------------------------------
def make_cable():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    segs, sides, r = 32, 6, 0.011
    rings = []
    for i in range(segs + 1):
        x = i / segs
        ring = []
        for s in range(sides):
            a = s / sides * math.pi * 2
            ring.append(bm.verts.new((x, math.cos(a) * r, math.sin(a) * r)))
        rings.append(ring)
    for i in range(segs):
        for s in range(sides):
            s2 = (s + 1) % sides
            f = bm.faces.new([rings[i][s], rings[i + 1][s], rings[i + 1][s2], rings[i][s2]])
            uvs = [(i / segs, s / sides), ((i + 1) / segs, s / sides), ((i + 1) / segs, (s + 1) / sides), (i / segs, (s + 1) / sides)]
            for loop, uv in zip(f.loops, uvs):
                loop[uvl].uv = uv
    bm.normal_update()
    export(new_obj("SM_Favela_Cable", bm, "Cable"))


def make_roof_sheet():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    L, W, pitch, depth, thick = 2.4, 1.1, 0.076, 0.0125, 0.006
    ny = int(round(W / pitch * 8))
    nx = 2

    def z_at(y):
        return depth * math.sin((y + W * 0.5) / pitch * math.pi * 2)

    for side in (0, 1):
        off = 0.0 if side == 0 else -thick
        grid = []
        for ix in range(nx + 1):
            x = -L * 0.5 + L * ix / nx
            row = []
            for iy in range(ny + 1):
                y = -W * 0.5 + W * iy / ny
                row.append(bm.verts.new((x, y, z_at(y) + off)))
            grid.append(row)
        for ix in range(nx):
            for iy in range(ny):
                q = [grid[ix][iy], grid[ix + 1][iy], grid[ix + 1][iy + 1], grid[ix][iy + 1]]
                if side == 1:
                    q = q[::-1]
                f = bm.faces.new(q)
                for loop in f.loops:
                    co = loop.vert.co
                    loop[uvl].uv = ((co.y + W * 0.5) / W, (co.x + L * 0.5) / L)
    bm.normal_update()
    export(new_obj("SM_Favela_RoofSheet", bm, "Metal"))


def make_weeds():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    w, h = 0.6, 0.5
    for k in range(3):
        a = k * math.pi / 3
        dx, dy = math.cos(a) * w * 0.5, math.sin(a) * w * 0.5
        add_quad(bm, uvl, [((-dx, -dy, 0), (0, 0)), ((dx, dy, 0), (1, 0)), ((dx, dy, h), (1, 1)), ((-dx, -dy, h), (0, 1))])
    bm.normal_update()
    export(new_obj("SM_Favela_Weeds", bm, "Foliage", smooth=False))


def make_ivy():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    cards = [(-0.45, 0.0, 0.0, 0.9, 2.0, 0.015), (0.35, 0.3, 0.0, 0.9, 1.6, 0.03), (0.0, 0.9, 0.0, 1.2, 1.1, 0.045), (-0.3, 1.2, 0.1, 0.7, 0.8, 0.06)]
    for cy, cz, _, cw, ch, cx in cards:
        add_quad(bm, uvl, [((cx, cy + cw * 0.5, cz), (0, 0)), ((cx, cy - cw * 0.5, cz), (1, 0)), ((cx, cy - cw * 0.5, cz + ch), (1, 1)), ((cx, cy + cw * 0.5, cz + ch), (0, 1))])
    bm.normal_update()
    export(new_obj("SM_Favela_Ivy", bm, "Foliage", smooth=False))


def make_banana():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    # stem (UV packed into the stem strip u 0.88..1.0, v 0..0.02 -> zero wind weight in Unreal)
    sides, stem_h = 8, 1.7
    bottom, top = [], []
    for s in range(sides):
        a = s / sides * math.pi * 2
        bottom.append(bm.verts.new((math.cos(a) * 0.09, math.sin(a) * 0.09, 0)))
        top.append(bm.verts.new((math.cos(a) * 0.06, math.sin(a) * 0.06, stem_h)))
    for s in range(sides):
        s2 = (s + 1) % sides
        f = bm.faces.new([bottom[s], bottom[s2], top[s2], top[s]])
        for loop in f.loops:
            loop[uvl].uv = (0.9 + 0.08 * (s / sides), 0.01)
    # leaves
    n_leaf, segs, L, W = 7, 10, 1.7, 0.5
    for k in range(n_leaf):
        yaw = k / n_leaf * math.pi * 2 + 0.3 * (k % 2)
        d = Vector((math.cos(yaw), math.sin(yaw), 0))
        side = Vector((-d.y, d.x, 0))
        rise = 0.9 + 0.25 * ((k * 37) % 5) / 5
        base = Vector((0, 0, stem_h - 0.05))
        rows = []
        for i in range(segs + 1):
            t = i / segs
            c = base + d * (t * L) + Vector((0, 0, (rise * t - 1.35 * t * t) * L * 0.6))
            hw = W * 0.5
            rows.append([
                (bm.verts.new(c - side * hw), (0.0, t)),
                (bm.verts.new(c + Vector((0, 0, 0.05))), (0.42, t)),
                (bm.verts.new(c + side * hw), (0.84, t)),
            ])
        for i in range(segs):
            for j in range(2):
                quad = [rows[i][j], rows[i][j + 1], rows[i + 1][j + 1], rows[i + 1][j]]
                f = bm.faces.new([q[0] for q in quad])
                for loop, q in zip(f.loops, quad):
                    loop[uvl].uv = q[1]
    bm.normal_update()
    export(new_obj("SM_Favela_Banana", bm, "Foliage", smooth=True))


def make_tire():
    reset()
    bpy.ops.mesh.primitive_torus_add(major_radius=0.30, minor_radius=0.11, major_segments=18, minor_segments=8, location=(0, 0, 0.11))
    obj = bpy.context.active_object
    obj.name = "SM_Favela_Tire"
    obj.scale = (1, 1, 0.85)
    bpy.ops.object.transform_apply(scale=True)
    bpy.ops.object.shade_smooth()
    obj.data.materials.append(bpy.data.materials.new("Rubber"))
    export(obj)


def make_tank():
    reset()
    parts = []
    bpy.ops.mesh.primitive_cylinder_add(radius=0.55, depth=0.85, vertices=24, location=(0, 0, 0.425))
    parts.append(bpy.context.active_object)
    for z in (0.18, 0.42, 0.66):
        bpy.ops.mesh.primitive_torus_add(major_radius=0.555, minor_radius=0.025, major_segments=24, minor_segments=6, location=(0, 0, z))
        parts.append(bpy.context.active_object)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.6, depth=0.06, vertices=24, location=(0, 0, 0.88))
    parts.append(bpy.context.active_object)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.5, segments=24, ring_count=8, location=(0, 0, 0.88))
    dome = bpy.context.active_object
    dome.scale = (1, 1, 0.22)
    parts.append(dome)
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.active_object
    obj.name = "SM_Favela_WaterTank"
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.ops.object.shade_smooth()
    obj.data.materials.clear()
    obj.data.materials.append(bpy.data.materials.new("Plastic"))
    # simple cylindrical UVs
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cylinder_project()
    bpy.ops.object.mode_set(mode="OBJECT")
    export(obj)


if __name__ == "__main__":
    print("=== FAVELA MESH KIT ->", OUT)
    make_cable()
    make_roof_sheet()
    make_weeds()
    make_ivy()
    make_banana()
    make_tire()
    make_tank()
    print("=== FAVELA MESH KIT DONE ===")
