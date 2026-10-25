import unreal

print(">>> CHECKING NANITE SETTINGS ON STATIC MESHES <<<")
for path in [
    "/Game/Characters/BaseMale/SM_Soldier_Posed",
    "/Game/Environment/Hangar/SM_Hangar_Plinth",
    "/Game/Environment/Hangar/SM_Hangar_Floor"
]:
    sm = unreal.EditorAssetLibrary.load_asset(path)
    if sm:
        nanite_settings = sm.get_editor_property("nanite_settings")
        is_nanite = nanite_settings.get_editor_property("enabled")
        print(f"Mesh: {path}")
        print(f"  Nanite Enabled: {is_nanite}")
    else:
        print(f"Could not load: {path}")

print(">>> NANITE CHECK COMPLETE <<<")
