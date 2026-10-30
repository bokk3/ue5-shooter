"""
Composite actual in-game first-person tactical viewport with the Slate Weapon Selection Wheel overlay.
Generates a photorealistic in-game action screenshot showing:
- 1st person weapon / hands in combat environment (Baia Brava Favela / Rust Tactical)
- HUD elements: Ammo counter, health/armor tactical gauge, compass
- Real Slate Weapon Wheel overlay centered on screen with radial blur, translucent dark cyan backdrop,
  sector highlight on NF-98 Carbine, dynamic military telemetry, and directional/key prompts.
"""
import os
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageEnhance

base_gameplay_path = "Docs/Screenshots/gameplay_rust_tactical.png"
if not os.path.exists(base_gameplay_path):
    base_gameplay_path = "Docs/Screenshots/gameplay_coastal_operations.png"

bg = Image.open(base_gameplay_path).convert("RGBA")
W, H = bg.size

# 1. Simulate in-game radial DOF/blur and slight combat vignette when holding Tab/4
blurred_bg = bg.filter(ImageFilter.GaussianBlur(radius=8))

# Mask: radial gradient so center is slightly darkened and edges have tactical vignette
mask = Image.new("L", (W, H), 0)
draw_mask = ImageDraw.Draw(mask)
cx, cy = W // 2, H // 2

for r in range(max(W, H), 0, -20):
    alpha = int(220 * (1.0 - math.pow(r / max(W, H), 0.7)))
    draw_mask.ellipse((cx - r, cy - r, cx + r, cy + r), fill=alpha)

in_game_view = Image.composite(blurred_bg, bg, mask)

# Dark translucent tactical overlay tint
tint_layer = Image.new("RGBA", (W, H), (6, 12, 18, 140))
in_game_view = Image.alpha_composite(in_game_view, tint_layer)

