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
DOUBLE_CLEAR_W = 1.8
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
DOOR_DOUBLE_LEFT_FAMILY = "architecture.door_double_left"
DOOR_DOUBLE_RIGHT_FAMILY = "architecture.door_double_right"
SHUTTER_LEFT_FAMILY = "architecture.shutter_left"
SHUTTER_RIGHT_FAMILY = "architecture.shutter_right"
MULLION_FAMILY = "architecture.window_mullion"
CHIMNEY_CAP_FAMILY = "architecture.chimney_cap"
COLUMN_FAMILY = "architecture.column_round"
COLUMN_CAP_FAMILY = "architecture.column_cap"

# structural detail constants (backlog section C; sheet 06 language)
PLINTH_D = 0.125       # base-course half depth
PLINTH_LEDGE = 0.15    # cap-ledge half depth
PLINTH_BODY_H = 0.45
PLINTH_H = 0.5
COLUMN_R = 0.25
COLUMN_SEGS = 16
BEAM_HALF_W = 0.1      # 2m/4m beam section 0.2 x 0.3
BEAM_H = 0.3
PARAPET_D = 0.11       # parapet body half thickness
PARAPET_COPING = 0.14  # coping half thickness
PARAPET_BODY_H = 0.78
PARAPET_H = 0.88
RAMP_RISER = 0.1875    # stepped-ramp riser (stealth ramp precedent; < 0.35)

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


def make_ngon_prism(name, radius, z0, z1, segments, material=None,
                    cx=0.0, cy=0.0):
    """Vertical n-gon prism with cap fans; a vertex lands on +X and +Y so the
    AABB equals +/-radius on both axes. Baked verts, no operators."""
    import math as _m
    n = segments
    verts = []
    for zz in (z0, z1):
        for k in range(n):
            a = 2.0 * _m.pi * k / n
            verts.append((cx + radius * _m.cos(a), cy + radius * _m.sin(a),
                          zz))
    faces = []
    for k in range(n):
        k2 = (k + 1) % n
        faces.append((k, k2, n + k2, n + k))
    faces.append(tuple(reversed(range(n))))          # bottom cap (down)
    faces.append(tuple(range(n, 2 * n)))             # top cap (up)
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


def build_door_frame_double():
    """1.8 m clear opening, TWO hinge receivers (sheet 03 double pair). Left
    and right leaves are separate compatibility families because their hinge
    pivots are mirrored."""
    reset_scene()
    mat = tile_material("oak_timber")
    half = DOUBLE_CLEAR_W / 2.0
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
    add_socket("hinge.left", (-half, 0.0, 0.0), "receiver",
               DOOR_DOUBLE_LEFT_FAMILY)
    add_socket("hinge.right", (half, 0.0, 0.0), "receiver",
               DOOR_DOUBLE_RIGHT_FAMILY)
    export_glb("openings/door_frame_double", "oak_timber")


def _hand_leaf(rel_path, name, width, thickness, height, family, hand,
               closed):
    """Mirrored-hand leaf, same law as shutters: origin on the hinge line;
    left closed fills +X, right closed fills -X, both open swing 90 deg into
    -Y (exterior) with opposite rotation senses."""
    reset_scene()
    mat = tile_material("oak_plank")
    t = thickness
    if closed:
        x0, x1 = (0.0, width) if hand == "left" else (-width, 0.0)
        panel = make_box(name, x0, x1, -t, t, 0.0, height, material=mat)
    else:
        panel = make_box(name, -t, t, -width, 0.0, 0.0, height, material=mat)
    tag_bounds(panel, "openings")
    add_socket("hinge", (0.0, 0.0, 0.0), "plug", family)
    export_glb(rel_path, "oak_plank")


def build_door_leaf_double_left_closed():
    _hand_leaf("openings/door_leaf_double_left_closed",
               "door_leaf_double_left_closed", DOUBLE_CLEAR_W / 2.0, 0.025,
               DOOR_CLEAR_H, DOOR_DOUBLE_LEFT_FAMILY, "left", True)


def build_door_leaf_double_left_open():
    _hand_leaf("openings/door_leaf_double_left_open",
               "door_leaf_double_left_open", DOUBLE_CLEAR_W / 2.0, 0.025,
               DOOR_CLEAR_H, DOOR_DOUBLE_LEFT_FAMILY, "left", False)


