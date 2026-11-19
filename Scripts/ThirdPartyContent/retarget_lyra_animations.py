"""Retarget the copied Lyra character animations (UE5 Mannequin skeleton) onto the UEFN Mannequin
skeleton that the Game Animation Sample operator uses.

Creates  /Game/Characters/UEFN_Mannequin/Rigs/RTG_LyraManny_to_UEFN
Outputs  /Game/Characters/UEFN_Mannequin/Animations/Lyra/<same sub-folders as Lyra>/<name>
         (sequences, montages, blend spaces and aim offsets; weapon-skeleton animations untouched)

Run headless in Noble Fermi after copy_lyra_content.py (see README.md):
    UnrealEditor-Cmd.exe NobleFermi.uproject -run=pythonscript -script=<this file> -unattended -nullrhi
"""
import os
import time

import unreal

LYRA_ANIMS = "/Game/Characters/Heroes/Mannequin/Animations"
OUT_ROOT = "/Game/Characters/UEFN_Mannequin/Animations/Lyra"
RTG_DIR, RTG_NAME = "/Game/Characters/UEFN_Mannequin/Rigs", "RTG_LyraManny_to_UEFN"
SOURCE_RIG = "/Game/Characters/Heroes/Mannequin/Rig/IK_Mannequin"
TARGET_RIG = "/Game/Characters/UEFN_Mannequin/Rigs/IK_UEFN_Mannequin"
SOURCE_MESH = "/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny"
TARGET_MESH = "/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin"
SUFFIX = "_UEFNTMP"
RETARGET_CLASSES = {"AnimSequence", "AnimMontage", "BlendSpace", "BlendSpace1D", "AimOffsetBlendSpace",
                    "AimOffsetBlendSpace1D"}

log = unreal.log
eal = unreal.EditorAssetLibrary
SRC, TGT = unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetSourceOrTarget.TARGET


def try_call(label, fn, *variants):
    for args in variants:
        try:
            r = fn(*args)
            log(f"[Retarget] {label}{args} ok")
            return r
        except Exception as e:  # signatures moved between 5.x releases
            log(f"[Retarget] {label}{args} failed: {e}")
    return None


# 1. Retargeter -----------------------------------------------------------------------------------
source_rig, target_rig = unreal.load_asset(SOURCE_RIG), unreal.load_asset(TARGET_RIG)
source_mesh, target_mesh = unreal.load_asset(SOURCE_MESH), unreal.load_asset(TARGET_MESH)
for n, a in (("source rig", source_rig), ("target rig", target_rig), ("source mesh", source_mesh), ("target mesh", target_mesh)):
    if a is None:
        raise RuntimeError(f"{n} missing; run copy_lyra_content.py and copy IK_UEFN_Mannequin first")

rtg_path = f"{RTG_DIR}/{RTG_NAME}"
if eal.does_asset_exist(rtg_path):
    rtg = unreal.load_asset(rtg_path)  # reconfigured from scratch below
else:
    rtg = unreal.AssetToolsHelpers.get_asset_tools().create_asset(RTG_NAME, RTG_DIR, unreal.IKRetargeter, unreal.IKRetargetFactory())
ctrl = unreal.IKRetargeterController.get_controller(rtg)
ctrl.set_ik_rig(SRC, source_rig)
ctrl.set_ik_rig(TGT, target_rig)
try_call("set_preview_mesh", ctrl.set_preview_mesh, (SRC, source_mesh))
try_call("set_preview_mesh", ctrl.set_preview_mesh, (TGT, target_mesh))
try_call("remove_all_ops", ctrl.remove_all_ops, ())
try_call("add_default_ops", ctrl.add_default_ops, ())
try_call("assign_ik_rig_to_all_ops", ctrl.assign_ik_rig_to_all_ops, (SRC, source_rig))
try_call("assign_ik_rig_to_all_ops", ctrl.assign_ik_rig_to_all_ops, (TGT, target_rig))
try_call("auto_map_chains", ctrl.auto_map_chains,
         (unreal.AutoMapChainType.FUZZY, True), (unreal.AutoMapChainType.FUZZY, True, ""))
