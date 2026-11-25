"""Blender (headless): orient, scale and pivot each Trellis model for UE, drop floating fragments, export GLB.

  blender -b --factory-startup -P Scripts/Trellis/clean_assets.py -- <trellis_root> <out_root> [names...]

Reads <trellis_root>/<batch>/<category>/<Name>.glb (the sorted packs) and asset_specs.py; writes
<out_root>/<batch>/<category>/<Name>.glb at real-world scale with forward +X and up +Z in UE terms
(glTF +X forward, +Y up; UE imports glTF (x, y, z) as (x, z, y) * 100) and logs CLEAN lines.
"""
import glob
import json
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from asset_specs import REJECTED, S  # noqa: E402

args = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = args[0], args[1]
ONLY = set(args[2:])

FLOATER_AREA = 0.002   # a separate piece under 0.2% of the surface ...
FLOATER_SIZE = 0.04    # ... and under 4% of the model's size is debris


def gltf_to_blender(v):
    x, y, z = v
    return Vector((x, -z, y))


def axis_vec(s):
    """'+X' / '-Z' (glTF) -> Blender unit vector; '+X+Y' -> normalized sum."""
    out = Vector((0.0, 0.0, 0.0))
    for i in range(0, len(s), 2):
        sign = 1.0 if s[i] == "+" else -1.0
        out += gltf_to_blender({"X": (1, 0, 0), "Y": (0, 1, 0), "Z": (0, 0, 1)}[s[i + 1]]) * sign
    return out.normalized()


def principal_axes(points):
    centered = points - points.mean(axis=0)
    vals, vecs = np.linalg.eigh(np.cov(centered.T))
    return [Vector(vecs[:, i]) for i in np.argsort(vals)[::-1]]  # longest first


def resolve(rule, points):
    if rule.startswith("pca_h:"):
        hint = axis_vec(rule[6:])
        flat = points.copy()
        flat[:, 2] = 0.0
        v = principal_axes(flat)[0]
        v.z = 0.0
        v.normalize()
    elif rule.startswith("pca:"):
        hint = axis_vec(rule[4:])
        v = principal_axes(points)[0]
    elif rule.startswith("flat:"):
        hint = axis_vec(rule[5:])
        v = principal_axes(points)[2]
    else:
        return axis_vec(rule)
    return v if v.dot(hint) >= 0 else -v


def remove_floaters(me):
    """Delete loose pieces that are tiny next to the model. Vertices are welded by position first, since
    UV seams split them in the GLB."""
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.faces.ensure_lookup_table()
    co = np.array([v.co[:] for v in bm.verts])
    _, weld = np.unique(np.round(co / 1e-5).astype(np.int64), axis=0, return_inverse=True)
    weld = weld.ravel()
    parent = list(range(weld.max() + 1))

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    for f in bm.faces:
        ids = [find(weld[v.index]) for v in f.verts]
        for b in ids[1:]:
            parent[find(b)] = ids[0]
    comp_area, comp_faces, comp_lo, comp_hi = {}, {}, {}, {}
    for f in bm.faces:
        c = find(weld[f.verts[0].index])
        comp_area[c] = comp_area.get(c, 0.0) + f.calc_area()
        comp_faces.setdefault(c, []).append(f)
        pts = np.array([v.co[:] for v in f.verts])
        comp_lo[c] = np.minimum(comp_lo.get(c, pts.min(0)), pts.min(0))
        comp_hi[c] = np.maximum(comp_hi.get(c, pts.max(0)), pts.max(0))
    total = sum(comp_area.values())
    size = float(np.linalg.norm(co.max(0) - co.min(0)))
    doomed = [f for c, fs in comp_faces.items()
              if comp_area[c] < FLOATER_AREA * total and np.linalg.norm(comp_hi[c] - comp_lo[c]) < FLOATER_SIZE * size
              for f in fs]
    pieces = len(comp_faces)
    removed = len({find(weld[f.verts[0].index]) for f in doomed})
    if doomed:
        bmesh.ops.delete(bm, geom=doomed, context="FACES")
    bm.to_mesh(me)
    bm.free()
    return pieces, removed


def clean(name, src, dst, sp):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=src)
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    for o in meshes:  # bake node transforms into the mesh, drop the hierarchy
        o.data.transform(o.matrix_world)
        o.parent = None
        o.matrix_world = Matrix.Identity(4)
    for o in [o for o in bpy.context.scene.objects if o.type != "MESH"]:
        bpy.data.objects.remove(o)
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = obj.data.name = name
    me = obj.data

    pieces, removed = remove_floaters(me)

    points = np.array([v.co[:] for v in me.vertices])
    if sp["up"].startswith("flat:"):
        up = resolve(sp["up"], points)
        fwd = resolve(sp["fwd"], points)
        fwd = (fwd - up * fwd.dot(up)).normalized()
    else:
        fwd = resolve(sp["fwd"], points)
        up = resolve(sp["up"], points)
        up = (up - fwd * up.dot(fwd)).normalized()
    side = up.cross(fwd)  # Blender is right-handed: Y = Z x X
    me.transform(Matrix((fwd, side, up)).to_4x4())

    co = np.array([v.co[:] for v in me.vertices])
    lo, hi = co.min(0), co.max(0)
    dims = hi - lo
    axis, cm = sp["size"]
    measured = {"L": dims[0], "W": dims[1], "H": dims[2], "M": dims.max()}[axis]
    scale = (cm / 100.0) / measured
    me.transform(Matrix.Scale(scale, 4))
    lo, hi = lo * scale, hi * scale
    centre = (lo + hi) / 2
    pivot = centre if sp["kind"] == "held" else np.array([centre[0], centre[1], lo[2]])
    me.transform(Matrix.Translation(Vector(-pivot)))
    me.update()

    os.makedirs(os.path.dirname(dst), exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=dst, export_format="GLB", use_selection=False, export_yup=True,
                              export_apply=True, export_image_format="AUTO", export_animations=False)
    size_cm = (hi - lo) * 100
    print("CLEAN " + json.dumps({"name": name, "file": os.path.relpath(dst, OUT).replace("\\", "/"),
                                 "size_cm": [round(float(x), 1) for x in size_cm], "scale": round(scale, 5),
                                 "pieces": pieces, "floaters_removed": removed, "tris": len(me.polygons)}))


for name, sp in S.items():
    if ONLY and name not in ONLY:
        continue
    found = glob.glob(os.path.join(SRC, "0*", "*", f"{name}.glb"))
    if not found:
        print(f"CLEAN_MISSING {name}")
        continue
    src = found[0]
    rel = os.path.relpath(src, SRC)
    try:
        clean(name, src, os.path.join(OUT, rel), sp)
    except Exception as e:  # noqa: BLE001  keep going; report it
        print(f"CLEAN_FAILED {name}: {e}")
print(f"CLEAN_DONE {len(S)} specs, {len(REJECTED)} rejected")
