"""
NOBLE FERMI // FAVELA MESH KIT GENERATOR (Blender 4.2 headless)
Run: blender.exe -b --python Scripts/Favela/generate_favela_meshes.py

Exports to SourceArt/Favela/Meshes/*.fbx (1 Blender metre == 100 Unreal cm):
  SM_Favela_Cable          Slender realistic aerial wire (r = 3.5 mm). UV.x = span position.
  SM_Favela_RoofSheet      2.4 m x 1.1 m corrugated sine-profile zinc sheet.
  SM_Favela_Weeds          Organic tufted tropical weeds / gutter grass clumps.
  SM_Favela_Ivy            Wall-hugging ivy card cluster facing +X.
  SM_Favela_Banana         Botanical-grade Musa acuminata: tapered stem, arching ribbed leaves, spear leaf.
  SM_Favela_PalmFan        Brazilian Royal Fan Palm: ringed trunk, 12 radiating palmate fronds.
  SM_Favela_Tire           Discarded car tyre (roof hold-down weight).
  SM_Favela_WaterTank      1000 L polyethylene water tank with lid & PVC outlet connector.
  SM_Favela_WindowAC       Wall-mounted window air conditioner (7500 BTU) with rusted bracket.
  SM_Favela_SatelliteDish  Rooftop 70cm parabolic satellite TV dish with LNB feed horn.
  SM_Favela_RebarColumn    Roof pillar stub with 4 vertical rebar rods & safety PET bottles.
  SM_Favela_TrashBags      Curbside cluster of black plastic rubbish bags.
"""
import bpy
import bmesh
import math
import os
from mathutils import Vector, Matrix

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
# 1. SM_Favela_Cable: Slender realistic wire (radius = 3.5 mm instead of bulky 11 mm)
# -----------------------------------------------------------------------------------------
def make_cable():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    segs, sides, r = 32, 6, 0.0035  # 7 mm realistic aerial wire outer diameter
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


# -----------------------------------------------------------------------------------------
# 2. SM_Favela_RoofSheet: Corrugated zinc sheet
# -----------------------------------------------------------------------------------------
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


# -----------------------------------------------------------------------------------------
# 3. SM_Favela_Weeds: Multi-directional curved grass tufts
# -----------------------------------------------------------------------------------------
def make_weeds():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    num_blades = 8
    for b in range(num_blades):
        angle = b * (math.pi * 2 / num_blades) + (b % 3) * 0.2
        radius = 0.12 + 0.10 * ((b * 7) % 5) / 5.0
        h = 0.35 + 0.25 * ((b * 13) % 7) / 7.0
        w = 0.08
        
        ca, sa = math.cos(angle), math.sin(angle)
        perp_x, perp_y = -sa * w * 0.5, ca * w * 0.5
        
        # 3 vertical segments curling outward
        p0_l = (ca * 0.02 - perp_x, sa * 0.02 - perp_y, 0.0)
        p0_r = (ca * 0.02 + perp_x, sa * 0.02 + perp_y, 0.0)
        
        p1_l = (ca * radius * 0.5 - perp_x * 0.8, sa * radius * 0.5 - perp_y * 0.8, h * 0.55)
        p1_r = (ca * radius * 0.5 + perp_x * 0.8, sa * radius * 0.5 + perp_y * 0.8, h * 0.55)
        
        p2_tip = (ca * radius * 1.15, sa * radius * 1.15, h * 0.92)
        
        # Lower quad
        v0l = bm.verts.new(p0_l)
        v0r = bm.verts.new(p0_r)
        v1l = bm.verts.new(p1_l)
        v1r = bm.verts.new(p1_r)
        f1 = bm.faces.new([v0l, v0r, v1r, v1l])
        f1.loops[0][uvl].uv = (0.1, 0.0)
        f1.loops[1][uvl].uv = (0.9, 0.0)
        f1.loops[2][uvl].uv = (0.8, 0.6)
        f1.loops[3][uvl].uv = (0.2, 0.6)
        
        # Upper triangle tip
        v_tip = bm.verts.new(p2_tip)
        f2 = bm.faces.new([v1l, v1r, v_tip])
        f2.loops[0][uvl].uv = (0.2, 0.6)
        f2.loops[1][uvl].uv = (0.8, 0.6)
        f2.loops[2][uvl].uv = (0.5, 1.0)

    bm.normal_update()
    export(new_obj("SM_Favela_Weeds", bm, "Foliage", smooth=True))