def build_door_leaf_double_right_closed():
    _hand_leaf("openings/door_leaf_double_right_closed",
               "door_leaf_double_right_closed", DOUBLE_CLEAR_W / 2.0, 0.025,
               DOOR_CLEAR_H, DOOR_DOUBLE_RIGHT_FAMILY, "right", True)


def build_door_leaf_double_right_open():
    _hand_leaf("openings/door_leaf_double_right_open",
               "door_leaf_double_right_open", DOUBLE_CLEAR_W / 2.0, 0.025,
               DOOR_CLEAR_H, DOOR_DOUBLE_RIGHT_FAMILY, "right", False)


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


def build_window_frame_wide():
    # no sockets: shutters and the mullion cross are sized to the standard
    # opening; wide/tall inserts are future family members
    _window_frame("openings/window_frame_wide", 1.4, 1.2, with_sockets=False)


def build_window_frame_tall():
    _window_frame("openings/window_frame_tall", 0.8, 1.8, with_sockets=False)


def build_window_bars_standard():
    """Solid opaque bars for the standard opening (blocker). Mounts in the
    standard frame's mullion receiver — the receiver accepts either insert
    of the mullion family, never both."""
    reset_scene()
    mat = tile_material("iron_forged")
    parts = []
    for bx in (-0.2, 0.0, 0.2):
        parts.append(make_box("bar_%+.1f" % bx, bx - 0.02, bx + 0.02,
                              -0.02, 0.02, 0.0, WIN_STD_H, material=mat))
    parts.append(make_box("tie", -WIN_STD_W / 2.0, WIN_STD_W / 2.0,
                          -0.02, 0.02, WIN_STD_H / 2.0 - 0.02,
                          WIN_STD_H / 2.0 + 0.02, material=mat))
    bars = join_into(parts, "window_bars_standard")
    tag_bounds(bars, "openings")
    add_socket("mullion", (0.0, 0.0, 0.0), "plug", MULLION_FAMILY)
    export_glb("openings/window_bars_standard", "iron_forged")


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


def build_stair_quarter_turn_3m():
    """L stair with landing (backlog E): 7 risers ascending -Y to a 1x1 m
    turn landing (the 8th tread), then 8 risers turning +X to exactly 3.0 m.
    16 walkable compound parts; riser 0.1875 m throughout."""
    reset_scene()
    mat = tile_material("oak_timber")
    r = STAIR_RISER
    # run A: x +/-0.5, front edge y=1.75, stacked shrinking slabs like the
    # straight stair (slab i spans from the landing edge y=0 to its nose)
    for i in range(7):
        step = make_box("run_a_%02d" % (i + 1), -0.5, 0.5, 0.0,
                        1.75 - i * 0.25, i * r, (i + 1) * r, material=mat)
        tag_part(step, "traversal", walkable=True)
    # turn landing: 1 x 1 m solid column to the 8th tread height
    landing = make_box("turn_landing", -0.5, 0.5, -1.0, 0.0, 0.0, 8 * r,
                       material=mat)
    tag_part(landing, "traversal", walkable=True)
    # run B: solid columns ascending +X off the landing edge
    for j in range(8):
        step = make_box("run_b_%02d" % (j + 1), 0.5 + j * 0.25, 2.5, -1.0,
                        0.0, 0.0, (9 + j) * r, material=mat)
        tag_part(step, "traversal", walkable=True)
    export_glb("traversal/stair_quarter_turn_3m", "oak_timber")


def build_stair_landing_2x4m():
    reset_scene()
    mat = tile_material("oak_plank")
    slab = centered_box("stair_landing_2x4m", (4.0, 2.0, 0.2), base_z=0.0,
                        material=mat)
    tag_bounds(slab, "traversal", walkable=True)
    export_glb("traversal/stair_landing_2x4m", "oak_plank")


def _stepped_ramp(rel_path, name, width, run, rise):
    """Stepped walkable ramp (stealth ramp_2x2x1 precedent: compound tread
    parts, riser < the 0.35 m auto-step). Ascends -Y, front edge at
    y = +run/2, solid shrinking columns."""
    reset_scene()
    mat = tile_material("oak_plank")
    steps = int(round(rise / RAMP_RISER))
    tread = run / steps
    for i in range(steps):
        depth = run - i * tread
        step = centered_box("%s_tread_%02d" % (name, i + 1),
                            (width, depth, RAMP_RISER),
                            center_xy=(0.0, -i * tread / 2.0),
                            base_z=i * RAMP_RISER, material=mat)
        tag_part(step, "traversal", walkable=True)
    export_glb(rel_path, "oak_plank")


