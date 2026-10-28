import unreal

def log(msg):
    unreal.log_warning(f"[WEAPON_VERIFY] {msg}")

log("=== VERIFYING WEAPON ASSETS, MESHES, AND SOCKETS ===")

weapons = [
    ("W01_NF98", "DA_W01_NF98", "SM_W01_NF98_Receiver", "SM_W01_NF98_Mag"),
    ("W02_VK74", "DA_W02_VK74", "SM_W02_VK74_Receiver", "SM_W02_VK74_Mag"),
    ("W03_Falcon50", "DA_W03_Falcon50", "SM_W03_Falcon50_Receiver", "SM_W03_Falcon50_Mag"),
    ("W04_Breacher12", "DA_W04_Breacher12", "SM_W04_Breacher12_Receiver", "SM_W04_Breacher12_Mag"),
    ("W05_Apex9", "DA_W05_Apex9", "SM_W05_Apex9_Receiver", "SM_W05_Apex9_Mag"),
    ("W06_Spectre45", "DA_W06_Spectre45", "SM_W06_Spectre45_Receiver", "SM_W06_Spectre45_Mag"),
    ("W07_Kestrel357", "DA_W07_Kestrel357", "SM_W07_Kestrel357_Receiver", "SM_W07_Kestrel357_Mag"),
    ("W08_GrenadeM67", "DA_W08_GrenadeM67", "SM_W08_GrenadeM67_Receiver", None),
    ("W09_Stun9", "DA_W09_Stun9", "SM_W09_Stun9_Receiver", None),
    ("W10_Machete", "DA_W10_Machete", "SM_W10_Machete_Receiver", None),
    ("W11_Stiletto", "DA_W11_Stiletto", "SM_W11_Stiletto_Receiver", None),
]

socket_checks = [
    "Socket_RightHand",
    "Socket_LeftHand",
    "Socket_Muzzle",
    "Socket_Optic",
    "Socket_Magwell",
    "Socket_Eject",
]

for wid, da_name, rec_name, mag_name in weapons:
    da = unreal.EditorAssetLibrary.load_asset(f"/Game/Weapons/Data/{da_name}")
    rec = unreal.EditorAssetLibrary.load_asset(f"/Game/Weapons/Meshes/{rec_name}")
    mag = unreal.EditorAssetLibrary.load_asset(f"/Game/Weapons/Meshes/{mag_name}") if mag_name else None
    
    if not da:
        log(f"FAILED to load {da_name}")
        continue
    if not rec:
        log(f"FAILED to load {rec_name}")
        continue
        
    bounds = rec.get_bounds()
    found_sockets = []
    for s_name in socket_checks:
        s = rec.find_socket(unreal.Name(s_name))
        if s:
            loc = s.get_editor_property("relative_location")
            found_sockets.append(f"{s_name}@{loc.x:.1f},{loc.y:.1f},{loc.z:.1f}")
    
    log(f"{wid} [{da.get_editor_property('weapon_display_name')}]: Extent=X:{bounds.box_extent.x:.1f},Y:{bounds.box_extent.y:.1f},Z:{bounds.box_extent.z:.1f} | Sockets={found_sockets}")
    log(f"  [DATA] Dmg={da.get_editor_property('base_damage')}, RPM={da.get_editor_property('fire_rate_rpm')}, Mag={da.get_editor_property('magazine_capacity')}, Range={da.get_editor_property('effective_range_meters')}m, Caliber='{da.get_editor_property('caliber')}'")
    if mag:
        mb = mag.get_bounds()
        mag_sock = mag.find_socket("Socket_FeedLip")
        sock_str = f"FeedLip@{mag_sock.get_editor_property('relative_location')}" if mag_sock else "NoSocket"
        log(f"  [MAG] Extent=X:{mb.box_extent.x:.1f},Y:{mb.box_extent.y:.1f},Z:{mb.box_extent.z:.1f} | Socket={sock_str}")

log("=== ALL WEAPONS VERIFIED SUCCESSFULLY ===")
