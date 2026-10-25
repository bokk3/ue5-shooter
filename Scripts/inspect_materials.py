import unreal

print(">>> INSPECTING MATERIALS & STATIC MESHES <<<")

def inspect_mat(path):
    m = unreal.EditorAssetLibrary.load_asset(path)
    if not m:
        print(f"FAILED to load {path}")
        return
    print(f"Material: {path}")
    print(f"  Class: {m.get_class().get_name()}")
    if isinstance(m, unreal.Material):
        print(f"  BlendMode: {m.get_editor_property('blend_mode')}")
        print(f"  ShadingModel: {m.get_editor_property('shading_model')}")
        print(f"  TwoSided: {m.get_editor_property('two_sided')}")

for p in [
    "/Game/Characters/BaseMale/Materials/M_Operator_BaseBody",
    "/Game/Characters/BaseMale/Materials/M_Soldier_Visor",
    "/Game/Characters/BaseMale/Materials/M_AK47_Steel",
    "/Game/Characters/BaseMale/Materials/M_AK47_Wood",
    "/Game/Characters/BaseMale/Materials/M_AK47_Bakelite"
]:
    inspect_mat(p)

for mesh_path in [
    "/Game/Characters/BaseMale/SM_Soldier_Posed",
    "/Game/Environment/Hangar/SM_Hangar_Plinth",
    "/Game/Environment/Hangar/SM_Hangar_Floor"
]:
    sm = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if sm:
        bounds = sm.get_bounds()
        print(f"Mesh: {mesh_path}")
        print(f"  Origin: {bounds.origin}")
        print(f"  BoxExtent: {bounds.box_extent}")
        print(f"  SphereRadius: {bounds.sphere_radius}")
        num_mats = sm.get_num_sections(0)
        for i in range(num_mats):
            mat = sm.get_material(i)
            print(f"  Slot {i}: {mat.get_name() if mat else 'NONE'}")
    else:
        print(f"FAILED to load mesh {mesh_path}")

print(">>> INSPECTION COMPLETE <<<")
