"""Headless proof renders for the ASSET-CAL-1 batch (calibration + architecture).

Run headless only (Workbench engine -> no GPU surface / window):
    blender --background --python render_proofs.py

Writes, per Production Law 6:
  calibration/proofs/gallery_all16.png       gallery of all 16 assets
  calibration/proofs/scale_human_gauge.png   scale check beside the 1.8 m gauge
  calibration/proofs/collision_walkability.png  walkable (green) vs support
  calibration/proofs/socket_alignment.png    receiver+plug and frame+leaf aligned
  architecture/proofs/assembled_bay.png      the 4 x 4 m bay use-case
"""

import math
import os

import bpy
from mathutils import Vector

CREATIVE = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 os.pardir, os.pardir))
CAL = os.path.join(CREATIVE, "calibration")
ARC = os.path.join(CREATIVE, "architecture")
CAL_PROOFS = os.path.join(CAL, "proofs")
ARC_PROOFS = os.path.join(ARC, "proofs")

ALL16 = [
    os.path.join(CAL, "grid_1m_10x10.glb"),
    os.path.join(CAL, "human_gauge_1p8m.glb"),
    os.path.join(CAL, "door_clearance_0p9x2p1.glb"),
    os.path.join(CAL, "storey_3m.glb"),
    os.path.join(CAL, "pivot_hinge.glb"),
    os.path.join(CAL, "socket_receiver.glb"),
    os.path.join(CAL, "socket_plug.glb"),
    os.path.join(CAL, "collision_compound.glb"),
    os.path.join(ARC, "openings/door_frame_standard.glb"),
    os.path.join(ARC, "openings/door_leaf_standard_closed.glb"),
    os.path.join(ARC, "openings/door_leaf_standard_open.glb"),
    os.path.join(ARC, "traversal/stair_straight_3m.glb"),
    os.path.join(ARC, "traversal/stair_landing_2x2m.glb"),
    os.path.join(ARC, "structural/railing_straight_2m.glb"),
    os.path.join(ARC, "roof/ridge_cap_straight_4m.glb"),
    os.path.join(ARC, "roof/ridge_cap_end.glb"),
]


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    shading = scene.display.shading
    shading.light = "STUDIO"
    shading.color_type = "MATERIAL"
    shading.show_shadows = True
    shading.show_cavity = True
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 960
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "Standard"


def add_floor(span, z=0.0):
    bpy.ops.mesh.primitive_plane_add(size=max(24.0, span * 3.0),
                                     location=(0, 0, z))
    floor = bpy.context.active_object
    mat = bpy.data.materials.new("ProofFloor")
    mat.diffuse_color = (0.78, 0.78, 0.80, 1.0)
    floor.data.materials.append(mat)
    return floor


def import_glb(path, location=(0.0, 0.0, 0.0), rot_z=0.0):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    roots = [o for o in new if o.parent is None]
    for r in roots:
        r.location = (r.location.x + location[0], r.location.y + location[1],
                      r.location.z + location[2])
        r.rotation_euler.z += rot_z
    return [o for o in new if o.type == "MESH"]


def world_bounds(objects):
    lo = [1e9] * 3
    hi = [-1e9] * 3
    for obj in objects:
        for corner in obj.bound_box:
            w = obj.matrix_world @ Vector(corner)
            for i in range(3):
                lo[i] = min(lo[i], w[i])
                hi[i] = max(hi[i], w[i])
    return lo, hi


def place_camera(lo, hi, azimuth_deg=-58.0, elev=0.62, pad=1.5):
    cx, cy, cz = [(lo[i] + hi[i]) / 2.0 for i in range(3)]
    span = max(hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2])
    dist = span * pad + 3.0
    az = math.radians(azimuth_deg)
    bpy.ops.object.camera_add()
    cam = bpy.context.active_object
    cam.location = (cx + dist * math.cos(az), cy + dist * math.sin(az),
                    cz + dist * elev)
    d = (cx - cam.location.x, cy - cam.location.y, cz - cam.location.z)
    rot_z = math.atan2(d[1], d[0]) - math.pi / 2.0
    horiz = math.hypot(d[0], d[1])
    rot_x = math.pi / 2.0 - math.atan2(-d[2], horiz)
    cam.rotation_euler = (rot_x, 0.0, rot_z)
    bpy.context.scene.camera = cam
    return cam


