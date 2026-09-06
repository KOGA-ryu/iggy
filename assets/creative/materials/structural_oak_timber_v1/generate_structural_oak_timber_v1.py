#!/usr/bin/env python3
"""Generate one measured, authored structural-white-oak growth volume.

The generator deliberately does not use random fields or procedural noise.  A
single pith path and one explicit annual-ring table are sampled by the four
longitudinal faces and both end faces.  The side treatment borrows the rope
material's useful architecture—finite tracks, local events, quiet fields, and
separate identity/response lanes—without borrowing rope's helical geometry.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
from typing import Any

import numpy as np


SCRIPT_ROOT = Path(__file__).resolve().parent
MATERIALS_ROOT = SCRIPT_ROOT.parent
if str(MATERIALS_ROOT) not in sys.path:
    sys.path.insert(0, str(MATERIALS_ROOT))

from pattern_lab_common import (  # noqa: E402
    compose_material_chapter,
    gray_rgb,
    linear_to_srgb,
    mix,
    normalized_range,
    render_material_preview,
    render_material_single_light,
    scalar_tint,
    smoothstep,
    srgb_to_linear,
    write_png_gray16,
    write_png_rgb8,
)


PROFILE_PATH = SCRIPT_ROOT / "profiles" / "structural_oak_timber_v1.json"
PATTERN_PATH = (
    SCRIPT_ROOT / "patterns" / "structural_oak_growth_champion_v1.json"
)
DEFAULT_OUTPUT_ROOT = SCRIPT_ROOT / "output"
PROFILE_SCHEMA = "iggy3d.material.structural_oak_timber.v1"
PATTERN_SCHEMA = "iggy3d.pattern.structural_oak_growth_champion.v1"
FACE_NAMES = ("front", "back", "top", "bottom")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", type=Path, default=PROFILE_PATH)
    parser.add_argument("--pattern", type=Path, default=PATTERN_PATH)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--side-width", type=int)
    parser.add_argument("--side-face-height", type=int)
    parser.add_argument("--end-tile-resolution", type=int)
    return parser.parse_args()


def _load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def _resolve_dependency(profile_path: Path, relative_path: str) -> Path:
    return (profile_path.parent.parent / relative_path).resolve()


def _hex_to_srgb(hex_color: str) -> np.ndarray:
    stripped = hex_color.lstrip("#")
    if len(stripped) != 6:
        raise ValueError(f"expected six-digit hex color, got {hex_color!r}")
    return np.array(
        [int(stripped[index : index + 2], 16) / 255.0 for index in (0, 2, 4)],
        dtype=np.float32,
    )


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def load_bundle(
    profile_path: str | Path = PROFILE_PATH,
    pattern_path: str | Path = PATTERN_PATH,
) -> dict[str, Any]:
    profile_source = Path(profile_path).resolve()
    pattern_source = Path(pattern_path).resolve()
    profile = _load_json(profile_source)
    pattern = _load_json(pattern_source)
    if profile.get("schema") != PROFILE_SCHEMA:
        raise ValueError(
            f"{profile_source} uses {profile.get('schema')!r}; "
            f"expected {PROFILE_SCHEMA!r}"
        )
    if pattern.get("schema") != PATTERN_SCHEMA:
        raise ValueError(
            f"{pattern_source} uses {pattern.get('schema')!r}; "
            f"expected {PATTERN_SCHEMA!r}"
        )

    door_path = _resolve_dependency(
        profile_source, profile["source_dependencies"]["door_profile"]
    )
    beam_path = _resolve_dependency(
        profile_source, profile["source_dependencies"]["beam_profile"]
    )
    door_profile = _load_json(door_path)
    beam_profile = _load_json(beam_path)
    palette_hex = door_profile["palette"]["observed_reference_twenty"]
    if len(palette_hex) != 20:
        raise ValueError("the accepted door source must provide twenty colors")
    palette_srgb = np.stack([_hex_to_srgb(color) for color in palette_hex])
    palette_linear = srgb_to_linear(palette_srgb)

    growth = pattern["growth_volume"]
    knot_count = len(growth["pith_y_knots_m"])
    if knot_count != 9 or len(growth["pith_z_knots_m"]) != knot_count:
        raise ValueError("growth volume requires nine matching pith control points")
    if len(growth["ring_radius_drift_knots_m"]) != knot_count:
        raise ValueError("ring-radius drift must match the pith controls")
    if len(growth["champion_ring_widths_m"]) != 64:
        raise ValueError("champion requires sixty-four explicitly authored rings")

    families = {family["face"]: family for family in pattern["grain_track_families"]}
    if tuple(families) != FACE_NAMES:
        raise ValueError("grain track families must be ordered front/back/top/bottom")
    for face, family in families.items():
        count = len(family["radii_m"])
        for lane in (
            "widths_m",
            "palette_indices",
            "starts_m",
            "lengths_m",
            "strengths",
        ):
            if len(family[lane]) != count:
                raise ValueError(f"{face} track lane {lane} has the wrong length")
    for event in pattern["track_events"]:
        if event["kind"] not in {"bury", "fade", "flatten"}:
            raise ValueError(f"unsupported grain event {event['kind']!r}")
        if event["track"] >= len(families[event["face"]]["radii_m"]):
            raise ValueError("grain event targets an absent track")

    return {
        "profile_path": profile_source,
        "pattern_path": pattern_source,
        "door_profile_path": door_path,
        "beam_profile_path": beam_path,
        "profile": profile,
        "pattern": pattern,
        "door_profile": door_profile,
        "beam_profile": beam_profile,
        "palette_srgb": palette_srgb,
        "palette_linear": palette_linear,
    }


def compile_ring_table(pattern: dict[str, Any]) -> dict[str, Any]:
    """Extend the 64-ring champion deterministically across the whole cant."""
    growth = pattern["growth_volume"]
    champion = np.asarray(growth["champion_ring_widths_m"], dtype=np.float64)
    epoch_scales = np.asarray(growth["epoch_scales"], dtype=np.float64)
    widths: list[float] = []
    epoch = 0
    total = 0.0
    while total < 0.78:
        scale = float(epoch_scales[epoch % len(epoch_scales)])
        for width in champion:
            represented = float(np.clip(width * scale, 0.00012, 0.00222))
            widths.append(represented)
            total += represented
        epoch += 1
    width_array = np.asarray(widths, dtype=np.float32)
    boundaries = np.concatenate(
        [np.zeros(1, dtype=np.float32), np.cumsum(width_array, dtype=np.float32)]
    )
    digest = hashlib.sha256(boundaries.tobytes()).hexdigest()
    return {
        "widths_m": width_array,
        "boundaries_m": boundaries,
        "digest": digest,
    }


def _interpolate_knots(
    u_m: np.ndarray,
    values: list[float],
    length_m: float,
) -> np.ndarray:
    position = np.clip(u_m / length_m, 0.0, 1.0) * (len(values) - 1)
    left = np.floor(position).astype(np.int32)
    right = np.minimum(left + 1, len(values) - 1)
    fraction = position - left
    fraction = fraction * fraction * (3.0 - 2.0 * fraction)
    source = np.asarray(values, dtype=np.float32)
    return source[left] * (1.0 - fraction) + source[right] * fraction


def evaluate_growth_volume(
    profile: dict[str, Any],
    pattern: dict[str, Any],
    ring_table: dict[str, Any],
    *,
    u_m: np.ndarray,
    y_m: np.ndarray,
    z_m: np.ndarray,
) -> dict[str, np.ndarray]:
    growth = pattern["growth_volume"]
    length_m = float(profile["beam_fixture_m"]["length"])
    pith_y = _interpolate_knots(u_m, growth["pith_y_knots_m"], length_m)
    pith_z = _interpolate_knots(u_m, growth["pith_z_knots_m"], length_m)
    drift = _interpolate_knots(
        u_m, growth["ring_radius_drift_knots_m"], length_m
    )
    ellipse_y, ellipse_z = growth["ellipse"]
    local_y = (y_m - pith_y) / float(ellipse_y)
    local_z = (z_m - pith_z) / float(ellipse_z)
    radius = np.sqrt(local_y * local_y + local_z * local_z) + drift
    angle = np.arctan2(local_z, local_y)

    boundaries = ring_table["boundaries_m"]
    ring_index = np.searchsorted(boundaries, radius, side="right") - 1
    ring_index = np.clip(ring_index, 0, len(ring_table["widths_m"]) - 1)
    inner = boundaries[ring_index]
    width = ring_table["widths_m"][ring_index]
    phase = np.clip((radius - inner) / np.maximum(width, 1.0e-8), 0.0, 1.0)
    return {
        "pith_y_m": pith_y.astype(np.float32),
        "pith_z_m": pith_z.astype(np.float32),
        "radius_m": radius.astype(np.float32),
        "angle_rad": angle.astype(np.float32),
        "ring_index": ring_index.astype(np.int32),
        "ring_width_m": width.astype(np.float32),
        "ring_phase": phase.astype(np.float32),
    }


def _finite_window(
    value: np.ndarray,
    start: float,
    length: float,
    feather: float,
) -> np.ndarray:
    safe_feather = min(max(feather, 1.0e-6), max(length * 0.49, 1.0e-6))
    enter = smoothstep(start, start + safe_feather, value)
    leave = 1.0 - smoothstep(
        start + length - safe_feather, start + length, value
    )
    return (enter * leave).astype(np.float32)


def _quiet_mask(
    pattern: dict[str, Any], face: str, u_m: np.ndarray
) -> np.ndarray:
    quiet = np.zeros_like(u_m, dtype=np.float32)
    for field in pattern["quiet_fields"]:
        if field["face"] != face:
            continue
        window = _finite_window(
            u_m,
            float(field["start_u_m"]),
            float(field["length_u_m"]),
            min(0.16, float(field["length_u_m"]) * 0.28),
        )
        quiet = np.maximum(quiet, window * float(field["strength"]))
    return quiet


def _side_coordinates(
    profile: dict[str, Any],
    face: str,
    u_m: np.ndarray,
    cross_m: np.ndarray,
) -> tuple[np.ndarray, np.ndarray]:
    fixture = profile["beam_fixture_m"]
    if face == "front":
        return np.full_like(u_m, -float(fixture["width"]) * 0.5), cross_m
    if face == "back":
        return np.full_like(u_m, float(fixture["width"]) * 0.5), cross_m
    if face == "top":
        return cross_m, np.full_like(u_m, float(fixture["height"]) * 0.5)
    if face == "bottom":
        return cross_m, np.full_like(u_m, -float(fixture["height"]) * 0.5)
    raise ValueError(f"unknown face {face!r}")


def _apply_knot_deflection(
    bundle: dict[str, Any],
    face: str,
    u_m: np.ndarray,
    cross_m: np.ndarray,
    y_m: np.ndarray,
    z_m: np.ndarray,
) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    knot_mask = np.zeros_like(u_m, dtype=np.float32)
    y_result = y_m.copy()
    z_result = z_m.copy()
    beam_length = float(bundle["profile"]["beam_fixture_m"]["length"])
    for knot in bundle["beam_profile"]["anatomy"]["knots"]:
        if knot["face"] != face:
            continue
        center_u = float(knot["x_m"]) + beam_length * 0.5
        influence = float(knot["grain_influence_radius_m"])
        du = (u_m - center_u) / influence
        dc = (cross_m - float(knot["cross_m"])) / (influence * 0.72)
        distance_squared = du * du + dc * dc
        local = np.clip(1.0 - distance_squared, 0.0, 1.0)
        local = local * local * (3.0 - 2.0 * local)
        knot_mask = np.maximum(knot_mask, local)

        cross_push = (
            np.sign(cross_m - float(knot["cross_m"]) + 1.0e-6)
            * float(knot["minor_radius_m"])
            * 0.72
            * local
        )
        longitudinal_curl = (
            -du * float(knot["major_radius_m"]) * 0.48 * local
        )
        radial_lift = float(knot["minor_radius_m"]) * 0.62 * local
        if face == "front":
            z_result = z_result + cross_push + longitudinal_curl
            y_result = y_result + radial_lift
        else:
            y_result = y_result + cross_push + longitudinal_curl
            z_result = z_result - radial_lift
    return y_result, z_result, knot_mask


def _height_to_normal_rect(
    height_m: np.ndarray,
    *,
    meters_per_pixel_x: float,
    meters_per_pixel_y: float,
    strength: float,
) -> np.ndarray:
    gradient_y, gradient_x = np.gradient(
        height_m,
        meters_per_pixel_y,
        meters_per_pixel_x,
        edge_order=1,
    )
    normal = np.stack(
        [
            -gradient_x * strength,
            -gradient_y * strength,
            np.ones_like(height_m),
        ],
        axis=-1,
    )
    normal /= np.maximum(np.linalg.norm(normal, axis=-1, keepdims=True), 1.0e-8)
    return normal.astype(np.float32)


def _broad_color_fields(
    bundle: dict[str, Any],
    face: str,
    u_m: np.ndarray,
    cross_m: np.ndarray,
    color: np.ndarray,
) -> np.ndarray:
    palette = bundle["palette_linear"]
    result = color
    for passage in bundle["pattern"]["broad_colour_passages"]:
        if face not in passage["faces"]:
            continue
        longitudinal = _finite_window(
            u_m,
            float(passage["u_start_m"]),
            float(passage["u_length_m"]),
            min(0.34, float(passage["u_length_m"]) * 0.22),
        )
        cross_distance = np.abs(
            cross_m - float(passage["cross_centre_m"])
        )
        cross_mask = 1.0 - smoothstep(
            float(passage["cross_width_m"]) * 0.36,
            float(passage["cross_width_m"]),
            cross_distance,
        )
        mask = longitudinal * cross_mask * float(passage["strength"])
        result = mix(
            result,
            np.broadcast_to(palette[int(passage["palette_index"])], result.shape),
            mask,
        )
    return result


def _grain_tracks(
    bundle: dict[str, Any],
    face: str,
    u_m: np.ndarray,
    radius_m: np.ndarray,
    quiet: np.ndarray,
    meters_per_pixel_cross: float,
) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    family = next(
        item
        for item in bundle["pattern"]["grain_track_families"]
        if item["face"] == face
    )
    combined = np.zeros_like(u_m, dtype=np.float32)
    height = np.zeros_like(u_m, dtype=np.float32)
    color_index = np.zeros_like(u_m, dtype=np.int16)
    color_weight = np.zeros_like(u_m, dtype=np.float32)
    events_by_track: dict[int, list[dict[str, Any]]] = {}
    for event in bundle["pattern"]["track_events"]:
        if event["face"] == face:
            events_by_track.setdefault(int(event["track"]), []).append(event)

    for index, target_radius in enumerate(family["radii_m"]):
        authored_width = float(family["widths_m"][index])
        represented_width = max(authored_width, meters_per_pixel_cross * 0.85)
        area_weight = min(1.0, authored_width / represented_width)
        distance = np.abs(radius_m - float(target_radius))
        radial = 1.0 - smoothstep(
            represented_width * 0.18,
            represented_width * 0.72,
            distance,
        )
        window = _finite_window(
            u_m,
            float(family["starts_m"][index]),
            float(family["lengths_m"][index]),
            min(0.17, float(family["lengths_m"][index]) * 0.18),
        )
        visibility = window * float(family["strengths"][index]) * area_weight
        relief_scale = 1.0
        for event in events_by_track.get(index, []):
            event_window = _finite_window(
                u_m,
                float(event["start_u_m"]),
                float(event["length_u_m"]),
                min(0.12, float(event["length_u_m"]) * 0.25),
            )
            strength = float(event["strength"])
            if event["kind"] == "bury":
                visibility *= 1.0 - event_window * strength
            elif event["kind"] == "fade":
                progress = np.clip(
                    (u_m - float(event["start_u_m"]))
                    / float(event["length_u_m"]),
                    0.0,
                    1.0,
                )
                visibility *= 1.0 - event_window * strength * progress
            elif event["kind"] == "flatten":
                relief_scale = relief_scale * (1.0 - event_window * strength)
        mask = radial * visibility * (1.0 - quiet * 0.72)
        replace = mask > color_weight
        color_index[replace] = int(family["palette_indices"][index])
        color_weight = np.maximum(color_weight, mask)
        combined = np.maximum(combined, mask)
        signed = -1.0 if index % 3 == 0 else 1.0
        height += (
            signed
            * mask
            * relief_scale
            * (0.000018 + authored_width * 0.012)
        )
    return combined, height, np.stack([color_index, color_weight], axis=-1)


def _side_ray_flecks(
    bundle: dict[str, Any],
    face: str,
    u_m: np.ndarray,
    cross_m: np.ndarray,
) -> np.ndarray:
    result = np.zeros_like(u_m, dtype=np.float32)
    for fleck in bundle["pattern"]["side_ray_flecks"]:
        if fleck["face"] != face:
            continue
        angle = math.radians(float(fleck["angle_degrees"]))
        du = u_m - float(fleck["u_m"])
        dc = cross_m - float(fleck["cross_m"])
        along = du * math.cos(angle) + dc * math.sin(angle)
        across = -du * math.sin(angle) + dc * math.cos(angle)
        ellipse = (
            along / (float(fleck["length_m"]) * 0.5)
        ) ** 2 + (
            across / (float(fleck["width_m"]) * 0.5)
        ) ** 2
        local = 1.0 - smoothstep(0.34, 1.0, ellipse)
        result = np.maximum(result, local * float(fleck["strength"]))
    return result


def _tool_linework(
    bundle: dict[str, Any],
    face: str,
    u_m: np.ndarray,
    cross_m: np.ndarray,
    meters_per_pixel_cross: float,
) -> np.ndarray:
    result = np.zeros_like(u_m, dtype=np.float32)
    selected = bundle["pattern"]["tool_linework_event_indices"][face]
    marks = bundle["beam_profile"]["hewing"]["faces"][face]["marks"]
    width_range = bundle["profile"]["physical_anatomy_m"][
        "tool_striation_width_range"
    ]
    cross_span = float(bundle["profile"]["beam_fixture_m"]["atlas_cross_span"])
    length_m = float(bundle["profile"]["beam_fixture_m"]["length"])
    for order, mark_index in enumerate(selected):
        mark = marks[int(mark_index)]
        center_u = float(mark[0]) + length_m * 0.5
        center_cross = float(mark[2]) * cross_span * 0.5
        segment_length = float(mark[1]) * (0.72 + 0.06 * (order % 4))
        angle = math.radians(float(mark[5]))
        du = u_m - center_u
        dc = cross_m - center_cross
        along = du * math.cos(angle) + dc * math.sin(angle)
        across = -du * math.sin(angle) + dc * math.cos(angle)
        authored_width = float(
            width_range[order % 2]
            if order % 2
            else width_range[0] + 0.00011 * (order % 4)
        )
        represented_width = max(authored_width, meters_per_pixel_cross * 0.85)
        area_weight = min(1.0, authored_width / represented_width)
        line = 1.0 - smoothstep(
            represented_width * 0.16,
            represented_width * 0.62,
            np.abs(across),
        )
        stop = _finite_window(
            along,
            -segment_length * 0.5,
            segment_length,
            min(0.018, segment_length * 0.18),
        )
        result = np.maximum(result, line * stop * area_weight * 0.76)
    return result


def _generate_side_face(
    bundle: dict[str, Any],
    ring_table: dict[str, Any],
    face: str,
    *,
    width: int,
    height: int,
) -> dict[str, np.ndarray]:
    profile = bundle["profile"]
    pattern = bundle["pattern"]
    length_m = float(profile["beam_fixture_m"]["length"])
    cross_span = float(profile["beam_fixture_m"]["atlas_cross_span"])
    u_axis = np.linspace(0.0, length_m, width, endpoint=False, dtype=np.float32)
    u_axis += length_m / width * 0.5
    cross_axis = np.linspace(
        -cross_span * 0.5,
        cross_span * 0.5,
        height,
        endpoint=False,
        dtype=np.float32,
    )
    cross_axis += cross_span / height * 0.5
    u_m, cross_m = np.meshgrid(u_axis, cross_axis)
    y_m, z_m = _side_coordinates(profile, face, u_m, cross_m)
    y_m, z_m, knot = _apply_knot_deflection(
        bundle, face, u_m, cross_m, y_m, z_m
    )
    growth = evaluate_growth_volume(
        profile,
        pattern,
        ring_table,
        u_m=u_m,
        y_m=y_m,
        z_m=z_m,
    )
    ring_index = growth["ring_index"]
    ring_phase = growth["ring_phase"]
    early_cycle = np.asarray(
        pattern["growth_volume"]["earlywood_fraction_cycle"],
        dtype=np.float32,
    )
    early_fraction = early_cycle[ring_index % len(early_cycle)]
    latewood = smoothstep(early_fraction, np.minimum(early_fraction + 0.16, 0.96), ring_phase)
    ring_identity = (
        0.22
        + 0.78
        * (
            0.62 * latewood
            + 0.38 * (ring_index % 7).astype(np.float32) / 6.0
        )
    ).astype(np.float32)

    palette = bundle["palette_linear"]
    color_cycle = np.asarray(
        pattern["growth_volume"]["ring_colour_palette_cycle"], dtype=np.int16
    )
    ring_palette_indices = color_cycle[ring_index % len(color_cycle)]
    base_index = {"front": 11, "back": 6, "top": 11, "bottom": 7}[face]
    color = np.broadcast_to(palette[base_index], (*u_m.shape, 3)).copy()
    ring_color = palette[ring_palette_indices]
    ring_color_weight = (0.06 + 0.075 * latewood)[..., np.newaxis]
    color = color * (1.0 - ring_color_weight) + ring_color * ring_color_weight
    color = _broad_color_fields(bundle, face, u_m, cross_m, color)

    quiet = _quiet_mask(pattern, face, u_m)
    meters_per_pixel_cross = cross_span / height
    tracks, track_height, track_color_data = _grain_tracks(
        bundle,
        face,
        u_m,
        growth["radius_m"],
        quiet,
        meters_per_pixel_cross,
    )
    track_indices = track_color_data[..., 0].astype(np.int16)
    track_weight = track_color_data[..., 1]
    color = mix(color, palette[track_indices], track_weight * 0.66)

    ray = _side_ray_flecks(bundle, face, u_m, cross_m)
    tool = _tool_linework(
        bundle, face, u_m, cross_m, meters_per_pixel_cross
    )
    knot_halo = smoothstep(0.08, 0.78, knot)
    color = mix(
        color,
        np.broadcast_to(palette[0], color.shape),
        knot_halo * 0.23,
    )
    color = mix(
        color,
        np.broadcast_to(palette[17], color.shape),
        ray * 0.34,
    )
    color = mix(
        color,
        np.broadcast_to(palette[0], color.shape),
        tool * 0.29,
    )
    quiet_color = np.broadcast_to(
        palette[{"front": 11, "back": 7, "top": 11, "bottom": 7}[face]],
        color.shape,
    )
    color = mix(color, quiet_color, quiet[..., np.newaxis] * 0.11)

    ring_relief = (
        (latewood - 0.52)
        * (1.0 - quiet * 0.78)
        * (0.000008 + growth["ring_width_m"] * 0.006)
    )
    height_m = (
        ring_relief
        + track_height
        + ray * 0.000014
        - tool * 0.000019
        - knot_halo * 0.000021
    )
    maximum = float(profile["physical_anatomy_m"]["maximum_side_relief"])
    height_m = np.clip(height_m, -maximum, maximum).astype(np.float32)
    normal = _height_to_normal_rect(
        height_m,
        meters_per_pixel_x=length_m / width,
        meters_per_pixel_y=cross_span / height,
        strength=float(profile["surface_response"]["normal_strength"]),
    )
    identity = np.stack([ring_identity, tracks, knot], axis=-1).astype(np.float32)
    response = np.stack([ray, tool, quiet], axis=-1).astype(np.float32)
    return {
        "base_color_linear": np.clip(color, 0.0, 1.0).astype(np.float32),
        "normal": normal,
        "height_m": height_m,
        "identity": identity,
        "response": response,
    }


def _ray_mask(
    pattern: dict[str, Any],
    radius_m: np.ndarray,
    angle_rad: np.ndarray,
    meters_per_pixel: float,
) -> tuple[np.ndarray, np.ndarray]:
    families = pattern["ray_families"]
    wide = np.zeros_like(radius_m, dtype=np.float32)
    for angle_degrees, authored_width in zip(
        families["wide_angles_degrees"],
        families["wide_widths_m"],
        strict=True,
    ):
        delta = np.arctan2(
            np.sin(angle_rad - math.radians(float(angle_degrees))),
            np.cos(angle_rad - math.radians(float(angle_degrees))),
        )
        distance = np.abs(delta) * np.maximum(radius_m, 0.008)
        represented = max(float(authored_width), meters_per_pixel * 0.85)
        area_weight = min(1.0, float(authored_width) / represented)
        local = 1.0 - smoothstep(
            represented * 0.14, represented * 0.68, distance
        )
        wide = np.maximum(wide, local * area_weight)

    step = math.radians(float(families["narrow_angle_step_degrees"]))
    offsets = np.asarray(
        families["narrow_angle_offsets_degrees"], dtype=np.float32
    )
    sector = np.floor((angle_rad + math.pi) / step).astype(np.int32)
    offset = np.deg2rad(offsets[np.abs(sector) % len(offsets)])
    local_angle = np.mod(angle_rad + math.pi + offset, step) - step * 0.5
    narrow_width = 0.000095
    represented = max(narrow_width, meters_per_pixel * 0.85)
    area_weight = min(1.0, narrow_width / represented)
    distance = np.abs(local_angle) * np.maximum(radius_m, 0.008)
    narrow = (
        1.0
        - smoothstep(represented * 0.14, represented * 0.72, distance)
    ) * area_weight
    return wide.astype(np.float32), narrow.astype(np.float32)


def _pore_mask(
    bundle: dict[str, Any],
    growth: dict[str, np.ndarray],
    meters_per_pixel: float,
) -> np.ndarray:
    pattern = bundle["pattern"]
    anatomy = bundle["profile"]["physical_anatomy_m"]
    ring_index = growth["ring_index"]
    ring_phase = growth["ring_phase"]
    early_cycle = np.asarray(
        pattern["growth_volume"]["earlywood_fraction_cycle"],
        dtype=np.float32,
    )
    early_fraction = early_cycle[ring_index % len(early_cycle)]
    radial_center = early_fraction * 0.46
    radial_distance = (
        np.abs(ring_phase - radial_center) * growth["ring_width_m"]
    )

    cycles = pattern["pore_cycles"]
    spacing_cycle = np.asarray(cycles["spacing_m"], dtype=np.float32)
    sigma_cycle = np.asarray(cycles["diameter_sigma_cycle"], dtype=np.float32)
    presence_cycle = np.asarray(cycles["presence_cycle"], dtype=np.float32)
    phase_cycle = np.asarray(cycles["phase_cycle"], dtype=np.float32)
    spacing = spacing_cycle[ring_index % len(spacing_cycle)]
    sigma = sigma_cycle[ring_index % len(sigma_cycle)]
    diameter = (
        float(anatomy["earlywood_vessel_mean"])
        + sigma * float(anatomy["earlywood_vessel_standard_deviation"])
    )
    diameter = np.clip(diameter, 0.000098, 0.000215)
    presence = presence_cycle[ring_index % len(presence_cycle)]
    phase = phase_cycle[ring_index % len(phase_cycle)]
    tangential = growth["radius_m"] * growth["angle_rad"]
    tangential_distance = np.abs(
        np.mod(tangential + phase * spacing + spacing * 0.5, spacing)
        - spacing * 0.5
    )
    represented = np.maximum(diameter, meters_per_pixel * 0.85)
    area_weight = np.minimum(1.0, (diameter / represented) ** 2)
    ellipse = (
        tangential_distance / np.maximum(represented * 0.5, 1.0e-8)
    ) ** 2 + (
        radial_distance / np.maximum(represented * 0.72, 1.0e-8)
    ) ** 2
    pore = (1.0 - smoothstep(0.12, 1.0, ellipse)) * area_weight * presence
    pore *= (ring_phase <= early_fraction).astype(np.float32)
    return pore.astype(np.float32)


def _generate_end_face(
    bundle: dict[str, Any],
    ring_table: dict[str, Any],
    *,
    u_value_m: float,
    resolution: int,
) -> dict[str, np.ndarray]:
    profile = bundle["profile"]
    pattern = bundle["pattern"]
    span = float(profile["beam_fixture_m"]["atlas_cross_span"])
    axis = np.linspace(
        -span * 0.5, span * 0.5, resolution, endpoint=False, dtype=np.float32
    )
    axis += span / resolution * 0.5
    y_m, z_m = np.meshgrid(axis, axis)
    u_m = np.full_like(y_m, u_value_m)
    growth = evaluate_growth_volume(
        profile,
        pattern,
        ring_table,
        u_m=u_m,
        y_m=y_m,
        z_m=z_m,
    )
    ring_index = growth["ring_index"]
    ring_phase = growth["ring_phase"]
    early_cycle = np.asarray(
        pattern["growth_volume"]["earlywood_fraction_cycle"],
        dtype=np.float32,
    )
    early_fraction = early_cycle[ring_index % len(early_cycle)]
    earlywood = 1.0 - smoothstep(
        np.maximum(early_fraction - 0.055, 0.0),
        np.minimum(early_fraction + 0.045, 1.0),
        ring_phase,
    )
    ring_boundary = np.maximum(
        1.0 - smoothstep(0.0, 0.12, ring_phase),
        smoothstep(0.86, 1.0, ring_phase),
    )
    meters_per_pixel = span / resolution
    wide_ray, narrow_ray = _ray_mask(
        pattern,
        growth["radius_m"],
        growth["angle_rad"],
        meters_per_pixel,
    )
    radial_segmentation = (
        0.46
        + 0.54
        * ((ring_index + (ring_index // 7)) % 5 != 1).astype(np.float32)
    )
    ray = np.maximum(wide_ray, narrow_ray * 0.58) * radial_segmentation
    pore = _pore_mask(bundle, growth, meters_per_pixel)

    palette = bundle["palette_linear"]
    color_cycle = np.asarray(
        pattern["growth_volume"]["ring_colour_palette_cycle"], dtype=np.int16
    )
    indices = color_cycle[ring_index % len(color_cycle)]
    base = np.broadcast_to(palette[7], (*y_m.shape, 3)).copy()
    base = mix(base, palette[indices], 0.10 + earlywood * 0.07)
    base = mix(
        base,
        np.broadcast_to(palette[0], base.shape),
        ring_boundary * 0.13,
    )
    base = mix(
        base,
        np.broadcast_to(palette[13], base.shape),
        wide_ray * 0.31 + narrow_ray * 0.12,
    )
    base = mix(
        base,
        np.broadcast_to(palette[0], base.shape),
        pore * 0.72,
    )
    end_darkening = 0.075 if u_value_m <= 0.0 else 0.045
    base = mix(
        base,
        np.broadcast_to(palette[6], base.shape),
        np.full_like(y_m, end_darkening),
    )

    height_m = (
        (ring_boundary - earlywood * 0.28) * 0.000014
        + wide_ray * 0.000026
        + narrow_ray * 0.000008
        - pore * 0.000078
    )
    maximum = float(profile["physical_anatomy_m"]["maximum_end_relief"])
    height_m = np.clip(height_m, -maximum, maximum).astype(np.float32)
    normal = _height_to_normal_rect(
        height_m,
        meters_per_pixel_x=meters_per_pixel,
        meters_per_pixel_y=meters_per_pixel,
        strength=float(profile["surface_response"]["normal_strength"]),
    )
    identity = np.stack([ring_boundary, ray, pore], axis=-1).astype(np.float32)
    return {
        "base_color_linear": np.clip(base, 0.0, 1.0).astype(np.float32),
        "normal": normal,
        "height_m": height_m,
        "identity": identity,
    }


def generate_material(
    bundle: dict[str, Any],
    *,
    side_width: int,
    side_face_height: int,
    end_tile_resolution: int,
) -> dict[str, np.ndarray | str | int]:
    if min(side_width, side_face_height, end_tile_resolution) < 32:
        raise ValueError("all output dimensions must be at least thirty-two pixels")
    ring_table = compile_ring_table(bundle["pattern"])
    side_faces = [
        _generate_side_face(
            bundle,
            ring_table,
            face,
            width=side_width,
            height=side_face_height,
        )
        for face in FACE_NAMES
    ]
    length_m = float(bundle["profile"]["beam_fixture_m"]["length"])
    end_faces = [
        _generate_end_face(
            bundle,
            ring_table,
            u_value_m=u_value,
            resolution=end_tile_resolution,
        )
        for u_value in (0.0, length_m)
    ]
    result: dict[str, np.ndarray | str | int] = {
        "ring_table_digest": ring_table["digest"],
        "ring_count": len(ring_table["widths_m"]),
    }
    for key, output_key in (
        ("base_color_linear", "side_base_color_linear"),
        ("normal", "side_normal"),
        ("height_m", "side_height_m"),
        ("identity", "side_identity"),
        ("response", "side_response"),
    ):
        result[output_key] = np.concatenate(
            [face[key] for face in side_faces], axis=0
        )
    for key, output_key in (
        ("base_color_linear", "end_base_color_linear"),
        ("normal", "end_normal"),
        ("height_m", "end_height_m"),
        ("identity", "end_identity"),
    ):
        result[output_key] = np.concatenate(
            [face[key] for face in end_faces], axis=1
        )
    return result


def _material_book(
    bundle: dict[str, Any],
    material: dict[str, Any],
    *,
    side_face_height: int,
    end_tile_resolution: int,
) -> np.ndarray:
    side_base = linear_to_srgb(material["side_base_color_linear"])
    end_base = linear_to_srgb(material["end_base_color_linear"])
    front = side_base[:side_face_height]
    front_normal = material["side_normal"][:side_face_height] * 0.5 + 0.5
    front_height = material["side_height_m"][:side_face_height]
    maximum_side = float(
        bundle["profile"]["physical_anatomy_m"]["maximum_side_relief"]
    )
    height_display = np.clip(front_height / (2.0 * maximum_side) + 0.5, 0.0, 1.0)
    front_roughness = np.full(
        front.shape[:2],
        float(bundle["profile"]["surface_response"]["uniform_roughness"]),
        dtype=np.float32,
    )
    lit = render_material_preview(
        material["side_base_color_linear"][:side_face_height],
        material["side_normal"][:side_face_height],
        front_roughness,
        np.ones(front.shape[:2], dtype=np.float32),
    )
    grazing = render_material_single_light(
        material["side_base_color_linear"][:side_face_height],
        material["side_normal"][:side_face_height],
        front_roughness,
        np.ones(front.shape[:2], dtype=np.float32),
        direction=(0.94, -0.24, 0.22),
        intensity=2.2,
    )
    left_end = end_base[:, :end_tile_resolution]
    left_identity = material["end_identity"][:, :end_tile_resolution]
    panels = [
        ("SIDE BASE / FRONT", front),
        ("SIDE ID / RING TRACK KNOT", material["side_identity"][:side_face_height]),
        ("SIDE RESPONSE / RAY TOOL QUIET", material["side_response"][:side_face_height]),
        ("SIDE NORMAL", front_normal),
        ("END BASE / LEFT", left_end),
        ("END ID / RING RAY PORE", left_identity),
        ("NEUTRAL MATERIAL LIGHT", lit),
        ("GRAZING RELIEF", grazing),
    ]
    return compose_material_chapter(
        "STRUCTURAL WHITE OAK / GROWTH VOLUME V1",
        "ONE PITH PATH  |  ONE RING TABLE  |  FOUR SIDE SLICES + TWO END SLICES",
        panels,
        [
            "Measured anatomy: ring-porous Quercus alba; 0.12 mm suppressed rings to 2.0 mm+ vigorous rings.",
            "Rope transfer: finite authored tracks, bury/fade/flatten events, identity lanes, and deliberate quiet fields.",
            "No random noise, photographic projection, roughness map, damage, checks, chips, dings, or grime.",
            "Subpixel vessels and rays are area filtered. They are not inflated into decorative fantasy grain.",
        ],
    )


def build_outputs(
    bundle: dict[str, Any],
    output_root: str | Path,
    *,
    side_width: int,
    side_face_height: int,
    end_tile_resolution: int,
) -> dict[str, Any]:
    output = Path(output_root)
    output.mkdir(parents=True, exist_ok=True)
    material = generate_material(
        bundle,
        side_width=side_width,
        side_face_height=side_face_height,
        end_tile_resolution=end_tile_resolution,
    )
    prefix = "structural_oak_timber_v1"
    write_png_rgb8(
        output / f"{prefix}_side_basecolor.png",
        linear_to_srgb(material["side_base_color_linear"]),
    )
    write_png_rgb8(
        output / f"{prefix}_side_normal.png",
        material["side_normal"] * 0.5 + 0.5,
    )
    maximum_side = float(
        bundle["profile"]["physical_anatomy_m"]["maximum_side_relief"]
    )
    write_png_gray16(
        output / f"{prefix}_side_height.png",
        np.clip(material["side_height_m"] / (2.0 * maximum_side) + 0.5, 0.0, 1.0),
    )
    write_png_rgb8(
        output / f"{prefix}_side_identity.png", material["side_identity"]
    )
    write_png_rgb8(
        output / f"{prefix}_side_response.png", material["side_response"]
    )
    write_png_rgb8(
        output / f"{prefix}_end_basecolor.png",
        linear_to_srgb(material["end_base_color_linear"]),
    )
    write_png_rgb8(
        output / f"{prefix}_end_normal.png",
        material["end_normal"] * 0.5 + 0.5,
    )
    maximum_end = float(
        bundle["profile"]["physical_anatomy_m"]["maximum_end_relief"]
    )
    write_png_gray16(
        output / f"{prefix}_end_height.png",
        np.clip(material["end_height_m"] / (2.0 * maximum_end) + 0.5, 0.0, 1.0),
    )
    write_png_rgb8(
        output / f"{prefix}_end_identity.png", material["end_identity"]
    )
    book = _material_book(
        bundle,
        material,
        side_face_height=side_face_height,
        end_tile_resolution=end_tile_resolution,
    )
    write_png_rgb8(output / f"{prefix}_material_book.png", book)

    ring_digest = str(material["ring_table_digest"])
    manifest = {
        "schema": "iggy3d.material.structural_oak_timber.output.v1",
        "profile_id": bundle["profile"]["profile_id"],
        "pattern_id": bundle["pattern"]["pattern_id"],
        "dimensions_px": {
            "side": [side_width, side_face_height * 4],
            "side_face": [side_width, side_face_height],
            "end": [end_tile_resolution * 2, end_tile_resolution],
            "end_face": [end_tile_resolution, end_tile_resolution],
        },
        "physical_extent_m": {
            "side_face": [
                bundle["profile"]["beam_fixture_m"]["length"],
                bundle["profile"]["beam_fixture_m"]["atlas_cross_span"],
            ],
            "end_face": [
                bundle["profile"]["beam_fixture_m"]["atlas_cross_span"],
                bundle["profile"]["beam_fixture_m"]["atlas_cross_span"],
            ],
        },
        "continuity": {
            "ring_count": int(material["ring_count"]),
            "side_ring_table_digest": ring_digest,
            "end_ring_table_digest": ring_digest,
            "pith_control_count": len(
                bundle["pattern"]["growth_volume"]["pith_y_knots_m"]
            ),
            "rule": "Every side and end face sampled the same pith controls and ring table.",
        },
        "surface_response": {
            "uniform_roughness": bundle["profile"]["surface_response"][
                "uniform_roughness"
            ],
            "roughness_map": False,
            "damage": False,
            "side_height_decode_m": [-maximum_side, maximum_side],
            "end_height_decode_m": [-maximum_end, maximum_end],
        },
        "authored_counts": {
            "champion_ring_widths": len(
                bundle["pattern"]["growth_volume"]["champion_ring_widths_m"]
            ),
            "grain_tracks": sum(
                len(family["radii_m"])
                for family in bundle["pattern"]["grain_track_families"]
            ),
            "track_events": len(bundle["pattern"]["track_events"]),
            "wide_rays": len(
                bundle["pattern"]["ray_families"]["wide_angles_degrees"]
            ),
            "side_ray_flecks": len(bundle["pattern"]["side_ray_flecks"]),
            "tool_linework_events": sum(
                len(indices)
                for indices in bundle["pattern"][
                    "tool_linework_event_indices"
                ].values()
            ),
        },
        "source_sha256": {
            "profile": _sha256(bundle["profile_path"]),
            "pattern": _sha256(bundle["pattern_path"]),
            "accepted_door_palette": _sha256(bundle["door_profile_path"]),
            "beam_anatomy": _sha256(bundle["beam_profile_path"]),
        },
        "files": sorted(path.name for path in output.iterdir()),
    }
    (output / f"{prefix}_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n"
    )
    manifest["files"] = sorted(path.name for path in output.iterdir())
    return manifest


def main() -> None:
    args = parse_args()
    bundle = load_bundle(args.profile, args.pattern)
    representation = bundle["profile"]["representation"]
    side_width = (
        args.side_width
        if args.side_width is not None
        else int(representation["side_atlas"]["width"])
    )
    side_height_total = int(representation["side_atlas"]["height"])
    side_face_height = (
        args.side_face_height
        if args.side_face_height is not None
        else side_height_total // 4
    )
    end_tile_resolution = (
        args.end_tile_resolution
        if args.end_tile_resolution is not None
        else int(representation["end_atlas"]["height"])
    )
    manifest = build_outputs(
        bundle,
        args.output_root,
        side_width=side_width,
        side_face_height=side_face_height,
        end_tile_resolution=end_tile_resolution,
    )
    print(
        json.dumps(
            {
                "profile_id": manifest["profile_id"],
                "pattern_id": manifest["pattern_id"],
                "dimensions_px": manifest["dimensions_px"],
                "ring_count": manifest["continuity"]["ring_count"],
                "output_root": str(Path(args.output_root).resolve()),
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
