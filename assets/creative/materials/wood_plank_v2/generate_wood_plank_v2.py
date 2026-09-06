"""Generate a seamless, authored wood-plank material and pattern proofs."""

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
    draw_text,
    gray_rgb,
    height_to_normal,
    linear_to_srgb,
    mix,
    normalized_range,
    palette_panel,
    periodic_fbm,
    periodic_fbm_rect,
    periodic_gaussian_blur,
    profile_panel,
    proof_resize,
    render_material_preview,
    render_material_single_light,
    scalar_tint,
    seam_frame,
    signed_delta_rgb,
    smoothstep,
    srgb_to_linear,
    write_png_gray16,
    write_png_rgb8,
    zoom_square,
)


MATERIAL_SCHEMA = "iggy3d.material.wood_plank_v2.v1"
PATTERN_SCHEMA = "iggy3d.pattern.aged_oak.v1"
DEFAULT_SEED = 73129
DEFAULT_RESOLUTION = 1024
DEFAULT_TILE_SIZE_M = 1.6
DEFAULT_PATTERN_VARIATION = 0
DEFAULT_PATTERN_RECIPE_PATH = (
    Path(__file__).resolve().parent
    / "patterns"
    / "aged_oak_champion_v1.json"
)

WOOD_PALETTE_SRGB = (
    (102, 68, 42),
    (124, 84, 50),
    (144, 102, 61),
    (164, 119, 73),
    (181, 140, 92),
    (136, 98, 63),
)
GAP_SRGB = (43, 31, 23)
INK_SRGB = (53, 32, 20)
WOOD_SHADE_FAMILY_SIZE = 20


class PlankRecipe:
    def __init__(
        self,
        *,
        schema: str,
        name: str,
        description: str,
        tile_size_m: float,
        variation_seed: int,
        layout: dict[str, float],
        variation: dict[str, float],
        boards: list[dict[str, Any]],
        source_path: str,
    ) -> None:
        self.schema = schema
        self.name = name
        self.description = description
        self.tile_size_m = tile_size_m
        self.variation_seed = variation_seed
        self.layout = layout
        self.variation = variation
        self.boards = boards
        self.source_path = source_path


def load_plank_recipe(path: str | Path) -> PlankRecipe:
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
    tile_size_m = float(payload.get("tile_size_m", 0.0))
    if tile_size_m <= 0.0:
        raise ValueError(f"{source_path} tile_size_m must be positive")

    layout_payload = payload.get("layout")
    variation_payload = payload.get("variation")
    boards_payload = payload.get("boards")
    if not isinstance(layout_payload, dict):
        raise ValueError(f"{source_path} is missing layout settings")
    if not isinstance(variation_payload, dict):
        raise ValueError(f"{source_path} is missing variation settings")
    if not isinstance(boards_payload, list) or len(boards_payload) < 2:
        raise ValueError(f"{source_path} requires at least two boards")

    layout = {
        "gap_half_width_m": float(
            layout_payload.get("gap_half_width_m", 0.0012)
        ),
        "bevel_width_m": float(layout_payload.get("bevel_width_m", 0.0018)),
        "edge_wobble_m": float(layout_payload.get("edge_wobble_m", 0.0)),
    }
    if layout["gap_half_width_m"] <= 0.0:
        raise ValueError("plank gap_half_width_m must be positive")
    if layout["bevel_width_m"] <= 0.0:
        raise ValueError("plank bevel_width_m must be positive")
    if not 0.0 <= layout["edge_wobble_m"] <= 0.01:
        raise ValueError("plank edge_wobble_m must be between 0 and 0.01")

    variation = {
        "width_jitter_m": float(
            variation_payload.get("width_jitter_m", 0.0)
        ),
        "joint_jitter": float(variation_payload.get("joint_jitter", 0.0)),
    }
    if not 0.0 <= variation["width_jitter_m"] <= 0.02:
        raise ValueError("plank width_jitter_m must be between 0 and 0.02")
    if not 0.0 <= variation["joint_jitter"] <= 0.1:
        raise ValueError("plank joint_jitter must be between 0 and 0.1")

    identifiers: set[str] = set()
    boards: list[dict[str, Any]] = []
    for index, board_payload in enumerate(boards_payload):
        if not isinstance(board_payload, dict):
            raise ValueError(f"plank board {index} must be an object")
        identifier = str(board_payload.get("id", "")).strip()
        if not identifier or identifier in identifiers:
            raise ValueError(
                f"plank board {index} has a missing or duplicate id"
            )
        intent = str(board_payload.get("intent", "")).strip()
        width_m = float(board_payload.get("width_m", 0.0))
        joints = np.asarray(board_payload.get("joints", []), dtype=np.float32)
        if not intent:
            raise ValueError(f"plank board {identifier!r} is missing intent")
        if not 0.075 <= width_m <= 0.35:
            raise ValueError(
                f"plank board {identifier!r} width_m must be 0.075–0.35"
            )
        if joints.shape != (2,) or not (0.0 < joints[0] < joints[1] < 1.0):
            raise ValueError(
                f"plank board {identifier!r} needs two ordered internal joints"
            )
        span = float(joints[1] - joints[0])
        if span <= 0.30 or 1.0 - span <= 0.30:
            raise ValueError(
                f"plank board {identifier!r} creates an undersized segment"
            )
        pith_offset_widths = float(
            board_payload.get("pith_offset_widths", 0.0)
        )
        pith_depth_widths = float(
            board_payload.get("pith_depth_widths", 0.0)
        )
        cathedral_apex_v = np.asarray(
            board_payload.get("cathedral_apex_v", []),
            dtype=np.float32,
        )
        cathedral_aspect = float(board_payload.get("cathedral_aspect", 0.0))
        ring_mean_width_m = (
            float(board_payload.get("ring_mean_width_mm", 0.0)) / 1000.0
        )
        ring_epoch_strength = float(
            board_payload.get("ring_epoch_strength", -1.0)
        )
        ring_epoch_period = float(
            board_payload.get("ring_epoch_period", 0.0)
        )
        saw_angle_deg = float(board_payload.get("saw_angle_deg", 99.0))
        pith_drift_amplitude_m = (
            float(board_payload.get("pith_drift_amplitude_mm", -1.0))
            / 1000.0
        )
        pith_drift_period_m = float(
            board_payload.get("pith_drift_period_m", 0.0)
        )
        bark_side = int(board_payload.get("bark_side", 0))
        if not -2.0 <= pith_offset_widths <= 2.0:
            raise ValueError(
                f"plank board {identifier!r} pith offset is out of range"
            )
        if not 0.25 <= pith_depth_widths <= 2.5:
            raise ValueError(
                f"plank board {identifier!r} pith depth is out of range"
            )
        if cathedral_apex_v.shape != (2,):
            raise ValueError(
                f"plank board {identifier!r} needs two cathedral apex values"
            )
        if not 0.12 <= cathedral_aspect <= 0.65:
            raise ValueError(
                f"plank board {identifier!r} cathedral aspect is out of range"
            )
        if not 0.002 <= ring_mean_width_m <= 0.015:
            raise ValueError(
                f"plank board {identifier!r} ring width is out of range"
            )
        if not 0.0 <= ring_epoch_strength <= 0.4:
            raise ValueError(
                f"plank board {identifier!r} ring epoch strength is out of range"
            )
        if ring_epoch_period < 4.0:
            raise ValueError(
                f"plank board {identifier!r} ring epoch period is too short"
            )
        if not -10.0 <= saw_angle_deg <= 10.0:
            raise ValueError(
                f"plank board {identifier!r} saw angle is out of range"
            )
        if not 0.0 <= pith_drift_amplitude_m <= 0.020:
            raise ValueError(
                f"plank board {identifier!r} pith drift amplitude is invalid"
            )
        if not 0.35 <= pith_drift_period_m <= 1.60:
            raise ValueError(
                f"plank board {identifier!r} pith drift period is invalid"
            )
        if bark_side not in {-1, 1}:
            raise ValueError(
                f"plank board {identifier!r} bark_side must be -1 or 1"
            )

        rest_regions_payload = board_payload.get("rest_regions", [])
        rest_regions: list[tuple[float, float]] = []
        if not isinstance(rest_regions_payload, list):
            raise ValueError(
                f"plank board {identifier!r} rest_regions must be a list"
            )
        for region_index, region_payload in enumerate(rest_regions_payload):
            region = np.asarray(region_payload, dtype=np.float32)
            if region.shape != (2,) or not (
                0.0 <= region[0] < region[1] <= 1.0
            ):
                raise ValueError(
                    f"plank board {identifier!r} rest region "
                    f"{region_index} is invalid"
                )
            rest_regions.append((float(region[0]), float(region[1])))

        knots_payload = board_payload.get("knots", [])
        if not isinstance(knots_payload, list):
            raise ValueError(
                f"plank board {identifier!r} knots must be a list"
            )
        knots: list[dict[str, Any]] = []
        characters = ["clear", "clear"]
        for knot_index, knot_payload in enumerate(knots_payload):
            if not isinstance(knot_payload, dict):
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "must be an object"
                )
            knot_intent = str(knot_payload.get("intent", "")).strip()
            segment = int(knot_payload.get("segment", -1))
            center_u = float(knot_payload.get("center_u", -1.0))
            center_v = float(knot_payload.get("center_v", -1.0))
            diameter_m = float(knot_payload.get("diameter_m", 0.0))
            aspect_ratio = float(knot_payload.get("aspect_ratio", 0.0))
            angle_deg = float(knot_payload.get("angle_deg", 0.0))
            influence_ratio = float(
                knot_payload.get("influence_ratio", 0.0)
            )
            warp_strength = float(knot_payload.get("warp_strength", 0.0))
            ring_count = int(knot_payload.get("ring_count", 0))
            if not knot_intent:
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "is missing intent"
                )
            if segment not in {0, 1}:
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "needs segment 0 or 1"
                )
            if not 0.08 <= center_u <= 0.92 or not 0.08 <= center_v <= 0.92:
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "centre is outside the safe authored area"
                )
            if not 0.003 <= diameter_m <= min(width_m * 0.72, 0.075):
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "diameter is out of range"
                )
            if not 1.0 <= aspect_ratio <= 2.6:
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "aspect ratio is out of range"
                )
            if not -45.0 <= angle_deg <= 45.0:
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "angle is out of range"
                )
            if not 1.5 <= influence_ratio <= 4.5:
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "influence ratio is out of range"
                )
            if not 0.1 <= warp_strength <= 1.5:
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "warp strength is out of range"
                )
            if not 2 <= ring_count <= 6:
                raise ValueError(
                    f"plank board {identifier!r} knot {knot_index} "
                    "ring count is out of range"
                )
            characters[segment] = "knot"
            knots.append(
                {
                    "intent": knot_intent,
                    "segment": segment,
                    "center_u": center_u,
                    "center_v": center_v,
                    "diameter_m": diameter_m,
                    "aspect_ratio": aspect_ratio,
                    "angle_deg": angle_deg,
                    "influence_ratio": influence_ratio,
                    "warp_strength": warp_strength,
                    "ring_count": ring_count,
                }
            )

        damage_payload = board_payload.get("damage", [])
        if not isinstance(damage_payload, list):
            raise ValueError(
                f"plank board {identifier!r} damage must be a list"
            )
        damage: list[dict[str, Any]] = []
        damage_identifiers: set[str] = set()
        for damage_index, record_payload in enumerate(damage_payload):
            if not isinstance(record_payload, dict):
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_index} "
                    "must be an object"
                )
            damage_id = str(record_payload.get("id", "")).strip()
            kind = str(record_payload.get("kind", "")).strip()
            segment = int(record_payload.get("segment", -1))
            source = str(record_payload.get("source", "")).strip()
            path = np.asarray(record_payload.get("path", []), dtype=np.float32)
            width_start_m = float(
                record_payload.get("width_start_m", 0.0)
            )
            width_end_m = float(record_payload.get("width_end_m", 0.0))
            depth_m = float(record_payload.get("depth_m", 0.0))
            lift_m = float(record_payload.get("lift_m", 0.0))
            if not damage_id or damage_id in damage_identifiers:
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_index} "
                    "has a missing or duplicate id"
                )
            if kind not in {
                "knot_check",
                "end_check",
                "edge_splinter",
                "scratch",
            }:
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_id!r} "
                    f"uses unsupported kind {kind!r}"
                )
            if segment not in {0, 1}:
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_id!r} "
                    "needs segment 0 or 1"
                )
            if not source:
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_id!r} "
                    "is missing a source"
                )
            if (
                path.ndim != 2
                or path.shape[1] != 2
                or not 2 <= len(path) <= 8
                or np.any(path < 0.0)
                or np.any(path > 1.0)
            ):
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_id!r} "
                    "needs two to eight normalized path points"
                )
            if not 0.0001 <= width_end_m <= width_start_m <= 0.005:
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_id!r} "
                    "uses invalid path widths"
                )
            if not 0.00001 <= depth_m <= 0.0015:
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_id!r} "
                    "uses invalid depth"
                )
            if not 0.0 <= lift_m <= 0.0005:
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_id!r} "
                    "uses invalid lift"
                )

            start_u, start_v = (float(path[0, 0]), float(path[0, 1]))
            if kind == "knot_check":
                matching_knots = [
                    knot
                    for knot in knots
                    if knot["intent"] == source and knot["segment"] == segment
                ]
                if len(matching_knots) != 1:
                    raise ValueError(
                        f"plank board {identifier!r} damage {damage_id!r} "
                        "must name one knot on the same segment"
                    )
                source_knot = matching_knots[0]
                knot_start_distance = math.hypot(
                    (start_u - source_knot["center_u"]) * width_m,
                    (
                        start_v - source_knot["center_v"]
                    ) * tile_size_m,
                )
                if knot_start_distance > source_knot["diameter_m"] * 0.55:
                    raise ValueError(
                        f"plank board {identifier!r} damage {damage_id!r} "
                        "does not begin inside its source knot"
                    )
            elif kind == "end_check" and not (
                start_v <= 0.04 or start_v >= 0.96
            ):
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_id!r} "
                    "must begin at a segment end"
                )
            elif kind == "edge_splinter" and not (
                start_u <= 0.04 or start_u >= 0.96
            ):
                raise ValueError(
                    f"plank board {identifier!r} damage {damage_id!r} "
                    "must begin at a long edge"
                )

            damage_identifiers.add(damage_id)
            damage.append(
                {
                    "id": damage_id,
                    "kind": kind,
                    "segment": segment,
                    "source": source,
                    "path": path,
                    "width_start_m": width_start_m,
                    "width_end_m": width_end_m,
                    "depth_m": depth_m,
                    "lift_m": lift_m,
                }
            )

        identifiers.add(identifier)
        boards.append(
            {
                "id": identifier,
                "intent": intent,
                "width_m": width_m,
                "width": width_m / tile_size_m,
                "joints": joints,
                "characters": tuple(characters),
                "pith_offset_widths": pith_offset_widths,
                "pith_depth_widths": pith_depth_widths,
                "cathedral_apex_v": cathedral_apex_v,
                "cathedral_aspect": cathedral_aspect,
                "ring_mean_width_m": ring_mean_width_m,
                "ring_epoch_strength": ring_epoch_strength,
                "ring_epoch_period": ring_epoch_period,
                "saw_angle_deg": saw_angle_deg,
                "pith_drift_amplitude_m": pith_drift_amplitude_m,
                "pith_drift_period_m": pith_drift_period_m,
                "bark_side": bark_side,
                "rest_regions": tuple(rest_regions),
                "knots": tuple(knots),
                "damage": tuple(damage),
            }
        )

    width_sum_m = sum(board["width_m"] for board in boards)
    if not math.isclose(width_sum_m, tile_size_m, abs_tol=1.0e-6):
        raise ValueError(
            f"{source_path} board widths sum to {width_sum_m}, "
            f"expected tile size {tile_size_m}"
        )
    return PlankRecipe(
        schema=schema,
        name=name,
        description=str(payload.get("description", "")),
        tile_size_m=tile_size_m,
        variation_seed=int(payload.get("variation_seed", 0)),
        layout=layout,
        variation=variation,
        boards=boards,
        source_path=str(source_path),
    )


def generate_plank_layout(
    recipe: PlankRecipe,
    *,
    variation: int,
) -> dict[str, Any]:
    if variation < 0:
        raise ValueError("plank variation cannot be negative")
    widths = np.asarray(
        [board["width"] for board in recipe.boards], dtype=np.float32
    )
    joints = np.asarray(
        [board["joints"] for board in recipe.boards], dtype=np.float32
    ).copy()
    if variation > 0:
        rng = np.random.default_rng(
            recipe.variation_seed + variation * 104729
        )
        widths += rng.uniform(
            -recipe.variation["width_jitter_m"] / recipe.tile_size_m,
            recipe.variation["width_jitter_m"] / recipe.tile_size_m,
            size=widths.shape,
        )
        widths = np.maximum(widths, 0.09)
        widths /= widths.sum()
        joints += rng.uniform(
            -recipe.variation["joint_jitter"],
            recipe.variation["joint_jitter"],
            size=joints.shape,
        )
        joints.sort(axis=1)

    spans = joints[:, 1] - joints[:, 0]
    if np.any(spans <= 0.30) or np.any((1.0 - spans) <= 0.30):
        raise ValueError("plank variation created an undersized board segment")
    return {
        "schema": recipe.schema,
        "name": recipe.name,
        "source_path": recipe.source_path,
        "variation": variation,
        "tile_size_m": recipe.tile_size_m,
        "board_ids": tuple(board["id"] for board in recipe.boards),
        "board_intents": tuple(board["intent"] for board in recipe.boards),
        "characters": tuple(
            character
            for board in recipe.boards
            for character in board["characters"]
        ),
        "widths": widths.astype(np.float32),
        "joints": joints.astype(np.float32),
        "layout": dict(recipe.layout),
        "boards": tuple(recipe.boards),
        "field_seed": recipe.variation_seed + variation * 104729,
    }


