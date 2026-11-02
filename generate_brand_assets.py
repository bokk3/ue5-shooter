import os
import math
from PIL import Image, ImageDraw, ImageFilter, ImageFont

def render_shield(size=1024):
    """Render high-resolution textless shield emblem (1024x1024 RGBA) matching favicon.svg"""
    scale = size / 256.0
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    
    # Pre-render linear gradient masks
    def get_color_grad(p1, p2, col1, col2, y_val):
        t = max(0.0, min(1.0, (y_val - p1) / (p2 - p1)))
        r = int(col1[0] + (col2[0] - col1[0]) * t)
        g = int(col1[1] + (col2[1] - col1[1]) * t)
        b = int(col1[2] + (col2[2] - col1[2]) * t)
        a = int(col1[3] + (col2[3] - col1[3]) * t) if len(col1) > 3 else 255
        return (r, g, b, a)

    # 1. Dark obsidian background shield fill
    # Polygon points in 256 space: 128,14 226,56 206,178 128,242 50,178 30,56
    shield_pts = [
        (128 * scale, 14 * scale),
        (226 * scale, 56 * scale),
        (206 * scale, 178 * scale),
        (128 * scale, 242 * scale),
        (50 * scale, 178 * scale),
        (30 * scale, 56 * scale),
    ]

    # Draw gradient fill for shield interior
    mask_fill = Image.new("L", (size, size), 0)
    draw_mask = ImageDraw.Draw(mask_fill)
    draw_mask.polygon(shield_pts, fill=255)

    grad_img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    grad_draw = ImageDraw.Draw(grad_img)
    top_y = int(14 * scale)
    bot_y = int(242 * scale)
    for y in range(top_y, bot_y + 1):
        c = get_color_grad(top_y, bot_y, (14, 30, 50, 255), (3, 7, 18, 255), y)
        grad_draw.line([(0, y), (size, y)], fill=c)

    img.paste(grad_img, (0, 0), mask_fill)

    # 2. Outer Hexagonal Shield Border (Amber Gradient #FFE57F -> #FFB300 -> #B25900)
    # Stroke width 8 * scale
    draw = ImageDraw.Draw(img)
    stroke_w_outer = int(round(8 * scale))
    draw.polygon(shield_pts, outline=(255, 179, 0, 255), width=stroke_w_outer)

    # 3. Inner Hexagonal Bevel (Cyan Gradient #80F4FF -> #00E5FF -> #007799)
    # points: 128,28 210,64 192,170 128,224 64,170 46,64
    inner_pts = [
        (128 * scale, 28 * scale),
        (210 * scale, 64 * scale),
        (192 * scale, 170 * scale),
        (128 * scale, 224 * scale),
        (64 * scale, 170 * scale),
        (46 * scale, 64 * scale),
    ]
    stroke_w_inner = int(round(3.5 * scale))
    draw.polygon(inner_pts, outline=(0, 229, 255, 230), width=stroke_w_inner)

    # 4. Tactical Visor Brow
    # polygon points: 62,76 128,96 194,76 184,100 128,116 72,100
    brow_pts = [
        (62 * scale, 76 * scale),
        (128 * scale, 96 * scale),
        (194 * scale, 76 * scale),
        (184 * scale, 100 * scale),
        (128 * scale, 116 * scale),
        (72 * scale, 100 * scale),
    ]
    draw.polygon(brow_pts, fill=(0, 229, 255, 255))

    # 5. Interlocking NF Monogram
    # Letter N: M 66 174 L 66 112 L 86 112 L 114 152 L 114 112 L 130 112 L 130 174 L 110 174 L 82 134 L 82 174 Z
    n_pts = [
        (66 * scale, 174 * scale),
        (66 * scale, 112 * scale),
        (86 * scale, 112 * scale),
        (114 * scale, 152 * scale),
        (114 * scale, 112 * scale),
        (130 * scale, 112 * scale),
        (130 * scale, 174 * scale),
        (110 * scale, 174 * scale),
        (82 * scale, 134 * scale),
        (82 * scale, 174 * scale),
    ]
    draw.polygon(n_pts, fill=(255, 179, 0, 255))

    # Letter F: M 138 174 L 138 112 L 194 112 L 188 128 L 156 128 L 156 140 L 184 140 L 178 156 L 156 156 L 156 174 Z
    f_pts = [
        (138 * scale, 174 * scale),
        (138 * scale, 112 * scale),
        (194 * scale, 112 * scale),
        (188 * scale, 128 * scale),
        (156 * scale, 128 * scale),
        (156 * scale, 140 * scale),
        (184 * scale, 140 * scale),
        (178 * scale, 156 * scale),
        (156 * scale, 156 * scale),
        (156 * scale, 174 * scale),
    ]
    draw.polygon(f_pts, fill=(0, 229, 255, 255))

    # 6. Relativistic Chrono Ring Slash: Quadratic Bezier M 16 146 Q 128 190 240 110
    # Parametric quadratic curve: B(t) = (1-t)^2 P0 + 2(1-t)t P1 + t^2 P2
    p0 = (16.0 * scale, 146.0 * scale)
    p1 = (128.0 * scale, 190.0 * scale)
    p2 = (240.0 * scale, 110.0 * scale)
    curve_pts = []
    num_steps = 80
    for i in range(num_steps + 1):
        t = i / float(num_steps)
        bx = (1 - t)**2 * p0[0] + 2 * (1 - t) * t * p1[0] + t**2 * p2[0]
        by = (1 - t)**2 * p0[1] + 2 * (1 - t) * t * p1[1] + t**2 * p2[1]
        curve_pts.append((bx, by))

    draw.line(curve_pts, fill=(0, 229, 255, 255), width=int(round(6 * scale)), joint="curve")

    # 7. Red/Amber core singularity diamond: 128,184 140,200 128,216 116,200
    dia_pts = [
        (128 * scale, 184 * scale),
        (140 * scale, 200 * scale),
        (128 * scale, 216 * scale),
        (116 * scale, 200 * scale),
    ]
    draw.polygon(dia_pts, fill=(255, 42, 84, 255), outline=(255, 179, 0, 255))

    return img

