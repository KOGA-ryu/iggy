#!/usr/bin/env python3
"""Build the minimal clean ground-steel shader on a WPN-001 blade fixture."""

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


ROOT = Path(__file__).resolve().parents[4]
PACKAGE = Path(__file__).resolve().parent
PROFILE_PATH = PACKAGE / "profiles" / "sword_steel_v1.json"
COMMON_PATH = PACKAGE.parent / "pattern_lab_common.py"
FORGED_PACKAGE = PACKAGE.parent / "forged_iron_v1"
CONDUCTOR_MANIFEST = (
    FORGED_PACKAGE / "output" / "conductor_preview_v1" / "manifest.json"
)
TANGENT_MANIFEST = (
    FORGED_PACKAGE
    / "output"
    / "tangent_anisotropy_preview_v1"
    / "manifest.json"
)
DEFAULT_OUTPUT = PACKAGE / "output"
DEFAULT_BLEND = DEFAULT_OUTPUT / "sword_steel_v1.blend"
MANIFEST_PATH = DEFAULT_OUTPUT / "sword_steel_v1_manifest.json"
MATERIAL_NAME = "IGGY_MAT_SwordSteel_CleanGround_v001"
FIXTURE_NAME = "SM_WPN001_BladeSteelFixture"
REGION_ATTRIBUTE = "sinc_sword_region"
BLADE_UV = "IGGY_BladeUV"


