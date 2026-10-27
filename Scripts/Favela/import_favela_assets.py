"""
NOBLE FERMI // FAVELA ASSET IMPORT + MATERIAL BUILDER (Unreal Editor Python)
Run (commandlet):
  UnrealEditor-Cmd.exe NobleFermi.uproject -run=pythonscript -script="Scripts/Favela/import_favela_assets.py"

Creates under /Game/Environment/Favela:
  Textures/T_*                 (imported from SourceArt/Favela/Textures)
  Meshes/SM_Favela_*           (imported from SourceArt/Favela/Meshes)
  Materials/M_Favela_Surface   Opaque master: world-aligned (box-projected) or mesh-UV PBR, stucco-over-brick
                               peel layer, ground-damp grime gradient, rain streaks, macro variation,
                               per-instance tint / peel / base height (ISM PerInstanceCustomData 0..4).
  Materials/M_Favela_Foliage   Masked two-sided foliage with GPU wind (WorldPositionOffset).
  Materials/M_Favela_Cable     Overhead cable: GPU catenary sag + pendulum wind sway (WorldPositionOffset).
  Materials/MI_Favela_*        Material instances for each surface archetype.
"""
import os
import unreal

ROOT = r"C:\Users\Boris\Documents\antigravity\noble-fermi"
TEX_SRC = os.path.join(ROOT, "SourceArt", "Favela", "Textures")
MESH_SRC = os.path.join(ROOT, "SourceArt", "Favela", "Meshes")
BASE = "/Game/Environment/Favela"
TEX_DST = BASE + "/Textures"
MESH_DST = BASE + "/Meshes"
MAT_DST = BASE + "/Materials"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log_warning("[FAVELA] " + msg)


# ---------------------------------------------------------------------------------------------
# 1. Textures
# ---------------------------------------------------------------------------------------------
def import_texture(fname):
    name = os.path.splitext(fname)[0]
    task = unreal.AssetImportTask()
    task.filename = os.path.join(TEX_SRC, fname)
    task.destination_path = TEX_DST
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    asset_tools.import_asset_tasks([task])
    tex = eal.load_asset(TEX_DST + "/" + name)
    if not tex:
        log("FAILED texture " + name)
        return None
    if name.endswith("_N"):
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("flip_green_channel", False)
    elif name.endswith("_R") or name.endswith("_M"):
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    else:
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    eal.save_loaded_asset(tex, only_if_is_dirty=False)
    return tex


textures = {}
for f in sorted(os.listdir(TEX_SRC)):
    if f.lower().endswith(".png"):
        t = import_texture(f)
        if t:
            textures[os.path.splitext(f)[0]] = t
log("Imported %d textures" % len(textures))


def T(name):
    return textures.get(name)


# ---------------------------------------------------------------------------------------------
# 2. Meshes
# ---------------------------------------------------------------------------------------------
def import_mesh(fname):
    name = os.path.splitext(fname)[0]
    task = unreal.AssetImportTask()
    task.filename = os.path.join(MESH_SRC, fname)
    task.destination_path = MESH_DST
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = False
    opts = unreal.FbxImportUI()
    opts.import_mesh = True
    opts.import_as_skeletal = False
    opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    opts.automated_import_should_detect_type = False
    opts.import_animations = False
    opts.import_materials = False
    opts.import_textures = False
    opts.static_mesh_import_data.combine_meshes = True
    opts.static_mesh_import_data.generate_lightmap_u_vs = False
    opts.static_mesh_import_data.auto_generate_collision = name in ("SM_Favela_RoofSheet", "SM_Favela_WaterTank", "SM_Favela_Tire", "SM_Favela_RebarColumn", "SM_Favela_WindowAC", "SM_Favela_SatelliteDish", "SM_Favela_TrashBags")
    task.options = opts
    asset_tools.import_asset_tasks([task])
    mesh = eal.load_asset(MESH_DST + "/" + name)
    if not mesh:
        log("FAILED mesh " + name)
    return mesh


meshes = {}
for f in sorted(os.listdir(MESH_SRC)):
    if f.lower().endswith(".fbx"):
        m = import_mesh(f)
        if m:
            meshes[os.path.splitext(f)[0]] = m
log("Imported %d meshes" % len(meshes))


# ---------------------------------------------------------------------------------------------
# 3. Material graph helpers
# ---------------------------------------------------------------------------------------------
def fresh_material(name):
    path = MAT_DST + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return asset_tools.create_asset(name, MAT_DST, unreal.Material, unreal.MaterialFactoryNew())


