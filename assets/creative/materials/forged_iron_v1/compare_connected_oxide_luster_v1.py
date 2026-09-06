#!/usr/bin/env python3
"""Render the disposable connected-oxide and worked-luster repair gate."""

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
CUMULATIVE_SCRIPT = MATERIAL_ROOT / "build_cumulative_intact_hinge_preview_v1.py"
PRESENTATION_SCRIPT = MATERIAL_ROOT / "render_actual_hinge_acceptance_board_v1.py"
DEFAULT_OUTPUT = MATERIAL_ROOT / "output" / "connected_oxide_luster_sweep_v1"
EXPOSED_CONDUCTOR = 0.0

PANEL_ORDER = (
    "full_front",
    "close_neutral",
    "opposed_light_difference",
    "gameplay",
    "connected_oxide",
    "independent_roughness",
    "luster_diagnostic",
)
PANEL_LABELS = {
    "full_front": "FULL FRONT",
    "close_neutral": "CLOSE NEUTRAL",
    "opposed_light_difference": "OPPOSED LIGHT DIFFERENCE",
    "gameplay": "GAMEPLAY DISTANCE",
    "connected_oxide": "CONNECTED OXIDE",
    "independent_roughness": "INDEPENDENT ROUGHNESS",
    "luster_diagnostic": "LUSTER DIAGNOSTIC",
}


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


base = _load_module("iggy_connected_oxide_base", CUMULATIVE_SCRIPT)
presentation = _load_module("iggy_connected_oxide_presentation", PRESENTATION_SCRIPT)
common = base.common


@dataclass(frozen=True)
class CandidateRecipe:
    candidate_id: str
    label: str
    luster_mode: str
    luster_anisotropy_maximum: float


CANDIDATES = (
    CandidateRecipe(
        "A_frozen_root_control",
        "A  FROZEN ROOT CONTROL",
        "frozen_root",
        0.0,
    ),
    CandidateRecipe(
        "B_connected_oxide_no_luster",
        "B  CONNECTED OXIDE / NO LUSTER",
        "none",
        0.0,
    ),
    CandidateRecipe(
        "C_connected_oxide_continuous_luster",
        "C  CONTINUOUS ALIGNED LUSTER",
        "continuous",
        0.11,
    ),
    CandidateRecipe(
        "D_connected_oxide_open_rail_luster",
        "D  SPARSE OPEN-RAIL LUSTER",
        "open_rails",
        0.13,
    ),
)

# Filled only after direct board inspection, then the renderer is rerun so the
# review is part of the reproducible manifest rather than a detached claim.
VISUAL_REVIEW = {
    "decision": "select_B",
    "ranking": [
        "B_connected_oxide_no_luster",
        "C_connected_oxide_continuous_luster",
        "A_frozen_root_control",
        "D_connected_oxide_open_rail_luster",
    ],
    "recommended_candidate": "B_connected_oxide_no_luster",
    "advancement_scope": "response_strategy_only_not_material_acceptance",
    "physical_panel_comparison": {
        "B_vs_C": "pixel_identical_in_all_four_physical_panels",
        "B_vs_D": "pixel_identical_in_all_four_physical_panels",
        "interpretation": "The reduced Eevee gate proves no visible benefit from either added luster strategy.",
    },
    "defect_ledger": [
        "A retains the rejected soft closed colour clouds and finite striped worked-response stamps.",
        "B removes both symbolic owners and protects quiet plate regions, but remains a flat repair baseline rather than a finished forged-iron material.",
        "B roughness is independent and subtle; the opposed-light panel still reads geometry bevels and lamp travel more strongly than a forged surface hierarchy.",
        "C adds a continuous anisotropy control but changes no physical proof pixel under this Eevee gate, so it has no demonstrated value here.",
        "D adds three visually dominant rail bands in its isolated response, recreating a decorative stripe language while also changing no physical proof pixel.",
        "All new rows leave the pivot too smooth and coated-looking; integration must prove a component-scale response without reintroducing motifs.",
    ],
    "manual_acceptance_established": False,
}


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=560)
    parser.add_argument("--resolution-y", type=int, default=180)
    parser.add_argument("--samples", type=int, default=16)
    parser.add_argument("--board-resolution-x", type=int, default=3920)
    parser.add_argument("--board-resolution-y", type=int, default=720)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def stable_seed(component_name: str, lane: str) -> int:
    digest = hashlib.sha256(f"{component_name}:{lane}".encode("utf-8")).digest()
    return int.from_bytes(digest[:8], "big") & 0x7FFFFFFF


