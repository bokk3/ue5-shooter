"""Drive the Asset Studio server: submit prompts, retry failures and download finished models.

usage:
  python -I studio.py status
  python -I studio.py submit (--missing | --asset NAME ... | --prompt "Name=text" ...) [options] [--dry-run]
  python -I studio.py retry [--dry-run]
  python -I studio.py cancel (--asset NAME ... | JOBID ...) [--dry-run]
  python -I studio.py fetch [--wait] [--max-mb 1500] [--asset NAME ...] [--all] [--dry-run]

Sign-in: the access token in NF_STUDIO_TOKEN, or in the file NF_STUDIO_TOKEN_FILE names (default ~/.nf_studio_token).
Server: NF_STUDIO_URL (default https://win-main.tail6948f.ts.net).

Download size: a model's size is mostly its textures (Ultra, 30k faces with 4K textures, measured 18-50 MB).
`submit` defaults to Ultra for the cleanest meshes; --quality High (20k faces, 2K textures) is several times smaller.
The server accepts only the four UI presets, not other face/texture pairs. `fetch` downloads only models it does not have yet (every id in
_pack_manifests counts as had), checks the sizes first, stops at --max-mb and resumes broken downloads.

`fetch` writes a pack folder to SourceArt/Trellis/_downloads/<name> plus its manifest to _pack_manifests/; sort it with
  python -I sort_packs.py ..\\..\\SourceArt\\Trellis --merge ..\\..\\SourceArt\\Trellis\\_downloads\\<name>
"""
import argparse
import datetime
import glob
import http.client
import http.cookiejar
import json
import os
import re
import shutil
import struct
import sys
import time
import urllib.error
import urllib.request
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from prompts import PROMPTS  # noqa: E402

ROOT = os.path.normpath(os.environ.get("NF_STUDIO_ROOT") or os.path.join(HERE, "..", "..", "SourceArt", "Trellis"))
PACK_MANIFESTS = os.path.join(ROOT, "_pack_manifests")
DOWNLOADS = os.path.join(ROOT, "_downloads")
SUBMISSIONS = os.path.join(ROOT, "_studio_submissions.json")
DEFAULT_URL = "https://win-main.tail6948f.ts.net"

SUFFIX = ", realistic game asset, PBR, single object"
# The server accepts only these (faces, texture) pairs. Ultra (measured 18-50 MB per model) gives the cleanest
# meshes; High (20k faces, 2K textures) is far smaller and enough for the 2K the UE import keeps.
QUALITY = {"Fast": (5000, 1024), "Standard": (10000, 2048), "High": (20000, 2048), "Ultra": (30000, 4096)}
QUALITY_DEFAULT = "Ultra"
EST_MB = {1024: 4, 2048: 10, 4096: 32}  # typical GLB size (4K measured at 18-50 MB), used when the server gives no size
BUSY = ("pending", "submitted")
CHUNK = 1 << 20


class StudioError(Exception):
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status


def slug(text, n=40):
    return re.sub(r"[^a-z0-9]+", "-", text.lower())[:n].strip("-")


def norm(text):
    return re.sub(r"[^a-z0-9]+", " ", text.lower()).strip()


def load_json(path, default):
    if not os.path.isfile(path):
        return default
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def save_json(path, data):
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=1)
    os.replace(tmp, path)


def read_token():
    token = os.environ.get("NF_STUDIO_TOKEN", "").strip()
    if token:
        return token
    path = os.environ.get("NF_STUDIO_TOKEN_FILE") or os.path.join(os.path.expanduser("~"), ".nf_studio_token")
    if os.path.isfile(path):
        with open(path, encoding="utf-8") as f:
            token = f.read().strip()
    if not token:
        sys.exit(f"No access token: set NF_STUDIO_TOKEN or put the token in {path}.")
    return token