def build_ramp_2x3x1p5m():
    _stepped_ramp("traversal/ramp_2x3x1p5m", "ramp_2x3x1p5m", 2.0, 3.0, 1.5)


def build_ramp_2x6x3m():
    _stepped_ramp("traversal/ramp_2x6x3m", "ramp_2x6x3m", 2.0, 6.0, 3.0)


def build_ramp_landing_2x2m():
    reset_scene()
    mat = tile_material("oak_plank")
    slab = centered_box("ramp_landing_2x2m", (2.0, 2.0, 0.15), base_z=0.0,
                        material=mat)
    tag_bounds(slab, "traversal", walkable=True)
    export_glb("traversal/ramp_landing_2x2m", "oak_plank")


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
# Structural detail kit (backlog section C; sheet 06): foundation plinths,
# posts and round columns, beams, braces
# ---------------------------------------------------------------------------
def _plinth_run(name, x0, x1, material):
    """Base-course segment along X: body + proud cap ledge."""
    return [
        make_box(name + "_body", x0, x1, -PLINTH_D, PLINTH_D, 0.0,
                 PLINTH_BODY_H, material=material),
        make_box(name + "_ledge", x0, x1, -PLINTH_LEDGE, PLINTH_LEDGE,
                 PLINTH_BODY_H, PLINTH_H, material=material),
    ]


def _plinth_straight(rel_path, name, length):
    reset_scene()
    mat = tile_material("stone_rough")
    plinth = join_into(_plinth_run(name, -length / 2.0, length / 2.0, mat),
                       name)
    tag_none(plinth, "structural")
    export_glb(rel_path, "stone_rough")


def build_foundation_plinth_straight_2m():
    _plinth_straight("structural/foundation_plinth_straight_2m",
                     "foundation_plinth_straight_2m", 2.0)


def build_foundation_plinth_straight_4m():
    _plinth_straight("structural/foundation_plinth_straight_4m",
                     "foundation_plinth_straight_4m", 4.0)


def _plinth_corner(rel_path, name, with_quoin):
    """L junction: legs along +X and +Y. The convex (outer) corner adds a
    proud quoin block at the junction (sheet 06)."""
    reset_scene()
    mat = tile_material("stone_rough")
    parts = _plinth_run(name + "_x", 0.0, 0.5, mat)
    parts += [
        make_box(name + "_y_body", -PLINTH_D, PLINTH_D, 0.0, 0.5, 0.0,
                 PLINTH_BODY_H, material=mat),
        make_box(name + "_y_ledge", -PLINTH_LEDGE, PLINTH_LEDGE, 0.0, 0.5,
                 PLINTH_BODY_H, PLINTH_H, material=mat),
    ]
    if with_quoin:
        parts.append(make_box(name + "_quoin", -PLINTH_LEDGE, PLINTH_LEDGE,
                              -PLINTH_LEDGE, PLINTH_LEDGE, 0.0, 0.55,
                              material=mat))
    corner = join_into(parts, name)
    tag_none(corner, "structural")
    export_glb(rel_path, "stone_rough")


def build_foundation_plinth_inner_corner():
    _plinth_corner("structural/foundation_plinth_inner_corner",
                   "foundation_plinth_inner_corner", with_quoin=False)


def build_foundation_plinth_outer_corner():
    _plinth_corner("structural/foundation_plinth_outer_corner",
                   "foundation_plinth_outer_corner", with_quoin=True)


def build_foundation_plinth_end():
    """Finished exposed end: the ledge wraps past the body end."""
    reset_scene()
    mat = tile_material("stone_rough")
    parts = [
        make_box("plinth_end_body", 0.0, 0.4, -PLINTH_D, PLINTH_D, 0.0,
                 PLINTH_BODY_H, material=mat),
        make_box("plinth_end_ledge", 0.0, 0.45, -PLINTH_LEDGE, PLINTH_LEDGE,
                 PLINTH_BODY_H, PLINTH_H, material=mat),
    ]
    end = join_into(parts, "foundation_plinth_end")
    tag_none(end, "structural")
    export_glb("structural/foundation_plinth_end", "stone_rough")


