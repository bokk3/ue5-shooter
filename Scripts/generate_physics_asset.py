import unreal
import sys

out = []
def log(msg):
    out.append(str(msg))
    unreal.log(str(msg))

sk_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
log(f"Loaded SK_BaseMale: {sk_mesh}")

# Create Physics Asset
phys_asset = unreal.EditorSkeletalMeshLibrary.create_physics_asset(sk_mesh)
log(f"create_physics_asset result: {phys_asset}")

if not phys_asset:
    # Try SkeletalMeshEditorSubsystem
    subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    if subsystem:
        log(f"Found SkeletalMeshEditorSubsystem: {subsystem}")
        phys_asset = subsystem.create_physics_asset(sk_mesh)
        log(f"Subsystem create_physics_asset result: {phys_asset}")

if phys_asset:
    log(f"Successfully generated Physics Asset: {phys_asset.get_path_name()}")
    sk_mesh.set_editor_property("physics_asset", phys_asset)
    
    # Save the physics asset and skeletal mesh
    unreal.EditorAssetLibrary.save_loaded_asset(phys_asset)
    unreal.EditorAssetLibrary.save_loaded_asset(sk_mesh)
    unreal.EditorAssetLibrary.save_directory("/Game/Characters/BaseMale", only_if_is_dirty=False, recursive=True)

# List assets in /Game/Characters/BaseMale
assets = unreal.EditorAssetLibrary.list_assets("/Game/Characters/BaseMale", recursive=True)
log(f"Current Assets in /Game/Characters/BaseMale: {assets}")

with open(r"C:\Users\Boris\Documents\antigravity\noble-fermi\Scripts\physics_asset_created.txt", "w") as f:
    f.write("\n".join(out))

sys.exit(0)
