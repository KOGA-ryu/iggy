"""Calibration and pipeline-proof kit generator (ASSET-CAL-1, Batch 0).

Run headless only:
    blender --background --python generate_kit.py

Regenerates every .glb under assets/creative/calibration/ from the metric
constants below (engine provenance in ../README.md). Blockout-grade:
axis-aligned boxes, one flat material per asset, grounded assets rest on Z=0,
XZ centered. Determinism: no hand-saved .blend; this script is the source.

Conventions matched to the stealth_blockout kit (assets/creative/
stealth_blockout/tools/generate_kit.py) and to the importer contract in
src/content/assets/StaticMeshAuthoringMetadata.cpp + StaticMeshAsset.cpp:

  single collidable box  -> node extras {iggy_category, iggy_collision:"bounds",
                            iggy_walkable?} (iggy_walkable only valid on bounds)
  non-colliding proof    -> {iggy_category, iggy_collision:"none"} (no walkable)
  multi-box compound     -> every node {iggy_category,
                            iggy_collision:"compound_bounds"}; collidable nodes
                            add {iggy_collision_part:"bounds",
                            iggy_collision_part_walkable}

  NEW CONTRACT SURFACE (no stealth-kit precedent): named attachment sockets.
  A socket is an unrotated (or yaw-only) empty node carrying
    {iggy_socket, iggy_socket_role:("receiver"|"plug"),
     iggy_socket_compatibility}
  The importer reads the node's world transform: position = node origin,
  forward = +Z (glTF) = Blender -Y, up = +Y (glTF) = Blender +Z. The attachment
  snap requires up.y > 0.999 and forward horizontal, so socket empties MUST be
  unrotated or rotated about Blender Z (world up) only. Socket key names are
  recorded in ../README.md and flagged as new contract surface.
"""

import os

import bpy

# ---------------------------------------------------------------------------
# Measured engine metrics (provenance in stealth_blockout/README.md table)
# ---------------------------------------------------------------------------
STOREY = 3.0              # normal floor-to-floor (Production Law 4)
DOOR_CLEAR_W = 0.9        # interior door clear opening width (Law 4)
DOOR_CLEAR_H = 2.1        # interior door clear opening height (Law 4)
PLAYER_HEIGHT = 1.80      # MovementParams.hpp:8
STAND_EYE = 1.6           # NpcBehaviorProfile.hpp:26 (stand/guard eye)
SNEAK_EYE = 0.9           # NpcBehaviorProfile.hpp:27
LINTEL = DOOR_CLEAR_H     # door head height marker
CEILING = 2.7             # readable ceiling (0.3 floor assembly under 3.0)

CATEGORY = "calibration"
SOCKET_FAMILY = "calibration.socket"

OUT_DIR = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))

FAMILY_COLOR = ("Calibration", (0.20, 0.55, 0.85, 1.0))
WALK_COLOR = ("Calibration Walkable", (0.25, 0.70, 0.35, 1.0))
SUPPORT_COLOR = ("Calibration Support", (0.55, 0.40, 0.20, 1.0))
SOCKET_COLOR = ("Calibration Socket", (0.85, 0.55, 0.15, 1.0))


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def make_material(name_rgba):
    name, rgba = name_rgba
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = rgba
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.95
    mat.diffuse_color = rgba
    return mat


