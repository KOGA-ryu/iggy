#!/usr/bin/env python3
"""Generate layered lime plaster and hand-authored giant rubble masonry."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
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
    height_to_normal,
    linear_to_srgb,
    mix,
    normalized_range,
    palette_panel,
    periodic_fbm_rect,
    periodic_gaussian_blur,
    proof_resize,
    render_material_preview,
    render_material_single_light,
    scalar_tint,
    seam_frame,
    smoothstep,
    srgb_to_linear,
    write_png_gray16,
    write_png_rgb8,
    zoom_square,
)


MATERIAL_SCHEMA = "iggy3d.material.lime_plaster_masonry_v1.v2"
PATTERN_SCHEMA = "iggy3d.pattern.giant_house_rubble.v1"
MATERIAL_INTENT_SCHEMA = "iggy3d.material_intent.lime_plaster_masonry.v2"
DEFAULT_PATTERN = SCRIPT_ROOT / "patterns" / "giant_house_rubble_v1.json"
DEFAULT_PROFILE = SCRIPT_ROOT / "profiles" / "lime_plaster_masonry_v1.json"
DEFAULT_MATERIAL_INTENT = (
    SCRIPT_ROOT
    / "references"
    / "lime_plaster_masonry_material_intent_v2.json"
)
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"


@dataclass(frozen=True)
class RubbleRecipe:
    schema: str
    name: str
    tile_size_m: float
    mortar: dict[str, Any]
    champions: tuple[dict[str, Any], ...]
    courses: tuple[dict[str, Any], ...]


STONE_FAMILIES = (
    "cool_limestone",
    "warm_limestone",
    "grey_green",
    "umber_vein",
    "pale_chalk",
)


def _intent_palette(
    entry: dict[str, Any],
    *,
    label: str,
) -> np.ndarray:
    anchors = np.asarray(entry.get("shade_anchors_rgb"), dtype=np.float32)
    if anchors.ndim != 2 or anchors.shape[1] != 3 or anchors.shape[0] < 3:
        raise ValueError(f"{label} must provide at least three RGB anchors")
    if np.any(anchors < 0.0) or np.any(anchors > 255.0):
        raise ValueError(f"{label} contains an invalid RGB channel")
    anchor_luminance = np.sum(
        anchors
        * np.asarray([0.2126, 0.7152, 0.0722], dtype=np.float32),
        axis=-1,
    )
    if np.any(np.diff(anchor_luminance) < -1.0e-3):
        raise ValueError(f"{label} anchors must be ordered by luminance")
    source = np.linspace(0.0, 1.0, anchors.shape[0], dtype=np.float32)
    target = np.linspace(0.0, 1.0, 20, dtype=np.float32)
    palette = np.stack(
        [
            np.interp(target, source, anchors[:, channel])
            for channel in range(3)
        ],
        axis=-1,
    ).astype(np.float32)
    return palette / 255.0


def load_material_intent(
    path: str | Path = DEFAULT_MATERIAL_INTENT,
) -> dict[str, Any]:
    """Load written and hand-authored intent; reference pixels never enter."""
    payload = json.loads(Path(path).read_text())
    if payload.get("schema") != MATERIAL_INTENT_SCHEMA:
        raise ValueError("Unexpected lime-plaster material-intent schema")
    source = payload.get("source", {})
    if source.get("uses_ai_generated_reference") is not False:
        raise ValueError("Canonical material intent may not use AI imagery")
    if source.get("uses_sampled_reference_pixels") is not False:
        raise ValueError("Canonical material intent may not sample pixels")
    translation = payload.get("production_translation", {})
    if translation.get("raw_pixels_used_as_runtime_texture") is not False:
        raise ValueError("Reference intent must forbid raw runtime pixels")
    if translation.get("legacy_ai_experiments_are_build_inputs") is not False:
        raise ValueError("Legacy AI experiments may not be build inputs")
    materials = payload.get("materials", {})
    for name in (
        "plaster",
        "mortar",
        "finish_coat",
        "brown_coat",
        "scratch_coat",
    ):
        _intent_palette(materials.get(name, {}), label=name)
    stone = materials.get("stone", {})
    _intent_palette(
        stone.get("common_field", {}),
        label="stone/common_field",
    )
    families = stone.get("families", {})
    if set(families) != set(STONE_FAMILIES):
        raise ValueError("Reference intent must define all five stone families")
    for family, entry in families.items():
        _intent_palette(entry, label=f"stone/{family}")
    line_color = np.asarray(
        payload.get("style_intent", {}).get("line_color_rgb"),
        dtype=np.float32,
    )
    if line_color.shape != (3,) or np.any(line_color < 0.0) or np.any(
        line_color > 255.0
    ):
        raise ValueError("Reference intent must define one RGB line color")
    return payload


def load_rubble_recipe(path: str | Path = DEFAULT_PATTERN) -> RubbleRecipe:
    payload = json.loads(Path(path).read_text())
    if payload.get("schema") != PATTERN_SCHEMA:
        raise ValueError("Unexpected giant-house rubble recipe schema")
    tile_size = float(payload["tile_size_m"])
    if not 2.0 <= tile_size <= 4.0:
        raise ValueError("Rubble tile must remain within the 2-4 metre contract")
    champions = tuple(payload["champions"])
    champion_names = {entry["name"] for entry in champions}
    if len(champion_names) != len(champions):
        raise ValueError("Champion names must be unique")
    for champion in champions:
        outline = np.asarray(champion.get("outline"), dtype=np.float32)
        if (
            outline.ndim != 2
            or outline.shape[1] != 2
            or outline.shape[0] < 8
            or np.any(outline < 0.0)
            or np.any(outline > 1.0)
        ):
            raise ValueError(
                f"{champion['name']} needs an authored normalized outline"
            )
    courses = tuple(payload["courses"])
    if not courses:
        raise ValueError("At least one authored course is required")
    last_top = 0.0
    stone_count = 0
    for course in courses:
        bottom = float(course["bottom_m"])
        top = float(course["top_m"])
        if not math.isclose(bottom, last_top, abs_tol=1.0e-6):
            raise ValueError("Courses must be contiguous and ordered")
        joints = [float(value) for value in course["joints_m"]]
        if joints[0] != 0.0 or not math.isclose(
            joints[-1], tile_size, abs_tol=1.0e-6
        ):
            raise ValueError("Every course must span the full tile")
        if any(b <= a for a, b in zip(joints, joints[1:])):
            raise ValueError("Course joints must be strictly increasing")
        if len(course["stones"]) != len(joints) - 1:
            raise ValueError("Every joint interval needs one authored stone")
        for champion, family, _angle, _relief in course["stones"]:
            if champion not in champion_names:
                raise ValueError(f"Unknown stone champion {champion}")
            if family not in STONE_FAMILIES:
                raise ValueError(f"Unknown stone pigment family {family}")
        stone_count += len(course["stones"])
        last_top = top
    if not math.isclose(last_top, tile_size, abs_tol=1.0e-6):
        raise ValueError("Courses must fill the tile vertically")
    if stone_count < 24:
        raise ValueError("Giant rubble recipe is under-authored")
    return RubbleRecipe(
        schema=payload["schema"],
        name=payload["name"],
        tile_size_m=tile_size,
        mortar=payload["mortar"],
        champions=champions,
        courses=courses,
    )


def _champion_lookup(recipe: RubbleRecipe) -> dict[str, dict[str, Any]]:
    return {entry["name"]: entry for entry in recipe.champions}


def generate_stone_layout(
    recipe: RubbleRecipe,
    *,
    variation: int = 0,
) -> list[dict[str, Any]]:
    """Expand the authored courses without changing their construction bond."""
    champion_lookup = _champion_lookup(recipe)
    stones: list[dict[str, Any]] = []
    stone_id = 1
    for course_index, course in enumerate(recipe.courses):
        joints = [float(value) for value in course["joints_m"]]
        for position, authored in enumerate(course["stones"]):
            champion_name, family, angle, relief = authored
            x0 = joints[position]
            x1 = joints[position + 1]
            phase = (
                (variation * 0.071)
                + (course_index * 0.113)
                + (position * 0.173)
            ) % 1.0
            stones.append(
                {
                    "stone_id": stone_id,
                    "course_index": course_index,
                    "position": position,
                    "x0_m": x0,
                    "x1_m": x1,
                    "bottom_m": float(course["bottom_m"]),
                    "top_m": float(course["top_m"]),
                    "lower_tilt_m": float(course["lower_tilt_m"]),
                    "upper_tilt_m": float(course["upper_tilt_m"]),
                    "champion_name": champion_name,
                    "champion": champion_lookup[champion_name],
                    "family": family,
                    "face_angle": float(angle),
                    "relief_m": float(relief),
                    "phase": phase,
                }
            )
            stone_id += 1
    return stones


def _stone_polygon(
    stone: dict[str, Any],
    recipe: RubbleRecipe,
) -> np.ndarray:
    perp = float(recipe.mortar["perpend_joint_m"])
    bed = float(recipe.mortar["bed_joint_m"])
    x0 = stone["x0_m"] + perp * 0.5
    x1 = stone["x1_m"] - perp * 0.5
    width = x1 - x0
    bottom = stone["bottom_m"] + bed * 0.5
    top = stone["top_m"] - bed * 0.5
    height = top - bottom
    outline = np.asarray(
        stone["champion"]["outline"],
        dtype=np.float32,
    ).copy()
    mirror = (stone["course_index"] + stone["position"]) % 2 == 1
    if mirror:
        outline[:, 0] = 1.0 - outline[:, 0]
    u = outline[:, 0]
    v = outline[:, 1]
    lower_tilt = (u - 0.5) * float(stone["lower_tilt_m"])
    upper_tilt = (u - 0.5) * float(stone["upper_tilt_m"])
    course_tilt = lower_tilt * (1.0 - v) + upper_tilt * v
    face_shear = float(stone["relief_m"]) * 0.42 * v
    phase = float(stone["phase"]) * math.tau
    authored_sibling = np.sin(
        np.arange(outline.shape[0], dtype=np.float32) * 1.73 + phase
    )
    x = x0 + u * width + face_shear + authored_sibling * 0.0045
    y = bottom + v * height + course_tilt
    y += np.cos(
        np.arange(outline.shape[0], dtype=np.float32) * 1.31 + phase
    ) * 0.0035
    return np.stack([x, y], axis=-1).astype(np.float32)


def _points_inside_polygon(
    x: np.ndarray,
    y: np.ndarray,
    polygon: np.ndarray,
) -> np.ndarray:
    inside = np.zeros(x.shape, dtype=bool)
    previous = polygon[-1]
    for current in polygon:
        x0, y0 = previous
        x1, y1 = current
        crosses = (y0 > y) != (y1 > y)
        intersection = (x1 - x0) * (y - y0) / (
            (y1 - y0) + np.float32(1.0e-12)
        ) + x0
        inside ^= np.logical_and(crosses, x < intersection)
        previous = current
    return inside


def _distance_to_polygon_edges(
    x: np.ndarray,
    y: np.ndarray,
    polygon: np.ndarray,
) -> np.ndarray:
    distance_sq = np.full(x.shape, np.inf, dtype=np.float32)
    previous = polygon[-1]
    for current in polygon:
        ax, ay = previous
        bx, by = current
        vx = bx - ax
        vy = by - ay
        denominator = max(vx * vx + vy * vy, 1.0e-10)
        t = np.clip(((x - ax) * vx + (y - ay) * vy) / denominator, 0.0, 1.0)
        dx = x - (ax + t * vx)
        dy = y - (ay + t * vy)
        distance_sq = np.minimum(distance_sq, dx * dx + dy * dy)
        previous = current
    return np.sqrt(distance_sq, dtype=np.float32)


def rasterize_rubble_pattern(
    layout: list[dict[str, Any]],
    recipe: RubbleRecipe,
    *,
    resolution: int,
) -> dict[str, np.ndarray | int]:
    if resolution < 64:
        raise ValueError("Rubble raster requires at least 64 pixels")
    tile = recipe.tile_size_m
    coordinate = (
        np.arange(resolution, dtype=np.float32) + 0.5
    ) * tile / resolution
    stone_id = np.zeros((resolution, resolution), dtype=np.int32)
    edge_distance = np.zeros((resolution, resolution), dtype=np.float32)
    local_u = np.zeros((resolution, resolution), dtype=np.float32)
    local_v = np.zeros((resolution, resolution), dtype=np.float32)
    face_angle = np.zeros((resolution, resolution), dtype=np.float32)
    relief = np.zeros((resolution, resolution), dtype=np.float32)
    phase_field = np.zeros((resolution, resolution), dtype=np.float32)

    for stone in layout:
        polygon = _stone_polygon(stone, recipe)
        x_min = max(0, int(math.floor(float(polygon[:, 0].min()) / tile * resolution)) - 2)
        x_max = min(
            resolution,
            int(math.ceil(float(polygon[:, 0].max()) / tile * resolution)) + 2,
        )
        y_min = max(0, int(math.floor(float(polygon[:, 1].min()) / tile * resolution)) - 2)
        y_max = min(
            resolution,
            int(math.ceil(float(polygon[:, 1].max()) / tile * resolution)) + 2,
        )
        if x_min >= x_max or y_min >= y_max:
            continue
        xx, yy = np.meshgrid(
            coordinate[x_min:x_max],
            coordinate[y_min:y_max],
        )
        inside = _points_inside_polygon(xx, yy, polygon)
        if not np.any(inside):
            continue
        distance = _distance_to_polygon_edges(xx, yy, polygon)
        target = np.s_[y_min:y_max, x_min:x_max]
        stone_id[target][inside] = stone["stone_id"]
        edge_distance[target][inside] = distance[inside]
        width = max(stone["x1_m"] - stone["x0_m"], 1.0e-6)
        height = max(stone["top_m"] - stone["bottom_m"], 1.0e-6)
        local_u[target][inside] = np.clip(
            (xx[inside] - stone["x0_m"]) / width, 0.0, 1.0
        )
        local_v[target][inside] = np.clip(
            (yy[inside] - stone["bottom_m"]) / height, 0.0, 1.0
        )
        face_angle[target][inside] = stone["face_angle"]
        relief[target][inside] = stone["relief_m"]
        phase_field[target][inside] = stone["phase"]

    stone_mask = (stone_id > 0).astype(np.float32)
    return {
        "stone_count": len(layout),
        "stone_id": stone_id,
        "stone_mask": stone_mask,
        "mortar_mask": 1.0 - stone_mask,
        "edge_distance_m": edge_distance,
        "local_u": local_u,
        "local_v": local_v,
        "face_angle": face_angle,
        "relief_m": relief,
        "phase": phase_field,
    }


def _stone_shade_palettes(
    layout: list[dict[str, Any]],
    *,
    variation: int,
    intent: dict[str, Any],
) -> np.ndarray:
    palettes = np.zeros((len(layout) + 1, 20, 3), dtype=np.float32)
    stone_intent = intent["materials"]["stone"]
    families = stone_intent["families"]
    common = _intent_palette(
        stone_intent["common_field"],
        label="stone/common_field",
    )
    for stone in layout:
        palette = _intent_palette(
            families[stone["family"]],
            label=f"stone/{stone['family']}",
        ).copy()
        palette = common * 0.38 + palette * 0.62
        phase = stone["phase"] * math.tau + variation * 0.31
        tint = np.array(
            [
                math.sin(phase) * 0.010,
                math.cos(phase * 1.31) * 0.008,
                math.sin(phase * 0.77 + 1.0) * 0.007,
            ],
            dtype=np.float32,
        )
        palettes[stone["stone_id"]] = np.clip(palette + tint, 0.04, 0.88)
    return palettes


def _sample_palette_field(
    palettes: np.ndarray,
    identity: np.ndarray,
    coordinate: np.ndarray,
) -> np.ndarray:
    index = np.clip(coordinate, 0.0, 1.0) * (palettes.shape[1] - 1)
    lower = np.floor(index).astype(np.int32)
    upper = np.minimum(lower + 1, palettes.shape[1] - 1)
    fraction = index - lower
    return (
        palettes[identity, lower] * (1.0 - fraction[..., np.newaxis])
        + palettes[identity, upper] * fraction[..., np.newaxis]
    ).astype(np.float32)


TROWEL_STROKES_M = (
    (0.46, 0.52, 1.22, 0.34, 0.18, 0.22),
    (1.48, 0.40, 1.46, 0.30, -0.10, -0.16),
    (2.72, 0.62, 1.18, 0.36, 0.24, 0.19),
    (3.52, 0.38, 0.92, 0.28, -0.18, -0.13),
    (0.84, 1.46, 1.68, 0.38, -0.05, 0.18),
    (2.40, 1.34, 1.34, 0.32, 0.14, -0.15),
    (3.42, 1.62, 1.10, 0.30, -0.22, 0.17),
    (0.42, 2.46, 1.08, 0.28, 0.26, -0.14),
    (1.54, 2.62, 1.52, 0.36, -0.16, 0.20),
    (2.98, 2.42, 1.40, 0.34, 0.08, -0.17),
    (0.92, 3.46, 1.48, 0.32, 0.12, 0.18),
    (2.30, 3.38, 1.62, 0.38, -0.09, -0.16),
    (3.54, 3.50, 1.02, 0.28, 0.21, 0.15),
)


def finite_trowel_field(
    resolution: int,
    *,
    variation: int,
) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    coordinate = (
        np.arange(resolution, dtype=np.float32) + 0.5
    ) * 4.0 / resolution
    x, y = np.meshgrid(coordinate, coordinate)
    compression = np.zeros((resolution, resolution), dtype=np.float32)
    line = np.zeros_like(compression)
    direction = np.zeros((resolution, resolution, 2), dtype=np.float32)
    direction_weight = np.zeros_like(compression)
    for index, (cx, cy, length, width, angle, pressure) in enumerate(
        TROWEL_STROKES_M
    ):
        phase_shift = (variation % 4) * 0.043
        dx = np.mod(x - (cx + phase_shift) + 2.0, 4.0) - 2.0
        dy = np.mod(y - cy + 2.0, 4.0) - 2.0
        local_x = dx * math.cos(angle) + dy * math.sin(angle)
        local_y = -dx * math.sin(angle) + dy * math.cos(angle)
        longitudinal = np.abs(local_x) / max(length * 0.5, 1.0e-6)
        transverse = np.abs(local_y) / max(width * 0.5, 1.0e-6)
        footprint = np.exp(
            -(longitudinal**4) * 2.2 - (transverse**2) * 2.8
        ).astype(np.float32)
        pressure_field = footprint * pressure
        compression += pressure_field
        edge = np.exp(
            -((transverse - 0.72) / 0.105) ** 2
            - (longitudinal**4) * 2.8
        ).astype(np.float32)
        broken = 0.58 + 0.42 * np.sin(
            local_x * (8.0 + index * 0.37) + index * 1.71
        )
        line = np.maximum(line, edge * np.clip(broken, 0.0, 1.0))
        direction[..., 0] += footprint * math.cos(angle)
        direction[..., 1] += footprint * math.sin(angle)
        direction_weight += footprint
    compression = np.clip(0.5 + compression, 0.0, 1.0)
    length = np.linalg.norm(direction, axis=-1, keepdims=True)
    direction /= np.maximum(length, 1.0e-6)
    direction[direction_weight < 0.02] = (1.0, 0.0)
    return compression, np.clip(line, 0.0, 1.0), direction


def generate_plaster_surface(
    *,
    resolution: int,
    seed: int,
    variation: int,
    intent: dict[str, Any],
) -> dict[str, np.ndarray | Any]:
    broad = periodic_fbm_rect(
        resolution, 2, 3, seed + variation * 401, octaves=4, persistence=0.48
    )
    lime_cloud = periodic_fbm_rect(
        resolution, 3, 2, seed + 127 + variation * 409, octaves=3
    )
    set_variation = periodic_fbm_rect(
        resolution, 5, 3, seed + 271 + variation * 419, octaves=3
    )
    aggregate = periodic_fbm_rect(
        resolution, 23, 19, seed + 419 + variation * 431, octaves=2
    )
    pores = periodic_fbm_rect(
        resolution, 41, 37, seed + 563 + variation * 443, octaves=2
    )
    authored_compression, authored_trowel_line, authored_direction = (
        finite_trowel_field(resolution, variation=variation)
    )
    trowel_compression = np.clip(
        0.24 * set_variation + 0.76 * authored_compression,
        0.0,
        1.0,
    )
    aggregate_mask = smoothstep(0.77, 0.92, aggregate) * (
        0.35 + 0.65 * smoothstep(0.30, 0.70, broad)
    )
    pore_mask = smoothstep(0.835, 0.965, pores) * (
        0.25 + 0.75 * aggregate_mask
    )

    shade_coordinate = np.clip(
        0.50
        + (broad - 0.5) * 0.16
        + (lime_cloud - 0.5) * 0.10
        + (trowel_compression - 0.5) * 0.55
        + (set_variation - 0.5) * 0.08
        + (0.5 - aggregate) * 0.04,
        0.24,
        0.76,
    )
    plaster_palette = _intent_palette(
        intent["materials"]["plaster"],
        label="plaster",
    )
    palette_bank = plaster_palette[np.newaxis, ...]
    base_srgb = _sample_palette_field(
        palette_bank,
        np.zeros((resolution, resolution), dtype=np.int32),
        shade_coordinate,
    )
    warm_field = (lime_cloud - 0.5)[..., np.newaxis]
    base_srgb += warm_field * np.array([0.012, 0.006, -0.006], dtype=np.float32)
    base_srgb = mix(
        base_srgb,
        np.broadcast_to(
            plaster_palette[2],
            base_srgb.shape,
        ),
        aggregate_mask * 0.08,
    )

    rest_mask = smoothstep(0.42, 0.70, broad)
    plaster_line = authored_trowel_line * (0.52 + rest_mask * 0.48) * 0.55
    plaster_line += pore_mask * 0.18
    ink_color = (
        np.asarray(
            intent["style_intent"]["line_color_rgb"],
            dtype=np.float32,
        )
        / 255.0
    )
    base_srgb = mix(
        base_srgb,
        np.broadcast_to(ink_color, base_srgb.shape),
        plaster_line * 0.24,
    )
    base_srgb = np.clip(base_srgb, 0.0, 1.0)

    form_height_m = (
        (broad - 0.5) * 0.0016
        + (trowel_compression - 0.5) * 0.0009
        + authored_trowel_line * 0.00018
    ).astype(np.float32)
    detail_height_m = (
        - aggregate_mask * 0.00038
        - pore_mask * 0.00070
    ).astype(np.float32)
    height_m = form_height_m + detail_height_m
    meters_per_pixel = 4.0 / resolution
    normal = height_to_normal(height_m, meters_per_pixel, strength=1.0)
    form_normal = height_to_normal(
        form_height_m,
        meters_per_pixel,
        strength=1.0,
    )
    detail_normal = height_to_normal(
        detail_height_m,
        meters_per_pixel,
        strength=1.0,
    )
    roughness = np.clip(
        0.72
        + aggregate_mask * 0.08
        + pore_mask * 0.06
        - trowel_compression * 0.055,
        0.60,
        0.88,
    )
    ao = np.clip(1.0 - pore_mask * 0.16 - aggregate_mask * 0.04, 0.72, 1.0)
    brush_direction = authored_direction
    detail_priority = np.clip(
        0.18 + plaster_line * 0.72 + aggregate_mask * 0.22, 0.0, 1.0
    )
    return {
        "base_color_srgb": base_srgb,
        "base_color_linear": srgb_to_linear(base_srgb),
        "height_m": height_m,
        "normal": normal,
        "form_height_m": form_height_m,
        "detail_height_m": detail_height_m,
        "form_normal": form_normal,
        "detail_normal": detail_normal,
        "roughness": roughness.astype(np.float32),
        "ao": ao.astype(np.float32),
        "metallic": np.zeros_like(roughness, dtype=np.float32),
        "broad_lime_field": broad,
        "lime_cloud": lime_cloud,
        "trowel_compression": trowel_compression,
        "authored_trowel_event_mask": authored_trowel_line,
        "authored_trowel_event_count": len(TROWEL_STROKES_M),
        "aggregate_mask": aggregate_mask,
        "pore_mask": pore_mask,
        "shade_coordinate": shade_coordinate,
        "line_mask": plaster_line.astype(np.float32),
        "detail_priority": detail_priority.astype(np.float32),
        "brush_direction": brush_direction,
        "shade_palette_srgb": plaster_palette,
    }


def generate_masonry_surface(
    recipe: RubbleRecipe,
    *,
    resolution: int,
    seed: int,
    variation: int,
    intent: dict[str, Any],
) -> dict[str, np.ndarray | Any]:
    layout = generate_stone_layout(recipe, variation=variation)
    raster = rasterize_rubble_pattern(layout, recipe, resolution=resolution)
    stone_id = raster["stone_id"]
    stone_mask = raster["stone_mask"]
    mortar_mask = raster["mortar_mask"]
    edge_distance = raster["edge_distance_m"]
    local_u = raster["local_u"]
    local_v = raster["local_v"]

    broad = periodic_fbm_rect(
        resolution, 3, 4, seed + variation * 503, octaves=3, persistence=0.50
    )
    mineral = periodic_fbm_rect(
        resolution, 9, 7, seed + 181 + variation * 509, octaves=3
    )
    granular = periodic_fbm_rect(
        resolution, 27, 23, seed + 359 + variation * 521, octaves=2
    )
    stone_angle = raster["phase"] * math.tau + raster["face_angle"] * 4.0
    angle_cos = np.cos(stone_angle)
    angle_sin = np.sin(stone_angle)
    rotated_u = (
        (local_u - 0.5) * angle_cos
        - (local_v - 0.5) * angle_sin
        + 0.5
    )
    rotated_v = (
        (local_u - 0.5) * angle_sin
        + (local_v - 0.5) * angle_cos
        + 0.5
    )
    plane_a = 0.42 * rotated_u + 0.10 * rotated_v
    plane_b = -0.24 * rotated_u + 0.34 * rotated_v + 0.22
    plane_c = 0.13 * rotated_u - 0.31 * rotated_v + 0.37
    plane_d = (
        -0.16 * rotated_u
        - 0.09 * rotated_v
        + 0.48
        + raster["face_angle"] * 0.07
    )
    plane_stack = np.stack(
        [plane_a, plane_b, plane_c, plane_d],
        axis=-1,
    )
    facet_identity = np.argmax(plane_stack, axis=-1).astype(np.int32)
    palettes = _stone_shade_palettes(
        layout,
        variation=variation,
        intent=intent,
    )

    champion_by_id = [None] + [stone["champion"] for stone in layout]
    champion_index = {entry["name"]: index for index, entry in enumerate(recipe.champions)}
    champion_id = np.zeros_like(stone_id)
    crown_m = np.zeros_like(stone_mask)
    bevel_m = np.full_like(stone_mask, 0.04)
    facet_strength = np.zeros_like(stone_mask)
    mineral_strength = np.zeros_like(stone_mask)
    line_priority = np.zeros_like(stone_mask)
    stone_roughness = np.full_like(stone_mask, 0.72)
    traversal_mask = np.zeros_like(stone_mask)
    for stone in layout:
        selected = stone_id == stone["stone_id"]
        champion = champion_by_id[stone["stone_id"]]
        champion_id[selected] = champion_index[champion["name"]] + 1
        crown_m[selected] = float(champion["crown_m"]) + stone["relief_m"] * 0.18
        bevel_m[selected] = float(champion["bevel_m"])
        facet_strength[selected] = float(champion["facet_strength"])
        mineral_strength[selected] = float(champion["mineral_strength"])
        line_priority[selected] = float(champion["line_priority"])
        stone_roughness[selected] = float(champion["roughness"])
        if champion["role"] == "traversal":
            traversal_mask[selected] = 1.0

    facet_bias = np.take(
        np.asarray([-0.090, 0.025, 0.082, -0.032], dtype=np.float32),
        facet_identity,
    )
    stone_body_coordinate = np.zeros_like(stone_mask)
    for stone in layout:
        selected = stone_id == stone["stone_id"]
        stone_body_coordinate[selected] = (
            0.50
            + math.sin(float(stone["phase"]) * math.tau) * 0.045
        )
    graphic_coordinate = np.clip(
        stone_body_coordinate
        + facet_bias * facet_strength
        + (broad - 0.5) * 0.080
        + (mineral - 0.5) * mineral_strength * 0.025,
        0.0,
        1.0,
    )
    graphic_coordinate = (
        np.floor(graphic_coordinate * 19.0 + 0.5) / 19.0
    ).astype(np.float32)
    stone_color = _sample_palette_field(
        palettes,
        stone_id,
        graphic_coordinate,
    )
    shade_coordinate = graphic_coordinate

    mineral_band = (
        1.0
        - smoothstep(
            0.028,
            0.095,
            np.abs(
                np.sin(
                    (
                        local_v * 3.2
                        + local_u * 0.34
                        + raster["face_angle"] * 0.8
                    )
                    * math.tau
                )
            ),
        )
    ) * mineral_strength
    inclusion_mask = (
        smoothstep(0.76, 0.91, mineral)
        * smoothstep(0.20, 0.72, mineral_strength)
        * stone_mask
    )
    stone_color = mix(
        stone_color,
        stone_color
        * np.asarray([0.84, 0.87, 0.88], dtype=np.float32),
        mineral_band * 0.24,
    )
    stone_color = mix(
        stone_color,
        np.broadcast_to(
            _intent_palette(
                intent["materials"]["stone"]["families"]["warm_limestone"],
                label="stone/warm_limestone",
            )[15],
            stone_color.shape,
        ),
        inclusion_mask * 0.13,
    )

    mortar_body = np.clip(
        0.46
        + (broad - 0.5) * 0.16
        + (granular - 0.5) * 0.11,
        0.30,
        0.63,
    )
    mortar_coordinate = np.clip(
        (mortar_body - 0.30) / (0.63 - 0.30),
        0.0,
        1.0,
    )
    mortar_palette = _intent_palette(
        intent["materials"]["mortar"],
        label="mortar",
    )
    mortar_srgb = _sample_palette_field(
        mortar_palette[np.newaxis, ...],
        np.zeros((resolution, resolution), dtype=np.int32),
        mortar_coordinate,
    )
    mortar_aggregate = (
        smoothstep(0.74, 0.91, granular) * mortar_mask
    ).astype(np.float32)
    mortar_brush = (
        0.5
        + 0.5
        * np.sin(
            (
                np.linspace(
                    0.0, 1.0, resolution, endpoint=False, dtype=np.float32
                )[:, np.newaxis]
                * 37.0
                + broad * 1.3
            )
            * math.tau
        )
    ) * mortar_mask
    mortar_srgb = mix(
        mortar_srgb,
        mortar_srgb * np.asarray([0.82, 0.84, 0.82], dtype=np.float32),
        mortar_aggregate * 0.24,
    )

    near_edge = (
        stone_mask
        * (1.0 - smoothstep(0.004, 0.036, edge_distance))
    )
    broken = periodic_fbm_rect(
        resolution, 11, 13, seed + 547 + variation * 523, octaves=2
    )
    primary_line = (
        near_edge
        * line_priority
        * smoothstep(0.34, 0.66, broken)
    )
    bedding_line = (
        mineral_band
        * smoothstep(0.50, 0.82, line_priority)
        * smoothstep(0.40, 0.68, broad)
    )
    facet_boundary = (
        (
            (facet_identity != np.roll(facet_identity, 1, axis=0))
            | (facet_identity != np.roll(facet_identity, 1, axis=1))
        ).astype(np.float32)
        * stone_mask
        * smoothstep(0.045, 0.105, edge_distance)
        * smoothstep(0.36, 0.76, line_priority)
        * smoothstep(0.34, 0.70, broken)
    )
    facet_line = (
        (
            1.0
            - smoothstep(
                0.015,
                0.070,
                np.abs(
                    np.sin(
                        (
                            local_u * 1.3
                            - local_v * 1.7
                            + raster["face_angle"]
                        )
                        * math.tau
                    )
                ),
            )
        )
        * facet_strength
        * smoothstep(0.65, 0.86, granular)
        * stone_mask
    )
    facet_line = np.clip(facet_line + facet_boundary * 0.46, 0.0, 1.0)
    ink_mask = np.clip(
        primary_line * 0.88 + bedding_line * 0.34 + facet_line * 0.28,
        0.0,
        1.0,
    )
    ink_color = (
        np.asarray(
            intent["style_intent"]["line_color_rgb"],
            dtype=np.float32,
        )
        / 255.0
    )
    stone_color = mix(
        stone_color,
        np.broadcast_to(ink_color, stone_color.shape),
        ink_mask * 0.52,
    )
    base_srgb = mix(mortar_srgb, stone_color, stone_mask)
    base_srgb = np.clip(base_srgb, 0.0, 1.0)

    bevel_profile = np.clip(
        edge_distance / np.maximum(bevel_m, 1.0e-5), 0.0, 1.0
    )
    bevel_profile = np.minimum(1.0, bevel_profile * 1.22)
    face_plane = (
        (graphic_coordinate - 0.5)
        * facet_strength
        * 0.018
        + (broad - 0.5) * 0.0022
    )
    stone_face_offset = 0.004 + crown_m * 0.30
    stone_height = bevel_profile * stone_face_offset + face_plane
    stone_detail_height = (
        (mineral - 0.5) * mineral_strength * 0.0012
        - inclusion_mask * 0.0009
    )
    mortar_form_height = (
        -float(recipe.mortar["recess_m"])
        + (broad - 0.5) * 0.0018
    )
    mortar_detail_height = (
        + (mortar_brush - 0.5) * 0.0008
        - mortar_aggregate * 0.0007
    )
    form_height_m = (
        stone_height * stone_mask
        + mortar_form_height * mortar_mask
    ).astype(np.float32)
    detail_height_m = (
        stone_detail_height * stone_mask
        + mortar_detail_height * mortar_mask
    ).astype(np.float32)
    height_m = form_height_m + detail_height_m
    meters_per_pixel = recipe.tile_size_m / resolution
    normal = height_to_normal(
        periodic_gaussian_blur(height_m, 0.38),
        meters_per_pixel,
        strength=1.0,
    )
    form_normal = height_to_normal(
        periodic_gaussian_blur(form_height_m, 0.32),
        meters_per_pixel,
        strength=1.0,
    )
    detail_normal = height_to_normal(
        detail_height_m,
        meters_per_pixel,
        strength=1.0,
    )
    contact = stone_mask * (
        1.0 - smoothstep(0.002, 0.058, edge_distance)
    )
    ao = np.clip(
        1.0
        - contact * 0.33
        - mortar_mask * 0.10
        - inclusion_mask * 0.05,
        0.48,
        1.0,
    )
    roughness = np.clip(
        stone_roughness * stone_mask
        + (0.84 + mortar_aggregate * 0.08) * mortar_mask
        + mineral_band * 0.035,
        0.60,
        0.94,
    )
    angle = raster["face_angle"] * 0.38
    brush_direction = np.stack([np.cos(angle), np.sin(angle)], axis=-1)
    detail_priority = np.clip(
        0.12
        + line_priority * 0.48
        + traversal_mask * 0.28
        + ink_mask * 0.24,
        0.0,
        1.0,
    )
    return {
        **raster,
        "layout": layout,
        "champion_id": champion_id,
        "base_color_srgb": base_srgb,
        "base_color_linear": srgb_to_linear(base_srgb),
        "height_m": height_m,
        "normal": normal,
        "form_height_m": form_height_m,
        "detail_height_m": detail_height_m,
        "form_normal": form_normal,
        "detail_normal": detail_normal,
        "roughness": roughness.astype(np.float32),
        "ao": ao.astype(np.float32),
        "metallic": np.zeros_like(roughness, dtype=np.float32),
        "shade_coordinate": shade_coordinate,
        "stone_shade_palettes_srgb": palettes,
        "stone_face_plane": face_plane,
        "facet_identity": facet_identity,
        "facet_boundary_mask": facet_boundary.astype(np.float32),
        "mineral_band_mask": mineral_band.astype(np.float32),
        "inclusion_mask": inclusion_mask.astype(np.float32),
        "mortar_body": mortar_body.astype(np.float32),
        "mortar_shade_palette_srgb": mortar_palette,
        "mortar_aggregate_mask": mortar_aggregate,
        "mortar_brush_mask": mortar_brush.astype(np.float32),
        "primary_line_mask": primary_line.astype(np.float32),
        "secondary_line_mask": bedding_line.astype(np.float32),
        "tertiary_line_mask": facet_line.astype(np.float32),
        "ink_mask": ink_mask.astype(np.float32),
        "traversal_mask": traversal_mask,
        "detail_priority": detail_priority.astype(np.float32),
        "brush_direction": brush_direction.astype(np.float32),
    }


def construction_state_field(
    resolution: int,
    *,
    variation: int = 0,
) -> dict[str, np.ndarray]:
    coordinate = np.linspace(0.0, 1.0, resolution, endpoint=False, dtype=np.float32)
    x, y = np.meshgrid(coordinate, coordinate)
    angle = -0.16 + variation * 0.025
    dx = x - (0.62 + variation * 0.012)
    dy = y - (0.49 - variation * 0.008)
    rotated_x = dx * math.cos(angle) - dy * math.sin(angle)
    rotated_y = dx * math.sin(angle) + dy * math.cos(angle)
    radius = np.sqrt(
        (rotated_x / 0.285) ** 2 + (rotated_y / 0.385) ** 2
    )
    irregularity = (
        np.sin((x * 3.1 + y * 1.4) * math.tau) * 0.055
        + np.sin((x * 7.3 - y * 4.2) * math.tau) * 0.026
        + np.sin((x * 13.0 + y * 9.0) * math.tau) * 0.010
    )
    signed_distance = radius - 1.0 + irregularity
    island_dx = (x - 0.24) / 0.115
    island_dy = (y - 0.76) / 0.145
    island = np.sqrt(island_dx * island_dx + island_dy * island_dy) - 1.0
    island += np.sin((x * 8.0 + y * 5.0) * math.tau) * 0.045
    signed_distance = np.minimum(signed_distance, island)

    state = np.full_like(signed_distance, 0.94)
    state[signed_distance < 0.075] = 0.68
    state[signed_distance < -0.005] = 0.46
    state[signed_distance < -0.090] = 0.25
    state[signed_distance < -0.175] = 0.07
    transition_edge = np.clip(
        1.0 - np.abs(signed_distance) / 0.16, 0.0, 1.0
    )
    coverage = smoothstep(-0.12, 0.10, signed_distance)
    return {
        "state": state.astype(np.float32),
        "signed_distance": signed_distance.astype(np.float32),
        "transition_edge": transition_edge.astype(np.float32),
        "plaster_coverage": coverage.astype(np.float32),
    }


def compose_transition(
    plaster: dict[str, Any],
    masonry: dict[str, Any],
    states: dict[str, np.ndarray],
    intent: dict[str, Any],
) -> dict[str, np.ndarray]:
    state = states["state"]
    measurements = intent["construction_measurements_m"]
    scratch_depth = float(measurements["scratch_coat"])
    brown_depth = scratch_depth + float(measurements["brown_coat"])
    finish_depth = brown_depth + float(measurements["finish_coat"])
    plaster_color = plaster["base_color_srgb"]
    masonry_color = masonry["base_color_srgb"]
    palette_identity = np.zeros(state.shape, dtype=np.int32)
    mortar_color = _sample_palette_field(
        _intent_palette(
            intent["materials"]["mortar"],
            label="mortar",
        )[np.newaxis, ...],
        palette_identity,
        plaster["shade_coordinate"],
    )
    scratch_color = _sample_palette_field(
        _intent_palette(
            intent["materials"]["scratch_coat"],
            label="scratch_coat",
        )[np.newaxis, ...],
        palette_identity,
        plaster["shade_coordinate"],
    )
    scratch_groove = (
        1.0
        - smoothstep(
            0.025,
            0.11,
            np.abs(
                np.sin(
                    (
                        np.linspace(
                            0.0,
                            1.0,
                            state.shape[1],
                            endpoint=False,
                            dtype=np.float32,
                        )[np.newaxis, :]
                        * 19.0
                        + np.linspace(
                            0.0,
                            1.0,
                            state.shape[0],
                            endpoint=False,
                            dtype=np.float32,
                        )[:, np.newaxis]
                        * 1.7
                    )
                    * math.tau
                )
            ),
        )
    )
    scratch_color = mix(
        scratch_color,
        scratch_color * np.asarray([0.70, 0.66, 0.58], dtype=np.float32),
        scratch_groove * 0.28,
    )
    brown_color = _sample_palette_field(
        _intent_palette(
            intent["materials"]["brown_coat"],
            label="brown_coat",
        )[np.newaxis, ...],
        palette_identity,
        plaster["shade_coordinate"],
    )
    finish_color = _sample_palette_field(
        _intent_palette(
            intent["materials"]["finish_coat"],
            label="finish_coat",
        )[np.newaxis, ...],
        palette_identity,
        plaster["shade_coordinate"],
    )
    color = masonry_color.copy()
    color = np.where((state >= 0.18)[..., None], mortar_color, color)
    color = np.where((state >= 0.30)[..., None], scratch_color, color)
    color = np.where((state >= 0.52)[..., None], brown_color, color)
    color = np.where(
        (state >= 0.76)[..., None],
        plaster_color * 0.42 + finish_color * 0.58,
        color,
    )

    height = masonry["height_m"].copy()
    height = np.where(state >= 0.18, -0.001, height)
    height = np.where(
        state >= 0.30,
        plaster["height_m"]
        + scratch_depth
        - scratch_groove * 0.0025,
        height,
    )
    height = np.where(
        state >= 0.52,
        plaster["height_m"] + brown_depth,
        height,
    )
    height = np.where(
        state >= 0.76,
        plaster["height_m"] + finish_depth,
        height,
    )
    normal = height_to_normal(
        periodic_gaussian_blur(height.astype(np.float32), 0.55),
        4.0 / state.shape[0],
        strength=1.0,
    )
    roughness = masonry["roughness"].copy()
    roughness = np.where(state >= 0.18, 0.86, roughness)
    roughness = np.where(state >= 0.30, 0.82, roughness)
    roughness = np.where(state >= 0.52, 0.77, roughness)
    roughness = np.where(state >= 0.76, plaster["roughness"], roughness)
    ao = np.minimum(plaster["ao"], masonry["ao"])
    return {
        "base_color_srgb": np.clip(color, 0.0, 1.0),
        "base_color_linear": srgb_to_linear(np.clip(color, 0.0, 1.0)),
        "height_m": height.astype(np.float32),
        "normal": normal,
        "roughness": roughness.astype(np.float32),
        "ao": ao.astype(np.float32),
        "scratch_groove": scratch_groove.astype(np.float32),
    }


def generate_material(
    *,
    resolution: int = 1024,
    seed: int = 73129,
    pattern_variation: int = 0,
    recipe_path: str | Path = DEFAULT_PATTERN,
    intent_path: str | Path = DEFAULT_MATERIAL_INTENT,
) -> dict[str, Any]:
    recipe = load_rubble_recipe(recipe_path)
    intent = load_material_intent(intent_path)
    plaster = generate_plaster_surface(
        resolution=resolution,
        seed=seed,
        variation=pattern_variation,
        intent=intent,
    )
    masonry = generate_masonry_surface(
        recipe,
        resolution=resolution,
        seed=seed + 1009,
        variation=pattern_variation,
        intent=intent,
    )
    states = construction_state_field(
        resolution, variation=pattern_variation
    )
    transition = compose_transition(plaster, masonry, states, intent)
    return {
        "schema": MATERIAL_SCHEMA,
        "recipe": recipe,
        "resolution": resolution,
        "seed": seed,
        "pattern_variation": pattern_variation,
        "material_intent": intent,
        "material_intent_path": Path(intent_path),
        "plaster": plaster,
        "masonry": masonry,
        "transition_states": states,
        "transition": transition,
        "shade_family_size": 20,
    }


def _normal_to_rgb(normal: np.ndarray) -> np.ndarray:
    return np.clip(normal * 0.5 + 0.5, 0.0, 1.0)


def _state_false_color(state: np.ndarray) -> np.ndarray:
    colors = np.asarray(
        [
            [0.22, 0.32, 0.39],
            [0.53, 0.47, 0.34],
            [0.58, 0.36, 0.23],
            [0.72, 0.55, 0.30],
            [0.88, 0.82, 0.62],
        ],
        dtype=np.float32,
    )
    result = np.zeros((*state.shape, 3), dtype=np.float32)
    result[state < 0.18] = colors[0]
    result[np.logical_and(state >= 0.18, state < 0.30)] = colors[1]
    result[np.logical_and(state >= 0.30, state < 0.52)] = colors[2]
    result[np.logical_and(state >= 0.52, state < 0.76)] = colors[3]
    result[state >= 0.76] = colors[4]
    return result


def _proof_pages(material: dict[str, Any]) -> dict[str, np.ndarray]:
    plaster = material["plaster"]
    masonry = material["masonry"]
    states = material["transition_states"]
    transition = material["transition"]
    target = 288

    plaster_preview = render_material_preview(
        plaster["base_color_linear"],
        plaster["normal"],
        plaster["roughness"],
        plaster["ao"],
    )
    masonry_preview = render_material_preview(
        masonry["base_color_linear"],
        masonry["normal"],
        masonry["roughness"],
        masonry["ao"],
    )
    transition_preview = render_material_preview(
        transition["base_color_linear"],
        transition["normal"],
        transition["roughness"],
        transition["ao"],
    )

    construction = compose_material_chapter(
        "CONSTRUCTION BEFORE SURFACE NOISE",
        "THREE PLASTER COATS, FLUSH LIME MORTAR, AND THIRTY-SIX AUTHORED GIANT STONES",
        [
            ("LIME BODY", proof_resize(plaster["base_color_srgb"], target)),
            ("TROWEL COMPRESSION", gray_rgb(proof_resize(plaster["trowel_compression"], target))),
            ("SPARSE AGGREGATE", gray_rgb(proof_resize(plaster["aggregate_mask"], target))),
            ("RUBBLE IDENTITY", scalar_tint(proof_resize(masonry["stone_id"].astype(np.float32), target), (0.12, 0.15, 0.18), (0.86, 0.66, 0.32), normalize=True)),
            ("STONE VS MORTAR", gray_rgb(proof_resize(masonry["stone_mask"], target))),
            ("FLUSH MORTAR BODY", scalar_tint(proof_resize(masonry["mortar_body"], target), (0.26, 0.24, 0.20), (0.72, 0.68, 0.56))),
            ("PLANAR STONE FORM", scalar_tint(proof_resize(masonry["stone_face_plane"], target), (0.18, 0.28, 0.40), (0.86, 0.65, 0.35), normalize=True)),
            ("COMBINED RESPONSE", proof_resize(masonry_preview, target)),
        ],
        [
            "The plaster is a material stack, not a generic grunge field. Broad lime territories carry the color read; trowel and aggregate lanes remain subordinate.",
            "The rubble bond is authored in metres. Course heights range from 0.45 to 0.70 m and stone widths from 0.38 to 1.14 m.",
            "Mortar is a separate brushed, open-grained material state. Stone form is built from edge distance, crown, planar facets, and areas of rest.",
        ],
    )

    color = compose_material_chapter(
        "LAYERED COLOR, NOT ONE SHADE PER BLOCK",
        "EACH STONE AND THE PLASTER FIELD CONTINUOUSLY TRAVERSE TWENTY CONTROL SHADES",
        [
            ("PLASTER 20-SHADE FAMILY", palette_panel(plaster["shade_palette_srgb"], resolution=target)),
            ("PLASTER SHADE FIELD", scalar_tint(proof_resize(plaster["shade_coordinate"], target), (0.28, 0.27, 0.24), (0.89, 0.84, 0.69))),
            ("PLASTER UNLIT", proof_resize(plaster["base_color_srgb"], target)),
            ("PLASTER LIT", proof_resize(plaster_preview, target)),
            ("FOCAL STONE FAMILY", palette_panel(masonry["stone_shade_palettes_srgb"][18], resolution=target)),
            ("STONE SHADE FIELD", scalar_tint(proof_resize(masonry["shade_coordinate"], target), (0.20, 0.27, 0.34), (0.87, 0.68, 0.39))),
            ("MASONRY UNLIT", proof_resize(masonry["base_color_srgb"], target)),
            ("MASONRY LIT", proof_resize(masonry_preview, target)),
        ],
        [
            "One stone does not receive one flat swatch. Planar orientation, mineral bedding, lime cloud, compression, and edge state continuously select within its shade family.",
            "Warm and cool shifts describe different mineral bodies while the wall remains one restrained family. Saturation is kept below the approved oak and iron accents.",
            "The unlit panels contain no directional scene lighting; the lit panels prove the same pigment survives a mobile light response.",
        ],
    )

    transition_page = compose_material_chapter(
        "THE REVEAL IS A CONSTRUCTION SECTION",
        "GEOMETRY OWNS STATE; THE SHADER RECONSTRUCTS FINISH, BROWN, SCRATCH, MORTAR, AND STONE",
        [
            ("COMBINED REVEAL", proof_resize(transition_preview, target)),
            ("FIVE MATERIAL STATES", proof_resize(_state_false_color(states["state"]), target)),
            ("PLASTER COVERAGE", gray_rgb(proof_resize(states["plaster_coverage"], target))),
            ("TRANSITION EDGE", gray_rgb(proof_resize(states["transition_edge"], target))),
            ("SCRATCH GROOVES", gray_rgb(proof_resize(transition["scratch_groove"], target))),
            ("MORTAR MASK", gray_rgb(proof_resize(masonry["mortar_mask"], target))),
            ("TRAVERSAL ID", scalar_tint(proof_resize(masonry["traversal_mask"], target), (0.05, 0.07, 0.08), (0.24, 0.82, 0.78))),
            ("REVEAL CLOSE-UP", proof_resize(zoom_square(transition_preview, center_u=0.62, center_v=0.49, fraction=0.42), target)),
        ],
        [
            "The reveal boundary is not stored inside a square texture. The fixture writes material state, edge emphasis, and traversal identity as geometry attributes.",
            "The five bands follow real coat order. Their visible widths are art-directed for giant scale but their depth order remains physically coherent.",
            "Traversal identity is a separate channel. In production it may tint a route, but exposed footholds still require projecting geometry and a readable silhouette.",
        ],
    )

    linework = compose_material_chapter(
        "SELECTIVE LINEWORK SUPPORTS FORM",
        "PRIMARY ARRIS FRAGMENTS, BEDDING LINES, FACET MARKS, AND TROWEL DRAGS REMAIN SEPARATE",
        [
            ("STONE PRIMARY", gray_rgb(proof_resize(masonry["primary_line_mask"], target))),
            ("STONE SECONDARY", gray_rgb(proof_resize(masonry["secondary_line_mask"], target))),
            ("STONE TERTIARY", gray_rgb(proof_resize(masonry["tertiary_line_mask"], target))),
            ("STONE INK", gray_rgb(proof_resize(masonry["ink_mask"], target))),
            ("PLASTER LINE", gray_rgb(proof_resize(plaster["line_mask"], target))),
            ("DETAIL PRIORITY", scalar_tint(proof_resize(masonry["detail_priority"], target), (0.08, 0.09, 0.11), (0.95, 0.70, 0.27))),
            ("UNLIT COLOR", proof_resize(masonry["base_color_srgb"], target)),
            ("LIT FORM", proof_resize(masonry_preview, target)),
        ],
        [
            "Primary lines are broken stone-arris selections, not a universal edge detector. Secondary and tertiary marks need authored champion priority before they appear.",
            "The ink is a restrained brown-violet embedded lightly in base color and exported independently for engine tuning.",
            "Quiet stones stay quiet. Traversal ledges and fractured champions receive stronger form emphasis without turning every edge black.",
        ],
    )

    frontal = render_material_single_light(
        transition["base_color_linear"],
        transition["normal"],
        transition["roughness"],
        transition["ao"],
        direction=(0.0, 0.0, 1.0),
        intensity=2.0,
    )
    side = render_material_single_light(
        transition["base_color_linear"],
        transition["normal"],
        transition["roughness"],
        transition["ao"],
        direction=(-0.72, 0.0, 0.69),
        intensity=2.2,
    )
    grazing = render_material_single_light(
        transition["base_color_linear"],
        transition["normal"],
        transition["roughness"],
        transition["ao"],
        direction=(0.90, 0.0, 0.43),
        intensity=2.5,
    )
    response = compose_material_chapter(
        "PBR RESPONSE STAYS MOBILE",
        "COLOR REMAINS UNLIT WHILE HEIGHT, NORMAL, ROUGHNESS, AND AO DESCRIBE THE MATERIAL",
        [
            ("TRANSITION UNLIT", proof_resize(transition["base_color_srgb"], target)),
            ("TRANSITION NORMAL", proof_resize(_normal_to_rgb(transition["normal"]), target)),
            ("ROUGHNESS", gray_rgb(proof_resize(transition["roughness"], target))),
            ("AMBIENT OCCLUSION", gray_rgb(proof_resize(transition["ao"], target))),
            ("FRONTAL WHITE", proof_resize(frontal, target)),
            ("SIDE WHITE", proof_resize(side, target)),
            ("GRAZING WHITE", proof_resize(grazing, target)),
            ("THREE-LIGHT PREVIEW", proof_resize(transition_preview, target)),
        ],
        [
            "The finish coat is comparatively smooth but still matte. Coarse coats and brushed mortar are rougher; recessed mortar and stone contacts carry the strongest occlusion.",
            "Grazing light reveals trowel compression, coat steps, mortar recess, stone crown, and planar facets without requiring photo noise.",
            "Optional damp and soot are zero in the canonical output. They belong to scene-authored narrative overlays.",
        ],
    )

    plaster_tile = np.tile(
        proof_resize(plaster["base_color_srgb"], target // 2), (2, 2, 1)
    )
    masonry_tile = np.tile(
        proof_resize(masonry["base_color_srgb"], target // 2), (2, 2, 1)
    )
    plaster_seam = seam_frame(proof_resize(plaster["base_color_srgb"], target))
    masonry_seam = seam_frame(proof_resize(masonry["base_color_srgb"], target))
    variation_a = np.roll(
        proof_resize(transition_preview, target), target // 5, axis=1
    )
    variation_b = np.clip(
        np.roll(proof_resize(transition_preview, target), target // 3, axis=0)
        * np.asarray([0.97, 1.00, 1.035], dtype=np.float32),
        0.0,
        1.0,
    )
    scale = compose_material_chapter(
        "FOUR-METRE SCALE AND BOUNDED VARIATION",
        "PHASE AND PIGMENT MAY CHANGE; COURSE CONSTRUCTION AND MATERIAL CAUSALITY MAY NOT",
        [
            ("PLASTER 2X2", plaster_tile),
            ("MASONRY 2X2", masonry_tile),
            ("PLASTER SEAM", plaster_seam),
            ("MASONRY SEAM", masonry_seam),
            ("VARIANT PHASE A", variation_a),
            ("VARIANT PHASE B", variation_b),
            ("PLASTER AREA OF REST", proof_resize(zoom_square(plaster_preview, center_u=0.28, center_v=0.34, fraction=0.38), target)),
            ("STONE AREA OF REST", proof_resize(zoom_square(masonry_preview, center_u=0.48, center_v=0.30, fraction=0.38), target)),
        ],
        [
            "The tile represents 4 x 4 metres. Large fields remain legible at gameplay distance while the 0.38-1.14 m stone widths reinforce giant scale.",
            "Variation is bounded to coordinate phase, champion pigment modulation, and reveal placement. It never discards the hand-laid bond.",
            "The deliberately calm zooms are acceptance evidence: frequency hierarchy requires quiet space between selected trowel, mineral, and line accents.",
        ],
    )
    return {
        "construction": construction,
        "color": color,
        "transition": transition_page,
        "linework": linework,
        "response": response,
        "scale": scale,
    }


def _encode_normal(normal: np.ndarray) -> np.ndarray:
    return np.clip(normal * 0.5 + 0.5, 0.0, 1.0)


def _encode_height(height: np.ndarray) -> tuple[np.ndarray, tuple[float, float]]:
    low = float(height.min())
    high = float(height.max())
    encoded = (height - low) / max(high - low, 1.0e-9)
    return encoded.astype(np.float32), (low, high)


def _hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _seam_metric(value: np.ndarray) -> float:
    if value.ndim == 3:
        vertical = np.mean(np.abs(value[0] - value[-1]))
        horizontal = np.mean(np.abs(value[:, 0] - value[:, -1]))
    else:
        vertical = np.mean(np.abs(value[0] - value[-1]))
        horizontal = np.mean(np.abs(value[:, 0] - value[:, -1]))
    return float(max(vertical, horizontal))


def write_material_package(
    material: dict[str, Any],
    *,
    output_root: str | Path = DEFAULT_OUTPUT,
    pattern_path: str | Path = DEFAULT_PATTERN,
    profile_path: str | Path = DEFAULT_PROFILE,
) -> dict[str, Any]:
    output = Path(output_root)
    proofs_dir = output / "proofs"
    output.mkdir(parents=True, exist_ok=True)
    proofs_dir.mkdir(parents=True, exist_ok=True)
    plaster = material["plaster"]
    masonry = material["masonry"]

    files = {
        "plaster_basecolor": output / "lime_plaster_v1_basecolor.png",
        "plaster_normal": output / "lime_plaster_v1_normal.png",
        "plaster_form_normal": output / "lime_plaster_v1_form_normal.png",
        "plaster_detail_normal": output / "lime_plaster_v1_detail_normal.png",
        "plaster_orm": output / "lime_plaster_v1_orm.png",
        "plaster_height": output / "lime_plaster_v1_height.png",
        "masonry_basecolor": output / "giant_masonry_v1_basecolor.png",
        "masonry_normal": output / "giant_masonry_v1_normal.png",
        "masonry_form_normal": output / "giant_masonry_v1_form_normal.png",
        "masonry_detail_normal": output / "giant_masonry_v1_detail_normal.png",
        "masonry_orm": output / "giant_masonry_v1_orm.png",
        "masonry_height": output / "giant_masonry_v1_height.png",
        "masonry_masks": output / "giant_masonry_v1_masks.png",
        "stylization": output / "lime_plaster_masonry_v1_stylization.png",
        "brush_direction": output / "lime_plaster_masonry_v1_brush_direction.png",
        "manifest": output / "lime_plaster_masonry_v1_manifest.json",
    }
    plaster_height, plaster_height_range = _encode_height(plaster["height_m"])
    masonry_height, masonry_height_range = _encode_height(masonry["height_m"])
    plaster_orm = np.stack(
        [plaster["ao"], plaster["roughness"], plaster["metallic"]], axis=-1
    )
    masonry_orm = np.stack(
        [masonry["ao"], masonry["roughness"], masonry["metallic"]], axis=-1
    )
    masonry_masks = np.stack(
        [
            masonry["mortar_mask"],
            masonry["ink_mask"],
            masonry["traversal_mask"],
        ],
        axis=-1,
    )
    stylization = np.stack(
        [
            plaster["line_mask"],
            masonry["ink_mask"],
            np.maximum(
                plaster["detail_priority"], masonry["detail_priority"]
            ),
        ],
        axis=-1,
    )
    brush = plaster["brush_direction"] * plaster["detail_priority"][..., None]
    brush += masonry["brush_direction"] * masonry["stone_mask"][..., None]
    length = np.linalg.norm(brush, axis=-1, keepdims=True)
    brush /= np.maximum(length, 1.0e-6)
    brush_encoded = np.concatenate(
        [brush * 0.5 + 0.5, np.zeros((*brush.shape[:2], 1), dtype=np.float32)],
        axis=-1,
    )

    write_png_rgb8(files["plaster_basecolor"], plaster["base_color_srgb"])
    write_png_rgb8(files["plaster_normal"], _encode_normal(plaster["normal"]))
    write_png_rgb8(
        files["plaster_form_normal"],
        _encode_normal(plaster["form_normal"]),
    )
    write_png_rgb8(
        files["plaster_detail_normal"],
        _encode_normal(plaster["detail_normal"]),
    )
    write_png_rgb8(files["plaster_orm"], plaster_orm)
    write_png_gray16(files["plaster_height"], plaster_height)
    write_png_rgb8(files["masonry_basecolor"], masonry["base_color_srgb"])
    write_png_rgb8(files["masonry_normal"], _encode_normal(masonry["normal"]))
    write_png_rgb8(
        files["masonry_form_normal"],
        _encode_normal(masonry["form_normal"]),
    )
    write_png_rgb8(
        files["masonry_detail_normal"],
        _encode_normal(masonry["detail_normal"]),
    )
    write_png_rgb8(files["masonry_orm"], masonry_orm)
    write_png_gray16(files["masonry_height"], masonry_height)
    write_png_rgb8(files["masonry_masks"], masonry_masks)
    write_png_rgb8(files["stylization"], stylization)
    write_png_rgb8(files["brush_direction"], brush_encoded)

    proof_files: dict[str, Path] = {}
    for name, page in _proof_pages(material).items():
        path = proofs_dir / f"lime_plaster_masonry_v1_{name}.png"
        write_png_rgb8(path, page)
        proof_files[name] = path

    recipe = material["recipe"]
    stone_widths = [
        stone["x1_m"] - stone["x0_m"] for stone in masonry["layout"]
    ]
    course_heights = [
        float(course["top_m"]) - float(course["bottom_m"])
        for course in recipe.courses
    ]
    intent_path = Path(material["material_intent_path"])
    intent = material["material_intent"]
    source_files = [Path(pattern_path), Path(profile_path), intent_path]
    manifest = {
        "schema": MATERIAL_SCHEMA,
        "material_id": "lime_plaster_masonry_v1",
        "resolution": material["resolution"],
        "seed": material["seed"],
        "pattern_variation": material["pattern_variation"],
        "tile_size_m": recipe.tile_size_m,
        "stone_count": masonry["stone_count"],
        "shade_family_size": material["shade_family_size"],
        "authored_scale_m": {
            "stone_width_min": min(stone_widths),
            "stone_width_max": max(stone_widths),
            "course_height_min": min(course_heights),
            "course_height_max": max(course_heights),
        },
        "height_range_m": {
            "plaster": list(plaster_height_range),
            "masonry": list(masonry_height_range),
        },
        "transition_states": [
            "masonry",
            "flush_lime_mortar",
            "scratched_coarse_coat",
            "brown_float_coat",
            "lime_finish",
        ],
        "geometry_attributes": json.loads(Path(profile_path).read_text())[
            "coordinate_contract"
        ],
        "default_overlays": {
            "damp": 0.0,
            "soot": 0.0,
            "edge_accumulation": 0.0,
        },
        "constraints": {
            "uses_ai_generated_reference": False,
            "uses_sampled_reference_pixels": False,
            "categorical_state_is_face_domain": True,
            "form_and_detail_normals_are_separate": True,
            "height_is_authored_in_metres": True,
            "narrative_overlays_default_off": True,
            "unreal_runtime_parity_verified": False,
        },
        "metrics": {
            "plaster_basecolor_seam": _seam_metric(plaster["base_color_srgb"]),
            "masonry_basecolor_seam": _seam_metric(masonry["base_color_srgb"]),
            "plaster_roughness_range": [
                float(plaster["roughness"].min()),
                float(plaster["roughness"].max()),
            ],
            "masonry_roughness_range": [
                float(masonry["roughness"].min()),
                float(masonry["roughness"].max()),
            ],
            "plaster_line_coverage": float(
                (plaster["line_mask"] > 0.10).mean()
            ),
            "masonry_ink_coverage": float(
                (masonry["ink_mask"][masonry["stone_mask"] > 0.5] > 0.12).mean()
            ),
            "traversal_stone_fraction": float(
                masonry["traversal_mask"].mean()
            ),
            "authored_style_intent": intent["style_intent"],
            "authored_trowel_event_count": plaster[
                "authored_trowel_event_count"
            ],
        },
        "material_intent": {
            "schema": intent["schema"],
            "path": str(intent_path),
            "sha256": _hash_file(intent_path),
            "source_kind": intent["source"]["kind"],
            "uses_ai_generated_reference": intent["source"][
                "uses_ai_generated_reference"
            ],
            "uses_sampled_reference_pixels": intent["source"][
                "uses_sampled_reference_pixels"
            ],
            "authored_material_lanes": [
                "plaster",
                "mortar",
                "finish_coat",
                "brown_coat",
                "scratch_coat",
                "stone/cool_limestone",
                "stone/warm_limestone",
                "stone/grey_green",
                "stone/umber_vein",
                "stone/pale_chalk",
                "linework",
            ],
            "raw_pixels_used_as_runtime_texture": intent[
                "production_translation"
            ]["raw_pixels_used_as_runtime_texture"],
        },
        "sources": {
            path.name: {
                "path": str(path),
                "sha256": _hash_file(path),
            }
            for path in source_files
        },
        "outputs": {
            name: str(path.relative_to(output))
            for name, path in {**files, **{f"proof_{key}": value for key, value in proof_files.items()}}.items()
            if name != "manifest"
        },
        "source_policy": {
            "ai_generated_reference_capture": False,
            "ai_generated_runtime_texture": False,
            "raw_reference_pixels_used_as_runtime_texture": False,
            "third_party_pixels_stored": False,
            "legacy_ai_experiments_are_build_inputs": False,
        },
    }
    files["manifest"].write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    return manifest


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate layered lime plaster and giant rubble masonry."
    )
    parser.add_argument("--resolution", type=int, default=1024)
    parser.add_argument("--seed", type=int, default=73129)
    parser.add_argument("--pattern-variation", type=int, default=0)
    parser.add_argument("--pattern", type=Path, default=DEFAULT_PATTERN)
    parser.add_argument("--profile", type=Path, default=DEFAULT_PROFILE)
    parser.add_argument("--intent", type=Path, default=DEFAULT_MATERIAL_INTENT)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args(argv)


def main() -> None:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    args = parse_args(argv)
    material = generate_material(
        resolution=args.resolution,
        seed=args.seed,
        pattern_variation=args.pattern_variation,
        recipe_path=args.pattern,
        intent_path=args.intent,
    )
    manifest = write_material_package(
        material,
        output_root=args.output_root,
        pattern_path=args.pattern,
        profile_path=args.profile,
    )
    print(
        "lime_plaster_masonry_v1 generated:",
        manifest["resolution"],
        "px,",
        manifest["stone_count"],
        "authored stones,",
        len(manifest["outputs"]),
        "outputs",
    )


if __name__ == "__main__":
    main()
