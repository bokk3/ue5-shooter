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

def scan_bundle(bundle_name):
    print(f"\n=== Scanning {bundle_name} ===")
    html = fetch_page(f"?dir={urllib.parse.quote(bundle_name)}")
    subdirs = re.findall(r'href=[\'"](\?dir=[^\'"]+)[\'"]', html)
    
    # Filter subdirs for audio of interest
    target_subdirs = []
    keywords = ['gun', 'weapon', 'firearm', 'bullet', 'reload', 'foley', 'wind', 'rain', 'storm', 'weather', 'ambient', 'ambience']
    for s in subdirs:
        dec = urllib.parse.unquote(s)
        if any(k in dec.lower() for k in keywords):
            target_subdirs.append(s)
    
    print(f"  Found {len(target_subdirs)} relevant provider folders")
    
    found_files = []
    for sd in target_subdirs[:12]:
        folder_name = urllib.parse.unquote(sd).split('/')[-1]
        sub_html = fetch_page(sd)
        files = re.findall(r'href=[\'"]([^\'"]+\.(?:wav|ogg|mp3))[\'"]', sub_html, re.IGNORECASE)
        if files:
            print(f"  [{folder_name}] -> {len(files)} files:")
            for f in files:
                dec_f = urllib.parse.unquote(f).split('/')[-1]
                print(f"      - {dec_f}")
                found_files.append((folder_name, f))
    return found_files

def main():
    bundles = [
        "Sonniss.com - GDC 2018 - Game Audio Bundle",
        "Sonniss.com - GDC 2017 - Game Audio Bundle",
        "Sonniss.com - GDC 2016 - Game Audio Bundle",
        "Sonniss.com - GDC 2019 - Game Audio Bundle",
        "Sonniss.com - GDC 2020 - Game Audio Bundle"
    ]
    all_targets = []
    for b in bundles:
        targets = scan_bundle(b)
        all_targets.extend(targets)
    print(f"\nTotal collected targets: {len(all_targets)}")

if __name__ == '__main__':
    main()
