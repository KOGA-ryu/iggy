"""ASSET-CAL-1/BLD-1 contract linter (Addendum A + texture checks).

Validates every exported GLB against its declared acceptance-record contract
and mechanically (re)generates the README metrics tables so no count is ever
hand-typed.

Validator path: this box has no Khronos `gltf-validator` binary, so we use
`pygltflib` (structural parse) + explicit structural checks. Tool versions:
  - pygltflib 1.16.5
  - Blender 4.5.9 LTS (exporter, glTF 2.0 GLB, Y-up)
Record kept in the READMEs.

For each GLB the linter:
  1. parses the glTF (pygltflib) and checks it is a single-scene GLB 2.0;
  2. measures world-space bounds (glTF Y-up) and asserts them against the
     declared nominal bounds within 1 mm;
  3. asserts grounded assets have minimum vertical (Y) at 0 within 1 mm;
  4. asserts iggy_category / iggy_collision exist and are legal, iggy_walkable
     appears only on single bounds assets, iggy_collision_part_walkable only on
     compound part nodes, socket nodes carry all three socket keys, and every
     mesh node has identity TRS (socket empties exempt);
  5. asserts the texture contract (ASSET-BLD-1): palette-tiled assets embed
     exactly their tile's base-color PNG (<= 1024 px, material named
     palette_<tile>, TEXCOORD_0 present); untextured assets embed no images;
     the severed missing-texture probe stays severed (dangling image URI);
  6. extracts triangle / primitive / material / node / collision-part /
     walkable-part / socket counts.

Modes:
  (default)  validate all GLBs AND verify each README metrics table matches the
             measured values between its <!-- METRICS-TABLE:BEGIN/END --> markers.
             Exits nonzero on any contract violation or stale table.
  --write    rewrite the README metrics tables in place from measured values.

Run headless as part of the gate:
    python3 assets/creative/calibration/tools/lint_contracts.py
"""

import os
import struct
import sys

from pygltflib import GLTF2

CREATIVE = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 os.pardir, os.pardir))

TOL = 0.001  # 1 mm
LEGAL_COLLISION = {"bounds", "compound_bounds", "none", "convex", "mesh"}
LEGAL_CATEGORIES = {"calibration", "openings", "traversal", "structural", "roof"}
LEGAL_ROLES = {"receiver", "plug"}
MAX_TEXTURE_DIM = 1024
SEVERED_URI = "missing/does_not_exist.png"
# world-space cube projection at >= 1.0 m/tile over assets no larger than
# ~5 m in any axis: |uv| beyond this means a broken meters-per-tile or NaN
MAX_ABS_UV = 8.0

MARK_BEGIN = "<!-- METRICS-TABLE:BEGIN -->"
MARK_END = "<!-- METRICS-TABLE:END -->"


# assetId -> declared contract (nominal bounds in glTF Y-up: X width, Y up,
# Z depth; grounded => min Y == 0; category; collision; aggregate walkable;
# collision parts; walkable parts; sockets; receivers; plugs; texture mode).
# tile: None = untextured (no images); a palette tile name = exactly that
# embedded tile; "uv_marker" = the 512 px calibration checker; "severed" =
# the deliberately dangling missing-texture probe.
def spec(nmin, nmax, category, collision, grounded=True, walkable=False,
         parts=0, wparts=0, sockets=0, receivers=0, plugs=0, tile=None):
    return dict(nmin=nmin, nmax=nmax, category=category, collision=collision,
                grounded=grounded, walkable=walkable, parts=parts, wparts=wparts,
                sockets=sockets, receivers=receivers, plugs=plugs, tile=tile)


