import unreal

unreal.log_warning("==================================================")
unreal.log_warning(">>> BUILDING M_BreathVapor TRANSLUCENT MATERIAL <<<")
unreal.log_warning("==================================================")

materials_path = "/Game/Characters/BaseMale/Materials"
mat_name = "M_BreathVapor"
full_path = f"{materials_path}/{mat_name}"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mat_editing = unreal.MaterialEditingLibrary

mat = unreal.EditorAssetLibrary.load_asset(full_path)
if not mat:
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(mat_name, materials_path, unreal.Material, mat_factory)

# Configure Shading Model & Blend Mode
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)

# 1. Emissive Color: Frosty cool vapor tint multiplied by vertex RGB
col_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -200)
col_param.set_editor_property("parameter_name", "VaporColor")
col_param.set_editor_property("default_value", unreal.LinearColor(0.88, 0.94, 1.0, 1.0))

vert_col = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -400, 50)

mult_emissive = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, -100)
mat_editing.connect_material_expressions(col_param, "", mult_emissive, "A")
mat_editing.connect_material_expressions(vert_col, "RGB", mult_emissive, "B")
mat_editing.connect_material_property(mult_emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# 2. Soft Gaussian Radial Gradient for Opacity
# Radial distance = distance(UV, (0.5, 0.5))
tex_coord = mat_editing.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 250)

center_const = mat_editing.create_material_expression(mat, unreal.MaterialExpressionConstant2Vector, -900, 400)
center_const.set_editor_property("r", 0.5)
center_const.set_editor_property("g", 0.5)

dist_expr = mat_editing.create_material_expression(mat, unreal.MaterialExpressionDistance, -700, 300)
mat_editing.connect_material_expressions(tex_coord, "", dist_expr, "A")
mat_editing.connect_material_expressions(center_const, "", dist_expr, "B")

two_const = mat_editing.create_material_expression(mat, unreal.MaterialExpressionConstant, -700, 480)
two_const.set_editor_property("r", 2.0)

mult_dist = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -520, 320)
mat_editing.connect_material_expressions(dist_expr, "", mult_dist, "A")
mat_editing.connect_material_expressions(two_const, "", mult_dist, "B")

one_const = mat_editing.create_material_expression(mat, unreal.MaterialExpressionConstant, -520, 200)
one_const.set_editor_property("r", 1.0)

sub_expr = mat_editing.create_material_expression(mat, unreal.MaterialExpressionSubtract, -350, 220)
mat_editing.connect_material_expressions(one_const, "", sub_expr, "A")
mat_editing.connect_material_expressions(mult_dist, "", sub_expr, "B")

clamp_expr = mat_editing.create_material_expression(mat, unreal.MaterialExpressionClamp, -180, 220)
clamp_expr.set_editor_property("min_default", 0.0)
clamp_expr.set_editor_property("max_default", 1.0)
mat_editing.connect_material_expressions(sub_expr, "", clamp_expr, "Input")

# Smooth falloff: clamp^2
smooth_expr = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, 0, 220)
mat_editing.connect_material_expressions(clamp_expr, "", smooth_expr, "A")
mat_editing.connect_material_expressions(clamp_expr, "", smooth_expr, "B")

density_param = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, 0, 400)
density_param.set_editor_property("parameter_name", "VaporDensity")
density_param.set_editor_property("default_value", 0.75)

mult_alpha1 = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, 180, 250)
mat_editing.connect_material_expressions(smooth_expr, "", mult_alpha1, "A")
mat_editing.connect_material_expressions(vert_col, "A", mult_alpha1, "B")

mult_alpha2 = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, 350, 280)
mat_editing.connect_material_expressions(mult_alpha1, "", mult_alpha2, "A")
mat_editing.connect_material_expressions(density_param, "", mult_alpha2, "B")

mat_editing.connect_material_property(mult_alpha2, "", unreal.MaterialProperty.MP_OPACITY)

mat_editing.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
unreal.log_warning("[MAT] Successfully compiled and saved M_BreathVapor!")
