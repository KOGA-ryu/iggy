#!/usr/bin/env python3
"""Build layered, scale-aware three-strand plant-fibre rope texture maps."""

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


PROFILE_PATH = SCRIPT_ROOT / "profiles" / "rope_plant_fibre_v1.json"
DEFAULT_PATTERN_PATH = (
    SCRIPT_ROOT / "patterns" / "three_strand_regular_lay_v1.json"
)
DEFAULT_BUNDLE_PATTERN_PATH = (
    SCRIPT_ROOT / "patterns" / "rope_bundle_tracks_champion_v2.json"
)
DEFAULT_FIBRE_PATTERN_PATH = (
    SCRIPT_ROOT / "patterns" / "rope_fibre_tracks_champion_v2.json"
)
DEFAULT_OUTPUT_ROOT = SCRIPT_ROOT / "output"
PATTERN_SCHEMA = "iggy3d.pattern.rope_plant_fibre.v1"
BUNDLE_PATTERN_SCHEMA = "iggy3d.pattern.rope_bundle_tracks.v2"
FIBRE_PATTERN_SCHEMA = "iggy3d.pattern.rope_fibre_tracks.v2"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--pattern", type=Path, default=DEFAULT_PATTERN_PATH)
    parser.add_argument("--resolution-x", type=int, default=1536)
    parser.add_argument("--resolution-y", type=int, default=384)
    return parser.parse_args()


def load_pattern_recipe(path: str | Path) -> dict[str, Any]:
    source = Path(path)
    recipe = json.loads(source.read_text())
    if recipe.get("schema") != PATTERN_SCHEMA:
        raise ValueError(
            f"{source} uses {recipe.get('schema')!r}; expected {PATTERN_SCHEMA!r}"
        )
    if recipe.get("strand_count") != 3:
        raise ValueError("this capability is explicitly a three-strand rope")
    if recipe.get("lay_direction") == recipe.get("yarn_direction"):
        raise ValueError("regular lay requires opposing rope and yarn directions")
    if recipe.get("variation_count") != 4:
        raise ValueError("the authored atlas contract requires four variations")
    preset_ids = [preset["id"] for preset in recipe["presets"]]
    if preset_ids != ["fine_lashing", "utility_line", "heavy_hawser"]:
        raise ValueError(f"unexpected rope presets: {preset_ids}")
    for preset in recipe["presets"]:
        if preset["diameter_m"] <= 0.0 or preset["lay_length_m"] <= 0.0:
            raise ValueError("rope diameter and lay length must be positive")
        ratio = preset["lay_length_m"] / preset["diameter_m"]
        if not 2.4 <= ratio <= 3.2:
            raise ValueError(
                f"{preset['id']} lay ratio {ratio:.3f} is outside researched bounds"
            )
    return recipe


def load_bundle_recipe(path: str | Path) -> dict[str, Any]:
    source = Path(path)
    recipe = json.loads(source.read_text())
    if recipe.get("schema") != BUNDLE_PATTERN_SCHEMA:
        raise ValueError(
            f"{source} uses {recipe.get('schema')!r}; "
            f"expected {BUNDLE_PATTERN_SCHEMA!r}"
        )
    strand_profiles = recipe.get("strand_profiles", [])
    if [profile.get("strand") for profile in strand_profiles] != [0, 1, 2]:
        raise ValueError("bundle recipe requires explicit profiles for strands 0, 1, 2")
    twist_rates = recipe.get("counter_twist_turns_per_tile", {})
    if set(twist_rates) != {"fine_lashing", "utility_line", "heavy_hawser"}:
        raise ValueError("bundle recipe requires a counter-twist rate per preset")
    if not all(float(rate) < -3.0 for rate in twist_rates.values()):
        raise ValueError("bundle counter-twist must oppose the primary rope lay")
    tracks = recipe.get("track_templates", [])
    if len(tracks) != 11:
        raise ValueError("bundle champion requires eleven authored track templates")
    track_ids = [track.get("id") for track in tracks]
    if len(set(track_ids)) != len(track_ids):
        raise ValueError("bundle track ids must be unique")
    expected_width_classes = {"fine", "narrow", "medium", "wide", "broad"}
    actual_width_classes = {track.get("width_class") for track in tracks}
    if actual_width_classes != expected_width_classes:
        raise ValueError(
            f"bundle width classes {actual_width_classes} do not match "
            f"{expected_width_classes}"
        )
    allowed_events = {"burial", "split", "merge", "flatten", "fade"}
    for track in tracks:
        if not -0.49 < float(track["offset"]) < 0.49:
            raise ValueError(f"{track['id']} offset leaves its primary strand")
        if not 0.0 < float(track["width"]) < 0.16:
            raise ValueError(f"{track['id']} width is outside authored bounds")
        if int(track["subridge_count"]) not in {1, 2, 3}:
            raise ValueError(f"{track['id']} subridge count is outside authored bounds")
        if not 0.88 < float(track["twist_rate_multiplier"]) < 1.12:
            raise ValueError(f"{track['id']} twist rate is outside authored bounds")
        for event in track.get("events", []):
            if event.get("kind") not in allowed_events:
                raise ValueError(
                    f"{track['id']} uses unsupported event {event.get('kind')!r}"
                )
            if event["kind"] == "merge" and event.get("target") not in track_ids:
                raise ValueError(
                    f"{track['id']} merge target {event.get('target')!r} is absent"
                )
    adjustments = recipe.get("per_strand_track_adjustments", [])
    if [adjustment.get("strand") for adjustment in adjustments] != [0, 1, 2]:
        raise ValueError("bundle adjustments require strands 0, 1, 2")
    for adjustment in adjustments:
        if len(adjustment["width_multipliers"]) != len(tracks):
            raise ValueError("bundle width multiplier count does not match tracks")
        if len(adjustment["height_multipliers"]) != len(tracks):
            raise ValueError("bundle height multiplier count does not match tracks")
    return recipe


def load_fibre_recipe(path: str | Path) -> dict[str, Any]:
    source = Path(path)
    recipe = json.loads(source.read_text())
    if recipe.get("schema") != FIBRE_PATTERN_SCHEMA:
        raise ValueError(
            f"{source} uses {recipe.get('schema')!r}; "
            f"expected {FIBRE_PATTERN_SCHEMA!r}"
        )
    transforms = recipe.get("strand_transforms", [])
    if [transform.get("strand") for transform in transforms] != [0, 1, 2]:
        raise ValueError("fibre recipe requires explicit transforms for strands 0, 1, 2")

    preset_order = {
        "fine_lashing": 0,
        "utility_line": 1,
        "heavy_hawser": 2,
    }
    ribbon_ids: set[str] = set()
    for family_name in ("long_ribbons", "short_fibres"):
        elements = recipe.get(family_name, [])
        if not elements:
            raise ValueError(f"fibre recipe has no {family_name}")
        for element in elements:
            element_id = str(element["id"])
            if element_id in ribbon_ids:
                raise ValueError(f"duplicate fibre element id {element_id!r}")
            ribbon_ids.add(element_id)
            if element.get("minimum_preset") not in preset_order:
                raise ValueError(
                    f"{element_id} has unknown preset "
                    f"{element.get('minimum_preset')!r}"
                )
            if not 0.0 < float(element["length"]) < 0.9:
                raise ValueError(f"{element_id} length is outside authored bounds")
            if not 0.0 < float(element["width"]) < 0.08:
                raise ValueError(f"{element_id} width is outside authored bounds")
            if not 0 <= int(element["palette_index"]) < 20:
                raise ValueError(f"{element_id} palette index is outside the profile")
            companion_suffixes: set[str] = set()
            for companion in element.get("companions", []):
                suffix = str(companion["id_suffix"])
                if suffix in companion_suffixes:
                    raise ValueError(
                        f"{element_id} repeats companion suffix {suffix!r}"
                    )
                companion_suffixes.add(suffix)
                if not 0.15 <= float(companion["length_multiplier"]) <= 1.0:
                    raise ValueError(
                        f"{element_id}/{suffix} length multiplier is invalid"
                    )
                if not 0.2 <= float(companion["width_multiplier"]) <= 1.0:
                    raise ValueError(
                        f"{element_id}/{suffix} width multiplier is invalid"
                    )
                if not 0 <= int(companion["palette_index"]) < 20:
                    raise ValueError(
                        f"{element_id}/{suffix} palette index is invalid"
                    )

    long_lengths = [
        float(element["length"]) for element in recipe["long_ribbons"]
    ]
    short_lengths = [
        float(element["length"]) for element in recipe["short_fibres"]
    ]
    if max(long_lengths) / min(long_lengths) < 6.0:
        raise ValueError("long fibre lengths must span at least a six-to-one range")
    if max(short_lengths) / min(short_lengths) < 10.0:
        raise ValueError("short fibre lengths must span at least a ten-to-one range")
    if not any(
        bool(element["geometry_spawn"]) for element in recipe["short_fibres"]
    ):
        raise ValueError("short fibre recipe requires optional silhouette candidates")
    return recipe


