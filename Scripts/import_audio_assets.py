import unreal
import os

print(">>> IMPORTING NOBLE FERMI AUDIO ASSETS INTO UNREAL ENGINE 5.8 <<<")

project_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi"
audio_dir = os.path.join(project_dir, "Content", "Audio")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

tasks = []
imported_files = []

for root, dirs, files in os.walk(audio_dir):
    for f in sorted(files):
        if not f.endswith(".wav"):
            continue
        # Skip full takes / raw backups
        if "FullTake" in f or "Loop" in f and os.path.getsize(os.path.join(root, f)) > 20*1024*1024:
            # Skip massive 100MB ambiences for quick import, focus on combat SFX & footsteps
            continue
            
        full_path = os.path.join(root, f)
        rel_dir = os.path.relpath(root, audio_dir).replace("\\", "/")
        dest_path = f"/Game/Audio/{rel_dir}".rstrip("/")
        asset_name = os.path.splitext(f)[0]
        
        task = unreal.AssetImportTask()
        task.filename = full_path
        task.destination_path = dest_path
        task.destination_name = asset_name
        task.replace_existing = True
        task.automated = True
        task.save = True
        tasks.append(task)
        imported_files.append((f, dest_path))

print(f"Submitting {len(tasks)} audio import tasks...")
asset_tools.import_asset_tasks(tasks)

print("\nSaving imported audio assets...")
for f, dest in imported_files:
    asset_name = os.path.splitext(f)[0]
    asset_path = f"{dest}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.save_asset(asset_path)
        print(f"  [SAVED] {asset_path}")
    else:
        print(f"  [WARN] Asset not found at {asset_path}")

print(">>> AUDIO IMPORT COMPLETE! <<<")
