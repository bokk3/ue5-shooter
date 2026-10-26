"""
NOBLE FERMI // FAVELA TEXTURE GENERATOR
Procedurally synthesises seamless (tileable) PBR texture sets for the favela environment kit:
brick (tijolo baiano), cinder block, stucco (+peel mask in alpha), concrete, cobblestone,
asphalt, red laterite dirt, beach sand, weathered wood, corrugated rusted metal,
macro/streak grime mask, and alpha-masked foliage atlases (weeds, ivy, banana leaf).

Output: SourceArt/Favela/Textures/*.png
Normal map convention: R -> +U, G -> +V (image rows downward) == DirectX / Unreal convention.
"""
import os
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceArt", "Favela", "Textures")
OUT = os.path.abspath(OUT)
os.makedirs(OUT, exist_ok=True)
N = 512


# ---------------------------------------------------------------------------------------
# Core tileable primitives
# ---------------------------------------------------------------------------------------
def fbm(seed, beta=2.0, size=N, aniso=(1.0, 1.0), lowcut=1.0):
    """Periodic fractal noise via spectral synthesis. Returns 0..1."""
    rng = np.random.default_rng(seed)
    fx = np.fft.fftfreq(size)[None, :] * size * aniso[0]
    fy = np.fft.fftfreq(size)[:, None] * size * aniso[1]
    f = np.sqrt(fx * fx + fy * fy)
    f[0, 0] = 1.0
    amp = 1.0 / np.power(np.maximum(f, lowcut), beta / 2.0)
    amp[0, 0] = 0.0
    phase = rng.uniform(0, 2 * np.pi, (size, size))
    spec = amp * np.exp(1j * phase)
    n = np.real(np.fft.ifft2(spec))
    n -= n.min()
    n /= max(n.max(), 1e-8)
    return n


def voronoi(seed, cells, size=N, jitter=0.85):
    """Tileable jittered-grid Voronoi. Returns F1, F2 (in cell units) and cell id."""
    rng = np.random.default_rng(seed)
    jit = 0.5 + (rng.random((cells, cells, 2)) - 0.5) * jitter
    ys, xs = np.mgrid[0:size, 0:size].astype(np.float64)
    px = (xs + 0.5) / size * cells
    py = (ys + 0.5) / size * cells
    cx = np.floor(px).astype(int)
    cy = np.floor(py).astype(int)
    f1 = np.full((size, size), 1e9)
    f2 = np.full((size, size), 1e9)
    cid = np.zeros((size, size), dtype=np.int64)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            nx = cx + dx
            ny = cy + dy
            wx = nx % cells
            wy = ny % cells
            ptx = nx + jit[wy, wx, 0]
            pty = ny + jit[wy, wx, 1]
            d = np.sqrt((px - ptx) ** 2 + (py - pty) ** 2)
            ids = wy * cells + wx
            closer = d < f1
            f2 = np.where(closer, f1, np.minimum(f2, d))
            cid = np.where(closer, ids, cid)
            f1 = np.where(closer, d, f1)
    return f1, f2, cid


def normal_from_height(h, strength):
    dx = (np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)) * 0.5
    dy = (np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)) * 0.5
    nx = -dx * strength
    ny = -dy * strength
    nz = np.ones_like(h)
    l = np.sqrt(nx * nx + ny * ny + nz * nz)
    n = np.stack([nx / l, ny / l, nz / l], axis=-1)
    return n


def save_rgb(name, rgb, alpha=None):
    rgb = np.clip(rgb, 0, 1)
    arr = (rgb * 255.0 + 0.5).astype(np.uint8)
    if alpha is not None:
        a = (np.clip(alpha, 0, 1) * 255.0 + 0.5).astype(np.uint8)
        Image.fromarray(np.dstack([arr, a]), "RGBA").save(os.path.join(OUT, name))
    else:
        Image.fromarray(arr, "RGB").save(os.path.join(OUT, name))
    print("  wrote", name)


def save_normal(name, n):
    save_rgb(name, n * 0.5 + 0.5)


