#!/usr/bin/env python3
"""Generate authored cathedral ashlar with local planes and mason tooling."""

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


MATERIAL_SCHEMA = "iggy3d.material.cathedral_stone_v1.v6"
PATTERN_SCHEMA = "iggy3d.pattern.measured_cathedral_ashlar.v2"
CAPTURE_SCHEMA = "iggy3d.material_reference_capture.cathedral_stone.v2"
DEFAULT_PATTERN = (
    SCRIPT_ROOT / "patterns" / "cathedral_ashlar_courses_v1.json"
)
DEFAULT_PROFILE = (
    SCRIPT_ROOT / "profiles" / "cathedral_stone_v1.json"
)
DEFAULT_CAPTURE = (
    SCRIPT_ROOT / "references" / "santa_marina_biocalcarenite_capture.json"
)
DEFAULT_LEDGER = MATERIALS_ROOT / "reference_measurements_v1.json"
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"
BODY_DETAIL_SPAN_M = 0.064
BODY_DETAIL_DECORRELATION_SPAN_M = 0.091
BODY_RELIEF_PROXY_ID = "sabucina_calcarenite_surface_topography_proxy"
TOOLING_DETAIL_SPAN_M = 0.256
TOOLING_FINISH_ID = "medieval_fine_oblique_layage"
TOOLING_DENSITY_VARIANTS_PER_M2 = (720.0, 1369.0, 2420.0)

TOOLING_IDS = {
    "diagonal_drove": 1,
}
EDGE_IDS = {
    "square_true": 1,
}


@dataclass(frozen=True)
class AshlarRecipe:
    schema: str
    name: str
    tile_size_m: float
    mortar: dict[str, Any]
    families: tuple[str, ...]
    tooling: tuple[str, ...]
    edge_profiles: tuple[str, ...]
    courses: tuple[dict[str, Any], ...]
    measurement_authority: dict[str, Any]
    bond: dict[str, Any]
    excluded_from_intact_core: tuple[str, ...]


def _palette(entry: dict[str, Any], *, label: str) -> np.ndarray:
    result = np.asarray(entry.get("palette_20_rgb"), dtype=np.float32)
    if result.shape != (20, 3):
        raise ValueError(f"{label} must provide twenty RGB shades")
    if np.any(result < 0.0) or np.any(result > 255.0):
        raise ValueError(f"{label} contains an invalid RGB channel")
    luminance = np.sum(
        result
        * np.asarray([0.2126, 0.7152, 0.0722], dtype=np.float32),
        axis=-1,
    )
    if np.any(np.diff(luminance) < -0.75):
        raise ValueError(f"{label} must be ordered from dark to light")
    return result / 255.0


def load_reference_capture(
    path: str | Path = DEFAULT_CAPTURE,
) -> dict[str, Any]:
    payload = json.loads(Path(path).read_text())
    if payload.get("schema") != CAPTURE_SCHEMA:
        raise ValueError("Unexpected cathedral-stone capture schema")
    source = payload.get("source", {})
    if (
        source.get("asset_id")
        != "Santa_Marina_Aguas_Santas_2024_facade"
        or source.get("license") != "CC BY-SA 4.0"
        or source.get("attribution")
        != "Benjamin Smith / Wikimedia Commons"
    ):
        raise ValueError(
            "Cathedral stone capture must retain Santa Marina provenance and attribution"
        )
    translation = payload.get("production_translation", {})
    if translation.get("raw_pixels_used_as_runtime_texture") is not False:
        raise ValueError("Raw reference pixels cannot enter runtime maps")
    if translation.get("source_pixels_retained_in_repository") is not False:
        raise ValueError("The licensed facade pixels may not be retained in this package")
    body = payload["stone_body"]
    if body.get("measurement_id") != "santa_marina_naranjo_biocalcarenite_body":
        raise ValueError("Stone body must name the Santa Marina/Naranjo measurement")
    if not math.isclose(
        sum(body["nondegraded_naranjo_mineral_fractions"].values()),
        1.0,
        abs_tol=1.0e-9,
    ):
        raise ValueError("Naranjo mineral fractions must close to one")
    color = payload["photographic_color"]
    if color.get("measurement_limit") != (
        "relative sunlit color families only; not absolute albedo, "
        "roughness, height, or normal"
    ):
        raise ValueError("Facade photograph may only constrain relative color")
    families = color["families"]
    expected = {"cool_buff", "mid_buff", "warm_ochre", "pale_cream"}
    if set(families) != expected:
        raise ValueError("Reference capture must define four sunlit color families")
    for family, entry in families.items():
        _palette(entry, label=family)
    _palette(color["all_sunlit_stone"], label="all sunlit stone")
    return payload


def load_relief_proxy(
    path: str | Path = DEFAULT_LEDGER,
) -> dict[str, Any]:
    payload = json.loads(Path(path).read_text())
    measurements = {
        entry["id"]: entry
        for entry in payload["measurements"]["stone"]
    }
    proxy = measurements.get(BODY_RELIEF_PROXY_ID)
    if proxy is None:
        raise ValueError("Measured calcarenite relief proxy is missing")
    if (
        proxy.get("source") != "S24_sabucina_surface_metrology"
        or proxy.get("evidence") != "laboratory_characterized"
    ):
        raise ValueError("Stone-body relief must retain its metrology authority")
    if proxy.get("scan_area_m") != [0.003, 0.0015]:
        raise ValueError("Unexpected Sabucina surface-scan dimensions")
    if not math.isclose(
        float(proxy["roughness_cutoff_m"]),
        0.0008,
        abs_tol=1.0e-12,
    ):
        raise ValueError("Unexpected calcarenite roughness cutoff")
    unfiltered = proxy["fresh_reference_unfiltered"]
    expected = {
        "sa_m": 0.00010,
        "sq_m": 0.00013,
        "ssk": -0.39,
        "sku": 3.16,
        "sz_m": 0.00113,
    }
    for key, value in expected.items():
        if not math.isclose(
            float(unfiltered[key]),
            value,
            rel_tol=0.0,
            abs_tol=1.0e-12,
        ):
            raise ValueError(f"Unexpected Sabucina relief metric {key}")
    if "not the Santa Marina or Naranjo stone" not in proxy["use_limit"]:
        raise ValueError("Relief proxy must retain its transfer limitation")
    return proxy


def load_tooling_finish(
    path: str | Path = DEFAULT_LEDGER,
) -> dict[str, Any]:
    payload = json.loads(Path(path).read_text())
    measurements = {
        entry["id"]: entry
        for entry in payload["measurements"]["stone"]
    }
    finish = measurements.get(TOOLING_FINISH_ID)
    if finish is None:
        raise ValueError("Measured medieval tooling finish is missing")
    if (
        finish.get("source") != "S25_moulis_medieval_tool_traces"
        or finish.get("evidence") != "surveyed_and_experimental"
    ):
        raise ValueError("Tooling finish must retain its archaeological authority")
    if finish.get("straight_hammer_edge_length_range_m") != [0.06, 0.094]:
        raise ValueError("Unexpected straight-hammer edge-length range")
    if finish.get("fine_impact_width_range_m") != [0.001, 0.002]:
        raise ValueError("Unexpected fine-impact width range")
    density = finish.get("visible_impact_density_per_m2", {})
    if density.get("observed_range") != [720, 2420]:
        raise ValueError("Unexpected historical impact-density range")
    if density.get("observed_mean") != 1369:
        raise ValueError("Unexpected historical mean impact density")
    if finish.get("depth_status") != (
        "No groove depth is published for the surveyed fine layage."
    ):
        raise ValueError("Tooling depth limitation is missing")
    if "not a Cordoba face survey" not in finish.get("use_limit", ""):
        raise ValueError("Tooling transfer limitation is missing")
    return finish