def create_ico(shield_1024, output_ico_path):
    """Generate multi-resolution .ico file"""
    sizes = [256, 128, 64, 48, 32, 16]
    images = [shield_1024.resize((s, s), Image.Resampling.LANCZOS) for s in sizes]
    os.makedirs(os.path.dirname(output_ico_path), exist_ok=True)
    images[0].save(
        output_ico_path,
        format="ICO",
        sizes=[(s, s) for s in sizes],
        append_images=images[1:]
    )
    print(f"Saved icon to: {output_ico_path}")

def create_splash_bmp(shield_1024, logo_png_path, output_bmp_path, width=800, height=450):
    """Build AAA photorealistic tactical splash screen combining shield + logo"""
    splash = Image.new("RGB", (width, height), (3, 7, 18)) # #030712
    draw = ImageDraw.Draw(splash)

    # 1. Subtle Radial / Gradient Glow in background
    glow = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow)
    cx, cy = width // 2, 160
    # Cyan atmospheric radial glow behind the shield
    for r in range(220, 0, -10):
        alpha = int(45 * (1.0 - r / 220.0))
        glow_draw.ellipse(
            [(cx - r * 1.5, cy - r * 0.9), (cx + r * 1.5, cy + r * 0.9)],
            fill=(0, 229, 255, alpha)
        )
    # Amber warmth at base of shield
    for r in range(120, 0, -10):
        alpha = int(35 * (1.0 - r / 120.0))
        glow_draw.ellipse(
            [(cx - r, cy + 40 - r * 0.6), (cx + r, cy + 40 + r * 0.6)],
            fill=(255, 179, 0, alpha)
        )
    splash.paste(glow, (0, 0), glow)

    # 2. Subtle military grid lines
    grid_col = (10, 24, 42) # subtle dark cyan-blue grid
    for x in range(0, width, 40):
        draw.line([(x, 0), (x, height)], fill=grid_col, width=1)
    for y in range(0, height, 40):
        draw.line([(0, y), (width, y)], fill=grid_col, width=1)

    # 3. Outer border and HUD corner brackets
    border_col = (0, 180, 220)
    bracket_len = 24
    # Top-Left
    draw.line([(12, 12), (12 + bracket_len, 12)], fill=border_col, width=2)
    draw.line([(12, 12), (12, 12 + bracket_len)], fill=border_col, width=2)
    # Top-Right
    draw.line([(width - 12 - bracket_len, 12), (width - 12, 12)], fill=border_col, width=2)
    draw.line([(width - 12, 12), (width - 12, 12 + bracket_len)], fill=border_col, width=2)
    # Bottom-Left
    draw.line([(12, height - 12), (12 + bracket_len, height - 12)], fill=border_col, width=2)
    draw.line([(12, height - 12 - bracket_len), (12, height - 12)], fill=border_col, width=2)
    # Bottom-Right
    draw.line([(width - 12 - bracket_len, height - 12), (width - 12, height - 12)], fill=border_col, width=2)
    draw.line([(width - 12, height - 12 - bracket_len), (width - 12, height - 12)], fill=border_col, width=2)

    # 4. Paste Shield Emblem (scaled to ~140x140 at top center)
    shield_size = 140
    shield_thumb = shield_1024.resize((shield_size, shield_size), Image.Resampling.LANCZOS)
    sx = (width - shield_size) // 2
    sy = 45
    splash.paste(shield_thumb, (sx, sy), shield_thumb)

    # 5. Paste the big Battle-Scarred "NOBLE FERMI" logo
    if os.path.exists(logo_png_path):
        logo_img = Image.open(logo_png_path).convert("RGBA")
        # Target width ~560px
        lw = 560
        lh = int(logo_img.height * (lw / float(logo_img.width)))
        logo_thumb = logo_img.resize((lw, lh), Image.Resampling.LANCZOS)
        lx = (width - lw) // 2
        ly = sy + shield_size + 10
        splash.paste(logo_thumb, (lx, ly), logo_thumb)

    # 6. Tactical Telemetry & Subtitle Text
    # Subtitle
    subtext_y = height - 105
    # Thin divider lines
    draw.line([(80, subtext_y), (width - 80, subtext_y)], fill=(0, 229, 255, 120), width=1)
    
    # Try using system font or default font
    try:
        font_sub = ImageFont.truetype("arial.ttf", 13)
        font_micro = ImageFont.truetype("arial.ttf", 10)
    except Exception:
        font_sub = ImageFont.load_default()
        font_micro = ImageFont.load_default()

    subtitle = "CHRONO-TACTICAL WARFARE  //  128 HZ SIMULATION ENGINE"
    draw.text((width // 2, subtext_y + 16), subtitle, fill=(240, 245, 255), anchor="mm", font=font_sub)

    # Bottom status bar (tactical 18px tall bar for live GDI progress & in-bar text)
    bar_y = 386
    bar_w = 520
    bar_h = 18
    bx = (width - bar_w) // 2
    draw.rectangle([(bx, bar_y), (bx + bar_w, bar_y + bar_h)], fill=(8, 18, 32), outline=(0, 229, 255, 200))
    # Initial 2% boot tick before NobleFermi module takes over live GDI rendering
    draw.rectangle([(bx + 2, bar_y + 2), (bx + 14, bar_y + bar_h - 2)], fill=(255, 179, 0))
    draw.text((width // 2, bar_y + bar_h // 2), "BOOTING UNREAL ENGINE 5.8 RUNTIME — 2%", fill=(240, 248, 255), anchor="mm", font=font_micro)

    telemetry = "INITIALIZING CORE RHI & MODULE SUBSYSTEMS..."
    draw.text((width // 2, 420), telemetry, fill=(0, 229, 255), anchor="mm", font=font_micro)

    os.makedirs(os.path.dirname(output_bmp_path), exist_ok=True)
    splash.save(output_bmp_path, format="BMP")
    print(f"Saved splash screen BMP to: {output_bmp_path}")

if __name__ == "__main__":
    print("Generating Shield Emblem & Icons...")
    os.makedirs("Content/Slate/Icons", exist_ok=True)
    shield = render_shield(1024)
    shield.save("Content/Slate/Icons/NobleFermi_Shield_1024.png")
    
    # Output icons
    icon_paths = [
        "Build/Windows/Application.ico",
        "Source/NobleFermi/Resources/Windows/NobleFermi.ico",
        "Content/Splash/NobleFermi.ico",
        "Content/Slate/Icons/NobleFermi.ico",
        "web/favicon.ico"
    ]
    for p in icon_paths:
        create_ico(shield, p)

    # Output splash screens
    splash_paths = [
        "Content/Splash/Splash.bmp",
        "Content/Splash/EdSplash.bmp",
        "Build/Windows/Splash/Splash.bmp"
    ]
    for sp in splash_paths:
        create_splash_bmp(shield, "Docs/images/noble_fermi_logo.png", sp)

    print("Brand assets successfully generated!")
