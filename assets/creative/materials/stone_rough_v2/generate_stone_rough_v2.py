"""Generate a layered, seamless stone PBR material and its proof sheets.

The generator runs in Blender's bundled Python because the repository does not
carry a separate Python environment and Blender supplies NumPy. Blender itself
is not used for image construction; the material is built from deterministic
arrays and written with a small PNG encoder.

Example:
    blender -b --factory-startup \
      --python generate_stone_rough_v2.py -- \
      --resolution 1024
"""

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
    compose_material_book_index,
    compose_material_chapter,
    draw_text as _draw_text,
    gray_rgb as _gray_rgb,
    height_to_normal as _height_to_normal,
    linear_to_srgb,
    mix,
    normalized_range as _normalized_range,
    palette_panel,
    periodic_fbm,
    periodic_gaussian_blur,
    profile_panel,
    proof_resize as _proof_resize,
    render_material_preview,
    scalar_tint,
    seam_frame,
    signed_delta_rgb,
    smoothstep,
    srgb_to_linear,
    write_png_gray16,
    write_png_rgb8,
    zoom_square,
)


MATERIAL_SCHEMA = "iggy3d.material.stone_rough_v2.v1"
PATTERN_SCHEMA = "iggy3d.pattern.site_layout.v1"
DEFAULT_SEED = 240619
DEFAULT_RESOLUTION = 1024
DEFAULT_TILE_SIZE_M = 1.6
DEFAULT_PATTERN_VARIATION = 0
DEFAULT_PATTERN_RECIPE_PATH = (
    Path(__file__).resolve().parent
    / "patterns"
    / "fieldstone_hand_authored_v1.json"
)
MAX_REFERENCE_COLORS = 8

DEFAULT_STONE_PALETTE_SRGB = (
    (82, 81, 75),
    (89, 87, 79),
    (96, 93, 84),
    (104, 101, 91),
    (113, 109, 98),
    (92, 90, 83),
)
DEFAULT_STONE_PALETTE_WEIGHTS = (0.07, 0.20, 0.30, 0.22, 0.07, 0.14)
MORTAR_SRGB = (82, 80, 73)
DIRT_SRGB = (61, 52, 39)


class ReferencePalette:
    def __init__(
        self,
        colors_srgb: list[tuple[int, int, int]],
        weights: list[float],
        source_schema: str,
        source_path: str,
    ) -> None:
        self.colors_srgb = colors_srgb
        self.weights = weights
        self.source_schema = source_schema
        self.source_path = source_path


class PatternRecipe:
    def __init__(
        self,
        *,
        schema: str,
        name: str,
        description: str,
        variation_seed: int,
        layout: dict[str, float],
        roles: dict[str, dict[str, float]],
        sites: list[dict[str, Any]],
        source_path: str,
    ) -> None:
        self.schema = schema
        self.name = name
        self.description = description
        self.variation_seed = variation_seed
        self.layout = layout
        self.roles = roles
        self.sites = sites
        self.source_path = source_path


def load_reference_palette(path: str | Path) -> ReferencePalette:
    """Load reusable palette evidence exported by the ~/font tools.

    Supported inputs are the editable reference-style recipe and the direct
    reference-transfer palette. Explicit outline, shadow, and highlight lanes
    are rejected because they are likely to encode lighting rather than surface
    pigment.
    """

    source_path = Path(path)
    payload = json.loads(source_path.read_text(encoding="utf-8"))
    schema = str(payload.get("schema", "unknown"))
    weighted_colors: list[tuple[tuple[int, int, int], float]] = []

    if schema == "glyph_lab.reference_style_recipe.v0":
        for layer in payload.get("layers", []):
            role = str(layer.get("role", ""))
            name = str(layer.get("name", ""))
            if role in {"linework", "shadow", "highlight"}:
                continue
            if name in {"outline", "dark", "highlight"}:
                continue
            colors = layer.get("palette", [])
            layer_weight = float(
                layer.get("pixel_count", layer.get("cell_count", 1.0))
            )
            color_weight = layer_weight / max(1, len(colors))
            for color in colors:
                weighted_colors.append((_parse_hex_rgb(color), color_weight))
    else:
        for color in payload.get("palette", []):
            weighted_colors.append((_parse_hex_rgb(color), 1.0))

    merged: dict[tuple[int, int, int], float] = {}
    for color, weight in weighted_colors:
        luminance = _relative_luminance_srgb(color)
        if luminance <= 0.025 or luminance >= 0.94:
            continue
        merged[color] = merged.get(color, 0.0) + max(0.0, weight)

    if len(merged) < 2:
        raise ValueError(
            f"{source_path} did not contain at least two usable midtone colors"
        )

    ranked = sorted(
        merged.items(),
        key=lambda item: (-item[1], _relative_luminance_srgb(item[0]), item[0]),
    )[:MAX_REFERENCE_COLORS]
    ranked.sort(key=lambda item: (_relative_luminance_srgb(item[0]), item[0]))
    colors = [color for color, _ in ranked]
    weights = _normalized_weights([weight for _, weight in ranked])
    return ReferencePalette(colors, weights, schema, str(source_path))


def load_pattern_recipe(path: str | Path) -> PatternRecipe:
    """Load an authored, normalized pattern-site recipe."""

    source_path = Path(path)
    payload = json.loads(source_path.read_text(encoding="utf-8"))
    schema = str(payload.get("schema", ""))
    if schema != PATTERN_SCHEMA:
        raise ValueError(
            f"{source_path} uses pattern schema {schema!r}; "
            f"expected {PATTERN_SCHEMA!r}"
        )

    name = str(payload.get("name", "")).strip()
    if not name:
        raise ValueError(f"{source_path} is missing a pattern name")
    layout_payload = payload.get("layout")
    roles_payload = payload.get("roles")
    sites_payload = payload.get("sites")
    if not isinstance(layout_payload, dict):
        raise ValueError(f"{source_path} is missing layout settings")
    if not isinstance(roles_payload, dict) or not roles_payload:
        raise ValueError(f"{source_path} is missing role settings")
    if not isinstance(sites_payload, list) or len(sites_payload) < 3:
        raise ValueError(f"{source_path} requires at least three authored sites")

    layout = {
        "vertical_metric": float(layout_payload.get("vertical_metric", 1.0)),
        "power_weight_scale": float(
            layout_payload.get("power_weight_scale", 0.0)
        ),
        "boundary_warp": float(layout_payload.get("boundary_warp", 0.0)),
    }
    if layout["vertical_metric"] <= 0.0:
        raise ValueError("pattern vertical_metric must be positive")
    if layout["power_weight_scale"] < 0.0:
        raise ValueError("pattern power_weight_scale cannot be negative")
    if not 0.0 <= layout["boundary_warp"] <= 0.1:
        raise ValueError("pattern boundary_warp must be between 0 and 0.1")

    roles: dict[str, dict[str, float]] = {}
    for role_name, role_payload in roles_payload.items():
        if not isinstance(role_payload, dict):
            raise ValueError(f"pattern role {role_name!r} must be an object")
        role = {
            "influence": float(role_payload.get("influence", 1.0)),
            "position_jitter": float(
                role_payload.get("position_jitter", 0.0)
            ),
            "influence_jitter": float(
                role_payload.get("influence_jitter", 0.0)
            ),
        }
        if role["influence"] <= 0.0:
            raise ValueError(
                f"pattern role {role_name!r} influence must be positive"
            )
        if not 0.0 <= role["position_jitter"] <= 0.1:
            raise ValueError(
                f"pattern role {role_name!r} position_jitter is out of range"
            )
        if not 0.0 <= role["influence_jitter"] <= 0.5:
            raise ValueError(
                f"pattern role {role_name!r} influence_jitter is out of range"
            )
        roles[str(role_name)] = role

    identifiers: set[str] = set()
    sites: list[dict[str, Any]] = []
    for index, site_payload in enumerate(sites_payload):
        if not isinstance(site_payload, dict):
            raise ValueError(f"pattern site {index} must be an object")
        identifier = str(site_payload.get("id", "")).strip()
        role_name = str(site_payload.get("role", "")).strip()
        if not identifier or identifier in identifiers:
            raise ValueError(
                f"pattern site {index} has a missing or duplicate id"
            )
        if role_name not in roles:
            raise ValueError(
                f"pattern site {identifier!r} uses unknown role {role_name!r}"
            )
        u = float(site_payload.get("u", -1.0))
        v = float(site_payload.get("v", -1.0))
        influence_scale = float(site_payload.get("influence_scale", 1.0))
        if not 0.0 <= u < 1.0 or not 0.0 <= v < 1.0:
            raise ValueError(
                f"pattern site {identifier!r} must use normalized coordinates"
            )
        if influence_scale <= 0.0:
            raise ValueError(
                f"pattern site {identifier!r} influence_scale must be positive"
            )
        identifiers.add(identifier)
        sites.append(
            {
                "id": identifier,
                "role": role_name,
                "u": u,
                "v": v,
                "influence_scale": influence_scale,
            }
        )

    return PatternRecipe(
        schema=schema,
        name=name,
        description=str(payload.get("description", "")),
        variation_seed=int(payload.get("variation_seed", 0)),
        layout=layout,
        roles=roles,
        sites=sites,
        source_path=str(source_path),
    )


