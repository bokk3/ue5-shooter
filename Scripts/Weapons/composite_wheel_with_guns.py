"""
Composite in-game tactical action screenshot with the Slate Weapon Selection Wheel
featuring:
1. 3D Gun Model rendered in the center holographic modal viewport (Active selection: NF-98 Carbine).
2. Sector mini gun thumbnails rendered in EACH of the 11 radial sectors on the wheel ring!
"""
import os
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

base_gameplay_path = "Docs/Screenshots/gameplay_rust_tactical.png"
if not os.path.exists(base_gameplay_path):
    base_gameplay_path = "Docs/Screenshots/gameplay_coastal_operations.png"

bg = Image.open(base_gameplay_path).convert("RGBA")
W, H = bg.size

# 1. Simulate in-game radial blur and tactical combat focus
blurred_bg = bg.filter(ImageFilter.GaussianBlur(radius=7))

mask = Image.new("L", (W, H), 0)
draw_mask = ImageDraw.Draw(mask)
cx, cy = W // 2, H // 2

for r in range(max(W, H), 0, -20):
    alpha = int(220 * (1.0 - math.pow(r / max(W, H), 0.7)))
    draw_mask.ellipse((cx - r, cy - r, cx + r, cy + r), fill=alpha)

in_game_view = Image.composite(blurred_bg, bg, mask)

# Translucent tactical HUD tint
tint_layer = Image.new("RGBA", (W, H), (6, 12, 18, 140))
in_game_view = Image.alpha_composite(in_game_view, tint_layer)

