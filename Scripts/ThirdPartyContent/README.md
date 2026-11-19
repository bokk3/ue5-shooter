# Third-party content (not in git)

Noble Fermi uses assets from Epic's free **Game Animation Sample** (UE 5.8): the motion-matched character, animation blueprint and animation library on the UEFN Mannequin skeleton. Those assets are about 2 GB and are **not committed**; see `.gitignore`. Recreate them on any machine as follows.

1. Download the Game Animation Sample (Fab / Epic launcher) and create the project, for example at `D:\epuc\Projects\GameAnimationSample`.
2. List the hard dependencies of the CMC character and its animation blueprint by running a headless editor on the sample:

   ```powershell
   $env:GASP_DEPS_OUT = "$env:TEMP\gasp_deps.json"
   & "D:\epuc\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\epuc\Projects\GameAnimationSample\GameAnimationSample.uproject" -run=pythonscript -script="$PWD\Scripts\ThirdPartyContent\gasp_list_dependencies.py" -unattended -nullrhi
   ```

3. Copy them into `Content/`, keeping the sample's `/Game/...` paths so references stay valid:

   ```powershell
   python Scripts\ThirdPartyContent\copy_gasp_content.py "$env:TEMP\gasp_deps.json"
   ```

This brings in about 2,030 files (≈1.95 GB) under `Content/Blueprints`, `Content/Characters/UEFN_Mannequin`, `Content/Input`, `Content/Levels`, `Content/Audio/Foley`, `Content/Audio/Mix` and `Content/Misc`. The plugins they need are already enabled in `NobleFermi.uproject`.

4. Copy the UEFN Mannequin IK rig, which the retarget step below needs:

   ```powershell
   Copy-Item D:\epuc\Projects\GameAnimationSample\Content\Characters\UEFN_Mannequin\Rigs\IK_UEFN_Mannequin.uasset Content\Characters\UEFN_Mannequin\Rigs\
   ```

## Lyra Starter Game (weapon animations)

Rifle, pistol and shotgun animations (locomotion, aim offsets, fire, reload, equip, melee, grenade toss), hit reacts and deaths, and the three weapon meshes come from Epic's free **Lyra Starter Game** (UE 5.8). Lyra animates the UE5 Mannequin, so the animations are retargeted onto the UEFN Mannequin. Nothing here is committed either.

1. Create the Lyra project from the launcher, for example at `D:\epuc\Projects\LyraStarterGame`, and build its editor target once (it has C++ modules).
2. List what we use and its hard dependencies (about 510 packages):

   ```powershell
   $env:LYRA_DEPS_OUT = "$env:TEMP\lyra_deps.json"
   & "D:\epuc\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\epuc\Projects\LyraStarterGame\LyraStarterGame.uproject" -run=pythonscript -script="$PWD\Scripts\ThirdPartyContent\lyra_list_dependencies.py" -unattended -nullrhi
   ```

3. Copy them in under Lyra's own paths (about 435 files, 505 MB, under `Content/Characters/Heroes`, `Content/Weapons/{Rifle,Pistol,Shotgun,Generic}`, `Content/Effects`, `Content/PhysicsMaterials`). Lyra's audio, Blueprints and anything needing Lyra's C++ module are left out, and existing files are never overwritten:

   ```powershell
   python Scripts\ThirdPartyContent\copy_lyra_content.py "$env:TEMP\lyra_deps.json"
   ```

4. Retarget onto the UEFN Mannequin with `retarget_lyra_animations.py`, in **two passes**. In UE 5.8, Epic's batch retarget (`IKRetargetBatchOperation.duplicate_and_retarget`) hangs forever on Lyra's hit-react montages, and on any montage or blend space whose referenced sequences it pulls in. So:
   - **Pass 1** retargets the 280 sequences, about 3 minutes plus saving.
   - **Pass 2** retargets the montages and blend spaces one at a time, without their sequences, then merges each Lyra sequence into its UEFN copy so they all point at the retargeted ones. The 13 hit-react montages are skipped and rebuilt from their retargeted sequence instead (each plays a single sequence, in `DefaultSlot`).

   ```powershell
   $ue = "D:\epuc\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"; $s = "$PWD\Scripts\ThirdPartyContent\retarget_lyra_animations.py"
   $env:LYRA_RETARGET_CLEAN = "1"; $env:LYRA_RETARGET_CLASSES = "AnimSequence"
   & $ue NobleFermi.uproject -run=pythonscript -script="$s" -unattended -nullrhi
   $env:LYRA_RETARGET_CLEAN = ""; $env:LYRA_RETARGET_ONE_BY_ONE = "1"; $env:LYRA_RETARGET_INCLUDE_REFS = "0"; $env:LYRA_RETARGET_SKIP = "HitReact"
   $env:LYRA_RETARGET_CLASSES = "AnimMontage,BlendSpace,BlendSpace1D,AimOffsetBlendSpace,AimOffsetBlendSpace1D"
   & $ue NobleFermi.uproject -run=pythonscript -script="$s" -unattended -nullrhi
   ```

   Pass 2 uses up the Lyra-skeleton sequences in this project (they become redirectors). To redo it, re-run `copy_lyra_content.py` after deleting `Content/Characters/Heroes`. Setting `LYRA_RETARGET_REBUILD_ONLY=1` reruns just the hit-react rebuild.

   The result is 317 assets in `/Game/Characters/UEFN_Mannequin/Animations/Lyra/`, under Lyra's sub-folder names (`Actions`, `AimOffsets`, `Locomotion/Rifle`, ...). The retargeter is `/Game/Characters/UEFN_Mannequin/Rigs/RTG_LyraManny_to_UEFN`, built from Lyra's `IK_Mannequin` and the sample's `IK_UEFN_Mannequin`, with chains auto-mapped and bones auto-aligned. Feminine (`MF_`) variants and emotes are skipped. The commandlet reports a crash while shutting Python down, *after* `LYRA_RETARGET_DONE` is logged; by then everything is saved.