def generate_pattern_layout(
    recipe: PatternRecipe,
    *,
    variation: int,
) -> dict[str, Any]:
    """Create a stable variation around the authored site arrangement."""

    if variation < 0:
        raise ValueError("pattern variation cannot be negative")
    positions = np.asarray(
        [[site["u"], site["v"]] for site in recipe.sites],
        dtype=np.float32,
    )
    influences = np.asarray(
        [
            recipe.roles[site["role"]]["influence"]
            * site["influence_scale"]
            for site in recipe.sites
        ],
        dtype=np.float32,
    )

    if variation > 0:
        rng = np.random.default_rng(
            recipe.variation_seed + variation * 104729
        )
        for index, site in enumerate(recipe.sites):
            settings = recipe.roles[site["role"]]
            position_jitter = settings["position_jitter"]
            influence_jitter = settings["influence_jitter"]
            positions[index] += rng.uniform(
                -position_jitter,
                position_jitter,
                size=2,
            )
            influences[index] *= 1.0 + rng.uniform(
                -influence_jitter,
                influence_jitter,
            )
        positions = np.mod(positions, 1.0).astype(np.float32)

    return {
        "schema": recipe.schema,
        "name": recipe.name,
        "source_path": recipe.source_path,
        "variation": variation,
        "site_ids": tuple(site["id"] for site in recipe.sites),
        "roles": tuple(site["role"] for site in recipe.sites),
        "positions": positions,
        "influences": influences,
        "layout": dict(recipe.layout),
    }


def rasterize_pattern(
    layout: dict[str, Any],
    *,
    resolution: int,
    tile_size_m: float,
    coordinate_u: np.ndarray | None = None,
    coordinate_v: np.ndarray | None = None,
) -> dict[str, np.ndarray]:
    """Rasterize a periodic weighted Voronoi layout and metric edge distance."""

    if resolution <= 0 or tile_size_m <= 0.0:
        raise ValueError("pattern resolution and tile size must be positive")
    if coordinate_u is None or coordinate_v is None:
        axis = np.arange(resolution, dtype=np.float32) / resolution
        coordinate_u = np.broadcast_to(
            axis[np.newaxis, :], (resolution, resolution)
        )
        coordinate_v = np.broadcast_to(
            axis[:, np.newaxis], (resolution, resolution)
        )
    if coordinate_u.shape != (resolution, resolution):
        raise ValueError("pattern U coordinate shape does not match resolution")
    if coordinate_v.shape != (resolution, resolution):
        raise ValueError("pattern V coordinate shape does not match resolution")

    positions = np.asarray(layout["positions"], dtype=np.float32)
    influences = np.asarray(layout["influences"], dtype=np.float32)
    settings = layout["layout"]
    vertical_metric = float(settings["vertical_metric"])
    power_scale = float(settings["power_weight_scale"])
    power_weights = (influences - 1.0) * power_scale

    nearest = np.full((resolution, resolution), np.inf, dtype=np.float32)
    second = np.full((resolution, resolution), np.inf, dtype=np.float32)
    cell_id = np.full((resolution, resolution), -1, dtype=np.int16)
    second_id = np.full((resolution, resolution), -1, dtype=np.int16)
    for index, ((seed_u, seed_v), power_weight) in enumerate(
        zip(positions, power_weights, strict=True)
    ):
        delta_u = np.abs(coordinate_u - seed_u)
        delta_u = np.minimum(delta_u, 1.0 - delta_u)
        delta_v = np.abs(coordinate_v - seed_v)
        delta_v = np.minimum(delta_v, 1.0 - delta_v)
        distance_squared = (
            delta_u * delta_u
            + (delta_v * vertical_metric) * (delta_v * vertical_metric)
            - power_weight
        )
        closer = distance_squared < nearest
        becomes_second = np.logical_and(
            np.logical_not(closer), distance_squared < second
        )
        second_id = np.where(
            closer,
            cell_id,
            np.where(becomes_second, index, second_id),
        )
        second = np.where(
            closer,
            nearest,
            np.where(becomes_second, distance_squared, second),
        )
        nearest = np.where(closer, distance_squared, nearest)
        cell_id = np.where(closer, index, cell_id)

    first_positions = positions[cell_id]
    runner_up_positions = positions[second_id]
    separation_u = np.abs(
        first_positions[..., 0] - runner_up_positions[..., 0]
    )
    separation_u = np.minimum(separation_u, 1.0 - separation_u)
    separation_v = np.abs(
        first_positions[..., 1] - runner_up_positions[..., 1]
    )
    separation_v = np.minimum(separation_v, 1.0 - separation_v)
    separation = np.sqrt(
        separation_u * separation_u
        + (separation_v * vertical_metric) ** 2
    )
    edge_distance = (
        0.5
        * np.maximum(second - nearest, 0.0)
        / np.maximum(separation, 1.0e-6)
        * tile_size_m
    ).astype(np.float32)
    return {
        "cell_id": cell_id,
        "second_id": second_id,
        "edge_distance": edge_distance,
    }


