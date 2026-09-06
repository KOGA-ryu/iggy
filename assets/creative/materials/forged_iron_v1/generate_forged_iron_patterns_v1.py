#!/usr/bin/env python3
"""Compile a hand-authored forged-iron face grammar into PBR channel maps."""

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
    height_to_normal,
    linear_to_srgb,
    mix,
    normalized_range,
    srgb_to_linear,
    write_png_gray16,
    write_png_rgb8,
)


PROFILE_PATH = SCRIPT_ROOT / "profiles" / "forged_iron_v1.json"
DEFAULT_PATTERN_PATH = (
    SCRIPT_ROOT / "patterns" / "forged_iron_face_champion_v1.json"
)
DEFAULT_OUTPUT_ROOT = SCRIPT_ROOT / "output"
PATTERN_SCHEMA = "iggy3d.pattern.forged_iron_face.v1"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--pattern", type=Path, default=DEFAULT_PATTERN_PATH)
    parser.add_argument("--resolution-x", type=int, default=1536)
    parser.add_argument("--resolution-y", type=int, default=256)
    return parser.parse_args()


def load_pattern_recipe(path: str | Path) -> dict[str, Any]:
    source = Path(path)
    recipe = json.loads(source.read_text())
    if recipe.get("schema") != PATTERN_SCHEMA:
        raise ValueError(
            f"{source} uses {recipe.get('schema')!r}; expected {PATTERN_SCHEMA!r}"
        )
    tile = recipe["physical_tile_m"]
    if tile["length"] <= 0.0 or tile["width"] <= 0.0:
        raise ValueError("physical tile dimensions must be positive")
    if recipe["variation_count"] != 4:
        raise ValueError("the authored atlas contract requires four variations")
    measured_kinds = {"planishing_face", "cross_peen_face"}
    event_kinds = {event["kind"] for event in recipe["events"]}
    if event_kinds != measured_kinds:
        raise ValueError(f"unexpected forged event kinds: {event_kinds}")
    if len(recipe.get("compact_scale_plates", [])) < 20:
        raise ValueError("compact forge scale requires at least twenty plates")
    if len(recipe.get("authored_micro_modes", [])) < 8:
        raise ValueError("authored micro surface requires at least eight modes")
    worked = recipe.get("worked_surface", {})
    if len(worked.get("passes", [])) < 12:
        raise ValueError("worked iron requires at least twelve finite passes")
    worked_tile = worked.get("physical_tile_m", {})
    if (
        float(worked_tile.get("length", 0.0)) <= float(tile["length"])
        or float(worked_tile.get("width", 0.0)) <= float(tile["width"])
    ):
        raise ValueError(
            "worked iron needs a larger independent non-grid tile"
        )
    spacing_min, spacing_max = worked["visible_bundle_spacing_m"]
    for authored_pass in worked["passes"]:
        spacing = float(authored_pass["bundle_spacing_m"])
        if not float(spacing_min) <= spacing <= float(spacing_max):
            raise ValueError("worked-pass spacing is outside the declared range")
    return recipe


def _wrapped(value: float, period: float) -> float:
    return float(value % period)


