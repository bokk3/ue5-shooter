
import unreal

world = unreal.EditorLevelLibrary.get_editor_world()
print('Editor world:', world)

# Load meshes
soldier_mesh = unreal.EditorAssetLibrary.load_asset('/Game/Characters/BaseMale/SM_Soldier_Posed')
plinth_mesh = unreal.EditorAssetLibrary.load_asset('/Game/Environment/Hangar/SM_Hangar_Plinth')
floor_mesh = unreal.EditorAssetLibrary.load_asset('/Game/Environment/Hangar/SM_Hangar_Floor')

print('Meshes loaded:', bool(soldier_mesh), bool(plinth_mesh), bool(floor_mesh))