def _post_square(rel_path, name, half):
    reset_scene()
    mat = tile_material("oak_timber")
    post = make_box(name, -half, half, -half, half, 0.0, STOREY,
                    material=mat)
    tag_bounds(post, "structural")
    export_glb(rel_path, "oak_timber")


def build_post_square_0p3x3m():
    _post_square("structural/post_square_0p3x3m", "post_square_0p3x3m", 0.15)


def build_post_square_0p5x3m():
    _post_square("structural/post_square_0p5x3m", "post_square_0p5x3m", 0.25)


def build_column_round_0p5x3m():
    """Storey round column; plug mates the base's receiver, receiver on top
    takes the cap (chained sockets)."""
    reset_scene()
    mat = tile_material("stone_rough")
    col = make_ngon_prism("column_round_0p5x3m", COLUMN_R, 0.0, STOREY,
                          COLUMN_SEGS, material=mat)
    tag_bounds(col, "structural")
    add_socket("column.base", (0.0, 0.0, 0.0), "plug", COLUMN_FAMILY)
    add_socket("column.cap", (0.0, 0.0, STOREY), "receiver",
               COLUMN_CAP_FAMILY)
    export_glb("structural/column_round_0p5x3m", "stone_rough")


def build_column_round_base():
    """Stepped base (sheet 06): square pad, round disc, round stub; receiver
    on top of the stub."""
    reset_scene()
    mat = tile_material("stone_rough")
    parts = [
        make_box("base_pad", -0.3, 0.3, -0.3, 0.3, 0.0, 0.08, material=mat),
        make_ngon_prism("base_disc", 0.28, 0.08, 0.18, COLUMN_SEGS,
                        material=mat),
        make_ngon_prism("base_stub", COLUMN_R, 0.18, 0.25, COLUMN_SEGS,
                        material=mat),
    ]
    base = join_into(parts, "column_round_base")
    tag_none(base, "structural")
    add_socket("column.base", (0.0, 0.0, 0.25), "receiver", COLUMN_FAMILY)
    export_glb("structural/column_round_base", "stone_rough")


def build_column_round_cap():
    """Round neck under a square abacus; plug mates the column's top
    receiver."""
    reset_scene()
    mat = tile_material("stone_rough")
    parts = [
        make_ngon_prism("cap_neck", COLUMN_R, 0.0, 0.15, COLUMN_SEGS,
                        material=mat),
        make_box("cap_abacus", -0.3, 0.3, -0.3, 0.3, 0.15, 0.25,
                 material=mat),
    ]
    cap = join_into(parts, "column_round_cap")
    tag_none(cap, "structural")
    add_socket("column.cap", (0.0, 0.0, 0.0), "plug", COLUMN_CAP_FAMILY)
    export_glb("structural/column_round_cap", "stone_rough")


def _beam(rel_path, name, length, half_w, height):
    reset_scene()
    mat = tile_material("oak_timber")
    beam = make_box(name, -length / 2.0, length / 2.0, -half_w, half_w, 0.0,
                    height, material=mat)
    tag_bounds(beam, "structural")
    export_glb(rel_path, "oak_timber")


def build_beam_2m():
    _beam("structural/beam_2m", "beam_2m", 2.0, BEAM_HALF_W, BEAM_H)


def build_beam_4m():
    _beam("structural/beam_4m", "beam_4m", 4.0, BEAM_HALF_W, BEAM_H)


def build_beam_6m():
    # long span with credible depth
    _beam("structural/beam_6m", "beam_6m", 6.0, 0.125, 0.45)


def build_beam_end_cap():
    """Exposed beam-end treatment: section stub with a proud end plate."""
    reset_scene()
    mat = tile_material("oak_timber")
    parts = [
        make_box("cap_stub", 0.0, 0.2, -BEAM_HALF_W, BEAM_HALF_W, 0.03,
                 0.33, material=mat),
        make_box("cap_plate", 0.2, 0.24, -0.13, 0.13, 0.0, 0.36,
                 material=mat),
    ]
    cap = join_into(parts, "beam_end_cap")
    tag_none(cap, "structural")
    export_glb("structural/beam_end_cap", "oak_timber")