def compile_variations(recipe: dict[str, Any]) -> list[dict[str, Any]]:
    """Derive bounded variants from the authored champion, never from a fill."""
    bounds = recipe["variation_bounds"]
    tile = recipe["physical_tile_m"]
    worked_tile = recipe["worked_surface"]["physical_tile_m"]
    seed = int(recipe["variation_seed"])
    variations = []
    for variation_index in range(int(recipe["variation_count"])):
        if variation_index == 0:
            events = [dict(event) for event in recipe["events"]]
        else:
            rng = np.random.default_rng(seed + variation_index * 1613)
            drop_count = variation_index % (
                int(bounds["maximum_event_drop_count"]) + 1
            )
            drop_order = set(
                int(value)
                for value in rng.choice(
                    len(recipe["events"]),
                    size=drop_count,
                    replace=False,
                )
            )
            events = []
            for event_index, champion in enumerate(recipe["events"]):
                if event_index in drop_order:
                    continue
                event = dict(champion)
                jitter_x, jitter_y = bounds["position_jitter_m"]
                event["x_m"] = _wrapped(
                    float(event["x_m"])
                    + float(rng.uniform(-jitter_x, jitter_x)),
                    float(tile["length"]),
                )
                event["y_m"] = _wrapped(
                    float(event["y_m"])
                    + float(rng.uniform(-jitter_y, jitter_y)),
                    float(tile["width"]),
                )
                rotation = float(bounds["rotation_jitter_deg"])
                event["rotation_deg"] = float(event["rotation_deg"]) + float(
                    rng.uniform(-rotation, rotation)
                )
                low, high = bounds["strength_multiplier"]
                event["strength"] = float(event["strength"]) * float(
                    rng.uniform(low, high)
                )
                events.append(event)
        variations.append(
            {
                "variation": variation_index,
                "events": events,
                "scale_plates": [
                    {
                        **plate,
                        "x_m": _wrapped(
                            float(plate["x_m"])
                            + (
                                0.0
                                if variation_index == 0
                                else ((variation_index * 0.0047) % 0.013)
                            ),
                            float(tile["length"]),
                        ),
                        "y_m": _wrapped(
                            float(plate["y_m"])
                            + (
                                0.0
                                if variation_index == 0
                                else ((variation_index * 0.0031) % 0.009)
                            ),
                            float(tile["width"]),
                        ),
                        "rotation_deg": float(plate["rotation_deg"])
                        + variation_index * 3.5,
                    }
                    for plate in recipe["compact_scale_plates"]
                ],
                "worked_passes": [
                    {
                        **authored_pass,
                        "x_m": _wrapped(
                            float(authored_pass["x_m"])
                            + variation_index
                            * (0.009 + (pass_index % 3) * 0.0017),
                            float(worked_tile["length"]),
                        ),
                        "y_m": _wrapped(
                            float(authored_pass["y_m"])
                            + variation_index
                            * (0.0023 + (pass_index % 2) * 0.0011),
                            float(worked_tile["width"]),
                        ),
                        "rotation_deg": (
                            float(authored_pass["rotation_deg"])
                            + variation_index
                            * (-1.7 + (pass_index % 4) * 1.1)
                        ),
                    }
                    for pass_index, authored_pass in enumerate(
                        recipe["worked_surface"]["passes"]
                    )
                ],
                "colour_field_phase": variation_index * 0.017,
                "micro_phase": variation_index * 0.137,
            }
        )
    return variations


def _hex_to_linear(value: str) -> np.ndarray:
    value = value.removeprefix("#")
    srgb = np.asarray(
        [int(value[index : index + 2], 16) / 255.0 for index in (0, 2, 4)],
        dtype=np.float32,
    )
    return srgb_to_linear(srgb)


def _periodic_delta(
    coordinate: np.ndarray,
    centre: float,
    period: float,
) -> np.ndarray:
    return (
        (coordinate - float(centre) + period * 0.5) % period
    ) - period * 0.5


def _rotated_superellipse(
    x: np.ndarray,
    y: np.ndarray,
    *,
    centre_x: float,
    centre_y: float,
    size_x: float,
    size_y: float,
    angle_deg: float,
    period_x: float,
    period_y: float,
    exponent: float,
) -> np.ndarray:
    dx = _periodic_delta(x, centre_x, period_x)
    dy = _periodic_delta(y, centre_y, period_y)
    angle = math.radians(angle_deg)
    cosine = math.cos(angle)
    sine = math.sin(angle)
    local_x = dx * cosine + dy * sine
    local_y = -dx * sine + dy * cosine
    normalized = (
        (np.abs(local_x) / max(size_x * 0.5, 1.0e-8)) ** exponent
        + (np.abs(local_y) / max(size_y * 0.5, 1.0e-8)) ** exponent
    )
    core = np.clip(1.0 - normalized, 0.0, 1.0)
    return (core * core * (3.0 - 2.0 * core)).astype(np.float32)


def _irregular_plate(
    x: np.ndarray,
    y: np.ndarray,
    *,
    centre_x: float,
    centre_y: float,
    radius_x: float,
    radius_y: float,
    angle_deg: float,
    period_x: float,
    period_y: float,
    phase: float,
) -> tuple[np.ndarray, np.ndarray]:
    dx = _periodic_delta(x, centre_x, period_x)
    dy = _periodic_delta(y, centre_y, period_y)
    angle = math.radians(angle_deg)
    cosine = math.cos(angle)
    sine = math.sin(angle)
    local_x = dx * cosine + dy * sine
    local_y = -dx * sine + dy * cosine
    normalized_x = local_x / max(radius_x, 1.0e-8)
    normalized_y = local_y / max(radius_y, 1.0e-8)
    polar = np.arctan2(normalized_y, normalized_x)
    boundary = (
        1.0
        + 0.15 * np.sin(polar * 3.0 + phase)
        + 0.09 * np.sin(polar * 5.0 - phase * 0.71)
        + 0.045 * np.sin(polar * 8.0 + phase * 1.37)
    )
    distance = np.sqrt(normalized_x**2 + normalized_y**2) / boundary

    outer_core = np.clip((1.08 - distance) / 0.20, 0.0, 1.0)
    outer = outer_core * outer_core * (3.0 - 2.0 * outer_core)
    inner_core = np.clip((0.82 - distance) / 0.13, 0.0, 1.0)
    inner = inner_core * inner_core * (3.0 - 2.0 * inner_core)
    return outer.astype(np.float32), inner.astype(np.float32)