# -----------------------------------------------------------------------------------------
# 4. SM_Favela_Ivy: Wall-hugging foliage cards
# -----------------------------------------------------------------------------------------
def make_ivy():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    cards = [(-0.45, 0.0, 0.0, 0.9, 2.0, 0.015), (0.35, 0.3, 0.0, 0.9, 1.6, 0.03), (0.0, 0.9, 0.0, 1.2, 1.1, 0.045), (-0.3, 1.2, 0.1, 0.7, 0.8, 0.06)]
    for cy, cz, _, cw, ch, cx in cards:
        add_quad(bm, uvl, [((cx, cy + cw * 0.5, cz), (0, 0)), ((cx, cy - cw * 0.5, cz), (1, 0)), ((cx, cy - cw * 0.5, cz + ch), (1, 1)), ((cx, cy + cw * 0.5, cz + ch), (0, 1))])
    bm.normal_update()
    export(new_obj("SM_Favela_Ivy", bm, "Foliage", smooth=False))


# -----------------------------------------------------------------------------------------
# 5. SM_Favela_Banana: Botanical-Grade Musa acuminata (arch, midrib, V-groove, wind-tears)
# -----------------------------------------------------------------------------------------
def make_banana():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    
    # Pseudostem: tapered trunk with flared base and natural lean
    stem_h = 1.95
    sides = 10
    n_stem_rings = 8
    stem_rings = []
    
    for r_idx in range(n_stem_rings):
        t = r_idx / (n_stem_rings - 1)
        z = t * stem_h
        # Flared base, tapering upwards
        rad = 0.13 * (1.0 - t * 0.52) + (0.04 if t < 0.1 else 0.0)
        # Subtle organic curve in the trunk
        ox = math.sin(t * 1.5) * 0.04
        oy = math.cos(t * 1.2) * 0.03
        
        ring = []
        for s in range(sides):
            a = s / sides * math.pi * 2
            ring.append(bm.verts.new((ox + math.cos(a) * rad, oy + math.sin(a) * rad, z)))
        stem_rings.append(ring)
        
    for r_idx in range(n_stem_rings - 1):
        for s in range(sides):
            s2 = (s + 1) % sides
            f = bm.faces.new([stem_rings[r_idx][s], stem_rings[r_idx][s2], stem_rings[r_idx + 1][s2], stem_rings[r_idx + 1][s]])
            f.loops[0][uvl].uv = (0.92, r_idx / (n_stem_rings - 1))
            f.loops[1][uvl].uv = (0.98, r_idx / (n_stem_rings - 1))
            f.loops[2][uvl].uv = (0.98, (r_idx + 1) / (n_stem_rings - 1))
            f.loops[3][uvl].uv = (0.92, (r_idx + 1) / (n_stem_rings - 1))
            
    # Central emerging spear leaf (pointing straight up)
    spear_h = 1.1
    spear_base = Vector((0.03, 0.02, stem_h))
    spear_tip = spear_base + Vector((0.02, 0.01, spear_h))
    v_sb_l = bm.verts.new(spear_base + Vector((-0.03, 0, 0)))
    v_sb_r = bm.verts.new(spear_base + Vector((0.03, 0, 0)))
    v_st = bm.verts.new(spear_tip)
    f_spear = bm.faces.new([v_sb_l, v_sb_r, v_st])
    f_spear.loops[0][uvl].uv = (0.4, 0.0)
    f_spear.loops[1][uvl].uv = (0.6, 0.0)
    f_spear.loops[2][uvl].uv = (0.5, 1.0)
    
    # 10 Gracefully arching fronds at golden-ratio spiral angles
    n_leaf = 10
    segs = 14  # segments along frond length for smooth parabolic drooping
    golden_angle = 2.399963  # ~137.5 degrees
    
    for k in range(n_leaf):
        yaw = k * golden_angle + 0.15 * (k % 3)
        dir_xy = Vector((math.cos(yaw), math.sin(yaw), 0.0))
        side_xy = Vector((-dir_xy.y, dir_xy.x, 0.0))
        
        # Vary length, arch height, and droop per leaf
        is_lower = (k < 3)
        L = 2.15 + 0.35 * ((k * 17) % 5) / 5.0
        W = 0.52 + 0.08 * ((k * 11) % 4) / 4.0
        arch_peak = 0.65 if not is_lower else 0.25
        droop_fall = 1.35 if not is_lower else 1.85
        
        base_pt = Vector((0.03, 0.02, stem_h - (0.15 if is_lower else 0.05)))
        
        # Generate 5-point cross-sections along frond (Left edge, Left blade, Midrib, Right blade, Right edge)
        # This produces realistic V-grooved leaf blades that cup downwards!
        sections = []
        for i in range(segs + 1):
            t = i / segs
            # Parabolic trajectory: rises up, levels out, curves downward towards ground
            z_offset = (arch_peak * t - droop_fall * (t ** 2.2)) * L * 0.7
            center_pt = base_pt + dir_xy * (t * L) + Vector((0, 0, z_offset))
            
            # Leaf width envelope: narrow base, wide middle, tapering point
            cur_w = W * math.sin(t * math.pi) ** 0.75 if t > 0.0 else 0.03
            # Blade droop: outer edges droop below the central midrib
            blade_curl = -cur_w * 0.22 * math.sin(t * math.pi * 0.8)
            
            v_l_edge = bm.verts.new(center_pt - side_xy * cur_w + Vector((0, 0, blade_curl)))
            v_l_mid  = bm.verts.new(center_pt - side_xy * (cur_w * 0.45) + Vector((0, 0, blade_curl * 0.3)))
            v_midrib = bm.verts.new(center_pt + Vector((0, 0, 0.015)))
            v_r_mid  = bm.verts.new(center_pt + side_xy * (cur_w * 0.45) + Vector((0, 0, blade_curl * 0.3)))
            v_r_edge = bm.verts.new(center_pt + side_xy * cur_w + Vector((0, 0, blade_curl)))
            
            sections.append([v_l_edge, v_l_mid, v_midrib, v_r_mid, v_r_edge])
            
        # Connect sections with quads
        for i in range(segs):
            t0 = i / segs
            t1 = (i + 1) / segs
            for j in range(4):
                v00 = sections[i][j]
                v01 = sections[i][j + 1]
                v11 = sections[i + 1][j + 1]
                v10 = sections[i + 1][j]
                
                f = bm.faces.new([v00, v01, v11, v10])
                u0 = j / 4.0 * 0.84
                u1 = (j + 1) / 4.0 * 0.84
                f.loops[0][uvl].uv = (u0, t0)
                f.loops[1][uvl].uv = (u1, t0)
                f.loops[2][uvl].uv = (u1, t1)
                f.loops[3][uvl].uv = (u0, t1)

    bm.normal_update()
    export(new_obj("SM_Favela_Banana", bm, "Foliage", smooth=True))


