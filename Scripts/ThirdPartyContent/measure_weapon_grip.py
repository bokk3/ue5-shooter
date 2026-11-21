"""Measure where the operator's right hand should hold our weapons, from the Lyra hold poses.

For each hold pose (rifle, pistol) this evaluates frame 0 on the UEFN skeleton in component space and
derives the weapon transform relative to hand_r:
  * grip point: inside the fist, 60% of the way from the wrist (hand_r) to the middle knuckle
    (middle_01_r); the UEFN mannequin has no weapon socket,
  * barrel axis: straight ahead (+Y in component space), which is where the hold pose aims with the
    aim offset centred; left-hand IK in the anim graph then brings the left hand to the foregrip,
  * weapon up: component up, made orthogonal to the barrel.
Prints the hand_r-relative transform of the weapon *grip point*; AFPSCharacter adds the weapon's own
grip offset. Run headless; results go to the log and to $NF_GRIP_OUT (JSON).
"""
import json
import os

import unreal

LYRA = "/Game/Characters/UEFN_Mannequin/Animations/Lyra"
POSES = {
    "Rifle": f"{LYRA}/Locomotion/Rifle/MM_Rifle_Idle_Hipfire",
    "Pistol": f"{LYRA}/Locomotion/Pistol/MM_Pistol_Idle_Hipfire",
}
GRIP_ALONG_HAND = 0.6
log = unreal.log
alib = unreal.AnimationLibrary


def component_space(anim, bone):
    """Bone transform in component space at frame 0, composed from the bone's path to the root."""
    path = list(alib.find_bone_path_to_root(anim, bone))  # bone ... root
    xf = unreal.Transform()
    for name in path:  # child first: world = local(child) * local(parent) * ...
        local = alib.get_bone_pose_for_frame(anim, name, 0, False)
        xf = xf.multiply(local) if name != path[0] else local
    return xf


def vec(v):
    return [round(v.x, 3), round(v.y, 3), round(v.z, 3)]


results = {}
for family, path in POSES.items():
    anim = unreal.load_asset(path)
    hand_r = component_space(anim, "hand_r")
    hand_l = component_space(anim, "hand_l")
    knuckle = component_space(anim, "middle_01_r")
    grip = hand_r.translation + (knuckle.translation - hand_r.translation) * GRIP_ALONG_HAND

    to_left = hand_l.translation - hand_r.translation
    forward = unreal.Vector(0, 1, 0)  # UE mannequins face +Y in component space
    two_handed = to_left.length() > 15.0 and to_left.normal().dot(forward) > 0.3
    barrel = forward
    up = unreal.Vector(0, 0, 1)
    right = up.cross(barrel).normal()
    up = barrel.cross(right).normal()
    weapon_rot = unreal.MathLibrary.make_rot_from_xz(barrel, up)
    weapon = unreal.Transform(grip, weapon_rot, unreal.Vector(1, 1, 1))

    rel = weapon.multiply(hand_r.inverse())  # weapon grip relative to hand_r
    r = rel.rotation.rotator()
    results[family] = {
        "location": vec(rel.translation),
        "rotation_pitch_yaw_roll": [round(r.pitch, 3), round(r.yaw, 3), round(r.roll, 3)],
        "two_handed": two_handed,
        "wrist_to_knuckle_cm": round((knuckle.translation - hand_r.translation).length(), 1),
        "hand_gap_cm": round(to_left.length(), 1),
        "hand_line_cs": vec(to_left.normal()),
    }
    log(f"[Grip] {family}: {results[family]}")

out = os.environ.get("NF_GRIP_OUT")
if out:
    json.dump(results, open(out, "w"), indent=1)
log("[Grip] GRIP_DONE")