try_call("auto_align_all_bones", ctrl.auto_align_all_bones, (TGT,))
for s in ctrl.get_all_chain_settings() or []:
    log(f"[Retarget] chain {s.target_chain} <- {s.source_chain}")
disable = [n for n in os.environ.get("LYRA_RETARGET_DISABLE_OPS", "").split(",") if n]
for i in range(ctrl.get_num_retarget_ops()):
    name = str(ctrl.get_op_name(i))
    if any(d.lower() in name.lower() for d in disable):
        try_call("set_retarget_op_enabled", ctrl.set_retarget_op_enabled, (i, False))
    log(f"[Retarget] op {i}: {name} enabled={ctrl.get_retarget_op_enabled(i)}")
eal.save_loaded_asset(rtg)

# 2. Pick what to retarget: everything on Lyra's character skeleton -------------------------------
ar = unreal.AssetRegistryHelpers.get_asset_registry()
ar.wait_for_completion()
to_retarget = [d for d in ar.get_assets_by_path(LYRA_ANIMS, recursive=True)
               if str(d.asset_class_path.asset_name) in RETARGET_CLASSES]
only = {c for c in os.environ.get("LYRA_RETARGET_CLASSES", "").split(",") if c}
if only:
    to_retarget = [d for d in to_retarget if str(d.asset_class_path.asset_name) in only]
if os.environ.get("LYRA_RETARGET_CLEAN") == "1" and eal.does_directory_exist(OUT_ROOT):
    log(f"[Retarget] clearing previous output {OUT_ROOT}: {eal.delete_directory(OUT_ROOT)}")
limit = int(os.environ.get("LYRA_RETARGET_LIMIT", "0"))
if limit:
    to_retarget = [d for d in to_retarget if str(d.asset_class_path.asset_name) == "AnimSequence"][:limit]
log(f"[Retarget] {len(to_retarget)} assets to retarget")
t0 = time.time()

# 3. Retarget (duplicates next to the source with a temporary suffix, references remapped) ---------
batch = unreal.IKRetargetBatchOperation.duplicate_and_retarget
log(f"[Retarget] signature: {batch.__doc__}")
# 5.8 added a target_path parameter; pass everything after the meshes by name.
# Pulling referenced sequences along with montages/blend spaces hangs the 5.8 batch operation, so the
# second pass runs with LYRA_RETARGET_INCLUDE_REFS=0 and repoints them afterwards (step 5).
include_refs = os.environ.get("LYRA_RETARGET_INCLUDE_REFS", "1") == "1"
results = None
# Some Lyra montages hang the 5.8 batch op forever (first seen: AM_MM_HitReact_Back_Med_01). Montages
# whose name contains a LYRA_RETARGET_SKIP token are rebuilt from their retargeted sequence instead
# (step 4b).
skip = [n for n in os.environ.get("LYRA_RETARGET_SKIP", "").split(",") if n]
rebuild = [d for d in to_retarget if any(s in str(d.asset_name) for s in skip)]
to_retarget = [d for d in to_retarget if d not in rebuild]
if os.environ.get("LYRA_RETARGET_REBUILD_ONLY") == "1":
    to_retarget = []
if os.environ.get("LYRA_RETARGET_ONE_BY_ONE") == "1":
    # Containers hang the batch op in 5.8; one call per asset shows which one (last "start" line).
    results = []
    for i, d in enumerate(to_retarget):
        log(f"[Retarget] start {i + 1}/{len(to_retarget)} {d.asset_name} ({d.asset_class_path.asset_name})")
        t1 = time.time()
        results += list(batch([d], source_mesh, target_mesh, rtg, suffix=SUFFIX, use_source_path=True,
                              include_referenced_assets=False))
        log(f"[Retarget] done {d.asset_name} in {time.time() - t1:.1f}s")
