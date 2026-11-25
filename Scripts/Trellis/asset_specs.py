"""How each Trellis model is oriented, scaled and imported (read by clean_assets.py and import_assets.py).

Trellis models come normalized to ~1 m in glTF space (+Y up, the source image's view at +Z) with an arbitrary yaw.
Each spec names, in those glTF axes, the model's forward (becomes UE +X: muzzle, vehicle nose, the face a prop
shows) and up (becomes UE +Z), and one real-world dimension in cm that sets the scale.

fwd / up: "+X" "-Z" ... a glTF axis; "pca:+X+Y" the longest axis, signed toward the hint (blades lying at an
          angle); "pca_h:+X" the longest horizontal axis (boats yawed off-axis). up "flat:+Y" = the thinnest axis
          (sheets generated tilted).
size:     ("L"|"W"|"H"|"M", cm) after orienting: UE X length, Y width, Z height, or the largest of the three.
kind:     held   - weapons, attachments, gear: pivot at the bounds centre, no collision, no Nanite
          pickup - items lying in the world: pivot bottom centre, box collision, no Nanite
          prop   - world props: pivot bottom centre, convex collision, Nanite
          big    - buildings, vehicles, structures: pivot bottom centre, complex-as-simple collision, Nanite
"""

S = {}


def spec(name, kind, size, fwd="+Z", up="+Y", note=""):
    S[name] = {"kind": kind, "size": size, "fwd": fwd, "up": up, "note": note}


# 01 Weapons (barrel along UE +X)
spec("NF98_Carbine", "held", ("L", 84), fwd="+Z", note="has a magazine baked in")
spec("VK74_Rifle", "held", ("L", 94), fwd="+X", note="has a magazine baked in")
spec("Falcon50_Sniper", "held", ("L", 145), fwd="+X", note="has a magazine baked in")
spec("Breacher12_Shotgun", "held", ("L", 100), fwd="+X", note="box magazine instead of a tube")
spec("Apex9_SMG", "held", ("L", 68), fwd="-Z", note="has a magazine baked in")
spec("Spectre45_Pistol", "held", ("L", 21), fwd="-Z")
spec("Kestrel357_Revolver", "held", ("L", 24), fwd="+Z")
spec("M67_FragGrenade", "held", ("H", 9))
spec("Stun9_Flashbang", "held", ("H", 13))
spec("Machete", "held", ("L", 60), fwd="pca:+X+Y")
spec("Stiletto", "held", ("L", 24), fwd="pca:+X+Y", note="a folding knife, not a stiletto")
spec("Mag_NF98_30", "held", ("H", 19), fwd="+Z")
spec("Mag_VK74_30", "held", ("H", 21), fwd="-Z")
spec("Mag_Falcon50_5", "held", ("H", 15), fwd="+Z", note="three magazines side by side, not one box magazine")
spec("Mag_Apex9_30", "held", ("H", 22), fwd="-X")
spec("Mag_Spectre45_8", "held", ("H", 13), fwd="+X")
spec("ShotgunShell_12ga", "held", ("H", 7))
spec("Sheath_Machete", "held", ("H", 60))
spec("Sheath_Knife", "held", ("H", 30))

# 02 Attachments (muzzle direction along UE +X)
spec("RedDot", "held", ("L", 7), fwd="+Z")
spec("Scope_8x", "held", ("L", 36), fwd="+X")
spec("Suppressor_Rifle", "held", ("L", 18), fwd="-Z")
spec("Suppressor_Pistol", "held", ("L", 16), fwd="+Y", up="+Z")
spec("Bipod", "held", ("H", 25))
spec("Flashlight", "held", ("L", 14), fwd="+Y", up="+Z")
spec("IronSight_Rear", "held", ("L", 5), fwd="+X")
spec("IronSight_Front", "held", ("L", 4.5), fwd="+X")
spec("IronSight_Canted", "held", ("L", 5), fwd="+X")
spec("Mag_Drum_50", "held", ("H", 19), fwd="-X", up="-Z", note="a curved stick magazine, not a drum; generated lying down")
spec("Mag_Pistol_Extended_33", "held", ("H", 25))
spec("Sling", "held", ("M", 40))
spec("Holographic", "held", ("L", 12), fwd="+Z", note="a closed tube sight with a round lens, not an open window")
spec("Scope_4x", "held", ("L", 30), fwd="+X")
spec("Magnifier_3x", "held", ("M", 9), fwd="+Z")
spec("LaserModule", "held", ("L", 10), fwd="+Z")
spec("MuzzleBrake", "held", ("L", 6), fwd="+X", note="a plain tube with one slot per side")