def node(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def scalar(mat, name, val, x, y):
    n = node(mat, unreal.MaterialExpressionScalarParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", val)
    return n


def vector(mat, name, rgba, x, y):
    n = node(mat, unreal.MaterialExpressionVectorParameter, x, y)
    n.set_editor_property("parameter_name", name)
    n.set_editor_property("default_value", unreal.LinearColor(*rgba))
    return n


def texparam(mat, name, tex, sampler, x, y):
    n = node(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    n.set_editor_property("parameter_name", name)
    if tex:
        n.set_editor_property("texture", tex)
    n.set_editor_property("sampler_type", sampler)
    return n


def custom(mat, desc, code, out_type, inputs, x, y):
    n = node(mat, unreal.MaterialExpressionCustom, x, y)
    n.set_editor_property("description", desc)
    n.set_editor_property("code", code)
    n.set_editor_property("output_type", out_type)
    ins = []
    for i in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", i)
        ins.append(ci)
    n.set_editor_property("inputs", ins)
    return n


def link(a, a_out, b, b_in):
    ok = mel.connect_material_expressions(a, a_out, b, b_in)
    if not ok:
        log("LINK FAILED %s.%s -> %s.%s" % (a.get_name(), a_out, b.get_name(), b_in))
    return ok


def pic_data(mat, index, default_node, x, y, vec3=False):
    cls = unreal.MaterialExpressionPerInstanceCustomData3Vector if vec3 else unreal.MaterialExpressionPerInstanceCustomData
    n = node(mat, cls, x, y)
    n.set_editor_property("data_index", index)
    if default_node is not None:
        link(default_node, "", n, "DefaultValue")
    return n


def set_prop(obj, prop, val):
    try:
        obj.set_editor_property(prop, val)
        return True
    except Exception as e:  # property names drift between engine versions
        log("prop %s not set: %s" % (prop, e))
        return False


CMOT_F1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
CMOT_F2 = unreal.CustomMaterialOutputType.CMOT_FLOAT2
CMOT_F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
S_COLOR = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
S_LINEAR = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
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
float peelT = Peel * IPeel;
float show = (peelT <= 0.001) ? 1.0 : smoothstep(peelT - 0.04, peelT + 0.04, BCA);
float3 c = lerp(UBC, BC * Tint * ITint, show);
c *= lerp(1.0, 0.6 + 0.8 * Macro, MS);
float vert = 1.0 - saturate(abs(N.z));
float g = saturate(1.0 - (WP.z - BaseZ) / max(GH, 1.0));
c *= lerp(float3(1.0, 1.0, 1.0), float3(0.48, 0.50, 0.38), g * g * GS * vert);
c *= 1.0 - Streak * SS * vert * 0.5;
return max(c, 0.0);
"""

NORMAL_CODE = r"""
float peelT = Peel * IPeel;
float show = (peelT <= 0.001) ? 1.0 : smoothstep(peelT - 0.04, peelT + 0.04, BCA);
float3 t = lerp(UTN, TN, show);
t = normalize(float3(t.xy * Strength, max(t.z, 0.05)));
if (UseMesh > 0.5) { return normalize(mul(t, (float3x3)Parameters.TangentToWorld)); }
float3 a = abs(N);
float3 w;
if (a.z >= a.x && a.z >= a.y) { float s = N.z >= 0 ? 1.0 : -1.0; w = float3(t.x, t.y, t.z * s); }
else if (a.x >= a.y) { float s = N.x >= 0 ? 1.0 : -1.0; w = float3(t.z * s, t.x * s, -t.y); }
else { float s = N.y >= 0 ? 1.0 : -1.0; w = float3(-t.x * s, t.z * s, -t.y); }
return normalize(w);
"""

ROUGH_CODE = r"""
float peelT = Peel * IPeel;
float show = (peelT <= 0.001) ? 1.0 : smoothstep(peelT - 0.04, peelT + 0.04, BCA);
float r = lerp(0.88, R, show);
return saturate(r * RM + RA);
"""

FOLIAGE_WPO = r"""
float w = pow(saturate(1.0 - UV.y), 1.6);
float ph = dot(WP.xy, float2(0.0123, 0.0171));
float t = T * Speed;
float s = sin(t * 1.30 + ph) * 1.0 + sin(t * 3.70 + ph * 2.3) * 0.35 + sin(t * 8.10 + ph * 5.1) * 0.12;
float3 dir = normalize(float3(1.0, 0.45, 0.0));
return dir * (s * Strength * w) + float3(0.0, 0.0, -abs(s) * Strength * 0.25 * w);
"""

CABLE_WPO = r"""
float u = saturate(UV.x);
float e = 4.0 * u * (1.0 - u);
float3 side = normalize(Side + float3(1e-4, 0.0, 0.0));
side = normalize(float3(side.x, side.y, 0.0) + float3(1e-4, 0.0, 0.0));
float t = T * Speed + Phase;
float s = sin(t * 1.55) + 0.35 * sin(t * 3.9 + 1.3) + 0.15 * sin(t * 7.3 + 0.7);
float3 sway = side * (s * Amp * e);
float lift = (1.0 - cos(clamp(s * 0.35, -1.2, 1.2))) * Amp * 1.5 * e;
return float3(0.0, 0.0, -Sag * e + lift) + sway;
"""


# ---------------------------------------------------------------------------------------------
# 4. Master: M_Favela_Surface
# ---------------------------------------------------------------------------------------------
def build_surface_master():
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
    s_r = texparam(mat, "RoughnessTex", T("T_Concrete_R"), S_LINEAR, -1500, -100)
    s_ubc = texparam(mat, "UnderBaseColorTex", T("T_Brick_BC"), S_COLOR, -1500, 150)
    s_un = texparam(mat, "UnderNormalTex", T("T_Brick_N"), S_NORMAL, -1500, 400)
    s_gm = texparam(mat, "GrimeTex", T("T_Grime_M"), S_LINEAR, -1500, 650)
    s_gs = texparam(mat, "GrimeTex", T("T_Grime_M"), S_LINEAR, -1500, 900)
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
    log("Built M_Favela_Surface")
    return mat


# ---------------------------------------------------------------------------------------------
# 5. Master: M_Favela_Foliage
# ---------------------------------------------------------------------------------------------
def build_foliage_master():
    mat = fresh_material("M_Favela_Foliage")
    set_prop(mat, "blend_mode", unreal.BlendMode.BLEND_MASKED)
    set_prop(mat, "two_sided", True)
    set_prop(mat, "opacity_mask_clip_value", 0.45)
    set_prop(mat, "used_with_instanced_static_meshes", True)
    set_prop(mat, "max_world_position_offset_displacement", 60.0)

    uv0 = node(mat, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    s = texparam(mat, "LeafTex", T("T_Weeds_BC"), S_COLOR, -1100, 0)
    link(uv0, "", s, "UVs")
    tint = vector(mat, "Tint", (1, 1, 1, 1), -1100, 300)
    one3 = node(mat, unreal.MaterialExpressionConstant3Vector, -1300, 450)
    one3.set_editor_property("constant", unreal.LinearColor(1, 1, 1, 1))
    itint = pic_data(mat, 0, one3, -1100, 450, vec3=True)
    m1 = node(mat, unreal.MaterialExpressionMultiply, -800, 100)
    link(s, "RGB", m1, "A")
    link(tint, "", m1, "B")
    m2 = node(mat, unreal.MaterialExpressionMultiply, -600, 100)
    link(m1, "", m2, "A")
    link(itint, "", m2, "B")
    rough = scalar(mat, "Roughness", 0.7, -600, 300)

    wp = node(mat, unreal.MaterialExpressionWorldPosition, -1100, 700)
    tm = node(mat, unreal.MaterialExpressionTime, -1100, 850)
    strength = scalar(mat, "WindStrength", 5.0, -1100, 950)
    speed = scalar(mat, "WindSpeed", 1.0, -1100, 1050)
    c = custom(mat, "FavelaFoliageWind", FOLIAGE_WPO, CMOT_F3, ["WP", "UV", "T", "Strength", "Speed"], -700, 800)
    link(wp, "", c, "WP")
    link(uv0, "", c, "UV")
    link(tm, "", c, "T")
    link(strength, "", c, "Strength")
    link(speed, "", c, "Speed")

    mel.connect_material_property(m2, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(s, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat, only_if_is_dirty=False)
    log("Built M_Favela_Foliage")
    return mat


# ---------------------------------------------------------------------------------------------
# 6. Master: M_Favela_Cable
# ---------------------------------------------------------------------------------------------
def build_cable_master():
    mat = fresh_material("M_Favela_Cable")
    set_prop(mat, "used_with_instanced_static_meshes", True)
    set_prop(mat, "max_world_position_offset_displacement", 320.0)

    col_ = vector(mat, "CableColor", (0.018, 0.018, 0.02, 1), -600, -200)
    rough = scalar(mat, "Roughness", 0.45, -600, -50)

    uv0 = node(mat, unreal.MaterialExpressionTextureCoordinate, -1400, 200)
    tm = node(mat, unreal.MaterialExpressionTime, -1400, 300)
    wp = node(mat, unreal.MaterialExpressionWorldPosition, -1400, 400)
    side_c = node(mat, unreal.MaterialExpressionConstant3Vector, -1600, 550)
    side_c.set_editor_property("constant", unreal.LinearColor(0, 1, 0, 0))
    xf = node(mat, unreal.MaterialExpressionTransform, -1400, 550)
    set_prop(xf, "transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
    set_prop(xf, "transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
    link(side_c, "", xf, "Input")

    def_sag = scalar(mat, "DefaultSag", 45.0, -1600, 700)
    zero = node(mat, unreal.MaterialExpressionConstant, -1600, 800)
    zero.set_editor_property("r", 0.0)
    one = node(mat, unreal.MaterialExpressionConstant, -1600, 900)
    one.set_editor_property("r", 1.0)
    sag = pic_data(mat, 0, def_sag, -1400, 700)
    phase = pic_data(mat, 1, zero, -1400, 800)
    amp_i = pic_data(mat, 2, one, -1400, 900)
    amp_p = scalar(mat, "SwayAmplitude", 7.0, -1400, 1000)
    amp = node(mat, unreal.MaterialExpressionMultiply, -1200, 950)
    link(amp_i, "", amp, "A")
    link(amp_p, "", amp, "B")
    speed = scalar(mat, "WindSpeed", 1.0, -1400, 1100)

    c = custom(mat, "FavelaCableSway", CABLE_WPO, CMOT_F3, ["UV", "T", "WP", "Side", "Sag", "Phase", "Amp", "Speed"], -900, 500)
    link(uv0, "", c, "UV")
    link(tm, "", c, "T")
    link(wp, "", c, "WP")
    link(xf, "", c, "Side")
    link(sag, "", c, "Sag")
    link(phase, "", c, "Phase")
    link(amp, "", c, "Amp")
    link(speed, "", c, "Speed")

    mel.connect_material_property(col_, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat, only_if_is_dirty=False)
    log("Built M_Favela_Cable")
    return mat


m_surface = build_surface_master()
m_foliage = build_foliage_master()
m_cable = build_cable_master()


# ---------------------------------------------------------------------------------------------
# 7. Material instances
# ---------------------------------------------------------------------------------------------
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
    log("MI " + name)
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
make_mi("MI_Favela_Weeds", m_foliage, {"LeafTex": T("T_Weeds_BC")}, {"WindStrength": 4.0})
make_mi("MI_Favela_Ivy", m_foliage, {"LeafTex": T("T_Ivy_BC")}, {"WindStrength": 1.2, "WindSpeed": 1.4})
make_mi("MI_Favela_Banana", m_foliage, {"LeafTex": T("T_Banana_BC")}, {"WindStrength": 14.0, "WindSpeed": 0.8})
make_mi("MI_Favela_PalmFan", m_foliage, {"LeafTex": T("T_Banana_BC")}, {"WindStrength": 7.0, "WindSpeed": 0.9})
make_mi("MI_Favela_Cable", m_cable, None, {"SwayAmplitude": 7.0})

# Default material slots on the meshes
slot_map = {
    "SM_Favela_Cable": "MI_Favela_Cable",
    "SM_Favela_RoofSheet": "MI_Favela_RoofMetal",
    "SM_Favela_Weeds": "MI_Favela_Weeds",
    "SM_Favela_Ivy": "MI_Favela_Ivy",
    "SM_Favela_Banana": "MI_Favela_Banana",
    "SM_Favela_Tire": "MI_Favela_Rubber",
    "SM_Favela_WaterTank": "MI_Favela_Plastic",
    "SM_Favela_PalmFan": "MI_Favela_PalmFan",
    "SM_Favela_WindowAC": "MI_Favela_Metal",
    "SM_Favela_SatelliteDish": "MI_Favela_Metal",
    "SM_Favela_RebarColumn": "MI_Favela_Concrete",
    "SM_Favela_TrashBags": "MI_Favela_Plastic",
}
for mesh_name, mi_name in slot_map.items():
    mesh = meshes.get(mesh_name)
    mi = eal.load_asset(MAT_DST + "/" + mi_name)
    if mesh and mi:
        mesh.set_material(0, mi)
        eal.save_loaded_asset(mesh, only_if_is_dirty=False)

eal.save_directory(BASE, only_if_is_dirty=False, recursive=True)
log("=== FAVELA ASSET PIPELINE COMPLETE ===")
