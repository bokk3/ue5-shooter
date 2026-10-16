import urllib.request
import urllib.parse
import os
import re

HEADERS = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'}

# Curated high-fidelity assets from Sonniss GDC archives on gamesounds.xyz
AUDIO_TARGETS = [
    # 1. Gunshots & Firearms (STRICTLY ONE SINGLE SHOT PER SAMPLE)
    {
        "category": "Weapons/Fire",
        "filename": "A_Carbine_556_Single_Shot.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Audiobeast%20-%20The%20London%20Warehouse%20Firearms%20Library/Audiobeast_Small_Room_H%26K_416_5.56_Assault_Rifle_05m_LS5_001_Single_Shot_x1.wav"
    },
    {
        "category": "Weapons/Fire",
        "filename": "A_Sniper_Heavy_Single_Shot.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202017%20-%20Game%20Audio%20Bundle/Gamemaster%20Audio%20-%20%20Gun%20Sound%20Pack/gun_rifle_sniper_shot_01.wav"
    },
    {
        "category": "Weapons/Fire",
        "filename": "A_Pistol_Revolver_Single_Shot.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202017%20-%20Game%20Audio%20Bundle/Gamemaster%20Audio%20-%20%20Gun%20Sound%20Pack/gun_revolver_pistol_shot_04.wav"
    },
    {
        "category": "Weapons/Fire",
        "filename": "A_Shotgun_Single_Shot.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Wav%20Junction%20Sound%20Effects%20-%20Shotguns/0003_Shotgun_Fire_3.wav"
    },
    {
        "category": "Weapons/Fire",
        "filename": "A_Rifle_Silenced_Fire.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202018%20-%20Game%20Audio%20Bundle/Gamemaster%20Audio%20-%20Silenced%20Gun%20Sounds/gun_silenced_sniper1_shot_01.wav"
    },
    {
        "category": "Weapons/Fire",
        "filename": "A_Pistol_Silenced_Fire.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202018%20-%20Game%20Audio%20Bundle/Gamemaster%20Audio%20-%20Silenced%20Gun%20Sounds/gun_pistol_shot_silenced_04.wav"
    },

    # 2. 4-Phase Reload Mechanics & Foley
    {
        "category": "Weapons/Reloads",
        "filename": "A_Reload_Phase1_Cant_Rustle.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Systematic-Sound%20-%20Sound%20Themes%20-%20Modern%20Cloth%20Foley%2001/SFX%20CLOTH%20Foley%20Jacket%20Synthetic%20Soft%20Shell%20Whoosh%20Flutter.wav"
    },
    {
        "category": "Weapons/Reloads",
        "filename": "A_Reload_Phase2_MagRelease_TopCover.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Pole%20Position%20-%20MG%2042%20machine%20gun/MG%2042%2C%20Handling%2C%20Top%20Cover%20Up%2C%201m%2C%20Right%2C%20MKH8060.wav"
    },
    {
        "category": "Weapons/Reloads",
        "filename": "A_Reload_Phase3_MagInsert_Cocking.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Pole%20Position%20-%20MP%2040%20submachine%20gun/MP%2040%2C%20Handling%2C%20Cocking%2C%20MKH416.wav"
    },
    {
        "category": "Weapons/Reloads",
        "filename": "A_Reload_Phase4_BoltCatch_Release.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Pole%20Position%20-%20Tokarev%20PPSh-41%20submachine%20gun/PPSh41%2C%20Handling%2C%20Pulling%20Back%20Bolt%2C%20MKH416.wav"
    },
    {
        "category": "Weapons/Reloads",
        "filename": "A_Weapon_DryFire_Click.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Pole%20Position%20-%20Thompson%201928A1%20automatic%20submachine%20gun/1928A1%2C%20Handling%2C%20Dry%20Fire%2C%20MKH416.wav"
    },

    # 3. Ballistics & Combat Impact Foley
    {
        "category": "Combat",
        "filename": "A_Bullet_Flyby_Crack.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202018%20-%20Game%20Audio%20Bundle/Gamemaster%20Audio%20-%20Guns%2C%20Bullets%20and%20Explosions/bullet_flyby_fast_06.wav"
    },
    {
        "category": "Combat",
        "filename": "A_Shell_Casing_Drop.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202018%20-%20Game%20Audio%20Bundle/Gamemaster%20Audio%20-%20Silenced%20Gun%20Sounds/bullet_shell_bounce_wood2_05.wav"
    },
    {
        "category": "Combat",
        "filename": "A_Bullet_Impact_Concrete_Hard.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202018%20-%20Game%20Audio%20Bundle/Gamemaster%20Audio%20-%20Guns%2C%20Bullets%20and%20Explosions/bullet_impact_ice_06.wav"
    },

    # 4. Environmental Soundscapes (Wind & Rain)
    {
        "category": "Ambience/Wind",
        "filename": "A_Wind_Desert_Gusts_Loop.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202018%20-%20Game%20Audio%20Bundle/Discover%20Oregon%20-%20Wind%20and%20Storms/Wind%20Gusts%202016%20Lookout%20Snow%20-%20Biggest%20Gusts%20Long.wav"
    },
    {
        "category": "Ambience/Wind",
        "filename": "A_Wind_Tactical_Patagonia_Mountain_Loop.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Articulated%20Sounds%20-%20Rare%20Winds/WEATHER%20WIND%20Mountain%20Slope%2C%20Strong%2C%20Inside%20Jacket%20Hood%2C%20Sustained%20Blustery%2C%20Gusty%2C%20Binaural%2C%20Patagonia%2C%20LOOP.wav"
    },
    {
        "category": "Ambience/Rain",
        "filename": "A_Rain_Urban_Rooftop_Steady_Loop.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202018%20-%20Game%20Audio%20Bundle/GainWalkers%20-%20Urban%20Rain/RAIN_Exterior%2C%20Rooftop%2C%20Medium%2C%20Pavement%2C%20Steady%2C%20City%20Rumble_Stereo.wav"
    },
    {
        "category": "Ambience/Rain",
        "filename": "A_Rain_MetalPlate_Asynchronous_Drips_Loop.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Articulated%20Sounds%20-%20Moody%20Rain%20Loops/WEATHER%20Rain%2C%20Small%2C%20Designed%2C%20Metal%20Plate%2C%20Asynchronous%20Drips%2C%20LOOP.wav"
    },

    # 5. Authentic Physical Surface Footsteps
    # Surface: Concrete / Asphalt
    {
        "category": "Footsteps/Concrete",
        "filename": "FS_Concrete_01.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Soldier%20Footsteps/FS%20Concrete%20Soldier%20Crouch%20N02.wav"
    },
    {
        "category": "Footsteps/Concrete",
        "filename": "FS_Concrete_02.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Soldier%20Footsteps/FS%20Concrete%20Soldier%20Crouch%20N05.wav"
    },
    {
        "category": "Footsteps/Concrete",
        "filename": "FS_Concrete_Run.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202017%20-%20Game%20Audio%20Bundle/Tovusound%20-%20Edward%20%E2%80%93%20Foleyart%20Collection%20Add-On%20Extended%20Footsteps/015_Foley_Footsteps_Asphalt_Boot_Walk_Fast_Run_Jog_Close.wav"
    },

    # Surface: Wood
    {
        "category": "Footsteps/Wood",
        "filename": "FS_Wood_01.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Civilian%20Footsteps/FS%20Wood%20Civilian%20Crouch%20N03.wav"
    },
    {
        "category": "Footsteps/Wood",
        "filename": "FS_Wood_02.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Civilian%20Footsteps/FS%20Wood%20Civilian%20Crouch%20N05.wav"
    },

    # Surface: Sand / Dirt
    {
        "category": "Footsteps/Sand",
        "filename": "FS_Sand_01.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Civilian%20Footsteps/FS%20Ground%20Civilian%20Walk%20N03.wav"
    },
    {
        "category": "Footsteps/Sand",
        "filename": "FS_Sand_02.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Civilian%20Footsteps/FS%20Ground%20Civilian%20Walk%20N05.wav"
    },
    {
        "category": "Footsteps/Sand",
        "filename": "FS_Sand_Jump.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202017%20-%20Game%20Audio%20Bundle/Tovusound%20-%20Edward%20%E2%80%93%20Foleyart%20Collection%20Add-On%20Extended%20Footsteps/289_Foley_Footsteps_Rocks_Sneaker_Jump_Land_On_Two_Feet_Close.wav"
    },

    # Surface: Water / Puddle
    {
        "category": "Footsteps/Water",
        "filename": "FS_Water_Puddle_01.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Wav%20Junction%20Sound%20Effects%20-%20Footsteps/0014_Footsteps_water_puddle_single_splashes.wav"
    },

    # Surface: Metal
    {
        "category": "Footsteps/Metal",
        "filename": "FS_Metal_01.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Soldier%20Footsteps/FS%20Metal%20Soldier%20Walk%20N01.wav"
    },
    {
        "category": "Footsteps/Metal",
        "filename": "FS_Metal_02.wav",
        "url": "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Soldier%20Footsteps/FS%20Metal%20Soldier%20Walk%20N05.wav"
    }
]