def compile_fibre_elements(
    recipe: dict[str, Any],
    *,
    preset_id: str,
    variation_index: int,
) -> dict[str, list[dict[str, Any]]]:
    """Compile authored fibre segments without replacing them with noise."""
    preset_order = {
        "fine_lashing": 0,
        "utility_line": 1,
        "heavy_hawser": 2,
    }
    if preset_id not in preset_order:
        raise ValueError(f"unknown rope preset {preset_id!r}")
    if not 0 <= variation_index < 4:
        raise ValueError("fibre variation index must be between zero and three")
    active_level = preset_order[preset_id]
    bounds = recipe["variation_bounds"]
    compiled: dict[str, list[dict[str, Any]]] = {
        "long_ribbons": [],
        "short_fibres": [],
    }
    for family_index, family_name in enumerate(compiled):
        active_templates = [
            element
            for element in recipe[family_name]
            if preset_order[element["minimum_preset"]] <= active_level
        ]
        for strand, transform in enumerate(recipe["strand_transforms"]):
            for element_index, source in enumerate(active_templates):
                element = json.loads(json.dumps(source))
                element["strand"] = strand
                element["start"] = (
                    float(element["start"])
                    + float(transform["start_offset"])
                ) % 1.0
                element["lateral_offset"] = (
                    float(element["lateral_offset"])
                    * float(transform["lateral_multiplier"])
                )
                element["width"] = (
                    float(element["width"])
                    * float(transform["width_multiplier"])
                )
                element["height"] = (
                    float(element["height"])
                    * float(transform["height_multiplier"])
                )
                element["slope"] = (
                    float(element["slope"])
                    + float(transform["slope_bias"])
                )
                if variation_index:
                    rng = np.random.default_rng(
                        78157
                        + variation_index * 4079
                        + strand * 673
                        + family_index * 10007
                        + element_index * 131
                    )
                    element["start"] = (
                        float(element["start"])
                        + float(
                            rng.uniform(
                                -float(bounds["start_jitter"]),
                                float(bounds["start_jitter"]),
                            )
                        )
                    ) % 1.0
                    element["lateral_offset"] += float(
                        rng.uniform(
                            -float(bounds["lateral_jitter"]),
                            float(bounds["lateral_jitter"]),
                        )
                    )
                    element["width"] *= float(
                        rng.uniform(*bounds["width_multiplier"])
                    )
                    element["length"] *= float(
                        rng.uniform(*bounds["length_multiplier"])
                    )
                    element["slope"] += float(
                        rng.uniform(
                            -float(bounds["slope_jitter"]),
                            float(bounds["slope_jitter"]),
                        )
                    )
                    element["bend_phase"] = (
                        float(element["bend_phase"])
                        + float(
                            rng.uniform(
                                -float(bounds["bend_phase_jitter"]),
                                float(bounds["bend_phase_jitter"]),
                            )
                        )
                    ) % 1.0
                companions = element.pop("companions", [])
                compiled[family_name].append(element)
                for companion_index, companion in enumerate(companions):
                    child = json.loads(json.dumps(element))
                    child["id"] = (
                        f"{element['id']}__{companion['id_suffix']}"
                    )
                    child["companion_of"] = str(element["id"])
                    child["companion_index"] = companion_index
                    child["start"] = (
                        float(child["start"])
                        + float(companion["start_shift"])
                    ) % 1.0
                    child["length"] = min(
                        float(child["length"])
                        * float(companion["length_multiplier"]),
                        0.88,
                    )
                    child["lateral_offset"] = (
                        float(child["lateral_offset"])
                        + float(companion["lateral_offset_delta"])
                        * float(transform["lateral_multiplier"])
                    )
                    child["width"] = (
                        float(child["width"])
                        * float(companion["width_multiplier"])
                    )
                    child["height"] = (
                        float(child["height"])
                        * float(companion["height_multiplier"])
                    )
                    child["palette_index"] = int(
                        companion["palette_index"]
                    )
                    child["sheen"] = float(
                        np.clip(
                            float(child["sheen"])
                            * float(companion["sheen_multiplier"]),
                            0.0,
                            1.0,
                        )
                    )
                    child["valley_bridge"] = float(
                        child.get("valley_bridge", 0.012)
                    ) * 0.82
                    compiled[family_name].append(child)
    return compiled


def compile_bundle_tracks(
    recipe: dict[str, Any],
    *,
    preset_id: str,
    variation_index: int,
) -> list[dict[str, Any]]:
    """Compile bounded variants from one explicit bundle-track champion."""
    preset_order = {
        "fine_lashing": 0,
        "utility_line": 1,
        "heavy_hawser": 2,
    }
    if preset_id not in preset_order:
        raise ValueError(f"unknown rope preset {preset_id!r}")
    if not 0 <= variation_index < 4:
        raise ValueError("bundle variation index must be between zero and three")
    active_level = preset_order[preset_id]
    templates = [
        track
        for track in recipe["track_templates"]
        if preset_order[track["minimum_preset"]] <= active_level
    ]
    bounds = recipe["variation_bounds"]
    compiled: list[dict[str, Any]] = []
    for strand in range(3):
        profile = recipe["strand_profiles"][strand]
        adjustments = recipe["per_strand_track_adjustments"][strand]
        drop_index: int | None = None
        if variation_index > 0 and int(bounds["maximum_track_drop_count"]) > 0:
            drop_index = (
                variation_index * 7 + strand * 3
            ) % len(templates)
        for active_index, source in enumerate(templates):
            if (
                drop_index == active_index
                and preset_id == "heavy_hawser"
                and variation_index == 3
            ):
                continue
            template_index = next(
                index
                for index, candidate in enumerate(recipe["track_templates"])
                if candidate["id"] == source["id"]
            )
            track = json.loads(json.dumps(source))
            track["strand"] = strand
            track["template_index"] = template_index
            track["offset"] = (
                float(track["offset"])
                + float(profile["bundle_offset_bias"])
                + float(adjustments["offset_wave"])
                * math.sin(
                    math.tau
                    * (
                        template_index / len(recipe["track_templates"])
                        + strand * 0.19
                    )
                )
            )
            track["width"] = float(track["width"]) * float(
                adjustments["width_multipliers"][template_index]
            )
            track["height"] = float(track["height"]) * float(
                adjustments["height_multipliers"][template_index]
            )
            track["counter_twist_rate"] = float(
                recipe["counter_twist_turns_per_tile"][preset_id]
            ) * float(track["twist_rate_multiplier"])
            track["counter_twist_phase"] = (
                float(profile["packing_bulge_phase"]) * 0.37
                + strand * 0.113
            )
            track["crown_width_multiplier"] = float(
                profile["crown_width_multiplier"]
            )
            track["crown_flattening"] = float(profile["crown_flattening"])
            track["left_contact_width"] = float(profile["left_contact_width"])
            track["right_contact_width"] = float(profile["right_contact_width"])
            if variation_index > 0:
                rng = np.random.default_rng(
                    39119
                    + variation_index * 3571
                    + strand * 613
                    + template_index * 97
                )
                track["offset"] += float(
                    rng.uniform(
                        -float(bounds["offset_jitter"]),
                        float(bounds["offset_jitter"]),
                    )
                )
                track["width"] *= float(
                    rng.uniform(*bounds["width_multiplier"])
                )
                track["height"] *= float(
                    rng.uniform(*bounds["height_multiplier"])
                )
                track["curve_phase"] = (
                    float(track["curve_phase"])
                    + float(
                        rng.uniform(
                            -float(bounds["curve_phase_jitter"]),
                            float(bounds["curve_phase_jitter"]),
                        )
                    )
                ) % 1.0
                for event in track["events"]:
                    event["start"] = (
                        float(event["start"])
                        + float(
                            rng.uniform(
                                -float(bounds["event_start_jitter"]),
                                float(bounds["event_start_jitter"]),
                            )
                        )
                    ) % 1.0
            compiled.append(track)
    return compiled


def _hex_to_srgb(value: str) -> np.ndarray:
    value = value.removeprefix("#")
    return np.asarray(
        [int(value[index : index + 2], 16) / 255.0 for index in (0, 2, 4)],
        dtype=np.float32,
    )


def _periodic_delta(
    value: np.ndarray,
    centre: float | np.ndarray,
) -> np.ndarray:
    return ((value - centre + 0.5) % 1.0) - 0.5


def _periodic_window(
    coordinate: np.ndarray,
    *,
    start: float,
    length: float,
    edge: float,
) -> np.ndarray:
    centre = (float(start) + float(length) * 0.5) % 1.0
    half_width = float(length) * 0.5
    distance = np.abs(_periodic_delta(coordinate, centre))
    return 1.0 - smoothstep(
        max(half_width - edge, 0.0),
        half_width,
        distance,
    )


def _smootherstep(
    edge0: float,
    edge1: float,
    value: np.ndarray,
) -> np.ndarray:
    if edge1 <= edge0:
        raise ValueError("smootherstep requires edge1 greater than edge0")
    unit = np.clip((value - edge0) / (edge1 - edge0), 0.0, 1.0)
    result = unit * unit * unit * (unit * (unit * 6.0 - 15.0) + 10.0)
    return np.clip(result, 0.0, 1.0)


def _periodic_segment_envelope(
    coordinate: np.ndarray,
    *,
    start: float,
    length: float,
    taper_fraction: float,
) -> tuple[np.ndarray, np.ndarray]:
    """Return a quintic tapered segment and its signed local coordinate."""
    centre = (float(start) + float(length) * 0.5) % 1.0
    half_length = float(length) * 0.5
    signed_distance = _periodic_delta(coordinate, centre)
    normalized_distance = np.abs(signed_distance) / max(half_length, 1.0e-6)
    taper_start = max(0.0, 1.0 - float(taper_fraction))
    envelope = np.clip(
        1.0
        - _smootherstep(
            taper_start,
            1.0,
            normalized_distance,
        ),
        0.0,
        1.0,
    )
    envelope[normalized_distance >= 1.0] = 0.0
    return envelope.astype(np.float32), signed_distance.astype(np.float32)