def rasterize_plank_pattern(
    layout: dict[str, Any],
    *,
    resolution: int,
    tile_size_m: float,
) -> dict[str, np.ndarray]:
    if resolution <= 0 or tile_size_m <= 0.0:
        raise ValueError("plank resolution and tile size must be positive")
    axis = np.arange(resolution, dtype=np.float32) / resolution
    u = np.broadcast_to(axis[np.newaxis, :], (resolution, resolution))
    v = np.broadcast_to(axis[:, np.newaxis], (resolution, resolution))
    widths = np.asarray(layout["widths"], dtype=np.float32)
    joints = np.asarray(layout["joints"], dtype=np.float32)
    board_count = len(widths)

    internal_boundaries = np.cumsum(widths)[:-1]
    boundary_curves = np.empty(
        (board_count - 1, resolution), dtype=np.float32
    )
    edge_wobble = (
        float(layout["layout"]["edge_wobble_m"]) / tile_size_m
    )
    for boundary_index, boundary in enumerate(internal_boundaries):
        field = periodic_fbm(
            resolution,
            2 + boundary_index % 2,
            layout["field_seed"] + 313 + boundary_index * 197,
            octaves=3,
        )
        wobble = field[:, (boundary_index * 37) % resolution] - 0.5
        boundary_curves[boundary_index] = boundary + wobble * edge_wobble

    board_id = np.zeros((resolution, resolution), dtype=np.int16)
    for boundary_curve in boundary_curves:
        board_id += (u >= boundary_curve[:, np.newaxis]).astype(np.int16)

    left_curves = np.vstack(
        [
            np.zeros((1, resolution), dtype=np.float32),
            boundary_curves,
        ]
    )
    right_curves = np.vstack(
        [
            boundary_curves,
            np.ones((1, resolution), dtype=np.float32),
        ]
    )
    local_u = np.zeros_like(u, dtype=np.float32)
    local_v = np.zeros_like(v, dtype=np.float32)
    segment_id = np.zeros_like(board_id)
    board_edge_distance = np.zeros_like(u, dtype=np.float32)
    joint_edge_distance = np.zeros_like(v, dtype=np.float32)

    for board_index in range(board_count):
        board_pixels = board_id == board_index
        left = left_curves[board_index][:, np.newaxis]
        right = right_curves[board_index][:, np.newaxis]
        width = np.maximum(right - left, 1.0e-5)
        board_local_u = np.clip((u - left) / width, 0.0, 1.0)
        local_u = np.where(board_pixels, board_local_u, local_u)
        horizontal_distance = np.minimum(u - left, right - u)
        board_edge_distance = np.where(
            board_pixels,
            np.maximum(horizontal_distance, 0.0) * tile_size_m,
            board_edge_distance,
        )

        joint_a, joint_b = joints[board_index]
        between = np.logical_and(v >= joint_a, v < joint_b)
        segment_a = board_index * 2
        segment_b = segment_a + 1
        board_segment = np.where(between, segment_a, segment_b)
        segment_id = np.where(board_pixels, board_segment, segment_id)

        span_a = joint_b - joint_a
        span_b = 1.0 - span_a
        board_local_v = np.where(
            between,
            (v - joint_a) / span_a,
            np.mod(v - joint_b, 1.0) / span_b,
        )
        local_v = np.where(board_pixels, board_local_v, local_v)
        delta_a = np.abs(v - joint_a)
        delta_a = np.minimum(delta_a, 1.0 - delta_a)
        delta_b = np.abs(v - joint_b)
        delta_b = np.minimum(delta_b, 1.0 - delta_b)
        joint_distance = np.minimum(delta_a, delta_b) * tile_size_m
        joint_edge_distance = np.where(
            board_pixels,
            joint_distance,
            joint_edge_distance,
        )

    return {
        "tile_u": u,
        "tile_v": v,
        "board_id": board_id,
        "segment_id": segment_id,
        "local_u": local_u,
        "local_v": local_v,
        "board_edge_distance": board_edge_distance,
        "joint_edge_distance": joint_edge_distance,
        "edge_distance": np.minimum(
            board_edge_distance, joint_edge_distance
        ).astype(np.float32),
    }


def branch_intersection_node(
    raster: dict[str, np.ndarray],
    layout: dict[str, Any],
    *,
    tile_size_m: float,
) -> dict[str, np.ndarray]:
    """Intersect authored branches and produce a stable knot coordinate warp."""
    board_id = raster["board_id"]
    segment_id = raster["segment_id"]
    board_widths_m = np.asarray(layout["widths"], dtype=np.float32) * tile_size_m
    segment_lengths_m = _segment_lengths_m(layout, tile_size_m)
    local_t_m = (
        (raster["local_u"] - 0.5) * board_widths_m[board_id]
    ).astype(np.float32)
    local_s_m = (
        (raster["local_v"] - 0.5) * segment_lengths_m[segment_id]
    ).astype(np.float32)
    warped_t_m = local_t_m.copy()
    warped_s_m = local_s_m.copy()
    knot_mask = np.zeros_like(local_t_m, dtype=np.float32)
    knot_core_mask = np.zeros_like(local_t_m, dtype=np.float32)
    knot_ring_mask = np.zeros_like(local_t_m, dtype=np.float32)
    knot_boundary_mask = np.zeros_like(local_t_m, dtype=np.float32)
    knot_influence = np.zeros_like(local_t_m, dtype=np.float32)
    warp_magnitude_m = np.zeros_like(local_t_m, dtype=np.float32)

    for board_index, board in enumerate(layout["boards"]):
        board_width_m = float(board_widths_m[board_index])
        for knot in board["knots"]:
            owned_segment = board_index * 2 + int(knot["segment"])
            owner = (segment_id == owned_segment).astype(np.float32)
            segment_length_m = float(segment_lengths_m[owned_segment])
            center_t_m = (float(knot["center_u"]) - 0.5) * board_width_m
            center_s_m = (
                float(knot["center_v"]) - 0.5
            ) * segment_length_m
            delta_t = local_t_m - center_t_m
            delta_s = local_s_m - center_s_m
            angle = math.radians(float(knot["angle_deg"]))
            cosine = math.cos(angle)
            sine = math.sin(angle)
            knot_x = cosine * delta_t + sine * delta_s
            knot_y = -sine * delta_t + cosine * delta_s
            radius_x = float(knot["diameter_m"]) * 0.5
            radius_y = radius_x * float(knot["aspect_ratio"])
            radius = np.sqrt(
                (knot_x / max(radius_x, 1.0e-6)) ** 2
                + (knot_y / max(radius_y, 1.0e-6)) ** 2
            )
            body = (
                1.0 - smoothstep(0.72, 1.0, radius)
            ) * owner
            core = (
                1.0 - smoothstep(0.04, 0.34, radius)
            ) * owner
            ring_window = (
                smoothstep(0.22, 0.38, radius)
                * (1.0 - smoothstep(0.82, 0.98, radius))
                * owner
            )
            ring_wave = 0.5 + 0.5 * np.cos(
                2.0
                * math.pi
                * (
                    radius * float(knot["ring_count"])
                    + board_index * 0.173
                )
            )
            rings = smoothstep(0.58, 0.92, ring_wave) * ring_window
            boundary = (
                1.0
                - smoothstep(
                    0.025,
                    0.13,
                    np.abs(radius - 0.84),
                )
            ) * owner
            influence_ratio = float(knot["influence_ratio"])
            influence = (
                1.0 - smoothstep(0.92, influence_ratio, radius)
            ) * owner

            across_falloff = np.exp(
                -(
                    knot_x
                    / max(radius_x * influence_ratio * 0.72, 1.0e-6)
                )
                ** 2
            )
            along_falloff = np.exp(
                -(
                    knot_y
                    / max(radius_y * influence_ratio * 0.78, 1.0e-6)
                )
                ** 2
            )
            smooth_side = np.tanh(knot_x / max(radius_x * 0.24, 1.0e-6))
            asymmetry = 1.0 + 0.20 * np.tanh(
                knot_y / max(radius_y, 1.0e-6)
            )
            warp_strength = float(knot["warp_strength"])
            displacement_x = (
                smooth_side
                * radius_x
                * warp_strength
                * influence
                * across_falloff
                * along_falloff
                * asymmetry
            )
            displacement_y = (
                -knot_y
                * 0.10
                * warp_strength
                * influence
                * along_falloff
            )
            displacement_t = (
                cosine * displacement_x - sine * displacement_y
            )
            displacement_s = (
                sine * displacement_x + cosine * displacement_y
            )
            warped_t_m += displacement_t.astype(np.float32)
            warped_s_m += displacement_s.astype(np.float32)
            knot_mask = np.maximum(knot_mask, body).astype(np.float32)
            knot_core_mask = np.maximum(knot_core_mask, core).astype(
                np.float32
            )
            knot_ring_mask = np.maximum(knot_ring_mask, rings).astype(
                np.float32
            )
            knot_boundary_mask = np.maximum(
                knot_boundary_mask, boundary
            ).astype(np.float32)
            knot_influence = np.maximum(knot_influence, influence).astype(
                np.float32
            )
            warp_magnitude_m += np.hypot(
                displacement_t, displacement_s
            ).astype(np.float32)

    return {
        "local_t_m": local_t_m,
        "local_s_m": local_s_m,
        "warped_t_m": warped_t_m,
        "warped_s_m": warped_s_m,
        "knot_mask": knot_mask,
        "knot_core_mask": knot_core_mask,
        "knot_ring_mask": knot_ring_mask,
        "knot_boundary_mask": knot_boundary_mask,
        "knot_influence": knot_influence,
        "knot_warp_magnitude_m": warp_magnitude_m,
    }


def virtual_log_ring_node(
    raster: dict[str, np.ndarray],
    layout: dict[str, Any],
    branch: dict[str, np.ndarray],
    *,
    tile_size_m: float,
    board_mask: np.ndarray,
) -> dict[str, np.ndarray]:
    """Project a simplified virtual log through each authored board face."""
    board_id = raster["board_id"]
    segment_id = raster["segment_id"]
    board_widths_m = np.asarray(layout["widths"], dtype=np.float32) * tile_size_m
    segment_lengths_m = _segment_lengths_m(layout, tile_size_m)
    boards = layout["boards"]

    pith_offset_m = np.asarray(
        [
            float(board["pith_offset_widths"]) * board_widths_m[index]
            for index, board in enumerate(boards)
        ],
        dtype=np.float32,
    )
    pith_depth_m = np.asarray(
        [
            float(board["pith_depth_widths"]) * board_widths_m[index]
            for index, board in enumerate(boards)
        ],
        dtype=np.float32,
    )
    cathedral_aspect = np.asarray(
        [float(board["cathedral_aspect"]) for board in boards],
        dtype=np.float32,
    )
    ring_mean_width_m = np.asarray(
        [float(board["ring_mean_width_m"]) for board in boards],
        dtype=np.float32,
    )
    epoch_strength = np.asarray(
        [float(board["ring_epoch_strength"]) for board in boards],
        dtype=np.float32,
    )
    epoch_period = np.asarray(
        [float(board["ring_epoch_period"]) for board in boards],
        dtype=np.float32,
    )
    saw_angle_rad = np.radians(
        np.asarray(
            [float(board["saw_angle_deg"]) for board in boards],
            dtype=np.float32,
        )
    )
    pith_drift_amplitude_m = np.asarray(
        [float(board["pith_drift_amplitude_m"]) for board in boards],
        dtype=np.float32,
    )
    pith_drift_period_m = np.asarray(
        [float(board["pith_drift_period_m"]) for board in boards],
        dtype=np.float32,
    )
    segment_apex_m = np.empty(len(boards) * 2, dtype=np.float32)
    for board_index, board in enumerate(boards):
        for local_segment in range(2):
            owned_segment = board_index * 2 + local_segment
            segment_apex_m[owned_segment] = (
                float(board["cathedral_apex_v"][local_segment]) - 0.5
            ) * segment_lengths_m[owned_segment]

    def projected_fields(
        coordinate_t_m: np.ndarray,
        coordinate_s_m: np.ndarray,
    ) -> tuple[np.ndarray, np.ndarray]:
        pith_track_m = (
            pith_offset_m[board_id]
            + np.tan(saw_angle_rad[board_id]) * coordinate_s_m
            + pith_drift_amplitude_m[board_id]
            * np.sin(
                2.0
                * math.pi
                * (
                    coordinate_s_m / pith_drift_period_m[board_id]
                    + board_id * 0.173
                )
            )
        )
        across = coordinate_t_m - pith_track_m
        along = (
            coordinate_s_m - segment_apex_m[segment_id]
        ) * cathedral_aspect[board_id]
        radius_m = np.sqrt(
            across * across
            + along * along
            + pith_depth_m[board_id] * pith_depth_m[board_id]
        )
        base_phase = radius_m / ring_mean_width_m[board_id]
        primary_epoch = (
            epoch_strength[board_id]
            * np.sin(
                2.0
                * math.pi
                * (
                    base_phase / epoch_period[board_id]
                    + board_id * 0.137
                )
            )
        )
        secondary_epoch = (
            epoch_strength[board_id]
            * 0.34
            * np.sin(
                2.0
                * math.pi
                * (
                    base_phase / (epoch_period[board_id] * 2.37)
                    + board_id * 0.311
                )
            )
        )
        residual_epoch = (
            0.050
            * np.sin(
                2.0
                * math.pi
                * (base_phase / 3.71 + board_id * 0.193)
            )
            + 0.022
            * np.sin(
                2.0
                * math.pi
                * (base_phase / 1.83 + board_id * 0.347)
            )
        )
        phase_derivative = (
            1.0
            + epoch_strength[board_id]
            * (2.0 * math.pi / epoch_period[board_id])
            * np.cos(
                2.0
                * math.pi
                * (
                    base_phase / epoch_period[board_id]
                    + board_id * 0.137
                )
            )
            + epoch_strength[board_id]
            * 0.34
            * (2.0 * math.pi / (epoch_period[board_id] * 2.37))
            * np.cos(
                2.0
                * math.pi
                * (
                    base_phase / (epoch_period[board_id] * 2.37)
                    + board_id * 0.311
                )
            )
            + 0.050
            * (2.0 * math.pi / 3.71)
            * np.cos(
                2.0
                * math.pi
                * (base_phase / 3.71 + board_id * 0.193)
            )
            + 0.022
            * (2.0 * math.pi / 1.83)
            * np.cos(
                2.0
                * math.pi
                * (base_phase / 1.83 + board_id * 0.347)
            )
        )
        spacing_m = ring_mean_width_m[board_id] / np.clip(
            phase_derivative,
            0.40,
            2.20,
        )
        return (
            (
                base_phase
                + primary_epoch
                + secondary_epoch
                + residual_epoch
            ).astype(np.float32),
            spacing_m.astype(np.float32),
        )

    ring_phase_unwarped, _ = projected_fields(
        branch["local_t_m"],
        branch["local_s_m"],
    )
    ring_phase, ring_spacing_m = projected_fields(
        branch["warped_t_m"],
        branch["warped_s_m"],
    )
    ring_fraction_unwarped = np.mod(ring_phase_unwarped, 1.0).astype(
        np.float32
    )
    ring_fraction = np.mod(ring_phase, 1.0).astype(np.float32)
    earlywood_unwarped = (
        1.0 - smoothstep(0.04, 0.30, ring_fraction_unwarped)
    ) * board_mask
    earlywood = (
        1.0 - smoothstep(0.04, 0.30, ring_fraction)
    ) * board_mask
    latewood = (1.0 - earlywood) * board_mask

    gradient_v, gradient_u = np.gradient(ring_phase)
    flow_u = -gradient_v
    flow_v = gradient_u
    flow_length = np.maximum(np.hypot(flow_u, flow_v), 1.0e-8)
    growth_flow = np.stack(
        [flow_u / flow_length, flow_v / flow_length],
        axis=-1,
    ).astype(np.float32)

    return {
        "ring_phase_unwarped": ring_phase_unwarped,
        "ring_phase": ring_phase,
        "ring_fraction_unwarped": ring_fraction_unwarped,
        "ring_fraction": ring_fraction,
        "earlywood_unwarped": earlywood_unwarped.astype(np.float32),
        "earlywood_mask": earlywood.astype(np.float32),
        "latewood_mask": latewood.astype(np.float32),
        "growth_flow": growth_flow,
        "ring_spacing_m": ring_spacing_m,
        "ring_spacing_multiplier": (
            ring_spacing_m / ring_mean_width_m[board_id]
        ).astype(np.float32),
    }


