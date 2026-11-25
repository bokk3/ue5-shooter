# Handoff: Trellis asset batch, 2026-11-25

You're taking over the Noble Fermi project in `D:\epuc\Projects\noble-fermi` (UE 5.8 at `D:\epuc\UE_5.8`), branch
`feat/pvp-foundation`. A side session finished the generated-asset work below. Nothing from it is committed yet.

## What's done

- **Asset Studio client.** `Scripts/Trellis/studio.py` talks to the Asset Studio web API, so nobody needs the
  browser any more. It can check status, submit prompts, retry, cancel and fetch.
  - The token lives in `%USERPROFILE%\.nf_studio_token`. Never print it or put it in a command line.
  - `fetch` checks each model's size before downloading, resumes a broken download, and validates the GLB.
- **Sorting.** `Scripts/Trellis/sort_packs.py --merge` adds a downloaded pack to `SourceArt/Trellis/manifest.json`
  without overwriting existing models. A new model for an asset that already has a file becomes `<Asset>_vN`.
- **14 missing models generated, 10 imported into UE.** They are at real scale, in
  `/Game/NobleFermi/Generated/.../SM_<Name>`:
  - Holographic, Scope_4x, Magnifier_3x, MuzzleBrake, LaserModule
  - TireStack, BoatRope_Coil, SandbagWall, SandbagStack
  - Pallet_Boxes, which is an empty pallet: the version with boxes kept timing out on the server.
- **In total, 152 of the 160 prompts in `prompts.py` are imported.** The other 8 are in `asset_specs.REJECTED`
  with the reason for each:
  - Speedloader_357, BattleBelt, DropLegHolster
  - FlashHider, Grip_Vertical, Grip_Angled
  - Container40_Blue, Container_Open
  - The user decided to leave these for now.
- **Docs updated.**
  - `Scripts/Trellis/README.md`: studio.py usage, server limits, prompt lessons.
  - `SourceArt/Trellis/README.md`: per-asset status.
  - `Docs/AGENT_STARTER_PROMPT_AND_PROJECT_STATE.md`: new section "Generated 3D assets (Trellis)".

## What's open

1. **Commit, when the user asks.** Ask before you commit or push.
   - Files from this work:
     - `Scripts/Trellis/` (untracked)
     - `Content/NobleFermi/Generated/` (untracked, 4.1 GB; `*.uasset` goes to LFS via `.gitattributes`)
     - `Docs/AGENT_STARTER_PROMPT_AND_PROJECT_STATE.md`
     - this file
   - Leave out the user's other changes in the working tree: `Config/DefaultEngine.ini`, `functions/`, `web/` and
     `wrangler.toml`. They are not part of this work.
   - `SourceArt/` is meant to stay out of git (several GB).
   - Check with the user whether 4.1 GB of LFS content is OK to push.
2. **Rebuild `L_AssetGallery`** so it shows the 10 new meshes. It lays out every imported mesh:
   ```powershell
   & 'D:\epuc\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' "$PWD\NobleFermi.uproject" -run=pythonscript -script="$PWD\Scripts\Trellis\create_gallery_map.py" -unattended -nullrhi
   ```
3. **Optional: place the new cover props.** SandbagWall, SandbagStack, TireStack and Pallet_Boxes are ready to
   place in `L_Range` or the arena.
4. **Not started:**
   - The 12 stylized, lower-poly attachments listed in `SourceArt/Trellis/README.md`, to regenerate at 30k without
     the style.
   - The 8 rejected assets.

## How to work with the pipeline

- **Commands** (see `Scripts/Trellis/README.md`):
  ```powershell
  python -I Scripts\Trellis\studio.py status
  python -I Scripts\Trellis\studio.py submit --prompt "Asset=shape description"
  python -I -u Scripts\Trellis\studio.py fetch --wait --asset Asset --name <pack>
  python -I Scripts\Trellis\sort_packs.py SourceArt\Trellis --merge SourceArt\Trellis\_downloads\<pack>
  ```
- **Before adding specs,** render and check every model:
  - `render_views.py` (Blender at `C:\Users\Boris\tools\blender\blender.exe`), then `sheet.py`. Run `sheet.py`
    without `-I`; it needs PIL from the user site-packages.
  - Then add a spec to `asset_specs.py`, run `clean_assets.py`, then the `import_assets.py` commandlet with
    `NF_TRELLIS_ONLY=...`.
- **Server limits.** The server runs one job at a time. It stops a job after 30 minutes on Ultra and 15 on High.
  Scenes with many separate parts don't finish: simplify the object rather than lowering the preset.
- **Prompt pitfalls.** The image model takes words literally:
  - "grip" or "foregrip" gives a pistol.
  - "birdcage" gives a birdcage.
  - Describe the shape instead, and add "isolated".
- **Known log noise.** The `FPSFermiDecoyActor` CDO errors about the missing `Anim_BaseMale_Run`/`_Sprint` show up
  in every commandlet run. They have nothing to do with the asset import.
