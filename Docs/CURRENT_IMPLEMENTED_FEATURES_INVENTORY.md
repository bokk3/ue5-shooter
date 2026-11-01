# Noble Fermi - Current Implemented Features Inventory
**Document Date:** November 1, 2026  
**Scope:** Exhaustive audit of all gameplay, animation, weapon, vital, economy, and framework systems currently present in the C++ and content codebase.  
**Directive:** Feature freeze in effect. No new systems or features until existing systems operate cleanly and reliably in the base game framework.

---

## 1. Executive Summary & Audit Matrix

| System Domain | Implementation Status | Functional Reality | Blockers / Known Issues |
| :--- | :--- | :--- | :--- |
| **Executable & Launching** | ⚠️ Resolved (Path Fix) | Launchers (`Launch_Menu.bat`, `Play_NobleFermi.bat`) work. Direct run had `tbbmalloc.dll` missing. | Fixed: Copied `tbbmalloc.dll` / `tbb12.dll` to all staging/root folders and added UE Win64 binaries to user `PATH`. |
| **Locomotion & Stances** | 🟢 Verified & Stabilized | Walk, Run, Sprint, Slide, Ballistic Dive, Jump in `UFPSCharacterMovementComponent`. | Pass 1 Complete: `FollowCamera` deconflicted from `LyraCameraComponent`. 0.35s S-curve dive recovery blend and slide-to-jump momentum conservation integrated. |
| **Animation Systems** | 🟢 Verified & Stabilized | Raw anim assets exist for Walk, Run, Sprint, Slide, Crouch, ADS. 21-bone procedural system drives leaning, recoil, sway. | Pass 2 Complete: Integrated procedural left-hand reload IK solver in `UFPSAnimInstance` tracking multi-phase magazine release, chest-rig retrieval, and palm slap. |
| **Weapons & Gunsmith** | 🟢 Verified & Stabilized | 11 weapon data assets, procedural weapon construction, procedural slide/recoil, procedural magazine reload translation, projectile penetration. | Pass 2 Complete: Direct input binds `1`, `2`, `3` and `MouseWheelUp`/`Down`. `UFPSInventoryComponent` manages 3 slots (Primary, Secondary, Sidearm). Fail-safe `AbortReload()` prevents desyncs. |
| **Health, Armor & Damage** | 🟢 Verified & Stabilized | 4-plate Level IV ceramic armor + biological health in `UFPSHealthComponent`. Headshots bypass armor plates directly to vitals with bone multipliers. Chest plate reduces bullet kinetic damage by 30%. Death cleanly decouples camera boom and tracks pelvis ragdoll with 12cm collision probe. | Pass 3 Complete: Reliable damage calculation, headshot armor penetration, and smooth ragdoll death tracking. |
| **Stamina & Tactical Energy** | 🟢 Verified & Stabilized | Sprint drain, ballistic dive cost, regeneration delay, exhaustion lockout in `UFPSCharacterMovementComponent`. Auto-resumes sprinting on recovery if sprint key remains held. | Pass 3 Complete: `SetWantsToSprint` preserves player intent, recovering from exhaustion back into sprint at >=25 stamina without key re-press. `AddStamina` restores sprinting dynamically. |
| **Economy, Profile & Auth** | 🟢 Functional | Offline local JSON fallback (`Saved/NobleFermiProfile.json`, `Saved/NobleFermiLocalAccounts.json`) + online API stub. Camo/armor/weapon loadouts serialize properly. | Menu load time in Hangar is slow due to synchronous package checks and texture streaming. |
| **HUD & Slate Overlay** | 🟢 Working | High-fidelity canvas HUD (`UFPSHUDWidget`) with dynamic ammo, 4-plate ceramic armor, ECG vitals, compass, hitmarkers, crosshair sway, and killfeed. | High framerate cost if dirty checks aren't batched; requires caching for Slate drawing calls. |

---

## 2. Deep Dive: Locomotion & Movement Mechanics

### Implemented Files
- [`Source/NobleFermi/Character/FPSCharacterMovementComponent.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Character/FPSCharacterMovementComponent.h)
- [`Source/NobleFermi/Character/FPSCharacterMovementComponent.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Character/FPSCharacterMovementComponent.cpp)
- [`Source/NobleFermi/Character/FPSCharacter.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Character/FPSCharacter.h)
- [`Source/NobleFermi/Character/FPSCharacter.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Character/FPSCharacter.cpp)

### State
1. **Movement Speeds**:
   - Walk: `260.0 cm/s`
   - Run / Combat Jog: `460.0 cm/s`
   - Tactical Sprint: `720.0 cm/s` (drains stamina at `18.0/s`)
   - Crouch: `190.0 cm/s`
   - Slide: `820.0 cm/s` decaying over `1.15s`
2. **Ballistic Dive**:
   - Trigger: `LeftAlt` or `Dive` action when moving forward.
   - Impulse: forward vector impulse + vertical pitch.
   - Stamina Cost: `35.0` (requires >= `25.0` to trigger).
