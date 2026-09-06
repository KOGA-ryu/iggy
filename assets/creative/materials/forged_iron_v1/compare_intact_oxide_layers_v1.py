#!/usr/bin/env python3
"""Render a cheap actual-hinge comparison of intact oxide layer strategies."""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
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
CUMULATIVE_SCRIPT = (
    MATERIAL_ROOT / "build_cumulative_intact_hinge_preview_v1.py"
)
DEFAULT_OUTPUT = (
    MATERIAL_ROOT / "output" / "intact_oxide_layer_sweep_v1"
)
EXPOSED_CONDUCTOR = 0.0


def _load_cumulative():
    spec = importlib.util.spec_from_file_location(
        "iggy_cumulative_intact_hinge_donor",
        CUMULATIVE_SCRIPT,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load cumulative donor: {CUMULATIVE_SCRIPT}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


base = _load_cumulative()
common = base.common


Mode = tuple[float, float, float, float]


@dataclass(frozen=True)
class CandidateRecipe:
    candidate_id: str
    label: str
    thickness_minimum_m: float
    thickness_maximum_m: float
    thin_srgb: tuple[float, float, float]
    thick_srgb: tuple[float, float, float]
    morphology_colour_delta_srgb: tuple[float, float, float]
    compression_colour_delta_srgb: tuple[float, float, float]
    compression_modes: tuple[Mode, ...]
    morphology_modes: tuple[Mode, ...]
    grain_modes: tuple[Mode, ...]
    rest_modes: tuple[Mode, ...]
    surface_relief_amplitude_m: float
    roughness_base: float
    thermal_roughness_amplitude: float
    compression_roughness_amplitude: float
    morphology_roughness_amplitude: float
    grain_roughness_amplitude: float
    roughness_minimum: float
    roughness_maximum: float
    oxide_anisotropy: float


CANDIDATES = (
    CandidateRecipe(
        candidate_id="A_smooth_control",
        label="A  SMOOTH CONTROL",
        thickness_minimum_m=0.000020,
        thickness_maximum_m=0.000090,
        thin_srgb=(0.090, 0.100, 0.116),
        thick_srgb=(0.158, 0.174, 0.202),
        morphology_colour_delta_srgb=(0.0, 0.0, 0.0),
        compression_colour_delta_srgb=(0.0, 0.0, 0.0),
        compression_modes=(),
        morphology_modes=(),
        grain_modes=(),
        rest_modes=(),
        surface_relief_amplitude_m=0.0,
        roughness_base=0.7075,
        thermal_roughness_amplitude=0.105,
        compression_roughness_amplitude=0.0,
        morphology_roughness_amplitude=0.0,
        grain_roughness_amplitude=0.0,
        roughness_minimum=0.655,
        roughness_maximum=0.760,
        oxide_anisotropy=0.0,
    ),
    CandidateRecipe(
        candidate_id="B_quiet_compact_scale",
        label="B  QUIET COMPACT SCALE",
        thickness_minimum_m=0.000008,
        thickness_maximum_m=0.000035,
        thin_srgb=(0.067, 0.074, 0.086),
        thick_srgb=(0.116, 0.124, 0.140),
        morphology_colour_delta_srgb=(0.006, 0.003, -0.002),
        compression_colour_delta_srgb=(-0.003, 0.001, 0.005),
        compression_modes=(
            (0.310, 84.0, 1.00, 0.20),
            (0.173, 78.0, 0.46, 1.70),
        ),
        morphology_modes=(
            (0.340, 17.0, 1.00, 0.35),
            (0.190, -31.0, 0.52, 2.10),
            (0.112, 64.0, 0.28, 4.40),
        ),
        grain_modes=(
            (0.031, 12.0, 1.00, 0.45),
            (0.019, 57.0, 0.55, 2.55),
            (0.012, -38.0, 0.27, 5.10),
        ),
        rest_modes=(
            (0.920, 21.0, 1.00, 0.60),
            (0.530, -42.0, 0.38, 3.20),
        ),
        surface_relief_amplitude_m=0.000020,
        roughness_base=0.635,
        thermal_roughness_amplitude=0.038,
        compression_roughness_amplitude=0.012,
        morphology_roughness_amplitude=0.018,
        grain_roughness_amplitude=0.012,
        roughness_minimum=0.575,
        roughness_maximum=0.705,
        oxide_anisotropy=0.08,
    ),
    CandidateRecipe(
        candidate_id="C_heavy_forged_scale",
        label="C  HEAVY FORGED SCALE",
        thickness_minimum_m=0.000020,
        thickness_maximum_m=0.000090,
        thin_srgb=(0.060, 0.065, 0.074),
        thick_srgb=(0.137, 0.145, 0.162),
        morphology_colour_delta_srgb=(0.014, 0.006, -0.003),
        compression_colour_delta_srgb=(-0.004, 0.002, 0.007),
        compression_modes=(
            (0.370, 80.0, 1.00, 0.45),
            (0.205, 72.0, 0.58, 2.20),
            (0.124, 87.0, 0.25, 4.80),
        ),
        morphology_modes=(
            (0.460, 11.0, 1.00, 0.15),
            (0.260, -26.0, 0.68, 1.85),
            (0.133, 52.0, 0.38, 4.15),
        ),
        grain_modes=(
            (0.038, 8.0, 1.00, 0.70),
            (0.023, 49.0, 0.62, 2.75),
            (0.014, -44.0, 0.34, 5.25),
        ),
        rest_modes=(
            (1.100, 14.0, 1.00, 0.20),
            (0.610, -37.0, 0.42, 2.90),
        ),
        surface_relief_amplitude_m=0.000055,
        roughness_base=0.685,
        thermal_roughness_amplitude=0.060,
        compression_roughness_amplitude=0.020,
        morphology_roughness_amplitude=0.032,
        grain_roughness_amplitude=0.020,
        roughness_minimum=0.590,
        roughness_maximum=0.785,
        oxide_anisotropy=0.10,
    ),
    CandidateRecipe(
        candidate_id="D_compressed_directional_scale",
        label="D  COMPRESSED DIRECTIONAL",
        thickness_minimum_m=0.000010,
        thickness_maximum_m=0.000045,
        thin_srgb=(0.069, 0.077, 0.091),
        thick_srgb=(0.120, 0.130, 0.148),
        morphology_colour_delta_srgb=(0.005, 0.003, -0.001),
        compression_colour_delta_srgb=(-0.006, 0.002, 0.009),
        compression_modes=(
            (0.255, 88.0, 1.00, 0.35),
            (0.142, 82.0, 0.72, 1.95),
            (0.081, 86.0, 0.38, 4.60),
        ),
        morphology_modes=(
            (0.390, 8.0, 1.00, 0.25),
            (0.215, -18.0, 0.46, 2.40),
            (0.128, 39.0, 0.22, 5.00),
        ),
        grain_modes=(
            (0.029, 79.0, 1.00, 0.50),
            (0.018, 87.0, 0.56, 2.65),
            (0.011, 74.0, 0.28, 5.30),
        ),
        rest_modes=(
            (0.980, 18.0, 1.00, 0.45),
            (0.570, -33.0, 0.36, 3.40),
        ),
        surface_relief_amplitude_m=0.000028,
        roughness_base=0.610,
        thermal_roughness_amplitude=0.034,
        compression_roughness_amplitude=0.060,
        morphology_roughness_amplitude=0.015,
        grain_roughness_amplitude=0.012,
        roughness_minimum=0.515,
        roughness_maximum=0.710,
        oxide_anisotropy=0.28,
    ),
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


def component_phase(component_name: str) -> float:
    digest = hashlib.sha256(component_name.encode("utf-8")).digest()
    integer = int.from_bytes(digest[:8], byteorder="big", signed=False)
    return math.tau * (integer / float(2**64 - 1))


def open_mode_field(
    u_m: np.ndarray,
    v_m: np.ndarray,
    modes: tuple[Mode, ...],
    phase_offset: float,
) -> np.ndarray:
    if u_m.shape != v_m.shape or u_m.ndim != 2:
        raise ValueError("open-mode coordinates must be matching HxW arrays")
    if not modes:
        return np.zeros_like(u_m, dtype=np.float32)
    value = np.zeros_like(u_m, dtype=np.float64)
    weight = 0.0
    for wavelength_m, angle_degrees, amplitude, phase in modes:
        if wavelength_m <= 0.0 or not math.isfinite(wavelength_m):
            raise ValueError("open-mode wavelength must be finite and positive")
        if not math.isfinite(amplitude) or amplitude == 0.0:
            raise ValueError("open-mode amplitude must be finite and nonzero")
        angle = math.radians(angle_degrees)
        projected_m = u_m * math.cos(angle) + v_m * math.sin(angle)
        value += amplitude * np.sin(
            math.tau * projected_m / wavelength_m + phase + phase_offset
        )
        weight += abs(amplitude)
    return (value / weight).astype(np.float32)


def thickness_rails(
    minimum_u_m: float,
    maximum_u_m: float,
    minimum_v_m: float,
    maximum_v_m: float,
    recipe: CandidateRecipe,
) -> list[dict[str, Any]]:
    length_m = maximum_u_m - minimum_u_m
    width_m = maximum_v_m - minimum_v_m
    span_m = recipe.thickness_maximum_m - recipe.thickness_minimum_m

    def thickness(normalized: float) -> float:
        return recipe.thickness_minimum_m + span_m * normalized

    return [
        {
            "y_m": minimum_v_m,
            "knots_m": [
                [minimum_u_m, thickness(0.34)],
                [minimum_u_m + length_m * 0.23, thickness(0.60)],
                [minimum_u_m + length_m * 0.58, thickness(0.25)],
                [maximum_u_m, thickness(0.46)],
            ],
        },
        {
            "y_m": minimum_v_m + width_m * 0.5,
            "knots_m": [
                [minimum_u_m, thickness(0.45)],
                [minimum_u_m + length_m * 0.34, thickness(0.18)],
                [minimum_u_m + length_m * 0.72, thickness(0.73)],
                [maximum_u_m, thickness(0.39)],
            ],
        },
        {
            "y_m": maximum_v_m,
            "knots_m": [
                [minimum_u_m, thickness(0.41)],
                [minimum_u_m + length_m * 0.27, thickness(0.53)],
                [minimum_u_m + length_m * 0.65, thickness(0.29)],
                [maximum_u_m, thickness(0.49)],
            ],
        },
    ]


def build_candidate_fields(
    frame: dict[str, Any],
    component_name: str,
    recipe: CandidateRecipe,
) -> dict[str, np.ndarray]:
    minimum_u_m, minimum_v_m = frame["minimum_m"]
    maximum_u_m, maximum_v_m = frame["maximum_m"]
    length_m, width_m = frame["span_m"]
    resolution_x, resolution_y = base.field_resolution(length_m, width_m)
    u_axis = np.linspace(
        minimum_u_m, maximum_u_m, resolution_x, dtype=np.float32
    )
    v_axis = np.linspace(
        minimum_v_m, maximum_v_m, resolution_y, dtype=np.float32
    )
    u_m, v_m = np.meshgrid(u_axis, v_axis)

    donor = base.build_component_fields(frame)
    phase = component_phase(component_name)
    if recipe.candidate_id == "A_smooth_control":
        thermal_macro_response = np.clip(
            (
                donor["oxide_thickness_m"]
                - recipe.thickness_minimum_m
            )
            / (recipe.thickness_maximum_m - recipe.thickness_minimum_m),
            0.0,
            1.0,
        ).astype(np.float32)
        compression_flow_response = np.zeros_like(thermal_macro_response)
        medium_morphology_response = np.zeros_like(thermal_macro_response)
        micro_grain_response = np.zeros_like(thermal_macro_response)
        detail_priority = np.ones_like(thermal_macro_response)
        combined_surface_height_m = donor["forging_height_m"].copy()
        oxide_base_linear = donor["oxide_base_linear"].copy()
        oxide_roughness = donor["oxide_roughness"].copy()
        oxide_thickness_m = donor["oxide_thickness_m"].copy()
        oxide_coverage = donor["oxide_coverage"].copy()
        oxide_metalness = donor["oxide_metalness"].copy()
        oxide_surface_height_m = donor["oxide_surface_height_m"].copy()
    else:
        oxide = common.continuous_oxide_layer_fields(
            u_m,
            v_m,
            thickness_rails(
                minimum_u_m,
                maximum_u_m,
                minimum_v_m,
                maximum_v_m,
                recipe,
            ),
            longitudinal_modes=[],
            minimum_thickness_m=recipe.thickness_minimum_m,
            maximum_thickness_m=recipe.thickness_maximum_m,
        )
        thermal_macro_response = common.smoothstep(
            0.0, 1.0, oxide["thickness_response"]
        ).astype(np.float32)
        compression_flow_response = open_mode_field(
            u_m, v_m, recipe.compression_modes, phase
        )
        medium_morphology_response = open_mode_field(
            u_m, v_m, recipe.morphology_modes, phase * 0.73 + 0.91
        )
        micro_grain_response = open_mode_field(
            u_m, v_m, recipe.grain_modes, phase * 1.31 + 1.77
        )
        rest_response = open_mode_field(
            u_m, v_m, recipe.rest_modes, phase * 0.47 + 2.33
        )
        detail_priority = (
            0.25 + 0.75 * common.smoothstep(-0.20, 0.65, rest_response)
        ).astype(np.float32)
        combined_surface_height_m = (
            donor["forging_height_m"]
            + medium_morphology_response
            * detail_priority
            * recipe.surface_relief_amplitude_m
        ).astype(np.float32)

        thin = np.asarray(recipe.thin_srgb, dtype=np.float32)
        thick = np.asarray(recipe.thick_srgb, dtype=np.float32)
        base_srgb = common.mix(
            np.broadcast_to(thin, thermal_macro_response.shape + (3,)),
            np.broadcast_to(thick, thermal_macro_response.shape + (3,)),
            thermal_macro_response,
        )
        morphology_delta = np.asarray(
            recipe.morphology_colour_delta_srgb, dtype=np.float32
        )
        compression_delta = np.asarray(
            recipe.compression_colour_delta_srgb, dtype=np.float32
        )
        oxide_srgb = np.clip(
            base_srgb
            + medium_morphology_response[..., np.newaxis]
            * detail_priority[..., np.newaxis]
            * morphology_delta
            + compression_flow_response[..., np.newaxis]
            * compression_delta,
            0.0,
            1.0,
        )
        oxide_base_linear = common.srgb_to_linear(oxide_srgb).astype(np.float32)
        oxide_roughness = np.clip(
            recipe.roughness_base
            + (thermal_macro_response - 0.5)
            * recipe.thermal_roughness_amplitude
            + compression_flow_response
            * recipe.compression_roughness_amplitude
            + medium_morphology_response
            * detail_priority
            * recipe.morphology_roughness_amplitude
            + micro_grain_response
            * detail_priority
            * recipe.grain_roughness_amplitude,
            recipe.roughness_minimum,
            recipe.roughness_maximum,
        ).astype(np.float32)
        oxide_thickness_m = oxide["thickness_m"]
        oxide_coverage = oxide["coverage"]
        oxide_metalness = oxide["metalness"]
        oxide_surface_height_m = oxide["surface_height_m"]

    combined_normal = common.height_to_normal_nonperiodic(
        combined_surface_height_m,
        meters_per_pixel_x=length_m / (resolution_x - 1),
        meters_per_pixel_y=width_m / (resolution_y - 1),
    )
    layer_response = np.stack(
        [
            thermal_macro_response,
            np.clip(0.5 + compression_flow_response * 0.5, 0.0, 1.0),
            np.clip(
                detail_priority * (0.5 + micro_grain_response * 0.5),
                0.0,
                1.0,
            ),
        ],
        axis=-1,
    ).astype(np.float32)
    exposure = np.full(
        thermal_macro_response.shape,
        EXPOSED_CONDUCTOR,
        dtype=np.float32,
    )
    if float(oxide_coverage.min()) != 1.0:
        raise RuntimeError("intact candidate lost continuous oxide coverage")
    if float(oxide_metalness.max()) != 0.0:
        raise RuntimeError("intact oxide unexpectedly became conductive")
    return {
        "oxide_base_linear": oxide_base_linear,
        "oxide_roughness": oxide_roughness,
        "combined_normal": combined_normal,
        "layer_response": layer_response,
        "thermal_macro_response": thermal_macro_response,
        "compression_flow_response": compression_flow_response,
        "medium_morphology_response": medium_morphology_response,
        "micro_grain_response": micro_grain_response,
        "detail_priority": detail_priority,
        "combined_surface_height_m": combined_surface_height_m,
        "oxide_coverage": oxide_coverage,
        "oxide_metalness": oxide_metalness,
        "oxide_surface_height_m": oxide_surface_height_m,
        "oxide_thickness_m": oxide_thickness_m,
        "exposure": exposure,
    }


def build_candidate_material(
    component_name: str,
    recipe: CandidateRecipe,
    images: dict[str, bpy.types.Image],
) -> tuple[bpy.types.Material, dict[str, Any]]:
    material = bpy.data.materials.new(
        f"IGGY_MAT_{recipe.candidate_id}_{component_name}_Preview"
    )
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()

    output = tree.nodes.new("ShaderNodeOutputMaterial")
    output.name = "Material_Output"
    output.location = (980.0, 80.0)
    surface = tree.nodes.new("ShaderNodeMixShader")
    surface.name = "Complete_Oxide_And_Conductor_Responses"
    surface.location = (720.0, 80.0)
    exposure = tree.nodes.new("ShaderNodeValue")
    exposure.name = "Explicit_Exposed_Conductor_Only_Owner"
    exposure.outputs[0].default_value = EXPOSED_CONDUCTOR
    exposure.location = (450.0, 390.0)

    oxide_bsdf = tree.nodes.new("ShaderNodeBsdfPrincipled")
    oxide_bsdf.name = "Complete_Intact_Oxide_Response"
    oxide_bsdf.location = (390.0, 160.0)
    base.set_principled_input(oxide_bsdf, ("Metallic",), 0.0)
    base.set_principled_input(oxide_bsdf, ("IOR",), 2.10)
    base.set_principled_input(oxide_bsdf, ("Coat Weight", "Coat"), 0.0)
    oxide_anisotropy_socket = base.set_principled_input(
        oxide_bsdf,
        ("Anisotropic IOR Level", "Anisotropic"),
        recipe.oxide_anisotropy,
    )

    conductor_bsdf = tree.nodes.new("ShaderNodeBsdfPrincipled")
    conductor_bsdf.name = "Complete_Hidden_Iron_Response"
    conductor_bsdf.location = (390.0, -170.0)
    base.set_principled_input(
        conductor_bsdf,
        ("Base Color",),
        (*base.CONDUCTOR_F0_LINEAR, 1.0),
    )
    base.set_principled_input(conductor_bsdf, ("Metallic",), 1.0)
    base.set_principled_input(
        conductor_bsdf, ("Roughness",), base.CONDUCTOR_ROUGHNESS
    )
    base.set_principled_input(
        conductor_bsdf,
        ("Anisotropic IOR Level", "Anisotropic"),
        base.ANISOTROPY,
    )

    tangent = tree.nodes.new("ShaderNodeTangent")
    tangent.name = "Component_Owned_Material_Tangent"
    tangent.direction_type = "UV_MAP"
    tangent.uv_map = base.TANGENT_UV
    tangent.location = (40.0, -420.0)
    tree.links.new(tangent.outputs["Tangent"], oxide_bsdf.inputs["Tangent"])
    tree.links.new(tangent.outputs["Tangent"], conductor_bsdf.inputs["Tangent"])

    uv = tree.nodes.new("ShaderNodeUVMap")
    uv.name = "Nonrepeating_Component_Field_UV"
    uv.uv_map = base.FIELD_UV
    uv.location = (-760.0, 140.0)
    color_image = base.add_image_node(
        tree,
        "Continuous_Oxide_Optical_Colour",
        images["oxide_base"],
        uv,
        (-520.0, 290.0),
    )
    roughness_image = base.add_image_node(
        tree,
        "Independent_Oxide_Roughness_Stack",
        images["oxide_roughness"],
        uv,
        (-520.0, 70.0),
    )
    normal_image = base.add_image_node(
        tree,
        "Combined_Broad_And_Oxide_Surface_Normal",
        images["combined_normal"],
        uv,
        (-520.0, -160.0),
    )
    diagnostic_image = base.add_image_node(
        tree,
        "Causal_Layer_Response_Diagnostic",
        images["layer_response"],
        uv,
        (-520.0, -390.0),
    )
    diagnostic_image.hide = True

    normal_map = tree.nodes.new("ShaderNodeNormalMap")
    normal_map.name = "Combined_Surface_Normal_Decode"
    normal_map.space = "TANGENT"
    normal_map.uv_map = base.FIELD_UV
    normal_map.inputs["Strength"].default_value = 1.0
    normal_map.location = (-170.0, -150.0)

    tree.links.new(color_image.outputs["Color"], oxide_bsdf.inputs["Base Color"])
    tree.links.new(
        roughness_image.outputs["Color"], oxide_bsdf.inputs["Roughness"]
    )
    tree.links.new(normal_image.outputs["Color"], normal_map.inputs["Color"])
    tree.links.new(normal_map.outputs["Normal"], oxide_bsdf.inputs["Normal"])
    tree.links.new(normal_map.outputs["Normal"], conductor_bsdf.inputs["Normal"])
    tree.links.new(exposure.outputs[0], surface.inputs["Fac"])
    tree.links.new(oxide_bsdf.outputs["BSDF"], surface.inputs[1])
    tree.links.new(conductor_bsdf.outputs["BSDF"], surface.inputs[2])
    tree.links.new(surface.outputs["Shader"], output.inputs["Surface"])
    return material, {
        "oxide_anisotropy": recipe.oxide_anisotropy,
        "oxide_anisotropy_socket": oxide_anisotropy_socket,
        "exposure": float(exposure.outputs[0].default_value),
        "image_texture_count": 4,
        "principled_count": 2,
        "mix_shader_count": 1,
    }


def build_comparison_board(
    output_root: Path,
    candidate_recipes: tuple[CandidateRecipe, ...],
) -> dict[str, Any]:
    view_names = ("front", "grazing", "gameplay", "layer_response")
    board_path = output_root / "comparison_board.png"
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required to assemble the comparison board")
    inputs: list[str] = []
    filters: list[str] = []
    input_index = 0
    row_outputs = []
    for row_index, recipe in enumerate(candidate_recipes):
        padded = []
        for view_name in view_names:
            tile_path = output_root / recipe.candidate_id / f"{view_name}.png"
            if not tile_path.is_file():
                raise FileNotFoundError(f"comparison tile is missing: {tile_path}")
            inputs.extend(("-i", str(tile_path)))
            output_label = f"tile_{input_index}"
            filters.append(
                f"[{input_index}:v]pad=iw+4:ih+4:2:2:color=0x181b22[{output_label}]"
            )
            padded.append(f"[{output_label}]")
            input_index += 1
        row_output = f"row_{row_index}"
        filters.append("".join(padded) + f"hstack=inputs=4[{row_output}]")
        row_outputs.append(f"[{row_output}]")
    filters.append("".join(row_outputs) + "vstack=inputs=4[board]")
    command = [
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
    ]
    completed = subprocess.run(
        command,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            "comparison-board assembly failed: " + completed.stderr.strip()
        )
    result = {
        "path": str(board_path),
        "bytes": board_path.stat().st_size,
        "sha256": base.sha256_file(board_path),
        "row_order": [recipe.candidate_id for recipe in candidate_recipes],
        "column_order": list(view_names),
    }
    return result


def cleanup_candidate_resources(
    targets: list[bpy.types.Object],
    materials: list[bpy.types.Material],
    images: list[bpy.types.Image],
) -> None:
    material_names = [material.name for material in materials]
    image_names = [image.name for image in images]
    for obj in targets:
        obj.data.materials.clear()
    for material in materials:
        if material.name in bpy.data.materials:
            bpy.data.materials.remove(material, do_unlink=True)
    for image in images:
        if image.name in bpy.data.images:
            bpy.data.images.remove(image, do_unlink=True)
    leaked_materials = [name for name in material_names if name in bpy.data.materials]
    leaked_images = [name for name in image_names if name in bpy.data.images]
    if leaked_materials or leaked_images:
        raise RuntimeError(
            f"candidate cleanup failed: materials={leaked_materials}, images={leaked_images}"
        )


def configure_scene(scene: bpy.types.Scene, args: argparse.Namespace) -> None:
    try:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE"
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.resolution_x = args.resolution_x
    scene.render.resolution_y = args.resolution_y
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = 0.0
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.010, 0.014, 0.022, 1.0)
    background.inputs["Strength"].default_value = 0.08
    if hasattr(scene, "eevee") and hasattr(scene.eevee, "taa_render_samples"):
        scene.eevee.taa_render_samples = args.samples


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if not base.SOURCE_BLEND.is_file() or not base.GEOMETRY_BLEND.is_file():
        raise FileNotFoundError("accepted forged-iron proof sources are missing")
    source_hash_before = base.sha256_file(base.SOURCE_BLEND)
    geometry_hash_before = base.sha256_file(base.GEOMETRY_BLEND)

    bpy.ops.wm.open_mainfile(filepath=str(base.SOURCE_BLEND))
    scene = bpy.context.scene
    configure_scene(scene, args)
    collection = bpy.data.collections.get(base.TARGET_COLLECTION)
    if collection is None:
        raise RuntimeError(f"Missing target collection {base.TARGET_COLLECTION}")
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
    center = (
        (minimum_x + maximum_x) * 0.5,
        0.0,
        (minimum_z + maximum_z) * 0.5,
    )
    span_x = maximum_x - minimum_x
    span_z = maximum_z - minimum_z
    aspect = args.resolution_x / args.resolution_y

    candidate_manifest: dict[str, Any] = {}
    for recipe in CANDIDATES:
        candidate_root = output_root / recipe.candidate_id
        candidate_root.mkdir(parents=True, exist_ok=True)
        physical_materials: dict[str, bpy.types.Material] = {}
        diagnostic_materials: dict[str, bpy.types.Material] = {}
        tracked_materials: list[bpy.types.Material] = []
        tracked_images: list[bpy.types.Image] = []
        all_fields: dict[str, dict[str, np.ndarray]] = {}
        topology: dict[str, dict[str, Any]] = {}
        for obj in targets:
            fields = build_candidate_fields(frames[obj.name], obj.name, recipe)
            slug = obj.name.replace(".", "_")
            image_prefix = f"IGGY_IMG_{recipe.candidate_id}_{slug}"
            images = {
                "oxide_base": base.make_float_image(
                    f"{image_prefix}_OxideBase",
                    fields["oxide_base_linear"],
                    color=True,
                ),
                "oxide_roughness": base.make_float_image(
                    f"{image_prefix}_OxideRoughness",
                    fields["oxide_roughness"],
                    color=False,
                ),
                "combined_normal": base.make_float_image(
                    f"{image_prefix}_CombinedNormal",
                    fields["combined_normal"] * 0.5 + 0.5,
                    color=True,
                ),
                "layer_response": base.make_float_image(
                    f"{image_prefix}_LayerResponse",
                    fields["layer_response"],
                    color=True,
                ),
            }
            tracked_images.extend(images.values())
            physical, material_topology = build_candidate_material(
                obj.name, recipe, images
            )
            diagnostic = base.build_lane_material(
                f"IGGY_MAT_{recipe.candidate_id}_{slug}_LayerResponse",
                images["layer_response"],
            )
            tracked_materials.extend((physical, diagnostic))
            physical_materials[obj.name] = physical
            diagnostic_materials[obj.name] = diagnostic
            topology[obj.name] = material_topology
            all_fields[obj.name] = fields

        for obj in targets:
            base.set_material(obj, physical_materials[obj.name])
        renders: dict[str, dict[str, Any]] = {}
        for output_view, donor_view in (
            ("front", "neutral_front"),
            ("grazing", "grazing"),
            ("gameplay", "gameplay"),
        ):
            base.configure_camera(
                scene, donor_view, center, span_x, span_z, aspect
            )
            base.configure_lights(lights, donor_view, center)
            renders[output_view] = base.render_still(
                scene, candidate_root / f"{output_view}.png"
            )

        for obj in targets:
            base.set_material(obj, diagnostic_materials[obj.name])
        base.configure_camera(
            scene, "neutral_front", center, span_x, span_z, aspect
        )
        base.configure_lights(lights, "neutral_front", center)
        renders["layer_response"] = base.render_still(
            scene, candidate_root / "layer_response.png"
        )

        coverage = np.concatenate(
            [fields["oxide_coverage"].reshape(-1) for fields in all_fields.values()]
        )
        metalness = np.concatenate(
            [fields["oxide_metalness"].reshape(-1) for fields in all_fields.values()]
        )
        exposure = np.concatenate(
            [fields["exposure"].reshape(-1) for fields in all_fields.values()]
        )
        roughness = np.concatenate(
            [fields["oxide_roughness"].reshape(-1) for fields in all_fields.values()]
        )
        detail_priority = np.concatenate(
            [fields["detail_priority"].reshape(-1) for fields in all_fields.values()]
        )
        candidate_manifest[recipe.candidate_id] = {
            "label": recipe.label,
            "recipe": asdict(recipe),
            "renders": renders,
            "coverage_minimum": float(coverage.min()),
            "coverage_maximum": float(coverage.max()),
            "metalness_maximum": float(metalness.max()),
            "exposure_maximum": float(exposure.max()),
            "roughness_range": [float(roughness.min()), float(roughness.max())],
            "quiet_fraction": float(np.mean(detail_priority < 0.50)),
            "component_topology": topology,
            "resources_released_after_render": True,
        }
        cleanup_candidate_resources(targets, tracked_materials, tracked_images)

    comparison_board = build_comparison_board(output_root, CANDIDATES)
    source_hash_after = base.sha256_file(base.SOURCE_BLEND)
    geometry_hash_after = base.sha256_file(base.GEOMETRY_BLEND)
    canonical_source_unchanged = (
        source_hash_before == source_hash_after
        and geometry_hash_before == geometry_hash_after
    )
    if not canonical_source_unchanged:
        raise RuntimeError("exploratory sweep changed a canonical source")

    manifest = {
        "schema": "iggy3d.intact-oxide-layer-sweep.v1",
        "status": "EXPLORATORY_COMPARISON_NOT_CANONICAL",
        "consumer": {
            "collection": base.TARGET_COLLECTION,
            "objects": list(base.TARGET_OBJECTS),
            "assembly_span_m": [span_x, 0.116, span_z],
        },
        "comparison_resolution": [args.resolution_x, args.resolution_y],
        "render_engine": scene.render.engine,
        "candidates": candidate_manifest,
        "comparison_board": comparison_board,
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
        "canonical_source_unchanged": canonical_source_unchanged,
        "uses_ai_generated_imagery": False,
        "uses_damage": False,
        "unreal_parity_verified": False,
        "limitations": [
            "This board selects a surface-layer strategy; it is not a canonical material build.",
            "Eevee reduced-resolution proofs compare art direction but do not prove final Cycles or Unreal response.",
            "The 20 to 90 micrometre heavy-scale envelope is an authored giant-forge translation.",
            "Sub-texel grain is represented as aggregate roughness response, not literal resolved grains.",
        ],
    }
    manifest_path = output_root / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(
        "INTACT_OXIDE_SWEEP="
        f"candidates:{len(CANDIDATES)},source_unchanged:{canonical_source_unchanged}"
    )
    print(f"INTACT_OXIDE_SWEEP_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
