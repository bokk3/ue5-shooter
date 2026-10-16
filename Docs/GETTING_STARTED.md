# Noble Fermi: Getting Started Guide

This guide walks you through launching the standalone game, exploring the 3D character customization menu, hosting dedicated multiplayer servers on a VPS or local machine, and building from source.

---

## 1. Quick Launch (Packaged Standalone Build)

The project includes pre-configured batch launchers in the root folder:

### 1.1. Launching the Interactive Start Menu
Double-click [`Launch_Menu.bat`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Launch_Menu.bat) or run in PowerShell:
```powershell
.\Launch_Menu.bat
```
Alternatively, launch the standalone binary directly:
```powershell
Packaged\Windows\NobleFermi.exe -windowed -ResX=1280 -ResY=720 -log
```

**What to expect**:
- The window boots in 1280x720 windowed mode with full mouse freedom (`DoNotLock` / `NoCapture`).
- The primary start menu renders via native hardware-accelerated Slate C++.
- The 3D viewport behind the menu renders the **tactical human male operator model** rotating continuously on a turntable while aiming down sights (ADS).
- Dedicated 3-point studio lighting highlights the armor plates, helmet, and weapon receiver.

---

## 2. Exploring Operator Customisation

In the start menu, click **`02. OPERATOR // CUSTOMISATION`** in the left navigation column.

### 2.1. Available Controls
- **Camouflage Patterns**:
  - `Urban Digital`: Metropolitan slate gray uniform with dark charcoal plate carrier.
  - `Woodland Tactical`: Forest combat olive drab with ranger khaki vest.
  - `Desert Spec Ops`: Arid coyote tan with Flat Dark Earth (FDE) rifle.
  - `Stealth Blackout`: Midnight matte black uniform with carbon kevlar armor.
- **Tactical Headgear**:
  - `FAST Ballistic Helmet`: High-cut Kevlar helmet with side accessory rails.
  - `GPNVG-18 Quad-Nods`: Panoramic four-tube night vision goggles with green phosphor glow.
  - `Spec Ops Balaclava`: Flame-retardant stealth face mask.
  - `Tactical Cap & Comm`: Baseball operator cap with Peltor communication headset.
- **Armor Vests**:
  - `Level IV Heavy Carrier`: Bulkier ceramic plates protecting chest and thoracic spine.
  - `Lightweight JPC Vest`: Jumpable plate carrier optimized for sprint speed and mobility.
  - `Recon Tactical Rig`: Minimalist chest rig with magazine pouches.
- **Stance & Turntable Inspection**:
  - **3-Stance Selector (`[ AIMED ADS ]`, `[ LOW READY ]`, `[ WALK ANIM ]`)**: Switches the operator between Aimed Down Sights (with right hand on the AK-47 pistol grip and left hand cupping the wooden handguard), Low-Ready Patrol stance, and the full bipedal Aimed Tactical Walking gait cycle.
  - **Turntable Rotation**: Pause/resume rotation, toggle speed presets (`12°/s`, `24°/s`, `45°/s`), or click `RESET FRONT FACING` to align the operator directly at the camera.
  - **Biometrics Telemetry**: Displays the live 21-bone skeletal status, 180 cm standing height, 82.0 kg mass, and Level IV armor absorption stats.

---

## 3. Entering the Combat Arena: "Rust Tactical" (60m x 60m)

Everything is launchable directly from the main menu without needing separate scripts:

1. In the Start Menu, simply click **`DEPLOY NOW`** on the **Home** tab (or navigate to **`04. SERVER BROWSER & VPS`** and click **`HOST LISTEN ROOM`**).
2. The game transitions directly into the **Rust Tactical** combat arena (`FPSMultiplayerGameMode`).
3. You will spawn at one of the tactical team spawn points with the 3D **AK-47** rifle socketed to `weapon_r`, standing on the desert arena grounds.

### 3.1. Tactical Controls & The Fermi Paradox Engine

