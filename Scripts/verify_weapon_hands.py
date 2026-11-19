import subprocess
import shutil
import os
import time
from PIL import Image

src_bin = r'C:\Users\Boris\Documents\antigravity\noble-fermi\Binaries\Win64'
pkg_bin = r'C:\Users\Boris\Documents\antigravity\noble-fermi\Packaged\Windows\NobleFermi\Binaries\Win64'
art_dir = r'C:\Users\Boris\.gemini\antigravity\brain\c135ce77-4bef-4380-b69a-99d73fc7b5a7'
shot_dir = r'C:\Users\Boris\Documents\antigravity\noble-fermi\Packaged\Windows\NobleFermi\Docs\Screenshots'

os.makedirs(pkg_bin, exist_ok=True)
os.makedirs(shot_dir, exist_ok=True)

# 1. Copy fresh binaries
for f in ['NobleFermi.exe', 'NobleFermi.pdb']:
    sp = os.path.join(src_bin, f)
    dp = os.path.join(pkg_bin, f)
    if os.path.exists(sp):
        shutil.copy2(sp, dp)
        print(f'Staged {f} to {pkg_bin}')

exe = os.path.join(pkg_bin, 'NobleFermi.exe')

shots = [
    ('hangar_grip_lowready.png', 7.0),
    ('hangar_grip_magcheck.png', 1.8),
    ('hangar_grip_barrelwipe.png', 4.0),
]

for shot_name, inspect_time in shots:
    dest = os.path.join(shot_dir, shot_name)
    if os.path.exists(dest):
        os.remove(dest)
    print(f'Taking screenshot {shot_name} at inspect time {inspect_time}...')
    args = [
        exe,
        '-TakeMenuShot',
        '-MenuShotDelay=4.5',
        f'-InspectTime={inspect_time}',
        f'-ShotName={shot_name}',
        '-ResX=1920',
        '-ResY=1080',
        '-Windowed',
        '-NoSound'
    ]
    p = subprocess.Popen(args)
    try:
        p.wait(timeout=35)
    except subprocess.TimeoutExpired:
        p.kill()
    
    if os.path.exists(dest):
        art_dest = os.path.join(art_dir, shot_name)
        shutil.copy2(dest, art_dest)
        print(f'Successfully captured and copied {shot_name} (size: {os.path.getsize(dest)} bytes)')
    else:
        print(f'Warning: {shot_name} was not created!')

# 2. Crops and comparisons
crops = {}
for key, fname in [('lowready', 'hangar_grip_lowready.png'), ('magcheck', 'hangar_grip_magcheck.png'), ('barrelwipe', 'hangar_grip_barrelwipe.png')]:
    p = os.path.join(art_dir, fname)
    if os.path.exists(p):
        im = Image.open(p)
        chest = im.crop((700, 320, 1300, 820))
        chest.save(os.path.join(art_dir, f'chest_{key}.png'))
        
        rh = im.crop((780, 460, 1030, 710))
        rh.save(os.path.join(art_dir, f'rh_{key}.png'))
        
        if key == 'lowready':
            lh = im.crop((980, 460, 1240, 710))
        elif key == 'magcheck':
            lh = im.crop((820, 500, 1080, 750))
        else:
            lh = im.crop((1020, 410, 1260, 660))
        lh.save(os.path.join(art_dir, f'lh_{key}.png'))
        crops[key] = (chest, rh, lh)

if len(crops) == 3:
    # Save composite triptychs
    w, h = crops['lowready'][0].size
    triptych = Image.new('RGB', (w * 3, h))
    for i, k in enumerate(['lowready', 'magcheck', 'barrelwipe']):
        triptych.paste(crops[k][0], (i * w, 0))
    triptych.save(os.path.join(art_dir, 'hangar_pose_triptych_solid.png'))
    print('Generated hangar_pose_triptych_solid.png')

print('All verification actions complete!')
