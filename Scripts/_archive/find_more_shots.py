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
        "?dir=Sonniss.com%20-%20GDC%202015%20-%20Game%20Audio%20Bundle",
        "?dir=Sonniss.com%20-%20GDC%202019%20-%20Game%20Audio%20Bundle",
        "?dir=Sonniss.com%20-%20GDC%202020%20-%20Game%20Audio%20Bundle"
    ]
    for b in target_folders:
        html = fetch_page(b)
        subdirs = re.findall(r'href=[\'"](\?dir=[^\'"]+)[\'"]', html)
        for s in subdirs:
            dec = urllib.parse.unquote(s)
            if any(k in dec.lower() for k in ['pole position', 'soundmorph', 'firearm', 'shot', 'weapon', 'gun', 'acoustic']):
                f_list, _ = list_folder_files(s)
                print(f"Folder: {dec.split('/')[-1]} ({len(f_list)} files)")
                for f in f_list:
                    fn = urllib.parse.unquote(f).split('/')[-1]
                    if any(k in fn.lower() for k in ['single', 'shot', 'fire', 'rifle']):
                        print(f"   -> {fn}")
                        print(f"      {f}")

if __name__ == '__main__':
    main()