3. **Identified Issues**:
   - `AFPSCharacter` creates both `FollowCamera` (with `CameraBoom`) and `LyraCameraComponent`. Depending on possession, the active camera can hitch or fight between third-person spring arm evaluation and Lyra camera mode evaluation.
   - Ballistic Dive completion leaves the character prone/crouched without a smooth procedural transition curve back to standing or combat-ready stance.

---

## 3. Deep Dive: Weapon, Reload & Switching Architecture

### Implemented Files
- [`Source/NobleFermi/Weapons/FPSWeaponBase.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Weapons/FPSWeaponBase.h)
- [`Source/NobleFermi/Weapons/FPSWeaponBase.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Weapons/FPSWeaponBase.cpp)
- [`Source/NobleFermi/Inventory/FPSInventoryComponent.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Inventory/FPSInventoryComponent.h)
- [`Source/NobleFermi/Inventory/FPSInventoryComponent.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Inventory/FPSInventoryComponent.cpp)
- [`Source/NobleFermi/Character/FPSAnimInstance.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Character/FPSAnimInstance.h)
- [`Source/NobleFermi/Character/FPSAnimInstance.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Character/FPSAnimInstance.cpp)

### State
1. **Weapon Roster**:
   - 11 Data Assets defined (`DA_W01_NF98` through `DA_W11_Stiletto`).
   - Weapon base supports modular static meshes (Receiver, Barrel, Handguard, Stock, Magazine, Optic, Muzzle, Grip).
   - Multi-phase reload: Weapon cants 22 degrees outward, magazine slides out along local -Z, audio triggers per phase (`A_Reload_Phase1` - `A_Reload_Phase4`), new magazine translates in, slide releases.
2. **Identified Critical Gaps**:
   - **No standard weapon switch input bindings**: `AFPSCharacter::SetupPlayerInputComponent` does not bind `1` for Primary, `2` for Secondary, `3` for Sidearm/Melee, or `MouseWheelUp`/`MouseWheelDown` for `CycleNextWeapon()`. The user can only open the radial wheel with `Tab` or `4`.
   - **Character Arm Reload Animation Gap**: In `UFPSAnimInstance`, there is no reload state or procedural arm offset for the operator's hands/arms. While the weapon mesh tilts and ejects its magazine procedurally, the operator character's arms remain static in their standard grip.
   - **Reload Interruption / State Lock**: If a player triggers weapon switch or dive during the reload timer, `CurrentState` can desync if `FinishReload()` is called or aborted midway without resetting `MagazineRoot` transform.

---

## 4. Deep Dive: Damage, Armor, Vitals & Stamina

### Implemented Files
- [`Source/NobleFermi/Combat/FPSHealthComponent.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Combat/FPSHealthComponent.h)
- [`Source/NobleFermi/Combat/FPSHealthComponent.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Combat/FPSHealthComponent.cpp)
- [`Source/NobleFermi/BodyMechanics/FPSHumanBones.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/BodyMechanics/FPSHumanBones.h)
- [`Source/NobleFermi/BodyMechanics/FPSBodyMechanicsComponent.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/BodyMechanics/FPSBodyMechanicsComponent.h)

### State
1. **Hit Detection & Bone Zones**:
   - Head: 2.5x - 3.0x multiplier, bypasses body armor.
   - Torso / Chest: Protected by 4 ceramic armor plates (CurrentShield). Damage to plates is mitigated by 30% (`FinalDamage *= 0.70f`). Plate breaks trigger audio/visual feedback.
   - Limbs: Reduced damage multipliers (0.6x - 0.75x).
   - Death: Ragdoll activation with bone directional impulse and weapon drop.
2. **Stamina System**:
   - Maximum Stamina: `100.0`.
   - Sprint Drain: `18.0/s`.
   - Regeneration Rate: `22.0/s` after `1.2s` idle/walk cooldown.
   - Lockout threshold: Exhaustion flag clears only when stamina regenerates back above `25.0`.
3. **Identified Issues**:
   - Lyra GAS attributes (`UFPSCombatAttributeSet`) and legacy `UFPSHealthComponent` both exist. To ensure stability, gameplay logic must strictly use one authority source (`UFPSHealthComponent`) and prevent conflicting health modifications.

---

## 5. Deep Dive: Economy, User Profile & Menu System

### Implemented Files
- [`Source/NobleFermi/Economy/FPSEconomySubsystem.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Economy/FPSEconomySubsystem.h)
- [`Source/NobleFermi/Economy/FPSEconomySubsystem.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Economy/FPSEconomySubsystem.cpp)
- [`Source/NobleFermi/Auth/FPSAuthSubsystem.h`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Auth/FPSAuthSubsystem.h)
- [`Source/NobleFermi/Auth/FPSAuthSubsystem.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Auth/FPSAuthSubsystem.cpp)
- [`Source/NobleFermi/UI/FPSStartMenuGameMode.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/UI/FPSStartMenuGameMode.cpp)

### State
1. **Profile Persistence**:
   - Stored in `Saved/NobleFermiProfile.json`.
   - Tracks credits, premium tokens, operator camo cosmetics, weapon finishes, attachment unlocks.
   - Safe local fallback when auth server is unreachable.
