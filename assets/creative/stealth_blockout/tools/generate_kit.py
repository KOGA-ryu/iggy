"""Stealth blockout kit generator.

Run headless only:
    ~/.local/bin/blender --background --python generate_kit.py

Regenerates every .glb in assets/creative/stealth_blockout/ from the metric
constants in ../README.md (extracted from engine code — see the README table
for file:line provenance). Blockout-grade: axis-aligned boxes/cylinders, one
flat-shaded material per asset, base resting on the ground plane, XZ centered.

Conventions matched to the existing creative kit:
  single collidable box   -> node extras {iggy_category, iggy_collision:"bounds",
                             iggy_walkable} (walkway_stone_01 pattern)
  multi-box piece         -> every node carries iggy_category +
                             iggy_collision:"compound_bounds"; collidable nodes
                             add iggy_collision_part:"bounds" +
                             iggy_collision_part_walkable (cave/ramp_4x4x1p5 and
                             infrastructure/retaining_wall_2x1p2 pattern)
"""

import os

import bpy

# ---------------------------------------------------------------------------
# Measured engine metrics (provenance in ../README.md)
# ---------------------------------------------------------------------------
PLAYER_HEIGHT = 1.80      # MovementParams.hpp:8
PLAYER_RADIUS = 0.30      # MovementParams.hpp:7
STEP_HEIGHT = 0.35        # MovementParams.hpp:10
GUARD_EYE = 1.6           # NpcBehaviorProfile.hpp:25
SNEAK_EYE = 0.9           # NpcBehaviorProfile.hpp:27
EDITOR_EYE = 1.7          # CreativeSceneFrame.cpp:67
SNAP_QUARTER = 0.25       # Tools.hpp:91-96 / ToolSettingLabels.cpp:38
# Clamber band: NOT in code (TraversalTag.hpp:15-17 is tag-only). Band top is
# assumed = player height; the fail block sits 0.2 above it.
CLAMBER_BAND_TOP = PLAYER_HEIGHT
CLAMBER_FAIL = CLAMBER_BAND_TOP + 0.2

CATEGORY = "stealth_blockout"

FAMILY_COLORS = {
    # one distinct solid color per family for instant readability
    "cover": ("Stealth Cover", (0.16, 0.32, 0.65, 1.0)),
    "traversal": ("Stealth Traversal", (0.80, 0.42, 0.10, 1.0)),
    "structure": ("Stealth Structure", (0.52, 0.52, 0.55, 1.0)),
    "reference": ("Stealth Reference", (0.18, 0.60, 0.28, 1.0)),
}

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def make_material(family):
    name, rgba = FAMILY_COLORS[family]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = rgba
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.95
    mat.diffuse_color = rgba
    return mat


def add_box(name, size, center_xy=(0.0, 0.0), base_z=0.0, material=None):
    """Axis-aligned box: size=(x,y,z) meters, base resting at base_z."""
    sx, sy, sz = size
    bpy.ops.mesh.primitive_cube_add(size=1.0)
    obj = bpy.context.active_object
    obj.name = name
    obj.data.name = name + "_mesh"
    obj.scale = (sx, sy, sz)
    obj.location = (center_xy[0], center_xy[1], base_z + sz / 2.0)
    bpy.ops.object.transform_apply(scale=True)
    if material is not None:
        obj.data.materials.append(material)
    for poly in obj.data.polygons:
        poly.use_smooth = False
    return obj


def add_cylinder(name, radius, height, material=None, vertices=16):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=height)
    obj = bpy.context.active_object
    obj.name = name
    obj.data.name = name + "_mesh"
    obj.location = (0.0, 0.0, height / 2.0)
    if material is not None:
        obj.data.materials.append(material)
    for poly in obj.data.polygons:
        poly.use_smooth = False
    return obj


def tag_single(obj, walkable=None):
    obj["iggy_category"] = CATEGORY
    obj["iggy_collision"] = "bounds"
    if walkable is not None:
        obj["iggy_walkable"] = walkable


