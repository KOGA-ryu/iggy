#!/usr/bin/env python3
"""Build World Asset 001: one rough-hewn structural timber master.

Run inside Blender:

    Blender --background --factory-startup \
      --python build_rough_hewn_timber_beam_v1.py

This is a geometry-first build.  It deliberately does not use the existing
wood colour, roughness, or damage systems.  Broad-axe facets, restrained
intrinsic checking, knot sockets, taper, bow, crook, twist, and edge hierarchy
must pass in neutral clay before a material is allowed to improve the read.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
from typing import Any, Iterable

import bpy
import bmesh
from mathutils import Vector
import numpy as np


SCRIPT_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "rough_hewn_timber_beam_v1.json"
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"

MASTER_NAME = "IGGY_WA001_RoughHewnTimberBeam_Master"
CLEAN_SOURCE_NAME = "IGGY_WA001_RoughHewnTimberBeam_CleanSource"
ASSET_COLLECTION_NAME = "IGGY_WA001_RoughHewnTimberBeam"
CUTTER_COLLECTION_NAME = "IGGY_WA001_ConstructionCutters"
REVIEW_COLLECTION_NAME = "IGGY_WA001_ReviewStage"
CLAY_MATERIAL_NAME = "IGGY_MAT_WA001_NeutralClay"
WIRE_MATERIAL_NAME = "IGGY_MAT_WA001_WireProof"
SILHOUETTE_MATERIAL_NAME = "IGGY_MAT_WA001_SilhouetteProof"
GROUND_MATERIAL_NAME = "IGGY_MAT_WA001_ReviewGround"

FACE_FRONT = 0
FACE_BACK = 1
FACE_TOP = 2
FACE_BOTTOM = 3
FACE_LEFT_END = 4
FACE_RIGHT_END = 5
FACE_CHAMFER = 6

FACE_NAME_TO_ID = {
    "front": FACE_FRONT,
    "back": FACE_BACK,
    "top": FACE_TOP,
    "bottom": FACE_BOTTOM,
}


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--render-width", type=int)
    parser.add_argument("--render-height", type=int)
    parser.add_argument("--skip-renders", action="store_true")
    return parser.parse_args(argv)


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def srgb_channel_to_linear(value: float) -> float:
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def srgb_rgba(
    red: int,
    green: int,
    blue: int,
    alpha: float = 1.0,
) -> tuple[float, float, float, float]:
    return (
        srgb_channel_to_linear(red / 255.0),
        srgb_channel_to_linear(green / 255.0),
        srgb_channel_to_linear(blue / 255.0),
        alpha,
    )


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for collection in list(bpy.data.collections):
        if collection.name != "Collection":
            bpy.data.collections.remove(collection)
    for datablocks in (
        bpy.data.meshes,
        bpy.data.curves,
        bpy.data.cameras,
        bpy.data.lights,
        bpy.data.materials,
    ):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def ensure_collection(
    name: str,
    *,
    parent: bpy.types.Collection | None = None,
) -> bpy.types.Collection:
    collection = bpy.data.collections.get(name)
    if collection is None:
        collection = bpy.data.collections.new(name)
    parent = parent or bpy.context.scene.collection
    if collection.name not in {child.name for child in parent.children}:
        parent.children.link(collection)
    return collection


def unlink_from_all_collections(obj: bpy.types.Object) -> None:
    for collection in list(obj.users_collection):
        collection.objects.unlink(obj)


def link_object(
    obj: bpy.types.Object,
    collection: bpy.types.Collection,
) -> None:
    unlink_from_all_collections(obj)
    collection.objects.link(obj)


def smoothstep(value: float) -> float:
    value = max(0.0, min(1.0, value))
    return value * value * (3.0 - 2.0 * value)


def smootherstep(value: float) -> float:
    value = max(0.0, min(1.0, value))
    return value * value * value * (
        value * (value * 6.0 - 15.0) + 10.0
    )


def lerp(start: float, end: float, amount: float) -> float:
    return start + (end - start) * amount


def in_joinery_reserved_zone(
    profile: dict[str, Any],
    x_m: float,
) -> bool:
    return any(
        zone["x_min_m"] <= x_m <= zone["x_max_m"]
        for zone in profile["joinery_reserved_zones"]
    )


def local_section_frame(
    profile: dict[str, Any],
    x_m: float,
) -> dict[str, Any]:
    """Return the measured cross-section and centreline at longitudinal X."""

    dimensions = profile["dimensions_m"]
    form = profile["primary_form"]
    length = dimensions["length"]
    amount = (x_m + length * 0.5) / length
    amount = max(0.0, min(1.0, amount))

    width = lerp(
        dimensions["start_width"],
        dimensions["end_width"],
        amount,
    )
    height = lerp(
        dimensions["start_height"],
        dimensions["end_height"],
        amount,
    )

    # Ends remain joinery-readable.  The middle carries the natural deviation.
    envelope = math.sin(math.pi * amount)
    bow = (
        form["maximum_vertical_bow_m"]
        * envelope
        * (0.82 + 0.18 * math.sin(math.tau * amount + 0.42))
    )
    crook = (
        form["maximum_lateral_crook_m"]
        * envelope
        * math.sin(math.pi * amount + 0.31)
    )
    twist = math.radians(
        form["total_twist_degrees"] * (amount - 0.5)
        + 0.12 * math.sin(math.tau * amount * 1.73 + 0.6)
    )
    return {
        "amount": amount,
        "width": width,
        "height": height,
        "centre_y": crook,
        "centre_z": bow,
        "twist": twist,
    }


def corner_chamfers(
    profile: dict[str, Any],
    x_m: float,
) -> tuple[float, float, float, float]:
    """Four independently varying chamfers: FT, BT, BB, FB."""

    limits = profile["primary_form"]["chamfer_width_m"]
    nominal = limits["nominal"]
    length = profile["dimensions_m"]["length"]
    amount = (x_m + length * 0.5) / length
    waves = (
        math.sin(math.tau * (amount * 1.07 + 0.04)),
        math.sin(math.tau * (amount * 0.83 + 0.31)),
        math.sin(math.tau * (amount * 1.29 + 0.57)),
        math.sin(math.tau * (amount * 0.71 + 0.78)),
    )
    small = (
        math.sin(math.tau * (amount * 3.10 + 0.18)),
        math.sin(math.tau * (amount * 2.70 + 0.46)),
        math.sin(math.tau * (amount * 3.45 + 0.71)),
        math.sin(math.tau * (amount * 2.95 + 0.92)),
    )
    return tuple(
        max(
            limits["minimum"],
            min(
                limits["maximum"],
                nominal + 0.0042 * wave + 0.0014 * detail,
            ),
        )
        for wave, detail in zip(waves, small)
    )


def baseline_plane_drift(
    profile: dict[str, Any],
    face_name: str,
    x_m: float,
    cross_normalized: float,
) -> tuple[float, int]:
    """Interpolate authored target planes and return their semantic pass ID."""

    anchors = profile["hewing"]["plane_passes"][face_name]
    effective_x = x_m + (
        cross_normalized
        * profile["hewing"]["plane_pass_cross_skew_m"][face_name]
    )
    effective_x = max(anchors[0][0], min(anchors[-1][0], effective_x))
    segment = 0
    for index in range(len(anchors) - 1):
        if anchors[index][0] <= effective_x <= anchors[index + 1][0]:
            segment = index
            break
    left = anchors[segment]
    right = anchors[segment + 1]
    amount = (
        0.0
        if right[0] == left[0]
        else (effective_x - left[0]) / (right[0] - left[0])
    )
    depth = lerp(left[1], right[1], amount)
    cross_tilt = lerp(left[2], right[2], amount)
    event_id = 1000 + FACE_NAME_TO_ID[face_name] * 100 + segment
    return max(0.0, depth + cross_normalized * cross_tilt), event_id


def evaluate_hewing_depth(
    profile: dict[str, Any],
    face_name: str,
    x_m: float,
    cross_normalized: float,
    _cross_size_m: float,
) -> tuple[float, int]:
    """Evaluate only the broad dressed planes owned by source geometry.

    The measured local events remain in the profile as the later striation and
    tool-signature lane. Clay comparisons proved that sampling those marks as
    depth fields made soft dents, while Boolean wedges made repeated stamps.
    Neither failure belongs in the accepted master.
    """

    baseline_depth, plane_event = baseline_plane_drift(
        profile,
        face_name,
        x_m,
        cross_normalized,
    )
    depth = (
        profile["hewing"]["baseline_depth_scale"] * baseline_depth
    )
    if in_joinery_reserved_zone(profile, x_m):
        depth *= profile["hewing"]["protected_zone_depth_multiplier"]
    return depth, plane_event


def rotate_cross_section(
    y_m: float,
    z_m: float,
    frame: dict[str, Any],
) -> tuple[float, float]:
    cosine = math.cos(frame["twist"])
    sine = math.sin(frame["twist"])
    y_rotated = y_m * cosine - z_m * sine
    z_rotated = y_m * sine + z_m * cosine
    return (
        y_rotated + frame["centre_y"],
        z_rotated + frame["centre_z"],
    )


def build_ring_samples(
    profile: dict[str, Any],
    x_m: float,
) -> list[dict[str, Any]]:
    """Build 36 perimeter samples: 8 per face plus four chamfer spans."""

    frame = local_section_frame(profile, x_m)
    form = profile["primary_form"]
    width = frame["width"]
    height = frame["height"]
    half_width = width * 0.5
    half_height = height * 0.5
    samples_per_face = form["samples_per_hewn_face"]
    chamfer_ft, chamfer_bt, chamfer_bb, chamfer_fb = corner_chamfers(
        profile,
        x_m,
    )
    lean = form["cross_section_lean_m"]
    samples: list[dict[str, Any]] = []

    def append_sample(
        *,
        face_name: str,
        cross: float,
        y_local: float,
        z_local: float,
        inward_y: float,
        inward_z: float,
        cross_size: float,
    ) -> None:
        depth, event_id = evaluate_hewing_depth(
            profile,
            face_name,
            x_m,
            cross,
            cross_size,
        )
        # A tiny face-wide lean preserves imperfect squareness without
        # replacing the authored facets.
        face_lean = lean[face_name] * cross * 0.5
        if face_name in {"top", "bottom"}:
            z_local += (
                -face_lean if face_name == "top" else face_lean
            )
        else:
            y_local += (
                face_lean if face_name == "front" else -face_lean
            )
        y_local += inward_y * depth
        z_local += inward_z * depth
        y_world, z_world = rotate_cross_section(y_local, z_local, frame)
        samples.append(
            {
                "co": (x_m, y_world, z_world),
                "face_id": FACE_NAME_TO_ID[face_name],
                "hewing_depth": depth,
                "hewing_event_id": event_id,
                "cross": cross,
            }
        )

    # Clockwise when viewed from +X: top front->back, back top->bottom,
    # bottom back->front, front bottom->top.
    for index in range(samples_per_face):
        amount = index / samples_per_face
        y_local = lerp(-half_width + chamfer_ft, half_width - chamfer_bt, amount)
        append_sample(
            face_name="top",
            cross=amount * 2.0 - 1.0,
            y_local=y_local,
            z_local=half_height,
            inward_y=0.0,
            inward_z=-1.0,
            cross_size=width,
        )
    samples.append(
        {
            "co": (
                x_m,
                *rotate_cross_section(
                    half_width - chamfer_bt * 0.28,
                    half_height - chamfer_bt * 0.28,
                    frame,
                ),
            ),
            "face_id": FACE_CHAMFER,
            "hewing_depth": 0.0,
            "hewing_event_id": 0,
            "cross": 0.0,
        }
    )

    for index in range(samples_per_face):
        amount = index / samples_per_face
        z_local = lerp(half_height - chamfer_bt, -half_height + chamfer_bb, amount)
        append_sample(
            face_name="back",
            cross=1.0 - amount * 2.0,
            y_local=half_width,
            z_local=z_local,
            inward_y=-1.0,
            inward_z=0.0,
            cross_size=height,
        )
    samples.append(
        {
            "co": (
                x_m,
                *rotate_cross_section(
                    half_width - chamfer_bb * 0.28,
                    -half_height + chamfer_bb * 0.28,
                    frame,
                ),
            ),
            "face_id": FACE_CHAMFER,
            "hewing_depth": 0.0,
            "hewing_event_id": 0,
            "cross": 0.0,
        }
    )

    for index in range(samples_per_face):
        amount = index / samples_per_face
        y_local = lerp(half_width - chamfer_bb, -half_width + chamfer_fb, amount)
        append_sample(
            face_name="bottom",
            cross=1.0 - amount * 2.0,
            y_local=y_local,
            z_local=-half_height,
            inward_y=0.0,
            inward_z=1.0,
            cross_size=width,
        )
    samples.append(
        {
            "co": (
                x_m,
                *rotate_cross_section(
                    -half_width + chamfer_fb * 0.28,
                    -half_height + chamfer_fb * 0.28,
                    frame,
                ),
            ),
            "face_id": FACE_CHAMFER,
            "hewing_depth": 0.0,
            "hewing_event_id": 0,
            "cross": 0.0,
        }
    )

    for index in range(samples_per_face):
        amount = index / samples_per_face
        z_local = lerp(-half_height + chamfer_fb, half_height - chamfer_ft, amount)
        append_sample(
            face_name="front",
            cross=amount * 2.0 - 1.0,
            y_local=-half_width,
            z_local=z_local,
            inward_y=1.0,
            inward_z=0.0,
            cross_size=height,
        )
    samples.append(
        {
            "co": (
                x_m,
                *rotate_cross_section(
                    -half_width + chamfer_ft * 0.28,
                    half_height - chamfer_ft * 0.28,
                    frame,
                ),
            ),
            "face_id": FACE_CHAMFER,
            "hewing_depth": 0.0,
            "hewing_event_id": 0,
            "cross": 0.0,
        }
    )
    return samples


def write_geometry_attributes(
    mesh: bpy.types.Mesh,
    vertex_u: list[float],
    vertex_hewing_depth: list[float],
    vertex_hewing_event: list[int],
    vertex_end_distance: list[float],
    face_ids: list[int],
    face_joinery: list[float],
    length_m: float,
) -> None:
    attributes = mesh.attributes
    u_attribute = attributes.new(
        "sinc_timber_u_m",
        type="FLOAT",
        domain="POINT",
    )
    depth_attribute = attributes.new(
        "sinc_hewing_depth_m",
        type="FLOAT",
        domain="POINT",
    )
    event_attribute = attributes.new(
        "sinc_hewing_event_id",
        type="INT",
        domain="POINT",
    )
    end_attribute = attributes.new(
        "sinc_end_distance_m",
        type="FLOAT",
        domain="POINT",
    )
    face_attribute = attributes.new(
        "sinc_timber_face_id",
        type="INT",
        domain="FACE",
    )
    joinery_attribute = attributes.new(
        "sinc_joinery_reserved",
        type="FLOAT",
        domain="FACE",
    )

    for item, value in zip(u_attribute.data, vertex_u):
        item.value = value
    for item, value in zip(depth_attribute.data, vertex_hewing_depth):
        item.value = value
    for item, value in zip(event_attribute.data, vertex_hewing_event):
        item.value = value
    for item, value in zip(end_attribute.data, vertex_end_distance):
        item.value = value
    for item, value in zip(face_attribute.data, face_ids):
        item.value = value
    for item, value in zip(joinery_attribute.data, face_joinery):
        item.value = value

    uv_layer = mesh.uv_layers.new(name="IGGY_TimberUV")
    for polygon, face_id in zip(mesh.polygons, face_ids):
        for loop_index in polygon.loop_indices:
            vertex = mesh.vertices[mesh.loops[loop_index].vertex_index].co
            u_m = vertex.x + length_m * 0.5
            if face_id in {FACE_FRONT, FACE_BACK}:
                v_m = vertex.z + 0.5
            elif face_id in {FACE_TOP, FACE_BOTTOM}:
                v_m = vertex.y + 0.5
            elif face_id in {FACE_LEFT_END, FACE_RIGHT_END}:
                u_m = vertex.y + 0.5
                v_m = vertex.z + 0.5
            else:
                v_m = math.atan2(vertex.z, vertex.y) / math.tau + 0.5
            uv_layer.data[loop_index].uv = (u_m, v_m)

    # Smooth the global bow/taper/crook, but preserve the authored cutting
    # boundaries. The sharp lane is based on causal face ownership and
    # significant removal changes, never on a random angle pass.
    adjacent_face_ids: dict[tuple[int, int], list[int]] = {}
    adjacent_face_events: dict[tuple[int, int], list[int]] = {}
    for polygon, face_id in zip(mesh.polygons, face_ids):
        polygon_events = [
            vertex_hewing_event[index]
            for index in polygon.vertices
            if vertex_hewing_event[index] != 0
        ]
        polygon_event = (
            max(set(polygon_events), key=polygon_events.count)
            if polygon_events
            else 0
        )
        for edge_key in polygon.edge_keys:
            key = tuple(sorted(edge_key))
            adjacent_face_ids.setdefault(key, []).append(face_id)
            adjacent_face_events.setdefault(key, []).append(polygon_event)
    sharp_attribute = attributes.new(
        "sharp_edge",
        type="BOOLEAN",
        domain="EDGE",
    )
    for edge, item in zip(mesh.edges, sharp_attribute.data):
        first, second = edge.vertices
        owners = adjacent_face_ids.get(tuple(sorted((first, second))), [])
        neighbouring_events = adjacent_face_events.get(
            tuple(sorted((first, second))),
            [],
        )
        transverse_edge = (
            abs(mesh.vertices[first].co.x - mesh.vertices[second].co.x)
            <= 1.0e-6
        )
        face_boundary = len(set(owners)) > 1 or any(
            owner in {
                FACE_LEFT_END,
                FACE_RIGHT_END,
                FACE_CHAMFER,
            }
            for owner in owners
        )
        plane_transition_boundary = (
            transverse_edge
            and len(set(neighbouring_events)) > 1
            and any(event != 0 for event in neighbouring_events)
        )
        item.value = face_boundary or plane_transition_boundary


def planarize_authored_hewing_regions(
    profile: dict[str, Any],
    vertices: list[tuple[float, float, float]],
    rings: list[list[dict[str, Any]]],
) -> None:
    """Fit every authored hewing region to a real plane.

    A sculptor reaches these shapes with Scrape/Multiplane Scrape and Flatten.
    The script records the same intent explicitly: retain X and the cross-face
    coordinate, solve the face-normal coordinate as a plane, then move each
    vertex toward that plane within a measured correction limit.
    """

    if not rings:
        return
    ring_size = len(rings[0])
    groups: dict[tuple[int, int], list[int]] = {}
    for section, ring in enumerate(rings):
        for ring_index, sample in enumerate(ring):
            face_id = sample["face_id"]
            event_id = sample["hewing_event_id"]
            if face_id not in {
                FACE_FRONT,
                FACE_BACK,
                FACE_TOP,
                FACE_BOTTOM,
            }:
                continue
            groups.setdefault((face_id, event_id), []).append(
                section * ring_size + ring_index
            )

    strength = profile["hewing"]["plane_fit_strength"]
    maximum = profile["hewing"]["plane_fit_maximum_correction_m"]
    for (face_id, _event_id), indices in groups.items():
        if len(indices) < 8:
            continue
        coordinates = np.asarray(
            [vertices[index] for index in indices],
            dtype=np.float64,
        )
        if face_id in {FACE_FRONT, FACE_BACK}:
            design = np.column_stack(
                (
                    coordinates[:, 0],
                    coordinates[:, 2],
                    np.ones(len(coordinates)),
                )
            )
            target = coordinates[:, 1]
            coefficients, *_ = np.linalg.lstsq(
                design,
                target,
                rcond=None,
            )
            fitted = design @ coefficients
            target_axis = 1
        else:
            design = np.column_stack(
                (
                    coordinates[:, 0],
                    coordinates[:, 1],
                    np.ones(len(coordinates)),
                )
            )
            target = coordinates[:, 2]
            coefficients, *_ = np.linalg.lstsq(
                design,
                target,
                rcond=None,
            )
            fitted = design @ coefficients
            target_axis = 2

        for local_index, vertex_index in enumerate(indices):
            correction = float(fitted[local_index] - target[local_index])
            correction = max(-maximum, min(maximum, correction))
            coordinate = list(vertices[vertex_index])
            coordinate[target_axis] += correction * strength
            vertices[vertex_index] = tuple(coordinate)


def build_hewn_volume(
    profile: dict[str, Any],
) -> bpy.types.Mesh:
    """Build the clean semantic volume before intrinsic anatomy modifiers."""

    dimensions = profile["dimensions_m"]
    form = profile["primary_form"]
    length = dimensions["length"]
    section_count = form["longitudinal_sections"]
    rings: list[list[dict[str, Any]]] = []
    vertices: list[tuple[float, float, float]] = []
    vertex_u: list[float] = []
    vertex_depth: list[float] = []
    vertex_event: list[int] = []
    vertex_end_distance: list[float] = []
    faces: list[tuple[int, ...]] = []
    face_ids: list[int] = []
    face_joinery: list[float] = []

    for section in range(section_count):
        amount = section / (section_count - 1)
        x_m = lerp(-length * 0.5, length * 0.5, amount)
        ring = build_ring_samples(profile, x_m)
        rings.append(ring)
        for sample in ring:
            vertices.append(sample["co"])
            vertex_u.append(x_m + length * 0.5)
            vertex_depth.append(sample["hewing_depth"])
            vertex_event.append(sample["hewing_event_id"])
            vertex_end_distance.append(min(x_m + length * 0.5, length * 0.5 - x_m))

    planarize_authored_hewing_regions(profile, vertices, rings)

    ring_size = len(rings[0])
    for section in range(section_count - 1):
        current = section * ring_size
        following = (section + 1) * ring_size
        x_mid = (
            vertices[current][0] + vertices[following][0]
        ) * 0.5
        reserved = 1.0 if in_joinery_reserved_zone(profile, x_mid) else 0.0
        for index in range(ring_size):
            next_index = (index + 1) % ring_size
            faces.append(
                (
                    current + index,
                    following + index,
                    following + next_index,
                    current + next_index,
                )
            )
            first_id = rings[section][index]["face_id"]
            second_id = rings[section][next_index]["face_id"]
            face_ids.append(
                first_id if first_id == second_id else FACE_CHAMFER
            )
            face_joinery.append(reserved)

    left_centre_index = len(vertices)
    left_frame = local_section_frame(profile, -length * 0.5)
    vertices.append(
        (
            -length * 0.5,
            left_frame["centre_y"],
            left_frame["centre_z"],
        )
    )
    vertex_u.append(0.0)
    vertex_depth.append(0.0)
    vertex_event.append(0)
    vertex_end_distance.append(0.0)

    right_centre_index = len(vertices)
    right_frame = local_section_frame(profile, length * 0.5)
    vertices.append(
        (
            length * 0.5,
            right_frame["centre_y"],
            right_frame["centre_z"],
        )
    )
    vertex_u.append(length)
    vertex_depth.append(0.0)
    vertex_event.append(0)
    vertex_end_distance.append(0.0)

    right_offset = (section_count - 1) * ring_size
    for index in range(ring_size):
        next_index = (index + 1) % ring_size
        faces.append((left_centre_index, index, next_index))
        face_ids.append(FACE_LEFT_END)
        face_joinery.append(1.0)
        faces.append(
            (
                right_centre_index,
                right_offset + next_index,
                right_offset + index,
            )
        )
        face_ids.append(FACE_RIGHT_END)
        face_joinery.append(1.0)

    mesh = bpy.data.meshes.new("IGGY_WA001_RoughHewnTimberBeam_SourceMesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update(calc_edges=True)
    write_geometry_attributes(
        mesh,
        vertex_u,
        vertex_depth,
        vertex_event,
        vertex_end_distance,
        face_ids,
        face_joinery,
        length,
    )
    # The source strip carries slow bow, taper, crook, and target-plane drift.
    # Keep that carrier smooth.  Literal axe facets are imposed later by
    # measured Boolean wedges and retain their own geometric plane breaks.
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    mesh.validate(verbose=True)
    return mesh


def create_prism_mesh(
    name: str,
    start_points: Iterable[Vector],
    end_points: Iterable[Vector],
) -> bpy.types.Mesh:
    start = list(start_points)
    end = list(end_points)
    if len(start) != len(end) or len(start) < 3:
        raise ValueError("Prism needs matching start/end loops of at least 3 points")
    count = len(start)
    vertices = [tuple(point) for point in start + end]
    faces: list[tuple[int, ...]] = []
    faces.append(tuple(reversed(range(count))))
    faces.append(tuple(range(count, count * 2)))
    for index in range(count):
        following = (index + 1) % count
        faces.append(
            (
                index,
                following,
                count + following,
                count + index,
            )
        )
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update(calc_edges=True)
    topology = bmesh.new()
    topology.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(topology, faces=list(topology.faces))
    topology.to_mesh(mesh)
    topology.free()
    mesh.validate(verbose=True)
    return mesh


def create_end_check_cutter(
    profile: dict[str, Any],
    check: dict[str, Any],
    cutter_collection: bpy.types.Collection,
) -> bpy.types.Object:
    """Create one closed, tapered radial seasoning-check cutter."""

    dimensions = profile["dimensions_m"]
    length = dimensions["length"]
    left = check["end"] == "left"
    end_x = -length * 0.5 if left else length * 0.5
    outside_x = end_x + (-0.014 if left else 0.014)
    inside_x = end_x + (check["depth_m"] if left else -check["depth_m"])
    angle = math.radians(check["angle_degrees"])
    radial = Vector((0.0, math.cos(angle), math.sin(angle)))
    side = Vector((0.0, -math.sin(angle), math.cos(angle)))
    radial_start = check["radial_start_m"]
    radial_end = check["radial_end_m"]
    mouth = check["mouth_width_m"]

    # A check follows wood, not a ruler. The path uses explicit bounded
    # offsets and a varying aperture. The terminal point is narrow rather than
    # a perfect triangular spear.
    phases = {
        "left_check_a": 0.20,
        "left_check_b": 1.10,
        "left_check_c": 2.05,
        "right_check_a": 2.80,
        "right_check_b": 3.70,
    }
    phase = phases[check["id"]]
    amounts = (0.0, 0.16, 0.34, 0.53, 0.71, 0.87, 1.0)
    width_factors = (0.42, 1.0, 0.73, 0.88, 0.54, 0.32, 0.10)

    def ribbon_loop(
        x_m: float,
        *,
        reach_multiplier: float,
        width_multiplier: float,
        wobble_multiplier: float,
    ) -> list[Vector]:
        left_edge: list[Vector] = []
        right_edge: list[Vector] = []
        reach = (radial_end - radial_start) * reach_multiplier
        for index, (amount, width_factor) in enumerate(
            zip(amounts, width_factors)
        ):
            wobble = (
                mouth
                * wobble_multiplier
                * math.sin(math.pi * amount)
                * (
                    0.48 * math.sin(amount * math.tau * 1.4 + phase)
                    + 0.22 * math.sin(amount * math.tau * 3.1 + phase * 0.7)
                )
            )
            centreline = (
                Vector((x_m, 0.0, 0.0))
                + radial * (radial_start + reach * amount)
                + side * wobble
            )
            half_aperture = max(
                0.00022,
                mouth * width_factor * width_multiplier * 0.5,
            )
            # One edge is allowed to carry a slightly different rhythm. That
            # prevents the two walls from reading as a copied ribbon.
            asymmetry = 1.0 + 0.10 * math.sin(index * 2.1 + phase)
            left_edge.append(centreline - side * half_aperture)
            right_edge.append(
                centreline + side * half_aperture * asymmetry
            )
        return left_edge + list(reversed(right_edge))

    start_loop = ribbon_loop(
        outside_x,
        reach_multiplier=1.0,
        width_multiplier=1.0,
        wobble_multiplier=1.0,
    )
    end_loop = ribbon_loop(
        inside_x,
        reach_multiplier=0.76,
        width_multiplier=0.14,
        wobble_multiplier=0.42,
    )
    mesh = create_prism_mesh(
        f"IGGY_CUT_{check['id']}_Mesh",
        start_loop,
        end_loop,
    )
    obj = bpy.data.objects.new(f"IGGY_CUT_{check['id']}", mesh)
    cutter_collection.objects.link(obj)
    obj.display_type = "WIRE"
    obj.hide_render = True
    obj["iggy_cutter_role"] = "intrinsic_end_check"
    obj["iggy_end"] = check["end"]
    return obj


def face_surface_frame(
    profile: dict[str, Any],
    face_name: str,
    x_m: float,
    cross_m: float,
) -> tuple[Vector, Vector, Vector, Vector]:
    """Return centre, longitudinal, cross-face, and outward normal vectors."""

    frame = local_section_frame(profile, x_m)
    cosine = math.cos(frame["twist"])
    sine = math.sin(frame["twist"])
    local_y = Vector((0.0, cosine, sine))
    local_z = Vector((0.0, -sine, cosine))
    tangent = Vector((1.0, 0.0, 0.0))
    centre = Vector((x_m, frame["centre_y"], frame["centre_z"]))
    if face_name == "front":
        normal = -local_y
        cross = local_z
        centre += normal * (frame["width"] * 0.5)
        centre += cross * cross_m
    elif face_name == "back":
        normal = local_y
        cross = local_z
        centre += normal * (frame["width"] * 0.5)
        centre += cross * cross_m
    elif face_name == "top":
        normal = local_z
        cross = local_y
        centre += normal * (frame["height"] * 0.5)
        centre += cross * cross_m
    else:
        normal = -local_z
        cross = local_y
        centre += normal * (frame["height"] * 0.5)
        centre += cross * cross_m
    return centre, tangent, cross, normal


def ellipse_loop(
    centre: Vector,
    tangent: Vector,
    cross: Vector,
    major: float,
    minor: float,
    segments: int,
) -> list[Vector]:
    points = []
    for index in range(segments):
        angle = math.tau * index / segments
        irregularity = (
            1.0
            + 0.075 * math.sin(angle * 3.0 + 0.4)
            + 0.035 * math.sin(angle * 5.0 + 1.2)
        )
        points.append(
            centre
            + tangent * (math.cos(angle) * major * irregularity)
            + cross * (math.sin(angle) * minor * irregularity)
        )
    return points


def create_knot_socket(
    profile: dict[str, Any],
    knot: dict[str, Any],
    cutter_collection: bpy.types.Collection,
    asset_collection: bpy.types.Collection,
    material: bpy.types.Material,
) -> tuple[bpy.types.Object, bpy.types.Object]:
    """Create an elliptical socket cutter and a recessed knot-body insert."""

    centre, tangent, cross, normal = face_surface_frame(
        profile,
        knot["face"],
        knot["x_m"],
        knot["cross_m"],
    )
    segments = 28
    outside = centre + normal * 0.014
    inside = centre - normal * (knot["recess_m"] + 0.014)
    outer_loop = ellipse_loop(
        outside,
        tangent,
        cross,
        knot["major_radius_m"] * 1.06,
        knot["minor_radius_m"] * 1.06,
        segments,
    )
    inner_loop = ellipse_loop(
        inside,
        tangent,
        cross,
        knot["major_radius_m"] * 0.90,
        knot["minor_radius_m"] * 0.90,
        segments,
    )
    cutter_mesh = create_prism_mesh(
        f"IGGY_CUT_{knot['id']}_Mesh",
        outer_loop,
        inner_loop,
    )
    cutter = bpy.data.objects.new(f"IGGY_CUT_{knot['id']}", cutter_mesh)
    cutter_collection.objects.link(cutter)
    cutter.display_type = "WIRE"
    cutter.hide_render = True
    cutter["iggy_cutter_role"] = "knot_socket"

    plug_surface = centre - normal * knot["recess_m"]
    plug_back = plug_surface - normal * 0.009
    plug_outer = ellipse_loop(
        plug_surface,
        tangent,
        cross,
        knot["major_radius_m"] * 0.87,
        knot["minor_radius_m"] * 0.87,
        segments,
    )
    plug_inner = ellipse_loop(
        plug_back,
        tangent,
        cross,
        knot["major_radius_m"] * 0.76,
        knot["minor_radius_m"] * 0.76,
        segments,
    )
    plug_mesh = create_prism_mesh(
        f"IGGY_GEO_{knot['id']}_Mesh",
        plug_outer,
        plug_inner,
    )
    plug = bpy.data.objects.new(f"IGGY_GEO_{knot['id']}", plug_mesh)
    asset_collection.objects.link(plug)
    plug.data.materials.append(material)
    plug.parent = None
    plug["iggy_anatomy_role"] = "recessed_knot_body"
    bevel = plug.modifiers.new("IGGY_KnotEdgeSoftening", "BEVEL")
    bevel.width = 0.0013
    bevel.segments = 2
    return cutter, plug


def create_edge_chip_cutter(
    profile: dict[str, Any],
    chip: dict[str, Any],
    cutter_collection: bpy.types.Collection,
) -> bpy.types.Object:
    frame = local_section_frame(profile, chip["x_m"])
    half_width = frame["width"] * 0.5
    half_height = frame["height"] * 0.5
    front = "front" in chip["corner"]
    top = "top" in chip["corner"]
    y_sign = -1.0 if front else 1.0
    z_sign = 1.0 if top else -1.0
    centre = Vector(
        (
            chip["x_m"],
            frame["centre_y"] + y_sign * half_width,
            frame["centre_z"] + z_sign * half_height,
        )
    )
    half_length = chip["length_m"] * 0.5
    width = chip["width_m"]
    depth = chip["depth_m"]
    local_points = (
        (-half_length, y_sign * width * 1.15, z_sign * width * 0.42),
        (half_length, y_sign * width * 0.56, z_sign * width * 1.08),
        (-half_length * 0.84, -y_sign * depth, z_sign * width * 0.78),
        (half_length * 0.72, y_sign * width * 0.68, -z_sign * depth),
        (0.0, -y_sign * depth, -z_sign * depth),
        (-half_length * 0.18, y_sign * width * 1.22, z_sign * width * 1.16),
        (half_length * 0.24, -y_sign * depth * 0.72, z_sign * width * 0.18),
    )
    mesh = bpy.data.meshes.new(f"IGGY_CUT_{chip['id']}_Mesh")
    hull = bmesh.new()
    hull_vertices = [
        hull.verts.new(centre + Vector(point))
        for point in local_points
    ]
    bmesh.ops.convex_hull(
        hull,
        input=hull_vertices,
        use_existing_faces=False,
    )
    hull.to_mesh(mesh)
    hull.free()
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(f"IGGY_CUT_{chip['id']}", mesh)
    cutter_collection.objects.link(obj)
    obj.display_type = "WIRE"
    obj.hide_render = True
    obj["iggy_cutter_role"] = "restrained_edge_loss"
    return obj


def create_principled_material(
    name: str,
    base_color: tuple[float, float, float, float],
    *,
    roughness: float,
    metallic: float = 0.0,
) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    material.diffuse_color = base_color
    tree = material.node_tree
    principled = tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = base_color
    principled.inputs["Roughness"].default_value = roughness
    principled.inputs["Metallic"].default_value = metallic
    return material


def create_wire_material() -> bpy.types.Material:
    material = bpy.data.materials.new(WIRE_MATERIAL_NAME)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    shader = tree.nodes.new("ShaderNodeBsdfPrincipled")
    wire = tree.nodes.new("ShaderNodeWireframe")
    ramp = tree.nodes.new("ShaderNodeValToRGB")
    wire.inputs["Size"].default_value = 0.00075
    ramp.color_ramp.elements[0].position = 0.22
    ramp.color_ramp.elements[0].color = srgb_rgba(28, 32, 35)
    ramp.color_ramp.elements[1].position = 0.36
    ramp.color_ramp.elements[1].color = srgb_rgba(186, 190, 184)
    shader.inputs["Roughness"].default_value = 0.78
    tree.links.new(wire.outputs["Fac"], ramp.inputs["Fac"])
    tree.links.new(ramp.outputs["Color"], shader.inputs["Base Color"])
    tree.links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def attach_boolean_modifier(
    master: bpy.types.Object,
    cutter: bpy.types.Object,
    index: int,
) -> bpy.types.BooleanModifier:
    modifier = master.modifiers.new(
        f"IGGY_{index:02d}_{cutter.name.removeprefix('IGGY_CUT_')}",
        "BOOLEAN",
    )
    modifier.operation = "DIFFERENCE"
    modifier.solver = "EXACT"
    modifier.object = cutter
    modifier.show_viewport = True
    modifier.show_render = True
    return modifier


def create_beam_master(
    profile: dict[str, Any],
    asset_collection: bpy.types.Collection,
    cutter_collection: bpy.types.Collection,
    clay: bpy.types.Material,
) -> tuple[bpy.types.Object, bpy.types.Object, list[bpy.types.Object]]:
    mesh = build_hewn_volume(profile)
    clean = bpy.data.objects.new(CLEAN_SOURCE_NAME, mesh)
    asset_collection.objects.link(clean)
    clean.hide_render = True
    clean.hide_viewport = True
    clean["iggy_role"] = "editable_clean_semantic_source"

    master_mesh = mesh.copy()
    master_mesh.name = "IGGY_WA001_RoughHewnTimberBeam_MasterMesh"
    master = bpy.data.objects.new(MASTER_NAME, master_mesh)
    asset_collection.objects.link(master)
    master.data.materials.append(clay)
    master["iggy_asset_schema"] = profile["schema"]
    master["iggy_asset_id"] = profile["asset_id"]
    master["iggy_joinery_reserved_zones"] = json.dumps(
        profile["joinery_reserved_zones"],
        sort_keys=True,
    )
    master["iggy_knot_count"] = len(profile["anatomy"]["knots"])
    master["iggy_end_check_count"] = len(profile["anatomy"]["end_checks"])
    master["iggy_edge_chip_count"] = len(profile["anatomy"]["edge_chips"])
    master["iggy_ai_generated_imagery"] = False
    master["iggy_primary_relief_owner"] = "authored_geometry"
    master["iggy_material_status"] = "neutral_clay_only"

    cutters: list[bpy.types.Object] = []
    knot_bodies: list[bpy.types.Object] = []
    for check in profile["anatomy"]["end_checks"]:
        cutters.append(
            create_end_check_cutter(profile, check, cutter_collection)
        )
    for knot in profile["anatomy"]["knots"]:
        cutter, body = create_knot_socket(
            profile,
            knot,
            cutter_collection,
            asset_collection,
            clay,
        )
        cutters.append(cutter)
        knot_bodies.append(body)
    for chip in profile["anatomy"]["edge_chips"]:
        cutters.append(
            create_edge_chip_cutter(profile, chip, cutter_collection)
        )

    for index, cutter in enumerate(cutters, start=1):
        attach_boolean_modifier(master, cutter, index)

    bevel = master.modifiers.new(
        f"IGGY_{len(cutters) + 1:02d}_SelectiveMicroBevel",
        "BEVEL",
    )
    bevel.limit_method = "ANGLE"
    bevel.angle_limit = math.radians(24.0)
    # The 6-19 mm arris hierarchy already exists in source geometry.  This
    # sub-millimetre bevel is only an anti-aliasing glint for Boolean anatomy;
    # a larger value outlines shallow axe facets like decorative inset panels.
    bevel.width = 0.00045
    bevel.segments = 2
    bevel.harden_normals = True
    bevel.affect = "EDGES"

    weighted = master.modifiers.new(
        f"IGGY_{len(cutters) + 2:02d}_PreserveBroadPlanes",
        "WEIGHTED_NORMAL",
    )
    weighted.keep_sharp = True
    weighted.weight = 74
    weighted.mode = "FACE_AREA_WITH_ANGLE"
    weighted.use_face_influence = True

    for body in knot_bodies:
        body.parent = master
        body.matrix_parent_inverse = master.matrix_world.inverted()
    return master, clean, knot_bodies


def aim_object(
    obj: bpy.types.Object,
    target: Vector,
    *,
    track: str = "-Z",
    up: str = "Y",
) -> None:
    direction = target - obj.location
    obj.rotation_euler = direction.to_track_quat(track, up).to_euler()


def create_area_light(
    name: str,
    location: tuple[float, float, float],
    energy: float,
    size: float,
    color: tuple[float, float, float],
    collection: bpy.types.Collection,
    target: Vector,
) -> bpy.types.Object:
    data = bpy.data.lights.new(name, "AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = color
    obj = bpy.data.objects.new(name, data)
    collection.objects.link(obj)
    obj.location = location
    aim_object(obj, target)
    return obj


def create_review_stage(
    width: int,
    height: int,
    review_collection: bpy.types.Collection,
) -> dict[str, bpy.types.Object | bpy.types.Material]:
    scene = bpy.context.scene
    # Blender 5.1 exposes the Eevee Next renderer under the historical
    # BLENDER_EEVEE enum value.
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.film_transparent = False
    scene.render.image_settings.color_depth = "8"
    scene.render.image_settings.compression = 28
    scene.render.film_transparent = False
    scene.view_settings.look = "AgX - Medium High Contrast"

    world = bpy.data.worlds.new("IGGY_WA001_NeutralWorld")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (
        0.055,
        0.060,
        0.066,
        1.0,
    )
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.33
    scene.world = world

    camera_data = bpy.data.cameras.new("IGGY_WA001_ReviewCamera")
    camera = bpy.data.objects.new("IGGY_WA001_ReviewCamera", camera_data)
    review_collection.objects.link(camera)
    camera_data.lens = 58.0
    camera_data.sensor_width = 36.0
    scene.camera = camera

    ground_material = create_principled_material(
        GROUND_MATERIAL_NAME,
        srgb_rgba(56, 60, 63),
        roughness=0.92,
    )
    bpy.ops.mesh.primitive_plane_add(size=16.0, location=(0.0, 0.0, -0.215))
    ground = bpy.context.object
    ground.name = "IGGY_WA001_ReviewGround"
    link_object(ground, review_collection)
    ground.data.materials.append(ground_material)

    target = Vector((0.0, 0.0, 0.03))
    key = create_area_light(
        "IGGY_WA001_Key",
        (-1.2, -3.5, 4.8),
        1050.0,
        4.0,
        (1.0, 0.84, 0.69),
        review_collection,
        target,
    )
    fill = create_area_light(
        "IGGY_WA001_Fill",
        (2.8, 3.4, 2.1),
        570.0,
        3.2,
        (0.64, 0.78, 1.0),
        review_collection,
        target,
    )
    rim = create_area_light(
        "IGGY_WA001_Rim",
        (-3.0, 1.7, 3.2),
        760.0,
        2.2,
        (0.80, 0.88, 1.0),
        review_collection,
        target,
    )
    return {
        "camera": camera,
        "ground": ground,
        "key": key,
        "fill": fill,
        "rim": rim,
        "ground_material": ground_material,
    }


def set_camera(
    camera: bpy.types.Object,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    *,
    lens: float = 58.0,
    orthographic_scale: float | None = None,
) -> None:
    camera.location = location
    aim_object(camera, Vector(target))
    if orthographic_scale is None:
        camera.data.type = "PERSP"
        camera.data.lens = lens
    else:
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = orthographic_scale


def replace_material(
    objects: Iterable[bpy.types.Object],
    material: bpy.types.Material,
) -> list[list[bpy.types.Material]]:
    original: list[list[bpy.types.Material]] = []
    for obj in objects:
        original.append([slot.material for slot in obj.material_slots])
        obj.data.materials.clear()
        obj.data.materials.append(material)
    return original


def restore_materials(
    objects: Iterable[bpy.types.Object],
    original: list[list[bpy.types.Material]],
) -> None:
    for obj, materials in zip(objects, original):
        obj.data.materials.clear()
        for material in materials:
            if material is not None:
                obj.data.materials.append(material)


def render_proof(
    scene: bpy.types.Scene,
    output_path: Path,
) -> dict[str, Any]:
    scene.render.filepath = str(output_path)
    bpy.ops.render.render(write_still=True)
    image = bpy.data.images.get("Render Result")
    return {
        "path": str(output_path.resolve()),
        "width": int(scene.render.resolution_x),
        "height": int(scene.render.resolution_y),
        "file_size_bytes": output_path.stat().st_size,
        "render_result_size": (
            [int(image.size[0]), int(image.size[1])] if image else None
        ),
    }


def create_review_package(
    profile: dict[str, Any],
    output_root: Path,
    master: bpy.types.Object,
    knot_bodies: list[bpy.types.Object],
    stage: dict[str, bpy.types.Object | bpy.types.Material],
    clay: bpy.types.Material,
    wire: bpy.types.Material,
    silhouette: bpy.types.Material,
    *,
    skip_renders: bool,
) -> dict[str, dict[str, Any]]:
    if skip_renders:
        return {}

    scene = bpy.context.scene
    camera = stage["camera"]
    ground = stage["ground"]
    key = stage["key"]
    fill = stage["fill"]
    rim = stage["rim"]
    render_objects = [master, *knot_bodies]
    proofs: dict[str, dict[str, Any]] = {}
    render_dir = output_root / "renders"
    render_dir.mkdir(parents=True, exist_ok=True)

    specifications = {
        "front": ((0.0, -5.7, 0.15), (0.0, 0.0, 0.02), 5.0),
        "back": ((0.0, 5.7, 0.15), (0.0, 0.0, 0.02), 5.0),
        "top": ((0.0, 0.0, 5.7), (0.0, 0.0, 0.0), 5.0),
        "bottom": ((0.0, 0.0, -5.7), (0.0, 0.0, 0.0), 5.0),
        "end_left": ((-3.25, -0.15, 0.18), (-2.06, 0.0, 0.0), 0.74),
        "end_right": ((3.25, 0.15, 0.16), (2.06, 0.0, 0.0), 0.74),
    }
    for name, (location, target, scale) in specifications.items():
        ground.hide_render = name in {"top", "bottom", "end_left", "end_right"}
        set_camera(
            camera,
            location,
            target,
            orthographic_scale=scale,
        )
        proofs[name] = render_proof(
            scene,
            render_dir / f"rough_hewn_timber_beam_v1_{name}.png",
        )

    ground.hide_render = False
    for name, location, target, lens in (
        ("perspective_a", (4.0, -4.2, 2.65), (0.15, 0.0, 0.0), 61.0),
        ("perspective_b", (-3.85, 3.8, 2.1), (-0.1, 0.0, 0.0), 64.0),
    ):
        set_camera(camera, location, target, lens=lens)
        proofs[name] = render_proof(
            scene,
            render_dir / f"rough_hewn_timber_beam_v1_{name}.png",
        )

    original = replace_material(render_objects, wire)
    ground.hide_render = True
    set_camera(
        camera,
        (3.7, -4.1, 2.5),
        (0.0, 0.0, 0.0),
        lens=60.0,
    )
    proofs["wireframe"] = render_proof(
        scene,
        render_dir / "rough_hewn_timber_beam_v1_wireframe.png",
    )
    restore_materials(render_objects, original)

    original = replace_material(render_objects, silhouette)
    world_background = scene.world.node_tree.nodes["Background"]
    old_world_color = tuple(world_background.inputs["Color"].default_value)
    old_world_strength = world_background.inputs["Strength"].default_value
    world_background.inputs["Color"].default_value = (0.72, 0.72, 0.72, 1.0)
    world_background.inputs["Strength"].default_value = 0.8
    for light in (key, fill, rim):
        light.hide_render = True
    set_camera(
        camera,
        (3.7, -4.0, 1.8),
        (0.0, 0.0, 0.0),
        lens=68.0,
    )
    proofs["clay_silhouette"] = render_proof(
        scene,
        render_dir / "rough_hewn_timber_beam_v1_clay_silhouette.png",
    )
    restore_materials(render_objects, original)
    world_background.inputs["Color"].default_value = old_world_color
    world_background.inputs["Strength"].default_value = old_world_strength
    for light in (key, fill, rim):
        light.hide_render = False

    ground.hide_render = True
    key.location = (-3.55, -1.15, 0.44)
    key.data.energy = 360.0
    key.data.size = 0.62
    aim_object(key, Vector((0.15, -0.14, -0.02)))
    fill.data.energy = 110.0
    rim.data.energy = 220.0
    set_camera(
        camera,
        (0.20, -3.55, 0.86),
        (0.15, -0.14, -0.01),
        lens=58.0,
    )
    proofs["toolmark_grazing"] = render_proof(
        scene,
        render_dir / "rough_hewn_timber_beam_v1_toolmark_grazing.png",
    )

    key.location = (-3.1, -1.65, 2.1)
    key.data.energy = 1080.0
    key.data.size = 1.8
    aim_object(key, Vector((-2.05, 0.0, 0.0)))
    fill.data.energy = 390.0
    rim.data.energy = 520.0
    set_camera(
        camera,
        (-3.02, -1.34, 0.58),
        (-2.05, 0.0, 0.0),
        lens=77.0,
    )
    proofs["end_checks_closeup"] = render_proof(
        scene,
        render_dir / "rough_hewn_timber_beam_v1_end_checks_closeup.png",
    )

    ground.hide_render = False
    master.data.materials.clear()
    master.data.materials.append(clay)
    return proofs


def validate_evaluated_master(
    profile: dict[str, Any],
    master: bpy.types.Object,
    clean: bpy.types.Object,
) -> dict[str, Any]:
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = master.evaluated_get(depsgraph)
    mesh = bpy.data.meshes.new_from_object(evaluated)
    dimensions = [float(value) for value in master.dimensions]
    expected = profile["dimensions_m"]
    if abs(dimensions[0] - expected["length"]) > 0.035:
        raise RuntimeError(f"Beam length drifted: {dimensions}")
    if len(master.data.vertices) <= 3000 or len(master.data.polygons) <= 3000:
        raise RuntimeError("The authored hewn source is under-resolved")
    if len(mesh.vertices) <= 4000 or len(mesh.polygons) <= 4000:
        progression = []
        modifier_states = [modifier.show_viewport for modifier in master.modifiers]
        for modifier in master.modifiers:
            modifier.show_viewport = False
        bpy.context.view_layer.update()
        for modifier in master.modifiers:
            modifier.show_viewport = True
            bpy.context.view_layer.update()
            step_evaluated = master.evaluated_get(depsgraph)
            step_mesh = bpy.data.meshes.new_from_object(step_evaluated)
            progression.append(
                (modifier.name, len(step_mesh.vertices), len(step_mesh.polygons))
            )
            bpy.data.meshes.remove(step_mesh)
        for modifier, state in zip(master.modifiers, modifier_states):
            modifier.show_viewport = state
        bpy.context.view_layer.update()
        raise RuntimeError(
            "The evaluated beam did not retain its anatomy detail: "
            f"{len(mesh.vertices)} vertices, {len(mesh.polygons)} polygons; "
            f"progression={progression}"
        )
    missing_attributes = {
        "sinc_timber_u_m",
        "sinc_timber_face_id",
        "sinc_joinery_reserved",
        "sinc_hewing_depth_m",
        "sinc_end_distance_m",
        "IGGY_TimberUV",
    } - set(mesh.attributes.keys())
    if missing_attributes:
        raise RuntimeError(f"Missing evaluated attributes: {missing_attributes}")
    topology = bmesh.new()
    topology.from_mesh(mesh)
    non_manifold_edges = sum(not edge.is_manifold for edge in topology.edges)
    topology.free()
    if non_manifold_edges:
        raise RuntimeError(
            f"Evaluated master has {non_manifold_edges} non-manifold edges"
        )
    modifier_types = [modifier.type for modifier in master.modifiers]
    if modifier_types.count("BOOLEAN") < 9:
        raise RuntimeError(f"Missing intrinsic Boolean layers: {modifier_types}")
    if modifier_types[-2:] != ["BEVEL", "WEIGHTED_NORMAL"]:
        raise RuntimeError(f"Modifier tail is wrong: {modifier_types[-2:]}")
    result = {
        "source_vertices": len(master.data.vertices),
        "source_polygons": len(master.data.polygons),
        "evaluated_vertices": len(mesh.vertices),
        "evaluated_polygons": len(mesh.polygons),
        "evaluated_edges": len(mesh.edges),
        "non_manifold_edges": non_manifold_edges,
        "dimensions_m": dimensions,
        "clean_source_modifier_count": len(clean.modifiers),
        "modifier_types": modifier_types,
        "attribute_names": sorted(mesh.attributes.keys()),
    }
    bpy.data.meshes.remove(mesh)
    return result


def write_manifest(
    profile: dict[str, Any],
    output_root: Path,
    blend_path: Path,
    validation: dict[str, Any],
    renders: dict[str, dict[str, Any]],
) -> Path:
    manifest = {
        "schema": "iggy3d.world_asset.rough_hewn_timber_beam.build.v1",
        "asset_id": profile["asset_id"],
        "asset_scope": "one_master_beam",
        "repair_pass_count": 10,
        "source": {
            "profile": str(PROFILE_PATH.resolve()),
            "profile_sha256": sha256_file(PROFILE_PATH),
            "builder": str(Path(__file__).resolve()),
            "builder_sha256": sha256_file(Path(__file__)),
            "ai_generated_imagery": False,
        },
        "geometry": {
            "hewn_face_count": 4,
            "knot_count": len(profile["anatomy"]["knots"]),
            "end_check_count": len(profile["anatomy"]["end_checks"]),
            "edge_chip_count": len(profile["anatomy"]["edge_chips"]),
            "joinery_reserved_zone_count": len(
                profile["joinery_reserved_zones"]
            ),
            **validation,
        },
        "blender": {
            "version": bpy.app.version_string,
            "blend_path": str(blend_path.resolve()),
        },
        "renders": renders,
        "material_status": "neutral_clay_only",
        "remaining_material_only_details": [
            "white-oak longitudinal fibre and growth-ring colour hierarchy",
            "end-grain ring and pore response",
            "knot-driven grain deflection",
            "fine tool-edge linework",
            "independent roughness after geometry acceptance",
        ],
    }
    manifest_path = (
        output_root / "rough_hewn_timber_beam_v1_manifest.json"
    )
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest_path


def main() -> None:
    args = parse_args()
    profile = load_json(PROFILE_PATH)
    output_root = args.output_root.resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    width = args.render_width or profile["review_contract"]["render_width"]
    height = args.render_height or profile["review_contract"]["render_height"]

    clear_scene()
    asset_collection = ensure_collection(ASSET_COLLECTION_NAME)
    cutter_collection = ensure_collection(CUTTER_COLLECTION_NAME)
    review_collection = ensure_collection(REVIEW_COLLECTION_NAME)
    cutter_collection.hide_render = True

    clay = create_principled_material(
        CLAY_MATERIAL_NAME,
        srgb_rgba(126, 113, 96),
        roughness=0.82,
    )
    wire = create_wire_material()
    silhouette = create_principled_material(
        SILHOUETTE_MATERIAL_NAME,
        srgb_rgba(12, 14, 15),
        roughness=1.0,
    )
    master, clean, knot_bodies = create_beam_master(
        profile,
        asset_collection,
        cutter_collection,
        clay,
    )
    stage = create_review_stage(width, height, review_collection)

    validation = validate_evaluated_master(profile, master, clean)
    renders = create_review_package(
        profile,
        output_root,
        master,
        knot_bodies,
        stage,
        clay,
        wire,
        silhouette,
        skip_renders=args.skip_renders,
    )

    bpy.context.view_layer.objects.active = master
    master.select_set(True)
    blend_path = output_root / "rough_hewn_timber_beam_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    manifest_path = write_manifest(
        profile,
        output_root,
        blend_path,
        validation,
        renders,
    )
    print(
        "IGGY_WA001_BUILD="
        + json.dumps(
            {
                "blend": str(blend_path),
                "manifest": str(manifest_path),
                "geometry": validation,
                "render_count": len(renders),
            },
            sort_keys=True,
        )
    )


if __name__ == "__main__":
    main()
