"""Architecture building-closure kit generator (ASSET-CAL-1, Batch 1 seed).

Run headless only:
    blender --background --python generate_kit.py

Regenerates the eight architecture GLBs under assets/creative/architecture/
(openings/, traversal/, structural/, roof/). Blockout-grade, meters, grounded
assets rest on Z=0, XZ centered, one flat material per asset. This script is
the deterministic source (no hand-saved .blend).

Contract (see ../../calibration/README.md and the importer
src/content/assets/StaticMeshAuthoringMetadata.cpp):
  single box     -> {iggy_category, iggy_collision:"bounds", iggy_walkable?}
  compound       -> every node {iggy_category, iggy_collision:"compound_bounds"};
                    collidable nodes add {iggy_collision_part:"bounds",
                    iggy_collision_part_walkable}
  socket empty   -> unrotated/yaw-only {iggy_socket, iggy_socket_role,
                    iggy_socket_compatibility}

Door state family (Production Law 3): door_leaf_standard_closed / _open are two
static assets sharing one identical hinge pivot (object origin at the hinge
edge, local x=0) and one identical plug socket at that origin.
"""

import math
import os

import bpy

# Roof caps match the generated roof recipe default pitch so Blender trim and
# the generated roof plane share a slope (kDefaultCreativeStructuralRoofPitchDegrees
# in src/app/iggy3d/creative/recipes/StructuralRoofRecipe.hpp).
ROOF_PITCH_DEG = 30.0
RIDGE_HALF = 0.20
RIDGE_APEX = RIDGE_HALF * math.tan(math.radians(ROOF_PITCH_DEG))

STOREY = 3.0
DOOR_CLEAR_W = 0.9
DOOR_CLEAR_H = 2.1
STAIR_RISE = STOREY          # full-storey stair, exact 3.0 m
STAIR_STEPS = 16
STAIR_RISER = STAIR_RISE / STAIR_STEPS   # 0.1875 (< auto-step 0.35)
STAIR_TREAD = 0.25
STAIR_WIDTH = 1.0

DOOR_FAMILY = "architecture.door_standard"

OUT_DIR = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))

COLORS = {
    "openings": ("Opening", (0.75, 0.68, 0.45, 1.0)),
    "leaf": ("Door Leaf", (0.60, 0.42, 0.24, 1.0)),
    "traversal": ("Traversal", (0.80, 0.42, 0.10, 1.0)),
    "walk": ("Traversal Walkable", (0.25, 0.70, 0.35, 1.0)),
    "structural": ("Structural", (0.52, 0.52, 0.55, 1.0)),
    "roof": ("Roof", (0.62, 0.28, 0.24, 1.0)),
    "socket": ("Socket", (0.85, 0.55, 0.15, 1.0)),
}


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def make_material(key):
    name, rgba = COLORS[key]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = rgba
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.95
    mat.diffuse_color = rgba
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
    """Bake every mesh object's TRS into its vertices so exported mesh nodes have
    identity transforms. Socket empties are left untouched — their world
    transform IS the socket contract."""
    bpy.ops.object.select_all(action="DESELECT")
    active = None
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            obj.select_set(True)
            active = obj
    if active is not None:
        bpy.context.view_layer.objects.active = active
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)


def export_glb(rel_path):
    apply_mesh_transforms()
    path = os.path.join(OUT_DIR, rel_path + ".glb")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=path, export_format="GLB", export_extras=True,
        export_apply=True, export_yup=True)
    print("WROTE", path)


# ---------------------------------------------------------------------------
def build_door_frame_standard():
    """0.9 x 2.1 m clear opening; compound jambs+lintel; one hinge receiver."""
    reset_scene()
    mat = make_material("openings")
    half = DOOR_CLEAR_W / 2.0
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
    # receiver at the left hinge line, floor level; leaf plug lands here
    add_socket("hinge.left", (-half, 0.0, 0.0), "receiver", DOOR_FAMILY)
    export_glb("openings/door_frame_standard")


def _door_leaf(name, closed):
    reset_scene()
    mat = make_material("leaf")
    if closed:
        # hinge at local x=0, panel fills the opening along +X
        panel = make_box(name, 0.0, DOOR_CLEAR_W, -0.025, 0.025, 0.0,
                         DOOR_CLEAR_H, material=mat)
    else:
        # same hinge origin, statically swung 90 deg into -Y
        panel = make_box(name, -0.025, 0.025, -DOOR_CLEAR_W, 0.0, 0.0,
                         DOOR_CLEAR_H, material=mat)
    tag_bounds(panel, "openings")
    # identical plug at the hinge origin for both states (Law 3)
    add_socket("hinge", (0.0, 0.0, 0.0), "plug", DOOR_FAMILY)
    return panel