def _colour_field(
    x: np.ndarray,
    y: np.ndarray,
    field: dict[str, Any],
    *,
    period_x: float,
    period_y: float,
    phase: float,
) -> np.ndarray:
    radii = field["radius_m"]
    return _rotated_superellipse(
        x,
        y,
        centre_x=_wrapped(float(field["x_m"]) + phase, period_x),
        centre_y=float(field["y_m"]),
        size_x=float(radii[0]) * 2.0,
        size_y=float(radii[1]) * 2.0,
        angle_deg=float(field["rotation_deg"]),
        period_x=period_x,
        period_y=period_y,
        exponent=2.0,
    )


def _rasterize_worked_pass(
    x: np.ndarray,
    y: np.ndarray,
    authored_pass: dict[str, Any],
    *,
    period_x: float,
    period_y: float,
    variation_phase: float,
) -> tuple[np.ndarray, np.ndarray]:
    """Rasterize one finite directional finish pass in physical metres."""
    size_x, size_y = authored_pass["size_m"]
    angle_deg = float(authored_pass["rotation_deg"])
    mask = _rotated_superellipse(
        x,
        y,
        centre_x=float(authored_pass["x_m"]),
        centre_y=float(authored_pass["y_m"]),
        size_x=float(size_x),
        size_y=float(size_y),
        angle_deg=angle_deg,
        period_x=period_x,
        period_y=period_y,
        exponent=4.2,
    )
    dx = _periodic_delta(x, float(authored_pass["x_m"]), period_x)
    dy = _periodic_delta(y, float(authored_pass["y_m"]), period_y)
    angle = math.radians(angle_deg)
    cosine = math.cos(angle)
    sine = math.sin(angle)
    local_x = dx * cosine + dy * sine
    local_y = -dx * sine + dy * cosine
    spacing = float(authored_pass["bundle_spacing_m"])
    phase = math.tau * (
        float(authored_pass["phase"]) + variation_phase
    )
    cross_bundle = np.sin(math.tau * local_y / spacing + phase)
    longitudinal_breakup = (
        0.74
        + 0.17
        * np.sin(
            math.tau * local_x / (spacing * 13.0)
            + phase * 0.61
        )
        + 0.09
        * np.sin(
            math.tau * local_x / (spacing * 29.0)
            - phase * 0.37
        )
    )
    coverage_breakup = np.clip(
        0.82
        + 0.13
        * np.sin(
            math.tau * local_x / (spacing * 8.5)
            + phase * 0.43
        )
        + 0.08
        * np.sin(
            math.tau * local_x / (spacing * 21.0)
            - phase * 0.29
        ),
        0.48,
        1.0,
    )
    strength = float(authored_pass["strength"])
    response = np.clip(
        mask * coverage_breakup * strength,
        0.0,
        1.0,
    ).astype(np.float32)
    signed = (
        cross_bundle * longitudinal_breakup * response
    ).astype(np.float32)
    return response, signed


def _normalize_height_to_ra(
    source: np.ndarray,
    target_ra_m: float,
) -> np.ndarray:
    source = source - float(np.median(source))
    deviation = float(np.mean(np.abs(source)))
    if deviation <= 1.0e-12:
        raise ValueError("height source has no usable variation")
    normalized = np.tanh(source / deviation * 0.52).astype(np.float32)
    normalized -= float(np.median(normalized))
    normalized_deviation = float(np.mean(np.abs(normalized)))
    return (
        normalized * (target_ra_m / normalized_deviation)
    ).astype(np.float32)


