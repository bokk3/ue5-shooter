"""Tile <name>__front/right/top.png renders into labelled contact sheets, N assets per sheet (2 per row)."""
import os
import sys

from PIL import Image, ImageDraw

views_dir, out_prefix, per_sheet = sys.argv[1], sys.argv[2], int(sys.argv[3])
names = sys.argv[4:]
T = 200  # tile size
LABEL = 18
COLS = 2  # assets side by side
for s in range(0, len(names), per_sheet):
    chunk = names[s:s + per_sheet]
    rows = (len(chunk) + COLS - 1) // COLS
    sheet = Image.new("RGB", (COLS * 3 * T + (COLS - 1) * 12, rows * (T + LABEL)), (20, 20, 20))
    d = ImageDraw.Draw(sheet)
    for i, name in enumerate(chunk):
        r, c = divmod(i, COLS)
        x0, y0 = c * (3 * T + 12), r * (T + LABEL)
        d.text((x0 + 4, y0 + 3), f"{s + i + 1}. {name}   [front +Z | right +X | top]", fill=(255, 230, 120))
        for j, view in enumerate(("front", "right", "top")):
            p = os.path.join(views_dir, f"{name}__{view}.png")
            if os.path.exists(p):
                sheet.paste(Image.open(p).convert("RGB").resize((T, T)), (x0 + j * T, y0 + LABEL))
    out = f"{out_prefix}_{s // per_sheet + 1:02d}.png"
    sheet.save(out)
    print(out)
