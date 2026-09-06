"""Generate authored kiln-fired brick masonry with explicit stylization lanes."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import sys
from typing import Any

import numpy as np

MATERIALS_ROOT = Path(__file__).resolve().parents[1]
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
    periodic_fbm,
    periodic_fbm_rect,
    periodic_gaussian_blur,
    proof_resize,
    render_material_preview,
    render_material_single_light,
    scalar_tint,
    smoothstep,
    srgb_to_linear,
    write_png_gray16,
    write_png_rgb8,
    zoom_square,
)


MATERIAL_SCHEMA = "iggy3d.material.brick_masonry_v1.v1"
PATTERN_SCHEMA = "iggy3d.pattern.brick_masonry.v1"
DEFAULT_SEED = 88421
DEFAULT_RESOLUTION = 1024
DEFAULT_TILE_SIZE_M = 1.6
DEFAULT_VARIATION = 0
SHADE_FAMILY_SIZE = 20
DEFAULT_RECIPE_PATH = (
    Path(__file__).resolve().parent
    / "patterns"
    / "kiln_fired_running_bond_v1.json"
)

KILN_BASE_SRGB = {
    "rose": (168, 78, 49),
    "ochre": (184, 112, 57),
    "sienna": (151, 68, 41),
    "russet": (132, 57, 39),
    "umber": (106, 59, 45),
    "violet": (91, 54, 57),
}
INK_SRGB = (54, 30, 30)
MORTAR_DARK_SRGB = (93, 82, 69)
MORTAR_LIGHT_SRGB = (158, 146, 126)
PROOF_BRICK_ID = 73


class BrickRecipe:
    def __init__(
        self,
        *,
        schema: str,
        name: str,
        description: str,
        tile_size_m: float,
        columns: int,
        mortar: dict[str, Any],
        variation: dict[str, float],
        courses: list[dict[str, float]],
        champions: list[dict[str, Any]],
        placement: list[list[int]],
        references: list[dict[str, str]],
        source_path: str,
    ) -> None:
        self.schema = schema
        self.name = name
        self.description = description
        self.tile_size_m = tile_size_m
        self.columns = columns
        self.mortar = mortar
        self.variation = variation
        self.courses = courses
        self.champions = champions
        self.placement = placement
        self.references = references
        self.source_path = source_path


def load_brick_recipe(path: str | Path) -> BrickRecipe:
    source_path = Path(path)
    payload = json.loads(source_path.read_text(encoding="utf-8"))
    if payload.get("schema") != PATTERN_SCHEMA:
        raise ValueError(
            f"{source_path} must use schema {PATTERN_SCHEMA!r}"
        )
    name = str(payload.get("name", "")).strip()
    if not name:
        raise ValueError(f"{source_path} is missing a name")
    tile_size_m = float(payload.get("tile_size_m", 0.0))
    columns = int(payload.get("columns", 0))
    if tile_size_m <= 0.0 or columns < 2:
        raise ValueError("brick recipe needs positive size and columns")
    mortar = payload.get("mortar")
    variation = payload.get("variation")
    courses = payload.get("courses")
    champions = payload.get("champions")
    placement = payload.get("placement")
    if not isinstance(mortar, dict) or not isinstance(variation, dict):
        raise ValueError("brick recipe needs mortar and variation objects")
    if not isinstance(courses, list) or len(courses) < 4:
        raise ValueError("brick recipe needs at least four courses")
    if not isinstance(champions, list) or len(champions) < 8:
        raise ValueError("brick recipe needs at least eight champions")
    if not isinstance(placement, list) or len(placement) != len(courses):
        raise ValueError("placement must have one row per course")
    for course_index, row in enumerate(placement):
        if not isinstance(row, list) or len(row) != columns:
            raise ValueError(
                f"placement row {course_index} must contain {columns} entries"
            )
        for champion_id in row:
            if not 0 <= int(champion_id) < len(champions):
                raise ValueError("placement references unknown champion")
    required_champion_fields = {
        "name",
        "face_role",
        "kiln_family",
        "width_scale",
        "height_scale",
        "corner_round_mm",
        "crown_mm",
        "lamination_strength",
        "compression_strength",
        "pit_strength",
        "mineral_strength",
        "fired_skin_strength",
        "line_priority",
        "brush_angle_deg",
    }
    for champion in champions:
        missing = required_champion_fields - set(champion)
        if missing:
            raise ValueError(
                f"champion {champion.get('name', '?')} misses {sorted(missing)}"
            )
        if champion["kiln_family"] not in KILN_BASE_SRGB:
            raise ValueError("champion uses an unknown kiln family")
    height_weights = np.asarray(
        [float(course["height_weight"]) for course in courses],
        dtype=np.float32,
    )
    if np.any(height_weights <= 0.0):
        raise ValueError("course height weights must be positive")
    return BrickRecipe(
        schema=PATTERN_SCHEMA,
        name=name,
        description=str(payload.get("description", "")),
        tile_size_m=tile_size_m,
        columns=columns,
        mortar=dict(mortar),
        variation={
            str(key): float(value) for key, value in variation.items()
        },
        courses=[dict(course) for course in courses],
        champions=[dict(champion) for champion in champions],
        placement=[
            [int(champion_id) for champion_id in row] for row in placement
        ],
        references=[
            dict(reference) for reference in payload.get("references", [])
        ],
        source_path=str(source_path.resolve()),
    )


def generate_brick_layout(
    recipe: BrickRecipe,
    *,
    variation: int = DEFAULT_VARIATION,
) -> dict[str, Any]:
    if variation < 0:
        raise ValueError("pattern variation must be non-negative")
    course_count = len(recipe.courses)
    brick_count = course_count * recipe.columns
    height_weights = np.asarray(
        [course["height_weight"] for course in recipe.courses],
        dtype=np.float32,
    )
    course_heights = height_weights / float(height_weights.sum())
    course_boundaries = np.concatenate(
        [np.asarray([0.0], dtype=np.float32), np.cumsum(course_heights)]
    )
    course_boundaries[-1] = 1.0
    course_offsets = np.asarray(
        [course["offset_fraction"] for course in recipe.courses],
        dtype=np.float32,
    )
    champion_id = np.asarray(recipe.placement, dtype=np.int32).copy()
    rng = np.random.default_rng(DEFAULT_SEED + variation * 1699)
    if variation > 0:
        course_offsets = np.mod(
            course_offsets
            + rng.uniform(
                -recipe.variation["course_offset_fraction"],
                recipe.variation["course_offset_fraction"],
                course_count,
            ),
            1.0,
        ).astype(np.float32)
        max_rotation = max(
            1,
            int(round(recipe.variation["champion_rotation"])),
        )
        for course_index in range(course_count):
            rotation = int(
                rng.integers(-max_rotation, max_rotation + 1)
            )
            champion_id[course_index] = np.roll(
                champion_id[course_index],
                rotation,
            )
    brick_value = rng.uniform(
        1.0 - recipe.variation["brick_value"],
        1.0 + recipe.variation["brick_value"],
        brick_count,
    ).astype(np.float32)
    brick_temperature = rng.uniform(-1.0, 1.0, brick_count).astype(
        np.float32
    )
    brick_phase = rng.random((brick_count, 4), dtype=np.float32)
    edge_side_weight = rng.uniform(
        0.08,
        1.0,
        (brick_count, 4),
    ).astype(np.float32)
    edge_side_weight[
        np.arange(brick_count),
        rng.integers(0, 4, brick_count),
    ] *= 0.18
    edge_side_weight[
        np.arange(brick_count),
        rng.integers(0, 4, brick_count),
    ] = np.maximum(
        edge_side_weight[
            np.arange(brick_count),
            rng.integers(0, 4, brick_count),
        ],
        0.72,
    )
    line_width_m = rng.uniform(
        0.00125,
        0.00265,
        brick_count,
    ).astype(np.float32)
    pit_centers = rng.uniform(
        [-0.34, -0.26],
        [0.34, 0.26],
        (brick_count, 3, 2),
    ).astype(np.float32)
    pit_radius_m = rng.uniform(
        0.0012,
        0.0046,
        (brick_count, 3),
    ).astype(np.float32)
    mineral_centers = rng.uniform(
        [-0.38, -0.29],
        [0.38, 0.29],
        (brick_count, 2, 2),
    ).astype(np.float32)
    mineral_radius_m = rng.uniform(
        0.0010,
        0.0034,
        (brick_count, 2),
    ).astype(np.float32)
    return {
        "course_count": course_count,
        "brick_count": brick_count,
        "course_boundaries": course_boundaries,
        "course_heights": course_heights,
        "course_offsets": course_offsets,
        "champion_id": champion_id,
        "brick_value": brick_value,
        "brick_temperature": brick_temperature,
        "brick_phase": brick_phase,
        "edge_side_weight": edge_side_weight,
        "line_width_m": line_width_m,
        "pit_centers": pit_centers,
        "pit_radius_m": pit_radius_m,
        "mineral_centers": mineral_centers,
        "mineral_radius_m": mineral_radius_m,
        "variation": variation,
    }


def _champion_values(
    recipe: BrickRecipe,
    field: str,
    *,
    dtype: Any = np.float32,
) -> np.ndarray:
    return np.asarray(
        [champion[field] for champion in recipe.champions],
        dtype=dtype,
    )


def rasterize_brick_pattern(
    layout: dict[str, Any],
    recipe: BrickRecipe,
    *,
    resolution: int,
) -> dict[str, Any]:
    if resolution < 32:
        raise ValueError("resolution must be at least 32")
    coordinate = np.arange(resolution, dtype=np.float32) / resolution
    u, v = np.meshgrid(coordinate, coordinate)
    course_id = np.searchsorted(
        layout["course_boundaries"][1:],
        v,
        side="right",
    )
    course_id = np.minimum(
        course_id,
        layout["course_count"] - 1,
    ).astype(np.int32)
    course_bottom = layout["course_boundaries"][course_id]
    course_height_uv = layout["course_heights"][course_id]
    local_y = (v - course_bottom) / course_height_uv - 0.5
    x_scaled = np.mod(
        u * recipe.columns + layout["course_offsets"][course_id],
        recipe.columns,
    )
    column_id = np.floor(x_scaled).astype(np.int32)
    local_x = x_scaled - np.floor(x_scaled) - 0.5
    brick_id = course_id * recipe.columns + column_id
    champion_id = layout["champion_id"][course_id, column_id]

    width_scale = _champion_values(recipe, "width_scale")[champion_id]
    height_scale = _champion_values(recipe, "height_scale")[champion_id]
    corner_round_m = (
        _champion_values(recipe, "corner_round_mm")[champion_id] * 0.001
    )
    phase = layout["brick_phase"][brick_id]
    x_wobble = (
        np.sin((local_y + 0.5) * math.tau * 1.9 + phase[..., 0] * math.tau)
        * 0.0045
        + np.sin(
            (local_y + 0.5) * math.tau * 4.1
            + phase[..., 1] * math.tau
        )
        * 0.0018
    )
    y_wobble = (
        np.sin((local_x + 0.5) * math.tau * 1.4 + phase[..., 2] * math.tau)
        * 0.010
        + np.sin(
            (local_x + 0.5) * math.tau * 3.3
            + phase[..., 3] * math.tau
        )
        * 0.003
    )
    warped_x = local_x + x_wobble
    warped_y = local_y + y_wobble
    pitch_x_m = recipe.tile_size_m / recipe.columns
    course_height_m = course_height_uv * recipe.tile_size_m
    half_width_m = width_scale * pitch_x_m * 0.5
    half_height_m = height_scale * course_height_m * 0.5
    qx = np.abs(warped_x) * pitch_x_m - (
        half_width_m - corner_round_m
    )
    qy = np.abs(warped_y) * course_height_m - (
        half_height_m - corner_round_m
    )
    outside = np.sqrt(
        np.maximum(qx, 0.0) ** 2 + np.maximum(qy, 0.0) ** 2
    )
    inside = np.minimum(np.maximum(qx, qy), 0.0)
    edge_distance_m = (
        corner_round_m - outside - inside
    ).astype(np.float32)
    bevel_m = max(
        0.0011,
        recipe.tile_size_m / resolution * 1.25,
    )
    brick_mask = smoothstep(
        -bevel_m,
        bevel_m,
        edge_distance_m,
    ).astype(np.float32)
    mortar_mask = (1.0 - brick_mask).astype(np.float32)
    vertical_edge = qx > qy
    side_index = np.where(
        vertical_edge,
        np.where(warped_x < 0.0, 0, 1),
        np.where(warped_y < 0.0, 2, 3),
    ).astype(np.int32)
    return {
        "resolution": resolution,
        "brick_count": layout["brick_count"],
        "u": u,
        "v": v,
        "course_id": course_id,
        "column_id": column_id,
        "brick_id": brick_id,
        "champion_id": champion_id,
        "local_x": local_x.astype(np.float32),
        "local_y": local_y.astype(np.float32),
        "warped_x": warped_x.astype(np.float32),
        "warped_y": warped_y.astype(np.float32),
        "course_height_m": course_height_m.astype(np.float32),
        "pitch_x_m": float(pitch_x_m),
        "half_width_m": half_width_m.astype(np.float32),
        "half_height_m": half_height_m.astype(np.float32),
        "edge_distance_m": edge_distance_m,
        "brick_mask": brick_mask,
        "mortar_mask": mortar_mask,
        "side_index": side_index,
    }


def clay_anatomy_node(
    raster: dict[str, Any],
    layout: dict[str, Any],
    recipe: BrickRecipe,
    *,
    seed: int,
) -> dict[str, np.ndarray]:
    resolution = raster["resolution"]
    brick_id = raster["brick_id"]
    champion_id = raster["champion_id"]
    brick_mask = raster["brick_mask"]
    local_x = raster["local_x"]
    local_y = raster["local_y"]
    phase = layout["brick_phase"][brick_id]
    lamination_strength = _champion_values(
        recipe,
        "lamination_strength",
    )[champion_id]
    compression_strength = _champion_values(
        recipe,
        "compression_strength",
    )[champion_id]
    pit_strength = _champion_values(
        recipe,
        "pit_strength",
    )[champion_id]
    mineral_strength = _champion_values(
        recipe,
        "mineral_strength",
    )[champion_id]
    fired_skin_strength = _champion_values(
        recipe,
        "fired_skin_strength",
    )[champion_id]

    broad_noise = periodic_fbm_rect(
        resolution,
        5,
        9,
        seed + 101,
        octaves=3,
        persistence=0.47,
    )
    medium_noise = periodic_fbm_rect(
        resolution,
        17,
        29,
        seed + 211,
        octaves=3,
        persistence=0.50,
    )
    fine_noise = periodic_fbm(
        resolution,
        47,
        seed + 307,
        octaves=3,
        persistence=0.46,
    )
    kiln_gradient = np.clip(
        0.50
        + local_x * (0.42 + phase[..., 0] * 0.22)
        + local_y * (phase[..., 1] - 0.5) * 0.34
        + np.sin(local_y * math.pi + phase[..., 2] * math.tau) * 0.08,
        0.0,
        1.0,
    ).astype(np.float32)
    heat_cloud = np.clip(
        0.50
        + np.sin(
            local_x * math.tau * 0.82
            + phase[..., 1] * math.tau
        )
        * 0.18
        + np.cos(
            local_y * math.tau * 1.15
            + phase[..., 2] * math.tau
        )
        * 0.14
        + (broad_noise - 0.5) * 0.22,
        0.0,
        1.0,
    ).astype(np.float32)
    clay_body = np.clip(
        0.50
        + (medium_noise - 0.5) * 0.34
        + np.sin(
            local_x * math.tau * 1.7
            + local_y * math.tau * 0.55
            + phase[..., 3] * math.tau
        )
        * 0.10,
        0.0,
        1.0,
    ).astype(np.float32)

    lamination_phase = (
        (local_y + 0.5) * (3.0 + phase[..., 0] * 2.2)
        + np.sin(
            local_x * math.tau * 1.1 + phase[..., 1] * math.tau
        )
        * 0.13
    )
    lamination_wave = 0.5 + 0.5 * np.cos(
        lamination_phase * math.tau
    )
    lamination_break = smoothstep(0.23, 0.72, medium_noise)
    clay_lamination_mask = (
        smoothstep(0.79, 0.97, lamination_wave)
        * lamination_break
        * lamination_strength
        * brick_mask
    ).astype(np.float32)

    drag_phase = (
        (local_y + 0.5) * (8.0 + phase[..., 2] * 5.0)
        + np.sin(
            local_x * math.tau * 1.8 + phase[..., 3] * math.tau
        )
        * 0.18
    )
    drag_wave = 0.5 + 0.5 * np.cos(drag_phase * math.tau)
    compression_drag_mask = (
        smoothstep(0.84, 0.985, drag_wave)
        * smoothstep(0.32, 0.72, fine_noise)
        * compression_strength
        * brick_mask
    ).astype(np.float32)

    pit_mask = np.zeros_like(brick_mask, dtype=np.float32)
    for pit_index in range(3):
        centers = layout["pit_centers"][brick_id, pit_index]
        radii = layout["pit_radius_m"][brick_id, pit_index]
        distance = np.sqrt(
            ((local_x - centers[..., 0]) * raster["pitch_x_m"]) ** 2
            + (
                (local_y - centers[..., 1])
                * raster["course_height_m"]
            )
            ** 2
        )
        pit_mask = np.maximum(
            pit_mask,
            1.0
            - smoothstep(
                radii * 0.42,
                radii,
                distance,
            ),
        )
    pit_mask = (
        pit_mask
        * pit_strength
        * smoothstep(0.30, 0.78, fine_noise)
        * brick_mask
    ).astype(np.float32)

    mineral_inclusion_mask = np.zeros_like(
        brick_mask,
        dtype=np.float32,
    )
    for mineral_index in range(2):
        centers = layout["mineral_centers"][brick_id, mineral_index]
        radii = layout["mineral_radius_m"][brick_id, mineral_index]
        distance = np.sqrt(
            ((local_x - centers[..., 0]) * raster["pitch_x_m"]) ** 2
            + (
                (local_y - centers[..., 1])
                * raster["course_height_m"]
            )
            ** 2
        )
        mineral_inclusion_mask = np.maximum(
            mineral_inclusion_mask,
            1.0 - smoothstep(radii * 0.50, radii, distance),
        )
    mineral_inclusion_mask = (
        mineral_inclusion_mask
        * mineral_strength
        * brick_mask
    ).astype(np.float32)

    fired_skin_mask = (
        (
            1.0
            - smoothstep(
                0.0025,
                0.015,
                raster["edge_distance_m"],
            )
        )
        * fired_skin_strength
        * brick_mask
        * (0.64 + heat_cloud * 0.36)
    ).astype(np.float32)
    moulding_relief = (
        (broad_noise - 0.5) * 0.62
        + (clay_body - 0.5) * 0.38
    ).astype(np.float32)
    return {
        "kiln_gradient": kiln_gradient,
        "heat_cloud": heat_cloud,
        "clay_body": clay_body,
        "clay_lamination_mask": clay_lamination_mask,
        "compression_drag_mask": compression_drag_mask,
        "mineral_inclusion_mask": mineral_inclusion_mask,
        "pit_mask": pit_mask,
        "fired_skin_mask": fired_skin_mask,
        "moulding_relief": moulding_relief,
        "broad_noise": broad_noise,
        "medium_noise": medium_noise,
        "fine_noise": fine_noise,
    }


def mortar_state_node(
    raster: dict[str, Any],
    recipe: BrickRecipe,
    anatomy: dict[str, np.ndarray],
) -> dict[str, np.ndarray]:
    mortar_mask = raster["mortar_mask"]
    aggregate_mask = (
        smoothstep(0.73, 0.91, anatomy["fine_noise"])
        * mortar_mask
        * float(recipe.mortar["aggregate_strength"])
    ).astype(np.float32)
    nearest_vertical = raster["side_index"] <= 1
    trowel_phase = np.where(
        nearest_vertical,
        raster["local_y"] * 13.0,
        raster["local_x"] * 18.0,
    )
    trowel_wave = 0.5 + 0.5 * np.cos(
        trowel_phase * math.tau
        + anatomy["medium_noise"] * 1.3
    )
    trowel_mask = (
        smoothstep(0.72, 0.96, trowel_wave)
        * mortar_mask
        * float(recipe.mortar["trowel_strength"])
    ).astype(np.float32)
    contact_cavity = (
        (
            1.0
            - smoothstep(
                -0.007,
                -0.001,
                raster["edge_distance_m"],
            )
        )
        * mortar_mask
    ).astype(np.float32)
    mortar_body = (
        mortar_mask
        * np.clip(
            0.76
            + (anatomy["broad_noise"] - 0.5) * 0.18
            + trowel_mask * 0.10,
            0.0,
            1.0,
        )
    ).astype(np.float32)
    return {
        "mortar_body": mortar_body,
        "mortar_aggregate_mask": aggregate_mask,
        "mortar_trowel_mask": trowel_mask,
        "mortar_contact_cavity": contact_cavity,
    }


def structural_linework_node(
    raster: dict[str, Any],
    layout: dict[str, Any],
    recipe: BrickRecipe,
    anatomy: dict[str, np.ndarray],
) -> dict[str, np.ndarray]:
    brick_id = raster["brick_id"]
    champion_id = raster["champion_id"]
    brick_mask = raster["brick_mask"]
    side_weight = layout["edge_side_weight"][
        brick_id,
        raster["side_index"],
    ]
    line_priority = _champion_values(
        recipe,
        "line_priority",
    )[champion_id]
    line_width = layout["line_width_m"][brick_id]
    edge_line_profile = (
        smoothstep(
            0.0001,
            line_width * 0.52,
            raster["edge_distance_m"],
        )
        * (
            1.0
            - smoothstep(
                line_width * 0.90,
                line_width * 2.35,
                raster["edge_distance_m"],
            )
        )
    )
    edge_break = np.clip(
        0.28
        + anatomy["medium_noise"] * 0.88
        + np.sin(
            raster["local_x"] * math.tau * 2.4
            + raster["local_y"] * math.tau * 0.8
            + layout["brick_phase"][brick_id, 0] * math.tau
        )
        * 0.18,
        0.0,
        1.0,
    )
    primary = (
        edge_line_profile
        * smoothstep(0.30, 0.68, edge_break)
        * np.sqrt(side_weight)
        * np.sqrt(line_priority)
        * brick_mask
        * 1.55
    ).astype(np.float32)
    primary = np.clip(primary, 0.0, 1.0).astype(np.float32)

    pit_rim = np.maximum(
        periodic_gaussian_blur(
            anatomy["pit_mask"],
            max(0.65, raster["resolution"] / 1800.0),
        )
        - anatomy["pit_mask"] * 0.72,
        0.0,
    )
    pit_rim = normalized_range(pit_rim) * (
        anatomy["pit_mask"] > 0.01
    )
    mineral_rim = np.maximum(
        periodic_gaussian_blur(
            anatomy["mineral_inclusion_mask"],
            max(0.60, raster["resolution"] / 1900.0),
        )
        - anatomy["mineral_inclusion_mask"] * 0.76,
        0.0,
    )
    mineral_rim = normalized_range(mineral_rim)
    overfired = np.asarray(
        [
            champion["face_role"] == "overfired"
            for champion in recipe.champions
        ],
        dtype=np.float32,
    )[champion_id]
    fissure_curve = (
        raster["local_y"]
        - (
            raster["local_x"] * (0.16 + layout["brick_phase"][brick_id, 1] * 0.24)
            + np.sin(
                raster["local_x"] * math.tau * 1.3
                + layout["brick_phase"][brick_id, 2] * math.tau
            )
            * 0.045
            - 0.04
        )
    )
    fissure = (
        (
            1.0
            - smoothstep(
                0.006,
                0.021,
                np.abs(fissure_curve),
            )
        )
        * smoothstep(0.12, 0.30, 0.5 - np.abs(raster["local_x"]))
        * overfired
        * brick_mask
    )
    secondary = np.maximum.reduce(
        [
            anatomy["clay_lamination_mask"] * 0.74,
            pit_rim * 0.70,
            mineral_rim * 0.48,
            fissure * 0.86,
        ]
    ).astype(np.float32)
    tertiary = (
        np.maximum(
            anatomy["compression_drag_mask"] * 0.52,
            anatomy["mineral_inclusion_mask"] * 0.24,
        )
        * smoothstep(0.28, 0.68, anatomy["fine_noise"])
        * brick_mask
    ).astype(np.float32)
    ink_mask = np.clip(
        np.maximum.reduce(
            [
                primary * 0.92,
                secondary * 0.80,
                tertiary * 0.42,
            ]
        ),
        0.0,
        1.0,
    ).astype(np.float32)

    upper_or_left = np.logical_or(
        raster["side_index"] == 0,
        raster["side_index"] == 2,
    )
    highlight = (
        edge_line_profile
        * upper_or_left
        * np.clip(1.0 - side_weight * 0.55, 0.0, 1.0)
        * (0.32 + line_priority * 0.38)
        * smoothstep(0.42, 0.78, anatomy["medium_noise"])
        * brick_mask
    ).astype(np.float32)
    quiet_factor = np.asarray(
        [
            0.30 if champion["face_role"] == "calm" else 0.72
            for champion in recipe.champions
        ],
        dtype=np.float32,
    )[champion_id]
    detail_priority = np.clip(
        0.12
        + quiet_factor * 0.34
        + primary * 0.36
        + secondary * 0.42
        + anatomy["pit_mask"] * 0.24,
        0.0,
        1.0,
    ).astype(np.float32)
    brush_angle = np.radians(
        _champion_values(
            recipe,
            "brush_angle_deg",
        )[champion_id]
    )
    brush_direction = np.stack(
        [np.cos(brush_angle), np.sin(brush_angle)],
        axis=-1,
    ).astype(np.float32)
    return {
        "primary_line_mask": primary,
        "secondary_line_mask": secondary,
        "tertiary_line_mask": tertiary,
        "ink_mask": ink_mask,
        "highlight_stroke_mask": highlight,
        "detail_priority": detail_priority,
        "brush_direction": brush_direction,
    }


def _build_brick_shade_families(
    layout: dict[str, Any],
    recipe: BrickRecipe,
) -> np.ndarray:
    champion_flat = layout["champion_id"].reshape(-1)
    bases = np.asarray(
        [
            KILN_BASE_SRGB[
                recipe.champions[int(champion_id)]["kiln_family"]
            ]
            for champion_id in champion_flat
        ],
        dtype=np.float32,
    ) / 255.0
    dark_violet = np.asarray([55, 32, 37], dtype=np.float32) / 255.0
    deep_umber = np.asarray([78, 42, 31], dtype=np.float32) / 255.0
    burnt_orange = np.asarray([197, 95, 42], dtype=np.float32) / 255.0
    pale_clay = np.asarray([222, 158, 96], dtype=np.float32) / 255.0
    chalk = np.asarray([220, 183, 133], dtype=np.float32) / 255.0
    anchor_positions = np.asarray(
        [0.00, 0.13, 0.28, 0.47, 0.65, 0.83, 1.00],
        dtype=np.float32,
    )
    positions = np.linspace(
        0.0,
        1.0,
        SHADE_FAMILY_SIZE,
        dtype=np.float32,
    )
    families = np.empty(
        (layout["brick_count"], SHADE_FAMILY_SIZE, 3),
        dtype=np.float32,
    )
    for brick_index, base in enumerate(bases):
        base = base * float(layout["brick_value"][brick_index])
        temperature = float(layout["brick_temperature"][brick_index])
        base *= np.asarray(
            [
                1.0 + temperature * 0.060,
                1.0 + temperature * 0.004,
                1.0 - temperature * 0.055,
            ],
            dtype=np.float32,
        )
        anchors = np.stack(
            [
                base * 0.33 + dark_violet * 0.67,
                base * 0.54 + dark_violet * 0.20 + deep_umber * 0.26,
                base * 0.79 + deep_umber * 0.21,
                base,
                base * 0.78 + burnt_orange * 0.22,
                base * 0.60 + burnt_orange * 0.17 + pale_clay * 0.23,
                base * 0.43 + pale_clay * 0.38 + chalk * 0.19,
            ],
            axis=0,
        )
        family = np.stack(
            [
                np.interp(
                    positions,
                    anchor_positions,
                    anchors[:, channel],
                )
                for channel in range(3)
            ],
            axis=-1,
        )
        families[brick_index] = srgb_to_linear(
            np.clip(family, 0.0, 1.0)
        )
    return families


def _stretch_coordinate_per_brick(
    raw_coordinate: np.ndarray,
    raster: dict[str, Any],
    brick_count: int,
) -> np.ndarray:
    result = np.full(raw_coordinate.shape, 0.5, dtype=np.float32)
    for brick_index in range(brick_count):
        mask = np.logical_and(
            raster["brick_id"] == brick_index,
            raster["brick_mask"] > 0.72,
        )
        values = raw_coordinate[mask]
        if values.size < 8:
            continue
        low = float(np.percentile(values, 4.0))
        high = float(np.percentile(values, 96.0))
        normalized = np.clip(
            (raw_coordinate - low) / max(high - low, 1.0e-5),
            0.0,
            1.0,
        )
        result[mask] = normalized[mask]
    return result


def _sample_shade_families(
    families: np.ndarray,
    brick_id: np.ndarray,
    coordinate: np.ndarray,
) -> np.ndarray:
    scaled = np.clip(coordinate, 0.0, 1.0) * (
        SHADE_FAMILY_SIZE - 1
    )
    lower = np.floor(scaled).astype(np.int32)
    upper = np.minimum(lower + 1, SHADE_FAMILY_SIZE - 1)
    fraction = (scaled - lower)[..., np.newaxis]
    return (
        families[brick_id, lower] * (1.0 - fraction)
        + families[brick_id, upper] * fraction
    ).astype(np.float32)


def layered_color_node(
    raster: dict[str, Any],
    layout: dict[str, Any],
    recipe: BrickRecipe,
    anatomy: dict[str, np.ndarray],
    mortar: dict[str, np.ndarray],
    linework: dict[str, np.ndarray],
) -> dict[str, Any]:
    families = _build_brick_shade_families(layout, recipe)
    shade_blur = max(1.25, raster["resolution"] / 256.0)
    smooth_clay = periodic_gaussian_blur(
        anatomy["clay_body"],
        shade_blur * 0.65,
    )
    smooth_skin = periodic_gaussian_blur(
        anatomy["fired_skin_mask"],
        shade_blur * 0.45,
    )
    raw_coordinate = (
        anatomy["kiln_gradient"] * 0.47
        + anatomy["heat_cloud"] * 0.31
        + smooth_clay * 0.16
        + (1.0 - smooth_skin) * 0.06
    ).astype(np.float32)
    coordinate = _stretch_coordinate_per_brick(
        raw_coordinate,
        raster,
        layout["brick_count"],
    )
    brick_color = _sample_shade_families(
        families,
        raster["brick_id"],
        coordinate,
    )
    reduction_mask = (
        (1.0 - smoothstep(0.18, 0.48, anatomy["heat_cloud"]))
        * raster["brick_mask"]
        * 0.34
    ).astype(np.float32)
    oxidation_mask = (
        smoothstep(0.54, 0.86, anatomy["kiln_gradient"])
        * raster["brick_mask"]
        * 0.30
    ).astype(np.float32)
    reduction_color = _sample_shade_families(
        families,
        raster["brick_id"],
        np.clip(coordinate - 0.17, 0.0, 1.0),
    )
    reduction_color = np.clip(
        reduction_color * np.asarray([0.94, 0.94, 1.08], dtype=np.float32),
        0.0,
        1.0,
    )
    oxidation_color = _sample_shade_families(
        families,
        raster["brick_id"],
        np.clip(coordinate + 0.14, 0.0, 1.0),
    )
    oxidation_color = np.clip(
        oxidation_color * np.asarray([1.06, 1.00, 0.92], dtype=np.float32),
        0.0,
        1.0,
    )
    brick_color = mix(brick_color, reduction_color, reduction_mask)
    brick_color = mix(brick_color, oxidation_color, oxidation_mask)
    lamination_color = _sample_shade_families(
        families,
        raster["brick_id"],
        np.clip(coordinate - 0.10, 0.0, 1.0),
    )
    brick_color = mix(
        brick_color,
        lamination_color,
        anatomy["clay_lamination_mask"] * 0.44,
    )
    pale_mineral = srgb_to_linear(
        np.asarray([210, 177, 128], dtype=np.float32) / 255.0
    )
    brick_color = mix(
        brick_color,
        np.broadcast_to(pale_mineral, brick_color.shape),
        anatomy["mineral_inclusion_mask"] * 0.76,
    )
    pit_color = _sample_shade_families(
        families,
        raster["brick_id"],
        np.clip(coordinate - 0.28, 0.0, 1.0),
    )
    brick_color = mix(
        brick_color,
        pit_color,
        anatomy["pit_mask"] * 0.72,
    )
    fired_skin_color = _sample_shade_families(
        families,
        raster["brick_id"],
        np.clip(coordinate - 0.13, 0.0, 1.0),
    )
    brick_color = mix(
        brick_color,
        fired_skin_color,
        anatomy["fired_skin_mask"] * 0.30,
    )
    brick_pigment = np.clip(brick_color, 0.0, 1.0).astype(np.float32)

    mortar_dark = srgb_to_linear(
        np.asarray(MORTAR_DARK_SRGB, dtype=np.float32) / 255.0
    )
    mortar_light = srgb_to_linear(
        np.asarray(MORTAR_LIGHT_SRGB, dtype=np.float32) / 255.0
    )
    mortar_coordinate = np.clip(
        mortar["mortar_body"]
        + anatomy["broad_noise"] * 0.16
        - mortar["mortar_contact_cavity"] * 0.20,
        0.0,
        1.0,
    )
    mortar_color = mix(
        np.broadcast_to(mortar_dark, brick_color.shape),
        np.broadcast_to(mortar_light, brick_color.shape),
        mortar_coordinate,
    )
    mortar_color = mix(
        mortar_color,
        np.broadcast_to(mortar_dark * 0.68, brick_color.shape),
        mortar["mortar_aggregate_mask"] * 0.62,
    )
    base_color = mix(
        mortar_color,
        brick_pigment,
        raster["brick_mask"],
    )
    ink_color = srgb_to_linear(
        np.asarray(INK_SRGB, dtype=np.float32) / 255.0
    )
    base_color = mix(
        base_color,
        np.broadcast_to(ink_color, base_color.shape),
        linework["ink_mask"] * 0.74,
    )
    highlight_color = _sample_shade_families(
        families,
        raster["brick_id"],
        np.clip(coordinate + 0.20, 0.0, 1.0),
    )
    base_color = mix(
        base_color,
        highlight_color,
        linework["highlight_stroke_mask"] * 0.30,
    )
    return {
        "shade_family_size": SHADE_FAMILY_SIZE,
        "brick_shade_palettes_linear": families,
        "shade_coordinate": coordinate.astype(np.float32),
        "shade_index_continuous": (
            coordinate * (SHADE_FAMILY_SIZE - 1)
        ).astype(np.float32),
        "shade_drivers": {
            "kiln_gradient": anatomy["kiln_gradient"],
            "heat_cloud": anatomy["heat_cloud"],
            "clay_body": smooth_clay.astype(np.float32),
            "fired_skin": smooth_skin.astype(np.float32),
        },
        "reduction_mask": reduction_mask,
        "oxidation_mask": oxidation_mask,
        "brick_pigment_linear": brick_pigment,
        "mortar_color_linear": mortar_color.astype(np.float32),
        "base_color_linear": np.clip(base_color, 0.0, 1.0).astype(
            np.float32
        ),
    }


def generate_material(
    *,
    resolution: int = DEFAULT_RESOLUTION,
    seed: int = DEFAULT_SEED,
    tile_size_m: float = DEFAULT_TILE_SIZE_M,
    recipe_path: str | Path | None = None,
    pattern_variation: int = DEFAULT_VARIATION,
) -> dict[str, Any]:
    if resolution < 64:
        raise ValueError("resolution must be at least 64")
    recipe = load_brick_recipe(recipe_path or DEFAULT_RECIPE_PATH)
    if not math.isclose(tile_size_m, recipe.tile_size_m, abs_tol=1.0e-6):
        raise ValueError("requested tile size differs from authored recipe")
    meters_per_pixel = tile_size_m / resolution
    layout = generate_brick_layout(recipe, variation=pattern_variation)
    raster = rasterize_brick_pattern(
        layout,
        recipe,
        resolution=resolution,
    )
    anatomy = clay_anatomy_node(
        raster,
        layout,
        recipe,
        seed=seed,
    )
    mortar = mortar_state_node(raster, recipe, anatomy)
    linework = structural_linework_node(
        raster,
        layout,
        recipe,
        anatomy,
    )
    color = layered_color_node(
        raster,
        layout,
        recipe,
        anatomy,
        mortar,
        linework,
    )

    champion_id = raster["champion_id"]
    crown_m = (
        _champion_values(recipe, "crown_mm")[champion_id] * 0.001
    )
    normalized_x = np.clip(
        np.abs(raster["warped_x"])
        / np.maximum(
            raster["half_width_m"] / raster["pitch_x_m"],
            1.0e-5,
        ),
        0.0,
        1.0,
    )
    normalized_y = np.clip(
        np.abs(raster["warped_y"])
        / np.maximum(
            raster["half_height_m"] / raster["course_height_m"],
            1.0e-5,
        ),
        0.0,
        1.0,
    )
    crown = (
        (1.0 - normalized_x**2)
        * (1.0 - normalized_y**2)
        * crown_m
    ).astype(np.float32)
    brick_raise = (
        0.0072
        + (layout["brick_value"][raster["brick_id"]] - 1.0) * 0.010
    )
    brick_height = (
        brick_raise
        + crown
        + anatomy["moulding_relief"] * 0.00085
        + anatomy["compression_drag_mask"] * 0.00018
        + anatomy["mineral_inclusion_mask"] * 0.00020
        - anatomy["clay_lamination_mask"] * 0.00011
        - anatomy["pit_mask"] * 0.00125
    ).astype(np.float32)
    mortar_recess_m = float(recipe.mortar["recess_mm"]) * 0.001
    mortar_height = (
        -mortar_recess_m
        + (anatomy["broad_noise"] - 0.5) * 0.00050
        + mortar["mortar_trowel_mask"] * 0.00024
        - mortar["mortar_aggregate_mask"] * 0.00018
        - mortar["mortar_contact_cavity"] * 0.00042
    ).astype(np.float32)
    height_m = (
        mortar_height * (1.0 - raster["brick_mask"])
        + brick_height * raster["brick_mask"]
    ).astype(np.float32)
    normal = height_to_normal(
        height_m,
        meters_per_pixel,
        strength=0.90,
    )
    cavity = np.maximum(
        periodic_gaussian_blur(
            height_m,
            max(1.0, resolution / 720.0),
        )
        - height_m,
        0.0,
    )
    cavity = normalized_range(cavity)
    ao = np.clip(
        1.0
        - cavity * 0.46
        - mortar["mortar_contact_cavity"] * 0.20
        - anatomy["pit_mask"] * 0.12,
        0.34,
        1.0,
    ).astype(np.float32)
    brick_roughness = np.clip(
        0.73
        + (anatomy["fine_noise"] - 0.5) * 0.10
        + anatomy["pit_mask"] * 0.09
        + anatomy["clay_lamination_mask"] * 0.03
        - anatomy["fired_skin_mask"] * 0.07
        - anatomy["mineral_inclusion_mask"] * 0.04,
        0.52,
        0.92,
    )
    mortar_roughness = np.clip(
        0.90
        + mortar["mortar_aggregate_mask"] * 0.05
        - mortar["mortar_trowel_mask"] * 0.07,
        0.76,
        0.98,
    )
    roughness = (
        mortar_roughness * (1.0 - raster["brick_mask"])
        + brick_roughness * raster["brick_mask"]
    ).astype(np.float32)
    orm = np.stack(
        [ao, roughness, np.zeros_like(roughness)],
        axis=-1,
    ).astype(np.float32)
    preview = render_material_preview(
        color["base_color_linear"],
        normal,
        roughness,
        ao,
    )
    frontal = render_material_single_light(
        color["base_color_linear"],
        normal,
        roughness,
        ao,
        direction=(-0.08, -0.10, 1.0),
        intensity=2.0,
    )
    side = render_material_single_light(
        color["base_color_linear"],
        normal,
        roughness,
        ao,
        direction=(-0.74, -0.18, 0.65),
        intensity=2.0,
    )
    grazing = render_material_single_light(
        color["base_color_linear"],
        normal,
        roughness,
        ao,
        direction=(0.94, 0.06, 0.34),
        intensity=2.0,
    )
    return {
        "schema": MATERIAL_SCHEMA,
        "resolution": resolution,
        "seed": seed,
        "tile_size_m": tile_size_m,
        "meters_per_pixel": meters_per_pixel,
        "recipe": recipe,
        "layout": layout,
        "pattern_variation": pattern_variation,
        "brick_count": layout["brick_count"],
        "proof_brick_id": PROOF_BRICK_ID,
        "brick_id": raster["brick_id"],
        "champion_id": raster["champion_id"],
        "course_id": raster["course_id"],
        "local_x": raster["local_x"],
        "local_y": raster["local_y"],
        "edge_distance_m": raster["edge_distance_m"],
        "brick_mask": raster["brick_mask"],
        "mortar_mask": raster["mortar_mask"],
        **anatomy,
        **mortar,
        **linework,
        **color,
        "crown_m": crown,
        "brick_height_m": brick_height,
        "mortar_height_m": mortar_height,
        "height_m": height_m,
        "normal": normal,
        "ao": ao,
        "roughness": roughness,
        "orm": orm,
        "preview_srgb": preview,
        "white_light_frontal_srgb": frontal,
        "white_light_side_srgb": side,
        "white_light_grazing_srgb": grazing,
        "pattern": {
            "schema": recipe.schema,
            "name": recipe.name,
            "source": recipe.source_path,
            "variation": pattern_variation,
            "course_count": layout["course_count"],
            "columns": recipe.columns,
            "brick_count": layout["brick_count"],
            "champion_count": len(recipe.champions),
            "mortar_profile": recipe.mortar["profile"],
        },
    }


def _brick_focus(
    material: dict[str, Any],
    image: np.ndarray,
    *,
    brick_id: int | None = None,
    padding: float = 0.18,
) -> np.ndarray:
    chosen_id = material["proof_brick_id"] if brick_id is None else brick_id
    mask = np.logical_and(
        material["brick_id"] == chosen_id,
        material["brick_mask"] > 0.50,
    )
    yy, xx = np.where(mask)
    if yy.size == 0:
        raise ValueError("proof brick has no pixels")
    pad_x = max(2, int(round((xx.max() - xx.min() + 1) * padding)))
    pad_y = max(2, int(round((yy.max() - yy.min() + 1) * padding)))
    x0 = max(0, int(xx.min()) - pad_x)
    x1 = min(image.shape[1], int(xx.max()) + pad_x + 1)
    y0 = max(0, int(yy.min()) - pad_y)
    y1 = min(image.shape[0], int(yy.max()) + pad_y + 1)
    source = np.asarray(image[y0:y1, x0:x1], dtype=np.float32)
    crop_identity = (
        material["brick_id"][y0:y1, x0:x1] == chosen_id
    ).astype(np.float32)
    crop_alpha = (
        material["brick_mask"][y0:y1, x0:x1] * crop_identity
    )
    if source.ndim == 2:
        source = gray_rgb(source)
    isolated = mix(
        np.full(source.shape, 0.035, dtype=np.float32),
        source,
        crop_alpha,
    )
    side = max(isolated.shape[0], isolated.shape[1])
    square = np.full((side, side, 3), 0.035, dtype=np.float32)
    offset_y = (side - isolated.shape[0]) // 2
    offset_x = (side - isolated.shape[1]) // 2
    square[
        offset_y : offset_y + isolated.shape[0],
        offset_x : offset_x + isolated.shape[1],
    ] = isolated
    return square


def _shade_usage_panel(material: dict[str, Any]) -> np.ndarray:
    brick_id = material["proof_brick_id"]
    mask = np.logical_and(
        material["brick_id"] == brick_id,
        material["brick_mask"] > 0.90,
    )
    bins = np.clip(
        np.rint(material["shade_index_continuous"][mask]),
        0,
        SHADE_FAMILY_SIZE - 1,
    ).astype(np.int32)
    counts = np.bincount(bins, minlength=SHADE_FAMILY_SIZE).astype(
        np.float32
    )
    counts /= max(float(counts.max()), 1.0)
    colors = linear_to_srgb(
        material["brick_shade_palettes_linear"][brick_id]
    )
    panel = np.full((256, 256, 3), 0.035, dtype=np.float32)
    width = 256.0 / SHADE_FAMILY_SIZE
    for index, (color, amount) in enumerate(
        zip(colors, counts, strict=True)
    ):
        x0 = int(round(index * width))
        x1 = max(x0 + 1, int(round((index + 1) * width)) - 1)
        height = max(4, int(round(float(amount) * 226.0)))
        panel[244 - height : 244, x0:x1] = color
        panel[244:252, x0:x1] = color * 0.65
    return panel


def _construction_proof(material: dict[str, Any]) -> np.ndarray:
    id_color = scalar_tint(
        np.mod(material["brick_id"] * 0.173, 1.0),
        (0.12, 0.20, 0.36),
        (0.94, 0.62, 0.18),
    )
    return compose_material_chapter(
        "AUTHORED BRICK CONSTRUCTION",
        "TWENTY COURSES SEVEN WRAPPED POSITIONS AND TWENTY CHAMPION FACES",
        [
            ("BRICK IDENTITIES", id_color),
            ("BRICK MASK", gray_rgb(material["brick_mask"])),
            ("MORTAR BODY", gray_rgb(material["mortar_body"])),
            ("RECESSED HEIGHT", scalar_tint(
                material["height_m"],
                (0.12, 0.18, 0.32),
                (0.92, 0.58, 0.18),
                normalize=True,
            )),
            ("CROWN RELIEF", scalar_tint(
                material["crown_m"],
                (0.08, 0.10, 0.12),
                (0.96, 0.70, 0.28),
                normalize=True,
            )),
            ("CLAY LAMINATIONS", gray_rgb(
                material["clay_lamination_mask"]
            )),
            ("PITS RED MINERAL GREEN", np.stack(
                [
                    material["pit_mask"],
                    material["mineral_inclusion_mask"],
                    np.zeros_like(material["pit_mask"]),
                ],
                axis=-1,
            )),
            ("FINAL UNLIT COLOR", linear_to_srgb(
                material["base_color_linear"]
            )),
        ],
        [
            "THE BOND WRAPS IN BOTH AXES BUT COURSE HEIGHT OFFSET CORNER ROUNDING CROWN AND FACE ROLE CHANGE BY AUTHORED RECORD",
            "MORTAR IS A RECESSED COMPRESSED MATERIAL WITH ITS OWN AGGREGATE TROWEL AND CONTACT CAVITY SIGNALS",
            "BRICK RELIEF IS BUILT FROM CROWN MOULDING DRAG LAMINATION PITS AND MINERALS BEFORE NORMALS OR LIGHTING EXIST",
        ],
    )


def _color_proof(material: dict[str, Any]) -> np.ndarray:
    brick_id = material["proof_brick_id"]
    family = linear_to_srgb(
        material["brick_shade_palettes_linear"][brick_id]
    )
    return compose_material_chapter(
        "TWENTY FIRED CLAY SHADES INSIDE ONE BRICK",
        f"BRICK {brick_id:03d} ONLY NEIGHBORING BRICK COLORS DO NOT COUNT",
        [
            ("20 SHADE FAMILY", palette_panel(family)),
            ("ACTUAL SHADE USE", _shade_usage_panel(material)),
            ("PALETTE COORDINATE", _brick_focus(
                material,
                scalar_tint(
                    material["shade_coordinate"],
                    (0.11, 0.04, 0.05),
                    (0.98, 0.64, 0.25),
                ),
            )),
            ("KILN GRADIENT", _brick_focus(
                material,
                scalar_tint(
                    material["kiln_gradient"],
                    (0.18, 0.08, 0.09),
                    (0.96, 0.55, 0.18),
                ),
            )),
            ("HEAT CLOUD", _brick_focus(
                material,
                scalar_tint(
                    material["heat_cloud"],
                    (0.16, 0.10, 0.18),
                    (0.94, 0.42, 0.18),
                ),
            )),
            ("CLAY BODY", _brick_focus(
                material,
                scalar_tint(
                    material["shade_drivers"]["clay_body"],
                    (0.22, 0.09, 0.05),
                    (0.91, 0.64, 0.33),
                ),
            )),
            ("FIRED SKIN", _brick_focus(
                material,
                scalar_tint(
                    material["fired_skin_mask"],
                    (0.08, 0.05, 0.07),
                    (0.78, 0.28, 0.12),
                ),
            )),
            ("ONE BRICK UNLIT", _brick_focus(
                material,
                linear_to_srgb(material["brick_pigment_linear"]),
            )),
        ],
        [
            "IRON AND FIRING HISTORY DRIVE THE LARGE RED OCHRE UMBER AND VIOLET MOVEMENT WHILE CLAY STRUCTURE MAKES SMALLER LOCAL CHANGES",
            "THE PALETTE IS SAMPLED CONTINUOUSLY BETWEEN TWENTY CONTROLS SO IT HAS RANGE WITHOUT POSTERIZED BANDING",
            "PITS MINERALS LAMINATIONS AND FIRED SKIN ALTER RELATED SHADES INSTEAD OF RECEIVING UNRELATED RANDOM COLORS",
        ],
    )


def _linework_proof(material: dict[str, Any]) -> np.ndarray:
    no_ink = mix(
        material["mortar_color_linear"],
        material["brick_pigment_linear"],
        material["brick_mask"],
    )
    return compose_material_chapter(
        "SELECTIVE STRUCTURAL LINEWORK",
        "THREE LINE FAMILIES WITH BROKEN WIDTH PRIORITY AND QUIET REGIONS",
        [
            ("PRIMARY CONTOURS", gray_rgb(
                material["primary_line_mask"]
            )),
            ("SECONDARY FORM", gray_rgb(
                material["secondary_line_mask"]
            )),
            ("TERTIARY STROKES", gray_rgb(
                material["tertiary_line_mask"]
            )),
            ("COMBINED INK", gray_rgb(material["ink_mask"])),
            ("HIGHLIGHT STROKES", gray_rgb(
                material["highlight_stroke_mask"]
            )),
            ("DETAIL PRIORITY", scalar_tint(
                material["detail_priority"],
                (0.10, 0.14, 0.24),
                (0.96, 0.56, 0.18),
            )),
            ("COLOR WITHOUT INK", linear_to_srgb(no_ink)),
            ("COLOR WITH INK", linear_to_srgb(
                material["base_color_linear"]
            )),
        ],
        [
            "PRIMARY LINES SELECT ONLY SOME ARRIS SECTIONS SECONDARY LINES DESCRIBE LAMINATIONS PITS MINERALS AND FIRING FISSURES",
            "TERTIARY STROKES ADD SPARSE FORMING DRAGS THE DETAIL PRIORITY FIELD PROTECTS CALM BRICKS FROM UNIFORM BUSYNESS",
            "INK IS DEEP BROWN VIOLET RATHER THAN BLACK AND REMAINS A SEPARATE EXPORTABLE MASK FOR RENDERER CONTROL",
        ],
    )


def _response_proof(material: dict[str, Any]) -> np.ndarray:
    normal_rgb = material["normal"] * 0.5 + 0.5
    return compose_material_chapter(
        "FORM AND RESPONSE UNDER NEUTRAL LIGHT",
        "BASE COLOR REMAINS UNLIT WHILE THREE DIRECTIONS TEST HEIGHT AND ROUGHNESS",
        [
            ("UNLIT BASE COLOR", linear_to_srgb(
                material["base_color_linear"]
            )),
            ("TANGENT NORMAL", normal_rgb),
            ("ROUGHNESS", gray_rgb(material["roughness"])),
            ("AMBIENT OCCLUSION", gray_rgb(material["ao"])),
            ("FRONTAL WHITE", material["white_light_frontal_srgb"]),
            ("SIDE WHITE", material["white_light_side_srgb"]),
            ("GRAZING WHITE", material["white_light_grazing_srgb"]),
            ("THREE LIGHT PREVIEW", material["preview_srgb"]),
        ],
        [
            "RECESSED MORTAR BRICK CROWN PITS AND COMPRESSED CLAY MUST CHANGE THE SILHOUETTE OF LIGHT NOT ONLY THE ALBEDO",
            "FIRED SKIN IS SLIGHTLY LESS ROUGH PITS AND MORTAR AGGREGATE ARE ROUGHER AND MORTAR CONTACTS RECEIVE CAVITY OCCLUSION",
            "THE INK MASK SURVIVES ALL LIGHT DIRECTIONS BECAUSE IT DESCRIBES MATERIAL FORM WHILE THE PBR RESPONSE REMAINS MOBILE",
        ],
    )


def _tiling_proof(material: dict[str, Any]) -> np.ndarray:
    final = material["preview_srgb"]
    unlit = linear_to_srgb(material["base_color_linear"])
    return compose_material_chapter(
        "REPETITION AND DISTANCE PRESSURE",
        "ONE TWO FOUR AND EIGHT TILE READS EXPOSE LANDMARKS AND LOST DETAIL",
        [
            ("1X UNLIT", unlit),
            ("2X UNLIT", np.tile(unlit, (2, 2, 1))),
            ("4X UNLIT", np.tile(unlit, (4, 4, 1))),
            ("8X UNLIT", np.tile(unlit, (8, 8, 1))),
            ("1X LIT", final),
            ("2X LIT", np.tile(final, (2, 2, 1))),
            ("4X LIT", np.tile(final, (4, 4, 1))),
            ("8X LIT", np.tile(final, (8, 8, 1))),
        ],
        [
            "THE RUNNING BOND MUST CROSS TILE EDGES WITHOUT A DOUBLED JOINT OR CLIPPED BRICK",
            "CALM BRICKS HOLD THE WALL TOGETHER WHILE CLINKER LAMINATED AND PITTED CHAMPIONS PROVIDE SPARSE LANDMARKS",
            "PRIMARY CONTOURS SHOULD SURVIVE DISTANCE WHILE TERTIARY DRAGS ARE EXPECTED TO FALL AWAY FIRST",
        ],
    )


def _variation_proof(material: dict[str, Any]) -> np.ndarray:
    target = min(256, int(material["resolution"]))
    panels: list[tuple[str, np.ndarray]] = []
    for variation in range(8):
        candidate = generate_material(
            resolution=target,
            seed=int(material["seed"]),
            tile_size_m=float(material["tile_size_m"]),
            recipe_path=material["recipe"].source_path,
            pattern_variation=variation,
        )
        panels.append(
            (
                f"VARIATION {variation:02d}",
                candidate["preview_srgb"],
            )
        )
    return compose_material_chapter(
        "BOUNDED AUTHORED VARIATIONS",
        "COURSE DRIFT AND CHAMPION REORDERING CHANGE WITHOUT LOSING THE BOND",
        panels,
        [
            "VARIATION ZERO IS THE CHAMPION COMPOSITION POSITIVE INDICES MOVE COURSES ONLY INSIDE AUTHORED LIMITS",
            "CHAMPION FACE ROLES REORDER BUT MORTAR SCALE BRICK PROPORTION AND TWENTY SHADE OWNERSHIP REMAIN STABLE",
            "THE PURPOSE IS TO BREAK REPEATED LANDMARKS NOT TO PRODUCE AN UNREVIEWABLE RANDOM WALL",
        ],
    )


def write_material_package(
    material: dict[str, Any],
    output_directory: str | Path,
) -> dict[str, Any]:
    output = Path(output_directory)
    proofs = output / "proofs"
    output.mkdir(parents=True, exist_ok=True)
    proofs.mkdir(parents=True, exist_ok=True)
    paths = {
        "base_color": output / "brick_masonry_v1_basecolor.png",
        "normal": output / "brick_masonry_v1_normal.png",
        "orm": output / "brick_masonry_v1_orm.png",
        "height": output / "brick_masonry_v1_height.png",
        "stylization": output / "brick_masonry_v1_stylization.png",
        "brush_direction": output / "brick_masonry_v1_brush_direction.png",
        "construction_proof": proofs / "brick_masonry_v1_construction.png",
        "color_proof": proofs / "brick_masonry_v1_color_anatomy.png",
        "linework_proof": proofs / "brick_masonry_v1_linework.png",
        "response_proof": proofs / "brick_masonry_v1_response.png",
        "tiling_proof": proofs / "brick_masonry_v1_tiling.png",
        "variation_proof": proofs / "brick_masonry_v1_variations.png",
        "manifest": output / "brick_masonry_v1_manifest.json",
    }
    height = material["height_m"]
    height_min = float(height.min())
    height_max = float(height.max())
    height_encoded = (height - height_min) / max(
        height_max - height_min,
        1.0e-8,
    )
    stylization = np.stack(
        [
            material["ink_mask"],
            material["highlight_stroke_mask"],
            material["detail_priority"],
        ],
        axis=-1,
    )
    brush_direction = np.concatenate(
        [
            material["brush_direction"] * 0.5 + 0.5,
            np.full(
                (*material["brush_direction"].shape[:2], 1),
                0.5,
                dtype=np.float32,
            ),
        ],
        axis=-1,
    )
    write_png_rgb8(
        paths["base_color"],
        linear_to_srgb(material["base_color_linear"]),
    )
    write_png_rgb8(paths["normal"], material["normal"] * 0.5 + 0.5)
    write_png_rgb8(paths["orm"], material["orm"])
    write_png_gray16(paths["height"], height_encoded)
    write_png_rgb8(paths["stylization"], stylization)
    write_png_rgb8(paths["brush_direction"], brush_direction)
    write_png_rgb8(paths["construction_proof"], _construction_proof(material))
    write_png_rgb8(paths["color_proof"], _color_proof(material))
    write_png_rgb8(paths["linework_proof"], _linework_proof(material))
    write_png_rgb8(paths["response_proof"], _response_proof(material))
    write_png_rgb8(paths["tiling_proof"], _tiling_proof(material))
    write_png_rgb8(paths["variation_proof"], _variation_proof(material))
    outputs = {
        name: path.relative_to(output).as_posix()
        for name, path in paths.items()
    }
    manifest = {
        "schema": MATERIAL_SCHEMA,
        "seed": int(material["seed"]),
        "resolution": int(material["resolution"]),
        "tile_size_m": float(material["tile_size_m"]),
        "height_encoding": {
            "file": outputs["height"],
            "format": "16-bit linear grayscale PNG",
            "minimum_m": height_min,
            "maximum_m": height_max,
        },
        "map_contract": {
            "base_color": "sRGB fired-clay and mortar color with structural ink; no baked light",
            "normal": "linear RGB tangent-space OpenGL/Y+",
            "orm": "linear RGB R=AO G=roughness B=metallic",
            "height": "linear normalized data reconstructed from height_encoding",
            "stylization": "linear RGB R=ink G=highlight stroke B=detail priority",
            "brush_direction": "linear RGB; RG stores signed XY direction remapped to zero through one",
        },
        "pattern": material["pattern"],
        "stylization": {
            "shade_family_size_per_brick": int(
                material["shade_family_size"]
            ),
            "line_families": ["primary", "secondary", "tertiary"],
            "proof_brick_id": int(material["proof_brick_id"]),
            "ink_color_srgb": list(INK_SRGB),
        },
        "statistics": {
            "brick_coverage": float(material["brick_mask"].mean()),
            "mortar_coverage": float(material["mortar_mask"].mean()),
            "ink_coverage": float((material["ink_mask"] > 0.12).mean()),
            "highlight_coverage": float(
                (material["highlight_stroke_mask"] > 0.10).mean()
            ),
            "height_min_m": height_min,
            "height_max_m": height_max,
            "roughness_mean": float(material["roughness"].mean()),
        },
        "references": material["recipe"].references,
        "outputs": outputs,
        "reproduction": (
            "Run Blender headless with this generator and the recorded seed, "
            "resolution, recipe, tile size, and variation."
        ),
    }
    paths["manifest"].write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return manifest


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate the authored brick masonry material package."
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).resolve().parent,
    )
    parser.add_argument("--resolution", type=int, default=DEFAULT_RESOLUTION)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED)
    parser.add_argument(
        "--tile-size-m",
        type=float,
        default=DEFAULT_TILE_SIZE_M,
    )
    parser.add_argument(
        "--recipe",
        type=Path,
        default=DEFAULT_RECIPE_PATH,
    )
    parser.add_argument(
        "--pattern-variation",
        type=int,
        default=DEFAULT_VARIATION,
    )
    return parser.parse_args(argv)


def main() -> None:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    args = parse_args(arguments)
    material = generate_material(
        resolution=args.resolution,
        seed=args.seed,
        tile_size_m=args.tile_size_m,
        recipe_path=args.recipe,
        pattern_variation=args.pattern_variation,
    )
    manifest = write_material_package(material, args.output)
    print(
        "brick_masonry_v1 generated:",
        json.dumps(
            {
                "resolution": material["resolution"],
                "seed": material["seed"],
                "pattern": manifest["pattern"],
                "outputs": manifest["outputs"],
            },
            sort_keys=True,
        ),
    )


if __name__ == "__main__":
    main()