def oak_microstructure_node(
    raster: dict[str, np.ndarray],
    layout: dict[str, Any],
    anatomy: dict[str, np.ndarray],
    branch: dict[str, np.ndarray],
    *,
    board_mask: np.ndarray,
    seed: int,
) -> dict[str, np.ndarray]:
    """Build ring-owned pores, cut-dependent rays, fibres, and quiet rests."""
    resolution = board_mask.shape[0]
    board_id = raster["board_id"]
    local_v = raster["local_v"]
    rest_mask = np.zeros_like(board_mask, dtype=np.float32)
    for board_index, board in enumerate(layout["boards"]):
        owner = (board_id == board_index).astype(np.float32)
        for start, end in board["rest_regions"]:
            feather = min(0.035, max((end - start) * 0.16, 0.012))
            region = (
                smoothstep(start - feather, start + feather, local_v)
                * (
                    1.0
                    - smoothstep(end - feather, end + feather, local_v)
                )
                * owner
            )
            rest_mask = np.maximum(rest_mask, region)
    rest_mask = (rest_mask * board_mask).astype(np.float32)
    detail_weight = np.clip(
        1.0 - rest_mask * 0.78,
        0.18,
        1.0,
    ).astype(np.float32)

    pith_offset = np.asarray(
        [
            abs(float(board["pith_offset_widths"]))
            for board in layout["boards"]
        ],
        dtype=np.float32,
    )
    pith_depth = np.asarray(
        [
            float(board["pith_depth_widths"])
            for board in layout["boards"]
        ],
        dtype=np.float32,
    )
    radial_face_ratio = pith_offset / np.maximum(
        pith_offset + pith_depth,
        1.0e-5,
    )
    ray_visibility = (
        0.16
        + 0.84
        * smoothstep(
            0.22,
            0.58,
            radial_face_ratio[board_id],
        )
    ) * board_mask
    ray_visibility = ray_visibility.astype(np.float32)

    earlywood_carrier = periodic_fbm_rect(
        resolution,
        236,
        108,
        seed + 1709,
        octaves=2,
        persistence=0.42,
    )
    earlywood_breaks = periodic_fbm_rect(
        resolution,
        54,
        176,
        seed + 1877,
        octaves=2,
        persistence=0.44,
    )
    earlywood_pore_mask = (
        anatomy["earlywood_mask"]
        * smoothstep(0.60, 0.82, earlywood_carrier)
        * smoothstep(0.40, 0.72, earlywood_breaks)
        * detail_weight
        * board_mask
    )
    earlywood_pore_mask = np.clip(
        earlywood_pore_mask,
        0.0,
        1.0,
    ).astype(np.float32)

    latewood_carrier = periodic_fbm_rect(
        resolution,
        318,
        164,
        seed + 1999,
        octaves=2,
        persistence=0.40,
    )
    latewood_pore_mask = (
        anatomy["latewood_mask"]
        * smoothstep(0.70, 0.90, latewood_carrier)
        * detail_weight
        * 0.48
        * board_mask
    )
    latewood_pore_mask = np.clip(
        latewood_pore_mask,
        0.0,
        1.0,
    ).astype(np.float32)
    vessel_mask = np.maximum(
        earlywood_pore_mask,
        latewood_pore_mask,
    ).astype(np.float32)

    ray_carrier = periodic_fbm_rect(
        resolution,
        58,
        264,
        seed + 2137,
        octaves=2,
        persistence=0.40,
    )
    ray_breaks = periodic_fbm_rect(
        resolution,
        94,
        72,
        seed + 2293,
        octaves=2,
        persistence=0.44,
    )
    ray_mask = (
        smoothstep(0.62, 0.84, ray_carrier)
        * smoothstep(0.44, 0.74, ray_breaks)
        * ray_visibility
        * (0.72 + 0.28 * detail_weight)
        * board_mask
    )
    ray_mask = np.clip(ray_mask, 0.0, 1.0).astype(np.float32)

    phase = anatomy["ring_phase"]
    fibre_wobble = (
        periodic_fbm_rect(
            resolution,
            11,
            23,
            seed + 2333,
            octaves=3,
            persistence=0.48,
        )
        - 0.5
    ) * 0.0022
    fibre_coordinate_m = branch["warped_t_m"] + fibre_wobble
    long_carrier = 0.5 + 0.5 * np.cos(
        2.0
        * math.pi
        * (fibre_coordinate_m / 0.0082 + seed * 0.000013)
    )
    middle_carrier = 0.5 + 0.5 * np.cos(
        2.0
        * math.pi
        * (fibre_coordinate_m / 0.0037 + seed * 0.000021)
    )
    fine_carrier = 0.5 + 0.5 * np.cos(
        2.0
        * math.pi
        * (fibre_coordinate_m / 0.0017 + seed * 0.000037)
    )
    long_breaks = periodic_fbm_rect(
        resolution,
        17,
        13,
        seed + 2371,
        octaves=2,
        persistence=0.48,
    )
    middle_breaks = periodic_fbm_rect(
        resolution,
        41,
        47,
        seed + 2531,
        octaves=2,
        persistence=0.47,
    )
    fine_breaks = periodic_fbm_rect(
        resolution,
        88,
        146,
        seed + 2689,
        octaves=2,
        persistence=0.45,
    )
    long_fibres = (
        smoothstep(0.88, 0.985, long_carrier)
        * smoothstep(0.42, 0.70, long_breaks)
    )
    middle_fibres = (
        smoothstep(0.91, 0.992, middle_carrier)
        * smoothstep(0.48, 0.74, middle_breaks)
        * 0.72
    )
    fine_fibres = (
        smoothstep(0.94, 0.996, fine_carrier)
        * smoothstep(0.54, 0.78, fine_breaks)
        * 0.48
    )
    fibre_bundle_mask = (
        np.maximum.reduce([long_fibres, middle_fibres, fine_fibres])
        * detail_weight
        * board_mask
    )
    fibre_bundle_mask = np.clip(
        fibre_bundle_mask,
        0.0,
        1.0,
    ).astype(np.float32)

    gradient_v, gradient_u = np.gradient(phase)
    ray_length = np.maximum(
        np.hypot(gradient_u, gradient_v),
        1.0e-8,
    )
    ray_direction = np.stack(
        [
            gradient_u / ray_length,
            gradient_v / ray_length,
        ],
        axis=-1,
    ).astype(np.float32)
    grain_activity = np.clip(
        (
            anatomy["earlywood_mask"] * 0.34
            + vessel_mask * 0.58
            + ray_mask * 0.42
            + fibre_bundle_mask * 0.46
        )
        * detail_weight
        * board_mask,
        0.0,
        1.0,
    ).astype(np.float32)
    return {
        "rest_mask": rest_mask,
        "ray_visibility": ray_visibility,
        "earlywood_pore_mask": earlywood_pore_mask,
        "latewood_pore_mask": latewood_pore_mask,
        "vessel_mask": vessel_mask,
        "ray_mask": ray_mask,
        "ray_direction": ray_direction,
        "fibre_bundle_mask": fibre_bundle_mask,
        "grain_activity": grain_activity,
    }


def authored_damage_node(
    raster: dict[str, np.ndarray],
    layout: dict[str, Any],
    *,
    tile_size_m: float,
) -> dict[str, np.ndarray]:
    """Rasterize typed authored damage and preserve its source habitat."""
    shape = raster["local_u"].shape
    meters_per_pixel = tile_size_m / shape[0]
    board_widths_m = np.asarray(layout["widths"], dtype=np.float32) * tile_size_m
    segment_lengths_m = _segment_lengths_m(layout, tile_size_m)
    knot_check_mask = np.zeros(shape, dtype=np.float32)
    end_check_mask = np.zeros(shape, dtype=np.float32)
    check_shoulder_mask = np.zeros(shape, dtype=np.float32)
    check_fibre_lip_mask = np.zeros(shape, dtype=np.float32)
    splinter_cut_mask = np.zeros(shape, dtype=np.float32)
    splinter_lip_mask = np.zeros(shape, dtype=np.float32)
    scratch_mask = np.zeros(shape, dtype=np.float32)
    finish_loss_mask = np.zeros(shape, dtype=np.float32)
    height_delta_m = np.zeros(shape, dtype=np.float32)

    def path_field(
        points: np.ndarray,
        *,
        owned_segment: int,
        board_width_m: float,
        segment_length_m: float,
        width_start_m: float,
        width_end_m: float,
        fade_start: bool,
    ) -> tuple[np.ndarray, np.ndarray]:
        coordinate_t = raster["local_u"] * board_width_m
        coordinate_s = raster["local_v"] * segment_length_m
        physical_points = np.stack(
            [
                points[:, 0] * board_width_m,
                points[:, 1] * segment_length_m,
            ],
            axis=-1,
        )
        edge_lengths = np.linalg.norm(
            np.diff(physical_points, axis=0),
            axis=1,
        )
        total_length = max(float(edge_lengths.sum()), 1.0e-6)
        best_distance = np.full(shape, np.inf, dtype=np.float32)
        best_progress = np.zeros(shape, dtype=np.float32)
        travelled = 0.0
        for edge_index, edge_length in enumerate(edge_lengths):
            start = physical_points[edge_index]
            end = physical_points[edge_index + 1]
            direction = end - start
            length_squared = max(float(np.dot(direction, direction)), 1.0e-12)
            projection = np.clip(
                (
                    (coordinate_t - start[0]) * direction[0]
                    + (coordinate_s - start[1]) * direction[1]
                )
                / length_squared,
                0.0,
                1.0,
            )
            closest_t = start[0] + projection * direction[0]
            closest_s = start[1] + projection * direction[1]
            distance = np.hypot(
                coordinate_t - closest_t,
                coordinate_s - closest_s,
            )
            closer = distance < best_distance
            best_distance = np.where(closer, distance, best_distance)
            progress = (
                travelled + projection * float(edge_length)
            ) / total_length
            best_progress = np.where(closer, progress, best_progress)
            travelled += float(edge_length)

        local_width = (
            width_start_m * (1.0 - best_progress)
            + width_end_m * best_progress
        )
        outer_width = np.maximum(local_width, meters_per_pixel * 0.82)
        inner_width = np.minimum(
            outer_width * 0.38,
            np.maximum(local_width * 0.22, meters_per_pixel * 0.10),
        )
        mask = 1.0 - smoothstep(
            inner_width,
            outer_width,
            best_distance,
        )
        mask *= 1.0 - smoothstep(0.82, 1.0, best_progress)
        if fade_start:
            mask *= smoothstep(0.0, 0.08, best_progress)
        mask *= (raster["segment_id"] == owned_segment).astype(np.float32)
        return mask.astype(np.float32), best_progress

    for board_index, board in enumerate(layout["boards"]):
        board_width_m = float(board_widths_m[board_index])
        for record in board["damage"]:
            owned_segment = board_index * 2 + int(record["segment"])
            segment_length_m = float(segment_lengths_m[owned_segment])
            mask, progress = path_field(
                record["path"],
                owned_segment=owned_segment,
                board_width_m=board_width_m,
                segment_length_m=segment_length_m,
                width_start_m=float(record["width_start_m"]),
                width_end_m=float(record["width_end_m"]),
                fade_start=record["kind"] == "scratch",
            )
            depth_profile = (
                float(record["depth_m"])
                * (1.0 - 0.68 * progress)
                * mask
            )
            kind = record["kind"]
            if kind == "knot_check":
                knot_check_mask = np.maximum(knot_check_mask, mask)
                finish_loss_mask = np.maximum(
                    finish_loss_mask, mask * 0.48
                )
            elif kind == "end_check":
                end_check_mask = np.maximum(end_check_mask, mask)
                finish_loss_mask = np.maximum(
                    finish_loss_mask, mask * 0.58
                )
            if kind in {"knot_check", "end_check"}:
                shoulder, _ = path_field(
                    record["path"],
                    owned_segment=owned_segment,
                    board_width_m=board_width_m,
                    segment_length_m=segment_length_m,
                    width_start_m=float(record["width_start_m"]) * 2.6,
                    width_end_m=float(record["width_end_m"]) * 2.2,
                    fade_start=False,
                )
                shoulder = np.clip(
                    shoulder - mask * 0.58,
                    0.0,
                    1.0,
                )
                check_shoulder_mask = np.maximum(
                    check_shoulder_mask, shoulder
                )
                fibre_points = np.asarray(
                    record["path"], dtype=np.float32
                ).copy()
                fibre_points[:, 0] += (
                    float(record["width_start_m"])
                    * 1.25
                    / board_width_m
                )
                fibre_points[:, 0] = np.clip(
                    fibre_points[:, 0], 0.0, 1.0
                )
                fibre_lip, fibre_progress = path_field(
                    fibre_points,
                    owned_segment=owned_segment,
                    board_width_m=board_width_m,
                    segment_length_m=segment_length_m,
                    width_start_m=float(record["width_start_m"]) * 0.52,
                    width_end_m=float(record["width_end_m"]) * 0.42,
                    fade_start=True,
                )
                fibre_pulse = smoothstep(
                    0.42,
                    0.78,
                    0.5
                    + 0.5
                    * np.sin(
                        2.0
                        * math.pi
                        * (fibre_progress * 2.7 + board_index * 0.19)
                    ),
                )
                fibre_lip *= fibre_pulse
                check_fibre_lip_mask = np.maximum(
                    check_fibre_lip_mask, fibre_lip
                )
                height_delta_m += (
                    fibre_lip
                    * min(float(record["depth_m"]) * 0.22, 0.00008)
                ).astype(np.float32)
            elif kind == "edge_splinter":
                splinter_cut_mask = np.maximum(splinter_cut_mask, mask)
                finish_loss_mask = np.maximum(
                    finish_loss_mask, mask * 0.88
                )
                lip_points = np.asarray(record["path"], dtype=np.float32).copy()
                edge_sign = (
                    1.0 if float(lip_points[0, 0]) < 0.5 else -1.0
                )
                lip_points[:, 0] += (
                    edge_sign
                    * float(record["width_start_m"])
                    * 1.35
                    / board_width_m
                )
                lip_mask, lip_progress = path_field(
                    lip_points,
                    owned_segment=owned_segment,
                    board_width_m=board_width_m,
                    segment_length_m=segment_length_m,
                    width_start_m=float(record["width_start_m"]) * 0.72,
                    width_end_m=float(record["width_end_m"]) * 0.62,
                    fade_start=False,
                )
                splinter_lip_mask = np.maximum(
                    splinter_lip_mask, lip_mask
                )
                height_delta_m += (
                    lip_mask
                    * float(record["lift_m"])
                    * (1.0 - 0.74 * lip_progress)
                ).astype(np.float32)
            else:
                scratch_group = mask.copy()
                for sibling_index, offset_factor in enumerate((-1.55, 1.85)):
                    sibling_points = np.asarray(
                        record["path"], dtype=np.float32
                    ).copy()
                    sibling_points[:, 0] += (
                        offset_factor
                        * float(record["width_start_m"])
                        / board_width_m
                    )
                    sibling_points[:, 0] = np.clip(
                        sibling_points[:, 0], 0.0, 1.0
                    )
                    sibling, sibling_progress = path_field(
                        sibling_points,
                        owned_segment=owned_segment,
                        board_width_m=board_width_m,
                        segment_length_m=segment_length_m,
                        width_start_m=(
                            float(record["width_start_m"])
                            * (0.58 - sibling_index * 0.10)
                        ),
                        width_end_m=float(record["width_end_m"]) * 0.55,
                        fade_start=True,
                    )
                    scratch_group = np.maximum(scratch_group, sibling)
                    height_delta_m -= (
                        sibling
                        * float(record["depth_m"])
                        * (0.46 - sibling_index * 0.08)
                        * (1.0 - 0.72 * sibling_progress)
                    ).astype(np.float32)
                scratch_mask = np.maximum(scratch_mask, scratch_group)
                finish_loss_mask = np.maximum(
                    finish_loss_mask, scratch_group * 0.92
                )
            height_delta_m -= depth_profile.astype(np.float32)

    check_mask = np.maximum(knot_check_mask, end_check_mask)
    damage_mask = np.maximum.reduce(
        [check_mask, splinter_cut_mask, scratch_mask]
    ).astype(np.float32)
    return {
        "knot_check_mask": knot_check_mask,
        "end_check_mask": end_check_mask,
        "check_mask": check_mask.astype(np.float32),
        "check_shoulder_mask": check_shoulder_mask,
        "check_fibre_lip_mask": check_fibre_lip_mask,
        "splinter_cut_mask": splinter_cut_mask,
        "splinter_lip_mask": splinter_lip_mask,
        "scratch_mask": scratch_mask,
        "finish_loss_mask": finish_loss_mask,
        "damage_mask": damage_mask,
        "damage_height_delta_m": height_delta_m.astype(np.float32),
    }


def _segment_lengths_m(
    layout: dict[str, Any],
    tile_size_m: float,
) -> np.ndarray:
    joints = np.asarray(layout["joints"], dtype=np.float32)
    spans = joints[:, 1] - joints[:, 0]
    lengths = np.stack([spans, 1.0 - spans], axis=1) * tile_size_m
    return lengths.reshape(-1).astype(np.float32)


def _focal_knot_center_uv(
    layout: dict[str, Any],
) -> tuple[float, float]:
    left_u = 0.0
    widths = np.asarray(layout["widths"], dtype=np.float32)
    joints = np.asarray(layout["joints"], dtype=np.float32)
    for board_index, board in enumerate(layout["boards"]):
        width = float(widths[board_index])
        for knot in board["knots"]:
            if knot["intent"] != "focal_intergrown":
                continue
            center_u = left_u + width * float(knot["center_u"])
            joint_a, joint_b = joints[board_index]
            if int(knot["segment"]) == 0:
                center_v = float(
                    joint_a + (joint_b - joint_a) * knot["center_v"]
                )
            else:
                complement_span = 1.0 - float(joint_b - joint_a)
                center_v = float(
                    np.mod(
                        joint_b + complement_span * knot["center_v"],
                        1.0,
                    )
                )
            return center_u, center_v
        left_u += width
    raise ValueError("aged-oak recipe requires one focal_intergrown knot")


