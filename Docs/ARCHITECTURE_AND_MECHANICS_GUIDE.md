# Noble Fermi: Architecture & Mechanics Guide

## 1. Executive Summary

**Noble Fermi** is a standalone, competitive tactical first-person shooter built on **Unreal Engine 5.8** using high-performance, modular **C++20**. 

The design goal is to recreate the golden-era tactical multiplayer experience reminiscent of classic *Call of Duty: Modern Warfare 2* (snappy gunplay, high lethality, fast ADS, authoritative hit registration, iconic combat archetypes), backed by **128-tick dedicated server networking**, an **anatomically accurate human male skeletal mechanics system** (180 cm / 82 kg anthropometry), and a **hardware-accelerated Slate military start menu with real-time 3D character customization**.

---

## 2. Core Architecture & System Decomposition

```
NobleFermi (Source/NobleFermi/)
├── Character/
│   ├── FPSCharacter.h/.cpp              # Authoritative player character pawn (locomotion, ADS, health, camera)
│   └── FPSCharacterPreviewActor.h/.cpp  # 3D turntable operator model (ADS stance, studio lighting, customization)
├── BodyMechanics/
│   ├── FPSHumanBones.h/.cpp             # 21-bone adult human male hierarchy & anthropometric registry
│   └── FPSBodyMechanicsComponent.h/.cpp # 2nd-order spring aim punch, flinch, IK, and limb trauma tracking
├── Combat/
│   ├── FPSHealthComponent.h/.cpp        # Ballistic damage absorption, Level IV plates, ragdoll triggers
│   ├── FPSCombatInterface.h             # Common damage dispatch and impact event contracts
│   └── FPSWeaponBase.h/.cpp             # Modular ballistics, recoil kick, procedural sway, fire modes
├── Game/
│   ├── FPSGameMode.h/.cpp               # Match lifecycle, 128-tick configuration, team management
│   ├── FPSMenuGameMode.h/.cpp           # Menu scene coordinator (spawns 3D preview actor, sets view target)
│   ├── FPSMenuPlayerController.h/.cpp   # Input routing, mouse unlocking, smooth camera blend
│   └── FPSMenuDioramaActor.h/.cpp       # 3D scene lighting, weapon inspection pedestals
└── UI/
    ├── MainMenuWidget.h/.cpp            # Slate C++ full-screen military tactical HUD & customization deck
    ├── FPSHUDWidget.h/.cpp              # In-game tactical HUD (crosshair, ammo, health, hitmarkers)
    ├── FPSPauseMenuWidget.h/.cpp        # In-game pause & disconnect overlay
    └── SettingsMenuWidget.h/.cpp        # FOV, mouse raw input sensitivity, audio mixing
```

---

## 3. Human Male Skeletal Anatomy & Body Mechanics

Rather than treating the character as a generic capsule collider, Noble Fermi uses an anatomically accurate model based on the standard **adult human male** (standing height 180 cm, total body mass 82.0 kg).

### 3.1. 21-Segment Skeletal Bone Tree
Defined in [`FPSHumanBones.h`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/BodyMechanics/FPSHumanBones.h):

| Bone Name | Anatomical Region | Height (Z) | Mass (kg) | Hit Multiplier | Armor Protection |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `root` | Ground Datum | 0.0 cm | 0.0 kg | 1.0x | None |
| `pelvis` | Sacrum / Center of Gravity | 96.0 cm | 14.5 kg | 1.0x | Unprotected |
| `spine_01` | Lumbar Spine (L1–L5) | 110.0 cm | 8.0 kg | 1.0x | Unprotected |
| `spine_02` | Mid-Thoracic (Lower Ribs) | 124.0 cm | 12.0 kg | 1.15x | **Level IV Ceramic** |
| `spine_03` | Upper Thoracic (Chest/Vitals) | 138.0 cm | 14.0 kg | 1.25x | **Level IV Ceramic** |
| `neck_01` | Cervical Spine (C1–C7) | 148.0 cm | 1.8 kg | 2.0x | Unprotected |
| `head` | Cranium & Facial Skeleton | 164.0 cm | 4.8 kg | 2.5x | Helmet / Critical |
| `clavicle_l/r` | Collarbone & Acromion | 142.0 cm | 1.5 kg ea | 0.8x | Unprotected |
| `upperarm_l/r`| Humerus | 136.0 cm | 2.5 kg ea | 0.8x | Unprotected |
| `lowerarm_l/r`| Radius & Ulna | 137.0 cm | 1.8 kg ea | 0.75x | Unprotected |
| `hand_l/r` | Carpals & Metacarpals | 138.0 cm | 0.6 kg ea | 0.75x | Unprotected |
| `thigh_l/r` | Femur & Hamstring | 74.0 cm | 9.0 kg ea | 0.85x | Unprotected |
| `calf_l/r` | Tibia & Fibula | 32.0 cm | 4.2 kg ea | 0.80x | Unprotected |
| `foot_l/r` | Tarsals & Ankle | 5.0 cm | 1.2 kg ea | 0.75x | Unprotected |