def load_ashlar_recipe(
    path: str | Path = DEFAULT_PATTERN,
) -> AshlarRecipe:
    payload = json.loads(Path(path).read_text())
    if payload.get("schema") != PATTERN_SCHEMA:
        raise ValueError("Unexpected cathedral ashlar recipe schema")
    module = payload["module_m"]
    tile_size = float(module["width"])
    tile_height = float(module["height"])
    if not math.isclose(tile_size, 4.0, abs_tol=1.0e-6) or not math.isclose(
        tile_height, 4.0, abs_tol=1.0e-6
    ):
        raise ValueError("Measured cathedral ashlar uses a four-metre module")
    families = tuple(payload["families"])
    tooling = tuple(payload["tooling"])
    edges = tuple(payload["edge_profiles"])
    if set(tooling) != set(TOOLING_IDS):
        raise ValueError("Ashlar recipe must define all tooling families")
    if set(edges) != set(EDGE_IDS):
        raise ValueError("Ashlar recipe must define all edge profiles")
    authority = payload["measurement_authority"]
    if authority["dimension_measurement_id"] != "cordoba_early_gothic_ashlar":
        raise ValueError("The intact ashlar module must name its dimension survey")
    if authority.get("tool_finish_measurement_id") != TOOLING_FINISH_ID:
        raise ValueError("The ashlar module must name its tooling-finish authority")
    if authority["construction_scope"] != "single facing wythe; wall core is not inferred":
        raise ValueError("The measured scope may not imply an unsurveyed wall core")
    mortar = payload["mortar"]
    if float(mortar["recess_m"]) != 0.0 or mortar["profile"] != "flush":
        raise ValueError("The intact measured ashlar base must use flush mortar")
    bed = float(mortar["bed_joint_m"])
    perpend = float(mortar["perpend_joint_m"])
    surface_tooling = payload["surface_tooling"]
    if surface_tooling.get("detail_span_m") != TOOLING_DETAIL_SPAN_M:
        raise ValueError("Tooling detail must retain its 256 mm physical span")
    if surface_tooling.get("straight_hammer_edge_length_range_m") != [
        0.06,
        0.094,
    ]:
        raise ValueError("Tooling mark lengths exceed the measured finish")
    if surface_tooling.get("fine_impact_width_range_m") != [0.001, 0.002]:
        raise ValueError("Tooling mark widths exceed the measured finish")
    normalized_courses: list[dict[str, Any]] = []
    last_top = 0.0
    count = 0
    for course_index, course in enumerate(payload["courses"]):
        face_height = float(course["face_height_m"])
        bottom = last_top
        top = bottom + face_height + bed
        row_span = sum(
            float(block["face_length_m"]) + perpend
            for block in course["blocks"]
        )
        if not math.isclose(row_span, tile_size, abs_tol=1.0e-6):
            raise ValueError(f"Ashlar course {course_index} does not close")
        for block in course["blocks"]:
            family = block["family"]
            tool = block["tooling"]
            edge = block["edge_profile"]
            if family not in families:
                raise ValueError(f"Unknown stone family {family}")
            if tool not in tooling:
                raise ValueError(f"Unknown tooling family {tool}")
            if edge not in edges:
                raise ValueError(f"Unknown edge profile {edge}")
            if not 0.78 <= float(block["face_length_m"]) <= 1.13:
                raise ValueError("Block length exceeds the Cordoba survey")
            if not 0.17 <= float(block["depth_m"]) <= 0.30:
                raise ValueError("Block depth exceeds the Cordoba survey")
            if not 0.001 <= float(block["tool_spacing_m"]) <= 0.004:
                raise ValueError("Tool spacing exceeds the Caen-stone survey")
        if not 0.32 <= face_height <= 0.43:
            raise ValueError("Course height exceeds the Cordoba survey")
        normalized = dict(course)
        normalized["bottom_m"] = bottom
        normalized["top_m"] = top
        normalized_courses.append(normalized)
        count += len(course["blocks"])
        last_top = top
    if not math.isclose(last_top, tile_size, abs_tol=1.0e-6):
        raise ValueError("Ashlar courses must fill the tile vertically")
    if count != 40:
        raise ValueError("The measured cathedral pattern must own 40 blocks")
    return AshlarRecipe(
        schema=payload["schema"],
        name=payload["name"],
        tile_size_m=tile_size,
        mortar=mortar,
        families=families,
        tooling=tooling,
        edge_profiles=edges,
        courses=tuple(normalized_courses),
        measurement_authority=authority,
        bond=payload["bond"],
        excluded_from_intact_core=tuple(payload["excluded_from_intact_core"]),
    )


def generate_block_layout(
    recipe: AshlarRecipe,
    *,
    variation: int = 0,
) -> list[dict[str, Any]]:
    blocks: list[dict[str, Any]] = []
    for course_index, course in enumerate(recipe.courses):
        row_blocks = course["blocks"]
        row_origins: list[float] = []
        cursor = float(course["bond_offset_m"])
        for specification in row_blocks:
            row_origins.append(cursor)
            cursor += (
                float(specification["face_length_m"])
                + float(recipe.mortar["perpend_joint_m"])
            )
        for position, specification in enumerate(course["blocks"]):
            family = specification["family"]
            tooling = specification["tooling"]
            edge_profile = specification["edge_profile"]
            face_seed = int(specification["face_seed"])
            phase = (
                int(face_seed) * 0.61803398875
                + variation * 0.137
                + course_index * 0.071
            ) % 1.0
            pitch = (
                float(specification["face_length_m"])
                + float(recipe.mortar["perpend_joint_m"])
            )
            cyclic_id = course_index * len(row_blocks) + position + 1
            for repeat in (-1, 0, 1):
                x0 = row_origins[position] + repeat * recipe.tile_size_m
                x1 = x0 + pitch
                if x1 <= 0.0 or x0 >= recipe.tile_size_m:
                    continue
                blocks.append(
                    {
                        "block_id": cyclic_id,
                        "course_index": course_index,
                        "position": position,
                        "x0_m": x0,
                        "x1_m": x1,
                        "bottom_m": float(course["bottom_m"]),
                        "top_m": float(course["top_m"]),
                        "face_length_m": float(specification["face_length_m"]),
                        "face_height_m": float(course["face_height_m"]),
                        "depth_m": float(specification["depth_m"]),
                        "family": family,
                        "tooling": tooling,
                        "tool_spacing_m": float(specification["tool_spacing_m"]),
                        "tool_angle_deg": float(specification["tool_angle_deg"]),
                        "edge_profile": edge_profile,
                        "face_seed": face_seed,
                        "relief_m": 0.0,
                        "tilt_x_deg": 0.0,
                        "tilt_y_deg": 0.0,
                        "line_priority": (
                            0.18,
                            0.30,
                            0.48,
                            0.62,
                        )[(face_seed + cyclic_id) % 4],
                        "phase": phase,
                    }
                )
    return blocks


def rasterize_ashlar(
    layout: list[dict[str, Any]],
    recipe: AshlarRecipe,
    *,
    resolution: int,
) -> dict[str, np.ndarray]:
    coordinate = (
        np.arange(resolution, dtype=np.float32) + 0.5
    ) / resolution * recipe.tile_size_m
    x, y = np.meshgrid(coordinate, coordinate)
    block_id = np.zeros((resolution, resolution), dtype=np.int32)
    tooling_id = np.zeros_like(block_id)
    edge_id = np.zeros_like(block_id)
    local_u = np.zeros_like(x)
    local_v = np.zeros_like(y)
    edge_distance = np.zeros_like(x)
    line_priority = np.zeros_like(x)
    phase = np.zeros_like(x)
    relief = np.zeros_like(x)
    bed = float(recipe.mortar["bed_joint_m"])
    perpend = float(recipe.mortar["perpend_joint_m"])

    for block in layout:
        x0 = block["x0_m"] + perpend * 0.5
        x1 = block["x1_m"] - perpend * 0.5
        y0 = block["bottom_m"] + bed * 0.5
        y1 = block["top_m"] - bed * 0.5
        inside = (x >= x0) & (x < x1) & (y >= y0) & (y < y1)
        width = x1 - x0
        height = y1 - y0
        u = np.clip((x - x0) / width, 0.0, 1.0)
        v = np.clip((y - y0) / height, 0.0, 1.0)
        distance = np.minimum.reduce(
            [x - x0, x1 - x, y - y0, y1 - y]
        )
        block_id[inside] = block["block_id"]
        tooling_id[inside] = TOOLING_IDS[block["tooling"]]
        edge_id[inside] = EDGE_IDS[block["edge_profile"]]
        local_u[inside] = u[inside]
        local_v[inside] = v[inside]
        edge_distance[inside] = distance[inside]
        line_priority[inside] = block["line_priority"]
        phase[inside] = block["phase"]
        relief[inside] = block["relief_m"]

    stone_mask = (block_id > 0).astype(np.float32)
    return {
        "block_id": block_id,
        "tooling_id": tooling_id,
        "edge_profile_id": edge_id,
        "local_u": local_u,
        "local_v": local_v,
        "edge_distance_m": edge_distance,
        "line_priority": line_priority,
        "phase": phase,
        "relief_m": relief,
        "stone_mask": stone_mask,
        "mortar_mask": 1.0 - stone_mask,
    }


def _block_plane_system(
    layout: list[dict[str, Any]],
    raster: dict[str, np.ndarray],
) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    block_id = raster["block_id"]
    local_u = raster["local_u"]
    local_v = raster["local_v"]
    height = np.zeros_like(local_u)
    plane_identity = np.zeros_like(block_id)
    normalized = np.zeros_like(local_u)
    for block in layout:
        selected = block_id == block["block_id"]
        u = local_u[selected] - 0.5
        v = local_v[selected] - 0.5
        phase = block["phase"] * math.tau
        tilt = (
            u * math.tan(math.radians(block["tilt_x_deg"]))
            * (block["x1_m"] - block["x0_m"])
            + v * math.tan(math.radians(block["tilt_y_deg"]))
            * (block["top_m"] - block["bottom_m"])
        )
        planes = np.stack(
            [
                0.52 * u + 0.10 * v + math.sin(phase) * 0.08,
                -0.28 * u + 0.44 * v + math.cos(phase * 1.3) * 0.06,
                0.12 * u - 0.48 * v + math.sin(phase * 0.7) * 0.07,
                -0.36 * u - 0.18 * v + math.cos(phase * 1.7) * 0.05,
            ],
            axis=-1,
        )
        identity = np.argmax(planes, axis=-1)
        maximum = np.max(planes, axis=-1)
        low = float(np.percentile(maximum, 2))
        high = float(np.percentile(maximum, 98))
        coordinate = np.clip(
            (maximum - low) / max(high - low, 1.0e-6),
            0.0,
            1.0,
        )
        relief = block["relief_m"]
        height[selected] = tilt + (coordinate - 0.5) * relief * 0.72
        normalized[selected] = coordinate
        plane_identity[selected] = identity + 1

    boundary = np.zeros_like(height)
    for axis in (0, 1):
        changed = np.roll(plane_identity, 1, axis=axis) != plane_identity
        same_block = np.roll(block_id, 1, axis=axis) == block_id
        boundary = np.maximum(
            boundary,
            (changed & same_block & (block_id > 0)).astype(np.float32),
        )
    boundary = periodic_gaussian_blur(boundary, 0.65)
    return height.astype(np.float32), plane_identity, np.clip(boundary, 0.0, 1.0)