def generate_material(
    *,
    resolution: int = DEFAULT_RESOLUTION,
    seed: int = DEFAULT_SEED,
    tile_size_m: float = DEFAULT_TILE_SIZE_M,
    reference_palette_path: str | Path | None = None,
    pattern_recipe_path: str | Path | None = None,
    pattern_variation: int = DEFAULT_PATTERN_VARIATION,
) -> dict[str, Any]:
    if resolution < 64:
        raise ValueError("resolution must be at least 64 pixels")
    if tile_size_m <= 0.0:
        raise ValueError("tile size must be positive")

    meters_per_pixel = tile_size_m / resolution
    axis = np.arange(resolution, dtype=np.float32) / resolution
    u = np.broadcast_to(axis[np.newaxis, :], (resolution, resolution))
    v = np.broadcast_to(axis[:, np.newaxis], (resolution, resolution))

    pattern_recipe = load_pattern_recipe(
        pattern_recipe_path or DEFAULT_PATTERN_RECIPE_PATH
    )
    pattern_layout = generate_pattern_layout(
        pattern_recipe,
        variation=pattern_variation,
    )
    warp_x = periodic_fbm(resolution, 3, seed + 101, octaves=3)
    warp_y = periodic_fbm(resolution, 3, seed + 211, octaves=3)
    warp_amount = pattern_recipe.layout["boundary_warp"]
    warped_u = np.mod(u + (warp_x - 0.5) * warp_amount, 1.0)
    warped_v = np.mod(v + (warp_y - 0.5) * warp_amount, 1.0)

    pattern_raster = rasterize_pattern(
        pattern_layout,
        resolution=resolution,
        tile_size_m=tile_size_m,
        coordinate_u=warped_u,
        coordinate_v=warped_v,
    )
    cell_id = pattern_raster["cell_id"]
    edge_distance = pattern_raster["edge_distance"]
    mortar_half_m = max(0.0045, meters_per_pixel * 0.65)
    bevel_m = max(0.0070, meters_per_pixel * 0.80)
    stone_mask = smoothstep(
        mortar_half_m, mortar_half_m + bevel_m, edge_distance
    ).astype(np.float32)

    attribute_rng = np.random.default_rng(seed + 3001)
    cell_count = len(pattern_recipe.sites)
    cell_raise = attribute_rng.uniform(0.010, 0.019, cell_count).astype(
        np.float32
    )
    cell_roughness = attribute_rng.uniform(0.70, 0.82, cell_count).astype(
        np.float32
    )
    cell_value = attribute_rng.uniform(0.97, 1.03, cell_count).astype(
        np.float32
    )
    chip_bias = attribute_rng.uniform(0.0, 1.0, cell_count).astype(np.float32)
    cracked_cell = (
        attribute_rng.uniform(0.0, 1.0, cell_count) > 0.72
    ).astype(np.float32)

    face_macro = periodic_fbm(resolution, 5, seed + 401, octaves=4)
    face_grain = periodic_fbm(resolution, 17, seed + 503, octaves=3)
    chip_noise = periodic_fbm(resolution, 11, seed + 607, octaves=4)
    pit_noise = periodic_fbm(resolution, 24, seed + 709, octaves=3)
    mineral_noise = periodic_fbm(resolution, 8, seed + 811, octaves=4)
    dirt_noise = periodic_fbm(resolution, 4, seed + 919, octaves=4)

    interior_start = mortar_half_m + bevel_m * 0.85
    interior_end = mortar_half_m + bevel_m * 3.4
    edge_zone = smoothstep(
        mortar_half_m * 0.65, interior_start, edge_distance
    )
    edge_zone *= 1.0 - smoothstep(
        interior_start, interior_end, edge_distance
    )
    local_chip_threshold = 0.68 + 0.13 * (1.0 - chip_bias[cell_id])
    chip_mask = edge_zone * smoothstep(
        local_chip_threshold, 0.94, chip_noise
    )
    chip_mask = np.clip(chip_mask, 0.0, 1.0).astype(np.float32)

    pit_mask = (
        stone_mask
        * smoothstep(0.79, 0.92, pit_noise)
        * (0.45 + 0.55 * face_grain)
    ).astype(np.float32)
    crack_wave_a = np.abs(
        np.sin(
            2.0
            * math.pi
            * (3.0 * u + 5.0 * v + (face_macro - 0.5) * 1.7)
        )
    )
    crack_wave_b = np.abs(
        np.sin(
            2.0
            * math.pi
            * (7.0 * u - 2.0 * v + (warp_y - 0.5) * 1.2)
        )
    )
    crack_mask = (
        np.maximum(
            1.0 - smoothstep(0.0, 0.055, crack_wave_a),
            1.0 - smoothstep(0.0, 0.045, crack_wave_b),
        )
        * cracked_cell[cell_id]
        * smoothstep(interior_start, interior_end, edge_distance)
    ).astype(np.float32)

    mortar_height = (face_grain - 0.5) * 0.0007
    stone_height = (
        cell_raise[cell_id]
        + (face_macro - 0.5) * 0.0032
        + (face_grain - 0.5) * 0.0010
    )
    undamaged_height_m = (
        mortar_height * (1.0 - stone_mask) + stone_height * stone_mask
    ).astype(np.float32)
    height_m = undamaged_height_m.copy()
    height_m -= chip_mask * 0.0045
    height_m -= pit_mask * 0.0017
    height_m -= crack_mask * 0.0024
    height_m = height_m.astype(np.float32)

    small_sigma = max(1.0, resolution / 512.0)
    large_sigma = max(3.0, resolution / 128.0)
    small_cavity = np.maximum(
        periodic_gaussian_blur(height_m, small_sigma) - height_m, 0.0
    )
    large_cavity = np.maximum(
        periodic_gaussian_blur(height_m, large_sigma) - height_m, 0.0
    )
    small_cavity = _normalize_positive(small_cavity)
    large_cavity = _normalize_positive(large_cavity)
    ao = np.clip(
        1.0 - small_cavity * 0.43 - large_cavity * 0.24,
        0.34,
        1.0,
    ).astype(np.float32)

    normal = _height_to_normal(height_m, meters_per_pixel)
    dirt_mask = (
        np.clip(small_cavity * 0.75 + large_cavity * 0.35, 0.0, 1.0)
        * smoothstep(0.36, 0.74, dirt_noise)
    )
    dirt_mask = np.clip(
        dirt_mask + (1.0 - stone_mask) * 0.12 * dirt_noise,
        0.0,
        1.0,
    ).astype(np.float32)

    if reference_palette_path is not None:
        reference = load_reference_palette(reference_palette_path)
        palette_srgb = reference.colors_srgb
        palette_weights = reference.weights
        palette_metadata = {
            "source": reference.source_path,
            "schema": reference.source_schema,
            "colors_srgb": [list(color) for color in reference.colors_srgb],
            "weights": reference.weights,
            "lighting_extremes_rejected": True,
        }
    else:
        palette_srgb = list(DEFAULT_STONE_PALETTE_SRGB)
        palette_weights = list(DEFAULT_STONE_PALETTE_WEIGHTS)
        palette_metadata = {
            "source": "built_in",
            "schema": "iggy3d.stone_palette.v1",
            "colors_srgb": [list(color) for color in palette_srgb],
            "weights": palette_weights,
            "lighting_extremes_rejected": True,
        }

    palette_linear = srgb_to_linear(
        np.asarray(palette_srgb, dtype=np.float32) / 255.0
    )
    if reference_palette_path is not None:
        palette_linear = _compress_palette_luminance(
            palette_linear, palette_weights, contrast=0.45
        )
        palette_metadata["luminance_contrast_compression"] = 0.45
    palette_choice = attribute_rng.choice(
        len(palette_linear),
        size=cell_count,
        p=np.asarray(_normalized_weights(palette_weights)),
    )
    stone_color = palette_linear[palette_choice[cell_id]].copy()
    stone_color *= cell_value[cell_id][..., np.newaxis]
    cell_pigment_linear = stone_color.copy()

    palette_luminance = np.sum(
        palette_linear * np.array([0.2126, 0.7152, 0.0722], dtype=np.float32),
        axis=1,
    )
    darkest = palette_linear[int(np.argmin(palette_luminance))]
    lightest = palette_linear[int(np.argmax(palette_luminance))]
    average_color = np.average(
        palette_linear,
        axis=0,
        weights=np.asarray(_normalized_weights(palette_weights)),
    )
    cool_tint = np.clip(average_color * np.array([0.93, 0.98, 1.02]), 0.0, 1.0)
    warm_tint = np.clip(average_color * np.array([1.05, 0.98, 0.88]), 0.0, 1.0)
    cool_mask = np.clip((0.5 - face_macro) * 2.0, 0.0, 1.0)
    warm_mask = np.clip((face_macro - 0.5) * 2.0, 0.0, 1.0)
    stone_color = mix(stone_color, cool_tint, cool_mask * 0.08)
    stone_color = mix(stone_color, warm_tint, warm_mask * 0.07)
    temperature_pigment_linear = stone_color.copy()
    mineral_mask = stone_mask * smoothstep(0.77, 0.91, mineral_noise)
    stone_color = mix(stone_color, lightest, mineral_mask * 0.12)
    fresh_chip = np.clip(
        lightest * 0.72 + average_color * 0.28, 0.0, 1.0
    )
    stone_color = mix(stone_color, fresh_chip, chip_mask * 0.24)
    detailed_pigment_linear = stone_color.copy()

    mortar_color = srgb_to_linear(
        np.asarray(MORTAR_SRGB, dtype=np.float32) / 255.0
    )
    base_color_linear = mix(
        np.broadcast_to(mortar_color, stone_color.shape),
        stone_color,
        stone_mask,
    )
    base_color_before_dirt_linear = base_color_linear.copy()
    dirt_color = srgb_to_linear(
        np.asarray(DIRT_SRGB, dtype=np.float32) / 255.0
    )
    base_color_linear = mix(
        base_color_linear, dirt_color, dirt_mask * 0.18
    )
    crack_tint = np.clip(darkest * 0.82, 0.0, 1.0)
    base_color_linear = mix(
        base_color_linear, crack_tint, crack_mask * 0.16
    )
    base_color_linear = np.clip(base_color_linear, 0.0, 1.0).astype(
        np.float32
    )

    stone_roughness = (
        cell_roughness[cell_id] + (face_grain - 0.5) * 0.075
    )
    roughness_base = (
        0.90 * (1.0 - stone_mask) + stone_roughness * stone_mask
    ).astype(np.float32)
    roughness = roughness_base.copy()
    roughness += chip_mask * 0.055
    roughness += pit_mask * 0.035
    roughness += crack_mask * 0.045
    roughness += dirt_mask * 0.085
    roughness = np.clip(roughness, 0.38, 0.98).astype(np.float32)

    orm = np.stack(
        [ao, roughness, np.zeros_like(roughness)], axis=-1
    ).astype(np.float32)
    preview_srgb = render_material_preview(
        base_color_linear, normal, roughness, ao
    )

    return {
        "schema": MATERIAL_SCHEMA,
        "resolution": resolution,
        "seed": seed,
        "tile_size_m": tile_size_m,
        "meters_per_pixel": meters_per_pixel,
        "cell_id": cell_id,
        "edge_distance": edge_distance,
        "warp_x": warp_x,
        "warp_y": warp_y,
        "stone_mask": stone_mask,
        "edge_zone": edge_zone,
        "face_macro": face_macro,
        "face_grain": face_grain,
        "chip_noise": chip_noise,
        "pit_noise": pit_noise,
        "cracked_cell_mask": cracked_cell[cell_id],
        "chip_mask": chip_mask,
        "pit_mask": pit_mask,
        "crack_mask": crack_mask,
        "dirt_mask": dirt_mask,
        "mineral_mask": mineral_mask,
        "cool_mask": cool_mask,
        "warm_mask": warm_mask,
        "cell_raise_m": cell_raise[cell_id],
        "undamaged_height_m": undamaged_height_m,
        "damage_height_delta_m": height_m - undamaged_height_m,
        "small_cavity": small_cavity,
        "large_cavity": large_cavity,
        "height_m": height_m,
        "normal": normal,
        "cell_pigment_linear": cell_pigment_linear,
        "temperature_pigment_linear": temperature_pigment_linear,
        "detailed_pigment_linear": detailed_pigment_linear,
        "base_color_before_dirt_linear": base_color_before_dirt_linear,
        "base_color_linear": base_color_linear,
        "roughness_base": roughness_base,
        "roughness": roughness,
        "ao": ao,
        "orm": orm,
        "preview_srgb": preview_srgb,
        "reference_palette": palette_metadata,
        "pattern": {
            "schema": pattern_recipe.schema,
            "name": pattern_recipe.name,
            "source": pattern_recipe.source_path,
            "variation": pattern_variation,
            "site_count": cell_count,
            "role_counts": {
                role: pattern_layout["roles"].count(role)
                for role in pattern_recipe.roles
            },
        },
        "pattern_recipe": pattern_recipe,
        "parameters": {
            "stone_cell_count": cell_count,
            "mortar_half_width_m": mortar_half_m,
            "bevel_width_m": bevel_m,
            "stone_raise_range_m": [0.010, 0.019],
            "face_relief_m": 0.0042,
            "chip_depth_m": 0.0045,
            "pit_depth_m": 0.0017,
            "crack_depth_m": 0.0024,
        },
    }


