# NOBLE FERMI // MODERN MILITARY TACTICAL WARFARE

[![Engine](https://img.shields.io/badge/Unreal%20Engine-5.8-blue.svg)](https://www.unrealengine.com/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B20-orange.svg)](https://en.cppreference.com/w/cpp/20)
[![Tickrate](https://img.shields.io/badge/Tickrate-128%20Hz-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20VPS-lightgrey.svg)]()
[![Build](https://img.shields.io/badge/Build-Standalone%20Monolithic-success.svg)]()
[![Website](https://img.shields.io/badge/Live%20Website-noble--fermi.pages.dev-cyan.svg)](https://noble-fermi.pages.dev/)

> **Noble Fermi** is a competitive tactical shooter built from the ground up in Unreal Engine 5.8 C++, engineered with a primary focus on an authoritative **tactical 3rd-person over-the-shoulder perspective**. It blends the lethal, high-tempo combat of golden-era tactical multiplayer with authoritative **128-tick dedicated server networking**, an **anatomically accurate human male operator model** (180 cm / 82 kg anthropometry, 21-bone skeletal hierarchy, PBR ripstop uniform with skin SSS), dynamic **over-the-shoulder camera mechanics** (shoulder swapping, smooth ADS zoom, collision probe protection, camera lag), and a **hardware-accelerated Slate C++ start menu featuring real-time 3D character customization**.

---

## Key Pillars & Engineering Features

```
┌────────────────────────────────────────────────────────────────────────┐
│                              NOBLE FERMI                               │
│                Tactical Military Warfare FPS Architecture              │
└────────────────────────────────────────────────────────────────────────┘
          │                                              │
          ▼                                              ▼
┌──────────────────────────────┐        ┌──────────────────────────────┐
│     Multiplayer Network      │        │       Combat & Anatomy       │
├──────────────────────────────┤        ├──────────────────────────────┤
│ • 128 Hz Authoritative Tick  │        │ • 21 Human Male Bones (180cm)│
│ • Dedicated VPS Server Binary│        │ • Over-the-Shoulder 3rd-P Cam│
│ • Client Listen Room (?listen)│       │ • Level IV Ceramic Plates    │
│ • Direct IP Connect (7777)   │        │ • Ragdoll Physics on Death   │
└──────────────────────────────┘        └──────────────────────────────┘
          │                                              │
          └──────────────────────┬───────────────────────┘
                                 │
                                 ▼
┌────────────────────────────────────────────────────────────────────────┐
│           Interactive Start Menu, Slate Tactical HUD & 3D Diorama      │
├────────────────────────────────────────────────────────────────────────┤
│ • Pure Slate C++ Minimap, Radar Compass Tape, Stamina & Ammo Counters  │
│ • 360° Rotating Turntable with Live ADS Aiming Operator                │
│ • Procedural Idle Breathing Motion & 3-Point Studio Lighting           │
│ • Real-time Camo (Urban/Woodland/Desert/Blackout) & Gear Customization │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. Primary Tactical 3rd-Person Over-the-Shoulder Camera System
- **Over-the-Shoulder Framing (`CameraBoom` + `FollowCamera`)**:
  - **Hipfire & Traversal**: Camera sits 175 cm back with a `(0, 48, 25)` cm right-shoulder offset, framing the operator and combat space cleanly.
  - **Dynamic ADS Zoom**: Smoothly contracts arm length from 175 cm to 110 cm, tightens socket offset to `(10, 38, 18)` cm, narrows FOV from 90° to 68°, and applies 0.65x mouse sensitivity scaling for pinpoint aiming.
  - **Tactical Shoulder Swapping**: Instantaneous toggle between right shoulder (+48 cm Y) and left shoulder (-48 cm Y) via key `V` or mouse thumb button for corner peeking.
  - **Collision Probe Protection**: 12 cm probe radius ensures camera never clips through shipping containers, catwalks, or bunker walls.
  - **Visceral Camera Lag**: Responsive 16.0f positional and 24.0f rotational lag convey genuine physical weight and momentum during combat maneuvers.
  - **Lumen Dynamic Anti-Whiteout Exposure**: Strict manual exposure lock (`AEM_Manual`, bias 0.0, bloom 0.1) guarantees anti-blinding compliance under real-time Lumen illumination.

### 2. 128-Tick Authoritative Networking & Dual-Host Model
- **VPS Dedicated Server**: Headless, low-overhead binary runnable on any standard Windows or Linux VPS (`-server -nullrhi -log -PORT=7777 -TickRate=128 -MaxPlayers=18`).
- **Client Listen Room**: Peer-hosted rooms (`?listen`) launched directly from the main menu for instant matches with friends.
- **Direct Connect**: Direct IP and port entry (`127.0.0.1:7777` or remote public IP) in the in-game server browser.
- **High-Fidelity Replication**: Sub-millisecond hit registration, lag compensation, client prediction, and reconciliation.

### 3. Anatomical Human Male Skeletal Mechanics & Ragdoll Physics
- **Adult Male Proportions**: Rigged to a standard 180 cm standing height and 82.0 kg mass distribution based on military anthropometry (`SK_BaseMale`).
- **21-Bone Skeletal Registry**: Full hierarchy (`root`, `pelvis`, `spine_01` lumbar, `spine_02` mid-thoracic, `spine_03` upper chest, `neck_01`, `head`, `clavicles`, `arms`, `hands`, `thighs`, `calves`, `feet`).
- **Locomotion Pipeline**: 4 distinct motion sequences with dynamic stride matching: high-ready ADS idle with natural thoracic breathing, tactical walk, combat run, and tactical sprint.
- **PBR Micro-Ripstop Materials**: 2048x2048 seamless micro-weave normal/roughness, skin subsurface scattering (SSS), and 4 multi-zone camos (Urban Digital, Woodland Tactical, Desert Spec Ops, Stealth Blackout).
- **Authoritative Ragdoll Death Transition**: On zero health, character capsule collision uncouples instantly, all 21 rigid bodies simulate physics with ballistic momentum transfer, and camera boom seamlessly tracks the falling pelvis without snapping.
- **Ballistic Zoning & Armor Absorption**:
  - Head (2.5x critical lethal), Neck (2.0x), Upper Torso (1.25x), Pelvis (1.0x), Extremities (0.75-0.85x).
  - **Level IV Ceramic Armor Plates**: Absorbs kinetic energy on chest and mid-torso vitals.
- **Second-Order Spring Aim Punch**: Damped harmonic oscillator equation models realistic flinch dynamics and reticle recovery upon kinetic impact.
- **Procedural Foot IK & Limb Trauma**: Terrain-conforming feet with pelvis drop; arm trauma amplifies weapon sway, and leg trauma halts tactical sprint.

### 4. Classic MW2-Inspired Combat & Archetypes
- **Five Iconic Archetypes**:
  1. **Assault (M4A1)**: Red-dot optic, compensator, 40-round mag, 210 ms ADS.
  2. **Spec Ops Stealth (MP5SD)**: Integrated suppressor, subsonic 9mm, zero radar ping, 150 ms ADS.
  3. **Heavy Gunner (L86 LSW)**: 100-round continuous drum suppression, fluted barrel, 380 ms ADS.
  4. **Recon Sniper (Intervention .50)**: High-caliber one-shot torso/head lethality (1150 m/s velocity).
  5. **Run & Gun Rusher (Vector .45 ACP)**: 1200 RPM extreme fire rate, 120 ms ADS speed.
- **Tactical Ballistics**: Deterministic recoil climb, procedural breathing sway, and wall-mount stabilization.

### 5. Competitive Combat Map: "Rust Tactical" (60m x 60m)
- **Procedural C++ Map Architecture (`AFPSCombatArenaMapActor`)**: Automatically assembled with zero editor GUI dependency, compiling directly into standalone and dedicated server builds.
- **3 Tactical Lanes**:
  1. **West Flank (Container Yard)**: 4 multi-colored industrial shipping containers with an open shoot-through interior tunnel.
  2. **Central Tower & Chokepoint**: 12-meter tall oil derrick with multi-tier diamond-plate catwalks, dual 25-degree access ramps, armored vehicle wreck, and 8 concrete Jersey barriers.
  3. **East Flank (Pipeline & Bunker)**: 30-meter elevated oil pipeline trench leading into a reinforced concrete bunker.
- **Pure Real-Time Dynamic Lighting (Lumen)**: Zero static lightmass baking required (`r.AllowStaticLighting=False`), calibrated 10.0 lux desert directional sunlight, real-time sky atmosphere, volumetric dust haze, manual exposure lock to prevent whiteout, and a pulsing red aviation hazard beacon atop the derrick tower.
- **Domination Objectives & Tactical Spawning**: 3 holographic flag bases (A: Container Yard, B: Derrick Base, C: Bunker Trench) and 8 anti-spawn-trap PlayerStarts (4 Task Force 141 Alpha, 4 OpFor Bravo).

### 6. Procedural 3D Weaponry, ADS Alignment & 4-Phase Tactical Reload
- **Procedural 3D Tactical Carbine Model (`AFPSWeaponBase`)**:
  - 11 modular components: Receiver, Picatinny top rail, blued steel barrel, M-LOK handguard, muzzle brake compensator, pistol grip, buffer tube, buttstock, independent magazine assembly, reflex optic sight hood, and co-witnessed front iron sight post.
  - Authentic military finishes: Charcoal matte gunmetal, parkerized steel, textured tactical polymer, and a glowing green tritium front sight pip.
- **Hand Socketing & ADS Alignment**:
  - Socketed directly to the operator's right hand (`hand_r`), seated in a natural two-handed high-ready shooting pocket.
  - Smooth camera translation from hipfire to ADS center over the shoulder.
  - High-luminance emissive red dot reticle (`FLinearColor(15, 0.02, 0.02)`) provides clear target acquisition.
  - Smooth camera FOV magnification (90° &rarr; 68° in 180 ms) and dynamic mouse sensitivity scaling (0.65x).
  - 2D HUD crosshair collapses during ADS to prioritize the physical 3D reflex optic.
- **4-Phase Procedural Reload Sequence**:
  - **Phase 1 (Cant & Dip, 0-20%)**: Carbine rolls 22° outward and tilts up 6°, presenting the magwell into the operator's direct field of view.
  - **Phase 2 (Mag Ejection, 20-42%)**: Magazine root translates downward (-28 cm) and rotates out of the receiver.
  - **Phase 3 (Fresh Mag Insert & Palm Slap, 42-76%)**: Fresh magazine slides smoothly up into the magwell, ending in a tactile palm-slap bump impulse.
  - **Phase 4 (Bolt Rack & Return, 76-100%)**: Empty reloads execute a mechanical bolt catch release snap, followed by a smooth return glide to active ready.
- **Physical Recoil & Inertia Sway**:
  - Procedural weapon kickback and muzzle climb on each round fired, with rapid recovery.
  - Mouse look sway inertia lag in hipfire, dampened by 85% during ADS for rock-solid stability.

### 7. Hardware-Accelerated Slate Menu & Real-Time 3D Customization
- Built entirely in native **Slate C++** with translucent military HUD panels.
- **3D Turntable Preview Actor**: Rotates 360° continuously around Yaw (12°/s, 24°/s, 45°/s, or paused) with the operator holding weapon in Aim-Down-Sights (ADS) posture with live breathing motion.
- **Embedded Studio Lighting**: 3-point dynamic illumination (key spotlight, cyan rim light, fill light) with manual exposure locking.
- **Live Customization Controls**:
  - **Camo Patterns**: Urban Digital, Woodland Tactical, Desert Spec Ops, Stealth Blackout.
  - **Tactical Headgear**: FAST Ballistic Helmet, GPNVG-18 Quad-Nods (with phosphor green glow), Spec Ops Balaclava, Tactical Cap & Headset.
  - **Armor Vests**: Level IV Heavy Carrier, Lightweight JPC, Recon Tactical Rig.
  - **Stance Switching**: High-Ready ADS vs. Low-Ready Patrol.

### 8. Slate Tactical In-Game HUD & Telemetry
- **Top-Left Acoustic Minimap**:
  - 180x180 px circular radar display with 25m and 50m range rings and 120°/s rotating phosphor radar sweep.
  - Authoritative player forward chevron indicator and Domination flags A, B, and C with team color badges.
  - Acoustic gunfire detection: Hostile unsuppressed gunshots ping real-time radar blips with expanding dissipation rings.
- **Top-Center Radar Compass Tape**:
  - 440 px horizontal tape covering a 90° tactical bearing window.
  - Major 15° and minor 5° tick marks with bold cardinal points (`N`, `NE`, `E`, `SE`, `S`, `SW`, `W`, `NW`).
  - Centered digital heading readout box (`[ 045° ]`).
  - Objective badges and acoustic gunshot threat diamonds projected directly on the compass tape.
- **Bottom-Left Operator Vitals & Tactical Stamina**:
  - **Level IV Ceramic Armor Shield (100)**: Cobalt blue kinetic absorption bar protecting thoracic vitals (`spine_02`, `spine_03`).
  - **Health (100 HP)**: High-contrast digital HP readout with dynamic color shift (cyan &rarr; amber &rarr; critical red).
  - **Tactical Stamina (100)**: Emerald green sprint meter with 18.0/s sprint drain, 28.0/s rest/walk recovery, and exhaustion lock at 0 until restored to 15.
- **Bottom-Right Digital Ammo & Loadout**:
  - Weapon name, fire mode (`[FULL-AUTO 750 RPM]`), large 30pt digital counter with reserve (`30 / 120`), emergency low-ammo flashing ($\le 5$), and active `[>> RELOADING...]` banner.
- **Dynamic Crosshair & Hitmarkers**:
  - Spread-responsive 4-pip reticle that collapses during ADS for clean reflex optic sightlines.
  - Direct confirmed bullet impact diagonal `X` hitmarkers with rapid exponential fade.

---

## Quick Launch

Everything is launchable directly from the interactive start menu (`Launch_Menu.bat`), including instant match deployment into the Rust Tactical arena, 3D character customization, loadout selection, and server browsing. Headless dedicated hosting runs via the separate server binary launcher (`Launch_DedicatedServer.bat`).

### 1. Launch the Game & Start Menu
Double-click [`Launch_Menu.bat`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Launch_Menu.bat) or run:
```powershell
.\Launch_Menu.bat
```
*(Direct path: `Packaged\Windows\NobleFermi.exe -windowed -ResX=1280 -ResY=720 -log`)*

From the menu, click **DEPLOY NOW** on the Home tab to immediately enter the Rust Tactical combat arena!

### 2. Launch a Dedicated 128-Tick Server
Double-click [`Launch_DedicatedServer.bat`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Launch_DedicatedServer.bat) or run:
```powershell
.\Launch_DedicatedServer.bat
```
*(Runs headless on port 7777 at 128 Hz tickrate)*

---

## Project Structure

```
noble-fermi/
├── Config/                      # Engine, Game, Input and Packaging configuration (.ini)
├── Docs/                        # Technical guides & documentation
│   ├── ARCHITECTURE_AND_MECHANICS_GUIDE.md  # Deep-dive architecture & physics reference
│   └── GETTING_STARTED.md                   # Setup, launching, and dedicated hosting guide
├── Launch_Menu.bat              # Universal game launcher (Menu, 3D Customization, Match Hosting)
├── Launch_DedicatedServer.bat   # Headless 128-tick VPS dedicated server launcher
├── Packaged/Windows/            # Pre-cooked monolithic distribution (NobleFermi.exe)
└── Source/NobleFermi/           # Modular C++20 engine source
    ├── BodyMechanics/           # 21-bone anatomy, spring aim punch, foot IK
    ├── Character/               # FPSCharacter pawn & FPSCharacterPreviewActor turntable
    ├── Combat/                  # Weapon ballistics, health, Level IV armor plates
    ├── Game/                    # Match game modes, menu diorama director, controllers
    ├── Map/                     # FPSCombatArenaMapActor (Rust Tactical 60m x 60m procedural map)
    ├── Multiplayer/             # 128-tick authoritative multiplayer game mode and game state
    └── UI/                      # Slate military start menu, HUD, and settings widgets
```

---

## Detailed Documentation

- [**Architecture & Mechanics Guide**](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Docs/ARCHITECTURE_AND_MECHANICS_GUIDE.md): Mathematical formulations for spring aim punch, skeletal bone dimensions, damage tables, and networking pipeline.
- [**Getting Started Guide**](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Docs/GETTING_STARTED.md): Step-by-step instructions for launching, customization controls, VPS server configuration, and source compilation.

---

## License

All rights reserved © 2026 Noble Fermi Studio.