### 3.2. Second-Order Spring-Mass-Damper Aim Punch
When taking fire, incoming kinetic energy displaces the player's aim reticle. The component computes this using a damped harmonic oscillator:

$$\mathbf{F}_{spring} = -k \cdot \mathbf{\theta}$$
$$\mathbf{F}_{damping} = -c \cdot \mathbf{\omega}$$
$$\mathbf{\alpha} = \frac{\mathbf{F}_{spring} + \mathbf{F}_{damping} + \mathbf{\tau}_{impact}}{I}$$

This yields a realistic, sharp flinch followed by an organic, springy recovery rather than a flat, synthetic camera snap.

### 3.3. Level IV Ceramic Body Armor
- Hard ballistic plates shelter `spine_02` (mid-torso) and `spine_03` (vital upper chest).
- Incoming ballistic damage in these zones is attenuated by the plate carrier before penetrating flesh.
- Once plate integrity is compromised by sustained fire, full ballistic damage transfers to the thoracic cavity.

### 3.4. Limb Trauma & Locomotion Inhibition
- **Arm Trauma**: High-velocity impacts to `upperarm` or `lowerarm` induce neuromuscular tremors, significantly increasing weapon sway amplitude.
- **Leg Trauma**: Impacts to `thigh` or `calf` cause acute motor inhibition, automatically canceling and preventing tactical sprint until healed.

---

## 4. Multiplayer & Dedicated Server Architecture

### 4.1. 128-Tick Authoritative Networking
- High-fidelity physics simulation ticking at **128 Hz** (7.8125 ms tick budget).
- Server-authoritative movement with client-side prediction and error reconciliation.
- Lag compensation and rewind buffers for sub-millisecond bullet registration.

### 4.2. Dual Hosting Model
1. **Dedicated VPS Server**:
   - Runs headless via `-server -nullrhi -log -PORT=7777 -TickRate=128`.
   - Lightweight footprint capable of running on affordable Linux (via Wine/Proton or native cross-compile) or Windows VPS hosts.
   - Clients connect directly via IP/Port in the in-game Server Browser or via direct connect.
2. **Client Listen Room**:
   - Any player can host a listen server directly from the game client (`?listen`).
   - Friends and LAN peers can join immediately without port-forwarding dedicated servers.

---

## 5. Modern Warfare 2-Inspired Gunplay Mechanics

### 5.1. Five Iconic Combat Archetypes
1. **Class 1: Assault (M4A1)**: Versatile all-rounder. Red-dot optic, compensator, 40-round magazine. ADS speed 210 ms.
2. **Class 2: Spec Ops Stealth (MP5SD)**: Subsonic 9mm, integrated suppressor, zero radar ping. ADS speed 150 ms.
3. **Class 3: Heavy Gunner (L86 LSW)**: 100-round continuous drum suppression, fluted heavy barrel. ADS speed 380 ms.
4. **Class 4: Recon Sniper (Intervention .50)**: High-caliber one-shot lethal to upper chest, neck, and head. ADS speed 320 ms, bullet speed 1150 m/s.
5. **Class 5: Run & Gun Rusher (Vector .45 ACP)**: 1200 RPM extreme fire rate, 120 ms ADS, 100 ms sprint-out time.

### 5.2. Procedural Ballistics & Weapon Sway
- Deterministic recoil climb curves with horizontal bias.
- Breathing sway simulated via sine oscillation during ADS.
- Weapon mount stabilization when approaching walls or cover.

---

## 6. Slate UI & 3D Character Customisation

The user interface is built entirely in **Slate C++**, executing natively on the render thread with zero webview overhead or bloated UMG assets.

### 6.1. 3D Rotating Preview Actor (`AFPSCharacterPreviewActor`)
- Placed in the world origin and rendered directly through the game viewport.
- **Turntable**: Rotates 360° continuously around Yaw (12°/s, 24°/s, 45°/s, or paused).
- **Aiming Posture**: Displays the adult human male operator aiming down sights (ADS) with procedural breathing motion.
- **3-Point Studio Lighting**: Warm key spotlight (14,000 lm), cyan-blue rim light (9,000 lm), and ambient fill light (4,500 lm) embedded into the actor, guaranteeing studio illumination in any environment.

