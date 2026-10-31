import unreal
import os

print("=== STARTING HANGAR PBR MATERIAL & MESH IMPORT PIPELINE ===")

mat_dir = "/Game/Environment/Hangar/Materials"
mesh_dir = "/Game/Environment/Hangar/Meshes"
hangar_root_dir = "/Game/Environment/Hangar"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_editing = unreal.MaterialEditingLibrary

for d in [mat_dir, mesh_dir, hangar_root_dir]:
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

# Create Hangar PBR Materials
mat_c17_fuselage = create_pbr_material("M_CargoJet_Fuselage", unreal.LinearColor(0.20, 0.23, 0.27, 1.0), 0.52, 0.25, 0.16)
mat_c17_details  = create_pbr_material("M_CargoJet_Details",  unreal.LinearColor(0.06, 0.08, 0.10, 1.0), 0.15, 0.85, 0.12)
mat_gantry_yellow = create_pbr_material("M_Hangar_GantryYellow", unreal.LinearColor(0.85, 0.62, 0.06, 1.0), 0.42, 0.18, 0.22)
mat_gantry_steel  = create_pbr_material("M_Hangar_StructuralSteel", unreal.LinearColor(0.11, 0.13, 0.15, 1.0), 0.36, 0.85, 0.14)
mat_turntable_steel = create_pbr_material("M_Turntable_Steel", unreal.LinearColor(0.14, 0.17, 0.20, 1.0), 0.28, 0.88, 0.15)
mat_turntable_neon  = create_pbr_material("M_Turntable_CyanNeon", unreal.LinearColor(0.0, 0.85, 1.0, 1.0), 0.08, 0.05, 1.0, unreal.LinearColor(0.0, 4.5, 6.0, 1.0))
mat_turntable_deck  = create_pbr_material("M_Turntable_Deck", unreal.LinearColor(0.08, 0.09, 0.11, 1.0), 0.68, 0.15, 0.14)

# Import FBX Meshes
project_root = "C:/Users/Boris/Documents/antigravity/noble-fermi"
fbx_tasks = [
    ("SM_Cargo_Jet_C17", f"{project_root}/Content/Environment/Hangar/Meshes/SM_Cargo_Jet_C17.fbx", mesh_dir, [mat_c17_fuselage, mat_c17_details]),
    ("SM_Hangar_GantryCrane", f"{project_root}/Content/Environment/Hangar/Meshes/SM_Hangar_GantryCrane.fbx", mesh_dir, [mat_gantry_yellow, mat_gantry_steel]),
    ("SM_Hangar_Turntable", f"{project_root}/Content/Environment/Hangar/Meshes/SM_Hangar_Turntable.fbx", mesh_dir, [mat_turntable_steel, mat_turntable_neon, mat_turntable_deck]),
    ("SM_Hangar_Plinth", f"{project_root}/Content/Environment/Hangar/SM_Hangar_Plinth.fbx", hangar_root_dir, [mat_turntable_steel, mat_turntable_neon, mat_turntable_deck]),
]

for name, abs_fbx, dest_pkg_dir, mats in fbx_tasks:
    if not os.path.exists(abs_fbx):
        unreal.log_error(f"File not found: {abs_fbx}")
        continue

    task = unreal.AssetImportTask()
    task.filename = abs_fbx
    task.destination_path = dest_pkg_dir
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
    sm = unreal.EditorAssetLibrary.load_asset(f"{dest_pkg_dir}/{name}")
    if sm:
        for idx, m in enumerate(mats):
            if m:
                sm.set_material(idx, m)
        unreal.EditorAssetLibrary.save_loaded_asset(sm, only_if_is_dirty=False)
        unreal.log_warning(f"[IMPORT] Successfully imported and assigned materials to {name} in {dest_pkg_dir}")

print("=== HANGAR PIPELINE FINISHED SUCCESSFULLY ===")
