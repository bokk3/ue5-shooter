# NOBLE FERMI // PROJECT MASTER SUMMARY & AGENT HANDOFF PROMPT

> **Document Type**: Project Architecture, History, Feature Status & AI Agent Starter Prompt  
> **Project Name**: Noble Fermi (Modern Military Tactical Warfare)  
> **Engine Version**: Unreal Engine 5.8 (Custom Build at `D:\epuc\UE_5.8`)  
> **Primary Language**: Modern C++20  
> **Networking**: 128 Hz Authoritative Dedicated Server / Client Listen Architecture  
> **Repository Path**: `C:\Users\Boris\Documents\antigravity\noble-fermi`  
> **Target Executable**: `Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe`  
> **Live Project Site**: [https://noble-fermi.pages.dev/](https://noble-fermi.pages.dev/)  

---

## 1. Project Vision & Creative Intentions

**Noble Fermi** is an independent, competitive tactical first-person shooter engineered from the ground up to capture the peak mechanical feel, lethal tempo, and crisp feedback of golden-era tactical multiplayer (*Call of Duty: Modern Warfare 2*, 2009).

Unlike standard indie prototypes that rely on bloated marketplace asset packs or brittle visual scripting, Noble Fermi is engineered with an **"Architecture Before Assets"** philosophy:
1. **Authoritative 128-Tick Networking**: High-frequency physics updates, server rewind lag compensation, client prediction, and sub-millisecond hit registration.
2. **True Anatomical Human Male Mechanics**: The player model is strictly calibrated to an adult human male (180 cm standing height, 82.0 kg mass, 21 anatomical bones) with distinct ballistic tissue zoning, Level IV ceramic chest plates, and a second-order spring aim punch flinch equation.
3. **Hardware-Accelerated Slate C++ Interfaces**: The entire user interface—from the full-screen military start menu with live 3D character customization to the in-game tactical HUD (minimap, radar compass, stamina, vitals, ammo)—is rendered natively via Slate C++, ensuring zero webview overhead and zero garbage collection hitches.
4. **Procedural Map & Weapon Geometry**: Both the primary combat arena (*Rust Tactical* 60m x 60m) and the primary carbine (*NF-98 Tactical Carbine* with 11 modular subcomponents) are constructed dynamically via procedural C++, enabling instant compilation, headless server execution, and deterministic packaging without manual Unreal Editor GUI intervention.
5. **Pure Real-Time Dynamic Lighting (Lumen)**: Static light baking is permanently disabled (`r.AllowStaticLighting=False`), guaranteeing zero "Lighting needs to be rebuilt" errors, complemented by strict manual camera exposure locks that prevent auto-exposure whiteout.

---

## 2. Hard Constraints & Development Rules

Any engineer or AI agent working on this codebase must adhere strictly to these project rules:

1. **No Fragmented Batch Files**:
   - Everything player-facing runs through a single master launcher: [`Launch_Menu.bat`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Launch_Menu.bat).
   - From this start menu, the user can customize character gear, browse servers, host listen matches (`?listen`), or deploy directly into Rust Tactical.
   - The only separate script permitted is [`Launch_DedicatedServer.bat`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Launch_DedicatedServer.bat), dedicated exclusively to headless 128-tick VPS hosting.
2. **Zero Unbaked Blueprint / Heavy UMG Dependencies**:
   - Core gameplay mechanics, UI widgets, map generation, and ballistics must compile directly in C++.
   - Do not introduce webviews, Chromium wrappers, or heavy external widget libraries. Use Slate C++ vector drawing (`FSlateDrawElement`).
3. **Maintain Dynamic Lighting & Exposure Clamps**:
   - Never re-enable static lighting in `Config/DefaultEngine.ini`.
   - Never rely on default auto-exposure (eye adaptation); always clamp min/max brightness and lock exposure to avoid screen blinding.
4. **Compile & Deploy Workflow**:
   - The engine is at `D:\epuc\UE_5.8`.
   - The required .NET runtime is: `$env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"`.
   - Compilation command:
     ```powershell
     $env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"; & "D:\epuc\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" NobleFermi Win64 Development -Project="C:\Users\Boris\Documents\antigravity\noble-fermi\NobleFermi.uproject" -WaitMutex
     ```
   - Always copy the compiled binary to the packaged distribution:
     ```powershell
     Copy-Item -Path "Binaries\Win64\NobleFermi.exe" -Destination "Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe" -Force
     ```

---

## 3. Chronological Record of Completed Work

### Phase 1: Core Engine & C++20 Build Foundation
- Configured UE 5.8 custom engine environment with MSVC v143/v145 and .NET 10.
- Established clean module hierarchy under `Source/NobleFermi/`.
- Configured monolithic standalone packaging settings in `Config/DefaultGame.ini` and `Config/DefaultEngine.ini`.

### Phase 2: Human Male Skeletal Anatomy & Body Mechanics
- **21-Bone Skeletal Hierarchy (`FPSHumanBones.h/.cpp`)**: Modeled an adult human male (180 cm, 82 kg).
  - Defined explicit bone dimensions, masses, and heights for `root`, `pelvis`, `spine_01` (lumbar), `spine_02` (mid-thoracic), `spine_03` (upper thoracic), `neck_01`, `head`, `clavicles`, `arms`, `hands`, `thighs`, `calves`, and `feet`.
- **Ballistic Zoning & Level IV Armor (`FPSHealthComponent.h/.cpp`)**:
  - Head (2.5x critical), Neck (2.0x), Upper Chest (1.25x), Pelvis (1.0x), Limbs (0.75x–0.85x).
  - Level IV ceramic armor plates shelter `spine_02` and `spine_03`, absorbing kinetic energy prior to tissue penetration.
- **2nd-Order Spring Aim Punch (`FPSBodyMechanicsComponent.h/.cpp`)**:
  - Damped harmonic oscillator formulation ($\mathbf{F}_{spring} = -k\mathbf{\theta}$, $\mathbf{F}_{damping} = -c\mathbf{\omega}$) simulates realistic physical flinch and weapon recovery when shot.
- **Limb Trauma Tracking**:
  - Arm trauma amplifies weapon sway amplitude; leg trauma cancels and suppresses tactical sprinting.

### Phase 3: Start Menu & 3D Character Customization Diorama
- **Hardware-Accelerated Slate Menu (`MainMenuWidget.h/.cpp`)**:
  - Pure C++ Slate interface with translucent military HUD styling, animated tabs, and telemetry.
- **3D Turntable Preview Actor (`FPSCharacterPreviewActor.h/.cpp`)**:
  - Continuous 360° rotation (12°/s, 24°/s, 45°/s, or paused) in high-ready ADS aiming stance with procedural idle breathing.
  - Integrated 3-point studio lighting (14,000 lm key, 9,000 lm cyan rim, 4,500 lm fill) and locked camera exposure.
- **Runtime Customization Controls**:
  - Camouflage patterns: Urban Digital, Woodland Tactical, Desert Spec Ops, Stealth Blackout.
  - Tactical headgear: FAST Ballistic Helmet, GPNVG-18 Quad-Nods (with phosphor green glow), Spec Ops Balaclava, Comm Cap & Headset.
  - Armor carriers: Level IV Heavy Carrier, Lightweight JPC Vest, Recon Rig.

### Phase 4: Web Deployment & Brand Presence
- Deployed responsive official website at `https://noble-fermi.pages.dev/` showcasing gameplay pillars, live server browser simulation, and interactive 3D loadout inspector.

### Phase 5: Procedural Map: "Rust Tactical" (60m x 60m)
- **Map Actor (`AFPSCombatArenaMapActor.h/.cpp`)**:
  - Fully procedural C++ construction of an iconic 60m x 60m desert combat arena.
  - **West Flank**: 4 shipping containers with a central shoot-through tunnel and Domination Flag A.
  - **Center Chokepoint**: 12-meter tall industrial oil derrick with multi-tier catwalks, dual 25° access ramps, armored vehicle wreck, 8 concrete Jersey barriers, and Domination Flag B.
  - **East Flank**: 30-meter elevated oil pipeline trench leading into a reinforced concrete bunker with Domination Flag C.
- **Dynamic Lighting & Anti-Whiteout Fix**:
  - Disabled static lightmass baking permanently (`r.AllowStaticLighting=False`).
  - Warm 10.0 lux golden-hour sun, real-time skylight, volumetric dust haze, and pulsing red derrick beacon.
  - Unbound post-process volume with manual exposure lock (`AEM_Manual`, fixed bias 0.0).
- **Tactical Spawning**: 8 anti-spawn-trap `APlayerStart` positions (4 Task Force 141 Alpha, 4 OpFor Bravo).

### Phase 6: Procedural 3D Weaponry, ADS Mechanics & 4-Phase Reload
- **Procedural NF-98 Tactical Carbine (`AFPSWeaponBase.h/.cpp`)**:
  - 11 procedural components: Upper/lower receiver, Picatinny top rail, blued steel barrel, M-LOK handguard, muzzle brake, pistol grip, buffer tube, buttstock, independent magazine root, reflex optic hood, and glowing tritium front iron sight.
- **Pixel-Perfect ADS Alignment**:
  - Smooth camera translation from hipfire (`X=26, Y=14, Z=-13`) to ADS center (`X=20, Y=0, Z=-6.8`).
  - Emissive red dot reticle sits dead-center on camera eye-line (`Y=0, Z=0`) with zero parallax error.
  - Dynamic FOV zoom (95° &rarr; 68° in 180 ms) and 0.65x mouse look sensitivity scaling.
- **4-Phase Procedural Reload Sequence**:
  - Phase 1 (Cant & Dip, 0-20%): Carbine rolls 22° outward, presenting magwell to the operator.
  - Phase 2 (Mag Ejection, 20-42%): Empty magazine slides downward -28 cm out of receiver.
  - Phase 3 (Fresh Insert & Slap, 42-76%): Fresh magazine slides in with a tactile palm-slap impulse.
  - Phase 4 (Bolt Rack & Return, 76-100%): Bolt catch snaps forward on empty reload, returning smoothly to active high-ready.

### Phase 7: Slate Tactical In-Game HUD & Operator Telemetry
- **Top-Left Acoustic Minimap**:
  - $180\times 180\text{ px}$ circular radar with 25m and 50m range rings and 120°/s rotating radar sweep.
  - Forward-facing player chevron indicator and Domination flags A, B, and C.
  - Acoustic gunfire detection: Hostile unsuppressed shots ping real-time radar blips with expanding dissipation rings via `UFPSRadarSubsystem`.
- **Top-Center Radar Compass Tape**:
  - $440\text{ px}$ horizontal bearing tape covering a 90° FOV.
  - Cardinal labels (`N`, `NE`, `E`, `SE`, `S`, `SW`, `W`, `NW`), minor 5° and major 15° tick marks.
  - Framed digital heading readout box (`[ 045° ]`).
  - Objective waypoints and acoustic threat diamonds projected on the tape.
- **Bottom-Left Operator Vitals & Tactical Stamina**:
  - **Level IV Ceramic Armor Shield (100)**: Cobalt blue kinetic absorption bar protecting thoracic vitals (`spine_02`, `spine_03`).
  - **Health (100 HP)**: Digital readout with dynamic color shift (cyan $\to$ amber $\to$ critical red).
  - **Tactical Stamina (100)**: Integrated into `FPSCharacterMovementComponent`. Drains at 18.0/s during tactical sprint, regenerates at 28.0/s after 0.8s rest, with exhaustion lockout at 0 until restored to 15.
- **Bottom-Right Digital Ammo & Loadout**:
  - Weapon name, fire mode (`[FULL-AUTO 750 RPM]`), large 30pt digital counter with reserve (`30 / 120`), emergency low-ammo flashing ($\le 5$), and active `[>> RELOADING...]` banner.
- **Dynamic Crosshair & Hitmarkers**:
  - Spread-responsive 4-pip reticle that collapses during ADS for unobstructed sightlines.
  - Direct confirmed projectile impact diagonal `X` hitmarkers wired to `FPSWeaponBase.cpp`.

### Phase 8: Tactical 3rd-Person Operator Camera & Anatomical Body Pipeline (branch: `body-v1`)
- **Step 1: Centered Base Male Mesh & Ragdoll Physics (`SK_BaseMale`)**:
  - Authored and centered the 180cm / 82kg adult human male anatomical mesh at world origin `(0,0,0)` with clean outward-facing normals.
  - Configured 21-bone skeletal hierarchy with authoritative collision capsules (`SK_BaseMale_PhysicsAsset.uasset`) for physics and ragdoll.
- **Step 2: Locomotion Pipeline & AnimInstance (`UFPSAnimInstance`)**:
  - Authored 4 distinct locomotion animations: high-ready ADS idle with natural breathing (`Anim_BaseMale_Idle_ADS`), tactical walk (`Anim_BaseMale_Walk`), combat run (`Anim_BaseMale_Run`), and sprint (`Anim_BaseMale_Sprint`).
  - Implemented dynamic stride matching and velocity scaling in `AFPSCharacter`.
- **Step 3: PBR Ripstop Master Material & Multi-Zone Camouflage (`M_Operator_BaseBody`)**:
  - 2048x2048 seamless micro-ripstop normal & roughness maps, skin subsurface scattering (SSS), and 4 procedural camouflage patterns (Urban Digital, Woodland Tactical, Desert Spec Ops, Stealth Blackout).
- **Step 4: Primary Tactical 3rd-Person Over-the-Shoulder Camera System**:
  - Transitioned primary gameplay perspective to tactical 3rd-person over-the-shoulder view (`CameraBoom` + `FollowCamera`).
  - Spring arm configured with 175cm arm length, 12cm collision probe radius, and 16.0f/24.0f camera & rotation lag for smooth, visceral operator movement.
  - Anti-whiteout manual exposure lock (`AEM_Manual`) for Lumen dynamic lighting compliance.
  - Dynamic ADS zoom: arm length compresses to 110cm, socket offset tightens, FOV narrows from 90° to 68°, and mouse sensitivity scales to 0.65x.
  - Tactical shoulder swapping: Instant toggle between right shoulder (+48cm Y) and left shoulder (-48cm Y) via key `V` and mouse thumb button.
  - Weapon hand socketing: NF-98 Carbine attached directly to operator right hand (`hand_r`) with procedural recoil and 4-phase reload retained.
  - Authoritative death ragdoll: Disables movement and capsule collision, wakes all 21 physics bodies, and smoothly attaches camera boom to pelvis without jarring cuts.

- **Step 5: 3D AK-47 Rifle (`SM_AK47`) & IK-to-FK Baked Stances**:
  - Generated 3D AK-47 model (`SM_AK47.uasset`) with stamped steel receiver, wooden buttstock/handguard/pistol grip, and curved Bakelite magazine (`M_AK47_Steel`, `M_AK47_Wood`, `M_AK47_Bakelite`).
  - Added dedicated `weapon_r` bone socket on `SK_BaseMale` (`hand_r` child) and baked exact two-handed IK contact poses onto pure FK bone rotations across `Anim_BaseMale_Idle_ADS`, `Anim_BaseMale_LowReady`, `Anim_BaseMale_Walk`, `Anim_BaseMale_Run`, and `Anim_BaseMale_Sprint`.
  - Integrated `[ AIMED ADS ]`, `[ LOW READY ]`, and `[ WALK ANIM ]` stance controls in the `02. OPERATOR // CUSTOMISATION` menu tab.
- **Step 6 (Phase 8A & 8B): "The Fermi Paradox Engine" (Dive, Clone, Swap, Shatter Time)**:
  - **3rd-Person OTS Camera Overhaul**: `DefaultArmLength = 210.0f`, `ADSArmLength = 95.0f`, smooth `ShoulderSideMultiplier` interpolation (`V` key), `62°` tight OTS ADS FOV, and hybrid toggle (`MiddleMouseButton` while aiming) into `1st-Person True Optic ADS` (`TargetArmLength = 0.0f`).
  - **Max Payne Omnidirectional Ballistic Dive (`LeftAlt` / `Move+C`)**: Launches operator at `820 cm/s` horizontal + `340 cm/s` vertical (`35.0f` stamina cost), tilts the 21-bone body `75°` into the dive direction (chest-down forward, supine backward, side-roll left/right) with full `360°` mid-air aiming/firing, and transitions into a `0.45s` kinetic ground slide on landing.
  - **Localized Fermi Time-Dilation Bubble (`AFPSFermiTimeBubble`)**: `4.5m` (`450 cm`) radius chrono sphere lasting `2.4s`, slowing enemy pawns and projectiles inside to `0.25x` (`CustomTimeDilation = 0.25f`) while granting the diving operator `0.55x` reflex speed with inverse time-dilation mouse compensation and physicalized `AFPSBallisticTracer` brass slugs (`2,800 cm/s`).
  - **"Fermi Echo" Acoustic Hologram Decoy & Mid-Air Quantum Swap (`AFPSFermiDecoyActor`, `Q` key)**: Deploys a phosphor-cyan `SK_BaseMale` hologram sprinting at `680 cm/s` and broadcasting unsuppressed gunfire pings to `UFPSRadarSubsystem`. Pressing `Q` again while active executes an instant **Quantum Phase-Swap**, exchanging world positions and conserving/redirecting dive momentum along the player's sightline.
  - **Slate HUD Chrono & Echo Telemetry**: Added the `FERMI DIVE // CHRONO FIELD` status bar, `[Q] QUANTUM SWAP READY` indicator, peripheral cyan time-dilation vignette, and `CHRONO FIELD // 0.25x c` digital chronometer to `SFPSHUDWidget`.

---

## 4. Codebase Architecture & File Inventory

```
Source/NobleFermi/
├── NobleFermi.Build.cs                     # Build rules (Core, Slate, SlateCore, InputCore, NetCore)
├── NobleFermi.h / .cpp                     # Primary game module
├── BodyMechanics/
│   ├── FPSHumanBones.h / .cpp              # 21-bone male hierarchy, anthropometry, mass distribution
│   └── FPSBodyMechanicsComponent.h / .cpp  # 2nd-order spring aim punch, flinch, IK, limb trauma
├── Character/
│   ├── FPSCharacter.h / .cpp               # Authoritative player pawn (3P OTS, Ballistic Dive, Quantum Swap)
│   ├── FPSCharacterMovementComponent.h/.cpp# Tactical sprint, slide, Max Payne Omnidirectional Ballistic Dive
│   └── FPSCharacterPreviewActor.h / .cpp   # 3D turntable operator actor (AK-47 stances, camo, studio lights)
├── Combat/
│   ├── FPSFermiTimeBubble.h / .cpp         # Localized 4.5m Chrono-Dilation Field (0.25x enemies, 0.55x owner)
│   ├── FPSFermiDecoyActor.h / .cpp         # Autonomous Acoustic Hologram Clone & Quantum Phase-Swap anchor
│   ├── FPSBallisticTracer.h / .cpp         # Physicalized 2,800 cm/s slow-motion brass projectiles & wake trails
│   ├── FPSEquipmentComponent.h / .cpp      # Lethal frag cooking & tactical ordnance
│   ├── FPSProjectileBase.h / .cpp          # Grenade & ordnance physics
│   └── FPSHealthComponent.h / .cpp         # Health (100), Level IV Armor Shield (100), damage zoning
├── Game/
│   ├── FPSGameMode.h / .cpp                # Match lifecycle, 128-tick configuration, team management
│   ├── FPSMenuGameMode.h / .cpp            # Menu coordinator (spawns 3D preview, sets camera target)
│   ├── FPSMenuPlayerController.h / .cpp    # UI mouse unlock, input routing, viewport focus
│   └── FPSMenuDioramaActor.h / .cpp        # 3D scene lighting, backdrop architecture
├── Map/
│   └── FPSCombatArenaMapActor.h / .cpp     # "Rust Tactical" 60mx60m procedural map & Lumen lighting
├── Multiplayer/
│   ├── FPSMultiplayerGameMode.h / .cpp     # Dedicated server & listen match rules
│   ├── FPSGameState.h / .cpp               # Match state replication, team scores
│   └── FPSPlayerState.h / .cpp             # K/D stats, ping replication, team assignment
├── Radar/
│   └── FPSRadarSubsystem.h / .cpp          # Acoustic gunfire detection & radar ping dispatcher
├── UI/
│   ├── FPSHUDWidget.h / .cpp               # In-Game AR Visor HUD (Minimap, Compass, Vitals, Fermi Chrono Field)
│   ├── MainMenuWidget.h / .cpp             # Full-screen military start menu & 3D stance/camo customizer
│   ├── FPSPauseMenuWidget.h / .cpp         # In-game escape pause menu & settings
│   └── SettingsMenuWidget.h / .cpp         # Video, audio, mouse sensitivity configuration
└── Weapons/
    ├── FPSWeaponBase.h / .cpp              # 3D SM_AK47 rifle, procedural ADS, 4-phase reload, tracer spawning
    ├── FPSDilatedBulletActor.h / .cpp      # Piecewise hitscan-to-slug physical bullet actor (1,400 cm/s)
    ├── FPSGunsmithComponent.h / .cpp       # Modular attachment slots (barrel, optic, mag, muzzle)
    └── FPSInventoryComponent.h / .cpp      # Weapon inventory & weapon_r bone socket attachment
```

---

## 5. Planned Features & Development Roadmap

The next development milestones to prioritize:

1. **Weapon Arsenal Expansion (Iconic MW2 Archetypes)**:
   - *Spec Ops Stealth (MP5SD)*: Integrated suppressor, subsonic 9mm, zero acoustic radar ping, 150 ms ADS.
   - *Heavy Gunner (L86 LSW)*: 100-round continuous drum suppression, fluted heavy barrel, 380 ms ADS.
   - *Recon Sniper (Intervention .50)*: High-caliber one-shot torso/head lethality, bolt-cycle animation, 1150 m/s velocity.
   - *Run & Gun Rusher (Vector .45 ACP)*: 1200 RPM extreme fire rate, 120 ms ADS speed.
2. **Interactive 3D Gunsmith Customizer**:
   - Allow players to inspect weapons on a rotating 3D pedestal in the start menu.
   - Live attachment hot-swapping: Reflex optic &rarr; Holographic &rarr; 4x ACOG; Standard mag &rarr; Extended 45-round &rarr; Fast-mag; Flash hider &rarr; Tactical Suppressor.
3. **Dedicated Server VPS Packaging & Containerization**:
   - Linux headless server build scripts (via cross-compile toolchain or Docker/Wine).
   - Automated server heartbeat reporting to master server list.
4. **Audio Subsystem Integration**:
   - Spatial acoustic sound propagation for unsuppressed gunshots, bullet cracks/whizz-bys, and mechanical reload audio (mag drop, slap, bolt release).
   - Surface-aware footstep Foley (metal catwalk, dirt ground, concrete bunker).
5. **Tactical AI Bot Opponents**:
   - Navigation mesh generation on Rust Tactical.
   - Behavior tree / State tree bots with combat flanking, cover utilization, and objective capture logic.
6. **Classic MW2 Killstreak System**:
   - 3 Kills: UAV Recon (pulsing radar scan on minimap).
   - 5 Kills: Predator Missile (steerable overhead ballistic camera).
   - 7 Kills: Harrier Airstrike (cluster bombardment + VTOL hover defense).
   - 11 Kills: Chopper Gunner (side-door minigun control).
   - 25 Kills: Tactical Nuke (game-ending match victory).

---

## 6. Master Starter Prompt for New AI Agents

*Copy and paste the exact block below to immediately onboard any new AI coding agent into the project:*

```markdown
You are an expert Unreal Engine 5.8 C++ systems engineer working on **Noble Fermi**, a competitive 3rd-person tactical shooter featuring "The Fermi Paradox Engine" (Omnidirectional Ballistic Dives, Localized Time-Dilation Bubbles, Acoustic Hologram Decoys, and Mid-Air Quantum Phase-Swapping).

### Project Context & Environment
- **Engine**: Unreal Engine 5.8 custom source build located at `D:\epuc\UE_5.8`.
- **Project Root**: `C:\Users\Boris\Documents\antigravity\noble-fermi`.
- **Build System**: UBT with .NET 10 (`$env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"`).
- **Target Distribution**: `Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe`.
- **Primary Launcher**: `Launch_Menu.bat` (all game modes, character customization, and match hosting must launch from here; do not create fragmented batch files).
- **Dedicated Server**: `Launch_DedicatedServer.bat` (runs headless on port 7777 at 128 Hz tickrate).

### Core Architectural Principles
1. **Primary Tactical 3rd-Person OTS Perspective**: Visceral over-the-shoulder framing (`CameraBoom` 210cm hipfire -> 95cm tight OTS ADS at 62° FOV, plus `MiddleMouseButton` hybrid toggle to 1st-Person Optic ADS), 12cm collision probe radius, smooth camera lag, smooth shoulder swapping (key `V`), and full anatomical body visibility (`SK_BaseMale` holding `SM_AK47` on socket `weapon_r`).
2. **The Fermi Paradox Engine**:
   - **Max Payne Omnidirectional Ballistic Dive (`LeftAlt` / `Move+C`)**: Launches at 820 cm/s horiz + 340 cm/s vert, tilts the 21-bone operator 75° into the dive vector with full 360° aiming, and lands in a 0.45s kinetic slide.
   - **Localized Fermi Time-Dilation Bubble (`AFPSFermiTimeBubble`)**: 4.5m sphere lasting 2.4s (0.25x enemy speed, 0.55x owner reflex speed, 2,800 cm/s physicalized `AFPSBallisticTracer` brass rounds).
   - **Acoustic Fermi Echo & Mid-Air Quantum Swap (`AFPSFermiDecoyActor`, `Q` key)**: Deploys a phosphor-cyan `SK_BaseMale` hologram sprinting at 680 cm/s and spoofing radar gunfire; pressing `Q` again swaps world transforms while conserving dive momentum.
3. **Authoritative 128-Tick Networking**: High-frequency physics, lag compensation rewind, and client prediction across both headless dedicated VPS servers and client listen rooms (`?listen`).
4. **Human Male Anatomical Mechanics**: 21-bone hierarchy (`FPSHumanBones.h`), 180 cm / 82 kg anthropometry, Level IV ceramic plates covering `spine_02`/`spine_03`, and 2nd-order spring aim punch flinch equations (`FPSBodyMechanicsComponent.h`).
5. **Hardware-Accelerated Slate C++ UI**: Pure C++ Slate rendering (`MainMenuWidget.h`, `FPSHUDWidget.h` / `SFPSHUDWidget`). Zero webview or heavy UMG overhead.
6. **Pure Dynamic Lumen Lighting & Locked Exposure**: Static lightmass baking is permanently disabled (`r.AllowStaticLighting=False`). Camera exposure is strictly locked (`AEM_Manual`) to prevent whiteout.

### Standard Build & Deploy Command
Always compile with:
```powershell
$env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"; & "D:\epuc\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" NobleFermi Win64 Development -Project="C:\Users\Boris\Documents\antigravity\noble-fermi\NobleFermi.uproject" -WaitMutex
Copy-Item -Path "Binaries\Win64\NobleFermi.exe" -Destination "Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe" -Force
if (Test-Path "Binaries\Win64\NobleFermi.pdb") { Copy-Item -Path "Binaries\Win64\NobleFermi.pdb" -Destination "Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.pdb" -Force }
```

When building new features, maintain clean separation of concerns, preserve documentation, respect the existing code style, and ensure the game is fully launchable via `Launch_Menu.bat`.
```

