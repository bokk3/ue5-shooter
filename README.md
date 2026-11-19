<p align="center">
  <img src="Docs/images/noble_fermi_logo.png" alt="NOBLE FERMI wordmark" width="760" />
</p>

# NOBLE FERMI

[![Release](https://img.shields.io/badge/Release-v0.4.2--alpha-FFEA00.svg?style=flat-square&logo=github)](https://github.com/bokk3/ue5-shooter/releases/tag/v0.4.2-alpha)
[![Engine](https://img.shields.io/badge/Unreal%20Engine-5.8-00E5FF.svg?style=flat-square&logo=unrealengine)](https://www.unrealengine.com/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B20-FFB300.svg?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![Status](https://img.shields.io/badge/Status-Pre--production%20prototype-FF2A54.svg?style=flat-square)]()

> **Noble Fermi** is a **competitive PvP third-person shooter** in early prototype, built in Unreal Engine 5.8 C++. Its hook is the *Fermi* kit:
> - a **ballistic dive**
> - a **local time-dilation bubble** that turns incoming hitscan rounds into slow, dodgeable slugs
> - an **acoustic hologram decoy** that you can **phase-swap** with
>
> The goal is a AA-quality over-the-shoulder PvP shooter. The [Status](#status) section below says honestly how far off that is.

---

## Status

*Last audited: 2026-11-19, branch `feat/pvp-foundation`.*

| Area | Current state | Target |
|---|---|---|
| **Networking** | Effectively single-player. Firing, hit traces and damage run on whichever machine fires. Only cosmetic multicasts exist. The "128-tick authoritative" label from earlier versions was never implemented. | Server-authoritative fire with client prediction and server-side rewind (lag compensation); predicted movement and abilities; 30–60 Hz dedicated servers |
| **Levels** | No authored gameplay maps. `MenuMap` is the only `.umap`. Arenas are built at runtime from scaled engine cubes and cylinders (`Map/FPSCombatArenaMapActor.cpp`, `Map/FPSCoastalOperationsMapActor.cpp`). | Hand-authored maps: grey-box first, then art pass. PCG for scatter. Peek angles designed for third-person PvP |
| **Animation** | Clips are keyframed by Blender scripts. In game, a custom `FAnimInstanceProxy::Evaluate` poses bones by hand in C++ (`Character/FPSAnimInstance.cpp`). There's no AnimGraph state machine, montages or motion matching. | Game Animation Sample motion matching, Control Rig hand IK and aim offsets, montages for reload and abilities |
| **Characters and gear** | Blender base mesh. Belt gear is generated with `ProceduralMeshComponent`. | Authored operator meshes (Fab or MetaHuman base), shared skeleton, hitboxes that are the same for every cosmetic |
| **Bots** | `AFPSCharacter` with a `bIsTacticalBot` flag, walking between hard-coded waypoints (`TickTacticalBotAI`). No AIController, navigation mesh or perception. | Practice and onboarding bots on StateTree, AI Perception and the navigation mesh |
| **Abilities** | Dive, time bubble and decoy are hand-rolled in `AFPSCharacter`. GAS and a "Lyra" folder are present but mostly unused. | GAS abilities with prediction |
| **Input** | Old `BindAxis`/`BindAction` system (Enhanced Input is enabled but not used). | Enhanced Input mapping contexts |
| **UI** | Hand-written Slate. `UI/MainMenuWidget.cpp` is about 11.5k lines. | CommonUI/UMG, so designers can edit it |
| **Economy / auth / bounties** | Local JSON profile, Cloudflare-backed auth prototype. | Parked until the core PvP loop is fun |

### What's worth keeping
- The Fermi mechanics: time bubble, dilated slugs, decoy and phase-swap. They're distinctive, but each needs a networked prototype before more content is built on it.
- Weapon data assets and the 11-weapon roster.
- The HUD's visual language.

---

## Roadmap (PvP)

1. **Netcode foundation:** server-authoritative fire, damage and death, with a 2-client test under simulated lag.
2. **Foundation swap:** motion-matched locomotion, Enhanced Input, GAS abilities, one equipment system instead of two.
3. **One grey-box competitive map**, with server-side visibility checks against third-person corner peeking.
4. **The time bubble networked and playtested** at realistic ping.
5. **Art pass:** authored environments, operators, Niagara VFX, MetaSounds, UI in CommonUI.
6. **Online:** dedicated server fleet, EOS matchmaking, Easy Anti-Cheat, replays and kill cam.

---

## Building and running

The engine path is currently hard-coded to `D:\epuc\UE_5.8` in the `.bat` files.

```powershell
.\build_temp.bat        # compile the NobleFermi game target (Win64 Development)
.\LaunchEditor.bat      # open the editor
.\Launch_Menu.bat       # run the packaged build if present, otherwise the dev binary or editor -game
```

To test networking in the editor, set **Net Mode = Play As Client** with **2 players**, and enable network emulation (Editor Preferences, Level Editor, Play, Multiplayer Options).

## Controls

| Input | Action |
|---|---|
| `WASD` + `Shift` | Move / sprint |
| `Left Alt` (or sprint + `Space`) | Ballistic dive (spawns time bubble) |
| `Q` / `Q` again | Deploy decoy / phase-swap with it |
| `C` / `Ctrl` | Crouch / slide |
| `LMB` / `RMB` | Fire / aim down sights |
| `MMB` while aiming | Toggle first-person optic |
| `V` | Swap shoulder |
| `B` | Cycle fire mode |
| `G` / `4` | Frag / flashbang |
| `1` `2` `3`, mouse wheel | Weapon slots |
| `Esc` | Escape menu |

## Repository layout

```
Config/            Engine, game and input .ini
Content/           Imported meshes, textures, audio. Only one map (MenuMap)
Docs/              Design docs. Docs/Screenshots = real captures, Docs/ConceptArt = AI-generated targets
Scripts/           Blender / editor Python pipeline and one-off debug scripts
Source/NobleFermi/ Game module
Source/Launcher/   Win32 splash/bootloader exe
web/               Marketing web portal
```

## Screenshots

All images below are real in-engine captures. AI-generated **concept art** showing the visual target lives in [`Docs/ConceptArt`](Docs/ConceptArt) and is never mixed in here.

**Command Center menu with hangar diorama**
![Command Center](Docs/Screenshots/menu_command_center.png)

**Favela arena, over-the-shoulder view**
![Favela arena](Docs/Screenshots/gameplay_rust_tactical.png)

**Operator customization**
![Operator customization](Docs/Screenshots/operator_customization_armory.png)

---

## License

All rights reserved © 2026 Noble Fermi Studio.