# 03 Operator gear (front along UE +X, as worn)
spec("PlateCarrier", "held", ("H", 50))
spec("ChestRig", "held", ("H", 48))
spec("KneePad", "held", ("H", 28), note="has a helmet-like dome on top")
spec("ElbowPad", "held", ("H", 20))
spec("AssaultBackpack", "held", ("H", 48))
spec("NVG_QuadTube", "held", ("W", 23), note="generated on a helmet")
spec("NVG_Binocular", "held", ("W", 23), note="generated on a helmet")
spec("Headset", "held", ("W", 20))
spec("Goggles", "held", ("W", 19))
spec("ShootingGlasses", "held", ("W", 15))
spec("Helmet_HighCut_Tan", "held", ("L", 27))
spec("Helmet_HighCut_Black", "held", ("L", 27))
spec("TacticalCap", "held", ("L", 28))
spec("GasMask", "held", ("H", 25))
spec("Radio", "held", ("H", 22))
spec("ArmorPlate", "held", ("H", 30))
spec("ArmorShard", "pickup", ("M", 12))
spec("Glove", "held", ("H", 24))

# 04 Gameplay items
spec("AmmoCrate", "pickup", ("M", 80))
spec("AmmoCan", "pickup", ("M", 30))
spec("FirstAidKit", "pickup", ("M", 25), fwd="+X")
spec("ArmorPlateBag", "pickup", ("H", 45))
spec("AirdropCrate", "pickup", ("M", 120))
spec("FermiDecoy", "pickup", ("M", 18))
spec("TimeBubbleGrenade", "held", ("H", 10))
spec("TeleportBeacon", "pickup", ("H", 30))
spec("C4Charge", "pickup", ("H", 20), note="dynamite sticks, not a C4 brick")
spec("Claymore", "pickup", ("H", 22), note="a lantern-like cylinder, not a claymore")
spec("SmokeGrenade", "held", ("H", 15))
spec("DeployableShield", "prop", ("H", 110))
spec("ReconDrone", "pickup", ("M", 40))
spec("CapturePointBeacon", "prop", ("H", 250), note="looks like a street lamp")
spec("BombDevice", "pickup", ("M", 50))
spec("WeaponRack", "prop", ("H", 150), note="spiked bars rather than a rack")
spec("LoadoutLocker", "prop", ("H", 190))
spec("RuggedLaptop", "pickup", ("M", 36))
spec("FieldRadioStation", "prop", ("M", 60))
spec("TacticalTable", "prop", ("M", 150))

# 05 Favela
spec("House_BrickSmall", "big", ("H", 560))
spec("House_TwoStory", "big", ("H", 650))
spec("CinderWall_Broken", "prop", ("H", 200))
spec("RoofSheet_Rusty", "prop", ("M", 240), up="flat:+Y")
spec("WaterTank_Blue", "prop", ("H", 150))
spec("UtilityPole_Wires", "prop", ("H", 900))
spec("Staircase_Concrete", "big", ("H", 300), fwd="+X")
spec("Door_MetalWeathered", "prop", ("H", 210))
spec("Window_Barred", "prop", ("H", 150))
spec("Column_Rebar", "prop", ("H", 300))
spec("SatelliteDish", "prop", ("M", 80))
spec("ACUnit_Rusty", "prop", ("M", 90), note="mostly untextured grey, messy surface")
spec("PatioChair", "prop", ("H", 85))
spec("BeerCrates", "prop", ("H", 120))
spec("GasCylinder_LPG", "prop", ("H", 60))
spec("TrashBags", "prop", ("H", 70))
spec("Clothesline", "prop", ("H", 200))
spec("FoodStall", "big", ("H", 240))
spec("Motorcycle_Old", "prop", ("L", 190))
spec("Car_RustedHatchback", "big", ("L", 380))