def build_door_leaf_closed():
    _door_leaf("door_leaf_standard_closed", closed=True)
    export_glb("openings/door_leaf_standard_closed")


def build_door_leaf_open():
    _door_leaf("door_leaf_standard_open", closed=False)
    export_glb("openings/door_leaf_standard_open")


def build_stair_straight_3m():
    """Full-storey straight stair; compound collision matches visible steps."""
    reset_scene()
    mat = make_material("walk")
    run = STAIR_TREAD * STAIR_STEPS
    for i in range(STAIR_STEPS):
        depth = run - i * STAIR_TREAD
        step = centered_box("step_%02d" % (i + 1),
                            (STAIR_WIDTH, depth, STAIR_RISER),
                            center_xy=(0.0, -i * STAIR_TREAD / 2.0),
                            base_z=i * STAIR_RISER, material=mat)
        tag_part(step, "traversal", walkable=True)
    export_glb("traversal/stair_straight_3m")


def build_stair_landing_2x2m():
    """Walkable 2 x 2 m landing (single bounds, walkable top)."""
    reset_scene()
    mat = make_material("walk")
    slab = centered_box("stair_landing_2x2m", (2.0, 2.0, 0.2), base_z=0.0,
                        material=mat)
    tag_bounds(slab, "traversal", walkable=True)
    export_glb("traversal/stair_landing_2x2m")


def build_railing_straight_2m():
    """2 m traversal-blocker rail: two posts + top and mid rails."""
    reset_scene()
    mat = make_material("structural")
    parts = [
        make_box("post_l", -1.0, -0.92, -0.04, 0.04, 0.0, 1.0, material=mat),
        make_box("post_r", 0.92, 1.0, -0.04, 0.04, 0.0, 1.0, material=mat),
        make_box("rail_top", -1.0, 1.0, -0.03, 0.03, 0.92, 0.98, material=mat),
        make_box("rail_mid", -1.0, 1.0, -0.03, 0.03, 0.48, 0.54, material=mat),
    ]
    rail = join_into(parts, "railing_straight_2m")
    tag_bounds(rail, "structural")
    export_glb("structural/railing_straight_2m")


def _ridge_prism(name, length, base_half, apex_z, out_half, material):
    """Ridge cap along +/-X: inner cross-section base full, outer narrowed to
    out_half (out_half == base_half -> straight cap; < -> finished end)."""
    x0 = -length / 2.0
    x1 = length / 2.0
    verts = [
        (x0, -base_half, 0.0), (x0, base_half, 0.0), (x0, 0.0, apex_z),
        (x1, -out_half, 0.0), (x1, out_half, 0.0), (x1, 0.0, apex_z),
    ]
    faces = [
        (0, 1, 2),            # inner end triangle
        (5, 4, 3),            # outer end triangle
        (0, 3, 4, 1),         # base quad
        (1, 4, 5, 2),         # +Y slope
        (2, 5, 3, 0),         # -Y slope
    ]
    return make_prism(name, verts, faces, material=material)


def build_ridge_cap_straight_4m():
    reset_scene()
    mat = make_material("roof")
    # apex up, symmetric slopes at the 30 deg roof-recipe pitch, baked into verts
    cap = _ridge_prism("ridge_cap_straight_4m", 4.0, RIDGE_HALF, RIDGE_APEX,
                       RIDGE_HALF, mat)
    tag_bounds(cap, "roof")
    export_glb("roof/ridge_cap_straight_4m")


def build_ridge_cap_end():
    reset_scene()
    mat = make_material("roof")
    # short finished end: outer cross-section tapers toward the level ridge line
    cap = _ridge_prism("ridge_cap_end", 0.4, RIDGE_HALF, RIDGE_APEX, 0.05, mat)
    tag_bounds(cap, "roof")
    export_glb("roof/ridge_cap_end")


def main():
    build_door_frame_standard()
    build_door_leaf_closed()
    build_door_leaf_open()
    build_stair_straight_3m()
    build_stair_landing_2x2m()
    build_railing_straight_2m()
    build_ridge_cap_straight_4m()
    build_ridge_cap_end()
    print("ARCHITECTURE KIT GENERATION COMPLETE")


if __name__ == "__main__":
    main()