def _edge_profile(
    edge_distance: np.ndarray,
    edge_profile_id: np.ndarray,
) -> tuple[np.ndarray, np.ndarray]:
    # This transition is only sub-pixel filtering at the stone/mortar mask.
    # It is not an authored arris radius: the source establishes square, true
    # edges but does not publish a radius that could honestly be modelled.
    profile = smoothstep(0.00025, 0.0010, edge_distance)
    shoulder = np.clip(1.0 - profile, 0.0, 1.0)
    return profile.astype(np.float32), shoulder.astype(np.float32)


def _tooling_system(
    layout: list[dict[str, Any]],
    raster: dict[str, np.ndarray],
) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    block_id = raster["block_id"]
    height = np.zeros_like(raster["local_u"])
    identity = np.zeros_like(raster["local_u"])
    direction = np.zeros((*raster["local_u"].shape, 2), dtype=np.float32)
    for block in layout:
        selected = block_id == block["block_id"]
        tool = TOOLING_IDS[block["tooling"]]
        if tool != TOOLING_IDS["diagonal_drove"]:
            raise ValueError("Measured intact core only supports diagonal drove")
        angle = math.radians(block["tool_angle_deg"])
        # The four-metre construction atlas is not allowed to draw the marks:
        # at 1024 px it cannot resolve the measured 1-2 mm impact width.
        # It carries only family identity and direction. Finite impacts live in
        # the separately scaled 256 mm tooling-detail texture.
        height[selected] = 0.0
        identity[selected] = 1.0
        direction[selected, 0] = math.cos(angle)
        direction[selected, 1] = math.sin(angle)
    return height.astype(np.float32), identity.astype(np.float32), direction


def _periodic_finite_tool_impact(
    target: np.ndarray,
    *,
    center_x_m: float,
    center_y_m: float,
    length_m: float,
    width_m: float,
    angle_rad: float,
    strength: float,
    phase: float,
    span_m: float,
) -> None:
    resolution = target.shape[0]
    meters_per_pixel = span_m / resolution
    cosine = math.cos(angle_rad)
    sine = math.sin(angle_rad)
    half_length = length_m * 0.5
    radius = width_m * 0.5
    half_x = (
        abs(cosine) * half_length
        + abs(sine) * radius
        + width_m
    )
    half_y = (
        abs(sine) * half_length
        + abs(cosine) * radius
        + width_m
    )
    extent_x = max(2, int(math.ceil(half_x / meters_per_pixel)))
    extent_y = max(2, int(math.ceil(half_y / meters_per_pixel)))
    offsets_x = np.arange(-extent_x, extent_x + 1, dtype=np.int32)
    offsets_y = np.arange(-extent_y, extent_y + 1, dtype=np.int32)
    dx = offsets_x[np.newaxis, :].astype(np.float32) * meters_per_pixel
    dy = offsets_y[:, np.newaxis].astype(np.float32) * meters_per_pixel
    along = dx * cosine + dy * sine
    across = -dx * sine + dy * cosine
    closest_along = np.clip(along, -half_length, half_length)
    distance = np.sqrt((along - closest_along) ** 2 + across**2)
    profile = 1.0 - smoothstep(radius * 0.46, radius * 1.05, distance)
    # Individual impacts fade and strengthen along the edge. The modulation
    # is bounded and never converts a finite 60-94 mm strike into a stripe.
    pressure = np.clip(
        0.78
        + 0.15 * np.sin(along / max(length_m, 1.0e-9) * math.tau + phase)
        + 0.07 * np.sin(
            along / max(length_m, 1.0e-9) * math.tau * 3.0 - phase * 0.7
        ),
        0.42,
        1.0,
    )
    mark = np.clip(profile * pressure * strength, 0.0, 1.0)
    center_column = int(round(center_x_m / meters_per_pixel)) % resolution
    center_row = int(round(center_y_m / meters_per_pixel)) % resolution
    columns = (center_column + offsets_x) % resolution
    rows = (center_row + offsets_y) % resolution
    current = target[np.ix_(rows, columns)]
    target[np.ix_(rows, columns)] = np.maximum(current, mark)


def _tooling_detail_variants(
    *,
    resolution: int,
    seed: int,
    finish: dict[str, Any],
) -> dict[str, Any]:
    span_m = TOOLING_DETAIL_SPAN_M
    length_low, length_high = (
        float(value)
        for value in finish["straight_hammer_edge_length_range_m"]
    )
    width_low, width_high = (
        float(value)
        for value in finish["fine_impact_width_range_m"]
    )
    spacing_low = 0.001
    spacing_high = 0.004
    variants = np.zeros((resolution, resolution, 3), dtype=np.float32)
    stroke_counts: list[int] = []
    actual_densities: list[float] = []
    lengths: list[float] = []
    widths: list[float] = []
    spacings: list[float] = []
    fan_angles: list[float] = []

    for variant_index, density in enumerate(
        TOOLING_DENSITY_VARIANTS_PER_M2
    ):
        rng = np.random.default_rng(seed + 7411 + variant_index * 1699)
        count = int(round(density * span_m * span_m))
        stroke_counts.append(count)
        actual_densities.append(count / (span_m * span_m))
        stroke_index = 0
        pass_index = 0
        while stroke_index < count:
            remaining = count - stroke_index
            pass_size = min(int(rng.integers(3, 9)), remaining)
            spacing = float(rng.uniform(spacing_low, spacing_high))
            base_x = float(rng.uniform(0.0, span_m))
            base_y = float(rng.uniform(0.0, span_m))
            pass_angle = math.radians(float(rng.uniform(-3.0, 3.0)))
            fan_step = math.radians(float(rng.uniform(-0.65, 0.65)))
            lateral_drift = float(rng.uniform(-0.0035, 0.0035))
            centered = (pass_size - 1) * 0.5
            for pass_position in range(pass_size):
                offset = pass_position - centered
                length_m = float(rng.uniform(length_low, length_high))
                width_m = float(rng.uniform(width_low, width_high))
                angle = (
                    pass_angle
                    + offset * fan_step
                    + math.radians(float(rng.uniform(-0.8, 0.8)))
                )
                center_x = (
                    base_x
                    + offset * lateral_drift
                    + float(rng.uniform(-0.0025, 0.0025))
                ) % span_m
                center_y = (
                    base_y
                    + offset * spacing
                    + float(rng.uniform(-0.00035, 0.00035))
                ) % span_m
                _periodic_finite_tool_impact(
                    variants[..., variant_index],
                    center_x_m=center_x,
                    center_y_m=center_y,
                    length_m=length_m,
                    width_m=width_m,
                    angle_rad=angle,
                    strength=float(rng.uniform(0.46, 1.0)),
                    phase=float(rng.uniform(0.0, math.tau)),
                    span_m=span_m,
                )
                lengths.append(length_m)
                widths.append(width_m)
                spacings.append(spacing)
                fan_angles.append(math.degrees(angle))
                stroke_index += 1
            pass_index += 1

    metrics = {
        "finish_measurement_id": finish["id"],
        "variant_count": 3,
        "target_density_per_m2": list(TOOLING_DENSITY_VARIANTS_PER_M2),
        "stroke_counts": stroke_counts,
        "actual_density_per_m2": actual_densities,
        "stroke_length_range_m": [min(lengths), max(lengths)],
        "stroke_width_range_m": [min(widths), max(widths)],
        "intra_pass_spacing_range_m": [min(spacings), max(spacings)],
        "fan_angle_range_deg": [min(fan_angles), max(fan_angles)],
        "coverage_over_0_08": [
            float((variants[..., channel] > 0.08).mean())
            for channel in range(3)
        ],
        "height_depth_authored": False,
    }
    return {
        "span_m": span_m,
        "masks": variants,
        "metrics": metrics,
    }


def _expanded_family_palette(
    entry: dict[str, Any],
    *,
    tint: np.ndarray,
) -> np.ndarray:
    captured = _palette(entry, label="stone family")
    center = captured[10]
    expanded = center + (captured - center) * 1.08 + tint
    return np.clip(expanded, 0.22, 0.86).astype(np.float32)


def _block_color_family(face_seed: int, variation: int) -> str:
    value = (
        (face_seed * 2654435761 + variation * 2246822519) & 0xFFFFFFFF
    ) / 0xFFFFFFFF
    if value < 0.52:
        return "mid_buff"
    if value < 0.74:
        return "cool_buff"
    if value < 0.92:
        return "warm_ochre"
    return "pale_cream"


def _block_palettes(
    layout: list[dict[str, Any]],
    capture: dict[str, Any],
    *,
    variation: int,
) -> np.ndarray:
    result = np.zeros((len(layout) + 1, 20, 3), dtype=np.float32)
    families = capture["photographic_color"]["families"]
    for block in layout:
        phase = block["phase"] * math.tau + variation * 0.19
        tint = np.asarray(
            [
                math.sin(phase) * 0.007,
                math.cos(phase * 1.3) * 0.005,
                math.sin(phase * 0.8 + 0.6) * 0.004,
            ],
            dtype=np.float32,
        )
        family = _block_color_family(block["face_seed"], variation)
        result[block["block_id"]] = _expanded_family_palette(
            families[family],
            tint=tint,
        )
    return result


def _sample_palette(
    palettes: np.ndarray,
    identity: np.ndarray,
    coordinate: np.ndarray,
) -> np.ndarray:
    index = np.clip(coordinate, 0.0, 1.0) * (palettes.shape[1] - 1)
    lower = np.floor(index).astype(np.int32)
    upper = np.minimum(lower + 1, palettes.shape[1] - 1)
    fraction = index - lower
    return (
        palettes[identity, lower] * (1.0 - fraction[..., None])
        + palettes[identity, upper] * fraction[..., None]
    ).astype(np.float32)


