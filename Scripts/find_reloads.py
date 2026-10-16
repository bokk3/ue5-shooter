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
    
    print("Searching for reload/handling/foley...")
    for b in bundles:
        html = fetch_page(f"?dir={urllib.parse.quote(b)}")
        subdirs = re.findall(r'href=[\'"](\?dir=[^\'"]+)[\'"]', html)
        for sd in subdirs:
            dec = urllib.parse.unquote(sd)
            if any(k in dec.lower() for k in ['gun', 'weapon', 'firearm', 'foley', 'mechanic', 'locker']):
                sub_html = fetch_page(sd)
                files = re.findall(r'href=[\'"]([^\'"]+\.(?:wav|ogg|mp3))[\'"]', sub_html, re.IGNORECASE)
                for f in files:
                    fn = urllib.parse.unquote(f).split('/')[-1]
                    print(f"[{b.split()[3]}] {fn}")
                    print(f"   URL: {f}")

if __name__ == '__main__':
    main()