### 6.2. Customization Controls
- **Camouflage Patterns**: Dynamic runtime material updates across uniform and gear:
  - *Urban Digital* (Metro Slate Gray / Charcoal)
  - *Woodland Tactical* (Forest Olive Drab / Ranger Khaki)
  - *Desert Spec Ops* (Coyote Tan / Flat Dark Earth)
  - *Stealth Blackout* (Midnight Matte Black / Carbon Kevlar)
- **Tactical Headgear**: Fast Ballistic Helmet, GPNVG-18 Quad-Nods (with phosphor green glow), Spec Ops Balaclava, Comm Cap & Headset.
- **Armor Vests**: Level IV Heavy Carrier, Lightweight JPC Vest, Recon Tactical Rig.
- **Stance Toggle**: Seamless switch between High-Ready ADS and Low-Ready Patrol.

---

## 7. Competitive Combat Map: "Rust Tactical" (60m x 60m)

Noble Fermi features **Rust Tactical**, an intense, compact 60m x 60m (6000 x 6000 Unreal Units) competitive combat arena engineered in C++ (`AFPSCombatArenaMapActor`). Designed to capture the verticality, sightline control, and lethal pacing of classic MW2 arenas, the map requires zero manual editor placement and instantiates seamlessly in standalone clients and dedicated servers alike.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        RUST TACTICAL (60m x 60m)                       │
│                                                                        │
│   [OpFor Spawns - North Docks]         (Flag C: Bunker Trench)         │
│   ┌──────────────────────────────────────────────────────────────┐     │
│   │  [Container Yard]        [Oil Derrick Rig]     [Pipeline]    │     │
│   │  Blue / Red / Olive      12m Tower Catwalks    Elevated 30m  │     │
│   │  Shoot-thru Tunnel       Ramps & Sniper Nest   Concrete      │     │
│   │  (Flag A)                (Flag B)              Bunker        │     │
│   │                                                              │     │
│   │         [Armored Wreck]       [Jersey Barriers]              │     │
│   │         Center Chokepoints    Hard Cover Flanks              │     │
│   └──────────────────────────────────────────────────────────────┘     │
│   [Task Force 141 Spawns - South Revetments]                           │
└────────────────────────────────────────────────────────────────────────┘
```

### 7.1. Three Tactical Lanes & Combat Zones
1. **West Flank (Container Yard & Shoot-Through Tunnel)**:
   - 4 industrial shipping containers (Blue, Red, Olive, and Sand) arranged in a tight CQB labyrinth.
   - Central open-ended shoot-through tunnel allowing flanking runs while exposing players to ricochets and penetration fire.
   - Houses **Domination Flag A**.
2. **Central Lane (Oil Derrick Rig & Vehicle Wreck Chokepoint)**:
   - 12-meter tall industrial derrick structure with structural steel corner pillars, diamond-plate catwalks, dual 25-degree access ramps, and an apex sniper crow's nest.
   - Armored tactical vehicle wreck and 8 reinforced concrete Jersey barriers provide ground-level cover against cross-map sightlines.
   - Houses **Domination Flag B** at the tower base.
3. **East Flank (Pipeline Trench & Reinforced Bunker)**:
   - 30-meter elevated heavy industrial oil pipeline with concrete support stanchions.
   - Semi-submerged reinforced concrete bunker with narrow firing slits overlooking the central lane.
   - Houses **Domination Flag C**.

### 7.2. Anti-Spawn-Trap Tactical Player Starts
The arena dynamically provisions 8 dedicated `APlayerStart` actors during `InitGame()`:
- **4 Team Alpha Spawns (Task Force 141)**: Located in the South perimeter safe revetments at `X=-2400.0`, with protective barrier line-of-sight blocks preventing immediate sniper picks.
- **4 Team Bravo Spawns (OpFor)**: Located in the North perimeter loading docks at `X=+2400.0`, oriented 180 degrees south toward center lane cover.

### 7.3. Pure Real-Time Dynamic Lighting (Lumen) & Exposure Control
- **Global Static Light Baking Disabled**: Permanently disabled legacy lightmass baking in `Config/DefaultEngine.ini` (`r.AllowStaticLighting=False`, `bAllowStaticLighting=False`), eliminating all static lightmap caches and the "Lighting needs to be rebuilt" screen warning.
- **Directional Sunlight (Lumen)**: Calibrated 10.0 lux warm desert sun set at a 38-degree tactical golden-hour angle with real-time Virtual Shadow Maps (`EComponentMobility::Movable`).
- **Real-Time Skylight & Atmosphere**: Dynamically captures sky illumination with real-time capture (`bRealTimeCapture=true`) and warm exponential height dust haze (density `0.0015`).
- **Manual Exposure Clamping**: Fully unbound `UPostProcessComponent` enforces manual camera exposure (`AutoExposureMethod = AEM_Manual`, fixed bias `0.0`, min/max brightness `1.0`), preventing eye-adaptation auto-exposure from blowing out the screen to white.
- **Aviation Hazard Strobe**: Red beacon atop the derrick tower dynamically pulsing via sine oscillation (`Intensity 30.0`), providing visual landmark orientation.

---

## 8. Procedural 3D Weaponry, ADS Mechanics & 4-Phase Reload Timeline

Noble Fermi features a fully procedural first-person weapon pipeline (`AFPSWeaponBase`), delivering tactile, physical gunplay without requiring static pre-baked skeletal animation assets.

### 8.1. Mathematical Aim-Down-Sights (ADS) Alignment

When the operator transitions from hipfire to ADS, the weapon mesh interpolates between two reference frames relative to the camera root (`FirstPersonCamera`):

$$\mathbf{T}_{weapon}(t) = \text{Lerp}(\mathbf{T}_{hip}, \mathbf{T}_{ads}, \alpha_{ADS}(t)) + \mathbf{T}_{recoil}(t) + \mathbf{T}_{reload}(t) + \mathbf{T}_{sway}(t)$$

Where:
- **Hipfire Relative Offset**: $\mathbf{P}_{hip} = (26.0\text{ cm}, 14.0\text{ cm}, -13.0\text{ cm})$, $\mathbf{R}_{hip} = (0.5^\circ, -1.0^\circ, -1.5^\circ)$.
- **ADS Center Offset**: $\mathbf{P}_{ads} = (20.0\text{ cm}, 0.0\text{ cm}, -6.8\text{ cm})$, $\mathbf{R}_{ads} = (0.0^\circ, 0.0^\circ, 0.0^\circ)$.

```
   FirstPersonCamera Eye Datum (0, 0, 0)
        │
        │─── X = 20.0 cm forward ───► [ Reflex Optic Hood ]
        │                                  │
        ▼ Z = -6.8 cm down                 ▼ Z_optic = +6.8 cm up
   [ Weapon Receiver Root ] ──────────► [ Glowing Reticle Dot (0.0, 0.0, 0.0) ]