def _rasterize_variation(
    recipe: dict[str, Any],
    variation: dict[str, Any],
    profile: dict[str, Any],
    *,
    resolution_x: int,
    resolution_y: int,
) -> dict[str, np.ndarray]:
    tile = recipe["physical_tile_m"]
    length = float(tile["length"])
    width = float(tile["width"])
    coordinate_x = (
        np.arange(resolution_x, dtype=np.float32) + 0.5
    ) / resolution_x * length
    coordinate_y = (
        np.arange(resolution_y, dtype=np.float32) + 0.5
    ) / resolution_y * width
    x, y = np.meshgrid(coordinate_x, coordinate_y)
    worked_tile = recipe["worked_surface"]["physical_tile_m"]
    worked_length = float(worked_tile["length"])
    worked_width = float(worked_tile["width"])
    worked_coordinate_x = (
        np.arange(resolution_x, dtype=np.float32) + 0.5
    ) / resolution_x * worked_length
    worked_coordinate_y = (
        np.arange(resolution_y, dtype=np.float32) + 0.5
    ) / resolution_y * worked_width
    worked_x, worked_y = np.meshgrid(
        worked_coordinate_x,
        worked_coordinate_y,
    )

    planish = np.zeros((resolution_y, resolution_x), dtype=np.float32)
    cross_peen = np.zeros_like(planish)
    signed_planes = np.zeros_like(planish)
    envelopes = recipe["tool_envelopes_m"]
    for event_index, event in enumerate(variation["events"]):
        size = envelopes[event["kind"]]
        mask = _rotated_superellipse(
            x,
            y,
            centre_x=float(event["x_m"]),
            centre_y=float(event["y_m"]),
            size_x=float(size[0]),
            size_y=float(size[1]),
            angle_deg=float(event["rotation_deg"]),
            period_x=length,
            period_y=width,
            exponent=4.0 if event["kind"] == "planishing_face" else 5.0,
        )
        strength = float(event["strength"])
        if event["kind"] == "planishing_face":
            planish = np.maximum(planish, mask * strength)
            sign = -1.0 if event_index % 3 else 0.65
            signed_planes += mask * strength * sign
        else:
            cross_peen = np.maximum(cross_peen, mask * strength)
            sign = 0.72 if event_index % 2 else -0.58
            signed_planes += mask * strength * sign

    activity = np.clip(np.maximum(planish, cross_peen), 0.0, 1.0)

    normalized_x = x / length
    normalized_y = y / width
    micro = np.zeros_like(planish)
    for mode in recipe["authored_micro_modes"]:
        phase = math.tau * (
            float(mode["cycles_x"]) * normalized_x
            + float(mode["cycles_y"]) * normalized_y
            + float(mode["phase"])
            + float(variation["micro_phase"])
        )
        micro += np.sin(phase).astype(np.float32) * float(mode["amplitude"])
    micro /= max(
        sum(abs(float(mode["amplitude"])) for mode in recipe["authored_micro_modes"]),
        1.0e-6,
    )
    micro = np.clip(micro, -1.0, 1.0)
    micro_unit = micro * 0.5 + 0.5

    scale_plate_tone = np.zeros_like(planish)
    scale_lip = np.zeros_like(planish)
    for plate_index, plate in enumerate(variation["scale_plates"]):
        radius_x, radius_y = plate["radius_m"]
        outer, inner = _irregular_plate(
            x,
            y,
            centre_x=float(plate["x_m"]),
            centre_y=float(plate["y_m"]),
            radius_x=float(radius_x),
            radius_y=float(radius_y),
            angle_deg=float(plate["rotation_deg"]),
            period_x=length,
            period_y=width,
            phase=plate_index * 0.83 + float(variation["micro_phase"]) * 2.0,
        )
        strength = float(plate["strength"])
        scale_plate_tone = np.maximum(scale_plate_tone, outer * strength)
        scale_lip = np.maximum(scale_lip, (outer - inner) * strength)
    scale_plate_tone *= 0.90 + micro_unit * 0.10
    scale_plate_tone *= 1.0 - activity * 0.22
    scale_plate_tone = np.clip(scale_plate_tone, 0.0, 0.92)

    # The intact compact scale is continuous. A hammer mask changes the
    # worked plane; it is not evidence that oxide was removed to bright iron.
    exposed_iron = np.zeros_like(activity, dtype=np.float32)

    worked_mask = np.zeros_like(activity)
    worked_direction = np.full_like(activity, 0.5)
    worked_signal = np.zeros_like(activity)
    for pass_index, authored_pass in enumerate(
        variation["worked_passes"]
    ):
        response, signed = _rasterize_worked_pass(
            worked_x,
            worked_y,
            authored_pass,
            period_x=worked_length,
            period_y=worked_width,
            variation_phase=(
                float(variation["variation"]) * 0.071
                + pass_index * 0.013
            ),
        )
        stronger = response > worked_mask
        encoded_direction = (
            float(authored_pass["rotation_deg"]) + 90.0
        ) / 180.0
        worked_direction = np.where(
            stronger,
            encoded_direction,
            worked_direction,
        )
        worked_mask = np.maximum(worked_mask, response)
        worked_signal += signed
    worked_signal = np.clip(worked_signal, -1.0, 1.0)
    normal_proxy_amplitude = float(
        recipe["worked_surface"]["normal_proxy_amplitude_m"]
    )
    worked_proxy_height = (
        worked_signal * normal_proxy_amplitude
    ).astype(np.float32)
    protected_rest = np.clip(
        1.0 - worked_mask * 0.78,
        0.0,
        1.0,
    ).astype(np.float32)
    worked_response = np.stack(
        [
            worked_mask,
            np.clip(worked_direction, 0.0, 1.0),
            protected_rest,
        ],
        axis=-1,
    ).astype(np.float32)

    iron_palette = np.stack(
        [
            _hex_to_linear(value)
            for value in profile["palette"]["exposed_iron_reflectance_srgb"]
        ],
        axis=0,
    )
    scale_palette = np.stack(
        [
            _hex_to_linear(value)
            for value in profile["palette"]["forge_skin_srgb"]
        ],
        axis=0,
    )
    scale_base = np.broadcast_to(
        scale_palette[6],
        (resolution_y, resolution_x, 3),
    ).astype(np.float32, copy=True)
    iron_base = np.broadcast_to(
        iron_palette[5],
        scale_base.shape,
    ).astype(np.float32, copy=True)
    total_weight = np.zeros_like(planish)
    for field in recipe["colour_fields"]:
        mask = _colour_field(
            x,
            y,
            field,
            period_x=length,
            period_y=width,
            phase=float(variation["colour_field_phase"]),
        )
        weight = mask * float(field["strength"])
        scale_base = mix(
            scale_base,
            np.broadcast_to(
                scale_palette[int(field["shade_index"]) % len(scale_palette)],
                scale_base.shape,
            ),
            weight * 0.68,
        )
        iron_base = mix(
            iron_base,
            np.broadcast_to(
                iron_palette[int(field["shade_index"]) % len(iron_palette)],
                iron_base.shape,
            ),
            weight * 0.35,
        )
        total_weight = np.maximum(total_weight, weight)
    scale_plate_colour = np.broadcast_to(
        scale_palette[2],
        scale_base.shape,
    )
    scale_base = mix(
        scale_base,
        scale_plate_colour,
        scale_plate_tone * 0.24,
    )
    iron_base = mix(
        iron_base,
        np.broadcast_to(iron_palette[8], iron_base.shape),
        planish * 0.08,
    )
    iron_base = mix(
        iron_base,
        np.broadcast_to(iron_palette[2], iron_base.shape),
        cross_peen * 0.06,
    )
    base = scale_base
    metalness = exposed_iron

    scale_roughness = (
        0.71
        - planish * 0.035
        + cross_peen * 0.025
        + micro * 0.045
        + (total_weight - 0.35) * 0.018
    )
    scale_roughness += (
        scale_plate_tone * 0.035 + scale_lip * 0.045
    )
    iron_roughness = (
        0.49
        - planish * 0.070
        + cross_peen * 0.040
        + micro * 0.028
        + (total_weight - 0.35) * 0.012
    )
    roughness_range = profile["surface_response"]["roughness_range"]
    scale_roughness_unworked = np.clip(
        scale_roughness,
        0.62,
        float(roughness_range[1]),
    ).astype(np.float32)
    scale_roughness = np.clip(
        scale_roughness_unworked
        - worked_mask
        * float(recipe["worked_surface"]["roughness_reduction_max"]),
        0.56,
        float(roughness_range[1]),
    ).astype(np.float32)
    iron_roughness = np.clip(
        iron_roughness,
        float(roughness_range[0]),
        0.62,
    ).astype(np.float32)
    roughness = (
        scale_roughness * (1.0 - exposed_iron)
        + iron_roughness * exposed_iron
    )
    roughness = np.clip(
        roughness,
        float(roughness_range[0]),
        float(roughness_range[1]),
    ).astype(np.float32)

    target_ra = float(
        profile["surface_response"]["surface_roughness_proxy"]["ra_m"]
    )
    micro_height = _normalize_height_to_ra(micro, target_ra)
    hammer_height = np.clip(signed_planes, -1.0, 1.0) * 0.000085
    scale_height = (
        scale_plate_tone * 0.000004 + scale_lip * 0.000012
    )
    height_m = (
        micro_height
        + hammer_height
        + scale_height
    ).astype(np.float32)
    encoding = profile["surface_response"]["height_encoding"]
    height_m = np.clip(
        height_m,
        float(encoding["minimum_m"]),
        float(encoding["maximum_m"]),
    ).astype(np.float32)
    meters_per_pixel = 0.5 * (
        length / resolution_x + width / resolution_y
    )
    normal = height_to_normal(
        height_m,
        meters_per_pixel,
        strength=1.0,
    )
    macro_normal = height_to_normal(
        hammer_height,
        meters_per_pixel,
        strength=1.0,
    )
    scale_normal = height_to_normal(
        scale_height,
        meters_per_pixel,
        strength=1.0,
    )
    micro_normal = height_to_normal(
        micro_height,
        meters_per_pixel,
        strength=1.0,
    )
    worked_normal = height_to_normal(
        worked_proxy_height,
        0.5
        * (
            worked_length / resolution_x
            + worked_width / resolution_y
        ),
        strength=1.0,
    )
    masks = np.stack([planish, cross_peen, scale_plate_tone], axis=-1)
    layer_response = np.stack(
        [scale_roughness_unworked, iron_roughness, exposed_iron],
        axis=-1,
    )
    return {
        "base_color_linear": np.clip(base, 0.0, 1.0),
        "scale_color_linear": np.clip(scale_base, 0.0, 1.0),
        "iron_color_linear": np.clip(iron_base, 0.0, 1.0),
        "roughness": roughness,
        "metalness": metalness.astype(np.float32),
        "height_m": height_m,
        "normal": normal,
        "macro_height_m": hammer_height.astype(np.float32),
        "scale_height_m": scale_height.astype(np.float32),
        "micro_height_m": micro_height.astype(np.float32),
        "macro_normal": macro_normal,
        "scale_normal": scale_normal,
        "micro_normal": micro_normal,
        "worked_normal": worked_normal,
        "worked_response": worked_response,
        "layer_response": layer_response.astype(np.float32),
        "masks": masks.astype(np.float32),
    }