class Studio:
    def __init__(self, base):
        self.base = base.rstrip("/")
        self.opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))

    def open(self, method, path, body=None, headers=None, timeout=60):
        h = {"accept": "application/json", **(headers or {})}
        data = None
        if body is not None:
            data = json.dumps(body).encode()
            h["content-type"] = "application/json"
        req = urllib.request.Request(self.base + path, data=data, method=method, headers=h)
        try:
            return self.opener.open(req, timeout=timeout)
        except urllib.error.HTTPError as e:
            message = f"the server answered {e.code}"
            try:
                err = json.loads(e.read()).get("error")
                if isinstance(err, str):
                    message = err
            except Exception:  # noqa: BLE001
                pass
            raise StudioError(e.code, f"{method} {path}: {message}") from None
        except urllib.error.URLError as e:
            raise StudioError(0, f"{method} {path}: cannot reach {self.base} ({e.reason})") from None

    def call(self, method, path, body=None, headers=None):
        with self.open(method, path, body, headers) as r:
            raw = r.read()
        return json.loads(raw) if raw else None

    def sign_in(self, token):
        try:
            self.call("POST", "/api/session", {"token": token})
        except StudioError as e:
            if e.status == 429:
                sys.exit("Too many wrong tokens; wait a minute and try again.")
            if e.status in (400, 401, 403):
                sys.exit("The server did not accept the access token.")
            raise
        if not (self.call("GET", "/api/session") or {}).get("signedIn"):
            sys.exit("Signed in, but the server does not keep the session.")

    def jobs(self):
        return self.call("GET", "/api/jobs")["jobs"]

    def size(self, path):
        """The file's full size from a one-byte range request (the server has no HEAD), or None."""
        try:
            with self.open("GET", path, headers={"range": "bytes=0-0"}, timeout=30) as r:
                m = re.search(r"/(\d+)$", r.headers.get("content-range") or "")
                if m:
                    return int(m.group(1))
                n = r.headers.get("content-length")
                return int(n) if r.status == 200 and n and n.isdigit() else None
        except StudioError:
            return None


def connect():
    s = Studio(os.environ.get("NF_STUDIO_URL", DEFAULT_URL))
    health = s.call("GET", "/api/health")
    s.sign_in(read_token())
    return s, health


def title(job):
    return job.get("prompt") or job.get("upload") or job["id"]


def known_ids():
    ids = set()
    for path in glob.glob(os.path.join(PACK_MANIFESTS, "*.json")):
        for a in load_json(path, {}).get("assets", []):
            if a.get("id"):
                ids.add(a["id"])
    return ids


def asset_for(job, submissions):
    """The prompts.py asset a job was made for: from the submission log, else the longest prompt its text starts with."""
    sub = submissions.get(job["id"])
    if sub and sub.get("asset"):
        return sub["asset"]
    text = norm(job.get("prompt") or "")
    if not text:
        return None
    hits = [p for p in PROMPTS if text.startswith(norm(p[3]))]
    return max(hits, key=lambda p: len(p[3]))[2] if hits else None


def mb(n):
    return f"{n / 1e6:7.1f} MB"


# --- status -------------------------------------------------------------------------------------------------------

def cmd_status(args):
    s, health = connect()
    print(f"Asset Studio {health.get('version')} | ComfyUI {health.get('comfyui')} {health.get('comfyuiVersion', '')}")
    jobs = s.jobs()
    counts = {}
    for j in jobs:
        counts[j["state"]] = counts.get(j["state"], 0) + 1
    print("jobs: " + ", ".join(f"{n} {st}" for st, n in sorted(counts.items())))
    have = known_ids()
    new = [j for j in jobs if j["state"] == "done" and j["id"] not in have]
    print(f"done and not downloaded yet: {len(new)}")
    for j in jobs:
        if j["state"] == "failed":
            err = (j.get("error") or "").replace("\n", " ")[:90]
            print(f"  failed  {j['id'][:8]}  {title(j)[:70]}  {err}")


# --- submit -------------------------------------------------------------------------------------------------------