def write_material_package(
    material: dict[str, Any],
    output_directory: str | Path,
) -> dict[str, Any]:
    output = Path(output_directory)
    proof_directory = output / "proofs"
    output.mkdir(parents=True, exist_ok=True)
    proof_directory.mkdir(parents=True, exist_ok=True)

    base_color_path = output / "stone_rough_v2_basecolor.png"
    normal_path = output / "stone_rough_v2_normal.png"
    orm_path = output / "stone_rough_v2_orm.png"
    height_path = output / "stone_rough_v2_height.png"
    breakdown_path = proof_directory / "stone_rough_v2_breakdown.png"
    tiling_path = proof_directory / "stone_rough_v2_tiling.png"
    pattern_variations_path = (
        proof_directory / "stone_rough_v2_pattern_variations.png"
    )
    book_directory = proof_directory / "stone_rough_v2_book"
    book_text_path = output / "stone_rough_v2_material_book.md"
    manifest_path = output / "stone_rough_v2_manifest.json"

    base_color_srgb = linear_to_srgb(material["base_color_linear"])
    normal_rgb = material["normal"] * 0.5 + 0.5
    height = material["height_m"]
    height_min = float(height.min())
    height_max = float(height.max())
    height_normalized = (height - height_min) / max(
        height_max - height_min, 1.0e-8
    )

    write_png_rgb8(base_color_path, base_color_srgb)
    write_png_rgb8(normal_path, normal_rgb)
    write_png_rgb8(orm_path, material["orm"])
    write_png_gray16(height_path, height_normalized)
    book_index, book_chapters = _stone_material_book(material)
    write_png_rgb8(breakdown_path, book_index)
    chapter_records = []
    for chapter in book_chapters:
        chapter_path = book_directory / chapter["filename"]
        write_png_rgb8(chapter_path, chapter["page"])
        chapter_records.append(
            {
                "number": chapter["number"],
                "title": chapter["title"],
                "path": chapter_path.relative_to(output).as_posix(),
            }
        )
    book_text_path.write_text(
        _stone_material_book_markdown(material),
        encoding="utf-8",
    )
    write_png_rgb8(tiling_path, _tiling_sheet(material))
    write_png_rgb8(
        pattern_variations_path,
        _pattern_variation_sheet(material),
    )

    outputs = {
        "base_color": base_color_path.relative_to(output).as_posix(),
        "normal": normal_path.relative_to(output).as_posix(),
        "orm": orm_path.relative_to(output).as_posix(),
        "height": height_path.relative_to(output).as_posix(),
        "breakdown": breakdown_path.relative_to(output).as_posix(),
        "tiling": tiling_path.relative_to(output).as_posix(),
        "pattern_variations": pattern_variations_path.relative_to(
            output
        ).as_posix(),
        "book_text": book_text_path.relative_to(output).as_posix(),
        "manifest": manifest_path.relative_to(output).as_posix(),
    }
    cell_areas = np.bincount(
        material["cell_id"].ravel(),
        minlength=material["pattern"]["site_count"],
    )
    manifest = {
        "schema": MATERIAL_SCHEMA,
        "seed": int(material["seed"]),
        "resolution": int(material["resolution"]),
        "tile_size_m": float(material["tile_size_m"]),
        "meters_per_pixel": float(material["meters_per_pixel"]),
        "height_encoding": {
            "file": outputs["height"],
            "format": "16-bit linear grayscale PNG",
            "minimum_m": height_min,
            "maximum_m": height_max,
        },
        "map_contract": {
            "base_color": "sRGB color; no baked AO or lighting",
            "normal": "linear RGB; tangent-space OpenGL/Y+",
            "orm": "linear RGB; R=ambient occlusion G=roughness B=metallic",
            "height": "linear normalized data; reconstruct metres from height_encoding",
        },
        "reference_palette": material["reference_palette"],
        "pattern": material["pattern"],
        "material_book": {
            "index": outputs["breakdown"],
            "narrative": outputs["book_text"],
            "chapters": chapter_records,
        },
        "parameters": material["parameters"],
        "statistics": {
            "height_mean_m": float(material["height_m"].mean()),
            "roughness_mean": float(material["roughness"].mean()),
            "ao_mean": float(material["ao"].mean()),
            "stone_coverage": float(material["stone_mask"].mean()),
            "chip_coverage": float(material["chip_mask"].mean()),
            "crack_coverage": float(material["crack_mask"].mean()),
            "cell_area_coefficient_of_variation": float(
                np.std(cell_areas) / np.mean(cell_areas)
            ),
            "cell_area_p90_p10_ratio": float(
                np.percentile(cell_areas, 90)
                / max(np.percentile(cell_areas, 10), 1.0)
            ),
        },
        "outputs": outputs,
        "reproduction": (
            "Run Blender headless with this script and the recorded seed, "
            "resolution, tile size, pattern recipe, pattern variation, and "
            "optional Inkblotter style recipe."
        ),
    }
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return manifest