def generate_material(
    recipe: dict[str, Any],
    *,
    resolution_x: int,
    resolution_y: int,
) -> dict[str, Any]:
    if resolution_x < 128 or resolution_y < 64:
        raise ValueError("forged pattern resolution is too low")
    profile = json.loads(PROFILE_PATH.read_text())
    variations = compile_variations(recipe)
    rasters = [
        _rasterize_variation(
            recipe,
            variation,
            profile,
            resolution_x=resolution_x,
            resolution_y=resolution_y,
        )
        for variation in variations
    ]
    result = {
        key: np.concatenate([raster[key] for raster in rasters], axis=0)
        for key in (
            "base_color_linear",
            "scale_color_linear",
            "iron_color_linear",
            "roughness",
            "metalness",
            "height_m",
            "normal",
            "macro_height_m",
            "scale_height_m",
            "micro_height_m",
            "macro_normal",
            "scale_normal",
            "micro_normal",
            "worked_normal",
            "worked_response",
            "layer_response",
            "masks",
        )
    }
    result["variations"] = variations
    result["recipe"] = recipe
    return result


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _output_record(path: Path, *, channel: str) -> dict[str, Any]:
    return {
        "path": str(path),
        "channel": channel,
        "bytes": path.stat().st_size,
        "sha256": _sha256(path),
    }


