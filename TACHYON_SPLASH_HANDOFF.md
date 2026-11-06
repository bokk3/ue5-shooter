# HANDOFF REPORT: TACHYON ANIMATED HERO SPLASH SCREEN

**Date**: November 6, 2026  
**Target Repository**: `noble-fermi` (`C:\Users\Boris\Documents\antigravity\noble-fermi`)  
**Primary Author/Agent**: Web Studio & Graphics Agent  
**Receiving Agent**: Noble Fermi Main Gameplay / Engine Agent  

---

## 1. Executive Summary

As requested, the authentic **Tachyon Studio animated laser-cut hero logo** (`LaserCutLogo.tsx` from the studio web portal) has been implemented natively in C++ inside [`Source/NobleFermi/NobleFermi.cpp`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/NobleFermi.cpp) as a zero-stutter startup splash screen.

### Non-Destructive Scope Guarantee
- **Strictly scoped to [`Source/NobleFermi/NobleFermi.cpp`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/NobleFermi.cpp)**.
- **Zero changes** to characters (`SK_BaseMale`, `ABP_BaseMale`), weapon components (`FPSWeaponBase`, `FPSEquipmentComponent`), locomotion, materials, audio assets, or gameplay maps.
- Verified cleanly compiled with `Build_NobleFermi.bat` (UnrealBuildTool / UE 5.8) with **0 errors**.

---

## 2. Architecture & Execution Pipeline

```
[Engine Boot / StartupModule]
           │
           ▼
[NobleFermiSplashProgress::GSplashThread]
           │
           ├─► 1. RunTachyonTransparentSplash() (0.0s - 4.15s)
           │      • WS_EX_LAYERED | WS_EX_TOPMOST (840x470 centered)
           │      • True desktop transparency via UpdateLayeredWindow
           │      • Procedural in-memory Web Audio track via PlaySoundW
           │      • Micro-synchronized 4-phase laser cut sequence
           │      • Smooth global alpha dissolve (3.75s - 4.15s)
           │
           └─► 2. Restore Noble Fermi Splash Window (4.15s+)
                  • Show underlying UE splash
                  • Live tactical gold progress bar & system telemetry
                  • Seamless handoff into MenuMap / Command Center
```

### Key Technical Pillars:
1. **True Borderless Transparency**:
   Uses Win32 `WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW` and 32-bit top-down DIB section (`BI_RGB`). Frames are pushed to the desktop via `UpdateLayeredWindow` with `AC_SRC_ALPHA`.
2. **Strict Premultiplied Alpha Math**:
   Windows GDI `UpdateLayeredWindow` requires that color channels satisfy $R \le A, G \le A, B \le A$. Any additive operation or flash boost is clamped against pixel alpha, completely preventing black borders or inverted color artifacts.
3. **Procedural In-Memory Audio Synthesis (`BuildTachyonSplashAudioBuffer`)**:
   Synthesizes the entire 4.2-second soundtrack in RAM using standard 44.1 kHz 16-bit PCM RIFF WAV format. Zero disk audio assets or external sound packages are required. Started synchronously at $t=0.0\text{s}$ via `PlaySoundW(..., SND_MEMORY | SND_ASYNC)`.

---

## 3. Cinematic Sequence Breakdown (1:1 with `LaserCutLogo.tsx`)

| Phase | Timeline | Audio Synthesis | Visual Effects Rendered |
| :--- | :--- | :--- | :--- |
| **Phase 1: Laser Cut** | `0.0s - 2.40s` | Sawtooth pitch sweep ($440 \to 880 \to 620\text{ Hz}$) + random crackle | • Progressive top-to-bottom unveiling of Tachyon branding artwork<br/>• Perimeter faceted cut line with 9px soft cyan glow, 3.5px sharp seam, 1.5px white core<br/>• Torch cutter head: 18px cyan aura, 8px magenta ring, 4.5px white core<br/>• 5 fixed directional sparks matching SVG lines + 6 dynamic spatter sparks |
| **Phase 2: Thermal Flash** | `2.40s - 2.65s` | Triangle glide snap pop ($950 \to 120\text{ Hz}$) + $2800\text{ Hz}$ high-pass thermal hiss | • Surface flash with premultiplied alpha safety ($R, G, B \le A$ clamped)<br/>• Incandescent white-hot seam along entire faceted perimeter |
| **Phase 3: Thermal Cooldown & Specular Sheen** | `2.65s - 3.75s` | Sub-bass surge ($65 \to 140 \to 45\text{ Hz}$) + bandpass whoosh ($220 \to 1200 \to 180\text{ Hz}$) + crystalline chime ($1650 \to 2400 \to 1200\text{ Hz}$) | • **Thermal Seam Cooldown**: cools White `#FFFFFF` $\to$ Amber `#FFB800` $\to$ Cyan `#00F3FF` $\to$ Cobalt `#0055FF` $\to$ fade<br/>• **Specular Sheen Sweep**: $115^\circ$ angled beam (Cyan leading, White core, Magenta trailing)<br/>• **Expanding Shockwave**: Center radius $25\text{px} \to 290\text{px}$, Cyan $\to$ Magenta<br/>• **Rotating Diamond Starburst**: Scales up, rotates $0^\circ \to 135^\circ$<br/>• **36 Dust Particles**: Drift and fall with gravity curves across 6 palette colors |
| **Phase 4: Global Dissolve** | `3.75s - 4.15s` | Smooth audio tail decay | • Window global alpha dissolves from $255 \to 0$<br/>• Clean GDI and thread cleanup<br/>• Underlying Noble Fermi splash window restored |

---

## 4. Developer Controls & Bypass Flag

For rapid iteration or automated test runs where developers want to bypass the Tachyon splash screen:
- Launch with the `-NoTachyon` command line argument:
  ```powershell
  # Skip Tachyon splash and load directly to Noble Fermi progress bar / map:
  .\Launch_Menu.bat -NoTachyon
  ```
- Alternatively in code:
  `FParse::Param(FCommandLine::Get(), TEXT("NoTachyon"))` is checked at the entry of `RunTachyonTransparentSplash()`.

---

## 5. Verification & Testing

- **Compilation**: Verified via `Build_NobleFermi.bat` (UnrealBuildTool for UE 5.8).
  ```powershell
  .\Build_NobleFermi.bat
  # Result: Succeeded (0 errors, 16.22s)
  ```
- **Live Launch**:
  ```powershell
  .\Launch_Menu.bat
  ```
  Launches `UnrealEditor.exe NobleFermi.uproject /Game/Maps/MenuMap?game=/Script/NobleFermi.FPSMenuGameMode -game -d3d11 -windowed -ResX=1280 -ResY=720 -log`.

---

## 6. Relevant Code References

- **Implementation File**: [`Source/NobleFermi/NobleFermi.cpp`](file:///C:/Users/Boris/Documents/antigravity/noble-fermi/Source/NobleFermi/NobleFermi.cpp)
  - `BuildTachyonSplashAudioBuffer()`: lines 151–269
  - `RunTachyonTransparentSplash()`: lines 371–847
  - `SplashWorkerLoop()`: lines 849–1096
  - `StartupModule()` / `ShutdownModule()`: lines 1100–1135
- **Reference Web Implementation**: [`LaserCutLogo.tsx`](file:///C:/Users/Boris/Documents/antigravity/game-studio/studio-monorepo/apps/web-gateway/src/components/ui/LaserCutLogo.tsx)
- **Branding BGRA Asset**: `Content/Splash/tachyon_logo_840x470.bgra` (1,579,200 bytes)
