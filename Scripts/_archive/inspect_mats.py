import unreal

out_path = r"C:\Users\Boris\Documents\antigravity\noble-fermi\Intermediate\mat_inspect.txt"
lines = []

for mat_path in ["/Game/Characters/BaseMale/Materials/M_Operator_BaseBody", "/Game/Characters/BaseMale/Materials/M_Soldier_Visor"]:
    m = unreal.EditorAssetLibrary.load_asset(mat_path)
    if m:
        lines.append(f"Material {mat_path}:")
        try:
            skel_use = m.get_editor_property("used_with_skeletal_mesh")
            lines.append(f"  used_with_skeletal_mesh: {skel_use}")
        except Exception as e:
            lines.append(f"  used_with_skeletal_mesh error: {e}")
    else:
        lines.append(f"Material {mat_path}: NOT FOUND")

with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