def _per_block_shifted_field(
    layout: list[dict[str, Any]],
    raster: dict[str, np.ndarray],
    source: np.ndarray,
    *,
    salt: int,
) -> np.ndarray:
    result = np.zeros_like(raster["local_u"])
    rows, columns = np.indices(source.shape)
    resolution = source.shape[0]
    for block in layout:
        selected = raster["block_id"] == block["block_id"]
        shift_x = (block["face_seed"] * 37 + salt * 17) % resolution
        shift_y = (block["face_seed"] * 53 + salt * 29) % resolution
        result[selected] = source[
            (rows[selected] + shift_y) % resolution,
            (columns[selected] + shift_x) % resolution,
        ]
    return result.astype(np.float32)


def _macro_body_fields(
    layout: list[dict[str, Any]],
    raster: dict[str, np.ndarray],
    *,
    resolution: int,
    seed: int,
    variation: int,
) -> dict[str, np.ndarray]:
    cloud_source = periodic_fbm_rect(
        resolution,
        10,
        8,
        seed + variation * 101,
        octaves=4,
        persistence=0.48,
    )
    sandy_source = periodic_fbm_rect(
        resolution,
        34,
        29,
        seed + 1409 + variation * 131,
        octaves=3,
        persistence=0.44,
    )
    iron_source = periodic_fbm_rect(
        resolution,
        112,
        91,
        seed + 3203 + variation * 173,
        octaves=2,
        persistence=0.42,
    )
    cloud = _per_block_shifted_field(
        layout,
        raster,
        cloud_source,
        salt=7,
    )
    sandy = _per_block_shifted_field(
        layout,
        raster,
        sandy_source,
        salt=13,
    )
    iron = _per_block_shifted_field(
        layout,
        raster,
        iron_source,
        salt=19,
    )
    sandy_bloom = smoothstep(0.30, 0.82, sandy)
    iron_oxide = (
        smoothstep(0.88, 0.965, iron)
        * smoothstep(0.42, 0.72, sandy)
        * raster["stone_mask"]
    )
    return {
        "matrix_cloud": cloud.astype(np.float32),
        "sandy_bloom": sandy_bloom.astype(np.float32),
        "iron_oxide": np.clip(iron_oxide, 0.0, 1.0).astype(np.float32),
    }


def _periodic_ellipse_splats(
    resolution: int,
    *,
    count: int,
    radius_range_px: tuple[float, float],
    seed: int,
    hollow_fraction: float,
) -> np.ndarray:
    rng = np.random.default_rng(seed)
    result = np.zeros((resolution, resolution), dtype=np.float32)
    radius_scale = resolution / 1024.0
    # The physical tile always spans 64 mm, so the number of inclusions stays
    # constant when proof resolution changes. Only their pixel radius scales.
    for _ in range(count):
        center_x = int(rng.integers(0, resolution))
        center_y = int(rng.integers(0, resolution))
        radius_x = float(
            rng.uniform(*radius_range_px)
        ) * radius_scale
        radius_y = radius_x * float(rng.uniform(0.30, 1.05))
        angle = float(rng.uniform(0.0, math.tau))
        extent = max(2, math.ceil(max(radius_x, radius_y) * 1.25))
        offsets = np.arange(-extent, extent + 1, dtype=np.float32)
        dx = offsets[np.newaxis, :]
        dy = offsets[:, np.newaxis]
        cosine = math.cos(angle)
        sine = math.sin(angle)
        rotated_x = dx * cosine + dy * sine
        rotated_y = -dx * sine + dy * cosine
        distance = np.sqrt(
            (rotated_x / max(radius_x, 0.25)) ** 2
            + (rotated_y / max(radius_y, 0.25)) ** 2
        )
        angle_field = np.arctan2(
            rotated_y / max(radius_y, 0.25),
            rotated_x / max(radius_x, 0.25),
        )
        edge_variation = (
            1.0
            + float(rng.uniform(0.05, 0.14))
            * np.sin(
                angle_field * int(rng.integers(3, 6))
                + float(rng.uniform(0.0, math.tau))
            )
            + float(rng.uniform(0.025, 0.09))
            * np.sin(
                angle_field * int(rng.integers(6, 10))
                + float(rng.uniform(0.0, math.tau))
            )
        )
        distance /= np.maximum(edge_variation, 0.72)
        # A radial falloff avoids large flat-valued interiors. That preserves
        # an irregular fragment silhouette when coverage is normalized instead
        # of turning overlapping inclusions into one binary white carpet.
        shape = np.clip(1.0 - distance**1.6, 0.0, 1.0)
        if rng.random() < hollow_fraction:
            inner = smoothstep(
                float(rng.uniform(0.28, 0.48)),
                float(rng.uniform(0.54, 0.70)),
                distance,
            )
            shape *= inner
        rows = (
            center_y + offsets.astype(np.int32)
        ) % resolution
        columns = (
            center_x + offsets.astype(np.int32)
        ) % resolution
        current = result[np.ix_(rows, columns)]
        result[np.ix_(rows, columns)] = np.maximum(current, shape)
    return result


def _coverage_normalized_mask(
    field: np.ndarray,
    target_fraction: float,
) -> np.ndarray:
    threshold = float(np.quantile(field, 1.0 - target_fraction))
    positive = field[field > 0.0]
    if threshold <= 0.0 and len(positive):
        threshold = float(np.percentile(positive, 2))
    transition = (
        max(float(np.percentile(positive, 75) - threshold) * 0.18, 0.025)
        if len(positive)
        else 0.025
    )
    result = smoothstep(
        threshold - transition,
        threshold + transition,
        field,
    )
    return (result * (field > 0.0)).astype(np.float32)


def _stone_body_detail(
    *,
    resolution: int,
    seed: int,
    fossil_fraction: tuple[float, float],
) -> dict[str, np.ndarray]:
    target_fossil = sum(fossil_fraction) * 0.5
    fossil_raw = _periodic_ellipse_splats(
        resolution,
        count=4300,
        radius_range_px=(2.0, 15.0),
        seed=seed + 401,
        hollow_fraction=0.27,
    )
    meso_fossil_raw = _periodic_ellipse_splats(
        resolution,
        count=72,
        radius_range_px=(28.0, 112.0),
        seed=seed + 557,
        hollow_fraction=0.42,
    )
    fossil_raw = np.maximum(fossil_raw, meso_fossil_raw)
    fossil = _coverage_normalized_mask(fossil_raw, target_fossil)
    silicate_source = (
        periodic_fbm_rect(
            resolution,
            246,
            227,
            seed + 809,
            octaves=2,
            persistence=0.36,
        )
        * 0.68
        + periodic_fbm_rect(
            resolution,
            487,
            461,
            seed + 1217,
            octaves=1,
        )
        * 0.32
    )
    silicate = _coverage_normalized_mask(silicate_source, 0.18)
    pore_raw = _periodic_ellipse_splats(
        resolution,
        count=1650,
        radius_range_px=(1.0, 4.8),
        seed=seed + 1601,
        hollow_fraction=0.0,
    )
    pore_raw = np.maximum(
        pore_raw,
        _periodic_ellipse_splats(
            resolution,
            count=46,
            radius_range_px=(12.0, 54.0),
            seed=seed + 1741,
            hollow_fraction=0.0,
        ),
    )
    visible_pore = _coverage_normalized_mask(pore_raw, 0.035)
    masks = np.stack(
        [fossil, silicate, visible_pore],
        axis=-1,
    )
    return {
        "span_m": BODY_DETAIL_SPAN_M,
        "masks": masks.astype(np.float32),
        "fossil_fragment_mask": fossil,
        "silicate_grain_mask": silicate,
        "visible_pore_identity_mask": visible_pore,
    }


def _height_statistics(height_m: np.ndarray) -> dict[str, float]:
    centered = height_m.astype(np.float64) - float(height_m.mean())
    sq = float(np.sqrt(np.mean(centered * centered)))
    if sq <= 1.0e-15:
        return {
            "sa_m": 0.0,
            "sq_m": 0.0,
            "ssk": 0.0,
            "sku": 0.0,
            "sp_m": 0.0,
            "sv_m": 0.0,
            "sz_m": 0.0,
        }
    return {
        "sa_m": float(np.mean(np.abs(centered))),
        "sq_m": sq,
        "ssk": float(np.mean(centered**3) / sq**3),
        "sku": float(np.mean(centered**4) / sq**4),
        "sp_m": float(centered.max()),
        "sv_m": float(-centered.min()),
        "sz_m": float(height_m.max() - height_m.min()),
    }


