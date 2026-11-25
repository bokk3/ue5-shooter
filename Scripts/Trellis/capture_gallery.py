"""Render L_AssetGallery from each Shot_* camera to PNGs, then close the editor. Needs a real RHI (not a commandlet):

    UnrealEditor.exe NobleFermi.uproject /Game/NobleFermi/Maps/L_AssetGallery -ExecutePythonScript=Scripts/Trellis/capture_gallery.py

Waits NF_GALLERY_WAIT seconds (default 90) after the map opens, then for each camera moves a SceneCapture2D onto it,
captures for WARM_FRAMES frames (texture streaming and Lumen settle) and exports the render target to
Saved/Screenshots/Gallery/<camera>.png (1600x900).

A scene capture renders whether or not the editor window is in front; high-res screenshots of the editor viewport
were dropped while the window was in the background. The first run after an import is slow while textures build.
"""
import os
import time

import unreal

WAIT = float(os.environ.get("NF_GALLERY_WAIT", "90"))
WARM_FRAMES = 30
WIDTH, HEIGHT = 1600, 900
OUT = os.path.join(os.path.abspath(unreal.Paths.project_saved_dir()), "Screenshots", "Gallery")
log = unreal.log

state = {"start": time.time(), "cams": None, "i": 0, "warm": 0, "handle": None, "busy": False,
         "capture": None, "target": None}


def setup():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    state["cams"] = sorted((a for a in eas.get_all_level_actors()
                            if isinstance(a, unreal.CameraActor) and a.get_actor_label().startswith("Shot_")),
                           key=lambda a: a.get_actor_label())
    target = unreal.RenderingLibrary.create_render_target2d(world, WIDTH, HEIGHT, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    capture = eas.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0), transient=True)
    comp = capture.capture_component2d
    comp.set_editor_property("texture_target", target)
    comp.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    comp.set_editor_property("capture_every_frame", False)
    state.update(world=world, target=target, capture=capture)
    os.makedirs(OUT, exist_ok=True)
    log(f"[GalleryShot] {len(state['cams'])} cameras -> {OUT}")


def step():
    if time.time() - state["start"] < WAIT:
        return
    if state["cams"] is None:
        setup()
        return
    if state["i"] >= len(state["cams"]):
        log("[GalleryShot] GALLERY_SHOTS_DONE")
        unreal.unregister_slate_post_tick_callback(state["handle"])
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)  # -ExecutePythonScript now closes the editor
        return
    cam = state["cams"][state["i"]]
    comp = state["capture"].capture_component2d
    if state["warm"] == 0:
        state["capture"].set_actor_location_and_rotation(cam.get_actor_location(), cam.get_actor_rotation(), False, True)
        comp.set_editor_property("fov_angle", cam.camera_component.get_editor_property("field_of_view"))
    comp.capture_scene()
    state["warm"] += 1
    if state["warm"] < WARM_FRAMES:
        return
    name = cam.get_actor_label()
    unreal.RenderingLibrary.export_render_target(state["world"], state["target"], OUT, f"{name}.png")
    log(f"[GalleryShot] {name}.png")
    state["i"] += 1
    state["warm"] = 0


def tick(_dt):
    if state["busy"]:
        return  # Slate ticks while a texture build blocks the frame; don't re-enter
    state["busy"] = True
    try:
        step()
    except Exception as e:  # noqa: BLE001  never leave the editor running headless-ish on an error
        unreal.log_error(f"[GalleryShot] failed: {e}")
        unreal.unregister_slate_post_tick_callback(state["handle"])
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    finally:
        state["busy"] = False


# Without this, -ExecutePythonScript closes the editor as soon as this file returns.
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
state["handle"] = unreal.register_slate_post_tick_callback(tick)
log(f"[GalleryShot] waiting {WAIT:.0f}s before capturing")