def _along_strand_coordinate(
    u: np.ndarray,
    v: np.ndarray,
) -> np.ndarray:
    return (
        u
        + v
        + 0.006 * np.sin(math.tau * (2.0 * u - v))
        + 0.003 * np.sin(math.tau * (3.0 * u + 2.0 * v))
    ) % 1.0


def _periodic_harmonics(
    u: np.ndarray,
    v: np.ndarray,
    *,
    seed: int,
    amplitude: float,
    terms: tuple[tuple[int, int, float], ...],
) -> np.ndarray:
    rng = np.random.default_rng(seed)
    result = np.zeros_like(u, dtype=np.float32)
    weight_total = 0.0
    for frequency_u, frequency_v, weight in terms:
        phase = float(rng.uniform(0.0, math.tau))
        result += float(weight) * np.sin(
            math.tau * (frequency_u * u + frequency_v * v) + phase
        )
        weight_total += abs(float(weight))
    if weight_total:
        result *= float(amplitude) / weight_total
    return result.astype(np.float32)


def _height_to_normal_anisotropic(
    height_m: np.ndarray,
    *,
    meters_per_pixel_x: float,
    meters_per_pixel_y: float,
    strength: float,
) -> np.ndarray:
    gradient_x = (
        np.roll(height_m, -1, axis=1)
        - np.roll(height_m, 1, axis=1)
    ) / (2.0 * meters_per_pixel_x)
    gradient_y = (
        np.roll(height_m, -1, axis=0)
        - np.roll(height_m, 1, axis=0)
    ) / (2.0 * meters_per_pixel_y)
    normal = np.stack(
        [
            -gradient_x * strength,
            -gradient_y * strength,
            np.ones_like(height_m),
        ],
        axis=-1,
    )
    normal /= np.maximum(
        np.linalg.norm(normal, axis=-1, keepdims=True),
        1.0e-8,
    )
    return normal.astype(np.float32)


def _compile_variation(
    recipe: dict[str, Any],
    variation_index: int,
) -> dict[str, Any]:
    bounds = recipe["variation_bounds"]
    seed = int(recipe["variation_seed"]) + variation_index * 3571
    rng = np.random.default_rng(seed)
    width_low, width_high = bounds["strand_width_multiplier"]
    fibre_low, fibre_high = bounds["fibre_visibility"]
    quiet_low, quiet_high = bounds["quiet_fraction"]
    return {
        "index": variation_index,
        "seed": seed,
        "lay_phase_turns": float(
            bounds["lay_phase_turns"][variation_index]
        ),
        "strand_width_multiplier": float(rng.uniform(width_low, width_high)),
        "fibre_visibility": float(rng.uniform(fibre_low, fibre_high)),
        "quiet_fraction": float(rng.uniform(quiet_low, quiet_high)),
        "quiet_start": float(rng.uniform(0.04, 0.68)),
        "colour_phase": float(rng.uniform(0.0, 1.0)),
        "yarn_phase": float(rng.uniform(0.0, 1.0)),
    }


def _bundle_track_center(
    track: dict[str, Any],
    along_strand: np.ndarray,
) -> np.ndarray:
    return (
        float(track["offset"])
        + float(track["counter_twist_rate"]) * along_strand
        + float(track["counter_twist_phase"])
        + float(track["curve_amplitude"])
        * np.sin(
            math.tau
            * (
                int(track["curve_frequency"]) * along_strand
                + float(track["curve_phase"])
            )
        )
        + float(track["curve_amplitude"])
        * 0.43
        * np.sin(
            math.tau
            * (
                (int(track["curve_frequency"]) + 2) * along_strand
                + float(track["curve_phase"]) * 1.73
                + int(track["template_index"]) * 0.071
            )
        )
    ).astype(np.float32)


def _rasterize_strand_crowns(
    bundle_recipe: dict[str, Any],
    *,
    strand_coordinate: np.ndarray,
    along_strand: np.ndarray,
    width_multiplier: float,
) -> dict[str, np.ndarray]:
    """Build three separately shaped, asymmetrically packed strand crowns."""
    crowns: list[np.ndarray] = []
    colour_influences: list[np.ndarray] = []
    left_contacts: list[np.ndarray] = []
    right_contacts: list[np.ndarray] = []
    for profile in bundle_recipe["strand_profiles"]:
        strand = int(profile["strand"])
        local_cross = (
            _periodic_delta(strand_coordinate, strand / 3.0) * 3.0
        )
        packing_bulge = (
            1.0
            + float(profile["packing_bulge_strength"])
            * np.sin(
                math.tau
                * (
                    2.0 * along_strand
                    + float(profile["packing_bulge_phase"])
                )
            )
        )
        centre_wander = (
            0.012
            * np.sin(
                math.tau
                * (
                    3.0 * along_strand
                    + float(profile["packing_bulge_phase"]) * 0.71
                )
            )
            + 0.006
            * np.sin(
                math.tau
                * (
                    along_strand
                    - float(profile["packing_bulge_phase"]) * 1.37
                )
            )
        )
        local_cross -= centre_wander

        contact_difference = (
            float(profile["right_contact_width"])
            - float(profile["left_contact_width"])
        )
        nominal_half_width = (
            0.515
            * float(profile["crown_width_multiplier"])
            * float(width_multiplier)
            * packing_bulge
        )
        left_half_width = nominal_half_width * (
            1.0 - contact_difference * 1.65
        )
        right_half_width = nominal_half_width * (
            1.0 + contact_difference * 1.65
        )
        normalized_cross = np.where(
            local_cross < 0.0,
            -local_cross / np.maximum(left_half_width, 1.0e-5),
            local_cross / np.maximum(right_half_width, 1.0e-5),
        )
        clipped_cross = np.clip(normalized_cross, 0.0, 1.0)
        round_crown = np.cos(clipped_cross * math.pi * 0.5)
        crown_exponent = max(
            0.43,
            0.82 - float(profile["crown_flattening"]) * 1.18,
        )
        crown = np.power(
            np.clip(round_crown, 0.0, 1.0),
            crown_exponent,
        )
        crown[normalized_cross >= 1.0] = 0.0
        crown = crown.astype(np.float32)
        crowns.append(crown)
        colour_influences.append(
            np.exp(
                -np.power(
                    normalized_cross / 1.08,
                    4.0,
                )
            ).astype(np.float32)
        )

        contact_band = _smootherstep(0.68, 0.98, normalized_cross)
        contact_band *= 1.0 - _smootherstep(0.98, 1.0, normalized_cross)
        left_contacts.append(
            (contact_band * (local_cross < 0.0)).astype(np.float32)
        )
        right_contacts.append(
            (contact_band * (local_cross >= 0.0)).astype(np.float32)
        )

    crown_stack = np.stack(crowns, axis=-1)
    colour_stack = np.stack(colour_influences, axis=-1)
    colour_weights = colour_stack / np.maximum(
        colour_stack.sum(axis=-1, keepdims=True),
        1.0e-6,
    )
    identity_sum = np.maximum(crown_stack.sum(axis=-1, keepdims=True), 1.0e-6)
    strand_identity = crown_stack / identity_sum
    strand_lobes = np.max(crown_stack, axis=-1).astype(np.float32)
    contact_left = np.stack(left_contacts, axis=-1)
    contact_right = np.stack(right_contacts, axis=-1)
    cavity_large = (
        1.0 - smoothstep(0.08, 0.62, strand_lobes)
    ).astype(np.float32)
    cavity_core = (
        1.0 - smoothstep(0.015, 0.20, strand_lobes)
    ).astype(np.float32)
    return {
        "strand_lobes": strand_lobes,
        "strand_identity": strand_identity.astype(np.float32),
        "strand_colour_weights": colour_weights.astype(np.float32),
        "strand_crowns": crown_stack.astype(np.float32),
        "left_contacts": contact_left,
        "right_contacts": contact_right,
        "cavity_large": cavity_large,
        "cavity_core": cavity_core,
    }


