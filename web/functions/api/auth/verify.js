// =============================================================================
// NOBLE FERMI // 6-DIGIT WEBSITE VERIFICATION CODE ENDPOINT
// POST /api/auth/verify
// Verifies the 6-digit code displayed on the website Operator Dossier to unlock
// full Client <-> Website deployment and cloud profile sync.
// =============================================================================

import {
  jsonResponse,
  handleOptions,
  getBearerToken,
  verifyJwt,
  signJwt
} from "../utils.js";
import { handleAuthMe } from "./me.js";

export async function onRequestOptions() {
  return handleOptions();
}

export async function onRequestGet(context) {
  return handleAuthMe(context);
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

    let body = {};
    try {
      const text = await request.text();
      if (text && text.trim().length > 0) {
        body = JSON.parse(text);
      }
    } catch (e) {
      body = {};
    }

    const code = String(body.code || body.verification_code || "").trim();
    const identifier = String(body.identifier || body.callsign || body.email || "").trim();

    // If no verification code was submitted, fall back to token telemetry (/api/auth/me)
    if (!code) {
      return handleAuthMe(context);
    }

    let user = null;
    const token = getBearerToken(request);
    if (token) {
      const payload = await verifyJwt(token, env.JWT_SECRET);
      if (payload && payload.sub) {
        user = await env.DB.prepare(
          "SELECT id, callsign, email, is_verified, verification_code, created_at, last_login_at FROM users WHERE id = ? LIMIT 1"
        ).bind(payload.sub).first();
      }
    }

    if (!user && identifier) {
      user = await env.DB.prepare(
        "SELECT id, callsign, email, is_verified, verification_code, created_at, last_login_at FROM users WHERE LOWER(callsign) = LOWER(?) OR LOWER(email) = LOWER(?) LIMIT 1"
      ).bind(identifier, identifier).first();
    }

    if (!user) {
      return jsonResponse({
        ok: false,
        error: "Operator account not found for verification."
      }, 404);
    }

    const expectedCode = String(user.verification_code || "").trim();
    if (!expectedCode || code.toUpperCase() !== expectedCode.toUpperCase()) {
      return jsonResponse({
        ok: false,
        error: "Invalid 6-digit verification code. Open the website Operator Dossier to view your code."
      }, 400);
    }

    const now = Math.floor(Date.now() / 1000);
    await env.DB.prepare(
      "UPDATE users SET is_verified = 1, last_login_at = ? WHERE id = ?"
    ).bind(now, user.id).run();

    let profile = await env.DB.prepare(
      "SELECT user_id, gold_balance, opal_balance, unlocked_items_json, equipped_loadout_json, login_streak, updated_at FROM profiles WHERE user_id = ? LIMIT 1"
    ).bind(user.id).first();

    const freshToken = await signJwt(
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
      verified: true,
      requires_verification: false,
      token: freshToken,
      user: {
        id: user.id,
        callsign: user.callsign,
        email: user.email,
        is_verified: true,
        verification_code: expectedCode
      },
      profile
    }, 200);

  } catch (err) {
    console.error("Auth verify error:", err);
    return jsonResponse({
      ok: false,
      error: "Verification error: " + (err.message || "Unknown error")
    }, 500);
  }
}