def make_box(name, x0, x1, y0, y1, z0, z1, material=None):
    """Axis-aligned box in explicit min/max coords, object origin at (0,0,0).

    Deterministic from_pydata build (no operator/context dependence). glTF
    export triangulates the six quads to 12 triangles.
    """
    verts = [
        (x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
        (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1),
    ]
    faces = [
        (0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
        (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0),
    ]
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
    return make_box(name, cx - sx / 2.0, cx + sx / 2.0,
                    cy - sy / 2.0, cy + sy / 2.0,
                    base_z, base_z + sz, material=material)


def join_into(objects, final_name):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    result = bpy.context.view_layer.objects.active
    result.name = final_name
    return result


def tag_none(obj):
    obj["iggy_category"] = CATEGORY
    obj["iggy_collision"] = "none"


def tag_bounds(obj, walkable=None):
    obj["iggy_category"] = CATEGORY
    obj["iggy_collision"] = "bounds"
    if walkable is not None:
        obj["iggy_walkable"] = walkable


def tag_part(obj, walkable):
    obj["iggy_category"] = CATEGORY
    obj["iggy_collision"] = "compound_bounds"
    obj["iggy_collision_part"] = "bounds"
    obj["iggy_collision_part_walkable"] = walkable


def add_socket(name, location, role, compatibility, yaw_deg=0.0):
    """Attachment-socket empty. Unrotated or yaw-only so up stays world +Y."""
    empty = bpy.data.objects.new(name, None)
    bpy.context.collection.objects.link(empty)
    empty.empty_display_type = "ARROWS"
    empty.empty_display_size = 0.2
    empty.location = location
    if yaw_deg:
        empty.rotation_euler = (0.0, 0.0, yaw_deg * 3.141592653589793 / 180.0)
    empty["iggy_socket"] = name
    empty["iggy_socket_role"] = role
    empty["iggy_socket_compatibility"] = compatibility
    return empty


def axis_markers(prefix, origin, length, material):
    """Three short bars along +X/+Y/+Z from a socket origin (visualization)."""
    ox, oy, oz = origin
    t = 0.02
    return [
        make_box(prefix + "_ax", ox, ox + length, oy - t, oy + t,
                 oz - t, oz + t, material=material),
        make_box(prefix + "_ay", ox - t, ox + t, oy, oy + length,
                 oz - t, oz + t, material=material),
        make_box(prefix + "_az", ox - t, ox + t, oy - t, oy + t,
                 oz, oz + length, material=material),
    ]


def apply_mesh_transforms():
    """Bake every mesh object's TRS into its vertices so exported mesh nodes have
    identity transforms. Socket empties keep their world transform — that IS the
    socket contract."""
    bpy.ops.object.select_all(action="DESELECT")
    active = None
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            obj.select_set(True)
            active = obj
    if active is not None:
        bpy.context.view_layer.objects.active = active
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)


def export_glb(rel_name):
    apply_mesh_transforms()
    path = os.path.join(OUT_DIR, rel_name + ".glb")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=path, export_format="GLB", export_extras=True,
        export_apply=True, export_yup=True)
    print("WROTE", path)


# ---------------------------------------------------------------------------
# Assets
# ---------------------------------------------------------------------------
def build_grid():
    """Non-colliding 1 m reference grid, 10 x 10 m on the ground plane."""
    reset_scene()
    mat = make_material(FAMILY_COLOR)
    parts = []
    t = 0.02
    for i in range(11):
        y = -5.0 + i
        parts.append(centered_box("grid_x_%02d" % i, (10.0, t, t),
                                   center_xy=(0.0, y), material=mat))
        x = -5.0 + i
        parts.append(centered_box("grid_y_%02d" % i, (t, 10.0, t),
                                   center_xy=(x, 0.0), material=mat))
    grid = join_into(parts, "grid_1m_10x10")
    tag_none(grid)
    export_glb("grid_1m_10x10")


def build_human_gauge():
    """Canonical 1.8 m scale figure with stand-eye and sneak-eye datum plates."""
    reset_scene()
    mat = make_material(FAMILY_COLOR)
    parts = [
        centered_box("body", (0.30, 0.20, 1.56), base_z=0.0, material=mat),
        centered_box("head", (0.24, 0.24, 0.24), base_z=1.56, material=mat),
        centered_box("shoulders", (0.50, 0.24, 0.05), base_z=1.40,
                     material=mat),
        centered_box("stand_eye_1p6", (0.40, 0.30, 0.02),
                     base_z=STAND_EYE - 0.01, material=mat),
        centered_box("sneak_eye_0p9", (0.40, 0.30, 0.02),
                     base_z=SNEAK_EYE - 0.01, material=mat),
        centered_box("foot", (0.36, 0.30, 0.02), base_z=0.0, material=mat),
    ]
    gauge = join_into(parts, "human_gauge_1p8m")
    tag_none(gauge)
    export_glb("human_gauge_1p8m")


def build_door_clearance():
    """Exact 0.9 x 2.1 m clear-opening gauge (inner faces)."""
    reset_scene()
    mat = make_material(FAMILY_COLOR)
    half = DOOR_CLEAR_W / 2.0
    post = 0.05
    parts = [
        # left/right posts: inner faces at -/+half, so clear width == 0.9
        make_box("post_l", -half - post, -half, -0.025, 0.025, 0.0,
                 DOOR_CLEAR_H, material=mat),
        make_box("post_r", half, half + post, -0.025, 0.025, 0.0,
                 DOOR_CLEAR_H, material=mat),
        # lintel: underside at 2.1, so clear height == 2.1
        make_box("lintel", -half - post, half + post, -0.025, 0.025,
                 DOOR_CLEAR_H, DOOR_CLEAR_H + post, material=mat),
        # threshold datum on the ground
        make_box("threshold", -half - post, half + post, -0.025, 0.025,
                 0.0, 0.01, material=mat),
    ]
    gauge = join_into(parts, "door_clearance_0p9x2p1")
    tag_none(gauge)
    export_glb("door_clearance_0p9x2p1")


