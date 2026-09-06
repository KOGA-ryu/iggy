#!/usr/bin/env python3
"""Assemble the accepted intact forged-iron donors on the real hinge."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import sys
from typing import Any

import bpy
from mathutils import Vector
import numpy as np


ROOT = Path(__file__).resolve().parents[4]
MATERIAL_ROOT = Path(__file__).resolve().parent
COMMON_PATH = MATERIAL_ROOT.parent / "pattern_lab_common.py"
SOURCE_BLEND = MATERIAL_ROOT / "output" / "forged_iron_v1.blend"
GEOMETRY_BLEND = (
    MATERIAL_ROOT / "output" / "openwork_strap_hinge_geometry_v1.blend"
)
DEFAULT_OUTPUT = (
    MATERIAL_ROOT / "output" / "cumulative_intact_hinge_preview_v1"
)
TARGET_COLLECTION = "IGGY_ForgedIronActualHingeProof"
TARGET_OBJECTS = (
    "SM_GH018_FixedLeaf",
    "SM_GH018_MovingLeaf_Openwork",
    "SM_GH018_Pintle",
    "MovingKnuckle_01",
    "FixedKnuckle_02",
    "MovingKnuckle_03",
    "FixedKnuckle_04",
    "MovingKnuckle_05",
)
TANGENT_MODES = {
    "SM_GH018_FixedLeaf": "longitudinal_leaf",
    "SM_GH018_MovingLeaf_Openwork": "longitudinal_leaf",
    "SM_GH018_Pintle": "axial_pin",
    "MovingKnuckle_01": "circumferential_barrel",
    "FixedKnuckle_02": "circumferential_barrel",
    "MovingKnuckle_03": "circumferential_barrel",
    "FixedKnuckle_04": "circumferential_barrel",
    "MovingKnuckle_05": "circumferential_barrel",
}

CONDUCTOR_F0_LINEAR = (0.56, 0.57, 0.58)
CONDUCTOR_ROUGHNESS = 0.38
ANISOTROPY = 0.18
GIANT_LEAF_FORGING_AMPLITUDE_M = 0.006
EXPOSED_CONDUCTOR = 0.0
OXIDE_MINIMUM_THICKNESS_M = 0.000020
OXIDE_MAXIMUM_THICKNESS_M = 0.000090
TANGENT_UV = "IGGY_IronTangentUV_v001"
FIELD_UV = "IGGY_IronFieldUV_v001"


def _load_common():
    spec = importlib.util.spec_from_file_location(
        "iggy_pattern_lab_common_cumulative",
        COMMON_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load shared texture tools: {COMMON_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


common = _load_common()


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=1200)
    parser.add_argument("--resolution-y", type=int, default=360)
    parser.add_argument("--samples", type=int, default=24)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def point_at(obj: bpy.types.Object, target) -> None:
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def set_principled_input(
    node: bpy.types.Node,
    names: tuple[str, ...],
    value,
) -> str:
    for name in names:
        socket = node.inputs.get(name)
        if socket is not None:
            socket.default_value = value
            return name
    raise KeyError(f"Principled BSDF has none of {names}")


def _mesh_arrays(obj: bpy.types.Object) -> dict[str, np.ndarray]:
    mesh = obj.data
    mesh.update()
    return {
        "vertices": np.asarray(
            [vertex.co[:] for vertex in mesh.vertices],
            dtype=np.float32,
        ),
        "loop_vertex_indices": np.asarray(
            [loop.vertex_index for loop in mesh.loops],
            dtype=np.int32,
        ),
        "polygon_loop_starts": np.asarray(
            [polygon.loop_start for polygon in mesh.polygons],
            dtype=np.int32,
        ),
        "polygon_loop_totals": np.asarray(
            [polygon.loop_total for polygon in mesh.polygons],
            dtype=np.int32,
        ),
        "polygon_normals": np.asarray(
            [polygon.normal[:] for polygon in mesh.polygons],
            dtype=np.float32,
        ),
    }


def install_component_frames(obj: bpy.types.Object) -> dict[str, Any]:
    arrays = _mesh_arrays(obj)
    uv_m = common.component_tangent_uv(
        arrays["vertices"],
        arrays["loop_vertex_indices"],
        arrays["polygon_loop_starts"],
        arrays["polygon_loop_totals"],
        arrays["polygon_normals"],
        mode=TANGENT_MODES[obj.name],
    )
    tangent_layer = obj.data.uv_layers.get(TANGENT_UV)
    if tangent_layer is None:
        tangent_layer = obj.data.uv_layers.new(name=TANGENT_UV)
    field_layer = obj.data.uv_layers.get(FIELD_UV)
    if field_layer is None:
        field_layer = obj.data.uv_layers.new(name=FIELD_UV)

    tangent_min = uv_m.min(axis=0)
    tangent_max = uv_m.max(axis=0)
    tangent_span = np.maximum(tangent_max - tangent_min, 1.0e-6)
    if TANGENT_MODES[obj.name] == "longitudinal_leaf":
        field_coordinate_mode = "object_xz_host"
        field_coordinate_m = arrays["vertices"][:, (0, 2)][
            arrays["loop_vertex_indices"]
        ]
    else:
        field_coordinate_mode = "component_tangent_host"
        field_coordinate_m = uv_m
    field_min = field_coordinate_m.min(axis=0)
    field_max = field_coordinate_m.max(axis=0)
    field_span = np.maximum(field_max - field_min, 1.0e-6)
    normalized = (field_coordinate_m - field_min) / field_span
    for loop_index, coordinate_m in enumerate(uv_m):
        tangent_layer.data[loop_index].uv = tuple(
            float(value) for value in coordinate_m
        )
        field_layer.data[loop_index].uv = tuple(
            float(value) for value in normalized[loop_index]
        )
    return {
        "mode": TANGENT_MODES[obj.name],
        "field_coordinate_mode": field_coordinate_mode,
        "minimum_m": [float(value) for value in field_min],
        "maximum_m": [float(value) for value in field_max],
        "span_m": [float(value) for value in field_span],
        "tangent_minimum_m": [float(value) for value in tangent_min],
        "tangent_maximum_m": [float(value) for value in tangent_max],
        "tangent_span_m": [float(value) for value in tangent_span],
        "loop_count": len(uv_m),
    }


def field_resolution(length_m: float, width_m: float) -> tuple[int, int]:
    target_pitch_m = 0.004
    resolution_x = int(np.clip(math.ceil(length_m / target_pitch_m), 128, 768))
    resolution_y = int(np.clip(math.ceil(width_m / target_pitch_m), 64, 384))
    return resolution_x, resolution_y


def forging_rails(
    minimum_u_m: float,
    maximum_u_m: float,
    minimum_v_m: float,
    maximum_v_m: float,
    amplitude_m: float,
) -> list[dict[str, Any]]:
    length_m = maximum_u_m - minimum_u_m
    width_m = maximum_v_m - minimum_v_m
    unit_rails = (
        (
            -0.5,
            (
                (0.00, 0.00),
                (0.16, 0.35),
                (0.37, -0.20),
                (0.61, 0.50),
                (0.82, -0.30),
                (1.00, 0.00),
            ),
        ),
        (
            0.0,
            (
                (0.00, 0.10),
                (0.22, -0.40),
                (0.49, 0.45),
                (0.71, -0.35),
                (1.00, 0.05),
            ),
        ),
        (
            0.5,
            (
                (0.00, 0.00),
                (0.18, 0.28),
                (0.45, -0.32),
                (0.78, 0.38),
                (1.00, 0.00),
            ),
        ),
    )
    return [
        {
            "y_m": minimum_v_m + (rail_v + 0.5) * width_m,
            "knots_m": [
                [
                    minimum_u_m + knot_u * length_m,
                    knot_height * amplitude_m,
                ]
                for knot_u, knot_height in knots
            ],
        }
        for rail_v, knots in unit_rails
    ]


def oxide_thickness_rails(
    minimum_u_m: float,
    maximum_u_m: float,
    minimum_v_m: float,
    maximum_v_m: float,
) -> list[dict[str, Any]]:
    length_m = maximum_u_m - minimum_u_m
    width_m = maximum_v_m - minimum_v_m
    return [
        {
            "y_m": minimum_v_m,
            "knots_m": [
                [minimum_u_m, 0.000045],
                [minimum_u_m + length_m * 0.23, 0.000062],
                [minimum_u_m + length_m * 0.58, 0.000038],
                [maximum_u_m, 0.000053],
            ],
        },
        {
            "y_m": minimum_v_m + width_m * 0.5,
            "knots_m": [
                [minimum_u_m, 0.000052],
                [minimum_u_m + length_m * 0.34, 0.000033],
                [minimum_u_m + length_m * 0.72, 0.000071],
                [maximum_u_m, 0.000048],
            ],
        },
        {
            "y_m": maximum_v_m,
            "knots_m": [
                [minimum_u_m, 0.000049],
                [minimum_u_m + length_m * 0.27, 0.000057],
                [minimum_u_m + length_m * 0.65, 0.000041],
                [maximum_u_m, 0.000055],
            ],
        },
    ]


def build_component_fields(frame: dict[str, Any]) -> dict[str, np.ndarray]:
    minimum_u_m, minimum_v_m = frame["minimum_m"]
    maximum_u_m, maximum_v_m = frame["maximum_m"]
    length_m, width_m = frame["span_m"]
    resolution_x, resolution_y = field_resolution(length_m, width_m)
    u_axis = np.linspace(
        minimum_u_m,
        maximum_u_m,
        resolution_x,
        dtype=np.float32,
    )
    v_axis = np.linspace(
        minimum_v_m,
        maximum_v_m,
        resolution_y,
        dtype=np.float32,
    )
    u_m, v_m = np.meshgrid(u_axis, v_axis)

    amplitude_m = GIANT_LEAF_FORGING_AMPLITUDE_M * min(
        1.0,
        max(width_m, 1.0e-6) / 0.410,
    )
    forging_height_m = common.broad_forging_plane_field(
        u_m,
        v_m,
        forging_rails(
            minimum_u_m,
            maximum_u_m,
            minimum_v_m,
            maximum_v_m,
            amplitude_m,
        ),
    )
    forging_normal = common.height_to_normal_nonperiodic(
        forging_height_m,
        meters_per_pixel_x=length_m / (resolution_x - 1),
        meters_per_pixel_y=width_m / (resolution_y - 1),
    )

    oxide = common.continuous_oxide_layer_fields(
        u_m,
        v_m,
        oxide_thickness_rails(
            minimum_u_m,
            maximum_u_m,
            minimum_v_m,
            maximum_v_m,
        ),
        longitudinal_modes=[],
        minimum_thickness_m=OXIDE_MINIMUM_THICKNESS_M,
        maximum_thickness_m=OXIDE_MAXIMUM_THICKNESS_M,
    )
    response = common.smoothstep(
        0.0,
        1.0,
        oxide["thickness_response"],
    ).astype(np.float32)
    thin_srgb = np.asarray((0.090, 0.100, 0.116), dtype=np.float32)
    thick_srgb = np.asarray((0.158, 0.174, 0.202), dtype=np.float32)
    oxide_base_linear = common.srgb_to_linear(
        common.mix(
            np.broadcast_to(thin_srgb, response.shape + (3,)),
            np.broadcast_to(thick_srgb, response.shape + (3,)),
            response,
        )
    ).astype(np.float32)
    oxide_roughness = np.clip(
        0.655 + response * 0.105,
        0.655,
        0.760,
    ).astype(np.float32)

    exposure = np.full(response.shape, EXPOSED_CONDUCTOR, dtype=np.float32)
    endpoint = common.compose_surface_layer_responses(
        oxide_base_linear,
        np.broadcast_to(
            np.asarray(CONDUCTOR_F0_LINEAR, dtype=np.float32),
            oxide_base_linear.shape,
        ),
        exposure,
    )
    if float(endpoint["conductor_weight"].max()) != 0.0:
        raise RuntimeError("intact surface unexpectedly exposed conductor")
    return {
        "oxide_base_linear": oxide_base_linear,
        "oxide_roughness": oxide_roughness,
        "oxide_coverage": oxide["coverage"],
        "oxide_metalness": oxide["metalness"],
        "oxide_surface_height_m": oxide["surface_height_m"],
        "oxide_thickness_m": oxide["thickness_m"],
        "forging_height_m": forging_height_m,
        "forging_normal": forging_normal,
        "exposure": exposure,
        "amplitude_m": np.asarray(amplitude_m, dtype=np.float32),
    }


def make_float_image(
    name: str,
    values: np.ndarray,
    *,
    color: bool,
) -> bpy.types.Image:
    values = np.asarray(values, dtype=np.float32)
    if color:
        if values.ndim != 3 or values.shape[-1] != 3:
            raise ValueError("colour image requires HxWx3 values")
        height, width = values.shape[:2]
    else:
        if values.ndim != 2:
            raise ValueError("data image requires HxW values")
        height, width = values.shape
    image = bpy.data.images.new(
        name=name,
        width=width,
        height=height,
        alpha=True,
        float_buffer=True,
    )
    image.colorspace_settings.name = "Non-Color"
    rgba = np.empty((height, width, 4), dtype=np.float32)
    if color:
        rgba[..., :3] = values
    else:
        rgba[..., :3] = values[..., np.newaxis]
    rgba[..., 3] = 1.0
    image.pixels.foreach_set(rgba.reshape(-1))
    image.pack()
    return image


def add_image_node(
    tree: bpy.types.NodeTree,
    name: str,
    image: bpy.types.Image,
    uv_node: bpy.types.Node,
    location: tuple[float, float],
) -> bpy.types.Node:
    texture = tree.nodes.new("ShaderNodeTexImage")
    texture.name = name
    texture.label = name.replace("_", " ")
    texture.image = image
    texture.interpolation = "Linear"
    texture.extension = "EXTEND"
    texture.location = location
    tree.links.new(uv_node.outputs["UV"], texture.inputs["Vector"])
    return texture


def build_physical_material(
    obj: bpy.types.Object,
    images: dict[str, bpy.types.Image],
) -> tuple[bpy.types.Material, dict[str, Any]]:
    material = bpy.data.materials.new(
        f"IGGY_MAT_CumulativeIntactIron_{obj.name}_v001"
    )
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()

    output = tree.nodes.new("ShaderNodeOutputMaterial")
    output.name = "Material_Output"
    output.location = (920.0, 80.0)
    surface = tree.nodes.new("ShaderNodeMixShader")
    surface.name = "Complete_Oxide_And_Conductor_Responses"
    surface.location = (650.0, 80.0)
    exposure = tree.nodes.new("ShaderNodeValue")
    exposure.name = "Explicit_Exposed_Conductor_Only_Owner"
    exposure.outputs[0].default_value = EXPOSED_CONDUCTOR
    exposure.location = (380.0, 360.0)

    oxide_bsdf = tree.nodes.new("ShaderNodeBsdfPrincipled")
    oxide_bsdf.name = "Complete_Intact_Oxide_Response"
    oxide_bsdf.location = (330.0, 150.0)
    set_principled_input(oxide_bsdf, ("Metallic",), 0.0)
    set_principled_input(oxide_bsdf, ("IOR",), 2.10)
    set_principled_input(oxide_bsdf, ("Coat Weight", "Coat"), 0.0)

    conductor_bsdf = tree.nodes.new("ShaderNodeBsdfPrincipled")
    conductor_bsdf.name = "Complete_Hidden_Iron_Response"
    conductor_bsdf.location = (330.0, -170.0)
    set_principled_input(
        conductor_bsdf,
        ("Base Color",),
        (*CONDUCTOR_F0_LINEAR, 1.0),
    )
    set_principled_input(conductor_bsdf, ("Metallic",), 1.0)
    set_principled_input(
        conductor_bsdf,
        ("Roughness",),
        CONDUCTOR_ROUGHNESS,
    )
    anisotropy_socket = set_principled_input(
        conductor_bsdf,
        ("Anisotropic IOR Level", "Anisotropic"),
        ANISOTROPY,
    )

    tangent = tree.nodes.new("ShaderNodeTangent")
    tangent.name = "Component_Owned_Material_Tangent"
    tangent.direction_type = "UV_MAP"
    tangent.uv_map = TANGENT_UV
    tangent.location = (-20.0, -400.0)
    tree.links.new(tangent.outputs["Tangent"], conductor_bsdf.inputs["Tangent"])

    uv = tree.nodes.new("ShaderNodeUVMap")
    uv.name = "Nonrepeating_Component_Field_UV"
    uv.uv_map = FIELD_UV
    uv.location = (-760.0, 140.0)
    color_image = add_image_node(
        tree,
        "Continuous_Oxide_Optical_Colour",
        images["oxide_base"],
        uv,
        (-520.0, 260.0),
    )
    roughness_image = add_image_node(
        tree,
        "Continuous_Oxide_Roughness",
        images["oxide_roughness"],
        uv,
        (-520.0, 40.0),
    )
    normal_image = add_image_node(
        tree,
        "Open_Rail_Broad_Forging_Normal",
        images["forging_normal"],
        uv,
        (-520.0, -190.0),
    )
    normal_map = tree.nodes.new("ShaderNodeNormalMap")
    normal_map.name = "Broad_Plane_Normal_Only"
    normal_map.space = "TANGENT"
    normal_map.uv_map = FIELD_UV
    normal_map.inputs["Strength"].default_value = 1.0
    normal_map.location = (-210.0, -175.0)

    tree.links.new(color_image.outputs["Color"], oxide_bsdf.inputs["Base Color"])
    tree.links.new(
        roughness_image.outputs["Color"],
        oxide_bsdf.inputs["Roughness"],
    )
    tree.links.new(normal_image.outputs["Color"], normal_map.inputs["Color"])
    tree.links.new(normal_map.outputs["Normal"], oxide_bsdf.inputs["Normal"])
    tree.links.new(normal_map.outputs["Normal"], conductor_bsdf.inputs["Normal"])
    tree.links.new(exposure.outputs[0], surface.inputs["Fac"])
    tree.links.new(oxide_bsdf.outputs["BSDF"], surface.inputs[1])
    tree.links.new(conductor_bsdf.outputs["BSDF"], surface.inputs[2])
    tree.links.new(surface.outputs["Shader"], output.inputs["Surface"])

    node_counts = {
        "principled": sum(
            node.bl_idname == "ShaderNodeBsdfPrincipled"
            for node in tree.nodes
        ),
        "mix_shader": sum(
            node.bl_idname == "ShaderNodeMixShader" for node in tree.nodes
        ),
        "normal_map": sum(
            node.bl_idname == "ShaderNodeNormalMap" for node in tree.nodes
        ),
        "tangent": sum(
            node.bl_idname == "ShaderNodeTangent" for node in tree.nodes
        ),
        "image_texture": sum(
            node.bl_idname == "ShaderNodeTexImage" for node in tree.nodes
        ),
    }
    if node_counts != {
        "principled": 2,
        "mix_shader": 1,
        "normal_map": 1,
        "tangent": 1,
        "image_texture": 3,
    }:
        raise RuntimeError(f"Unexpected physical topology: {node_counts}")
    return material, {
        "node_counts": node_counts,
        "anisotropy_socket": anisotropy_socket,
        "exposure": float(exposure.outputs[0].default_value),
    }


def build_lane_material(
    name: str,
    image: bpy.types.Image | None,
    *,
    constant: tuple[float, float, float, float] | None = None,
) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    output.location = (430.0, 0.0)
    emission = tree.nodes.new("ShaderNodeEmission")
    emission.location = (160.0, 0.0)
    emission.inputs["Strength"].default_value = 1.0
    if image is None:
        if constant is None:
            raise ValueError("lane material requires an image or constant")
        emission.inputs["Color"].default_value = constant
    else:
        uv = tree.nodes.new("ShaderNodeUVMap")
        uv.uv_map = FIELD_UV
        uv.location = (-410.0, 0.0)
        texture = add_image_node(
            tree,
            f"{name}_Image",
            image,
            uv,
            (-140.0, 0.0),
        )
        tree.links.new(texture.outputs["Color"], emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def set_material(obj: bpy.types.Object, material: bpy.types.Material) -> None:
    obj.data.materials.clear()
    obj.data.materials.append(material)


def set_visible(objects: list[bpy.types.Object], visible: bool) -> None:
    for obj in objects:
        obj.hide_render = not visible
        obj.hide_viewport = not visible


def render_still(scene: bpy.types.Scene, path: Path) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def configure_lights(
    lights: dict[str, bpy.types.Object],
    view: str,
    target,
) -> None:
    for light in lights.values():
        light.hide_render = True
        light.hide_viewport = True
        light.data.color = (1.0, 1.0, 1.0)
    neutral = lights["neutral"]
    fill = lights["fill"]
    grazing = lights["grazing"]
    if view in {"neutral_front", "three_quarter", "gameplay"}:
        neutral.hide_render = neutral.hide_viewport = False
        fill.hide_render = fill.hide_viewport = False
        neutral.data.energy = 82.0
        neutral.data.shape = "DISK"
        neutral.data.size = 2.1
        neutral.location = (0.45, 3.3, 2.25)
        point_at(neutral, target)
        fill.data.energy = 15.0
        fill.data.size = 1.8
        fill.location = (2.8, 2.0, -1.1)
        point_at(fill, target)
    elif view == "moving_a":
        grazing.hide_render = grazing.hide_viewport = False
        grazing.data.energy = 58.0
        grazing.data.shape = "RECTANGLE"
        grazing.data.size = 2.4
        grazing.data.size_y = 0.055
        grazing.location = (-1.15, 0.62, 0.80)
        point_at(grazing, target)
    elif view in {"moving_b", "grazing"}:
        grazing.hide_render = grazing.hide_viewport = False
        grazing.data.energy = 58.0
        grazing.data.shape = "RECTANGLE"
        grazing.data.size = 2.4
        grazing.data.size_y = 0.055
        grazing.location = (2.55, 0.70, 0.30)
        point_at(grazing, target)
    else:
        raise ValueError(f"Unknown light view {view}")


def configure_camera(
    scene: bpy.types.Scene,
    view: str,
    center: tuple[float, float, float],
    span_x: float,
    span_z: float,
    aspect: float,
) -> None:
    camera = scene.camera
    base_scale = max(span_z * aspect * 1.48, span_x * 1.12)
    if view in {"neutral_front", "moving_a", "moving_b"}:
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = base_scale
        camera.location = (center[0], 5.6, center[2])
        point_at(camera, center)
    elif view == "gameplay":
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = base_scale * 1.58
        camera.location = (center[0], 7.2, center[2] + 0.35)
        point_at(camera, center)
    elif view == "three_quarter":
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = base_scale * 1.08
        camera.location = (
            center[0] - span_x * 0.05,
            5.0,
            center[2] + max(0.92, span_x * 0.24),
        )
        point_at(camera, center)
    elif view == "grazing":
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = base_scale * 1.05
        camera.location = (
            center[0] + span_x * 0.03,
            4.8,
            center[2] + max(0.45, span_x * 0.10),
        )
        point_at(camera, center)
    else:
        raise ValueError(f"Unknown camera view {view}")


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if not SOURCE_BLEND.is_file() or not GEOMETRY_BLEND.is_file():
        raise FileNotFoundError("The accepted forged-iron proof sources are missing")
    source_hash_before = sha256_file(SOURCE_BLEND)
    geometry_hash_before = sha256_file(GEOMETRY_BLEND)

    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
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

    collection = bpy.data.collections.get(TARGET_COLLECTION)
    if collection is None:
        raise RuntimeError(f"Missing target collection {TARGET_COLLECTION}")
    targets = [bpy.data.objects[name] for name in TARGET_OBJECTS]
    lights = {
        "neutral": bpy.data.objects["IGGY_IronTarget_NeutralKey"],
        "fill": bpy.data.objects["IGGY_IronTarget_CoolFill"],
        "grazing": bpy.data.objects["IGGY_IronTarget_GrazingStrip"],
    }
    keep = set(targets) | set(lights.values()) | {scene.camera}
    for obj in scene.objects:
        obj.hide_render = obj not in keep
        obj.hide_viewport = obj not in keep

    physical_materials = {}
    lane_materials: dict[str, dict[str, bpy.types.Material]] = {}
    component_contract = {}
    all_fields = {}
    for obj in targets:
        frame = install_component_frames(obj)
        fields = build_component_fields(frame)
        slug = obj.name.replace(".", "_")
        images = {
            "oxide_base": make_float_image(
                f"IGGY_IMG_{slug}_ContinuousOxideBase_v001",
                fields["oxide_base_linear"],
                color=True,
            ),
            "oxide_roughness": make_float_image(
                f"IGGY_IMG_{slug}_ContinuousOxideRoughness_v001",
                fields["oxide_roughness"],
                color=False,
            ),
            "forging_normal": make_float_image(
                f"IGGY_IMG_{slug}_OpenRailForgingNormal_v001",
                fields["forging_normal"] * 0.5 + 0.5,
                color=True,
            ),
        }
        physical, topology = build_physical_material(obj, images)
        physical_materials[obj.name] = physical
        lane_materials[obj.name] = {
            "oxide_colour": build_lane_material(
                f"IGGY_MAT_{slug}_OxideColourProof_v001",
                images["oxide_base"],
            ),
            "roughness": build_lane_material(
                f"IGGY_MAT_{slug}_RoughnessProof_v001",
                images["oxide_roughness"],
            ),
            "forging_normal": build_lane_material(
                f"IGGY_MAT_{slug}_ForgingNormalProof_v001",
                images["forging_normal"],
            ),
            "exposure": build_lane_material(
                f"IGGY_MAT_{slug}_ExposureProof_v001",
                None,
                constant=(0.0, 0.0, 0.0, 1.0),
            ),
        }
        set_material(obj, physical)
        component_contract[obj.name] = {
            "frame": frame,
            "field_resolution": [
                int(fields["oxide_roughness"].shape[1]),
                int(fields["oxide_roughness"].shape[0]),
            ],
            "forging_amplitude_m": float(fields["amplitude_m"]),
            "oxide_thickness_minimum_m": float(
                fields["oxide_thickness_m"].min()
            ),
            "oxide_thickness_maximum_m": float(
                fields["oxide_thickness_m"].max()
            ),
            "topology": topology,
            "material": physical.name,
            "images": {name: image.name for name, image in images.items()},
        }
        all_fields[obj.name] = fields

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

    renders = {}
    for view in (
        "neutral_front",
        "three_quarter",
        "grazing",
        "gameplay",
        "moving_a",
        "moving_b",
    ):
        for obj in targets:
            set_material(obj, physical_materials[obj.name])
        configure_camera(scene, view, center, span_x, span_z, aspect)
        configure_lights(lights, view, center)
        renders[view] = render_still(
            scene,
            output_root / f"cumulative_intact_hinge_{view}.png",
        )

    configure_camera(scene, "neutral_front", center, span_x, span_z, aspect)
    configure_lights(lights, "neutral_front", center)
    for lane in (
        "oxide_colour",
        "roughness",
        "forging_normal",
        "exposure",
    ):
        for obj in targets:
            set_material(obj, lane_materials[obj.name][lane])
        renders[lane] = render_still(
            scene,
            output_root / f"cumulative_intact_hinge_{lane}.png",
        )

    for obj in targets:
        set_material(obj, physical_materials[obj.name])
    saved_blend = output_root / "cumulative_intact_hinge_preview_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(saved_blend))

    source_hash_after = sha256_file(SOURCE_BLEND)
    geometry_hash_after = sha256_file(GEOMETRY_BLEND)
    if source_hash_after != source_hash_before:
        raise RuntimeError("Canonical forged-iron blend changed during proof")
    if geometry_hash_after != geometry_hash_before:
        raise RuntimeError("Approved hinge geometry changed during proof")

    coverage = np.concatenate(
        [fields["oxide_coverage"].reshape(-1) for fields in all_fields.values()]
    )
    metalness = np.concatenate(
        [fields["oxide_metalness"].reshape(-1) for fields in all_fields.values()]
    )
    surface_height = np.concatenate(
        [
            fields["oxide_surface_height_m"].reshape(-1)
            for fields in all_fields.values()
        ]
    )
    exposure = np.concatenate(
        [fields["exposure"].reshape(-1) for fields in all_fields.values()]
    )
    manifest = {
        "schema": "iggy3d.cumulative-intact-forged-iron-proof.v1",
        "status": "CUMULATIVE_INTACT_CANDIDATE_NOT_CANONICAL",
        "consumer": {
            "collection": TARGET_COLLECTION,
            "objects": list(TARGET_OBJECTS),
            "assembly_span_m": [span_x, 0.116, span_z],
        },
        "selected_donors": [
            "clean_conductor_rgb_f0",
            "component_owned_tangent",
            "open_rail_broad_forging_planes",
            "continuous_thermal_oxide",
            "dual_complete_bsdf_mixer",
        ],
        "constants": {
            "conductor_f0_linear": list(CONDUCTOR_F0_LINEAR),
            "conductor_roughness": CONDUCTOR_ROUGHNESS,
            "anisotropy": ANISOTROPY,
            "giant_leaf_forging_amplitude_m": GIANT_LEAF_FORGING_AMPLITUDE_M,
            "oxide_thickness_envelope_m": [
                OXIDE_MINIMUM_THICKNESS_M,
                OXIDE_MAXIMUM_THICKNESS_M,
            ],
        },
        "exposed_conductor": {
            "owner": "Explicit_Exposed_Conductor_Only_Owner",
            "minimum": float(exposure.min()),
            "maximum": float(exposure.max()),
        },
        "oxide": {
            "coverage_minimum": float(coverage.min()),
            "coverage_maximum": float(coverage.max()),
            "metalness_maximum": float(metalness.max()),
            "surface_height_abs_maximum_m": float(np.abs(surface_height).max()),
        },
        "node_topology": {
            "principled_count_per_material": 2,
            "mix_shader_count_per_material": 1,
            "material_identity_owner_count": 1,
        },
        "components": component_contract,
        "renders": renders,
        "saved_blend": {
            "path": str(saved_blend),
            "bytes": saved_blend.stat().st_size,
            "sha256": sha256_file(saved_blend),
            "authored_images_packed": True,
        },
        "sources": {
            "canonical_blend": {
                "path": str(SOURCE_BLEND),
                "sha256_before": source_hash_before,
                "sha256_after": source_hash_after,
            },
            "geometry_blend": {
                "path": str(GEOMETRY_BLEND),
                "sha256_before": geometry_hash_before,
                "sha256_after": geometry_hash_after,
            },
        },
        "canonical_source_unchanged": True,
        "uses_ai_generated_imagery": False,
        "uses_damage": False,
        "unreal_parity_verified": False,
        "limitations": [
            "This is a cumulative actual-hinge candidate, not canonical v004.",
            "Broad forging amplitude is calibrated to the giant leaf and scaled down by stock width for smaller components.",
            "No condition or narrative history layer is present.",
            "Blender Cycles proof does not establish Unreal parity.",
        ],
    }
    manifest_path = output_root / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(
        "CUMULATIVE_INTACT_FORGED_IRON="
        f"objects:{len(targets)},exposure:{exposure.min():.1f}-{exposure.max():.1f},"
        f"oxide_coverage:{coverage.min():.1f}-{coverage.max():.1f}"
    )
    print(f"CUMULATIVE_INTACT_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