def _segment_shade_families(
    *,
    base_palette_srgb: np.ndarray,
    palette_choice: np.ndarray,
    segment_value: np.ndarray,
    segment_temperature: np.ndarray,
) -> np.ndarray:
    """Author twenty related brown controls around every segment identity."""
    ink = np.asarray(INK_SRGB, dtype=np.float32) / 255.0
    olive = np.asarray([116, 103, 70], dtype=np.float32) / 255.0
    amber = np.asarray([174, 108, 48], dtype=np.float32) / 255.0
    pale = np.asarray([205, 166, 116], dtype=np.float32) / 255.0
    anchor_positions = np.asarray(
        [0.00, 0.13, 0.29, 0.47, 0.64, 0.82, 1.00],
        dtype=np.float32,
    )
    shade_positions = np.linspace(
        0.0,
        1.0,
        WOOD_SHADE_FAMILY_SIZE,
        dtype=np.float32,
    )
    families = np.empty(
        (len(palette_choice), WOOD_SHADE_FAMILY_SIZE, 3),
        dtype=np.float32,
    )
    for segment_index, palette_index in enumerate(palette_choice):
        base = (
            base_palette_srgb[int(palette_index)]
            * float(segment_value[segment_index])
        )
        temperature = float(segment_temperature[segment_index])
        base *= np.asarray(
            [
                1.0 + temperature * 0.055,
                1.0 + temperature * 0.010,
                1.0 - temperature * 0.065,
            ],
            dtype=np.float32,
        )
        anchors = np.stack(
            [
                base * 0.43 + ink * 0.57,
                base * 0.62 + ink * 0.23 + olive * 0.15,
                base * 0.82 + olive * 0.12 + ink * 0.06,
                base,
                base * 0.86 + amber * 0.14,
                base * 0.73 + amber * 0.10 + pale * 0.17,
                base * 0.57 + pale * 0.43,
            ],
            axis=0,
        )
        family_srgb = np.stack(
            [
                np.interp(
                    shade_positions,
                    anchor_positions,
                    anchors[:, channel],
                )
                for channel in range(3)
            ],
            axis=-1,
        )
        families[segment_index] = srgb_to_linear(
            np.clip(family_srgb, 0.0, 1.0)
        )
    return families.astype(np.float32)


def _sample_segment_shade_families(
    segment_shade_palettes: np.ndarray,
    segment_id: np.ndarray,
    shade_coordinate: np.ndarray,
) -> np.ndarray:
    """Linearly sample shade controls without posterized color bands."""
    scaled = (
        np.clip(shade_coordinate, 0.0, 1.0)
        * float(WOOD_SHADE_FAMILY_SIZE - 1)
    )
    lower = np.floor(scaled).astype(np.int32)
    upper = np.minimum(lower + 1, WOOD_SHADE_FAMILY_SIZE - 1)
    fraction = (scaled - lower)[..., np.newaxis].astype(np.float32)
    lower_color = segment_shade_palettes[segment_id, lower]
    upper_color = segment_shade_palettes[segment_id, upper]
    return (
        lower_color * (1.0 - fraction) + upper_color * fraction
    ).astype(np.float32)


def _stretch_shade_coordinate_per_segment(
    raw_coordinate: np.ndarray,
    *,
    segment_id: np.ndarray,
    segment_count: int,
    board_mask: np.ndarray,
) -> np.ndarray:
    """Give each plank its full authored color range without hard bands."""
    coordinate = np.full(
        raw_coordinate.shape,
        0.5,
        dtype=np.float32,
    )
    for segment_index in range(segment_count):
        core = np.logical_and(
            segment_id == segment_index,
            board_mask > 0.72,
        )
        samples = raw_coordinate[core]
        if samples.size < 8:
            continue
        low = float(np.percentile(samples, 2.0))
        high = float(np.percentile(samples, 98.0))
        span = max(high - low, 1.0e-5)
        segment_coordinate = np.clip(
            (raw_coordinate - low) / span,
            0.0,
            1.0,
        )
        coordinate[segment_id == segment_index] = segment_coordinate[
            segment_id == segment_index
        ]
    return coordinate.astype(np.float32)


def layered_wood_state_node(
    *,
    segment_id: np.ndarray,
    segment_count: int,
    attribute_rng: np.random.Generator,
    board_mask: np.ndarray,
    macro: np.ndarray,
    color_temperature: np.ndarray,
    surface_noise: np.ndarray,
    grain_mask: np.ndarray,
    anatomy: dict[str, np.ndarray],
    microstructure: dict[str, np.ndarray],
    branch: dict[str, np.ndarray],
    damage: dict[str, np.ndarray],
    cavity: np.ndarray,
    raster: dict[str, np.ndarray],
    gap_half_m: float,
    bevel_m: float,
) -> dict[str, Any]:
    """Compose wood pigment, finish, and damage as distinct material states."""
    palette_weights = np.asarray(
        [0.08, 0.18, 0.28, 0.22, 0.08, 0.16],
        dtype=np.float32,
    )
    palette = srgb_to_linear(
        np.asarray(WOOD_PALETTE_SRGB, dtype=np.float32) / 255.0
    )
    palette_choice = attribute_rng.choice(
        len(palette),
        size=segment_count,
        p=palette_weights,
    )
    segment_value = attribute_rng.uniform(
        0.95, 1.05, segment_count
    ).astype(np.float32)
    segment_temperature = attribute_rng.uniform(
        -1.0,
        1.0,
        segment_count,
    ).astype(np.float32)
    segment_shade_palettes = _segment_shade_families(
        base_palette_srgb=(
            np.asarray(WOOD_PALETTE_SRGB, dtype=np.float32) / 255.0
        ),
        palette_choice=palette_choice,
        segment_value=segment_value,
        segment_temperature=segment_temperature,
    )

    pigment_temperature = np.clip(
        color_temperature * 0.72 + macro * 0.28,
        0.0,
        1.0,
    ).astype(np.float32)
    slow_pigment = np.clip(
        color_temperature * 0.64
        + macro * 0.28
        + periodic_gaussian_blur(
            surface_noise,
            max(4.0, board_mask.shape[0] / 36.0),
        )
        * 0.08,
        0.0,
        1.0,
    ).astype(np.float32)
    growth_history_detail = np.clip(
        0.50
        + (anatomy["ring_spacing_multiplier"] - 1.0) * 1.15
        + (anatomy["earlywood_mask"] - anatomy["latewood_mask"])
        * 0.11,
        0.0,
        1.0,
    ).astype(np.float32)
    tissue_detail = np.clip(
        0.48
        + anatomy["earlywood_mask"] * 0.24
        - anatomy["latewood_mask"] * 0.13
        - microstructure["vessel_mask"] * 0.20
        + microstructure["ray_mask"] * 0.16
        - microstructure["fibre_bundle_mask"] * 0.08,
        0.0,
        1.0,
    ).astype(np.float32)
    shade_blur = max(1.5, board_mask.shape[0] / 192.0)
    growth_history = periodic_gaussian_blur(
        growth_history_detail,
        shade_blur,
    ).astype(np.float32)
    tissue = periodic_gaussian_blur(
        tissue_detail,
        shade_blur * 0.72,
    ).astype(np.float32)
    raw_shade_coordinate = (
        slow_pigment * 0.60
        + growth_history * 0.29
        + tissue * 0.11
    ).astype(np.float32)
    shade_coordinate = _stretch_shade_coordinate_per_segment(
        raw_shade_coordinate,
        segment_id=segment_id,
        segment_count=segment_count,
        board_mask=board_mask,
    )
    shade_index_continuous = (
        shade_coordinate * float(WOOD_SHADE_FAMILY_SIZE - 1)
    ).astype(np.float32)
    heartwood = _sample_segment_shade_families(
        segment_shade_palettes,
        segment_id,
        shade_coordinate,
    )

    average_color = np.average(
        palette,
        axis=0,
        weights=palette_weights,
    )
    pale_oak = srgb_to_linear(
        np.asarray([196, 151, 101], dtype=np.float32) / 255.0
    )
    olive_oak = srgb_to_linear(
        np.asarray([116, 103, 70], dtype=np.float32) / 255.0
    )
    amber_oak = srgb_to_linear(
        np.asarray([174, 108, 48], dtype=np.float32) / 255.0
    )
    earlywood_color = _sample_segment_shade_families(
        segment_shade_palettes,
        segment_id,
        np.clip(shade_coordinate + 0.18, 0.0, 1.0),
    )
    earlywood_color = np.clip(
        earlywood_color * np.asarray([1.025, 1.010, 0.970])
        + pale_oak * 0.018,
        0.0,
        1.0,
    )
    latewood_color = _sample_segment_shade_families(
        segment_shade_palettes,
        segment_id,
        np.clip(shade_coordinate - 0.16, 0.0, 1.0),
    )
    latewood_color = np.clip(
        latewood_color * np.asarray([0.990, 0.990, 1.015]),
        0.0,
        1.0,
    )
    warm_zone_color = _sample_segment_shade_families(
        segment_shade_palettes,
        segment_id,
        np.clip(shade_coordinate + 0.10, 0.0, 1.0),
    )
    warm_zone_color = np.clip(
        warm_zone_color * np.asarray([1.040, 1.010, 0.945])
        + amber_oak * 0.016,
        0.0,
        1.0,
    )
    cool_zone_color = _sample_segment_shade_families(
        segment_shade_palettes,
        segment_id,
        np.clip(shade_coordinate - 0.09, 0.0, 1.0),
    )
    cool_zone_color = np.clip(
        cool_zone_color * np.asarray([0.975, 1.000, 1.035])
        + olive_oak * 0.012,
        0.0,
        1.0,
    )

    finish_loss_halo = np.clip(
        periodic_gaussian_blur(
            damage["finish_loss_mask"],
            max(1.0, board_mask.shape[0] / 640.0),
        )
        * 1.18,
        0.0,
        1.0,
    )
    finish_body = 0.86 + 0.14 * smoothstep(0.22, 0.82, surface_noise)
    finish_thickness = (
        board_mask
        * finish_body
        * (1.0 - finish_loss_halo * 0.92)
    )
    finish_thickness = np.clip(finish_thickness, 0.0, 1.0).astype(
        np.float32
    )

    compressed_wood_mask = np.maximum(
        damage["check_shoulder_mask"],
        periodic_gaussian_blur(
            damage["splinter_cut_mask"],
            max(0.8, board_mask.shape[0] / 960.0),
        )
        * (1.0 - damage["splinter_cut_mask"])
        * 0.30,
    )
    compressed_wood_mask *= board_mask
    compressed_wood_mask = np.clip(
        compressed_wood_mask, 0.0, 1.0
    ).astype(np.float32)

    exposed_wood_mask = np.maximum.reduce(
        [
            damage["check_fibre_lip_mask"],
            damage["splinter_lip_mask"],
            damage["scratch_mask"] * 0.34,
        ]
    )
    exposed_wood_mask *= board_mask
    exposed_wood_mask = np.clip(
        exposed_wood_mask, 0.0, 1.0
    ).astype(np.float32)

    cavity_habitat = np.maximum(
        damage["check_mask"],
        damage["splinter_cut_mask"],
    )
    dirt_clumps = 0.34 + 0.66 * smoothstep(0.42, 0.72, surface_noise)
    dirt_deposit_mask = (
        cavity_habitat
        * (0.46 + 0.54 * cavity)
        * dirt_clumps
        * (1.0 - exposed_wood_mask * 0.72)
        * board_mask
    )
    dirt_deposit_mask = np.clip(
        dirt_deposit_mask, 0.0, 1.0
    ).astype(np.float32)

    end_accent = (
        1.0
        - smoothstep(
            gap_half_m + bevel_m,
            gap_half_m + bevel_m * 3.0,
            raster["joint_edge_distance"],
        )
    ) * board_mask
    compressed_coverage = np.maximum(
        compressed_wood_mask * 0.78,
        end_accent * 0.12,
    )
    compressed_color = np.clip(
        heartwood
        * np.asarray([0.76, 0.68, 0.62], dtype=np.float32),
        0.0,
        1.0,
    )
    pale_fibre = np.broadcast_to(
        srgb_to_linear(
            np.asarray([196, 145, 91], dtype=np.float32) / 255.0
        ),
        heartwood.shape,
    )
    exposed_substrate_color = mix(
        heartwood,
        pale_fibre,
        np.full(board_mask.shape, 0.72, dtype=np.float32),
    ).astype(np.float32)
    dirt_color = np.broadcast_to(
        srgb_to_linear(
            np.asarray([48, 39, 31], dtype=np.float32) / 255.0
        ),
        heartwood.shape,
    ).astype(np.float32)

    knot_body_color = np.clip(
        average_color * np.asarray([0.68, 0.65, 0.58], dtype=np.float32),
        0.0,
        1.0,
    )
    ink_color = srgb_to_linear(
        np.asarray(INK_SRGB, dtype=np.float32) / 255.0
    )
    branch_coverage = np.maximum.reduce(
        [
            branch["knot_mask"] * board_mask * 0.44,
            branch["knot_boundary_mask"] * 0.52,
            branch["knot_ring_mask"] * 0.48,
            branch["knot_core_mask"] * 0.76,
        ]
    )
    branch_color = np.broadcast_to(
        knot_body_color,
        heartwood.shape,
    ).copy()
    branch_color = mix(
        branch_color,
        np.broadcast_to(ink_color, heartwood.shape),
        branch["knot_boundary_mask"],
    )
    branch_color = mix(
        branch_color,
        np.broadcast_to(knot_body_color * 0.70, heartwood.shape),
        branch["knot_ring_mask"],
    )
    branch_color = mix(
        branch_color,
        np.broadcast_to(ink_color * 0.58, heartwood.shape),
        branch["knot_core_mask"],
    )

    warm_coverage = (
        smoothstep(0.52, 0.82, pigment_temperature)
        * board_mask
        * 0.23
    )
    cool_coverage = (
        (
            1.0
            - smoothstep(0.18, 0.48, pigment_temperature)
        )
        * board_mask
        * 0.18
    )
    earlywood_coverage = np.clip(
        anatomy["earlywood_mask"] * board_mask * 0.045
        + microstructure["earlywood_pore_mask"] * 0.32,
        0.0,
        1.0,
    )
    latewood_coverage = np.clip(
        anatomy["latewood_mask"] * board_mask * 0.035
        + microstructure["latewood_pore_mask"] * 0.14
        + microstructure["fibre_bundle_mask"] * 0.10,
        0.0,
        1.0,
    )
    segment_roughness = attribute_rng.uniform(
        0.57, 0.70, segment_count
    ).astype(np.float32)
    roughness_base = (
        0.94 * (1.0 - board_mask)
        + segment_roughness[segment_id] * board_mask
    ).astype(np.float32)
    zero = np.zeros_like(board_mask, dtype=np.float32)
    surface_layers: dict[str, dict[str, np.ndarray]] = {}
    groove_color = heartwood.copy()

    def apply_surface_layer(
        name: str,
        color: np.ndarray,
        coverage: np.ndarray,
        roughness_delta: np.ndarray,
    ) -> None:
        nonlocal groove_color
        clipped_coverage = np.clip(
            coverage, 0.0, 1.0
        ).astype(np.float32)
        color_field = np.broadcast_to(
            np.asarray(color, dtype=np.float32),
            heartwood.shape,
        ).copy()
        groove_color = mix(
            groove_color,
            color_field,
            clipped_coverage,
        )
        surface_layers[name] = {
            "coverage": clipped_coverage,
            "color_linear": color_field,
            "roughness_delta": np.asarray(
                roughness_delta,
                dtype=np.float32,
            ),
        }

    surface_layers["heartwood_substrate"] = {
        "coverage": board_mask.copy(),
        "color_linear": heartwood.copy(),
        "roughness_delta": (
            (surface_noise - 0.5) * 0.045
        ).astype(np.float32),
    }
    apply_surface_layer(
        "cool_olive_wash",
        cool_zone_color,
        cool_coverage,
        zero,
    )
    apply_surface_layer(
        "warm_amber_wash",
        warm_zone_color,
        warm_coverage,
        zero,
    )
    apply_surface_layer(
        "earlywood_tone",
        earlywood_color,
        earlywood_coverage,
        anatomy["earlywood_mask"] * 0.010,
    )
    apply_surface_layer(
        "latewood_tone",
        latewood_color,
        latewood_coverage,
        (
            microstructure["latewood_pore_mask"] * 0.030
            + microstructure["fibre_bundle_mask"] * 0.014
        ),
    )
    apply_surface_layer(
        "vessel_tone",
        latewood_color * 0.72,
        microstructure["vessel_mask"] * 0.34,
        microstructure["vessel_mask"] * 0.050,
    )
    apply_surface_layer(
        "ray_figure",
        earlywood_color * np.asarray(
            [1.03, 0.98, 0.84],
            dtype=np.float32,
        ),
        microstructure["ray_mask"] * 0.25,
        microstructure["ray_mask"] * 0.018,
    )
    apply_surface_layer(
        "fibre_tone",
        latewood_color * 0.86,
        microstructure["fibre_bundle_mask"] * 0.12,
        microstructure["fibre_bundle_mask"] * 0.018,
    )
    growth_pigment = np.clip(
        groove_color, 0.0, 1.0
    ).astype(np.float32)
    apply_surface_layer(
        "branch_wood",
        branch_color,
        branch_coverage,
        (
            branch["knot_mask"] * 0.025
            + branch["knot_boundary_mask"] * 0.065
            + branch["knot_core_mask"] * 0.050
        ),
    )
    amber_finish_color = np.clip(
        groove_color
        * np.asarray([1.10, 1.00, 0.78], dtype=np.float32)
        + srgb_to_linear(
            np.asarray([164, 91, 34], dtype=np.float32) / 255.0
        )
        * 0.06,
        0.0,
        1.0,
    )
    apply_surface_layer(
        "amber_finish",
        amber_finish_color,
        finish_thickness * 0.24,
        -finish_thickness * 0.055,
    )
    finished_wood = groove_color.copy()
    apply_surface_layer(
        "compressed_wood",
        compressed_color,
        compressed_coverage,
        -compressed_wood_mask * 0.045,
    )
    cavity_habitat = np.maximum(
        damage["check_mask"],
        damage["splinter_cut_mask"],
    )
    damage_core_color = np.clip(
        growth_pigment
        * np.asarray([0.46, 0.40, 0.36], dtype=np.float32),
        0.0,
        1.0,
    )
    damage_core_coverage = np.clip(
        cavity_habitat * (0.18 + cavity * 0.34),
        0.0,
        1.0,
    )
    apply_surface_layer(
        "damage_core",
        damage_core_color,
        damage_core_coverage,
        damage_core_coverage * 0.090 + finish_loss_halo * 0.115,
    )
    apply_surface_layer(
        "exposed_fibre",
        exposed_substrate_color,
        exposed_wood_mask * 0.90,
        exposed_wood_mask * 0.175,
    )
    apply_surface_layer(
        "retained_dirt",
        dirt_color,
        dirt_deposit_mask * 0.94,
        dirt_deposit_mask * 0.120,
    )
    groove_color = np.clip(groove_color, 0.0, 1.0).astype(np.float32)

    wood_roughness = segment_roughness[segment_id].copy()
    for layer in surface_layers.values():
        wood_roughness += layer["roughness_delta"]
    roughness = (
        0.94 * (1.0 - board_mask) + wood_roughness * board_mask
    )
    roughness = np.clip(roughness, 0.42, 0.98).astype(np.float32)
    finish_coat_roughness = np.clip(
        0.30
        + (surface_noise - 0.5) * 0.10
        + finish_loss_halo * 0.24,
        0.24,
        0.62,
    ).astype(np.float32)

    gap_color = srgb_to_linear(
        np.asarray(GAP_SRGB, dtype=np.float32) / 255.0
    )
    base_color = mix(
        np.broadcast_to(gap_color, groove_color.shape),
        groove_color,
        board_mask,
    )
    pigment_layer_order = (
        "heartwood_base",
        "cool_olive_wash",
        "warm_amber_wash",
        "earlywood_tone",
        "latewood_tone",
        "vessel_tone",
        "ray_figure",
        "fibre_tone",
    )
    pigment_layers = {
        "heartwood_base": surface_layers["heartwood_substrate"],
        "cool_olive_wash": surface_layers["cool_olive_wash"],
        "warm_amber_wash": surface_layers["warm_amber_wash"],
        "earlywood_tone": surface_layers["earlywood_tone"],
        "latewood_tone": surface_layers["latewood_tone"],
        "vessel_tone": surface_layers["vessel_tone"],
        "ray_figure": surface_layers["ray_figure"],
        "fibre_tone": surface_layers["fibre_tone"],
    }
    return {
        "shade_family_size": WOOD_SHADE_FAMILY_SIZE,
        "segment_shade_palettes_linear": segment_shade_palettes,
        "shade_coordinate": shade_coordinate,
        "shade_index_continuous": shade_index_continuous,
        "shade_drivers": {
            "slow_pigment": slow_pigment,
            "growth_history": growth_history,
            "tissue": tissue,
        },
        "heartwood_pigment_linear": heartwood,
        "growth_pigment_linear": growth_pigment,
        "finished_wood_linear": finished_wood.astype(np.float32),
        "finish_loss_halo": finish_loss_halo.astype(np.float32),
        "finish_thickness": finish_thickness,
        "compressed_wood_mask": compressed_wood_mask,
        "exposed_wood_mask": exposed_wood_mask,
        "dirt_deposit_mask": dirt_deposit_mask,
        "groove_state_color_linear": groove_color,
        "base_color_linear": np.clip(base_color, 0.0, 1.0).astype(
            np.float32
        ),
        "end_accent": end_accent.astype(np.float32),
        "roughness_base": roughness_base,
        "roughness": roughness,
        "finish_coat_roughness": finish_coat_roughness,
        "surface_layers": surface_layers,
        "warm_pigment_mask": warm_coverage.astype(np.float32),
        "cool_pigment_mask": cool_coverage.astype(np.float32),
        "pigment_temperature": pigment_temperature,
        "pigment_layer_order": pigment_layer_order,
        "pigment_layers": pigment_layers,
    }


