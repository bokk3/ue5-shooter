// =============================================================================
// NOBLE FERMI // OPERATOR ENLISTMENT (REGISTRATION) ENDPOINT
// POST /api/auth/register
// =============================================================================

import {
  jsonResponse,
  handleOptions,
  generateSalt,
  hashPassword,
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

    const rawCallsign = (body.callsign || "").trim();
    const rawEmail = (body.email || "").trim().toLowerCase();
    const password = body.password || "";

    // 1. Validation
    if (!rawCallsign || rawCallsign.length < 3 || rawCallsign.length > 20) {
      return jsonResponse({
        ok: false,
        error: "Callsign must be between 3 and 20 alphanumeric/underscore characters."
      }, 400);
    }

    if (!/^[a-zA-Z0-9_]{3,20}$/.test(rawCallsign)) {
      return jsonResponse({
        ok: false,
        error: "Callsign may only contain letters, numbers, and underscores."
      }, 400);
    }

    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    if (!rawEmail || !emailRegex.test(rawEmail)) {
      return jsonResponse({
        ok: false,
        error: "A valid tactical email address is required."
      }, 400);
    }

    if (!password || password.length < 6) {
      return jsonResponse({
        ok: false,
        error: "Passkey must be at least 6 characters in length."
      }, 400);
    }

    // 2. Uniqueness check in D1
    const existing = await env.DB.prepare(
      "SELECT id, callsign, email FROM users WHERE LOWER(callsign) = LOWER(?) OR LOWER(email) = LOWER(?) LIMIT 1"
    ).bind(rawCallsign, rawEmail).first();

    if (existing) {
      if (existing.callsign && existing.callsign.toLowerCase() === rawCallsign.toLowerCase()) {
        return jsonResponse({
          ok: false,
          error: `Callsign '${rawCallsign}' is already claimed by an active operator.`
        }, 409);
      }
      return jsonResponse({
        ok: false,
        error: `Tactical email '${rawEmail}' is already enlisted.`
      }, 409);
    }

    // 3. Cryptographic salt & PBKDF2 hash (100,000 iterations)
    const salt = generateSalt();
    const passwordHash = await hashPassword(password, salt);

    const userId = crypto.randomUUID();
    const now = Math.floor(Date.now() / 1000);

    const defaultUnlockedJson = JSON.stringify([
      "camo_digital",
      "headgear_fast",
      "vest_level4",
      "weapon_steel",
      "Camo_0",
      "Headgear_0",
      "Armor_0",
      "WeaponFinish_0"
    ]);

    const defaultEquippedJson = JSON.stringify({
      camo: 0,
      headgear: 0,
      armor: 0,
      weaponFinish: 0,
      theater: 0
    });

    // 4. Batch insert into D1
    await env.DB.batch([
      env.DB.prepare(
        "INSERT INTO users (id, callsign, email, password_hash, password_salt, created_at, last_login_at) VALUES (?, ?, ?, ?, ?, ?, ?)"
      ).bind(userId, rawCallsign, rawEmail, passwordHash, salt, now, now),
      env.DB.prepare(
        "INSERT INTO profiles (user_id, gold_balance, opal_balance, unlocked_items_json, equipped_loadout_json, login_streak, updated_at) VALUES (?, 1500, 50, ?, ?, 1, ?)"
      ).bind(userId, defaultUnlockedJson, defaultEquippedJson, now)
    ]);

    // 5. Generate 30-day JWT
    const token = await signJwt(
      {
        sub: userId,
        callsign: rawCallsign,
        email: rawEmail,
        iat: now,
        exp: now + 30 * 86400
      },
      env.JWT_SECRET
    );

    const userObj = {
      id: userId,
      callsign: rawCallsign,
      email: rawEmail
    };

    const profileObj = {
      user_id: userId,
      gold_balance: 1500,
      opal_balance: 50,
      unlocked_items_json: defaultUnlockedJson,
      equipped_loadout_json: defaultEquippedJson,
      login_streak: 1,
      updated_at: now
    };

    return jsonResponse({
      ok: true,
      token,
      user: userObj,
      profile: profileObj
    }, 201);

  } catch (err) {
    console.error("Auth register error:", err);
    return jsonResponse({
      ok: false,
      error: "Internal operator enlistment failure: " + (err.message || "Unknown error")
    }, 500);
  }
}
