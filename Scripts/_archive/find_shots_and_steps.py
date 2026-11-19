import urllib.request
import urllib.parse
import re

HEADERS = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'}

def fetch_page(dir_query):
    url = f"https://gamesounds.xyz/{dir_query}"
    req = urllib.request.Request(url, headers=HEADERS)
    try:
        with urllib.request.urlopen(req, timeout=12) as resp:
            return resp.read().decode('utf-8', errors='ignore')
    except Exception as e:
        return ""

def main():
    bundles = [
        "Sonniss.com - GDC 2015 - Game Audio Bundle",
        "Sonniss.com - GDC 2016 - Game Audio Bundle",
        "Sonniss.com - GDC 2017 - Game Audio Bundle",
        "Sonniss.com - GDC 2018 - Game Audio Bundle",
        "Sonniss.com - GDC 2019 - Game Audio Bundle",
        "Sonniss.com - GDC 2020 - Game Audio Bundle"
    ]
    
    print("=== Searching for Single Gunshots and Footsteps ===")
    
    shot_matches = []
    footstep_matches = []
    
    for b in bundles:
        html = fetch_page(f"?dir={urllib.parse.quote(b)}")
        subdirs = re.findall(r'href=[\'"](\?dir=[^\'"]+)[\'"]', html)
        for sd in subdirs:
            dec = urllib.parse.unquote(sd)
            is_wep = any(k in dec.lower() for k in ['gun', 'weapon', 'firearm', 'rifle', 'pistol', 'shot'])
            is_step = any(k in dec.lower() for k in ['step', 'footstep', 'foley', 'movement', 'walk'])
            
            if is_wep or is_step:
                sub_html = fetch_page(sd)
                files = re.findall(r'href=[\'"]([^\'"]+\.(?:wav|ogg|mp3))[\'"]', sub_html, re.IGNORECASE)
                for f in files:
                    fn = urllib.parse.unquote(f).split('/')[-1]
                    fn_lower = fn.lower()
                    
                    # Single shots
                    if any(k in fn_lower for k in ['single', 'shot_01', 'shot_1', 'single_shot', 'oneshot', 'solo', 'semi']) and not any(k in fn_lower for k in ['burst', 'auto', 'automatic_preview', 'loop']):
                        shot_matches.append((b.split()[3], fn, f))
                    elif any(k in fn_lower for k in ['gunshot', 'rifle', 'carbine']) and 'burst' not in fn_lower and 'auto' not in fn_lower and 'loop' not in fn_lower:
                        shot_matches.append((b.split()[3], fn, f))
                        
                    # Footsteps
                    if any(k in fn_lower for k in ['concrete', 'wood', 'sand', 'dirt', 'puddle', 'water', 'mud', 'gravel', 'step']):
                        footstep_matches.append((b.split()[3], fn, f))
                        
    print(f"\n--- Found {len(shot_matches)} Potential Single-Shot Sounds ---")
    for yr, fn, url in shot_matches[:25]:
        print(f"[{yr}] {fn}")
        print(f"      {url}")
        
    print(f"\n--- Found {len(footstep_matches)} Potential Footstep Sounds ---")
    for yr, fn, url in footstep_matches[:30]:
        print(f"[{yr}] {fn}")
        print(f"      {url}")

if __name__ == '__main__':
    main()
