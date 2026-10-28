"""
Noble Fermi // Open-Source Weapon Asset Downloader
Downloads CC0/CC-BY tactical weapon base models for the 11-weapon arsenal.
"""
import os
import urllib.request

RAW_DIR = os.path.join("SourceArt", "Weapons", "Raw")
os.makedirs(RAW_DIR, exist_ok=True)

BASE_QUATERNIUS_GLB = "https://raw.githubusercontent.com/trebeljahr/quaternius-showcase/main/public/glb/"

# 11 Weapon Source Mapping:
# 1. NF-98 Carbine (5.56 Assault Rifle) -> modular_sci_fi_guns_pack/AR_1.glb & Magazine_AR.glb
# 2. VK-74 Vektor (7.62 Assault Rifle)  -> modular_sci_fi_guns_pack/AR_4.glb & Magazine_AR.glb
# 3. Falcon-50 DMR (7.62 DMR / Sniper)  -> modular_sci_fi_guns_pack/Sniper_1.glb & Magazine_Sniper_1.glb
# 4. Breacher-12 (Combat Shotgun)      -> survival_pack/Shotgun_1.glb
# 5. Apex-9 (9mm Striker Pistol)       -> survival_pack/Pistol_1.glb & modular_sci_fi_guns_pack/Body_Pistol_1.glb
# 6. Spectre-45 (.45 Tactical Pistol)  -> survival_pack/Pistol_2.glb
# 7. Kestrel-357 (Magnum Revolver)     -> survival_pack/Revolver_1.glb
# 8. M67 Frag Grenade                  -> modular_sci_fi_guns_pack/Grenade_1.glb
# 9. Stun-9 Flashbang                  -> modular_sci_fi_guns_pack/Grenade.glb
# 10. Favela Bolo Machete              -> survival_pack/Knife.glb
# 11. Shadow Stiletto                  -> medieval_weapons_pack/Dagger.glb

DOWNLOADS = [
    # Rifles & Mags
    ("modular_sci_fi_guns_pack/AR_1.glb", "AR_Carbine.glb"),
    ("modular_sci_fi_guns_pack/AR_4.glb", "AR_Vektor.glb"),
    ("modular_sci_fi_guns_pack/Sniper_1.glb", "Rifle_DMR.glb"),
    ("modular_sci_fi_guns_pack/Magazine_AR.glb", "Mag_STANAG.glb"),
    ("modular_sci_fi_guns_pack/Magazine_Sniper_1.glb", "Mag_DMR.glb"),
    # Shotgun
    ("survival_pack/Shotgun_1.glb", "Shotgun_Breacher.glb"),
    # Pistols
    ("survival_pack/Pistol_1.glb", "Pistol_Apex9.glb"),
    ("survival_pack/Pistol_2.glb", "Pistol_Spectre45.glb"),
    ("survival_pack/Revolver_1.glb", "Pistol_Kestrel357.glb"),
    # Throwables
    ("modular_sci_fi_guns_pack/Grenade_1.glb", "Grenade_M67.glb"),
    ("modular_sci_fi_guns_pack/Grenade.glb", "Grenade_Flashbang.glb"),
    # Melee Blades
    ("survival_pack/Knife.glb", "Blade_Machete.glb"),
    ("medieval_weapons_pack/Dagger.glb", "Blade_Stiletto.glb"),
]

def main():
    print(f"[DOWNLOAD] Starting download of {len(DOWNLOADS)} base weapon models into {RAW_DIR}...")
    headers = {'User-Agent': 'Mozilla/5.0'}
    for subpath, outname in DOWNLOADS:
        dst = os.path.join(RAW_DIR, outname)
        if os.path.exists(dst) and os.path.getsize(dst) > 1000:
            print(f"  [OK] Already exists: {outname} ({os.path.getsize(dst)} bytes)")
            continue
        url = BASE_QUATERNIUS_GLB + subpath
        print(f"  [FETCH] {url} -> {outname}")
        req = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(req) as resp, open(dst, "wb") as f:
            f.write(resp.read())
        print(f"  [SUCCESS] {outname} ({os.path.getsize(dst)} bytes)")
    print("[DOWNLOAD] All weapon base models downloaded successfully.")

if __name__ == "__main__":
    main()