def _rasterize_bundle_tracks(
    bundle_recipe: dict[str, Any],
    profile: dict[str, Any],
    *,
    preset_id: str,
    variation_index: int,
    u: np.ndarray,
    v: np.ndarray,
    strand_coordinate: np.ndarray,
    strand_weights: np.ndarray,
) -> dict[str, Any]:
    tracks = compile_bundle_tracks(
        bundle_recipe,
        preset_id=preset_id,
        variation_index=variation_index,
    )
    palette = np.stack(
        [_hex_to_srgb(value) for value in profile["palette"]["dry_warm_srgb"]]
    )
    along_strand = _along_strand_coordinate(u, v)
    centers = {
        (int(track["strand"]), str(track["id"])): _bundle_track_center(
            track,
            along_strand,
        )
        for track in tracks
    }

    shape = u.shape
    cross_pixel_width = 3.0 / float(shape[0])
    identity = np.zeros(shape, dtype=np.float32)
    height_shape = np.zeros(shape, dtype=np.float32)
    event_mask = np.zeros(shape, dtype=np.float32)
    merge_split_mask = np.zeros(shape, dtype=np.float32)
    edge_mask = np.zeros(shape, dtype=np.float32)
    subridge_light_mask = np.zeros(shape, dtype=np.float32)
    subridge_dark_mask = np.zeros(shape, dtype=np.float32)
    colour_sum = np.zeros((*shape, 3), dtype=np.float32)
    colour_weight = np.zeros(shape, dtype=np.float32)
    roughness_sum = np.zeros(shape, dtype=np.float32)
    dominant_score = np.zeros(shape, dtype=np.float32)
    dominant_colour = np.zeros((*shape, 3), dtype=np.float32)
    dominant_roughness = np.zeros(shape, dtype=np.float32)
    width_class_masks = {
        width_class: np.zeros(shape, dtype=np.float32)
        for width_class in ("fine", "narrow", "medium", "wide", "broad")
    }

    for track in tracks:
        strand = int(track["strand"])
        local_cross = (
            _periodic_delta(strand_coordinate, strand / 3.0) * 3.0
        )
        center = centers[(strand, str(track["id"]))].copy()
        visibility = np.ones(shape, dtype=np.float32)
        height_multiplier = np.ones(shape, dtype=np.float32)
        split_children: list[tuple[np.ndarray, np.ndarray]] = []

        for event in track["events"]:
            window, _ = _periodic_segment_envelope(
                along_strand,
                start=float(event["start"]),
                length=float(event["length"]),
                taper_fraction=0.46,
            )
            event_mask = np.maximum(
                event_mask,
                window * float(event["strength"]),
            )
            if event["kind"] in {"burial", "fade"}:
                visibility *= 1.0 - window * float(event["strength"])
            elif event["kind"] == "flatten":
                height_multiplier *= 1.0 - window * float(event["strength"])
            elif event["kind"] == "merge":
                target_key = (strand, str(event["target"]))
                if target_key in centers:
                    amount = window * float(event["strength"])
                    center = center + _periodic_delta(
                        centers[target_key],
                        center,
                    ) * amount
                    visibility *= 1.0 - amount * 0.58
                    merge_split_mask = np.maximum(merge_split_mask, amount)
            elif event["kind"] == "split":
                amount = window * float(event["strength"])
                child_center = center + float(event["offset_delta"]) * amount
                split_children.append((child_center, amount))
                merge_split_mask = np.maximum(merge_split_mask, amount)

        width_modulation = (
            0.86
            + 0.14
            * (
                0.5
                + 0.5
                * np.sin(
                    math.tau
                    * (
                        (int(track["template_index"]) % 3 + 1) * along_strand
                        + strand * 0.23
                        + float(track["curve_phase"])
                    )
                )
            )
        )
        width = np.maximum(
            float(track["width"]) * width_modulation,
            0.008,
        )
        distance = np.abs(_periodic_delta(local_cross, center))
        coverage_radius = np.maximum(
            width * 0.5,
            cross_pixel_width * 0.64,
        )
        ridge_radius = np.maximum(
            width * 0.38,
            cross_pixel_width * 0.52,
        )
        coverage = np.exp(
            -np.power(distance / coverage_radius, 4.0)
        ).astype(np.float32)
        broad_ridge = np.exp(
            -np.power(distance / ridge_radius, 2.0)
        ).astype(np.float32)
        ridge = broad_ridge * 0.38
        track_subridge_light = np.zeros(shape, dtype=np.float32)
        track_subridge_dark = np.zeros(shape, dtype=np.float32)
        subridge_count = int(track["subridge_count"])
        subridge_width = width / (subridge_count * 0.92)
        for subridge_index in range(subridge_count):
            lateral_fraction = (
                subridge_index - (subridge_count - 1) * 0.5
            ) / max(subridge_count, 1)
            subridge_center = (
                center
                + lateral_fraction * width * 0.86
                + width
                * 0.035
                * np.sin(
                    math.tau
                    * (
                        (subridge_index + 1) * along_strand
                        + float(track["curve_phase"]) * 1.31
                    )
                )
            )
            subridge_distance = np.abs(
                _periodic_delta(local_cross, subridge_center)
            )
            subridge = np.exp(
                -np.power(
                    subridge_distance
                    / np.maximum(
                        subridge_width * 0.46,
                        cross_pixel_width * 0.48,
                    ),
                    2.0,
                )
            ).astype(np.float32)
            longitudinal_strength = (
                0.74
                + 0.26
                * (
                    0.5
                    + 0.5
                    * np.sin(
                        math.tau
                        * (
                            (subridge_index + 2) * along_strand
                            + strand * 0.29
                            + int(track["template_index"]) * 0.113
                        )
                    )
                )
            )
            subridge *= longitudinal_strength
            ridge = np.maximum(ridge, subridge)
            tone_selector = (
                int(track["template_index"]) + subridge_index + strand
            ) % 3
            if tone_selector == 0:
                track_subridge_light = np.maximum(
                    track_subridge_light,
                    subridge,
                )
            elif tone_selector == 1:
                track_subridge_dark = np.maximum(
                    track_subridge_dark,
                    subridge,
                )
        for child_center, child_amount in split_children:
            child_distance = np.abs(
                _periodic_delta(local_cross, child_center)
            )
            child_coverage = np.exp(
                -np.power(
                    child_distance / np.maximum(width * 0.34, 0.003),
                    4.0,
                )
            ).astype(np.float32)
            child_ridge = np.exp(
                -np.power(
                    child_distance / np.maximum(width * 0.25, 0.0025),
                    2.0,
                )
            ).astype(np.float32)
            child_visibility = np.power(child_amount, 0.72)
            child_coverage *= child_visibility
            child_ridge *= child_visibility
            coverage = 1.0 - (
                (1.0 - coverage) * (1.0 - child_coverage)
            )
            ridge = np.maximum(ridge, child_ridge)
        strand_gate = np.power(
            np.clip(strand_weights[..., strand], 0.0, 1.0),
            0.42,
        )
        mask = coverage * visibility * strand_gate
        ridge *= visibility * strand_gate
        track_subridge_light *= visibility * strand_gate
        track_subridge_dark *= visibility * strand_gate
        edge_mask = np.maximum(
            edge_mask,
            np.clip(mask - ridge * 0.74, 0.0, 1.0),
        )
        subridge_light_mask = np.maximum(
            subridge_light_mask,
            track_subridge_light,
        )
        subridge_dark_mask = np.maximum(
            subridge_dark_mask,
            track_subridge_dark,
        )

        height_track = (
            ridge
            * float(track["height"])
            * height_multiplier
        ).astype(np.float32)
        identity = 1.0 - (1.0 - identity) * (1.0 - mask)
        height_shape = np.sqrt(
            height_shape * height_shape + height_track * height_track
        )
        width_class = str(track["width_class"])
        width_class_masks[width_class] = 1.0 - (
            (1.0 - width_class_masks[width_class]) * (1.0 - mask)
        )
        colour = palette[int(track["palette_index"])]
        colour_sum += mask[..., np.newaxis] * colour
        colour_weight += mask
        roughness_sum += mask * float(track["roughness_bias"])
        dominant = height_track > dominant_score
        dominant_score = np.where(dominant, height_track, dominant_score)
        dominant_colour = np.where(
            dominant[..., np.newaxis],
            colour,
            dominant_colour,
        )
        dominant_roughness = np.where(
            dominant,
            float(track["roughness_bias"]),
            dominant_roughness,
        )

    safe_weight = np.maximum(colour_weight, 1.0e-6)
    average_colour = colour_sum / safe_weight[..., np.newaxis]
    average_roughness = roughness_sum / safe_weight
    colour = mix(
        average_colour,
        dominant_colour,
        np.clip(dominant_score * 0.75, 0.0, 0.68),
    )
    roughness_bias = (
        average_roughness * 0.62 + dominant_roughness * 0.38
    )
    return {
        "tracks": tracks,
        "identity": np.clip(identity, 0.0, 1.0),
        "height_shape": np.clip(height_shape, 0.0, 1.35),
        "colour_srgb": np.clip(colour, 0.0, 1.0),
        "colour_weight": np.clip(colour_weight, 0.0, 1.0),
        "roughness_bias": np.clip(roughness_bias, -0.06, 0.06),
        "edge_mask": np.clip(edge_mask, 0.0, 1.0),
        "subridge_light_mask": np.clip(
            subridge_light_mask,
            0.0,
            1.0,
        ),
        "subridge_dark_mask": np.clip(
            subridge_dark_mask,
            0.0,
            1.0,
        ),
        "event_mask": np.clip(event_mask, 0.0, 1.0),
        "merge_split_mask": np.clip(merge_split_mask, 0.0, 1.0),
        "width_class_masks": width_class_masks,
    }