def generate_material(
    *,
    resolution: int = DEFAULT_RESOLUTION,
    seed: int = DEFAULT_SEED,
    tile_size_m: float = DEFAULT_TILE_SIZE_M,
    pattern_recipe_path: str | Path | None = None,
    pattern_variation: int = DEFAULT_PATTERN_VARIATION,
) -> dict[str, Any]:
    if resolution < 64:
        raise ValueError("resolution must be at least 64 pixels")
    if tile_size_m <= 0.0:
        raise ValueError("tile size must be positive")
    meters_per_pixel = tile_size_m / resolution
    recipe = load_plank_recipe(
        pattern_recipe_path or DEFAULT_PATTERN_RECIPE_PATH
    )
    if not math.isclose(tile_size_m, recipe.tile_size_m, abs_tol=1.0e-6):
        raise ValueError(
            f"requested tile size {tile_size_m} does not match "
            f"authored recipe tile size {recipe.tile_size_m}"
        )
    layout = generate_plank_layout(recipe, variation=pattern_variation)
    raster = rasterize_plank_pattern(
        layout,
        resolution=resolution,
        tile_size_m=tile_size_m,
    )
    segment_id = raster["segment_id"]
    segment_count = len(recipe.boards) * 2

    gap_half_m = max(
        recipe.layout["gap_half_width_m"], meters_per_pixel * 0.60
    )
    bevel_m = max(recipe.layout["bevel_width_m"], meters_per_pixel * 0.85)
    board_mask = smoothstep(
        gap_half_m,
        gap_half_m + bevel_m,
        raster["edge_distance"],
    ).astype(np.float32)

    macro = periodic_fbm(resolution, 3, seed + 101, octaves=4)
    color_temperature = periodic_fbm_rect(
        resolution,
        5,
        9,
        seed + 211,
        octaves=3,
        persistence=0.48,
    )
    surface_noise = periodic_fbm(resolution, 23, seed + 401, octaves=3)
    attribute_rng = np.random.default_rng(seed + pattern_variation * 1009)
    branch = branch_intersection_node(
        raster,
        layout,
        tile_size_m=tile_size_m,
    )
    anatomy = virtual_log_ring_node(
        raster,
        layout,
        branch,
        tile_size_m=tile_size_m,
        board_mask=board_mask,
    )
    microstructure = oak_microstructure_node(
        raster,
        layout,
        anatomy,
        branch,
        board_mask=board_mask,
        seed=seed,
    )
    damage = authored_damage_node(
        raster,
        layout,
        tile_size_m=tile_size_m,
    )

    knot_mask = branch["knot_mask"] * board_mask
    broad_grain = anatomy["earlywood_mask"]
    fine_grain = microstructure["fibre_bundle_mask"]
    grain_mask = np.clip(
        broad_grain * 0.46
        + microstructure["earlywood_pore_mask"] * 0.32
        + microstructure["latewood_pore_mask"] * 0.18
        + microstructure["ray_mask"] * 0.13
        + fine_grain * 0.22,
        0.0,
        1.0,
    )
    grain_mask *= board_mask
    grain_mask = grain_mask.astype(np.float32)

    segment_raise = attribute_rng.uniform(
        -0.0004, 0.0013, segment_count
    ).astype(np.float32)
    wood_height_base = (
        0.0042
        + segment_raise[segment_id]
        + (macro - 0.5) * 0.00075
        + (surface_noise - 0.5) * 0.00028
    ).astype(np.float32)
    grain_height_delta = -(
        broad_grain * 0.000025
        + microstructure["earlywood_pore_mask"] * 0.000060
        + microstructure["latewood_pore_mask"] * 0.000018
        + microstructure["ray_mask"] * 0.000014
        + fine_grain * 0.000030
    )
    character_height_delta = (
        -branch["knot_core_mask"] * 0.00032
        - branch["knot_ring_mask"] * 0.00010
        - branch["knot_boundary_mask"] * 0.00008
        + damage["damage_height_delta_m"]
    ).astype(np.float32)
    wood_height = (
        wood_height_base
        + grain_height_delta
        + character_height_delta
    )
    gap_height = -0.0010 + (surface_noise - 0.5) * 0.00018
    height_m = (
        gap_height * (1.0 - board_mask) + wood_height * board_mask
    ).astype(np.float32)
    normal = height_to_normal(
        height_m,
        meters_per_pixel,
        strength=0.78,
    )

    cavity = np.maximum(
        periodic_gaussian_blur(height_m, max(1.0, resolution / 512.0))
        - height_m,
        0.0,
    )
    cavity = _normalize_positive(cavity)
    ao = np.clip(
        1.0 - cavity * 0.42 - (1.0 - board_mask) * 0.20,
        0.38,
        1.0,
    ).astype(np.float32)

    surface_state = layered_wood_state_node(
        segment_id=segment_id,
        segment_count=segment_count,
        attribute_rng=attribute_rng,
        board_mask=board_mask,
        macro=macro,
        color_temperature=color_temperature,
        surface_noise=surface_noise,
        grain_mask=grain_mask,
        anatomy=anatomy,
        microstructure=microstructure,
        branch=branch,
        damage=damage,
        cavity=cavity,
        raster=raster,
        gap_half_m=gap_half_m,
        bevel_m=bevel_m,
    )
    base_color_linear = surface_state["base_color_linear"]
    roughness_base = surface_state["roughness_base"]
    roughness = surface_state["roughness"]
    orm = np.stack(
        [ao, roughness, np.zeros_like(roughness)], axis=-1
    ).astype(np.float32)
    preview_srgb = render_material_preview(
        base_color_linear,
        normal,
        roughness,
        ao,
    )
    white_light_frontal_srgb = render_material_single_light(
        base_color_linear,
        normal,
        roughness,
        ao,
        direction=(-0.10, -0.10, 1.00),
        intensity=2.0,
    )
    white_light_side_srgb = render_material_single_light(
        base_color_linear,
        normal,
        roughness,
        ao,
        direction=(-0.78, -0.12, 0.62),
        intensity=2.0,
    )
    white_light_grazing_srgb = render_material_single_light(
        base_color_linear,
        normal,
        roughness,
        ao,
        direction=(0.92, 0.10, 0.37),
        intensity=2.0,
    )
    grain_only_height_m = (
        gap_height * (1.0 - board_mask)
        + (wood_height_base + grain_height_delta) * board_mask
    ).astype(np.float32)
    grain_only_normal = height_to_normal(
        grain_only_height_m,
        meters_per_pixel,
        strength=0.78,
    )
    grain_only_cavity = np.maximum(
        periodic_gaussian_blur(
            grain_only_height_m,
            max(1.0, resolution / 512.0),
        )
        - grain_only_height_m,
        0.0,
    )
    grain_only_cavity = _normalize_positive(grain_only_cavity)
    grain_only_ao = np.clip(
        1.0
        - grain_only_cavity * 0.30
        - (1.0 - board_mask) * 0.20,
        0.45,
        1.0,
    ).astype(np.float32)
    grain_only_roughness = np.clip(
        roughness_base
        + board_mask
        * (
            anatomy["earlywood_mask"] * 0.018
            + microstructure["vessel_mask"] * 0.052
            + microstructure["ray_mask"] * 0.018
            + microstructure["fibre_bundle_mask"] * 0.020
        ),
        0.45,
        0.96,
    ).astype(np.float32)
    grain_only_white_light_srgb = render_material_single_light(
        surface_state["growth_pigment_linear"],
        grain_only_normal,
        grain_only_roughness,
        grain_only_ao,
        direction=(-0.72, -0.16, 0.68),
        intensity=2.0,
    )
    return {
        "schema": MATERIAL_SCHEMA,
        "resolution": resolution,
        "seed": seed,
        "tile_size_m": tile_size_m,
        "meters_per_pixel": meters_per_pixel,
        "board_id": raster["board_id"],
        "segment_id": segment_id,
        "local_u": raster["local_u"],
        "local_v": raster["local_v"],
        "edge_distance": raster["edge_distance"],
        "board_edge_distance": raster["board_edge_distance"],
        "joint_edge_distance": raster["joint_edge_distance"],
        "board_mask": board_mask,
        "macro": macro,
        "surface_noise": surface_noise,
        "ring_phase_unwarped": anatomy["ring_phase_unwarped"],
        "ring_phase": anatomy["ring_phase"],
        "ring_fraction_unwarped": anatomy["ring_fraction_unwarped"],
        "ring_fraction": anatomy["ring_fraction"],
        "ring_spacing_m": anatomy["ring_spacing_m"],
        "ring_spacing_multiplier": anatomy["ring_spacing_multiplier"],
        "earlywood_unwarped": anatomy["earlywood_unwarped"],
        "earlywood_mask": anatomy["earlywood_mask"],
        "latewood_mask": anatomy["latewood_mask"],
        "growth_flow": anatomy["growth_flow"],
        "rest_mask": microstructure["rest_mask"],
        "ray_visibility": microstructure["ray_visibility"],
        "earlywood_pore_mask": microstructure[
            "earlywood_pore_mask"
        ],
        "latewood_pore_mask": microstructure["latewood_pore_mask"],
        "vessel_mask": microstructure["vessel_mask"],
        "ray_mask": microstructure["ray_mask"],
        "ray_direction": microstructure["ray_direction"],
        "fibre_bundle_mask": microstructure["fibre_bundle_mask"],
        "grain_activity": microstructure["grain_activity"],
        "grain_only_height_m": grain_only_height_m,
        "grain_only_normal": grain_only_normal,
        "grain_only_roughness": grain_only_roughness,
        "grain_only_ao": grain_only_ao,
        "grain_only_white_light_srgb": grain_only_white_light_srgb,
        "broad_grain": broad_grain,
        "fine_grain": fine_grain,
        "grain_mask": grain_mask,
        "knot_mask": knot_mask,
        "knot_core_mask": branch["knot_core_mask"],
        "knot_ring_mask": branch["knot_ring_mask"],
        "knot_boundary_mask": branch["knot_boundary_mask"],
        "knot_influence": branch["knot_influence"],
        "knot_warp_magnitude_m": branch["knot_warp_magnitude_m"],
        "knot_check_mask": damage["knot_check_mask"],
        "end_check_mask": damage["end_check_mask"],
        "check_mask": damage["check_mask"],
        "check_shoulder_mask": damage["check_shoulder_mask"],
        "check_fibre_lip_mask": damage["check_fibre_lip_mask"],
        "splinter_cut_mask": damage["splinter_cut_mask"],
        "splinter_lip_mask": damage["splinter_lip_mask"],
        "scratch_mask": damage["scratch_mask"],
        "finish_loss_mask": damage["finish_loss_mask"],
        "damage_mask": damage["damage_mask"],
        "damage_height_delta_m": damage["damage_height_delta_m"],
        "end_accent": surface_state["end_accent"],
        "segment_raise_m": segment_raise[segment_id],
        "wood_height_base_m": wood_height_base,
        "grain_height_delta_m": grain_height_delta,
        "character_height_delta_m": character_height_delta,
        "cavity": cavity,
        "height_m": height_m,
        "normal": normal,
        "segment_pigment_linear": surface_state[
            "heartwood_pigment_linear"
        ],
        "painted_pigment_linear": surface_state[
            "growth_pigment_linear"
        ],
        "grained_pigment_linear": surface_state[
            "growth_pigment_linear"
        ],
        "heartwood_pigment_linear": surface_state[
            "heartwood_pigment_linear"
        ],
        "growth_pigment_linear": surface_state[
            "growth_pigment_linear"
        ],
        "finished_wood_linear": surface_state["finished_wood_linear"],
        "warm_pigment_mask": surface_state["warm_pigment_mask"],
        "cool_pigment_mask": surface_state["cool_pigment_mask"],
        "pigment_temperature": surface_state["pigment_temperature"],
        "shade_family_size": surface_state["shade_family_size"],
        "segment_shade_palettes_linear": surface_state[
            "segment_shade_palettes_linear"
        ],
        "shade_coordinate": surface_state["shade_coordinate"],
        "shade_index_continuous": surface_state[
            "shade_index_continuous"
        ],
        "shade_drivers": surface_state["shade_drivers"],
        "pigment_layer_order": surface_state["pigment_layer_order"],
        "pigment_layers": surface_state["pigment_layers"],
        "finish_loss_halo": surface_state["finish_loss_halo"],
        "finish_thickness": surface_state["finish_thickness"],
        "compressed_wood_mask": surface_state[
            "compressed_wood_mask"
        ],
        "exposed_wood_mask": surface_state["exposed_wood_mask"],
        "dirt_deposit_mask": surface_state["dirt_deposit_mask"],
        "groove_state_color_linear": surface_state[
            "groove_state_color_linear"
        ],
        "base_color_linear": base_color_linear,
        "roughness_base": roughness_base,
        "roughness": roughness,
        "ao": ao,
        "orm": orm,
        "preview_srgb": preview_srgb,
        "white_light_frontal_srgb": white_light_frontal_srgb,
        "white_light_side_srgb": white_light_side_srgb,
        "white_light_grazing_srgb": white_light_grazing_srgb,
        "focal_knot_uv": _focal_knot_center_uv(layout),
        "pattern_recipe": recipe,
        "pattern": {
            "schema": recipe.schema,
            "name": recipe.name,
            "source": recipe.source_path,
            "variation": pattern_variation,
            "board_count": len(recipe.boards),
            "segment_count": segment_count,
            "characters": {
                character: layout["characters"].count(character)
                for character in ("clear", "knot")
            },
            "damage": {
                kind: sum(
                    record["kind"] == kind
                    for board in recipe.boards
                    for record in board["damage"]
                )
                for kind in (
                    "knot_check",
                    "end_check",
                    "edge_splinter",
                    "scratch",
                )
            },
        },
        "parameters": {
            "gap_half_width_m": gap_half_m,
            "bevel_width_m": bevel_m,
            "board_raise_m": 0.0042,
            "grain_relief_m": float(-grain_height_delta.min()),
            "knot_depth_m": 0.00032,
            "deepest_damage_m": float(
                -np.minimum(damage["damage_height_delta_m"], 0.0).min()
            ),
        },
    }