def tag_part(obj, walkable=None):
    """Compound-piece node. walkable=None -> visual-only node (no collision part)."""
    obj["iggy_category"] = CATEGORY
    obj["iggy_collision"] = "compound_bounds"
    if walkable is not None:
        obj["iggy_collision_part"] = "bounds"
        obj["iggy_collision_part_walkable"] = walkable


def export_glb(filename):
    path = os.path.join(OUT_DIR, filename + ".glb")
    bpy.ops.export_scene.gltf(
        filepath=path,
        export_format="GLB",
        export_extras=True,
        export_apply=True,
        export_yup=True,
    )
    print("WROTE", path)


def single_box_asset(filename, family, size, walkable=None):
    reset_scene()
    mat = make_material(family)
    add_box(filename, size, material=mat)
    obj = bpy.context.active_object
    tag_single(obj, walkable=walkable)
    export_glb(filename)


def stair_asset(filename, family, width, riser, tread, steps, name_prefix):
    """Stepped run: walkable compound treads, climbing toward -Y like cave ramp."""
    reset_scene()
    mat = make_material(family)
    run = tread * steps
    for i in range(steps):
        # solid stack: level i runs from the back edge to its own front edge,
        # so the stair climbs toward -Y (cave ramp convention)
        depth = run - i * tread
        obj = add_box(
            "%s_%02d" % (name_prefix, i + 1),
            (width, depth, riser),
            center_xy=(0.0, -i * tread / 2.0),
            base_z=i * riser,
            material=mat,
        )
        tag_part(obj, walkable=True)
    export_glb(filename)


def build_cover():
    single_box_asset("cover_low_wall_2x1p1", "cover", (2.0, 0.4, 1.1))
    single_box_asset("cover_high_wall_2x2", "cover", (2.0, 0.4, 2.0))
    single_box_asset("cover_crate_1m", "cover", (1.0, 1.0, 1.0))
    single_box_asset("cover_crate_0p5", "cover", (0.5, 0.5, 0.5))
    single_box_asset("cover_sandbag_run_2x0p9", "cover", (2.0, 0.6, 0.9))

    reset_scene()
    mat = make_material("cover")
    obj = add_cylinder("cover_barrel_0p6x1p1", 0.3, 1.1, material=mat)
    tag_single(obj)
    export_glb("cover_barrel_0p6x1p1")


def build_traversal():
    # clamber test ladder — tops walkable so a successful clamber has footing
    for height, tag in ((0.6, "0p6"), (1.0, "1p0"), (1.4, "1p4"),
                        (CLAMBER_BAND_TOP, "1p8")):
        single_box_asset("clamber_block_" + tag, "traversal",
                         (1.0, 1.0, height), walkable=True)
    # must refuse: 0.2 above the assumed band top
    single_box_asset("clamber_fail_2p0", "traversal", (1.0, 1.0, CLAMBER_FAIL),
                     walkable=True)

    single_box_asset("vault_rail_2x1p0", "traversal", (2.0, 0.15, 1.0))
    single_box_asset("platform_2x2x0p5", "traversal", (2.0, 2.0, 0.5),
                     walkable=True)

    # stair: riser = one snap step (0.25), under auto-step 0.35
    stair_asset("stair_run_2x3x1p5", "traversal", width=2.0,
                riser=SNAP_QUARTER, tread=0.5, steps=6,
                name_prefix="stair_run_step")
    # ramp 2x1: fine treads read as a slope; 26.6 deg < 40 deg walkable limit
    stair_asset("ramp_2x2x1", "traversal", width=2.0,
                riser=0.125, tread=0.25, steps=8, name_prefix="ramp_tread")

    # ledge shelf: overhanging slab at the assumed clamber band top
    reset_scene()
    mat = make_material("traversal")
    core = add_box("ledge_shelf_core", (1.0, 0.4, 1.6), material=mat)
    tag_part(core, walkable=False)
    slab = add_box("ledge_shelf_slab", (1.0, 0.8, 0.2),
                   center_xy=(0.0, -0.2), base_z=1.6, material=mat)
    tag_part(slab, walkable=True)
    export_glb("ledge_shelf_1x1p8")