2. **Menu -> Game Transition**:
   - Start Menu dioramas (Hangar / Coastal Operations) load assets.
   - Transition to `CoastalOperationsMap` or `FavelaMap` via `UGameplayStatics::OpenLevel`.
3. **Identified Issues**:
   - Opening Hangar scene takes noticeable loading time due to high-poly static mesh streaming and shader compilation on first launch.

---

## 6. Actionable Stabilization Plan Summary

1. **Pass 1: Locomotion & Camera Stabilization** - [x] **Completed (Commit 8686062)**
   - Cleaned up camera hierarchy: `FollowCamera` designated as authoritative tactical camera (`SetActive(true)`), inactive Lyra `UFPSCameraComponent` and `FirstPersonCamera` deactivated with `bAutoActivate = false`.
   - Smooth S-curve Ballistic Dive recovery timer (0.35s) smoothly blending character pitch from prone (-78°) to crouch (-6°) or stand (0°).
   - Ballistic Dive respect for `bWantsToCrouch`, and Slide-Jump preserves horizontal momentum without snagging.

2. **Pass 2: Weapon Switching, Direct Binds & Reload State Machine** - [x] **Completed (Commit a162462)**
   - Added direct key bindings: `1` (Primary), `2` (Secondary), `3` (Sidearm), `MouseWheelUp` (Cycle Previous) / `MouseWheelDown` (Cycle Next).
   - `UFPSInventoryComponent` initializes 3 active weapon slots (`DA_W01_NF98`, `DA_W05_Apex9`, `DA_W06_Spectre45`).
   - Implemented procedural anatomical arm animation in `UFPSAnimInstance::SolveLeftHandReloadIK()` via `AnimationCore::SolveTwoBoneIK` (5-phase reload arc).
   - Fail-safe `AFPSWeaponBase::AbortReload()` resets magazine translation to root and returns weapon state to Idle on weapon swap or dive interrupt.

3. **Pass 3: Vitals, Armor & Stamina Hardening** - [x] **Completed (Commit a30f340)**
   - Verified zero-stamina recovery never gets trapped: `SetWantsToSprint` preserves player intent; character automatically resumes sprinting once stamina regens above 25 threshold while holding Shift.
   - Headshots cleanly bypass body armor plates directly to biological health pool.
   - Ceramic plates absorb 30% of incoming kinetic bullet energy before shield depletion.
   - Camera boom decouples from pawn controller rotation on death, inherits -28° pitch angle, and smoothly tracks pelvis ragdoll with 12cm ground probe test preventing floor clipping.

4. **Pass 4: Base Game Loop & Build Packaging Verification** - [x] **Completed (Commit 09c98c0)**
   - Fixed map navigation in `FPSEscapeMenuWidget` and `FPSPauseMenuWidget` to resolve directly to `/Game/Maps/MenuMap` instead of non-existent `MainMenuMap`.
   - Verified HUD telemetry sync (interpolated health/shield ghost bars, low-ammo tracer pips, 4-stage reload progress arc, directional threat radar).
   - Intel TBB DLL dependencies (`tbbmalloc.dll`, `tbb12.dll`) staged to engine, packaged, and project root binaries.
   - Both direct binary (`NobleFermi.exe`) and standalone launcher (`Packaged/Windows/NobleFermi.exe`) verified running stably without crash.

5. **Stabilization & Hardening Passes (Tasks 1 - 6)** - [x] **Completed**
   - **TASK-1 (Commit 0c4aa22)**: Fixed `ApplyWeaponDataAsset` scale & blockout material persistence. Reset `ReceiverMesh` & `MagazineMesh` relative transform to `(0,0,0)`, `Rotator::ZeroRotator`, and `(1,1,1)` scale. Stripped override materials, properly initialized fire mode, magazine ammo, and recoil stats.
   - **TASK-2 (Commit 84bc21f)**: Fixed 3rd-person weapon pinning during reload, cant (22°), recoil snap, and sway. Moved camera recoil spring-back above mesh attachment early-out. Decoupled rifle barrel from left-hand movement during mag release so rifle is not pulled down into operator chest rig.
   - **TASK-3 (Commit a7e85d8)**: Generated and imported native `Anim_BaseMale_Reload.uasset` and `Anim_Operator_Reload.uasset` matching `SK_BaseMale` 52-bone hierarchy with 2-bone left arm reach, mag drop, chest dip, palm slap, and return. Enabled live in-game single-node reload playback.
   - **TASK-4 (Commit 23bfb89)**: Omnidirectional dive recovery smoothing. Fixed recovery to interpolate from `DiveExitMeshRotation` and `DiveExitMeshZOffset` eliminating visual snap on strafe/backward dives. Synchronized slide/dive stop with `bIsHoldingCrouchKey` and stamina lockout.
   - **TASK-5 (Commit 23bfb89)**: Death/respawn cleanup. Implemented `EndPlay` in `UFPSInventoryComponent` to destroy spawned weapon actors; hid active weapons on death ragdoll; cleaned up `ActiveHUDWidget` on pawn destroy to prevent stacked HUDs.
   - **TASK-6**: Cleaned documentation formatting, completed full build compilation, and performed standalone engine smoke verification.
