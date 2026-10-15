import unreal
import os
import sys

print(">>> IMPORTING OPERATOR TEXTURES AND CREATING TACTICAL MASTER MATERIAL <<<")

texture_dir = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Characters\BaseMale\Textures"
destination_path = "/Game/Characters/BaseMale/Textures"
material_path = "/Game/Characters/BaseMale/Materials"

os.makedirs(r"C:\Users\Boris\Documents\antigravity\noble-fermi\Content\Characters\BaseMale\Materials", exist_ok=True)

# -------------------------------------------------------------------------
# Step 1: Import Textures
# -------------------------------------------------------------------------
textures_to_import = [
    ("T_Ripstop_Normal.png", True, False, unreal.TextureCompressionSettings.TC_NORMALMAP),
    ("T_Ripstop_Roughness.png", False, False, unreal.TextureCompressionSettings.TC_MASKS),
    ("T_Camo_Digital.png", False, True, unreal.TextureCompressionSettings.TC_DEFAULT),
    ("T_Operator_Body_Mask.png", False, False, unreal.TextureCompressionSettings.TC_MASKS),
]

imported_textures = {}

for tex_file, is_normal, srgb, comp_setting in textures_to_import:
    full_path = os.path.join(texture_dir, tex_file)
    tex_name = os.path.splitext(tex_file)[0]
    
    task = unreal.AssetImportTask()
    task.filename = full_path
    task.destination_path = destination_path
    task.destination_name = tex_name
    task.replace_existing = True
    task.automated = True
    task.save = True
    
    options = unreal.TextureFactory()
    task.factory = options
    
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    
    asset = unreal.EditorAssetLibrary.load_asset(f"{destination_path}/{tex_name}")
    if asset:
        print(f"Loaded texture asset: {asset.get_path_name()}")
        try:
            asset.set_editor_property("compression_settings", comp_setting)
        except Exception as e:
            print(f"Compression setting note: {e}")
        try:
            asset.set_editor_property("srgb", srgb)
        except Exception:
            pass
        if is_normal:
            try:
                asset.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP)
            except Exception:
                pass
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        imported_textures[tex_name] = asset
    else:
        print(f"WARNING: Could not load imported texture {tex_name}")

unreal.EditorAssetLibrary.save_directory(destination_path, only_if_is_dirty=False, recursive=True)

# -------------------------------------------------------------------------
# Step 2: Create Master Material M_Operator_BaseBody
# -------------------------------------------------------------------------
mat_name = "M_Operator_BaseBody"
mat_package = f"{material_path}/{mat_name}"

# Check if exists, or create new
existing_mat = unreal.EditorAssetLibrary.load_asset(mat_package)
if existing_mat:
    unreal.EditorAssetLibrary.delete_asset(mat_package)

mat_factory = unreal.MaterialFactoryNew()
material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    mat_name, material_path, unreal.Material, mat_factory
)

if not material:
    print("ERROR: Failed to create M_Operator_BaseBody material")
    sys.exit(1)

print(f"Created material: {material.get_path_name()}")

# Configure Material Properties
material.set_editor_property("two_sided", False)
try:
    material.set_editor_property("used_with_skeletal_mesh", True)
except Exception as e:
    print(f"used_with_skeletal_mesh setting note: {e}")

# Set Shading Model to Subsurface Profile or Subsurface
try:
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SUBSURFACE)
except Exception as e:
    print(f"Shading model setting note: {e}")

# Helper to create expression
def add_expr(expr_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, expr_class, x, y)

# 1. Texture Coordinate nodes
tex_coord_camo = add_expr(unreal.MaterialExpressionTextureCoordinate, -1200, -200)
tex_coord_ripstop = add_expr(unreal.MaterialExpressionTextureCoordinate, -1200, 300)

# Scalars for tiling
scalar_camo_tiling = add_expr(unreal.MaterialExpressionScalarParameter, -1200, -400)
scalar_camo_tiling.set_editor_property("parameter_name", "CamoTiling")
scalar_camo_tiling.set_editor_property("default_value", 6.0)