def cmd_submit(args):
    by_name = {p[2]: p for p in PROMPTS}
    wanted = []  # (asset or None, prompt text)
    if args.missing:
        rows = load_json(os.path.join(ROOT, "manifest.json"), {}).get("assets", [])
        wanted += [(r["asset"], by_name[r["asset"]][3]) for r in rows if r["status"] != "ok" and r["asset"] in by_name]
    for name in args.asset or []:
        if name not in by_name:
            sys.exit(f"{name} is not in prompts.py.")
        wanted.append((name, by_name[name][3]))
    for raw in args.prompt or []:
        m = re.match(r"^([A-Za-z][A-Za-z0-9_]*)=(.+)$", raw)
        wanted.append((m.group(1), m.group(2)) if m else (None, raw))
    if not wanted:
        sys.exit("Nothing to submit: give --missing, --asset or --prompt.")

    faces, tex = QUALITY[args.quality]
    items_all, seen = [], set()
    for asset, text in wanted:
        full = text.strip()
        if not full.lower().endswith(SUFFIX.strip(", ").lower()):
            full += SUFFIX
        if norm(full) in seen:
            continue
        seen.add(norm(full))
        if len(full) > 2000:
            sys.exit(f"Prompt longer than 2000 characters: {full[:60]}...")
        items_all.append((asset, full))

    s, _ = connect()
    have = known_ids()
    # Queued, generating, or finished but not downloaded yet: submitting those again only costs GPU time.
    busy = {norm(j.get("prompt") or "") for j in s.jobs()
            if j["state"] in BUSY or (j["state"] == "done" and j["id"] not in have)}
    items = [it for it in items_all if args.force or norm(it[1]) not in busy]
    if len(items) < len(items_all):
        print(f"{len(items_all) - len(items)} prompt(s) skipped: already queued, or finished and waiting for fetch"
              " (--force submits anyway)")
    if not items:
        return
    presets = {"faces": faces, "tex": tex, "variants": args.variants, "style": not args.no_style,
               "seed": args.seed, "sharedStyle": args.shared_style}
    print(f"batch: {len(items)} prompt(s) x {args.variants} variant(s), {args.quality} ({faces} faces, {tex} px"
          f" textures), about {EST_MB[tex] * len(items) * args.variants} MB to download")
    for asset, full in items:
        print(f"  {asset or '-':24} {full}")
    if args.dry_run:
        return
    res = s.call("POST", "/api/batches", {"kind": "prompt", "prompts": [f for _, f in items], "presets": presets},
                 headers={"x-batch-id": str(uuid.uuid4())})
    jobs = ((res or {}).get("batch") or {}).get("jobs") or []
    submissions = load_json(SUBMISSIONS, {})
    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    for k, job in enumerate(jobs):
        jid = job.get("id") if isinstance(job, dict) else job
        jprompt = job.get("prompt") if isinstance(job, dict) else None
        match = [a for a, f in items if jprompt and norm(f) == norm(jprompt)]
        n = k // args.variants
        asset = match[0] if match else (items[n][0] if n < len(items) else None)
        if jid:
            submissions[jid] = {"asset": asset, "prompt": jprompt, "quality": args.quality, "submitted": now}
    save_json(SUBMISSIONS, submissions)
    print(f"{len(jobs)} job(s) added")


# --- retry --------------------------------------------------------------------------------------------------------

def cmd_retry(args):
    s, _ = connect()
    failed = [j for j in s.jobs() if j["state"] == "failed"]
    for j in failed:
        print(f"retry  {j['id'][:8]}  {title(j)[:80]}")
        if not args.dry_run:
            s.call("POST", f"/api/jobs/{j['id']}/retry")
    print(f"{len(failed)} failed job(s)" + (" (dry run)" if args.dry_run else " sent back to the queue"))


# --- cancel -------------------------------------------------------------------------------------------------------

def cmd_cancel(args):
    if not args.asset and not args.ids:
        sys.exit("Give --asset names or job ids (a prefix is enough).")
    s, _ = connect()
    submissions = load_json(SUBMISSIONS, {})
    hits = [j for j in s.jobs() if j["state"] in BUSY and (
        (args.asset and asset_for(j, submissions) in args.asset) or any(j["id"].startswith(p) for p in args.ids or []))]
    for j in hits:
        print(f"cancel  {j['id'][:8]}  {j.get('faces')}f/{j.get('tex')}px  {title(j)[:70]}")
        if not args.dry_run:
            s.call("POST", f"/api/jobs/{j['id']}/cancel")
    print(f"{len(hits)} queued or running job(s)" + (" (dry run)" if args.dry_run else " cancelled"))


# --- fetch --------------------------------------------------------------------------------------------------------

def glb_ok(path):
    """A complete GLB: the magic and the length in its header match the file."""
    try:
        with open(path, "rb") as f:
            head = f.read(12)
        return len(head) == 12 and head[:4] == b"glTF" and struct.unpack_from("<I", head, 8)[0] == os.path.getsize(path)
    except OSError:
        return False


