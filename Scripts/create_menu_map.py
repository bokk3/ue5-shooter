import unreal

print(">>> CREATING DEDICATED MENU MAP <<<")
level_path = "/Game/Maps/MenuMap"

# Ensure /Game/Maps directory exists
if not unreal.EditorAssetLibrary.does_directory_exist("/Game/Maps"):
    unreal.EditorAssetLibrary.make_directory("/Game/Maps")

# Create new empty level
success = unreal.EditorLevelLibrary.new_level(level_path)
print(f"new_level returned: {success}")

if success:
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world:
        world_settings = world.get_world_settings()
        if world_settings:
            # Set default game mode to FPSMenuGameMode
            gm_class = unreal.EditorAssetLibrary.load_blueprint_class("/Script/NobleFermi.FPSMenuGameMode")
            if not gm_class:
                gm_class = unreal.load_class(None, "/Script/NobleFermi.FPSMenuGameMode")
            print(f"Loaded game mode class: {gm_class}")
            if gm_class:
                world_settings.set_editor_property("default_game_mode", gm_class)
                print("Assigned FPSMenuGameMode to MenuMap world settings.")

        saved = unreal.EditorLevelLibrary.save_current_level()
        print(f"Level saved: {saved}")
else:
    # Fallback to WorldFactory
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.WorldFactory()
    new_world = asset_tools.create_asset("MenuMap", "/Game/Maps", unreal.World, factory)
    if new_world:
        unreal.EditorAssetLibrary.save_loaded_asset(new_world)
        print("Created and saved MenuMap via WorldFactory.")

print(f"MenuMap exists: {unreal.EditorAssetLibrary.does_asset_exist(level_path)}")
print(">>> MENU MAP CREATION COMPLETE <<<")