def write_material_package(
    material: dict[str, Any],
    output_directory: str | Path,
) -> dict[str, Any]:
    output = Path(output_directory)
    proof_directory = output / "proofs"
    book_directory = proof_directory / "wood_plank_v2_book"
    output.mkdir(parents=True, exist_ok=True)
    proof_directory.mkdir(parents=True, exist_ok=True)
    paths = {
        "base_color": output / "wood_plank_v2_basecolor.png",
        "normal": output / "wood_plank_v2_normal.png",
        "orm": output / "wood_plank_v2_orm.png",
        "height": output / "wood_plank_v2_height.png",
        "breakdown": proof_directory / "wood_plank_v2_breakdown.png",
        "anatomy_causality": (
            proof_directory / "wood_plank_v2_anatomy_causality.png"
        ),
        "grain_anatomy": (
            proof_directory / "wood_plank_v2_grain_anatomy.png"
        ),
        "pigment_layers": (
            proof_directory / "wood_plank_v2_pigment_layers.png"
        ),
        "intra_board_shades": (
            proof_directory / "wood_plank_v2_intra_board_shades.png"
        ),
        "damage_causality": (
            proof_directory / "wood_plank_v2_damage_causality.png"
        ),
        "groove_specimen": (
            proof_directory / "wood_plank_v2_groove_specimen.png"
        ),
        "tiling": proof_directory / "wood_plank_v2_tiling.png",
        "pattern_variations": (
            proof_directory / "wood_plank_v2_pattern_variations.png"
        ),
        "book_text": output / "wood_plank_v2_material_book.md",
        "manifest": output / "wood_plank_v2_manifest.json",
    }
    height = material["height_m"]
    height_min = float(height.min())
    height_max = float(height.max())
    height_normalized = (height - height_min) / max(
        height_max - height_min, 1.0e-8
    )
    write_png_rgb8(paths["base_color"], linear_to_srgb(material["base_color_linear"]))
    write_png_rgb8(paths["normal"], material["normal"] * 0.5 + 0.5)
    write_png_rgb8(paths["orm"], material["orm"])
    write_png_gray16(paths["height"], height_normalized)
    book_index, book_chapters = _wood_material_book(material)
    write_png_rgb8(paths["breakdown"], book_index)
    write_png_rgb8(
        paths["anatomy_causality"],
        _anatomy_causality_sheet(material),
    )
    write_png_rgb8(
        paths["grain_anatomy"],
        _grain_anatomy_sheet(material),
    )
    write_png_rgb8(
        paths["pigment_layers"],
        _pigment_layer_sheet(material),
    )
    write_png_rgb8(
        paths["intra_board_shades"],
        _intra_board_shade_sheet(material),
    )
    write_png_rgb8(
        paths["damage_causality"],
        _damage_causality_sheet(material),
    )
    write_png_rgb8(
        paths["groove_specimen"],
        _groove_specimen_sheet(material),
    )
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
    paths["book_text"].write_text(
        _wood_material_book_markdown(material),
        encoding="utf-8",
    )
    write_png_rgb8(paths["tiling"], _tiling_sheet(material))
    write_png_rgb8(
        paths["pattern_variations"],
        _pattern_variation_sheet(material),
    )
    outputs = {
        name: path.relative_to(output).as_posix() for name, path in paths.items()
    }
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
            "base_color": "sRGB color; painterly pigment and ink, no baked light",
            "normal": "linear RGB; tangent-space OpenGL/Y+",
            "orm": "linear RGB; R=ambient occlusion G=roughness B=metallic",
            "height": "linear normalized data; reconstruct metres from height_encoding",
        },
        "node_contract": {
            "virtual_log_rings": (
                "authored saw position and growth epochs produce continuous "
                "ring phase, earlywood, latewood, and growth flow"
            ),
            "branch_intersection": (
                "authored branches produce knot body, core, boundary rings, "
                "influence, and a metre-valued coordinate warp"
            ),
            "oak_microstructure": (
                "ring history owns earlywood and latewood pores, saw relation "
                "owns rays, and broken fibre families follow growth flow"
            ),
            "authored_damage": (
                "typed paths validate knot, end, edge, and use habitats before "
                "producing recess, raised lip, finish loss, and response masks"
            ),
            "layered_wood_state": (
                "each segment owns twenty related brown controls; slow pigment "
                "history and wood anatomy sample them continuously before "
                "finish, compressed wood, exposed substrate, and retained dirt"
            ),
            "causality_proof": outputs["anatomy_causality"],
            "grain_proof": outputs["grain_anatomy"],
            "pigment_proof": outputs["pigment_layers"],
            "intra_board_shade_proof": outputs["intra_board_shades"],
            "damage_proof": outputs["damage_causality"],
            "groove_specimen": outputs["groove_specimen"],
        },
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
            "board_coverage": float(material["board_mask"].mean()),
            "grain_coverage": float(material["grain_mask"].mean()),
            "shade_family_size_per_segment": int(
                material["shade_family_size"]
            ),
            "knot_coverage": float(material["knot_mask"].mean()),
            "check_coverage": float(material["check_mask"].mean()),
            "splinter_coverage": float(
                material["splinter_cut_mask"].mean()
            ),
            "scratch_coverage": float(material["scratch_mask"].mean()),
            "compressed_wood_coverage": float(
                material["compressed_wood_mask"].mean()
            ),
            "exposed_wood_coverage": float(
                material["exposed_wood_mask"].mean()
            ),
            "dirt_deposit_coverage": float(
                material["dirt_deposit_mask"].mean()
            ),
        },
        "outputs": outputs,
        "reproduction": (
            "Run Blender headless with this script and the recorded seed, "
            "resolution, tile size, pattern recipe, and variation."
        ),
    }
    paths["manifest"].write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return manifest


def _normalize_positive(value: np.ndarray) -> np.ndarray:
    scale = float(np.percentile(value, 99.0))
    if scale <= 1.0e-10:
        return np.zeros_like(value, dtype=np.float32)
    return np.clip(value / scale, 0.0, 1.0).astype(np.float32)


def _anatomy_causality_sheet(material: dict[str, Any]) -> np.ndarray:
    unwarped = material["earlywood_unwarped"]
    warped = material["earlywood_mask"]
    branch_overlay = mix(
        gray_rgb(unwarped),
        np.broadcast_to(
            np.asarray([0.88, 0.28, 0.12], dtype=np.float32),
            (*unwarped.shape, 3),
        ),
        material["knot_mask"] * 0.88,
    )
    phase_delta = _normalize_positive(
        np.abs(
            material["ring_phase"] - material["ring_phase_unwarped"]
        )
    )
    warp_magnitude = _normalize_positive(
        material["knot_warp_magnitude_m"]
    )
    growth_flow = material["growth_flow"]
    flow_rgb = np.stack(
        [
            growth_flow[..., 0] * 0.5 + 0.5,
            growth_flow[..., 1] * 0.5 + 0.5,
            np.full(growth_flow.shape[:2], 0.42, dtype=np.float32),
        ],
        axis=-1,
    )
    knot_components = np.stack(
        [
            material["knot_core_mask"],
            material["knot_ring_mask"],
            material["knot_boundary_mask"],
        ],
        axis=-1,
    )
    focal_u, focal_v = material["focal_knot_uv"]

    def focal(image: np.ndarray) -> np.ndarray:
        return zoom_square(
            image,
            center_u=focal_u,
            center_v=focal_v,
            fraction=0.16,
        )

    return compose_material_chapter(
        "ANATOMY AND KNOT CAUSALITY",
        "THE BRANCH CHANGES THE WOOD BEFORE PIGMENT DESCRIBES THE BRANCH",
        [
            ("UNWARPED CLOSE", focal(gray_rgb(unwarped))),
            ("BRANCH STAMP ONLY", focal(branch_overlay)),
            (
                "INFLUENCE CLOSE",
                focal(gray_rgb(material["knot_influence"])),
            ),
            ("COORDINATE WARP", focal(gray_rgb(warp_magnitude))),
            ("WARPED EARLYWOOD", focal(gray_rgb(warped))),
            ("RING PHASE CHANGE", focal(gray_rgb(phase_delta))),
            ("GROWTH FLOW XY", focal(flow_rgb)),
            ("CORE RINGS BOUNDARY", focal(knot_components)),
        ],
        [
            "ALL PANELS HOLD THE SAME FOCAL CROP SO THE WRONG DARK STAMP CAN BE COMPARED DIRECTLY WITH THE DEFORMED WOOD",
            "INFLUENCE AND COORDINATE WARP ARE CAUSES WARPED EARLYWOOD PHASE CHANGE AND GROWTH FLOW ARE THEIR CONSEQUENCES",
            "CORE RINGS AND BOUNDARY REMAIN SEPARATE SO HEIGHT PIGMENT AND ROUGHNESS CAN INTERPRET THE BRANCH DIFFERENTLY",
        ],
    )


def _grain_anatomy_sheet(material: dict[str, Any]) -> np.ndarray:
    spacing_rgb = scalar_tint(
        material["ring_spacing_multiplier"],
        (0.14, 0.32, 0.68),
        (0.96, 0.66, 0.20),
        normalize=True,
    )
    early_late = np.stack(
        [
            material["earlywood_mask"],
            material["latewood_mask"] * 0.72,
            np.zeros_like(material["earlywood_mask"]),
        ],
        axis=-1,
    )
    pore_families = np.stack(
        [
            material["earlywood_pore_mask"],
            material["latewood_pore_mask"],
            material["vessel_mask"] * 0.64,
        ],
        axis=-1,
    )
    ray_family = np.stack(
        [
            material["ray_mask"],
            material["ray_visibility"] * 0.62,
            np.zeros_like(material["ray_mask"]),
        ],
        axis=-1,
    )
    activity_rest = np.stack(
        [
            material["grain_activity"],
            np.zeros_like(material["grain_activity"]),
            material["rest_mask"],
        ],
        axis=-1,
    )
    candidate = (
        periodic_gaussian_blur(
            material["vessel_mask"]
            + material["ray_mask"] * 0.35
            + material["fibre_bundle_mask"] * 0.25,
            max(2.0, material["resolution"] * 0.012),
        )
        * (1.0 - material["rest_mask"])
        * (1.0 - material["knot_influence"] * 0.92)
        * material["board_mask"]
        * smoothstep(
            0.045,
            0.070,
            material["edge_distance"],
        )
    )
    detail_y, detail_x = np.unravel_index(
        int(np.argmax(candidate)),
        candidate.shape,
    )
    detail_u = float(detail_x) / max(candidate.shape[1] - 1, 1)
    detail_v = float(detail_y) / max(candidate.shape[0] - 1, 1)
    ray_y, ray_x = np.unravel_index(
        int(np.argmax(material["ray_mask"])),
        material["ray_mask"].shape,
    )
    ray_u = float(ray_x) / max(material["ray_mask"].shape[1] - 1, 1)
    ray_v = float(ray_y) / max(material["ray_mask"].shape[0] - 1, 1)

    def detail(image: np.ndarray) -> np.ndarray:
        return zoom_square(
            image,
            center_u=detail_u,
            center_v=detail_v,
            fraction=0.07,
        )

    return compose_material_chapter(
        "WHITE OAK GRAIN ANATOMY",
        "DAMAGE DISABLED IN THE ACCEPTANCE READ SO THE WOOD MUST CARRY ITSELF",
        [
            ("RING SPACING HISTORY", spacing_rgb),
            ("EARLYWOOD LATEWOOD", early_late),
            ("PORE FAMILIES CLOSE", detail(pore_families)),
            (
                "CUT DEPENDENT RAYS",
                zoom_square(
                    ray_family,
                    center_u=ray_u,
                    center_v=ray_v,
                    fraction=0.12,
                ),
            ),
            (
                "BROKEN FIBRE FAMILIES",
                detail(gray_rgb(material["fibre_bundle_mask"])),
            ),
            ("ACTIVITY RED REST BLUE", activity_rest),
            (
                "GRAIN ONLY COLOR",
                detail(
                    linear_to_srgb(material["growth_pigment_linear"])
                ),
            ),
            (
                "GRAIN ONLY WHITE LIGHT",
                detail(material["grain_only_white_light_srgb"]),
            ),
        ],
        [
            "BLUE TO GOLD SHOWS SLOWLY CHANGING RING SPACING RATHER THAN ONE METRONOMIC INTERVAL",
            "EARLYWOOD OWNS LARGE BROKEN PORE CHAINS LATEWOOD OWNS SMALLER PORES AND THE SAW RELATION CONTROLS RAY VISIBILITY",
            "THE FINAL TWO PANELS EXCLUDE KNOT BODY DAMAGE AND INK SO COLOR RELIEF AND ROUGHNESS MUST READ AS WHITE OAK BY THEMSELVES",
        ],
    )


def _pigment_layer_sheet(material: dict[str, Any]) -> np.ndarray:
    layers = material["pigment_layers"]
    order = material["pigment_layer_order"]
    heartwood = layers["heartwood_base"]["color_linear"]
    composed = mix(
        np.broadcast_to(
            srgb_to_linear(
                np.asarray(GAP_SRGB, dtype=np.float32) / 255.0
            ),
            heartwood.shape,
        ),
        heartwood,
        material["board_mask"],
    )
    stages = [linear_to_srgb(composed)]
    for name in order[1:]:
        layer = layers[name]
        composed = mix(
            composed,
            layer["color_linear"],
            layer["coverage"],
        )
        stages.append(linear_to_srgb(composed))
    labels = (
        "HEARTWOOD BASE",
        "PLUS COOL OLIVE WASH",
        "PLUS WARM AMBER WASH",
        "PLUS EARLYWOOD TONE",
        "PLUS LATEWOOD TONE",
        "PLUS VESSEL TONE",
        "PLUS RAY FIGURE",
        "PLUS FIBRE TONE",
    )
    return compose_material_chapter(
        "WHITE OAK PIGMENT LAYER STACK",
        "EACH PANEL ADDS ONE SOFT COLOR LAYER AND PRESERVES THE PREVIOUS ONES",
        list(zip(labels, stages)),
        [
            "HEARTWOOD ESTABLISHES BOARD IDENTITY WHILE COOL AND WARM WASHES MOVE THROUGH EACH BOARD WITHOUT HARD PATCH BORDERS",
            "EARLYWOOD LATEWOOD VESSELS RAYS AND FIBRES RECEIVE DIFFERENT RELATED SHADES BECAUSE THEY ARE DIFFERENT TISSUES",
            "THE FINAL PANEL IS GRAIN PIGMENT ONLY IT EXCLUDES FINISH KNOT BODY DAMAGE DIRT INK AND DIRECTIONAL LIGHT",
        ],
    )


def _intra_board_shade_sheet(material: dict[str, Any]) -> np.ndarray:
    segment_index = 6
    segment_mask = np.logical_and(
        material["segment_id"] == segment_index,
        material["board_mask"] > 0.90,
    )
    y_positions, x_positions = np.where(segment_mask)
    if y_positions.size == 0:
        raise ValueError("shade proof segment has no visible pixels")
    center_u = float(
        (x_positions.min() + x_positions.max()) * 0.5
        / max(material["resolution"] - 1, 1)
    )
    center_v = float(
        (y_positions.min() + y_positions.max()) * 0.5
        / max(material["resolution"] - 1, 1)
    )
    segment_width = int(x_positions.max() - x_positions.min() + 1)
    segment_height = int(y_positions.max() - y_positions.min() + 1)
    close_fraction = (
        min(segment_width, segment_height)
        / float(material["resolution"])
        * 0.78
    )
    close_fraction = max(close_fraction, 0.025)

    def segment_close(image: np.ndarray) -> np.ndarray:
        return zoom_square(
            image,
            center_u=center_u,
            center_v=center_v,
            fraction=close_fraction,
        )

    def whole_segment(image: np.ndarray) -> np.ndarray:
        margin = max(2, int(round(material["resolution"] * 0.004)))
        y0 = max(0, int(y_positions.min()) - margin)
        y1 = min(image.shape[0], int(y_positions.max()) + margin + 1)
        x0 = max(0, int(x_positions.min()) - margin)
        x1 = min(image.shape[1], int(x_positions.max()) + margin + 1)
        crop = np.asarray(image[y0:y1, x0:x1], dtype=np.float32)
        crop_mask = segment_mask[y0:y1, x0:x1]
        isolated = np.full(crop.shape, 0.035, dtype=np.float32)
        isolated[crop_mask] = crop[crop_mask]
        side = max(isolated.shape[0], isolated.shape[1])
        square = np.full((side, side, 3), 0.035, dtype=np.float32)
        offset_y = (side - isolated.shape[0]) // 2
        offset_x = (side - isolated.shape[1]) // 2
        square[
            offset_y : offset_y + isolated.shape[0],
            offset_x : offset_x + isolated.shape[1],
        ] = isolated
        return square

    family_srgb = linear_to_srgb(
        material["segment_shade_palettes_linear"][segment_index]
    )
    palette = palette_panel(family_srgb)
    shade_bins = np.clip(
        np.rint(material["shade_index_continuous"][segment_mask]),
        0,
        material["shade_family_size"] - 1,
    ).astype(np.int32)
    counts = np.bincount(
        shade_bins,
        minlength=material["shade_family_size"],
    ).astype(np.float32)
    counts /= max(float(counts.max()), 1.0)
    usage = np.full((256, 256, 3), 0.035, dtype=np.float32)
    bar_width = 256.0 / float(material["shade_family_size"])
    for shade_index, (color, amount) in enumerate(
        zip(family_srgb, counts, strict=True)
    ):
        x0 = int(round(shade_index * bar_width))
        x1 = max(x0 + 1, int(round((shade_index + 1) * bar_width)) - 1)
        bar_height = max(4, int(round(float(amount) * 226.0)))
        usage[244 - bar_height : 244, x0:x1] = color
        usage[244:252, x0:x1] = color * 0.66

    shade_field = scalar_tint(
        material["shade_coordinate"],
        (0.11, 0.055, 0.025),
        (0.94, 0.69, 0.37),
    )
    slow_field = scalar_tint(
        material["shade_drivers"]["slow_pigment"],
        (0.16, 0.19, 0.15),
        (0.92, 0.51, 0.20),
    )
    growth_field = scalar_tint(
        material["shade_drivers"]["growth_history"],
        (0.15, 0.09, 0.05),
        (0.93, 0.74, 0.42),
    )
    tissue_field = scalar_tint(
        material["shade_drivers"]["tissue"],
        (0.17, 0.20, 0.12),
        (0.88, 0.49, 0.25),
    )
    return compose_material_chapter(
        "TWENTY SHADES INSIDE ONE PLANK",
        "SEGMENT 06 ONLY NEIGHBORING BOARDS ARE EXCLUDED FROM EVERY MEASUREMENT",
        [
            ("20 BROWN CONTROLS", palette),
            ("ACTUAL SHADE USAGE", usage),
            (
                "WHOLE SEGMENT 06",
                whole_segment(
                    linear_to_srgb(material["growth_pigment_linear"])
                ),
            ),
            ("CONTINUOUS PALETTE READ", segment_close(shade_field)),
            ("SLOW PIGMENT DRIFT", segment_close(slow_field)),
            ("GROWTH HISTORY", segment_close(growth_field)),
            ("TISSUE INFLUENCE", segment_close(tissue_field)),
            (
                "UNLIT LAYERED CLOSE",
                segment_close(
                    linear_to_srgb(material["growth_pigment_linear"])
                ),
            ),
        ],
        [
            "THE FIRST PANEL IS THE TWENTY SHADE FAMILY OWNED BY THIS ONE SEGMENT THE SECOND PROVES WHICH CONTROLS ITS PIXELS ACTUALLY VISIT",
            "THE PALETTE COORDINATE IS CONTINUOUS BETWEEN CONTROLS SO THE RESULT HAS SOFT COLOR MIGRATION RATHER THAN TWENTY POSTERIZED STRIPES",
            "SLOW PIGMENT DRIFT CARRIES THE LARGE COLOR MASSES GROWTH HISTORY MODULATES THEM AND TISSUE ADDS LOCAL EARLYWOOD LATEWOOD VESSEL RAY AND FIBRE DIFFERENCES",
            "THE WHOLE SEGMENT AND FINAL CLOSE SHOW THE SAME PLANK WITHOUT DAMAGE KNOT PIGMENT FINISH DIRT INK LIGHTING OR NEIGHBORING BOARD COLOR",
        ],
    )


