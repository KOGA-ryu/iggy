#!/usr/bin/env python3
"""Compare roughness and direction-dependent reflection on saved v004.1."""

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
COORDINATE_SCRIPT = MATERIAL_ROOT / "compare_v004_1_construction_response_v1.py"
SOURCE_BLEND = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_connected_oxide_candidate_v004_1"
    / "forged_iron_connected_oxide_candidate_v004_1.blend"
)
DEFAULT_OUTPUT = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_v004_1_reflection_routing_sweep_v1"
)
REVIEW_PATH = MATERIAL_ROOT / "REFLECTION_ROUTING_SWEEP_REVIEW.md"

CANDIDATES = (
    "A_v004_1_control",
    "B_construction_roughness_only",
    "C_directional_reflection_only",
    "D_roughness_plus_direction",
)
PANEL_ORDER = (
    "isolated_roughness",
    "isolated_direction_amount",
    "close_neutral",
    "grazing_left",
    "grazing_right",
    "pivot_close",
    "gameplay",
)
PANEL_LABELS = {
    "isolated_roughness": "ABSOLUTE ROUGHNESS",
    "isolated_direction_amount": "DIRECTION RESPONSE AMOUNT",
    "close_neutral": "CLOSE NEUTRAL",
    "grazing_left": "MOVING STRIP LEFT",
    "grazing_right": "MOVING STRIP RIGHT",
    "pivot_close": "PIVOT CLOSE",
    "gameplay": "GAMEPLAY DISTANCE",
}
GEOMETRY_PANEL_ORDER = (
    "clay_full_left",
    "clay_full_right",
    "clay_pivot_left",
    "clay_pivot_right",
    "construction_tangent",
)
GEOMETRY_PANEL_LABELS = {
    "clay_full_left": "CLAY FULL  STRIP LEFT",
    "clay_full_right": "CLAY FULL  STRIP RIGHT",
    "clay_pivot_left": "CLAY PIVOT  STRIP LEFT",
    "clay_pivot_right": "CLAY PIVOT  STRIP RIGHT",
    "construction_tangent": "CONSTRUCTION TANGENT RGB",
}

ROUGHNESS_STATIONS = (0.00, 0.12, 0.29, 0.51, 0.73, 0.90, 1.00)
LONGITUDINAL_VALUES = {
    "moving_leaf_stock": (-0.08, 0.04, 0.10, -0.03, -0.09, 0.05, 0.01),
    "fixed_leaf_stock": (0.06, -0.10, 0.02, 0.09, -0.03, -0.08, 0.04),
}
TILT_VALUES = {
    "moving_leaf_stock": (-0.62, 0.31, 0.88, -0.14, -0.76, 0.42, 0.18),
    "fixed_leaf_stock": (0.48, -0.83, 0.16, 0.72, -0.25, -0.66, 0.37),
}
PINTLE_AXIAL_VALUES = (-0.54, 0.24, 0.76, -0.18, -0.69, 0.33, 0.12)
PINTLE_PHASE_VALUES = (-0.18, 0.06, 0.21, 0.10, -0.14, -0.04, 0.16)

ROUGHNESS_LONGITUDINAL_AMPLITUDE = 0.040
PINTLE_ROUGHNESS_AMPLITUDE = 0.020
ROUGHNESS_MINIMUM = 0.58
ROUGHNESS_MAXIMUM = 0.80
DIRECTIONAL_RESPONSE_AMOUNT = 0.28
DIRECTIONAL_RESPONSE_ROTATION = 0.50
TANGENT_UV_NAME = "IGGY_IronTangentUV_v001"

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