def download(s, path, target, tries=3):
    """Stream path to target through target.part; a broken download resumes where it stopped when the server
    allows ranges (else it starts over)."""
    part = target + ".part"
    for attempt in range(1, tries + 1):
        have = os.path.getsize(part) if os.path.isfile(part) else 0
        headers = {"range": f"bytes={have}-"} if have else {}
        try:
            with s.open("GET", path, headers=headers, timeout=120) as r:
                resumed = have and r.status == 206
                length = r.headers.get("content-length")
                expected = (have if resumed else 0) + int(length) if length and length.isdigit() else None
                with open(part, "ab" if resumed else "wb") as f:
                    shutil.copyfileobj(r, f, CHUNK)
            if expected is not None and os.path.getsize(part) < expected:
                raise OSError(f"got {os.path.getsize(part)} of {expected} bytes")
            os.replace(part, target)
            return
        except (OSError, http.client.HTTPException) as e:
            if attempt == tries:
                raise StudioError(0, f"download broke off {tries} times ({e}); run fetch again to resume") from None
            print(f"   download broke off at {mb(os.path.getsize(part) if os.path.isfile(part) else 0).strip()}, resuming")
            time.sleep(2 * attempt)


def wait_for(s, ids):
    while True:
        busy = [j for j in s.jobs() if j["state"] in BUSY and (not ids or j["id"] in ids)]
        if not busy:
            return
        run = [j for j in busy if j.get("running")]
        now = time.time() * 1000
        doing = ", ".join(f"{title(j)[:40]} ({(now - (j.get('startedAt') or now)) / 60000:.0f} min)" for j in run)
        print(f"  {time.strftime('%H:%M:%S')}  generating: {doing or '-'}; {len(busy) - len(run)} queued")
        time.sleep(30)


