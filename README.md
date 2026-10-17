<p align="center">
  <img src="Docs/images/noble_fermi_logo.png" alt="NOBLE FERMI — Battle-Scarred Heavy Metal Wordmark Logo" width="760" />
</p>

# NOBLE FERMI // CHRONO-TACTICAL BALLISTIC WARFARE

[![Engine](https://img.shields.io/badge/Unreal%20Engine-5.8-00E5FF.svg?style=flat-square&logo=unrealengine)](https://www.unrealengine.com/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B20-FFB300.svg?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![Tickrate](https://img.shields.io/badge/Tickrate-128%20Hz%20Authoritative-00E676.svg?style=flat-square)]()
[![UI](https://img.shields.io/badge/UI-Pure%20Slate%20C%2B%2B-06B6D4.svg?style=flat-square)]()
[![Build](https://img.shields.io/badge/Distribution-IoStore%20Monolithic-10B981.svg?style=flat-square)]()
[![Web Portal](https://img.shields.io/badge/Web%20Portal-noble--fermi.pages.dev-00E5FF.svg?style=flat-square)](https://noble-fermi.pages.dev/)

> **Noble Fermi** is an authoritative 128-tick tactical shooter built natively in **Unreal Engine 5.8 C++20**. Engineered around a responsive **3rd-person over-the-shoulder (OTS) combat perspective** and **The Fermi Paradox Engine**, it fuses high-lethality military gunplay, physical **Level IV Ceramic Armor Plate fracture**, localized **0.25x relativistic time-dilation spheres (`AFPSFermiTimeBubble`)** that convert incoming hitscan rounds into dodgeable 3D brass slugs (`AFPSDilatedBulletActor`), **Acoustic Hologram Decoys & Mid-Air Quantum Phase-Swapping (`AFPSFermiDecoyActor`)**, and a **zero-UMG Pure Slate C++ AR Visor HUD & 3-Zone Acrylic Command Lobby**.

---

## System Architecture Overview

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                                 NOBLE FERMI                                  │
│         Unreal Engine 5.8 C++20 // 128-Hz Chrono-Tactical Architecture       │
└──────────────────────────────────────────────────────────────────────────────┘
             │                                                  │
             ▼                                                  ▼
┌────────────────────────────────────┐            ┌────────────────────────────────────┐
│      The Fermi Paradox Engine      │            │   21-Bone Anatomy & Trauma Rig     │
├────────────────────────────────────┤            ├────────────────────────────────────┤
│ • Omnidirectional Chrono-Dive      │            │ • SK_BaseMale (180cm, 82kg, 21-B)  │
│ • 450cm 0.25x Time-Dilation Sphere │            │ • UFPSAnimInstance + Two-Bone IK   │
│ • Physical 850 cm/s Dilated Slugs  │            │ • 3D SM_AK47 (weapon_r/weapon_l_ik)│
│ • 120cm Graze Dodge (+20 Stamina)  │            │ • Lvl-IV Ceramic Shard Ejection    │
│ • [Q] Acoustic Decoy & Bot Baiting │            │ • 2.5x Headshot Helmet Pop Physics │
│ • [Q+Q] Mid-Air Quantum Phase-Swap │            │ • 4 Runtime Camo & Weapon Finishes │
└────────────────────────────────────┘            └────────────────────────────────────┘
             │                                                  │
             └────────────────────────┬─────────────────────────┘
                                      │
                                      ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│     128-Tick Multiplayer, Pure Slate AR Visor HUD & 3-Zone Acrylic Lobby     │
├──────────────────────────────────────────────────────────────────────────────┤
│ • 3-Zone Frosted Glass Lobby (SBackgroundBlur) + Unobstructed 3D Operator    │
│ • 36-Segment Circular Radar, Compass Tape, 4-Plate Vitals & [ ! BAITED ] HUD │
│ • Responsive 3-Lane Tactical Bot AI (VIPER-1, SPECTRE-2, JACKAL-3, PHANTOM-4)│
│ • Persistent Economy Subsystem (Gold/Opals), Quartermaster & Daily Bounties  │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## Core Engineering Pillars

### 1. The Fermi Paradox Engine (Dive, Dilate, Bait, Phase-Swap)
- **Omnidirectional Ballistic Dive (`LeftAlt` or `Sprint + Space/C`)**:
  - Launches the operator along any `WASD` input vector (`820 cm/s` horizontal, `340 cm/s` vertical, `25.0` Stamina cost) with full $360^\circ$ mid-air aiming and firing.
  - Dynamically pitches and rolls the 21-bone `SK_BaseMale` posture into the dive vector, transitioning smoothly into a kinetic ground slide on touchdown.
- **Relativistic Time-Dilation Sphere (`AFPSFermiTimeBubble`) & Physical Dilated Slugs (`AFPSDilatedBulletActor`)**:
  - Deploys a **`450.0f cm` chrono-dilation sphere** that slows actors inside to **`0.25x` time** while preserving un-dilated `1:1` camera aim rotation for the diving operator.
  - **Hitscan-to-Projectile Conversion**: Any hitscan shot entering an active Fermi Bubble is intercepted at the sphere boundary and converted into a physical 3D brass slug (`AFPSDilatedBulletActor`) traveling at **`850.0f cm/s`** with a glowing cyan/amber ribbon wake.
  - **Near-Miss Bullet Graze (`120.0f cm`)**: Dodging a dilated slug within `120.0f cm` without taking damage triggers **`HandleBulletGraze`**, refunding **`+20.0f` Stamina**, awarding **`BULLET DODGE (+50 GOLD)`**, and progressing the **Matrix Dodge** daily bounty.
- **"Fermi Echo" Acoustic Hologram Decoy & Mid-Air Quantum Swap (`AFPSFermiDecoyActor`, `Q` Key)**:
  - **First Press (`Q` — Deploy Acoustic Echo)**: Spawns a translucent phosphor-green wireframe hologram of the 21-bone operator (`4.0s` lifespan, `9.0s` cooldown) that sprints forward at **`620.0f cm/s`** and fires simulated unsuppressed blanks every `0.45s`.
  - **Acoustic Bot Baiting & Triangulation**: Decoy gunfire pings hostile red diamonds on `UFPSRadarSubsystem` and alerts all enemy Tactical Bots within `2,500.0f` cm with line-of-sight, locking them into a **`[ ! BAITED ]`** state for `2.5s` where they prioritize turning and firing bursts at the hologram. If an enemy shoots the decoy, the attacker's location is triangulated (`DECOY BAITED // +50 GOLD`).
  - **Second Press (`Q` — Quantum Phase-Swap)**: Instantly swaps 3D world transforms with the active hologram decoy while preserving mid-air Ballistic Dive velocity, applying a `+12.0°` FOV punch and opening a `3.0s` **`FERMI PHANTOM (+150 GOLD)`** kill window.

### 2. 21-Bone Anthropometry, Two-Bone Left-Hand IK & Visceral Trauma Physics
- **Unified 21-Bone Skeletal Hierarchy (`SK_BaseMale` + `UFPSAnimInstance`)**:
  - Rigged to $180\text{ cm}$ standing height and $82.0\text{ kg}$ military anthropometry (`root`, `pelvis`, `spine_01..03`, `neck_01`, `head`, clavicles, upper/lower arms, hands, `weapon_r`, thighs, calves, feet).
  - Driven by `UFPSAnimInstance` with native **Two-Bone Left-Hand IK** locked to `weapon_l_ik` on `SM_AK47`, `BS_BaseMale_Locomotion` blend space, procedural spine aim-offset pitch, and 2nd-order harmonic spring aim punch.
- **Visceral 3rd-Person Level IV Ceramic Armor Shatter (`MulticastTriggerArmorPlateShatter`)**:
  - Operators wear a 3rd-person `ChestCarrierMesh` (`spine_03`) and `BallisticHelmetMesh` (`head`).
  - When an opponent's `100` Level IV Ceramic Armor Shield reaches `0`, `MulticastTriggerArmorPlateShatter` ejects a physical tumbling ceramic shard (`440 cm/s` with gravity and ground bounce), bursts 5 ballistic spark tracers from the chest, transitions the carrier mesh to a shredded battle-damaged state, and awards **`PLATE SHATTERED (+75 GOLD)`**.
- **Headshot Ballistic Helmet Pop (`MulticastPopBallisticHelmet`) & Anatomical Multipliers**:
  - **Head (`2.5x`)**: Lethal headshots impart an upward rotational impulse (`580 cm/s`) that pops the enemy's ballistic helmet off into the air.
  - **Upper Chest (`1.0x`)**, **Clavicle/Neck (`1.2x`)**, **Stomach/Pelvis (`0.9x`)**, **Limbs (`0.75x`)**: Limb hits trigger directional upper-body trauma flinch reads.

### 3. 128-Hz Authoritative Multiplayer, Responsive Tactical Bots & Rust Tactical (60m × 60m)
- **Rust Tactical 3-Lane Arena (`AFPSCombatArenaMapActor`)**:
  - Procedurally constructed in C++ with zero editor GUI dependency and pure real-time dynamic Lumen illumination (`r.AllowStaticLighting=False`, locked manual exposure `AEM_Manual` for zero whiteout):
    - **Flag A (West Flank — Container Yard)**: `[-14.0m, -10.0m]`
    - **Flag B (Center Chokepoint — Oil Derrick Core)**: `[+00.0m, +00.0m]`
    - **Flag C (East Flank — Pipeline Bunker)**: `[+14.0m, +10.0m]`
- **Responsive Tactical Bot AI (`AFPSMultiplayerGameMode`)**:
  - Automatically populates the arena with 4 Team Bravo Spec-Ops operators (**`VIPER-1`**, **`SPECTRE-2`**, **`JACKAL-3`**, **`PHANTOM-4`**) that patrol between Flags A, B, and C, strafe into cover during firefights, fire controlled bursts that populate slow-mo bullets inside Fermi Bubbles, react to acoustic hologram decoys, and respawn cleanly after `2.5s`.
- **Acoustic Gunfire Radar (`UFPSRadarSubsystem`) & Spatial Audio (`UFPSAudioSubsystem`)**:
  - All 34 cooked WAV assets (concrete/metal/sand/water/wood footsteps, 4-phase reloads, suppressed/unsuppressed gunfire, bullet flyby cracks, ambient rain/wind) are packaged into the IoStore container (`+DirectoriesToAlwaysCook=(Path="/Game/Audio")`).

### 4. Pure Slate C++ AR Visor HUD, 3-Zone Acrylic Lobby & Dual-Currency Economy
- **3-Zone Modern Warfare Acrylic Command Lobby (`UMainMenuWidget` & `AFPSCharacterPreviewActor`)**:
  - Frosted glass panels (`SBackgroundBlur` strength `7.0f`–`14.0f`, slate `rgba(0.03, 0.06, 0.10, 0.68)`) with Tactical Cyan (`#00E5FF`), Afterburner Amber (`#FFB300`), Crimson Alert (`#FF2A54`), and Emerald Ready (`#00E676`) 4-corner brackets.
  - Keeps the center 50% of the viewport completely unobstructed to frame the live 360° 3D operator showcase (`AFPSCharacterPreviewActor`) lit by 3-point warm amber key and cool cyan rim lighting.
  - **Live Non-Pausing Tactical Escape Overlay (`UFPSEscapeMenuWidget`)**: Pressing `ESC` in-match toggles a sleek translucent overlay (FOV slider `75°–110°`, mouse sensitivity, audio mixer, instant operator redeploy, or return to lobby) without freezing the 128-Hz simulation.
- **Free-to-Play Dual-Currency Economy, Daily Bounties & Slate Minigames (`UFPSEconomySubsystem`)**:
  - **Persistent Profile Serialization (`Saved/NobleFermiProfile.json`)**: Automatically saves and restores **Gold** (`1,500` starting), rare **Crystalline Opals** (`50` starting), unlocked catalog items, daily streak state, and the player's **Equipped 3rd-Person Loadout** (`EquippedCamoIndex`, `EquippedHeadgearIndex`, `EquippedArmorIndex`, `EquippedWeaponFinishIndex`) across lobby and arena sessions.
  - **Unlockable Camo, Tactical Gear & NF-98 Weapon Finishes**: Zero pay-to-win cosmetic catalog purchasable with *either* earned Gold *or* Opals, featuring live 360° turntable previewing for locked and unlocked items and 4 PBR rifle finishes (**Factory Blued Steel**, **Desert Tan Cerakote**, **Carbon Fiber Spec-Ops**, and **24K Gold & Opal Damascene**).
  - **3-Slot Daily Tactical Contracts (`SFPSBountyProgressBarWidget`)**: Real-time combat events (`"ChronoDiveKill"`, `"FermiSwapKill"`, `"BulletDodge"`, `"PlateShatter"`) progress daily bounties (**Chrono-Hunter**, **Quantum Deception**, **Untouchable**) rendered with pure Slate vector progress bars and on-demand contract rerolling.
  - **7-Day Login Streak Vault & Hardware-Accelerated Slate Minigames**: Includes the **Tactical Fortune Wheel (`SFPSFortuneWheelWidget`)** with exponential rotational friction decay and the **Tactical Dartboard (`SFPSDartboardWidget`)** with Lissajous oscillating reticle timing and radial vector spark bursts on Jackpot/Bullseye hits.
- **Pure Slate C++ AR Visor HUD (`SFPSHUDWidget`)**:
  - **+20% Ergonomic Visor Scale (`1.20x`) & -25% Shake Reduction (`0.75x`)** across all corner telemetry panels for high-tempo competitive clarity.
  - Custom GPU vertex/line renderer featuring a 36-segment circular radar minimap, cosine-faded horizontal compass tape, 4-plate Level IV ceramic armor cells with trailing damage ghost bars, live ECG waveform, 30-round individual bullet pip matrix, differentiated hitmarkers (White standard, Cobalt Blue armor shield, Crimson expanding double-X headshot/kill), and 3D-projected **`[ ! BAITED ]`** diamond indicators above baited enemies.
  - **Center-Screen Directional Threat & Visual Audio Radar (Deaf Accessibility)**: Renders a `165px` semicircle + outward chevron ring around the crosshair color-coded by threat priority — **Crimson (`#FF2A54`)** for incoming damage, **Amber (`#FFB300`)** for gunfire & supersonic bullet flybys, and **Cyan (`#00E5FF`)** for enemy footsteps.

### 5. Dark-Luxury Interactive Web Portal (`web/index.html`)
- Built with **GSAP + ScrollTrigger** and **Lenis Smooth Scroll** using a strict `cubic-bezier(0.22, 1, 0.36, 1)` micro-interaction easing token and full `prefers-reduced-motion` accessibility support (plus an interactive header motion toggle).
- **Emulated Water Droplets in a Sea of Pixels (`#pixelSeaCanvas`)**: Real-time 2D damped wave propagation equation running on a quantized micro-pixel grid where pointer velocity, clicks, and autonomous quantum droplets refract Tactical Cyan (`#00E5FF`) and Afterburner Amber (`#FFB300`) wave crests.
- **Scroll-Linked Sticky Architectural Deck**: Four progressive full-viewport chapters (**01 // Chrono Engine**, **02 // 21-Bone & Armor**, **03 // 128-Hz Netcode**, **04 // Monolithic Deployment**) that smoothly scale down and recede with depth blur as subsequent cards slide over, complete with interactive Chrono-Sphere telemetry tabs, live Armor Shatter / Helmet Pop simulators, and one-click copyable CLI launch blocks.

---

## Quick Launch & Deployment

### 1. Launch Standalone Client (3-Zone Lobby → Rust Tactical Arena)
Double-click `Launch_Menu.bat` or run from PowerShell:
```powershell
.\Launch_Menu.bat
# Or invoke the packaged monolithic binary directly:
Packaged\Windows\NobleFermi.exe -windowed -ResX=1280 -ResY=720 -log
```
Click **`DEPLOY TO ARENA`** on the left blade to drop directly into Rust Tactical with active Team Bravo tactical bots.

### 2. Launch 128-Hz Headless Dedicated VPS Server
Double-click `Launch_DedicatedServer.bat` or run on Windows / Linux VPS:
```powershell
.\Launch_DedicatedServer.bat
# Headless CLI flags:
Packaged\Windows\NobleFermi.exe ?game=/Script/NobleFermi.FPSMultiplayerGameMode -server -nullrhi -log -PORT=7777 -TickRate=128 -MaxPlayers=18
```

---

## Combat Controls Quick Reference

| Input | Action | Tactical System |
| :--- | :--- | :--- |
| `W A S D` + `Left Shift` | 8-Way Locomotion / Tactical Sprint | `UFPSCharacterMovementComponent` |
| `Left Alt` *(or Sprint + `Space`)* | **Omnidirectional Ballistic Dive** | Spawns `450cm` `0.25x` `AFPSFermiTimeBubble` |
| `Q` *(1st Press)* | **Deploy Fermi Echo Decoy** | Spawns `620 cm/s` acoustic hologram (`AFPSFermiDecoyActor`) |
| `Q` *(2nd Press while Active)* | **Quantum Phase-Swap** | Swaps 3D transform & preserves dive momentum (`3.0s` Phantom window) |
| `C` / `Left Ctrl` | Crouch / Kinetic Slide | Transitions into low-profile slide when sprinting |
| `Left Mouse Button` | Fire Primary Weapon | `AFPSWeaponBase` (`SM_AK47` / `NF-98 Carbine`) |
| `Right Mouse Button` | Tight OTS ADS Zoom | Contracts camera boom & narrows FOV (`90° → 62°`) |
| `Middle Mouse Button` *(in ADS)* | Hybrid 1P Optic ADS Toggle | Switches between 3rd-person OTS and true 1st-person sightline |
| `V` | Swap Camera Shoulder | Smoothly interpolates right/left shoulder peeking |
| `B` | Cycle Fire Mode | `FULL AUTO` / `3-ROUND BURST` / `SEMI-AUTO` |
| `G` / `4` | Cook Frag / Throw Flashbang | `AFPSExplosiveProjectile` |
| `ESC` | Live Tactical Escape Menu | Non-pausing overlay (`UFPSEscapeMenuWidget`) |

---

## Repository Structure

```
noble-fermi/
├── Config/                      # Engine, Game (+DirectoriesToAlwaysCook), Input & Packaging (.ini)
├── Content/                     # Cooked 21-bone SK_BaseMale, SM_AK47, PBR materials & 34 WAV audio assets
├── Docs/                        # Architectural blueprints, master tasklist & engineering documentation
├── Scripts/                     # Python asset pipeline scripts (materials, sockets, blendspaces, audio)
├── Source/NobleFermi/           # Unreal Engine 5.8 C++20 Monolithic Module
│   ├── Audio/                   # UFPSAudioSubsystem spatial falloff & acoustic occlusion
│   ├── BodyMechanics/           # 21-bone registry, anatomical zones, 2nd-order spring aim punch
│   ├── Character/               # AFPSCharacter, UFPSAnimInstance (Two-Bone IK), AFPSCharacterPreviewActor
│   ├── Combat/                  # AFPSFermiTimeBubble, AFPSFermiDecoyActor, AFPSBallisticTracer, Health
│   ├── Economy/                 # UFPSEconomySubsystem persistent Gold/Opals, Loadout & Bounties
│   ├── Game/                    # Match game modes, menu diorama director & player controllers
│   ├── Inventory/               # UFPSInventoryComponent primary/secondary/grenade slots
│   ├── Map/                     # AFPSCombatArenaMapActor (Rust Tactical 60m x 60m procedural arena)
│   ├── Multiplayer/             # AFPSMultiplayerGameMode (128-Hz + Tactical Bot AI), GameState, PlayerState
│   ├── Radar/                   # UFPSRadarSubsystem acoustic gunfire & threat triangulation
│   ├── UI/                      # SFPSHUDWidget (Pure Slate AR Visor), MainMenuWidget, EscapeMenuWidget
│   └── Weapons/                 # AFPSWeaponBase, AFPSDilatedBulletActor, AFPSExplosiveProjectile
├── web/                         # Dark-luxury GSAP + Lenis + Hydro-Pixel Wave web portal (index.html)
├── Launch_Menu.bat              # Universal standalone client launcher
└── Launch_DedicatedServer.bat   # 128-Hz headless dedicated server launcher
```

---

## Engineering Documentation Index

- [**Master Engineering Tasklist & Roadmap (`Docs/MASTER_TASKLIST.md`)**](Docs/MASTER_TASKLIST.md): End-to-end milestone tracker covering OTS Combat, the Fermi Loop, AR Visor HUD accessibility, Dual-Currency Economy, Daily Bounties, and the 3-Agent Build Pipeline.
- [**The Fermi Paradox Engine Specification (`Docs/FERMI_PARADOX_ENGINE.md`)**](Docs/FERMI_PARADOX_ENGINE.md): Complete physics equations, sequence diagrams, and C++ architecture for Omnidirectional Ballistic Dives, Localized Time-Dilation Bubbles, Acoustic Hologram Decoys, and Mid-Air Quantum Phase-Swapping.
- [**Architecture & Mechanics Guide (`Docs/ARCHITECTURE_AND_MECHANICS_GUIDE.md`)**](Docs/ARCHITECTURE_AND_MECHANICS_GUIDE.md): Mathematical formulations for 2nd-order harmonic spring aim punch, 21-bone skeletal dimensions, anatomical damage multipliers, and 128-Hz replication.
- [**Player Locomotion & Stances Plan (`Docs/PLAYER_LOCOMOTION_AND_STANCES_PLAN.md`)**](Docs/PLAYER_LOCOMOTION_AND_STANCES_PLAN.md): Blend space architecture (`BS_BaseMale_Locomotion`), Two-Bone IK rigging, and multi-stance transitions.
- [**Getting Started & Dedicated Server Guide (`Docs/GETTING_STARTED.md`)**](Docs/GETTING_STARTED.md): Complete instructions for standalone client execution, customization, combat controls, and headless VPS server hosting.

---

## In-Game Showcase // 3rd-Person Tactical Gameplay

![Noble Fermi - Tactical 3rd-Person Over-the-Shoulder Gameplay on Rust Tactical](Docs/images/gameplay_3rd_person_rust_tactical.png)

*Live in-engine capture of the 21-bone tactical operator (`SK_BaseMale`) holding the 3D `SM_AK47` rifle on Rust Tactical, featuring the pure Slate C++ AR visor HUD (36-segment circular radar minimap, cosine-faded compass tape, 4-plate Level IV ceramic armor vitals, live ECG waveform, center-screen directional threat radar, and 128-tick Fermi Chrono-Field telemetry).*

---

## License

All rights reserved © 2026 Noble Fermi Studio.