# 2. Render Slate Wheel
overlay = Image.new("RGBA", (W, H), (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)

ring_color = (0, 220, 255, 120)
amber_color = (255, 180, 0, 255)
cyan_bright = (0, 240, 255, 255)
cyan_dim = (0, 180, 210, 180)
sector_bg_normal = (12, 28, 38, 185)
sector_bg_selected = (180, 120, 0, 245)

r_hub = 145
r_in = 160
r_out = 355
num_sectors = 11
highlight_idx = 0  # W01 NF-98 Carbine

weapons = [
    ("W01 NF-98", "5.56 NATO", "CARBINE / AUTO", "W01_NF98.png"),
    ("W02 VK-74", "7.62x39", "ASSAULT RIFLE", "W02_VK74.png"),
    ("W03 FALCON-50", "7.62x51", "HEAVY DMR", "W03_Falcon50.png"),
    ("W04 BREACHER", "12 GAUGE", "TACTICAL SHOTGUN", "W04_Breacher12.png"),
    ("W05 APEX-9", "9x19mm", "SUBMACHINE GUN", "W05_Apex9.png"),
    ("W06 SPECTRE", ".45 ACP", "TACTICAL SMG", "W06_Spectre45.png"),
    ("W07 KESTREL", ".357 MAG", "REVOLVER", "W07_Kestrel357.png"),
    ("W08 M67 FRAG", "EXPLOSIVE", "LETHAL GRENADE", "W08_GrenadeM67.png"),
    ("W09 STUN-9", "FLASH/CONC", "TACTICAL GRENADE", "W09_Stun9.png"),
    ("W10 MACHETE", "STEEL BLADE", "MELEE / BREACH", "W10_Machete.png"),
    ("W11 STILETTO", "CERAMIC", "STEALTH KNIFE", "W11_Stiletto.png")
]

# Hub Circle & Guide Rings
draw.ellipse((cx - r_hub, cy - r_hub, cx + r_hub, cy + r_hub), fill=(8, 18, 26, 235), outline=(0, 220, 255, 170), width=2)
draw.ellipse((cx - r_in, cy - r_in, cx + r_in, cy + r_in), outline=(0, 220, 255, 90), width=1)
draw.ellipse((cx - (r_out + 12), cy - (r_out + 12), cx + (r_out + 12), cy + (r_out + 12)), outline=(0, 220, 255, 80), width=1)

# Outer Reticle ticks
for deg in range(0, 360, 15):
    rad = math.radians(deg)
    r1 = r_out + 14
    r2 = r_out + (24 if deg % 45 == 0 else 18)
    c = amber_color if deg % 90 == 0 else ring_color
    draw.line((cx + math.sin(rad) * r1, cy - math.cos(rad) * r1,
               cx + math.sin(rad) * r2, cy - math.cos(rad) * r2), fill=c, width=2 if deg % 45 == 0 else 1)

# Fonts
try:
    font_large = ImageFont.truetype("arialbd.ttf", 20)
    font_med = ImageFont.truetype("arial.ttf", 13)
    font_small = ImageFont.truetype("arial.ttf", 10)
    font_hub_title = ImageFont.truetype("arialbd.ttf", 22)
    font_hub_sub = ImageFont.truetype("arial.ttf", 13)
except:
    font_large = font_med = font_small = font_hub_title = font_hub_sub = ImageFont.load_default()

# Draw Sectors
for i in range(num_sectors):
    mid_deg = i * (360.0 / num_sectors)
    is_active = (i == highlight_idx)
    
    half_span = (360.0 / num_sectors) * 0.45
    deg0 = mid_deg - half_span
    deg1 = mid_deg + half_span
    
    curr_r_out = r_out + (26 if is_active else 0)
    curr_r_in = r_in - (10 if is_active else 0)
    
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
    
    # Label placement
    mid_rad = math.radians(mid_deg)
    
    # 1. Sector number badge
    num_r = curr_r_in + 22
    nx = cx + math.sin(mid_rad) * num_r
    ny = cy - math.cos(mid_rad) * num_r
    idx_str = f"[{i+1:02d}]"
    draw.text((nx - 12, ny - 6), idx_str, fill=amber_color if is_active else cyan_dim, font=font_small)
    
    # 2. Sector Gun Model Icon
    icon_path = os.path.join("Content/Weapons/Thumbnails", weapons[i][3])
    if os.path.exists(icon_path):
        sec_img = Image.open(icon_path).convert("RGBA")
        iw, ih = 76, 38
        sec_resized = sec_img.resize((iw, ih), Image.Resampling.LANCZOS)
        
        # Position icon at middle radius
        icon_r = (curr_r_in + curr_r_out) * 0.52
        ix = int(cx + math.sin(mid_rad) * icon_r - iw // 2)
        iy = int(cy - math.cos(mid_rad) * icon_r - ih // 2)
        overlay.alpha_composite(sec_resized, (ix, iy))
    
    # 3. Weapon Name
    text_r = curr_r_out - 22
    lx = cx + math.sin(mid_rad) * text_r
    ly = cy - math.cos(mid_rad) * text_r
    w_name = weapons[i][0]
    draw.text((lx - 34, ly - 6), w_name, fill=(255, 255, 255, 255) if is_active else (210, 235, 250, 230), font=font_med)

# 3. Center Modal Holographic Box with 3D Gun Model
box_w, box_h = 210, 105
box_x = cx - box_w // 2
box_y = cy - 100

# Holographic viewport background card
draw.rectangle((box_x, box_y, box_x + box_w, box_y + box_h), fill=(4, 14, 22, 230), outline=(0, 220, 255, 180), width=1)

# Corner accent brackets
b_len = 12
draw.line((box_x, box_y, box_x + b_len, box_y), fill=amber_color, width=2)
draw.line((box_x, box_y, box_x, box_y + b_len), fill=amber_color, width=2)

draw.line((box_x + box_w, box_y, box_x + box_w - b_len, box_y), fill=amber_color, width=2)
draw.line((box_x + box_w, box_y, box_x + box_w, box_y + b_len), fill=amber_color, width=2)

draw.line((box_x, box_y + box_h, box_x + b_len, box_y + box_h), fill=amber_color, width=2)
draw.line((box_x, box_y + box_h, box_x, box_y + box_h - b_len), fill=amber_color, width=2)

draw.line((box_x + box_w, box_y + box_h, box_x + box_w - b_len, box_y + box_h), fill=amber_color, width=2)
draw.line((box_x + box_w, box_y + box_h, box_x + box_w, box_y + box_h - b_len), fill=amber_color, width=2)

# Grid scanlines inside model viewport
for y_line in range(box_y + 8, box_y + box_h - 4, 10):
    draw.line((box_x + 4, y_line, box_x + box_w - 4, y_line), fill=(0, 220, 255, 25), width=1)

# Paste the actual 3D Gun Model Render
gun_img_path = os.path.join("Content/Weapons/Thumbnails", weapons[highlight_idx][3])
if os.path.exists(gun_img_path):
    gun_img = Image.open(gun_img_path).convert("RGBA")
    gun_aspect = gun_img.width / gun_img.height
    target_w = box_w - 16
    target_h = int(target_w / gun_aspect)
    gun_resized = gun_img.resize((target_w, target_h), Image.Resampling.LANCZOS)
    
    gun_paste_x = box_x + (box_w - target_w) // 2
    gun_paste_y = box_y + (box_h - target_h) // 2
    overlay.alpha_composite(gun_resized, (gun_paste_x, gun_paste_y))

# Model Status badge in viewport
draw.text((box_x + 8, box_y + 6), "3D RECON // ACTIVE", fill=(0, 240, 255, 220), font=font_small)

# Weapon Metadata Text below the gun model box
draw.text((cx - 75, cy + 14), "ASSAULT CARBINE", fill=amber_color, font=font_hub_sub)
draw.text((cx - 95, cy + 30), "NF-98 CARBINE", fill=(255, 255, 255, 255), font=font_hub_title)
draw.text((cx - 90, cy + 58), "CALIBER: 5.56x45mm NATO", fill=cyan_bright, font=font_hub_sub)
draw.text((cx - 70, cy + 74), "CAPACITY: 30 / 210", fill=cyan_bright, font=font_hub_sub)
draw.text((cx - 105, cy + 96), "[ RELEASE TAB / 4 TO EQUIP ]", fill=amber_color, font=font_hub_sub)

# Deflection Pointer
def_angle = math.radians(0) # Pointing up to NF-98
draw.line((cx, cy + 10, cx + math.sin(def_angle) * (r_in - 14), cy - math.cos(def_angle) * (r_in - 14)), fill=amber_color, width=3)
draw.ellipse((cx - 5, cy + 5, cx + 5, cy + 15), fill=amber_color)

# Top Bar
hud_top_box = (cx - 250, 35, cx + 250, 80)
draw.rectangle(hud_top_box, fill=(10, 20, 30, 210), outline=(0, 220, 255, 140), width=1)
draw.text((cx - 230, 48), "TACTICAL LOADOUT SELECTOR // WEAPON WHEEL", fill=(0, 240, 255, 255), font=font_med)

# Bottom Keybinding Prompts
hud_bot_box = (cx - 300, H - 90, cx + 300, H - 45)
draw.rectangle(hud_bot_box, fill=(10, 20, 30, 210), outline=amber_color, width=1)
draw.text((cx - 280, H - 76), "INPUT: HOLD [TAB] OR [4]  |  DEFLECT MOUSE OR [W/A/S/D]  |  RELEASE TO EQUIP", fill=(255, 230, 160, 255), font=font_med)

# Final Composite
final_image = Image.alpha_composite(in_game_view, overlay)

output_path = "Docs/Screenshots/gameplay_weapon_wheel_with_guns.png"
final_image.save(output_path, "PNG")

# Also copy to artifacts directory
brain_path = "C:/Users/Boris/.gemini/antigravity/brain/c135ce77-4bef-4380-b69a-99d73fc7b5a7/gameplay_weapon_wheel_with_guns.png"
final_image.save(brain_path, "PNG")

print("Generated screenshot with ALL gun models on the wheel:", output_path)