else:
    for kw in ({"suffix": SUFFIX, "use_source_path": True, "include_referenced_assets": include_refs},
               {"suffix": SUFFIX, "include_referenced_assets": include_refs}):
        try:
            results = batch(to_retarget, source_mesh, target_mesh, rtg, **kw)
            log(f"[Retarget] duplicate_and_retarget({sorted(kw)}) ok")
            break
        except Exception as e:
            log(f"[Retarget] duplicate_and_retarget({sorted(kw)}) failed: {e}")
if results is None:
    raise RuntimeError("duplicate_and_retarget: no signature matched; see the logged signature")
log(f"[Retarget] batch produced {len(results)} assets in {time.time() - t0:.1f}s")

# 4. Move the results under OUT_ROOT with their original names ------------------------------------
# Montages and blend spaces drag their sequences along (include_referenced_assets). When a sequence
# was retargeted by an earlier run, point the new copies at the existing one instead of renaming.
renames, merged = [], 0
for d in results:
    pkg, name = str(d.package_path), str(d.asset_name)
    if not name.endswith(SUFFIX) or not pkg.startswith(LYRA_ANIMS):
        log(f"[Retarget] leaving {pkg}/{name} in place")
        continue
    new_pkg, new_name = OUT_ROOT + pkg[len(LYRA_ANIMS):], name[:-len(SUFFIX)]
    existing = f"{new_pkg}/{new_name}"
    if eal.does_asset_exist(existing):
        eal.consolidate_assets(unreal.load_asset(existing), [d.get_asset()])
        merged += 1
        continue
    renames.append(unreal.AssetRenameData(d.get_asset(), new_pkg, new_name))
log(f"[Retarget] moving {len(renames)}, merging {merged} into earlier output")
if renames:
    unreal.AssetToolsHelpers.get_asset_tools().rename_assets(renames)
# 4b. Rebuild skipped montages: one new montage per montage, playing the retargeted copy of the
#     sequence the Lyra montage plays (found through the asset registry), in the same slot.
dep_opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False, include_hard_package_references=True,
                                                 include_searchable_names=False, include_soft_management_references=False,
                                                 include_hard_management_references=False)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
rebuilt = 0
for d in rebuild:
    if str(d.asset_class_path.asset_name) != "AnimMontage":
        log(f"[Retarget] cannot rebuild {d.asset_name} ({d.asset_class_path.asset_name}); skipped")
        continue
    # Lyra names montages AM_<sequence>, next to the sequence (its dependencies can't be used: the
    # montage also references the additive base pose, and after step 5 they are redirectors).
    out_pkg = OUT_ROOT + str(d.package_path)[len(LYRA_ANIMS):]
    seq_path = f"{out_pkg}/{str(d.asset_name)[len('AM_'):]}"
    uefn_seq = unreal.load_asset(seq_path) if eal.does_asset_exist(seq_path) else None
    if uefn_seq is None:
        log(f"[Retarget] cannot rebuild {d.asset_name}: no retargeted {seq_path}")
        continue
    if eal.does_asset_exist(f"{out_pkg}/{d.asset_name}"):
        eal.delete_asset(f"{out_pkg}/{d.asset_name}")
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", uefn_seq)
    factory.set_editor_property("target_skeleton", uefn_seq.get_editor_property("skeleton"))
    montage = asset_tools.create_asset(str(d.asset_name), out_pkg, unreal.AnimMontage, factory)
    src = d.get_asset()
    for prop in ("blend_in", "blend_out", "blend_out_trigger_time", "enable_auto_blend_out"):
        try:
            montage.set_editor_property(prop, src.get_editor_property(prop))
        except Exception:
            pass
    rebuilt += 1  # plays in DefaultSlot; the operator's anim graph decides which slots it blends
log(f"[Retarget] rebuilt {rebuilt}/{len(rebuild)} skipped montages")

