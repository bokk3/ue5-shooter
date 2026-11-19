import sys

# Read binary FBX or print bone relations
with open('Content/Characters/BaseMale/SK_BaseMale.fbx', 'rb') as f:
    content = f.read()

# Search for bone names in binary FBX
for name in [b'hand_r', b'thumb_01_r', b'thumb_02_r', b'index_01_r', b'index_02_r', b'middle_01_r', b'middle_02_r', b'hand_l', b'index_01_l']:
    idx = 0
    found = []
    while True:
        pos = content.find(name, idx)
        if pos == -1: break
        found.append(pos)
        idx = pos + len(name)
    print(f"Bone {name.decode()}: found at {len(found)} places: {found[:3]}")
