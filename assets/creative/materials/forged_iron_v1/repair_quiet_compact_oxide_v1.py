#!/usr/bin/env python3
"""Render one noncanonical B-only repair of intact compact forge scale."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Any

import bpy
from mathutils import Vector
import numpy as np


MATERIAL_ROOT = Path(__file__).resolve().parent
SWEEP_SCRIPT = MATERIAL_ROOT / "compare_intact_oxide_layers_v1.py"
DEFAULT_OUTPUT = MATERIAL_ROOT / "output" / "quiet_compact_oxide_repair_v1"
FIELD_WAVELENGTHS_M = (0.240, 0.075, 0.024)
FIELD_WEIGHTS = (0.48, 0.34, 0.18)
GRAIN_WAVELENGTH_M = 0.016
FLOW_RAIL_COUNT = 3
EXPOSED_CONDUCTOR = 0.0


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


sweep = _load_module("iggy_intact_oxide_sweep_repair_donor", SWEEP_SCRIPT)
base = sweep.base
common = base.common


RECIPE = sweep.CandidateRecipe(
    candidate_id="B_repaired_quiet_compact_scale_r2",
    label="B REPAIRED QUIET COMPACT SCALE R2",
    thickness_minimum_m=0.000008,
    thickness_maximum_m=0.000035,
    # Preserve the cumulative intact-hinge value hierarchy. This repair owns
    # oxide morphology, not a surprise global darkening of the material.
    thin_srgb=(0.090, 0.100, 0.116),
    thick_srgb=(0.158, 0.174, 0.202),
    morphology_colour_delta_srgb=(0.006, 0.003, -0.002),
    compression_colour_delta_srgb=(-0.003, 0.001, 0.004),
    compression_modes=(),
    morphology_modes=(),
    grain_modes=(),
    rest_modes=(),
    surface_relief_amplitude_m=0.000012,
    roughness_base=0.682,
    thermal_roughness_amplitude=0.040,
    compression_roughness_amplitude=0.018,
    morphology_roughness_amplitude=0.030,
    grain_roughness_amplitude=0.010,
    roughness_minimum=0.610,
    roughness_maximum=0.740,
    oxide_anisotropy=0.04,
)


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=560)
    parser.add_argument("--resolution-y", type=int, default=180)
    parser.add_argument("--samples", type=int, default=16)
    return parser.parse_args(argv)


def deterministic_seed(component_name: str, field_name: str) -> int:
    digest = hashlib.sha256(f"{component_name}:{field_name}".encode()).digest()
    return int.from_bytes(digest[:8], "big") & 0x7FFFFFFF


def normalize_signed(field: np.ndarray) -> np.ndarray:
    value = np.asarray(field, dtype=np.float64)
    value -= float(value.mean())
    sigma = float(value.std())
    if sigma <= 1.0e-12:
        raise ValueError("field has no measurable variation")
    value /= sigma
    limit = max(float(np.quantile(np.abs(value), 0.995)), 1.0)
    return np.clip(value / limit, -1.0, 1.0).astype(np.float32)


def reflect_padded_band_field(
    shape: tuple[int, int],
    pixel_size_m: tuple[float, float],
    wavelength_m: float,
    bandwidth: float,
    seed: int,
) -> np.ndarray:
    """Return a cropped nonperiodic field from a reflect-padded spectrum."""

    height, width = shape
    pixel_y_m, pixel_x_m = pixel_size_m
    if min(height, width) < 8:
        raise ValueError("field is too small for a reflect-padded spectrum")
    if min(pixel_y_m, pixel_x_m, wavelength_m, bandwidth) <= 0.0:
        raise ValueError("field scales must be positive")
    random = np.random.default_rng(seed)
    white = random.standard_normal((height, width))
    pad_y = max(8, height // 2)
    pad_x = max(8, width // 2)
    padded = np.pad(
        white,
        ((pad_y, pad_y), (pad_x, pad_x)),
        mode="reflect",
    )
    frequency_y = np.fft.fftfreq(padded.shape[0], d=pixel_y_m)[:, None]
    frequency_x = np.fft.rfftfreq(padded.shape[1], d=pixel_x_m)[None, :]
    radius = np.sqrt(frequency_x * frequency_x + frequency_y * frequency_y)
    center = 1.0 / wavelength_m
    sigma = max(center * bandwidth, 1.0e-9)
    band = np.exp(-0.5 * ((radius - center) / sigma) ** 2)
    band[0, 0] = 0.0
    filtered = np.fft.irfft2(
        np.fft.rfft2(padded) * band,
        s=padded.shape,
    )
    cropped = filtered[pad_y : pad_y + height, pad_x : pad_x + width]
    return normalize_signed(cropped)


def compact_scale_morphology(
    shape: tuple[int, int],
    pixel_size_m: tuple[float, float],
    component_name: str,
) -> tuple[np.ndarray, list[dict[str, float]]]:
    combined = np.zeros(shape, dtype=np.float64)
    bands = []
    for index, (wavelength_m, weight) in enumerate(
        zip(FIELD_WAVELENGTHS_M, FIELD_WEIGHTS, strict=True)
    ):
        field = reflect_padded_band_field(
            shape,
            pixel_size_m,
            wavelength_m,
            bandwidth=0.34 + index * 0.08,
            seed=deterministic_seed(component_name, f"morphology_{index}"),
        )
        combined += weight * field
        bands.append({
            "wavelength_m": wavelength_m,
            "weight": weight,
        })
    return normalize_signed(combined), bands


def detail_priority_field(
    shape: tuple[int, int],
    pixel_size_m: tuple[float, float],
    component_name: str,
) -> np.ndarray:
    broad = reflect_padded_band_field(
        shape,
        pixel_size_m,
        0.420,
        0.42,
        deterministic_seed(component_name, "detail_priority_broad"),
    )
    medium = reflect_padded_band_field(
        shape,
        pixel_size_m,
        0.140,
        0.38,
        deterministic_seed(component_name, "detail_priority_medium"),
    )
    source = 0.72 * broad + 0.28 * medium
    threshold = float(np.quantile(source, 0.62))
    spread = max(float(source.std()) * 0.28, 1.0e-5)
    return common.smoothstep(
        threshold - spread,
        threshold + spread,
        source,
    ).astype(np.float32)


def finite_flow_rails(
    u_m: np.ndarray,
    v_m: np.ndarray,
    component_name: str,
) -> np.ndarray:
    """Build three finite open compression rails with tapered endpoints."""

    minimum_u = float(u_m.min())
    maximum_u = float(u_m.max())
    minimum_v = float(v_m.min())
    maximum_v = float(v_m.max())
    length = max(maximum_u - minimum_u, 1.0e-6)
    width = max(maximum_v - minimum_v, 1.0e-6)
    normalized_u = (u_m - minimum_u) / length
    normalized_v = (v_m - minimum_v) / width
    phase = deterministic_seed(component_name, "finite_flow_rails") / 0x7FFFFFFF
    rail_specs = (
        (0.05, 0.78, 0.22, 0.30, 0.07, 0.070, 0.72),
        (0.18, 0.94, 0.50, 0.45, -0.06, 0.055, 1.00),
        (0.02, 0.69, 0.77, 0.68, 0.05, 0.065, 0.58),
    )
    result = np.zeros_like(u_m, dtype=np.float64)
    weight = np.zeros_like(u_m, dtype=np.float64)
    for index, (
        start,
        end,
        start_v,
        end_v,
        bend,
        width_v,
        pressure,
    ) in enumerate(rail_specs):
        component_offset = (phase - 0.5) * 0.035 * (index - 1)
        t = np.clip((normalized_u - start) / (end - start), 0.0, 1.0)
        center = (
            start_v
            + (end_v - start_v) * t
            + bend * 4.0 * t * (1.0 - t)
            + component_offset
        )
        variable_width = width_v * (
            0.78 + 0.32 * t + 0.10 * (2.0 * t - 1.0) ** 2
        )
        cross = np.exp(
            -0.5 * ((normalized_v - center) / variable_width) ** 2
        )
        fade_in = common.smoothstep(start, start + 0.11, normalized_u)
        fade_out = 1.0 - common.smoothstep(end - 0.13, end, normalized_u)
        along_pressure = pressure * (0.72 + 0.28 * t)
        rail = cross * fade_in * fade_out * along_pressure
        result += rail
        weight += fade_in * fade_out * pressure
    normalized = np.divide(
        result,
        np.maximum(weight, 1.0e-6),
        out=np.zeros_like(result),
        where=weight > 1.0e-6,
    )
    return np.clip(normalized, 0.0, 1.0).astype(np.float32)


def nonperiodic_shift_correlation(field: np.ndarray) -> float:
    correlations = []
    height, width = field.shape
    for shift_y, shift_x in (
        (0, max(1, width // 4)),
        (0, max(1, width // 2)),
        (max(1, height // 3), 0),
        (max(1, height // 3), max(1, width // 3)),
    ):
        first = field[: height - shift_y or None, : width - shift_x or None]
        second = field[shift_y:, shift_x:]
        left = first.reshape(-1).astype(np.float64)
        right = second.reshape(-1).astype(np.float64)
        if left.size < 16 or float(left.std()) <= 1.0e-9 or float(right.std()) <= 1.0e-9:
            continue
        correlations.append(abs(float(np.corrcoef(left, right)[0, 1])))
    return max(correlations, default=0.0)


def build_repaired_fields(
    frame: dict[str, Any],
    component_name: str,
) -> dict[str, Any]:
    minimum_u_m, minimum_v_m = frame["minimum_m"]
    maximum_u_m, maximum_v_m = frame["maximum_m"]
    length_m, width_m = frame["span_m"]
    resolution_x, resolution_y = base.field_resolution(length_m, width_m)
    u_axis = np.linspace(minimum_u_m, maximum_u_m, resolution_x, dtype=np.float32)
    v_axis = np.linspace(minimum_v_m, maximum_v_m, resolution_y, dtype=np.float32)
    u_m, v_m = np.meshgrid(u_axis, v_axis)
    pixel_size_m = (
        width_m / max(resolution_y - 1, 1),
        length_m / max(resolution_x - 1, 1),
    )

    donor = base.build_component_fields(frame)
    oxide = common.continuous_oxide_layer_fields(
        u_m,
        v_m,
        sweep.thickness_rails(
            minimum_u_m,
            maximum_u_m,
            minimum_v_m,
            maximum_v_m,
            RECIPE,
        ),
        longitudinal_modes=[],
        minimum_thickness_m=RECIPE.thickness_minimum_m,
        maximum_thickness_m=RECIPE.thickness_maximum_m,
    )
    thermal = common.smoothstep(0.0, 1.0, oxide["thickness_response"]).astype(np.float32)
    morphology, morphology_bands = compact_scale_morphology(
        thermal.shape,
        pixel_size_m,
        component_name,
    )
    priority = detail_priority_field(thermal.shape, pixel_size_m, component_name)
    flow = finite_flow_rails(u_m, v_m, component_name)
    grain = reflect_padded_band_field(
        thermal.shape,
        pixel_size_m,
        GRAIN_WAVELENGTH_M,
        0.55,
        deterministic_seed(component_name, "aggregate_grain"),
    )
    # Intact forge scale is a continuous bonded layer. Priority controls where
    # its morphology becomes legible; it must not cut the layer into islands
    # that resemble corrosion blooms or chipped paint.
    morphology_strength = 0.16 + 0.84 * priority
    active_morphology = morphology * morphology_strength

    thin = np.asarray(RECIPE.thin_srgb, dtype=np.float32)
    thick = np.asarray(RECIPE.thick_srgb, dtype=np.float32)
    base_srgb = common.mix(
        np.broadcast_to(thin, thermal.shape + (3,)),
        np.broadcast_to(thick, thermal.shape + (3,)),
        thermal,
    )
    morphology_delta = np.asarray(RECIPE.morphology_colour_delta_srgb, dtype=np.float32)
    flow_delta = np.asarray(RECIPE.compression_colour_delta_srgb, dtype=np.float32)
    oxide_srgb = np.clip(
        base_srgb
        + active_morphology[..., None] * morphology_delta
        + flow[..., None] * flow_delta,
        0.0,
        1.0,
    )
    oxide_base_linear = common.srgb_to_linear(oxide_srgb).astype(np.float32)
    oxide_roughness = np.clip(
        RECIPE.roughness_base
        + (thermal - 0.5) * RECIPE.thermal_roughness_amplitude
        + flow * RECIPE.compression_roughness_amplitude
        + active_morphology * RECIPE.morphology_roughness_amplitude
        + grain * priority * RECIPE.grain_roughness_amplitude,
        RECIPE.roughness_minimum,
        RECIPE.roughness_maximum,
    ).astype(np.float32)
    combined_height_m = (
        donor["forging_height_m"]
        + active_morphology * RECIPE.surface_relief_amplitude_m
    ).astype(np.float32)
    combined_normal = common.height_to_normal_nonperiodic(
        combined_height_m,
        meters_per_pixel_x=pixel_size_m[1],
        meters_per_pixel_y=pixel_size_m[0],
    )
    layer_response = np.stack(
        [
            thermal,
            flow,
            np.clip(0.5 + 0.5 * active_morphology, 0.0, 1.0),
        ],
        axis=-1,
    ).astype(np.float32)
    exposure = np.full(thermal.shape, EXPOSED_CONDUCTOR, dtype=np.float32)
    if float(oxide["coverage"].min()) != 1.0:
        raise RuntimeError("repair lost complete oxide coverage")
    if float(oxide["metalness"].max()) != 0.0:
        raise RuntimeError("repair made intact oxide conductive")
    return {
        "oxide_base_linear": oxide_base_linear,
        "oxide_roughness": oxide_roughness,
        "combined_normal": combined_normal,
        "layer_response": layer_response,
        "morphology_diagnostic": np.clip(0.5 + 0.5 * active_morphology, 0.0, 1.0).astype(np.float32),
        "thermal_macro_response": thermal,
        "compression_flow_response": flow,
        "medium_morphology_response": morphology,
        "micro_grain_response": grain,
        "detail_priority": priority,
        "combined_surface_height_m": combined_height_m,
        "oxide_coverage": oxide["coverage"],
        "oxide_metalness": oxide["metalness"],
        "oxide_surface_height_m": oxide["surface_height_m"],
        "oxide_thickness_m": oxide["thickness_m"],
        "exposure": exposure,
        "morphology_bands": morphology_bands,
        "periodic_shift_correlation": nonperiodic_shift_correlation(morphology),
    }


def build_board(output_root: Path) -> dict[str, Any]:
    cumulative_root = MATERIAL_ROOT / "output" / "cumulative_intact_hinge_preview_v1"
    views = (
        ("previous_front", cumulative_root / "cumulative_intact_hinge_neutral_front.png"),
        ("repaired_front", output_root / "front.png"),
        ("repaired_morphology", output_root / "medium_morphology.png"),
        ("previous_grazing", cumulative_root / "cumulative_intact_hinge_grazing.png"),
        ("repaired_grazing", output_root / "grazing.png"),
        ("repaired_roughness", output_root / "roughness.png"),
    )
    board_path = output_root / "comparison_board.png"
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required to assemble the repair board")
    inputs = []
    filters = []
    labels = []
    for index, (label, tile) in enumerate(views):
        if not tile.is_file():
            raise FileNotFoundError(f"repair proof is missing: {tile}")
        inputs.extend(("-i", str(tile)))
        if label.startswith("previous"):
            frame_colour = "0x8b6f47"
        elif label in {"repaired_front", "repaired_grazing"}:
            frame_colour = "0x466b8a"
        else:
            frame_colour = "0x705078"
        filters.append(
            f"[{index}:v]scale=548:168:force_original_aspect_ratio=decrease,"
            f"pad=560:180:(ow-iw)/2:(oh-ih)/2:color={frame_colour}[t{index}]"
        )
        labels.append(f"[t{index}]")
    filters.append("".join(labels[:3]) + "hstack=inputs=3[row0]")
    filters.append("".join(labels[3:]) + "hstack=inputs=3[row1]")
    filters.append("[row0][row1]vstack=inputs=2[board]")
    completed = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            *inputs,
            "-filter_complex",
            ";".join(filters),
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(board_path),
        ],
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError("repair-board assembly failed: " + completed.stderr.strip())
    return {
        "path": str(board_path),
        "bytes": board_path.stat().st_size,
        "sha256": base.sha256_file(board_path),
        "layout": [
            ["previous_front", "repaired_front", "repaired_morphology"],
            ["previous_grazing", "repaired_grazing", "repaired_roughness"],
        ],
        "frame_legend": {
            "brown": "previous cumulative",
            "blue": "repaired physical view",
            "purple": "repaired isolated diagnostic",
        },
    }


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    source_hash_before = base.sha256_file(base.SOURCE_BLEND)
    geometry_hash_before = base.sha256_file(base.GEOMETRY_BLEND)

    bpy.ops.wm.open_mainfile(filepath=str(base.SOURCE_BLEND))
    scene = bpy.context.scene
    sweep.configure_scene(scene, args)
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    lights = {
        "neutral": bpy.data.objects["IGGY_IronTarget_NeutralKey"],
        "fill": bpy.data.objects["IGGY_IronTarget_CoolFill"],
        "grazing": bpy.data.objects["IGGY_IronTarget_GrazingStrip"],
    }
    keep = set(targets) | set(lights.values()) | {scene.camera}
    for obj in scene.objects:
        obj.hide_render = obj not in keep
        obj.hide_viewport = obj not in keep

    frames = {obj.name: base.install_component_frames(obj) for obj in targets}
    world_bounds = [
        obj.matrix_world @ Vector(corner)
        for obj in targets
        for corner in obj.bound_box
    ]
    minimum_x = min(point.x for point in world_bounds)
    maximum_x = max(point.x for point in world_bounds)
    minimum_z = min(point.z for point in world_bounds)
    maximum_z = max(point.z for point in world_bounds)
    center = ((minimum_x + maximum_x) * 0.5, 0.0, (minimum_z + maximum_z) * 0.5)
    span_x = maximum_x - minimum_x
    span_z = maximum_z - minimum_z
    aspect = args.resolution_x / args.resolution_y

    physical_materials = {}
    diagnostic_materials = {
        "roughness": {},
        "medium_morphology": {},
        "causal_response": {},
    }
    tracked_materials = []
    tracked_images = []
    fields_by_component = {}
    topology = {}
    for obj in targets:
        fields = build_repaired_fields(frames[obj.name], obj.name)
        fields_by_component[obj.name] = fields
        slug = obj.name.replace(".", "_")
        prefix = f"IGGY_IMG_BRepair_{slug}"
        images = {
            "oxide_base": base.make_float_image(f"{prefix}_OxideBase", fields["oxide_base_linear"], color=True),
            "oxide_roughness": base.make_float_image(f"{prefix}_OxideRoughness", fields["oxide_roughness"], color=False),
            "combined_normal": base.make_float_image(f"{prefix}_CombinedNormal", fields["combined_normal"] * 0.5 + 0.5, color=True),
            "layer_response": base.make_float_image(f"{prefix}_LayerResponse", fields["layer_response"], color=True),
            "morphology": base.make_float_image(f"{prefix}_Morphology", fields["morphology_diagnostic"], color=False),
        }
        tracked_images.extend(images.values())
        physical, component_topology = sweep.build_candidate_material(obj.name, RECIPE, images)
        roughness_material = base.build_lane_material(f"IGGY_MAT_BRepair_{slug}_Roughness", images["oxide_roughness"])
        morphology_material = base.build_lane_material(f"IGGY_MAT_BRepair_{slug}_Morphology", images["morphology"])
        causal_material = base.build_lane_material(f"IGGY_MAT_BRepair_{slug}_Causal", images["layer_response"])
        tracked_materials.extend((physical, roughness_material, morphology_material, causal_material))
        physical_materials[obj.name] = physical
        diagnostic_materials["roughness"][obj.name] = roughness_material
        diagnostic_materials["medium_morphology"][obj.name] = morphology_material
        diagnostic_materials["causal_response"][obj.name] = causal_material
        topology[obj.name] = component_topology

    renders = {}
    for obj in targets:
        base.set_material(obj, physical_materials[obj.name])
    for output_view, donor_view in (
        ("front", "neutral_front"),
        ("grazing", "grazing"),
        ("gameplay", "gameplay"),
    ):
        base.configure_camera(scene, donor_view, center, span_x, span_z, aspect)
        base.configure_lights(lights, donor_view, center)
        renders[output_view] = base.render_still(scene, output_root / f"{output_view}.png")

    for lane in ("roughness", "medium_morphology", "causal_response"):
        for obj in targets:
            base.set_material(obj, diagnostic_materials[lane][obj.name])
        base.configure_camera(scene, "neutral_front", center, span_x, span_z, aspect)
        base.configure_lights(lights, "neutral_front", center)
        renders[lane] = base.render_still(scene, output_root / f"{lane}.png")

    coverage = np.concatenate([field["oxide_coverage"].reshape(-1) for field in fields_by_component.values()])
    metalness = np.concatenate([field["oxide_metalness"].reshape(-1) for field in fields_by_component.values()])
    exposure = np.concatenate([field["exposure"].reshape(-1) for field in fields_by_component.values()])
    roughness = np.concatenate([field["oxide_roughness"].reshape(-1) for field in fields_by_component.values()])
    priority = np.concatenate([field["detail_priority"].reshape(-1) for field in fields_by_component.values()])
    maximum_periodic_correlation = max(field["periodic_shift_correlation"] for field in fields_by_component.values())
    board = build_board(output_root)
    source_hash_after = base.sha256_file(base.SOURCE_BLEND)
    geometry_hash_after = base.sha256_file(base.GEOMETRY_BLEND)
    canonical_unchanged = source_hash_before == source_hash_after and geometry_hash_before == geometry_hash_after
    if not canonical_unchanged:
        raise RuntimeError("B-only repair changed a canonical source")

    manifest = {
        "schema": "iggy3d.quiet-compact-oxide-repair-proof.v1",
        "status": "B_REPAIR_PROOF_NOT_CANONICAL",
        "consumer": {
            "collection": base.TARGET_COLLECTION,
            "objects": list(base.TARGET_OBJECTS),
            "assembly_span_m": [span_x, 0.116, span_z],
        },
        "recipe": {
            "candidate_id": RECIPE.candidate_id,
            "repair_revision": 2,
            "preserves_cumulative_intact_palette": True,
            "morphology_primitive": "reflect_padded_band_limited_random_field",
            "morphology_bands_m": list(FIELD_WAVELENGTHS_M),
            "morphology_weights": list(FIELD_WEIGHTS),
            "finite_flow_rail_count": FLOW_RAIL_COUNT,
            "grain_wavelength_m": GRAIN_WAVELENGTH_M,
            "grain_outputs": ["roughness"],
            "grain_prohibited_outputs": ["base_color", "height", "normal", "metalness"],
            "continuous_morphology_floor": 0.16,
            "surface_relief_amplitude_m": RECIPE.surface_relief_amplitude_m,
        },
        "metrics": {
            "coverage_minimum": float(coverage.min()),
            "coverage_maximum": float(coverage.max()),
            "metalness_maximum": float(metalness.max()),
            "exposure_maximum": float(exposure.max()),
            "roughness_range": [float(roughness.min()), float(roughness.max())],
            "quiet_fraction": float(np.mean(priority < 0.20)),
            "maximum_periodic_shift_correlation": float(maximum_periodic_correlation),
        },
        "renders": renders,
        "comparison_board": board,
        "component_topology": topology,
        "sources": {
            "canonical_blend": {
                "path": str(base.SOURCE_BLEND),
                "sha256_before": source_hash_before,
                "sha256_after": source_hash_after,
            },
            "geometry_blend": {
                "path": str(base.GEOMETRY_BLEND),
                "sha256_before": geometry_hash_before,
                "sha256_after": geometry_hash_after,
            },
        },
        "canonical_source_unchanged": canonical_unchanged,
        "uses_ai_generated_imagery": False,
        "uses_damage": False,
        "uses_rust": False,
        "uses_contact_polish": False,
        "saved_blend": None,
        "unreal_parity_verified": False,
        "decision_boundary": "Visual review is required before cumulative or canonical integration.",
    }
    (output_root / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    sweep.cleanup_candidate_resources(targets, tracked_materials, tracked_images)
    print(
        "QUIET_COMPACT_OXIDE_REPAIR="
        f"quiet:{manifest['metrics']['quiet_fraction']:.3f},"
        f"periodic:{manifest['metrics']['maximum_periodic_shift_correlation']:.3f},"
        f"source_unchanged:{canonical_unchanged}"
    )
    print(f"QUIET_COMPACT_OXIDE_REPAIR_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
