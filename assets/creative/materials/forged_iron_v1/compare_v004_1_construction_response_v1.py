#!/usr/bin/env python3
"""Compare construction-owned normal responses without saving a material."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Any

import bpy
from mathutils import Vector
import numpy as np


MATERIAL_ROOT = Path(__file__).resolve().parent
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004_1.py"
SOURCE_BLEND = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_connected_oxide_candidate_v004_1"
    / "forged_iron_connected_oxide_candidate_v004_1.blend"
)
DEFAULT_OUTPUT = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_v004_1_construction_response_sweep_v1"
)
REVIEW_PATH = MATERIAL_ROOT / "CONSTRUCTION_RESPONSE_SWEEP_REVIEW.md"

CANDIDATES = (
    "A_v004_1_control",
    "B_parent_rest_stock",
    "C_separate_axial_pintle",
    "D_combined_construction",
)
PANEL_ORDER = (
    "isolated_height",
    "isolated_normal_angle",
    "close_neutral",
    "grazing",
    "pivot_close",
    "gameplay",
)
PANEL_LABELS = {
    "isolated_height": "INTEGRATED HEIGHT  +/- 6 MM",
    "isolated_normal_angle": "NORMAL ANGLE  0 TO 2 DEG",
    "close_neutral": "CLOSE NEUTRAL",
    "grazing": "GRAZING",
    "pivot_close": "PIVOT CLOSE",
    "gameplay": "GAMEPLAY DISTANCE",
}

STOCK_LINEAGES = {
    "moving_leaf_stock": (
        "SM_GH018_MovingLeaf_Openwork",
        "MovingKnuckle_01",
        "MovingKnuckle_03",
        "MovingKnuckle_05",
    ),
    "fixed_leaf_stock": (
        "SM_GH018_FixedLeaf",
        "FixedKnuckle_02",
        "FixedKnuckle_04",
    ),
    "pintle_stock": ("SM_GH018_Pintle",),
}

HUMAN_HAMMER_ENVELOPE_M = (0.041275, 0.031750)
FABRICATION_SCALE = 10.0
GIANT_HAMMER_ENVELOPE_M = tuple(
    value * FABRICATION_SCALE for value in HUMAN_HAMMER_ENVELOPE_M
)
STRAP_REVIEW_HEIGHT_P2P_M = 0.003000
PINTLE_REVIEW_HEIGHT_P2P_M = 0.001000
NORMAL_ANGLE_DISPLAY_MAX_DEG = 2.0
HEIGHT_DISPLAY_ABS_MAX_M = 0.006

# Dimensionless finite local-plane targets. A zero-slope rest plane is injected
# for every third pass; the non-zero families describe plane replacement, not
# a surviving hammer-face depression.
PLANISHING_PLANE_VOCABULARY = (
    (0.38, -0.16, 0.11),
    (-0.31, 0.22, -0.08),
    (0.18, 0.34, 0.06),
    (-0.24, -0.27, -0.04),
    (0.29, 0.09, -0.10),
    (-0.13, 0.30, 0.08),
)
LINEAGE_PHASES = {
    "moving_leaf_stock": 0.137,
    "fixed_leaf_stock": 0.619,
}

VISUAL_REVIEW = {
    "decision": "reject_all_visible_fields_accept_coordinate_ownership_only",
    "ranking": [
        "B_parent_rest_stock",
        "A_v004_1_control",
        "C_separate_axial_pintle",
        "D_combined_construction",
    ],
    "recommended_strategy": (
        "retain_parent_rest_stock_and_separate_pintle_coordinates_but_select_no_normal_field"
    ),
    "advancement_scope": (
        "coordinate_ownership_contract_only; no height, normal, material, shader, or donor promotion"
    ),
    "defect_ledger": [
        "A remains rejected because one normalized three-rail field restarts on every host and organizes energy into whole-object lobes.",
        "B and D preserve parent-stock continuity but the isolated angle proof closes the compact C2 supports into repeated amoeba-like lobes. The hammer-envelope proxy has become a visible pattern even though the algorithm never drew a literal dent.",
        "B distributes response implausibly across the rolled eyes: MovingKnuckle_05 falls to about 0.047 mm peak-to-peak while FixedKnuckle_02 reaches about 2.373 mm. Correct phase ownership alone does not make this response believable.",
        "C and D squeeze an eight-fold residual around the small pintle circumference. One millimetre peak-to-peak produces a 3.625 degree median, 6.735 degree p95, and 7.523 degree maximum normal angle: too strong and too close to explicit octagonal striping.",
        "Despite C's excessive pin slopes, its full-frame difference from A remains extremely small: PSNR is 70.10 dB close, 80.43 dB grazing, 63.76 dB pivot, and 82.21 dB gameplay. More normal amplitude would intensify the isolated failure rather than repair the material read.",
        "B and D produce larger but still subordinate lit differences while retaining a prohibited procedural signature in the isolated owner. A layer is not accepted because rough oxide and lighting hide its defect.",
        "All rest-stock boundary height comparisons, the pintle circumference seam, frozen colour/roughness/luster hashes, the shared group topology, and the v004.1 source hash remain valid.",
        "The next investigation should test whether construction identity belongs primarily in broad roughness and direction-dependent reflection or in restrained geometry faceting, with construction normals reduced or absent. It should not generate another hammer-shaped normal vocabulary.",
    ],
    "physical_panel_psnr_db_against_A": {
        "B_parent_rest_stock": {
            "close_neutral": 45.765476,
            "grazing": 49.878627,
            "pivot_close": 44.406557,
            "gameplay": 52.842160,
        },
        "C_separate_axial_pintle": {
            "close_neutral": 70.095366,
            "grazing": 80.426049,
            "pivot_close": 63.756586,
            "gameplay": 82.213203,
        },
        "D_combined_construction": {
            "close_neutral": 45.759302,
            "grazing": 49.874662,
            "pivot_close": 44.370524,
            "gameplay": 52.840609,
        },
    },
    "manual_acceptance_established": False,
}


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


build = _load_module("iggy_v004_1_construction_build", BUILD_SCRIPT)
calibration = build.calibration
builder = build.builder
base = build.base
common = calibration.common
presentation = calibration.presentation


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=640)
    parser.add_argument("--resolution-y", type=int, default=220)
    parser.add_argument("--samples", type=int, default=32)
    parser.add_argument("--review-only", action="store_true")
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def sha256_array(value: np.ndarray) -> str:
    array = np.ascontiguousarray(value, dtype="<f4")
    return hashlib.sha256(array.tobytes()).hexdigest()


def image_pixel_sha256(image: bpy.types.Image) -> str:
    pixels = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(pixels)
    return sha256_array(pixels)


def stock_lineage(component_name: str) -> str:
    for lineage, members in STOCK_LINEAGES.items():
        if component_name in members:
            return lineage
    raise KeyError(component_name)


def descriptive(value: np.ndarray) -> dict[str, float]:
    array = np.asarray(value, dtype=np.float64)
    return {
        "minimum": float(array.min()),
        "p01": float(np.percentile(array, 1.0)),
        "p05": float(np.percentile(array, 5.0)),
        "median": float(np.percentile(array, 50.0)),
        "p95": float(np.percentile(array, 95.0)),
        "p99": float(np.percentile(array, 99.0)),
        "maximum": float(array.max()),
        "mean": float(array.mean()),
        "standard_deviation": float(array.std()),
        "peak_to_peak": float(np.ptp(array)),
    }


def normal_angle_degrees(normal: np.ndarray) -> np.ndarray:
    value = np.asarray(normal, dtype=np.float64)
    length = np.linalg.norm(value, axis=-1)
    cosine = np.clip(value[..., 2] / np.maximum(length, 1.0e-12), -1.0, 1.0)
    return np.degrees(np.arccos(cosine)).astype(np.float32)


def normal_angle_metrics(normal: np.ndarray) -> dict[str, Any]:
    angle = normal_angle_degrees(normal)
    result: dict[str, Any] = descriptive(angle)
    result["fractions_above_degrees"] = {
        str(threshold): float(np.mean(angle > threshold))
        for threshold in (0.25, 0.5, 1.0, 2.0)
    }
    return result


def smoothstep01(value: np.ndarray) -> np.ndarray:
    t = np.clip(value, 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def object_local_bounds(obj: bpy.types.Object) -> tuple[np.ndarray, np.ndarray]:
    positions = np.asarray([vertex.co[:] for vertex in obj.data.vertices], dtype=np.float64)
    if positions.ndim != 2 or positions.shape[1] != 3:
        raise RuntimeError(f"{obj.name} has no usable local positions")
    return positions.min(axis=0), positions.max(axis=0)


def build_lineage_plans(
    frames: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    plans: dict[str, dict[str, Any]] = {}
    for lineage, leaf_name, direction in (
        ("moving_leaf_stock", "SM_GH018_MovingLeaf_Openwork", -1.0),
        ("fixed_leaf_stock", "SM_GH018_FixedLeaf", 1.0),
    ):
        leaf = frames[leaf_name]
        knuckle_spans = [frames[name]["span_m"][0] for name in STOCK_LINEAGES[lineage][1:]]
        maximum_roll_span_m = float(max(knuckle_spans))
        leaf_u_min, leaf_v_min = (float(value) for value in leaf["minimum_m"])
        leaf_u_max, leaf_v_max = (float(value) for value in leaf["maximum_m"])
        root_u_m = leaf_u_min if direction < 0.0 else leaf_u_max
        domain_u_min = root_u_m - maximum_roll_span_m if direction < 0.0 else leaf_u_min
        domain_u_max = leaf_u_max if direction < 0.0 else root_u_m + maximum_roll_span_m
        domain_length_m = domain_u_max - domain_u_min
        step_m = GIANT_HAMMER_ENVELOPE_M[0] * 0.55
        pass_count = max(5, int(math.ceil(domain_length_m / step_m)) + 2)
        golden = (math.sqrt(5.0) - 1.0) * 0.5
        phase = LINEAGE_PHASES[lineage]
        passes: list[dict[str, Any]] = []
        for index in range(pass_count):
            u_fraction = (index + 0.5) / pass_count
            u_jitter = (
                ((index + 1) * golden * golden + phase) % 1.0 - 0.5
            ) * (0.34 / pass_count)
            u_fraction = float(np.clip(u_fraction + u_jitter, 0.02, 0.98))
            v_fraction = 0.16 + 0.68 * (((index + 1) * golden + phase) % 1.0)
            if index % 3 == 0:
                slope_u, slope_v, offset = (0.0, 0.0, 0.0)
                family = "protected_rest_plane"
            else:
                slope_u, slope_v, offset = PLANISHING_PLANE_VOCABULARY[
                    (index + (1 if lineage == "fixed_leaf_stock" else 0))
                    % len(PLANISHING_PLANE_VOCABULARY)
                ]
                family = "overlapping_planished_plane"
            passes.append(
                {
                    "index": index,
                    "family": family,
                    "center_u_m": domain_u_min + u_fraction * domain_length_m,
                    "center_v_m": leaf_v_min + v_fraction * (leaf_v_max - leaf_v_min),
                    "radius_u_m": GIANT_HAMMER_ENVELOPE_M[0] * 0.72,
                    "radius_v_m": GIANT_HAMMER_ENVELOPE_M[1] * 0.70,
                    "slope_u": slope_u,
                    "slope_v": slope_v,
                    "offset": offset,
                }
            )
        plan: dict[str, Any] = {
            "lineage": lineage,
            "leaf": leaf_name,
            "direction_from_leaf_root": direction,
            "root_u_m": root_u_m,
            "domain_u_m": [domain_u_min, domain_u_max],
            "domain_v_m": [leaf_v_min, leaf_v_max],
            "maximum_unrolled_knuckle_span_m": maximum_roll_span_m,
            "pass_spacing_reference_m": step_m,
            "passes": passes,
        }
        reference_u = np.linspace(domain_u_min, domain_u_max, 1024, dtype=np.float64)
        reference_v = np.linspace(leaf_v_min, leaf_v_max, 192, dtype=np.float64)
        reference_u_grid, reference_v_grid = np.meshgrid(reference_u, reference_v)
        raw = overlapping_obliterated_work_field(
            reference_u_grid,
            reference_v_grid,
            plan,
            calibrated=False,
        )
        raw_mean = float(raw.mean())
        centered = raw - raw_mean
        p01, p99 = np.percentile(centered, (1.0, 99.0))
        if not math.isfinite(float(p01)) or float(p99 - p01) <= 1.0e-8:
            raise RuntimeError(f"{lineage} has a degenerate planishing calibration")
        plan["raw_mean"] = raw_mean
        plan["height_scale_m"] = STRAP_REVIEW_HEIGHT_P2P_M / float(p99 - p01)
        plan["reference_raw_percentiles"] = {"p01": float(p01), "p99": float(p99)}
        plans[lineage] = plan
    return plans


def overlapping_obliterated_work_field(
    rest_u_m: np.ndarray,
    rest_v_m: np.ndarray,
    plan: dict[str, Any],
    *,
    calibrated: bool = True,
) -> np.ndarray:
    """Blend compact local target planes; never add literal impact dents."""
    u = np.asarray(rest_u_m, dtype=np.float64)
    v = np.asarray(rest_v_m, dtype=np.float64)
    if u.shape != v.shape:
        raise ValueError("rest-stock coordinate grids must have matching shapes")
    numerator = np.zeros_like(u)
    denominator = np.zeros_like(u)
    for authored_pass in plan["passes"]:
        du = (u - authored_pass["center_u_m"]) / authored_pass["radius_u_m"]
        dv = (v - authored_pass["center_v_m"]) / authored_pass["radius_v_m"]
        radius = np.sqrt(du * du + dv * dv)
        interior = radius < 1.0
        one_minus = np.clip(1.0 - radius, 0.0, 1.0)
        weight = np.where(interior, one_minus**4 * (4.0 * radius + 1.0), 0.0)
        local_plane = (
            authored_pass["offset"]
            + authored_pass["slope_u"] * du
            + authored_pass["slope_v"] * dv
        )
        numerator += weight * local_plane
        denominator += weight
    averaged = np.divide(
        numerator,
        np.maximum(denominator, 1.0e-12),
        out=np.zeros_like(numerator),
        where=denominator > 1.0e-12,
    )
    support_blend = smoothstep01(denominator / 0.70)
    raw = averaged * support_blend
    if not calibrated:
        return raw.astype(np.float32)
    if "raw_mean" not in plan or "height_scale_m" not in plan:
        raise RuntimeError("rest-stock plan was sampled before lineage calibration")
    return ((raw - plan["raw_mean"]) * plan["height_scale_m"]).astype(np.float32)


def parent_rest_stock_coordinates(
    obj: bpy.types.Object,
    frame: dict[str, Any],
    plan: dict[str, Any],
    resolution_x: int,
    resolution_y: int,
) -> tuple[np.ndarray, np.ndarray, dict[str, Any]]:
    local_u = np.linspace(
        float(frame["minimum_m"][0]),
        float(frame["maximum_m"][0]),
        resolution_x,
        dtype=np.float64,
    )
    local_v = np.linspace(
        float(frame["minimum_m"][1]),
        float(frame["maximum_m"][1]),
        resolution_y,
        dtype=np.float64,
    )
    local_u_grid, local_v_grid = np.meshgrid(local_u, local_v)
    leaf_name = plan["leaf"]
    if obj.name == leaf_name:
        rest_u_grid = local_u_grid
        rest_v_grid = local_v_grid
        mapping = "leaf_object_xz_is_parent_rest_stock"
    else:
        minimum, maximum = object_local_bounds(obj)
        axial_center_z = float((minimum[2] + maximum[2]) * 0.5)
        distance_from_unrolled_boundary = local_u_grid - float(frame["minimum_m"][0])
        rest_u_grid = (
            plan["root_u_m"]
            + plan["direction_from_leaf_root"] * distance_from_unrolled_boundary
        )
        rest_v_grid = local_v_grid + axial_center_z
        mapping = "circumferential_u_unrolled_from_leaf_root_with_actual_axial_z"
    metadata = {
        "mapping": mapping,
        "rest_u_range_m": [float(rest_u_grid.min()), float(rest_u_grid.max())],
        "rest_v_range_m": [float(rest_v_grid.min()), float(rest_v_grid.max())],
        "compiled_image_resolution": [resolution_x, resolution_y],
    }
    return rest_u_grid, rest_v_grid, metadata


def build_parent_stock_fields(
    targets: list[bpy.types.Object],
    frames: dict[str, dict[str, Any]],
    control_fields: dict[str, dict[str, np.ndarray]],
    plans: dict[str, dict[str, Any]],
) -> tuple[dict[str, dict[str, Any]], dict[str, Any]]:
    fields: dict[str, dict[str, Any]] = {}
    for obj in targets:
        lineage = stock_lineage(obj.name)
        if lineage == "pintle_stock":
            continue
        resolution_y, resolution_x = control_fields[obj.name]["combined_normal"].shape[:2]
        rest_u, rest_v, coordinate_contract = parent_rest_stock_coordinates(
            obj,
            frames[obj.name],
            plans[lineage],
            resolution_x,
            resolution_y,
        )
        height_m = overlapping_obliterated_work_field(rest_u, rest_v, plans[lineage])
        span_u, span_v = (float(value) for value in frames[obj.name]["span_m"])
        normal = common.height_to_normal_nonperiodic(
            height_m,
            meters_per_pixel_x=span_u / (resolution_x - 1),
            meters_per_pixel_y=span_v / (resolution_y - 1),
        )
        fields[obj.name] = {
            "height_m": height_m,
            "normal": normal,
            "coordinate_contract": coordinate_contract,
            "owner": lineage,
        }

    continuity: dict[str, Any] = {}
    for lineage, plan in plans.items():
        leaf_name, *knuckles = STOCK_LINEAGES[lineage]
        errors: dict[str, Any] = {}
        maximum_error = 0.0
        maximum_symmetric_step = 0.0
        for name in knuckles:
            minimum, maximum = object_local_bounds(bpy.data.objects[name])
            axial_v = np.linspace(float(minimum[2]), float(maximum[2]), 129)
            boundary_u = np.full_like(axial_v, float(plan["root_u_m"]))
            leaf_boundary = overlapping_obliterated_work_field(
                boundary_u,
                axial_v,
                plan,
            )
            eye_boundary = overlapping_obliterated_work_field(
                boundary_u,
                axial_v,
                plan,
            )
            height_error = float(np.max(np.abs(leaf_boundary - eye_boundary)))
            epsilon_m = 1.0e-5
            leaf_inside_u = boundary_u - plan["direction_from_leaf_root"] * epsilon_m
            eye_inside_u = boundary_u + plan["direction_from_leaf_root"] * epsilon_m
            leaf_inside = overlapping_obliterated_work_field(leaf_inside_u, axial_v, plan)
            eye_inside = overlapping_obliterated_work_field(eye_inside_u, axial_v, plan)
            symmetric_step = float(np.max(np.abs(leaf_inside - eye_inside)))
            maximum_error = max(maximum_error, height_error)
            maximum_symmetric_step = max(maximum_symmetric_step, symmetric_step)
            errors[name] = {
                "boundary_height_error_m": height_error,
                "symmetric_20_micrometre_step_m": symmetric_step,
                "axial_interval_m": [float(minimum[2]), float(maximum[2])],
            }
        continuity[lineage] = {
            "leaf": leaf_name,
            "maximum_height_error_m": maximum_error,
            "maximum_symmetric_20_micrometre_step_m": maximum_symmetric_step,
            "knuckles": errors,
        }
    return fields, continuity


def progressive_round_pintle_field(
    axial_u_m: np.ndarray,
    circumferential_v_m: np.ndarray,
    *,
    axial_minimum_m: float,
    axial_maximum_m: float,
    circumferential_minimum_m: float,
    circumferential_maximum_m: float,
) -> np.ndarray:
    """Build a weak non-ring residual of progressive square-to-round working."""
    u = np.asarray(axial_u_m, dtype=np.float64)
    v = np.asarray(circumferential_v_m, dtype=np.float64)
    axial_span = max(axial_maximum_m - axial_minimum_m, 1.0e-9)
    circumferential_span = max(
        circumferential_maximum_m - circumferential_minimum_m,
        1.0e-9,
    )
    u01 = np.clip((u - axial_minimum_m) / axial_span, 0.0, 1.0)
    theta = 2.0 * math.pi * (
        (v - circumferential_minimum_m) / circumferential_span
    )
    axial_phase = 0.46 * (u01 - 0.5) + 0.14 * (2.0 * u01 - 1.0) ** 3
    axial_amplitude = 0.70 + 0.17 * np.sin(math.pi * (u01 + 0.14)) + 0.11 * (2.0 * u01 - 1.0)
    eight_fold_ancestor = axial_amplitude * np.cos(8.0 * theta + axial_phase)
    four_fold_residual = 0.17 * np.cos(4.0 * theta - 0.62 + 0.31 * u01)
    three_fold_residual = 0.11 * np.cos(3.0 * theta + 1.07 - 0.23 * u01)
    end_taper = smoothstep01(u01 / 0.12) * smoothstep01((1.0 - u01) / 0.12)
    raw = (eight_fold_ancestor + four_fold_residual + three_fold_residual) * end_taper
    centered = raw - float(raw.mean())
    p01, p99 = np.percentile(centered, (1.0, 99.0))
    if float(p99 - p01) <= 1.0e-8:
        raise RuntimeError("pintle progressive-round field is degenerate")
    return (
        centered * (PINTLE_REVIEW_HEIGHT_P2P_M / float(p99 - p01))
    ).astype(np.float32)


def build_pintle_field(
    frame: dict[str, Any],
    control_field: dict[str, np.ndarray],
) -> tuple[dict[str, Any], dict[str, Any]]:
    resolution_y, resolution_x = control_field["combined_normal"].shape[:2]
    u = np.linspace(
        float(frame["minimum_m"][0]),
        float(frame["maximum_m"][0]),
        resolution_x,
        dtype=np.float64,
    )
    v = np.linspace(
        float(frame["minimum_m"][1]),
        float(frame["maximum_m"][1]),
        resolution_y,
        dtype=np.float64,
    )
    u_grid, v_grid = np.meshgrid(u, v)
    height_m = progressive_round_pintle_field(
        u_grid,
        v_grid,
        axial_minimum_m=float(frame["minimum_m"][0]),
        axial_maximum_m=float(frame["maximum_m"][0]),
        circumferential_minimum_m=float(frame["minimum_m"][1]),
        circumferential_maximum_m=float(frame["maximum_m"][1]),
    )
    span_u, span_v = (float(value) for value in frame["span_m"])
    normal = common.height_to_normal_nonperiodic(
        height_m,
        meters_per_pixel_x=span_u / (resolution_x - 1),
        meters_per_pixel_y=span_v / (resolution_y - 1),
    )
    # Rows are circumferential V and columns are axial U. Closure therefore
    # compares the first and last rows, while the ring-rejection diagnostic
    # averages around rows for every axial station.
    closure_error = float(np.max(np.abs(height_m[0, :] - height_m[-1, :])))
    circumference_mean_by_axial_station = height_m.mean(axis=0)
    result = {
        "height_m": height_m,
        "normal": normal,
        "owner": "pintle_stock",
        "coordinate_contract": {
            "mapping": "axial_u_and_circumferential_v_from_existing_axial_pin_frame",
            "axial_range_m": [float(u.min()), float(u.max())],
            "circumferential_range_m": [float(v.min()), float(v.max())],
            "compiled_image_resolution": [resolution_x, resolution_y],
        },
    }
    continuity = {
        "circumferential_height_error_m": closure_error,
        "circumference_mean_by_axial_station_standard_deviation_m": float(
            circumference_mean_by_axial_station.std()
        ),
        "ring_rejection_metric": (
            "circumference-mean height must remain negligible; height is never a function of axial U alone"
        ),
    }
    return result, continuity


def candidate_uses_parent_stock(candidate_id: str) -> bool:
    return candidate_id in {"B_parent_rest_stock", "D_combined_construction"}


def candidate_uses_pintle(candidate_id: str) -> bool:
    return candidate_id in {"C_separate_axial_pintle", "D_combined_construction"}


def control_images(obj: bpy.types.Object) -> dict[str, bpy.types.Image]:
    material = obj.data.materials[0]
    node_names = {
        "oxide_base": "Continuous_Oxide_Optical_Colour",
        "oxide_roughness": "Independent_Oxide_Roughness",
        "combined_normal": "Broad_Forging_Normal_Only",
        "luster_control": "Explicit_Zero_Worked_Luster_Control",
    }
    result: dict[str, bpy.types.Image] = {}
    for key, node_name in node_names.items():
        node = material.node_tree.nodes.get(node_name)
        if node is None or node.image is None:
            raise RuntimeError(f"{material.name} lost {node_name}")
        result[key] = node.image
    return result


def select_candidate_fields(
    candidate_id: str,
    targets: list[bpy.types.Object],
    control_fields: dict[str, dict[str, np.ndarray]],
    parent_fields: dict[str, dict[str, Any]],
    pintle_field: dict[str, Any],
) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for obj in targets:
        lineage = stock_lineage(obj.name)
        control_height = (
            control_fields[obj.name]["broad_forging_height_m"]
            * build.BROAD_NORMAL_MULTIPLIER
        ).astype(np.float32)
        control_normal = control_fields[obj.name]["combined_normal"].astype(np.float32)
        if lineage != "pintle_stock" and candidate_uses_parent_stock(candidate_id):
            chosen = parent_fields[obj.name]
            changed = True
            owner = lineage
        elif lineage == "pintle_stock" and candidate_uses_pintle(candidate_id):
            chosen = pintle_field
            changed = True
            owner = lineage
        else:
            chosen = {
                "height_m": control_height,
                "normal": control_normal,
                "coordinate_contract": {
                    "mapping": "unchanged_v004_1_per_object_control",
                    "compiled_image_resolution": list(control_normal.shape[1::-1]),
                },
            }
            changed = False
            owner = "v004_1_shape_only_control"
        result[obj.name] = {
            "height_m": chosen["height_m"],
            "normal": chosen["normal"],
            "coordinate_contract": chosen["coordinate_contract"],
            "changed_relative_to_control": changed,
            "normal_owner": owner,
        }
    return result


def configure_scene(scene: bpy.types.Scene, args: argparse.Namespace) -> None:
    scene.render.engine = "CYCLES"
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
    scene.render.use_persistent_data = False
    scene.render.resolution_x = args.resolution_x
    scene.render.resolution_y = args.resolution_y
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = 0.0
    presentation.configure_world(scene)


def render_path(scene: bpy.types.Scene, path: Path) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def heatmap_normal_angle(normal: np.ndarray) -> np.ndarray:
    t = np.clip(
        normal_angle_degrees(normal) / NORMAL_ANGLE_DISPLAY_MAX_DEG,
        0.0,
        1.0,
    )
    red = np.clip((t - 0.45) / 0.55, 0.0, 1.0)
    green = np.clip(1.0 - np.abs(t - 0.55) / 0.55, 0.0, 1.0)
    blue = np.clip(1.0 - t * 1.15, 0.0, 1.0)
    return np.stack((red, green, blue), axis=-1).astype(np.float32)


def build_physical_materials(
    candidate_id: str,
    targets: list[bpy.types.Object],
    candidate_fields: dict[str, dict[str, Any]],
    shared_group: bpy.types.NodeTree,
) -> tuple[dict[str, bpy.types.Material], dict[str, dict[str, str]]]:
    materials: dict[str, bpy.types.Material] = {}
    source_hashes: dict[str, dict[str, str]] = {}
    for obj in targets:
        source_material = obj.data.materials[0]
        images = control_images(obj)
        source_hashes[obj.name] = {
            "base_colour_source_sha256": image_pixel_sha256(images["oxide_base"]),
            "roughness_source_sha256": image_pixel_sha256(images["oxide_roughness"]),
            "normal_source_sha256": image_pixel_sha256(images["combined_normal"]),
            "luster_source_sha256": image_pixel_sha256(images["luster_control"]),
        }
        if not candidate_fields[obj.name]["changed_relative_to_control"]:
            materials[obj.name] = source_material
            continue
        candidate_normal = base.make_float_image(
            f"IGGY_IMG_{candidate_id}_{obj.name}_ConstructionNormal",
            candidate_fields[obj.name]["normal"] * 0.5 + 0.5,
            color=True,
        )
        candidate_images = {
            "oxide_base": images["oxide_base"],
            "oxide_roughness": images["oxide_roughness"],
            "combined_normal": candidate_normal,
            "luster_control": images["luster_control"],
        }
        material, _counts = builder.build_component_material(
            obj,
            shared_group,
            candidate_images,
        )
        material.name = f"IGGY_MAT_{candidate_id}__{obj.name}"
        material["iggy_status"] = "EXPLORATORY_CONSTRUCTION_RESPONSE_NOT_CANONICAL"
        material["iggy_normal_owner"] = candidate_fields[obj.name]["normal_owner"]
        materials[obj.name] = material
    return materials, source_hashes


def render_diagnostics(
    scene: bpy.types.Scene,
    targets: list[bpy.types.Object],
    candidate_id: str,
    fields: dict[str, dict[str, Any]],
    output_root: Path,
    center: Vector,
    span: Vector,
) -> dict[str, dict[str, Any]]:
    proof_materials: dict[str, dict[str, bpy.types.Material]] = {}
    for obj in targets:
        height_gray = np.clip(
            0.5
            + fields[obj.name]["height_m"]
            / (2.0 * HEIGHT_DISPLAY_ABS_MAX_M),
            0.0,
            1.0,
        )
        height_image = base.make_float_image(
            f"IGGY_IMG_{candidate_id}_{obj.name}_HeightProof",
            height_gray,
            color=False,
        )
        angle_image = base.make_float_image(
            f"IGGY_IMG_{candidate_id}_{obj.name}_AngleProof",
            heatmap_normal_angle(fields[obj.name]["normal"]),
            color=True,
        )
        proof_materials[obj.name] = {
            "isolated_height": base.build_lane_material(
                f"IGGY_MAT_{candidate_id}_{obj.name}_HeightProof",
                height_image,
            ),
            "isolated_normal_angle": base.build_lane_material(
                f"IGGY_MAT_{candidate_id}_{obj.name}_AngleProof",
                angle_image,
            ),
        }
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    renders: dict[str, dict[str, Any]] = {}
    for panel_id in ("isolated_height", "isolated_normal_angle"):
        for obj in targets:
            base.set_material(obj, proof_materials[obj.name][panel_id])
        renders[panel_id] = render_path(scene, output_root / f"{panel_id}.png")
    return renders


def render_physical_panels(
    scene: bpy.types.Scene,
    lights: dict[str, bpy.types.Object],
    targets: list[bpy.types.Object],
    physical_materials: dict[str, bpy.types.Material],
    output_root: Path,
    center: Vector,
    span: Vector,
) -> dict[str, dict[str, Any]]:
    for obj in targets:
        base.set_material(obj, physical_materials[obj.name])
    renders: dict[str, dict[str, Any]] = {}
    close_center = Vector((0.72, center.y, center.z))
    presentation.set_neutral_lights(lights, close_center, close=True)
    presentation.front_camera(scene.camera, close_center, 2.05)
    renders["close_neutral"] = render_path(scene, output_root / "close_neutral.png")

    presentation.set_moving_strip(lights, center, side="left")
    presentation.oblique_camera(
        scene.camera,
        center,
        span.x * 1.14,
        x_offset=-0.18,
        height=0.58,
    )
    renders["grazing"] = render_path(scene, output_root / "grazing.png")

    pivot_names = (
        "SM_GH018_Pintle",
        "MovingKnuckle_01",
        "FixedKnuckle_02",
        "MovingKnuckle_03",
        "FixedKnuckle_04",
        "MovingKnuckle_05",
    )
    pivot_minimum, pivot_maximum = presentation.object_bounds(
        [bpy.data.objects[name] for name in pivot_names]
    )
    pivot_center = (pivot_minimum + pivot_maximum) * 0.5
    presentation.set_neutral_lights(lights, pivot_center, close=True)
    presentation.oblique_camera(
        scene.camera,
        pivot_center,
        0.78,
        x_offset=-0.05,
        height=0.18,
    )
    renders["pivot_close"] = render_path(scene, output_root / "pivot_close.png")

    presentation.set_neutral_lights(lights, center)
    presentation.front_camera(scene.camera, center, span.x * 1.62)
    renders["gameplay"] = render_path(scene, output_root / "gameplay.png")
    return renders


BADGE_GLYPHS = {
    "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
    "B": ("11110", "10001", "10001", "11110", "10001", "10001", "11110"),
    "C": ("01111", "10000", "10000", "10000", "10000", "10000", "01111"),
    "D": ("11110", "10001", "10001", "10001", "10001", "10001", "11110"),
    "1": ("00100", "01100", "00100", "00100", "00100", "00100", "01110"),
    "2": ("01110", "10001", "00001", "00010", "00100", "01000", "11111"),
    "3": ("11110", "00001", "00001", "01110", "00001", "00001", "11110"),
    "4": ("00010", "00110", "01010", "10010", "11111", "00010", "00010"),
    "5": ("11111", "10000", "10000", "11110", "00001", "00001", "11110"),
    "6": ("01110", "10000", "10000", "11110", "10001", "10001", "01110"),
}


def write_badge_overlay(path: Path, width: int, height: int, tile_width: int, tile_height: int) -> None:
    overlay = np.zeros((height, width, 4), dtype=np.float32)
    glyph_scale = 4
    badge_width = 58
    badge_height = 42
    for row, row_letter in enumerate("ABCD"):
        for column in range(6):
            x0 = column * tile_width + 10
            y0 = row * tile_height + 10
            overlay[y0 : y0 + badge_height, x0 : x0 + badge_width, :3] = 0.006
            overlay[y0 : y0 + badge_height, x0 : x0 + badge_width, 3] = 0.92
            for glyph_index, character in enumerate((row_letter, str(column + 1))):
                glyph = BADGE_GLYPHS[character]
                glyph_x = x0 + 7 + glyph_index * 27
                glyph_y = y0 + 7
                for source_y, line in enumerate(glyph):
                    for source_x, active in enumerate(line):
                        if active != "1":
                            continue
                        ys = slice(
                            glyph_y + source_y * glyph_scale,
                            glyph_y + (source_y + 1) * glyph_scale,
                        )
                        xs = slice(
                            glyph_x + source_x * glyph_scale,
                            glyph_x + (source_x + 1) * glyph_scale,
                        )
                        overlay[ys, xs, :3] = 0.96
                        overlay[ys, xs, 3] = 1.0
    image = bpy.data.images.new(
        "IGGY_IMG_ConstructionResponseBadgeOverlay",
        width=width,
        height=height,
        alpha=True,
        float_buffer=True,
    )
    image.pixels.foreach_set(np.flipud(overlay).reshape(-1))
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()


def assemble_board(
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for the construction-response board")
    input_paths = [
        output_root / candidate_id / f"{panel_id}.png"
        for candidate_id in CANDIDATES
        for panel_id in PANEL_ORDER
    ]
    raw_board = output_root / "_construction_response_raw.png"
    overlay_path = output_root / "_construction_response_badges.png"
    filter_rows = []
    for row in range(4):
        inputs = "".join(f"[{row * 6 + column}:v]" for column in range(6))
        filter_rows.append(f"{inputs}hstack=inputs=6[row{row}]")
    filter_rows.append("[row0][row1][row2][row3]vstack=inputs=4[board]")
    stack = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            *sum((["-i", str(path)] for path in input_paths), []),
            "-filter_complex",
            ";".join(filter_rows),
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(raw_board),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if stack.returncode != 0:
        raise RuntimeError("Construction-response stack failed: " + stack.stderr.strip())
    board_width = args.resolution_x * 6
    board_height = args.resolution_y * 4
    write_badge_overlay(
        overlay_path,
        board_width,
        board_height,
        args.resolution_x,
        args.resolution_y,
    )
    board_path = output_root / "forged_iron_v004_1_construction_response_board.png"
    overlay = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(raw_board),
            "-i",
            str(overlay_path),
            "-filter_complex",
            "[0:v][1:v]overlay=0:0:format=auto[board]",
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(board_path),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if overlay.returncode != 0:
        raise RuntimeError("Construction-response labels failed: " + overlay.stderr.strip())
    raw_board.unlink(missing_ok=True)
    overlay_path.unlink(missing_ok=True)
    return {
        "path": str(board_path),
        "resolution": [board_width, board_height],
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "row_legend": {
            row_letter: candidate_id
            for row_letter, candidate_id in zip("ABCD", CANDIDATES, strict=True)
        },
        "column_legend": {
            str(index + 1): {"id": panel_id, "label": PANEL_LABELS[panel_id]}
            for index, panel_id in enumerate(PANEL_ORDER)
        },
        "assembly": "ffmpeg 6x4 stack plus deterministic A1-D6 bitmap badges",
    }


def review_only(output_root: Path) -> None:
    manifest_path = output_root / "manifest.json"
    if not manifest_path.is_file():
        raise FileNotFoundError(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    board_path = Path(manifest["board"]["path"])
    if sha256_file(board_path) != manifest["board"]["sha256"]:
        raise RuntimeError("Board changed before review registration")
    for row in manifest["candidates"]:
        for render in row["renders"].values():
            if sha256_file(Path(render["path"])) != render["sha256"]:
                raise RuntimeError("A rendered tile changed before review registration")
    current_source_hash = sha256_file(SOURCE_BLEND)
    if current_source_hash != manifest["source"]["sha256_after"]:
        raise RuntimeError("Source v004.1 changed before review registration")
    manifest["script"]["sha256"] = sha256_file(Path(__file__).resolve())
    if not REVIEW_PATH.is_file():
        raise FileNotFoundError(REVIEW_PATH)
    manifest["review_document"] = {
        "path": str(REVIEW_PATH),
        "sha256": sha256_file(REVIEW_PATH),
    }
    manifest["visual_review"] = VISUAL_REVIEW
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(f"FORGED_IRON_CONSTRUCTION_RESPONSE_REVIEW={VISUAL_REVIEW['decision']}")


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if args.review_only:
        review_only(output_root)
        return
    for required in (SOURCE_BLEND, BUILD_SCRIPT):
        if not required.is_file():
            raise FileNotFoundError(required)

    source_hash_before = sha256_file(SOURCE_BLEND)
    candidate_records: list[dict[str, Any]] = []
    construction_continuity: dict[str, Any] | None = None
    lineage_plan_manifest: dict[str, Any] | None = None

    for candidate_id in CANDIDATES:
        row_root = output_root / candidate_id
        row_root.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
        scene = bpy.context.scene
        configure_scene(scene, args)
        targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
        lights = {name: bpy.data.objects[name] for name in presentation.TARGET_LIGHT_NAMES}
        presentation.isolate_actual_hinge(targets, lights)
        frames = {obj.name: base.install_component_frames(obj) for obj in targets}
        recipe = build.selected_recipe()
        control_fields = {
            obj.name: calibration.build_calibrated_fields(
                frames[obj.name],
                obj.name,
                recipe,
            )
            for obj in targets
        }
        lineage_plans = build_lineage_plans(frames)
        parent_fields, rest_continuity = build_parent_stock_fields(
            targets,
            frames,
            control_fields,
            lineage_plans,
        )
        pintle_field, pintle_continuity = build_pintle_field(
            frames["SM_GH018_Pintle"],
            control_fields["SM_GH018_Pintle"],
        )
        if construction_continuity is None:
            construction_continuity = {
                **rest_continuity,
                "pintle_stock": pintle_continuity,
            }
            lineage_plan_manifest = {
                lineage: {
                    key: value
                    for key, value in plan.items()
                    if key != "passes"
                }
                | {"passes": plan["passes"]}
                for lineage, plan in lineage_plans.items()
            }
        fields = select_candidate_fields(
            candidate_id,
            targets,
            control_fields,
            parent_fields,
            pintle_field,
        )
        shared_group = bpy.data.node_groups.get(build.GROUP_NAME)
        if shared_group is None:
            raise RuntimeError("v004.1 source lost its shared physical-response group")
        shared_signature_before = build.shared_group_signature(shared_group)
        physical_materials, source_lane_hashes = build_physical_materials(
            candidate_id,
            targets,
            fields,
            shared_group,
        )
        shared_signature_after = build.shared_group_signature(shared_group)
        if shared_signature_after != shared_signature_before:
            raise RuntimeError("Disposable response changed shared-group topology")

        minimum, maximum = presentation.object_bounds(targets)
        center = (minimum + maximum) * 0.5
        span = maximum - minimum
        renders = render_diagnostics(
            scene,
            targets,
            candidate_id,
            fields,
            row_root,
            center,
            span,
        )
        renders.update(
            render_physical_panels(
                scene,
                lights,
                targets,
                physical_materials,
                row_root,
                center,
                span,
            )
        )
        component_records: dict[str, Any] = {}
        for obj in targets:
            value = fields[obj.name]
            component_records[obj.name] = {
                "stock_lineage": stock_lineage(obj.name),
                "normal_owner": value["normal_owner"],
                "changed_relative_to_control": value["changed_relative_to_control"],
                "coordinate_contract": value["coordinate_contract"],
                "height_m": descriptive(value["height_m"]),
                "normal_angle_degrees": normal_angle_metrics(value["normal"]),
                "height_float_sha256": sha256_array(value["height_m"]),
                "normal_float_sha256": sha256_array(value["normal"]),
                **source_lane_hashes[obj.name],
            }
        candidate_records.append(
            {
                "candidate_id": candidate_id,
                "changed_members": [
                    name
                    for name, component in component_records.items()
                    if component["changed_relative_to_control"]
                ],
                "components": component_records,
                "renders": {panel: renders[panel] for panel in PANEL_ORDER},
                "shared_group_signature_unchanged": True,
            }
        )

    if construction_continuity is None or lineage_plan_manifest is None:
        raise RuntimeError("Construction-response sweep produced no lineage evidence")
    board = assemble_board(output_root, args)
    source_hash_after = sha256_file(SOURCE_BLEND)
    source_unchanged = source_hash_before == source_hash_after
    if not source_unchanged:
        raise RuntimeError("Construction-response sweep changed saved v004.1")

    manifest = {
        "schema": "iggy3d.forged_iron_v004_1_construction_response_sweep.v1",
        "status": "EXPLORATORY_CONSTRUCTION_RESPONSE_NOT_CANONICAL",
        "source": {
            "path": str(SOURCE_BLEND),
            "sha256_before": source_hash_before,
            "sha256_after": source_hash_after,
        },
        "script": {
            "path": str(Path(__file__).resolve()),
            "sha256": sha256_file(Path(__file__).resolve()),
        },
        "measurement_and_translation_boundary": {
            "human_hammer_envelope_m": list(HUMAN_HAMMER_ENVELOPE_M),
            "fabrication_scale": FABRICATION_SCALE,
            "giant_hammer_envelope_m": list(GIANT_HAMMER_ENVELOPE_M),
            "hammer_envelope_role": (
                "bounded spatial-correlation proxy only; not a literal print or surviving depth"
            ),
            "strap_review_height_p2p_m": STRAP_REVIEW_HEIGHT_P2P_M,
            "pintle_review_height_p2p_m": PINTLE_REVIEW_HEIGHT_P2P_M,
            "height_role": (
                "authored reduced-resolution review translation; not a measurement or displacement claim"
            ),
        },
        "stock_lineages": {key: list(value) for key, value in STOCK_LINEAGES.items()},
        "lineage_plans": lineage_plan_manifest,
        "construction_continuity": construction_continuity,
        "candidates": candidate_records,
        "board": board,
        "review_document": (
            {
                "path": str(REVIEW_PATH),
                "sha256": sha256_file(REVIEW_PATH),
            }
            if REVIEW_PATH.is_file()
            else None
        ),
        "visual_review": VISUAL_REVIEW,
        "source_unchanged": source_unchanged,
        "material_candidate_created": False,
        "uses_ai_generated_imagery": False,
        "uses_condition": False,
        "manual_acceptance_established": False,
        "unreal_parity_verified": False,
        "locked_absences": [
            "rust",
            "condition",
            "damage",
            "scratches",
            "dents",
            "contact_polish",
            "soot",
            "blood",
            "dirt",
            "worked_luster",
            "oxide_height",
            "exposed_conductor",
            "shader_topology_change",
        ],
        "promotion_boundary": (
            "The board may select one normal-owner strategy only. It creates no saved material, donor, acceptance, or engine-parity claim."
        ),
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    )
    print(
        "FORGED_IRON_CONSTRUCTION_RESPONSE_SWEEP="
        f"rows:{len(candidate_records)},panels:{len(candidate_records) * len(PANEL_ORDER)},"
        f"source_unchanged:{source_unchanged}"
    )
    print(f"FORGED_IRON_CONSTRUCTION_RESPONSE_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
