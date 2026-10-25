# Phase 11: Frictionless User Management, Cloudflare D1 Auth Backend & UE 5.8 Client Authentication

## 1. Architectural Overview
Phase 11 establishes a high-security, zero-friction, cross-platform **Operator Authentication & Cloud Profile Synchronization** system bridging the official web portal ([https://noble-fermi.pages.dev/](https://noble-fermi.pages.dev/)) and the Unreal Engine 5.8 standalone C++20 client (`NobleFermi.exe`).

```
+-----------------------------------------------------------------------------------------+
|                                    CLOUDFLARE EDGE                                     |
|                                                                                         |
|  +----------------------------------+        +---------------------------------------+  |
|  |       Cloudflare Pages Web       |        |       Cloudflare D1 SQL Database      |  |
|  |   https://noble-fermi.pages.dev/ |        |            noble-fermi-db             |  |
|  |  Interactive Operator Terminal   |        |   +-------------------------------+   |  |
|  |  Live Dossier (Gold/Opals/Gear)  |        |   | users     | profiles          |   |  |
|  +-----------------+----------------+        +---| id (UUID) | user_id (FK)      |---+  |
|                    |                             | callsign  | gold_balance (1500) |  |
|                    | WebCrypto (subtle)          | email     | opal_balance (50) |  |
|                    | PBKDF2 (100,000 iters)      | hash/salt | unlocked_items    |  |
|                    v HMAC-SHA256 30-Day JWT      | last_login| loadout_json      |  |
|  +-----------------------------------------+     +-------------------------------+   |  |
|  |       Cloudflare Pages Functions        |                                            |
|  |  POST /api/auth/register                |                                            |
|  |  POST /api/auth/login (Callsign or Email)                                            |
|  |  GET  /api/auth/me (Bearer Token)       |                                            |
|  |  POST /api/profile/sync                 |                                            |
|  +--------------------+--------------------+                                            |
+-----------------------|-----------------------------------------------------------------+
                        | HTTPS (CORS: *)
                        v
+-----------------------------------------------------------------------------------------+
|                           UNREAL ENGINE 5.8 C++ CLIENT                                  |
|                                                                                         |
|  +-----------------------------------------------------------------------------------+  |
|  |                              UFPSAuthSubsystem                                    |  |
|  |  - 0ms Optimistic Auto-Login on boot via Saved/NobleFermiAuth.json                |  |
|  |  - Background Profile Hydration via GET /api/auth/me                              |  |
|  |  - Bi-directional sync with UFPSEconomySubsystem (Gold, Opals, Loadouts, Items)   |  |
|  |  - Resilient Local Cryptographic Fallback (Saved/NobleFermiLocalAccounts.json)   |  |
|  |  - Authoritative PlayerState->SetPlayerName(Callsign) Sync                        |  |
|  +-----------------------------------------------------------------------------------+  |
|                                           |                                             |
|                                           v                                             |
|  +-----------------------------------------------------------------------------------+  |
|  |                       SMainMenuWidget (Pure Slate C++)                            |  |
|  |  - Layer 3 Translucent Military Modal Overlay over 3D Diorama                      |  |
|  |  - Tab 0: [ SIGN IN ] (Callsign/Email + Passkey, Enter-to-Submit)                 |  |
|  |  - Tab 1: [ NEW ENLISTMENT ] (Callsign + Email + Passkey, Starter 1,500G + 50O)   |  |
|  |  - 1-Click [ QUICK GUEST / OFFLINE OPERATOR ]                                     |  |
|  |  - Top Header Live Callsign Badge: OPERATOR: <CALLSIGN> [ONLINE // CLOUD SYNCED]  |  |
|  +-----------------------------------------------------------------------------------+  |
+-----------------------------------------------------------------------------------------+
```

---

## 2. Cloudflare D1 SQL Database & Edge Security API

### 2.1 Database Schema (`schema.sql`)
The authoritative database `noble-fermi-db` is deployed on Cloudflare D1 in region WEUR (Western Europe) with foreign key cascading and case-insensitive unique indices:

- **`users` Table**:
  - `id TEXT PRIMARY KEY` (Cryptographic UUID v4 generated via `crypto.randomUUID()`)
  - `callsign TEXT COLLATE NOCASE UNIQUE NOT NULL` (3–20 alphanumeric and underscore characters)
  - `email TEXT UNIQUE NOT NULL` (Normalized lowercase email address)
  - `password_hash TEXT NOT NULL` (Hex-encoded PBKDF2 derived bits using SHA-256 and 100,000 iterations)
  - `password_salt TEXT NOT NULL` (Unique 16-byte cryptographic random salt via `crypto.getRandomValues`)
  - `created_at INTEGER NOT NULL` (Unix epoch timestamp in seconds)
  - `last_login_at INTEGER NOT NULL` (Unix epoch timestamp in seconds)

- **`profiles` Table**:
  - `user_id TEXT PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE`
  - `gold_balance INTEGER NOT NULL DEFAULT 1500` (Starter currency awarded upon enlistment)
  - `opal_balance INTEGER NOT NULL DEFAULT 50` (Starter premium currency)
  - `unlocked_items_json TEXT NOT NULL DEFAULT '["camo_digital","headgear_fast","vest_level4","weapon_steel","Camo_0","Headgear_0","Armor_0","WeaponFinish_0"]'`
  - `equipped_loadout_json TEXT NOT NULL DEFAULT '{"camo":0,"headgear":0,"armor":0,"weaponFinish":0,"theater":0}'`
  - `login_streak INTEGER NOT NULL DEFAULT 1`
  - `updated_at INTEGER NOT NULL`

### 2.2 Edge Cryptography (Zero NPM Dependencies)
All cryptographic operations use native Web Standards (`crypto.subtle` / WebCrypto API) so the deployment requires zero external npm packages:
- **PBKDF2 Password Hashing**:
  1. Generate 16 bytes using `crypto.getRandomValues(new Uint8Array(16))`.
  2. Import password as raw key material with `crypto.subtle.importKey("raw", ...)`.
  3. Derive 256 bits with `crypto.subtle.deriveBits` using `PBKDF2`, `100,000` iterations, and `SHA-256`.
  4. Perform comparisons in constant time (`constantTimeEqual`) to prevent timing side-channel attacks.
- **HMAC-SHA256 Signed JWTs**:
  1. Base64URL-encoded header `{"alg":"HS256","typ":"JWT"}`.
  2. Base64URL-encoded payload containing `sub` (User ID), `callsign`, `email`, `iat`, and `exp` ($30\text{ days}$).
  3. HMAC-SHA256 signature using `env.JWT_SECRET` (with secure development fallback).

### 2.3 Edge Endpoints (`functions/api/`)
- `POST /api/auth/register`: Validates syntax, verifies uniqueness in D1, salts/hashes password, inserts user & initial profile, issues 30-day JWT, and immediately returns `{ ok: true, token, user, profile }`.
- `POST /api/auth/login`: Accepts either `callsign` OR `email` as `identifier`, checks password hash in constant time, updates `last_login_at`, and returns `{ ok: true, token, user, profile }`.
- `GET /api/auth/me` & `POST /api/auth/verify`: Validates Bearer token and returns latest operator user data and profile telemetry.
- `POST /api/profile/sync`: Authenticated endpoint that safely merges client gains with server state (ensuring unlocked items union and preventing negative balance attacks).

---

## 3. Unreal Engine 5.8 C++ Authentication Subsystem (`UFPSAuthSubsystem`)

### 3.1 Architecture & Lifecyle
- Defined in `Source/NobleFermi/Auth/FPSAuthSubsystem.h` and `FPSAuthSubsystem.cpp`.
- Inherits from `UGameInstanceSubsystem`, running continuously throughout the entire game lifecycle.
- Reads configuration from command line (`-AuthUrl=...`) or `Config/DefaultGame.ini` (`[NobleFermi.Auth] ApiBaseUrl=...`).
- Defaults to `https://noble-fermi.pages.dev/api`.

### 3.2 0ms Optimistic Auto-Login on Startup
On subsystem `Initialize(...)`:
1. Inspects `FPaths::ProjectSavedDir() / TEXT("NobleFermiAuth.json")`.
2. If a valid token and callsign are present, marks the player as optimistically authenticated immediately (`bIsAuthenticated = true`, using cached Callsign).
3. The lobby opens with **0ms UI freeze** and displays the operator's callsign.
4. Concurrently in the background, fires an asynchronous `GET /api/auth/me` request to Cloudflare D1.
5. On HTTP 200, updates player telemetry and synchronizes economy balances.
6. If the network is unreachable (offline play), transitions gracefully to offline mode (`bIsOfflineMode = true`) while keeping the player in-game.
7. If the token was revoked (HTTP 401), clears the expired session.

### 3.3 Unbreakable Offline Cryptographic Fallback
If the client cannot reach the Cloudflare Edge API due to network outages, firewall blocks, or offline development:
1. `TryLocalOfflineLogin(Identifier, Password)` inspects `Saved/NobleFermiLocalAccounts.json`.
2. Validates credentials using salted SHA-1 cryptographic hashes.
3. Automatically marks the player authenticated with status `"LOCAL / OFFLINE TACTICAL LINK ACTIVE"`.
4. Players can also register offline or click **`[ QUICK GUEST / OFFLINE OPERATOR ]`** to jump straight into combat without typing.

### 3.4 Economy & PlayerState Bridging
- `ApplyCloudProfileToEconomy()` parses cloud JSON and bridges directly with `UFPSEconomySubsystem`:
  - `SetBalances(Gold, Opals)`
  - `UnlockItemDirectly(ItemID)`
  - `SetEquippedLoadout(Camo, Headgear, Armor, WeaponFinish)`
- `RefreshLocalPlayerName()` calls `PlayerController->PlayerState->SetPlayerName(Callsign)` so the operator's callsign is automatically propagated across multiplayer netcode, killfeed, and scoreboards.

---

## 4. Pure Slate C++ In-Client Tactical UI (`MainMenuWidget`)

Respecting **Zero Webviews / Zero UMG Bloat**:
1. **Modal Overlay Layer (`BuildAuthModal()`)**:
   - Rendered as Layer 3 in `SOverlay` directly over the real-time 3D operator diorama.
   - Dark acrylic border (`#030712`) with 14.0f background blur, cyan accent stripes, and tactical monospace typography.
2. **Instant Mode Tabs**:
   - **`[ SIGN IN ]`**:
     - `SEditableTextBox` for Callsign or Email.
     - `SEditableTextBox` (Password mode) with Enter-key event commit.
     - Primary button `[ SIGN IN & DEPLOY ]`.
   - **`[ NEW ENLISTMENT ]`**:
     - `SEditableTextBox` for Callsign.
     - `SEditableTextBox` for Tactical Email.
     - `SEditableTextBox` (Password mode) with Enter-key event commit.
     - Primary button `[ ENLIST & DEPLOY IMMEDIATELY (+1,500 GOLD) ]`.
3. **Persistent Header Operator Badge**:
   - Top-right header widget displaying:
     - `OPERATOR: <CALLSIGN>`
     - Status: `[ONLINE // CLOUD SYNCED]` (Emerald) or `[LOCAL // OFFLINE TACTICAL LINK ACTIVE]` (Amber)
     - `[ SWITCH / LOGOUT ]` action button.

---

## 5. Web Operator Portal (`web/index.html`)

- Added top navigation button: `OPERATOR // ENLIST / SIGN IN`.
- Full modal overlay with dark glass styling, vector corner brackets, and tab switching between enlistment and login.
- Connected to `/api/auth/register`, `/api/auth/login`, and `/api/auth/me`.
- Displays real-time **Gold Reserve**, **Opal Vault**, unlocked item count, and consecutive login streak.
- Displays the client deployment readiness badge:
  `"READY FOR DEPLOYMENT — SIGN IN WITH THIS CALLSIGN IN NOBLE FERMI CLIENT"`.
- Persists session token in browser `localStorage` and hydrates automatically on visit.
