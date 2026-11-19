import unreal, json, os

import os
OUT = os.environ.get("GASP_DEPS_OUT", os.path.join(os.environ.get("TEMP", "."), "gasp_deps.json"))

ROOTS = [
    "/Game/Blueprints/SandboxCharacter_CMC",
    "/Game/Blueprints/SandboxCharacter_CMC_ABP",
]

ar = unreal.AssetRegistryHelpers.get_asset_registry()
ar.wait_for_completion()

opts_hard = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False, include_hard_package_references=True,
                                                  include_searchable_names=False, include_soft_management_references=False,
                                                  include_hard_management_references=False)
opts_all = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                                 include_searchable_names=False, include_soft_management_references=False,
                                                 include_hard_management_references=False)

def closure(opts):
    seen, stack = set(), list(ROOTS)
    while stack:
        p = stack.pop()
        if p in seen:
            continue
        seen.add(p)
        for d in ar.get_dependencies(p, opts) or []:
            d = str(d)
            if d.startswith("/Game/") and d not in seen:
                stack.append(d)
    return sorted(seen)

hard = closure(opts_hard)
allp = closure(opts_all)

api = {
    "has_migrate_packages": hasattr(unreal.AssetTools, "migrate_packages"),
    "has_MigrationOptions": hasattr(unreal, "MigrationOptions"),
    "has_reparent_blueprint": hasattr(unreal.BlueprintEditorLibrary, "reparent_blueprint"),
}

# Parent class of the CMC character and skeleton/mesh of the ABP
info = {}
try:
    bp = unreal.load_asset("/Game/Blueprints/SandboxCharacter_CMC")
    gen = bp.generated_class()
    cdo = unreal.get_default_object(gen)
    info["cmc_parent"] = str(unreal.BlueprintEditorLibrary.get_blueprint_parent_class(bp)) if hasattr(unreal.BlueprintEditorLibrary, "get_blueprint_parent_class") else "?"
    mesh = cdo.get_editor_property("mesh")
    info["cmc_mesh"] = str(mesh.get_editor_property("skeletal_mesh_asset").get_path_name()) if mesh else None
    info["cmc_anim_class"] = str(mesh.get_editor_property("anim_class").get_path_name()) if mesh and mesh.get_editor_property("anim_class") else None
    info["cmc_components"] = [c.get_name() + ":" + c.get_class().get_name() for c in (cdo.get_components_by_class(unreal.ActorComponent) or [])]
except Exception as e:
    info["err"] = str(e)
try:
    abp = unreal.load_asset("/Game/Blueprints/SandboxCharacter_CMC_ABP")
    info["abp_skeleton"] = str(abp.get_editor_property("target_skeleton").get_path_name())
except Exception as e:
    info["abp_err"] = str(e)

json.dump({"api": api, "info": info, "hard": hard, "all": allp}, open(OUT, "w"), indent=1)
print("GASP_DEPS_DONE", len(hard), len(allp))
