#!/usr/bin/env python3
"""
===============================================================================
NOBLE FERMI: Coastal Operations - Procedural Dune Heightmap & Mesh Generator
===============================================================================
Generates geomorphologically accurate North Sea coastal dune heightfields (.r16)
and modular static mesh chunks (.obj):
  - SM_Dune_Ridge_30m: 30m wide x 20m deep continuous meandering ridge chunk
  - SM_Dune_Peak_20m:  20m x 20m barchanoid / pyramidal dune peak
  - SM_Dune_Transverse_40m: 40m x 30m transverse dune with interdune trough

Geomorphology Spec:
  - Windward Slope (Stoss Side): Gentle 10-15 degree slope facing the sea wind.
  - Leeward Slope (Slip Face):  Steep 30-34 degree angle-of-repose drop-off.
  - Crest: Sharp, undulating curvilinear ridge separating stoss from slip face.
  - Interdune Trough: Deflation flat with wind ripples.
===============================================================================
"""

import os
import sys
import math
import struct
import argparse
import numpy as np

def compute_dune_elevation(x_norm, y_norm, dune_type="ridge", peak_height_cm=450.0):
    """
    Computes elevation Z in centimeters for normalized coordinates x_norm in [0, 1], y_norm in [0, 1].
    Y represents the windward-to-leeward axis:
      - y < y_crest: Windward Stoss Slope (gentle 10-15 degrees)
      - y >= y_crest: Leeward Slip Face (steep 30-34 degrees angle of repose)
    """
    if dune_type == "ridge":
        # Meandering crest line across X
        crest_y = 0.58 + 0.08 * math.sin(x_norm * math.pi * 2.5) + 0.04 * math.sin(x_norm * math.pi * 5.0)
        crest_height = peak_height_cm * (0.88 + 0.12 * math.sin(x_norm * math.pi * 2.0))

        if y_norm < crest_y:
            # Stoss slope: convex-concave gentle ramp (10-14 degrees)
            t = y_norm / max(1e-4, crest_y)
            # Smooth cubic acceleration into the crest
            elevation = crest_height * (0.15 * t + 0.85 * (t ** 1.85))
            # Secondary wind ripples (period ~1.2m, amp ~3.5cm)
            ripple = 3.5 * math.sin(y_norm * 45.0 + x_norm * 6.0) * math.sin(t * math.pi)
            elevation += ripple
        else:
            # Slip face: rapid drop at 32-degree angle of repose down to interdune trough
            t = (y_norm - crest_y) / max(1e-4, 1.0 - crest_y)
            elevation = crest_height * ((1.0 - t) ** 1.45)
            # Interdune toe flattening
            if t > 0.85:
                toe_t = (t - 0.85) / 0.15
                elevation = max(10.0, elevation * (1.0 - toe_t * 0.4))

    elif dune_type == "peak":
        # Barchanoid crescent / peak: peak centered at (0.5, 0.6) with parabolic arms sweeping downwind
        cx, cy = 0.5, 0.62
        dx = (x_norm - cx) * 2.0
        crest_y = cy - 0.18 * (dx ** 2)
        crest_height = peak_height_cm * math.exp(-3.5 * (dx ** 2))

        if y_norm < crest_y:
            t = y_norm / max(1e-4, crest_y)
            elevation = crest_height * (0.2 * t + 0.8 * (t ** 2.1))
            ripple = 2.8 * math.sin(y_norm * 38.0) * t
            elevation += ripple
        else:
            t = (y_norm - crest_y) / max(1e-4, 1.0 - crest_y)
            elevation = crest_height * ((1.0 - t) ** 1.35)

    elif dune_type == "transverse":
        # 40m wide transverse dune chain with two ridges and a deep central interdune trough
        crest_y1 = 0.35 + 0.06 * math.sin(x_norm * math.pi * 3.0)
        crest_y2 = 0.82 + 0.05 * math.cos(x_norm * math.pi * 2.0)
        h1 = peak_height_cm * 0.85
        h2 = peak_height_cm * 1.05

        if y_norm < crest_y1:
            t = y_norm / max(1e-4, crest_y1)
            elevation = h1 * (t ** 1.7) + 3.0 * math.sin(y_norm * 50.0)
        elif y_norm < 0.55:
            # Slip face of ridge 1 into trough
            t = (y_norm - crest_y1) / (0.55 - crest_y1)
            elevation = h1 * ((1.0 - t) ** 1.5) + 35.0
        elif y_norm < crest_y2:
            # Stoss slope of ridge 2 rising out of trough
            t = (y_norm - 0.55) / (crest_y2 - 0.55)
            elevation = 35.0 + (h2 - 35.0) * (t ** 1.9) + 3.2 * math.sin(y_norm * 45.0)
        else:
            # Slip face of ridge 2
            t = (y_norm - crest_y2) / max(1e-4, 1.0 - crest_y2)
            elevation = h2 * ((1.0 - t) ** 1.4)
    else:
        elevation = 0.0

    return max(0.0, float(elevation))