def smooth_interpolation(value: np.ndarray) -> np.ndarray:
    return value * value * (3.0 - 2.0 * value)


def aperiodic_value_field(
    u_m: np.ndarray,
    v_m: np.ndarray,
    cell_size_m: float,
    seed: int,
) -> np.ndarray:
    """Finite seeded value field with no periodic texture-domain wrap."""
    if u_m.shape != v_m.shape or u_m.ndim != 2:
        raise ValueError("aperiodic coordinates must be matching HxW arrays")
    if not math.isfinite(cell_size_m) or cell_size_m <= 0.0:
        raise ValueError("cell size must be finite and positive")
    grid_u = u_m / cell_size_m
    grid_v = v_m / cell_size_m
    floor_u = np.floor(grid_u).astype(np.int64)
    floor_v = np.floor(grid_v).astype(np.int64)
    minimum_u = int(floor_u.min()) - 1
    maximum_u = int(floor_u.max()) + 2
    minimum_v = int(floor_v.min()) - 1
    maximum_v = int(floor_v.max()) + 2
    random = np.random.default_rng(seed)
    lattice = random.uniform(
        -1.0,
        1.0,
        (maximum_v - minimum_v + 1, maximum_u - minimum_u + 1),
    ).astype(np.float32)
    index_u = floor_u - minimum_u
    index_v = floor_v - minimum_v
    fraction_u = smooth_interpolation(grid_u - floor_u)
    fraction_v = smooth_interpolation(grid_v - floor_v)
    lower = (
        lattice[index_v, index_u] * (1.0 - fraction_u)
        + lattice[index_v, index_u + 1] * fraction_u
    )
    upper = (
        lattice[index_v + 1, index_u] * (1.0 - fraction_u)
        + lattice[index_v + 1, index_u + 1] * fraction_u
    )
    return (lower * (1.0 - fraction_v) + upper * fraction_v).astype(np.float32)


def normalized_component_coordinates(
    u_m: np.ndarray,
    v_m: np.ndarray,
    frame: dict[str, Any],
) -> tuple[np.ndarray, np.ndarray]:
    minimum_u_m, minimum_v_m = frame["minimum_m"]
    length_m, width_m = frame["span_m"]
    normalized_u = (u_m - minimum_u_m) / max(length_m, 1.0e-7)
    normalized_v = (v_m - minimum_v_m) / max(width_m, 1.0e-7)
    return normalized_u.astype(np.float32), normalized_v.astype(np.float32)


def open_luster_rails(
    normalized_u: np.ndarray,
    normalized_v: np.ndarray,
    component_name: str,
) -> tuple[np.ndarray, np.ndarray]:
    """Two uncapped rails evaluated beyond the visible component domain."""
    phase = stable_seed(component_name, "open_luster_rails") / float(0x7FFFFFFF)
    phase *= math.tau
    amount = np.zeros_like(normalized_u, dtype=np.float32)
    signed_direction = np.zeros_like(normalized_u, dtype=np.float32)
    rail_specs = (
        (0.30, 0.075, 0.58, phase + 0.31, 0.055, 0.012),
        (0.70, 0.065, 0.43, phase + 2.17, 0.048, 0.010),
    )
    # Curves are defined for every u, including the conceptual -0.25..1.25
    # extension. Clipping happens only at the host image boundary, so there are
    # no finite caps or endpoint silhouettes inside the component.
    for center_v, curve_amplitude, frequency, rail_phase, width, width_delta in rail_specs:
        argument = math.tau * frequency * (normalized_u + 0.25) + rail_phase
        center = center_v + curve_amplitude * np.sin(argument)
        local_width = width + width_delta * np.sin(argument * 0.47 + 1.13)
        distance = (normalized_v - center) / np.maximum(local_width, 1.0e-4)
        rail = np.exp(-0.5 * distance * distance).astype(np.float32)
        slope = curve_amplitude * math.tau * frequency * np.cos(argument)
        amount = np.maximum(amount, rail)
        signed_direction += rail * np.tanh(slope).astype(np.float32)
    rotation = np.clip(0.5 + signed_direction * 0.055, 0.42, 0.58).astype(np.float32)
    return amount, rotation