def _normalize_positive(value: np.ndarray) -> np.ndarray:
    scale = float(np.percentile(value, 99.0))
    if scale <= 1.0e-10:
        return np.zeros_like(value, dtype=np.float32)
    return np.clip(value / scale, 0.0, 1.0).astype(np.float32)


def _parse_hex_rgb(value: Any) -> tuple[int, int, int]:
    text = str(value).strip()
    if text.startswith("#"):
        text = text[1:]
    if len(text) != 6:
        raise ValueError(f"expected #RRGGBB palette color, got {value!r}")
    try:
        return tuple(int(text[index : index + 2], 16) for index in (0, 2, 4))
    except ValueError as error:
        raise ValueError(
            f"expected #RRGGBB palette color, got {value!r}"
        ) from error


def _relative_luminance_srgb(color: tuple[int, int, int]) -> float:
    linear = srgb_to_linear(np.asarray(color, dtype=np.float32) / 255.0)
    return float(np.dot(linear, np.array([0.2126, 0.7152, 0.0722])))


def _normalized_weights(values: list[float] | tuple[float, ...]) -> list[float]:
    array = np.maximum(np.asarray(values, dtype=np.float64), 0.0)
    total = float(array.sum())
    if total <= 0.0:
        array[:] = 1.0 / len(array)
    else:
        array /= total
    return [float(value) for value in array]


def _compress_palette_luminance(
    palette_linear: np.ndarray,
    weights: list[float] | tuple[float, ...],
    *,
    contrast: float,
) -> np.ndarray:
    """Reduce photographic light/shadow contrast without discarding hue."""

    if not 0.0 <= contrast <= 1.0:
        raise ValueError("palette luminance contrast must be between 0 and 1")
    luminance_weights = np.array(
        [0.2126, 0.7152, 0.0722], dtype=np.float32
    )
    luminance = np.sum(palette_linear * luminance_weights, axis=1)
    normalized = np.asarray(_normalized_weights(weights), dtype=np.float32)
    order = np.argsort(luminance)
    cumulative = np.cumsum(normalized[order])
    median_index = order[int(np.searchsorted(cumulative, 0.5, side="left"))]
    center = float(luminance[median_index])
    target = center + (luminance - center) * contrast
    scale = target / np.maximum(luminance, 1.0e-5)
    compressed = palette_linear * scale[:, np.newaxis]
    return np.clip(compressed, 0.0, 1.0).astype(np.float32)