def generate_mesh_obj(filename, width_m, depth_m, peak_height_m, res_x=61, res_y=61, dune_type="ridge"):
    """
    Generates a dense Wavefront OBJ file for a modular dune chunk with vertex positions (in cm),
    UV texture coordinates, and smoothed vertex normals.
    """
    width_cm = width_m * 100.0
    depth_cm = depth_m * 100.0
    peak_height_cm = peak_height_m * 100.0

    # Build heightfield grid
    verts = []
    uvs = []
    normals = []

    xs = np.linspace(-width_cm * 0.5, width_cm * 0.5, res_x)
    ys = np.linspace(-depth_cm * 0.5, depth_cm * 0.5, res_y)

    grid_z = np.zeros((res_y, res_x), dtype=np.float32)

    for iy, y in enumerate(ys):
        y_norm = iy / (res_y - 1)
        for ix, x in enumerate(xs):
            x_norm = ix / (res_x - 1)
            z = compute_dune_elevation(x_norm, y_norm, dune_type=dune_type, peak_height_cm=peak_height_cm)
            grid_z[iy, ix] = z

    # Compute smoothed analytical normals from grid gradients
    dx = width_cm / (res_x - 1)
    dy = depth_cm / (res_y - 1)
    grad_y, grad_x = np.gradient(grid_z, dy, dx)

    # Normal vector = normalize(-dz/dx, -dz/dy, 1.0)
    norm_x = -grad_x
    norm_y = -grad_y
    norm_z = np.ones_like(grid_z)
    lens = np.sqrt(norm_x**2 + norm_y**2 + norm_z**2)
    norm_x /= lens
    norm_y /= lens
    norm_z /= lens

    for iy in range(res_y):
        for ix in range(res_x):
            verts.append((xs[ix], ys[iy], grid_z[iy, ix]))
            uvs.append((ix / (res_x - 1), iy / (res_y - 1)))
            normals.append((norm_x[iy, ix], norm_y[iy, ix], norm_z[iy, ix]))

    # Generate quad faces as triangles
    faces = []
    for iy in range(res_y - 1):
        for ix in range(res_x - 1):
            # 1-indexed vertex indices
            v00 = iy * res_x + ix + 1
            v10 = iy * res_x + (ix + 1) + 1
            v01 = (iy + 1) * res_x + ix + 1
            v11 = (iy + 1) * res_x + (ix + 1) + 1

            # Triangle 1: (v00, v10, v11)
            faces.append((v00, v10, v11))
            # Triangle 2: (v00, v11, v01)
            faces.append((v00, v11, v01))

    # Write OBJ file
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, 'w', encoding='utf-8') as f:
        f.write("# NOBLE FERMI PROCEDURAL DUNE MESH\n")
        f.write(f"# Mesh: {os.path.basename(filename)}\n")
        f.write(f"# Dimensions: {width_m}m x {depth_m}m x {peak_height_m}m peak\n")
        f.write(f"# Vertices: {len(verts)}, Faces: {len(faces)}\n\n")

        # Vertices
        for v in verts:
            f.write(f"v {v[0]:.2f} {v[1]:.2f} {v[2]:.2f}\n")

        # Texture coordinates
        for uv in uvs:
            f.write(f"vt {uv[0]:.4f} {uv[1]:.4f}\n")

        # Normals
        for n in normals:
            f.write(f"vn {n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n")

        f.write(f"\ng {os.path.splitext(os.path.basename(filename))[0]}\n")
        f.write("usemtl M_Coastal_Sand\n")
        f.write("s 1\n")

        # Faces: f v/vt/vn
        for face in faces:
            f.write(f"f {face[0]}/{face[0]}/{face[0]} {face[1]}/{face[1]}/{face[1]} {face[2]}/{face[2]}/{face[2]}\n")

    print(f"[AssetGen] Exported OBJ mesh: {filename} ({len(verts)} verts, {len(faces)} tris)")