def _stone_body_relief(
    *,
    body_detail: dict[str, np.ndarray],
    resolution: int,
    seed: int,
    proxy: dict[str, Any],
) -> dict[str, Any]:
    span_m = BODY_DETAIL_SPAN_M
    meters_per_pixel = span_m / resolution
    unfiltered = proxy["fresh_reference_unfiltered"]
    roughness_component = proxy["fresh_reference_roughness_component"]
    waviness_component = proxy["fresh_reference_waviness_component"]

    cutoff_m = float(proxy["roughness_cutoff_m"])
    cutoff_cycles = max(8, int(round(span_m / cutoff_m)))
    rough_cells_x = min(max(12, resolution // 2), cutoff_cycles)
    rough_cells_y = min(max(12, resolution // 2), max(8, cutoff_cycles - 7))
    fine_cells_x = min(max(16, resolution // 2), int(round(cutoff_cycles * 1.42)))
    fine_cells_y = min(max(16, resolution // 2), int(round(cutoff_cycles * 1.34)))

    grain_field = (
        periodic_fbm_rect(
            resolution,
            rough_cells_x,
            rough_cells_y,
            seed + 2027,
            octaves=2,
            persistence=0.42,
        )
        - 0.5
    )
    fine_field = (
        periodic_fbm_rect(
            resolution,
            fine_cells_x,
            fine_cells_y,
            seed + 3251,
            octaves=1,
        )
        - 0.5
    )
    fossil = periodic_gaussian_blur(
        body_detail["fossil_fragment_mask"],
        max(0.00009 / meters_per_pixel, 0.35),
    )
    fossil -= float(fossil.mean())
    silicate = periodic_gaussian_blur(
        body_detail["silicate_grain_mask"],
        max(0.00006 / meters_per_pixel, 0.30),
    )
    silicate -= float(silicate.mean())
    visible_pore = np.power(
        np.clip(
            periodic_gaussian_blur(
                body_detail["visible_pore_identity_mask"],
                max(0.000045 / meters_per_pixel, 0.25),
            ),
            0.0,
            1.0,
        ),
        1.6,
    )
    visible_pore -= float(visible_pore.mean())

    roughness = (
        grain_field * 0.62
        + fine_field * 0.28
        + fossil * 0.20
        + silicate * 0.06
        - visible_pore * 0.30
    )
    roughness -= float(roughness.mean())
    roughness_std = float(roughness.std())
    if roughness_std <= 1.0e-12:
        raise ValueError("Calcarenite roughness synthesis collapsed")
    roughness /= roughness_std
    # A small negative quadratic bias reproduces the measured distribution:
    # most of the surface remains near the upper body while sparse pores and
    # grain boundaries form deeper valleys.
    roughness = roughness - 0.09 * (
        roughness * roughness - float(np.mean(roughness * roughness))
    )
    roughness -= float(roughness.mean())
    roughness *= (
        float(roughness_component["sq_m"])
        / max(float(roughness.std()), 1.0e-12)
    )

    waviness = (
        periodic_fbm_rect(
            resolution,
            13,
            11,
            seed + 4937,
            octaves=3,
            persistence=0.48,
        )
        - 0.5
    )
    waviness = periodic_gaussian_blur(
        waviness,
        max(cutoff_m / meters_per_pixel, 0.5),
    )
    waviness -= float(waviness.mean())
    waviness *= (
        float(waviness_component["sq_m"])
        / max(float(waviness.std()), 1.0e-12)
    )

    height_m = roughness + waviness
    height_m -= float(height_m.mean())
    target_sq = float(unfiltered["sq_m"])
    height_m *= target_sq / max(float(height_m.std()), 1.0e-12)
    target_range = float(unfiltered["sz_m"])
    height_m *= target_range / max(float(np.ptp(height_m)), 1.0e-12)
    height_m -= float(height_m.min())
    height_m = height_m.astype(np.float32)
    normal = height_to_normal(
        height_m,
        meters_per_pixel,
        strength=1.0,
    )
    metrics = _height_statistics(height_m)
    metrics["height_seam_m"] = _seam_metric(height_m)
    metrics["roughness_cutoff_m"] = cutoff_m
    metrics["meters_per_pixel"] = meters_per_pixel
    return {
        "span_m": span_m,
        "height_m": height_m,
        "normal": normal,
        "metrics": metrics,
        "proxy_measurement_id": proxy["id"],
    }


def generate_material(
    *,
    resolution: int = 1024,
    seed: int = 823451,
    pattern_variation: int = 0,
    recipe_path: str | Path = DEFAULT_PATTERN,
    capture_path: str | Path = DEFAULT_CAPTURE,
    ledger_path: str | Path = DEFAULT_LEDGER,
) -> dict[str, Any]:
    recipe = load_ashlar_recipe(recipe_path)
    capture = load_reference_capture(capture_path)
    relief_proxy = load_relief_proxy(ledger_path)
    tooling_finish = load_tooling_finish(ledger_path)
    layout = generate_block_layout(recipe, variation=pattern_variation)
    raster = rasterize_ashlar(layout, recipe, resolution=resolution)
    plane_height, plane_id, plane_boundary = _block_plane_system(
        layout, raster
    )
    edge_profile, edge_shoulder = _edge_profile(
        raster["edge_distance_m"],
        raster["edge_profile_id"],
    )
    tooling_height, tooling_identity, brush_direction = _tooling_system(
        layout, raster
    )
    stone_mask = raster["stone_mask"]
    mortar_mask = raster["mortar_mask"]
    body_fields = _macro_body_fields(
        layout,
        raster,
        resolution=resolution,
        seed=seed,
        variation=pattern_variation,
    )
    body_detail = _stone_body_detail(
        resolution=resolution,
        seed=seed + pattern_variation * 409,
        fossil_fraction=tuple(
            capture["stone_body"]["fossil_component_fraction"]
        ),
    )
    body_relief = _stone_body_relief(
        body_detail=body_detail,
        resolution=resolution,
        seed=seed + pattern_variation * 607,
        proxy=relief_proxy,
    )
    tooling_detail = _tooling_detail_variants(
        resolution=resolution,
        seed=seed + pattern_variation * 811,
        finish=tooling_finish,
    )
    block_palettes = _block_palettes(
        layout,
        capture,
        variation=pattern_variation,
    )

    graphic_plane_bias = np.take(
        np.asarray(
            [0.0, -0.060, 0.018, 0.052, -0.024],
            dtype=np.float32,
        ),
        np.clip(plane_id, 0, 4),
    )
    block_body_coordinate = np.zeros_like(stone_mask)
    for block in layout:
        selected = raster["block_id"] == block["block_id"]
        block_body_coordinate[selected] = (
            0.50
            + math.sin(float(block["phase"]) * math.tau) * 0.075
        )
    shade_coordinate = np.clip(
        block_body_coordinate
        + graphic_plane_bias
        * (0.42 + raster["line_priority"] * 0.58)
        + (body_fields["matrix_cloud"] - 0.5) * 0.14
        + (body_fields["sandy_bloom"] - 0.5) * 0.045,
        0.18,
        0.82,
    )
    shade_coordinate = (
        np.floor(shade_coordinate * 19.0 + 0.5) / 19.0
    ).astype(np.float32)
    stone_color = _sample_palette(
        block_palettes,
        raster["block_id"],
        shade_coordinate,
    )
    sandy_tint = np.clip(
        stone_color
        * np.asarray([0.985, 0.995, 1.018], dtype=np.float32)
        + np.asarray([0.004, 0.004, 0.006], dtype=np.float32),
        0.0,
        1.0,
    )
    stone_color = mix(
        stone_color,
        sandy_tint,
        body_fields["sandy_bloom"] * 0.16,
    )
    iron_tint = np.clip(
        stone_color
        * np.asarray([1.035, 0.955, 0.86], dtype=np.float32),
        0.0,
        1.0,
    )
    stone_color = mix(
        stone_color,
        iron_tint,
        body_fields["iron_oxide"] * 0.34,
    )
    captured_stone_palette = _palette(
        capture["photographic_color"]["all_sunlit_stone"],
        label="all sunlit stone",
    )
    mortar_palette = np.clip(
        captured_stone_palette * 0.72
        + np.asarray([0.19, 0.18, 0.16], dtype=np.float32),
        0.0,
        1.0,
    )
    mortar_noise = periodic_fbm_rect(
        resolution,
        4,
        3,
        seed + pattern_variation * 271,
        octaves=3,
        persistence=0.48,
    )
    mortar_coordinate = np.clip(
        0.30 + mortar_noise * 0.48,
        0.0,
        1.0,
    )
    mortar_color = _sample_palette(
        mortar_palette[np.newaxis, ...],
        np.zeros((resolution, resolution), dtype=np.int32),
        mortar_coordinate,
    )

    edge_zone = (
        stone_mask
        * (1.0 - smoothstep(0.003, 0.021, raster["edge_distance_m"]))
    )
    segment_selector = smoothstep(
        0.56,
        0.83,
        0.5
        + 0.5
        * np.sin(
            (
                raster["local_u"] * 2.4
                + raster["local_v"] * 1.7
                + raster["phase"] * 1.9
            )
            * math.tau
        ),
    )
    primary_ink = (
        edge_zone
        * segment_selector
        * raster["line_priority"]
    )
    plane_ink = (
        plane_boundary
        * smoothstep(0.42, 0.70, raster["line_priority"])
        * stone_mask
        * 0.46
    )
    ink_mask = np.clip(primary_ink + plane_ink, 0.0, 1.0)
    upper_arris = (
        stone_mask
        * (1.0 - smoothstep(0.006, 0.025, 1.0 - raster["local_v"]))
    )
    highlight_selector = smoothstep(
        0.70,
        0.91,
        0.5
        + 0.5
        * np.sin(
            (
                raster["local_u"] * 1.8
                - raster["local_v"] * 0.9
                + raster["phase"] * 2.3
            )
            * math.tau
        ),
    )
    highlight_mask = np.clip(
        upper_arris
        * highlight_selector
        * (0.26 + raster["line_priority"] * 0.48)
        + plane_boundary
        * smoothstep(0.65, 0.88, shade_coordinate)
        * 0.20,
        0.0,
        1.0,
    )

    base_color_srgb = mix(mortar_color, stone_color, stone_mask)
    base_color_srgb = np.clip(base_color_srgb, 0.0, 1.0)

    detail_coordinate = np.clip(
        0.26
        + periodic_fbm_rect(
            resolution,
            7,
            6,
            seed + 2017 + pattern_variation * 227,
            octaves=4,
            persistence=0.46,
        )
        * 0.58,
        0.0,
        1.0,
    )
    detail_palette = captured_stone_palette[np.newaxis, ...]
    detail_base = _sample_palette(
        detail_palette,
        np.zeros((resolution, resolution), dtype=np.int32),
        detail_coordinate,
    )
    pale_fossil = np.broadcast_to(
        _palette(
            capture["photographic_color"]["families"]["pale_cream"],
            label="pale cream",
        )[12],
        detail_base.shape,
    )
    cool_silicate = np.broadcast_to(
        _palette(
            capture["photographic_color"]["families"]["cool_buff"],
            label="cool buff",
        )[14],
        detail_base.shape,
    )
    body_detail_color = mix(
        detail_base,
        pale_fossil,
        body_detail["fossil_fragment_mask"] * 0.52,
    )
    body_detail_color = mix(
        body_detail_color,
        cool_silicate,
        body_detail["silicate_grain_mask"] * 0.58,
    )
    body_detail_color = mix(
        body_detail_color,
        body_detail_color
        * np.asarray([0.68, 0.70, 0.68], dtype=np.float32),
        body_detail["visible_pore_identity_mask"] * 0.76,
    )
    body_detail_color = np.clip(body_detail_color, 0.0, 1.0)

    block_raise = np.zeros_like(raster["relief_m"])
    stone_height = (
        block_raise
        + plane_height
        + tooling_height
    ) * edge_profile
    mortar_height = np.full_like(
        stone_mask,
        -float(recipe.mortar["recess_m"]),
    )
    height_m = (
        stone_height * stone_mask + mortar_height * mortar_mask
    ).astype(np.float32)
    normal = height_to_normal(
        periodic_gaussian_blur(height_m, 0.42),
        recipe.tile_size_m / resolution,
        strength=1.0,
    )

    stone_roughness = np.full_like(stone_mask, 0.735)
    stone_roughness += (
        body_fields["matrix_cloud"] - 0.5
    ) * 0.055
    stone_roughness += body_fields["sandy_bloom"] * 0.018
    mortar_roughness = 0.785 + (mortar_noise - 0.5) * 0.045
    roughness = np.clip(
        stone_roughness * stone_mask
        + mortar_roughness * mortar_mask,
        0.66,
        0.82,
    )
    body_detail_roughness = np.clip(
        0.735
        + body_detail["fossil_fragment_mask"] * 0.015
        - body_detail["silicate_grain_mask"] * 0.025
        + body_detail["visible_pore_identity_mask"] * 0.045,
        0.68,
        0.82,
    )
    ao = np.ones_like(stone_mask, dtype=np.float32)
    detail_priority = np.clip(
        0.10
        + ink_mask * 0.42
        + highlight_mask * 0.24,
        0.0,
        1.0,
    )

    return {
        "schema": MATERIAL_SCHEMA,
        "recipe": recipe,
        "reference_capture": capture,
        "reference_capture_path": Path(capture_path),
        "relief_proxy": relief_proxy,
        "relief_proxy_path": Path(ledger_path),
        "tooling_finish": tooling_finish,
        "resolution": resolution,
        "seed": seed,
        "pattern_variation": pattern_variation,
        "block_count": len({block["block_id"] for block in layout}),
        "raster_block_occurrence_count": len(layout),
        "shade_family_size": 20,
        "layout": layout,
        **raster,
        "plane_height_m": plane_height,
        "plane_id": plane_id,
        "plane_boundary_mask": plane_boundary,
        "edge_profile": edge_profile,
        "edge_shoulder_mask": edge_shoulder,
        "tooling_height_m": tooling_height,
        "tooling_identity_mask": tooling_identity,
        "brush_direction": brush_direction,
        "tooling_detail_span_m": tooling_detail["span_m"],
        "tooling_detail_masks": tooling_detail["masks"],
        "tooling_detail_metrics": tooling_detail["metrics"],
        "matrix_cloud": body_fields["matrix_cloud"],
        "sandy_bloom_mask": body_fields["sandy_bloom"],
        "iron_oxide_mask": body_fields["iron_oxide"],
        "stone_body_masks": body_detail["masks"],
        "stone_body_detail_span_m": body_detail["span_m"],
        "stone_body_relief_span_m": body_relief["span_m"],
        "stone_body_height_m": body_relief["height_m"],
        "stone_body_normal": body_relief["normal"],
        "stone_body_relief_metrics": body_relief["metrics"],
        "stone_body_relief_proxy_measurement_id": body_relief[
            "proxy_measurement_id"
        ],
        "stone_body_detail_base_color_srgb": body_detail_color,
        "stone_body_detail_base_color_linear": srgb_to_linear(
            body_detail_color
        ),
        "stone_body_detail_roughness": body_detail_roughness.astype(
            np.float32
        ),
        "fossil_fragment_mask": body_detail["fossil_fragment_mask"],
        "silicate_grain_mask": body_detail["silicate_grain_mask"],
        "visible_pore_identity_mask": body_detail[
            "visible_pore_identity_mask"
        ],
        "shade_coordinate": shade_coordinate,
        "graphic_plane_bias": graphic_plane_bias.astype(np.float32),
        "block_palettes_srgb": block_palettes,
        "mortar_palette_srgb": mortar_palette,
        "primary_ink_mask": primary_ink.astype(np.float32),
        "ink_mask": ink_mask.astype(np.float32),
        "highlight_mask": highlight_mask.astype(np.float32),
        "detail_priority": detail_priority.astype(np.float32),
        "base_color_srgb": base_color_srgb,
        "base_color_linear": srgb_to_linear(base_color_srgb),
        "height_m": height_m,
        "normal": normal,
        "roughness": roughness.astype(np.float32),
        "ao": ao.astype(np.float32),
        "metallic": np.zeros_like(roughness, dtype=np.float32),
    }


def _normal_rgb(normal: np.ndarray) -> np.ndarray:
    return np.clip(normal * 0.5 + 0.5, 0.0, 1.0)


def _false_identity(identity: np.ndarray) -> np.ndarray:
    colors = np.asarray(
        [
            [0.07, 0.08, 0.09],
            [0.27, 0.47, 0.58],
            [0.64, 0.50, 0.29],
            [0.45, 0.56, 0.36],
            [0.58, 0.37, 0.49],
            [0.62, 0.65, 0.61],
        ],
        dtype=np.float32,
    )
    result = colors[identity % len(colors)]
    return np.where((identity > 0)[..., None], result, colors[0])


def _tile_2x2(image: np.ndarray) -> np.ndarray:
    return np.tile(image, (2, 2, 1) if image.ndim == 3 else (2, 2))


def _proof_pages(material: dict[str, Any]) -> dict[str, np.ndarray]:
    target = 288
    base = material["base_color_srgb"]
    lit = render_material_preview(
        material["base_color_linear"],
        material["normal"],
        material["roughness"],
        material["ao"],
    )
    grazing = render_material_single_light(
        material["base_color_linear"],
        material["normal"],
        material["roughness"],
        material["ao"],
        direction=(-0.91, -0.22, 0.34),
        intensity=2.8,
    )
    opposite = render_material_single_light(
        material["base_color_linear"],
        material["normal"],
        material["roughness"],
        material["ao"],
        direction=(0.86, 0.18, 0.48),
        intensity=2.4,
    )
    detail_normal = material["stone_body_normal"]
    detail_lit = render_material_preview(
        material["stone_body_detail_base_color_linear"],
        detail_normal,
        material["stone_body_detail_roughness"],
        np.ones_like(material["stone_body_detail_roughness"]),
    )
    height = normalized_range(material["height_m"])
    tooling = material["tooling_id"]
    edge = material["edge_profile_id"]
    palette = np.concatenate(
        [
            _palette(entry, label=family)
            for family, entry in material["reference_capture"][
                "photographic_color"
            ]["families"].items()
        ],
        axis=0,
    )
    construction = compose_material_chapter(
        "ARCHITECTURE BEFORE WEATHER",
        "TEN COURSES, FORTY FULL-VOLUME BLOCK DESIGNS, ONE SURVEY FAMILY",
        [
            ("BLOCK ID", _false_identity(material["block_id"])),
            ("TOOLING ID", _false_identity(tooling)),
            ("EDGE PROFILE", _false_identity(edge)),
            ("MORTAR MASK", gray_rgb(material["mortar_mask"])),
            ("LOCAL U", scalar_tint(material["local_u"], (0.12, 0.18, 0.25), (0.82, 0.70, 0.34))),
            ("LOCAL V", scalar_tint(material["local_v"], (0.20, 0.14, 0.22), (0.36, 0.78, 0.66))),
            ("HEIGHT", gray_rgb(height)),
            ("NEUTRAL LIT", lit),
        ],
        [
            "Cordoba survey ranges constrain every face height, length, and depth; the exact bond is an authored translation, not a claimed elevation survey.",
            "Local coordinates stop one surface noise from crossing unrelated blocks.",
            "Three-millimetre flush joints replace the unsupported eleven-to-fourteen-millimetre recessed grooves.",
            "Damage and weather overlays are absent from this core acceptance pass.",
        ],
    )
    planes = compose_material_chapter(
        "PLANES CREATE TOUCH",
        "PIECEWISE PLANAR FACES REPLACE AIRBRUSHED CLOUD VOLUME",
        [
            ("PLANE ID", _false_identity(material["plane_id"])),
            ("PLANE HEIGHT", gray_rgb(normalized_range(material["plane_height_m"]))),
            ("PLANE BOUNDARY", gray_rgb(material["plane_boundary_mask"])),
            ("EDGE SHOULDER", gray_rgb(material["edge_shoulder_mask"])),
            ("NORMAL", _normal_rgb(material["normal"])),
            ("LEFT GRAZING", grazing),
            ("RIGHT GRAZING", opposite),
            ("FACE CLOSE-UP", proof_resize(zoom_square(grazing, center_u=0.42, center_v=0.58, fraction=0.22), target)),
        ],
        [
            "The intact base does not invent face relief or bed tilt where the selected survey gives no depth measurement.",
            "Square, true edges are retained; the mask transition is filtering, not a claimed arris radius.",
            "Full block volumes and real joints now carry the architectural read under grazing light.",
            "Measured tooling spacing remains a separate identity lane until groove depth is measured.",
        ],
    )
    tooling_detail = material["tooling_detail_masks"]
    tool_page = compose_material_chapter(
        "FINITE MEDIEVAL FACE DRESSING",
        "SIXTY-TO-NINETY-FOUR MILLIMETRE IMPACTS, NOT PERFECT STRIPES",
        [
            ("SPARSE 720/M2", gray_rgb(tooling_detail[..., 0])),
            ("AVERAGE 1369/M2", gray_rgb(tooling_detail[..., 1])),
            ("DENSE 2420/M2", gray_rgb(tooling_detail[..., 2])),
            (
                "AVERAGE 2X2",
                gray_rgb(
                    proof_resize(_tile_2x2(tooling_detail[..., 1]), target)
                ),
            ),
            (
                "FINITE ENDS",
                proof_resize(
                    zoom_square(
                        gray_rgb(tooling_detail[..., 1]),
                        center_u=0.48,
                        center_v=0.54,
                        fraction=0.28,
                    ),
                    target,
                ),
            ),
            ("MEASURED ID", _false_identity(tooling)),
            ("TOOL HEIGHT", gray_rgb(normalized_range(material["tooling_height_m"]))),
            ("BRUSH X", scalar_tint(material["brush_direction"][..., 0] * 0.5 + 0.5, (0.14, 0.19, 0.28), (0.87, 0.62, 0.28))),
        ],
        [
            "The Caen-stone assemblage supplies one-to-four-millimetre rhythm and diagonal working; the Moulis survey supplies 60-94 mm tool edges and 1-2 mm fine-impact width.",
            "Each density channel contains finite overlapping impacts at the historical 720, 1369, and 2420 visible-marks-per-square-metre levels.",
            "The shader rotates and phases one density variant per block, with slight fan variation inside each pass; it never projects one perfect stripe field across the wall.",
            "No normal or height depth is emitted because neither selected source publishes groove depth. Sharp color and roughness response remain explicitly authored optical linework.",
        ],
    )
    color_page = compose_material_chapter(
        "TWENTY-STEP STONE COLOR FAMILIES",
        "FOUR TO SIX RELATED VALUES INSIDE EACH BLOCK KEEP THE BODY QUIET",
        [
            ("FACADE FAMILIES", palette_panel(palette, resolution=target)),
            ("SHADE COORDINATE", gray_rgb(material["shade_coordinate"])),
            ("MATRIX CLOUD", gray_rgb(material["matrix_cloud"])),
            ("SANDY BLOOM", gray_rgb(material["sandy_bloom_mask"])),
            ("IRON COLOR", gray_rgb(material["iron_oxide_mask"])),
            ("BASE COLOR", base),
            ("UNLIT CLOSE-UP", proof_resize(zoom_square(base, center_u=0.52, center_v=0.43, fraction=0.23), target)),
            ("NEUTRAL LIT", lit),
        ],
        [
            "The color families come from sunlit stone interiors on the selected church; exposure is normalized and no source pixels enter runtime maps.",
            "Every block traverses twenty related values internally; a whole ashlar is never one flat brown or yellow swatch.",
            "Micritic matrix, sandy blooms, and sparse iron color are separate deterministic layers instead of one generic sine-noise field.",
            "The photograph constrains relative warm/cool ordering only. It is not treated as absolute albedo, roughness, or baked lighting.",
        ],
    )
    body_page = compose_material_chapter(
        "THE ROCK BODY NOW HAS MEASURED RELIEF",
        "SIXTY-FOUR MILLIMETRES CARRIES A SCALE-BOUND CALCARENITE TOPOGRAPHY PROXY",
        [
            ("FOSSIL FRAGMENTS", gray_rgb(material["fossil_fragment_mask"])),
            ("SILICATE GRAINS", gray_rgb(material["silicate_grain_mask"])),
            ("VISIBLE PORES", gray_rgb(material["visible_pore_identity_mask"])),
            ("BODY HEIGHT", gray_rgb(normalized_range(material["stone_body_height_m"]))),
            ("BODY NORMAL", _normal_rgb(material["stone_body_normal"])),
            ("DETAIL BASE", material["stone_body_detail_base_color_srgb"]),
            ("DETAIL NEUTRAL LIT", detail_lit),
            ("DETAIL 2X2", proof_resize(_tile_2x2(detail_lit), target)),
        ],
        [
            "The Santa Marina study reports 30-40 percent fossil components and a non-degraded Naranjo sample with 15 percent quartz, 3 percent feldspar, and 2 percent clay.",
            "The separate Sabucina proxy measured a 3 by 1.5 millimetre calcarenite surface: 0.10 mm Sa, 0.13 mm Sq, and 1.13 mm peak-to-valley range.",
            "Its 0.8 mm filter boundary separates broad waviness from angular grain-scale relief. Sparse deeper valleys reproduce the measured negative height skew.",
            "The proxy does not authorize tool grooves, block-plane deformation, rounded arrises, recessed mortar, damage, or PBR roughness values.",
        ],
    )
    tile_page = compose_material_chapter(
        "FOUR METRES MUST SURVIVE REPETITION",
        "SEAMS, DISTANCE, AND OPPOSING LIGHT REVEAL FALSE QUALITY",
        [
            ("2X2 BASE COLOR", proof_resize(_tile_2x2(base), target)),
            ("2X2 NEUTRAL LIT", proof_resize(_tile_2x2(lit), target)),
            ("2X2 GRAZING", proof_resize(_tile_2x2(grazing), target)),
            ("SEAM FRAME", seam_frame(base)),
            ("FULL TILE", base),
            ("GAME DISTANCE", proof_resize(proof_resize(lit, 96), target)),
            ("OPPOSITE LIGHT", opposite),
            ("NORMAL", _normal_rgb(material["normal"])),
        ],
        [
            "The tile boundary completes half of a lime joint, so repetition forms the same joint width as the interior.",
            "Stable course dimensions matter more than disguising the tile with noise.",
            "Opposing lights test whether shape comes from normals instead of baked pigment.",
            "Compatible pattern phases and trim modules are separate future assets, not excuses for a weak core tile.",
        ],
    )
    return {
        "construction": construction,
        "planes": planes,
        "tooling": tool_page,
        "color": color_page,
        "body": body_page,
        "tiling": tile_page,
    }


def _hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _seam_metric(value: np.ndarray) -> float:
    return float(
        max(
            np.mean(np.abs(value[0] - value[-1])),
            np.mean(np.abs(value[:, 0] - value[:, -1])),
        )
    )


def _encode_height(value: np.ndarray) -> tuple[np.ndarray, tuple[float, float]]:
    low = float(value.min())
    high = float(value.max())
    encoded = (
        np.zeros_like(value)
        if high <= low
        else (value - low) / (high - low)
    )
    return encoded.astype(np.float32), (low, high)


def write_material_package(
    material: dict[str, Any],
    *,
    output_root: str | Path = DEFAULT_OUTPUT,
    pattern_path: str | Path = DEFAULT_PATTERN,
    profile_path: str | Path = DEFAULT_PROFILE,
) -> dict[str, Any]:
    output = Path(output_root)
    proofs = output / "proofs"
    output.mkdir(parents=True, exist_ok=True)
    proofs.mkdir(parents=True, exist_ok=True)
    files = {
        "basecolor": output / "cathedral_stone_v1_basecolor.png",
        "normal": output / "cathedral_stone_v1_normal.png",
        "orm": output / "cathedral_stone_v1_orm.png",
        "height": output / "cathedral_stone_v1_height.png",
        "identity_masks": output / "cathedral_stone_v1_identity_masks.png",
        "stone_body_masks": output / "cathedral_stone_v1_stone_body_masks.png",
        "stone_body_height": output / "cathedral_stone_v1_stone_body_height.png",
        "stone_body_normal": output / "cathedral_stone_v1_stone_body_normal.png",
        "tooling_detail": output / "cathedral_stone_v1_tooling_detail.png",
        "stylization": output / "cathedral_stone_v1_stylization.png",
        "brush_direction": output / "cathedral_stone_v1_brush_direction.png",
        "manifest": output / "cathedral_stone_v1_manifest.json",
    }
    height, height_range = _encode_height(material["height_m"])
    body_height, body_height_range = _encode_height(
        material["stone_body_height_m"]
    )
    orm = np.stack(
        [
            material["ao"],
            material["roughness"],
            material["metallic"],
        ],
        axis=-1,
    )
    identity_masks = np.stack(
        [
            material["mortar_mask"],
            material["tooling_identity_mask"],
            np.clip(
                1.0
                - smoothstep(
                    0.003,
                    0.025,
                    material["edge_distance_m"],
                ),
                0.0,
                1.0,
            )
            * material["stone_mask"],
        ],
        axis=-1,
    )
    stylization = np.stack(
        [
            material["ink_mask"],
            material["highlight_mask"],
            material["detail_priority"],
        ],
        axis=-1,
    )
    brush = np.concatenate(
        [
            material["brush_direction"] * 0.5 + 0.5,
            np.zeros((*material["brush_direction"].shape[:2], 1), dtype=np.float32),
        ],
        axis=-1,
    )
    write_png_rgb8(files["basecolor"], material["base_color_srgb"])
    write_png_rgb8(files["normal"], _normal_rgb(material["normal"]))
    write_png_rgb8(files["orm"], orm)
    write_png_gray16(files["height"], height)
    write_png_rgb8(files["identity_masks"], identity_masks)
    write_png_rgb8(files["stone_body_masks"], material["stone_body_masks"])
    write_png_gray16(files["stone_body_height"], body_height)
    write_png_rgb8(
        files["stone_body_normal"],
        _normal_rgb(material["stone_body_normal"]),
    )
    write_png_rgb8(files["tooling_detail"], material["tooling_detail_masks"])
    write_png_rgb8(files["stylization"], stylization)
    write_png_rgb8(files["brush_direction"], brush)

    proof_files: dict[str, Path] = {}
    for name, page in _proof_pages(material).items():
        path = proofs / f"cathedral_stone_v1_{name}.png"
        write_png_rgb8(path, page)
        proof_files[name] = path

    recipe = material["recipe"]
    capture_path = Path(material["reference_capture_path"])
    capture = material["reference_capture"]
    relief_proxy_path = Path(material["relief_proxy_path"])
    source_files = [
        Path(pattern_path),
        Path(profile_path),
        capture_path,
        relief_proxy_path,
    ]
    manifest = {
        "schema": MATERIAL_SCHEMA,
        "material_id": "cathedral_stone_v1",
        "resolution": material["resolution"],
        "seed": material["seed"],
        "pattern_variation": material["pattern_variation"],
        "tile_size_m": recipe.tile_size_m,
        "block_count": material["block_count"],
        "raster_block_occurrence_count": material[
            "raster_block_occurrence_count"
        ],
        "course_count": len(recipe.courses),
        "shade_family_size": material["shade_family_size"],
        "tooling_families": list(recipe.tooling),
        "edge_profiles": list(recipe.edge_profiles),
        "measurement_authority": recipe.measurement_authority,
        "bond": recipe.bond,
        "excluded_from_intact_core": list(recipe.excluded_from_intact_core),
        "height_range_m": list(height_range),
        "stone_body": {
            "measurement_id": capture["stone_body"]["measurement_id"],
            "detail_span_m": material["stone_body_detail_span_m"],
            "decorrelation_sample_span_m": BODY_DETAIL_DECORRELATION_SPAN_M,
            "meters_per_texel": (
                material["stone_body_detail_span_m"]
                / material["resolution"]
            ),
            "packed_channels": {
                "R": "fragmented calcareous fossil identity",
                "G": "silicate grain identity",
                "B": "visible pore identity contributing sparse proxy-bounded valleys",
            },
            "sampling_rule": (
                "Sample independently from the four-metre construction field "
                "using per-block phase, quarter-turn rotation, and mirror variation; "
                "blend 64 mm and incommensurate 91 mm samples to suppress lockstep repetition."
            ),
            "relief_proxy_measurement_id": material[
                "stone_body_relief_proxy_measurement_id"
            ],
            "relief_proxy_scope": (
                "comparable yellow coarse calcarenite; not a Santa Marina, "
                "Naranjo, or medieval dressed-face scan"
            ),
            "height_range_m": list(body_height_range),
            "height_amplitude_m": float(
                body_height_range[1] - body_height_range[0]
            ),
            "relief_metrics": material["stone_body_relief_metrics"],
            "roughness_cutoff_m": float(
                material["relief_proxy"]["roughness_cutoff_m"]
            ),
            "excluded_relief_lanes": [
                "block plane",
                "tool mark",
                "arris",
                "mortar",
                "damage",
                "weathering",
            ],
            "optical_roughness_status": capture["stone_body"][
                "optical_roughness_status"
            ],
        },
        "tooling_finish": {
            "finish_measurement_id": material["tooling_finish"]["id"],
            "spacing_measurement_id": recipe.measurement_authority[
                "tooling_measurement_id"
            ],
            "physical_span_m": material["tooling_detail_span_m"],
            "meters_per_texel": (
                material["tooling_detail_span_m"]
                / material["resolution"]
            ),
            "packed_channels": {
                "R": "720 visible impacts per square metre",
                "G": "1369 visible impacts per square metre",
                "B": "2420 visible impacts per square metre",
            },
            "stroke_length_range_m": [0.06, 0.094],
            "stroke_width_range_m": [0.001, 0.002],
            "intra_pass_spacing_range_m": [0.001, 0.004],
            "organization": (
                "finite oblique parallel passes with slight fan variation; "
                "one density channel, explicit angle, and explicit phase per block"
            ),
            "depth_status": "unknown; color and roughness linework only",
            "metrics": material["tooling_detail_metrics"],
            "transfer_limit": material["tooling_finish"]["use_limit"],
        },
        "pbr_contract": {
            "shading_model": "opaque default-lit dielectric",
            "metallic": 0.0,
            "specular": 0.5,
            "base_color_contains_baked_lighting": False,
            "normal_and_height": (
                "64 mm stone-body relief uses the Sabucina metrology proxy; "
                "construction, tooling, arris, mortar, and damage depth remain neutral. "
                "Tooling marks affect only authored color and roughness."
            ),
            "roughness_status": (
                "authored calibration under neutral and grazing light"
            ),
            "stone_body_ao": 1.0,
        },
        "default_overlays": {
            "fracture": 0.0,
            "lichen": 0.0,
            "damp": 0.0,
            "soot": 0.0,
            "traversal_tint": 0.0,
        },
        "constraints": {
            "uses_twenty_shade_block_families": True,
            "uses_authored_graphic_plane_values": True,
            "ink_highlight_are_separate_packed_lanes": True,
            "stone_body_detail_requires_runtime_distance_fade": True,
            "tooling_detail_requires_runtime_distance_fade": True,
            "unmeasured_arris_radius_authored": False,
            "unmeasured_tool_depth_authored": False,
            "unreal_runtime_parity_verified": False,
        },
        "reference_capture": {
            "path": str(capture_path),
            "sha256": _hash_file(capture_path),
            "asset_id": capture["source"]["asset_id"],
            "license": capture["source"]["license"],
            "attribution": capture["source"]["attribution"],
            "source_image_sha256": capture["source"]["image_sha256"],
            "raw_pixels_used_as_runtime_texture": False,
        },
        "metrics": {
            "basecolor_seam": _seam_metric(material["base_color_srgb"]),
            "height_seam": _seam_metric(material["height_m"]),
            "ink_coverage": float(
                (
                    material["ink_mask"][material["stone_mask"] > 0.5]
                    > 0.12
                ).mean()
            ),
            "highlight_coverage": float(
                (
                    material["highlight_mask"][material["stone_mask"] > 0.5]
                    > 0.10
                ).mean()
            ),
            "mortar_fraction": float(material["mortar_mask"].mean()),
            "fossil_identity_mean": float(
                material["fossil_fragment_mask"].mean()
            ),
            "silicate_identity_mean": float(
                material["silicate_grain_mask"].mean()
            ),
            "visible_pore_identity_mean": float(
                material["visible_pore_identity_mask"].mean()
            ),
            "tooling_detail_coverage_over_0_08": material[
                "tooling_detail_metrics"
            ]["coverage_over_0_08"],
            "stone_body_height_range_m": [
                float(material["stone_body_height_m"].min()),
                float(material["stone_body_height_m"].max()),
            ],
            "stone_body_relief": material["stone_body_relief_metrics"],
            "stone_body_normal_xy_max_abs": float(
                np.max(np.abs(material["stone_body_normal"][..., :2]))
            ),
            "stone_body_ao_range": [
                float(material["ao"].min()),
                float(material["ao"].max()),
            ],
            "normal_xy_max_abs": float(
                np.max(np.abs(material["normal"][..., :2]))
            ),
            "roughness_range": [
                float(material["roughness"].min()),
                float(material["roughness"].max()),
            ],
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
            for name, path in {
                **files,
                **{
                    f"proof_{name}": path
                    for name, path in proof_files.items()
                },
            }.items()
            if name != "manifest"
        },
        "source_policy": {
            "ai_generated_reference_capture": False,
            "photogrammetry_runtime_texture": False,
            "raw_reference_pixels_used_as_runtime_texture": False,
            "licensed_facade_color_capture": True,
            "facade_capture_license": capture["source"]["license"],
            "facade_capture_attribution": capture["source"]["attribution"],
            "measured_relief_proxy": True,
            "relief_proxy_measurement_id": material[
                "stone_body_relief_proxy_measurement_id"
            ],
            "relief_proxy_is_exact_lithology": False,
        },
    }
    files["manifest"].write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    )
    return manifest


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate the cathedral ashlar stone material core."
    )
    parser.add_argument("--resolution", type=int, default=1024)
    parser.add_argument("--seed", type=int, default=823451)
    parser.add_argument("--pattern-variation", type=int, default=0)
    parser.add_argument("--pattern", type=Path, default=DEFAULT_PATTERN)
    parser.add_argument("--profile", type=Path, default=DEFAULT_PROFILE)
    parser.add_argument("--capture", type=Path, default=DEFAULT_CAPTURE)
    parser.add_argument("--ledger", type=Path, default=DEFAULT_LEDGER)
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
        capture_path=args.capture,
        ledger_path=args.ledger,
    )
    manifest = write_material_package(
        material,
        output_root=args.output_root,
        pattern_path=args.pattern,
        profile_path=args.profile,
    )
    print(
        "cathedral_stone_v1 generated:",
        manifest["resolution"],
        "px,",
        manifest["block_count"],
        "blocks,",
        len(manifest["outputs"]),
        "outputs",
    )


if __name__ == "__main__":
    main()