def _stone_material_book(
    material: dict[str, Any],
) -> tuple[np.ndarray, list[dict[str, Any]]]:
    base_color = linear_to_srgb(material["base_color_linear"])
    normal_rgb = material["normal"] * 0.5 + 0.5
    height_rgb = _gray_rgb(_normalized_range(material["height_m"]))
    undamaged_normal = (
        _height_to_normal(
            material["undamaged_height_m"],
            material["meters_per_pixel"],
        )
        * 0.5
        + 0.5
    )
    edge_scale = max(
        material["parameters"]["bevel_width_m"] * 4.0,
        1.0e-6,
    )
    edge_field = np.clip(material["edge_distance"] / edge_scale, 0.0, 1.0)
    mortar_core = 1.0 - material["stone_mask"]
    bevel_band = 4.0 * material["stone_mask"] * mortar_core
    warp_magnitude = np.sqrt(
        (material["warp_x"] - 0.5) ** 2
        + (material["warp_y"] - 0.5) ** 2
    )
    recipe = material["pattern_recipe"]
    role_colors = {
        "anchor": np.array([0.76, 0.40, 0.18], dtype=np.float32),
        "medium": np.array([0.33, 0.52, 0.62], dtype=np.float32),
        "infill": np.array([0.50, 0.64, 0.28], dtype=np.float32),
    }
    site_colors = np.asarray(
        [role_colors[site["role"]] for site in recipe.sites],
        dtype=np.float32,
    )
    role_map = site_colors[material["cell_id"]]
    cell_areas = np.bincount(
        material["cell_id"].ravel(),
        minlength=len(recipe.sites),
    ).astype(np.float32)
    area_heat = scalar_tint(
        cell_areas[material["cell_id"]],
        (0.10, 0.18, 0.34),
        (0.98, 0.63, 0.18),
        normalize=True,
    )
    damage_rgb = np.stack(
        [
            material["chip_mask"],
            material["crack_mask"],
            material["pit_mask"],
        ],
        axis=-1,
    )
    color_palette = (
        np.asarray(
            material["reference_palette"]["colors_srgb"],
            dtype=np.float32,
        )
        / 255.0
    )
    color_weights = np.asarray(
        material["reference_palette"]["weights"],
        dtype=np.float32,
    )
    palette_image = palette_panel(color_palette, color_weights)
    row = material["resolution"] // 2
    edge_profile = profile_panel(
        [
            edge_field[row],
            material["stone_mask"][row],
            bevel_band[row],
        ],
        [
            (0.95, 0.72, 0.24),
            (0.30, 0.74, 0.92),
            (0.88, 0.32, 0.24),
        ],
    )
    height_profile = profile_panel(
        [
            _normalized_range(material["undamaged_height_m"][row]),
            _normalized_range(material["height_m"][row]),
        ],
        [(0.38, 0.76, 0.94), (0.96, 0.48, 0.22)],
    )
    tiled_base = np.tile(base_color, (2, 2, 1))
    tiled_lit = np.tile(material["preview_srgb"], (2, 2, 1))
    variations = _pattern_variation_sheet(material)

    chapter_specs = [
        {
            "title": "PATTERN LANGUAGE",
            "filename": "01_pattern_language.png",
            "summary": "AUTHORED ROLES BECOME UNEQUAL STONE RHYTHM",
            "hero": role_map,
            "subtitle": "COMPOSITION BEFORE SURFACE DETAIL",
            "panels": [
                ("ROLE MAP", role_map),
                ("CELL IDENTITIES", _cell_id_colors(material["cell_id"])),
                ("AREA HEAT", area_heat),
                (
                    "BOUNDARY WARP",
                    scalar_tint(
                        warp_magnitude,
                        (0.08, 0.16, 0.25),
                        (0.90, 0.62, 0.24),
                        normalize=True,
                    ),
                ),
                ("EDGE FIELD", _gray_rgb(edge_field)),
                ("STONE MASK", _gray_rgb(material["stone_mask"])),
                ("VARIATION FAMILY", variations),
                ("COMPILED READ", base_color),
            ],
            "notes": [
                "EIGHT ANCHORS HOLD THE COMPOSITION THIRTEEN MEDIUM STONES CARRY RHYTHM TEN INFILL STONES CLOSE GAPS",
                "CELL AREA IS INTENTIONALLY UNEQUAL SO THE EYE FINDS LARGE RESTS SMALL INTERRUPTIONS AND NO MACHINE COURSE",
                f"VARIATION {material['pattern']['variation']} REMAINS PERIODIC WHILE ROLE BOUNDS PROTECT THE AUTHORED HIERARCHY",
            ],
        },
        {
            "title": "MORTAR AND BEVEL",
            "filename": "02_mortar_and_bevel.png",
            "summary": "ONE DISTANCE FIELD SCULPTS GAP EDGE AND CROWN",
            "hero": _gray_rgb(material["stone_mask"]),
            "subtitle": "THE JOINT IS A VOLUME NOT A BLACK LINE",
            "panels": [
                ("EDGE DISTANCE", _gray_rgb(edge_field)),
                ("MORTAR CORE", _gray_rgb(mortar_core)),
                ("BEVEL BAND", _gray_rgb(bevel_band)),
                ("EDGE ACTIVITY", _gray_rgb(material["edge_zone"])),
                (
                    "CELL RAISE",
                    scalar_tint(
                        material["cell_raise_m"],
                        (0.16, 0.20, 0.28),
                        (0.96, 0.72, 0.26),
                        normalize=True,
                    ),
                ),
                (
                    "UNDAMAGED HEIGHT",
                    _gray_rgb(
                        _normalized_range(material["undamaged_height_m"])
                    ),
                ),
                ("UNDAMAGED NORMAL", undamaged_normal),
                ("CENTER PROFILE", edge_profile),
            ],
            "notes": [
                f"MORTAR HALF WIDTH IS {material['parameters']['mortar_half_width_m']:.4f} M AND THE ROUNDED BEVEL IS {material['parameters']['bevel_width_m']:.4f} M",
                "THE SAME METRIC EDGE DISTANCE DRIVES MASK HEIGHT NORMAL COLOR SEPARATION AND DAMAGE LOCALIZATION",
                "THIS SHARED PROVENANCE PREVENTS A COLOR JOINT FROM DRIFTING AWAY FROM ITS PHYSICAL GROOVE",
            ],
        },
        {
            "title": "SURFACE RELIEF",
            "filename": "03_surface_relief.png",
            "summary": "MACRO UNDULATION AND FINE GRAIN SHARE ONE HEIGHT",
            "hero": _gray_rgb(
                _normalized_range(material["undamaged_height_m"])
            ),
            "subtitle": "BROAD FORM FIRST FINE WEATHER SECOND",
            "panels": [
                ("FACE MACRO", _gray_rgb(material["face_macro"])),
                ("FACE GRAIN", _gray_rgb(material["face_grain"])),
                (
                    "CELL RAISE",
                    _gray_rgb(_normalized_range(material["cell_raise_m"])),
                ),
                (
                    "MACRO RELIEF",
                    signed_delta_rgb(
                        (material["face_macro"] - 0.5) * 0.0032
                    ),
                ),
                (
                    "GRAIN RELIEF",
                    signed_delta_rgb(
                        (material["face_grain"] - 0.5) * 0.0010
                    ),
                ),
                (
                    "COMBINED HEIGHT",
                    _gray_rgb(
                        _normalized_range(material["undamaged_height_m"])
                    ),
                ),
                ("COMBINED NORMAL", undamaged_normal),
                (
                    "RELIEF CLOSEUP",
                    zoom_square(
                        undamaged_normal,
                        center_u=0.58,
                        center_v=0.45,
                    ),
                ),
            ],
            "notes": [
                "STONE RAISE ESTABLISHES THE BLOCK PLANE MACRO RELIEF BENDS THE FACE AND FINE GRAIN BREAKS THE LAST PERFECT PATCH",
                "RELIEF AMPLITUDES ARE RECORDED IN METRES SO A NEW RESOLUTION DOES NOT SILENTLY CHANGE THE MATERIAL SCALE",
                "THE CLOSEUP IS A TEST OF HIERARCHY LARGE FORM MUST STILL READ AFTER THE FINE FIELD ARRIVES",
            ],
        },
        {
            "title": "DAMAGE GRAMMAR",
            "filename": "04_damage_grammar.png",
            "summary": "CHIPS PITS AND CRACKS OCCUPY DIFFERENT HABITATS",
            "hero": damage_rgb,
            "subtitle": "DAMAGE IS PLACED BY CAUSE NOT SPRINKLED AS NOISE",
            "panels": [
                ("EDGE HABITAT", _gray_rgb(material["edge_zone"])),
                ("CHIP SOURCE", _gray_rgb(material["chip_noise"])),
                ("CHIP RESULT", _gray_rgb(material["chip_mask"])),
                ("PIT SOURCE", _gray_rgb(material["pit_noise"])),
                ("PIT RESULT", _gray_rgb(material["pit_mask"])),
                (
                    "CRACK ELIGIBLE",
                    _gray_rgb(material["cracked_cell_mask"]),
                ),
                ("CRACK RESULT", _gray_rgb(material["crack_mask"])),
                (
                    "HEIGHT REMOVED",
                    signed_delta_rgb(material["damage_height_delta_m"]),
                ),
            ],
            "notes": [
                "CHIPS LIVE ON BEVELS PITS LIVE ON FACES AND CRACKS REQUIRE A CELL LEVEL ELIGIBILITY DECISION",
                f"CHIP PIT AND CRACK DEPTHS ARE {material['parameters']['chip_depth_m']:.4f} M {material['parameters']['pit_depth_m']:.4f} M AND {material['parameters']['crack_depth_m']:.4f} M",
                "THE FINAL PANEL SHOWS ONLY SUBTRACTED HEIGHT SO DAMAGE CAN BE JUDGED WITHOUT COLOR OR LIGHT HIDING ITS DENSITY",
            ],
        },
        {
            "title": "COLOR SCRIPT",
            "filename": "05_color_script.png",
            "summary": "PIGMENT TEMPERATURE MINERAL AND DIRT ARRIVE IN ORDER",
            "hero": base_color,
            "subtitle": "COLOR DESCRIBES MATERIAL HISTORY WITHOUT BAKED LIGHT",
            "panels": [
                ("PALETTE WEIGHT", palette_image),
                (
                    "CELL PIGMENT",
                    linear_to_srgb(material["cell_pigment_linear"]),
                ),
                ("COOL FIELD", _gray_rgb(material["cool_mask"])),
                ("WARM FIELD", _gray_rgb(material["warm_mask"])),
                (
                    "TEMPERATURE PASS",
                    linear_to_srgb(material["temperature_pigment_linear"]),
                ),
                ("MINERAL MASK", _gray_rgb(material["mineral_mask"])),
                (
                    "DETAIL PIGMENT",
                    linear_to_srgb(material["detailed_pigment_linear"]),
                ),
                ("FINAL BASE COLOR", base_color),
            ],
            "notes": [
                "THE PALETTE CHOOSES LOCAL STONE IDENTITY BEFORE BROAD COOL AND WARM FIELDS KEEP NEIGHBOURS FROM FEELING COPIED",
                "MINERAL AND FRESH CHIP COLOR ARE SPARSE MATERIAL EVENTS DIRT AND CRACK TINT ARRIVE ONLY AFTER MORTAR COMPOSITION",
                "NO LIGHT DIRECTION IS PAINTED INTO BASE COLOR THE VALUE SCRIPT REMAINS USEFUL UNDER A FUTURE CEL LIGHTING RAMP",
            ],
        },
        {
            "title": "MATERIAL RESPONSE",
            "filename": "06_material_response.png",
            "summary": "HEIGHT BECOMES NORMAL CAVITY AO AND ROUGHNESS",
            "hero": normal_rgb,
            "subtitle": "EVERY RESPONSE CHANNEL CAN NAME ITS PHYSICAL SOURCE",
            "panels": [
                ("FINAL HEIGHT", height_rgb),
                ("FINAL NORMAL", normal_rgb),
                ("SMALL CAVITY", _gray_rgb(material["small_cavity"])),
                ("LARGE CAVITY", _gray_rgb(material["large_cavity"])),
                ("AMBIENT OCCLUSION", _gray_rgb(material["ao"])),
                ("ROUGHNESS BASE", _gray_rgb(material["roughness_base"])),
                ("ROUGHNESS FINAL", _gray_rgb(material["roughness"])),
                ("LIT RESPONSE", material["preview_srgb"]),
            ],
            "notes": [
                "NORMALS ARE CENTRAL DIFFERENCES OF METRE HEIGHT NOT AN UNRELATED NOISE TEXTURE",
                "TWO CAVITY RADII SEPARATE TINY DAMAGE FROM DEEP JOINTS BEFORE THEY MODULATE AMBIENT OCCLUSION",
                "ROUGHNESS STARTS PER STONE THEN CHIPS PITS CRACKS AND DIRT EACH ADD A SMALL EXPLAINABLE RESPONSE",
            ],
        },
        {
            "title": "SCALE AND SEAMS",
            "filename": "07_scale_and_seams.png",
            "summary": "THE TILE IS INTERROGATED AS A REPEATING WORLD SURFACE",
            "hero": tiled_base,
            "subtitle": "A BEAUTIFUL SINGLE TILE IS NOT ENOUGH",
            "panels": [
                ("BASE TILE", base_color),
                ("BASE REPEAT 2X2", tiled_base),
                ("LIT TILE", material["preview_srgb"]),
                ("LIT REPEAT 2X2", tiled_lit),
                ("SEAM BORDER", seam_frame(base_color)),
                (
                    "HEIGHT BORDER",
                    seam_frame(
                        _gray_rgb(_normalized_range(material["height_m"]))
                    ),
                ),
                ("HEIGHT PROFILE", height_profile),
                ("LAYOUT FAMILY", variations),
            ],
            "notes": [
                f"THE TILE COVERS {material['tile_size_m']:.2f} M AND EACH PIXEL REPRESENTS {material['meters_per_pixel']:.6f} M",
                "GOLD BORDERS EXPOSE THE PERIODIC CONTRACT WHILE TWO BY TWO REPEATS REVEAL LARGE LANDMARKS THAT A SEAM TEST CANNOT",
                "THE VARIATION FAMILY IS THE BRIDGE TO FUTURE MULTI TILE PLACEMENT NOT AN EXCUSE TO HIDE A BAD INDIVIDUAL TILE",
            ],
        },
        {
            "title": "FINAL READ",
            "filename": "08_final_read.png",
            "summary": "THE COMPLETE MATERIAL IS JUDGED ACROSS EVERY CONTRACT",
            "hero": material["preview_srgb"],
            "subtitle": "STRUCTURE PIGMENT RESPONSE AND DISTANCE MUST AGREE",
            "panels": [
                ("BASE COLOR", base_color),
                ("NORMAL", normal_rgb),
                ("HEIGHT", height_rgb),
                ("AO", _gray_rgb(material["ao"])),
                ("ROUGHNESS", _gray_rgb(material["roughness"])),
                ("ORM PACKING", material["orm"]),
                ("LIT PREVIEW", material["preview_srgb"]),
                (
                    "FINAL CLOSEUP",
                    zoom_square(
                        material["preview_srgb"],
                        center_u=0.58,
                        center_v=0.45,
                    ),
                ),
            ],
            "notes": [
                "THE FINAL CHAPTER PUTS DELIVERY MAPS BESIDE THE LOOK THEY PRODUCE SO A BEAUTIFUL PREVIEW CANNOT CONCEAL A BROKEN CHANNEL",
                "THE CLOSEUP ASKS WHETHER BEVEL DAMAGE GRAIN COLOR AND ROUGHNESS STILL DESCRIBE THE SAME PIECE OF STONE",
                "ACCEPTANCE REQUIRES BOTH DISTANCE READABILITY AND A QUIET ENOUGH SURFACE FOR FUTURE PAINTERLY LIGHTING",
            ],
        },
    ]

    chapters: list[dict[str, Any]] = []
    for number, spec in enumerate(chapter_specs, start=1):
        chapters.append(
            {
                "number": number,
                "title": spec["title"].title(),
                "filename": spec["filename"],
                "page": compose_material_chapter(
                    spec["title"],
                    spec["subtitle"],
                    spec["panels"],
                    spec["notes"],
                ),
                "hero": spec["hero"],
                "summary": spec["summary"],
            }
        )
    index = compose_material_book_index(
        "STONE ROUGH V2 MATERIAL BOOK",
        [
            (chapter["title"], chapter["hero"], chapter["summary"])
            for chapter in chapters
        ],
    )
    return index, chapters