def build_storey():
    """Floor / eye / lintel / ceiling / next-floor markers over a 3.0 m storey."""
    reset_scene()
    mat = make_material(FAMILY_COLOR)
    parts = [centered_box("post", (0.10, 0.10, STOREY), material=mat)]
    for label, z in (("floor_0", 0.0), ("eye_1p6", STAND_EYE),
                     ("lintel_2p1", LINTEL), ("ceiling_2p7", CEILING),
                     ("next_floor_3p0", STOREY)):
        plate_z = min(z, STOREY - 0.02)
        parts.append(centered_box(label, (0.60, 0.60, 0.02), base_z=plate_z,
                                  material=mat))
    storey = join_into(parts, "storey_3m")
    tag_none(storey)
    export_glb("storey_3m")


def build_pivot_hinge():
    """Door/gate hinge-pivot proof: leaf panel with object origin AT the hinge
    edge (local x=0), so the pivot lands on the hinge line after glTF conversion.
    """
    reset_scene()
    mat = make_material(FAMILY_COLOR)
    # panel spans +X from the hinge at local origin; bounds.min.x == 0
    make_box("pivot_hinge", 0.0, DOOR_CLEAR_W, -0.025, 0.025, 0.0,
             DOOR_CLEAR_H, material=mat)
    panel = bpy.context.view_layer.objects.active = bpy.data.objects[
        "pivot_hinge"]
    tag_bounds(panel)
    export_glb("pivot_hinge")


def build_socket_receiver():
    """Visible receiver frame + axis markers; one receiver socket."""
    reset_scene()
    frame_mat = make_material(FAMILY_COLOR)
    mark_mat = make_material(SOCKET_COLOR)
    socket_z = 0.5
    parts = [
        centered_box("base", (0.40, 0.40, 0.05), base_z=0.0, material=frame_mat),
        make_box("back", -0.20, 0.20, -0.20, -0.15, 0.05, 0.90,
                 material=frame_mat),
    ]
    parts += axis_markers("recv_mark", (0.0, 0.0, socket_z), 0.20, mark_mat)
    frame = join_into(parts, "socket_receiver")
    tag_none(frame)
    add_socket("receiver.main", (0.0, 0.0, socket_z), "receiver", SOCKET_FAMILY)
    export_glb("socket_receiver")


def build_socket_plug():
    """Compatible plug body + axis markers; one plug socket."""
    reset_scene()
    body_mat = make_material(FAMILY_COLOR)
    mark_mat = make_material(SOCKET_COLOR)
    socket_z = 0.3
    parts = [
        centered_box("plug_body", (0.20, 0.20, 0.40), base_z=0.0,
                     material=body_mat),
        centered_box("plug_nub", (0.08, 0.08, 0.12), base_z=0.40,
                     material=body_mat),
    ]
    parts += axis_markers("plug_mark", (0.0, 0.0, socket_z), 0.20, mark_mat)
    body = join_into(parts, "socket_plug")
    tag_none(body)
    add_socket("plug.main", (0.0, 0.0, socket_z), "plug", SOCKET_FAMILY)
    export_glb("socket_plug")


def build_collision_compound():
    """Multi-part compound: non-walkable support + walkable cap top."""
    reset_scene()
    support_mat = make_material(SUPPORT_COLOR)
    walk_mat = make_material(WALK_COLOR)
    support = centered_box("support", (1.0, 1.0, 0.5), base_z=0.0,
                           material=support_mat)
    tag_part(support, walkable=False)
    cap = centered_box("walkable_cap", (1.4, 1.4, 0.2), base_z=0.5,
                       material=walk_mat)
    tag_part(cap, walkable=True)
    export_glb("collision_compound")


def main():
    build_grid()
    build_human_gauge()
    build_door_clearance()
    build_storey()
    build_pivot_hinge()
    build_socket_receiver()
    build_socket_plug()
    build_collision_compound()
    print("CALIBRATION KIT GENERATION COMPLETE")


if __name__ == "__main__":
    main()