eal.save_directory(OUT_ROOT, only_if_is_dirty=False, recursive=True)
eal.save_directory(RTG_DIR, only_if_is_dirty=False, recursive=True)

# 5. Repoint: merge each Lyra-skeleton sequence into its UEFN copy so every reference to it (the new
#    montages and blend spaces) now uses the retargeted one. The Lyra originals become redirectors.
if not include_refs:
    pairs = 0
    for d in ar.get_assets_by_path(OUT_ROOT, recursive=True):
        if str(d.asset_class_path.asset_name) != "AnimSequence":
            continue
        src = LYRA_ANIMS + str(d.package_name)[len(OUT_ROOT):]
        if eal.does_asset_exist(src):
            dst_asset, src_asset = d.get_asset(), unreal.load_asset(src)
            if src_asset is None or src_asset == dst_asset:  # already a redirector to the copy
                continue
            eal.consolidate_assets(dst_asset, [src_asset])
            pairs += 1
    log(f"[Retarget] repointed {pairs} sequences")
    eal.save_directory(OUT_ROOT, only_if_is_dirty=True, recursive=True)
    eal.save_directory(LYRA_ANIMS, only_if_is_dirty=True, recursive=True)

# 6. Rewrite every reference that still goes through a Lyra redirector, then drop the redirectors.
#    load_asset follows a redirector, so load the package and then find the redirector object itself.
redirectors = []
for d in ar.get_assets_by_path(LYRA_ANIMS, recursive=True):
    if str(d.asset_class_path.asset_name) == "ObjectRedirector":
        unreal.load_asset(str(d.package_name))
        r = unreal.find_object(None, f"{d.package_name}.{d.asset_name}")
        if r is not None:
            redirectors.append(r)
log(f"[Retarget] {len(redirectors)} redirectors under {LYRA_ANIMS}")
tools = unreal.AssetToolsHelpers.get_asset_tools()
fixup = getattr(tools, "fixup_referencers", None)
if redirectors and fixup:
    fixup(redirectors, False)
    log(f"[Retarget] fixed up referencers of {len(redirectors)} redirectors")
    eal.save_directory(OUT_ROOT, only_if_is_dirty=True, recursive=True)
elif redirectors:
    # Not exposed to Python in 5.8; ResavePackages -fixupredirects does it (see README).
    log(f"[Retarget] fixup not scriptable here; AssetTools has {[m for m in dir(tools) if 'redirect' in m.lower() or 'fix' in m.lower()]}")

skel_ok = 0
for d in ar.get_assets_by_path(OUT_ROOT, recursive=True):
    a = d.get_asset()
    sk = a.get_editor_property("skeleton") if hasattr(a, "get_editor_property") else None
    if sk and "UEFN" in sk.get_path_name():
        skel_ok += 1
dep_opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=False, include_hard_package_references=True,
                                                 include_searchable_names=False, include_soft_management_references=False,
                                                 include_hard_management_references=False)
still_lyra, lyra_targets = [], {}
for d in ar.get_assets_by_path(OUT_ROOT, recursive=True):
    for x in ar.get_dependencies(d.package_name, dep_opts) or []:
        if str(x).startswith(LYRA_ANIMS):
            still_lyra.append(str(d.package_name))
            cls = ",".join(str(a.asset_class_path.asset_name) for a in ar.get_assets_by_package_name(x)) or "missing"
            lyra_targets[cls] = lyra_targets.get(cls, 0) + 1
log(f"[Retarget] outputs still referencing Lyra paths: {len(set(still_lyra))} (by target class {lyra_targets}) "
    f"e.g. {sorted(set(still_lyra))[:3]}")
log(f"[Retarget] LYRA_RETARGET_DONE moved={len(renames)} on_uefn_skeleton={skel_ok}")

# Drop Python's handles on UObjects before the commandlet tears Python down (avoids a crash on exit).
a = d = sk = None
del results, renames, rtg, ctrl, to_retarget, source_rig, target_rig, source_mesh, target_mesh
import gc
gc.collect()
