"""ASSET-BLD-1 gallery proof: one inspection-distance render per kit GLB.

Run headless:
    blender --background --python render_gallery.py
Then montage (ImageMagick):
    montage /tmp/bld1_gallery/cell_*.png -tile 6x -geometry +2+2 \
            /tmp/bld1_gallery/gallery.png

Cells are numbered in the deterministic order below; the manifest printed at
the end maps cell index -> asset id (also written to manifest.txt).
"""
import math
import os

import bpy

W = os.path.abspath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)), os.pardir))
OUT = "/tmp/bld1_gallery"
os.makedirs(OUT, exist_ok=True)

ASSETS = [
    "openings/door_frame_standard",
    "openings/door_leaf_standard_closed",
    "openings/door_leaf_standard_open",
    "openings/door_frame_wide",
    "openings/door_leaf_wide_closed",
    "openings/door_leaf_wide_open",
    "openings/window_frame_standard",
    "openings/window_frame_small",
    "openings/window_shutter_left_closed",
    "openings/window_shutter_left_open",
    "openings/window_shutter_right_closed",
    "openings/window_shutter_right_open",
    "openings/window_mullion_cross",
    "openings/window_sill_standard",
    "openings/window_lintel_standard",
    "traversal/stair_straight_3m",
    "traversal/stair_landing_2x2m",
    "traversal/stair_straight_3m_with_rails",
    "traversal/stair_rail_slope_3m",
    "traversal/stair_newel_post",
    "structural/railing_straight_2m",
    "roof/ridge_cap_straight_4m",
    "roof/ridge_cap_straight_2m",
    "roof/ridge_cap_end",
    "roof/eave_trim_2m",
    "roof/eave_trim_4m",
    "roof/eave_outer_corner",
    "roof/fascia_end",
    "roof/gable_cap_4m",
    "roof/gutter_straight_2m",
    "roof/downspout_3m",
    "roof/downspout_outlet",
    "roof/chimney_stack_short",
    "roof/chimney_cap",
]


def reset():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.materials):
        bpy.data.materials.remove(m)
    for i in list(bpy.data.images):
        if i.users == 0:
            bpy.data.images.remove(i)


def render_cell(path, objs):
    import mathutils
    lo = mathutils.Vector((1e9,) * 3)
    hi = mathutils.Vector((-1e9,) * 3)
    for o in objs:
        if o.type != "MESH":
            continue
        for c in o.bound_box:
            wc = o.matrix_world @ mathutils.Vector(c)
            lo = mathutils.Vector(map(min, lo, wc))
            hi = mathutils.Vector(map(max, hi, wc))
    center = (lo + hi) / 2
    size = max((hi - lo).length, 0.5)
    scene = bpy.context.scene
    sun = bpy.data.objects.new("s", bpy.data.lights.new("s", "SUN"))
    bpy.context.collection.objects.link(sun)
    sun.data.energy = 2.2
    sun.data.color = (1.0, 0.95, 0.88)
    sun.rotation_euler = (math.radians(55), 0, math.radians(-35))
    fill = bpy.data.objects.new("f", bpy.data.lights.new("f", "SUN"))
    bpy.context.collection.objects.link(fill)
    fill.data.energy = 0.6
    fill.rotation_euler = (math.radians(60), 0, math.radians(140))
    cam_d = bpy.data.cameras.new("c")
    cam = bpy.data.objects.new("c", cam_d)
    bpy.context.collection.objects.link(cam)
    import mathutils as mu
    direction = mu.Vector((1.0, -1.2, 0.7)).normalized()
    cam.location = center + direction * size * 1.6
    target = bpy.data.objects.new("t", None)
    bpy.context.collection.objects.link(target)
    target.location = center
    tr = cam.constraints.new("TRACK_TO")
    tr.target = target
    tr.track_axis = "TRACK_NEGATIVE_Z"
    tr.up_axis = "UP_Y"
    scene.camera = cam
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 48
    scene.cycles.use_denoising = True
    scene.cycles.device = "CPU"
    scene.view_settings.view_transform = "Filmic"
    scene.view_settings.exposure = -0.2
    scene.render.resolution_x = 512
    scene.render.resolution_y = 400
    scene.render.film_transparent = False
    world = bpy.data.worlds.get("gallery") or bpy.data.worlds.new("gallery")
    scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    bg.inputs["Color"].default_value = (0.72, 0.72, 0.74, 1.0)
    bg.inputs["Strength"].default_value = 0.55
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


manifest = []
for idx, asset_id in enumerate(ASSETS):
    reset()
    bpy.ops.import_scene.gltf(filepath=os.path.join(W, asset_id + ".glb"))
    objs = list(bpy.data.objects)
    cell = os.path.join(OUT, "cell_%02d.png" % idx)
    render_cell(cell, objs)
    manifest.append("%02d %s" % (idx, asset_id))
    print("CELL", idx, asset_id)

with open(os.path.join(OUT, "manifest.txt"), "w") as fh:
    fh.write("\n".join(manifest) + "\n")
print("\n".join(manifest))
print("GALLERY CELLS COMPLETE: %d" % len(ASSETS))