def _stone_material_book_markdown(material: dict[str, Any]) -> str:
    parameters = material["parameters"]
    pattern = material["pattern"]
    return f"""# Stone Rough V2 Material Book

This book follows the material from authored composition to final response.
It treats every map as the consequence of a shared construction rather than
as an independent decorative image. The selected pattern is variation
{pattern["variation"]} of `{pattern["name"]}` at a physical tile size of
{material["tile_size_m"]:.2f} metres.

## Chapter 1 — Pattern Language

The stone wall begins with hierarchy. Eight anchor stones create visual rests,
thirteen medium stones connect those rests, and ten infill stones interrupt
gaps that would otherwise reveal a grid. The weighted layout is deliberately
unequal: large stones are not merely scaled medium stones, and the smallest
stones have a compositional job rather than being random debris.

Variation moves each role within a bounded range. It preserves the authored
sentence while changing its cadence. The boundary warp then bends the diagram
slightly, but it is subordinate to the authored placement; noise is never
allowed to become the designer.

The diagnostic views separate role, identity, occupied area, boundary warp,
and final edge distance because each answers a different question. Role asks
why a stone exists. Identity proves that every authored site survives
rasterization. Area exposes a composition that has become too uniform.
Boundary warp shows how much imperfection was added after design. Reading
these views in order makes “less procedural” an editable request instead of a
guess about which noise slider to move.

## Chapter 2 — Mortar and Bevel

Mortar is constructed as physical negative space. A single metric distance
field establishes a {parameters["mortar_half_width_m"]:.4f} m half-gap and a
{parameters["bevel_width_m"]:.4f} m transition into each stone face. That field
also localizes edge wear, so the visible joint, the height groove, the normal
break, and the damage habitat cannot drift apart.

The bevel is a sentence of transition: mortar floor, rounded shoulder, raised
stone crown. Treating it as a black painted line would be cheaper, but it
would collapse under grazing light and betray the material at close range.

The centre profile is the chapter's audit instrument. It overlays distance,
stone occupancy, and bevel activity along one cut through the tile. A broad
white mortar core with no corresponding height fall would reveal a painted
gap; a normal break outside the bevel would reveal a second, conflicting
edge. The profile turns visual intuition into a traceable construction.

## Chapter 3 — Surface Relief

Every stone receives an individual raise between
{parameters["stone_raise_range_m"][0]:.3f} and
{parameters["stone_raise_range_m"][1]:.3f} metres. A broad face field bends
that plane; a finer field gives weather somewhere to live. These frequencies
are separated so that the material can be read first as masonry, then as
stone, and only last as texture.

All amplitudes are expressed in metres. Resolution can change without quietly
turning a chipped wall into a field of mountains or sanding the face flat.

Macro and fine fields are inspected alone before they are allowed to combine.
The macro pass should be visible in the silhouette of light across a face but
quiet in the pigment. Fine grain should interrupt a perfect patch without
breaking the stone into sand. The close study is intentionally unforgiving:
if the eye sees only grain, the hierarchy is upside down; if it sees a flat
polygon, the upper frequencies have been starved.

## Chapter 4 — Damage Grammar

Damage is divided by cause. Chips are permitted near exposed bevels, pits live
on broad faces, and cracks require a cell-level eligibility decision before
their line field can remove height. Their depths are
{parameters["chip_depth_m"]:.4f} m, {parameters["pit_depth_m"]:.4f} m, and
{parameters["crack_depth_m"]:.4f} m respectively.

This separation gives editing language. “Fewer cracks” does not accidentally
erase edge wear; “older mortar” does not pepper every stone with identical
spots. Each family can be art-directed because each can explain why it exists.

Source and result are shown beside one another to reveal selection pressure.
The chip source may be everywhere, but only exposed shoulders should survive
as chips. Pit noise may cover the tile, but mortar and bevels reject it.
Cracks begin even earlier, with entire stones chosen as eligible. The final
height-removed view combines the consequences while preserving the ability to
return to each cause.

## Chapter 5 — Color Script

Color begins with weighted stone pigments, not light and shadow. Broad cool
and warm fields keep neighbouring stones from reading as clones while
preserving a restrained masonry family. Mineral traces, fresh chips, dirt, and
crack tint then arrive in that order, each using a structural mask already
owned by the material.

The base color intentionally avoids a fixed light direction. Its variation
describes pigment and history, leaving a future cel-shading ramp free to move
the scene lighting without fighting a photograph baked into the surface.

Layer order is part of the color design. Cell pigment establishes identity;
temperature fields produce neighbourhood-scale drift; mineral traces create
rare internal events; fresh damage reveals a related but cleaner material;
mortar, dirt, and crack tint finish the history. Showing the intermediate
pigment buffers makes it possible to decide whether a dull result began with
a timid palette or was buried by a later deposit.

## Chapter 6 — Material Response

The final metre-scaled height produces the tangent-space normal. Small and
large cavity searches read different spatial stories: tiny damage and grain at
one radius, deep joints and broad recesses at another. Ambient occlusion is the
measured consequence of those cavities.

Roughness begins with per-stone identity. Chips, pits, cracks, and dirt each
make small additive changes. The response remains varied but legible; there is
no unrelated roughness noise pretending to be material complexity.

The packed delivery map is only the final container. Its red ambient-occlusion
channel comes from cavity, its green roughness channel comes from material
events, and its blue metallic channel remains zero. Keeping those origins
visible matters in a stylized renderer: graphic lighting can simplify the
result, but it should simplify a coherent physical story rather than conceal
three unrelated grayscale images.

## Chapter 7 — Scale and Seams

The tile is {material["tile_size_m"]:.2f} m wide, with
{material["meters_per_pixel"]:.6f} m represented by each pixel. Periodic fields
protect the mathematical seam, while repeated proofs expose a different
failure: memorable landmarks repeating like wallpaper.

The variation contact sheet therefore belongs inside the material story. It
documents the path toward compatible multi-tile placement, but it does not
excuse the selected tile from standing on its own.

Three proofs test three different failure modes. The gold border checks that
periodic data meets itself. The two-by-two repeat checks whether large stones,
cracks, or temperature clouds announce a square tile. The centre profile
checks whether the accumulated height still occupies a believable metric
range. Passing one does not imply the others, which is why they share a
chapter instead of being reduced to a single “seamless” badge.

## Chapter 8 — Final Read

The delivery maps are shown beside the lit consequence they create. Base
color, normal, height, ambient occlusion, roughness, and packed ORM must all
describe the same stones at the same borders. A pleasing preview is not enough
if one of those maps tells a contradictory story.

At close range, the test is poetic but strict: the joint must feel carved, the
edge must remember impact, the face must carry age without static, and color
must leave enough silence for light.

At middle distance, individual damage should recede and the unequal masonry
rhythm should take over. At long distance, the wall should become a stable
value family rather than a checkerboard of stone identities. These reads are
not separate materials; they are the same hierarchy surviving changes of
scale. The final chapter keeps every delivery map beside its consequence so
that technical correctness and visual character are accepted together.
"""


