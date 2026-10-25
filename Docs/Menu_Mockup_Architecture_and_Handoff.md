# Noble Fermi — Tactical Command Menu & Online Account Verification Documentation

**Reference Mockup Image**: [`Docs/Menu_Mockup_Reference.jpg`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Docs/Menu_Mockup_Reference.jpg)

---

## 1. Visual & Functional Architecture of `UMainMenuWidget`

All menu UI is implemented in Pure C++20 Slate inside [`Source/NobleFermi/UI/MainMenuWidget.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/UI/MainMenuWidget.cpp) (zero UMG `.uasset` dependencies, ensuring 100% deterministic loading in standalone IoStore builds).

### A. Custom Slate Vector Primitives (`MainMenuWidget.cpp`)
1. **`SFPSNobleFermiEmblemWidget`**:
   - Draws the hexagonal Ballistic Aegis Shield emblem (outer Amber hexagon, inner Cyan hexagon, tactical visor brow chevron, interlocking `NF` monogram, and relativistic chrono slash).
2. **`SFPSNobleFermiWordmarkWidget`**:
   - Draws the battle-scarred `"NOBLE FERMI"` wordmark with `"TACTICAL WARFARE SIMULATION"` subtitle, 4-corner Cyan/Amber HUD brackets, diagonal hacksaw shear line, and 7.62mm molten amber bullet crater.
3. **`SFPSChamferedHudFrameWidget`**:
   - Wraps the Left Navigation Panel and Right Mission Briefing Panel in a dark translucent wet-glass container (`#050B16` at 80% opacity) with a subtle cyan border and glowing 45° chamfered **Cyan & Amber corner brackets** matching [`Docs/Menu_Mockup_Reference.jpg`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Docs/Menu_Mockup_Reference.jpg).
4. **`SFPSMenuVectorIconWidget`**:
   - Renders the 6 crisp line-art vector icons (`28x28px`) for the Left Navigation Rail:
     - `0` (`01 DEPLOYMENT`): Target reticle circle + inner ring + 4 crosshair ticks.
     - `1` (`02 CHARACTER & LOADOUT`): 3-operator tactical squad silhouettes.
     - `2` (`03 ARSENAL`): Isometric 3D supply crate cube.
     - `3` (`04 TACTICAL MAP`): 3-fold topographic map vector.
     - `4` (`05 OPERATIONS`): 4 ascending campaign/bounty progression bars.
     - `5` (`06 SYSTEM OPTIONS`): 8-tooth mechanical gear cog.
5. **`SFPSTacticalRadarGlobeWidget`**:
   - Renders the `130x130px` circular phosphor/cyan radar globe inside the Right Panel's `TACTICAL OVERVIEW` section:
     - Draws N / E / S / W compass bearings, concentric radar rings, sweep line, map-specific terrain contours (`Rust Tactical` vs `Coastal Operations`), and glowing unit blips.
6. **`SFPSSegmentedBarWidget`**:
   - Renders the 10-segment status bars next to the radar globe (`ENEMY PRESENCE: HIGH` in Crimson/Amber and `FRIENDLY UNITS: 3/8` in Tactical Cyan).

---

### B. 4-Zone Mockup Layout (`RebuildWidget()`)

1. **Top Telemetry Header Bar (`BuildHeader()`)**:
   - **Left**: `SFPSNobleFermiEmblemWidget` + `NOBLE FERMI` (`TACTICAL WARFARE SIMULATION`) + angled cyan status notch `● ONLINE • 3/8 OPERATORS`.
   - **Center**: `👤 VERSION 1.2.8.47  |  🌐 EU-WEST-1  |  ⚡ 42 MS` + live Gold (`🪙`) and Opal (`💎`) balances.
   - **Right**: Quick action links (`LOADOUT  |  ⚙ SETTINGS  |  🎧 ACCOUNT`) + bright Amber/Gold **`[ ⏻ EXIT ]`** button.
2. **Left Navigation Panel (`295px` wide inside `SFPSChamferedHudFrameWidget`)**:
   - `01` — **DEPLOYMENT** (`MISSION SELECTION & BRIEFING`) $\rightarrow$ `EFPSMenuSection::Play` (`Slot 0`)
   - `02` — **CHARACTER & LOADOUT** (`OPERATOR CUSTOMIZATION`) $\rightarrow$ `EFPSMenuSection::Customization` (`Slot 5`)
   - `03` — **ARSENAL** (`WEAPONS & GEAR`) $\rightarrow$ `EFPSMenuSection::Loadout` (`Slot 1`)
   - `04` — **TACTICAL MAP** (`INTEL & STRATEGY`) $\rightarrow$ `EFPSMenuSection::ServerBrowser` (`Slot 2`)
   - `05` — **OPERATIONS** (`CAMPAIGN PROGRESSION`) $\rightarrow$ `EFPSMenuSection::DailyRewards` (`Slot 4`)
   - `06` — **SYSTEM OPTIONS** (`GRAPHICS / AUDIO / CONTROLS`) $\rightarrow$ `EFPSMenuSection::Settings` (`Slot 3`)
   - Bottom watermark: `/// NOBLE FERMI — TACTICAL WARFARE SIMULATION`.
3. **Center 3D Viewport (`BuildCenterViewportHUD()`)**:
   - Leaves the center 3D `AFPSCharacterPreviewActor` 100% unobstructed from head to toe on the arrival screen (`01 DEPLOYMENT`), while surfacing turntable/pose controls when inside `02 CHARACTER & LOADOUT`.
4. **Right Mission Briefing & Deployment Panel (`390px` wide inside `SFPSChamferedHudFrameWidget`)**:
   - **Header**: Hexagonal Shield Emblem + `NOBLE FERMI` (`TACTICAL WARFARE SIMULATION`).
   - **`ACTIVE OPERATION` Selector Box** (`LOBBY 3/8`):
     - Clickable card with glowing cyan border and right `>` chevron that toggles between:
       - `OPERATION: RUST TACTICAL` (`Industrial Oil Yard / High Intensity`)
       - `OPERATION: COASTAL OPERATIONS` (`North Sea Littoral / Dike & Pier`)
   - **`OPERATION BRIEF`**:
     - Dynamic 2-line mission brief and recommended loadout (`"Secure the facility, eliminate hostiles, extract intel. Recommended Loadout: Kit 2, Medium Armor."`).
   - **`TACTICAL OVERVIEW`**:
     - Left: `SFPSTacticalRadarGlobeWidget` (`130x130px` circular radar with N/E/S/W compass and blips).
     - Right: `ENEMY PRESENCE` (`HIGH` red segmented bar), `FRIENDLY UNITS` (`3/8` cyan segmented bar), and `MISSION TIME` (`⏱ 00:15:00`).
   - **3 Action Pills**:
     - `[ VIEW INTEL ]` (opens `04 TACTICAL MAP`)
     - `[ LOADOUT ]` (opens `02 CHARACTER & LOADOUT`)
     - `[ RULES ]` (cycles `6v6 Tactical TDM` $\rightarrow$ `Chrono Lab` $\rightarrow$ `Bot Skirmish`)
   - **Bottom Hero CTA Button**:
     - Full-width bright Amber/Yellow button: **`DEPLOY TO AREA  >`** with dynamic subtitle (`OPERATION: RUST TACTICAL` / `OPERATION: COASTAL OPERATIONS`).
5. **Bottom Chamfered Status Footer (`BuildFooter()`)**:
   - Left: `🌐 SERVER: EU-WEST-1   |   PING: 42 MS`
   - Right: `REAL OPERATORS. REAL TACTICS. NO EXCUSES.`

---

## 2. Cloudflare D1 & 6-Digit Website Verification System

- **Production Website & API**: `https://noble-fermi.pages.dev` (Cloudflare Pages project `noble-fermi`, D1 database `noble-fermi-db`).
- **Database Schema ([`web/schema.sql`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/web/schema.sql))**:
  - `users` table includes `is_verified INTEGER NOT NULL DEFAULT 0` and `verification_code TEXT NOT NULL DEFAULT ''`.
- **Endpoints**:
  - `POST /api/auth/register` ([`register.js`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/web/functions/api/auth/register.js)): Creates user with `is_verified = 0` and a random 6-digit `verification_code`.
  - `POST /api/auth/login` ([`login.js`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/web/functions/api/auth/login.js)): Returns `requires_verification: !user.is_verified` and `user.verification_code`.
  - `POST /api/auth/verify` ([`verify.js`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/web/functions/api/auth/verify.js)): Validates `{ identifier, code }` against `users.verification_code` and sets `is_verified = 1`.
- **Client Verification Flow ([`FPSAuthSubsystem.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Auth/FPSAuthSubsystem.cpp) & [`MainMenuWidget.cpp`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/UI/MainMenuWidget.cpp))**:
  - Whenever an account registers on the client or logs into the client with `is_verified == false`, `OnAuthError` broadcasts `VERIFY_REQUIRED::<Callsign>::<Token>`, switching the client Auth Modal to **Slot 2 (`WEBSITE CODE VERIFICATION`)**.
  - Clicking **`[ 🌐 OPEN WEBSITE & VIEW 6-DIGIT CODE ► ]`** opens `https://noble-fermi.pages.dev/?verify=<Callsign>&token=<Token>`, which auto-logs the user into the website and opens their **Operator Dossier** displaying their 6-digit verification code.

---

## 3. Build & Stage Commands

### Compile C++ (`Win64 Development`)
```powershell
$env:DOTNET_ROOT = "D:\epuc\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
& "D:\epuc\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" NobleFermi Win64 Development -Project="C:\Users\Boris\Documents\antigravity\noble-fermi\NobleFermi.uproject" -WaitMutex
```

### Stage to Packaged Standalone Build
```powershell
Copy-Item -Path "c:\Users\Boris\Documents\antigravity\noble-fermi\Binaries\Win64\NobleFermi.exe" -Destination "c:\Users\Boris\Documents\antigravity\noble-fermi\Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.exe" -Force
Copy-Item -Path "c:\Users\Boris\Documents\antigravity\noble-fermi\Binaries\Win64\NobleFermi.pdb" -Destination "c:\Users\Boris\Documents\antigravity\noble-fermi\Packaged\Windows\NobleFermi\Binaries\Win64\NobleFermi.pdb" -Force
```

### Deploy Website & D1 Functions to Cloudflare Pages
```powershell
Set-Location "c:\Users\Boris\Documents\antigravity\noble-fermi\web"
npx wrangler pages deploy . --project-name=noble-fermi --branch=main --commit-dirty=true
```


---

## 9. 3D Hangar Diorama & Rain-Glass Optics (`AFPSMenuHangarStage`)

### 9.1 Architectural Overview
To replace the black void behind the Operator turntable with the live 3D industrial hangar shown in `Docs/Menu_Mockup_Reference.jpg` (`menu-prop.jpeg`), [`AFPSMenuHangarStage`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Game/FPSMenuHangarStage.h) is spawned automatically by [`AFPSMenuGameMode::BeginPlay()`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Game/FPSMenuGameMode.cpp#L61-L72) at `FVector::ZeroVector` around the active [`AFPSCharacterPreviewActor`](file:///c:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/Character/FPSCharacterPreviewActor.h).

Because `PreviewCamera` is positioned at `(X = +250, Y = 0, Z = 115)` looking toward `-X` (`Yaw = 180°`, `Pitch = -3°`, `FOV = 50°`):
- **Depth into the Hangar (Behind the Operator)**: `-X` (`X = -150` to `X = -1250 cm`)
- **Screen Left (Operator's Right Shoulder — Warm Amber Storm Windows Zone)**: `+Y` (`Y = +45` to `Y = +480 cm`)
- **Screen Right (Operator's Left Arm — Cyan Command Consoles Zone)**: `-Y` (`Y = -60` to `Y = -360 cm`)
- **Floor Plane**: `Z = 0.0f` (flush beneath the Operator's boots).

### 9.2 Five Forced-Perspective Depth Layers
1. **Layer 1 — Wet Industrial Grate & Concrete Floor + Tactical Turntable Plinth**:
   - `WetFloorPlane`: Uses a runtime-generated `256x256` normal-mapped and spatially roughness-mapped PBR shader (`GenerateWetHangarFloorTextures`) with `Roughness = 0.09` inside organic rain-water puddle pools and `0.48–0.62` on grate treads (`#070B10` to `#0E151D`).
   - `FloorGrateStrips`: Physical dark metallic steel grating ribs (`M_AK47_Steel`, `Metallic = 0.85`, `Roughness = 0.35`) catching broken orange and cyan specular highlights across the foreground floor.
   - `PlinthOuterRing`, `PlinthBevelRing`, & `PlinthCyanPips`: Low-profile beveled dark steel turntable rim (`Radius = 53 cm`) with 12 glowing cyan (`#00DDF5`) micro-pips grounding the `360° TURNTABLE` in world space.
2. **Layer 2 — Left-Midground Angled Storm Windows & Structural Ribs (`+Y`, Warm Backlight Zone)**:
   - `LeftStructuralBeams`: Heavy dark-steel I-beam pillars tilted inward at `-13.5°` roll along a `-16.5°` angled bulkhead wall, paired with slanted upper skylight rafters and horizontal transoms.
   - `LeftRainWindowPanes`: Translucent/subsurface rain-streaked window glass (`GenerateRainGlassTextures`, `MSM_SUBSURFACE` warm amber transmission `SubsurfaceIntensity = 2.8f` + vertical rain-rivulet normal distortion). Configured with `CastShadow = false` so exterior spotlights cast crisp diagonal window-rib shafts into the hangar.
   - `ExteriorSilhouetteMeshes` & `ExteriorFireBackdrop`: Silhouetted industrial machinery, derrick legs, refinery pipes, and container stacks behind the slanted glass bathed in fiery sodium-amber light (`#FF7A1A`).
   - `LeftMidgroundGeneratorCrate` & `LeftCrate_CyanLED`: Midground industrial floor crate at `(X = -315, Y = +94)` with a glowing cyan status LED pip.
3. **Layer 3 — Right-Midground Tactical Command Consoles (`-Y`, Cyan Tech Zone)**:
   - `ConsoleRackFrames` & `ConsoleMonitorScreens`: Framed cleanly in the gap between the Operator's left arm and the Right UI Panel (`X = -370` to `-560`, `Y = -75` to `-245`).
   - Features 2 upper widescreen monitors + 2 lower angled desk monitors displaying procedural `256x256` tactical radar rings, crosshairs, telemetry bar graphs, and data matrices (`GenerateTerminalScreenTextures`) in **Noble Fermi Cyan (`#00DDF5`)** and **Teal (`#0A2633`)**, boosted to ~3.5x HDR luminance by `CyanScreenIllumRectLight`.
4. **Layer 4 — Overhead Gantry, Pipes & Ceiling Skylights**:
   - `OverheadGantryMeshes`: Transverse steel ceiling I-beam trusses, longitudinal crane rails, cylindrical pipes (`Z = 224–270 cm`), and an enclosing dark roof slab (`Z = 345 cm`) keeping the Top Status Bar 100% legible.
5. **Layer 5 — Deep Background Wall & Atmospheric Terminus**:
   - `DeepBulkheadMeshes`: Rear ribbed steel bulkhead wall and blast door frame at `X = -1250 cm` plus side enclosure walls so zero empty skybox or void ever leaks through.

### 9.3 Three-Tone Split Lighting Rig, Rain FX & Post-Process Lock
- **Warm Sodium-Amber Key/Rim (Left Rear `+Y`)**: `WarmWindowKeySpotLight` (`48,000`, `#FF7A1A`), `WarmOperatorRimSpotLight` (`28,000`, `#FFA033`), `WarmWindowRectLight`, and `ExteriorAmberGlowLight`.
- **Cool Tactical Ambient & Overhead Fill**: `CoolOverheadRectLight` (`6,500`, `#2A5C7A`) and `HangarSkyLight` (`#1B3B52`, dark navy lower hemisphere).
- **Noble Fermi Cyan Counter-Light (Right Rear `-Y`)**: `CyanConsoleBounceLight` (`11,000`, `#00DDF5`), `CyanOperatorRimSpotLight` (`20,000`, `#00DDF5`), and `CyanScreenIllumRectLight`.
- **Operator Front Readability Fill**: `OperatorFrontFillSpotLight` (`5,500`, `#C8DCF0`).
- **Forefront Rain FX (`Tick`)**: 18 vertical rain-streak ribbons (`ForefrontRainStreaks`) and 24 glistening water droplet beads (`ForefrontRainDroplets`) falling along 5 drip-chains in the left-foreground bay (`Y = +44..+148 cm`), leaving the Operator's face and UI panels completely unobstructed.
- **Exposure, DoF & `06 // SYSTEM OPTIONS` Hook**: Locked `AEM_Manual` exposure, physical Depth of Field focused at `FocalDistance = 250.0f` (`f/3.2`), `0.35` vignette, and automatic scalability switching via `AFPSMenuHangarStage::ApplyScalabilityTier(bool bInUltraGIMode)`.
