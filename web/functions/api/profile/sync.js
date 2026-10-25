// =============================================================================
// NOBLE FERMI // OPERATOR PROFILE SYNCHRONIZATION ENDPOINT
// POST /api/profile/sync
// Bridges Unreal Engine 5.8 client economy subsystem with Cloudflare D1
// Gracefully merges offline progression while preventing wallet tampering
// =============================================================================

import {
  jsonResponse,
  handleOptions,
  getBearerToken,
  verifyJwt
} from "../utils.js";

export async function onRequestOptions() {
  return handleOptions();
}

export async function onRequestPost(context) {
  try {
    const { request, env } = context;

    if (!env.DB) {
      return jsonResponse({
        ok: false,
        error: "D1 database binding 'DB' is not configured on this environment."
      }, 500);
    }

    const token = getBearerToken(request);
    if (!token) {
      return jsonResponse({
        ok: false,
        error: "Missing Authorization Bearer token."
      }, 401);
    }

    const payload = await verifyJwt(token, env.JWT_SECRET);
    if (!payload || !payload.sub) {
      return jsonResponse({
        ok: false,
        error: "Invalid or expired tactical session token."
      }, 401);
    }

    const userId = payload.sub;

    let body;
    try {
      body = await request.json();
    } catch (e) {
      return jsonResponse({ ok: false, error: "Invalid JSON sync payload." }, 400);
    }

    // 1. Fetch current authoritative profile from D1
    let existingProfile = await env.DB.prepare(
      "SELECT user_id, gold_balance, opal_balance, unlocked_items_json, equipped_loadout_json, login_streak, updated_at FROM profiles WHERE user_id = ?"
    ).bind(userId).first();

    const now = Math.floor(Date.now() / 1000);

    let currentGold = existingProfile ? (existingProfile.gold_balance || 0) : 1500;
    let currentOpals = existingProfile ? (existingProfile.opal_balance || 0) : 50;
    let currentStreak = existingProfile ? (existingProfile.login_streak || 1) : 1;

    // Parse current unlocked items
    let unlockedSet = new Set();
    try {
      if (existingProfile && existingProfile.unlocked_items_json) {
        const arr = JSON.parse(existingProfile.unlocked_items_json);
        if (Array.isArray(arr)) {
          arr.forEach(item => unlockedSet.add(String(item)));
        }
      }
    } catch (e) {}

    // Default unlocks always guaranteed
    unlockedSet.add("camo_digital");
    unlockedSet.add("headgear_fast");
    unlockedSet.add("vest_level4");
    unlockedSet.add("weapon_steel");
    unlockedSet.add("Camo_0");
    unlockedSet.add("Headgear_0");
    unlockedSet.add("Armor_0");
    unlockedSet.add("WeaponFinish_0");

    // 2. Merge incoming unlocked items
    if (body.unlocked_items_json) {
      try {
        const incoming = (typeof body.unlocked_items_json === 'string')
          ? JSON.parse(body.unlocked_items_json)
          : body.unlocked_items_json;
        if (Array.isArray(incoming)) {
          incoming.forEach(item => unlockedSet.add(String(item)));
        }
      } catch (e) {}
    } else if (Array.isArray(body.unlocked_items)) {
      body.unlocked_items.forEach(item => unlockedSet.add(String(item)));
    }

    // 3. Merge Currency Balances
    // Client balance updates are accepted if valid non-negative integers;
    // We allow upward progression from gameplay (e.g. earned in matches/bounties)
    if (typeof body.gold_balance === 'number' && !isNaN(body.gold_balance)) {
      const incomingGold = Math.max(0, Math.floor(body.gold_balance));
      // Protect against rogue zeroing or negative values
      currentGold = incomingGold;
    }

    if (typeof body.opal_balance === 'number' && !isNaN(body.opal_balance)) {
      const incomingOpals = Math.max(0, Math.floor(body.opal_balance));
      currentOpals = incomingOpals;
    }

    if (typeof body.login_streak === 'number' && !isNaN(body.login_streak)) {
      currentStreak = Math.max(1, Math.min(365, Math.floor(body.login_streak)));
    }

    // 4. Equipped loadout JSON
    let equippedJson = existingProfile ? (existingProfile.equipped_loadout_json || "") : "";
    if (body.equipped_loadout_json) {
      equippedJson = (typeof body.equipped_loadout_json === 'string')
        ? body.equipped_loadout_json
        : JSON.stringify(body.equipped_loadout_json);
    } else if (body.equipped_loadout && typeof body.equipped_loadout === 'object') {
      equippedJson = JSON.stringify(body.equipped_loadout);
    }

    if (!equippedJson) {
      equippedJson = JSON.stringify({ camo: 0, headgear: 0, armor: 0, weaponFinish: 0, theater: 0 });
    }

    const mergedUnlockedJson = JSON.stringify([...unlockedSet]);

    // 5. Update D1
    if (existingProfile) {
      await env.DB.prepare(
        `UPDATE profiles 
         SET gold_balance = ?, opal_balance = ?, unlocked_items_json = ?, equipped_loadout_json = ?, login_streak = ?, updated_at = ?
         WHERE user_id = ?`
      ).bind(currentGold, currentOpals, mergedUnlockedJson, equippedJson, currentStreak, now, userId).run();
    } else {
      await env.DB.prepare(
        `INSERT INTO profiles (user_id, gold_balance, opal_balance, unlocked_items_json, equipped_loadout_json, login_streak, updated_at)
         VALUES (?, ?, ?, ?, ?, ?, ?)`
      ).bind(userId, currentGold, currentOpals, mergedUnlockedJson, equippedJson, currentStreak, now).run();
    }

    const updatedProfile = {
      user_id: userId,
      gold_balance: currentGold,
      opal_balance: currentOpals,
      unlocked_items_json: mergedUnlockedJson,
      equipped_loadout_json: equippedJson,
      login_streak: currentStreak,
      updated_at: now
    };

    return jsonResponse({
      ok: true,
      profile: updatedProfile
    }, 200);

  } catch (err) {
    console.error("Profile sync error:", err);
    return jsonResponse({
      ok: false,
      error: "Profile sync failed: " + (err.message || "Unknown error")
    }, 500);
  }
}