def save_gray(name, g):
    g = np.clip(g, 0, 1)
    save_rgb(name, np.dstack([g, g, g]))


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def col(r, g, b):
    return np.array([r, g, b], dtype=np.float64)


def masonry_layout(rows, per_row, mortar_px, seed, offset_alt=0.5, size=N):
    """Running-bond masonry. Returns edge distance (px) inside unit, unit id, local v (0..1)."""
    ys, xs = np.mgrid[0:size, 0:size].astype(np.float64)
    rh = size / rows
    bw = size / per_row
    row = np.floor(ys / rh).astype(int)
    shift = np.where(row % 2 == 1, offset_alt * bw, 0.0)
    xx = (xs + shift) % size
    unit = np.floor(xx / bw).astype(int)
    lx = xx - unit * bw
    ly = ys - row * rh
    # sloppy mortar: jitter the mortar thickness with noise
    wob = (fbm(seed + 7, 2.2) - 0.5) * mortar_px * 0.9
    m = mortar_px * 0.5 + wob
    edge = np.minimum(np.minimum(lx, bw - lx), np.minimum(ly, rh - ly)) - m
    uid = row * per_row + (unit % per_row)
    return edge, uid, ly / rh, row


# ---------------------------------------------------------------------------------------
# Texture sets
# ---------------------------------------------------------------------------------------
def tex_brick():
    print("Brick (tijolo baiano)")
    edge, uid, lv, row = masonry_layout(6, 4, 7.0, 11)
    rng = np.random.default_rng(3)
    hue = rng.random(4096)
    n1 = fbm(21, 2.0)
    n2 = fbm(22, 1.2)
    brick_mask = smoothstep(-0.5, 1.5, edge)
    base_a = col(0.66, 0.30, 0.15)
    base_b = col(0.52, 0.22, 0.11)
    base_c = col(0.72, 0.40, 0.22)
    t = hue[uid][..., None]
    bcol = np.where(t < 0.5, lerp(base_a, base_b, t * 2), lerp(base_a, base_c, (t - 0.5) * 2))
    bcol = bcol * (0.82 + 0.3 * n1[..., None]) * (0.92 + 0.1 * n2[..., None])
    # horizontal face grooves typical of hollow ceramic blocks
    grooves = 1.0 - 0.35 * (smoothstep(0.02, 0.0, np.abs(lv - 0.36)) + smoothstep(0.02, 0.0, np.abs(lv - 0.66)))
    bcol = bcol * grooves[..., None]
    mortar = col(0.58, 0.56, 0.52) * (0.75 + 0.4 * fbm(23, 1.5)[..., None])
    rgb = lerp(mortar, bcol, brick_mask[..., None])
    h = brick_mask * (0.85 + 0.15 * n1) * grooves + (1 - brick_mask) * (0.15 + 0.2 * fbm(24, 1.0))
    h = h - 0.06 * (smoothstep(0.55, 0.9, fbm(25, 0.8)))  # chipped corners
    rough = lerp(0.95, 0.80, brick_mask) + 0.05 * n2
    save_rgb("T_Brick_BC.png", rgb)
    save_normal("T_Brick_N.png", normal_from_height(h, 6.0))
    save_gray("T_Brick_R.png", rough)


def tex_cinder():
    print("Cinder block")
    edge, uid, lv, row = masonry_layout(6, 3, 6.0, 31)
    rng = np.random.default_rng(5)
    v = rng.random(4096)[uid]
    pores = fbm(41, 0.4)
    n1 = fbm(42, 2.0)
    mask = smoothstep(-0.5, 1.5, edge)
    block = col(0.50, 0.49, 0.46)[None, None, :] * (0.85 + 0.25 * v[..., None]) * (0.85 + 0.25 * n1[..., None])
    block *= (1.0 - 0.25 * smoothstep(0.62, 0.8, pores))[..., None]
    mortar = col(0.60, 0.58, 0.54) * (0.8 + 0.3 * fbm(43, 1.2)[..., None])
    rgb = lerp(mortar, block, mask[..., None])
    h = mask * (0.9 - 0.15 * smoothstep(0.6, 0.85, pores)) + (1 - mask) * (0.3 + 0.3 * fbm(44, 1.0))
    save_rgb("T_Cinder_BC.png", rgb)
    save_normal("T_Cinder_N.png", normal_from_height(h, 5.0))
    save_gray("T_Cinder_R.png", 0.9 + 0.08 * pores)