def _rasterize_fibre_tracks(
    fibre_recipe: dict[str, Any],
    profile: dict[str, Any],
    *,
    preset_id: str,
    variation_index: int,
    u: np.ndarray,
    v: np.ndarray,
    strand_coordinate: np.ndarray,
    strand_weights: np.ndarray,
    bundle_tracks: list[dict[str, Any]],
    quiet: np.ndarray,
) -> dict[str, Any]:
    """Rasterize finite fibre blades and stubs owned by coarse bundle tracks."""
    elements = compile_fibre_elements(
        fibre_recipe,
        preset_id=preset_id,
        variation_index=variation_index,
    )
    palette = np.stack(
        [_hex_to_srgb(value) for value in profile["palette"]["dry_warm_srgb"]]
    )
    along_strand = _along_strand_coordinate(u, v)
    bundle_centers = {
        (int(track["strand"]), str(track["id"])): _bundle_track_center(
            track,
            along_strand,
        )
        for track in bundle_tracks
    }

    shape = u.shape
    cross_pixel_width = 3.0 / float(shape[0])
    long_mask = np.zeros(shape, dtype=np.float32)
    short_mask = np.zeros(shape, dtype=np.float32)
    geometry_spawn = np.zeros(shape, dtype=np.float32)
    height_shape = np.zeros(shape, dtype=np.float32)
    long_height_shape = np.zeros(shape, dtype=np.float32)
    short_height_shape = np.zeros(shape, dtype=np.float32)
    normal_only_mask = np.zeros(shape, dtype=np.float32)
    sheen_mask = np.zeros(shape, dtype=np.float32)
    lifted_mask = np.zeros(shape, dtype=np.float32)
    colour_sum = np.zeros((*shape, 3), dtype=np.float32)
    colour_weight = np.zeros(shape, dtype=np.float32)
    roughness_sum = np.zeros(shape, dtype=np.float32)
    roughness_weight = np.zeros(shape, dtype=np.float32)

    for family_name in ("long_ribbons", "short_fibres"):
        for element in elements[family_name]:
            strand = int(element["strand"])
            bundle_key = (strand, str(element["bundle_id"]))
            if bundle_key not in bundle_centers:
                continue
            envelope, signed_along = _periodic_segment_envelope(
                along_strand,
                start=float(element["start"]),
                length=float(element["length"]),
                taper_fraction=0.28 if family_name == "long_ribbons" else 0.52,
            )
            local_unit = signed_along / max(float(element["length"]), 1.0e-6)
            bend_cycles = int(element.get("bend_cycles", 1))
            bend = (
                float(element["bend_amplitude"])
                * np.sin(
                    math.tau
                    * (
                        bend_cycles * local_unit
                        + float(element["bend_phase"])
                    )
                )
                * np.power(envelope, 0.55)
            )
            centre = (
                bundle_centers[bundle_key]
                + float(element["lateral_offset"])
                + float(element["slope"]) * signed_along
                + bend
            )
            local_cross = (
                _periodic_delta(strand_coordinate, strand / 3.0) * 3.0
            )
            taper = 0.18 + 0.82 * np.power(envelope, 0.42)
            minimum_width = cross_pixel_width * (
                0.78 if family_name == "long_ribbons" else 0.64
            )
            width = np.maximum(
                float(element["width"]) * taper,
                minimum_width,
            )
            distance = np.abs(_periodic_delta(local_cross, centre))
            coverage_radius = np.maximum(
                width * 0.5,
                cross_pixel_width * 0.64,
            )
            ridge_radius = np.maximum(
                width * 0.36,
                cross_pixel_width * 0.52,
            )
            coverage = np.exp(
                -np.power(distance / coverage_radius, 4.0)
            ).astype(np.float32)
            ridge = np.exp(
                -np.power(distance / ridge_radius, 2.0)
            ).astype(np.float32)
            coverage *= envelope
            ridge *= envelope
            crown_gate = np.power(
                np.clip(strand_weights[..., strand], 0.0, 1.0),
                0.34,
            )
            bridge = float(element.get("valley_bridge", 0.012))
            surface_gate = bridge + (1.0 - bridge) * crown_gate
            coverage *= surface_gate
            ridge *= surface_gate
            quiet_eligibility = float(element.get("quiet_eligibility", 0.36))
            quiet_gate = 1.0 - quiet * (1.0 - quiet_eligibility) * 0.88
            coverage *= quiet_gate
            ridge *= quiet_gate
            mask = np.clip(coverage, 0.0, 1.0).astype(np.float32)
            ridge = np.clip(ridge, 0.0, 1.0).astype(np.float32)

            if family_name == "long_ribbons":
                long_mask = 1.0 - (1.0 - long_mask) * (1.0 - mask)
            else:
                short_mask = 1.0 - (1.0 - short_mask) * (1.0 - mask)
                if bool(element["geometry_spawn"]):
                    geometry_spawn = np.maximum(
                        geometry_spawn,
                        mask * np.power(envelope, 0.38),
                    )

            element_height = ridge * float(element["height"])
            height_shape = np.sqrt(
                height_shape * height_shape
                + element_height * element_height
            )
            if family_name == "long_ribbons":
                long_height_shape = np.sqrt(
                    long_height_shape * long_height_shape
                    + element_height * element_height
                )
            else:
                short_height_shape = np.sqrt(
                    short_height_shape * short_height_shape
                    + element_height * element_height
                )
            if bool(element.get("normal_only", False)):
                normal_only_mask = np.maximum(normal_only_mask, mask)
            else:
                colour = palette[int(element["palette_index"])]
                colour_sum += mask[..., np.newaxis] * colour
                colour_weight += mask
            sheen = ridge * float(element["sheen"])
            sheen_mask = np.maximum(sheen_mask, sheen)
            lifted_mask = np.maximum(
                lifted_mask,
                ridge
                * np.clip(
                    (float(element["height"]) - 0.48) / 0.52,
                    0.0,
                    1.0,
                ),
            )
            roughness_sum += mask * float(element["roughness_bias"])
            roughness_weight += mask

    safe_colour_weight = np.maximum(colour_weight, 1.0e-6)
    safe_roughness_weight = np.maximum(roughness_weight, 1.0e-6)
    combined_mask = 1.0 - (1.0 - long_mask) * (1.0 - short_mask)
    return {
        "elements": elements,
        "long_mask": np.clip(long_mask, 0.0, 1.0),
        "short_mask": np.clip(short_mask, 0.0, 1.0),
        "combined_mask": np.clip(combined_mask, 0.0, 1.0),
        "geometry_spawn": np.clip(geometry_spawn, 0.0, 1.0),
        "height_shape": np.clip(height_shape, 0.0, 1.7),
        "long_height_shape": np.clip(long_height_shape, 0.0, 1.5),
        "short_height_shape": np.clip(short_height_shape, 0.0, 1.5),
        "normal_only_mask": np.clip(normal_only_mask, 0.0, 1.0),
        "sheen_mask": np.clip(sheen_mask, 0.0, 1.0),
        "lifted_mask": np.clip(lifted_mask, 0.0, 1.0),
        "colour_srgb": np.clip(
            colour_sum / safe_colour_weight[..., np.newaxis],
            0.0,
            1.0,
        ),
        "colour_weight": np.clip(colour_weight, 0.0, 1.0),
        "roughness_bias": np.clip(
            roughness_sum / safe_roughness_weight,
            -0.07,
            0.07,
        ),
    }