def _load_common():
    spec = importlib.util.spec_from_file_location(
        "iggy_pattern_lab_common_sword_steel",
        COMMON_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load shared material tools: {COMMON_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


common = _load_common()


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=960)
    parser.add_argument("--resolution-y", type=int, default=320)
    parser.add_argument("--samples", type=int, default=24)
    parser.add_argument("--validate-only", action="store_true")
    parser.add_argument("--blend-path", type=Path, default=DEFAULT_BLEND)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load_contracts() -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    for path in (PROFILE_PATH, CONDUCTOR_MANIFEST, TANGENT_MANIFEST):
        if not path.is_file():
            raise FileNotFoundError(f"Sword-steel contract is missing: {path}")
    profile = json.loads(PROFILE_PATH.read_text())
    conductor = json.loads(CONDUCTOR_MANIFEST.read_text())
    tangent = json.loads(TANGENT_MANIFEST.read_text())
    f0 = conductor["comparison_contract"]["shared_linear_rgb_value"]
    body_roughness = conductor["comparison_contract"]["shared_roughness"]
    body_anisotropy = tangent["selection_contract"]["selected_default"]
    if f0 != profile["donors"]["expected_f0_linear"]:
        raise RuntimeError("clean conductor F0 drifted from the sword profile")
    if body_roughness != profile["donors"]["body_roughness"]:
        raise RuntimeError("clean conductor roughness drifted from the sword profile")
    if body_anisotropy != profile["donors"]["body_anisotropy"]:
        raise RuntimeError("accepted tangent response drifted from the sword profile")
    return profile, conductor, tangent


def generate_macro_polish_field(profile: dict[str, Any]) -> np.ndarray:
    contract = profile["finish"]["macro_polish"]
    resolution_x, resolution_y = contract["map_resolution"]
    u_knots = np.asarray(contract["u_knots"], dtype=np.float32)
    v_knots = np.asarray(contract["v_knots"], dtype=np.float32)
    controls = np.asarray(contract["control_values"], dtype=np.float32)
    if controls.shape != tuple(contract["control_grid_shape"]):
        raise ValueError("macro polish control lattice has the wrong shape")
    if controls.shape != (len(v_knots), len(u_knots)):
        raise ValueError("macro polish knots do not match the control lattice")
    if np.any(np.diff(u_knots) <= 0.0) or np.any(np.diff(v_knots) <= 0.0):
        raise ValueError("macro polish knots must increase strictly")
    u = np.linspace(0.0, 1.0, resolution_x, dtype=np.float32)
    v = np.linspace(0.0, 1.0, resolution_y, dtype=np.float32)
    rows = np.stack([np.interp(u, u_knots, row) for row in controls])
    field = np.empty((resolution_y, resolution_x), dtype=np.float32)
    for column in range(resolution_x):
        field[:, column] = np.interp(v, v_knots, rows[:, column])
    field = np.clip(field, 0.0, 1.0).astype(np.float32)
    if not np.isfinite(field).all():
        raise RuntimeError("macro polish compiler produced non-finite values")
    return field


def generate_grind_field(profile: dict[str, Any]) -> np.ndarray:
    contract = profile["finish"]["medium_grind"]
    resolution_x, resolution_y = contract["map_resolution"]
    track_count = contract["track_count"]
    if (resolution_x, resolution_y, track_count) != (1024, 256, 47):
        raise ValueError("sword grind field contract changed unexpectedly")
    u = np.linspace(0.0, 1.0, resolution_x, dtype=np.float32)
    v = np.linspace(-1.0, 1.0, resolution_y, dtype=np.float32)
    u_field, v_field = np.meshgrid(u, v)
    signed = np.zeros_like(u_field)
    for track_index in range(track_count):
        nominal = -0.97 + (track_index + 0.5) * 1.94 / track_count
        center = nominal + 0.014 * math.sin(track_index * 2.399963)
        width = 0.0016 + 0.0038 * (
            0.5 + 0.5 * math.sin(track_index * 1.618034)
        )
        pressure = math.sin(track_index * 2.173 + 0.4)
        if track_index % 7 == 0 or track_index % 11 == 0:
            pressure *= 0.12
        wander = 0.0026 * np.sin(
            math.tau * u_field * (1.0 + track_index % 3)
            + track_index * 0.73
        )
        envelope = np.interp(
            u,
            [0.0, 0.22, 0.51, 0.78, 1.0],
            [
                0.72,
                1.0,
                0.82 + 0.15 * math.sin(track_index),
                0.94,
                0.66,
            ],
        )[np.newaxis, :]
        signed += pressure * envelope * np.exp(
            -0.5 * ((v_field - center - wander) / width) ** 2
        )
    signed /= max(float(np.max(np.abs(signed))), 1.0e-6)
    grind = np.clip(0.5 + signed * 0.5, 0.0, 1.0).astype(np.float32)
    if grind.shape != (resolution_y, resolution_x):
        raise RuntimeError("grind compiler produced the wrong shape")
    if not np.isfinite(grind).all() or grind.min() < 0.0 or grind.max() > 1.0:
        raise RuntimeError("grind compiler produced illegal values")
    return grind


def _smoothstep(value: float) -> float:
    value = min(max(value, 0.0), 1.0)
    return value * value * (3.0 - 2.0 * value)


def _station_shape(
    x_m: float,
    profile: dict[str, Any],
) -> tuple[float, float, float, list[float]]:
    consumer = profile["consumer"]
    geometry = profile["geometry"]
    length_m = consumer["blade_length_m"]
    half_width_m = consumer["base_width_m"] * 0.5
    half_thickness_m = consumer["base_thickness_m"] * 0.5
    t = x_m / length_m
    width_m = half_width_m * (
        1.0 - (1.0 - geometry["profile_taper"]) * t
    )
    height_m = half_thickness_m * (
        1.0 - (1.0 - geometry["distal_taper"]) * t
    )
    tip_start_m = length_m - geometry["tip_length_m"]
    tip = _smoothstep(
        (x_m - tip_start_m) / max(geometry["tip_length_m"], 1.0e-6)
    )
    residual_m = geometry["edge_land_m"] * 0.5
    width_m = width_m * (1.0 - tip) + residual_m * tip
    height_m = height_m * (1.0 - tip) + residual_m * tip
    fuller_half_m = min(
        geometry["fuller_width_m"] * 0.5,
        width_m * geometry["bevel_shoulder_fraction"] * 0.82,
    )
    fuller_fraction = fuller_half_m / max(width_m, 1.0e-8)
    rings = [
        -1.0,
        -geometry["edge_band_fraction"],
        -geometry["bevel_shoulder_fraction"],
        -fuller_fraction,
        0.0,
        fuller_fraction,
        geometry["bevel_shoulder_fraction"],
        geometry["edge_band_fraction"],
        1.0,
    ]
    return width_m, height_m, fuller_fraction, rings


def _surface_height_m(
    normalized_y: float,
    half_thickness_m: float,
    fuller_fraction: float,
    profile: dict[str, Any],
) -> float:
    geometry = profile["geometry"]
    absolute = abs(normalized_y)
    bevel = geometry["bevel_shoulder_fraction"]
    residual_m = geometry["edge_land_m"] * 0.5
    if absolute <= bevel:
        height_m = half_thickness_m
    else:
        blend = (absolute - bevel) / max(1.0 - bevel, 1.0e-6)
        height_m = half_thickness_m * (1.0 - blend) + residual_m * blend
    if fuller_fraction > 1.0e-6 and absolute <= fuller_fraction:
        fuller_t = absolute / fuller_fraction
        fuller_bump = (1.0 - fuller_t * fuller_t) ** 2
        height_m -= (
            geometry["fuller_depth_fraction"]
            * half_thickness_m
            * fuller_bump
        )
    return max(height_m, residual_m)


REGION_RGBA = {
    "body": (1.0, 0.0, 0.0, 0.0),
    "fuller": (0.0, 1.0, 0.0, 0.0),
    "bevel": (0.0, 0.0, 1.0, 0.0),
    "edge": (0.0, 0.0, 0.0, 1.0),
}


def _segment_region(
    normalized_mid: float,
    fuller_fraction: float,
    profile: dict[str, Any],
) -> str:
    geometry = profile["geometry"]
    absolute = abs(normalized_mid)
    if absolute >= geometry["edge_band_fraction"]:
        return "edge"
    if absolute >= geometry["bevel_shoulder_fraction"]:
        return "bevel"
    if absolute <= fuller_fraction:
        return "fuller"
    return "body"


def create_analytic_blade_fixture(
    profile: dict[str, Any],
) -> tuple[bpy.types.Object, dict[str, Any]]:
    consumer = profile["consumer"]
    geometry = profile["geometry"]
    length_m = consumer["blade_length_m"]
    station_count = geometry["length_segments"] + 1
    x_axis = np.linspace(0.0, length_m, station_count, dtype=np.float64)

    vertices: list[tuple[float, float, float]] = []
    vertex_uv: list[tuple[float, float]] = []
    station_rings: list[list[float]] = []
    station_fuller: list[float] = []
    for x_m in x_axis:
        width_m, half_thickness_m, fuller_fraction, rings = _station_shape(
            float(x_m), profile
        )
        station_rings.append(rings)
        station_fuller.append(fuller_fraction)
        for normalized_y in rings:
            y_m = normalized_y * width_m
            z_m = _surface_height_m(
                normalized_y,
                half_thickness_m,
                fuller_fraction,
                profile,
            )
            for side in (1.0, -1.0):
                vertices.append((float(x_m), y_m, side * z_m))
                vertex_uv.append(
                    (float(x_m) / length_m, (normalized_y + 1.0) * 0.5)
                )

    ring_count = len(station_rings[0])

    def index(station: int, ring: int, side: int) -> int:
        return (station * ring_count + ring) * 2 + side

    faces: list[tuple[int, ...]] = []
    regions: list[str] = []
    for station in range(station_count - 1):
        for ring in range(ring_count - 1):
            normalized_mid = 0.25 * (
                station_rings[station][ring]
                + station_rings[station][ring + 1]
                + station_rings[station + 1][ring]
                + station_rings[station + 1][ring + 1]
            )
            fuller_fraction = 0.5 * (
                station_fuller[station] + station_fuller[station + 1]
            )
            region = _segment_region(normalized_mid, fuller_fraction, profile)
            faces.append(
                (
                    index(station, ring, 0),
                    index(station + 1, ring, 0),
                    index(station + 1, ring + 1, 0),
                    index(station, ring + 1, 0),
                )
            )
            regions.append(region)
            faces.append(
                (
                    index(station, ring + 1, 1),
                    index(station + 1, ring + 1, 1),
                    index(station + 1, ring, 1),
                    index(station, ring, 1),
                )
            )
            regions.append(region)

        for ring in (0, ring_count - 1):
            next_station = station + 1
            faces.append(
                (
                    index(station, ring, 0),
                    index(station, ring, 1),
                    index(next_station, ring, 1),
                    index(next_station, ring, 0),
                )
            )
            regions.append("edge")

    for station, reverse in ((0, True), (station_count - 1, False)):
        for ring in range(ring_count - 1):
            face = (
                index(station, ring, 0),
                index(station, ring + 1, 0),
                index(station, ring + 1, 1),
                index(station, ring, 1),
            )
            faces.append(tuple(reversed(face)) if reverse else face)
            normalized_mid = 0.5 * (
                station_rings[station][ring]
                + station_rings[station][ring + 1]
            )
            regions.append(
                _segment_region(
                    normalized_mid,
                    station_fuller[station],
                    profile,
                )
            )

    mesh = bpy.data.meshes.new("SM_WPN001_BladeSteelFixture_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.validate(verbose=False)
    mesh.update()
    obj = bpy.data.objects.new(FIXTURE_NAME, mesh)
    bpy.context.scene.collection.objects.link(obj)

    region_attribute = mesh.color_attributes.new(
        name=REGION_ATTRIBUTE,
        type="FLOAT_COLOR",
        domain="CORNER",
    )
    region_counts = {name: 0 for name in REGION_RGBA}
    for polygon, region in zip(mesh.polygons, regions, strict=True):
        polygon.use_smooth = False
        region_counts[region] += 1
        colour = REGION_RGBA[region]
        for loop_index in polygon.loop_indices:
            region_attribute.data[loop_index].color = colour

    uv_layer = mesh.uv_layers.new(name=BLADE_UV)
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = vertex_uv[loop.vertex_index]
    obj["sinc_asset_id"] = "WPN-001-diagnostic-blade"
    obj["sinc_surface_contract"] = "clean-ground-steel-v1"
    obj["sinc_actual_production_mesh"] = False
    return obj, {
        "vertex_count": len(mesh.vertices),
        "polygon_count": len(mesh.polygons),
        "region_polygon_counts": region_counts,
        "length_m": length_m,
        "base_width_m": consumer["base_width_m"],
        "base_thickness_m": consumer["base_thickness_m"],
        "edge_land_m": geometry["edge_land_m"],
        "straight_centerline_deviation_m": 0.0,
        "region_attribute": REGION_ATTRIBUTE,
        "blade_uv": BLADE_UV,
    }


def set_principled_input(
    node: bpy.types.Node,
    names: tuple[str, ...],
    value: Any,
) -> str:
    for name in names:
        socket = node.inputs.get(name)
        if socket is not None:
            socket.default_value = value
            return name
    raise KeyError(f"Principled node has none of {names}")


def _weighted_sum(
    tree: bpy.types.NodeTree,
    channels: dict[str, bpy.types.NodeSocket],
    values: dict[str, float],
    name: str,
    x: float,
    y: float,
) -> bpy.types.NodeSocket:
    weighted: list[bpy.types.NodeSocket] = []
    for offset, region in enumerate(("body", "fuller", "bevel", "edge")):
        multiply = tree.nodes.new("ShaderNodeMath")
        multiply.name = f"{name}_{region.title()}_Weight"
        multiply.operation = "MULTIPLY"
        multiply.location = (x, y - offset * 90.0)
        multiply.inputs[1].default_value = values[region]
        tree.links.new(channels[region], multiply.inputs[0])
        weighted.append(multiply.outputs[0])
    first_add = tree.nodes.new("ShaderNodeMath")
    first_add.name = f"{name}_BodyPlusFuller"
    first_add.operation = "ADD"
    first_add.location = (x + 190.0, y - 45.0)
    tree.links.new(weighted[0], first_add.inputs[0])
    tree.links.new(weighted[1], first_add.inputs[1])
    second_add = tree.nodes.new("ShaderNodeMath")
    second_add.name = f"{name}_BevelPlusEdge"
    second_add.operation = "ADD"
    second_add.location = (x + 190.0, y - 225.0)
    tree.links.new(weighted[2], second_add.inputs[0])
    tree.links.new(weighted[3], second_add.inputs[1])
    total = tree.nodes.new("ShaderNodeMath")
    total.name = f"{name}_RegionalSum"
    total.operation = "ADD"
    total.location = (x + 390.0, y - 130.0)
    tree.links.new(first_add.outputs[0], total.inputs[0])
    tree.links.new(second_add.outputs[0], total.inputs[1])
    return total.outputs[0]


def _center_scale_mask_field(
    tree: bpy.types.NodeTree,
    field_socket: bpy.types.NodeSocket,
    mask_socket: bpy.types.NodeSocket,
    amplitude: float,
    name: str,
    x: float,
    y: float,
) -> bpy.types.NodeSocket:
    centered = tree.nodes.new("ShaderNodeMath")
    centered.name = f"Center_{name}"
    centered.operation = "SUBTRACT"
    centered.inputs[1].default_value = 0.5
    centered.location = (x, y)
    tree.links.new(field_socket, centered.inputs[0])
    scaled = tree.nodes.new("ShaderNodeMath")
    scaled.name = f"{name}_Amplitude"
    scaled.operation = "MULTIPLY"
    scaled.inputs[1].default_value = amplitude
    scaled.location = (x + 180.0, y)
    tree.links.new(centered.outputs[0], scaled.inputs[0])
    masked = tree.nodes.new("ShaderNodeMath")
    masked.name = f"{name}_Regional_Gain"
    masked.operation = "MULTIPLY"
    masked.location = (x + 360.0, y)
    tree.links.new(scaled.outputs[0], masked.inputs[0])
    tree.links.new(mask_socket, masked.inputs[1])
    return masked.outputs[0]


def build_sword_steel_material(
    profile: dict[str, Any],
    conductor: dict[str, Any],
    tangent_contract: dict[str, Any],
    macro_image: bpy.types.Image,
    grind_image: bpy.types.Image,
) -> tuple[bpy.types.Material, dict[str, Any]]:
    material = bpy.data.materials.new(MATERIAL_NAME)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()

    output = tree.nodes.new("ShaderNodeOutputMaterial")
    output.name = "SwordSteel_MaterialOutput"
    output.location = (1120.0, 40.0)
    principled = tree.nodes.new("ShaderNodeBsdfPrincipled")
    principled.name = "Clean_Steel_Conductor"
    principled.location = (850.0, 40.0)
    f0 = conductor["comparison_contract"]["shared_linear_rgb_value"]
    set_principled_input(principled, ("Base Color",), (*f0, 1.0))
    set_principled_input(principled, ("Metallic",), 1.0)
    set_principled_input(principled, ("Coat Weight", "Coat"), 0.0)

    uv = tree.nodes.new("ShaderNodeUVMap")
    uv.name = "Blade_Metre_Aligned_UV"
    uv.uv_map = BLADE_UV
    uv.location = (-920.0, 300.0)
    macro_texture = tree.nodes.new("ShaderNodeTexImage")
    macro_texture.name = "Macro_Polish_Field"
    macro_texture.image = macro_image
    macro_texture.interpolation = "Linear"
    macro_texture.extension = "EXTEND"
    macro_texture.location = (-690.0, 420.0)
    tree.links.new(uv.outputs["UV"], macro_texture.inputs["Vector"])
    grind_texture = tree.nodes.new("ShaderNodeTexImage")
    grind_texture.name = "Authored_Longitudinal_Grind_Field"
    grind_texture.image = grind_image
    grind_texture.interpolation = "Linear"
    grind_texture.extension = "EXTEND"
    grind_texture.location = (-690.0, 250.0)
    tree.links.new(uv.outputs["UV"], grind_texture.inputs["Vector"])

    regions = tree.nodes.new("ShaderNodeVertexColor")
    regions.name = "Geometry_Owned_Blade_Regions"
    regions.layer_name = REGION_ATTRIBUTE
    regions.location = (-920.0, -60.0)
    separate = tree.nodes.new("ShaderNodeSeparateColor")
    separate.name = "Separate_Body_Fuller_Bevel"
    separate.mode = "RGB"
    separate.location = (-690.0, -60.0)
    tree.links.new(regions.outputs["Color"], separate.inputs["Color"])
    channels = {
        "body": separate.outputs["Red"],
        "fuller": separate.outputs["Green"],
        "bevel": separate.outputs["Blue"],
        "edge": regions.outputs["Alpha"],
    }

    region_values = profile["finish"]["regions"]
    roughness = _weighted_sum(
        tree,
        channels,
        {name: values["roughness"] for name, values in region_values.items()},
        "Roughness",
        -420.0,
        160.0,
    )
    anisotropy = _weighted_sum(
        tree,
        channels,
        {name: values["anisotropy"] for name, values in region_values.items()},
        "Anisotropy",
        -420.0,
        -320.0,
    )
    macro_strength = _weighted_sum(
        tree,
        channels,
        {name: values["macro_strength"] for name, values in region_values.items()},
        "MacroStrength",
        -40.0,
        -700.0,
    )
    grind_strength = _weighted_sum(
        tree,
        channels,
        {name: values["grind_strength"] for name, values in region_values.items()},
        "GrindStrength",
        340.0,
        -700.0,
    )

    macro_roughness = _center_scale_mask_field(
        tree,
        macro_texture.outputs["Color"],
        macro_strength,
        profile["finish"]["macro_polish"]["roughness_amplitude"],
        "Macro_Polish_Roughness",
        -380.0,
        480.0,
    )
    grind_roughness = _center_scale_mask_field(
        tree,
        grind_texture.outputs["Color"],
        grind_strength,
        profile["finish"]["medium_grind"]["roughness_amplitude"],
        "Medium_Grind_Roughness",
        -380.0,
        280.0,
    )
    finish_roughness = tree.nodes.new("ShaderNodeMath")
    finish_roughness.name = "Macro_Plus_Medium_Roughness"
    finish_roughness.operation = "ADD"
    finish_roughness.location = (190.0, 360.0)
    tree.links.new(macro_roughness, finish_roughness.inputs[0])
    tree.links.new(grind_roughness, finish_roughness.inputs[1])
    combined_roughness = tree.nodes.new("ShaderNodeMath")
    combined_roughness.name = "Regional_Plus_MultiBand_Roughness"
    combined_roughness.operation = "ADD"
    combined_roughness.use_clamp = True
    combined_roughness.location = (390.0, 250.0)
    tree.links.new(roughness, combined_roughness.inputs[0])
    tree.links.new(finish_roughness.outputs[0], combined_roughness.inputs[1])

    tangent = tree.nodes.new("ShaderNodeTangent")
    tangent.name = "Blade_Longitudinal_Tangent"
    tangent.direction_type = "UV_MAP"
    tangent.uv_map = BLADE_UV
    tangent.location = (350.0, -260.0)
    bump = tree.nodes.new("ShaderNodeBump")
    bump.name = "Micrometre_Ground_Finish_Bump"
    bump.inputs["Distance"].default_value = profile["finish"]["medium_grind"][
        "bump_distance_m"
    ]
    bump.location = (390.0, -80.0)
    bump_strength = tree.nodes.new("ShaderNodeMath")
    bump_strength.name = "Regional_Medium_Grind_Bump_Strength"
    bump_strength.operation = "MULTIPLY"
    bump_strength.inputs[1].default_value = profile["finish"]["medium_grind"][
        "bump_strength"
    ]
    bump_strength.location = (170.0, -70.0)
    tree.links.new(grind_strength, bump_strength.inputs[0])
    tree.links.new(bump_strength.outputs[0], bump.inputs["Strength"])
    tree.links.new(grind_texture.outputs["Color"], bump.inputs["Height"])

    tree.links.new(combined_roughness.outputs[0], principled.inputs["Roughness"])
    anisotropy_socket = set_principled_input(
        principled,
        ("Anisotropic IOR Level", "Anisotropic"),
        tangent_contract["selection_contract"]["selected_default"],
    )
    tree.links.new(anisotropy, principled.inputs[anisotropy_socket])
    tree.links.new(tangent.outputs["Tangent"], principled.inputs["Tangent"])
    tree.links.new(bump.outputs["Normal"], principled.inputs["Normal"])
    tree.links.new(principled.outputs["BSDF"], output.inputs["Surface"])

    node_counts = {
        "principled_count": sum(
            node.bl_idname == "ShaderNodeBsdfPrincipled" for node in tree.nodes
        ),
        "mix_shader_count": sum(
            node.bl_idname == "ShaderNodeMixShader" for node in tree.nodes
        ),
        "image_texture_count": sum(
            node.bl_idname == "ShaderNodeTexImage" for node in tree.nodes
        ),
        "bump_count": sum(node.bl_idname == "ShaderNodeBump" for node in tree.nodes),
        "tangent_count": sum(
            node.bl_idname == "ShaderNodeTangent" for node in tree.nodes
        ),
    }
    expected = {
        "principled_count": 1,
        "mix_shader_count": 0,
        "image_texture_count": 2,
        "bump_count": 1,
        "tangent_count": 1,
    }
    if node_counts != expected:
        raise RuntimeError(f"Sword-steel topology drifted: {node_counts}")
    return material, {
        **node_counts,
        "material_name": material.name,
        "metalness": 1.0,
        "conductor_f0_linear": f0,
        "body_roughness": profile["donors"]["body_roughness"],
        "body_anisotropy": profile["donors"]["body_anisotropy"],
        "anisotropy_socket": anisotropy_socket,
        "region_attribute": REGION_ATTRIBUTE,
        "blade_uv": BLADE_UV,
        "micro_response_owner": profile["finish"]["micro_response"]["owner"],
    }


def build_clay_material() -> bpy.types.Material:
    material = bpy.data.materials.new("IGGY_MAT_SwordSteel_ClayProof")
    material.diffuse_color = (0.28, 0.30, 0.33, 1.0)
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = (0.28, 0.30, 0.33, 1.0)
    principled.inputs["Roughness"].default_value = 0.64
    return material


def build_region_material() -> bpy.types.Material:
    material = bpy.data.materials.new("IGGY_MAT_SwordSteel_RegionProof")
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    emission = tree.nodes.new("ShaderNodeEmission")
    regions = tree.nodes.new("ShaderNodeVertexColor")
    regions.layer_name = REGION_ATTRIBUTE
    separate = tree.nodes.new("ShaderNodeSeparateColor")
    separate.mode = "RGB"
    combine = tree.nodes.new("ShaderNodeCombineColor")
    combine.mode = "RGB"
    tree.links.new(regions.outputs["Color"], separate.inputs["Color"])
    for channel in ("Red", "Green", "Blue"):
        add = tree.nodes.new("ShaderNodeMath")
        add.operation = "ADD"
        tree.links.new(separate.outputs[channel], add.inputs[0])
        tree.links.new(regions.outputs["Alpha"], add.inputs[1])
        tree.links.new(add.outputs[0], combine.inputs[channel])
    tree.links.new(combine.outputs["Color"], emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def build_finish_material(
    image: bpy.types.Image,
    material_name: str,
    diagnostic_range: tuple[float, float] | None = None,
) -> bpy.types.Material:
    material = bpy.data.materials.new(material_name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    emission = tree.nodes.new("ShaderNodeEmission")
    uv = tree.nodes.new("ShaderNodeUVMap")
    uv.uv_map = BLADE_UV
    texture = tree.nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Linear"
    texture.extension = "EXTEND"
    tree.links.new(uv.outputs["UV"], texture.inputs["Vector"])
    diagnostic_output = texture.outputs["Color"]
    if diagnostic_range is not None:
        minimum, maximum = diagnostic_range
        if maximum <= minimum:
            raise ValueError("finish diagnostic range must increase")
        subtract = tree.nodes.new("ShaderNodeMath")
        subtract.name = "Diagnostic_Subtract_Minimum"
        subtract.operation = "SUBTRACT"
        subtract.inputs[1].default_value = minimum
        tree.links.new(texture.outputs["Color"], subtract.inputs[0])
        divide = tree.nodes.new("ShaderNodeMath")
        divide.name = "Diagnostic_Divide_Range"
        divide.operation = "DIVIDE"
        divide.inputs[1].default_value = maximum - minimum
        divide.use_clamp = True
        tree.links.new(subtract.outputs[0], divide.inputs[0])
        diagnostic_output = divide.outputs[0]
    tree.links.new(diagnostic_output, emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def set_material(obj: bpy.types.Object, material: bpy.types.Material) -> None:
    obj.data.materials.clear()
    obj.data.materials.append(material)


def point_at(obj: bpy.types.Object, target: tuple[float, float, float]) -> None:
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def create_area_light(
    scene: bpy.types.Scene,
    name: str,
    location: tuple[float, float, float],
    energy: float,
    size: float,
    size_y: float,
) -> bpy.types.Object:
    data = bpy.data.lights.new(name, type="AREA")
    data.energy = energy
    data.shape = "RECTANGLE"
    data.size = size
    data.size_y = size_y
    obj = bpy.data.objects.new(name, data)
    obj.location = location
    scene.collection.objects.link(obj)
    return obj


def configure_camera_and_lights(
    scene: bpy.types.Scene,
    camera: bpy.types.Object,
    lights: dict[str, bpy.types.Object],
    profile: dict[str, Any],
    view: str,
) -> None:
    length_m = profile["consumer"]["blade_length_m"]
    width_m = profile["consumer"]["base_width_m"]
    aspect = scene.render.resolution_x / scene.render.resolution_y
    center = (length_m * 0.5, 0.0, 0.0)
    for light in lights.values():
        light.hide_render = True
    if view in {"finish", "macro"}:
        target = (length_m * 0.47, 0.0, 0.0)
        camera.data.ortho_scale = 0.095
    elif view in {"front", "grazing", "clay"}:
        target = (length_m * 0.48, 0.0, 0.0)
        camera.data.ortho_scale = 0.115
    else:
        target = center
        base_scale = max(width_m * 6.0, length_m / aspect * 1.18)
        camera.data.ortho_scale = base_scale * (1.62 if view == "gameplay" else 1.0)
    camera.data.type = "ORTHO"
    camera.location = (
        (target[0], -0.32, 0.28)
        if view == "clay"
        else (target[0], -0.14, 0.52)
    )
    point_at(camera, target)

    if view == "grazing":
        grazing = lights["grazing"]
        grazing.hide_render = False
        grazing.data.energy = 24.0
        grazing.location = (length_m * 0.18, -0.16, 0.045)
        point_at(grazing, (length_m * 0.72, 0.0, 0.0))
    elif view == "clay":
        key = lights["key"]
        fill = lights["fill"]
        key.hide_render = False
        fill.hide_render = False
        key.data.energy = 4.8
        key.location = (length_m * 0.28, 0.34, 0.16)
        point_at(key, center)
        fill.data.energy = 0.35
        fill.location = (length_m * 0.76, -0.22, 0.20)
        point_at(fill, center)
    elif view in {"front", "gameplay"}:
        key = lights["key"]
        fill = lights["fill"]
        key.hide_render = False
        fill.hide_render = False
        key.data.energy = 22.0
        key.location = (length_m * 0.28, -0.19, 0.46)
        point_at(key, center)
        fill.data.energy = 5.0
        fill.location = (length_m * 0.76, 0.28, 0.30)
        point_at(fill, center)


def render_still(
    scene: bpy.types.Scene,
    path: Path,
) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def build_comparison_board(
    output_root: Path,
    renders: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    order = ("front", "grazing", "gameplay", "clay", "macro", "finish")
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required to assemble the proof board")
    inputs: list[str] = []
    filters: list[str] = []
    for index, name in enumerate(order):
        inputs.extend(("-i", renders[name]["path"]))
        filters.append(
            f"[{index}:v]pad=iw+4:ih+4:2:2:color=0x181b22[tile{index}]"
        )
    filters.extend(
        (
            "[tile0][tile1][tile2]hstack=inputs=3[row0]",
            "[tile3][tile4][tile5]hstack=inputs=3[row1]",
            "[row0][row1]vstack=inputs=2[board]",
        )
    )
    board_path = output_root / "sword_steel_v1_comparison_board.png"
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
    completed = subprocess.run(command, capture_output=True, text=True)
    if completed.returncode != 0:
        raise RuntimeError("proof-board assembly failed: " + completed.stderr)
    return {
        "path": str(board_path),
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "order": list(order),
    }


def validate_saved_blend(blend_path: Path, manifest_path: Path) -> None:
    blend_path = blend_path.expanduser().resolve()
    manifest_path = manifest_path.expanduser().resolve()
    if not blend_path.is_file() or not manifest_path.is_file():
        raise FileNotFoundError("saved sword-steel proof or manifest is missing")
    bpy.ops.wm.open_mainfile(filepath=str(blend_path))
    obj = bpy.data.objects.get(FIXTURE_NAME)
    material = bpy.data.materials.get(MATERIAL_NAME)
    if obj is None or material is None:
        raise RuntimeError("saved sword-steel fixture or material is missing")
    if REGION_ATTRIBUTE not in obj.data.color_attributes:
        raise RuntimeError("saved blade region attribute is missing")
    if BLADE_UV not in obj.data.uv_layers:
        raise RuntimeError("saved blade tangent UV is missing")
    tree = material.node_tree
    if sum(node.bl_idname == "ShaderNodeBsdfPrincipled" for node in tree.nodes) != 1:
        raise RuntimeError("saved sword steel does not have exactly one Principled BSDF")
    if any(node.bl_idname == "ShaderNodeMixShader" for node in tree.nodes):
        raise RuntimeError("saved sword steel unexpectedly contains a layer mixer")
    image_nodes = [node for node in tree.nodes if node.bl_idname == "ShaderNodeTexImage"]
    if len(image_nodes) != 2 or any(node.image is None for node in image_nodes):
        raise RuntimeError("saved sword steel does not have two live finish maps")
    if any(node.image.packed_file is None for node in image_nodes):
        raise RuntimeError("saved sword finish maps are not packed")
    manifest = json.loads(manifest_path.read_text())
    manifest["reopen_validated"] = True
    manifest["reopen_validation"] = {
        "blend_sha256": sha256_file(blend_path),
        "object": obj.name,
        "material": material.name,
        "packed_images": sorted(node.image.name for node in image_nodes),
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print("SWORD_STEEL_REOPEN_VALIDATED=true")


def build(args: argparse.Namespace) -> None:
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    profile, conductor, tangent_contract = load_contracts()
    donor_hashes_before = {
        "conductor": sha256_file(CONDUCTOR_MANIFEST),
        "tangent": sha256_file(TANGENT_MANIFEST),
    }

    macro_polish = generate_macro_polish_field(profile)
    grind = generate_grind_field(profile)
    macro_path = output_root / "sword_steel_v1_macro_polish.png"
    grind_path = output_root / "sword_steel_v1_grind_field.png"
    common.write_png_gray16(macro_path, macro_polish)
    common.write_png_gray16(grind_path, grind)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version = 0
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
    scene.render.resolution_x = args.resolution_x
    scene.render.resolution_y = args.resolution_y
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.look = "AgX - Medium High Contrast"
    if scene.world is None:
        scene.world = bpy.data.worlds.new("IGGY_SwordSteel_ProofWorld")
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.008, 0.011, 0.017, 1.0)
    background.inputs["Strength"].default_value = 0.035

    obj, geometry_contract = create_analytic_blade_fixture(profile)
    macro_image = bpy.data.images.load(str(macro_path), check_existing=False)
    macro_image.name = "IGGY_IMG_SwordSteel_MacroPolish_v001"
    macro_image.colorspace_settings.name = "Non-Color"
    macro_image.pack()
    grind_image = bpy.data.images.load(str(grind_path), check_existing=False)
    grind_image.name = "IGGY_IMG_SwordSteel_LongitudinalGrind_v001"
    grind_image.colorspace_settings.name = "Non-Color"
    grind_image.pack()
    physical, material_contract = build_sword_steel_material(
        profile, conductor, tangent_contract, macro_image, grind_image
    )
    clay = build_clay_material()
    regions = build_region_material()
    macro_proof = build_finish_material(
        macro_image,
        "IGGY_MAT_SwordSteel_MacroPolishProof",
        (float(macro_polish.min()), float(macro_polish.max())),
    )
    finish = build_finish_material(
        grind_image,
        "IGGY_MAT_SwordSteel_MediumGrindProof",
    )
    set_material(obj, physical)

    camera_data = bpy.data.cameras.new("IGGY_SwordSteel_ProofCamera")
    camera = bpy.data.objects.new("IGGY_SwordSteel_ProofCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    lights = {
        "key": create_area_light(
            scene, "IGGY_SwordSteel_Key", (0.25, -0.19, 0.46), 22.0, 0.42, 0.045
        ),
        "fill": create_area_light(
            scene, "IGGY_SwordSteel_Fill", (0.72, 0.28, 0.30), 5.0, 0.54, 0.12
        ),
        "grazing": create_area_light(
            scene, "IGGY_SwordSteel_Grazing", (0.08, -0.16, 0.045), 24.0, 0.62, 0.012
        ),
    }

    renders: dict[str, dict[str, Any]] = {}
    for view in ("front", "grazing", "gameplay"):
        set_material(obj, physical)
        configure_camera_and_lights(scene, camera, lights, profile, view)
        renders[view] = render_still(
            scene, output_root / f"sword_steel_v1_{view}.png"
        )
    set_material(obj, clay)
    configure_camera_and_lights(scene, camera, lights, profile, "clay")
    renders["clay"] = render_still(
        scene, output_root / "sword_steel_v1_clay.png"
    )
    set_material(obj, regions)
    configure_camera_and_lights(scene, camera, lights, profile, "regions")
    renders["regions"] = render_still(
        scene, output_root / "sword_steel_v1_regions.png"
    )
    set_material(obj, macro_proof)
    configure_camera_and_lights(scene, camera, lights, profile, "macro")
    renders["macro"] = render_still(
        scene, output_root / "sword_steel_v1_macro.png"
    )
    set_material(obj, finish)
    configure_camera_and_lights(scene, camera, lights, profile, "finish")
    renders["finish"] = render_still(
        scene, output_root / "sword_steel_v1_finish.png"
    )
    set_material(obj, physical)

    blend_path = output_root / "sword_steel_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    comparison_board = build_comparison_board(output_root, renders)
    donor_hashes_after = {
        "conductor": sha256_file(CONDUCTOR_MANIFEST),
        "tangent": sha256_file(TANGENT_MANIFEST),
    }
    if donor_hashes_after != donor_hashes_before:
        raise RuntimeError("sword-steel build changed an accepted donor")

    macro_centered = macro_polish - 0.5
    finish_centered = grind - 0.5
    manifest = {
        "schema": "iggy3d.sword-steel-build.v1",
        "status": "DIAGNOSTIC_PRODUCTION_CANDIDATE",
        "consumer": {
            **profile["consumer"],
            "fixture_object": obj.name,
        },
        "geometry": geometry_contract,
        "material": material_contract,
        "finish": {
            "macro_polish": {
                "resolution": [macro_polish.shape[1], macro_polish.shape[0]],
                "minimum": float(macro_polish.min()),
                "maximum": float(macro_polish.max()),
                "mean": float(macro_polish.mean()),
                "signed_abs_p95": float(
                    np.percentile(np.abs(macro_centered), 95.0)
                ),
                "control_grid_shape": profile["finish"]["macro_polish"][
                    "control_grid_shape"
                ],
                "map_span_m": profile["finish"]["macro_polish"]["map_span_m"],
                "map_bit_depth": profile["finish"]["macro_polish"][
                    "map_bit_depth"
                ],
            },
            "medium_grind": {
                "resolution": [grind.shape[1], grind.shape[0]],
                "minimum": float(grind.min()),
                "maximum": float(grind.max()),
                "mean": float(grind.mean()),
                "signed_abs_p95": float(
                    np.percentile(np.abs(finish_centered), 95.0)
                ),
                "track_count": profile["finish"]["medium_grind"][
                    "track_count"
                ],
                "map_span_m": profile["finish"]["medium_grind"]["map_span_m"],
                "map_bit_depth": profile["finish"]["medium_grind"][
                    "map_bit_depth"
                ],
            },
            "micro_response": profile["finish"]["micro_response"],
        },
        "outputs": {
            "macro_polish_field": {
                "path": str(macro_path),
                "bytes": macro_path.stat().st_size,
                "sha256": sha256_file(macro_path),
            },
            "grind_field": {
                "path": str(grind_path),
                "bytes": grind_path.stat().st_size,
                "sha256": sha256_file(grind_path),
            },
            "saved_blend": {
                "path": str(blend_path),
                "bytes": blend_path.stat().st_size,
                "sha256": sha256_file(blend_path),
            },
            "comparison_board": comparison_board,
            "renders": renders,
        },
        "donors": {
            "conductor_manifest": {
                "path": str(CONDUCTOR_MANIFEST),
                "sha256_before": donor_hashes_before["conductor"],
                "sha256_after": donor_hashes_after["conductor"],
            },
            "tangent_manifest": {
                "path": str(TANGENT_MANIFEST),
                "sha256_before": donor_hashes_before["tangent"],
                "sha256_after": donor_hashes_after["tangent"],
            },
        },
        "saved_blend_images_packed": all(
            image.packed_file is not None for image in (macro_image, grind_image)
        ),
        "reopen_validated": False,
        "uses_ai_generated_imagery": False,
        "uses_damage": False,
        "unreal_parity_verified": False,
        "limitations": [
            "The consumer is an analytic diagnostic fixture, not the finished WPN-001 mesh.",
            "Only length and base width use the declared Met measurement proxy.",
            "Section, thickness, fuller, finish tracks, and response amplitudes are authored proof translations.",
            "Blender Cycles proof does not establish Unreal anisotropy parity.",
        ],
    }
    manifest_path = output_root / "sword_steel_v1_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(
        "SWORD_STEEL_BUILD="
        f"principled:{material_contract['principled_count']},"
        f"maps:{material_contract['image_texture_count']},"
        f"regions:{geometry_contract['region_polygon_counts']}"
    )
    print(f"SWORD_STEEL_OUTPUT={output_root}")


def main() -> None:
    args = parse_args()
    if args.validate_only:
        validate_saved_blend(args.blend_path, MANIFEST_PATH)
    else:
        build(args)


if __name__ == "__main__":
    main()