| Input | Action | Tactical Details |
| :--- | :--- | :--- |
| **W, A, S, D** | Move Forward / Strafe | Standard 450 cm/s tactical combat movement with stride-matched gait cycles. |
| **Mouse X / Y** | Look & Aim | Raw input mouse look with inverse time-dilation compensation inside Chrono Fields. |
| **Left Click** | Fire Weapon | Full-auto 750 RPM firing physicalized brass tracers (`2,800 cm/s` in slow-motion). |
| **Right Click (Hold)** | Tight OTS ADS Zoom | Smooth Over-The-Shoulder zoom (`210cm` &rarr; `95cm` boom, `90°` &rarr; `62°` FOV), `0.65x` sensitivity. |
| **Middle Click (While Aiming)** | Hybrid 1P Optic ADS Toggle | Toggles between **3rd-Person Tight OTS ADS** and **1st-Person True Optic Sightline** (`0cm` boom). |
| **V / Thumb Mouse** | Smooth Shoulder Swap | Smoothly interpolates camera offset between Right Shoulder (`+52cm Y`) and Left (`-52cm Y`). |
| **Left Alt** *(or Move + Double-Tap C / Ctrl+Space)* | **Max Payne Ballistic Dive + Fermi Time Bubble** | Consumes 35 Stamina. Launches an omnidirectional dive (`820 cm/s` horiz, `340 cm/s` vert) with `75°` body tilt, `0.45s` landing slide, and expands a **4.5m Chrono Field** (`0.25x` enemy speed, `0.55x` player reflex speed). |
| **Q (Ground / No Echo)** | **Deploy Acoustic Fermi Echo** | Spawns a phosphor-cyan `SK_BaseMale` hologram sprinting at `680 cm/s` and firing simulated unsuppressed bursts to flood enemy radar/compass with red threat blips. |
| **Q (While Echo Active / Mid-Dive)** | **Quantum Phase-Swap** | Instantly swaps world positions with your active Fermi Echo while conserving and redirecting your mid-air ballistic dive momentum along your sightline. |
| **Left Shift** | Tactical Sprint | High-speed forward sprint (`750 cm/s`) with weapon held in sprint posture. |
| **Space** | Jump | Physical impulse vaulting over cover. |
| **C / Left Ctrl** | Crouch / Slide | Lowered stance behind cover, or momentum slide (`950 cm/s`) when sprinting. |
| **R** | Tactical / Empty Reload | 4-phase procedural reload animation (cant & dip, mag drop, fresh insert & palm slap). |
| **G** | Cook & Throw Lethal | Fragmentation grenade with cookable fuse. |

### 3.2. The Fermi Paradox Engine & 3rd-Person Weapon Mechanics
- **Over-The-Shoulder Camera (`CameraBoom` + `FollowCamera`)**: Primary tactical 3rd-person view (`210 cm` boom, `(0, ±52, 22)` offset) with `12 cm` collision probe radius and `18.0f` camera lag.
- **Omnidirectional Ballistic Dive & 4.5m Chrono Bubble**: Diving in any `WASD` direction tilts the 21-bone operator `75°` into the dive vector (chest-down forward, supine on back when diving backward, side-roll left/right) with full `360°` mid-air aiming and spawns `AFPSFermiTimeBubble` (`2.4s` duration, `0.25x` enemy dilation).
- **Acoustic Fermi Echo & Quantum Swap**: Press `Q` to send a holographic clone sprinting out to draw sniper fire and ping enemy radar, then dive and tap `Q` in slow-motion to materialize mid-air directly at the clone's position.
- **3D AK-47 Socketing (`weapon_r`)**: `SM_AK47` attaches directly to the `weapon_r` bone socket on `SK_BaseMale`, keeping both hands locked to the pistol grip and wooden handguard across all stances and locomotion cycles.

### 3.3. Pure Dynamic Lighting (Lumen) & Exposure
- **Zero Lightmass Baking**: Static lighting has been permanently disabled in `DefaultEngine.ini` (`r.AllowStaticLighting=False`), guaranteeing zero "Lighting needs to be rebuilt" warnings.
- **Exposure Lock**: Manual camera exposure (`AEM_Manual`) is locked across both the preview actor and player pawn, ensuring the Fermi Time Bubble and holographic decoy never cause screen whiteout.
- **Golden-Hour Sun & Volumetric Dust**: Directional sun at 10.0 lux with real-time dynamic shadows, sky atmosphere, warm height fog dust haze, and a pulsating aviation hazard beacon atop the 12m central derrick.

