"""Copy the Lyra Starter Game assets Noble Fermi needs into Content/, keeping Lyra's /Game paths.

Usage:
    python copy_lyra_content.py <lyra_deps.json> [<LyraStarterGame Content dir>] [--dry-run]

<lyra_deps.json> comes from lyra_list_dependencies.py (see README.md). Left out on purpose:
  * Lyra's audio (sound notifies in the montages just go silent; we have our own weapon audio),
  * Blueprints and anything else that needs Lyra's C++ module (anim notifies, physical materials,
    abilities). Animation notifies that use Lyra classes load as empty and are ignored.
Existing files in Content/ are never overwritten.
"""
import json
import os
import shutil
import sys

DEFAULT_LYRA_CONTENT = r"D:\epuc\Projects\LyraStarterGame\Content"
PROJECT_CONTENT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "Content"))
EXCLUDE_PREFIXES = ("/Game/Audio/", "/Game/Characters/Heroes/Abilities/")
EXCLUDE_CLASSES = {"Blueprint", "AnimBlueprint", "WidgetBlueprint", "PhysicalMaterialWithTags"}
EXTS = (".uasset", ".umap", ".uexp", ".ubulk")


def wanted(pkg, deps):
    if pkg.startswith(EXCLUDE_PREFIXES):
        return False
    if deps["classes"].get(pkg) in EXCLUDE_CLASSES:
        return False
    return True


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    dry = "--dry-run" in sys.argv
    deps = json.load(open(args[0]))
    lyra = args[1] if len(args) > 1 else DEFAULT_LYRA_CONTENT
    copied, total, skipped, clashes = 0, 0, 0, []
    for pkg in deps["hard"]:
        if not wanted(pkg, deps):
            skipped += 1
            continue
        rel = pkg[len("/Game/"):]
        for ext in EXTS:
            src = os.path.join(lyra, rel + ext)
            if not os.path.exists(src):
                continue
            dst = os.path.join(PROJECT_CONTENT, rel + ext)
            if os.path.exists(dst):
                clashes.append(dst)
                continue
            if not dry:
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.copy2(src, dst)
            copied += 1
            total += os.path.getsize(src)
    verb = "Would copy" if dry else "Copied"
    print(f"{verb} {copied} files ({total / 2**20:.0f} MB) into {PROJECT_CONTENT}; "
          f"left out {skipped} packages; {len(clashes)} already existed (kept)")
    for c in clashes[:20]:
        print("  exists:", c)


if __name__ == "__main__":
    main()
