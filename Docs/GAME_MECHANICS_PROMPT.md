Combining **Concept 1 (Max Payne / Superhot Time-Dilation Dives)** and **Concept 4 (Acoustic Hologram Decoys & Quantum Phase-Swapping)** creates an absolute monster of a gameplay loop—and it turns the title **"Noble Fermi"** from a cool code-name into the literal physics core of the game.

### The Core Mechanic: "The Fermi Paradox Engine" (Dive, Clone, Swap, Shatter Time)

Picture how this plays out in a 128-tick multiplayer firefight on **Rust Tactical**:

1. **The Acoustic Clone (Press `Q`):** You are pinned behind the Jersey barriers at Flag B. A sniper is hard-scoping the lane from the Oil Derrick catwalk. You tap `Q`: an **Acoustic Fermi Echo** (a glowing phosphor-green wireframe clone of your 21-bone operator) sprints out from the *left* side of your cover, firing simulated unsuppressed bursts that ping red threat diamonds all over the enemy sniper's `SFPSHUDWidget` minimap and compass tape.
2. **The Max Payne Dive + Time Bubble (Sprint + `C` / `Alt`):** As the sniper swings their aim left to shoot your decoy, you launch into a **3rd-Person Omnidirectional Combat Dive** out the *right* side of cover—or straight off a shipping container roof. The moment you leave your feet, a **4.5-meter Fermi Time-Dilation Field** expands around you (and around your Decoy!).
3. **The Dopamine Payoff (Press `Q` Mid-Dive):** Inside the field, time slows to **25%**. Incoming bullets turn into visible, glowing brass projectiles slicing through the air that you can physically dodge in 3rd-person mid-air. Even wilder: **you can tap `Q` while airborne in slow-motion to Quantum-Swap places with your Decoy**, conservarving your dive momentum so you materialize mid-air directly behind the enemy and light them up with pixel-perfect Over-The-Shoulder / First-Person hybrid ADS before you even hit the dirt.

---

### Architectural Blueprint (How We Build It Cleanly in C++)

To make this rock-solid without breaking your existing code, we should split this into **two sequential Agent Prompts** so your coding agent doesn't get overwhelmed:

* **Step 1 (Prompt A - Today): 3rd-Person Tactical Camera Overhaul + Max Payne Omnidirectional Dive + Localized Time-Dilation Bubble.**
* Converts `FPSCharacter` into a tactical **3rd-Person Over-The-Shoulder (OTS)** shooter with shoulder-swapping (`V` key) and hybrid smooth zoom into either tight OTS or First-Person Red-Dot ADS.
* Makes the operator's full 21-bone procedural body visible to the local player at all times so you see your own dive posture, armor plates, and weapon handling.
* Implements the **Omnidirectional Ballistic Dive** + **Fermi Time-Dilation Sphere** (`AFPSFermiTimeBubble`) that slows local actors/projectiles (`CustomTimeDilation = 0.25f` for environment/threats, `0.65f` boosted reflex speed for the diving operator).


* **Step 2 (Prompt B - Next): The "Fermi Echo" Acoustic Decoy & Mid-Air Quantum Swap.**
* Spawns the autonomous `AFPSFermiDecoyActor` that sprints forward, broadcasts fake gunfire to `UFPSRadarSubsystem`, projects its own secondary time-bubble, and executes the instant transform/momentum **Fermi Swap** + Slate HUD telemetry bar.



---

### Prompt A: Feed This to Your Agent First (3rd-Person OTS Camera + Max Payne Dive + Time-Dilation Field)

Copy and paste the block below directly into your coding agent:

