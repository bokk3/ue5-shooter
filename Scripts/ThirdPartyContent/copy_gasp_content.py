"""Copy the Game Animation Sample assets Noble Fermi needs into Content/.

Usage:
    python copy_gasp_content.py <gasp_deps.json> [<GameAnimationSample Content dir>]

<gasp_deps.json> is produced by gasp_list_dependencies.py (see README.md).
"""
import json
import os
import shutil
import sys

DEFAULT_SAMPLE_CONTENT = r"D:\epuc\Projects\GameAnimationSample\Content"
PROJECT_CONTENT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "Content"))


def main():
    deps = json.load(open(sys.argv[1]))
    sample = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_SAMPLE_CONTENT
    copied, total = 0, 0
    for pkg in deps["hard"]:
        rel = pkg[len("/Game/"):]
        for ext in (".uasset", ".umap", ".uexp", ".ubulk"):
            src = os.path.join(sample, rel + ext)
            if os.path.exists(src):
                dst = os.path.join(PROJECT_CONTENT, rel + ext)
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.copy2(src, dst)
                copied += 1
                total += os.path.getsize(src)
    print(f"Copied {copied} files ({total / 2**20:.0f} MB) into {PROJECT_CONTENT}")


if __name__ == "__main__":
    main()
