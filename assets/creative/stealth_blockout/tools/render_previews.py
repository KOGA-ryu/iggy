"""Headless preview renders for the stealth blockout kit.

Run headless only:
    ~/.local/bin/blender --background --python render_previews.py

Writes one PNG per asset plus a scale-sanity composite (every family lined up
beside guard_post_1p8) into ~/stealth_blockout_review/. Cycles CPU so no GPU
surface or window is ever created.
"""

import glob
import math
import os
import sys

import bpy
from mathutils import Vector

KIT_DIR = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))
OUT_DIR = os.path.expanduser("~/stealth_blockout_review")


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 48
    scene.render.resolution_x = 640
    scene.render.resolution_y = 480
    scene.render.film_transparent = False
    scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    bg = scene.world.node_tree.nodes["Background"]
    bg.inputs[0].default_value = (0.85, 0.87, 0.90, 1.0)
    bg.inputs[1].default_value = 1.0


def add_light_and_floor(span):
    bpy.ops.object.light_add(type="SUN", location=(4, -4, 8))
    sun = bpy.context.active_object
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(35), math.radians(15), math.radians(30))
    bpy.ops.mesh.primitive_plane_add(size=max(20.0, span * 4.0), location=(0, 0, 0))
    floor = bpy.context.active_object
    mat = bpy.data.materials.new("Floor")
    mat.use_nodes = True
    mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (
        0.70, 0.70, 0.70, 1.0)
    floor.data.materials.append(mat)


def imported_objects(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    return [o for o in bpy.data.objects if o not in before and o.type == "MESH"]


def scene_bounds(objects):
    lo = [1e9] * 3
    hi = [-1e9] * 3
    for obj in objects:
        for corner in obj.bound_box:
            world = obj.matrix_world @ Vector(corner)
            for i in range(3):
                lo[i] = min(lo[i], world[i])
                hi[i] = max(hi[i], world[i])
    return lo, hi


def frame_camera(lo, hi):
    cx, cy, cz = [(lo[i] + hi[i]) / 2.0 for i in range(3)]
    span = max(hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2])
    dist = span * 2.2 + 2.0
    bpy.ops.object.camera_add()
    cam = bpy.context.active_object
    az = math.radians(-60)
    cam.location = (cx + dist * math.cos(az), cy + dist * math.sin(az),
                    cz + dist * 0.55)
    direction = (cx - cam.location.x, cy - cam.location.y, cz - cam.location.z)
    rot_z = math.atan2(direction[1], direction[0]) - math.pi / 2.0
    horiz = math.hypot(direction[0], direction[1])
    rot_x = math.pi / 2.0 - math.atan2(-direction[2], horiz)
    cam.rotation_euler = (rot_x, 0.0, rot_z)
    bpy.context.scene.camera = cam


def render_to(path):
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print("RENDERED", path)


def render_asset(glb_path):
    name = os.path.splitext(os.path.basename(glb_path))[0]
    reset_scene()
    objs = imported_objects(glb_path)
    lo, hi = scene_bounds(objs)
    add_light_and_floor(max(hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]))
    frame_camera(lo, hi)
    render_to(os.path.join(OUT_DIR, name + ".png"))


def render_composite(glb_paths):
    """Every asset in one row next to the guard post — scale sanity check."""
    reset_scene()
    ordered = sorted(glb_paths,
                     key=lambda p: os.path.basename(p) != "guard_post_1p8.glb")
    cursor = 0.0
    all_objs = []
    for path in ordered:
        objs = imported_objects(path)
        lo, hi = scene_bounds(objs)
        width = hi[0] - lo[0]
        shift = cursor - lo[0]
        for obj in objs:
            if obj.parent is None:
                obj.location.x += shift
        cursor += width + 0.8
        all_objs.extend(objs)
    bpy.context.view_layer.update()
    lo, hi = scene_bounds(all_objs)
    add_light_and_floor(hi[0] - lo[0])
    scene = bpy.context.scene
    scene.render.resolution_x = 4800
    scene.render.resolution_y = 600
    cx = (lo[0] + hi[0]) / 2.0
    span = (hi[0] - lo[0]) * 1.04
    bpy.ops.object.camera_add()
    cam = bpy.context.active_object
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = span
    vertical = span * (600.0 / 4800.0)
    cam.location = (cx, -30.0, vertical / 2.0 - 0.5)
    cam.rotation_euler = (math.radians(90), 0.0, 0.0)
    scene.camera = cam
    render_to(os.path.join(OUT_DIR, "composite_lineup.png"))


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    glbs = sorted(glob.glob(os.path.join(KIT_DIR, "*.glb")))
    if "--composite-only" not in sys.argv:
        for path in glbs:
            render_asset(path)
    render_composite(glbs)
    print("PREVIEWS COMPLETE:", len(glbs))


if __name__ == "__main__":
    main()
