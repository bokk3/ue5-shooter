import unreal

print("=== STARTING COASTAL PBR MATERIAL & MESH IMPORT PIPELINE ===")

mat_dir = "/Game/Environment/Coastal/Materials"
mesh_dir = "/Game/Environment/Coastal/Meshes"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_editing = unreal.MaterialEditingLibrary

for d in [mat_dir, mesh_dir]:
    if not unreal.EditorAssetLibrary.does_directory_exist(d):
        unreal.EditorAssetLibrary.make_directory(d)

def create_pbr_material(mat_name, base_color, roughness, metallic, ambient_scale, emissive_tint=None):
    full_path = f"{mat_dir}/{mat_name}"
    mat = unreal.EditorAssetLibrary.load_asset(full_path)
    if not mat:
        mat_factory = unreal.MaterialFactoryNew()
        mat = asset_tools.create_asset(mat_name, mat_dir, unreal.Material, mat_factory)

    try:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
        mat.set_editor_property("two_sided", "Grass" in mat_name)
    except Exception as e:
        pass

    # Base Color
    col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -200)
    col_param.set_editor_property("parameter_name", "BaseColor")
    col_param.set_editor_property("default_value", base_color)
    mat_editing.connect_material_property(col_param, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Roughness
    rough_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 50)
    rough_param.set_editor_property("parameter_name", "Roughness")
    rough_param.set_editor_property("default_value", roughness)
    mat_editing.connect_material_property(rough_param, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Metallic
    metal_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 120)
    metal_param.set_editor_property("parameter_name", "Metallic")
    metal_param.set_editor_property("default_value", metallic)
    mat_editing.connect_material_property(metal_param, "", unreal.MaterialProperty.MP_METALLIC)

    # Ambient Fill Emissive
    amb_scale = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 200)
    amb_scale.set_editor_property("parameter_name", "AmbientFillScale")
    amb_scale.set_editor_property("default_value", ambient_scale)

    amb_mult = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 200)
    mat_editing.connect_material_expressions(col_param, "", amb_mult, "A")
    mat_editing.connect_material_expressions(amb_scale, "", amb_mult, "B")

    if emissive_tint:
        emis_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 300)
        emis_param.set_editor_property("parameter_name", "EmissiveAdd")
        emis_param.set_editor_property("default_value", emissive_tint)

        add_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionAdd, 0, 250)
        mat_editing.connect_material_expressions(amb_mult, "", add_node, "A")
        mat_editing.connect_material_expressions(emis_param, "", add_node, "B")
        mat_editing.connect_material_property(add_node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        mat_editing.connect_material_property(amb_mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat_editing.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    unreal.log_warning(f"[MAT] Created {mat_name}")
    return mat

mat_concrete = create_pbr_material("M_Coastal_Concrete", unreal.LinearColor(0.24, 0.27, 0.31, 1.0), 0.55, 0.05, 0.18)
mat_metal = create_pbr_material("M_Coastal_Metal", unreal.LinearColor(0.08, 0.09, 0.11, 1.0), 0.32, 0.92, 0.14)
mat_timber = create_pbr_material("M_Coastal_Timber", unreal.LinearColor(0.16, 0.12, 0.09, 1.0), 0.72, 0.02, 0.18)
mat_roof = create_pbr_material("M_Coastal_RoofMetal", unreal.LinearColor(0.18, 0.22, 0.26, 1.0), 0.42, 0.85, 0.15)
mat_grass = create_pbr_material("M_Coastal_DuneGrass", unreal.LinearColor(0.32, 0.38, 0.22, 1.0), 0.65, 0.02, 0.22)
mat_sand = create_pbr_material("M_Coastal_WetSand", unreal.LinearColor(0.38, 0.35, 0.28, 1.0), 0.18, 0.02, 0.18)
mat_water = create_pbr_material("M_Coastal_OceanWater", unreal.LinearColor(0.08, 0.18, 0.22, 1.0), 0.04, 0.10, 0.15, unreal.LinearColor(0.04, 0.08, 0.10, 1.0))

# Import Static Meshes
import os

project_root = "C:/Users/Boris/Documents/antigravity/noble-fermi"
fbx_files = [
    ("SM_Seawall_Curved", f"{project_root}/Content/Environment/Coastal/Meshes/SM_Seawall_Curved.fbx", [mat_concrete, mat_metal]),
    ("SM_Tetrapod_Concrete", f"{project_root}/Content/Environment/Coastal/Meshes/SM_Tetrapod_Concrete.fbx", [mat_concrete]),
    ("SM_Pier_Watchtower", f"{project_root}/Content/Environment/Coastal/Meshes/SM_Pier_Watchtower.fbx", [mat_timber, mat_roof]),
    ("SM_Marram_Grass_Clump", f"{project_root}/Content/Environment/Coastal/Meshes/SM_Marram_Grass_Clump.fbx", [mat_grass]),
]

for name, abs_fbx, mats in fbx_files:
    if not os.path.exists(abs_fbx):
        unreal.log_error(f"File not found: {abs_fbx}")
        continue

    task = unreal.AssetImportTask()
    task.filename = abs_fbx
    task.destination_path = mesh_dir
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = True

    opts = unreal.FbxImportUI()
    opts.import_mesh = True
    opts.import_as_skeletal = False
    opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    opts.automated_import_should_detect_type = False
    opts.import_animations = False
    opts.import_materials = False
    opts.import_textures = False
    opts.static_mesh_import_data.combine_meshes = True
    opts.static_mesh_import_data.generate_lightmap_u_vs = True
    task.options = opts

    asset_tools.import_asset_tasks([task])
    sm = unreal.EditorAssetLibrary.load_asset(f"{mesh_dir}/{name}")
    if sm:
        for idx, m in enumerate(mats):
            if m:
                sm.set_material(idx, m)
        unreal.EditorAssetLibrary.save_loaded_asset(sm, only_if_is_dirty=False)
        unreal.log_warning(f"[IMPORT] Successfully imported and assigned materials to {name}")

print("=== COASTAL PIPELINE FINISHED ===")