def _rasterize_variation(
    recipe: dict[str, Any],
    bundle_recipe: dict[str, Any],
    fibre_recipe: dict[str, Any],
    profile: dict[str, Any],
    preset: dict[str, Any],
    variation: dict[str, Any],
    *,
    resolution_x: int,
    resolution_y: int,
) -> dict[str, Any]:
    diameter = float(preset["diameter_m"])
    lay_length = float(preset["lay_length_m"])
    circumference = math.pi * diameter
    u = (np.arange(resolution_x, dtype=np.float32) + 0.5) / resolution_x
    v = (np.arange(resolution_y, dtype=np.float32) + 0.5) / resolution_y
    u, v = np.meshgrid(u, v)

    seed = int(variation["seed"])
    bounds = recipe["variation_bounds"]
    macro_warp = _periodic_harmonics(
        u,
        v,
        seed=seed + 11,
        amplitude=float(bounds["macro_warp_turns_max"]),
        terms=((1, 1, 1.0), (2, -1, 0.55), (3, 2, 0.28)),
    )
    local_pulse = (
        0.5
        + 0.5
        * np.sin(
            math.tau
            * (
                2.0 * u
                - v
                + float(variation["colour_phase"])
            )
        )
    )
    macro_warp *= 0.68 + 0.32 * local_pulse

    strand_coordinate = (
        v
        - u
        + float(variation["lay_phase_turns"])
        + macro_warp / 3.0
    ) % 1.0
    along_strand = _along_strand_coordinate(u, v)
    strand = _rasterize_strand_crowns(
        bundle_recipe,
        strand_coordinate=strand_coordinate,
        along_strand=along_strand,
        width_multiplier=float(variation["strand_width_multiplier"]),
    )
    strand_lobes_mask = strand["strand_lobes"]
    strand_weights = strand["strand_identity"]
    strand_crowns = strand["strand_crowns"]
    strand_colour_weights = strand["strand_colour_weights"]

    bundle = _rasterize_bundle_tracks(
        bundle_recipe,
        profile,
        preset_id=str(preset["id"]),
        variation_index=int(variation["index"]),
        u=u,
        v=v,
        strand_coordinate=strand_coordinate,
        strand_weights=strand_crowns,
    )
    yarn_groups_mask = bundle["identity"].astype(np.float32)
    yarn_height_shape = bundle["height_shape"].astype(np.float32)

    quiet_coordinate = (
        u
        + v
        + 0.075 * np.sin(math.tau * (2.0 * u - v))
        + 0.031 * np.sin(math.tau * (3.0 * u + 2.0 * v))
    ) % 1.0
    quiet = _periodic_window(
        quiet_coordinate,
        start=float(variation["quiet_start"]),
        length=float(variation["quiet_fraction"]),
        edge=0.045,
    )
    quiet *= 0.84 + 0.16 * (
        0.5
        + 0.5
        * np.cos(
            math.tau
            * (
                v
                + 0.13 * np.sin(math.tau * u)
                + variation["colour_phase"]
            )
        )
    )
    quiet = np.clip(quiet, 0.0, 1.0).astype(np.float32)
    fibre = _rasterize_fibre_tracks(
        fibre_recipe,
        profile,
        preset_id=str(preset["id"]),
        variation_index=int(variation["index"]),
        u=u,
        v=v,
        strand_coordinate=strand_coordinate,
        strand_weights=strand_crowns,
        bundle_tracks=bundle["tracks"],
        quiet=quiet,
    )
    fibre_ribbons_mask = (
        fibre["combined_mask"] * float(variation["fibre_visibility"])
    ).astype(np.float32)

    amplitudes = profile["surface_response"][
        "height_layers_diameter_fraction"
    ]
    strand_relief_shape = (
        strand_lobes_mask
        - 0.21 * strand["cavity_large"]
        - 0.11 * strand["cavity_core"]
    )
    strand_height = (
        (strand_relief_shape - float(strand_relief_shape.mean()))
        * diameter
        * float(amplitudes["rope_lobes"])
    ).astype(np.float32)
    yarn_height = (
        (yarn_height_shape - float(yarn_height_shape.mean()))
        * diameter
        * float(amplitudes["yarn_groups"])
        * (1.0 - quiet * 0.22)
    ).astype(np.float32)
    long_fibre_height = (
        (
            fibre["long_height_shape"]
            - float(fibre["long_height_shape"].mean())
        )
        * diameter
        * float(amplitudes["fibre_ribbons"])
        * 0.84
    ).astype(np.float32)
    short_fibre_height = (
        (
            fibre["short_height_shape"]
            - float(fibre["short_height_shape"].mean())
        )
        * diameter
        * float(amplitudes["fibre_ribbons"])
        * 0.64
    ).astype(np.float32)
    fibre_height = (long_fibre_height + short_fibre_height).astype(np.float32)
    height_m = (
        strand_height + yarn_height + long_fibre_height + short_fibre_height
    ).astype(np.float32)

    normal_strand = _height_to_normal_anisotropic(
        strand_height,
        meters_per_pixel_x=lay_length / resolution_x,
        meters_per_pixel_y=circumference / resolution_y,
        strength=0.64,
    )
    normal_yarn = _height_to_normal_anisotropic(
        yarn_height,
        meters_per_pixel_x=lay_length / resolution_x,
        meters_per_pixel_y=circumference / resolution_y,
        strength=1.08,
    )
    normal_long_fibre = _height_to_normal_anisotropic(
        long_fibre_height,
        meters_per_pixel_x=lay_length / resolution_x,
        meters_per_pixel_y=circumference / resolution_y,
        strength=0.76,
    )
    normal_short_fibre = _height_to_normal_anisotropic(
        short_fibre_height,
        meters_per_pixel_x=lay_length / resolution_x,
        meters_per_pixel_y=circumference / resolution_y,
        strength=0.94,
    )
    normal = (
        normal_strand
        + normal_yarn
        + normal_long_fibre
        + normal_short_fibre
    )
    normal[..., 2] -= 3.0
    normal /= np.maximum(
        np.linalg.norm(normal, axis=-1, keepdims=True),
        1.0e-8,
    )

    palette = np.stack(
        [_hex_to_srgb(value) for value in profile["palette"]["dry_warm_srgb"]]
    )
    base = np.broadcast_to(palette[6], (*u.shape, 3)).copy()
    strand_colours = np.stack(
        [
            palette[int(strand_profile["colour_palette_index"])]
            for strand_profile in bundle_recipe["strand_profiles"]
        ]
    )
    weighted_strand_colour = np.sum(
        strand_colour_weights[..., np.newaxis] * strand_colours,
        axis=2,
    )
    base = base * 0.78 + weighted_strand_colour * 0.22

    for passage_index, passage in enumerate(
        recipe["authored_colour_passages"]
    ):
        passage_coordinate = (
            u
            + v
            + 0.06
            * np.sin(
                math.tau
                * (
                    (passage_index % 3 + 1) * u
                    - (passage_index % 2 + 1) * v
                )
            )
        ) % 1.0
        passage_mask = _periodic_window(
            passage_coordinate,
            start=(
                float(passage["start_u"])
                + float(variation["colour_phase"]) * 0.071
                + passage_index * variation["index"] * 0.013
            )
            % 1.0,
            length=float(passage["length_u"]),
            edge=min(0.035, float(passage["length_u"]) * 0.24),
        )
        cross_fade = 0.66 + 0.34 * (
            0.5
            + 0.5
            * np.cos(
                math.tau
                * (
                    v * (passage_index % 3 + 1)
                    + passage_index * 0.17
                )
            )
        )
        selected_strand = strand_colour_weights[..., passage_index % 3]
        passage_mask *= (
            cross_fade
            * float(passage["strength"])
            * (0.18 + 0.82 * selected_strand)
        )
        base = mix(
            base,
            np.broadcast_to(
                palette[int(passage["palette_index"])],
                base.shape,
            ),
            passage_mask,
        )

    base = mix(
        base,
        bundle["colour_srgb"],
        np.clip(
            bundle["colour_weight"] * 0.62 * (1.0 - quiet * 0.28),
            0.0,
            0.82,
        ),
    )
    bundle_groove_material = (
        (1.0 - yarn_groups_mask)
        * smoothstep(0.18, 0.91, strand_lobes_mask)
        * (1.0 - quiet * 0.24)
    )
    base = mix(
        base,
        np.broadcast_to(palette[3], base.shape),
        bundle_groove_material * 0.075,
    )
    base = mix(
        base,
        np.broadcast_to(palette[2], base.shape),
        bundle["edge_mask"] * 0.13 * (1.0 - quiet * 0.18),
    )
    base = mix(
        base,
        np.broadcast_to(palette[11], base.shape),
        bundle["subridge_light_mask"] * 0.19 * (1.0 - quiet * 0.31),
    )
    base = mix(
        base,
        np.broadcast_to(palette[4], base.shape),
        bundle["subridge_dark_mask"] * 0.12 * (1.0 - quiet * 0.24),
    )
    base = mix(
        base,
        fibre["colour_srgb"],
        np.clip(
            fibre["colour_weight"] * 0.82,
            0.0,
            0.90,
        ),
    )
    cavity_material = (
        0.10 * strand["cavity_large"]
        + 0.075 * strand["cavity_core"]
    )
    cavity_material *= 1.0 - 0.72 * fibre["combined_mask"]
    base = mix(
        base,
        np.broadcast_to(palette[2], base.shape),
        np.clip(cavity_material, 0.0, 0.19),
    )

    line_coordinate = (
        u
        + v
        + 0.09 * np.sin(math.tau * (2.0 * u - v + 0.17))
        + 0.045 * np.sin(math.tau * (3.0 * u + 2.0 * v + 0.41))
    ) % 1.0
    line_gate = np.zeros_like(u, dtype=np.float32)
    for window_index, window in enumerate(recipe["linework_windows"]):
        shifted_start = (
            float(window["start_u"])
            + variation["index"] * (0.019 + window_index * 0.007)
        ) % 1.0
        line_gate = np.maximum(
            line_gate,
            _periodic_window(
                line_coordinate,
                start=shifted_start,
                length=float(window["length_u"]),
                edge=0.026,
            )
            * float(window["strength"]),
        )
    separator_coordinate = (
        strand_coordinate + 1.0 / 6.0
    ) % 1.0
    separator_index = np.floor(separator_coordinate * 3.0).astype(np.int32)
    separator_weights = np.asarray(
        [
            [1.0, 0.42, 0.68],
            [0.52, 1.0, 0.34],
            [0.72, 0.38, 1.0],
            [0.44, 0.76, 1.0],
        ][int(variation["index"])],
        dtype=np.float32,
    )
    irregular_break = smoothstep(
        0.31,
        0.73,
        0.5
        + 0.31 * np.sin(math.tau * (5.0 * u - 3.0 * v + 0.17))
        + 0.19 * np.sin(math.tau * (2.0 * u + 7.0 * v + 0.61)),
    )
    separator_ink = (
        (1.0 - smoothstep(0.08, 0.28, strand_lobes_mask))
        * line_gate
        * irregular_break
        * separator_weights[separator_index]
        * (1.0 - quiet * 0.82)
        * (1.0 - fibre["combined_mask"] * 0.72)
    ).astype(np.float32)
    crown_lift = smoothstep(0.74, 0.96, strand_lobes_mask) * (
        1.0 - quiet * 0.16
    )
    base = np.clip(base, 0.0, 1.0).astype(np.float32)
    base_linear = srgb_to_linear(base)

    absorbency = (
        0.5
        + 0.5
        * np.sin(
            math.tau
            * (
                2.0 * u
                + v
                + variation["colour_phase"] * 0.7
            )
        )
    )
    roughness = (
        0.735
        + 0.044 * absorbency
        + bundle["roughness_bias"]
        + fibre["roughness_bias"]
        + 0.051 * fibre["lifted_mask"]
        - 0.057 * fibre["sheen_mask"]
        + 0.038 * strand["cavity_large"]
        + 0.026 * strand["cavity_core"]
        - 0.036 * crown_lift
        + 0.021 * np.cos(math.tau * (u - 2.0 * v + 0.13))
    )
    roughness = np.clip(roughness, 0.66, 0.86).astype(np.float32)

    valley = strand["cavity_large"]
    yarn_valley = (1.0 - yarn_groups_mask) * smoothstep(
        0.15, 0.88, strand_lobes_mask
    )
    ao = np.clip(
        1.0
        - 0.22 * valley
        - 0.11 * strand["cavity_core"]
        - 0.07 * yarn_valley,
        0.62,
        1.0,
    ).astype(np.float32)

    return {
        "base_color_srgb": base,
        "base_color_linear": base_linear,
        "normal": normal.astype(np.float32),
        "roughness": roughness,
        "ao": ao,
        "height_m": height_m,
        "identity_masks": np.stack(
            [
                strand_lobes_mask,
                yarn_groups_mask,
                fibre_ribbons_mask,
            ],
            axis=-1,
        ).astype(np.float32),
        "strand_identity_masks": strand_weights.astype(np.float32),
        "fibre_detail_masks": np.stack(
            [
                fibre["long_mask"],
                fibre["short_mask"],
                fibre["geometry_spawn"],
            ],
            axis=-1,
        ).astype(np.float32),
        "response_masks": np.stack(
            [
                strand["cavity_large"],
                strand["cavity_core"],
                fibre["sheen_mask"],
            ],
            axis=-1,
        ).astype(np.float32),
        "bundle_event_masks": np.stack(
            [
                bundle["event_mask"],
                bundle["merge_split_mask"],
                bundle["width_class_masks"]["broad"],
            ],
            axis=-1,
        ).astype(np.float32),
        "stylization": np.stack(
            [separator_ink, crown_lift, quiet],
            axis=-1,
        ).astype(np.float32),
        "height_layers": {
            "strand_lobes": strand_height,
            "yarn_groups": yarn_height,
            "long_fibre_ribbons": long_fibre_height,
            "short_fibres": short_fibre_height,
        },
    }