def tex_stucco():
    print("Stucco (+peel mask)")
    n1 = fbm(51, 2.4)
    n2 = fbm(52, 1.4)
    trowel = fbm(53, 2.8, aniso=(1.0, 0.35))
    f1, f2, _ = voronoi(54, 7)
    crack_line = smoothstep(0.035, 0.0, f2 - f1) * smoothstep(0.55, 0.75, fbm(55, 2.0))
    rgb = col(0.86, 0.85, 0.81)[None, None, :] * (0.82 + 0.22 * n1[..., None]) * (0.95 + 0.08 * n2[..., None])
    rgb *= (1.0 - 0.45 * crack_line)[..., None]
    h = 0.5 + 0.35 * trowel + 0.1 * n2 - 0.5 * crack_line
    peel = fbm(56, 3.0)  # large irregular blotches
    peel = 0.75 * peel + 0.25 * fbm(57, 1.6)
    peel = (peel - peel.min()) / (peel.max() - peel.min())
    save_rgb("T_Stucco_BC.png", rgb, alpha=peel)
    save_normal("T_Stucco_N.png", normal_from_height(h, 3.5))
    save_gray("T_Stucco_R.png", 0.88 + 0.08 * n2)


def tex_concrete():
    print("Concrete")
    n1 = fbm(61, 2.2)
    n2 = fbm(62, 1.0)
    stains = smoothstep(0.55, 0.85, fbm(63, 2.8))
    pores = smoothstep(0.78, 0.9, fbm(64, 0.3))
    rgb = col(0.52, 0.51, 0.49)[None, None, :] * (0.82 + 0.25 * n1[..., None])
    rgb *= (1.0 - 0.25 * stains)[..., None]
    rgb *= (1.0 - 0.35 * pores)[..., None]
    h = 0.5 + 0.2 * n2 - 0.4 * pores + 0.1 * n1
    save_rgb("T_Concrete_BC.png", rgb)
    save_normal("T_Concrete_N.png", normal_from_height(h, 2.5))
    save_gray("T_Concrete_R.png", 0.82 + 0.1 * n2 - 0.1 * stains)


def tex_cobble():
    print("Cobblestone (paralelepipedo / pedra portuguesa)")
    f1, f2, cid = voronoi(71, 15, jitter=0.75)
    rng = np.random.default_rng(7)
    v = rng.random(15 * 15)[cid]
    edge = f2 - f1
    stone = smoothstep(0.04, 0.16, edge)
    dome = np.sqrt(np.clip(edge / 0.5, 0, 1))
    n1 = fbm(72, 1.0)
    n2 = fbm(73, 2.4)
    grey = col(0.42, 0.41, 0.39)
    light = col(0.70, 0.68, 0.64)
    warm = col(0.48, 0.40, 0.33)
    scol = np.where((v < 0.18)[..., None], light, np.where((v > 0.8)[..., None], warm, grey))
    scol = scol * (0.8 + 0.35 * n1[..., None]) * (0.85 + 0.2 * n2[..., None]) * (0.85 + 0.25 * v[..., None])
    gap = col(0.17, 0.13, 0.10) * (0.7 + 0.5 * n2[..., None])
    rgb = lerp(gap, scol, stone[..., None])
    h = stone * (0.55 + 0.45 * dome) + 0.08 * n1
    save_rgb("T_Cobble_BC.png", rgb)
    save_normal("T_Cobble_N.png", normal_from_height(h, 9.0))
    save_gray("T_Cobble_R.png", lerp(0.97, 0.62, stone) + 0.08 * n1)


