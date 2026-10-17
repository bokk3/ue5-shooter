/*!
 * Noble Fermi — Wet Window + Sea of Pixels backdrop
 * ------------------------------------------------------------------
 * Layer stack (back -> front):
 *   1. Sea of pixels: a quantised grid of light nodes with slow ambient
 *      swells. Sporadic raindrops fall into it and spread ripple rings.
 *      Scrolling only nudges it very slightly.
 *   2. Wet window glass: misted pane with refractive water drops, fine
 *      mist beads, and heavy drops that slide down leaving wiped trails.
 *   3. The website content (regular DOM, rendered above this canvas).
 *
 * WebGL1, zero dependencies.
 * API: window.WetGlass.init(canvas, { reducedMotion }), .burst(),
 *      .setReducedMotion(bool), .config (live-tweakable values)
 */
(() => {
  'use strict';

  // ------------------------------------------------------------------
  // Tunables (safe to tweak; also exposed as window.WetGlass.config)
  // ------------------------------------------------------------------
  const CFG = {
    pitch: 12,              // css px between pixel nodes in the sea
    dotBase: 1.7,           // css px size of a resting pixel node
    dotGain: 3.0,           // extra css px on wave crests
    fog: 0.85,              // window mist strength (0 = perfectly clear glass)
    lensPx: 24,             // how far drops refract the scene behind (css px)
    rippleSpeed: 150,       // ripple ring speed in the sea (css px / s)
    rainEvery: [0.45, 1.8], // seconds between raindrops falling into the sea
    glassEvery: [0.7, 2.6], // seconds between drops landing on the glass
    mistPerSec: 18,         // fine mist beads condensing per second
    maxDrops: 42,           // resting/sliding drops on the glass
    maxTrail: 80,           // small beads left behind by sliding drops
    scrollParallax: 0.03,   // sea shift per scrolled px (very slight)
    pointerParallax: 9,     // css px of sea shift from pointer position
    driftPx: 6,             // autonomous slow drift so the scene always breathes
    maxDpr: 1.75,
  };

  const RAIN_SLOTS = 16;
  const WAKE_SLOTS = 10;

  const rand = (a, b) => a + Math.random() * (b - a);

  // ------------------------------------------------------------------
  // Shaders
  // ------------------------------------------------------------------
  const FP = '#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n';

  const VS_QUAD = `
attribute vec2 a_pos;
varying vec2 v_uv;
void main() {
  v_uv = a_pos * 0.5 + 0.5;
  gl_Position = vec4(a_pos, 0.0, 1.0);
}`;

  // Deep backdrop behind the pixel nodes (palette glows drift very slowly)
  const FS_BG = FP + `
varying vec2 v_uv;
uniform float u_time;
uniform vec2 u_aspect;
void main() {
  vec2 p = v_uv * u_aspect;
  vec3 c = vec3(0.008, 0.018, 0.040);
  vec2 cy = vec2(0.22 + 0.04 * sin(u_time * 0.05), 0.58) * u_aspect;
  vec2 am = vec2(0.82 + 0.04 * cos(u_time * 0.04), 0.40) * u_aspect;
  c += vec3(0.00, 0.30, 0.40) * exp(-dot(p - cy, p - cy) * 4.0) * 0.26;
  c += vec3(0.42, 0.24, 0.00) * exp(-dot(p - am, p - am) * 5.0) * 0.18;
  gl_FragColor = vec4(c, 1.0);
}`;

  // One GL point per pixel node; height field = ambient swell + ripples
  const VS_SEA = `
precision highp float;
attribute vec2 a_idx;
uniform vec2 u_view;
uniform vec2 u_cam;
uniform vec2 u_camBase;
uniform float u_pitch;
uniform float u_dpr;
uniform float u_time;
uniform float u_amb;
uniform float u_speed;
uniform float u_dotBase;
uniform float u_dotGain;
uniform vec4 u_rain[${RAIN_SLOTS}];
uniform vec4 u_wake[${WAKE_SLOTS}];
uniform vec3 u_mouse;
varying vec3 v_col;

float ambient(vec2 p, float t) {
  float h = 0.0;
  h += sin(dot(p, vec2(0.0090, 0.0034)) + t * 0.50) * 0.34;
  h += sin(dot(p, vec2(-0.0051, 0.0097)) + t * 0.37 + 1.7) * 0.27;
  h += sin(dot(p, vec2(0.0133, -0.0071)) + t * 0.66 + 4.1) * 0.15;
  h += sin(dot(p, vec2(0.0027, 0.0171)) - t * 0.31 + 2.3) * 0.12;
  return h;
}

void main() {
  vec2 world = (a_idx + u_camBase - 1.0) * u_pitch;
  float t = u_time;
  float h = ambient(world, t) * u_amb;
  vec2 disp = vec2(0.0);
  float flash = 0.0;

  // Raindrops falling into the sea: expanding ring trains
  for (int i = 0; i < ${RAIN_SLOTS}; i++) {
    vec4 r = u_rain[i];
    float age = t - r.z;
    if (age > 0.0 && age < 7.0) {
      vec2 dv = world - r.xy;
      float d = length(dv);
      float x = d - age * u_speed;
      float env = exp(-x * x * 0.00045);
      float fall = exp(-age * 0.62) / (1.0 + d * 0.0035);
      float w = sin(x * 0.19) * env * fall * r.w;
      h += w * 1.55;
      disp += (d > 0.001 ? dv / d : vec2(0.0)) * w * 2.2;
      flash += exp(-d * d * 0.004) * exp(-age * 9.0) * r.w;
    }
  }

  // Small, fast-fading wakes from pointer movement
  for (int i = 0; i < ${WAKE_SLOTS}; i++) {
    vec4 k = u_wake[i];
    float age = t - k.z;
    if (age > 0.0 && age < 2.5) {
      float d = length(world - k.xy);
      float x = d - age * u_speed * 0.8;
      h += sin(x * 0.24) * exp(-x * x * 0.0012) * exp(-age * 2.2) * k.w;
    }
  }

  // Soft lift of the surface under the pointer
  vec2 md = world - u_mouse.xy;
  h += exp(-dot(md, md) * 0.00009) * u_mouse.z * 0.55;

  vec2 screen = world - u_cam + disp;
  gl_Position = vec4(screen.x / u_view.x * 2.0 - 1.0, 1.0 - screen.y / u_view.y * 2.0, 0.0, 1.0);

  float e = clamp(h, -1.5, 2.5);
  float lum = 0.20 + 0.16 * e + 0.55 * max(e - 0.55, 0.0) + flash * 1.4;
  lum = clamp(lum, 0.04, 1.6);
  vec3 deep = vec3(0.10, 0.42, 0.62);
  vec3 cyan = vec3(0.00, 0.90, 1.00);
  vec3 amber = vec3(1.00, 0.70, 0.00);
  vec3 col = mix(deep, cyan, smoothstep(0.1, 0.9, e));
  col = mix(col, amber, clamp(smoothstep(1.05, 1.9, e) * 0.85 + flash * 0.4, 0.0, 1.0));
  float depth = mix(0.72, 1.0, smoothstep(0.0, 0.7, screen.y / u_view.y));
  v_col = col * lum * depth;

  float grow = clamp((e - 0.25) * 0.6, 0.0, 1.2) + min(flash, 1.0) * 0.8;
  gl_PointSize = max(1.0, (u_dotBase + u_dotGain * grow) * u_dpr);
}`;

  const FS_SEA = FP + `
varying vec3 v_col;
void main() {
  vec2 q = abs(gl_PointCoord - 0.5) * 2.0;
  float a = 1.0 - smoothstep(0.6, 1.0, max(q.x, q.y));
  gl_FragColor = vec4(v_col * a, 1.0);
}`;

  // 2x downsample (4 bilinear taps = 4x4 box)
  const FS_DOWN = FP + `
varying vec2 v_uv;
uniform sampler2D u_tex;
uniform vec2 u_texel;
void main() {
  vec3 c = texture2D(u_tex, v_uv + u_texel * vec2(-1.0, -1.0)).rgb;
  c += texture2D(u_tex, v_uv + u_texel * vec2( 1.0, -1.0)).rgb;
  c += texture2D(u_tex, v_uv + u_texel * vec2(-1.0,  1.0)).rgb;
  c += texture2D(u_tex, v_uv + u_texel * vec2( 1.0,  1.0)).rgb;
  gl_FragColor = vec4(c * 0.25, 1.0);
}`;

  // Separable 9-tap gaussian (5 linear taps)
  const FS_BLUR = FP + `
varying vec2 v_uv;
uniform sampler2D u_tex;
uniform vec2 u_dir;
void main() {
  vec3 c = texture2D(u_tex, v_uv).rgb * 0.2270270270;
  c += texture2D(u_tex, v_uv + u_dir * 1.3846153846).rgb * 0.3162162162;
  c += texture2D(u_tex, v_uv - u_dir * 1.3846153846).rgb * 0.3162162162;
  c += texture2D(u_tex, v_uv + u_dir * 3.2307692308).rgb * 0.0702702703;
  c += texture2D(u_tex, v_uv - u_dir * 3.2307692308).rgb * 0.0702702703;
  gl_FragColor = vec4(c, 1.0);
}`;

  // Final: look at the sea through the wet, misted window
  const FS_COMP = FP + `
varying vec2 v_uv;
uniform sampler2D u_sea;
uniform sampler2D u_fog;
uniform sampler2D u_water;
uniform sampler2D u_clear;
uniform vec2 u_lens;
uniform float u_fogAmt;

void main() {
  vec3 sharp = texture2D(u_sea, v_uv).rgb;
  vec3 fog = texture2D(u_fog, v_uv).rgb;
  float wiped = smoothstep(0.10, 0.75, texture2D(u_clear, v_uv).r);
  vec3 haze = vec3(0.016, 0.032, 0.050);

  // Misted pane: nodes soften and bloom into the condensation
  vec3 misted = sharp * (1.0 - 0.45 * u_fogAmt) + fog * (2.6 * u_fogAmt) + haze * u_fogAmt;
  // Where a drop slid through, the glass is wiped clean and crisp
  vec3 glass = mix(misted, sharp * 1.1 + haze * 0.3, wiped * 0.9);

  // Water on the glass (rg = surface normal, a = coverage)
  vec4 w = texture2D(u_water, v_uv);
  float cov = smoothstep(0.12, 0.8, w.a);
  vec2 n = (w.rg - 0.5) * 2.0;
  n.y = -n.y;
  float nz = sqrt(max(0.0, 1.0 - dot(n, n)));
  vec2 ruv = v_uv - n * u_lens;           // inverted lens image, like real drops
  vec3 drop = texture2D(u_sea, ruv).rgb * 1.45 + texture2D(u_fog, ruv).rgb * 1.2;
  drop += vec3(0.0, 0.030, 0.050);
  drop *= 1.0 - smoothstep(0.30, 0.92, 1.0 - nz) * 0.78;   // dark refractive rim
  vec3 N = normalize(vec3(n, nz + 0.2));
  float spec = pow(max(dot(N, normalize(vec3(-0.38, 0.52, 0.76))), 0.0), 60.0);
  float glint = pow(max(dot(N, normalize(vec3(0.30, -0.55, 0.78))), 0.0), 24.0);
  drop += vec3(0.90, 0.97, 1.00) * spec * 0.85 + vec3(0.0, 0.85, 1.0) * glint * 0.12;

  vec3 col = mix(glass, drop, cov);

  // Faint pane sheen + vignette
  col += vec3(0.55, 0.80, 1.00) * smoothstep(0.85, 0.0, distance(v_uv, vec2(0.12, 0.98))) * 0.04;
  vec2 q = v_uv - 0.5;
  col *= mix(0.55, 1.0, smoothstep(0.85, 0.2, length(q * vec2(1.15, 1.0))));
  gl_FragColor = vec4(col, 1.0);
}`;

  // ------------------------------------------------------------------
  // State
  // ------------------------------------------------------------------
  const S = {
    canvas: null, gl: null, ok: false,
    P: {}, quad: null, grid: null, gridCount: 0, T: null,
    waterTex: null, clearTex: null,
    W: 0, H: 0, dpr: 1, bw: 0, bh: 0,
    water: null, wctx: null, mist: null, mctx: null, clear: null, cctx: null,
    ws: 1, cs: 0.5, dropSprite: null, clearSprite: null,
    drops: [],
    rain: new Float32Array(RAIN_SLOTS * 4), rainIdx: 0,
    wake: new Float32Array(WAKE_SLOTS * 4), wakeIdx: 0,
    cam: [0, 0], ptr: [0, 0], ptrT: [0, 0],
    mouse: [-9999, -9999, 0], wakePos: [-9999, -9999], lastMoveT: 0, lastWake: 0,
    scrollY: 0, scrollVel: 0, amb: 1,
    t0: performance.now(), t: 0, lastT: 0,
    nextRain: 0.5, nextGlass: 0.4, mistAcc: 0, evapAcc: 0, fogAcc: 0,
    running: false, raf: 0, reduced: false, frame: 0, resizeTimer: 0,
  };

  // ------------------------------------------------------------------
  // GL helpers
  // ------------------------------------------------------------------
  function compile(gl, type, src) {
    const s = gl.createShader(type);
    gl.shaderSource(s, src);
    gl.compileShader(s);
    if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) {
      throw new Error('WetGlass shader: ' + gl.getShaderInfoLog(s));
    }
    return s;
  }

  function program(gl, vs, fs, attr) {
    const p = gl.createProgram();
    gl.attachShader(p, compile(gl, gl.VERTEX_SHADER, vs));
    gl.attachShader(p, compile(gl, gl.FRAGMENT_SHADER, fs));
    gl.bindAttribLocation(p, 0, attr);
    gl.linkProgram(p);
    if (!gl.getProgramParameter(p, gl.LINK_STATUS)) {
      throw new Error('WetGlass link: ' + gl.getProgramInfoLog(p));
    }
    const u = {};
    const n = gl.getProgramParameter(p, gl.ACTIVE_UNIFORMS);
    for (let i = 0; i < n; i++) {
      const info = gl.getActiveUniform(p, i);
      u[info.name.replace(/\[0\]$/, '')] = gl.getUniformLocation(p, info.name);
    }
    return { p, u };
  }

  function makeTex(gl, w, h) {
    const t = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, t);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    if (w && h) gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, w, h, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
    return t;
  }

  function makeTarget(gl, w, h) {
    const tex = makeTex(gl, w, h);
    const fb = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fb);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, tex, 0);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    return { fb, tex, w, h };
  }

  function buildGL() {
    const gl = S.gl;
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
    gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
    gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, gl.NONE);
    S.P.bg = program(gl, VS_QUAD, FS_BG, 'a_pos');
    S.P.sea = program(gl, VS_SEA, FS_SEA, 'a_idx');
    S.P.down = program(gl, VS_QUAD, FS_DOWN, 'a_pos');
    S.P.blur = program(gl, VS_QUAD, FS_BLUR, 'a_pos');
    S.P.comp = program(gl, VS_QUAD, FS_COMP, 'a_pos');
    S.quad = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, S.quad);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);
    S.grid = gl.createBuffer();
    S.waterTex = makeTex(gl, 0, 0);
    S.clearTex = makeTex(gl, 0, 0);
    S.T = null;
  }

  function bindQuad() {
    const gl = S.gl;
    gl.bindBuffer(gl.ARRAY_BUFFER, S.quad);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);
  }

  function pass(prog, srcTex, dst, uniforms) {
    const gl = S.gl;
    gl.bindFramebuffer(gl.FRAMEBUFFER, dst.fb);
    gl.viewport(0, 0, dst.w, dst.h);
    gl.useProgram(prog.p);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, srcTex);
    gl.uniform1i(prog.u.u_tex, 0);
    for (const k in uniforms) gl.uniform2fv(prog.u[k], uniforms[k]);
    bindQuad();
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
  }

  // ------------------------------------------------------------------
  // Glass (2D canvas) helpers
  // ------------------------------------------------------------------
  function makeSprites() {
    // Water drop normal map: rg = surface normal, a = coverage
    const N = 64;
    const c = document.createElement('canvas');
    c.width = c.height = N;
    const x = c.getContext('2d');
    const img = x.createImageData(N, N);
    const d = img.data;
    const R = N / 2;
    for (let j = 0; j < N; j++) {
      for (let i = 0; i < N; i++) {
        const dx = (i + 0.5 - R) / R;
        const dy = (j + 0.5 - R) / R;
        const r = Math.sqrt(dx * dx + dy * dy);
        const o = (j * N + i) * 4;
        if (r >= 1) { d[o + 3] = 0; continue; }
        d[o] = Math.round(128 + dx * 127);
        d[o + 1] = Math.round(128 + dy * 127);
        d[o + 2] = Math.round(Math.sqrt(1 - r * r) * 255);
        d[o + 3] = Math.round(Math.min(1, ((1 - r) * R) / 1.6) * 255);
      }
    }
    x.putImageData(img, 0, 0);
    S.dropSprite = c;

    // Soft brush used to wipe the mist off the glass
    const c2 = document.createElement('canvas');
    c2.width = c2.height = 64;
    const x2 = c2.getContext('2d');
    const g = x2.createRadialGradient(32, 32, 0, 32, 32, 32);
    g.addColorStop(0, 'rgba(255,255,255,1)');
    g.addColorStop(0.55, 'rgba(255,255,255,0.85)');
    g.addColorStop(1, 'rgba(255,255,255,0)');
    x2.fillStyle = g;
    x2.fillRect(0, 0, 64, 64);
    S.clearSprite = c2;
  }

  function sprinkleMist(n) {
    const m = S.mctx;
    const s = S.ws;
    for (let i = 0; i < n; i++) {
      const r = (0.9 + Math.pow(Math.random(), 2.5) * 2.4) * s;
      m.drawImage(S.dropSprite, rand(0, S.W) * s - r, rand(0, S.H) * s - r, r * 2, r * 2);
    }
  }

  function clearFog(x, y, r, a) {
    const c = S.cctx;
    const s = S.cs;
    c.globalAlpha = a;
    c.drawImage(S.clearSprite, (x - r) * s, (y - r) * s, r * 2 * s, r * 2 * s);
    c.globalAlpha = 1;
  }

  function eraseMist(x, y, r) {
    const m = S.mctx;
    const s = S.ws;
    m.save();
    m.globalCompositeOperation = 'destination-out';
    m.beginPath();
    m.arc(x * s, y * s, r * s, 0, Math.PI * 2);
    m.fill();
    m.restore();
  }

  function addDrop(x, y, r, impact) {
    // Landing on an existing drop merges into it
    for (const o of S.drops) {
      if (o.dead || o.trail) continue;
      const dx = o.x - x;
      const dy = o.y - y;
      const rr = (o.r + r) * 0.8;
      if (dx * dx + dy * dy < rr * rr) {
        o.r = Math.min(24, Math.sqrt(o.r * o.r + r * r));
        o.wob = 1;
        eraseMist(o.x, o.y, o.r * 1.05);
        clearFog(o.x, o.y, o.r * 1.3, 0.85);
        return o;
      }
    }
    let live = 0;
    for (const o of S.drops) if (!o.trail && !o.dead) live++;
    if (live >= CFG.maxDrops) {
      let s = null;
      for (const o of S.drops) {
        if (!o.trail && !o.dead && !o.sliding && (!s || o.r < s.r)) s = o;
      }
      if (!s) return null;
      s.dead = true;
    }
    const d = {
      x, y, r, vx: 0, vy: 0,
      sliding: false, trail: false, dead: false, parent: null,
      wob: impact ? 1 : 0, trailDist: 0, nextTrail: rand(6, 16),
    };
    S.drops.push(d);
    if (impact) {
      eraseMist(x, y, r * 1.05);
      clearFog(x, y, r * 1.3, 0.85);
    }
    return d;
  }

  function addTrail(parent) {
    S.drops.push({
      x: parent.x + rand(-0.2, 0.2) * parent.r,
      y: parent.y - parent.r * 1.25,
      r: parent.r * rand(0.18, 0.34),
      vx: 0, vy: 0, sliding: false, trail: true, dead: false, parent,
      wob: 0, trailDist: 0, nextTrail: 0,
    });
  }

  function seed() {
    S.drops.length = 0;
    const W = S.W;
    const H = S.H;
    const n = Math.round((W * H) / 70000) + 6;
    for (let i = 0; i < n; i++) {
      addDrop(rand(0, W), rand(0, H), 3 + Math.pow(Math.random(), 2.2) * 15, false);
    }
    for (let i = 0; i < 2; i++) {
      const d = addDrop(rand(W * 0.15, W * 0.85), rand(0, H * 0.4), rand(13, 17), false);
      if (d) { d.sliding = true; d.vy = 40; }
    }
  }

  // ------------------------------------------------------------------
  // Sea events
  // ------------------------------------------------------------------
  function rainAt(sx, sy, strength, t0) {
    const o = S.rainIdx * 4;
    S.rain[o] = sx + S.cam[0];
    S.rain[o + 1] = sy + S.cam[1];
    S.rain[o + 2] = t0;
    S.rain[o + 3] = strength;
    S.rainIdx = (S.rainIdx + 1) % RAIN_SLOTS;
  }

  function wakeAt(sx, sy, strength, t0) {
    const o = S.wakeIdx * 4;
    S.wake[o] = sx + S.cam[0];
    S.wake[o + 1] = sy + S.cam[1];
    S.wake[o + 2] = t0;
    S.wake[o + 3] = strength;
    S.wakeIdx = (S.wakeIdx + 1) % WAKE_SLOTS;
  }

  // ------------------------------------------------------------------
  // Resize
  // ------------------------------------------------------------------
  function resize(force) {
    const W = Math.max(1, window.innerWidth);
    const H = Math.max(1, window.innerHeight);
    // Ignore mobile URL-bar height jitter
    if (!force && Math.abs(W - S.W) < 2 && Math.abs(H - S.H) < 140) return false;
    const gl = S.gl;
    const oldW = S.W;
    const oldH = S.H;
    S.W = W;
    S.H = H;
    S.dpr = Math.min(window.devicePixelRatio || 1, CFG.maxDpr);
    S.bw = Math.max(1, Math.round(W * S.dpr));
    S.bh = Math.max(1, Math.round(H * S.dpr));
    S.canvas.width = S.bw;
    S.canvas.height = S.bh;

    // Pixel node grid (+3 margin so parallax never shows an edge)
    const cols = Math.ceil(W / CFG.pitch) + 3;
    const rows = Math.ceil(H / CFG.pitch) + 3;
    const arr = new Float32Array(cols * rows * 2);
    let k = 0;
    for (let j = 0; j < rows; j++) {
      for (let i = 0; i < cols; i++) { arr[k++] = i; arr[k++] = j; }
    }
    gl.bindBuffer(gl.ARRAY_BUFFER, S.grid);
    gl.bufferData(gl.ARRAY_BUFFER, arr, gl.STATIC_DRAW);
    S.gridCount = cols * rows;

    // Render targets
    if (S.T) {
      for (const key in S.T) { gl.deleteFramebuffer(S.T[key].fb); gl.deleteTexture(S.T[key].tex); }
    }
    const hw = Math.max(1, Math.ceil(S.bw / 2));
    const hh = Math.max(1, Math.ceil(S.bh / 2));
    const qw = Math.max(1, Math.ceil(S.bw / 4));
    const qh = Math.max(1, Math.ceil(S.bh / 4));
    S.T = {
      sea: makeTarget(gl, S.bw, S.bh),
      half: makeTarget(gl, hw, hh),
      qa: makeTarget(gl, qw, qh),
      qb: makeTarget(gl, qw, qh),
    };

    // Glass canvases
    if (!S.water) {
      S.water = document.createElement('canvas');
      S.mist = document.createElement('canvas');
      S.clear = document.createElement('canvas');
    }
    S.ws = Math.min(1, 1280 / W);
    S.cs = S.ws * 0.5;
    S.water.width = S.mist.width = Math.max(1, Math.round(W * S.ws));
    S.water.height = S.mist.height = Math.max(1, Math.round(H * S.ws));
    S.clear.width = Math.max(1, Math.round(W * S.cs));
    S.clear.height = Math.max(1, Math.round(H * S.cs));
    S.wctx = S.water.getContext('2d');
    S.mctx = S.mist.getContext('2d');
    S.cctx = S.clear.getContext('2d');
    S.cctx.fillStyle = '#000';
    S.cctx.fillRect(0, 0, S.clear.width, S.clear.height);

    if (oldW && oldH) {
      for (const d of S.drops) { d.x *= W / oldW; d.y *= H / oldH; }
    }
    sprinkleMist(Math.round((W * H) / 3500));
    return true;
  }

  // ------------------------------------------------------------------
  // Simulation
  // ------------------------------------------------------------------
  function update(dt, t) {
    const W = S.W;
    const H = S.H;

    // Scrolling barely disturbs the sea: tiny shift + tiny extra swell
    const sy = window.scrollY || window.pageYOffset || 0;
    const v = (sy - S.scrollY) / Math.max(dt, 1e-3);
    S.scrollY = sy;
    S.scrollVel += (v - S.scrollVel) * Math.min(1, dt * 6);
    const agit = Math.min(0.18, Math.abs(S.scrollVel) * 0.00012);
    S.amb += (1 + agit - S.amb) * Math.min(1, dt * 2);

    // Always-on gentle parallax: slow drift + eased pointer offset
    S.ptr[0] += (S.ptrT[0] - S.ptr[0]) * Math.min(1, dt * 2.5);
    S.ptr[1] += (S.ptrT[1] - S.ptr[1]) * Math.min(1, dt * 2.5);
    S.cam[0] = Math.sin(t * 0.11) * CFG.driftPx + Math.cos(t * 0.067) * CFG.driftPx * 0.6
      + S.ptr[0] * CFG.pointerParallax;
    S.cam[1] = Math.cos(t * 0.09) * CFG.driftPx * 0.8 + S.ptr[1] * CFG.pointerParallax * 0.7
      + sy * CFG.scrollParallax;

    S.mouse[2] *= Math.exp(-dt * 1.4);

    // Sporadic raindrops falling into the sea (sometimes a little cluster)
    if (t >= S.nextRain) {
      const cluster = Math.random() < 0.22 ? 2 + ((Math.random() * 2) | 0) : 1;
      const bx = rand(0.05, 0.95) * W;
      const by = rand(0.05, 0.95) * H;
      for (let i = 0; i < cluster; i++) {
        const x = i ? bx + rand(-160, 160) : bx;
        const y = i ? by + rand(-110, 110) : by;
        rainAt(x, y, rand(0.65, 1.25), t + i * rand(0.05, 0.22));
      }
      S.nextRain = t + rand(CFG.rainEvery[0], CFG.rainEvery[1]);
    }

    // Drops landing on the window
    if (t >= S.nextGlass) {
      addDrop(rand(0, W), rand(0, H * 0.92), 3.5 + Math.pow(Math.random(), 2.0) * 14, true);
      S.nextGlass = t + rand(CFG.glassEvery[0], CFG.glassEvery[1]);
    }

    // Fine mist condenses, and slowly evaporates
    S.mistAcc += dt * CFG.mistPerSec;
    if (S.mistAcc >= 1) {
      const n = S.mistAcc | 0;
      S.mistAcc -= n;
      sprinkleMist(n);
    }
    S.evapAcc += dt;
    if (S.evapAcc > 0.5) {
      S.evapAcc = 0;
      const m = S.mctx;
      m.save();
      m.globalCompositeOperation = 'destination-out';
      m.fillStyle = 'rgba(0,0,0,0.05)';
      m.fillRect(0, 0, S.mist.width, S.mist.height);
      m.restore();
    }
    // Wiped trails slowly fog up again
    S.fogAcc += dt;
    if (S.fogAcc > 0.15) {
      S.fogAcc = 0;
      S.cctx.fillStyle = 'rgba(0,0,0,0.03)';
      S.cctx.fillRect(0, 0, S.clear.width, S.clear.height);
    }

    // Drops on the glass
    const drops = S.drops;
    let trailCount = 0;
    for (const d of drops) if (d.trail && !d.dead) trailCount++;

    for (let i = 0; i < drops.length; i++) {
      const d = drops[i];
      if (d.dead) continue;
      if (d.wob > 0) d.wob = Math.max(0, d.wob - dt * 3.5);

      if (d.trail) {
        d.r -= dt * 0.1;
        if (d.r < 0.9) d.dead = true;
        continue;
      }

      // Heavy drops eventually break loose
      if (!d.sliding && d.r > 9 && Math.random() < dt * (d.r - 9) * 0.07) d.sliding = true;
      if (!d.sliding) continue;

      // Stick-slip descent with a slight meander
      const target = (d.r - 5) * 9;
      d.vy += (target - d.vy) * Math.min(1, dt * 2.2);
      if (Math.random() < dt * 0.4) d.vy *= 0.2;
      d.vx += (Math.random() - 0.5) * 70 * dt;
      d.vx *= Math.exp(-dt * 2.5);
      const step = Math.max(0, d.vy * dt);
      d.x += d.vx * dt;
      d.y += step;
      d.r = Math.max(4.5, d.r - step * 0.0035);
      if (d.r < 7.5 && d.vy < 8) d.sliding = false;

      // Leave small beads behind
      d.trailDist += step;
      if (d.trailDist > d.nextTrail) {
        d.trailDist = 0;
        d.nextTrail = rand(7, 20);
        if (trailCount < CFG.maxTrail) { addTrail(d); trailCount++; }
      }

      // Wipe the mist along the path
      eraseMist(d.x, d.y, d.r * 0.95);
      clearFog(d.x, d.y - d.r * 0.2, d.r * 1.15, 0.55);

      // Swallow anything in the way
      for (let j = 0; j < drops.length; j++) {
        const o = drops[j];
        if (o === d || o.dead || o.parent === d) continue;
        const dx = o.x - d.x;
        const dy = o.y - d.y;
        const rr = (o.r + d.r) * 0.72;
        if (dx * dx + dy * dy < rr * rr) {
          d.r = Math.min(24, Math.sqrt(d.r * d.r + o.r * o.r * 0.9));
          d.vy += 12;
          d.wob = Math.max(d.wob, 0.5);
          o.dead = true;
        }
      }

      if (d.y - d.r > H + 20) d.dead = true;
    }
    S.drops = drops.filter((d) => !d.dead);
  }

  function drawWater() {
    const w = S.wctx;
    const s = S.ws;
    w.clearRect(0, 0, S.water.width, S.water.height);
    w.drawImage(S.mist, 0, 0);
    for (const d of S.drops) {
      let sx = 1;
      let sy = 1;
      if (d.sliding) {
        const k = Math.min(0.3, d.vy / 280);
        sy += k;
        sx -= k * 0.4;
      }
      if (d.wob > 0) {
        const k = Math.sin((1 - d.wob) * 22) * d.wob * 0.16;
        sx += k;
        sy -= k;
      }
      const rx = d.r * sx * s;
      const ry = d.r * sy * s;
      w.drawImage(S.dropSprite, d.x * s - rx, d.y * s - ry, rx * 2, ry * 2);
    }
  }

  // ------------------------------------------------------------------
  // Render
  // ------------------------------------------------------------------
  function render(t, uploadAll) {
    const gl = S.gl;
    const P = S.P;
    const T = S.T;

    // Upload glass maps
    gl.activeTexture(gl.TEXTURE2);
    gl.bindTexture(gl.TEXTURE_2D, S.waterTex);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, S.water);
    gl.activeTexture(gl.TEXTURE3);
    gl.bindTexture(gl.TEXTURE_2D, S.clearTex);
    if (uploadAll || (S.frame & 1) === 0) {
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, S.clear);
    }

    // 1. Sea of pixels -> T.sea
    gl.bindFramebuffer(gl.FRAMEBUFFER, T.sea.fb);
    gl.viewport(0, 0, T.sea.w, T.sea.h);
    gl.disable(gl.BLEND);
    gl.useProgram(P.bg.p);
    gl.uniform1f(P.bg.u.u_time, t);
    gl.uniform2f(P.bg.u.u_aspect, S.W / S.H, 1);
    bindQuad();
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

    gl.enable(gl.BLEND);
    gl.blendFunc(gl.ONE, gl.ONE);
    gl.useProgram(P.sea.p);
    const u = P.sea.u;
    gl.uniform2f(u.u_view, S.W, S.H);
    gl.uniform2f(u.u_cam, S.cam[0], S.cam[1]);
    gl.uniform2f(u.u_camBase, Math.floor(S.cam[0] / CFG.pitch), Math.floor(S.cam[1] / CFG.pitch));
    gl.uniform1f(u.u_pitch, CFG.pitch);
    gl.uniform1f(u.u_dpr, S.dpr);
    gl.uniform1f(u.u_time, t);
    gl.uniform1f(u.u_amb, S.amb);
    gl.uniform1f(u.u_speed, CFG.rippleSpeed);
    gl.uniform1f(u.u_dotBase, CFG.dotBase);
    gl.uniform1f(u.u_dotGain, CFG.dotGain);
    gl.uniform4fv(u.u_rain, S.rain);
    gl.uniform4fv(u.u_wake, S.wake);
    gl.uniform3f(u.u_mouse, S.mouse[0] + S.cam[0], S.mouse[1] + S.cam[1], S.mouse[2]);
    gl.bindBuffer(gl.ARRAY_BUFFER, S.grid);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);
    gl.drawArrays(gl.POINTS, 0, S.gridCount);
    gl.disable(gl.BLEND);

    // 2. Blurred copy = what the mist does to the view
    pass(P.down, T.sea.tex, T.half, { u_texel: [1 / T.sea.w, 1 / T.sea.h] });
    pass(P.down, T.half.tex, T.qa, { u_texel: [1 / T.half.w, 1 / T.half.h] });
    pass(P.blur, T.qa.tex, T.qb, { u_dir: [1 / T.qa.w, 0] });
    pass(P.blur, T.qb.tex, T.qa, { u_dir: [0, 1 / T.qa.h] });
    pass(P.blur, T.qa.tex, T.qb, { u_dir: [1.8 / T.qa.w, 0] });
    pass(P.blur, T.qb.tex, T.qa, { u_dir: [0, 1.8 / T.qa.h] });

    // 3. Composite through the wet window
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.viewport(0, 0, S.bw, S.bh);
    gl.useProgram(P.comp.p);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, T.sea.tex);
    gl.activeTexture(gl.TEXTURE1);
    gl.bindTexture(gl.TEXTURE_2D, T.qa.tex);
    gl.activeTexture(gl.TEXTURE2);
    gl.bindTexture(gl.TEXTURE_2D, S.waterTex);
    gl.activeTexture(gl.TEXTURE3);
    gl.bindTexture(gl.TEXTURE_2D, S.clearTex);
    const c = P.comp.u;
    gl.uniform1i(c.u_sea, 0);
    gl.uniform1i(c.u_fog, 1);
    gl.uniform1i(c.u_water, 2);
    gl.uniform1i(c.u_clear, 3);
    gl.uniform2f(c.u_lens, CFG.lensPx / S.W, CFG.lensPx / S.H);
    gl.uniform1f(c.u_fogAmt, CFG.fog);
    bindQuad();
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
  }

  function renderStatic() {
    if (!S.ok) return;
    const t = 8.0;
    S.t = t;
    S.cam[0] = 0;
    S.cam[1] = (window.scrollY || 0) * CFG.scrollParallax;
    drawWater();
    render(t, true);
  }

  // ------------------------------------------------------------------
  // Loop
  // ------------------------------------------------------------------
  function loop(now) {
    if (!S.running) return;
    S.raf = requestAnimationFrame(loop);
    const t = (now - S.t0) / 1000;
    const dt = Math.min(0.05, Math.max(0, t - S.lastT));
    S.lastT = t;
    S.t = t;
    update(dt, t);
    drawWater();
    render(t, false);
    S.frame++;
  }

  function start() {
    if (S.running || !S.ok || document.hidden) return;
    S.running = true;
    S.lastT = (performance.now() - S.t0) / 1000;
    S.raf = requestAnimationFrame(loop);
  }

  function stop() {
    S.running = false;
    if (S.raf) cancelAnimationFrame(S.raf);
    S.raf = 0;
  }

  // ------------------------------------------------------------------
  // Input
  // ------------------------------------------------------------------
  function onMove(e) {
    const x = e.clientX;
    const y = e.clientY;
    S.ptrT[0] = (x / S.W - 0.5) * 2;
    S.ptrT[1] = (y / S.H - 0.5) * 2;
    if (S.reduced) return;
    const now = S.t;
    const dx = x - S.mouse[0];
    const dy = y - S.mouse[1];
    const dtm = Math.max(0.008, now - S.lastMoveT);
    const speed = Math.min(4000, Math.hypot(dx, dy) / dtm);
    S.mouse[0] = x;
    S.mouse[1] = y;
    S.lastMoveT = now;
    S.mouse[2] = Math.min(1, S.mouse[2] + Math.min(0.25, speed * 0.00012));
    const wd = Math.hypot(x - S.wakePos[0], y - S.wakePos[1]);
    if (now - S.lastWake > 0.07 && wd > 24) {
      wakeAt(x, y, Math.min(0.55, 0.12 + speed * 0.00018), now);
      S.lastWake = now;
      S.wakePos[0] = x;
      S.wakePos[1] = y;
    }
  }

  function onDown(e) {
    if (S.reduced || !S.ok) return;
    rainAt(e.clientX, e.clientY, 1.5, S.t);
  }

  function onResize() {
    clearTimeout(S.resizeTimer);
    S.resizeTimer = setTimeout(() => {
      if (!S.ok) return;
      if (resize(false) && S.reduced) renderStatic();
    }, 140);
  }

  // ------------------------------------------------------------------
  // Public API
  // ------------------------------------------------------------------
  function init(canvas, opts) {
    if (S.canvas || !canvas) return S.ok;
    S.canvas = canvas;
    S.reduced = !!(opts && opts.reducedMotion);
    let gl = null;
    try {
      gl = canvas.getContext('webgl', {
        alpha: false, antialias: false, depth: false, stencil: false,
        premultipliedAlpha: false, preserveDrawingBuffer: false,
        powerPreference: 'high-performance',
      });
    } catch (err) {
      gl = null;
    }
    if (!gl) return false; // CSS gradient on the canvas stays as a fallback

    S.gl = gl;
    try {
      buildGL();
    } catch (err) {
      console.warn(err);
      return false;
    }
    for (let i = 0; i < RAIN_SLOTS; i++) S.rain[i * 4 + 2] = -1000;
    for (let i = 0; i < WAKE_SLOTS; i++) S.wake[i * 4 + 2] = -1000;
    S.ok = true;
    makeSprites();
    resize(true);
    seed();
    S.scrollY = window.scrollY || 0;

    window.addEventListener('pointermove', onMove, { passive: true });
    window.addEventListener('pointerdown', onDown, { passive: true });
    window.addEventListener('resize', onResize, { passive: true });
    document.addEventListener('visibilitychange', () => {
      if (document.hidden) stop();
      else if (!S.reduced) start();
    });
    canvas.addEventListener('webglcontextlost', (e) => {
      e.preventDefault();
      stop();
      S.ok = false;
    }, false);
    canvas.addEventListener('webglcontextrestored', () => {
      try {
        buildGL();
        S.ok = true;
        resize(true);
        if (S.reduced) renderStatic(); else start();
      } catch (err) {
        console.warn(err);
      }
    }, false);

    if (S.reduced) renderStatic(); else start();
    return true;
  }

  function burst() {
    if (!S.ok) return;
    const t = S.t;
    for (let i = 0; i < 5; i++) {
      rainAt(rand(0.12, 0.88) * S.W, rand(0.15, 0.85) * S.H, rand(0.9, 1.4), t + i * 0.09);
    }
    for (let i = 0; i < 4; i++) {
      addDrop(rand(0.1, 0.9) * S.W, rand(0.05, 0.8) * S.H, rand(7, 15), true);
    }
    if (S.reduced) renderStatic();
  }

  function setReducedMotion(on) {
    S.reduced = !!on;
    if (!S.ok) return;
    if (S.reduced) { stop(); renderStatic(); } else start();
  }

  window.WetGlass = { init, burst, setReducedMotion, config: CFG };
})();