def _encode_height(
    height_m: np.ndarray,
    encoding: dict[str, Any],
) -> np.ndarray:
    minimum = float(encoding["minimum_m"])
    maximum = float(encoding["maximum_m"])
    if maximum <= minimum:
        raise ValueError("height encoding maximum must exceed minimum")
    return np.clip(
        (height_m - minimum) / (maximum - minimum),
        0.0,
        1.0,
    ).astype(np.float32)


def _proof_sheet(material: dict[str, Any]) -> np.ndarray:
    scale_color = linear_to_srgb(material["scale_color_linear"])
    iron_color = linear_to_srgb(material["iron_color_linear"])
    layer_response = material["layer_response"]
    height = gray_rgb(normalized_range(material["height_m"]))
    masks = material["masks"]
    macro_normal = material["macro_normal"] * 0.5 + 0.5
    scale_normal = material["scale_normal"] * 0.5 + 0.5
    micro_normal = material["micro_normal"] * 0.5 + 0.5
    worked_normal = material["worked_normal"] * 0.5 + 0.5
    worked_response = material["worked_response"]
    variation_height = scale_color.shape[0] // 4
    variation_zero = slice(0, variation_height)
    return compose_material_chapter(
        "FORGED IRON / NODE-READY PHYSICAL LANES",
        "INTACT SCALE  |  FOUR NORMAL BANDS  |  FINITE WORKED DIRECTION",
        [
            ("DIELECTRIC SCALE", scale_color[variation_zero]),
            ("LAYER RESPONSE RGB", layer_response[variation_zero]),
            ("MACRO HAMMER NORMAL", macro_normal[variation_zero]),
            ("SCALE LIP NORMAL", scale_normal[variation_zero]),
            ("MICRO SURFACE NORMAL", micro_normal[variation_zero]),
            ("WORKED SURFACE NORMAL", worked_normal[variation_zero]),
            ("WORKED RESPONSE RGB", worked_response[variation_zero]),
            ("COMBINED HEIGHT", height[variation_zero]),
        ],
        [
            "Hammering changes planes and reflection; default metalness remains exactly zero.",
            "Macro hammer, scale-lip, micro, and worked normals remain independent until whiteout blending.",
            "Worked response stores finite coverage, direction, and protected rest; it is not scratches or damage.",
            "Macro fabrication scale and world-metre surface scale are reconstructed separately in the shader.",
        ],
    )


