import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

BASE = "/Game/Environment/Favela"
TEX_DST = BASE + "/Textures"
MAT_DST = BASE + "/Materials"
MESH_DST = BASE + "/Meshes"

def log(msg):
    unreal.log_warning("[FAVELA_FIX] " + str(msg))

def T(name):
    tex = eal.load_asset(TEX_DST + "/" + name)
    if not tex:
        log("Texture not found: " + name)
    return tex

def set_prop(obj, prop, val):
    try:
        obj.set_editor_property(prop, val)
        return True
    except Exception as e:
        log("prop %s not set: %s" % (prop, e))
        return False

def fresh_material(name):
    path = MAT_DST + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mat = asset_tools.create_asset(name, MAT_DST, unreal.Material, unreal.MaterialFactoryNew())
    return mat

def node(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)

def scalar(mat, name, val, x, y):
    n = node(mat, unreal.MaterialExpressionScalarParameter, x, y)
    set_prop(n, "parameter_name", name)
    set_prop(n, "default_value", val)
    return n

def vector(mat, name, rgba, x, y):
    n = node(mat, unreal.MaterialExpressionVectorParameter, x, y)
    set_prop(n, "parameter_name", name)
    set_prop(n, "default_value", unreal.LinearColor(*rgba))
    return n

def texparam(mat, name, tex, sampler_type, x, y):
    n = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    set_prop(n, "parameter_name", name)
    if tex:
        set_prop(n, "texture", tex)
    set_prop(n, "sampler_type", sampler_type)
    return n

def custom(mat, name, code, out_type, inputs, x, y):
    n = node(mat, unreal.MaterialExpressionCustom, x, y)
    set_prop(n, "description", name)
    set_prop(n, "code", code)
    set_prop(n, "output_type", out_type)
    in_arr = []
    for inp in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", inp)
        in_arr.append(ci)
    set_prop(n, "inputs", in_arr)
    return n

def pic_data(mat, idx, def_val_node, x, y, vec3=False):
    cls = unreal.MaterialExpressionPerInstanceCustomData3Vector if vec3 else unreal.MaterialExpressionPerInstanceCustomData
    n = node(mat, cls, x, y)
    set_prop(n, "data_index", idx)
    mel.connect_material_expressions(def_val_node, "", n, "DefaultValue")
    return n

def link(src, src_pin, dst, dst_pin):
    if not mel.connect_material_expressions(src, src_pin, dst, dst_pin):
        log("LINK FAILED %s.%s -> %s.%s" % (src.get_name(), src_pin, dst.get_name(), dst_pin))

CMOT_F1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
CMOT_F2 = unreal.CustomMaterialOutputType.CMOT_FLOAT2
CMOT_F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
S_COLOR = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
S_MASKS = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
S_NORMAL = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL

UV_CODE = r"""
if (UseMesh > 0.5) { return MeshUV * MeshTiling; }
float3 a = abs(N);
float2 uv;
if (a.z >= a.x && a.z >= a.y) { uv = WP.xy; }
else if (a.x >= a.y) { uv = float2(WP.y * (N.x >= 0 ? 1.0 : -1.0), -WP.z); }
else { uv = float2(-WP.x * (N.y >= 0 ? 1.0 : -1.0), -WP.z); }
return uv / max(Tile, 1.0);
"""

COLOR_CODE = r"""
float peelT = (IPeel > 0.001) ? (Peel * IPeel) : Peel;
float show = (peelT <= 0.001) ? 1.0 : smoothstep(peelT - 0.04, peelT + 0.04, BCA);
float3 effTint = (ITint.x > 0.001 || ITint.y > 0.001 || ITint.z > 0.001) ? (ITint * Tint) : Tint;
float3 c = lerp(UBC, BC * effTint, show);
c *= lerp(1.0, 0.6 + 0.8 * Macro, MS);
float vert = 1.0 - saturate(abs(N.z));
float g = saturate(1.0 - (WP.z - BaseZ) / max(GH, 1.0));
c *= lerp(float3(1.0, 1.0, 1.0), float3(0.48, 0.50, 0.38), g * g * GS * vert);
return c;
"""

