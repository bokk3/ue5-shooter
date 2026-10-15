import numpy as np
from PIL import Image
import os
import math

print("=== GENERATING HIGH-RESOLUTION AAA OPERATOR TEXTURES (2048x2048) ===")

out_dir = os.path.abspath("Content/Characters/BaseMale/Textures")
os.makedirs(out_dir, exist_ok=True)

RES = 2048

# -------------------------------------------------------------------------
# 1. T_Ripstop_Normal.png (2048x2048 Seamless Tangent Normal Map)
# -------------------------------------------------------------------------
print("1. Generating T_Ripstop_Normal (2048x2048 seamless tactical fabric weave)...")
grid_size = 64  # 32 grid squares across 2048
x = np.arange(RES, dtype=np.float32)
y = np.arange(RES, dtype=np.float32)
X, Y = np.meshgrid(x, y)

# Periodic distance to nearest grid line
dx = np.minimum(X % grid_size, grid_size - (X % grid_size))
dy = np.minimum(Y % grid_size, grid_size - (Y % grid_size))

# Raised grid rib lines (twin cross-hatch reinforcing threads)
sigma = 2.2
grid_rib_x = np.exp(-(dx ** 2) / (2.0 * (sigma ** 2)))
grid_rib_y = np.exp(-(dy ** 2) / (2.0 * (sigma ** 2)))
grid_h = np.maximum(grid_rib_x, grid_rib_y) * 0.55

# Double-thread micro ridge on the reinforcing grid line
sub_dx = np.minimum(np.abs(dx - 2.5), np.abs(dx + 2.5))
sub_dy = np.minimum(np.abs(dy - 2.5), np.abs(dy + 2.5))
grid_detail = (np.exp(-(sub_dx ** 2) / 2.0) + np.exp(-(sub_dy ** 2) / 2.0)) * 0.20
grid_h += grid_detail

# Interior twill fabric micro-weave (45 degree interlaced warp/weft yarns)
twill_period = 8.0  # 4-pixel micro threads
twill_phase1 = 2.0 * np.pi * (X + Y) / twill_period
twill_phase2 = 2.0 * np.pi * (X - Y) / twill_period
twill_h = (np.sin(twill_phase1) * np.cos(twill_phase2)) * 0.22

# Fine stippled yarn fiber porosity noise
np.random.seed(42)
micro_noise = np.random.normal(0.0, 0.035, (RES, RES)).astype(np.float32)

# Combined heightfield
H = grid_h + twill_h + micro_noise

# Compute periodic central difference gradients dH/dx and dH/dy
dH_dx = (np.roll(H, -1, axis=1) - np.roll(H, 1, axis=1)) * 0.5
dH_dy = (np.roll(H, -1, axis=0) - np.roll(H, 1, axis=0)) * 0.5

# Strength factor for normal tangent slope
normal_strength = 2.8
Nx = -dH_dx * normal_strength
Ny = -dH_dy * normal_strength
Nz = np.ones((RES, RES), dtype=np.float32)

# Normalize normal vector
length = np.sqrt(Nx**2 + Ny**2 + Nz**2)
Nx /= length
Ny /= length
Nz /= length

# Encode to 8-bit RGB normal map [0, 255]
norm_R = ((Nx * 0.5 + 0.5) * 255.0).clip(0, 255).astype(np.uint8)
norm_G = ((Ny * 0.5 + 0.5) * 255.0).clip(0, 255).astype(np.uint8)
norm_B = ((Nz * 0.5 + 0.5) * 255.0).clip(0, 255).astype(np.uint8)

normal_img = Image.fromarray(np.stack([norm_R, norm_G, norm_B], axis=-1))
normal_path = os.path.join(out_dir, "T_Ripstop_Normal.png")
normal_img.save(normal_path, "PNG", optimize=True)
print(f" -> Saved {normal_path} ({os.path.getsize(normal_path):,} bytes)")

# -------------------------------------------------------------------------
# 2. T_Ripstop_Roughness.png (2048x2048 Seamless PBR Roughness)
# -------------------------------------------------------------------------
print("2. Generating T_Ripstop_Roughness (2048x2048 seamless fabric roughness)...")
# Base cotton/nylon weave roughness: 0.80 (diffuse matte)
# Reinforced grid lines: 0.65 (tighter tension, slightly smoother sheen)
roughness_val = 0.80 - (grid_h * 0.22) + (twill_h * 0.08)
roughness_val += np.random.normal(0.0, 0.025, (RES, RES)).astype(np.float32)
roughness_uint8 = (roughness_val.clip(0.0, 1.0) * 255.0).astype(np.uint8)

roughness_img = Image.fromarray(roughness_uint8, mode='L')
roughness_path = os.path.join(out_dir, "T_Ripstop_Roughness.png")
roughness_img.save(roughness_path, "PNG", optimize=True)
print(f" -> Saved {roughness_path} ({os.path.getsize(roughness_path):,} bytes)")

# -------------------------------------------------------------------------
# 3. T_Camo_Digital.png (2048x2048 Multi-Tier Seamless Tactical Camouflage)
# -------------------------------------------------------------------------
print("3. Generating T_Camo_Digital (2048x2048 multi-tier digital pixel pattern)...")
# Tier 1: Macro blotches (128x128 blocks -> 16x16 grid)
macro_grid = np.random.RandomState(101).rand(16, 16)
macro_up = np.repeat(np.repeat(macro_grid, 128, axis=0), 128, axis=1)

# Tier 2: Medium pixel clusters (32x32 blocks -> 64x64 grid)
med_grid = np.random.RandomState(202).rand(64, 64)
med_up = np.repeat(np.repeat(med_grid, 32, axis=0), 32, axis=1)

# Tier 3: Micro dither pixels (8x8 blocks -> 256x256 grid)
micro_grid = np.random.RandomState(303).rand(256, 256)
micro_up = np.repeat(np.repeat(micro_grid, 8, axis=0), 8, axis=1)

# Channel R: Primary blotches & macro digital clusters
camo_R = ((macro_up > 0.48) & (med_up > 0.35)).astype(np.float32)

# Channel G: Secondary mid-frequency dithered digital pixels
camo_G = ((macro_up <= 0.48) & (med_up > 0.50) | (micro_up > 0.72)).astype(np.float32)

# Channel B: Tertiary accent / dark shadow spots
camo_B = ((macro_up > 0.70) & (med_up > 0.65) | (micro_up > 0.88)).astype(np.float32)

# Channel A: Inverted base mask
camo_A = (1.0 - np.maximum(camo_R, np.maximum(camo_G, camo_B))).clip(0.0, 1.0)

camo_R_u8 = (camo_R * 255.0).astype(np.uint8)
camo_G_u8 = (camo_G * 255.0).astype(np.uint8)
camo_B_u8 = (camo_B * 255.0).astype(np.uint8)
camo_A_u8 = (camo_A * 255.0).astype(np.uint8)

camo_img = Image.fromarray(np.stack([camo_R_u8, camo_G_u8, camo_B_u8, camo_A_u8], axis=-1), mode='RGBA')
camo_path = os.path.join(out_dir, "T_Camo_Digital.png")
camo_img.save(camo_path, "PNG", optimize=True)
print(f" -> Saved {camo_path} ({os.path.getsize(camo_path):,} bytes)")

print("=== TEXTURE GENERATION COMPLETE ===")