def generate(
    output_root: str | Path,
    *,
    pattern_path: str | Path = DEFAULT_PATTERN_PATH,
    resolution_x: int = 1536,
    resolution_y: int = 256,
) -> dict[str, Any]:
    output = Path(output_root).expanduser().resolve()
    output.mkdir(parents=True, exist_ok=True)
    recipe = load_pattern_recipe(pattern_path)
    material = generate_material(
        recipe,
        resolution_x=resolution_x,
        resolution_y=resolution_y,
    )

    base_path = output / "forged_iron_v1_base_color.png"
    scale_color_path = output / "forged_iron_v1_scale_color.png"
    iron_color_path = output / "forged_iron_v1_iron_color.png"
    roughness_path = output / "forged_iron_v1_roughness.png"
    metalness_path = output / "forged_iron_v1_metalness.png"
    layer_response_path = output / "forged_iron_v1_layer_response.png"
    height_path = output / "forged_iron_v1_height.png"
    macro_height_path = output / "forged_iron_v1_macro_height.png"
    scale_height_path = output / "forged_iron_v1_scale_height.png"
    micro_height_path = output / "forged_iron_v1_micro_height.png"
    normal_path = output / "forged_iron_v1_normal.png"
    macro_normal_path = output / "forged_iron_v1_macro_normal.png"
    scale_normal_path = output / "forged_iron_v1_scale_normal.png"
    micro_normal_path = output / "forged_iron_v1_micro_normal.png"
    worked_normal_path = output / "forged_iron_v1_worked_normal.png"
    worked_response_path = output / "forged_iron_v1_worked_response.png"
    masks_path = output / "forged_iron_v1_masks.png"
    proof_path = output / "forged_iron_v1_pattern_atlas.png"
    write_png_rgb8(
        base_path,
        linear_to_srgb(material["base_color_linear"]),
    )
    write_png_rgb8(
        scale_color_path,
        linear_to_srgb(material["scale_color_linear"]),
    )
    write_png_rgb8(
        iron_color_path,
        linear_to_srgb(material["iron_color_linear"]),
    )
    write_png_gray16(roughness_path, material["roughness"])
    write_png_gray16(metalness_path, material["metalness"])
    write_png_rgb8(layer_response_path, material["layer_response"])
    response = json.loads(PROFILE_PATH.read_text())["surface_response"]
    encoding = response["height_encoding"]
    layer_encodings = response["layer_height_encoding"]
    write_png_gray16(
        height_path,
        _encode_height(material["height_m"], encoding),
    )
    write_png_gray16(
        macro_height_path,
        _encode_height(
            material["macro_height_m"],
            layer_encodings["macro_hammer"],
        ),
    )
    write_png_gray16(
        scale_height_path,
        _encode_height(
            material["scale_height_m"],
            layer_encodings["scale_lip"],
        ),
    )
    write_png_gray16(
        micro_height_path,
        _encode_height(
            material["micro_height_m"],
            layer_encodings["micro_surface"],
        ),
    )
    write_png_rgb8(normal_path, material["normal"] * 0.5 + 0.5)
    write_png_rgb8(
        macro_normal_path,
        material["macro_normal"] * 0.5 + 0.5,
    )
    write_png_rgb8(
        scale_normal_path,
        material["scale_normal"] * 0.5 + 0.5,
    )
    write_png_rgb8(
        micro_normal_path,
        material["micro_normal"] * 0.5 + 0.5,
    )
    write_png_rgb8(
        worked_normal_path,
        material["worked_normal"] * 0.5 + 0.5,
    )
    write_png_rgb8(worked_response_path, material["worked_response"])
    write_png_rgb8(masks_path, material["masks"])
    write_png_rgb8(proof_path, _proof_sheet(material))

    manifest = {
        "schema": "iggy-forged-iron-pattern-build/4.0",
        "status": "AUTHORED_FORGE_PATTERN_COMPILED",
        "pattern": str(Path(pattern_path).expanduser().resolve()),
        "profile": str(PROFILE_PATH),
        "physical_tile_m": recipe["physical_tile_m"],
        "worked_physical_tile_m": recipe["worked_surface"][
            "physical_tile_m"
        ],
        "variation_count": recipe["variation_count"],
        "atlas_resolution": [
            resolution_x,
            resolution_y * int(recipe["variation_count"]),
        ],
        "tool_envelopes_m": recipe["tool_envelopes_m"],
        "height_encoding": encoding,
        "layer_height_encoding": layer_encodings,
        "constraints": {
            "uses_ai_generated_imagery": False,
            "uses_damage": False,
            "uses_generic_hammer_noise": False,
            "tool_faces_are_finite": True,
            "photograph_lighting_baked_into_base_color": False,
            "continuous_compact_scale_ground": True,
            "metalness_is_filtered_binary_layer": True,
            "hammering_never_implies_exposed_iron": True,
            "worked_surface_is_finite": True,
            "worked_surface_uses_independent_non_grid_tile": True,
        },
        "layer_statistics": {
            "quiet_fraction_activity_below_0_10": float(
                (
                    np.maximum(
                        material["masks"][..., 0],
                        material["masks"][..., 1],
                    )
                    < 0.10
                ).mean()
            ),
            "scale_plate_tone_fraction_above_0_10": float(
                (material["masks"][..., 2] > 0.10).mean()
            ),
            "conductive_fraction_above_0_90": float(
                (material["metalness"] > 0.90).mean()
            ),
            "filtered_metal_boundary_fraction": float(
                (
                    (material["metalness"] > 0.05)
                    & (material["metalness"] < 0.95)
                ).mean()
            ),
            "worked_fraction_above_0_10": float(
                (material["worked_response"][..., 0] > 0.10).mean()
            ),
            "protected_rest_fraction_above_0_75": float(
                (material["worked_response"][..., 2] > 0.75).mean()
            ),
        },
        "outputs": {
            "base_color": _output_record(
                base_path,
                channel="compatibility composite sRGB base colour",
            ),
            "scale_color": _output_record(
                scale_color_path,
                channel="sRGB dielectric compact-scale colour",
            ),
            "iron_color": _output_record(
                iron_color_path,
                channel="sRGB conductive exposed-iron reflectance",
            ),
            "roughness": _output_record(
                roughness_path,
                channel="linear 16-bit roughness",
            ),
            "metalness": _output_record(
                metalness_path,
                channel="linear 16-bit conductive iron versus dielectric compact scale",
            ),
            "layer_response": _output_record(
                layer_response_path,
                channel="R scale roughness, G iron roughness, B exposed-iron layer mask",
            ),
            "height": _output_record(
                height_path,
                channel="compatibility combined 16-bit signed height",
            ),
            "macro_height": _output_record(
                macro_height_path,
                channel="16-bit signed finite hammer-plane height",
            ),
            "scale_height": _output_record(
                scale_height_path,
                channel="16-bit positive compact-scale lip height",
            ),
            "micro_height": _output_record(
                micro_height_path,
                channel="16-bit signed measured-proxy micro height",
            ),
            "normal": _output_record(
                normal_path,
                channel="compatibility combined OpenGL tangent-space normal",
            ),
            "macro_normal": _output_record(
                macro_normal_path,
                channel="OpenGL tangent-space finite hammer-plane normal",
            ),
            "scale_normal": _output_record(
                scale_normal_path,
                channel="OpenGL tangent-space compact-scale lip normal",
            ),
            "micro_normal": _output_record(
                micro_normal_path,
                channel="OpenGL tangent-space measured-proxy micro normal",
            ),
            "worked_normal": _output_record(
                worked_normal_path,
                channel="OpenGL tangent-space finite worked-face normal",
            ),
            "worked_response": _output_record(
                worked_response_path,
                channel="R finite worked coverage, G encoded local direction, B protected rest",
            ),
            "masks": _output_record(
                masks_path,
                channel="R planishing, G cross-peen, B compact scale plate tone",
            ),
            "proof": _output_record(
                proof_path,
                channel="two material colours, response RGB, three normal bands, combined height, and semantic mask proof",
            ),
        },
    }
    manifest_path = output / "forged_iron_v1_pattern_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def main() -> None:
    args = parse_args()
    manifest = generate(
        args.output_root,
        pattern_path=args.pattern,
        resolution_x=args.resolution_x,
        resolution_y=args.resolution_y,
    )
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
