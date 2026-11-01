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
| **Animation Systems** | 🟡 Hybrid Procedural | Raw anim assets exist for Walk, Run, Sprint, Slide, Crouch, ADS. 21-bone procedural system drives leaning, recoil, sway. | Character mesh has no arms reload or holster/switch animations. Arms remain statically locked in ready/ADS pose while weapon animates procedurally. |
| **Weapons & Gunsmith** | 🟡 Working with gaps | 11 weapon data assets, procedural weapon construction, procedural slide/recoil, procedural magazine reload translation, projectile penetration. | No direct keybinds for `1`, `2`, `3` or Mouse Wheel weapon cycling in `AFPSCharacter`. Switching only accessible via weapon wheel (`Tab`/`4`). Reload lacks montage notify sync. |
| **Health, Armor & Damage** | 🟢 Functional | 4-plate Level IV ceramic armor + biological health in `UFPSHealthComponent`. Headshot multipliers, plate damage reduction, bone impulse on hit, ragdoll on death. | Lyra `UFPSCombatAttributeSet` exists as a duplicate GAS attribute layer; legacy `UFPSHealthComponent` is the active driver for gameplay. |
| **Stamina & Tactical Energy** | 🟢 Functional | Sprint drain, ballistic dive cost, regeneration delay, exhaustion lockout in `UFPSCharacterMovementComponent`. | Near-miss stamina refund and dive recovery timing need tuning to prevent accidental exhaustion locks during rapid maneuvers. |
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

1. **Pass 1: Locomotion & Camera Stabilization**
   - Clean up camera hierarchy: ensure unified camera control without fighting between SpringArm and Lyra camera component.
   - Smooth Ballistic Dive blend to sprint/crouch/stand.
2. **Pass 2: Weapon Switching, Direct Binds & Reload State Machine**
   - Add direct key bindings: `1` (Primary), `2` (Secondary), `3` (Melee), `MouseWheelUp` / `MouseWheelDown` (Cycle).
   - Implement procedural arm animation offsets in `UFPSAnimInstance` during reload to accompany procedural magazine movement.
   - Add fail-safe cancel/cleanup in `AFPSWeaponBase` so weapon switching during reload properly aborts and resets magazine position.
3. **Pass 3: Vitals, Armor & Stamina Hardening**
   - Verify zero-stamina recovery never gets trapped.
   - Ensure clean death ragdoll without collision glitches or camera drops below floor.
4. **Pass 4: Base Game Loop & Build Packaging Verification**
   - Test seamless Menu -> Deploy -> Combat -> Death/Respawn -> Return to Menu cycle.
   - Ensure packaged binaries launch cleanly with all required DLLs.