ATLAS_LANES = (
    "base_color_srgb",
    "base_color_linear",
    "normal",
    "roughness",
    "ao",
    "height_m",
    "identity_masks",
    "strand_identity_masks",
    "fibre_detail_masks",
    "response_masks",
    "bundle_event_masks",
    "stylization",
)
HEIGHT_LAYER_NAMES = (
    "strand_lobes",
    "yarn_groups",
    "long_fibre_ribbons",
    "short_fibres",
)


def _generate_preset_material(
    recipe: dict[str, Any],
    bundle_recipe: dict[str, Any],
    fibre_recipe: dict[str, Any],
    profile: dict[str, Any],
    preset: dict[str, Any],
    *,
    resolution_x: int,
    resolution_y: int,
) -> dict[str, Any]:
    """Fill one atlas incrementally so four full variations are not retained."""
    material: dict[str, Any] = {}
    for variation_index in range(recipe["variation_count"]):
        variation = _rasterize_variation(
            recipe,
            bundle_recipe,
            fibre_recipe,
            profile,
            preset,
            _compile_variation(recipe, variation_index),
            resolution_x=resolution_x,
            resolution_y=resolution_y,
        )
        if variation_index == 0:
            atlas_height = resolution_y * int(recipe["variation_count"])
            for lane in ATLAS_LANES:
                source = variation[lane]
                shape = (atlas_height, *source.shape[1:])
                material[lane] = np.empty(shape, dtype=np.float32)
            material["height_layers"] = {
                layer: np.empty(
                    (atlas_height, resolution_x),
                    dtype=np.float32,
                )
                for layer in HEIGHT_LAYER_NAMES
            }
        target = slice(
            variation_index * resolution_y,
            (variation_index + 1) * resolution_y,
        )
        for lane in ATLAS_LANES:
            material[lane][target] = variation[lane]
        for layer in HEIGHT_LAYER_NAMES:
            material["height_layers"][layer][target] = (
                variation["height_layers"][layer]
            )
    material["preset"] = dict(preset)
    return material


def generate_material(
    recipe: dict[str, Any],
    *,
    resolution_x: int,
    resolution_y: int,
) -> dict[str, dict[str, Any]]:
    if resolution_x < 128 or resolution_y < 64:
        raise ValueError("rope material resolution is too small")
    profile = json.loads(PROFILE_PATH.read_text())
    bundle_recipe = load_bundle_recipe(DEFAULT_BUNDLE_PATTERN_PATH)
    fibre_recipe = load_fibre_recipe(DEFAULT_FIBRE_PATTERN_PATH)
    return {
        str(preset["id"]): _generate_preset_material(
            recipe,
            bundle_recipe,
            fibre_recipe,
            profile,
            preset,
            resolution_x=resolution_x,
            resolution_y=resolution_y,
        )
        for preset in recipe["presets"]
    }


def _encode_height(height_m: np.ndarray) -> tuple[np.ndarray, float, float]:
    minimum = float(height_m.min())
    maximum = float(height_m.max())
    if maximum <= minimum:
        raise ValueError("height field has no range")
    return (
        ((height_m - minimum) / (maximum - minimum)).astype(np.float32),
        minimum,
        maximum,
    )


def _proof_sheet(
    material: dict[str, Any],
    *,
    resolution_y: int,
) -> np.ndarray:
    first = slice(0, resolution_y)
    base = material["base_color_srgb"][first]
    base_linear = material["base_color_linear"][first]
    normal = material["normal"][first]
    roughness = material["roughness"][first]
    ao = material["ao"][first]
    masks = material["identity_masks"][first]
    strand_identity = material["strand_identity_masks"][first]
    fibre_detail = material["fibre_detail_masks"][first]
    response_masks = material["response_masks"][first]
    stylization = material["stylization"][first]
    height = material["height_m"][first]
    combined = render_material_preview(base_linear, normal, roughness, ao)
    grazing = render_material_single_light(
        base_linear,
        normal,
        roughness,
        ao,
        direction=(-0.92, -0.18, 0.34),
        intensity=2.25,
    )
    stylized = mix(
        combined,
        np.broadcast_to(
            np.asarray((0.21, 0.15, 0.11), dtype=np.float32),
            combined.shape,
        ),
        stylization[..., 0] * 0.34,
    )
    return compose_material_chapter(
        f"{material['preset']['id'].replace('_', ' ')} rope",
        (
            f"{material['preset']['diameter_m'] * 1000:.0f} MM DIAMETER / "
            f"{material['preset']['lay_length_m'] * 1000:.0f} MM LAY / "
            "THREE-STRAND REGULAR LAY"
        ),
        [
            ("base colour", base),
            ("stylized response", stylized),
            ("grazing response", grazing),
            ("three strand identities", strand_identity),
            ("counter-twist yarn", gray_rgb(masks[..., 1])),
            ("long / short / flyaway", fibre_detail),
            ("large cavity / core / sheen", response_masks),
            (
                "height signed",
                scalar_tint(
                    normalized_range(height),
                    (0.12, 0.16, 0.20),
                    (0.93, 0.76, 0.33),
                ),
            ),
        ],
        [
            "Each primary strand has its own crown width, flattening, contact rolloff, packing bulge, colour tendency, and longitudinal wander.",
            "Coarse bundles use authored tracks with unequal widths, merge, split, burial, fade, and flatten events; they are not a repeated frequency.",
            "Long fibre blades and short staple fragments have finite tapered lengths, bundle ownership, persistent colours, separate height, and selective sheen.",
            "No dirt, damage, wetness, photographic noise, or baked light is present. Blue in the fibre panel is only an optional geometry-spawn mask.",
        ],
    )


def _sample_periodic_bilinear(
    image: np.ndarray,
    u: np.ndarray,
    v: np.ndarray,
) -> np.ndarray:
    height, width = image.shape[:2]
    x = (u % 1.0) * width - 0.5
    y = (v % 1.0) * height - 0.5
    x0 = np.floor(x).astype(np.int32)
    y0 = np.floor(y).astype(np.int32)
    x1 = (x0 + 1) % width
    y1 = (y0 + 1) % height
    x0 %= width
    y0 %= height
    fx = (x - np.floor(x)).astype(np.float32)
    fy = (y - np.floor(y)).astype(np.float32)
    if image.ndim == 3:
        fx = fx[..., np.newaxis]
        fy = fy[..., np.newaxis]
    return (
        image[y0, x0] * (1.0 - fx) * (1.0 - fy)
        + image[y0, x1] * fx * (1.0 - fy)
        + image[y1, x0] * (1.0 - fx) * fy
        + image[y1, x1] * fx * fy
    ).astype(np.float32)