def tex_asphalt():
    print("Cracked asphalt")
    n1 = fbm(81, 2.0)
    agg = fbm(82, 0.2)
    f1, f2, _ = voronoi(83, 5, jitter=0.95)
    presence = smoothstep(0.45, 0.65, fbm(84, 2.5))
    wob = (fbm(85, 1.8) - 0.5) * 0.06
    crack = smoothstep(0.03, 0.0, np.abs(f2 - f1 + wob)) * presence
    patch = smoothstep(0.6, 0.65, fbm(86, 3.2))
    rgb = col(0.17, 0.17, 0.17)[None, None, :] * (0.8 + 0.4 * n1[..., None])
    rgb = lerp(rgb, col(0.12, 0.12, 0.12), patch[..., None] * 0.8)
    rgb += (smoothstep(0.7, 0.85, agg) * 0.18)[..., None]
    rgb *= (1.0 - 0.7 * crack)[..., None]
    h = 0.5 + 0.25 * agg - 0.6 * crack + 0.05 * patch
    save_rgb("T_Asphalt_BC.png", rgb)
    save_normal("T_Asphalt_N.png", normal_from_height(h, 4.0))
    save_gray("T_Asphalt_R.png", 0.85 - 0.1 * patch + 0.05 * agg)


def tex_dirt():
    print("Red laterite dirt")
    n1 = fbm(91, 2.3)
    n2 = fbm(92, 1.2)
    f1, f2, cid = voronoi(93, 40, jitter=0.9)
    pebble = smoothstep(0.30, 0.18, f1) * (np.random.default_rng(9).random(1600)[cid] > 0.7)
    rgb = col(0.46, 0.22, 0.12)[None, None, :] * (0.75 + 0.45 * n1[..., None])
    rgb = lerp(rgb, col(0.36, 0.25, 0.18), smoothstep(0.55, 0.8, n2)[..., None] * 0.6)
    rgb = lerp(rgb, col(0.55, 0.47, 0.40), pebble[..., None] * 0.8)
    h = 0.4 + 0.3 * n1 + 0.2 * n2 + 0.35 * pebble
    save_rgb("T_Dirt_BC.png", rgb)
    save_normal("T_Dirt_N.png", normal_from_height(h, 5.0))
    save_gray("T_Dirt_R.png", 0.92 - 0.1 * pebble)


def tex_sand():
    print("Beach sand")
    n1 = fbm(101, 2.0)
    grain = fbm(102, 0.1)
    ys, xs = np.mgrid[0:N, 0:N].astype(np.float64)
    warp = fbm(103, 2.6) * 6.0
    ripple = 0.5 + 0.5 * np.sin((ys / N * 18.0 + warp) * 2 * np.pi)
    rgb = col(0.74, 0.65, 0.49)[None, None, :] * (0.85 + 0.25 * n1[..., None]) * (0.92 + 0.12 * grain[..., None])
    h = 0.35 * ripple + 0.2 * n1 + 0.15 * grain
    save_rgb("T_Sand_BC.png", rgb)
    save_normal("T_Sand_N.png", normal_from_height(h, 3.0))
    save_gray("T_Sand_R.png", 0.9 + 0.05 * grain)


def tex_wood():
    print("Weathered wood planks")
    planks = 8
    ys, xs = np.mgrid[0:N, 0:N].astype(np.float64)
    pw = N / planks
    pid = np.floor(xs / pw).astype(int)
    lx = xs - pid * pw
    gap = smoothstep(0.0, 3.0, np.minimum(lx, pw - lx))
    grain = fbm(111, 2.0, aniso=(1.0, 0.06))
    grain2 = fbm(112, 1.0, aniso=(1.0, 0.1))
    rng = np.random.default_rng(11)
    v = rng.random(planks)[pid]
    rgb = lerp(col(0.33, 0.27, 0.21), col(0.50, 0.46, 0.41), v[..., None])
    rgb = rgb * (0.7 + 0.45 * grain[..., None]) * (0.9 + 0.15 * grain2[..., None])
    rgb *= (0.25 + 0.75 * gap)[..., None]
    # nail heads
    for p in range(planks):
        for yy in (0.1, 0.6):
            cx, cy = int((p + 0.5) * pw), int(yy * N)
            rr = (xs - cx) ** 2 + (ys - cy) ** 2
            rgb = np.where((rr < 9)[..., None], col(0.2, 0.12, 0.08), rgb)
    h = gap * (0.6 + 0.3 * grain)
    save_rgb("T_Wood_BC.png", rgb)
    save_normal("T_Wood_N.png", normal_from_height(h, 4.0))
    save_gray("T_Wood_R.png", 0.85 + 0.1 * grain2)