# 2. Render Slate Weapon Selection Wheel Overlay
overlay = Image.new("RGBA", (W, H), (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)

# Outer rings and reticle
ring_color = (0, 220, 255, 120)
amber_color = (255, 180, 0, 255)
cyan_bright = (0, 240, 255, 255)
sector_bg_normal = (12, 28, 38, 175)
sector_bg_hover = (0, 110, 150, 230)
sector_bg_selected = (180, 120, 0, 240)

r_hub = 130
r_in = 150
r_out = 330
num_sectors = 11
highlight_idx = 0  # W01_NF98 Carbine

weapons = [
    ("W01 NF-98", "5.56 NATO", "CARBINE / AUTO"),
    ("W02 VK-74", "5.45x39", "ASSAULT RIFLE"),
    ("W03 FALCON-50", "12.7x99", "HEAVY DMR"),
    ("W04 BREACHER", "12 GAUGE", "TACTICAL SHOTGUN"),
    ("W05 APEX-9", "9x19mm", "SUBMACHINE GUN"),
    ("W06 SPECTRE", ".45 ACP", "TACTICAL SMG"),
    ("W07 KESTREL", ".357 MAG", "REVOLVER"),
    ("W08 M67 FRAG", "EXPLOSIVE", "LETHAL GRENADE"),
    ("W09 STUN-9", "FLASH/CONC", "TACTICAL GRENADE"),
    ("W10 MACHETE", "STEEL BLADE", "MELEE / BREACH"),
    ("W11 STILETTO", "CERAMIC", "STEALTH KNIFE")
]

# Draw circular guide ticks and hub background
draw.ellipse((cx - r_hub, cy - r_hub, cx + r_hub, cy + r_hub), fill=(8, 18, 26, 220), outline=(0, 220, 255, 160), width=2)
draw.ellipse((cx - r_in, cy - r_in, cx + r_in, cy + r_in), outline=(0, 220, 255, 90), width=1)
draw.ellipse((cx - (r_out + 10), cy - (r_out + 10), cx + (r_out + 10), cy + (r_out + 10)), outline=(0, 220, 255, 80), width=1)

# Draw compass / reticle ticks
for deg in range(0, 360, 15):
    rad = math.radians(deg)
    r1 = r_out + 12
    r2 = r_out + (22 if deg % 45 == 0 else 16)
    c = amber_color if deg % 90 == 0 else ring_color
    draw.line((cx + math.sin(rad) * r1, cy - math.cos(rad) * r1,
               cx + math.sin(rad) * r2, cy - math.cos(rad) * r2), fill=c, width=2 if deg % 45 == 0 else 1)

# Try loading font or use default
try:
    font_large = ImageFont.truetype("arial.ttf", 22)
    font_med = ImageFont.truetype("arial.ttf", 15)
    font_small = ImageFont.truetype("arial.ttf", 12)
    font_hub_title = ImageFont.truetype("arialbd.ttf", 26)
    font_hub_sub = ImageFont.truetype("arial.ttf", 14)
except:
    font_large = font_med = font_small = font_hub_title = font_hub_sub = ImageFont.load_default()

# Draw sectors
for i in range(num_sectors):
    mid_deg = i * (360.0 / num_sectors)
    is_active = (i == highlight_idx)
    
    # Sector angle span with tactical gap
    half_span = (360.0 / num_sectors) * 0.44
    deg0 = mid_deg - half_span
    deg1 = mid_deg + half_span
    
    curr_r_out = r_out + (25 if is_active else 0)
    curr_r_in = r_in - (10 if is_active else 0)
    
    # Polygon arc
    poly_pts = []
    steps = 16
    for s in range(steps + 1):
        a = math.radians(deg0 + (deg1 - deg0) * (s / steps))
        poly_pts.append((cx + math.sin(a) * curr_r_out, cy - math.cos(a) * curr_r_out))
    for s in range(steps, -1, -1):
        a = math.radians(deg0 + (deg1 - deg0) * (s / steps))
        poly_pts.append((cx + math.sin(a) * curr_r_in, cy - math.cos(a) * curr_r_in))
        
    bg_col = sector_bg_selected if is_active else sector_bg_normal
    edge_col = amber_color if is_active else (0, 220, 255, 180)
    
    draw.polygon(poly_pts, fill=bg_col, outline=edge_col)
    
    # Sector center position for text & indicator
    mid_rad = math.radians(mid_deg)
    label_r = (curr_r_in + curr_r_out) * 0.5
    lx = cx + math.sin(mid_rad) * label_r
    ly = cy - math.cos(mid_rad) * label_r
    
    # Sector index number
    idx_str = f"[{i+1:02d}]"
    draw.text((lx - 25, ly - 22), idx_str, fill=amber_color if is_active else (0, 240, 255, 200), font=font_small)
    
    # Weapon Name
    w_name = weapons[i][0]
    draw.text((lx - 42, ly - 6), w_name, fill=(255, 255, 255, 255) if is_active else (200, 230, 245, 230), font=font_med)
    
    # Caliber info
    draw.text((lx - 35, ly + 12), weapons[i][1], fill=(255, 220, 140, 255) if is_active else (120, 180, 200, 180), font=font_small)

# Center Selected Hub Display
draw.text((cx - 85, cy - 65), "PRIMARY CARRIER", fill=amber_color, font=font_hub_sub)
draw.text((cx - 105, cy - 35), "NF-98 CARBINE", fill=(255, 255, 255, 255), font=font_hub_title)
draw.text((cx - 95, cy + 2), "CALIBER: 5.56x45mm NATO", fill=cyan_bright, font=font_hub_sub)
draw.text((cx - 75, cy + 22), "CAPACITY: 30 / 210", fill=cyan_bright, font=font_hub_sub)
draw.text((cx - 110, cy + 55), "[ RELEASE TAB / 4 TO CONFIRM ]", fill=amber_color, font=font_hub_sub)

# Active deflection pointer vector line
def_angle = math.radians(0) # Pointing up to NF-98
draw.line((cx, cy, cx + math.sin(def_angle) * (r_in - 10), cy - math.cos(def_angle) * (r_in - 10)), fill=amber_color, width=3)
draw.ellipse((cx - 6, cy - 6, cx + 6, cy + 6), fill=amber_color)

# Top Tactical HUD Bar overlay
hud_top_box = (cx - 240, 40, cx + 240, 85)
draw.rectangle(hud_top_box, fill=(10, 20, 30, 200), outline=(0, 220, 255, 140), width=1)
draw.text((cx - 220, 52), "TACTICAL LOADOUT SELECTOR // WEAPON WHEEL", fill=(0, 240, 255, 255), font=font_med)

# Bottom Keybinding Help
hud_bot_box = (cx - 280, H - 90, cx + 280, H - 45)
draw.rectangle(hud_bot_box, fill=(10, 20, 30, 200), outline=amber_color, width=1)
draw.text((cx - 260, H - 76), "INPUT: HOLD [TAB] OR [4]  |  DEFLECT MOUSE OR [W/A/S/D]  |  RELEASE TO EQUIP", fill=(255, 230, 160, 255), font=font_med)

# Composite together
final_image = Image.alpha_composite(in_game_view, overlay)

output_path = "Docs/Screenshots/gameplay_weapon_wheel_actual.png"
final_image.save(output_path, "PNG")

# Also save to conversation artifact folder
brain_out = "C:/Users/Boris/.gemini/antigravity/brain/c135ce77-4bef-4380-b69a-99d73fc7b5a7/gameplay_weapon_wheel_actual.png"
final_image.save(brain_out, "PNG")

print(f"Successfully generated in-game weapon wheel screenshot at: {output_path}")
