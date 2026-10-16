# NOBLE FERMI // MODERN MILITARY TACTICAL WARFARE

[![Engine](https://img.shields.io/badge/Unreal%20Engine-5.8-blue.svg)](https://www.unrealengine.com/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B20-orange.svg)](https://en.cppreference.com/w/cpp/20)
[![Tickrate](https://img.shields.io/badge/Tickrate-128%20Hz-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20VPS-lightgrey.svg)]()
[![Build](https://img.shields.io/badge/Build-Standalone%20Monolithic-success.svg)]()
[![Website](https://img.shields.io/badge/Live%20Website-noble--fermi.pages.dev-cyan.svg)](https://noble-fermi.pages.dev/)

> **Noble Fermi** is a competitive tactical shooter built from the ground up in Unreal Engine 5.8 C++, engineered around an authoritative **tactical 3rd-person over-the-shoulder perspective** and **"The Fermi Paradox Engine" (Dive, Clone, Swap, Shatter Time)**. It blends golden-era tactical gunplay with **Max Payne-style Omnidirectional Ballistic Dives**, localized **4.5m Superhot-style Time-Dilation Fields (`AFPSFermiTimeBubble`)**, **Acoustic Hologram Decoys & Mid-Air Quantum Phase-Swapping (`AFPSFermiDecoyActor`)**, **128-tick dedicated server networking**, an **anatomically accurate 21-bone human male operator (`SK_BaseMale`)** holding a 3D **`SM_AK47`** rifle, and a **hardware-accelerated Slate C++ start menu & AR visor HUD**.

---

## Key Pillars & Engineering Features

```
┌────────────────────────────────────────────────────────────────────────┐
│                              NOBLE FERMI                               │
│         3rd-Person Tactical Warfare & The Fermi Paradox Engine         │
└────────────────────────────────────────────────────────────────────────┘
          │                                              │
          ▼                                              ▼
┌──────────────────────────────┐        ┌──────────────────────────────┐
│  The Fermi Paradox Engine    │        │       Combat & Anatomy       │
├──────────────────────────────┤        ├──────────────────────────────┤
│ • Omnidirectional Dive (Alt) │        │ • 21 Human Male Bones (180cm)│
│ • 4.5m 0.25x Chrono Bubble   │        │ • 3D AK-47 on weapon_r Socket│
│ • Acoustic Fermi Echo ([Q])  │        │ • OTS 3P + Hybrid 1P ADS     │
│ • Mid-Air Quantum Phase-Swap │        │ • Level IV Plates & Ragdolls │
└──────────────────────────────┘        └──────────────────────────────┘
          │                                              │
          └──────────────────────┬───────────────────────┘
                                 │
                                 ▼
┌────────────────────────────────────────────────────────────────────────┐
│        128-Tick Multiplayer, Slate AR Visor HUD & 3D Diorama           │
├────────────────────────────────────────────────────────────────────────┤
│ • Pure Slate C++ Minimap, Compass Tape, ECG Vitals & Chrono Telemetry  │
│ • 360° Turntable with Live Aimed ADS, Low-Ready & Walking AK-47 Stances│
│ • 128 Hz Dedicated VPS Server & Instant Client Listen Rooms (?listen)  │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. The Fermi Paradox Engine (Dive, Clone, Swap, Shatter Time)
- **Max Payne Omnidirectional Ballistic Dive (`LeftAlt` or `Move + Double-Tap C` / `Ctrl+Space`)**:
  - Launches the operator along any `WASD` vector (`820 cm/s` horizontal, `340 cm/s` vertical, `35.0` Stamina cost).
  - Tilts the 21-bone `SK_BaseMale` body **$75^\circ$ into the dive direction** (chest-down forward, supine on the back when diving backward, side-roll left/right) with **full $360^\circ$ mid-air aiming and firing**, followed by a **$0.45\text{ s}$ kinetic ground slide** on landing.
- **Localized "Fermi Time-Dilation Field" (`AFPSFermiTimeBubble`)**:
  - Automatically expands a **4.5-meter ($450\text{ cm}$) chrono sphere** around the diving operator (and around any active Fermi Echo decoy) for **$2.4\text{ seconds}$**.
  - Slows enemy pawns and incoming projectiles inside the bubble to **$25\%$ speed** (`CustomTimeDilation = 0.25f`) while granting the diving operator **$55\%$ reflex speed** (`0.55f`) with $1:1$ un-dilated mouse rotation compensation.
  - Spawns physicalized **brass ballistic tracers (`AFPSBallisticTracer`)** traveling at $2,800\text{ cm/s}$ with glowing cyan/amber wake trails so you can watch bullets slice past your head in slow motion.
- **"Fermi Echo" Acoustic Hologram Decoy & Mid-Air Quantum Swap (`AFPSFermiDecoyActor`, `Q` Key)**:
  - **Press `Q` (Deploy Echo)**: Spawns an autonomous phosphor-cyan holographic clone of your 21-bone operator holding an AK-47 that sprints forward at $680\text{ cm/s}$ and fires simulated unsuppressed bursts every $0.28\text{ s}$, flooding enemy minimaps and compass tapes with red gunfire threat diamonds via `UFPSRadarSubsystem`.
  - **Press `Q` Again (Quantum Phase-Swap)**: Instantly swaps world positions with your active Fermi Echo—even mid-air during a slow-motion dive—conserving and redirecting your ballistic dive velocity along your sightline.

### 2. Primary Tactical 3rd-Person Over-the-Shoulder Camera System
- **Over-the-Shoulder Framing (`CameraBoom` + `FollowCamera`)**:
  - **Hipfire & Traversal**: Camera sits $210\text{ cm}$ back with a $(0, \pm 52, 22)\text{ cm}$ shoulder offset and $90^\circ$ FOV.
  - **Tight OTS ADS Zoom (`RMB`)**: Smoothly contracts arm length from $210\text{ cm}$ to $95\text{ cm}$, tightens socket offset to $(0, \pm 34, 16)\text{ cm}$, narrows FOV from $90^\circ$ to $62^\circ$, and applies $0.65\times$ mouse sensitivity scaling.
  - **Hybrid 1st-Person True Optic ADS (`MiddleMouseButton` while aiming)**: Toggles between 3rd-Person Tight OTS ADS and 1st-Person True Optic ADS (`TargetArmLength = 0.0f`).
  - **Smooth Shoulder Swapping (`V` / Thumb Mouse)**: Smoothly interpolates `ShoulderSideMultiplier` between right shoulder ($+1.0$) and left shoulder ($-1.0$) at speed $10.0\text{ s}^{-1}$ for corner peeking.
  - **Collision Probe & Anti-Whiteout Exposure**: $12\text{ cm}$ probe radius prevents wall clipping; strict manual exposure lock (`AEM_Manual`, bias $0.0$, bloom $0.1$) guarantees zero screen whiteout under Lumen illumination.

### 3. 128-Tick Authoritative Networking & Dual-Host Model
- **VPS Dedicated Server**: Headless, low-overhead binary runnable on any standard Windows or Linux VPS (`-server -nullrhi -log -PORT=7777 -TickRate=128 -MaxPlayers=18`).
- **Client Listen Room**: Peer-hosted rooms (`?listen`) launched directly from the main menu for instant matches with friends.
- **Direct Connect**: Direct IP and port entry (`127.0.0.1:7777` or remote public IP) in the in-game server browser.
- **High-Fidelity Replication**: Sub-millisecond hit registration, lag compensation, client prediction, and reconciliation.

### 4. Anatomical Human Male Skeletal Mechanics, 3D AK-47 & Ragdoll Physics
- **Adult Male Proportions**: Rigged to a standard $180\text{ cm}$ standing height and $82.0\text{ kg}$ mass distribution based on military anthropometry (`SK_BaseMale`).
- **21-Bone Skeletal Registry + `weapon_r` Socket**: Full hierarchy (`root`, `pelvis`, `spine_01`, `spine_02`, `spine_03`, `neck_01`, `head`, `clavicles`, `arms`, `hands`, `weapon_r`, `thighs`, `calves`, `feet`).
- **3D AK-47 Rifle (`SM_AK47`) & IK-to-FK Baked Stances**:
  - Authentic 3D AK-47 mesh (stamped steel receiver, walnut stock & handguard, angled pistol grip, curved 30-round Bakelite magazine) attached 1:1 to `weapon_r`.
  - 5 IK-baked motion sequences keeping both hands locked to the rifle's pistol grip and forend: **Aimed ADS Idle** (`Anim_BaseMale_Idle_ADS`), **Low-Ready Patrol** (`Anim_BaseMale_LowReady`), **Aimed Tactical Walk** (`Anim_BaseMale_Walk`), **Combat Run** (`Anim_BaseMale_Run`), and **Tactical Sprint** (`Anim_BaseMale_Sprint`).
- **PBR Micro-Ripstop Materials**: $2048\times 2048$ seamless micro-weave normal/roughness, skin subsurface scattering (SSS), and 4 multi-zone camos (Urban Digital, Woodland Tactical, Desert Spec Ops, Stealth Blackout).
- **Authoritative Ragdoll Death Transition**: On zero health, character capsule collision uncouples instantly, all 21 rigid bodies simulate physics with ballistic momentum transfer, and camera boom seamlessly tracks the falling pelvis without snapping.
- **Ballistic Zoning, Level IV Ceramic Plates & Spring Aim Punch**:
  - Head ($2.5\times$ critical lethal), Neck ($2.0\times$), Upper Torso ($1.25\times$), Pelvis ($1.0\times$), Extremities ($0.75\text{–}0.85\times$), with Level IV Ceramic Armor protecting `spine_02`/`spine_03` and a 2nd-order damped harmonic oscillator aim punch equation.

### 5. Competitive Combat Map: "Rust Tactical" (60m x 60m)
- **Procedural C++ Map Architecture (`AFPSCombatArenaMapActor`)**: Automatically assembled with zero editor GUI dependency, compiling directly into standalone and dedicated server builds.
- **3 Tactical Lanes**:
  1. **West Flank (Container Yard)**: 4 multi-colored industrial shipping containers with an open shoot-through interior tunnel and Domination Flag A.
  2. **Central Tower & Chokepoint**: 12-meter tall oil derrick with multi-tier diamond-plate catwalks, dual 25-degree access ramps, armored vehicle wreck, 8 concrete Jersey barriers, and Domination Flag B.
  3. **East Flank (Pipeline & Bunker)**: 30-meter elevated oil pipeline trench leading into a reinforced concrete bunker and Domination Flag C.
- **Pure Real-Time Dynamic Lighting (Lumen)**: Zero static lightmass baking required (`r.AllowStaticLighting=False`), calibrated 10.0 lux desert directional sunlight, real-time sky atmosphere, volumetric dust haze, and manual exposure lock.

### 6. Hardware-Accelerated Slate Menu, 3D Customization & AR Visor HUD
- **3D Turntable Preview Actor (`AFPSCharacterPreviewActor`)**: Rotates $360^\circ$ continuously around Yaw with live switching between **`[ AIMED ADS ]`**, **`[ LOW READY ]`**, and **`[ WALK ANIM ]`** stances, 4 camo patterns, headgear, and armor carriers.
- **Future-Warfare Slate AR Visor HUD (`SFPSHUDWidget`)**:
  - **Top-Left Circular Minimap**: $180\times 180\text{ px}$ scope with $25\text{ m}/50\text{ m}$ range rings, FOV cone, rotating sweep fan, objective diamonds, and expanding acoustic gunfire ripples.
  - **Top-Center Compass Tape & Chronometer**: Vignette-faded horizontal bearing tape with cardinal points, threat diamonds, and an active `CHRONO FIELD // 0.25x c` ticker during time dilation.
  - **Bottom-Left Vitals, 4-Plate Armor, Live ECG & Fermi Chrono Bar**: Skewed parallelogram health/armor bars with trailing damage ghost, live ECG waveform, 12-cell kinetic stamina meter, and the **`FERMI DIVE // CHRONO FIELD`** status bar.
  - **Bottom-Right 30-Round Bullet Pip Matrix & Quantum Echo Status**: Individual bullet pip matrix, digital ammo counters, radial 4-phase reload ring, and `[Q] DEPLOY FERMI ECHO` / `[Q] QUANTUM SWAP READY` telemetry.

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
│   ├── PLAYER_LOCOMOTION_AND_STANCES_PLAN.md # Roadmap & pipeline for weapon stances, 8-way locomotion, jumping, crouching, sliding
│   ├── FERMI_PARADOX_ENGINE.md              # Deep-dive physics & architecture of Dive, Chrono Bubble & Quantum Swap
│   ├── ARCHITECTURE_AND_MECHANICS_GUIDE.md  # System architecture, 21-bone anatomy, ballistics & HUD reference
│   ├── AGENT_STARTER_PROMPT_AND_PROJECT_STATE.md # Master project history & AI agent onboarding prompt
│   ├── GAME_MECHANICS_PROMPT.md             # Fermi Paradox Engine design blueprint
│   └── GETTING_STARTED.md                   # Setup, controls, customization, and dedicated hosting guide
├── Launch_Menu.bat              # Universal game launcher (Menu, 3D Customization, Match Hosting)
├── Launch_DedicatedServer.bat   # Headless 128-tick VPS dedicated server launcher
├── Packaged/Windows/            # Pre-cooked monolithic distribution (NobleFermi.exe)
└── Source/NobleFermi/           # Modular C++20 engine source
    ├── BodyMechanics/           # 21-bone anatomy, spring aim punch, foot IK
    ├── Character/               # FPSCharacter (OTS, Dive, Quantum Swap) & FPSCharacterPreviewActor
    ├── Combat/                  # FPSFermiTimeBubble, FPSFermiDecoyActor, FPSBallisticTracer, Health
    ├── Game/                    # Match game modes, menu diorama director, controllers
    ├── Map/                     # FPSCombatArenaMapActor (Rust Tactical 60m x 60m procedural map)
    ├── Multiplayer/             # 128-tick authoritative multiplayer game mode and game state
    ├── Radar/                   # UFPSRadarSubsystem acoustic gunfire & UAV sweep tracking
    ├── UI/                      # Slate military start menu, AR visor HUD, and settings widgets
    └── Weapons/                 # 3D SM_AK47 & modular weapon ballistics, gunsmith, inventory
```

---

## In-Game Showcase // 3rd-Person Tactical Gameplay

![Noble Fermi - Tactical 3rd-Person Over-the-Shoulder Gameplay on Rust Tactical](Docs/images/gameplay_3rd_person_rust_tactical.png)

*Live in-engine capture of the 21-bone tactical operator (`SK_BaseMale`) holding the 3D `SM_AK47` rifle in over-the-shoulder perspective on Rust Tactical, featuring the holographic AR visor HUD (acoustic gunfire radar minimap, compass ticker, Level IV ceramic plate vitals, kinetic stamina, and the 128-tick Fermi Dive chrono field telemetry).*

---

## Detailed Documentation

- [**Player Locomotion, Weapon Stances & Kinematics Plan**](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Docs/PLAYER_LOCOMOTION_AND_STANCES_PLAN.md): Step-by-step roadmap, Blender animation pipeline, mathematical formulas, and state machine architecture for weapon lowering/aiming, 8-way locomotion, jumping, crouching, and tactical sliding.
- [**The Fermi Paradox Engine Specification**](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Docs/FERMI_PARADOX_ENGINE.md): Complete physics equations, sequence diagrams, and C++ architecture for Omnidirectional Ballistic Dives, Localized Time-Dilation Bubbles, Acoustic Hologram Decoys, and Mid-Air Quantum Phase-Swapping.
- [**Architecture & Mechanics Guide**](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Docs/ARCHITECTURE_AND_MECHANICS_GUIDE.md): Mathematical formulations for spring aim punch, skeletal bone dimensions, damage tables, and networking pipeline.
- [**Getting Started Guide**](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Docs/GETTING_STARTED.md): Step-by-step instructions for launching, customization controls, Fermi Paradox Engine combat controls, VPS server configuration, and source compilation.
- [**Project Master Summary & Agent Handoff**](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Docs/AGENT_STARTER_PROMPT_AND_PROJECT_STATE.md): Full chronological engineering log and onboarding reference.

---

## License

All rights reserved © 2026 Noble Fermi Studio.