# -----------------------------------------------------------------------------------------
# 6. SM_Favela_PalmFan: Brazilian Royal Fan Palm (ringed trunk + 12 palmate fan fronds)
# -----------------------------------------------------------------------------------------
def make_fan_palm():
    reset()
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    
    # Ringed palm trunk
    trunk_h = 3.6
    n_rings = 14
    sides = 10
    trunk_rings = []
    for r_idx in range(n_rings):
        t = r_idx / (n_rings - 1)
        z = t * trunk_h
        # Slight ring bulge every alternate segment
        bulge = 0.015 if (r_idx % 2 == 1) else 0.0
        rad = (0.16 * (1.0 - t * 0.42)) + bulge
        ring = []
        for s in range(sides):
            a = s / sides * math.pi * 2
            ring.append(bm.verts.new((math.cos(a) * rad, math.sin(a) * rad, z)))
        trunk_rings.append(ring)
        
    for r_idx in range(n_rings - 1):
        for s in range(sides):
            s2 = (s + 1) % sides
            f = bm.faces.new([trunk_rings[r_idx][s], trunk_rings[r_idx][s2], trunk_rings[r_idx + 1][s2], trunk_rings[r_idx + 1][s]])
            f.loops[0][uvl].uv = (0.0, r_idx / (n_rings - 1))
            f.loops[1][uvl].uv = (1.0, r_idx / (n_rings - 1))
            f.loops[2][uvl].uv = (1.0, (r_idx + 1) / (n_rings - 1))
            f.loops[3][uvl].uv = (0.0, (r_idx + 1) / (n_rings - 1))

    # 12 Palmate Fan Fronds
    n_fans = 12
    for f_idx in range(n_fans):
        yaw = f_idx / n_fans * math.pi * 2 + 0.1 * (f_idx % 2)
        tilt = 0.35 + 0.30 * ((f_idx * 5) % 4) / 4.0  # outward pitch
        stem_len = 1.1
        
        d_xy = Vector((math.cos(yaw), math.sin(yaw), 0))
        stem_base = Vector((0, 0, trunk_h - 0.1))
        stem_end = stem_base + d_xy * (stem_len * math.cos(tilt)) + Vector((0, 0, stem_len * math.sin(tilt) * 0.7))
        
        # Petiole (stem)
        v_pb1 = bm.verts.new(stem_base + Vector((-0.03, -0.03, 0)))
        v_pb2 = bm.verts.new(stem_base + Vector((0.03, 0.03, 0)))
        v_pe1 = bm.verts.new(stem_end + Vector((-0.02, -0.02, 0)))
        v_pe2 = bm.verts.new(stem_end + Vector((0.02, 0.02, 0)))
        f_pet = bm.faces.new([v_pb1, v_pb2, v_pe2, v_pe1])
        f_pet.loops[0][uvl].uv = (0.85, 0.0)
        f_pet.loops[1][uvl].uv = (0.95, 0.0)
        f_pet.loops[2][uvl].uv = (0.95, 0.5)
        f_pet.loops[3][uvl].uv = (0.85, 0.5)
        
        # Fan blade: 7 radiating leaf blades
        n_blades = 7
        fan_radius = 0.95
        spread_ang = 1.4  # radians width
        side_xy = Vector((-d_xy.y, d_xy.x, 0))
        
        blade_tips = []
        for b_idx in range(n_blades):
            b_alpha = (b_idx / (n_blades - 1) - 0.5) * spread_ang
            # Rotate d_xy by b_alpha
            b_dir = d_xy * math.cos(b_alpha) + side_xy * math.sin(b_alpha)
            # Tip droops under gravity
            tip_pos = stem_end + b_dir * fan_radius + Vector((0, 0, -0.25 * math.cos(b_alpha)))
            blade_tips.append(bm.verts.new(tip_pos))
            
        v_fan_hub = bm.verts.new(stem_end)
        for b_idx in range(n_blades - 1):
            f_tri = bm.faces.new([v_fan_hub, blade_tips[b_idx], blade_tips[b_idx + 1]])
            f_tri.loops[0][uvl].uv = (0.5, 0.4)
            f_tri.loops[1][uvl].uv = (b_idx / (n_blades - 1), 1.0)
            f_tri.loops[2][uvl].uv = ((b_idx + 1) / (n_blades - 1), 1.0)

    bm.normal_update()
    export(new_obj("SM_Favela_PalmFan", bm, "Foliage", smooth=True))


