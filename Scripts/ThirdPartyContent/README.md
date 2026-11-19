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

Lyra Starter Game weapon animations will follow the same pattern once imported.