def luster_control_field(
    recipe: CandidateRecipe,
    normalized_u: np.ndarray,
    normalized_v: np.ndarray,
    component_name: str,
) -> np.ndarray:
    if recipe.luster_mode == "none":
        amount = np.zeros_like(normalized_u, dtype=np.float32)
        rotation = np.full_like(normalized_u, 0.5, dtype=np.float32)
    elif recipe.luster_mode == "continuous":
        amount = np.full_like(normalized_u, 1.0, dtype=np.float32)
        rotation = np.full_like(normalized_u, 0.5, dtype=np.float32)
    elif recipe.luster_mode == "open_rails":
        amount, rotation = open_luster_rails(
            normalized_u,
            normalized_v,
            component_name,
        )
    else:
        raise ValueError(f"Unsupported luster mode {recipe.luster_mode}")
    rest = 1.0 - np.clip(amount, 0.0, 1.0)
    return np.stack((amount, rotation, rest), axis=-1).astype(np.float32)


def build_candidate_fields(
    frame: dict[str, Any],
    component_name: str,
    recipe: CandidateRecipe,
) -> dict[str, np.ndarray]:
    minimum_u_m, minimum_v_m = frame["minimum_m"]
    maximum_u_m, maximum_v_m = frame["maximum_m"]
    length_m, width_m = frame["span_m"]
    resolution_x, resolution_y = base.field_resolution(length_m, width_m)
    u_axis = np.linspace(minimum_u_m, maximum_u_m, resolution_x, dtype=np.float32)
    v_axis = np.linspace(minimum_v_m, maximum_v_m, resolution_y, dtype=np.float32)
    u_m, v_m = np.meshgrid(u_axis, v_axis)

    donor = base.build_component_fields(frame)
    thickness = donor["oxide_thickness_m"]
    thickness_span = max(float(thickness.max() - thickness.min()), 1.0e-9)
    thermal = common.smoothstep(
        0.0,
        1.0,
        (thickness - float(thickness.min())) / thickness_span,
    ).astype(np.float32)

    medium_a = aperiodic_value_field(
        u_m,
        v_m,
        0.034,
        stable_seed(component_name, "medium_34mm"),
    )
    medium_b = aperiodic_value_field(
        u_m,
        v_m,
        0.013,
        stable_seed(component_name, "medium_13mm"),
    )
    connected_medium = (medium_a * 0.64 + medium_b * 0.36).astype(np.float32)
    aggregate_grain = aperiodic_value_field(
        u_m,
        v_m,
        0.008,
        stable_seed(component_name, "aggregate_8mm"),
    )

    thin_srgb = np.asarray((0.058, 0.064, 0.075), dtype=np.float32)
    thick_srgb = np.asarray((0.104, 0.096, 0.091), dtype=np.float32)
    oxide_srgb = common.mix(
        np.broadcast_to(thin_srgb, thermal.shape + (3,)),
        np.broadcast_to(thick_srgb, thermal.shape + (3,)),
        thermal,
    )
    oxide_base_linear = common.srgb_to_linear(oxide_srgb).astype(np.float32)
    oxide_roughness = np.clip(
        0.715
        + (thermal - 0.5) * 0.040
        + connected_medium * 0.032
        + aggregate_grain * 0.010,
        0.665,
        0.765,
    ).astype(np.float32)

    # Broad forging planes are the only normal owner in this repair gate.
    broad_forging_height_m = donor["forging_height_m"].copy()
    combined_normal = donor["forging_normal"].copy()
    oxide_surface_height_m = np.zeros_like(thermal, dtype=np.float32)
    oxide_coverage = np.ones_like(thermal, dtype=np.float32)
    oxide_metalness = np.zeros_like(thermal, dtype=np.float32)
    exposure = np.full_like(thermal, EXPOSED_CONDUCTOR, dtype=np.float32)

    normalized_u, normalized_v = normalized_component_coordinates(u_m, v_m, frame)
    luster_control = luster_control_field(
        recipe,
        normalized_u,
        normalized_v,
        component_name,
    )
    oxide_diagnostic = np.stack(
        (
            thermal,
            np.clip(0.5 + connected_medium * 0.5, 0.0, 1.0),
            oxide_coverage,
        ),
        axis=-1,
    ).astype(np.float32)

    if float(oxide_coverage.min()) != 1.0 or float(oxide_coverage.max()) != 1.0:
        raise RuntimeError("connected oxide lost complete coverage")
    if float(oxide_metalness.max()) != 0.0:
        raise RuntimeError("intact oxide became conductive")
    if float(np.abs(oxide_surface_height_m).max()) != 0.0:
        raise RuntimeError("oxide morphology leaked into surface height")
    return {
        "oxide_base_linear": oxide_base_linear,
        "oxide_roughness": oxide_roughness,
        "combined_normal": combined_normal,
        "luster_control": luster_control,
        "oxide_diagnostic": oxide_diagnostic,
        "thermal_response": thermal,
        "connected_medium": connected_medium,
        "aggregate_grain": aggregate_grain,
        "broad_forging_height_m": broad_forging_height_m,
        "oxide_surface_height_m": oxide_surface_height_m,
        "oxide_coverage": oxide_coverage,
        "oxide_metalness": oxide_metalness,
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
    output.location = (1120.0, 80.0)
    surface = tree.nodes.new("ShaderNodeMixShader")
    surface.name = "Complete_Oxide_And_Conductor_Responses"
    surface.location = (850.0, 80.0)
    exposure = tree.nodes.new("ShaderNodeValue")
    exposure.name = "Explicit_Exposed_Conductor_Only_Owner"
    exposure.outputs[0].default_value = EXPOSED_CONDUCTOR
    exposure.location = (610.0, 390.0)

    oxide_bsdf = tree.nodes.new("ShaderNodeBsdfPrincipled")
    oxide_bsdf.name = "Complete_Intact_Oxide_Response"
    oxide_bsdf.location = (550.0, 170.0)
    base.set_principled_input(oxide_bsdf, ("Metallic",), 0.0)
    base.set_principled_input(oxide_bsdf, ("IOR",), 2.10)
    base.set_principled_input(oxide_bsdf, ("Coat Weight", "Coat"), 0.0)
    anisotropy_name = base.set_principled_input(
        oxide_bsdf,
        ("Anisotropic IOR Level", "Anisotropic"),
        0.0,
    )
    rotation_name = base.set_principled_input(
        oxide_bsdf,
        ("Anisotropic Rotation",),
        0.5,
    )

    conductor_bsdf = tree.nodes.new("ShaderNodeBsdfPrincipled")
    conductor_bsdf.name = "Complete_Hidden_Iron_Response"
    conductor_bsdf.location = (550.0, -170.0)
    base.set_principled_input(
        conductor_bsdf,
        ("Base Color",),
        (*base.CONDUCTOR_F0_LINEAR, 1.0),
    )
    base.set_principled_input(conductor_bsdf, ("Metallic",), 1.0)
    base.set_principled_input(
        conductor_bsdf,
        ("Roughness",),
        base.CONDUCTOR_ROUGHNESS,
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
    tangent.location = (0.0, -470.0)
    tree.links.new(tangent.outputs["Tangent"], oxide_bsdf.inputs["Tangent"])
    tree.links.new(tangent.outputs["Tangent"], conductor_bsdf.inputs["Tangent"])

    uv = tree.nodes.new("ShaderNodeUVMap")
    uv.name = "Nonrepeating_Component_Field_UV"
    uv.uv_map = base.FIELD_UV
    uv.location = (-900.0, 140.0)
    color_image = base.add_image_node(
        tree,
        "Continuous_Oxide_Optical_Colour",
        images["oxide_base"],
        uv,
        (-650.0, 310.0),
    )
    roughness_image = base.add_image_node(
        tree,
        "Independent_Oxide_Roughness",
        images["oxide_roughness"],
        uv,
        (-650.0, 90.0),
    )
    normal_image = base.add_image_node(
        tree,
        "Broad_Forging_Normal_Only",
        images["combined_normal"],
        uv,
        (-650.0, -150.0),
    )
    luster_image = base.add_image_node(
        tree,
        "Worked_Luster_Response_Only",
        images["luster_control"],
        uv,
        (-650.0, -390.0),
    )

    normal_map = tree.nodes.new("ShaderNodeNormalMap")
    normal_map.name = "Broad_Forging_Normal_Decode"
    normal_map.space = "TANGENT"
    normal_map.uv_map = base.FIELD_UV
    normal_map.inputs["Strength"].default_value = 1.0
    normal_map.location = (-260.0, -140.0)

    separate = tree.nodes.new("ShaderNodeSeparateColor")
    separate.name = "Separate_Luster_Amount_Rotation_Rest"
    separate.mode = "RGB"
    separate.location = (-260.0, -375.0)
    anisotropy = tree.nodes.new("ShaderNodeMath")
    anisotropy.name = "Bounded_Oxide_Anisotropy"
    anisotropy.operation = "MULTIPLY"
    anisotropy.inputs[1].default_value = recipe.luster_anisotropy_maximum
    anisotropy.location = (40.0, -335.0)

    tree.links.new(color_image.outputs["Color"], oxide_bsdf.inputs["Base Color"])
    tree.links.new(roughness_image.outputs["Color"], oxide_bsdf.inputs["Roughness"])
    tree.links.new(normal_image.outputs["Color"], normal_map.inputs["Color"])
    tree.links.new(normal_map.outputs["Normal"], oxide_bsdf.inputs["Normal"])
    tree.links.new(normal_map.outputs["Normal"], conductor_bsdf.inputs["Normal"])
    tree.links.new(luster_image.outputs["Color"], separate.inputs["Color"])
    tree.links.new(separate.outputs[0], anisotropy.inputs[0])
    tree.links.new(anisotropy.outputs[0], oxide_bsdf.inputs[anisotropy_name])
    tree.links.new(separate.outputs[1], oxide_bsdf.inputs[rotation_name])
    tree.links.new(exposure.outputs[0], surface.inputs["Fac"])
    tree.links.new(oxide_bsdf.outputs["BSDF"], surface.inputs[1])
    tree.links.new(conductor_bsdf.outputs["BSDF"], surface.inputs[2])
    tree.links.new(surface.outputs["Shader"], output.inputs["Surface"])

    counts = {
        "principled": sum(node.bl_idname == "ShaderNodeBsdfPrincipled" for node in tree.nodes),
        "mix_shader": sum(node.bl_idname == "ShaderNodeMixShader" for node in tree.nodes),
        "normal_map": sum(node.bl_idname == "ShaderNodeNormalMap" for node in tree.nodes),
        "tangent": sum(node.bl_idname == "ShaderNodeTangent" for node in tree.nodes),
        "image_texture": sum(node.bl_idname == "ShaderNodeTexImage" for node in tree.nodes),
    }
    expected = {
        "principled": 2,
        "mix_shader": 1,
        "normal_map": 1,
        "tangent": 1,
        "image_texture": 4,
    }
    if counts != expected:
        raise RuntimeError(f"Unexpected repair material topology: {counts}")
    return material, {
        **counts,
        "oxide_anisotropy_socket": anisotropy_name,
        "oxide_rotation_socket": rotation_name,
        "luster_anisotropy_maximum": recipe.luster_anisotropy_maximum,
        "exposure": float(exposure.outputs[0].default_value),
    }


def render_path(scene: bpy.types.Scene, path: Path) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def render_opposed_difference(
    scene: bpy.types.Scene,
    lights: dict[str, bpy.types.Object],
    close_center: Vector,
    output_path: Path,
) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for the opposed-light difference")
    left_path = output_path.with_name("_close_light_left.png")
    right_path = output_path.with_name("_close_light_right.png")
    presentation.front_camera(scene.camera, close_center, 2.05)
    presentation.set_moving_strip(lights, close_center, side="left")
    render_path(scene, left_path)
    presentation.set_moving_strip(lights, close_center, side="right")
    render_path(scene, right_path)
    command = [
        ffmpeg,
        "-y",
        "-hide_banner",
        "-loglevel",
        "error",
        "-i",
        str(left_path),
        "-i",
        str(right_path),
        "-filter_complex",
        "blend=all_mode=difference",
        "-frames:v",
        "1",
        str(output_path),
    ]
    completed = subprocess.run(command, capture_output=True, text=True, check=False)
    if completed.returncode != 0:
        raise RuntimeError("opposed-light difference failed: " + completed.stderr.strip())
    left_path.unlink(missing_ok=True)
    right_path.unlink(missing_ok=True)
    return {
        "path": str(output_path),
        "bytes": output_path.stat().st_size,
        "sha256": sha256_file(output_path),
        "operation": "absolute sRGB difference of identical left/right close renders",
    }


def render_physical_panels(
    scene: bpy.types.Scene,
    lights: dict[str, bpy.types.Object],
    center: Vector,
    close_center: Vector,
    span: Vector,
    output_root: Path,
) -> dict[str, dict[str, Any]]:
    renders: dict[str, dict[str, Any]] = {}
    presentation.set_neutral_lights(lights, center)
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    renders["full_front"] = render_path(scene, output_root / "full_front.png")
    presentation.set_neutral_lights(lights, close_center, close=True)
    presentation.front_camera(scene.camera, close_center, 2.05)
    renders["close_neutral"] = render_path(scene, output_root / "close_neutral.png")
    renders["opposed_light_difference"] = render_opposed_difference(
        scene,
        lights,
        close_center,
        output_root / "opposed_light_difference.png",
    )
    presentation.set_neutral_lights(lights, center)
    presentation.front_camera(scene.camera, center, span.x * 1.62)
    renders["gameplay"] = render_path(scene, output_root / "gameplay.png")
    return renders


def render_candidate_diagnostics(
    scene: bpy.types.Scene,
    lights: dict[str, bpy.types.Object],
    center: Vector,
    span: Vector,
    targets: list[bpy.types.Object],
    lane_materials: dict[str, dict[str, bpy.types.Material]],
    output_root: Path,
) -> dict[str, dict[str, Any]]:
    renders: dict[str, dict[str, Any]] = {}
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    presentation.set_neutral_lights(lights, center)
    lane_to_panel = (
        ("connected_oxide", "connected_oxide"),
        ("roughness", "independent_roughness"),
        ("luster", "luster_diagnostic"),
    )
    for lane, panel_id in lane_to_panel:
        for obj in targets:
            base.set_material(obj, lane_materials[obj.name][lane])
        renders[panel_id] = render_path(scene, output_root / f"{panel_id}.png")
    return renders


def render_root_diagnostics(
    scene: bpy.types.Scene,
    lights: dict[str, bpy.types.Object],
    center: Vector,
    span: Vector,
    proof_mode: bpy.types.Node,
    output_root: Path,
) -> dict[str, dict[str, Any]]:
    renders: dict[str, dict[str, Any]] = {}
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    presentation.set_neutral_lights(lights, center)
    proxy_modes = (
        ("connected_oxide", "base_colour"),
        ("independent_roughness", "roughness"),
        ("luster_diagnostic", "worked_response"),
    )
    for panel_id, proof_name in proxy_modes:
        proof_mode.outputs[0].default_value = presentation.PROOF_MODES[proof_name]
        renders[panel_id] = render_path(scene, output_root / f"{panel_id}.png")
    proof_mode.outputs[0].default_value = presentation.PROOF_MODES["combined"]
    return renders


def cleanup_candidate_resources(
    targets: list[bpy.types.Object],
    root_material: bpy.types.Material,
    materials: list[bpy.types.Material],
    images: list[bpy.types.Image],
) -> None:
    material_names = [material.name for material in materials]
    image_names = [image.name for image in images]
    for obj in targets:
        base.set_material(obj, root_material)
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


def build_comparison_board(
    scene: bpy.types.Scene,
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    camera_data = bpy.data.cameras.new("IGGY_CAM_ConnectedOxideLusterBoard")
    camera = bpy.data.objects.new("IGGY_CAM_ConnectedOxideLusterBoard", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 28.0
    camera.location = (0.0, 0.0, 10.0)
    presentation.point_object(camera, Vector((0.0, 0.0, 0.0)))
    presentation.configure_render(
        scene,
        args.board_resolution_x,
        args.board_resolution_y,
    )
    presentation.configure_world(scene, strength=0.0)
    scene.view_settings.view_transform = "Standard"
    try:
        scene.view_settings.look = "None"
    except TypeError:
        pass

    label_background = presentation.make_emission_material(
        "IGGY_MAT_RepairBoardLabel",
        colour=(0.004, 0.005, 0.007, 1.0),
    )
    label_text = presentation.make_emission_material(
        "IGGY_MAT_RepairBoardText",
        colour=(0.86, 0.90, 0.96, 1.0),
    )
    panel_width = 4.0
    panel_height = 28.0 / (args.board_resolution_x / args.board_resolution_y) / 4.0
    for row, recipe in enumerate(CANDIDATES):
        for column, panel_id in enumerate(PANEL_ORDER):
            panel_path = output_root / recipe.candidate_id / f"{panel_id}.png"
            if not panel_path.is_file():
                raise FileNotFoundError(panel_path)
            center_x = (column - 3.0) * panel_width
            center_y = (1.5 - row) * panel_height
            image = bpy.data.images.load(str(panel_path), check_existing=False)
            image_material = presentation.make_emission_material(
                f"IGGY_MAT_Board_{recipe.candidate_id}_{panel_id}",
                image=image,
            )
            presentation.add_panel_plane(
                f"Board_{recipe.candidate_id}_{panel_id}",
                center_x,
                center_y,
                panel_width - 0.035,
                panel_height - 0.035,
                0.0,
                image_material,
            )
            presentation.add_panel_plane(
                f"BoardLabel_{recipe.candidate_id}_{panel_id}",
                center_x,
                center_y + panel_height * 0.40,
                panel_width - 0.10,
                panel_height * 0.16,
                0.10,
                label_background,
            )
            font_curve = bpy.data.curves.new(
                f"BoardText_{recipe.candidate_id}_{panel_id}",
                type="FONT",
            )
            font_curve.body = f"{recipe.candidate_id[0]}  {PANEL_LABELS[panel_id]}"
            font_curve.align_x = "LEFT"
            font_curve.align_y = "CENTER"
            font_curve.size = 0.105
            font_curve.space_character = 1.02
            font_obj = bpy.data.objects.new(font_curve.name, font_curve)
            scene.collection.objects.link(font_obj)
            font_obj.location = (
                center_x - panel_width * 0.46,
                center_y + panel_height * 0.40,
                0.20,
            )
            font_obj.data.materials.append(label_text)

    board_path = output_root / "connected_oxide_luster_comparison_board.png"
    scene.render.filepath = str(board_path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(board_path),
        "resolution": [args.board_resolution_x, args.board_resolution_y],
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "row_order": [recipe.candidate_id for recipe in CANDIDATES],
        "column_order": list(PANEL_ORDER),
    }


def aggregate_scalar(
    all_fields: dict[str, dict[str, np.ndarray]],
    field_name: str,
) -> np.ndarray:
    return np.concatenate(
        [fields[field_name].reshape(-1) for fields in all_fields.values()]
    )


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if not base.SOURCE_BLEND.is_file() or not base.GEOMETRY_BLEND.is_file():
        raise FileNotFoundError("forged-iron root sources are missing")
    source_hash_before = sha256_file(base.SOURCE_BLEND)
    geometry_hash_before = sha256_file(base.GEOMETRY_BLEND)
    script_hash = sha256_file(Path(__file__).resolve())

    bpy.ops.wm.open_mainfile(filepath=str(base.SOURCE_BLEND))
    scene, root_material, proof_mode, targets, lights = presentation.validate_root()
    presentation.isolate_actual_hinge(targets, lights)
    presentation.configure_render(scene, args.resolution_x, args.resolution_y)
    presentation.configure_world(scene)
    if hasattr(scene, "eevee") and hasattr(scene.eevee, "taa_render_samples"):
        scene.eevee.taa_render_samples = args.samples

    minimum, maximum = presentation.object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    close_center = Vector((0.72, center.y, center.z))
    candidate_manifest: dict[str, Any] = {}

    # A is the untouched researched root and proves the failure being repaired.
    control = CANDIDATES[0]
    control_root = output_root / control.candidate_id
    control_root.mkdir(parents=True, exist_ok=True)
    for obj in targets:
        base.set_material(obj, root_material)
    proof_mode.outputs[0].default_value = presentation.PROOF_MODES["combined"]
    control_renders = render_physical_panels(
        scene,
        lights,
        center,
        close_center,
        span,
        control_root,
    )
    control_renders.update(
        render_root_diagnostics(
            scene,
            lights,
            center,
            span,
            proof_mode,
            control_root,
        )
    )
    candidate_manifest[control.candidate_id] = {
        "label": control.label,
        "recipe": asdict(control),
        "renders": {panel: control_renders[panel] for panel in PANEL_ORDER},
        "diagnostic_proxies": {
            "connected_oxide": "root base-colour proof",
            "independent_roughness": "root roughness proof",
            "luster_diagnostic": "root worked-response proof",
        },
        "canonical_material_unchanged": True,
    }

    frames = {obj.name: base.install_component_frames(obj) for obj in targets}
    for recipe in CANDIDATES[1:]:
        candidate_root = output_root / recipe.candidate_id
        candidate_root.mkdir(parents=True, exist_ok=True)
        physical_materials: dict[str, bpy.types.Material] = {}
        lane_materials: dict[str, dict[str, bpy.types.Material]] = {}
        tracked_materials: list[bpy.types.Material] = []
        tracked_images: list[bpy.types.Image] = []
        all_fields: dict[str, dict[str, np.ndarray]] = {}
        topology: dict[str, Any] | None = None
        for obj in targets:
            fields = build_candidate_fields(frames[obj.name], obj.name, recipe)
            slug = obj.name.replace(".", "_")
            prefix = f"IGGY_IMG_{recipe.candidate_id}_{slug}"
            images = {
                "oxide_base": base.make_float_image(
                    f"{prefix}_OxideBase",
                    fields["oxide_base_linear"],
                    color=True,
                ),
                "oxide_roughness": base.make_float_image(
                    f"{prefix}_OxideRoughness",
                    fields["oxide_roughness"],
                    color=False,
                ),
                "combined_normal": base.make_float_image(
                    f"{prefix}_BroadForgingNormal",
                    fields["combined_normal"] * 0.5 + 0.5,
                    color=True,
                ),
                "luster_control": base.make_float_image(
                    f"{prefix}_LusterControl",
                    fields["luster_control"],
                    color=True,
                ),
                "oxide_diagnostic": base.make_float_image(
                    f"{prefix}_OxideDiagnostic",
                    fields["oxide_diagnostic"],
                    color=True,
                ),
            }
            tracked_images.extend(images.values())
            physical, object_topology = build_candidate_material(
                obj.name,
                recipe,
                images,
            )
            if topology is None:
                topology = object_topology
            elif topology != object_topology:
                raise RuntimeError("candidate topology drifted between components")
            lanes = {
                "connected_oxide": base.build_lane_material(
                    f"IGGY_MAT_{recipe.candidate_id}_{slug}_ConnectedOxide",
                    images["oxide_diagnostic"],
                ),
                "roughness": base.build_lane_material(
                    f"IGGY_MAT_{recipe.candidate_id}_{slug}_Roughness",
                    images["oxide_roughness"],
                ),
                "luster": base.build_lane_material(
                    f"IGGY_MAT_{recipe.candidate_id}_{slug}_Luster",
                    images["luster_control"],
                ),
            }
            tracked_materials.extend((physical, *lanes.values()))
            physical_materials[obj.name] = physical
            lane_materials[obj.name] = lanes
            all_fields[obj.name] = fields

        for obj in targets:
            base.set_material(obj, physical_materials[obj.name])
        renders = render_physical_panels(
            scene,
            lights,
            center,
            close_center,
            span,
            candidate_root,
        )
        renders.update(
            render_candidate_diagnostics(
                scene,
                lights,
                center,
                span,
                targets,
                lane_materials,
                candidate_root,
            )
        )

        coverage = aggregate_scalar(all_fields, "oxide_coverage")
        metalness = aggregate_scalar(all_fields, "oxide_metalness")
        exposure = aggregate_scalar(all_fields, "exposure")
        oxide_height = aggregate_scalar(all_fields, "oxide_surface_height_m")
        roughness = aggregate_scalar(all_fields, "oxide_roughness")
        luster = np.concatenate(
            [fields["luster_control"][..., 0].reshape(-1) for fields in all_fields.values()]
        )
        candidate_manifest[recipe.candidate_id] = {
            "label": recipe.label,
            "recipe": asdict(recipe),
            "renders": {panel: renders[panel] for panel in PANEL_ORDER},
            "coverage_range": [float(coverage.min()), float(coverage.max())],
            "metalness_maximum": float(metalness.max()),
            "exposure_maximum": float(exposure.max()),
            "oxide_surface_height_absolute_maximum_m": float(np.abs(oxide_height).max()),
            "roughness_range": [float(roughness.min()), float(roughness.max())],
            "luster_amount_range": [float(luster.min()), float(luster.max())],
            "topology": topology,
            "resources_released_after_render": True,
        }
        cleanup_candidate_resources(
            targets,
            root_material,
            tracked_materials,
            tracked_images,
        )

    proof_mode.outputs[0].default_value = presentation.PROOF_MODES["combined"]
    source_hash_after = sha256_file(base.SOURCE_BLEND)
    geometry_hash_after = sha256_file(base.GEOMETRY_BLEND)
    canonical_source_unchanged = (
        source_hash_before == source_hash_after
        and geometry_hash_before == geometry_hash_after
    )
    if not canonical_source_unchanged:
        raise RuntimeError("exploratory repair changed a canonical source")

    comparison_board = build_comparison_board(scene, output_root, args)
    manifest = {
        "schema": "iggy3d.connected_oxide_luster_sweep.v1",
        "status": "EXPLORATORY_COMPARISON_NOT_CANONICAL",
        "consumer": {
            "collection": base.TARGET_COLLECTION,
            "objects": list(base.TARGET_OBJECTS),
            "assembly_span_m": [float(span.x), float(span.y), float(span.z)],
        },
        "tile_resolution": [args.resolution_x, args.resolution_y],
        "render_engine": scene.render.engine,
        "script": {
            "path": str(Path(__file__).resolve()),
            "sha256": script_hash,
        },
        "candidates": candidate_manifest,
        "comparison_board": comparison_board,
        "visual_review": VISUAL_REVIEW,
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
        "uses_condition": False,
        "unreal_parity_verified": False,
        "manual_acceptance_required": True,
        "limitations": [
            "This reduced Eevee matrix selects one response strategy only.",
            "The A-row connected-oxide and luster columns are frozen-root proof proxies.",
            "No candidate is integrated into or saved over the researched root.",
            "A Blender selection does not establish Unreal parity or user acceptance.",
        ],
    }
    manifest_path = output_root / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(
        "CONNECTED_OXIDE_LUSTER_SWEEP="
        f"rows:{len(CANDIDATES)},columns:{len(PANEL_ORDER)},"
        f"source_unchanged:{canonical_source_unchanged}"
    )
    print(f"CONNECTED_OXIDE_LUSTER_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