NORMAL_CODE = r"""
float peelT = (IPeel > 0.001) ? (Peel * IPeel) : Peel;
float show = (peelT <= 0.001) ? 1.0 : smoothstep(peelT - 0.04, peelT + 0.04, BCA);
float3 tn = lerp(UTN, TN, show);
tn = normalize(float3((tn.xy * 2.0 - 1.0) * Strength, tn.z * 2.0 - 1.0));
float3 t = tn;
if (UseMesh > 0.5) { return normalize(mul(t, (float3x3)Parameters.TangentToWorld)); }
float3 a = abs(N);
float3 w;
if (a.z >= a.x && a.z >= a.y) { float s = N.z >= 0 ? 1.0 : -1.0; w = float3(t.x, t.y, t.z * s); }
else if (a.x >= a.y) { float s = N.x >= 0 ? 1.0 : -1.0; w = float3(t.z * s, t.x * s, -t.y); }
else { float s = N.y >= 0 ? 1.0 : -1.0; w = float3(-t.x * s, t.z * s, -t.y); }
return normalize(w);
"""

ROUGH_CODE = r"""
float peelT = (IPeel > 0.001) ? (Peel * IPeel) : Peel;
float show = (peelT <= 0.001) ? 1.0 : smoothstep(peelT - 0.04, peelT + 0.04, BCA);
float r = lerp(0.88, R, show);
return saturate(r * RM + RA);
"""

log("Building corrected M_Favela_Surface with SAMPLERTYPE_MASKS...")
mat = fresh_material("M_Favela_Surface")
set_prop(mat, "tangent_space_normal", False)
set_prop(mat, "used_with_instanced_static_meshes", True)
set_prop(mat, "used_with_static_lighting", False)

wp = node(mat, unreal.MaterialExpressionWorldPosition, -2200, 0)
nrm = node(mat, unreal.MaterialExpressionVertexNormalWS, -2200, 150)
uv0 = node(mat, unreal.MaterialExpressionTextureCoordinate, -2200, 300)
use_mesh = scalar(mat, "UseMeshUV", 0.0, -2200, 420)
mesh_tiling = scalar(mat, "MeshTiling", 1.0, -2200, 520)

def uv_node(tile_param, y):
    c = custom(mat, "FavelaUV", UV_CODE, CMOT_F2, ["WP", "N", "MeshUV", "UseMesh", "MeshTiling", "Tile"], -1850, y)
    link(wp, "", c, "WP")
    link(nrm, "", c, "N")
    link(uv0, "", c, "MeshUV")
    link(use_mesh, "", c, "UseMesh")
    link(mesh_tiling, "", c, "MeshTiling")
    link(tile_param, "", c, "Tile")
    return c

tile = scalar(mat, "TileSize", 200.0, -2100, -400)
utile = scalar(mat, "UnderTileSize", 120.0, -2100, -300)
mtile = scalar(mat, "MacroTileSize", 900.0, -2100, -200)
stile = scalar(mat, "StreakTileSize", 300.0, -2100, -100)
uv_main = uv_node(tile, -400)
uv_under = uv_node(utile, -150)
uv_macro = uv_node(mtile, 100)
uv_streak = uv_node(stile, 350)

s_bc = texparam(mat, "BaseColorTex", T("T_Concrete_BC"), S_COLOR, -1500, -600)
s_n = texparam(mat, "NormalTex", T("T_Concrete_N"), S_NORMAL, -1500, -350)
s_r = texparam(mat, "RoughnessTex", T("T_Concrete_R"), S_MASKS, -1500, -100)
s_ubc = texparam(mat, "UnderBaseColorTex", T("T_Brick_BC"), S_COLOR, -1500, 150)
s_un = texparam(mat, "UnderNormalTex", T("T_Brick_N"), S_NORMAL, -1500, 400)
s_gm = texparam(mat, "GrimeTex", T("T_Grime_M"), S_MASKS, -1500, 650)
s_gs = texparam(mat, "GrimeTex", T("T_Grime_M"), S_MASKS, -1500, 900)
for s, u in ((s_bc, uv_main), (s_n, uv_main), (s_r, uv_main), (s_ubc, uv_under), (s_un, uv_under), (s_gm, uv_macro), (s_gs, uv_streak)):
    link(u, "", s, "UVs")

