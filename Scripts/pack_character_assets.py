import os
import subprocess
import sys

project_root = r"C:\Users\Boris\Documents\antigravity\noble-fermi"
unreal_pak = r"D:\epuc\UE_5.8\Engine\Binaries\Win64\UnrealPak.exe"
pak_output = os.path.join(project_root, "Packaged", "Windows", "NobleFermi", "Content", "Paks", "NobleFermi-Windows_0_P.pak")
response_file = os.path.join(project_root, "Intermediate", "pak_response.txt")

os.makedirs(os.path.dirname(response_file), exist_ok=True)
os.makedirs(os.path.dirname(pak_output), exist_ok=True)

search_dirs = [
    os.path.join(project_root, "Content", "Characters", "BaseMale"),
    os.path.join(project_root, "Content", "Audio")
]

lines = []
for search_dir in search_dirs:
    for root, dirs, files in os.walk(search_dir):
        for f in files:
            if f.endswith(".uasset") or f.endswith(".uexp") or f.endswith(".ubulk"):
                full_path = os.path.join(root, f)
                rel_path = os.path.relpath(full_path, os.path.join(project_root, "Content"))
                rel_path_ue = rel_path.replace("\\", "/")
                lines.append(f'"{full_path}" "../../../NobleFermi/Content/{rel_path_ue}"')

with open(response_file, "w", encoding="utf-8") as rf:
    rf.write("\n".join(lines) + "\n")

print(f"Created response file with {len(lines)} assets: {response_file}")

if os.path.exists(pak_output):
    os.remove(pak_output)

cmd = [unreal_pak, pak_output, f"-create={response_file}", "-compress"]
print("Running:", " ".join(cmd))
res = subprocess.run(cmd, capture_output=True, text=True)
print("UnrealPak ReturnCode:", res.returncode)
if res.stdout:
    print(res.stdout[-1500:])
if res.stderr:
    print("Stderr:", res.stderr)

if res.returncode == 0:
    print(f"SUCCESS: Created patch pak at {pak_output} (Size: {os.path.getsize(pak_output)} bytes)")
else:
    print("ERROR: UnrealPak failed!")
    sys.exit(res.returncode)
