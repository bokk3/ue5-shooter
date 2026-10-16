import os
import sys

script_dir = os.path.dirname(os.path.abspath(__file__))

print("=== STEP 1: IMPORTING SK_BASEMALE & SM_AK47 ===")
with open(os.path.join(script_dir, "import_base_male.py"), "r") as f:
    code1 = f.read().replace("sys.exit(0)", "")
exec(code1, globals())

print("=== STEP 2: IMPORTING 5 IK-BAKED TACTICAL ANIMATIONS ===")
with open(os.path.join(script_dir, "import_locomotion_anims.py"), "r") as f:
    code2 = f.read()
exec(code2, globals())