# -----------------------------------------------------------------------------------------
# 7. SM_Favela_Tire: Discarded roof tyre
# -----------------------------------------------------------------------------------------
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


# -----------------------------------------------------------------------------------------
# 8. SM_Favela_WaterTank: Polyethylene caixa d'água with lid & PVC connector
# -----------------------------------------------------------------------------------------
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
    
    # White PVC outlet pipe fitting at base
    bpy.ops.mesh.primitive_cylinder_add(radius=0.035, depth=0.22, vertices=12, location=(0.58, 0, 0.12))
    pipe = bpy.context.active_object
    pipe.rotation_euler = (0, math.pi * 0.5, 0)
    parts.append(pipe)
    
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
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cylinder_project()
    bpy.ops.object.mode_set(mode="OBJECT")
    export(obj)


# -----------------------------------------------------------------------------------------
# 9. SM_Favela_WindowAC: Wall-mounted window air conditioner with mounting bracket
# -----------------------------------------------------------------------------------------
def make_window_ac():
    reset()
    parts = []
    # Main AC body cabinet (65 cm wide, 45 cm deep, 40 cm tall)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0.20))
    body = bpy.context.active_object
    body.scale = (0.45, 0.65, 0.40)
    parts.append(body)
    
    # Front plastic grill frame
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.24, 0, 0.20))
    grill = bpy.context.active_object
    grill.scale = (0.04, 0.62, 0.36)
    parts.append(grill)
    
    # Metal angle-iron wall bracket underneath
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(-0.05, -0.28, -0.05))
    b1 = bpy.context.active_object
    b1.scale = (0.40, 0.04, 0.10)
    parts.append(b1)
    
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(-0.05, 0.28, -0.05))
    b2 = bpy.context.active_object
    b2.scale = (0.40, 0.04, 0.10)
    parts.append(b2)
    
    # Clear condensation drip pipe
    bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=0.35, vertices=8, location=(-0.20, 0.18, -0.15))
    pipe = bpy.context.active_object
    parts.append(pipe)
    
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.active_object
    obj.name = "SM_Favela_WindowAC"
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    obj.data.materials.append(bpy.data.materials.new("Metal"))
    export(obj)


