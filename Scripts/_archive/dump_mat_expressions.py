import unreal

mat = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/Materials/M_Operator_BaseBody")
print("Material:", mat.get_name())
for expr in mat.get_editor_property("expressions"):
    desc = f"  Expr: {expr.get_class().get_name()}"
    if isinstance(expr, unreal.MaterialExpressionTextureSampleParameter2D):
        tex = expr.get_editor_property("texture")
        desc += f" (Param: {expr.get_editor_property('parameter_name')}, Tex: {tex.get_name() if tex else 'NONE'})"
    elif isinstance(expr, unreal.MaterialExpressionVectorParameter):
        val = expr.get_editor_property("default_value")
        desc += f" (Param: {expr.get_editor_property('parameter_name')}, Val: {val})"
    elif isinstance(expr, unreal.MaterialExpressionScalarParameter):
        val = expr.get_editor_property("default_value")
        desc += f" (Param: {expr.get_editor_property('parameter_name')}, Val: {val})"
    print(desc)
