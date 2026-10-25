// =============================================================================
// NOBLE FERMI // OPERATOR VERIFICATION & PROFILE TELEMETRY ENDPOINT
// GET /api/auth/me & POST /api/auth/me
// Verifies JWT token and delivers real-time operator profile and economy balances
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

export async function onRequestGet(context) {
  return handleAuthMe(context);
}

export async function onRequestPost(context) {
  return handleAuthMe(context);
}

export async function handleAuthMe(context) {
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

    // Look up user
    const user = await env.DB.prepare(
      "SELECT id, callsign, email, is_verified, verification_code, created_at, last_login_at FROM users WHERE id = ? LIMIT 1"
    ).bind(userId).first();

    if (!user) {
      return jsonResponse({
        ok: false,
        error: "Operator account not found."
      }, 404);
    }

    let verificationCode = user.verification_code;
    if (!verificationCode || verificationCode.length < 4) {
      verificationCode = String(Math.floor(100000 + Math.random() * 900000));
      await env.DB.prepare(
        "UPDATE users SET verification_code = ? WHERE id = ?"
      ).bind(verificationCode, user.id).run();
    }

    // Look up profile
    let profile = await env.DB.prepare(
      "SELECT user_id, gold_balance, opal_balance, unlocked_items_json, equipped_loadout_json, login_streak, updated_at FROM profiles WHERE user_id = ? LIMIT 1"
    ).bind(userId).first();

    if (!profile) {
      const defaultUnlockedJson = JSON.stringify([
        "camo_digital", "headgear_fast", "vest_level4", "weapon_steel",
        "Camo_0", "Headgear_0", "Armor_0", "WeaponFinish_0"
      ]);
      const defaultEquippedJson = JSON.stringify({
        camo: 0, headgear: 0, armor: 0, weaponFinish: 0, theater: 0
      });
      const now = Math.floor(Date.now() / 1000);

      await env.DB.prepare(
        "INSERT INTO profiles (user_id, gold_balance, opal_balance, unlocked_items_json, equipped_loadout_json, login_streak, updated_at) VALUES (?, 1500, 50, ?, ?, 1, ?)"
      ).bind(userId, defaultUnlockedJson, defaultEquippedJson, now).run();

      profile = {
        user_id: userId,
        gold_balance: 1500,
        opal_balance: 50,
        unlocked_items_json: defaultUnlockedJson,
        equipped_loadout_json: defaultEquippedJson,
        login_streak: 1,
        updated_at: now
      };
    }

    const isVerified = Boolean(user.is_verified);

    return jsonResponse({
      ok: true,
      requires_verification: !isVerified,
      user: {
        id: user.id,
        callsign: user.callsign,
        email: user.email,
        is_verified: isVerified,
        verification_code: verificationCode,
        created_at: user.created_at,
        last_login_at: user.last_login_at
      },
      profile
    }, 200);

  } catch (err) {
    console.error("Auth me error:", err);
    return jsonResponse({
      ok: false,
      error: "Failed to verify operator token: " + (err.message || "Unknown error")
    }, 500);
  }
}
