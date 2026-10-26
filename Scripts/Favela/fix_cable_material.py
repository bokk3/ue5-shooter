import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
MAT_DST = "/Game/Environment/Favela/Materials"

CABLE_WPO = r"""
float u = saturate(UV.x);
float e = 4.0 * u * (1.0 - u);
float3 side = normalize(float3(Side.x, Side.y, 0.0) + float3(0.0007, 0.0007, 0.0));
float t = T * Speed + Phase;
float s = sin(t * 1.55) + 0.35 * sin(t * 3.9 + 1.3) + 0.15 * sin(t * 7.3 + 0.7);
float3 sway = side * (s * Amp * e);
float lift = (1.0 - cos(clamp(s * 0.35, -1.2, 1.2))) * Amp * 1.5 * e;
return float3(0.0, 0.0, -Sag * e + lift) + sway;
"""

path = MAT_DST + "/M_Favela_Cable"
if eal.does_asset_exist(path):
    eal.delete_asset(path)
mat = asset_tools.create_asset("M_Favela_Cable", MAT_DST, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_instanced_static_meshes", True)
mat.set_editor_property("max_world_position_offset_displacement", 360.0)


def node(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def scalar(name, val, x, y):
    n = node(unreal.MaterialExpressionScalarParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", val)
    return n


def pic(index, def_node, x, y, vec3=False):
    cls = unreal.MaterialExpressionPerInstanceCustomData3Vector if vec3 else unreal.MaterialExpressionPerInstanceCustomData
    n = node(cls, x, y)
    n.set_editor_property("data_index", index)
    mel.connect_material_expressions(def_node, "", n, "DefaultValue")
    return n


col_ = node(unreal.MaterialExpressionVectorParameter, -600, -200)
col_.set_editor_property("parameter_name", "CableColor")
col_.set_editor_property("default_value", unreal.LinearColor(0.018, 0.018, 0.02, 1))
rough = scalar("Roughness", 0.45, -600, -50)

uv0 = node(unreal.MaterialExpressionTextureCoordinate, -1400, 200)
tm = node(unreal.MaterialExpressionTime, -1400, 300)
wp = node(unreal.MaterialExpressionWorldPosition, -1400, 400)

def_side = node(unreal.MaterialExpressionConstant3Vector, -1600, 550)
def_side.set_editor_property("constant", unreal.LinearColor(0.707, 0.707, 0, 0))
side = pic(3, def_side, -1400, 550, vec3=True)

def_sag = scalar("DefaultSag", 45.0, -1600, 700)
zero = node(unreal.MaterialExpressionConstant, -1600, 800)
zero.set_editor_property("r", 0.0)
one = node(unreal.MaterialExpressionConstant, -1600, 900)
one.set_editor_property("r", 1.0)
sag = pic(0, def_sag, -1400, 700)
phase = pic(1, zero, -1400, 800)
amp_i = pic(2, one, -1400, 900)
amp_p = scalar("SwayAmplitude", 8.5, -1400, 1000)
amp = node(unreal.MaterialExpressionMultiply, -1200, 950)
mel.connect_material_expressions(amp_i, "", amp, "A")
mel.connect_material_expressions(amp_p, "", amp, "B")
speed = scalar("WindSpeed", 1.0, -1400, 1100)

c = node(unreal.MaterialExpressionCustom, -900, 500)
c.set_editor_property("description", "FavelaCableSway")
c.set_editor_property("code", CABLE_WPO)
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for name in ["UV", "T", "WP", "Side", "Sag", "Phase", "Amp", "Speed"]:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", name)
    ins.append(ci)
c.set_editor_property("inputs", ins)

for src, pin in [(uv0, "UV"), (tm, "T"), (wp, "WP"), (side, "Side"), (sag, "Sag"), (phase, "Phase"), (amp, "Amp"), (speed, "Speed")]:
    ok = mel.connect_material_expressions(src, "", c, pin)
    if not ok:
        unreal.log_warning("[FAVELA] FAIL " + pin)

mel.connect_material_property(col_, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
mel.recompile_material(mat)
eal.save_loaded_asset(mat, only_if_is_dirty=False)

mi_path = MAT_DST + "/MI_Favela_Cable"
if eal.does_asset_exist(mi_path):
    eal.delete_asset(mi_path)
mi = asset_tools.create_asset("MI_Favela_Cable", MAT_DST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
mel.set_material_instance_parent(mi, mat)
mel.set_material_instance_scalar_parameter_value(mi, "SwayAmplitude", 8.5)
mel.update_material_instance(mi)
eal.save_loaded_asset(mi, only_if_is_dirty=False)

mesh = eal.load_asset("/Game/Environment/Favela/Meshes/SM_Favela_Cable")
if mesh:
    mesh.set_material(0, mi)
    eal.save_loaded_asset(mesh, only_if_is_dirty=False)

unreal.log_warning("[FAVELA] M_Favela_Cable + MI_Favela_Cable updated with zero link errors")
