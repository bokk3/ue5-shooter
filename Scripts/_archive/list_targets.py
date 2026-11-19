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

def list_folder_files(folder_query):
    html = fetch_page(folder_query)
    files = re.findall(r'href=[\'"]([^\'"]+\.(?:wav|ogg|mp3))[\'"]', html, re.IGNORECASE)
    subdirs = re.findall(r'href=[\'"](\?dir=[^\'"]+)[\'"]', html)
    return files, subdirs

def main():
    target_folders = [
        "?dir=Sonniss.com%20-%20GDC%202018%20-%20Game%20Audio%20Bundle/Watson%20Wu%20-%20Battle%20Rifles%20%26%20Pistols",
        "?dir=Sonniss.com%20-%20GDC%202017%20-%20Game%20Audio%20Bundle/Gamemaster%20Audio%20-%20%20Gun%20Sound%20Pack",
        "?dir=Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Audiobeast%20-%20The%20London%20Warehouse%20Firearms%20Library",
        "?dir=Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle/Wav%20Junction%20Sound%20Effects%20-%20Footsteps",
        "?dir=Sonniss.com%20-%20GDC%202017%20-%20Game%20Audio%20Bundle/Tovusound%20-%20Edward%20%E2%80%93%20Foleyart%20Collection%20Add-On%20Extended%20Footsteps",
        "?dir=Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Soldier%20Footsteps",
        "?dir=Sonniss.com%20-%20GDC%202016%20-%20Game%20Audio%20Bundle/Levan%20Nadashvili%20-%20Civilian%20Footsteps"
    ]
    
    for fld in target_folders:
        name = urllib.parse.unquote(fld).split('/')[-1]
        print(f"\n=== {name} ===")
        files, subdirs = list_folder_files(fld)
        print(f"Files ({len(files)}):")
        for f in files:
            fn = urllib.parse.unquote(f).split('/')[-1]
            print(f"   {fn}")
            print(f"       -> {f}")
        if subdirs:
            print(f"Subdirs ({len(subdirs)}):")
            for s in subdirs:
                print(f"   {urllib.parse.unquote(s)}")

if __name__ == '__main__':
    main()