def cmd_fetch(args):
    s, _ = connect()
    submissions = load_json(SUBMISSIONS, {})
    if args.wait:
        wait_for(s, set(submissions) if not args.all else set())
    have = known_ids()
    todo = []
    for j in s.jobs():
        if j["state"] != "done" or j["id"] in have:
            continue
        asset = asset_for(j, submissions)
        if not args.all and not asset:
            continue
        if args.asset and asset not in args.asset:
            continue
        todo.append((j, asset))
    todo.sort(key=lambda t: t[0].get("finishedAt") or t[0].get("createdAt") or "")
    if not todo:
        print("Nothing new to download.")
        return

    budget = int(args.max_mb * 1e6)
    plan, used, over = [], 0, []
    for j, asset in todo:
        size = s.size(f"/api/assets/{j['id']}/model.glb")
        est = size is None
        if est:
            size = int(EST_MB.get(j.get("tex"), 11) * 1e6)
        if used + size > budget:
            over.append((j, asset, size, est))
            continue
        used += size
        plan.append((j, asset, size, est))
    for j, asset, size, est in plan:
        print(f"{mb(size)}{'~' if est else ' '} {asset or '-':24} {j.get('faces', '?')}f/{j.get('tex', '?')}px  {title(j)[:60]}")
    print(f"total {mb(used).strip()} for {len(plan)} model(s)" + (" (~ = estimated)" if any(p[3] for p in plan) else ""))
    if over:
        print(f"{len(over)} model(s) left out to stay under --max-mb {args.max_mb:g} ({mb(sum(o[2] for o in over)).strip()})")
    free = shutil.disk_usage(ROOT).free
    if used + 1e9 > free:
        sys.exit(f"Not enough disk space: {mb(free).strip()} free, need {mb(used).strip()} plus 1 GB headroom.")
    if args.dry_run or not plan:
        return

    name = args.name or "asset-studio-" + time.strftime("%Y-%m-%d-%H%M")
    out = os.path.join(DOWNLOADS, name)
    os.makedirs(out, exist_ok=True)
    os.makedirs(PACK_MANIFESTS, exist_ok=True)
    mpath = os.path.join(out, "manifest.json")
    manifest = load_json(mpath, {"schema": 1, "assets": []})
    done_ids = {a["id"] for a in manifest["assets"]}
    got = 0
    for k, (j, asset, size, _) in enumerate(plan, 1):
        if j["id"] in done_ids:
            continue
        stem = f"{slug(title(j))}-{j['id'][:8]}"
        glb = os.path.join(out, stem + ".glb")
        print(f"[{k}/{len(plan)}] {asset or '-'}  {stem}.glb")
        try:
            if not glb_ok(glb):
                download(s, f"/api/assets/{j['id']}/model.glb", glb)
            if not glb_ok(glb):
                os.replace(glb, glb + ".bad")
                print("   incomplete or not a GLB; kept as .bad")
                continue
            source = None
            if j.get("sourceReady"):
                with s.open("GET", f"/api/jobs/{j['id']}/source-preview") as r:
                    ext = {"image/jpeg": ".jpg", "image/webp": ".webp"}.get(r.headers.get_content_type(), ".png")
                    source = stem + ext
                    with open(os.path.join(out, source), "wb") as f:
                        shutil.copyfileobj(r, f, CHUNK)
        except (StudioError, OSError, http.client.HTTPException) as e:
            print(f"   skipped: {e}")
            continue
        got += os.path.getsize(glb)
        manifest["assets"].append({
            "id": j["id"], "glb": stem + ".glb", "source": source, "prompt": j.get("prompt"),
            "upload": j.get("upload"), "engine": j.get("engine", "trellis"), "faces": j.get("faces"),
            "texture": j.get("tex"), "style": j.get("style"), "sharedStyle": j.get("sharedStyle"),
            "seed": j.get("seed"), "created": j.get("createdAt"), "asset": asset})
        save_json(mpath, manifest)
        save_json(os.path.join(PACK_MANIFESTS, name + ".json"), manifest)
    print(f"downloaded {mb(got).strip()} to {out}")
    print("sort them in with:")
    print(f"  python -I {os.path.join(HERE, 'sort_packs.py')} {ROOT} --merge {out}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("status", help="server health, job counts, failed jobs")

    p = sub.add_parser("submit", help="queue prompts from prompts.py or given text")
    p.add_argument("--missing", action="store_true", help="every asset manifest.json does not mark ok")
    p.add_argument("--asset", nargs="+", help="asset names from prompts.py")
    p.add_argument("--prompt", nargs="+", help='free text, or "AssetName=text" to tie it to an asset')
    p.add_argument("--quality", choices=QUALITY, default=QUALITY_DEFAULT,
                   help="Fast 5k/1K, Standard 10k/2K, High 20k/2K, Ultra 30k/4K (default)")
    p.add_argument("--variants", type=int, default=1, choices=range(1, 5))
    p.add_argument("--seed", type=int)
    p.add_argument("--no-style", action="store_true", help="turn the game-asset style off")
    p.add_argument("--shared-style", help="shared style text added to every prompt")
    p.add_argument("--force", action="store_true", help="submit prompts that are already queued")
    p.add_argument("--dry-run", action="store_true")

    p = sub.add_parser("retry", help="send failed jobs back to the queue")
    p.add_argument("--dry-run", action="store_true")

    p = sub.add_parser("cancel", help="cancel queued or running jobs")
    p.add_argument("ids", nargs="*", help="job ids or id prefixes")
    p.add_argument("--asset", nargs="+", help="every queued or running job for these assets")
    p.add_argument("--dry-run", action="store_true")

    p = sub.add_parser("fetch", help="download finished models that are not here yet")
    p.add_argument("--wait", action="store_true", help="wait until the submitted jobs are finished first")
    p.add_argument("--max-mb", type=float, default=1500, help="download budget for this run (default 1500)")
    p.add_argument("--asset", nargs="+", help="only these assets")
    p.add_argument("--all", action="store_true", help="also jobs that match no prompts.py asset (photos, tests)")
    p.add_argument("--name", help="pack folder name (default asset-studio-<date>-<time>)")
    p.add_argument("--dry-run", action="store_true", help="list what would be downloaded and the sizes")

    args = ap.parse_args()
    try:
        {"status": cmd_status, "submit": cmd_submit, "retry": cmd_retry, "cancel": cmd_cancel,
         "fetch": cmd_fetch}[args.cmd](args)
    except StudioError as e:
        sys.exit(str(e))
    except KeyboardInterrupt:
        sys.exit("stopped; run the same command again to continue")


main()