### 3.4. In-Game Tactical AR Visor HUD
- **Top-Left Minimap**: $180\times 180\text{ px}$ circular tactical radar with $25\text{ m}$ / $50\text{ m}$ range rings, $120^\circ/\text{s}$ rotating sweep line, player orientation chevron, Domination flags A/B/C, and real-time gunfire acoustic radar blips (including Fermi Echo decoy bursts).
- **Top-Center Radar Compass & Chronometer**: $440\text{ px}$ horizontal bearing tape with cardinal points, objective waypoints, acoustic threat diamonds, and an active `CHRONO FIELD // 0.25x c` ticker during slow-motion dives.
- **Bottom-Left Operator Vitals & Chrono Telemetry**:
  - **Armor Shield (100)**: 4-plate Level IV ceramic armor row in cobalt blue.
  - **Health (100 HP)**: Live numerical HP readout with trailing damage ghost bar and animated ECG waveform.
  - **Tactical Stamina (100)**: 12-cell kinetic meter (`18.0/s` sprint drain, `28.0/s` regen).
  - **Fermi Dive // Chrono Field Bar**: Displays `[READY - ALT / DIVE]`, `[ACTIVE // 0.25x TIME DILATION]`, or recharge status directly below Stamina.
- **Bottom-Right Weapon, 30-Round Pip Matrix & Quantum Echo HUD**: Active weapon title (`AK-47 TACTICAL // 7.62x39MM`), 30-round individual bullet pip matrix, `[Q] DEPLOY FERMI ECHO` / `[Q] QUANTUM SWAP READY` status, and radial 4-phase reload progress ring.

---


## 4. Multiplayer & Dedicated Server Deployment

### 4.1. Hosting a Dedicated Server on a VPS (Linux or Windows)
To run a headless 128-tick authoritative server on Rust Tactical:
```powershell
Packaged\Windows\NobleFermi.exe ?game=/Script/NobleFermi.FPSMultiplayerGameMode -server -nullrhi -log -PORT=7777 -TickRate=128 -MaxPlayers=18
```
*(On Linux VPS hosts, execute via Proton/Wine or compile the native Linux server target).*

Or simply double-click [`Launch_DedicatedServer.bat`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Launch_DedicatedServer.bat).

### 4.2. Connecting to a Dedicated Server
1. In the Main Menu, click **`04. SERVER BROWSER & VPS`**.
2. Under **Direct IP Connection**, enter the server address (e.g. `127.0.0.1:7777` for local, or `YOUR_VPS_IP:7777` for remote).
3. Click **`CONNECT`**. The client travels to the server and enters the match.

### 4.3. Hosting a Client Listen Room
If you do not have a dedicated server, you can host a room directly from your client:
1. In the Main Menu, navigate to **`04. SERVER BROWSER & VPS`** or click **`DEPLOY NOW`** on the home screen.
2. Click **`HOST LISTEN ROOM`**.
3. The client immediately initializes a local listen server (`?listen`) that LAN peers or friends with port forwarding can join.

---

## 5. Building & Compiling from Source

### 5.1. Prerequisites
- **Unreal Engine 5.8** (installed at `D:\epuc\UE_5.8` or your local engine path).
- **MSVC v143 / v145** (Visual Studio 2022 C++ build tools).
- **Windows 10/11 SDK** (`10.0.26100.0` or compatible).
- **.NET 10.0 Runtime** (bundled in `Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe`).

### 5.2. Compiling the Standalone Game Target
Set the `DOTNET_ROOT` environment variable and run UnrealBuildTool:
```powershell
$env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
& "D:\epuc\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" NobleFermi Win64 Development -Project="C:\Users\Boris\Documents\antigravity\noble-fermi\NobleFermi.uproject" -WaitMutex
```

### 5.3. Cooking and Packaging Standalone Builds
To cook assets into IoStore packages (`.ucas`/`.utoc`) and archive the standalone game:
```powershell
$env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
cmd /c "D:\epuc\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project=C:\Users\Boris\Documents\antigravity\noble-fermi\NobleFermi.uproject -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory=C:\Users\Boris\Documents\antigravity\noble-fermi\Packaged -unversionedcookedcontent"
```
The resulting distribution will be in `Packaged\Windows\`.