def tex_metal():
    print("Corrugated / rusted metal")
    ys, xs = np.mgrid[0:N, 0:N].astype(np.float64)
    u = xs / N
    vv = ys / N
    n1 = fbm(121, 2.4)
    n2 = fbm(122, 1.0)
    streak = fbm(123, 2.0, aniso=(1.0, 0.08))  # streaks run along V (down the slope)
    edge = np.minimum(np.minimum(u, 1 - u), np.minimum(vv, 1 - vv))
    edge_rust = smoothstep(0.12, 0.0, edge)
    rust = np.clip(smoothstep(0.55, 0.75, n1 * 0.7 + streak * 0.5) + edge_rust * (0.4 + 0.6 * n2), 0, 1)
    zinc = col(0.56, 0.58, 0.58)[None, None, :] * (0.8 + 0.3 * n2[..., None])
    rustc = lerp(col(0.42, 0.18, 0.07), col(0.26, 0.11, 0.05), n2[..., None])
    rgb = lerp(zinc, rustc, rust[..., None])
    # corrugation shading baked lightly (waves along U)
    wave = np.sin(u * 2 * np.pi * 14.0)
    h = 0.5 + 0.05 * n2 + 0.08 * rust * n1
    save_rgb("T_Metal_BC.png", rgb * (0.95 + 0.05 * wave[..., None]))
    save_normal("T_Metal_N.png", normal_from_height(h, 4.0))
    save_gray("T_Metal_R.png", lerp(0.35, 0.92, rust))


def tex_grime():
    print("Macro (R) + rain streak (G) grime mask")
    macro = fbm(131, 3.0)
    streak = fbm(132, 2.0, aniso=(1.0, 0.05))
    streak = smoothstep(0.45, 0.85, streak)
    save_rgb("T_Grime_M.png", np.dstack([macro, streak, np.zeros_like(macro)]))


# ---------------------------------------------------------------------------------------
# Foliage atlases (RGBA, alpha mask). v = 0 (top) are tips, v = 1 (bottom) is the root.
# ---------------------------------------------------------------------------------------
def tex_weeds():
    print("Weed / grass clump atlas")
    S = 512
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    rng = np.random.default_rng(141)
    for i in range(120):
        x0 = rng.uniform(40, S - 40)
        hgt = rng.uniform(0.35, 0.98) * S
        lean = rng.uniform(-0.35, 0.35) * hgt
        w = rng.uniform(4, 11)
        g = rng.uniform(0.28, 0.55)
        c_base = (int(40 + 40 * g), int(70 + 120 * g), int(20 + 25 * g), 255)
        steps = 10
        pts_l, pts_r = [], []
        for s in range(steps + 1):
            t = s / steps
            y = S - t * hgt
            x = x0 + lean * t * t
            ww = w * (1 - t) + 0.6
            pts_l.append((x - ww, y))
            pts_r.append((x + ww, y))
        d.polygon(pts_l + pts_r[::-1], fill=c_base)
        # yellowish dry tips on some blades
        if rng.random() < 0.35:
            tip = pts_l[-3:] + pts_r[-3:][::-1]
            d.polygon(tip, fill=(150, 140, 60, 255))
    # a few broad-leaf weeds
    for i in range(14):
        cx = rng.uniform(60, S - 60)
        cy = rng.uniform(S * 0.45, S * 0.9)
        rw, rh = rng.uniform(14, 26), rng.uniform(30, 55)
        d.ellipse([cx - rw, cy - rh, cx + rw, cy + rh], fill=(55, 105, 35, 255))
        d.line([(cx, cy + rh), (cx, cy - rh)], fill=(90, 140, 60, 255), width=2)
    img = img.filter(ImageFilter.SMOOTH)
    img.save(os.path.join(OUT, "T_Weeds_BC.png"))
    print("  wrote T_Weeds_BC.png")