scalar_ripstop_tiling = add_expr(unreal.MaterialExpressionScalarParameter, -1200, 100)
scalar_ripstop_tiling.set_editor_property("parameter_name", "RipstopTiling")
scalar_ripstop_tiling.set_editor_property("default_value", 24.0)

mult_camo_uv = add_expr(unreal.MaterialExpressionMultiply, -1000, -300)
unreal.MaterialEditingLibrary.connect_material_expressions(tex_coord_camo, "", mult_camo_uv, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(scalar_camo_tiling, "", mult_camo_uv, "B")

mult_ripstop_uv = add_expr(unreal.MaterialExpressionMultiply, -1000, 200)
unreal.MaterialEditingLibrary.connect_material_expressions(tex_coord_ripstop, "", mult_ripstop_uv, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(scalar_ripstop_tiling, "", mult_ripstop_uv, "B")

# 2. Camo Colors (Vector Parameters)
col_primary = add_expr(unreal.MaterialExpressionVectorParameter, -1000, -800)
col_primary.set_editor_property("parameter_name", "CamoColorPrimary")
col_primary.set_editor_property("default_value", unreal.LinearColor(0.24, 0.26, 0.22, 1.0)) # Olive Drab

col_secondary = add_expr(unreal.MaterialExpressionVectorParameter, -1000, -600)
col_secondary.set_editor_property("parameter_name", "CamoColorSecondary")
col_secondary.set_editor_property("default_value", unreal.LinearColor(0.16, 0.18, 0.14, 1.0)) # Shadow Drab

col_tertiary = add_expr(unreal.MaterialExpressionVectorParameter, -1000, -400)
col_tertiary.set_editor_property("parameter_name", "CamoColorTertiary")
col_tertiary.set_editor_property("default_value", unreal.LinearColor(0.32, 0.30, 0.22, 1.0)) # Khaki Accent

# 3. Non-Camo Zone Colors (Vector Parameters)
col_torso = add_expr(unreal.MaterialExpressionVectorParameter, -600, -900)
col_torso.set_editor_property("parameter_name", "TorsoColor")
col_torso.set_editor_property("default_value", unreal.LinearColor(0.22, 0.24, 0.20, 1.0))

col_skin = add_expr(unreal.MaterialExpressionVectorParameter, -600, -700)
col_skin.set_editor_property("parameter_name", "SkinTone")
col_skin.set_editor_property("default_value", unreal.LinearColor(0.78, 0.58, 0.46, 1.0))

col_gear = add_expr(unreal.MaterialExpressionVectorParameter, -600, -500)
col_gear.set_editor_property("parameter_name", "GearColor")
col_gear.set_editor_property("default_value", unreal.LinearColor(0.08, 0.08, 0.09, 1.0))

# 4. Texture Samplers
samp_camo = add_expr(unreal.MaterialExpressionTextureSampleParameter2D, -800, -250)
samp_camo.set_editor_property("parameter_name", "Texture_Camo")
if "T_Camo_Digital" in imported_textures:
    samp_camo.set_editor_property("texture", imported_textures["T_Camo_Digital"])
unreal.MaterialEditingLibrary.connect_material_expressions(mult_camo_uv, "", samp_camo, "UVs")

samp_mask = add_expr(unreal.MaterialExpressionTextureSampleParameter2D, -600, -300)
samp_mask.set_editor_property("parameter_name", "Texture_BodyMask")
if "T_Operator_Body_Mask" in imported_textures:
    samp_mask.set_editor_property("texture", imported_textures["T_Operator_Body_Mask"])

samp_normal = add_expr(unreal.MaterialExpressionTextureSampleParameter2D, -800, 200)
samp_normal.set_editor_property("parameter_name", "Texture_RipstopNormal")
samp_normal.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
if "T_Ripstop_Normal" in imported_textures:
    samp_normal.set_editor_property("texture", imported_textures["T_Ripstop_Normal"])
unreal.MaterialEditingLibrary.connect_material_expressions(mult_ripstop_uv, "", samp_normal, "UVs")

samp_roughness = add_expr(unreal.MaterialExpressionTextureSampleParameter2D, -800, 450)
samp_roughness.set_editor_property("parameter_name", "Texture_RipstopRoughness")
samp_roughness.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
if "T_Ripstop_Roughness" in imported_textures:
    samp_roughness.set_editor_property("texture", imported_textures["T_Ripstop_Roughness"])
unreal.MaterialEditingLibrary.connect_material_expressions(mult_ripstop_uv, "", samp_roughness, "UVs")

# 5. Blend Camo Pattern: Lerp(Primary, Secondary, Camo.R), then Lerp(..., Tertiary, Camo.G)
lerp_camo_1 = add_expr(unreal.MaterialExpressionLinearInterpolate, -600, -600)
unreal.MaterialEditingLibrary.connect_material_expressions(col_primary, "", lerp_camo_1, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(col_secondary, "", lerp_camo_1, "B")
unreal.MaterialEditingLibrary.connect_material_expressions(samp_camo, "R", lerp_camo_1, "Alpha")

lerp_camo_2 = add_expr(unreal.MaterialExpressionLinearInterpolate, -400, -600)
unreal.MaterialEditingLibrary.connect_material_expressions(lerp_camo_1, "", lerp_camo_2, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(col_tertiary, "", lerp_camo_2, "B")
unreal.MaterialEditingLibrary.connect_material_expressions(samp_camo, "G", lerp_camo_2, "Alpha")

# 6. Multi-zone blend using Body Mask:
# Mask Channels: R = Torso, G = Uniform Camo, B = Skin, A = Gear
# Base = Camo (Uniform)
# Lerp with Torso where Mask.R > 0
lerp_zone_torso = add_expr(unreal.MaterialExpressionLinearInterpolate, -200, -600)
unreal.MaterialEditingLibrary.connect_material_expressions(lerp_camo_2, "", lerp_zone_torso, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(col_torso, "", lerp_zone_torso, "B")
unreal.MaterialEditingLibrary.connect_material_expressions(samp_mask, "R", lerp_zone_torso, "Alpha")

# Lerp with Skin where Mask.B > 0
lerp_zone_skin = add_expr(unreal.MaterialExpressionLinearInterpolate, 0, -600)
unreal.MaterialEditingLibrary.connect_material_expressions(lerp_zone_torso, "", lerp_zone_skin, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(col_skin, "", lerp_zone_skin, "B")
unreal.MaterialEditingLibrary.connect_material_expressions(samp_mask, "B", lerp_zone_skin, "Alpha")

# Lerp with Gear where Mask.A > 0
lerp_zone_gear = add_expr(unreal.MaterialExpressionLinearInterpolate, 200, -600)
unreal.MaterialEditingLibrary.connect_material_expressions(lerp_zone_skin, "", lerp_zone_gear, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(col_gear, "", lerp_zone_gear, "B")
unreal.MaterialEditingLibrary.connect_material_expressions(samp_mask, "A", lerp_zone_gear, "Alpha")

# Connect Base Color
unreal.MaterialEditingLibrary.connect_material_property(lerp_zone_gear, "", unreal.MaterialProperty.MP_BASE_COLOR)

# 7. Normal Map: Mask out normal map on skin areas so face/hands remain smooth organic skin
# Flat normal vector (0, 0, 1)
flat_normal = add_expr(unreal.MaterialExpressionConstant3Vector, -400, 350)
flat_normal.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0))

lerp_normal = add_expr(unreal.MaterialExpressionLinearInterpolate, -200, 200)
unreal.MaterialEditingLibrary.connect_material_expressions(samp_normal, "", lerp_normal, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(flat_normal, "", lerp_normal, "B")
unreal.MaterialEditingLibrary.connect_material_expressions(samp_mask, "B", lerp_normal, "Alpha")

unreal.MaterialEditingLibrary.connect_material_property(lerp_normal, "", unreal.MaterialProperty.MP_NORMAL)

# 8. Roughness Modulation:
scalar_roughness_base = add_expr(unreal.MaterialExpressionScalarParameter, -400, 500)
scalar_roughness_base.set_editor_property("parameter_name", "RoughnessBase")
scalar_roughness_base.set_editor_property("default_value", 0.75)

mult_roughness = add_expr(unreal.MaterialExpressionMultiply, -200, 480)
unreal.MaterialEditingLibrary.connect_material_expressions(samp_roughness, "R", mult_roughness, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(scalar_roughness_base, "", mult_roughness, "B")

# Skin roughness is ~0.45 (flesh specular highlight)
skin_roughness = add_expr(unreal.MaterialExpressionConstant, -200, 600)
skin_roughness.set_editor_property("r", 0.45)

lerp_roughness_skin = add_expr(unreal.MaterialExpressionLinearInterpolate, 0, 500)
unreal.MaterialEditingLibrary.connect_material_expressions(mult_roughness, "", lerp_roughness_skin, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(skin_roughness, "", lerp_roughness_skin, "B")
unreal.MaterialEditingLibrary.connect_material_expressions(samp_mask, "B", lerp_roughness_skin, "Alpha")

unreal.MaterialEditingLibrary.connect_material_property(lerp_roughness_skin, "", unreal.MaterialProperty.MP_ROUGHNESS)

# 9. Subsurface Scattering for Realistic Skin Shading
scalar_sss = add_expr(unreal.MaterialExpressionScalarParameter, -200, -100)
scalar_sss.set_editor_property("parameter_name", "SubsurfaceIntensity")
scalar_sss.set_editor_property("default_value", 0.40)

# Warm subsurface tint: fleshy peach/red scattering
sss_tint = add_expr(unreal.MaterialExpressionConstant3Vector, -200, -200)
sss_tint.set_editor_property("constant", unreal.LinearColor(1.0, 0.45, 0.35, 1.0))

mult_sss = add_expr(unreal.MaterialExpressionMultiply, 0, -150)
unreal.MaterialEditingLibrary.connect_material_expressions(sss_tint, "", mult_sss, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(scalar_sss, "", mult_sss, "B")

mult_sss_mask = add_expr(unreal.MaterialExpressionMultiply, 200, -150)
unreal.MaterialEditingLibrary.connect_material_expressions(mult_sss, "", mult_sss_mask, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(samp_mask, "B", mult_sss_mask, "B")

try:
    unreal.MaterialEditingLibrary.connect_material_property(mult_sss_mask, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
except Exception as e:
    print(f"Subsurface color connection note: {e}")

# Recompile Material
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
print("Successfully compiled and saved M_Operator_BaseBody!")

# -------------------------------------------------------------------------
# Step 3: Assign M_Operator_BaseBody to SK_BaseMale Slot 0
# -------------------------------------------------------------------------
sk_mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/BaseMale/SK_BaseMale")
if sk_mesh:
    materials = sk_mesh.get_editor_property("materials")
    if len(materials) > 0:
        materials[0].set_editor_property("material_interface", material)
        materials[0].set_editor_property("material_slot_name", "M_Operator_BaseBody")
        sk_mesh.set_editor_property("materials", materials)
        unreal.EditorAssetLibrary.save_loaded_asset(sk_mesh)
        print("Successfully assigned M_Operator_BaseBody to SK_BaseMale slot 0!")
    else:
        print("WARNING: SK_BaseMale has 0 materials slots")

unreal.EditorAssetLibrary.save_directory("/Game/Characters/BaseMale", only_if_is_dirty=False, recursive=True)
print(">>> ALL OPERATOR MATERIAL ASSETS GENERATED & BOUND SUCCESSFULLY <<<")
sys.exit(0)