tint = vector(mat, "Tint", (1, 1, 1, 1), -1500, 1150)
one3 = node(mat, unreal.MaterialExpressionConstant3Vector, -1700, 1300)
one3.set_editor_property("constant", unreal.LinearColor(1, 1, 1, 1))
one1 = node(mat, unreal.MaterialExpressionConstant, -1700, 1400)
one1.set_editor_property("r", 1.0)
ground_z = scalar(mat, "GroundZ", 0.0, -1700, 1500)
itint = pic_data(mat, 0, one3, -1500, 1300, vec3=True)
ipeel = pic_data(mat, 3, one1, -1500, 1400)
ibasez = pic_data(mat, 4, ground_z, -1500, 1500)

peel = scalar(mat, "PeelAmount", 0.0, -1300, 1150)
gs = scalar(mat, "GrimeStrength", 0.8, -1300, 1250)
gh = scalar(mat, "GrimeHeight", 90.0, -1300, 1350)
ss = scalar(mat, "StreakStrength", 0.6, -1300, 1450)
ms = scalar(mat, "MacroStrength", 0.35, -1300, 1550)
nstr = scalar(mat, "NormalStrength", 1.0, -1300, 1650)
rm = scalar(mat, "RoughnessMul", 1.0, -1300, 1750)
ra = scalar(mat, "RoughnessAdd", 0.0, -1300, 1850)
spec = scalar(mat, "Specular", 0.5, -400, 600)
metal = scalar(mat, "Metallic", 0.0, -400, 700)

c_col = custom(mat, "FavelaColor", COLOR_CODE, CMOT_F3,
               ["BC", "BCA", "UBC", "Tint", "ITint", "Peel", "IPeel", "WP", "N", "BaseZ", "GS", "GH", "Macro", "Streak", "SS", "MS"], -800, -400)
link(s_bc, "RGB", c_col, "BC")
link(s_bc, "A", c_col, "BCA")
link(s_ubc, "RGB", c_col, "UBC")
link(tint, "", c_col, "Tint")
link(itint, "", c_col, "ITint")
link(peel, "", c_col, "Peel")
link(ipeel, "", c_col, "IPeel")
link(wp, "", c_col, "WP")
link(nrm, "", c_col, "N")
link(ibasez, "", c_col, "BaseZ")
link(gs, "", c_col, "GS")
link(gh, "", c_col, "GH")
link(s_gm, "R", c_col, "Macro")
link(s_gs, "G", c_col, "Streak")
link(ss, "", c_col, "SS")
link(ms, "", c_col, "MS")

c_n = custom(mat, "FavelaNormal", NORMAL_CODE, CMOT_F3, ["TN", "UTN", "BCA", "Peel", "IPeel", "N", "UseMesh", "Strength"], -800, 0)
link(s_n, "RGB", c_n, "TN")
link(s_un, "RGB", c_n, "UTN")
link(s_bc, "A", c_n, "BCA")
link(peel, "", c_n, "Peel")
link(ipeel, "", c_n, "IPeel")
link(nrm, "", c_n, "N")
link(use_mesh, "", c_n, "UseMesh")
link(nstr, "", c_n, "Strength")

c_r = custom(mat, "FavelaRough", ROUGH_CODE, CMOT_F1, ["R", "BCA", "Peel", "IPeel", "RM", "RA"], -800, 300)
link(s_r, "R", c_r, "R")
link(s_bc, "A", c_r, "BCA")
link(peel, "", c_r, "Peel")
link(ipeel, "", c_r, "IPeel")
link(rm, "", c_r, "RM")
link(ra, "", c_r, "RA")