def download_all():
    base_dir = os.path.abspath("Content/Audio")
    print(f"Downloading {len(AUDIO_TARGETS)} curated royalty-free audio tracks into {base_dir}...\n")
    
    downloaded = 0
    skipped = 0
    total_bytes = 0
    for item in AUDIO_TARGETS:
        target_dir = os.path.join(base_dir, item["category"])
        os.makedirs(target_dir, exist_ok=True)
        dest_path = os.path.join(target_dir, item["filename"])
        
        # Skip if already downloaded and non-empty
        if os.path.exists(dest_path) and os.path.getsize(dest_path) > 1000:
            print(f"[{item['category']}] -> {item['filename']} (Already exists: {os.path.getsize(dest_path):,} bytes)")
            skipped += 1
            total_bytes += os.path.getsize(dest_path)
            continue
            
        print(f"[{item['category']}] -> {item['filename']}...")
        try:
            req = urllib.request.Request(item["url"], headers=HEADERS)
            with urllib.request.urlopen(req, timeout=35) as resp, open(dest_path, "wb") as f:
                data = resp.read()
                f.write(data)
                downloaded += 1
                total_bytes += len(data)
                print(f"   [OK] {len(data):,} bytes")
        except Exception as e:
            print(f"   [ERROR] Failed to download {item['url']}: {e}")
            
    print(f"\nFinished! Processed {len(AUDIO_TARGETS)} files ({downloaded} newly downloaded, {skipped} existing). Total size: {total_bytes / (1024*1024):.2f} MB.")

if __name__ == '__main__':
    download_all()
