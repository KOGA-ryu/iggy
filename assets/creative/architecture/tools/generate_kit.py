"""Architecture building-closure kit generator (ASSET-BLD-1, Batch 1).

Run headless only:
    blender --background --python generate_kit.py

Regenerates all 34 architecture GLBs under assets/creative/architecture/
(openings/, traversal/, structural/, roof/) with palette_v1 materials baked
in: every mesh gets deterministic world-space cube-projection UVs at the
tile's meters-per-tile scale and one embedded base-color texture from
assets/creative/materials/palette_v1/. This script is the single source for
the committed GLBs (no hand-saved .blend, no separate retexture pass).

Contract (see ../../calibration/README.md and the importer
src/content/assets/StaticMeshAuthoringMetadata.cpp):
  single box     -> {iggy_category, iggy_collision:"bounds", iggy_walkable?}
  compound       -> every node {iggy_category, iggy_collision:"compound_bounds"};
                    collidable nodes add {iggy_collision_part:"bounds",
                    iggy_collision_part_walkable}
  socket empty   -> unrotated/yaw-only {iggy_socket, iggy_socket_role,
                    iggy_socket_compatibility}
  mesh nodes     -> identity TRS; all pose/pitch baked into vertices

State families (Production Law 3): every closed/open pair shares one hinge
pivot (object origin on the hinge line) and one identical plug socket at that
origin. Shutter pairs mirror the door law; left and right shutters are
separate compatibility families because their pivots are mirrored.

Reference pack: ~/references/building_closure_v1/ sheets 02/04/05 own the
construction language (stone sills, timber lintels and bargeboards,
bracket-supported eave boards, U-channel gutters, corbel-topped chimneys,
capped newel posts). Numbers below are the authored contract.
"""

import math
import os

import bpy

# Roof caps match the generated roof recipe default pitch
# (kDefaultCreativeStructuralRoofPitchDegrees in
# src/app/iggy3d/creative/recipes/StructuralRoofRecipe.hpp).
ROOF_PITCH_DEG = 30.0
RIDGE_HALF = 0.20
RIDGE_APEX = RIDGE_HALF * math.tan(math.radians(ROOF_PITCH_DEG))

STOREY = 3.0
DOOR_CLEAR_W = 0.9
DOOR_CLEAR_H = 2.1
WIDE_CLEAR_W = 1.2
STAIR_RISE = STOREY
STAIR_STEPS = 16
STAIR_RISER = STAIR_RISE / STAIR_STEPS   # 0.1875
STAIR_TREAD = 0.25
STAIR_WIDTH = 1.0
STAIR_RUN = STAIR_TREAD * STAIR_STEPS    # 4.0
STAIR_PITCH = STAIR_RISE / STAIR_RUN     # 0.75 rise per meter of run

WIN_STD_W = 0.8      # standard window clear opening
WIN_STD_H = 1.2
WIN_SMALL_W = 0.5    # small service opening
WIN_SMALL_H = 0.6
WIN_MEMBER = 0.08    # frame member width
WIN_DEPTH = 0.15     # frame depth (matches door frames)

RAIL_TOP = 0.85      # rail band bottom offsets above the nosing line
RAIL_TOP_H = 0.10
RAIL_MID = 0.42
RAIL_MID_H = 0.08
RAIL_SHOE_H = 0.06

DOOR_FAMILY = "architecture.door_standard"
DOOR_WIDE_FAMILY = "architecture.door_wide"
SHUTTER_LEFT_FAMILY = "architecture.shutter_left"
SHUTTER_RIGHT_FAMILY = "architecture.shutter_right"
MULLION_FAMILY = "architecture.window_mullion"
CHIMNEY_CAP_FAMILY = "architecture.chimney_cap"

OUT_DIR = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))
PALETTE_DIR = os.path.abspath(
    os.path.join(OUT_DIR, os.pardir, "materials", "palette_v1"))