def generate_landscape_r16(filename, width_samples=505, height_samples=505, max_height_m=12.0):
    """
    Generates a 16-bit raw heightmap (.r16) for Unreal Engine 5 Landscape import.
    Samples a full 120m x 120m coastal backcountry dune terrain field with stoss/slip dynamics.
    Unreal 16-bit format: uint16 little-endian, midpoint 32768 = 0m datum.
    """
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    data = np.zeros((height_samples, width_samples), dtype=np.uint16)

    for iy in range(height_samples):
        y_norm = iy / (height_samples - 1)
        for ix in range(width_samples):
            x_norm = ix / (width_samples - 1)

            # Composite multi-ridge coastal dune field
            # Dominant dune ridge at Y ~ 0.35, secondary ridge at Y ~ 0.70
            z1 = compute_dune_elevation(x_norm, y_norm, dune_type="ridge", peak_height_cm=520.0)
            z2 = compute_dune_elevation((x_norm + 0.3) % 1.0, y_norm, dune_type="transverse", peak_height_cm=420.0)
            z_cm = max(z1, z2 * 0.8)

            # Map 0 to max_height_m * 100 cm into 0..32767 above datum
            # 32768 is datum (0 cm)
            height_ratio = min(1.0, z_cm / (max_height_m * 100.0))
            raw_val = int(32768 + height_ratio * 32000.0)
            data[iy, ix] = max(0, min(65535, raw_val))

    # Write little-endian raw binary
    with open(filename, 'wb') as f:
        data.astype('<u2').tofile(f)

    print(f"[AssetGen] Exported Unreal 16-bit raw heightmap: {filename} ({width_samples}x{height_samples})")

def main():
    parser = argparse.ArgumentParser(description="Noble Fermi Coastal Dune Asset Generator")
    parser.add_argument("--outdir", default="Content/Art/Environment/Dunes", help="Output directory for generated assets")
    args = parser.parse_args()

    outdir = args.outdir
    os.makedirs(outdir, exist_ok=True)

    print("====================================================================")
    print("NOBLE FERMI: Procedural Coastal Dune Generator (C++ / Unreal Engine 5.8)")
    print("====================================================================")

    # 1. SM_Dune_Ridge_30m: 30m wide x 20m deep x 4.5m peak
    ridge_path = os.path.join(outdir, "SM_Dune_Ridge_30m.obj")
    generate_mesh_obj(ridge_path, width_m=30.0, depth_m=20.0, peak_height_m=4.5, res_x=61, res_y=51, dune_type="ridge")

    # 2. SM_Dune_Peak_20m: 20m wide x 20m deep x 5.2m peak
    peak_path = os.path.join(outdir, "SM_Dune_Peak_20m.obj")
    generate_mesh_obj(peak_path, width_m=20.0, depth_m=20.0, peak_height_m=5.2, res_x=51, res_y=51, dune_type="peak")

    # 3. SM_Dune_Transverse_40m: 40m wide x 30m deep x 4.8m peak
    trans_path = os.path.join(outdir, "SM_Dune_Transverse_40m.obj")
    generate_mesh_obj(trans_path, width_m=40.0, depth_m=30.0, peak_height_m=4.8, res_x=71, res_y=61, dune_type="transverse")

    # 4. Full 120m x 120m Landscape Heightfield (.r16)
    r16_path = os.path.join(outdir, "H_Coastal_Dunes_120m.r16")
    generate_landscape_r16(r16_path, width_samples=505, height_samples=505, max_height_m=12.0)

    print("====================================================================")
    print("[AssetGen] SUCCESS: All coastal dune meshes and heightmaps generated.")
    print("====================================================================")

if __name__ == "__main__":
    main()
