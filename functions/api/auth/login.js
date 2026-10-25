// =============================================================================
// NOBLE FERMI // OPERATOR AUTHENTICATION (LOGIN) ENDPOINT
// POST /api/auth/login
// Supports dual-mode authentication via either Callsign OR Tactical Email
// =============================================================================

import {
  jsonResponse,
  handleOptions,
  hashPassword,
  constantTimeEqual,
  signJwt
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

    let body;
    try {
      body = await request.json();
    } catch (e) {
      return jsonResponse({ ok: false, error: "Invalid JSON request body." }, 400);
    }

    const identifier = (body.identifier || body.callsign || body.email || "").trim();
    const password = body.password || "";

    if (!identifier) {
      return jsonResponse({
        ok: false,
        error: "Please enter your Operator Callsign or Tactical Email."
      }, 400);
    }

    if (!password) {
      return jsonResponse({
        ok: false,
        error: "Passkey is required."
      }, 400);
    }

    // 1. Look up user by callsign OR email
    const user = await env.DB.prepare(
      "SELECT id, callsign, email, password_hash, password_salt FROM users WHERE LOWER(callsign) = LOWER(?) OR LOWER(email) = LOWER(?) LIMIT 1"
    ).bind(identifier, identifier).first();

    if (!user) {
      return jsonResponse({
        ok: false,
        error: "Invalid tactical credentials. Callsign/email not recognized."
      }, 401);
    }

    // 2. Hash incoming password with stored salt & verify in constant time
    const computedHash = await hashPassword(password, user.password_salt);
    if (!constantTimeEqual(computedHash, user.password_hash)) {
      return jsonResponse({
        ok: false,
        error: "Invalid tactical credentials. Passkey verification failed."
      }, 401);
    }

    const now = Math.floor(Date.now() / 1000);

    // 3. Update last login timestamp
    await env.DB.prepare(
      "UPDATE users SET last_login_at = ? WHERE id = ?"
    ).bind(now, user.id).run();

    // 4. Retrieve associated operator profile
    let profile = await env.DB.prepare(
      "SELECT user_id, gold_balance, opal_balance, unlocked_items_json, equipped_loadout_json, login_streak, updated_at FROM profiles WHERE user_id = ?"
    ).bind(user.id).first();

    if (!profile) {
      // Lazy repair for legacy users
      const defaultUnlockedJson = JSON.stringify([
        "camo_digital", "headgear_fast", "vest_level4", "weapon_steel",
        "Camo_0", "Headgear_0", "Armor_0", "WeaponFinish_0"
      ]);
      const defaultEquippedJson = JSON.stringify({
        camo: 0, headgear: 0, armor: 0, weaponFinish: 0, theater: 0
      });

      await env.DB.prepare(
        "INSERT INTO profiles (user_id, gold_balance, opal_balance, unlocked_items_json, equipped_loadout_json, login_streak, updated_at) VALUES (?, 1500, 50, ?, ?, 1, ?)"
      ).bind(user.id, defaultUnlockedJson, defaultEquippedJson, now).run();

      profile = {
        user_id: user.id,
        gold_balance: 1500,
        opal_balance: 50,
        unlocked_items_json: defaultUnlockedJson,
        equipped_loadout_json: defaultEquippedJson,
        login_streak: 1,
        updated_at: now
      };
    }

    // 5. Issue 30-day HMAC-SHA256 JWT
    const token = await signJwt(
      {
        sub: user.id,
        callsign: user.callsign,
        email: user.email,
        iat: now,
        exp: now + 30 * 86400
      },
      env.JWT_SECRET
    );

    return jsonResponse({
      ok: true,
      token,
      user: {
        id: user.id,
        callsign: user.callsign,
        email: user.email
      },
      profile
    }, 200);

  } catch (err) {
    console.error("Auth login error:", err);
    return jsonResponse({
      ok: false,
      error: "Authentication service error: " + (err.message || "Unknown error")
    }, 500);
  }
}