def _damage_causality_sheet(material: dict[str, Any]) -> np.ndarray:
    damage_types = np.stack(
        [
            material["check_mask"],
            material["splinter_cut_mask"],
            material["scratch_mask"],
        ],
        axis=-1,
    )
    final_preview = material["preview_srgb"]
    joint_habitat = 1.0 - smoothstep(
        0.0,
        0.08,
        material["joint_edge_distance"],
    )
    knot_habitat = np.stack(
        [
            material["knot_check_mask"],
            material["knot_influence"],
            material["knot_mask"],
        ],
        axis=-1,
    )
    end_habitat = np.stack(
        [
            material["end_check_mask"],
            joint_habitat,
            np.zeros_like(joint_habitat),
        ],
        axis=-1,
    )
    splinter_components = np.stack(
        [
            material["splinter_cut_mask"],
            material["splinter_lip_mask"],
            material["finish_loss_mask"]
            * material["splinter_cut_mask"],
        ],
        axis=-1,
    )
    scratch_components = np.stack(
        [
            material["scratch_mask"],
            material["finish_loss_mask"],
            np.zeros_like(material["scratch_mask"]),
        ],
        axis=-1,
    )
    damage_delta = material["damage_height_delta_m"]
    delta_scale = max(float(np.max(np.abs(damage_delta))), 1.0e-9)
    normalized_delta = np.clip(damage_delta / delta_scale, -1.0, 1.0)
    damage_height_rgb = np.full(
        (*damage_delta.shape, 3),
        0.08,
        dtype=np.float32,
    )
    damage_height_rgb = mix(
        damage_height_rgb,
        np.broadcast_to(
            np.asarray([0.92, 0.18, 0.12], dtype=np.float32),
            damage_height_rgb.shape,
        ),
        np.clip(-normalized_delta, 0.0, 1.0),
    )
    damage_height_rgb = mix(
        damage_height_rgb,
        np.broadcast_to(
            np.asarray([0.20, 0.72, 0.94], dtype=np.float32),
            damage_height_rgb.shape,
        ),
        np.clip(normalized_delta, 0.0, 1.0),
    )

    def mask_center(mask: np.ndarray) -> tuple[float, float]:
        y, x = np.unravel_index(int(np.argmax(mask)), mask.shape)
        return (
            float(x) / max(mask.shape[1] - 1, 1),
            float(y) / max(mask.shape[0] - 1, 1),
        )

    def detail(
        image: np.ndarray,
        center: tuple[float, float],
    ) -> np.ndarray:
        return zoom_square(
            image,
            center_u=center[0],
            center_v=center[1],
            fraction=0.15,
        )

    knot_center = mask_center(material["knot_check_mask"])
    end_center = mask_center(material["end_check_mask"])
    splinter_center = mask_center(material["splinter_cut_mask"])
    scratch_center = mask_center(material["scratch_mask"])
    return compose_material_chapter(
        "AUTHORED DAMAGE CAUSALITY",
        "EVERY MARK NAMES ITS ORIGIN AND CHANGES ONLY THE CHANNELS IT EARNS",
        [
            ("CHECK SPLINTER SCRATCH", damage_types),
            ("SIGNED HEIGHT DELTA", damage_height_rgb),
            ("KNOT CHECK FINAL", detail(final_preview, knot_center)),
            ("KNOT CHECK HABITAT", detail(knot_habitat, knot_center)),
            ("END CHECK FINAL", detail(final_preview, end_center)),
            ("END CHECK HABITAT", detail(end_habitat, end_center)),
            (
                "SPLINTER CUT LIP",
                detail(splinter_components, splinter_center),
            ),
            (
                "SCRATCH FINISH LOSS",
                detail(scratch_components, scratch_center),
            ),
        ],
        [
            "RED CHECKS GREEN SPLINTERS AND BLUE SCRATCHES ARE AUTHORED EVENTS RATHER THAN A GLOBAL DAMAGE NOISE",
            "KNOT CHECKS BEGIN INSIDE A NAMED BRANCH END CHECKS BEGIN AT A SEGMENT END AND SPLINTERS BEGIN AT A LONG EDGE",
            "THE SPLINTER PAIRS A RECESSED CUT WITH A RAISED FIBRE LIP WHILE MOST SCRATCH ENERGY LIVES IN FINISH AND ROUGHNESS",
        ],
    )


def _groove_specimen_sheet(material: dict[str, Any]) -> np.ndarray:
    state_masks = np.stack(
        [
            material["compressed_wood_mask"],
            material["exposed_wood_mask"],
            material["dirt_deposit_mask"],
        ],
        axis=-1,
    )
    state_masks = np.power(
        np.clip(state_masks, 0.0, 1.0),
        0.58,
    ).astype(np.float32)
    knot_check = material["knot_check_mask"]
    exposed_check = knot_check * (
        1.0 - material["knot_mask"] * 0.92
    )
    weight = exposed_check
    coordinate_y, coordinate_x = np.indices(weight.shape, dtype=np.float32)
    weight_sum = max(float(weight.sum()), 1.0e-8)
    center_u = float((coordinate_x * weight).sum() / weight_sum) / max(
        knot_check.shape[1] - 1, 1
    )
    center_v = float((coordinate_y * weight).sum() / weight_sum) / max(
        knot_check.shape[0] - 1, 1
    )

    def focal(image: np.ndarray) -> np.ndarray:
        return zoom_square(
            image,
            center_u=center_u,
            center_v=center_v,
            fraction=0.045,
        )

    return compose_material_chapter(
        "ENLARGED GROOVE MATERIAL SPECIMEN",
        "ONE CUT PASSES THROUGH PIGMENT FINISH FIBRE COMPRESSION AND RETAINED DIRT",
        [
            (
                "HEARTWOOD PIGMENT",
                focal(linear_to_srgb(material["heartwood_pigment_linear"])),
            ),
            (
                "GROWTH COLOR BLEND",
                focal(linear_to_srgb(material["growth_pigment_linear"])),
            ),
            (
                "AMBER FINISH DEPTH",
                focal(gray_rgb(material["finish_thickness"])),
            ),
            ("COMPRESSION EXPOSURE DIRT", focal(state_masks)),
            (
                "UNLIT MATERIAL COLOR",
                focal(
                    linear_to_srgb(material["groove_state_color_linear"])
                ),
            ),
            (
                "FRONTAL WHITE LIGHT",
                focal(material["white_light_frontal_srgb"]),
            ),
            (
                "SIDE WHITE LIGHT",
                focal(material["white_light_side_srgb"]),
            ),
            (
                "GRAZING WHITE LIGHT",
                focal(material["white_light_grazing_srgb"]),
            ),
        ],
        [
            "RED IS COMPRESSED WOOD GREEN IS PALE EXPOSED FIBRE AND BLUE IS DIRT RETAINED INSIDE THE RECESSED CORE",
            "THE UNLIT PANEL CONTAINS MATERIAL COLOR ONLY THE THREE FINAL PANELS CHANGE ONE PURE WHITE LIGHT DIRECTION",
            "THE RAISED LIP SHOULD BRIGHTEN AND TURN OVER WITH THE LIGHT WHILE THE CAVITY HOLDS DIRT COLOR WITHOUT A BAKED SHADOW",
        ],
    )