# 06 Favela / Coastal
spec("OilDrum_Rusty", "prop", ("H", 88))
spec("ChainLinkFence", "prop", ("H", 190))
spec("StreetLamp", "prop", ("H", 400))
spec("PottedPlant", "prop", ("H", 110))
spec("FishingBoat_Beached", "big", ("L", 500), fwd="pca_h:+X")
spec("Pier_Section", "big", ("M", 400))
spec("Seawall_Stairs", "big", ("H", 250))
spec("Tetrapod", "prop", ("M", 250))
spec("LifeguardTower", "big", ("H", 350))
spec("BeachKiosk", "big", ("H", 300), note="roof floats above the counter, posts missing")
spec("Rocks_Cluster", "prop", ("M", 300))
spec("Driftwood", "prop", ("L", 300))
spec("PalmTree", "prop", ("H", 900))
spec("Buoy", "prop", ("H", 100))
spec("Anchor_Rusty", "prop", ("H", 120))
spec("CrabPots", "prop", ("H", 150), note="a shelf of crabs rather than stacked pots")
spec("DuneFence", "prop", ("H", 110))
spec("TireStack", "prop", ("H", 90))
spec("BoatRope_Coil", "prop", ("M", 60))
spec("SandbagWall", "prop", ("H", 90))

# 07 Industrial
spec("Container20_Red", "big", ("L", 606), fwd="+X")
spec("JerseyBarrier", "prop", ("H", 81), fwd="+X")
spec("HescoBarrier", "prop", ("H", 100))
spec("Forklift", "big", ("H", 220))
spec("Pallet", "prop", ("M", 120))
spec("Generator", "prop", ("M", 250))
spec("StorageTank", "big", ("H", 600))
spec("Pipe_Valve", "prop", ("H", 120))
spec("Catwalk", "big", ("H", 110))
spec("ControlCabinet", "prop", ("H", 200))
spec("IBeamStack", "prop", ("H", 120))
spec("Scaffolding", "big", ("H", 400))
spec("Staircase_Steel", "big", ("H", 300), fwd="-X")
spec("CraneHook", "prop", ("H", 150))
spec("Gate_BarbedWire", "prop", ("H", 250))
spec("Floodlight", "prop", ("M", 60))
spec("HazardBollard", "prop", ("H", 110), note="two bollards")
spec("Pallet_Boxes", "prop", ("M", 120), note="an empty pallet; the version with boxes kept timing out on the server")

# 08 Range / Hangar
spec("Target_Silhouette", "prop", ("H", 180))
spec("Target_Paper", "prop", ("H", 170))
spec("Target_Popper", "prop", ("H", 90))
spec("ShootHouse_Panel", "big", ("H", 240))
spec("ShootingBench", "prop", ("M", 180))
spec("DistanceMarker", "prop", ("H", 180))
spec("TireBarricade", "prop", ("M", 110), note="one large tyre")
spec("VaultBox", "prop", ("H", 75))
spec("Workbench", "prop", ("M", 180))
spec("ToolChest", "prop", ("M", 70))
spec("WeaponDisplayStand", "prop", ("H", 120))
spec("ArmorMannequinStand", "prop", ("H", 185))
spec("WeaponCases", "prop", ("H", 80))
spec("TransportHelicopter", "big", ("M", 1950))
spec("MilitaryJeep", "big", ("L", 340))
spec("JerryCan", "prop", ("H", 47))
spec("Whiteboard_Map", "prop", ("M", 180))
spec("CeilingLight", "prop", ("M", 60))
spec("FoldingChair", "prop", ("H", 85))
spec("SandbagStack", "prop", ("H", 90))

# Generated, but the wrong object: not imported, regenerate.
REJECTED = {
    "Speedloader_357": "a revolver with a speedloader, not a speedloader",
    "BattleBelt": "a full vest, not a belt",
    "DropLegHolster": "a pair of trousers, not a holster",
    "Container40_Blue": "a near-cube box, not a 40 ft container",
    "FlashHider": "a birdcage, then a stepped tube with a loose end cap; no prongs",
    "Grip_Vertical": "a pistol-like receiver, then a pistol; 'grip' prompts give pistols",
    "Grip_Angled": "a pistol twice; 'grip' prompts give pistols",
    "Container_Open": "a short box with doors on its long side; the regenerate timed out at 30 min",
}
