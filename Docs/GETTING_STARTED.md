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

## 3. Multiplayer & Dedicated Server Deployment

### 3.1. Hosting a Dedicated Server on a VPS (Linux or Windows)
To run a headless 128-tick authoritative server:
```powershell
Packaged\Windows\NobleFermi.exe -server -nullrhi -log -PORT=7777 -TickRate=128 -MaxPlayers=18
```
*(On Linux VPS hosts, execute via Proton/Wine or compile the native Linux server target).*

Or simply double-click [`Launch_DedicatedServer.bat`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Launch_DedicatedServer.bat).

### 3.2. Connecting to a Dedicated Server
1. In the Main Menu, click **`04. SERVER BROWSER & VPS`**.
2. Under **Direct IP Connection**, enter the server address (e.g. `127.0.0.1:7777` for local, or `YOUR_VPS_IP:7777` for remote).
3. Click **`CONNECT`**. The client travels to the server and enters the match.

### 3.3. Hosting a Client Listen Room
If you do not have a dedicated server, you can host a room directly from your client:
1. In the Main Menu, navigate to **`04. SERVER BROWSER & VPS`** or click **`DEPLOY NOW`** on the home screen.
2. Click **`HOST LISTEN ROOM`**.
3. The client immediately initializes a local listen server (`?listen`) that LAN peers or friends with port forwarding can join.

---

## 4. Building & Compiling from Source

### 4.1. Prerequisites
- **Unreal Engine 5.8** (installed at `D:\epuc\UE_5.8` or your local engine path).
- **MSVC v143 / v145** (Visual Studio 2022 C++ build tools).
- **Windows 10/11 SDK** (`10.0.26100.0` or compatible).
- **.NET 10.0 Runtime** (bundled in `Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe`).

### 4.2. Compiling the Standalone Game Target
Set the `DOTNET_ROOT` environment variable and run UnrealBuildTool:
```powershell
$env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
& "D:\epuc\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" NobleFermi Win64 Development -Project="C:\Users\Boris\Documents\antigravity\noble-fermi\NobleFermi.uproject" -WaitMutex
```

### 4.3. Cooking and Packaging Standalone Builds
To cook assets into IoStore packages (`.ucas`/`.utoc`) and archive the standalone game:
```powershell
$env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
cmd /c "D:\epuc\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun -project=C:\Users\Boris\Documents\antigravity\noble-fermi\NobleFermi.uproject -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory=C:\Users\Boris\Documents\antigravity\noble-fermi\Packaged -unversionedcookedcontent"
```
The resulting distribution will be in `Packaged\Windows\`.