def _wood_material_book(
    material: dict[str, Any],
) -> tuple[np.ndarray, list[dict[str, Any]]]:
    base_color = linear_to_srgb(material["base_color_linear"])
    normal_rgb = material["normal"] * 0.5 + 0.5
    height_rgb = gray_rgb(normalized_range(material["height_m"]))
    tiled_base = np.tile(base_color, (2, 2, 1))
    tiled_lit = np.tile(material["preview_srgb"], (2, 2, 1))
    variations = _pattern_variation_sheet(material)
    edge_scale = max(
        material["parameters"]["bevel_width_m"] * 4.0,
        1.0e-6,
    )
    board_edge = np.clip(
        material["board_edge_distance"] / edge_scale,
        0.0,
        1.0,
    )
    joint_edge = np.clip(
        material["joint_edge_distance"] / edge_scale,
        0.0,
        1.0,
    )
    edge_field = np.clip(
        material["edge_distance"] / edge_scale,
        0.0,
        1.0,
    )
    character_rgb = np.stack(
        [
            material["knot_mask"],
            material["check_mask"],
            np.maximum(
                material["splinter_cut_mask"],
                material["scratch_mask"],
            ),
        ],
        axis=-1,
    )
    palette_image = palette_panel(
        np.asarray(WOOD_PALETTE_SRGB, dtype=np.float32) / 255.0,
        np.asarray([0.08, 0.18, 0.28, 0.22, 0.08, 0.16]),
    )
    center = material["resolution"] // 2
    board_profile = profile_panel(
        [
            board_edge[center],
            material["board_mask"][center],
            normalized_range(material["height_m"][center]),
        ],
        [
            (0.96, 0.72, 0.24),
            (0.30, 0.74, 0.92),
            (0.94, 0.34, 0.22),
        ],
    )
    joint_profile = profile_panel(
        [
            joint_edge[:, center],
            material["board_mask"][:, center],
            normalized_range(material["height_m"][:, center]),
        ],
        [
            (0.96, 0.72, 0.24),
            (0.30, 0.74, 0.92),
            (0.94, 0.34, 0.22),
        ],
    )
    grain_height = material["grain_height_delta_m"]
    character_height = material["character_height_delta_m"]
    pattern = material["pattern"]

    chapter_specs = [
        {
            "title": "AUTHORED LAYOUT",
            "filename": "01_authored_layout.png",
            "summary": (
                f"{pattern['board_count']} BOARDS AND "
                f"{pattern['segment_count']} SEGMENTS FORM THE SENTENCE"
            ),
            "hero": _identity_colors(material["segment_id"]),
            "subtitle": "JOINERY AND RHYTHM EXIST BEFORE GRAIN",
            "panels": [
                ("BOARD IDENTITIES", _identity_colors(material["board_id"])),
                (
                    "SEGMENT IDENTITIES",
                    _identity_colors(material["segment_id"]),
                ),
                (
                    "LOCAL ACROSS",
                    scalar_tint(
                        material["local_u"],
                        (0.08, 0.20, 0.30),
                        (0.94, 0.62, 0.20),
                    ),
                ),
                (
                    "LOCAL ALONG",
                    scalar_tint(
                        material["local_v"],
                        (0.20, 0.10, 0.28),
                        (0.34, 0.82, 0.68),
                    ),
                ),
                ("EDGE DISTANCE", gray_rgb(edge_field)),
                ("BOARD MASK", gray_rgb(material["board_mask"])),
                ("VARIATION FAMILY", variations),
                ("COMPILED READ", base_color),
            ],
            "notes": [
                f"{pattern['board_count']} BOARDS CROSS THE TILE WHILE {pattern['segment_count']} CYCLIC SEGMENTS BREAK THEIR LENGTHS INTO HUMAN SCALE",
                "EACH PIECE OWNS LOCAL ACROSS AND ALONG COORDINATES SO GRAIN KNOTS CHECKS AND SPLINTERS FOLLOW WOOD INSTEAD OF THE IMAGE FRAME",
                f"VARIATION {pattern['variation']} CHANGES WIDTH LENGTH AND STAGGER WITHOUT ABANDONING THE AUTHORED LAYOUT FAMILY",
            ],
        },
        {
            "title": "BOARDS AND JOINTS",
            "filename": "02_boards_and_joints.png",
            "summary": "LONG GAPS END JOINTS AND BEVELS SHARE ONE VOLUME",
            "hero": gray_rgb(material["board_mask"]),
            "subtitle": "THE PLANK EDGE IS CARVED NOT DRAWN",
            "panels": [
                ("LONG EDGE", gray_rgb(board_edge)),
                ("END JOINT", gray_rgb(joint_edge)),
                ("NEAREST EDGE", gray_rgb(edge_field)),
                ("BOARD MASK", gray_rgb(material["board_mask"])),
                (
                    "SEGMENT RAISE",
                    scalar_tint(
                        material["segment_raise_m"],
                        (0.14, 0.18, 0.28),
                        (0.94, 0.68, 0.22),
                        normalize=True,
                    ),
                ),
                (
                    "BASE WOOD HEIGHT",
                    gray_rgb(normalized_range(material["wood_height_base_m"])),
                ),
                ("ACROSS PROFILE", board_profile),
                ("ALONG PROFILE", joint_profile),
            ],
            "notes": [
                f"THE HALF GAP IS {material['parameters']['gap_half_width_m']:.4f} M AND THE BEVEL TRANSITION IS {material['parameters']['bevel_width_m']:.4f} M",
                "LONG EDGES AND END JOINTS ARE MEASURED SEPARATELY THEN MERGED BY THE NEAREST DISTANCE SO THEIR CORNERS CANNOT DISAGREE",
                "PER SEGMENT RAISE LETS ONE BOARD CATCH LIGHT BEFORE ANY COLOR OR GRAIN IS ASKED TO IMPLY DEPTH",
            ],
        },
        {
            "title": "GRAIN ANATOMY",
            "filename": "03_grain_anatomy.png",
            "summary": "RING HISTORY PORES RAYS FIBRES AND RESTS SHARE ONE LOG",
            "hero": gray_rgb(material["grain_mask"]),
            "subtitle": "WHITE OAK ANATOMY EXISTS BEFORE DAMAGE OR INK",
            "panels": [
                (
                    "RING SPACING",
                    scalar_tint(
                        material["ring_spacing_multiplier"],
                        (0.12, 0.28, 0.62),
                        (0.96, 0.66, 0.20),
                        normalize=True,
                    ),
                ),
                ("EARLYWOOD", gray_rgb(material["earlywood_mask"])),
                ("LATEWOOD", gray_rgb(material["latewood_mask"])),
                ("VESSEL PORES", gray_rgb(material["vessel_mask"])),
                ("MEDULLARY RAYS", gray_rgb(material["ray_mask"])),
                ("FIBRE BUNDLES", gray_rgb(material["fibre_bundle_mask"])),
                ("PROTECTED RESTS", gray_rgb(material["rest_mask"])),
                ("HEIGHT CUT", signed_delta_rgb(grain_height)),
            ],
            "notes": [
                "AUTHORED PITH DEPTH SAW ANGLE PITH DRIFT CATHEDRAL ASPECT AND GROWTH EPOCHS PREVENT ONE PERFECT NESTED STRIPE FORMULA",
                "RING POROUS EARLYWOOD OWNS THE LARGER BROKEN VESSEL CHAINS WHILE THE SAW TO PITH RELATION CONTROLS OAK RAY VISIBILITY",
                f"THREE BROKEN LONG AXIS FIBRE FAMILIES LEAVE AUTHORED RESTS AND RESOLVED ANATOMY REMOVES AT MOST {material['parameters']['grain_relief_m']:.5f} M",
            ],
        },
        {
            "title": "CHARACTER MARKS",
            "filename": "04_character_marks.png",
            "summary": "KNOT CHECK SPLINTER AND SCRATCH ENTER BY NAMED CAUSES",
            "hero": character_rgb,
            "subtitle": "MEMORY IS COMPOSED NOT SCATTERED",
            "panels": [
                ("KNOT MASK", gray_rgb(material["knot_mask"])),
                ("KNOT CHECK", gray_rgb(material["knot_check_mask"])),
                ("END CHECK", gray_rgb(material["end_check_mask"])),
                (
                    "SPLINTER CUT LIP",
                    np.stack(
                        [
                            material["splinter_cut_mask"],
                            material["splinter_lip_mask"],
                            np.zeros_like(material["splinter_cut_mask"]),
                        ],
                        axis=-1,
                    ),
                ),
                ("SCRATCH", gray_rgb(material["scratch_mask"])),
                ("CHARACTER STACK", character_rgb),
                ("HEIGHT REMOVED", signed_delta_rgb(character_height)),
                ("AFTER CHARACTER", base_color),
            ],
            "notes": [
                f"THE RECIPE OWNS {pattern['damage']['knot_check']} KNOT CHECK {pattern['damage']['end_check']} END CHECK {pattern['damage']['edge_splinter']} SPLINTER AND {pattern['damage']['scratch']} SCRATCH EVENTS",
                "KNOT CHECKS NAME A BRANCH END CHECKS TOUCH A JOINT SPLINTERS TOUCH A LONG EDGE AND SCRATCHES NAME A USE EVENT",
                f"THE DEEPEST AUTHORED DAMAGE IS {material['parameters']['deepest_damage_m']:.5f} M AND THE SPLINTER ALONE EARNS A RAISED FIBRE LIP",
            ],
        },
        {
            "title": "HEIGHT STACK",
            "filename": "05_height_stack.png",
            "summary": "RAISE UNDULATION FIBRE AND DAMAGE ACCUMULATE IN METRES",
            "hero": height_rgb,
            "subtitle": "EVERY NORMAL BEND HAS A NAMED SOURCE",
            "panels": [
                (
                    "SEGMENT RAISE",
                    signed_delta_rgb(material["segment_raise_m"]),
                ),
                ("MACRO FIELD", gray_rgb(material["macro"])),
                ("SURFACE FIELD", gray_rgb(material["surface_noise"])),
                (
                    "BASE HEIGHT",
                    gray_rgb(normalized_range(material["wood_height_base_m"])),
                ),
                ("GRAIN DELTA", signed_delta_rgb(grain_height)),
                ("CHARACTER DELTA", signed_delta_rgb(character_height)),
                ("FINAL HEIGHT", height_rgb),
                ("FINAL NORMAL", normal_rgb),
            ],
            "notes": [
                f"THE WOOD PLANE BEGINS {material['parameters']['board_raise_m']:.4f} M ABOVE THE GAP BEFORE SEGMENT RAISE AND BROAD UNDULATION",
                "GRAIN AND CHARACTER MARKS ARE EXPLICIT SUBTRACTIONS THE FINAL NORMAL IS DERIVED FROM THE COMPLETE METRE HEIGHT",
                "THIS STACK MAKES REVISION HONEST A DEEPER CHECK CHANGES HEIGHT NORMAL CAVITY AND LIGHT RESPONSE THROUGH ONE SOURCE",
            ],
        },
        {
            "title": "COLOR SCRIPT",
            "filename": "06_color_script.png",
            "summary": "BOARD PIGMENT LIGHT WASH GRAIN AND INK ARRIVE IN ORDER",
            "hero": base_color,
            "subtitle": "COLOR DESCRIBES FIBRE AGE AND HAND WITHOUT BAKED LIGHT",
            "panels": [
                ("PALETTE WEIGHT", palette_image),
                (
                    "SEGMENT PIGMENT",
                    linear_to_srgb(material["segment_pigment_linear"]),
                ),
                (
                    "LIGHT WASH",
                    gray_rgb(
                        smoothstep(0.58, 0.86, material["macro"])
                        * material["board_mask"]
                    ),
                ),
                (
                    "PAINTED PIGMENT",
                    linear_to_srgb(material["painted_pigment_linear"]),
                ),
                ("GRAIN INK", gray_rgb(material["grain_mask"])),
                (
                    "GRAINED PIGMENT",
                    linear_to_srgb(material["grained_pigment_linear"]),
                ),
                ("CHARACTER INK", character_rgb),
                ("FINAL BASE COLOR", base_color),
            ],
            "notes": [
                "WEIGHTED BROWNS GIVE NEIGHBOURING SEGMENTS DISTINCT PIGMENT BEFORE A QUIET WARM WASH CONNECTS THE FAMILY",
                "GRAIN INK IS TRANSLUCENT KNOT AND CHECK INK IS STRONGER AND END ACCENT IS RESTRAINED SO NO MASK DOES EVERY JOB",
                "THE BASE COLOR CONTAINS MATERIAL HISTORY BUT NO FIXED SHADOW DIRECTION LEAVING ROOM FOR A CEL LIGHTING RAMP",
            ],
        },
        {
            "title": "MATERIAL RESPONSE",
            "filename": "07_material_response.png",
            "summary": "CAVITY AO ROUGHNESS AND NORMAL ANSWER THE SAME HEIGHT",
            "hero": normal_rgb,
            "subtitle": "RESPONSE IS A CONSEQUENCE OF CONSTRUCTION",
            "panels": [
                ("FINAL HEIGHT", height_rgb),
                ("CAVITY", gray_rgb(material["cavity"])),
                ("AMBIENT OCCLUSION", gray_rgb(material["ao"])),
                ("ROUGHNESS BASE", gray_rgb(material["roughness_base"])),
                ("ROUGHNESS FINAL", gray_rgb(material["roughness"])),
                ("FINAL NORMAL", normal_rgb),
                ("ORM PACKING", material["orm"]),
                ("LIT RESPONSE", material["preview_srgb"]),
            ],
            "notes": [
                "CAVITY SEARCHES THE FINAL HEIGHT FOR LOCAL RECESSES THEN AMBIENT OCCLUSION DARKENS JOINTS AND CUTS WITHOUT STAINING BASE COLOR",
                "ROUGHNESS BEGINS PER SEGMENT THEN FIBRE KNOT CHECK SPLINTER SCRATCH AND SURFACE VARIATION EACH ADD A NAMED CHANGE",
                "METALLIC REMAINS ZERO THE MATERIAL EARNS INTEREST THROUGH FORM PIGMENT AND BROKEN REFLECTION NOT A MISUSED CHANNEL",
            ],
        },
        {
            "title": "SCALE FINAL READ",
            "filename": "08_scale_seams_final_read.png",
            "summary": "THE FINISHED FLOOR MUST SURVIVE DISTANCE REPEAT AND CLOSEUP",
            "hero": material["preview_srgb"],
            "subtitle": "A GOOD TILE BECOMES A BELIEVABLE SURFACE",
            "panels": [
                ("BASE TILE", base_color),
                ("BASE REPEAT 2X2", tiled_base),
                ("LIT TILE", material["preview_srgb"]),
                ("LIT REPEAT 2X2", tiled_lit),
                ("SEAM BORDER", seam_frame(base_color)),
                ("HEIGHT BORDER", seam_frame(height_rgb)),
                ("LAYOUT FAMILY", variations),
                (
                    "FINAL CLOSEUP",
                    zoom_square(
                        material["preview_srgb"],
                        center_u=0.32,
                        center_v=0.52,
                    ),
                ),
            ],
            "notes": [
                f"THE TILE COVERS {material['tile_size_m']:.2f} M AND EACH PIXEL REPRESENTS {material['meters_per_pixel']:.6f} M",
                "TWO BY TWO REPEATS EXPOSE LARGE LANDMARKS GOLD BORDERS EXPOSE THE PERIODIC CONTRACT AND THE FAMILY SHOWS PLACEMENT OPTIONS",
                "THE FINAL CLOSEUP ASKS WHETHER JOINT GRAIN CHARACTER PIGMENT AND RESPONSE STILL DESCRIBE ONE PIECE OF WOOD",
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
        "WOOD PLANK V2 MATERIAL BOOK",
        [
            (chapter["title"], chapter["hero"], chapter["summary"])
            for chapter in chapters
        ],
    )
    return index, chapters


def _wood_material_book_markdown(material: dict[str, Any]) -> str:
    parameters = material["parameters"]
    pattern = material["pattern"]
    return f"""# Wood Plank V2 Material Book

This book follows the floor from a hand-authored layout to a complete material
response. It is not a list of texture buffers. Each chapter isolates one
decision, shows the fields that carry it, and explains how that decision
survives into the final image. The selected recipe is variation
{pattern["variation"]} of `{pattern["name"]}` across a physical tile of
{material["tile_size_m"]:.2f} metres.

## Chapter 1 — Authored Layout

The floor begins as joinery, not noise. {pattern["board_count"]} boards cross
the tile and are divided into {pattern["segment_count"]} cyclic pieces. Their
widths, lengths, and offsets were authored to create a cadence of long rests
and short interruptions. The seam wraps, but the eye should read a laid floor
rather than a diagram designed merely to pass a seam test.

Each segment owns a local coordinate frame. One axis travels across the growth
rings and the other travels along the fibres. Every later feature inherits
that frame. Grain follows the board after its width changes; a knot belongs to
its piece; an end accent remains near a saw joint. This is the quiet structural
promise on which all decorative richness depends.

Variation changes the performance without rewriting the score. Width, length,
and stagger move inside bounded ranges, preserving the recognizable family
while preventing a single arrangement from becoming wallpaper.

## Chapter 2 — Boards and Joints

The spaces between boards are physical recesses. Long edges and end joints
are measured independently, then merged through the nearest distance. That
single construction yields a {parameters["gap_half_width_m"]:.4f} m half-gap
and a {parameters["bevel_width_m"]:.4f} m shoulder. At a corner, color, height,
normal, and occlusion therefore agree about where the wood ends.

A painted black line can suggest a joint from one camera, but it has no answer
for grazing light. Here the plank rises from the gap, crosses a rounded bevel,
and settles into its face. Individual segments receive a slight raise so the
floor catches light with the irregularity of boards planed, installed, and
worn by different hands.

The profile panels are deliberately unromantic evidence. One crosses the
boards; the other travels along them. They reveal whether the mask, distance,
and height describe the same cut.

## Chapter 3 — Grain Anatomy

Wood grain is a hierarchy of motion. Broad bands carry the slow record of
growth. Fine fibres interrupt those bands with a quicker rhythm. If both are
collapsed into one frequency, the surface becomes striped plastic; if they are
unrelated, it becomes television static. The material gives each family its
own warp while keeping both subordinate to the local plank frame.

The broad field owns the recognizable sweep. The fine field supplies broken
edges and small catches. Their combined mask removes up to
{parameters["grain_relief_m"]:.5f} m from height and gently darkens pigment.
Relief and color share a cause, but their strength differs: the eye can feel a
fibre in light without seeing every fibre painted as a black line.

Near a knot, the broad phase bends. That deformation is important because a
knot pasted over straight grain reads as a decal. The wood must remember the
branch before the artist adds the dark mark.

## Chapter 4 — Character Marks

Character is assigned sparingly at the layout level:
{pattern["characters"]["knot"]} knot segments,
{pattern["characters"]["clear"]} clear segments. Empty boards are part of the
composition. Without quiet pieces, every special mark competes for attention
and the floor loses scale.

Damage is authored as {pattern["damage"]["knot_check"]} knot check,
{pattern["damage"]["end_check"]} end checks,
{pattern["damage"]["edge_splinter"]} edge splinter, and
{pattern["damage"]["scratch"]} use scratches. Each record names a source,
segment, tapered path, physical width, depth, and optional raised lip.

The deepest damage removes {parameters["deepest_damage_m"]:.5f} m. Knot checks
begin inside a named branch, end checks begin at a segment end, and the
splinter begins at a long edge. Character reads as history held inside the
plank rather than trenches stamped on top.

## Chapter 5 — Height Stack

Height is built as an accountable stack. The wood begins
{parameters["board_raise_m"]:.4f} m above the gap. Per-segment raise changes
the plane. A broad undulation keeps the face from becoming mathematically
flat; a much smaller surface field removes sterile perfection. Grain then
scores the face, and character marks make their deeper, rarer cuts.

Every amount is expressed in metres. A 512-pixel study and a 2048-pixel
delivery can therefore describe the same physical floor instead of changing
the apparent depth whenever resolution changes.

The normal is derived only after the complete height has been assembled.
There is no independent normal noise with a different opinion about the
surface. This is both a technical contract and an artistic discipline: every
bend in light must be able to name the form that caused it.

## Chapter 6 — Color Script

Color begins with a weighted family of browns assigned per segment. That
choice gives neighbouring boards individual pigment before any grain,
lighting, or damage appears. A quiet warm wash moves through the macro field,
connecting the pieces without erasing their identities.

The passes then arrive in order. Grain receives translucent ink. Knots and
checks receive stronger ink because they expose denser or deeper material.
End accents remain restrained, suggesting contact without outlining every
joint like a comic panel border. The sequence matters because it preserves
control: an artist can soften the grain without washing out the knots.

No directional shadow is baked into base color. The map describes wood,
pigment, and age, leaving scene light free to become graphic. This restraint
is essential for a cel-shaded treatment, where broad illumination ramps need
a surface that enriches the light rather than contradicting it.

## Chapter 7 — Material Response

The completed height is searched for local cavities. Those recesses inform
ambient occlusion, darkening gaps and cuts as a response channel while keeping
the base-color pigment clean. The tangent-space normal carries the same form
into direct light.

Roughness begins as a per-segment property, because boards do not all accept
wear and finish identically. Surface variation, grain, knots, checks,
splinters, and scratches each make a named change. The result is broken
reflection with a readable cause, not arbitrary noise sprinkled into the green
channel.

The packed ORM map keeps ambient occlusion in red, roughness in green, and
metallic at zero in blue. Wood does not become more interesting by pretending
to be metal. Its richness comes from the agreement of form, fibre, pigment,
and restrained reflection.

## Chapter 8 — Scale, Seams, and Final Read

The tile spans {material["tile_size_m"]:.2f} m, and each pixel represents
{material["meters_per_pixel"]:.6f} m. Periodic fields protect the mathematical
border. Gold seam frames make that contract visible, while two-by-two repeats
ask a different question: does a memorable knot or board rhythm announce the
tile from across the room?

The variation family is the beginning of a placement system. Compatible
layouts can be selected, rotated, or distributed so repetition becomes a
designed rhythm rather than a hidden flaw. Even so, the chosen tile must stand
on its own; variation cannot rescue weak joinery or uncontrolled landmarks.

The final close study gathers every promise. A joint should feel cut into the
floor. Grain should bend with the board. A knot should feel grown rather than
pasted. Pigment should leave enough silence for bold cel light. At distance,
the boards form a readable cadence; up close, the surface rewards attention
without dissolving into static.
"""


def _tiling_sheet(material: dict[str, Any]) -> np.ndarray:
    tile_size = min(256, int(material["resolution"]))
    base = proof_resize(
        linear_to_srgb(material["base_color_linear"]), tile_size
    )
    preview = proof_resize(material["preview_srgb"], tile_size)
    base_grid = np.tile(base, (4, 4, 1))
    preview_grid = np.tile(preview, (4, 4, 1))
    gap = 18
    label_height = 34
    height = tile_size * 4 + label_height
    width = tile_size * 8 + gap
    sheet = np.full((height, width, 3), 0.045, dtype=np.float32)
    sheet[: tile_size * 4, : tile_size * 4] = base_grid
    sheet[: tile_size * 4, tile_size * 4 + gap :] = preview_grid
    draw_text(
        sheet,
        4,
        tile_size * 4 + 9,
        "BASE COLOR 4X4",
        color=(0.88, 0.88, 0.84),
        scale=2,
    )
    draw_text(
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
    colors = np.asarray(
        [
            [0.52, 0.28, 0.13],
            [0.66, 0.39, 0.18],
            [0.75, 0.51, 0.25],
            [0.47, 0.33, 0.22],
            [0.71, 0.44, 0.17],
            [0.58, 0.37, 0.16],
            [0.78, 0.58, 0.31],
            [0.46, 0.25, 0.12],
            [0.62, 0.47, 0.29],
            [0.70, 0.34, 0.16],
            [0.50, 0.39, 0.25],
            [0.80, 0.49, 0.22],
            [0.61, 0.30, 0.14],
            [0.72, 0.55, 0.33],
        ],
        dtype=np.float32,
    )
    for variation in range(columns * rows):
        layout = generate_plank_layout(recipe, variation=variation)
        raster = rasterize_plank_pattern(
            layout,
            resolution=panel_size,
            tile_size_m=material["tile_size_m"],
        )
        fill = colors[raster["segment_id"] % len(colors)]
        board = smoothstep(0.004, 0.012, raster["edge_distance"])
        panel = mix(np.full_like(fill, 0.065), fill, board)
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
        draw_text(
            sheet,
            x + 3,
            y + panel_size + 5,
            f"VAR {variation}",
            color=(0.88, 0.88, 0.84),
            scale=2,
        )
    return sheet


def _identity_colors(identity: np.ndarray) -> np.ndarray:
    red = ((identity.astype(np.int32) * 73 + 41) % 211 + 30) / 255.0
    green = ((identity.astype(np.int32) * 47 + 83) % 197 + 36) / 255.0
    blue = ((identity.astype(np.int32) * 97 + 19) % 181 + 42) / 255.0
    return np.stack([red, green, blue], axis=-1).astype(np.float32)


def _arguments(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate the layered wood_plank_v2 material"
    )
    parser.add_argument("--resolution", type=int, default=DEFAULT_RESOLUTION)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED)
    parser.add_argument("--tile-size-m", type=float, default=DEFAULT_TILE_SIZE_M)
    parser.add_argument(
        "--pattern-recipe",
        default=str(DEFAULT_PATTERN_RECIPE_PATH),
        help="authored plank-layout recipe",
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
        pattern_recipe_path=arguments.pattern_recipe,
        pattern_variation=arguments.pattern_variation,
    )
    manifest = write_material_package(material, arguments.out)
    print(
        "wood_plank_v2 generated:",
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