CALIBRATION = {
    "calibration/grid_1m_10x10":
        spec((-5.01, 0, -5.01), (5.01, 0.02, 5.01), "calibration", "none"),
    "calibration/human_gauge_1p8m":
        spec((-0.25, 0, -0.15), (0.25, 1.8, 0.15), "calibration", "none"),
    "calibration/door_clearance_0p9x2p1":
        spec((-0.5, 0, -0.025), (0.5, 2.15, 0.025), "calibration", "none"),
    "calibration/storey_3m":
        spec((-0.3, 0, -0.3), (0.3, 3.0, 0.3), "calibration", "none"),
    "calibration/pivot_hinge":
        spec((0.0, 0, -0.025), (0.9, 2.1, 0.025), "calibration", "bounds"),
    "calibration/socket_receiver":
        spec((-0.2, 0, -0.2), (0.2, 0.9, 0.2), "calibration", "none",
             sockets=1, receivers=1),
    "calibration/socket_plug":
        spec((-0.1, 0, -0.2), (0.2, 0.52, 0.1), "calibration", "none",
             sockets=1, plugs=1),
    "calibration/collision_compound":
        spec((-0.7, 0, -0.7), (0.7, 0.7, 0.7), "calibration", "compound_bounds",
             walkable=True, parts=2, wparts=1),
    # ASSET-BLD-1 phase 1 material calibration proofs
    "calibration/material_base_color":
        spec((-1.45, 0, -0.2), (1.45, 0.4, 0.2), "calibration", "none"),
    "calibration/material_texture_uv":
        spec((-0.5, 0, -0.5), (0.5, 1.0, 0.5), "calibration", "none",
             tile="uv_marker"),
    "calibration/material_missing_texture":
        spec((-0.5, 0, -0.5), (0.5, 1.0, 0.5), "calibration", "none",
             tile="severed"),
}

