import unreal
import sys

print(">>> UPDATING M_SOLDIER_VISOR WITH TACTICAL CYAN EMISSIVE GLOW <<<")

mat_path = "/Game/Characters/BaseMale/Materials/M_Soldier_Visor"
mat = unreal.EditorAssetLibrary.load_asset(mat_path)
if not mat:
    print(f"ERROR: Could not load {mat_path}")
    sys.exit(1)

mat_editing = unreal.MaterialEditingLibrary

# Create or find Emissive parameter
emissive_color_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 300)
emissive_color_node.set_editor_property("parameter_name", "EmissiveColor")
emissive_color_node.set_editor_property("default_value", unreal.LinearColor(0.0, 0.898, 1.0, 1.0)) # Tactical Cyan #00E5FF

emissive_mult_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 450)
emissive_mult_node.set_editor_property("parameter_name", "EmissiveIntensity")
emissive_mult_node.set_editor_property("default_value", 3.5)

mult_node = mat_editing.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 350)
mat_editing.connect_material_expressions(emissive_color_node, "", mult_node, "A")
mat_editing.connect_material_expressions(emissive_mult_node, "", mult_node, "B")

mat_editing.connect_material_property(mult_node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mat_editing.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
print("SUCCESSFULLY APPLIED CYAN EMISSIVE GLOW TO M_SOLDIER_VISOR")
