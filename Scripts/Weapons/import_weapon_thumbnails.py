import unreal
import os
import glob

def log(msg):
    unreal.log_warning(f"[WEAPON_THUMBNAIL_IMPORT] {msg}")

ROOT = r"C:\Users\Boris\Documents\antigravity\noble-fermi"
SRC_DIR = os.path.join(ROOT, "Content", "Weapons", "Thumbnails")
DEST_PATH = "/Game/Weapons/Thumbnails"
DATA_PATH = "/Game/Weapons/Data"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

log("Starting import of Weapon Thumbnails...")

png_files = glob.glob(os.path.join(SRC_DIR, "*.png"))
log(f"Found {len(png_files)} thumbnail PNGs to import.")

for png in png_files:
    fname = os.path.basename(png)
    asset_name = os.path.splitext(fname)[0]
    
    task = unreal.AssetImportTask()
    task.filename = png
    task.destination_path = DEST_PATH
    task.destination_name = asset_name
    task.replace_existing = True
    task.automated = True
    task.save = True
    
    asset_tools.import_asset_tasks([task])
    
    tex = eal.load_asset(f"{DEST_PATH}/{asset_name}")
    if tex:
        log(f"Successfully imported texture: {DEST_PATH}/{asset_name}")
        eal.save_loaded_asset(tex)
    else:
        log(f"FAILED to load imported texture: {asset_name}")

# Now assign the imported textures to the corresponding DataAssets
data_assets = {
    "W01_NF98": "DA_W01_NF98",
    "W02_VK74": "DA_W02_VK74",
    "W03_Falcon50": "DA_W03_Falcon50",
    "W04_Breacher12": "DA_W04_Breacher12",
    "W05_Apex9": "DA_W05_Apex9",
    "W06_Spectre45": "DA_W06_Spectre45",
    "W07_Kestrel357": "DA_W07_Kestrel357",
    "W08_GrenadeM67": "DA_W08_GrenadeM67",
    "W09_Stun9": "DA_W09_Stun9",
    "W10_Machete": "DA_W10_Machete",
    "W11_Stiletto": "DA_W11_Stiletto"
}

for tex_name, da_name in data_assets.items():
    da_path = f"{DATA_PATH}/{da_name}"
    tex_path = f"{DEST_PATH}/{tex_name}"
    
    da = eal.load_asset(da_path)
    tex = eal.load_asset(tex_path)
    
    if da and tex:
        da.set_editor_property("weapon_icon", tex)
        eal.save_loaded_asset(da)
        log(f"Assigned {tex_name} -> {da_name}.weapon_icon")
    else:
        log(f"Warning: DA ({da}) or Tex ({tex}) missing for {da_name}")

log("Weapon Thumbnails import and assignment complete!")