def build_structure():
    single_box_asset("wall_seg_2x2", "structure", (2.0, 0.2, 2.0))
    single_box_asset("wall_seg_4x2", "structure", (4.0, 0.2, 2.0))
    single_box_asset("pillar_0p5x3", "structure", (0.5, 0.5, 3.0))

    # window: full-width slot 1.4-1.9 spans stand/guard eye band (1.6/1.7)
    # while the sneak eye (0.9) stays occluded
    reset_scene()
    mat = make_material("structure")
    lower = add_box("wall_window_lower", (2.0, 0.2, 1.4), material=mat)
    tag_part(lower, walkable=False)
    upper = add_box("wall_window_header", (2.0, 0.2, 0.1), base_z=1.9,
                    material=mat)
    tag_part(upper, walkable=False)
    export_glb("wall_window_2x2")

    # doorway: 1.0 x 2.0 opening (player 1.80 clears), 2.5 total height
    reset_scene()
    mat = make_material("structure")
    left = add_box("doorway_jamb_l", (0.5, 0.2, 2.0), center_xy=(-0.75, 0.0),
                   material=mat)
    tag_part(left, walkable=False)
    right = add_box("doorway_jamb_r", (0.5, 0.2, 2.0), center_xy=(0.75, 0.0),
                    material=mat)
    tag_part(right, walkable=False)
    lintel = add_box("doorway_lintel", (2.0, 0.2, 0.5), base_z=2.0,
                     material=mat)
    tag_part(lintel, walkable=False)
    export_glb("doorway_2x1")

    # L corner: two 2m wall legs, outer faces flush at the corner
    reset_scene()
    mat = make_material("structure")
    leg_a = add_box("corner_l_leg_a", (2.0, 0.2, 2.0),
                    center_xy=(0.0, -0.9), material=mat)
    tag_part(leg_a, walkable=False)
    leg_b = add_box("corner_l_leg_b", (0.2, 1.8, 2.0),
                    center_xy=(-0.9, 0.1), material=mat)
    tag_part(leg_b, walkable=False)
    export_glb("corner_l_2x2")


def build_reference():
    # guard post: guard HEIGHT is not a code constant; body uses player height
    # (1.80) with the verified guard eye line (1.6) marked by a notch fin.
    reset_scene()
    mat = make_material("reference")
    body = add_box("guard_post_body", (0.4, 0.4, PLAYER_HEIGHT), material=mat)
    fin = add_box("guard_post_eye_notch", (0.7, 0.7, 0.04),
                  base_z=GUARD_EYE - 0.02, material=mat)
    bpy.ops.object.select_all(action="SELECT")
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.join()
    body.name = "guard_post_1p8"
    tag_single(body)
    export_glb("guard_post_1p8")

    # player gauge: player height column, crouch/sneak-eye band plate at 0.9
    reset_scene()
    mat = make_material("reference")
    column = add_box("player_gauge_body", (0.3, 0.3, PLAYER_HEIGHT),
                     material=mat)
    band = add_box("player_gauge_crouch_band", (0.55, 0.55, 0.04),
                   base_z=SNEAK_EYE - 0.02, material=mat)
    bpy.ops.object.select_all(action="SELECT")
    bpy.context.view_layer.objects.active = column
    bpy.ops.object.join()
    column.name = "player_gauge_1p8"
    tag_single(column)
    export_glb("player_gauge_1p8")


def build_giant():
    # liminal 2x-scale pieces; keep traversal family color
    stair_asset("giant_stair_4x6x3", "traversal", width=4.0, riser=0.5,
                tread=1.0, steps=6, name_prefix="giant_stair_step")

    reset_scene()
    mat = make_material("traversal")
    core = add_box("giant_shelf_core", (2.0, 0.8, 3.2), material=mat)
    tag_part(core, walkable=False)
    slab = add_box("giant_shelf_slab", (2.0, 1.6, 0.4),
                   center_xy=(0.0, -0.4), base_z=3.2, material=mat)
    tag_part(slab, walkable=True)
    export_glb("giant_shelf_2x3p6")


def main():
    build_cover()
    build_traversal()
    build_structure()
    build_reference()
    build_giant()
    print("KIT GENERATION COMPLETE")


if __name__ == "__main__":
    main()