```markdown
You are an expert Unreal Engine 5.8 C++ systems engineer working on **Noble Fermi**.

### Task Objective: Phase 8A — 3rd-Person Tactical OTS Camera, Max Payne Ballistic Dive & Fermi Time-Dilation Field
We are evolving Noble Fermi into a high-dopamine **3rd-Person Tactical Shooter** (with seamless Over-The-Shoulder / ADS transitions) featuring an omnidirectional **Max Payne-style Ballistic Dive** that deploys a localized **Superhot-style Time-Dilation Field ("Fermi Bubble")** in real time.

### 1. 3rd-Person Over-The-Shoulder (OTS) Camera & Full-Body Visibility (`FPSCharacter.h/.cpp`)
1. **SpringArm & OTS Camera Rig**:
   - Add a `USpringArmComponent* CameraBoom` to `AFPSCharacter` attached to the character root/pelvis or upper spine height (`Z = +65.0f`), with `bUsePawnControlRotation = true`, `bEnableCameraLag = true` (`CameraLagSpeed = 18.0f`), and collision probe testing enabled so the camera never clips through Rust Tactical geometry.
   - **Standard 3rd-Person Hipfire / Sprint Framing**:
     - `TargetArmLength = 210.0f`
     - `SocketOffset = FVector(0.0f, 52.0f * ShoulderSideMultiplier, 22.0f)` (classic Ghost Recon / Max Payne 3 over-the-right-shoulder framing).
     - Default FOV = `90.0f`.
   - **Shoulder Swap (`V` Key)**:
     - Bind key `V` to smoothly toggle `ShoulderSideMultiplier` between `+1.0f` (Right Shoulder) and `-1.0f` (Left Shoulder) via `FMath::FInterpTo` at speed `10.0f` for corner peeking.
   - **Tight Over-The-Shoulder ADS Zoom (Right Mouse Button)**:
     - When holding RMB (`bIsAiming`), smoothly interpolate `TargetArmLength` from `210.0f` to `95.0f`, `SocketOffset` to `FVector(0.0f, 34.0f * ShoulderSideMultiplier, 16.0f)`, and Camera FOV from `90.0f` to `62.0f`.
     - *(Optional Toggle)*: Allow pressing `MiddleMouseButton` (or `Alt`) while aiming to toggle between **3rd-Person Tight OTS ADS** and **1st-Person True Optic Red-Dot ADS** (`TargetArmLength = 0.0f`, aligning camera with the NF-98 Carbine reflex sight).
2. **Full 21-Bone Operator Body Visible in 3rd Person**:
   - Ensure the player's own 21-bone procedural body (`FPSHumanBones` / character visual components, armor carrier, helmet/GPNVG-18, and equipped `AFPSWeaponBase`) is fully visible to the owning player in 3rd-person view (`SetOwnerNoSee(false)`), and dynamically pitches the upper torso (`spine_02`, `spine_03`, `head`, and weapon mount) to match the player's `GetControlRotation().Pitch` so the operator visibly aims up/down at targets.

### 2. Max Payne Omnidirectional Ballistic Dive (`FPSCharacterMovementComponent` & `FPSCharacter`)
1. **Trigger & Cost**:
   - Pressing `LeftControl` or `C` while moving and pressing `Spacebar` (or binding a dedicated **Tactical Dive** trigger on `LeftAlt` / double-tap `C` while moving in any WASD direction) launches an **Omnidirectional Ballistic Dive**.
   - Consumes `35.0f` Stamina (requires at least `25.0f` current stamina).
2. **Physics & 3rd-Person Dive Posture**:
   - Launch the character along their current input movement vector (`Horizontal Launch = 820.0f cm/s`, `Vertical Upward Arc = 340.0f cm/s`).
   - While `bIsBallisticDiving` is active (airborne + brief 0.45s prone slide on landing):
     - Procedurally tilt/pitch the character's 3D body mesh `75 degrees` into the dive direction (so diving forward puts the operator horizontal chest-down; diving backward puts them horizontal on their back aiming forward; diving left/right rolls them onto their side like *Max Payne 3*).
     - Allow **full 360-degree aiming and weapon firing** with reduced spread throughout the entire mid-air dive and landing slide!
     - Upon ground impact, apply a smooth 0.45s kinetic ground slide before smoothly interpolating the character mesh back to upright tactical stance.

### 3. Localized "Fermi Time-Dilation Field" (Superhot / Quantum Break Bubble)
1. **Create `AFPSFermiTimeBubble` (`Source/NobleFermi/Combat/FPSFermiTimeBubble.h/.cpp`)**:
   - Spawned automatically attached to the player when a Ballistic Dive is triggered (lasts `2.4 seconds`, radius `450.0f` cm / 4.5 meters).
   - **Visual Representation**:
     - Render a sleek, translucent cyan/phosphor-green wireframe/fresnel procedural sphere shell (`Radius = 450.0f`) with subtle pulsing distortion/emissive edge glow that never blinds the camera (respecting our `AEM_Manual` exposure lock).
   - **Time-Dilation Physics Logic**:
     - Every tick while active, query overlapping actors inside the `450.0f` cm sphere:
       - **Enemy Characters / Other Pawns inside the bubble**: Set `CustomTimeDilation = 0.25f` (75% slow motion).
       - **The Diving Owner Operator**: Set `CustomTimeDilation = 0.55f` (giving the diving player superhuman reflex superiority, rapid fire-rate control, and graceful mid-air hang-time while maintaining crisp 1:1 mouse rotation responsiveness!).
       - **Ballistic Tracers / Physical Projectiles**: In `AFPSWeaponBase`, when firing while inside a Fermi Time Bubble (or when rounds pass through one), spawn visible glowing high-caliber brass projectile tracers that travel visually through 3D space at dilated speed (`2,800 cm/s` dilated) with bright orange/cyan wake trails so players can physically watch bullets slice past their operator's head in 3rd-person slow motion.
     - When the bubble expires (or an actor leaves the radius), cleanly restore `CustomTimeDilation = 1.0f` in `EndPlay` / overlap exit so no actor is ever permanently stuck in slow motion.

### 4. Slate HUD Telemetry Integration (`FPSHUDWidget.h/.cpp`)
- In the bottom-left Vitals panel (below Stamina), draw a sleek **"FERMI DIVE // CHRONO FIELD"** status bar in `FSlateDrawElement`:
  - Shows `[READY - ALT / DIVE]` in glowing phosphor cyan when available, `[ACTIVE // 0.25x TIME DILATION]` with an expanding pulse while diving, and cooldown recovery state.
- Add a subtle cyan vignette border & digital chronometer ticker (`0.25x c`) on the HUD during active Time Dilation.

### Build & Verification Constraints
- Strictly no new `.bat` files; everything launches through `Launch_Menu.bat`.
- Maintain `r.AllowStaticLighting=False` and `AEM_Manual` exposure lock (ensure the Fermi Bubble emissive material does not cause screen whiteout).
- Compile and copy to packaged binaries using:
```powershell
$env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"; & "D:\epuc\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" NobleFermi Win64 Development -Project="C:\Users\Boris\Documents\antigravity\noble-fermi\NobleFermi.uproject" -WaitMutex
Copy-Item -Path "Binaries\Win64\NobleFermi.exe" -Destination "Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe" -Force
if (Test-Path "Binaries\Win64\NobleFermi.pdb") { Copy-Item -Path "Binaries\Win64\NobleFermi.pdb" -Destination "Packaged\Windows\NobleFermi.pdb" -Force }

```

```

---

### What Happens Next?
Feed **Prompt A** to your agent first (either before or right after the Mannequin upgrade prompt—they complement each other directly since you'll now see your full 3rd-person operator diving in slow motion!). 

Once Prompt A compiles and you test the 3rd-person OTS camera + slow-mo dive on Rust Tactical, let me know how it feels and I will immediately drop **Prompt B: The "Fermi Echo" Acoustic Hologram Decoy & Mid-Air Quantum Swap (`Q` Key)**!

```