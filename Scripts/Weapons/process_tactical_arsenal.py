"""
Noble Fermi // Automated Tactical Arsenal Processor (Headless Blender 4.2)
Analyzes, scales, normalizes, separates detachable magazines, computes hand grip sockets,
embeds gameplay metadata properties, and exports game-ready FBX + JSON sidecars.
"""
import bpy
import mathutils
import json
import os
import math

RAW_DIR = os.path.abspath(os.path.join("SourceArt", "Weapons", "Raw"))
OUT_DIR = os.path.abspath(os.path.join("SourceArt", "Weapons", "Processed"))
os.makedirs(OUT_DIR, exist_ok=True)

# 11-Piece Tactical Arsenal Catalog Definition
WEAPON_CATALOG = [
    {
        "id": "W01_NF98",
        "name": "NF-98 Carbine",
        "archetype": "AssaultRifle",
        "caliber": "5.56x45mm NATO",
        "target_length_cm": 84.0,
        "base_raw": "AR_Carbine.glb",
        "mag_raw": "Mag_STANAG.glb",
        "stats": {
            "mag_capacity": 30,
            "reserve_ammo": 180,
            "base_damage": 34.0,
            "fire_rate_rpm": 750,
            "effective_range_m": 450.0,
            "reload_time_sec": 2.1,
            "empty_reload_time_sec": 2.6,
            "recoil_pitch": 0.38,
            "recoil_yaw_spread": 0.12,
            "fire_modes": ["FullAuto", "Burst", "SemiAuto"]
        },
        "grip_offsets": {
            "right_hand": [-12.0, 0.0, -8.0, -18.0, 0.0, 0.0],
            "left_hand": [22.0, 0.0, 1.0, 0.0, 45.0, 90.0],
            "muzzle": [42.0, 0.0, 2.5],
            "optic": [4.0, 0.0, 6.2],
            "magwell": [6.0, 0.0, -1.5],
            "eject": [2.0, 2.2, 3.0]
        }
    },
    {
        "id": "W02_VK74",
        "name": "VK-74 Vektor",
        "archetype": "AssaultRifle",
        "caliber": "7.62x39mm",
        "target_length_cm": 88.0,
        "base_raw": "AR_Vektor.glb",
        "mag_raw": None, # Embedded in glb
        "stats": {
            "mag_capacity": 30,
            "reserve_ammo": 150,
            "base_damage": 42.0,
            "fire_rate_rpm": 620,
            "effective_range_m": 400.0,
            "reload_time_sec": 2.3,
            "empty_reload_time_sec": 2.9,
            "recoil_pitch": 0.48,
            "recoil_yaw_spread": 0.18,
            "fire_modes": ["FullAuto", "SemiAuto"]
        },
        "grip_offsets": {
            "right_hand": [-14.0, 0.0, -8.5, -20.0, 0.0, 0.0],
            "left_hand": [24.0, 0.0, 0.8, 0.0, 45.0, 90.0],
            "muzzle": [44.0, 0.0, 2.8],
            "optic": [2.0, 0.0, 6.5],
            "magwell": [7.5, 0.0, -1.8],
            "eject": [3.0, 2.4, 3.2]
        }
    },
    {
        "id": "W03_Falcon50",
        "name": "Falcon-50 DMR",
        "archetype": "BattleRifle",
        "caliber": "7.62x51mm NATO",
        "target_length_cm": 104.0,
        "base_raw": "Rifle_DMR.glb",
        "mag_raw": None, # Embedded in glb
        "stats": {
            "mag_capacity": 20,
            "reserve_ammo": 100,
            "base_damage": 65.0,
            "fire_rate_rpm": 450,
            "effective_range_m": 750.0,
            "reload_time_sec": 2.4,
            "empty_reload_time_sec": 3.0,
            "recoil_pitch": 0.65,
            "recoil_yaw_spread": 0.08,
            "fire_modes": ["SemiAuto"]
        },
        "grip_offsets": {
            "right_hand": [-16.0, 0.0, -9.0, -18.0, 0.0, 0.0],
            "left_hand": [28.0, 0.0, 0.5, 0.0, 45.0, 90.0],
            "muzzle": [52.0, 0.0, 2.6],
            "optic": [5.0, 0.0, 7.0],
            "magwell": [5.0, 0.0, -1.5],
            "eject": [3.5, 2.5, 3.4]
        }
    },
    {
        "id": "W04_Breacher12",
        "name": "Breacher-12",
        "archetype": "Shotgun",
        "caliber": "12-Gauge Buckshot",
        "target_length_cm": 96.0,
        "base_raw": "Shotgun_Breacher.glb",
        "mag_raw": None,
        "stats": {
            "mag_capacity": 8,
            "reserve_ammo": 48,
            "base_damage": 120.0,
            "fire_rate_rpm": 220,
            "effective_range_m": 35.0,
            "reload_time_sec": 2.8,
            "empty_reload_time_sec": 3.4,
            "recoil_pitch": 0.95,
            "recoil_yaw_spread": 0.35,
            "fire_modes": ["SemiAuto"]
        },
        "grip_offsets": {
            "right_hand": [-15.0, 0.0, -8.0, -22.0, 0.0, 0.0],
            "left_hand": [22.0, 0.0, -1.0, 0.0, 40.0, 90.0],
            "muzzle": [48.0, 0.0, 1.8],
            "optic": [6.0, 0.0, 5.2],
            "magwell": [4.0, 0.0, -3.0],
            "eject": [2.0, 2.2, 2.2]
        }
    },
    {
        "id": "W05_Apex9",
        "name": "Apex-9",
        "archetype": "Pistol",
        "caliber": "9x19mm Parabellum",
        "target_length_cm": 19.5,
        "base_raw": "Pistol_Apex9.glb",
        "mag_raw": None,
        "stats": {
            "mag_capacity": 17,
            "reserve_ammo": 68,
            "base_damage": 28.0,
            "fire_rate_rpm": 480,
            "effective_range_m": 50.0,
            "reload_time_sec": 1.4,
            "empty_reload_time_sec": 1.8,
            "recoil_pitch": 0.28,
            "recoil_yaw_spread": 0.10,
            "fire_modes": ["SemiAuto"]
        },
        "grip_offsets": {
            "right_hand": [-3.5, 0.0, -6.0, -16.0, 0.0, 0.0],
            "left_hand": [-2.0, 0.0, -7.0, 0.0, 10.0, 0.0],
            "muzzle": [9.8, 0.0, 1.8],
            "optic": [0.0, 0.0, 4.0],
            "magwell": [-3.0, 0.0, -10.0],
            "eject": [1.0, 1.4, 2.4]
        }
    },
    {
        "id": "W06_Spectre45",
        "name": "Spectre-45",
        "archetype": "Pistol",
        "caliber": ".45 ACP",
        "target_length_cm": 21.5,
        "base_raw": "Pistol_Spectre45.glb",
        "mag_raw": None,
        "stats": {
            "mag_capacity": 8,
            "reserve_ammo": 40,
            "base_damage": 46.0,
            "fire_rate_rpm": 380,
            "effective_range_m": 60.0,
            "reload_time_sec": 1.5,
            "empty_reload_time_sec": 1.9,
            "recoil_pitch": 0.42,
            "recoil_yaw_spread": 0.12,
            "fire_modes": ["SemiAuto"]
        },
        "grip_offsets": {
            "right_hand": [-4.0, 0.0, -6.5, -18.0, 0.0, 0.0],
            "left_hand": [-2.5, 0.0, -7.5, 0.0, 10.0, 0.0],
            "muzzle": [10.8, 0.0, 2.0],
            "optic": [0.0, 0.0, 4.2],
            "magwell": [-3.5, 0.0, -11.0],
            "eject": [1.2, 1.5, 2.6]
        }
    },
    {
        "id": "W07_Kestrel357",
        "name": "Kestrel-357",
        "archetype": "Pistol",
        "caliber": ".357 Magnum",
        "target_length_cm": 24.0,
        "base_raw": "Pistol_Kestrel357.glb",
        "mag_raw": None,
        "stats": {
            "mag_capacity": 6,
            "reserve_ammo": 36,
            "base_damage": 72.0,
            "fire_rate_rpm": 240,
            "effective_range_m": 85.0,
            "reload_time_sec": 2.2,
            "empty_reload_time_sec": 2.2,
            "recoil_pitch": 0.75,
            "recoil_yaw_spread": 0.15,
            "fire_modes": ["SemiAuto"]
        },
        "grip_offsets": {
            "right_hand": [-4.5, 0.0, -7.0, -22.0, 0.0, 0.0],
            "left_hand": [-3.0, 0.0, -8.0, 0.0, 10.0, 0.0],
            "muzzle": [12.0, 0.0, 2.2],
            "optic": [0.0, 0.0, 4.5],
            "magwell": [0.0, -2.5, -1.0],
            "eject": [0.0, -3.0, 0.0]
        }
    },
    {
        "id": "W08_GrenadeM67",
        "name": "M67 Frag Grenade",
        "archetype": "Throwable",
        "caliber": "High-Explosive Frag",
        "target_length_cm": 9.0,
        "base_raw": "Grenade_M67.glb",
        "mag_raw": None,
        "stats": {
            "fuse_seconds": 3.8,
            "blast_radius_m": 8.5,
            "max_damage": 150.0,
            "detonation_sound": "A_Explosion_Grenade"
        },
        "grip_offsets": {
            "right_hand": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "left_hand": [0.0, 0.0, 4.0, 0.0, 0.0, 0.0],
            "pin": [-0.5, 2.2, 3.8],
            "spoon": [0.0, -2.2, 3.5]
        }
    },
    {
        "id": "W09_Stun9",
        "name": "Stun-9 Flashbang",
        "archetype": "Throwable",
        "caliber": "Concussive Flash",
        "target_length_cm": 13.5,
        "base_raw": "Grenade_Flashbang.glb",
        "mag_raw": None,
        "stats": {
            "fuse_seconds": 1.8,
            "flash_radius_m": 14.0,
            "blind_duration_sec": 4.5,
            "concussion_damage": 15.0
        },
        "grip_offsets": {
            "right_hand": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "left_hand": [0.0, 0.0, 5.0, 0.0, 0.0, 0.0],
            "pin": [-0.5, 2.0, 5.5],
            "spoon": [0.0, -2.0, 5.2]
        }
    },
    {
        "id": "W10_Machete",
        "name": "Favela Bolo Machete",
        "archetype": "Melee",
        "caliber": "High-Carbon Steel",
        "target_length_cm": 52.0,
        "base_raw": "Blade_Machete.glb",
        "mag_raw": None,
        "stats": {
            "blade_reach_cm": 85.0,
            "slash_damage": 75.0,
            "heavy_slash_damage": 125.0,
            "attack_rate_sec": 0.65
        },
        "grip_offsets": {
            "right_hand": [-18.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "blade_tip": [32.0, 0.0, 2.5]
        }
    },
    {
        "id": "W11_Stiletto",
        "name": "Shadow Stiletto",
        "archetype": "Melee",
        "caliber": "D2 Tool Steel",
        "target_length_cm": 28.0,
        "base_raw": "Blade_Stiletto.glb",
        "mag_raw": None,
        "stats": {
            "blade_reach_cm": 55.0,
            "slash_damage": 55.0,
            "backstab_damage": 250.0,
            "attack_rate_sec": 0.42
        },
        "grip_offsets": {
            "right_hand": [-8.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            "blade_tip": [18.0, 0.0, 0.0]
        }
    }
]

def get_mesh_bounds(objs):
    min_x, max_x = float('inf'), float('-inf')
    min_y, max_y = float('inf'), float('-inf')
    min_z, max_z = float('inf'), float('-inf')
    for o in objs:
        if o.type != 'MESH': continue
        for c in o.bound_box:
            world_c = o.matrix_world @ mathutils.Vector(c)
            min_x = min(min_x, world_c.x)
            max_x = max(max_x, world_c.x)
            min_y = min(min_y, world_c.y)
            max_y = max(max_y, world_c.y)
            min_z = min(min_z, world_c.z)
            max_z = max(max_z, world_c.z)
    return (min_x, max_x, min_y, max_y, min_z, max_z)

def process_weapon(entry):
    wid = entry["id"]
    wname = entry["name"]
    target_len = entry["target_length_cm"]
    base_file = os.path.join(RAW_DIR, entry["base_raw"])
    
    print(f"\n=======================================================")
    print(f"[PROCESS] Weapon {wid}: {wname} (Target Length: {target_len} cm)")
    print(f"=======================================================")
    
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=base_file)
    
    mesh_objs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    if not mesh_objs:
        print(f"  [ERROR] No mesh found in {base_file}")
        return
        
    # Unparent everything and apply current transforms
    bpy.ops.object.select_all(action='DESELECT')
    for o in mesh_objs:
        o.select_set(True)
        bpy.context.view_layer.objects.active = o
        bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
    
    # Remove empty nodes (RootNode etc)
    for o in list(bpy.context.scene.objects):
        if o.type == 'EMPTY':
            bpy.data.objects.remove(o, do_unlink=True)
            
    # Measure length across primary axis
    min_x, max_x, min_y, max_y, min_z, max_z = get_mesh_bounds(mesh_objs)
    dx = max_x - min_x
    dy = max_y - min_y
    dz = max_z - min_z
    
    primary_len = max(dx, dy, dz)
    print(f"  Raw Bounds: X={dx:.3f}, Y={dy:.3f}, Z={dz:.3f} -> Primary Length={primary_len:.3f}")
    
    # Calculate scale factor to target length in centimeters
    # (Unreal Engine: 1 unit = 1 cm)
    scale_factor = (target_len / 100.0) / primary_len
    print(f"  Applying Scale Factor: {scale_factor:.4f} (Target {target_len} cm)")
    
    for o in mesh_objs:
        o.scale = (o.scale.x * scale_factor, o.scale.y * scale_factor, o.scale.z * scale_factor)
        bpy.context.view_layer.objects.active = o
        o.select_set(True)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    
    # Check orientation: align longest dimension with World +X (Unreal Forward)
    min_x, max_x, min_y, max_y, min_z, max_z = get_mesh_bounds(mesh_objs)
    dx = max_x - min_x
    dy = max_y - min_y
    dz = max_z - min_z
    
    # If Y is longest, rotate 90 deg around Z
    if dy > dx and dy > dz:
        print("  Reorienting Y -> +X (Forward)...")
        for o in mesh_objs:
            o.rotation_euler = (0, 0, math.radians(-90))
        bpy.ops.object.select_all(action='SELECT')
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    elif dz > dx and dz > dy:
        print("  Reorienting Z -> +X (Forward)...")
        for o in mesh_objs:
            o.rotation_euler = (0, math.radians(90), 0)
        bpy.ops.object.select_all(action='SELECT')
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Center mesh at grip center / trigger guard
    min_x, max_x, min_y, max_y, min_z, max_z = get_mesh_bounds(mesh_objs)
    center_offset = mathutils.Vector((- (min_x + max_x) * 0.15, - (min_y + max_y) * 0.5, - (min_z + max_z) * 0.5))
    for o in mesh_objs:
        o.location += center_offset
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Detect / Separate Magazine
    mag_obj = None
    receiver_objs = []
    
    # If a separate magazine object was in the GLB (e.g. AR_4, Rifle_DMR)
    for o in mesh_objs:
        if "mag" in o.name.lower():
            mag_obj = o
        else:
            receiver_objs.append(o)
            
    # If no separate magazine object existed and a separate mag_raw is provided (e.g. NF-98)
    if not mag_obj and entry.get("mag_raw"):
        mag_path = os.path.join(RAW_DIR, entry["mag_raw"])
        if os.path.exists(mag_path):
            print(f"  Importing dedicated magazine: {entry['mag_raw']}")
            bpy.ops.import_scene.gltf(filepath=mag_path)
            new_mags = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o not in mesh_objs]
            if new_mags:
                mag_obj = new_mags[0]
                mag_obj.parent = None
                # Scale magazine consistently with receiver
                mag_obj.scale = (scale_factor, scale_factor, scale_factor)
                bpy.context.view_layer.objects.active = mag_obj
                bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
                # Position at magwell socket
                mag_obj.location = mathutils.Vector((entry["grip_offsets"]["magwell"]))
                bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # If it's a pistol or shotgun without a separate magazine, generate a discrete matching magazine mesh
    if not mag_obj and entry["archetype"] in ["Pistol", "Shotgun"]:
        print(f"  Constructing precision detachable magazine for {wname}...")
        # Create a rectangular box matching standard mag dimensions
        if entry["archetype"] == "Pistol":
            bpy.ops.mesh.primitive_cube_add(size=1.0)
            mag_obj = bpy.context.active_object
            mag_obj.name = f"{wid}_Mag"
            # 9mm / .45 standard magazine dimensions in cm: X=3.2cm, Y=2.0cm, Z=12.5cm
            mag_obj.scale = (0.032, 0.020, 0.125)
            mag_obj.rotation_euler = (0, math.radians(-12.0), 0)
            magwell_loc = entry["grip_offsets"]["magwell"]
            mag_obj.location = (magwell_loc[0], magwell_loc[1], magwell_loc[2] + 4.0)
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        elif entry["archetype"] == "Shotgun":
            bpy.ops.mesh.primitive_cylinder_add(radius=0.011, depth=0.068)
            mag_obj = bpy.context.active_object
            mag_obj.name = f"{wid}_Shell"
            mag_obj.rotation_euler = (0, math.radians(90.0), 0)
            magwell_loc = entry["grip_offsets"]["magwell"]
            mag_obj.location = (magwell_loc[0], magwell_loc[1], magwell_loc[2])
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Set Magazine Origin to Top Feed Lips (0, 0, 0)
    if mag_obj:
        print("  Aligning Magazine local origin to feed lips (0,0,0)...")
        mag_bbox = [mag_obj.matrix_world @ mathutils.Vector(c) for c in mag_obj.bound_box]
        top_z = max(v.z for v in mag_bbox)
        center_x = sum(v.x for v in mag_bbox) / len(mag_bbox)
        center_y = sum(v.y for v in mag_bbox) / len(mag_bbox)
        
        # Set 3D cursor to top feed lips
        bpy.context.scene.cursor.location = (center_x, center_y, top_z)
        bpy.context.view_layer.objects.active = mag_obj
        mag_obj.select_set(True)
        bpy.ops.object.origin_set(type='ORIGIN_CURSOR', center='MEDIAN')
        # Center magazine at feed lips (0, 0, 0)
        mag_obj.location = (0.0, 0.0, 0.0)

    # Join Receiver meshes if multiple
    bpy.ops.object.select_all(action='DESELECT')
    if receiver_objs:
        for o in receiver_objs:
            o.select_set(True)
        bpy.context.view_layer.objects.active = receiver_objs[0]
        if len(receiver_objs) > 1:
            bpy.ops.object.join()
        receiver_obj = bpy.context.active_object
        receiver_obj.name = f"SM_{wid}_Receiver"
    else:
        receiver_obj = mesh_objs[0]
        receiver_obj.name = f"SM_{wid}_Receiver"

    # Embed Sockets into Metadata
    sockets = {}
    for sname, sdata in entry["grip_offsets"].items():
        skey = f"Socket_{sname.title().replace('_', '')}"
        if len(sdata) == 3:
            sockets[skey] = {"loc": sdata, "rot": [0.0, 0.0, 0.0]}
        elif len(sdata) == 6:
            sockets[skey] = {"loc": sdata[:3], "rot": sdata[3:]}

    # Export Receiver FBX
    receiver_fbx = os.path.join(OUT_DIR, f"SM_{wid}_Receiver.fbx")
    bpy.ops.object.select_all(action='DESELECT')
    receiver_obj.select_set(True)
    bpy.context.view_layer.objects.active = receiver_obj
    bpy.ops.export_scene.fbx(
        filepath=receiver_fbx,
        use_selection=True,
        global_scale=1.0,
        apply_unit_scale=True,
        axis_forward='X',
        axis_up='Z',
        object_types={'MESH'},
        mesh_smooth_type='FACE',
        bake_anim=False
    )
    print(f"  [EXPORT] Receiver -> {receiver_fbx}")

    # Export Magazine FBX (if available)
    if mag_obj:
        mag_obj.name = f"SM_{wid}_Mag"
        mag_fbx = os.path.join(OUT_DIR, f"SM_{wid}_Mag.fbx")
        bpy.ops.object.select_all(action='DESELECT')
        mag_obj.select_set(True)
        bpy.context.view_layer.objects.active = mag_obj
        bpy.ops.export_scene.fbx(
            filepath=mag_fbx,
            use_selection=True,
            global_scale=1.0,
            apply_unit_scale=True,
            axis_forward='X',
            axis_up='Z',
            object_types={'MESH'},
            mesh_smooth_type='FACE',
            bake_anim=False
        )
        print(f"  [EXPORT] Magazine -> {mag_fbx}")

    # Export JSON Sidecar Metadata
    meta_path = os.path.join(OUT_DIR, f"SM_{wid}.meta.json")
    meta_data = {
        "id": wid,
        "name": wname,
        "archetype": entry["archetype"],
        "caliber": entry["caliber"],
        "target_length_cm": target_len,
        "receiver_mesh": f"SM_{wid}_Receiver.fbx",
        "magazine_mesh": f"SM_{wid}_Mag.fbx" if mag_obj else None,
        "stats": entry["stats"],
        "sockets": sockets
    }
    with open(meta_path, "w", encoding="utf-8") as f:
        json.dump(meta_data, f, indent=2)
    print(f"  [METADATA] Sidecar -> {meta_path}")

def main():
    print(f"[TACTICAL ARSENAL] Starting processing of {len(WEAPON_CATALOG)} tactical weapons...")
    for entry in WEAPON_CATALOG:
        process_weapon(entry)
    print("\n[TACTICAL ARSENAL] All 11 weapons processed, scaled, partitioned, and exported with metadata!")

if __name__ == "__main__":
    main()