# -----------------------------------------------------------------------------------------
# 10. SM_Favela_SatelliteDish: Rooftop parabolic dish antenna with LNB feed horn
# -----------------------------------------------------------------------------------------
def make_satellite_dish():
    reset()
    parts = []
    # Parabolic bowl: half sphere squashed
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.36, segments=20, ring_count=10, location=(0, 0, 0.55))
    dish = bpy.context.active_object
    dish.scale = (0.35, 1.0, 1.0)
    dish.rotation_euler = (0, -0.45, 0)  # Aimed up at equatorial orbital arc
    parts.append(dish)
    
    # Mounting bracket pole
    bpy.ops.mesh.primitive_cylinder_add(radius=0.022, depth=0.60, vertices=10, location=(-0.15, 0, 0.30))
    pole = bpy.context.active_object
    parts.append(pole)
    
    # Feed arm & LNB receiver horn
    bpy.ops.mesh.primitive_cylinder_add(radius=0.012, depth=0.45, vertices=8, location=(0.18, 0, 0.45))
    arm = bpy.context.active_object
    arm.rotation_euler = (0, 0.95, 0)
    parts.append(arm)
    
    bpy.ops.mesh.primitive_cylinder_add(radius=0.038, depth=0.08, vertices=12, location=(0.32, 0, 0.62))
    lnb = bpy.context.active_object
    parts.append(lnb)
    
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.active_object
    obj.name = "SM_Favela_SatelliteDish"
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    obj.data.materials.append(bpy.data.materials.new("Metal"))
    export(obj)


# -----------------------------------------------------------------------------------------
# 11. SM_Favela_RebarColumn: Pillar stub with 4 rusted rebar spires & safety PET bottles
# -----------------------------------------------------------------------------------------
def make_rebar_column():
    reset()
    parts = []
    # Concrete pillar top stub (26 x 26 x 35 cm)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0.175))
    stub = bpy.context.active_object
    stub.scale = (0.26, 0.26, 0.35)
    parts.append(stub)
    
    # 4 Deformed vertical rusted rebar rods
    offsets = [(-0.07, -0.07), (0.07, -0.07), (-0.07, 0.07), (0.07, 0.07)]
    for idx, (ox, oy) in enumerate(offsets):
        rebar_h = 1.15 + 0.25 * ((idx * 7) % 3)
        bpy.ops.mesh.primitive_cylinder_add(radius=0.009, depth=rebar_h, vertices=8, location=(ox, oy, 0.35 + rebar_h * 0.5))
        rod = bpy.context.active_object
        rod.rotation_euler = (0.04 * (idx - 1.5), 0.03 * (idx % 2 - 0.5), 0)
        parts.append(rod)
        
        # Plastic safety bottle / beer can capped over top of 2 rods
        if idx in (0, 3):
            bpy.ops.mesh.primitive_cylinder_add(radius=0.038, depth=0.18, vertices=10, location=(ox, oy, 0.35 + rebar_h - 0.05))
            bottle = bpy.context.active_object
            parts.append(bottle)
            
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.active_object
    obj.name = "SM_Favela_RebarColumn"
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    obj.data.materials.append(bpy.data.materials.new("Concrete"))
    export(obj)


# -----------------------------------------------------------------------------------------
# 12. SM_Favela_TrashBags: Curbside cluster of black rubbish bags
# -----------------------------------------------------------------------------------------
def make_trash_bags():
    reset()
    parts = []
    bag_locs = [(0, 0, 0.20, 0.26), (-0.18, 0.12, 0.16, 0.22), (0.16, -0.08, 0.15, 0.21)]
    for bx, by, bz, brad in bag_locs:
        bpy.ops.mesh.primitive_uv_sphere_add(radius=brad, segments=14, ring_count=8, location=(bx, by, bz))
        b = bpy.context.active_object
        b.scale = (1.1, 0.95, 0.78)  # Squashed bag resting on ground
        parts.append(b)
        
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.active_object
    obj.name = "SM_Favela_TrashBags"
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.ops.object.shade_smooth()
    obj.data.materials.append(bpy.data.materials.new("Plastic"))
    export(obj)


if __name__ == "__main__":
    print("=== NOBLE FERMI // FAVELA MESH REALISM KIT ===")
    make_cable()
    make_roof_sheet()
    make_weeds()
    make_ivy()
    make_banana()
    make_fan_palm()
    make_tire()
    make_tank()
    make_window_ac()
    make_satellite_dish()
    make_rebar_column()
    make_trash_bags()
    print("=== FAVELA MESH REALISM KIT COMPLETED SUCCESSFULLY ===")
