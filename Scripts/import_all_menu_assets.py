import unreal
import os
import sys

print(">>> IMPORTING MENU ENVIRONMENT & POSED SOLDIER ASSETS INTO UNREAL ENGINE 5.8 <<<")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
proj_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi"

mat_body = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody")
mat_visor = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_Soldier_Visor")
mat_steel = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_AK47_Steel")
mat_wood = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_AK47_Wood")
mat_bake = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_AK47_Bakelite")

def import_static_mesh(fbx_rel_path, dest_pkg, dest_name, materials_list):
    fbx_abs = os.path.join(proj_dir, fbx_rel_path)
    if not os.path.exists(fbx_abs):
        print(f"ERROR: FBX not found: {fbx_abs}")
        return None

    task = unreal.AssetImportTask()
    task.filename = fbx_abs
    task.destination_path = dest_pkg
    task.destination_name = dest_name
    task.replace_existing = True
    task.automated = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.automated_import_should_detect_type = False
    options.import_animations = False
    options.import_materials = False
    options.import_textures = False
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.generate_lightmap_u_vs = True
    task.options = options

    asset_tools.import_asset_tasks([task])

    asset_path = f"{dest_pkg}/{dest_name}"
    mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
    if mesh:
        for idx, m in enumerate(materials_list):
            if m:
                mesh.set_material(idx, m)
                print(f"  {dest_name} Slot {idx} -> {m.get_name()}")
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
        print(f"Successfully imported and saved: {asset_path}")
    else:
        print(f"FAILED to load imported mesh: {asset_path}")
    return mesh

# 1. Import SM_Soldier_Posed
import_static_mesh(
    r"Content\Characters\BaseMale\SM_Soldier_Posed.fbx",
    "/Game/Characters/BaseMale",
    "SM_Soldier_Posed",
    [mat_body, mat_visor, mat_steel, mat_wood, mat_bake]
)

# 2. Import SM_Hangar_Plinth
import_static_mesh(
    r"Content\Environment\Hangar\SM_Hangar_Plinth.fbx",
    "/Game/Environment/Hangar",
    "SM_Hangar_Plinth",
    [mat_steel, mat_visor]
)

# 3. Import SM_Hangar_Floor
import_static_mesh(
    r"Content\Environment\Hangar\SM_Hangar_Floor.fbx",
    "/Game/Environment/Hangar",
    "SM_Hangar_Floor",
    [mat_steel]
)

# 4. Import SM_Hangar_Wall
import_static_mesh(
    r"Content\Environment\Hangar\SM_Hangar_Wall.fbx",
    "/Game/Environment/Hangar",
    "SM_Hangar_Wall",
    [mat_steel]
)

unreal.EditorAssetLibrary.save_directory("/Game/Characters/BaseMale", only_if_is_dirty=False, recursive=True)
unreal.EditorAssetLibrary.save_directory("/Game/Environment/Hangar", only_if_is_dirty=False, recursive=True)
print("=== MENU ASSETS IMPORT COMPLETE ===")
sys.exit(0)
