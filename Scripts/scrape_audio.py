import urllib.request
import os

HEADERS = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'}

test_file = "https://gamesounds.xyz/Sonniss.com%20-%20GDC%202018%20-%20Game%20Audio%20Bundle/Gamemaster%20Audio%20-%20Guns%2C%20Bullets%20and%20Explosions/gun_submachine_auto_shot_00_automatic_preview_01.wav"
dest = "Content/Audio/Test_Gunshot.wav"
os.makedirs("Content/Audio", exist_ok=True)

print("Testing direct download from:", test_file)
req = urllib.request.Request(test_file, headers=HEADERS)
with urllib.request.urlopen(req, timeout=20) as resp, open(dest, "wb") as f:
    f.write(resp.read())

print("Downloaded successfully! File size:", os.path.getsize(dest), "bytes")