ARCHITECTURE = {
    # --- CAL-1 seed kit (REPLACE contract: bounds pinned since Batch 0) ---
    "architecture/openings/door_frame_standard":
        spec((-0.5, 0, -0.075), (0.5, 2.25, 0.075), "openings",
             "compound_bounds", parts=3, wparts=0, sockets=1, receivers=1,
             tile="oak_timber"),
    "architecture/openings/door_leaf_standard_closed":
        spec((0.0, 0, -0.025), (0.9, 2.1, 0.025), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/openings/door_leaf_standard_open":
        spec((-0.025, 0, 0.0), (0.025, 2.1, 0.9), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/traversal/stair_straight_3m":
        spec((-0.5, 0, -2.0), (0.5, 3.0, 2.0), "traversal", "compound_bounds",
             walkable=True, parts=16, wparts=16, tile="oak_timber"),
    "architecture/traversal/stair_landing_2x2m":
        spec((-1.0, 0, -1.0), (1.0, 0.2, 1.0), "traversal", "bounds",
             walkable=True, tile="oak_plank"),
    "architecture/structural/railing_straight_2m":
        spec((-1.0, 0, -0.04), (1.0, 1.0, 0.04), "structural", "bounds",
             tile="oak_timber"),
    # ridge caps: apex baked at the 30 deg roof-recipe pitch (0.20*tan30)
    "architecture/roof/ridge_cap_straight_4m":
        spec((-2.0, 0, -0.2), (2.0, 0.11547, 0.2), "roof", "bounds",
             tile="shingle_oak"),
    "architecture/roof/ridge_cap_end":
        spec((-0.2, 0, -0.2), (0.2, 0.11547, 0.2), "roof", "bounds",
             tile="shingle_oak"),

    # --- BLD-1 phase 4: wide door family ---
    "architecture/openings/door_frame_wide":
        spec((-0.65, 0, -0.075), (0.65, 2.25, 0.075), "openings",
             "compound_bounds", parts=3, wparts=0, sockets=1, receivers=1,
             tile="oak_timber"),
    "architecture/openings/door_leaf_wide_closed":
        spec((0.0, 0, -0.025), (1.2, 2.1, 0.025), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/openings/door_leaf_wide_open":
        spec((-0.025, 0, 0.0), (0.025, 2.1, 1.2), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),

    # --- BLD-2: double door family (two named receivers, mirrored leaf
    # families) ---
    "architecture/openings/door_frame_double":
        spec((-0.95, 0, -0.075), (0.95, 2.25, 0.075), "openings",
             "compound_bounds", parts=3, wparts=0, sockets=2, receivers=2,
             tile="oak_timber"),
    "architecture/openings/door_leaf_double_left_closed":
        spec((0.0, 0, -0.025), (0.9, 2.1, 0.025), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/openings/door_leaf_double_left_open":
        spec((-0.025, 0, 0.0), (0.025, 2.1, 0.9), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/openings/door_leaf_double_right_closed":
        spec((-0.9, 0, -0.025), (0.0, 2.1, 0.025), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/openings/door_leaf_double_right_open":
        spec((-0.025, 0, 0.0), (0.025, 2.1, 0.9), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),

    # --- BLD-1 phase 4: window family (frame origin: X centered, Y=0 at the
    # clear-opening bottom; the stool spans below it, so not grounded) ---
    "architecture/openings/window_frame_standard":
        spec((-0.48, -0.08, -0.075), (0.48, 1.28, 0.075), "openings",
             "compound_bounds", grounded=False, parts=4, wparts=0, sockets=3,
             receivers=3, tile="oak_timber"),
    "architecture/openings/window_frame_small":
        spec((-0.33, -0.08, -0.075), (0.33, 0.68, 0.075), "openings",
             "compound_bounds", grounded=False, parts=4, wparts=0,
             tile="oak_timber"),
    "architecture/openings/window_shutter_left_closed":
        spec((0.0, 0, -0.02), (0.4, 1.2, 0.02), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/openings/window_shutter_left_open":
        spec((-0.02, 0, 0.0), (0.02, 1.2, 0.4), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/openings/window_shutter_right_closed":
        spec((-0.4, 0, -0.02), (0.0, 1.2, 0.02), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/openings/window_shutter_right_open":
        spec((-0.02, 0, 0.0), (0.02, 1.2, 0.4), "openings", "bounds",
             sockets=1, plugs=1, tile="oak_plank"),
    "architecture/openings/window_frame_wide":
        spec((-0.78, -0.08, -0.075), (0.78, 1.28, 0.075), "openings",
             "compound_bounds", grounded=False, parts=4, wparts=0,
             tile="oak_timber"),
    "architecture/openings/window_frame_tall":
        spec((-0.48, -0.08, -0.075), (0.48, 1.88, 0.075), "openings",
             "compound_bounds", grounded=False, parts=4, wparts=0,
             tile="oak_timber"),
    "architecture/openings/window_bars_standard":
        spec((-0.4, 0, -0.02), (0.4, 1.2, 0.02), "openings", "bounds",
             sockets=1, plugs=1, tile="iron_forged"),
    "architecture/openings/window_mullion_cross":
        spec((-0.4, 0, -0.025), (0.4, 1.2, 0.025), "openings", "none",
             sockets=1, plugs=1, tile="oak_timber"),
    "architecture/openings/window_sill_standard":
        spec((-0.55, 0, -0.125), (0.55, 0.1, 0.125), "openings", "none",
             tile="stone_rough"),
    "architecture/openings/window_lintel_standard":
        spec((-0.6, 0, -0.1), (0.6, 0.12, 0.1), "openings", "none",
             tile="oak_timber"),

    # --- BLD-1 phase 4: traversal completion (rail collision parts sit
    # outside the 1.0 m walkable tread width; section E law) ---
    "architecture/traversal/stair_straight_3m_with_rails":
        spec((-0.585, 0, -2.0), (0.585, 4.05, 2.0), "traversal",
             "compound_bounds", walkable=True, parts=24, wparts=16,
             tile="oak_timber"),
    "architecture/traversal/stair_rail_slope_3m":
        spec((-0.04, 0, -2.0), (0.04, 3.95, 2.0), "traversal", "bounds",
             tile="oak_timber"),
    "architecture/traversal/stair_newel_post":
        spec((-0.08, 0, -0.08), (0.08, 1.12, 0.08), "traversal", "bounds",
             tile="oak_timber"),

    # --- BLD-2: structural detail kit (section C core) ---
    "architecture/structural/foundation_plinth_straight_2m":
        spec((-1.0, 0, -0.15), (1.0, 0.5, 0.15), "structural", "none",
             tile="stone_rough"),
    "architecture/structural/foundation_plinth_straight_4m":
        spec((-2.0, 0, -0.15), (2.0, 0.5, 0.15), "structural", "none",
             tile="stone_rough"),
    "architecture/structural/foundation_plinth_inner_corner":
        spec((-0.15, 0, -0.5), (0.5, 0.5, 0.15), "structural", "none",
             tile="stone_rough"),
    "architecture/structural/foundation_plinth_outer_corner":
        spec((-0.15, 0, -0.5), (0.5, 0.55, 0.15), "structural", "none",
             tile="stone_rough"),
    "architecture/structural/foundation_plinth_end":
        spec((0.0, 0, -0.15), (0.45, 0.5, 0.15), "structural", "none",
             tile="stone_rough"),
    "architecture/structural/post_square_0p3x3m":
        spec((-0.15, 0, -0.15), (0.15, 3.0, 0.15), "structural", "bounds",
             tile="oak_timber"),
    "architecture/structural/post_square_0p5x3m":
        spec((-0.25, 0, -0.25), (0.25, 3.0, 0.25), "structural", "bounds",
             tile="oak_timber"),
    "architecture/structural/column_round_0p5x3m":
        spec((-0.25, 0, -0.25), (0.25, 3.0, 0.25), "structural", "bounds",
             sockets=2, receivers=1, plugs=1, tile="stone_rough"),
    "architecture/structural/column_round_base":
        spec((-0.3, 0, -0.3), (0.3, 0.25, 0.3), "structural", "none",
             sockets=1, receivers=1, tile="stone_rough"),
    "architecture/structural/column_round_cap":
        spec((-0.3, 0, -0.3), (0.3, 0.25, 0.3), "structural", "none",
             sockets=1, plugs=1, tile="stone_rough"),
    "architecture/structural/beam_2m":
        spec((-1.0, 0, -0.1), (1.0, 0.3, 0.1), "structural", "bounds",
             tile="oak_timber"),
    "architecture/structural/beam_4m":
        spec((-2.0, 0, -0.1), (2.0, 0.3, 0.1), "structural", "bounds",
             tile="oak_timber"),
    "architecture/structural/beam_6m":
        spec((-3.0, 0, -0.125), (3.0, 0.45, 0.125), "structural", "bounds",
             tile="oak_timber"),
    "architecture/structural/beam_end_cap":
        spec((0.0, 0, -0.13), (0.24, 0.36, 0.13), "structural", "none",
             tile="oak_timber"),
    "architecture/structural/brace_left":
        spec((0.0, 0, -0.06), (0.8, 0.85, 0.06), "structural", "bounds",
             tile="oak_timber"),
    "architecture/structural/brace_right":
        spec((-0.8, 0, -0.06), (0.0, 0.85, 0.06), "structural", "bounds",
             tile="oak_timber"),

    # --- BLD-1 phase 4: roof closure ---
    "architecture/roof/ridge_cap_straight_2m":
        spec((-1.0, 0, -0.2), (1.0, 0.11547, 0.2), "roof", "bounds",
             tile="shingle_oak"),
    "architecture/roof/eave_trim_2m":
        spec((-1.0, 0, -0.17), (1.0, 0.3, 0.03), "roof", "none",
             tile="oak_timber"),
    "architecture/roof/eave_trim_4m":
        spec((-2.0, 0, -0.17), (2.0, 0.3, 0.03), "roof", "none",
             tile="oak_timber"),
    "architecture/roof/eave_outer_corner":
        spec((-0.03, 0, -0.6), (0.6, 0.3, 0.03), "roof", "none",
             tile="oak_timber"),
    "architecture/roof/fascia_end":
        spec((-0.05, 0, -0.05), (0.3, 0.3, 0.05), "roof", "none",
             tile="oak_timber"),
    "architecture/roof/gable_cap_4m":
        spec((-2.0, 0, -0.03), (2.0, 1.33470, 0.03), "roof", "bounds",
             tile="oak_timber"),
    "architecture/roof/gutter_straight_2m":
        spec((-1.0, 0, -0.09), (1.0, 0.14, 0.09), "roof", "none",
             tile="oak_plank"),
    "architecture/roof/downspout_3m":
        spec((-0.05, 0, -0.05), (0.05, 3.0, 0.05), "roof", "none",
             tile="iron_forged"),
    "architecture/roof/downspout_outlet":
        spec((-0.05, 0, -0.05), (0.05, 0.5, 0.3), "roof", "none",
             tile="iron_forged"),
    "architecture/roof/chimney_stack_short":
        spec((-0.475, 0, -0.475), (0.475, 2.2, 0.475), "roof", "bounds",
             sockets=1, receivers=1, tile="stone_rough"),
    "architecture/roof/chimney_cap":
        spec((-0.45, 0, -0.45), (0.45, 0.33, 0.45), "roof", "none",
             sockets=1, plugs=1, tile="stone_rough"),
}


class Violation(Exception):
    pass


def world_translation(nodes, parent, idx):
    t = [0.0, 0.0, 0.0]
    cur = idx
    while cur is not None:
        tr = nodes[cur].translation or [0.0, 0.0, 0.0]
        t = [t[i] + tr[i] for i in range(3)]
        cur = parent.get(cur)
    return t


def png_dims(blob, offset, length):
    """PNG IHDR width/height (big-endian at bytes 16..24)."""
    if length < 24 or blob[offset:offset + 8] != b"\x89PNG\r\n\x1a\n":
        return None
    return struct.unpack(">II", blob[offset + 16:offset + 24])


def measure(asset_id, path):
    g = GLTF2().load(path)
    if g.asset is None or (g.asset.version or "").split(".")[0] != "2":
        raise Violation("%s: not glTF 2.0" % asset_id)
    nodes = g.nodes or []
    meshes = g.meshes or []
    parent = {}
    for pi, n in enumerate(nodes):
        for c in (n.children or []):
            parent[c] = pi

    tris = prims = parts = wparts = sockets = receivers = plugs = 0
    prims_with_uv = 0
    gmin = [float("inf")] * 3
    gmax = [float("-inf")] * 3
    categories = set()
    collisions = set()
    single_walkable = False

    # texture facts
    blob = g.binary_blob()
    images = []
    for img in (g.images or []):
        if img.bufferView is not None:
            bv = g.bufferViews[img.bufferView]
            dims = png_dims(blob, bv.byteOffset or 0, bv.byteLength)
            images.append(dict(embedded=True, uri=None, dims=dims))
        else:
            images.append(dict(embedded=False, uri=img.uri, dims=None))
    materials = []
    for m in (g.materials or []):
        pbr = m.pbrMetallicRoughness
        has_bct = pbr is not None and pbr.baseColorTexture is not None
        materials.append(dict(name=m.name or "", has_bct=has_bct))

    for ni, n in enumerate(nodes):
        ex = n.extras or {}
        # extras legality
        if "iggy_category" in ex:
            categories.add(ex["iggy_category"])
            if ex["iggy_category"] not in LEGAL_CATEGORIES:
                raise Violation("%s: illegal iggy_category %r"
                                % (asset_id, ex["iggy_category"]))
        if "iggy_collision" in ex:
            collisions.add(ex["iggy_collision"])
            if ex["iggy_collision"] not in LEGAL_COLLISION:
                raise Violation("%s: illegal iggy_collision %r"
                                % (asset_id, ex["iggy_collision"]))
        if ex.get("iggy_walkable") is True:
            single_walkable = True
            if ex.get("iggy_collision") != "bounds":
                raise Violation(
                    "%s: iggy_walkable only legal on single bounds assets"
                    % asset_id)
        if "iggy_collision_part" in ex:
            if ex.get("iggy_collision_part") != "bounds":
                raise Violation("%s: iggy_collision_part must be 'bounds'"
                                % asset_id)
            if ex.get("iggy_collision") != "compound_bounds":
                raise Violation(
                    "%s: collision part node must be compound_bounds" % asset_id)
            parts += 1
            if ex.get("iggy_collision_part_walkable") is True:
                wparts += 1
        elif "iggy_collision_part_walkable" in ex:
            raise Violation("%s: walkable part flag without part" % asset_id)
        if any(k.startswith("iggy_socket") for k in ex):
            need = {"iggy_socket", "iggy_socket_role", "iggy_socket_compatibility"}
            if not need.issubset(ex):
                raise Violation("%s: socket node missing keys %s"
                                % (asset_id, need - set(ex)))
            role = ex["iggy_socket_role"]
            if role not in LEGAL_ROLES:
                raise Violation("%s: illegal socket role %r" % (asset_id, role))
            # socket empties must be unrotated or yaw-only (glTF Y after
            # export): the snap contract needs up = world up
            if n.rotation is not None and (abs(n.rotation[0]) > 1e-6 or
                                           abs(n.rotation[2]) > 1e-6):
                raise Violation("%s: socket node %r rotated off world up %s"
                                % (asset_id, n.name, n.rotation))
            sockets += 1
            receivers += 1 if role == "receiver" else 0
            plugs += 1 if role == "plug" else 0

        mi = n.mesh
        if mi is None:
            continue
        # every mesh node must have identity TRS (pitch/pose baked into verts);
        # socket empties are exempt and never carry a mesh
        if n.matrix is not None:
            raise Violation("%s: mesh node %r carries a matrix (not identity TRS)"
                            % (asset_id, n.name))
        if n.translation is not None and any(abs(v) > TOL for v in n.translation):
            raise Violation("%s: mesh node %r has non-zero translation %s"
                            % (asset_id, n.name, n.translation))
        if n.rotation is not None and (
                abs(n.rotation[0]) > 1e-6 or abs(n.rotation[1]) > 1e-6 or
                abs(n.rotation[2]) > 1e-6 or abs(abs(n.rotation[3]) - 1.0) > 1e-6):
            raise Violation("%s: mesh node %r has non-identity rotation %s"
                            % (asset_id, n.name, n.rotation))
        if n.scale is not None and any(abs(s - 1.0) > 1e-6 for s in n.scale):
            raise Violation("%s: mesh node %r has non-unit scale %s"
                            % (asset_id, n.name, n.scale))
        wt = world_translation(nodes, parent, ni)
        for p in meshes[mi].primitives:
            prims += 1
            if p.attributes.TEXCOORD_0 is not None:
                prims_with_uv += 1
                # UVs within sane bounds (BLD-1 standing law): decode the
                # float2 accessor and reject NaN/inf or runaway magnitudes
                acc = g.accessors[p.attributes.TEXCOORD_0]
                if acc.componentType != 5126 or acc.type != "VEC2":
                    raise Violation("%s: TEXCOORD_0 is not float VEC2"
                                    % asset_id)
                bv = g.bufferViews[acc.bufferView]
                start = (bv.byteOffset or 0) + (acc.byteOffset or 0)
                stride = bv.byteStride or 8
                for vi in range(acc.count):
                    u, v = struct.unpack_from("<2f", blob,
                                              start + vi * stride)
                    if not (abs(u) <= MAX_ABS_UV and abs(v) <= MAX_ABS_UV):
                        raise Violation(
                            "%s: UV out of sane bounds (%.3f, %.3f)"
                            % (asset_id, u, v))
            pos = g.accessors[p.attributes.POSITION]
            icount = (g.accessors[p.indices].count
                      if p.indices is not None else pos.count)
            tris += icount // 3
            for k in range(3):
                gmin[k] = min(gmin[k], pos.min[k] + wt[k])
                gmax[k] = max(gmax[k], pos.max[k] + wt[k])

    return dict(tris=tris, prims=prims, mats=len(g.materials or []),
                nodes=len(nodes), parts=parts, wparts=wparts, sockets=sockets,
                receivers=receivers, plugs=plugs, gmin=gmin, gmax=gmax,
                categories=categories, collisions=collisions,
                single_walkable=single_walkable, images=images,
                materials=materials, prims_with_uv=prims_with_uv)


def check_textures(asset_id, sp, m):
    tile = sp["tile"]
    if tile is None:
        if m["images"]:
            raise Violation("%s: untextured asset embeds %d image(s)"
                            % (asset_id, len(m["images"])))
        return
    if tile == "severed":
        if len(m["images"]) != 1 or m["images"][0]["embedded"]:
            raise Violation("%s: severed probe must have exactly one "
                            "non-embedded image" % asset_id)
        if m["images"][0]["uri"] != SEVERED_URI:
            raise Violation("%s: severed probe uri %r (expected %r)"
                            % (asset_id, m["images"][0]["uri"], SEVERED_URI))
        if not any(mat["has_bct"] for mat in m["materials"]):
            raise Violation("%s: severed probe lost its baseColorTexture "
                            "reference" % asset_id)
        return
    # palette tile or the uv marker: embedded, sane dims, wired to materials
    if not m["images"]:
        raise Violation("%s: expected an embedded texture" % asset_id)
    for img in m["images"]:
        if not img["embedded"]:
            raise Violation("%s: texture is not embedded" % asset_id)
        if img["dims"] is None:
            raise Violation("%s: embedded texture is not a readable PNG"
                            % asset_id)
        if img["dims"][0] > MAX_TEXTURE_DIM or img["dims"][1] > MAX_TEXTURE_DIM:
            raise Violation("%s: texture %dx%d exceeds %d px"
                            % (asset_id, img["dims"][0], img["dims"][1],
                               MAX_TEXTURE_DIM))
    for mat in m["materials"]:
        if not mat["has_bct"]:
            raise Violation("%s: material %r has no baseColorTexture"
                            % (asset_id, mat["name"]))
        if tile != "uv_marker" and mat["name"] != "palette_" + tile:
            raise Violation("%s: material %r not traceable to palette tile %r"
                            % (asset_id, mat["name"], tile))
    if m["prims_with_uv"] != m["prims"]:
        raise Violation("%s: %d/%d primitives missing TEXCOORD_0"
                        % (asset_id, m["prims"] - m["prims_with_uv"],
                           m["prims"]))


def check(asset_id, sp, m):
    for k in range(3):
        if abs(m["gmin"][k] - sp["nmin"][k]) > TOL:
            raise Violation("%s: bounds.min[%d]=%.4f nominal %.4f (>1mm)"
                            % (asset_id, k, m["gmin"][k], sp["nmin"][k]))
        if abs(m["gmax"][k] - sp["nmax"][k]) > TOL:
            raise Violation("%s: bounds.max[%d]=%.4f nominal %.4f (>1mm)"
                            % (asset_id, k, m["gmax"][k], sp["nmax"][k]))
    if sp["grounded"] and abs(m["gmin"][1]) > TOL:
        raise Violation("%s: grounded but min Y=%.4f (not 0)"
                        % (asset_id, m["gmin"][1]))
    if m["categories"] != {sp["category"]}:
        raise Violation("%s: categories %s expected {%s}"
                        % (asset_id, m["categories"], sp["category"]))
    if sp["collision"] not in m["collisions"]:
        raise Violation("%s: collision %s missing (saw %s)"
                        % (asset_id, sp["collision"], m["collisions"]))
    for field in ("parts", "wparts", "sockets", "receivers", "plugs"):
        if m[field] != sp[field]:
            raise Violation("%s: %s=%d expected %d"
                            % (asset_id, field, m[field], sp[field]))
    if sp["collision"] == "bounds" and sp["walkable"] and not m["single_walkable"]:
        raise Violation("%s: expected single walkable flag" % asset_id)
    check_textures(asset_id, sp, m)


def fmt_bounds(v):
    return "(%.3f,%.3f,%.3f)" % (v[0], v[1], v[2])


def table_rows(spec_map):
    rows = []
    for asset_id, sp in spec_map.items():
        path = os.path.join(CREATIVE, asset_id + ".glb")
        if not os.path.isfile(path):
            raise Violation("%s: missing GLB %s" % (asset_id, path))
        m = measure(asset_id, path)
        check(asset_id, sp, m)
        walk = (m["wparts"] if m["parts"] else (1 if m["single_walkable"] else 0))
        rows.append(
            "| `%s` | %d | %d | %d | %d | %s..%s | %s | %d | %d | %d | %s |" % (
                asset_id, m["tris"], m["prims"], m["mats"], m["nodes"],
                fmt_bounds(m["gmin"]), fmt_bounds(m["gmax"]),
                sp["collision"], m["parts"], walk, m["sockets"],
                sp["tile"] or "-"))
    return rows


def build_table(spec_map):
    header = ("| assetId | tris | prims | mats | nodes | bounds min..max (m, "
              "glTF Y-up) | collision | parts | walkable | sockets | tile |")
    sep = "|---|--:|--:|--:|--:|---|---|--:|--:|--:|---|"
    return "\n".join([header, sep] + table_rows(spec_map))


def readme_path(spec_map):
    first = next(iter(spec_map))
    kit = first.split("/")[0]
    return os.path.join(CREATIVE, kit, "README.md")


def splice(readme, table):
    with open(readme) as fh:
        text = fh.read()
    if MARK_BEGIN not in text or MARK_END not in text:
        raise Violation("%s: missing METRICS-TABLE markers" % readme)
    pre = text.split(MARK_BEGIN)[0]
    post = text.split(MARK_END)[1]
    return pre + MARK_BEGIN + "\n" + table + "\n" + MARK_END + post


def extract(readme):
    with open(readme) as fh:
        text = fh.read()
    if MARK_BEGIN not in text or MARK_END not in text:
        raise Violation("%s: missing METRICS-TABLE markers" % readme)
    return text.split(MARK_BEGIN)[1].split(MARK_END)[0].strip()


def main():
    write = "--write" in sys.argv
    failures = []
    total = 0
    for spec_map in (CALIBRATION, ARCHITECTURE):
        readme = readme_path(spec_map)
        try:
            table = build_table(spec_map)
            total += len(spec_map)
        except Violation as exc:
            failures.append(str(exc))
            continue
        if write:
            spliced = splice(readme, table)  # read+splice BEFORE truncating
            with open(readme, "w") as fh:
                fh.write(spliced)
            print("WROTE metrics table ->", readme)
        else:
            if not os.path.isfile(readme):
                failures.append("%s: README missing" % readme)
            elif extract(readme) != table.strip():
                failures.append("%s: README metrics table is stale "
                                "(run lint_contracts.py --write)" % readme)
            else:
                print("OK", readme, "(%d assets)" % len(spec_map))
    if failures:
        print("\nCONTRACT LINT FAILED:")
        for f in failures:
            print("  -", f)
        return 1
    print("\nCONTRACT LINT PASSED: %d assets validated against acceptance "
          "contracts." % total)
    return 0


if __name__ == "__main__":
    sys.exit(main())
