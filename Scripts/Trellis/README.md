# Trellis asset pipeline

This pipeline turns Trellis 2 asset-studio packs into static meshes in `/Game/NobleFermi/Generated`, at real-world scale with forward on +X and up on +Z.

| Step | Script | Runs in |
|---|---|---|
| 0. Generate on the Asset Studio server and download the new models | `studio.py` | Python |
| 1. Sort the packs, match each model to its prompt, list the gaps | `sort_packs.py` (+ `prompts.py`) | Python |
| 2. Render front / right / top contact sheets to check models | `render_views.py`, `sheet.py` | Blender, Python |
| 3. Orient, scale, set the pivot, remove floating debris | `clean_assets.py` (+ `asset_specs.py`) | Blender |
| 4. Import; set Nanite, collision and texture caps | `import_assets.py` (+ `asset_specs.py`) | UE commandlet |

```powershell
python sort_packs.py SourceArt\Trellis <pack.zip> ...
blender -b --factory-startup -P Scripts\Trellis\clean_assets.py -- D:\...\SourceArt\Trellis D:\...\SourceArt\Trellis\_Clean [Name ...]
$env:NF_TRELLIS_CLEAN='D:\...\SourceArt\Trellis\_Clean'; UnrealEditor-Cmd.exe NobleFermi.uproject -run=pythonscript -script=Scripts\Trellis\import_assets.py -unattended -nullrhi
```

- **Selecting assets:** use `NF_TRELLIS_ONLY=Name1,Name2` to limit the import. Use `NF_TRELLIS_CONFIGURE_ONLY=1` to re-apply the import settings to meshes that are already imported, without importing them again.
- **Paths:** give Blender Windows paths (`D:/...`), not `/d/...`.

## Asset Studio (`studio.py`)

`studio.py` talks to the Asset Studio web API directly, so you don't need the browser. Put the access token in
`%USERPROFILE%\.nf_studio_token`, or set `NF_STUDIO_TOKEN`. `NF_STUDIO_URL` overrides the server address.

```powershell
python -I Scripts\Trellis\studio.py status                       # health, job counts, failed jobs
python -I Scripts\Trellis\studio.py submit --missing --dry-run   # every asset manifest.json does not mark ok
python -I Scripts\Trellis\studio.py submit --asset Holographic Scope_4x --variants 2
python -I Scripts\Trellis\studio.py submit --prompt "BeltOnly=padded tactical battle belt only, no vest"
python -I Scripts\Trellis\studio.py retry
python -I Scripts\Trellis\studio.py cancel --asset TireStack     # queued or running jobs for an asset
python -I Scripts\Trellis\studio.py fetch --wait --dry-run        # what would download, and how big
python -I Scripts\Trellis\studio.py fetch --max-mb 1500
python -I Scripts\Trellis\sort_packs.py SourceArt\Trellis --merge SourceArt\Trellis\_downloads\<pack>
```

How it keeps downloads small:
- **Quality preset.** Textures make up most of a GLB's size. `submit` defaults to Ultra (30k faces, 4K textures)
  for the cleanest meshes, which measured 18–50 MB per model. `--quality High` (20k faces, 2K textures) is several
  times smaller, and `import_assets.py` keeps at most 2K textures anyway. The server accepts only the four presets
  Fast, Standard, High and Ultra, not other face/texture combinations.
- **Only new models.** `fetch` skips every job whose id is already in `_pack_manifests/`, and every image or photo
  job that matches no prompt in `prompts.py` (`--all` includes those).
- **Size check first.** `fetch` checks each model's size before downloading, stops at `--max-mb`, and refuses to
  start with less than 1 GB of free disk left over.
- **Resume.** A download that breaks off resumes from where it stopped. Each GLB's header length is checked
  against the file before it counts as downloaded.

`fetch` writes a pack folder to `_downloads/<pack>` and copies its manifest to `_pack_manifests/`.
`submit` logs which asset each job belongs to in `_studio_submissions.json`, so a prompt with different wording
still lands on the right asset. `sort_packs.py --merge` reads that pack folder in place and keeps the rows for assets
it doesn't touch. A new model for an asset that already has a file becomes `<Asset>_vN`; pick the better one before
`clean_assets.py`. To use a `_vN`, rename the old file to `_REJECTED_<Asset>.glb` and the `_vN` to `<Asset>.glb`.

### Server limits and prompts that work

- **Time limits.** The server runs one job at a time and stops a job after 30 minutes on Ultra and 15 on High.
  Most models take 2–8 minutes. A job that keeps hitting the limit, or running out of GPU memory, needs a simpler
  object, not a lower preset. Scenes with many separate parts failed on every preset: a pallet with boxes, a full
  shipping container, a loose rope coil. "single neat coil of thick rope lying flat" and "single empty wooden
  shipping pallet" got through in under 10 minutes.
- **The image model takes words literally.** "birdcage flash hider" became a birdcage, and "pipe fitting" shapes come
  back for "muzzle brake". Any prompt with "grip" or "foregrip" came back as a pistol, even with "isolated". Describe
  the shape instead: "short black 4x rifle scope tube with two mount rings, scope only, isolated" worked.
- **Check every result.** Render a contact sheet (`render_views.py` + `sheet.py`) before you add a spec. A wrong
  object goes into `asset_specs.REJECTED` with the reason, so `clean_assets.py` and the import skip it.
- **Progress output.** `python -I` ignores `PYTHONUNBUFFERED`. Run a background `fetch --wait` with `python -I -u`
  to see its progress lines as they happen.

## Adding a model

Add a line to `asset_specs.py`. Trellis output is normalized to about 1 m in glTF space, with +Y up and the source image's view toward +Z. The yaw is arbitrary, so render the model first and read which way it faces:
- In the **front** view, image right is glTF +X.
- In the **right** view, image right is glTF −Z.
- In the **top** view, image right is +X and image up is −Z.

| Field | Values |
|---|---|
| `fwd` | A glTF axis (`"+X"`, `"-Z"`).<br>`"pca:+X+Y"` takes the longest axis, signed toward the hint; use it for blades lying at an angle.<br>`"pca_h:+X"` takes the longest horizontal axis. |
| `up` | Normally `"+Y"`.<br>`"flat:+Y"` uses the thinnest axis; use it for sheets generated tilted. |
| `size` | One real dimension in cm, after orienting: `L` (UE X), `W` (Y), `H` (Z) or `M` (the largest). |
| `kind` | `held` — pivot at the centre, no collision, no Nanite, 1K textures.<br>`pickup` — pivot at the bottom, box collision, 1K textures.<br>`prop` — pivot at the bottom, convex hulls, Nanite, 2K textures.<br>`big` — pivot at the bottom, complex-as-simple collision, Nanite, 2K textures. |

## How the axes map

- UE imports glTF `(x, y, z)` metres as `(x, z, y)` centimetres. glTF +Z (the image view) becomes UE +Y.
- Blender exports its `(x, y, z)` as glTF `(x, z, -y)`.

So a model facing Blender +X with +Z up arrives in UE facing +X with +Z up.

## Notes

- In the commandlet, `get_editor_subsystem(StaticMeshEditorSubsystem)` returns None. The deprecated `EditorStaticMeshLibrary` then does nothing without reporting it. `import_assets.py` creates the subsystem directly.
- The source packs, `SourceArt/Trellis` and `_Clean` are not in git; they total several GB.