def _tiling_sheet(material: dict[str, Any]) -> np.ndarray:
    tile_size = min(256, int(material["resolution"]))
    base = _proof_resize(
        linear_to_srgb(material["base_color_linear"]), tile_size
    )
    preview = _proof_resize(material["preview_srgb"], tile_size)
    base_grid = np.tile(base, (4, 4, 1))
    preview_grid = np.tile(preview, (4, 4, 1))
    gap = 18
    label_height = 34
    height = tile_size * 4 + label_height
    width = tile_size * 8 + gap
    sheet = np.full((height, width, 3), 0.045, dtype=np.float32)
    sheet[: tile_size * 4, : tile_size * 4] = base_grid
    sheet[
        : tile_size * 4,
        tile_size * 4 + gap :,
    ] = preview_grid
    _draw_text(
        sheet,
        4,
        tile_size * 4 + 9,
        "BASE COLOR 4X4",
        color=(0.88, 0.88, 0.84),
        scale=2,
    )
    _draw_text(
        sheet,
        tile_size * 4 + gap + 4,
        tile_size * 4 + 9,
        "LIT PREVIEW 4X4",
        color=(0.88, 0.88, 0.84),
        scale=2,
    )
    return sheet


def _pattern_variation_sheet(material: dict[str, Any]) -> np.ndarray:
    recipe = material["pattern_recipe"]
    panel_size = 160
    columns = 4
    rows = 3
    label_height = 24
    gap = 8
    margin = 12
    width = margin * 2 + columns * panel_size + (columns - 1) * gap
    height = (
        margin * 2
        + rows * (panel_size + label_height)
        + (rows - 1) * gap
    )
    sheet = np.full((height, width, 3), 0.045, dtype=np.float32)

    axis = np.arange(panel_size, dtype=np.float32) / panel_size
    u = np.broadcast_to(axis[np.newaxis, :], (panel_size, panel_size))
    v = np.broadcast_to(axis[:, np.newaxis], (panel_size, panel_size))
    warp_x = periodic_fbm(panel_size, 3, material["seed"] + 101, octaves=3)
    warp_y = periodic_fbm(panel_size, 3, material["seed"] + 211, octaves=3)
    warp_amount = recipe.layout["boundary_warp"]
    warped_u = np.mod(u + (warp_x - 0.5) * warp_amount, 1.0)
    warped_v = np.mod(v + (warp_y - 0.5) * warp_amount, 1.0)
    role_colors = {
        "anchor": np.array([0.68, 0.40, 0.20], dtype=np.float32),
        "medium": np.array([0.38, 0.49, 0.53], dtype=np.float32),
        "infill": np.array([0.46, 0.57, 0.34], dtype=np.float32),
    }

    for variation in range(columns * rows):
        layout = generate_pattern_layout(recipe, variation=variation)
        raster = rasterize_pattern(
            layout,
            resolution=panel_size,
            tile_size_m=material["tile_size_m"],
            coordinate_u=warped_u,
            coordinate_v=warped_v,
        )
        site_colors = np.asarray(
            [
                role_colors.get(role, np.array([0.5, 0.5, 0.5]))
                for role in layout["roles"]
            ],
            dtype=np.float32,
        )
        identity_value = (
            0.88
            + (
                (np.arange(len(layout["roles"]), dtype=np.float32) * 37.0)
                % 17.0
            )
            / 100.0
        )
        fill = site_colors[raster["cell_id"]]
        fill *= identity_value[raster["cell_id"]][..., np.newaxis]
        stone = smoothstep(0.0045, 0.012, raster["edge_distance"])
        panel = mix(
            np.full_like(fill, 0.075),
            np.clip(fill, 0.0, 1.0),
            stone,
        )
        if variation == material["pattern"]["variation"]:
            panel[:4] = (0.98, 0.72, 0.24)
            panel[-4:] = (0.98, 0.72, 0.24)
            panel[:, :4] = (0.98, 0.72, 0.24)
            panel[:, -4:] = (0.98, 0.72, 0.24)

        row = variation // columns
        column = variation % columns
        x = margin + column * (panel_size + gap)
        y = margin + row * (panel_size + label_height + gap)
        sheet[y : y + panel_size, x : x + panel_size] = panel
        _draw_text(
            sheet,
            x + 3,
            y + panel_size + 5,
            f"VAR {variation}",
            color=(0.88, 0.88, 0.84),
            scale=2,
        )
    return sheet


def _cell_id_colors(cell_id: np.ndarray) -> np.ndarray:
    red = ((cell_id.astype(np.int32) * 73 + 41) % 211 + 30) / 255.0
    green = ((cell_id.astype(np.int32) * 47 + 83) % 197 + 36) / 255.0
    blue = ((cell_id.astype(np.int32) * 97 + 19) % 181 + 42) / 255.0
    return np.stack([red, green, blue], axis=-1).astype(np.float32)


def _arguments(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate the layered stone_rough_v2 material"
    )
    parser.add_argument("--resolution", type=int, default=DEFAULT_RESOLUTION)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED)
    parser.add_argument("--tile-size-m", type=float, default=DEFAULT_TILE_SIZE_M)
    parser.add_argument(
        "--inkblotter-style",
        help=(
            "optional reference_style_recipe.json or reference_palette.json "
            "exported by the ~/font tools"
        ),
    )
    parser.add_argument(
        "--pattern-recipe",
        default=str(DEFAULT_PATTERN_RECIPE_PATH),
        help="authored normalized site-layout recipe",
    )
    parser.add_argument(
        "--pattern-variation",
        type=int,
        default=DEFAULT_PATTERN_VARIATION,
        help="deterministic authored-layout variation index",
    )
    parser.add_argument(
        "--out",
        default=str(Path(__file__).resolve().parent),
        help="material output directory",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    arguments = _arguments(
        sys.argv[sys.argv.index("--") + 1 :]
        if argv is None and "--" in sys.argv
        else ([] if argv is None else argv)
    )
    material = generate_material(
        resolution=arguments.resolution,
        seed=arguments.seed,
        tile_size_m=arguments.tile_size_m,
        reference_palette_path=arguments.inkblotter_style,
        pattern_recipe_path=arguments.pattern_recipe,
        pattern_variation=arguments.pattern_variation,
    )
    manifest = write_material_package(material, arguments.out)
    print(
        "stone_rough_v2 generated:",
        json.dumps(
            {
                "resolution": manifest["resolution"],
                "tile_size_m": manifest["tile_size_m"],
                "seed": manifest["seed"],
                "pattern": manifest["pattern"],
                "outputs": manifest["outputs"],
            },
            sort_keys=True,
        ),
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
