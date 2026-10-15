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
  - **Toggle Aiming Stance**: Switches between High-Ready ADS and Low-Ready Patrol.
  - **Turntable Rotation**: Pause/resume rotation, toggle speed presets (`12°/s`, `24°/s`, `45°/s`), or click `RESET FRONT FACING` to align the operator directly at the camera.
  - **Biometrics Telemetry**: Displays the live 21-bone skeletal status, 180 cm standing height, 82.0 kg mass, and Level IV armor absorption stats.

---

---

## 3. Entering the Combat Arena: "Rust Tactical" (60m x 60m)

Everything is launchable directly from the main menu without needing separate scripts:

1. In the Start Menu, simply click **`DEPLOY NOW`** on the **Home** tab (or navigate to **`04. SERVER BROWSER & VPS`** and click **`HOST LISTEN ROOM`**).
2. The game transitions directly into the **Rust Tactical** combat arena (`FPSMultiplayerGameMode`).
3. You will spawn at one of the tactical team spawn points with weapon in hand, standing on the desert arena grounds.

### 3.1. Tactical Controls & Ballistics

| Input | Action | Tactical Details |
| :--- | :--- | :--- |
| **W, A, S, D** | Move Forward / Strafe | Standard 450 cm/s tactical combat movement with stride-matched gait cycles. |
| **Mouse X / Y** | Look & Aim | Raw input mouse look with sub-millisecond precision. |
| **Left Click** | Fire Weapon | Full-auto 750 RPM with procedural gun kick and dynamic cone bloom. |
| **Right Click** | Aim Down Sights (ADS) | Smooth over-the-shoulder zoom (90° &rarr; 68° FOV, 175cm &rarr; 110cm boom), 0.65x sensitivity scaling. |
| **V / Thumb Mouse**| Shoulder Swap | Toggles tactical over-the-shoulder peek between Right (+48cm Y) and Left (-48cm Y). |
| **Left Shift** | Tactical Sprint | High-speed forward sprint (720 cm/s) with weapon held in high-ready sprint posture. |
| **Space** | Jump | Physical impulse vaulting over cover. |
| **C / Left Ctrl** | Crouch | Lowered stance for cover behind Jersey barriers and containers. |
| **R** | Tactical / Empty Reload | 4-phase procedural reload animation (cant & dip, mag drop, fresh insert & palm slap). |
| **Q / G** | Tactical / Lethal | Concussion flashbangs and fragmentation grenades. |

### 3.2. Tactical 3rd-Person Perspective & Weapon Socketing
- **Over-the-Shoulder Camera (`CameraBoom` + `FollowCamera`)**: Primary tactical 3rd-person view with 175cm arm length, 12cm collision probe radius, and camera lag for visceral movement weight.
- **Dynamic ADS Over-the-Shoulder Zoom**: Right-clicking contracts arm length to 110cm and narrows FOV to 68°, framing the operator's right shoulder closely for pinpoint combat focus.
- **Direct Right-Hand Weapon Attachment**: The NF-98 Carbine is socketed directly into the operator's right hand (`hand_r`), reacting with physical recoil impulse kick and procedural 4-phase reload animations.
- **Authoritative Ragdoll Death**: Upon zero health, the character capsule uncouples and all 21 anatomical bones enter full physical ragdoll simulation with momentum transfer, while the camera smoothly frames the falling body from its current vantage point.
- **Procedural 4-Phase Reload**:
  1. *Cant & Tilt (0-20%)*: Weapon cants 22° outward, presenting the magwell into clear view.
  2. *Mag Ejection (20-42%)*: Empty magazine slides out and drops downward.
  3. *Mag Insertion & Slap (42-76%)*: Fresh magazine slides up into the magwell with a tactile palm-slap impulse shake.
  4. *Bolt Release & Ready (76-100%)*: Empty reload trips the bolt release snap before returning smoothly to active high-ready.

### 3.3. Pure Dynamic Lighting (Lumen) & Exposure
- **Zero Lightmass Baking**: Static lighting has been permanently disabled in `DefaultEngine.ini` (`r.AllowStaticLighting=False`), guaranteeing zero "Lighting needs to be rebuilt" warnings.
- **Exposure Lock**: Manual camera exposure is locked across both the preview actor and player pawn, eliminating auto-exposure eye-adaptation screen whiteout.
- **Golden-Hour Sun & Volumetric Dust**: Directional sun at 10.0 lux with real-time dynamic shadows, sky atmosphere, warm height fog dust haze, and a pulsating aviation hazard beacon atop the 12m central derrick.

### 3.4. In-Game Tactical HUD
- **Top-Left Minimap**: $180\times 180\text{ px}$ circular tactical radar with $25\text{ m}$ / $50\text{ m}$ range rings, $120^\circ/\text{s}$ rotating sweep line, player orientation chevron, Domination flags A/B/C, and real-time gunfire acoustic radar blips.
- **Top-Center Radar Compass**: $440\text{ px}$ horizontal bearing tape displaying cardinal directions (`N`, `NE`, `E`, `SE`, `S`, `SW`, `W`, `NW`), $5^\circ/15^\circ$ degree ticks, center digital heading ticker `[ 045° ]`, objective waypoints, and acoustic threat diamonds.
- **Bottom-Left Operator Vitals**:
  - **Armor Shield (100)**: Cobalt blue bar representing Level IV ceramic plate integrity.
  - **Health (100 HP)**: Live numerical HP readout with dynamic color shift (cyan &rarr; amber &rarr; critical red).
  - **Tactical Stamina (100)**: Emerald green meter. Drains at $18.0/\text{s}$ during tactical sprint; regenerates at $28.0/\text{s}$ after $0.8\text{ s}$ of rest/walking. Exhaustion locks sprint at 0 until 15 stamina is restored.
- **Bottom-Right Weapon & Ammo HUD**: Active weapon name, fire mode (`FULL-AUTO 750 RPM`), large 30pt digital counter with reserve (`30 / 120`), emergency low-ammo flashing, and `[>> RELOADING...]` sequence status.
- **Dynamic Reticle & Hitmarker**: Bloom-scaling crosshair that collapses smoothly during ADS, plus confirmed impact diagonal `X` hitmarkers.

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