mel.connect_material_property(c_col, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(c_n, "", unreal.MaterialProperty.MP_NORMAL)
mel.connect_material_property(c_r, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
mel.recompile_material(mat)
eal.save_loaded_asset(mat, only_if_is_dirty=False)
log("Successfully Recompiled and Saved M_Favela_Surface!")

# Material Instances
m_surface = mat

def make_mi(name, parent, textures_=None, scalars=None, vectors=None):
    path = MAT_DST + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mi = asset_tools.create_asset(name, MAT_DST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for k, v in (textures_ or {}).items():
        if v:
            mel.set_material_instance_texture_parameter_value(mi, k, v)
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, v)
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(*v))
    mel.update_material_instance(mi)
    eal.save_loaded_asset(mi, only_if_is_dirty=False)
    log("Rebuilt MI " + name)
    return mi

def surf(prefix):
    return {"BaseColorTex": T("T_%s_BC" % prefix), "NormalTex": T("T_%s_N" % prefix), "RoughnessTex": T("T_%s_R" % prefix)}

make_mi("MI_Favela_Brick", m_surface, surf("Brick"), {"TileSize": 120.0, "GrimeStrength": 0.9, "StreakStrength": 0.7})
make_mi("MI_Favela_Cinder", m_surface, surf("Cinder"), {"TileSize": 120.0, "GrimeStrength": 0.9, "StreakStrength": 0.6})
stucco = surf("Stucco")
stucco.update({"UnderBaseColorTex": T("T_Brick_BC"), "UnderNormalTex": T("T_Brick_N")})
make_mi("MI_Favela_Stucco", m_surface, stucco, {"TileSize": 260.0, "UnderTileSize": 120.0, "PeelAmount": 0.32, "GrimeStrength": 1.0, "StreakStrength": 0.8})
make_mi("MI_Favela_Concrete", m_surface, surf("Concrete"), {"TileSize": 250.0, "GrimeStrength": 0.7, "StreakStrength": 0.7})
make_mi("MI_Favela_Cobble", m_surface, surf("Cobble"), {"TileSize": 160.0, "MacroStrength": 0.45, "GrimeStrength": 0.0, "NormalStrength": 1.2})
make_mi("MI_Favela_Asphalt", m_surface, surf("Asphalt"), {"TileSize": 320.0, "MacroStrength": 0.4, "GrimeStrength": 0.0})
make_mi("MI_Favela_Dirt", m_surface, surf("Dirt"), {"TileSize": 260.0, "MacroStrength": 0.5, "GrimeStrength": 0.0})
make_mi("MI_Favela_Sand", m_surface, surf("Sand"), {"TileSize": 240.0, "MacroStrength": 0.35, "GrimeStrength": 0.0})
make_mi("MI_Favela_Wood", m_surface, surf("Wood"), {"TileSize": 120.0, "GrimeStrength": 0.6, "StreakStrength": 0.4})
make_mi("MI_Favela_RoofMetal", m_surface, surf("Metal"), {"UseMeshUV": 1.0, "MeshTiling": 1.0, "Metallic": 0.55, "GrimeStrength": 0.0, "StreakStrength": 0.0, "MacroStrength": 0.2})
make_mi("MI_Favela_Metal", m_surface, surf("Metal"), {"TileSize": 140.0, "Metallic": 0.5, "GrimeStrength": 0.3, "StreakStrength": 0.3})
make_mi("MI_Favela_Puddle", m_surface, surf("Asphalt"), {"TileSize": 320.0, "RoughnessMul": 0.0, "RoughnessAdd": 0.04, "Specular": 0.9, "NormalStrength": 0.05, "GrimeStrength": 0.0, "MacroStrength": 0.1}, {"Tint": (0.35, 0.37, 0.40, 1)})
make_mi("MI_Favela_Plastic", m_surface, surf("Concrete"), {"TileSize": 400.0, "RoughnessMul": 0.4, "RoughnessAdd": 0.05, "NormalStrength": 0.1, "GrimeStrength": 0.3, "MacroStrength": 0.1}, {"Tint": (1.6, 1.6, 1.6, 1)})
make_mi("MI_Favela_Rubber", m_surface, surf("Asphalt"), {"TileSize": 60.0, "RoughnessAdd": 0.1, "GrimeStrength": 0.0}, {"Tint": (0.45, 0.45, 0.45, 1)})
make_mi("MI_Favela_Dark", m_surface, surf("Concrete"), {"TileSize": 200.0, "GrimeStrength": 0.0, "StreakStrength": 0.0, "MacroStrength": 0.0, "NormalStrength": 0.2}, {"Tint": (0.06, 0.055, 0.05, 1)})

# Reassign material slots on the meshes
mesh_slots = {
    "SM_Favela_RoofSheet": "MI_Favela_RoofMetal",
    "SM_Favela_Tire": "MI_Favela_Rubber",
    "SM_Favela_WaterTank": "MI_Favela_Plastic",
}
for mname, miname in mesh_slots.items():
    m = eal.load_asset(MESH_DST + "/" + mname)
    inst = eal.load_asset(MAT_DST + "/" + miname)
    if m and inst:
        m.set_material(0, inst)
        eal.save_loaded_asset(m, only_if_is_dirty=False)
        log("Reassigned mesh slot 0 for " + mname)

eal.save_directory(MAT_DST, only_if_is_dirty=False, recursive=True)
log("ALL FAVELA SURFACE MATERIALS FIXED AND SAVED SUCCESSFULLY!")
