"""Extract Trellis asset-studio packs, match each model to its prompt, sort into a working folder, report gaps.

usage: python -I sort_packs.py <out_dir> [--merge] <pack.zip | pack folder> [...]
Files in a pack are named <prompt slug, 40 chars>-<hash>.<ext>; the slug finds the prompt.
A pack folder (what studio.py fetch writes) is read in place; its manifest may name each model's asset.
--merge adds to the existing <out_dir>/manifest.json: assets these packs lack keep their row, and a new model for an
asset that already has a file is written as the next free <Asset>_vN instead of overwriting it.
"""
import json
import os
import re
import shutil
import struct
import sys
import zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from prompts import PROMPTS  # noqa: E402

MIN_GLB_BYTES = 200_000  # a textured Trellis model is ~8-14 MB; anything tiny is a failed generation


def norm(text):
    return re.sub(r"[^a-z0-9]+", " ", text.lower()).strip()


def slug(text, n=40):
    return re.sub(r"[^a-z0-9]+", "-", text.lower())[:n].strip("-")


def glb_info(path):
    """(triangles, image count, problem or None)."""
    try:
        with open(path, "rb") as f:
            data = f.read()
        if data[:4] != b"glTF":
            return 0, 0, "not a GLB"
        n = struct.unpack_from("<I", data, 12)[0]
        g = json.loads(data[20:20 + n])
        tris = 0
        for m in g.get("meshes", []):
            for p in m.get("primitives", []):
                if "indices" in p:
                    tris += g["accessors"][p["indices"]]["count"] // 3
                else:
                    tris += g["accessors"][p["attributes"]["POSITION"]]["count"] // 3
        imgs = len(g.get("images", []))
        if tris == 0:
            return tris, imgs, "no geometry"
        if imgs == 0:
            return tris, imgs, "untextured"
        return tris, imgs, None
    except Exception as e:  # noqa: BLE001
        return 0, 0, f"unreadable ({e})"


def free_base(folder, base):
    n, cand = 1, base
    while os.path.exists(os.path.join(folder, cand + ".glb")):
        n += 1
        cand = f"{base}_v{n}"
    return cand