def tex_ivy():
    print("Ivy atlas")
    S = 512
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    rng = np.random.default_rng(151)
    # stems
    for i in range(16):
        x = rng.uniform(0, S)
        pts = []
        for s in range(12):
            pts.append((x + rng.uniform(-20, 20), S - s * S / 11))
        d.line(pts, fill=(70, 55, 35, 255), width=3)
    for i in range(900):
        cx, cy = rng.uniform(10, S - 10), rng.uniform(10, S - 10)
        r = rng.uniform(7, 15)
        g = rng.uniform(0.0, 1.0)
        c = (int(30 + 40 * g), int(70 + 70 * g), int(20 + 20 * g), 255)
        a = rng.uniform(0, np.pi * 2)
        pts = []
        for k in range(10):
            t = k / 10 * np.pi * 2
            rr = r * (0.75 + 0.25 * np.cos(3 * t))  # three-lobed ivy leaf
            pts.append((cx + rr * np.cos(t + a), cy + rr * np.sin(t + a)))
        d.polygon(pts, fill=c)
    img = img.filter(ImageFilter.SMOOTH)
    img.save(os.path.join(OUT, "T_Ivy_BC.png"))
    print("  wrote T_Ivy_BC.png")


def tex_banana():
    print("Banana leaf atlas")
    S = 512
    ys, xs = np.mgrid[0:S, 0:S].astype(np.float64)
    u = xs / S
    v = ys / S
    # leaf occupies u in [0, 0.84]; stem strip at u in [0.88, 1.0]
    lu = u / 0.84
    width = np.sin(np.clip(1 - v, 0, 1) ** 0.7 * np.pi) * 0.5  # tip at v=0, base at v=1
    width = np.where(v > 0.97, 0.03, width)
    dist = np.abs(lu - 0.5)
    tear_noise = fbm(161, 1.0, size=S, aniso=(0.05, 1.0))
    # lateral tears (slits) running across the leaf
    slit = (np.sin(v * 2 * np.pi * 22.0 + tear_noise * 8) > 0.96) & (dist > 0.12)
    inside = (dist < width) & (lu <= 1.0) & (~slit)
    n1 = fbm(162, 2.0, size=S)
    veins = 0.85 + 0.15 * np.sin((v * 90.0 + dist * 30.0) * np.pi)
    base = lerp(col(0.18, 0.36, 0.08), col(0.32, 0.52, 0.14), n1[..., None]) * veins[..., None]
    midrib = smoothstep(0.02, 0.0, dist)
    base = lerp(base, col(0.55, 0.62, 0.30), midrib[..., None])
    brown = smoothstep(0.75, 1.0, dist / np.maximum(width, 1e-3)) * smoothstep(0.4, 0.8, fbm(163, 2.0, size=S))
    base = lerp(base, col(0.40, 0.30, 0.12), brown[..., None])
    stem = (u >= 0.88)
    rgb = np.where(stem[..., None], col(0.38, 0.42, 0.18) * (0.8 + 0.3 * n1[..., None]), base)
    alpha = np.where(stem | inside, 1.0, 0.0)
    save_rgb("T_Banana_BC.png", rgb, alpha=alpha)


if __name__ == "__main__":
    print("=== FAVELA TEXTURE GENERATION ->", OUT)
    tex_brick()
    tex_cinder()
    tex_stucco()
    tex_concrete()
    tex_cobble()
    tex_asphalt()
    tex_dirt()
    tex_sand()
    tex_wood()
    tex_metal()
    tex_grime()
    tex_weeds()
    tex_ivy()
    tex_banana()
    print("=== DONE ===")