# tile name -> meters per tile (palette_v1 README table)
TILE_SCALE = {
    "oak_plank": 1.1,
    "oak_timber": 1.2,
    "lime_plaster": 2.5,
    "stone_rough": 1.6,
    "shingle_oak": 2.0,
    "iron_forged": 1.0,
}


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def tile_material(tile):
    """Palette-tile material; name palette_<tile> is the linter's
    traceability contract."""
    name = "palette_" + tile
    if name in bpy.data.materials:
        return bpy.data.materials[name]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.85
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(
        os.path.join(PALETTE_DIR, tile + ".png"))
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def make_box(name, x0, x1, y0, y1, z0, z1, material=None):
    verts = [
        (x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
        (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1),
    ]
    faces = [
        (0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
        (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0),
    ]
    return _mesh_obj(name, verts, faces, material)


def _mesh_obj(name, verts, faces, material):
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    for poly in mesh.polygons:
        poly.use_smooth = False
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    if material is not None:
        obj.data.materials.append(material)
    return obj


def centered_box(name, size, center_xy=(0.0, 0.0), base_z=0.0, material=None):
    sx, sy, sz = size
    cx, cy = center_xy
    return make_box(name, cx - sx / 2.0, cx + sx / 2.0, cy - sy / 2.0,
                    cy + sy / 2.0, base_z, base_z + sz, material=material)


def make_prism(name, verts, faces, material=None):
    return _mesh_obj(name, verts, faces, material)


def sloped_bar_y(name, x0, x1, y0, y1, zb0, zb1, height, material=None):
    """Bar of constant vertical thickness whose bottom follows a straight
    slope in Y: bottom z is zb0 at y0 and zb1 at y1. Box topology."""
    verts = [
        (x0, y0, zb0), (x1, y0, zb0), (x1, y1, zb1), (x0, y1, zb1),
        (x0, y0, zb0 + height), (x1, y0, zb0 + height),
        (x1, y1, zb1 + height), (x0, y1, zb1 + height),
    ]
    faces = [
        (0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
        (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0),
    ]
    return _mesh_obj(name, verts, faces, material)


def sloped_bar_x(name, x0, x1, y0, y1, zb0, zb1, height, material=None):
    """Same as sloped_bar_y but the slope runs along X (zb0 at x0)."""
    verts = [
        (x0, y0, zb0), (x1, y0, zb1), (x1, y1, zb1), (x0, y1, zb0),
        (x0, y0, zb0 + height), (x1, y0, zb1 + height),
        (x1, y1, zb1 + height), (x0, y1, zb0 + height),
    ]
    faces = [
        (0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
        (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0),
    ]
    return _mesh_obj(name, verts, faces, material)


def join_into(objects, final_name):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    result = bpy.context.view_layer.objects.active
    result.name = final_name
    return result


def tag_bounds(obj, category, walkable=None):
    obj["iggy_category"] = category
    obj["iggy_collision"] = "bounds"
    if walkable is not None:
        obj["iggy_walkable"] = walkable


def tag_none(obj, category):
    obj["iggy_category"] = category
    obj["iggy_collision"] = "none"


def tag_part(obj, category, walkable):
    obj["iggy_category"] = category
    obj["iggy_collision"] = "compound_bounds"
    obj["iggy_collision_part"] = "bounds"
    obj["iggy_collision_part_walkable"] = walkable


def add_socket(name, location, role, compatibility):
    empty = bpy.data.objects.new(name, None)
    bpy.context.collection.objects.link(empty)
    empty.empty_display_type = "ARROWS"
    empty.empty_display_size = 0.2
    empty.location = location
    empty["iggy_socket"] = name
    empty["iggy_socket_role"] = role
    empty["iggy_socket_compatibility"] = compatibility
    return empty


def apply_mesh_transforms():
    """Bake every mesh object's TRS into its vertices so exported mesh nodes
    have identity transforms. Socket empties are left untouched — their world
    transform IS the socket contract."""
    bpy.ops.object.select_all(action="DESELECT")
    active = None
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            obj.select_set(True)
            active = obj
    if active is not None:
        bpy.context.view_layer.objects.active = active
        bpy.ops.object.transform_apply(location=True, rotation=True,
                                       scale=True)


def assign_cube_uvs(meters_per_tile):
    """Deterministic world-space cube projection: each face maps its two
    non-dominant world axes to UV at 1 tile per meters_per_tile meters.
    Runs after apply_mesh_transforms so vertex coords are world coords."""
    inv = 1.0 / meters_per_tile
    for obj in bpy.context.scene.objects:
        if obj.type != "MESH":
            continue
        mesh = obj.data
        if not mesh.uv_layers:
            mesh.uv_layers.new(name="UVMap")
        uv = mesh.uv_layers.active.data
        for poly in mesh.polygons:
            n = poly.normal
            ax, ay, az = abs(n.x), abs(n.y), abs(n.z)
            if az >= ax and az >= ay:
                pick = (0, 1)
            elif ax >= ay:
                pick = (1, 2)
            else:
                pick = (0, 2)
            for li in poly.loop_indices:
                co = mesh.vertices[mesh.loops[li].vertex_index].co
                uv[li].uv = (co[pick[0]] * inv, co[pick[1]] * inv)


def export_glb(rel_path, tile):
    apply_mesh_transforms()
    assign_cube_uvs(TILE_SCALE[tile])
    path = os.path.join(OUT_DIR, rel_path + ".glb")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=path, export_format="GLB", export_extras=True,
        export_apply=True, export_yup=True)
    print("WROTE", path)


# ---------------------------------------------------------------------------
# Openings: standard door family (CAL-1 geometry, unchanged bounds)
# ---------------------------------------------------------------------------
def _door_frame(rel_path, clear_w, family):
    """Clear opening clear_w x 2.1; compound jambs+lintel; hinge receiver at
    the left jamb inner face, floor level."""
    reset_scene()
    mat = tile_material("oak_timber")
    half = clear_w / 2.0
    depth = 0.15
    left = make_box("jamb_l", -half - 0.05, -half, -depth / 2.0, depth / 2.0,
                    0.0, DOOR_CLEAR_H, material=mat)
    tag_part(left, "openings", walkable=False)
    right = make_box("jamb_r", half, half + 0.05, -depth / 2.0, depth / 2.0,
                     0.0, DOOR_CLEAR_H, material=mat)
    tag_part(right, "openings", walkable=False)
    lintel = make_box("lintel", -half - 0.05, half + 0.05, -depth / 2.0,
                      depth / 2.0, DOOR_CLEAR_H, DOOR_CLEAR_H + 0.15,
                      material=mat)
    tag_part(lintel, "openings", walkable=False)
    add_socket("hinge.left", (-half, 0.0, 0.0), "receiver", family)
    export_glb(rel_path, "oak_timber")


def _door_leaf(rel_path, name, width, family, closed):
    """Hinge law: object origin on the hinge line; closed fills +X, open is
    the same panel statically swung 90 deg into -Y. Identical plug socket at
    the origin for both states."""
    reset_scene()
    mat = tile_material("oak_plank")
    if closed:
        panel = make_box(name, 0.0, width, -0.025, 0.025, 0.0, DOOR_CLEAR_H,
                         material=mat)
    else:
        panel = make_box(name, -0.025, 0.025, -width, 0.0, 0.0, DOOR_CLEAR_H,
                         material=mat)
    tag_bounds(panel, "openings")
    add_socket("hinge", (0.0, 0.0, 0.0), "plug", family)
    export_glb(rel_path, "oak_plank")


def build_door_frame_standard():
    _door_frame("openings/door_frame_standard", DOOR_CLEAR_W, DOOR_FAMILY)


def build_door_leaf_standard_closed():
    _door_leaf("openings/door_leaf_standard_closed",
               "door_leaf_standard_closed", DOOR_CLEAR_W, DOOR_FAMILY, True)


def build_door_leaf_standard_open():
    _door_leaf("openings/door_leaf_standard_open",
               "door_leaf_standard_open", DOOR_CLEAR_W, DOOR_FAMILY, False)


def build_door_frame_wide():
    _door_frame("openings/door_frame_wide", WIDE_CLEAR_W, DOOR_WIDE_FAMILY)


def build_door_leaf_wide_closed():
    _door_leaf("openings/door_leaf_wide_closed", "door_leaf_wide_closed",
               WIDE_CLEAR_W, DOOR_WIDE_FAMILY, True)


def build_door_leaf_wide_open():
    _door_leaf("openings/door_leaf_wide_open", "door_leaf_wide_open",
               WIDE_CLEAR_W, DOOR_WIDE_FAMILY, False)


# ---------------------------------------------------------------------------
# Openings: window family (sheet 04 construction language)
# ---------------------------------------------------------------------------
def _window_frame(rel_path, clear_w, clear_h, with_sockets):
    """Four-member timber frame around a clear_w x clear_h opening. Local
    origin: X centered on the opening, Z=0 at the BOTTOM of the clear opening
    (the stool member spans below it), so placement recipes set sill height
    directly. Not floor-grounded."""
    reset_scene()
    mat = tile_material("oak_timber")
    half = clear_w / 2.0
    d = WIN_DEPTH / 2.0
    m = WIN_MEMBER
    left = make_box("jamb_l", -half - m, -half, -d, d, -m, clear_h + m,
                    material=mat)
    tag_part(left, "openings", walkable=False)
    right = make_box("jamb_r", half, half + m, -d, d, -m, clear_h + m,
                     material=mat)
    tag_part(right, "openings", walkable=False)
    head = make_box("head", -half, half, -d, d, clear_h, clear_h + m,
                    material=mat)
    tag_part(head, "openings", walkable=False)
    stool = make_box("stool", -half, half, -d, d, -m, 0.0, material=mat)
    tag_part(stool, "openings", walkable=False)
    if with_sockets:
        # shutter hinges on the exterior face (-Y), at the clear-opening edges
        add_socket("shutter.left", (-half, -d, 0.0), "receiver",
                   SHUTTER_LEFT_FAMILY)
        add_socket("shutter.right", (half, -d, 0.0), "receiver",
                   SHUTTER_RIGHT_FAMILY)
        add_socket("mullion", (0.0, 0.0, 0.0), "receiver", MULLION_FAMILY)
    export_glb(rel_path, "oak_timber")


def build_window_frame_standard():
    _window_frame("openings/window_frame_standard", WIN_STD_W, WIN_STD_H,
                  with_sockets=True)


def build_window_frame_small():
    _window_frame("openings/window_frame_small", WIN_SMALL_W, WIN_SMALL_H,
                  with_sockets=False)


def _shutter(rel_path, name, family, hand, closed):
    """Shutter hinge law mirrors the door law. Left hand: origin on the left
    hinge line, closed panel fills +X, open swings 90 deg to -Y (exterior).
    Right hand: origin on the right hinge line, closed panel fills -X, open
    swings to -Y with the opposite rotation sense."""
    reset_scene()
    mat = tile_material("oak_plank")
    w = WIN_STD_W / 2.0   # each shutter covers half the standard opening
    h = WIN_STD_H
    t = 0.02
    if closed:
        x0, x1 = (0.0, w) if hand == "left" else (-w, 0.0)
        panel = make_box(name, x0, x1, -t, t, 0.0, h, material=mat)
    else:
        panel = make_box(name, -t, t, -w, 0.0, 0.0, h, material=mat)
    tag_bounds(panel, "openings")
    family_name = (SHUTTER_LEFT_FAMILY if hand == "left"
                   else SHUTTER_RIGHT_FAMILY)
    add_socket("hinge", (0.0, 0.0, 0.0), "plug", family_name)
    export_glb(rel_path, "oak_plank")


def build_window_shutter_left_closed():
    _shutter("openings/window_shutter_left_closed",
             "window_shutter_left_closed", SHUTTER_LEFT_FAMILY, "left", True)


def build_window_shutter_left_open():
    _shutter("openings/window_shutter_left_open",
             "window_shutter_left_open", SHUTTER_LEFT_FAMILY, "left", False)


def build_window_shutter_right_closed():
    _shutter("openings/window_shutter_right_closed",
             "window_shutter_right_closed", SHUTTER_RIGHT_FAMILY, "right",
             True)


def build_window_shutter_right_open():
    _shutter("openings/window_shutter_right_open",
             "window_shutter_right_open", SHUTTER_RIGHT_FAMILY, "right",
             False)


def build_window_mullion_cross():
    """Socketable cross insert for the standard opening. Origin at the
    opening's bottom center; plug mates the frame's mullion receiver."""
    reset_scene()
    mat = tile_material("oak_timber")
    t = 0.025
    vert = make_box("mullion_v", -0.03, 0.03, -t, t, 0.0, WIN_STD_H,
                    material=mat)
    horiz = make_box("mullion_h", -WIN_STD_W / 2.0, WIN_STD_W / 2.0, -t, t,
                     WIN_STD_H / 2.0 - 0.03, WIN_STD_H / 2.0 + 0.03,
                     material=mat)
    cross = join_into([vert, horiz], "window_mullion_cross")
    tag_none(cross, "openings")
    add_socket("mullion", (0.0, 0.0, 0.0), "plug", MULLION_FAMILY)
    export_glb("openings/window_mullion_cross", "oak_timber")


def build_window_sill_standard():
    """Rough stone sill block, wider than the standard frame (sheet 04)."""
    reset_scene()
    mat = tile_material("stone_rough")
    sill = make_box("window_sill_standard", -0.55, 0.55, -0.125, 0.125,
                    0.0, 0.10, material=mat)
    tag_none(sill, "openings")
    export_glb("openings/window_sill_standard", "stone_rough")


def build_window_lintel_standard():
    """Heavy timber lintel board over the standard opening (sheet 04)."""
    reset_scene()
    mat = tile_material("oak_timber")
    lintel = make_box("window_lintel_standard", -0.6, 0.6, -0.10, 0.10,
                      0.0, 0.12, material=mat)
    tag_none(lintel, "openings")
    export_glb("openings/window_lintel_standard", "oak_timber")


# ---------------------------------------------------------------------------
# Traversal (CAL-1 stair geometry, plus the rail completion family)
# ---------------------------------------------------------------------------
def _nosing_z(y):
    """Nosing-line height of the standard stair at depth y (ascends -Y;
    bottom step nose at y=+2, top floor z=3.0 at y=-2)."""
    return (STAIR_RUN / 2.0 - y) * STAIR_PITCH


def _stair_steps(mat, category="traversal"):
    steps = []
    for i in range(STAIR_STEPS):
        depth = STAIR_RUN - i * STAIR_TREAD
        step = centered_box("step_%02d" % (i + 1),
                            (STAIR_WIDTH, depth, STAIR_RISER),
                            center_xy=(0.0, -i * STAIR_TREAD / 2.0),
                            base_z=i * STAIR_RISER, material=mat)
        tag_part(step, category, walkable=True)
        steps.append(step)
    return steps


def build_stair_straight_3m():
    reset_scene()
    mat = tile_material("oak_timber")
    _stair_steps(mat)
    export_glb("traversal/stair_straight_3m", "oak_timber")


def build_stair_landing_2x2m():
    reset_scene()
    mat = tile_material("oak_plank")
    slab = centered_box("stair_landing_2x2m", (2.0, 2.0, 0.2), base_z=0.0,
                        material=mat)
    tag_bounds(slab, "traversal", walkable=True)
    export_glb("traversal/stair_landing_2x2m", "oak_plank")


def build_stair_straight_3m_with_rails():
    """Full assembly: the standard stair plus side rails and newels. Rail and
    post collision parts sit OUTSIDE the 1.0 m walkable tread width (|x| >
    0.5) so they never block the walkable path (section E law)."""
    reset_scene()
    mat = tile_material("oak_timber")
    _stair_steps(mat)
    y0, y1 = 1.95, -1.95
    for side in (1.0, -1.0):
        cx = side * 0.545  # bars span |x| 0.505..0.585, strictly clear of 0.5
        top = sloped_bar_y("rail_top_%+.0f" % side, cx - 0.04, cx + 0.04,
                           y0, y1, _nosing_z(y0) + RAIL_TOP,
                           _nosing_z(y1) + RAIL_TOP, RAIL_TOP_H, material=mat)
        tag_part(top, "traversal", walkable=False)
        mid = sloped_bar_y("rail_mid_%+.0f" % side, cx - 0.04, cx + 0.04,
                           y0, y1, _nosing_z(y0) + RAIL_MID,
                           _nosing_z(y1) + RAIL_MID, RAIL_MID_H, material=mat)
        tag_part(mid, "traversal", walkable=False)
        low = make_box("newel_low_%+.0f" % side, cx - 0.04, cx + 0.04,
                       y0 - 0.04, y0 + 0.04, 0.0, 1.05, material=mat)
        tag_part(low, "traversal", walkable=False)
        high = make_box("newel_high_%+.0f" % side, cx - 0.04, cx + 0.04,
                        y1 - 0.04, y1 + 0.04, STAIR_RISE, STAIR_RISE + 1.05,
                        material=mat)
        tag_part(high, "traversal", walkable=False)
    export_glb("traversal/stair_straight_3m_with_rails", "oak_timber")


def build_stair_rail_slope_3m():
    """Standalone rail module at the standard stair pitch: sloped shoe, mid
    and top bands plus four balusters, joined into one thin-X mesh."""
    reset_scene()
    mat = tile_material("oak_timber")
    y0, y1 = STAIR_RUN / 2.0, -STAIR_RUN / 2.0
    parts = [
        sloped_bar_y("shoe", -0.04, 0.04, y0, y1, _nosing_z(y0),
                     _nosing_z(y1), RAIL_SHOE_H, material=mat),
        sloped_bar_y("mid", -0.04, 0.04, y0, y1, _nosing_z(y0) + RAIL_MID,
                     _nosing_z(y1) + RAIL_MID, RAIL_MID_H, material=mat),
        sloped_bar_y("top", -0.04, 0.04, y0, y1, _nosing_z(y0) + RAIL_TOP,
                     _nosing_z(y1) + RAIL_TOP, RAIL_TOP_H, material=mat),
    ]
    for by in (-1.5, -0.5, 0.5, 1.5):
        parts.append(make_box("baluster_%+.1f" % by, -0.03, 0.03,
                              by - 0.03, by + 0.03, _nosing_z(by),
                              _nosing_z(by) + RAIL_TOP + 0.05, material=mat))
    rail = join_into(parts, "stair_rail_slope_3m")
    tag_bounds(rail, "traversal")
    export_glb("traversal/stair_rail_slope_3m", "oak_timber")


def build_stair_newel_post():
    """Capped newel post (sheet 05): rail endpoint/junction."""
    reset_scene()
    mat = tile_material("oak_timber")
    shaft = make_box("shaft", -0.06, 0.06, -0.06, 0.06, 0.0, 1.05,
                     material=mat)
    cap = make_box("cap", -0.08, 0.08, -0.08, 0.08, 1.05, 1.12, material=mat)
    post = join_into([shaft, cap], "stair_newel_post")
    tag_bounds(post, "traversal")
    export_glb("traversal/stair_newel_post", "oak_timber")


# ---------------------------------------------------------------------------
# Structural
# ---------------------------------------------------------------------------
def build_railing_straight_2m():
    reset_scene()
    mat = tile_material("oak_timber")
    parts = [
        make_box("post_l", -1.0, -0.92, -0.04, 0.04, 0.0, 1.0, material=mat),
        make_box("post_r", 0.92, 1.0, -0.04, 0.04, 0.0, 1.0, material=mat),
        make_box("rail_top", -1.0, 1.0, -0.03, 0.03, 0.92, 0.98,
                 material=mat),
        make_box("rail_mid", -1.0, 1.0, -0.03, 0.03, 0.48, 0.54,
                 material=mat),
    ]
    rail = join_into(parts, "railing_straight_2m")
    tag_bounds(rail, "structural")
    export_glb("structural/railing_straight_2m", "oak_timber")


# ---------------------------------------------------------------------------
# Roof: ridge family (CAL-1 section), eave/gable trim, drainage, chimney
# ---------------------------------------------------------------------------
def _ridge_prism(name, length, base_half, apex_z, out_half, material):
    x0, x1 = -length / 2.0, length / 2.0
    verts = [
        (x0, -base_half, 0.0), (x0, base_half, 0.0), (x0, 0.0, apex_z),
        (x1, -out_half, 0.0), (x1, out_half, 0.0), (x1, 0.0, apex_z),
    ]
    faces = [
        (0, 1, 2), (5, 4, 3), (0, 3, 4, 1), (1, 4, 5, 2), (2, 5, 3, 0),
    ]
    return make_prism(name, verts, faces, material=material)


def _ridge_cap(rel_path, name, length, out_half):
    reset_scene()
    mat = tile_material("shingle_oak")
    cap = _ridge_prism(name, length, RIDGE_HALF, RIDGE_APEX, out_half, mat)
    tag_bounds(cap, "roof")
    export_glb(rel_path, "shingle_oak")


def build_ridge_cap_straight_4m():
    _ridge_cap("roof/ridge_cap_straight_4m", "ridge_cap_straight_4m", 4.0,
               RIDGE_HALF)


def build_ridge_cap_straight_2m():
    _ridge_cap("roof/ridge_cap_straight_2m", "ridge_cap_straight_2m", 2.0,
               RIDGE_HALF)


def build_ridge_cap_end():
    _ridge_cap("roof/ridge_cap_end", "ridge_cap_end", 0.4, 0.05)


def _eave_trim(rel_path, name, length, bracket_xs):
    """Bracket-supported eave board (sheet 02): fascia board with support
    bracket blocks reaching back toward the wall (+Y)."""
    reset_scene()
    mat = tile_material("oak_timber")
    parts = [make_box("board", -length / 2.0, length / 2.0, -0.03, 0.03,
                      0.12, 0.30, material=mat)]
    for bx in bracket_xs:
        parts.append(make_box("bracket_%+.1f" % bx, bx - 0.03, bx + 0.03,
                              0.03, 0.17, 0.0, 0.16, material=mat))
    trim = join_into(parts, name)
    tag_none(trim, "roof")
    export_glb(rel_path, "oak_timber")


def build_eave_trim_2m():
    _eave_trim("roof/eave_trim_2m", "eave_trim_2m", 2.0, (-0.7, 0.7))


def build_eave_trim_4m():
    _eave_trim("roof/eave_trim_4m", "eave_trim_4m", 4.0, (-1.7, 0.0, 1.7))


def build_eave_outer_corner():
    """Convex eave junction: two board legs meeting at the origin corner."""
    reset_scene()
    mat = tile_material("oak_timber")
    parts = [
        make_box("leg_x", -0.03, 0.6, -0.03, 0.03, 0.12, 0.30, material=mat),
        make_box("leg_y", -0.03, 0.03, 0.03, 0.6, 0.12, 0.30, material=mat),
        make_box("bracket", 0.03, 0.17, 0.03, 0.17, 0.0, 0.16, material=mat),
    ]
    corner = join_into(parts, "eave_outer_corner")
    tag_none(corner, "roof")
    export_glb("roof/eave_outer_corner", "oak_timber")


def build_fascia_end():
    """Finished exposed fascia end: board stub with a vertical end return."""
    reset_scene()
    mat = tile_material("oak_timber")
    parts = [
        make_box("board", 0.0, 0.30, -0.03, 0.03, 0.12, 0.30, material=mat),
        make_box("end_plate", -0.05, 0.0, -0.05, 0.05, 0.0, 0.30,
                 material=mat),
    ]
    end = join_into(parts, "fascia_end")
    tag_none(end, "roof")
    export_glb("roof/fascia_end", "oak_timber")


def build_gable_cap_4m():
    """Bargeboard pair over a 4 m gable span at the 30 deg recipe pitch,
    pitch baked into vertices (identity-TRS law). Eave ends rest at Z=0."""
    reset_scene()
    mat = tile_material("oak_timber")
    half_span = 2.0
    rise = half_span * math.tan(math.radians(ROOF_PITCH_DEG))
    board_h = 0.18
    right = sloped_bar_x("barge_r", 0.0, half_span, -0.03, 0.03, rise, 0.0,
                         board_h, material=mat)
    left = sloped_bar_x("barge_l", -half_span, 0.0, -0.03, 0.03, 0.0, rise,
                        board_h, material=mat)
    cap = join_into([right, left], "gable_cap_4m")
    tag_bounds(cap, "roof")
    export_glb("roof/gable_cap_4m", "oak_timber")


def build_gutter_straight_2m():
    """Opaque static U-channel gutter (sheet 02, wood fabrication)."""
    reset_scene()
    mat = tile_material("oak_plank")
    parts = [
        make_box("bottom", -1.0, 1.0, -0.06, 0.06, 0.0, 0.03, material=mat),
        make_box("wall_out", -1.0, 1.0, -0.09, -0.06, 0.0, 0.14,
                 material=mat),
        make_box("wall_in", -1.0, 1.0, 0.06, 0.09, 0.0, 0.14, material=mat),
    ]
    gutter = join_into(parts, "gutter_straight_2m")
    tag_none(gutter, "roof")
    export_glb("roof/gutter_straight_2m", "oak_plank")


def build_downspout_3m():
    """One-storey downspout, simplified per the reference README ruling."""
    reset_scene()
    mat = tile_material("iron_forged")
    spout = make_box("downspout_3m", -0.05, 0.05, -0.05, 0.05, 0.0, 3.0,
                     material=mat)
    tag_none(spout, "roof")
    export_glb("roof/downspout_3m", "iron_forged")


def build_downspout_outlet():
    """Ground outlet: short riser over an elbow mouth kicking out -Y."""
    reset_scene()
    mat = tile_material("iron_forged")
    parts = [
        make_box("elbow", -0.05, 0.05, -0.30, 0.05, 0.0, 0.16, material=mat),
        make_box("riser", -0.05, 0.05, -0.05, 0.05, 0.16, 0.50,
                 material=mat),
    ]
    outlet = join_into(parts, "downspout_outlet")
    tag_none(outlet, "roof")
    export_glb("roof/downspout_outlet", "iron_forged")


def build_chimney_stack_short():
    """One-roof-plane stone stack with a corbel band; cap receiver on top."""
    reset_scene()
    mat = tile_material("stone_rough")
    parts = [
        make_box("stack", -0.4, 0.4, -0.4, 0.4, 0.0, 1.9, material=mat),
        make_box("corbel", -0.475, 0.475, -0.475, 0.475, 1.9, 2.2,
                 material=mat),
    ]
    stack = join_into(parts, "chimney_stack_short")
    tag_bounds(stack, "roof")
    add_socket("cap", (0.0, 0.0, 2.2), "receiver", CHIMNEY_CAP_FAMILY)
    export_glb("roof/chimney_stack_short", "stone_rough")


def build_chimney_cap():
    """Socketable stone cap: slab on two side walls (sheet 02 language)."""
    reset_scene()
    mat = tile_material("stone_rough")
    parts = [
        make_box("wall_l", -0.36, -0.24, -0.36, 0.36, 0.0, 0.25,
                 material=mat),
        make_box("wall_r", 0.24, 0.36, -0.36, 0.36, 0.0, 0.25, material=mat),
        make_box("slab", -0.45, 0.45, -0.45, 0.45, 0.25, 0.33, material=mat),
    ]
    cap = join_into(parts, "chimney_cap")
    tag_none(cap, "roof")
    add_socket("cap", (0.0, 0.0, 0.0), "plug", CHIMNEY_CAP_FAMILY)
    export_glb("roof/chimney_cap", "stone_rough")


def main():
    builders = [
        build_door_frame_standard,
        build_door_leaf_standard_closed,
        build_door_leaf_standard_open,
        build_door_frame_wide,
        build_door_leaf_wide_closed,
        build_door_leaf_wide_open,
        build_window_frame_standard,
        build_window_frame_small,
        build_window_shutter_left_closed,
        build_window_shutter_left_open,
        build_window_shutter_right_closed,
        build_window_shutter_right_open,
        build_window_mullion_cross,
        build_window_sill_standard,
        build_window_lintel_standard,
        build_stair_straight_3m,
        build_stair_landing_2x2m,
        build_stair_straight_3m_with_rails,
        build_stair_rail_slope_3m,
        build_stair_newel_post,
        build_railing_straight_2m,
        build_ridge_cap_straight_4m,
        build_ridge_cap_straight_2m,
        build_ridge_cap_end,
        build_eave_trim_2m,
        build_eave_trim_4m,
        build_eave_outer_corner,
        build_fascia_end,
        build_gable_cap_4m,
        build_gutter_straight_2m,
        build_downspout_3m,
        build_downspout_outlet,
        build_chimney_stack_short,
        build_chimney_cap,
    ]
    for builder in builders:
        builder()
    print("ARCHITECTURE KIT GENERATION COMPLETE: %d assets" % len(builders))


if __name__ == "__main__":
    main()