def _brace(rel_path, name, hand):
    """Diagonal support with vertical-cut ends; left rises toward +X from
    the origin, right is the mirror (explicit mirrored pivots)."""
    reset_scene()
    mat = tile_material("oak_timber")
    if hand == "left":
        bar = sloped_bar_x(name, 0.0, 0.8, -0.06, 0.06, 0.0, 0.68, 0.17,
                           material=mat)
    else:
        bar = sloped_bar_x(name, -0.8, 0.0, -0.06, 0.06, 0.68, 0.0, 0.17,
                           material=mat)
    tag_bounds(bar, "structural")
    export_glb(rel_path, "oak_timber")


def build_brace_left():
    _brace("structural/brace_left", "brace_left", "left")


def build_brace_right():
    _brace("structural/brace_right", "brace_right", "right")


def _arch(rel_path, name, clear_w, spring, apex, post_w, half_d, band_h):
    """Passage arch (sheet 06): flanking posts to the spring line, segmental
    arch band chained from vertical-thickness segments along a circular arc
    (arc baked into verts). Compound: two posts + one band part, so the
    passage below the spring line stays clear."""
    reset_scene()
    mat = tile_material("oak_timber")
    half = clear_w / 2.0
    rise = apex - spring
    radius = (half * half + rise * rise) / (2.0 * rise)
    zc = apex - radius
    left = make_box("post_l", -half - post_w, -half, -half_d, half_d, 0.0,
                    spring, material=mat)
    tag_part(left, "structural", walkable=False)
    right = make_box("post_r", half, half + post_w, -half_d, half_d, 0.0,
                     spring, material=mat)
    tag_part(right, "structural", walkable=False)
    segs = 8
    pieces = []
    for k in range(segs):
        x0 = -half + clear_w * k / segs
        x1 = -half + clear_w * (k + 1) / segs
        zb0 = zc + math.sqrt(radius * radius - x0 * x0)
        zb1 = zc + math.sqrt(radius * radius - x1 * x1)
        pieces.append(sloped_bar_x("seg_%d" % k, x0, x1, -half_d, half_d,
                                   zb0, zb1, band_h, material=mat))
    band = join_into(pieces, name + "_band")
    tag_part(band, "structural", walkable=False)
    export_glb(rel_path, "oak_timber")


def build_arch_1p2m():
    _arch("structural/arch_1p2m", "arch_1p2m", 1.2, 1.8, 2.1, 0.15, 0.075,
          0.18)


def build_arch_2m():
    _arch("structural/arch_2m", "arch_2m", 2.0, 1.6, 2.1, 0.15, 0.075, 0.18)


def build_arch_4m():
    _arch("structural/arch_4m", "arch_4m", 4.0, 1.7, 2.7, 0.2, 0.1, 0.22)


def _buttress(rel_path, name, half_w, steps):
    """Stepped stone buttress projecting -Y from the wall face (sheet 06);
    steps = [(z0, z1, depth), ...]."""
    reset_scene()
    mat = tile_material("stone_rough")
    parts = []
    for i, (z0, z1, depth) in enumerate(steps):
        parts.append(make_box("step_%d" % i, -half_w, half_w, -depth, 0.0,
                              z0, z1, material=mat))
    buttress = join_into(parts, name)
    tag_bounds(buttress, "structural")
    export_glb(rel_path, "stone_rough")


def build_buttress_low():
    _buttress("structural/buttress_low", "buttress_low", 0.25,
              [(0.0, 1.2, 0.4), (1.2, 2.2, 0.28), (2.2, 3.0, 0.16)])


def build_buttress_tall():
    _buttress("structural/buttress_tall", "buttress_tall", 0.3,
              [(0.0, 1.6, 0.6), (1.6, 3.2, 0.45), (3.2, 4.6, 0.3),
               (4.6, 6.0, 0.15)])


def _wall_pier(rel_path, name, half_w):
    """Plastered masonry pier between openings (production pass on the
    homestead wall-pier roles). First lime_plaster consumer in the kit."""
    reset_scene()
    mat = tile_material("lime_plaster")
    pier = make_box(name, -half_w, half_w, -0.15, 0.15, 0.0, STOREY,
                    material=mat)
    tag_bounds(pier, "structural")
    export_glb(rel_path, "lime_plaster")


def build_wall_pier_0p5x3m():
    _wall_pier("structural/wall_pier_0p5x3m", "wall_pier_0p5x3m", 0.25)


def build_wall_pier_1x3m():
    _wall_pier("structural/wall_pier_1x3m", "wall_pier_1x3m", 0.5)


