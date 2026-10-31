import os
import subprocess
import time

packaged_dir = os.path.abspath(r"Packaged\Windows")
exe_path = os.path.join(packaged_dir, "NobleFermi.exe")

def run_shot(args_list, shot_name, wait_seconds=25):
    print(f"--- Taking screenshot: {shot_name} ---")
    cmd = [exe_path] + args_list
    print("Running:", " ".join(cmd))
    p = subprocess.Popen(cmd, cwd=packaged_dir)
    
    start_time = time.time()
    while time.time() - start_time < wait_seconds:
        if p.poll() is not None:
            print(f"Process exited cleanly with code {p.returncode}")
            break
        time.sleep(1)
        
    if p.poll() is None:
        print("Terminating process after timeout...")
        p.terminate()
        try:
            p.wait(timeout=5)
        except:
            p.kill()

    # Search for the screenshot file
    possible_paths = [
        os.path.join(packaged_dir, "NobleFermi", "Docs", "Screenshots", shot_name),
        os.path.join(os.getcwd(), "Docs", "Screenshots", shot_name),
        os.path.join(packaged_dir, "Docs", "Screenshots", shot_name),
        os.path.join(packaged_dir, "NobleFermi", "Saved", "Screenshots", "Windows", shot_name)
    ]
    found = None
    for path in possible_paths:
        if os.path.exists(path):
            found = path
            break
            
    if found:
        print(f"FOUND: {found} (Size: {os.path.getsize(found)} bytes)")
        dest_brain = os.path.join(r"C:\Users\Boris\.gemini\antigravity\brain\c135ce77-4bef-4380-b69a-99d73fc7b5a7", shot_name)
        dest_docs = os.path.join(os.getcwd(), "Docs", "Screenshots", shot_name)
        
        # Copy to brain and docs
        import shutil
        shutil.copy2(found, dest_brain)
        shutil.copy2(found, dest_docs)
        print(f"Copied to {dest_brain} and {dest_docs}")
        return True
    else:
        print(f"Screenshot {shot_name} not found in any expected location!")
        return False

# 1. Menu Hangar Actual
run_shot([
    "-TakeMenuShot",
    "-MenuShotDelay=5.0",
    "-ShotName=actual_inengine_hangar_menu_phase5.png",
    "-windowed",
    "-ResX=1920",
    "-ResY=1080",
    "-log"
], "actual_inengine_hangar_menu_phase5.png", wait_seconds=30)

# 2. Gunsmith / Loadout Actual
run_shot([
    "-TakeMenuShot",
    "-MenuShotDelay=6.0",
    "-MenuSection=Gunsmith",
    "-ShotName=actual_inengine_gunsmith_phase5.png",
    "-windowed",
    "-ResX=1920",
    "-ResY=1080",
    "-log"
], "actual_inengine_gunsmith_phase5.png", wait_seconds=30)

# 3. Gameplay Coastal Actual
run_shot([
    "?game=/Script/NobleFermi.FPSMultiplayerGameMode",
    "-TakeGameplayShot",
    "-GameplayShotDelay=6.0",
    "-ActiveTheater=1",
    "-ShotName=actual_inengine_coastal_gameplay_phase5.png",
    "-windowed",
    "-ResX=1920",
    "-ResY=1080",
    "-log"
], "actual_inengine_coastal_gameplay_phase5.png", wait_seconds=35)