```

Because the reflex optic reticle sits at $Z = +6.8\text{ cm}$ relative to the weapon root:
$$Z_{camera} = -6.8\text{ cm} + 6.8\text{ cm} = 0.0\text{ cm} \quad (\text{Exact Camera Eye Line})$$
$$Y_{camera} = 0.0\text{ cm} + 0.0\text{ cm} = 0.0\text{ cm} \quad (\text{Exact Screen Center})$$

- **Dynamic Field of View**:
  $$\text{FOV}(t) = \text{InterpTo}(\text{FOV}, \text{TargetFOV}, \Delta t, 16.0\text{ s}^{-1})$$
  Interpolates from $95.0^\circ$ (hipfire peripheral awareness) to $68.0^\circ$ (focused tactical magnification) in $\sim 180\text{ ms}$.
- **Sensitivity Modulation**: Rotational inputs (`Turn`, `LookUp`) are scaled by $0.65\times$ during ADS for pinpoint micro-adjustments.
- **HUD Reticle Collapse**: The 2D HUD crosshair collapses to radius $0.0$ when $\alpha_{ADS} \to 1.0$, directing player focus entirely through the physical 3D reflex optic.

### 8.2. Modular 3D Tactical Carbine Geometry
The NF-98 Carbine is constructed procedurally from 11 specialized components:
1. **Receiver (Upper & Lower)**: Matte charcoal gunmetal chassis ($22\times 3.8\times 5.6\text{ cm}$).
2. **Picatinny Top Rail**: Full-length optic & accessory rail ($36\times 2.4\times 0.8\text{ cm}$).
3. **M-LOK Handguard**: Textured foregrip enclosure ($20\times 4.2\times 4.6\text{ cm}$).
4. **Barrel & Muzzle Brake**: Blued steel rifled barrel ($38\times 1.6\text{ cm}$) with birdcage compensator ($4\times 2.4\text{ cm}$).
5. **Ergonomic Grip & Stock**: Swept polymer pistol grip ($-18^\circ$ rake), buffer tube, and contoured buttstock.
6. **Magazine Subsystem**: Independent `MagazineRoot` scene component housing a curved 30-round STANAG/PMAG magazine ($16\times 6.5\times 2.5\text{ cm}$).
7. **Reflex Optic & Night Sights**:
   - Elevated sight base and protective reflex frame hood.
   - High-luminance emissive red dot reticle ($15.0\text{ cd/m}^2$ bloom radiance).
   - Co-witnessed front iron sight post with glowing green tritium night pip.

### 8.3. 4-Phase Procedural Reload Timeline

When reloading (tactical: $T = 2.2\text{ s}$, empty: $T = 2.9\text{ s}$), the weapon executes a deterministic 4-phase sequence parameterized by normalized progress $\alpha = t / T$:

```
0.0           0.20                 0.42                          0.76         1.0
├──────────────┼────────────────────┼─────────────────────────────┼────────────┤
│ Phase 1      │ Phase 2            │ Phase 3                     │ Phase 4    │
│ Cant & Dip   │ Mag Ejection       │ Fresh Insert & Slap         │ Bolt Rack  │
│ (Roll 22°)   │ (Drop -28cm)       │ (Slide Up + Palm Shake)     │ & Return   │
```

- **Phase 1: Weapon Cant & Presentation ($\alpha \in [0.0, 0.20]$)**:
  Carbine rolls $22^\circ$ outward and pitches up $6^\circ$, canting the magwell directly into the operator's central field of view:
  $$\mathbf{R}_{cant}(\alpha) = (6.0^\circ, -4.0^\circ, 22.0^\circ) \cdot \text{SmoothStep}(0.0, 0.20, \alpha)$$
- **Phase 2: Mag Ejection ($\alpha \in [0.20, 0.42]$)**:
  `MagazineRoot` translates downward out of the receiver:
  $$\Delta z_{mag} = -28.0\text{ cm} \cdot \left(\frac{\alpha - 0.20}{0.22}\right), \quad \Delta\theta_{pitch} = 18.0^\circ \cdot \left(\frac{\alpha - 0.20}{0.22}\right)$$
- **Phase 3: Fresh Mag Insert & Palm Slap ($\alpha \in [0.42, 0.76]$)**:
  Fresh magazine slides upward from $-24.0\text{ cm}$ into the magwell ($0.42 \le \alpha < 0.70$). At $\alpha \in [0.70, 0.76]$, palm contact generates a sharp mechanical slap bump impulse:
  $$\Delta z_{bump} = 0.6\text{ cm} \cdot \sin\left(\pi \frac{\alpha - 0.70}{0.06}\right)$$
  Transmitting a reactive kickback impulse to the operator's hands.
- **Phase 4: Bolt Release & Ready ($\alpha \in [0.76, 1.00]$)**:
  On empty reloads, the bolt catch snaps forward ($\Delta x = -1.2\text{ cm}$ mechanical kick). The carbine smoothly returns from the canted pose back to active ready.

### 8.4. Physical Recoil Impulse & Mouse Look Inertia Sway
- **Shot Kickback**: Each round fired adds an impulse vector $\Delta\mathbf{P}_{recoil} = (-2.0, \pm 0.15, 0.5)\text{ cm}$ and $\Delta\mathbf{R}_{recoil} = (2.3^\circ, \pm 0.6^\circ, \pm 0.8^\circ)$ to the weapon mesh, dampening by $40\%$ during ADS.
- **Mouse Sway Lag**: Tracks the delta angular velocity of the player controller. In hipfire, the weapon drags naturally behind turning motions; in ADS, sway lag is attenuated by $85\%$ to ensure absolute reticle discipline.

---

## 9. Why This Foundation Is an Exceptional Start

1. **Architecture Before Assets**: Most failed indie shooters import gigabytes of marketplace 3D meshes before having solid networking or ballistics. When multiplayer is retrofitted, the project collapses. Noble Fermi started with 128-tick networking, dedicated server targets, and modular C++ physics first.
2. **Procedural Level Generation**: Instead of being locked into proprietary binary map files that can only be built inside the editor GUI, Noble Fermi's competitive maps are fully specified in procedural C++. They boot instantaneously, test deterministically in headless CI/CD, and package without missing reference headaches.
3. **Anatomical Authenticity**: By grounding hitboxes and damage models in real adult human male anthropometry (180 cm, 82 kg, 21 bones), combat feels visceral and predictable. Shots to the chest plate behave differently from shots to the lower abdomen or femoral artery.
4. **Instant Standalone Execution**: The game compiles into a lean, self-contained standalone executable (`Packaged\Windows\NobleFermi.exe`) that boots in seconds without requiring the Unreal Editor.
5. **Clean Codebase**: Written in modular C++ adhering to modern Unreal Engine standards, with complete separation between UI, combat simulation, body mechanics, and replication.
