import unreal
import os

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
fbx_abs = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Characters\BaseMale\test_scale_meters.fbx"

task = unreal.AssetImportTask()
task.filename = fbx_abs
task.destination_path = "/Game/Characters/BaseMale"
task.destination_name = "SM_Test_Scale"
task.replace_existing = True
task.automated = True
task.save = False

options = unreal.FbxImportUI()
options.import_mesh = True
options.import_as_skeletal = False
options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
options.automated_import_should_detect_type = False
options.import_animations = False
options.import_materials = False
options.import_textures = False
task.options = options

asset_tools.import_asset_tasks([task])

m = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SM_Test_Scale")
if m:
    b = m.get_bounds()
    unreal.log_warning(f"[TEST_SCALE] Origin={b.origin}, Extent={b.box_extent}, Radius={b.sphere_radius}")
else:
    unreal.log_warning("[TEST_SCALE] Failed to load SM_Test_Scale")
