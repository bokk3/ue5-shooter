-- =============================================================================
-- NOBLE FERMI // CLOUDFLARE D1 OPERATOR AUTHENTICATION & PROFILE SCHEMA
-- High-Security PBKDF2 WebCrypto Auth & Authoritative Profile State
-- =============================================================================

CREATE TABLE IF NOT EXISTS users (
    id TEXT PRIMARY KEY,
    callsign TEXT COLLATE NOCASE UNIQUE NOT NULL,
    email TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    password_salt TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    last_login_at INTEGER NOT NULL
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_users_callsign_nocase ON users(callsign COLLATE NOCASE);
CREATE UNIQUE INDEX IF NOT EXISTS idx_users_email_nocase ON users(email COLLATE NOCASE);

CREATE TABLE IF NOT EXISTS profiles (
    user_id TEXT PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    gold_balance INTEGER NOT NULL DEFAULT 1500,
    opal_balance INTEGER NOT NULL DEFAULT 50,
    unlocked_items_json TEXT NOT NULL DEFAULT '["Camo_0","Headgear_0","Armor_0","WeaponFinish_0","camo_digital","headgear_fast","vest_level4","weapon_steel"]',
    equipped_loadout_json TEXT NOT NULL DEFAULT '{"camo":0,"headgear":0,"armor":0,"weaponFinish":0,"theater":0}',
    login_streak INTEGER NOT NULL DEFAULT 1,
    updated_at INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_profiles_updated_at ON profiles(updated_at);