def _parapet_run(name, x0, x1, material):
    return [
        make_box(name + "_body", x0, x1, -PARAPET_D, PARAPET_D, 0.0,
                 PARAPET_BODY_H, material=material),
        make_box(name + "_coping", x0, x1, -PARAPET_COPING, PARAPET_COPING,
                 PARAPET_BODY_H, PARAPET_H, material=material),
    ]


def _parapet_straight(rel_path, name, length):
    reset_scene()
    mat = tile_material("stone_rough")
    parapet = join_into(_parapet_run(name, -length / 2.0, length / 2.0, mat),
                        name)
    tag_bounds(parapet, "structural")
    export_glb(rel_path, "stone_rough")


def build_parapet_straight_2m():
    _parapet_straight("structural/parapet_straight_2m",
                      "parapet_straight_2m", 2.0)


def build_parapet_straight_4m():
    _parapet_straight("structural/parapet_straight_4m",
                      "parapet_straight_4m", 4.0)


def _parapet_corner(rel_path, name, with_block):
    reset_scene()
    mat = tile_material("stone_rough")
    parts = _parapet_run(name + "_x", 0.0, 0.5, mat)
    parts += [
        make_box(name + "_y_body", -PARAPET_D, PARAPET_D, 0.0, 0.5, 0.0,
                 PARAPET_BODY_H, material=mat),
        make_box(name + "_y_coping", -PARAPET_COPING, PARAPET_COPING, 0.0,
                 0.5, PARAPET_BODY_H, PARAPET_H, material=mat),
    ]
    if with_block:
        parts.append(make_box(name + "_block", -PARAPET_COPING,
                              PARAPET_COPING, -PARAPET_COPING,
                              PARAPET_COPING, 0.0, 0.95, material=mat))
    corner = join_into(parts, name)
    tag_bounds(corner, "structural")
    export_glb(rel_path, "stone_rough")


def build_parapet_inner_corner():
    _parapet_corner("structural/parapet_inner_corner",
                    "parapet_inner_corner", with_block=False)


def build_parapet_outer_corner():
    _parapet_corner("structural/parapet_outer_corner",
                    "parapet_outer_corner", with_block=True)


def build_parapet_end():
    """Finished end: the coping wraps past the body end."""
    reset_scene()
    mat = tile_material("stone_rough")
    parts = [
        make_box("parapet_end_body", 0.0, 0.4, -PARAPET_D, PARAPET_D, 0.0,
                 PARAPET_BODY_H, material=mat),
        make_box("parapet_end_coping", 0.0, 0.45, -PARAPET_COPING,
                 PARAPET_COPING, PARAPET_BODY_H, PARAPET_H, material=mat),
    ]
    end = join_into(parts, "parapet_end")
    tag_bounds(end, "structural")
    export_glb("structural/parapet_end", "stone_rough")


def _balcony_deck(rel_path, name, half_w, joist_xs):
    """Walkable compound deck mounting at the wall plane (local y=0),
    projecting -Y (exterior): walkable slab part over non-walkable joists."""
    reset_scene()
    mat = tile_material("oak_plank")
    slab = make_box(name + "_deck", -half_w, half_w, -1.5, 0.0, 0.0, 0.15,
                    material=mat)
    tag_part(slab, "structural", walkable=True)
    for jx in joist_xs:
        joist = make_box(name + "_joist_%+.1f" % jx, jx - 0.06, jx + 0.06,
                         -1.4, 0.0, -0.12, 0.0, material=mat)
        tag_part(joist, "structural", walkable=False)
    export_glb(rel_path, "oak_plank")


def build_balcony_deck_2x1p5m():
    _balcony_deck("structural/balcony_deck_2x1p5m", "balcony_deck_2x1p5m",
                  1.0, (-0.75, 0.75))


def build_balcony_deck_4x1p5m():
    _balcony_deck("structural/balcony_deck_4x1p5m", "balcony_deck_4x1p5m",
                  2.0, (-1.5, 0.0, 1.5))


def build_balcony_bracket():
    """Non-walkable support under a balcony deck: wall arm, horizontal arm,
    diagonal strut (sheet 06 bracket language)."""
    reset_scene()
    mat = tile_material("oak_timber")
    parts = [
        make_box("arm_top", -0.06, 0.06, -0.5, 0.0, -0.12, 0.0,
                 material=mat),
        make_box("arm_wall", -0.06, 0.06, -0.12, 0.0, -0.55, 0.0,
                 material=mat),
        sloped_bar_y("strut", -0.05, 0.05, -0.45, -0.1, -0.22, -0.52, 0.1,
                     material=mat),
    ]
    bracket = join_into(parts, "balcony_bracket")
    tag_none(bracket, "structural")
    export_glb("structural/balcony_bracket", "oak_timber")


