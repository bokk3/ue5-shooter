import unreal

sk = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if sk:
    bounds = sk.get_bounds()
    print("=========================================")
    print(f"SK_BaseMale Bounds:")
    print(f"Origin: {bounds.origin}")
    print(f"Box Extent: {bounds.box_extent}")
    print(f"Sphere Radius: {bounds.sphere_radius}")
    print(f"Estimated Height (Z extent * 2): {bounds.box_extent.z * 2} cm")
    print("=========================================")
else:
    print("ERROR: SK_BaseMale not found!")