def _cylinder_proof(
    material: dict[str, Any],
    *,
    resolution_y: int,
) -> np.ndarray:
    """Evaluate the first atlas variation on a neutral analytic cylinder."""
    tile = slice(0, resolution_y)
    base = material["base_color_linear"][tile]
    tangent_normal = material["normal"][tile]
    roughness = material["roughness"][tile]
    ao = material["ao"][tile]
    ink = material["stylization"][tile, ..., 0]
    macro_lobes = material["identity_masks"][tile, ..., 0]

    canvas_width = 1152
    canvas_height = 512
    x = (
        np.arange(canvas_width, dtype=np.float32) + 0.5
    ) / canvas_width
    y = (
        np.arange(canvas_height, dtype=np.float32) + 0.5
    ) / canvas_height
    x, y = np.meshgrid(x, y)
    cylinder_left = 0.055
    cylinder_right = 0.945
    centre_y = 0.49
    radius = 0.295
    axial = (x - cylinder_left) / (cylinder_right - cylinder_left)
    texture_u = axial * 2.35 + 0.07
    top_lobe = _sample_periodic_bilinear(
        macro_lobes,
        texture_u,
        np.full_like(texture_u, 0.5),
    )
    bottom_lobe = _sample_periodic_bilinear(
        macro_lobes,
        texture_u,
        np.zeros_like(texture_u),
    )
    silhouette_lobe = np.where(y <= centre_y, top_lobe, bottom_lobe)
    silhouette_radius = radius * (0.92 + 0.08 * silhouette_lobe)
    radial_y = (centre_y - y) / silhouette_radius
    inside = (
        (axial >= 0.0)
        & (axial <= 1.0)
        & (np.abs(radial_y) <= 1.0)
    )
    radial_clamped = np.clip(radial_y, -1.0, 1.0)
    theta = np.arcsin(radial_clamped)
    texture_v = theta / math.tau + 0.25

    sampled_base = _sample_periodic_bilinear(base, texture_u, texture_v)
    sampled_normal = _sample_periodic_bilinear(
        tangent_normal,
        texture_u,
        texture_v,
    )
    sampled_normal /= np.maximum(
        np.linalg.norm(sampled_normal, axis=-1, keepdims=True),
        1.0e-7,
    )
    sampled_roughness = _sample_periodic_bilinear(
        roughness,
        texture_u,
        texture_v,
    )
    sampled_ao = _sample_periodic_bilinear(ao, texture_u, texture_v)
    sampled_ink = _sample_periodic_bilinear(ink, texture_u, texture_v)

    radial_z = np.sqrt(np.maximum(1.0 - radial_clamped * radial_clamped, 0.0))
    tangent = np.zeros((*x.shape, 3), dtype=np.float32)
    tangent[..., 0] = 1.0
    bitangent = np.stack(
        [
            np.zeros_like(x),
            radial_z,
            radial_clamped,
        ],
        axis=-1,
    )
    radial_normal = np.stack(
        [
            np.zeros_like(x),
            radial_clamped,
            radial_z,
        ],
        axis=-1,
    )
    world_normal = (
        sampled_normal[..., 0:1] * tangent
        + sampled_normal[..., 1:2] * bitangent
        + sampled_normal[..., 2:3] * radial_normal
    )
    world_normal /= np.maximum(
        np.linalg.norm(world_normal, axis=-1, keepdims=True),
        1.0e-7,
    )

    view = np.asarray((0.0, 0.0, 1.0), dtype=np.float32)
    result = sampled_base * (
        0.04 + 0.11 * sampled_ao[..., np.newaxis]
    )
    lights = (
        (
            np.asarray((-0.44, 0.48, 0.76), dtype=np.float32),
            np.asarray((1.0, 0.92, 0.80), dtype=np.float32),
            0.82,
        ),
        (
            np.asarray((0.62, -0.31, 0.72), dtype=np.float32),
            np.asarray((0.60, 0.72, 1.0), dtype=np.float32),
            0.24,
        ),
    )
    for direction, colour, intensity in lights:
        direction /= np.linalg.norm(direction)
        halfway = direction + view
        halfway /= np.linalg.norm(halfway)
        normal_light = np.clip(
            np.sum(world_normal * direction, axis=-1),
            0.0,
            1.0,
        )
        normal_half = np.clip(
            np.sum(world_normal * halfway, axis=-1),
            0.0,
            1.0,
        )
        diffuse = sampled_base * normal_light[..., np.newaxis] * intensity
        specular_power = 7.0 + (1.0 - sampled_roughness) * 42.0
        specular = (
            np.power(normal_half, specular_power)
            * np.square(1.0 - sampled_roughness)
            * intensity
            * 0.52
        )
        result += (
            diffuse + specular[..., np.newaxis]
        ) * colour * sampled_ao[..., np.newaxis]
    result = 1.0 - np.exp(-np.maximum(result, 0.0) * 1.05)
    result = linear_to_srgb(result)
    result = mix(
        result,
        np.broadcast_to(
            np.asarray((0.21, 0.15, 0.11), dtype=np.float32),
            result.shape,
        ),
        sampled_ink * 0.32,
    )

    background = np.zeros_like(result)
    background[...] = np.asarray((0.027, 0.031, 0.035), dtype=np.float32)
    floor_glow = np.exp(
        -np.square((y - (centre_y + radius + 0.035)) / 0.075)
    ) * np.exp(-np.square((x - 0.5) / 0.43))
    background += floor_glow[..., np.newaxis] * np.asarray(
        (0.025, 0.022, 0.018),
        dtype=np.float32,
    )
    edge_softness = np.clip(
        (1.0 - np.abs(radial_y)) * canvas_height * 0.18,
        0.0,
        1.0,
    )
    edge_softness *= inside
    return mix(
        background,
        result,
        edge_softness,
    ).astype(np.float32)


def generate(
    output_root: Path,
    *,
    resolution_x: int = 1536,
    resolution_y: int = 384,
    pattern_path: Path = DEFAULT_PATTERN_PATH,
) -> dict[str, Any]:
    recipe = load_pattern_recipe(pattern_path)
    profile = json.loads(PROFILE_PATH.read_text())
    bundle_recipe = load_bundle_recipe(DEFAULT_BUNDLE_PATTERN_PATH)
    fibre_recipe = load_fibre_recipe(DEFAULT_FIBRE_PATTERN_PATH)
    output_root.mkdir(parents=True, exist_ok=True)

    manifest: dict[str, Any] = {
        "schema": "iggy-rope-plant-fibre-build/1.0",
        "profile_id": profile["profile_id"],
        "pattern_id": recipe["pattern_id"],
        "bundle_pattern_id": bundle_recipe["pattern_id"],
        "fibre_pattern_id": fibre_recipe["pattern_id"],
        "variation_count": recipe["variation_count"],
        "atlas_layout": {
            "columns": 1,
            "rows": recipe["variation_count"],
            "tile_resolution": [resolution_x, resolution_y],
        },
        "coordinate_contract": profile["coordinate_contract"],
        "constraints": {
            "uses_ai_generated_imagery": False,
            "generates_geometry": False,
            "uses_damage": False,
            "uses_baked_directional_light": False,
        },
        "presets": {},
    }

    for preset in recipe["presets"]:
        preset_id = str(preset["id"])
        material = _generate_preset_material(
            recipe,
            bundle_recipe,
            fibre_recipe,
            profile,
            preset,
            resolution_x=resolution_x,
            resolution_y=resolution_y,
        )
        prefix = f"rope_plant_fibre_v1_{preset_id}"
        base_path = output_root / f"{prefix}_basecolor.png"
        normal_path = output_root / f"{prefix}_normal.png"
        height_path = output_root / f"{prefix}_height.png"
        orm_path = output_root / f"{prefix}_orm.png"
        identity_path = output_root / f"{prefix}_identity_masks.png"
        strand_identity_path = output_root / f"{prefix}_strand_identities.png"
        fibre_detail_path = output_root / f"{prefix}_fibre_detail_masks.png"
        response_path = output_root / f"{prefix}_response_masks.png"
        bundle_event_path = output_root / f"{prefix}_bundle_event_masks.png"
        stylization_path = output_root / f"{prefix}_stylization.png"
        proof_path = output_root / f"{prefix}_proof.png"
        cylinder_proof_path = output_root / f"{prefix}_cylinder_proof.png"

        encoded_height, height_minimum, height_maximum = _encode_height(
            material["height_m"]
        )
        orm = np.stack(
            [
                material["ao"],
                material["roughness"],
                np.zeros_like(material["roughness"]),
            ],
            axis=-1,
        )
        write_png_rgb8(base_path, material["base_color_srgb"])
        write_png_rgb8(normal_path, material["normal"] * 0.5 + 0.5)
        write_png_gray16(height_path, encoded_height)
        write_png_rgb8(orm_path, orm)
        write_png_rgb8(identity_path, material["identity_masks"])
        write_png_rgb8(
            strand_identity_path,
            material["strand_identity_masks"],
        )
        write_png_rgb8(
            fibre_detail_path,
            material["fibre_detail_masks"],
        )
        write_png_rgb8(response_path, material["response_masks"])
        write_png_rgb8(bundle_event_path, material["bundle_event_masks"])
        write_png_rgb8(stylization_path, material["stylization"])
        write_png_rgb8(
            proof_path,
            _proof_sheet(material, resolution_y=resolution_y),
        )
        write_png_rgb8(
            cylinder_proof_path,
            _cylinder_proof(material, resolution_y=resolution_y),
        )

        outputs = {
            "base_color": _output_record(base_path),
            "normal": _output_record(normal_path),
            "height": _output_record(height_path),
            "orm": _output_record(orm_path),
            "identity_masks": _output_record(identity_path),
            "strand_identities": _output_record(strand_identity_path),
            "fibre_detail_masks": _output_record(fibre_detail_path),
            "response_masks": _output_record(response_path),
            "bundle_event_masks": _output_record(bundle_event_path),
            "stylization": _output_record(stylization_path),
            "proof": _output_record(proof_path),
            "cylinder_proof": _output_record(cylinder_proof_path),
        }
        manifest["presets"][preset_id] = {
            "diameter_m": material["preset"]["diameter_m"],
            "lay_length_m": material["preset"]["lay_length_m"],
            "circumference_m": math.pi * material["preset"]["diameter_m"],
            "height_encoding_m": {
                "minimum": height_minimum,
                "maximum": height_maximum,
                "neutral": (
                    -height_minimum / (height_maximum - height_minimum)
                ),
            },
            "outputs": outputs,
        }
        del material
        del encoded_height
        del orm

    manifest_path = output_root / "rope_plant_fibre_v1_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def _output_record(path: Path) -> dict[str, str]:
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    return {
        "path": str(path.resolve()),
        "sha256": digest,
    }


def main() -> int:
    args = parse_args()
    manifest = generate(
        args.output_root,
        resolution_x=args.resolution_x,
        resolution_y=args.resolution_y,
        pattern_path=args.pattern,
    )
    print(
        json.dumps(
            {
                "profile_id": manifest["profile_id"],
                "presets": list(manifest["presets"]),
                "output_root": str(args.output_root.resolve()),
            },
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
