import unreal

print("=== TUNING M_SOLDIER_VISOR TO ELECTRIC CYAN (#00E5FF) ===")

mat_path = "/Game/Characters/BaseMale/Materials/M_Soldier_Visor"
mat = unreal.EditorAssetLibrary.load_asset(mat_path)
if not mat:
    print(f"ERROR: Could not load {mat_path}")
    exit(1)

mat_editing = unreal.MaterialEditingLibrary

# Clear existing property connections on BaseColor and EmissiveColor
# Create clean BaseColor constant vector (0.01, 0.08, 0.12)
base_col = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -300, -200)
base_col.set_editor_property("parameter_name", "BaseColor")
base_col.set_editor_property("default_value", unreal.LinearColor(0.01, 0.08, 0.12, 1.0))
mat_editing.connect_material_property(base_col, "", unreal.MaterialProperty.MP_BASE_COLOR)

# Emissive: (0.0, 0.898, 1.0) * 1.6
emissive_col = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 100)
emissive_col.set_editor_property("parameter_name", "EmissiveColor")
emissive_col.set_editor_property("default_value", unreal.LinearColor(0.0, 0.898, 1.0, 1.0))

emissive_mult = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 250)
emissive_mult.set_editor_property("parameter_name", "EmissiveIntensity")
emissive_mult.set_editor_property("default_value", 1.6)

mult_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 150)
mat_editing.connect_material_expressions(emissive_col, "", mult_node, "A")
mat_editing.connect_material_expressions(emissive_mult, "", mult_node, "B")
mat_editing.connect_material_property(mult_node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# Roughness
rough_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 350)
rough_node.set_editor_property("parameter_name", "Roughness")
rough_node.set_editor_property("default_value", 0.08)
mat_editing.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)

# Metallic
metal_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 450)
metal_node.set_editor_property("parameter_name", "Metallic")
metal_node.set_editor_property("default_value", 0.95)
mat_editing.connect_material_property(metal_node, "", unreal.MaterialProperty.MP_METALLIC)

mat_editing.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
print("SUCCESSFULLY TUNED M_SOLDIER_VISOR")