VISUAL_REVIEW = {
    "decision": "select_C_directional_reflection_recipe",
    "ranking": [
        "C_directional_reflection_only",
        "D_roughness_plus_direction",
        "B_construction_roughness_only",
        "A_v004_1_control",
    ],
    "recommended_recipe": "C_directional_reflection_only",
    "advancement_scope": (
        "uniform_0_28_component_tangent_oxide_response_recipe_only; no saved material or donor promotion"
    ),
    "geometry_defect_ledger": [
        "The moving-strip clay proof shows the long leaves as deliberately planar plates with one broad uninterrupted highlight. Any surviving hand-forged plane break must come from geometry or a separately accepted bounded response; material colour cannot claim it.",
        "The pintle and rolled eyes retain smooth cylindrical highlight bands. Hard black interruptions come from real bearing gaps, while the near-perfect vertical highlight is dominated by cylinder geometry rather than a missing texture pattern.",
        "The construction-tangent proof remains coherent: leaf U is longitudinal, rolled-eye U is circumferential, and pintle U is axial. The cyan pivot transition is a class change, not a random UV rotation.",
        "No material row repairs or hides the clay-owned pivot limitation; it remains a separate geometry decision outside this texture workstream.",
    ],
    "material_defect_ledger": [
        "A is the frozen control and retains the previously rejected shape-normalized broad normal owner.",
        "B preserves source roughness means and creates no closed motif, but its physical change is too weak to justify a new layer. Against A it measures 57.61 dB close, 63.42 dB strip-left, 63.70 dB strip-right, 57.40 dB pivot, and 64.62 dB gameplay.",
        "C supplies one uniform 0.28 oxide anisotropy amount through existing construction tangents. It draws no brushing line, groove, cloud, stamp, or condition mask and changes the opposing strip response in both directions.",
        "Against A, C measures 44.03 dB close, 39.28 dB strip-left, 39.47 dB strip-right, 44.40 dB pivot, and 49.52 dB gameplay. The change is visible but remains subordinate to the broad dark oxide and geometry.",
        "D adds B's roughness to C but is 57.74 dB close, 64.09 dB strip-left, 63.63 dB strip-right, 57.78 dB pivot, and 64.69 dB gameplay relative to C. The extra roughness has no persuasive visual contribution and is rejected.",
        "C is selected only as a response calibration. Its 0.28 value is authored, not measured; integration requires a separate demand, new candidate file, reopen proof, and paired comparison against v004.1.",
    ],
    "physical_panel_psnr_db": {
        "B_against_A": {
            "close_neutral": 57.610787,
            "grazing_left": 63.423462,
            "grazing_right": 63.702845,
            "pivot_close": 57.402862,
            "gameplay": 64.619959,
        },
        "C_against_A": {
            "close_neutral": 44.030040,
            "grazing_left": 39.282571,
            "grazing_right": 39.474193,
            "pivot_close": 44.397690,
            "gameplay": 49.521032,
        },
        "D_against_C": {
            "close_neutral": 57.736268,
            "grazing_left": 64.092582,
            "grazing_right": 63.631843,
            "pivot_close": 57.776165,
            "gameplay": 64.685039,
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


build = _load_module("iggy_v004_1_reflection_build", BUILD_SCRIPT)
construction = _load_module("iggy_v004_1_reflection_coordinates", COORDINATE_SCRIPT)
calibration = build.calibration
builder = build.builder
base = build.base
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
    return hashlib.sha256(
        np.ascontiguousarray(value, dtype="<f4").tobytes()
    ).hexdigest()


def image_values(image: bpy.types.Image) -> np.ndarray:
    rgba = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(rgba)
    return rgba.reshape(image.size[1], image.size[0], 4)


def image_pixel_sha256(image: bpy.types.Image) -> str:
    return sha256_array(image_values(image))


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


def stock_lineage(component_name: str) -> str:
    for lineage, members in STOCK_LINEAGES.items():
        if component_name in members:
            return lineage
    raise KeyError(component_name)


def source_images(obj: bpy.types.Object) -> dict[str, bpy.types.Image]:
    material = obj.data.materials[0]
    nodes = {
        "oxide_base": "Continuous_Oxide_Optical_Colour",
        "oxide_roughness": "Independent_Oxide_Roughness",
        "combined_normal": "Broad_Forging_Normal_Only",
        "luster_control": "Explicit_Zero_Worked_Luster_Control",
    }
    result: dict[str, bpy.types.Image] = {}
    for key, node_name in nodes.items():
        node = material.node_tree.nodes.get(node_name)
        if node is None or node.image is None:
            raise RuntimeError(f"{material.name} lost {node_name}")
        result[key] = node.image
    return result


def open_cubic_hermite_curve(
    t: np.ndarray,
    stations: tuple[float, ...],
    values: tuple[float, ...],
) -> np.ndarray:
    """Evaluate one nonperiodic cubic Hermite record at irregular stations."""
    x = np.asarray(stations, dtype=np.float64)
    y = np.asarray(values, dtype=np.float64)
    if len(x) != len(y) or len(x) < 4 or np.any(np.diff(x) <= 0.0):
        raise ValueError("Hermite record requires paired increasing stations")
    slopes = np.empty_like(y)
    slopes[0] = (y[1] - y[0]) / (x[1] - x[0])
    slopes[-1] = (y[-1] - y[-2]) / (x[-1] - x[-2])
    slopes[1:-1] = (y[2:] - y[:-2]) / (x[2:] - x[:-2])
    query = np.clip(np.asarray(t, dtype=np.float64), x[0], x[-1])
    interval = np.searchsorted(x, query, side="right") - 1
    interval = np.clip(interval, 0, len(x) - 2)
    x0 = x[interval]
    x1 = x[interval + 1]
    span = x1 - x0
    local = (query - x0) / span
    h00 = 2.0 * local**3 - 3.0 * local**2 + 1.0
    h10 = local**3 - 2.0 * local**2 + local
    h01 = -2.0 * local**3 + 3.0 * local**2
    h11 = local**3 - local**2
    return (
        h00 * y[interval]
        + h10 * span * slopes[interval]
        + h01 * y[interval + 1]
        + h11 * span * slopes[interval + 1]
    ).astype(np.float32)


def coordinate_plans(frames: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
    plans: dict[str, dict[str, Any]] = {}
    for lineage, leaf_name, direction in (
        ("moving_leaf_stock", "SM_GH018_MovingLeaf_Openwork", -1.0),
        ("fixed_leaf_stock", "SM_GH018_FixedLeaf", 1.0),
    ):
        leaf = frames[leaf_name]
        maximum_roll = max(
            float(frames[name]["span_m"][0])
            for name in STOCK_LINEAGES[lineage][1:]
        )
        leaf_u_min, leaf_v_min = (float(value) for value in leaf["minimum_m"])
        leaf_u_max, leaf_v_max = (float(value) for value in leaf["maximum_m"])
        root_u = leaf_u_min if direction < 0.0 else leaf_u_max
        domain_u_min = root_u - maximum_roll if direction < 0.0 else leaf_u_min
        domain_u_max = leaf_u_max if direction < 0.0 else root_u + maximum_roll
        plans[lineage] = {
            "lineage": lineage,
            "leaf": leaf_name,
            "direction_from_leaf_root": direction,
            "root_u_m": root_u,
            "domain_u_m": [domain_u_min, domain_u_max],
            "domain_v_m": [leaf_v_min, leaf_v_max],
            "maximum_unrolled_knuckle_span_m": maximum_roll,
        }
    return plans


def construction_owned_roughness(
    obj: bpy.types.Object,
    frame: dict[str, Any],
    plan: dict[str, Any] | None,
    source_roughness: np.ndarray,
) -> tuple[np.ndarray, dict[str, Any]]:
    """Add one open response field; compact supports and height are absent."""
    height, width = source_roughness.shape
    lineage = stock_lineage(obj.name)
    if lineage == "pintle_stock":
        local_u = np.linspace(
            float(frame["minimum_m"][0]),
            float(frame["maximum_m"][0]),
            width,
            dtype=np.float64,
        )
        local_v = np.linspace(
            float(frame["minimum_m"][1]),
            float(frame["maximum_m"][1]),
            height,
            dtype=np.float64,
        )
        u_grid, v_grid = np.meshgrid(local_u, local_v)
        u01 = (u_grid - local_u.min()) / max(float(np.ptp(local_u)), 1.0e-9)
        theta = 2.0 * math.pi * (
            (v_grid - local_v.min()) / max(float(np.ptp(local_v)), 1.0e-9)
        )
        axial = open_cubic_hermite_curve(
            u01,
            ROUGHNESS_STATIONS,
            PINTLE_AXIAL_VALUES,
        )
        phase = open_cubic_hermite_curve(
            u01,
            ROUGHNESS_STATIONS,
            PINTLE_PHASE_VALUES,
        )
        delta = PINTLE_ROUGHNESS_AMPLITUDE * axial * np.cos(theta + phase)
        coordinate_contract = {
            "mapping": "separate_axial_u_and_first_order_circumferential_passage",
            "u_range_m": [float(local_u.min()), float(local_u.max())],
            "v_range_m": [float(local_v.min()), float(local_v.max())],
            "prohibits_axial_only_rings": True,
        }
    else:
        if plan is None:
            raise RuntimeError(f"{obj.name} lost its parent-rest-stock plan")
        rest_u, rest_v, coordinate_contract = construction.parent_rest_stock_coordinates(
            obj,
            frame,
            plan,
            width,
            height,
        )
        u_min, u_max = (float(value) for value in plan["domain_u_m"])
        v_min, v_max = (float(value) for value in plan["domain_v_m"])
        u01 = np.clip((rest_u - u_min) / max(u_max - u_min, 1.0e-9), 0.0, 1.0)
        v_signed = np.clip(
            2.0 * (rest_v - v_min) / max(v_max - v_min, 1.0e-9) - 1.0,
            -1.0,
            1.0,
        )
        longitudinal = open_cubic_hermite_curve(
            u01,
            ROUGHNESS_STATIONS,
            LONGITUDINAL_VALUES[lineage],
        )
        tilt = open_cubic_hermite_curve(
            u01,
            ROUGHNESS_STATIONS,
            TILT_VALUES[lineage],
        )
        delta = ROUGHNESS_LONGITUDINAL_AMPLITUDE * (
            0.78 * longitudinal + 0.22 * tilt * v_signed
        )
        coordinate_contract = dict(coordinate_contract)
        coordinate_contract["field_operation"] = (
            "open_nonperiodic_longitudinal_Hermite_plus_cross_stock_tilt"
        )

    # Only a scalar DC compensation is permitted. It preserves each source
    # response mean without changing phase, frequency, or range of the shared
    # analytic field.
    delta = np.asarray(delta, dtype=np.float32)
    delta -= float(delta.mean())
    candidate = np.clip(
        source_roughness + delta,
        ROUGHNESS_MINIMUM,
        ROUGHNESS_MAXIMUM,
    ).astype(np.float32)
    mean_correction = float(source_roughness.mean() - candidate.mean())
    candidate = np.clip(
        candidate + mean_correction,
        ROUGHNESS_MINIMUM,
        ROUGHNESS_MAXIMUM,
    ).astype(np.float32)
    return candidate, {
        "coordinate_contract": coordinate_contract,
        "delta": descriptive(delta),
        "source_mean": float(source_roughness.mean()),
        "candidate_mean": float(candidate.mean()),
        "mean_drift": float(candidate.mean() - source_roughness.mean()),
        "dc_compensation_only": True,
    }


def set_principled_input(
    node: bpy.types.Node,
    names: tuple[str, ...],
    value: Any,
) -> str:
    return base.set_principled_input(node, names, value)


def build_clay_material() -> bpy.types.Material:
    material = bpy.data.materials.new("IGGY_MAT_ReflectionRoutingNeutralClay")
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    shader = tree.nodes.new("ShaderNodeBsdfPrincipled")
    shader.name = "Neutral_Clay_Geometry_Only"
    set_principled_input(shader, ("Base Color",), (0.42, 0.42, 0.42, 1.0))
    set_principled_input(shader, ("Metallic",), 0.0)
    set_principled_input(shader, ("Roughness",), 0.38)
    tree.links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def build_tangent_proof_material(obj: bpy.types.Object) -> bpy.types.Material:
    material = bpy.data.materials.new(f"IGGY_MAT_TangentProof_{obj.name}")
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    emission = tree.nodes.new("ShaderNodeEmission")
    tangent = tree.nodes.new("ShaderNodeTangent")
    tangent.name = "Construction_Tangent_Direction"
    tangent.direction_type = "UV_MAP"
    tangent.uv_map = TANGENT_UV_NAME
    scale = tree.nodes.new("ShaderNodeVectorMath")
    scale.operation = "SCALE"
    scale.inputs["Scale"].default_value = 0.5
    add = tree.nodes.new("ShaderNodeVectorMath")
    add.operation = "ADD"
    add.inputs[1].default_value = (0.5, 0.5, 0.5)
    tree.links.new(tangent.outputs["Tangent"], scale.inputs[0])
    tree.links.new(scale.outputs["Vector"], add.inputs[0])
    tree.links.new(add.outputs["Vector"], emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def build_disposable_direction_group(
    source_group: bpy.types.NodeTree,
    candidate_id: str,
) -> tuple[bpy.types.NodeTree, dict[str, Any]]:
    source_multiplier = source_group.nodes.get("Worked_Luster_Disabled")
    if source_multiplier is None or float(source_multiplier.inputs[1].default_value) != 0.0:
        raise RuntimeError("Source direction-response multiplier is not frozen at zero")
    group = source_group.copy()
    group.name = f"IGGY_SH_{candidate_id}_DisposableDirectionResponse"
    multiplier = group.nodes.get("Worked_Luster_Disabled")
    if multiplier is None:
        raise RuntimeError("Disposable group lost the existing anisotropy multiplier")
    multiplier.name = "Worked_Luster_Disabled"
    multiplier.inputs[1].default_value = 1.0
    group["iggy_status"] = "EXPLORATORY_REFLECTION_ROUTING_NOT_CANONICAL"
    group["iggy_direction_response_amount"] = DIRECTIONAL_RESPONSE_AMOUNT
    source_signature = build.shared_group_signature(source_group)
    candidate_signature = build.shared_group_signature(group)
    topology_matches = (
        source_signature["topology_sha256"] == candidate_signature["topology_sha256"]
        and source_signature["node_count"] == candidate_signature["node_count"]
        and source_signature["link_count"] == candidate_signature["link_count"]
        and source_signature["interface_socket_count"]
        == candidate_signature["interface_socket_count"]
    )
    if not topology_matches:
        raise RuntimeError("Direction experiment changed source-group topology")
    return group, {
        "source_multiplier": 0.0,
        "candidate_multiplier": 1.0,
        "topology_matches_source": topology_matches,
        "source_topology_sha256": source_signature["topology_sha256"],
        "candidate_topology_sha256": candidate_signature["topology_sha256"],
    }


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


def pivot_geometry() -> tuple[Vector, Vector, Vector]:
    names = (
        "SM_GH018_Pintle",
        "MovingKnuckle_01",
        "FixedKnuckle_02",
        "MovingKnuckle_03",
        "FixedKnuckle_04",
        "MovingKnuckle_05",
    )
    minimum, maximum = presentation.object_bounds([bpy.data.objects[name] for name in names])
    return minimum, maximum, (minimum + maximum) * 0.5


def render_geometry_moving_strip_proof(
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
    scene = bpy.context.scene
    configure_scene(scene, args)
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    lights = {name: bpy.data.objects[name] for name in presentation.TARGET_LIGHT_NAMES}
    presentation.isolate_actual_hinge(targets, lights)
    frames = {obj.name: base.install_component_frames(obj) for obj in targets}
    minimum, maximum = presentation.object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    clay = build_clay_material()
    for obj in targets:
        base.set_material(obj, clay)
    renders: dict[str, dict[str, Any]] = {}
    presentation.oblique_camera(
        scene.camera,
        center,
        span.x * 1.14,
        x_offset=-0.18,
        height=0.58,
    )
    presentation.set_moving_strip(lights, center, side="left")
    renders["clay_full_left"] = render_path(scene, output_root / "clay_full_left.png")
    presentation.set_moving_strip(lights, center, side="right")
    renders["clay_full_right"] = render_path(scene, output_root / "clay_full_right.png")

    _pivot_minimum, _pivot_maximum, pivot_center = pivot_geometry()
    presentation.oblique_camera(
        scene.camera,
        pivot_center,
        0.78,
        x_offset=-0.05,
        height=0.18,
    )
    presentation.set_moving_strip(lights, pivot_center, side="left")
    renders["clay_pivot_left"] = render_path(scene, output_root / "clay_pivot_left.png")
    presentation.set_moving_strip(lights, pivot_center, side="right")
    renders["clay_pivot_right"] = render_path(scene, output_root / "clay_pivot_right.png")

    for obj in targets:
        base.set_material(obj, build_tangent_proof_material(obj))
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    renders["construction_tangent"] = render_path(
        scene,
        output_root / "construction_tangent.png",
    )
    return {
        "frames": frames,
        "renders": renders,
        "view_contract": {
            "full_center": list(center),
            "full_span": list(span),
            "pivot_center": list(pivot_center),
            "clay_base_colour": [0.42, 0.42, 0.42],
            "clay_roughness": 0.38,
            "clay_metalness": 0.0,
            "normal_input": None,
        },
    }


def candidate_uses_roughness(candidate_id: str) -> bool:
    return candidate_id in {
        "B_construction_roughness_only",
        "D_roughness_plus_direction",
    }


def candidate_uses_direction(candidate_id: str) -> bool:
    return candidate_id in {
        "C_directional_reflection_only",
        "D_roughness_plus_direction",
    }


def build_candidate_materials(
    candidate_id: str,
    targets: list[bpy.types.Object],
    frames: dict[str, dict[str, Any]],
    plans: dict[str, dict[str, Any]],
    source_group: bpy.types.NodeTree,
) -> tuple[
    dict[str, bpy.types.Material],
    dict[str, dict[str, Any]],
    dict[str, Any] | None,
]:
    direction_contract: dict[str, Any] | None = None
    response_group = source_group
    if candidate_uses_direction(candidate_id):
        response_group, direction_contract = build_disposable_direction_group(
            source_group,
            candidate_id,
        )
    materials: dict[str, bpy.types.Material] = {}
    components: dict[str, dict[str, Any]] = {}
    for obj in targets:
        images = source_images(obj)
        source_roughness = image_values(images["oxide_roughness"])[..., 0]
        if candidate_uses_roughness(candidate_id):
            roughness, roughness_contract = construction_owned_roughness(
                obj,
                frames[obj.name],
                plans.get(stock_lineage(obj.name)),
                source_roughness,
            )
            roughness_image = base.make_float_image(
                f"IGGY_IMG_{candidate_id}_{obj.name}_OpenRoughness",
                roughness,
                color=False,
            )
        else:
            roughness = source_roughness.astype(np.float32)
            roughness_image = images["oxide_roughness"]
            roughness_contract = {
                "coordinate_contract": {"mapping": "unchanged_v004_1_control"},
                "delta": descriptive(np.zeros_like(roughness)),
                "source_mean": float(roughness.mean()),
                "candidate_mean": float(roughness.mean()),
                "mean_drift": 0.0,
                "dc_compensation_only": False,
            }
        if candidate_uses_direction(candidate_id):
            luster_values = np.empty((4, 4, 3), dtype=np.float32)
            luster_values[..., 0] = DIRECTIONAL_RESPONSE_AMOUNT
            luster_values[..., 1] = DIRECTIONAL_RESPONSE_ROTATION
            luster_values[..., 2] = 1.0
            luster_image = base.make_float_image(
                f"IGGY_IMG_{candidate_id}_{obj.name}_UniformDirectionResponse",
                luster_values,
                color=True,
            )
        else:
            luster_image = images["luster_control"]

        if candidate_id == "A_v004_1_control":
            material = obj.data.materials[0]
        else:
            material_images = {
                "oxide_base": images["oxide_base"],
                "oxide_roughness": roughness_image,
                "combined_normal": images["combined_normal"],
                "luster_control": luster_image,
            }
            material, _counts = builder.build_component_material(
                obj,
                response_group,
                material_images,
            )
            material.name = f"IGGY_MAT_{candidate_id}__{obj.name}"
            material["iggy_status"] = "EXPLORATORY_REFLECTION_ROUTING_NOT_CANONICAL"
        materials[obj.name] = material
        components[obj.name] = {
            "stock_lineage": stock_lineage(obj.name),
            "tangent_mode": frames[obj.name]["mode"],
            "roughness": descriptive(roughness),
            "roughness_float_sha256": sha256_array(roughness),
            "roughness_source_sha256": image_pixel_sha256(images["oxide_roughness"]),
            "roughness_mean_drift": roughness_contract["mean_drift"],
            "roughness_contract": roughness_contract,
            "base_colour_source_sha256": image_pixel_sha256(images["oxide_base"]),
            "normal_source_sha256": image_pixel_sha256(images["combined_normal"]),
            "luster_source_sha256": image_pixel_sha256(images["luster_control"]),
            "directional_response_amount": (
                DIRECTIONAL_RESPONSE_AMOUNT
                if candidate_uses_direction(candidate_id)
                else 0.0
            ),
        }
    return materials, components, direction_contract


def render_candidate_row(
    candidate_id: str,
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
    scene = bpy.context.scene
    configure_scene(scene, args)
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    lights = {name: bpy.data.objects[name] for name in presentation.TARGET_LIGHT_NAMES}
    presentation.isolate_actual_hinge(targets, lights)
    frames = {obj.name: base.install_component_frames(obj) for obj in targets}
    plans = coordinate_plans(frames)
    source_group = bpy.data.node_groups.get(build.GROUP_NAME)
    if source_group is None:
        raise RuntimeError("v004.1 source lost its physical response group")
    source_signature_before = build.shared_group_signature(source_group)
    source_multiplier = source_group.nodes["Worked_Luster_Disabled"]
    if float(source_multiplier.inputs[1].default_value) != 0.0:
        raise RuntimeError("Source group enabled direction response before the sweep")
    materials, components, direction_contract = build_candidate_materials(
        candidate_id,
        targets,
        frames,
        plans,
        source_group,
    )
    source_signature_after = build.shared_group_signature(source_group)
    source_value_unchanged = float(source_multiplier.inputs[1].default_value) == 0.0
    if source_signature_after != source_signature_before or not source_value_unchanged:
        raise RuntimeError("Disposable routing changed the source response group")

    minimum, maximum = presentation.object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    renders: dict[str, dict[str, Any]] = {}
    for obj in targets:
        roughness_values = components[obj.name]["roughness"]
        _ = roughness_values
        material_images = source_images(obj) if candidate_id == "A_v004_1_control" else None
        if candidate_id == "A_v004_1_control":
            roughness_image = material_images["oxide_roughness"]
        else:
            roughness_node = materials[obj.name].node_tree.nodes["Independent_Oxide_Roughness"]
            roughness_image = roughness_node.image
        roughness_proof = base.build_lane_material(
            f"IGGY_MAT_{candidate_id}_{obj.name}_RoughnessProof",
            roughness_image,
        )
        base.set_material(obj, roughness_proof)
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    renders["isolated_roughness"] = render_path(
        scene,
        output_root / "isolated_roughness.png",
    )

    direction_amount = (
        DIRECTIONAL_RESPONSE_AMOUNT if candidate_uses_direction(candidate_id) else 0.0
    )
    direction_material = presentation.make_emission_material(
        f"IGGY_MAT_{candidate_id}_DirectionAmountProof",
        colour=(direction_amount, direction_amount, direction_amount, 1.0),
    )
    for obj in targets:
        base.set_material(obj, direction_material)
    renders["isolated_direction_amount"] = render_path(
        scene,
        output_root / "isolated_direction_amount.png",
    )

    for obj in targets:
        base.set_material(obj, materials[obj.name])
    close_center = Vector((0.72, center.y, center.z))
    presentation.set_neutral_lights(lights, close_center, close=True)
    presentation.front_camera(scene.camera, close_center, 2.05)
    renders["close_neutral"] = render_path(scene, output_root / "close_neutral.png")

    presentation.oblique_camera(
        scene.camera,
        center,
        span.x * 1.14,
        x_offset=-0.18,
        height=0.58,
    )
    presentation.set_moving_strip(lights, center, side="left")
    renders["grazing_left"] = render_path(scene, output_root / "grazing_left.png")
    presentation.set_moving_strip(lights, center, side="right")
    renders["grazing_right"] = render_path(scene, output_root / "grazing_right.png")

    _pivot_minimum, _pivot_maximum, pivot_center = pivot_geometry()
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
    return {
        "candidate_id": candidate_id,
        "components": components,
        "directional_response_amount": direction_amount,
        "directional_response_rotation": (
            DIRECTIONAL_RESPONSE_ROTATION if direction_amount > 0.0 else None
        ),
        "disposable_direction_group_created": direction_contract is not None,
        "direction_group_contract": direction_contract,
        "source_group_signature": source_signature_before,
        "source_group_value_unchanged": source_value_unchanged,
        "renders": {panel: renders[panel] for panel in PANEL_ORDER},
    }


BADGE_GLYPHS = dict(construction.BADGE_GLYPHS)
BADGE_GLYPHS["7"] = (
    "11111",
    "00001",
    "00010",
    "00100",
    "01000",
    "01000",
    "01000",
)


def write_grid_badges(
    path: Path,
    width: int,
    height: int,
    tile_width: int,
    tile_height: int,
    rows: tuple[str, ...],
    columns: int,
) -> None:
    overlay = np.zeros((height, width, 4), dtype=np.float32)
    glyph_scale = 4
    badge_width = 58 if rows else 34
    badge_height = 42
    for row_index in range(max(len(rows), 1)):
        for column in range(columns):
            x0 = column * tile_width + 10
            y0 = row_index * tile_height + 10
            overlay[y0 : y0 + badge_height, x0 : x0 + badge_width, :3] = 0.006
            overlay[y0 : y0 + badge_height, x0 : x0 + badge_width, 3] = 0.92
            characters = (
                (rows[row_index], str(column + 1))
                if rows
                else (str(column + 1),)
            )
            for glyph_index, character in enumerate(characters):
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
        f"IGGY_IMG_ReflectionRoutingBadges_{width}x{height}",
        width=width,
        height=height,
        alpha=True,
        float_buffer=True,
    )
    image.pixels.foreach_set(np.flipud(overlay).reshape(-1))
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()


def assemble_grid(
    input_paths: list[Path],
    output_path: Path,
    *,
    columns: int,
    rows: int,
    tile_width: int,
    tile_height: int,
    row_badges: tuple[str, ...],
) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for proof-board assembly")
    if len(input_paths) != columns * rows:
        raise ValueError("grid input count does not match rows and columns")
    raw_path = output_path.with_name("_" + output_path.stem + "_raw.png")
    badge_path = output_path.with_name("_" + output_path.stem + "_badges.png")
    filters: list[str] = []
    for row in range(rows):
        inputs = "".join(f"[{row * columns + column}:v]" for column in range(columns))
        filters.append(f"{inputs}hstack=inputs={columns}[row{row}]")
    if rows == 1:
        filters.append("[row0]null[board]")
    else:
        row_inputs = "".join(f"[row{row}]" for row in range(rows))
        filters.append(f"{row_inputs}vstack=inputs={rows}[board]")
    stack = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            *sum((["-i", str(path)] for path in input_paths), []),
            "-filter_complex",
            ";".join(filters),
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(raw_path),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if stack.returncode != 0:
        raise RuntimeError("Proof-board stack failed: " + stack.stderr.strip())
    width = tile_width * columns
    height = tile_height * rows
    write_grid_badges(
        badge_path,
        width,
        height,
        tile_width,
        tile_height,
        row_badges,
        columns,
    )
    overlay = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(raw_path),
            "-i",
            str(badge_path),
            "-filter_complex",
            "[0:v][1:v]overlay=0:0:format=auto[board]",
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(output_path),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if overlay.returncode != 0:
        raise RuntimeError("Proof-board badge overlay failed: " + overlay.stderr.strip())
    raw_path.unlink(missing_ok=True)
    badge_path.unlink(missing_ok=True)
    return {
        "path": str(output_path),
        "resolution": [width, height],
        "bytes": output_path.stat().st_size,
        "sha256": sha256_file(output_path),
    }


def review_only(output_root: Path) -> None:
    manifest_path = output_root / "manifest.json"
    if not manifest_path.is_file():
        raise FileNotFoundError(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    paths = [Path(manifest["board"]["path"]), Path(manifest["geometry_proof"]["board"]["path"])]
    paths.extend(
        Path(render["path"])
        for row in manifest["candidates"]
        for render in row["renders"].values()
    )
    paths.extend(Path(render["path"]) for render in manifest["geometry_proof"]["renders"].values())
    expected = {
        manifest["board"]["path"]: manifest["board"]["sha256"],
        manifest["geometry_proof"]["board"]["path"]: manifest["geometry_proof"]["board"]["sha256"],
    }
    for row in manifest["candidates"]:
        expected.update({render["path"]: render["sha256"] for render in row["renders"].values()})
    expected.update(
        {render["path"]: render["sha256"] for render in manifest["geometry_proof"]["renders"].values()}
    )
    for path in paths:
        if sha256_file(path) != expected[str(path)]:
            raise RuntimeError(f"Proof changed before review registration: {path}")
    if sha256_file(SOURCE_BLEND) != manifest["source"]["sha256_after"]:
        raise RuntimeError("Saved v004.1 changed before review registration")
    if not REVIEW_PATH.is_file():
        raise FileNotFoundError(REVIEW_PATH)
    manifest["script"]["sha256"] = sha256_file(Path(__file__).resolve())
    manifest["review_document"] = {
        "path": str(REVIEW_PATH),
        "sha256": sha256_file(REVIEW_PATH),
    }
    manifest["visual_review"] = VISUAL_REVIEW
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(f"FORGED_IRON_REFLECTION_ROUTING_REVIEW={VISUAL_REVIEW['decision']}")


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if args.review_only:
        review_only(output_root)
        return
    for required in (SOURCE_BLEND, BUILD_SCRIPT, COORDINATE_SCRIPT):
        if not required.is_file():
            raise FileNotFoundError(required)
    if base.TANGENT_UV != TANGENT_UV_NAME:
        raise RuntimeError("Tangent UV name drifted from the frozen demand")

    source_hash_before = sha256_file(SOURCE_BLEND)
    geometry_root = output_root / "geometry_moving_strip"
    geometry_root.mkdir(parents=True, exist_ok=True)
    geometry_proof = render_geometry_moving_strip_proof(geometry_root, args)
    geometry_board_path = output_root / "forged_iron_v004_1_geometry_moving_strip_board.png"
    geometry_board = assemble_grid(
        [geometry_root / f"{panel}.png" for panel in GEOMETRY_PANEL_ORDER],
        geometry_board_path,
        columns=5,
        rows=1,
        tile_width=args.resolution_x,
        tile_height=args.resolution_y,
        row_badges=(),
    )
    geometry_board["panel_legend"] = {
        str(index + 1): {"id": panel, "label": GEOMETRY_PANEL_LABELS[panel]}
        for index, panel in enumerate(GEOMETRY_PANEL_ORDER)
    }
    geometry_proof["board"] = geometry_board

    candidate_records: list[dict[str, Any]] = []
    for candidate_id in CANDIDATES:
        row_root = output_root / candidate_id
        row_root.mkdir(parents=True, exist_ok=True)
        candidate_records.append(render_candidate_row(candidate_id, row_root, args))

    board_path = output_root / "forged_iron_v004_1_reflection_routing_board.png"
    board = assemble_grid(
        [
            output_root / candidate_id / f"{panel}.png"
            for candidate_id in CANDIDATES
            for panel in PANEL_ORDER
        ],
        board_path,
        columns=7,
        rows=4,
        tile_width=args.resolution_x,
        tile_height=args.resolution_y,
        row_badges=tuple("ABCD"),
    )
    board["row_legend"] = {
        row: candidate
        for row, candidate in zip("ABCD", CANDIDATES, strict=True)
    }
    board["column_legend"] = {
        str(index + 1): {"id": panel, "label": PANEL_LABELS[panel]}
        for index, panel in enumerate(PANEL_ORDER)
    }

    source_hash_after = sha256_file(SOURCE_BLEND)
    source_unchanged = source_hash_before == source_hash_after
    if not source_unchanged:
        raise RuntimeError("Reflection-routing sweep changed saved v004.1")
    source_group_unchanged = all(
        row["source_group_value_unchanged"] for row in candidate_records
    ) and len({row["source_group_signature"]["topology_sha256"] for row in candidate_records}) == 1
    if not source_group_unchanged:
        raise RuntimeError("Source response group changed across disposable rows")

    manifest = {
        "schema": "iggy3d.forged_iron_v004_1_reflection_routing_sweep.v1",
        "status": "EXPLORATORY_REFLECTION_ROUTING_NOT_CANONICAL",
        "source": {
            "path": str(SOURCE_BLEND),
            "sha256_before": source_hash_before,
            "sha256_after": source_hash_after,
        },
        "script": {
            "path": str(Path(__file__).resolve()),
            "sha256": sha256_file(Path(__file__).resolve()),
        },
        "geometry_proof": geometry_proof,
        "roughness_contract": {
            "stations": list(ROUGHNESS_STATIONS),
            "longitudinal_amplitude": ROUGHNESS_LONGITUDINAL_AMPLITUDE,
            "pintle_amplitude": PINTLE_ROUGHNESS_AMPLITUDE,
            "review_envelope": [ROUGHNESS_MINIMUM, ROUGHNESS_MAXIMUM],
            "compact_supports": False,
            "height_or_normal_source": False,
            "per_component_operation": "shared analytic sample plus scalar DC mean preservation only",
        },
        "direction_contract": {
            "amount": DIRECTIONAL_RESPONSE_AMOUNT,
            "rotation": DIRECTIONAL_RESPONSE_ROTATION,
            "control_image": "uniform 4x4 RGB=(0.28,0.50,1.0)",
            "source_multiplier": 0.0,
            "candidate_multiplier": 1.0,
            "tangent_uv": TANGENT_UV_NAME,
            "visible_direction_pattern": False,
        },
        "candidates": candidate_records,
        "board": board,
        "review_document": None,
        "visual_review": VISUAL_REVIEW,
        "source_unchanged": source_unchanged,
        "source_group_unchanged": source_group_unchanged,
        "material_candidate_created": False,
        "uses_ai_generated_imagery": False,
        "uses_condition": False,
        "manual_acceptance_established": False,
        "unreal_parity_verified": False,
        "locked_absences": [
            "new_normal",
            "height",
            "oxide_height",
            "rust",
            "damage",
            "scratches",
            "dents",
            "contact_polish",
            "exposed_conductor",
            "baked_light_direction",
            "saved_shader_topology_change",
        ],
        "promotion_boundary": (
            "The board may select one response-routing recipe only; integration requires a separate frozen demand."
        ),
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    )
    print(
        "FORGED_IRON_REFLECTION_ROUTING_SWEEP="
        f"rows:{len(candidate_records)},material_panels:{len(candidate_records) * len(PANEL_ORDER)},"
        f"geometry_panels:{len(GEOMETRY_PANEL_ORDER)},source_unchanged:{source_unchanged}"
    )
    print(f"FORGED_IRON_REFLECTION_ROUTING_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
