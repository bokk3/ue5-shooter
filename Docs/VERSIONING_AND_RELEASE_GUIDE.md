# NOBLE FERMI // Versioning & Release Management Guide

This document establishes the official semantic versioning conventions, build metadata standards, branching strategy, and release packaging workflows for **Noble Fermi**.

---

## 1. Semantic Versioning Specification (SemVer 2.0.0)

Noble Fermi follows **Semantic Versioning 2.0.0** (`MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`), structured as:

$$\mathbf{v[MAJOR].[MINOR].[PATCH]-[STAGE]}$$

Example: **`v0.4.0-alpha`**

### Component Definitions

| Component | Trigger / Scope | Example Progression |
| :--- | :--- | :--- |
| **`MAJOR`** (`0.x` $\rightarrow$ `1.x`) | Complete production release, architectural paradigm shifts, breaking API/netcode revisions. Pre-1.0 (`0.x`) denotes active development where major game systems are evolving. | `0.4.0` $\rightarrow$ `1.0.0` |
| **`MINOR`** (`x.4.x` $\rightarrow$ `x.5.x`) | Significant new gameplay systems, new maps/theaters, major UI redesigns, new weapon categories, or engine-level framework upgrades. | `0.3.0` $\rightarrow$ `0.4.0` |
| **`PATCH`** (`x.x.0` $\rightarrow$ `x.x.1`) | High-priority hotfixes, balance tuning, bug fixes, standalone asset patches, or stability improvements that do not add new game systems. | `0.4.0` $\rightarrow$ `0.4.1` |
| **`STAGE`** (Prerelease Tag) | Development milestone indicator (`alpha`, `beta`, `rc`, `release`). | `-alpha`, `-beta`, `-rc.1` |
| **`BUILD`** (Engine / Packaging) | Internal incremental build number tracked across compiler runs and CI pipelines. | `Build 0.4.0.50` (`ProjectVersion=0.4.0.0`) |

---

## 2. Release Progression History & Roadmap

```
 v0.1.0-alpha            v0.2.0-alpha            v0.3.0-alpha            v0.4.0-alpha (Current)
┌─────────────┐         ┌─────────────┐         ┌─────────────┐         ┌─────────────────────────┐
│ OTS Locomotion │  ───► │ Acrylic Lobby│  ───► │ 11-Sector   │  ───►  │ BaseMale Spec-Ops Mesh, │
│ 128Hz Netcode │       │ Favela Arena│         │ Weapon Wheel│         │ Diorama Plinth, Live    │
│ Time Bubbles  │       │ Bots & Armor│         │ 3D Thumbnails│        │ Win32 Splash Telemetry  │
└─────────────┘         └─────────────┘         └─────────────┘         └─────────────────────────┘
```

- **`v0.1.0-alpha`**: Foundational 3rd-person OTS locomotion, relativistic Fermi time-dilation sphere, and authoritative 128-Hz C++ netcode.
- **`v0.2.0-alpha`**: Modern Warfare-style 3-Zone Acrylic Command Lobby (`SBackgroundBlur`), Favela // Rocha Velha & Baía Brava arenas, 4 tactical bots, and Level IV ceramic armor plates.
- **`v0.3.0-alpha`**: 11-Sector tactical radial weapon wheel (`UFPSWeaponWheelWidget`), 3D gun models with transparent thumbnails, and modal telemetry inspector.
- **`v0.4.0-alpha`**: Restored clean high-fidelity Vanguard Spec-Ops operator (`SK_BaseMale` / `SM_Soldier_Posed`), dual PBR diffuse/normal materials, illuminated turntable plinth in 3D rain-streaked hangar diorama, and live Win32 GDI splash screen progress bar & loading telemetry.

---

## 3. Version Tracking Locations Checklist

When bumping versions, all of the following locations must be updated synchronously:

1. **`README.md`**:
   - Release badge: `https://img.shields.io/badge/Release-v0.4.0--alpha-FFEA00.svg`
   - Build badge: `https://img.shields.io/badge/Build-0.4.0.50-00E5FF.svg`
   - Header summary: `📦 Latest Release: [Noble Fermi v0.4.0-alpha (Build 0.4.0.50)]`
   - Download section: `v0.4.0-alpha Release`
2. **`Config/DefaultGame.ini`**:
   - `[/Script/EngineSettings.GeneralProjectSettings] -> ProjectVersion=0.4.0.0`
3. **`Source/NobleFermi/UI/MainMenuWidget.cpp`**:
   - Header telemetry strip: `VERSION 0.4.0-alpha | EU-WEST-1 | 42 MS`
4. **`web/index.html`**:
   - Download button anchor: `https://github.com/bokk3/ue5-shooter/releases/tag/v0.4.0-alpha`
   - Button text: `DOWNLOAD V0.4.0-ALPHA RELEASE`
5. **Git Tag**:
   - Annotated Git tag with release notes: `git tag -a v0.4.0-alpha -m "NOBLE FERMI v0.4.0-alpha: ..."`

---

## 4. Release Packaging & Distribution Workflow

### Packaging Command (PowerShell)

To generate the standalone release archive:

```powershell
# Create destination directory
New-Item -ItemType Directory -Force -Path "Releases"

# Package standalone client files into distribution zip
Compress-Archive -Path "Packaged\Windows\*" -DestinationPath "Releases\NobleFermi-v0.4.0-alpha-Windows.zip" -Force
```

### Git Tag Creation

```powershell
# Verify working tree is clean
git status

# Create annotated tag
git tag -a v0.4.0-alpha -m "NOBLE FERMI v0.4.0-alpha: High-Res Spec-Ops Operator, Turntable Plinth & Live Splash Telemetry"

# Push commit and tags to origin (when ready)
# git push origin master --tags
```