def render_to(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print("RENDERED", path)


def add_label(text, center_x, front_y):
    """Flat ground text (reads from an elevated 3/4 camera)."""
    curve = bpy.data.curves.new(text, "FONT")
    curve.body = text
    curve.size = 0.42
    curve.align_x = "CENTER"
    obj = bpy.data.objects.new("lbl_" + text, curve)
    bpy.context.collection.objects.link(obj)
    obj.location = (center_x, front_y, 0.02)
    mat = bpy.data.materials.new("Label")
    mat.diffuse_color = (0.08, 0.08, 0.10, 1.0)
    obj.data.materials.append(mat)
    return obj


GALLERY_LABELS = [
    "1 grid_1m_10x10", "2 human_gauge", "3 door_clearance", "4 storey_3m",
    "5 pivot_hinge", "6 socket_receiver", "7 socket_plug", "8 collision_cmpd",
    "9 door_frame", "10 leaf_closed", "11 leaf_open", "12 stair_3m",
    "13 landing_2x2", "14 railing_2m", "15 ridge_4m", "16 ridge_end",
]


def render_gallery():
    """Contact sheet: each asset framed tight to fill its own tile, then tiled
    4x4 with labels via ImageMagick montage (assets vary 0.4-10 m, so a single
    perspective frame cannot show them all readably)."""
    import subprocess
    tiles_dir = os.path.join(CAL_PROOFS, "tiles")
    os.makedirs(tiles_dir, exist_ok=True)
    # The two roof caps are long (4 m / 0.4 m) but shallow (0.115 m); the shared
    # 3/4 camera views them nearly edge-on so they read as sticks. Give them a
    # low, near-end-on camera so the apex-up triangular cross-section is legible.
    cam_override = {14: (-90.0, 0.16, 1.35), 15: (-90.0, 0.16, 1.5)}
    tile_paths = []
    for idx, path in enumerate(ALL16):
        reset_scene()
        bpy.context.scene.render.resolution_x = 512
        bpy.context.scene.render.resolution_y = 512
        objs = import_glb(path)
        lo, hi = world_bounds(objs)
        add_floor(max(hi[0] - lo[0], hi[1] - lo[1]))
        azimuth, elev, pad = cam_override.get(idx, (-55.0, 0.6, 1.7))
        place_camera(lo, hi, azimuth_deg=azimuth, elev=elev, pad=pad)
        tile = os.path.join(tiles_dir, GALLERY_LABELS[idx] + ".png")
        render_to(tile)
        tile_paths.append(tile)
    out = os.path.join(CAL_PROOFS, "gallery_all16.png")
    subprocess.run(
        ["montage", "-label", "%t", "-pointsize", "20", "-fill", "#101014",
         *tile_paths, "-tile", "4x4", "-geometry", "+6+6",
         "-background", "#c8c8cc", "-title", "ASSET-CAL-1 - 16 assets", out],
        check=True)
    for tile in tile_paths:
        os.remove(tile)
    os.rmdir(tiles_dir)
    print("RENDERED", out)


def render_scale():
    reset_scene()
    row = ["human_gauge_1p8m.glb", "door_clearance_0p9x2p1.glb",
           "storey_3m.glb", "pivot_hinge.glb"]
    arc = [os.path.join(ARC, "openings/door_frame_standard.glb"),
           os.path.join(ARC, "traversal/stair_straight_3m.glb")]
    cursor = 0.0
    allobjs = []
    for name in row:
        objs = import_glb(os.path.join(CAL, name), location=(cursor, 0.0, 0.0))
        allobjs += objs
        cursor += 1.6
    for path in arc:
        objs = import_glb(path, location=(cursor, 0.0, 0.0))
        allobjs += objs
        cursor += 3.0
    # a second human gauge at the far end for scale bracketing
    allobjs += import_glb(os.path.join(CAL, "human_gauge_1p8m.glb"),
                          location=(cursor, 0.0, 0.0))
    lo, hi = world_bounds(allobjs)
    add_floor(hi[0] - lo[0])
    place_camera(lo, hi, azimuth_deg=-72.0, elev=0.35, pad=1.1)
    render_to(os.path.join(CAL_PROOFS, "scale_human_gauge.png"))


def render_collision_walkability():
    reset_scene()
    allobjs = []
    allobjs += import_glb(os.path.join(CAL, "collision_compound.glb"),
                          location=(0.0, 0.0, 0.0))
    allobjs += import_glb(os.path.join(ARC, "traversal/stair_straight_3m.glb"),
                          location=(3.5, 0.0, 0.0))
    allobjs += import_glb(os.path.join(ARC, "traversal/stair_landing_2x2m.glb"),
                          location=(3.5, -5.0, 3.0))
    allobjs += import_glb(os.path.join(CAL, "human_gauge_1p8m.glb"),
                          location=(-1.5, 0.0, 0.0))
    lo, hi = world_bounds(allobjs)
    add_floor(max(hi[0] - lo[0], hi[1] - lo[1]))
    # low angle so the brown (non-walkable) compound support shows beneath the
    # green (walkable) cap, and the stair treads read as walkable
    place_camera(lo, hi, azimuth_deg=-52.0, elev=0.16, pad=1.35)
    render_to(os.path.join(CAL_PROOFS, "collision_walkability.png"))


def render_socket_alignment():
    reset_scene()
    allobjs = []
    # calibration receiver + plug, aligned per snap math:
    # plug object pos = receiver_socket_world - plug_socket_offset
    # receiver socket engine (0,0.5,0); plug socket engine (0,0.3,0)
    # In Blender: receiver socket at z=0.5, plug socket at z=0.3.
    recv_pos = (0.0, 0.0, 0.0)
    allobjs += import_glb(os.path.join(CAL, "socket_receiver.glb"),
                          location=recv_pos)
    # align plug so its socket (z=0.3) meets receiver socket (z=0.5)
    allobjs += import_glb(os.path.join(CAL, "socket_plug.glb"),
                          location=(recv_pos[0], recv_pos[1],
                                    recv_pos[2] + 0.5 - 0.3))
    # door frame + closed leaf, hinge plug (leaf origin) meets frame receiver
    # frame receiver Blender (-0.45,0,0); leaf plug at leaf origin (0,0,0)
    frame_pos = (3.0, 0.0, 0.0)
    allobjs += import_glb(os.path.join(ARC, "openings/door_frame_standard.glb"),
                          location=frame_pos)
    allobjs += import_glb(
        os.path.join(ARC, "openings/door_leaf_standard_closed.glb"),
        location=(frame_pos[0] - 0.45, frame_pos[1], frame_pos[2]))
    lo, hi = world_bounds(allobjs)
    add_floor(max(hi[0] - lo[0], hi[1] - lo[1]))
    place_camera(lo, hi, azimuth_deg=-62.0, elev=0.45, pad=1.4)
    render_to(os.path.join(CAL_PROOFS, "socket_alignment.png"))


# Bay placements DERIVED from the committed fixture (engine Y-up) via the
# engine->Blender map P_blender = (ex, -ez, ey), so this Blender render depicts
# the exact same physical scene the engine loads from calibration_bay.
def _b(ex, ey, ez):
    return (ex, -ez, ey)


BAY = [
    (os.path.join(CAL, "grid_1m_10x10.glb"), _b(0, 0, 0)),
    (os.path.join(CAL, "storey_3m.glb"), _b(-2, 0, -2)),
    (os.path.join(ARC, "openings/door_frame_standard.glb"), _b(0, 0, -2)),
    (os.path.join(ARC, "openings/door_leaf_standard_closed.glb"),
     _b(-0.45, 0, -2)),
    (os.path.join(ARC, "traversal/stair_straight_3m.glb"), _b(1, 0, 0)),
    (os.path.join(ARC, "traversal/stair_landing_2x2m.glb"), _b(1, 3, 3)),
    (os.path.join(ARC, "structural/railing_straight_2m.glb"), _b(1, 3.2, 3.9)),
    (os.path.join(ARC, "roof/ridge_cap_straight_4m.glb"), _b(0, 3, 0)),
    (os.path.join(ARC, "roof/ridge_cap_end.glb"), _b(2.2, 3, 0)),
    (os.path.join(CAL, "human_gauge_1p8m.glb"), _b(-1, 0, -1)),
]


def render_assembled_bay():
    """The 4 x 4 m bay use-case, rendered from the committed fixture layout:
    3 m storey gauge, socketed door, full-storey stair + landing resting on the
    stair top, rail on the landing, and the roof ridge cap + end."""
    for tag, azimuth, elev, pad in (("assembled_bay", -58.0, 0.5, 1.05),
                                    ("assembled_bay_side", 30.0, 0.3, 1.05)):
        reset_scene()
        allobjs = []
        for path, loc in BAY:
            allobjs += import_glb(path, location=loc)
        add_floor(12.0)
        lo, hi = world_bounds(allobjs)
        place_camera(lo, hi, azimuth_deg=azimuth, elev=elev, pad=pad)
        render_to(os.path.join(ARC_PROOFS, tag + ".png"))


def main():
    render_gallery()
    render_scale()
    render_collision_walkability()
    render_socket_alignment()
    render_assembled_bay()  # writes assembled_bay.png + assembled_bay_side.png
    print("PROOF RENDERS COMPLETE")


if __name__ == "__main__":
    main()
