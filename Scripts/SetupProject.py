"""
Noble Fermi - UE5 Start Menu Project Scaffolding & Setup Script
Executes inside Unreal Editor (UE 5.8) to configure directories, project settings, and asset placeholders.
"""
import unreal

def setup_project():
    unreal.log("===================================================================")
    unreal.log("Noble Fermi: Initializing AAA FPS Start Menu & Systems Scaffolding")
    unreal.log("===================================================================")
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    
    # Required folder structures for AAA FPS Menu & Game
    folders = [
        "/Game/UI",
        "/Game/UI/Widgets",
        "/Game/UI/Styles",
        "/Game/UI/Fonts",
        "/Game/UI/Audio",
        "/Game/UI/Icons",
        "/Game/Audio",
        "/Game/Audio/SoundClasses",
        "/Game/Audio/SoundMixes",
        "/Game/Maps",
        "/Game/Blueprints",
        "/Game/Blueprints/Core",
        "/Game/Weapons",
        "/Game/Characters",
        "/Game/Input"
    ]
    
    for folder in folders:
        if not editor_asset_subsystem.does_directory_exist(folder):
            editor_asset_subsystem.make_directory(folder)
            unreal.log(f"[NobleFermi] Created directory: {folder}")
        else:
            unreal.log(f"[NobleFermi] Directory verified: {folder}")
            
    unreal.log("-------------------------------------------------------------------")
    unreal.log("Folder hierarchies verified.")
    unreal.log("C++ Classes Available:")
    unreal.log(" - UMainMenuWidget (UCommonActivatableWidget)")
    unreal.log(" - USettingsMenuWidget (Video, Scalability, Sound, FOV, Reticle)")
    unreal.log(" - UFPSButtonBase (CommonUI gamepad-ready button)")
    unreal.log(" - UFPSKeybindSubsystem (Enhanced Input remapping & conflict detection)")
    unreal.log(" - UFPSAudioSubsystem (Persistent SoundClass volume management)")
    unreal.log(" - UFPSBenchmarkSubsystem (Hardware auto-detect & benchmark)")
    unreal.log(" - AFPSMenuDioramaActor (3D hangar/scene director & turntable)")
    unreal.log(" - AFPSMenuGameMode & AFPSMenuPlayerController")
    unreal.log("===================================================================")

if __name__ == "__main__":
    setup_project()