def _railing_run(name, x0, x1, material):
    return [
        make_box(name + "_top", x0, x1, -0.03, 0.03, 0.92, 0.98,
                 material=material),
        make_box(name + "_mid", x0, x1, -0.03, 0.03, 0.48, 0.54,
                 material=material),
    ]


def build_railing_straight_1m():
    reset_scene()
    mat = tile_material("oak_timber")
    parts = [
        make_box("post_l", -0.5, -0.42, -0.04, 0.04, 0.0, 1.0, material=mat),
        make_box("post_r", 0.42, 0.5, -0.04, 0.04, 0.0, 1.0, material=mat),
    ] + _railing_run("rail", -0.5, 0.5, mat)
    rail = join_into(parts, "railing_straight_1m")
    tag_bounds(rail, "structural")
    export_glb("structural/railing_straight_1m", "oak_timber")


def build_railing_corner():
    """90-degree railing junction: corner post with legs along +X and +Y."""
    reset_scene()
    mat = tile_material("oak_timber")
    parts = [make_box("corner_post", -0.04, 0.04, -0.04, 0.04, 0.0, 1.0,
                      material=mat)]
    parts += _railing_run("leg_x", 0.04, 0.5, mat)
    parts += [
        make_box("leg_y_top", -0.03, 0.03, 0.04, 0.5, 0.92, 0.98,
                 material=mat),
        make_box("leg_y_mid", -0.03, 0.03, 0.04, 0.5, 0.48, 0.54,
                 material=mat),
        make_box("end_x", 0.42, 0.5, -0.04, 0.04, 0.0, 1.0, material=mat),
        make_box("end_y", -0.04, 0.04, 0.42, 0.5, 0.0, 1.0, material=mat),
    ]
    corner = join_into(parts, "railing_corner")
    tag_bounds(corner, "structural")
    export_glb("structural/railing_corner", "oak_timber")


def build_railing_end_post():
    reset_scene()
    mat = tile_material("oak_timber")
    parts = [
        make_box("post", -0.04, 0.04, -0.04, 0.04, 0.0, 1.0, material=mat),
        make_box("cap", -0.055, 0.055, -0.055, 0.055, 1.0, 1.05,
                 material=mat),
    ]
    post = join_into(parts, "railing_end_post")
    tag_bounds(post, "structural")
    export_glb("structural/railing_end_post", "oak_timber")


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
        build_door_frame_double,
        build_door_leaf_double_left_closed,
        build_door_leaf_double_left_open,
        build_door_leaf_double_right_closed,
        build_door_leaf_double_right_open,
        build_window_frame_standard,
        build_window_frame_small,
        build_window_frame_wide,
        build_window_frame_tall,
        build_window_bars_standard,
        build_stair_quarter_turn_3m,
        build_stair_landing_2x4m,
        build_ramp_2x3x1p5m,
        build_ramp_2x6x3m,
        build_ramp_landing_2x2m,
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
        build_foundation_plinth_straight_2m,
        build_foundation_plinth_straight_4m,
        build_foundation_plinth_inner_corner,
        build_foundation_plinth_outer_corner,
        build_foundation_plinth_end,
        build_post_square_0p3x3m,
        build_post_square_0p5x3m,
        build_column_round_0p5x3m,
        build_column_round_base,
        build_column_round_cap,
        build_beam_2m,
        build_beam_4m,
        build_beam_6m,
        build_beam_end_cap,
        build_brace_left,
        build_brace_right,
        build_arch_1p2m,
        build_arch_2m,
        build_arch_4m,
        build_buttress_low,
        build_buttress_tall,
        build_wall_pier_0p5x3m,
        build_wall_pier_1x3m,
        build_parapet_straight_2m,
        build_parapet_straight_4m,
        build_parapet_inner_corner,
        build_parapet_outer_corner,
        build_parapet_end,
        build_balcony_deck_2x1p5m,
        build_balcony_deck_4x1p5m,
        build_balcony_bracket,
        build_railing_straight_1m,
        build_railing_corner,
        build_railing_end_post,
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