def main():
    merge = "--merge" in sys.argv[1:]
    argv = [a for a in sys.argv[1:] if a != "--merge"]
    out = os.path.abspath(argv[0])
    packs = argv[1:]
    old = {"assets": [], "unmatched": []}
    if merge and os.path.isfile(os.path.join(out, "manifest.json")):
        with open(os.path.join(out, "manifest.json"), encoding="utf-8") as f:
            old = json.load(f)
    prev = {r["asset"]: r for r in old.get("assets", [])}
    names = {p[2] for p in PROMPTS}
    by_slug = {}
    for p in PROMPTS:
        by_slug.setdefault(slug(p[3]), []).append(p)
    raw = os.path.join(out, "_raw")
    os.makedirs(raw, exist_ok=True)

    found = {}       # slug -> list of (glb path, stem), when the manifest can't say
    found_exact = {}  # asset name -> list of (glb path, stem), matched on the manifest prompt
    unmatched = []
    for pack in packs:
        if os.path.isdir(pack):
            dest = os.path.abspath(pack)
        else:
            dest = os.path.join(raw, os.path.splitext(os.path.basename(pack))[0])
            with zipfile.ZipFile(pack) as z:
                for info in z.infolist():
                    name = os.path.basename(info.filename)
                    if not name or ".." in info.filename:
                        continue
                    target = os.path.join(dest, name)
                    os.makedirs(dest, exist_ok=True)
                    with z.open(info) as src, open(target, "wb") as dst:
                        shutil.copyfileobj(src, dst)
        # The pack manifest has each model's full prompt (and from studio.py its asset); match on that,
        # fall back to the filename slug.
        manifest_prompt, manifest_asset = {}, {}
        mpath = os.path.join(dest, "manifest.json")
        if os.path.isfile(mpath):
            with open(mpath, encoding="utf-8") as f:
                for a in json.load(f).get("assets", []):
                    manifest_prompt[a.get("glb")] = a.get("prompt") or ""
                    manifest_asset[a.get("glb")] = a.get("asset")
        for name in sorted(os.listdir(dest)):
            stem, ext = os.path.splitext(name)
            if ext.lower() != ".glb":
                continue
            if manifest_asset.get(name) in names:
                found_exact.setdefault(manifest_asset[name], []).append((os.path.join(dest, name), stem))
                continue
            key = re.sub(r"-[0-9a-f]{8}$", "", stem)
            full = norm(manifest_prompt.get(name, ""))
            exact = [p for p in PROMPTS if full and full.startswith(norm(p[3]))]
            if exact:
                best = max(exact, key=lambda p: len(p[3]))
                found_exact.setdefault(best[2], []).append((os.path.join(dest, name), stem))
                continue
            if key not in by_slug:
                unmatched.append(f"{os.path.basename(pack)}: {name}")
                continue
            found.setdefault(key, []).append((os.path.join(dest, name), stem))

    # Prompts that share a 40-char slug split that slug's files in order; their names need a visual check.
    by_asset, shared = {k: list(v) for k, v in found_exact.items()}, set()
    for key, files in found.items():
        owners = by_slug[key]
        for i, f in enumerate(files):
            by_asset.setdefault(owners[min(i, len(owners) - 1)][2], []).append(f)
        if len(owners) > 1:
            shared.update(o[2] for o in owners)

    rows = []
    for batch, cat, asset, prompt in PROMPTS:
        entries = by_asset.get(asset, [])
        if merge and not entries and asset in prev:
            rows.append(prev[asset])
            continue
        status, note = (prev[asset]["status"], prev[asset]["note"]) if merge and asset in prev else ("missing", "")
        written = []
        for i, (glb, stem) in enumerate(entries):
            size = os.path.getsize(glb)
            tris, imgs, problem = glb_info(glb)
            if size < MIN_GLB_BYTES and not problem:
                problem = f"tiny file ({size} bytes)"
            folder = os.path.join(out, batch, cat)
            os.makedirs(folder, exist_ok=True)
            if merge:
                base = free_base(folder, asset if not problem else f"_FAILED_{asset}")
            else:
                suffix = "" if i == 0 else f"_v{i + 1}"
                base = f"{asset}{suffix}" if not problem else f"_FAILED_{asset}{suffix}"
            shutil.copy2(glb, os.path.join(folder, base + ".glb"))
            for ext in (".png", ".jpg", ".webp"):
                preview = os.path.splitext(glb)[0] + ext
                if os.path.isfile(preview):
                    shutil.copy2(preview, os.path.join(folder, base + ext))
                    break
            if problem:
                status, note = ("failed", problem) if status != "ok" else (status, note)
            elif merge:
                written.append(base)
                status, note = "ok", f"{tris} tris, {imgs} textures"
            else:
                status, note = "ok", f"{tris} tris, {imgs} textures" + (f", {len(entries)} versions" if len(entries) > 1 else "")
        if written and written != [asset]:
            note += "; new: " + ", ".join(written) + " (compare with the existing file)"
        if asset in shared and status == "ok":
            note += "; shares a filename slug with another prompt, check the preview matches"
        rows.append({"batch": batch, "category": cat, "asset": asset, "status": status, "note": note, "prompt": prompt})

    with open(os.path.join(out, "manifest.json"), "w", encoding="utf-8") as f:
        if merge:
            unmatched = old.get("unmatched", []) + [u for u in unmatched if u not in old.get("unmatched", [])]
        json.dump({"assets": rows, "unmatched": unmatched}, f, indent=1)
    for r in rows:
        print(f"{r['status']:8}|{r['batch']}|{r['category']}|{r['asset']}|{r['note']}|{r['prompt']}")
    for u in unmatched:
        print(f"unmatched|{u}")


main()
