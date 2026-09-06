#!/usr/bin/env python3
"""Build the neutral cathedral-stone wall, arch, and trim acceptance asset."""

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


SCRIPT_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "cathedral_stone_v1.json"
GENERATOR_PATH = SCRIPT_ROOT / "generate_cathedral_stone_v1.py"
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"

GROUP_NAME = "IGGY_SH_CathedralStone_v006"
MATERIAL_NAME = "IGGY_MAT_CathedralStone_v006"
COLLECTION_NAME = "IGGY_CathedralStoneProof"
ATTRIBUTE_NAMES = (
    "iggy_material_phase",
    "iggy_material_variant",
    "iggy_stone_block_id",
    "iggy_stone_tooling_id",
    "iggy_fracture_interior",
    "iggy_carved_trim",
    "iggy_lichen_mask",
    "iggy_damp_mask",
    "iggy_traversal_id",
    "iggy_stone_face_length_m",
    "iggy_stone_face_height_m",
    "iggy_stone_depth_m",
    "iggy_tool_angle_deg",
    "iggy_tool_spacing_m",
    "iggy_tool_density_variant",
)


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--texture-resolution", type=int, default=1024)
    parser.add_argument("--render-width", type=int, default=1200)
    parser.add_argument("--render-height", type=int, default=900)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_profile() -> dict[str, Any]:
    profile = json.loads(PROFILE_PATH.read_text())
    if profile["profile_id"] != "cathedral_stone_v1":
        raise ValueError("Unexpected cathedral-stone profile")
    policy = profile["source_policy"]
    if policy["photogrammetry_runtime_texture"]:
        raise ValueError("Photogrammetry cannot be a runtime texture")
    if policy["raw_reference_pixels_used_as_runtime_texture"]:
        raise ValueError("Raw reference pixels cannot enter runtime maps")
    if any(
        float(profile["default_overlays"][name]) != 0.0
        for name in (
            "fracture",
            "lichen",
            "damp",
            "soot",
            "traversal_tint",
        )
    ):
        raise ValueError("Damage and narrative overlays must default off")
    return profile


def load_generator():
    spec = importlib.util.spec_from_file_location(
        "iggy_cathedral_stone_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("Unable to load cathedral-stone generator")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def add_node(
    tree: bpy.types.NodeTree,
    node_type: str,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = tree.nodes.new(node_type)
    node.name = name
    node.label = name
    node.location = location
    return node


def link(
    tree: bpy.types.NodeTree,
    source: bpy.types.Node,
    source_socket: str,
    target: bpy.types.Node,
    target_socket: str,
) -> None:
    tree.links.new(source.outputs[source_socket], target.inputs[target_socket])


def load_image(path: Path, *, non_color: bool) -> bpy.types.Image:
    image = bpy.data.images.load(str(path), check_existing=True)
    image.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    image.pack()
    return image


def create_material_group(output_root: Path) -> bpy.types.NodeTree:
    existing = bpy.data.node_groups.get(GROUP_NAME)
    if existing is not None:
        bpy.data.node_groups.remove(existing, do_unlink=True)
    group = bpy.data.node_groups.new(GROUP_NAME, "ShaderNodeTree")
    group.color_tag = "SHADER"
    group.description = (
        "Four-metre Santa Marina ashlar macro field plus an independently "
        "phased 64 mm biocalcarenite body field with proxy-bounded relief "
        "and a 256 mm finite-impact medieval tooling field."
    )
    for name, socket_type in (
        ("Combined Color", "NodeSocketColor"),
        ("Combined Roughness", "NodeSocketFloat"),
        ("Combined Height", "NodeSocketFloat"),
        ("Stone Body Proof", "NodeSocketColor"),
        ("Tooling Proof", "NodeSocketColor"),
        ("Stylization Proof", "NodeSocketColor"),
        ("Detail Distance Fade", "NodeSocketFloat"),
    ):
        group.interface.new_socket(
            name=name,
            in_out="OUTPUT",
            socket_type=socket_type,
        )
    output = add_node(group, "NodeGroupOutput", "Group_Output", (880, 160))
    coordinates = add_node(
        group,
        "ShaderNodeTexCoord",
        "IGGY_ObjectCoordinateAnchor",
        (-1200, 460),
    )
    separate = add_node(
        group,
        "ShaderNodeSeparateXYZ",
        "IGGY_WorldXZMetres",
        (-1010, 460),
    )
    link(group, coordinates, "Object", separate, "Vector")
    scale_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_FourMetreScaleX",
        (-810, 500),
    )
    scale_x.operation = "MULTIPLY"
    scale_x.inputs[1].default_value = 0.25
    scale_z = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_FourMetreScaleZ",
        (-810, 390),
    )
    scale_z.operation = "MULTIPLY"
    scale_z.inputs[1].default_value = 0.25
    link(group, separate, "X", scale_x, "Value")
    link(group, separate, "Z", scale_z, "Value")
    coordinate = add_node(
        group,
        "ShaderNodeCombineXYZ",
        "IGGY_WorldXZTileCoordinate",
        (-610, 450),
    )
    link(group, scale_x, "Value", coordinate, "X")
    link(group, scale_z, "Value", coordinate, "Y")

    phase = add_node(
        group,
        "ShaderNodeAttribute",
        "IGGY_PerBlockPhase",
        (-1200, -40),
    )
    phase.attribute_name = "iggy_material_phase"
    variant = add_node(
        group,
        "ShaderNodeAttribute",
        "IGGY_PerBlockVariant",
        (-1200, -180),
    )
    variant.attribute_name = "iggy_material_variant"
    detail_scale_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_64mmDetailScaleX",
        (-1000, 40),
    )
    detail_scale_x.operation = "MULTIPLY"
    detail_scale_x.inputs[1].default_value = 1.0 / 0.064
    detail_scale_z = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_64mmDetailScaleZ",
        (-1000, -80),
    )
    detail_scale_z.operation = "MULTIPLY"
    detail_scale_z.inputs[1].default_value = 1.0 / 0.064
    link(group, separate, "X", detail_scale_x, "Value")
    link(group, separate, "Z", detail_scale_z, "Value")

    quarter = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_VariantQuarterTurn",
        (-1000, -210),
    )
    quarter.operation = "MODULO"
    quarter.inputs[1].default_value = 4.0
    link(group, variant, "Fac", quarter, "Value")
    angle = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_QuarterTurnRadians",
        (-820, -210),
    )
    angle.operation = "MULTIPLY"
    angle.inputs[1].default_value = math.pi * 0.5
    link(group, quarter, "Value", angle, "Value")
    cosine = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_QuarterTurnCosine",
        (-640, -160),
    )
    cosine.operation = "COSINE"
    sine = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_QuarterTurnSine",
        (-640, -260),
    )
    sine.operation = "SINE"
    link(group, angle, "Value", cosine, "Value")
    link(group, angle, "Value", sine, "Value")

    mirror_band = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_VariantMirrorBand",
        (-1000, -350),
    )
    mirror_band.operation = "DIVIDE"
    mirror_band.inputs[1].default_value = 4.0
    link(group, variant, "Fac", mirror_band, "Value")
    mirror_floor = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_VariantMirrorFloor",
        (-820, -350),
    )
    mirror_floor.operation = "FLOOR"
    link(group, mirror_band, "Value", mirror_floor, "Value")
    mirror_modulo = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_VariantMirrorModulo",
        (-640, -350),
    )
    mirror_modulo.operation = "MODULO"
    mirror_modulo.inputs[1].default_value = 2.0
    link(group, mirror_floor, "Value", mirror_modulo, "Value")
    mirror_double = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_VariantMirrorDouble",
        (-460, -350),
    )
    mirror_double.operation = "MULTIPLY"
    mirror_double.inputs[1].default_value = -2.0
    link(group, mirror_modulo, "Value", mirror_double, "Value")
    mirror_sign = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_VariantMirrorSign",
        (-280, -350),
    )
    mirror_sign.operation = "ADD"
    mirror_sign.inputs[1].default_value = 1.0
    link(group, mirror_double, "Value", mirror_sign, "Value")
    mirrored_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_MirroredDetailX",
        (-460, -30),
    )
    mirrored_x.operation = "MULTIPLY"
    link(group, detail_scale_x, "Value", mirrored_x, "Value")
    link(group, mirror_sign, "Value", mirrored_x, "Value_001")

    x_cos = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DetailXCos",
        (-260, 40),
    )
    x_cos.operation = "MULTIPLY"
    z_sin = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DetailZSin",
        (-260, -40),
    )
    z_sin.operation = "MULTIPLY"
    x_sin = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DetailXSin",
        (-260, -120),
    )
    x_sin.operation = "MULTIPLY"
    z_cos = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DetailZCos",
        (-260, -200),
    )
    z_cos.operation = "MULTIPLY"
    link(group, mirrored_x, "Value", x_cos, "Value")
    link(group, cosine, "Value", x_cos, "Value_001")
    link(group, detail_scale_z, "Value", z_sin, "Value")
    link(group, sine, "Value", z_sin, "Value_001")
    link(group, mirrored_x, "Value", x_sin, "Value")
    link(group, sine, "Value", x_sin, "Value_001")
    link(group, detail_scale_z, "Value", z_cos, "Value")
    link(group, cosine, "Value", z_cos, "Value_001")
    rotated_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_RotatedDetailX",
        (-70, 20),
    )
    rotated_x.operation = "SUBTRACT"
    rotated_y = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_RotatedDetailY",
        (-70, -130),
    )
    rotated_y.operation = "ADD"
    link(group, x_cos, "Value", rotated_x, "Value")
    link(group, z_sin, "Value", rotated_x, "Value_001")
    link(group, x_sin, "Value", rotated_y, "Value")
    link(group, z_cos, "Value", rotated_y, "Value_001")
    phase_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DetailPhaseX",
        (110, 20),
    )
    phase_x.operation = "MULTIPLY"
    phase_x.inputs[1].default_value = 17.0
    phase_y = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DetailPhaseY",
        (110, -130),
    )
    phase_y.operation = "MULTIPLY"
    phase_y.inputs[1].default_value = 31.0
    link(group, phase, "Fac", phase_x, "Value")
    link(group, phase, "Fac", phase_y, "Value")
    detail_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_PhasedDetailX",
        (290, 20),
    )
    detail_x.operation = "ADD"
    detail_y = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_PhasedDetailY",
        (290, -130),
    )
    detail_y.operation = "ADD"
    link(group, rotated_x, "Value", detail_x, "Value")
    link(group, phase_x, "Value", detail_x, "Value_001")
    link(group, rotated_y, "Value", detail_y, "Value")
    link(group, phase_y, "Value", detail_y, "Value_001")
    detail_coordinate = add_node(
        group,
        "ShaderNodeCombineXYZ",
        "IGGY_64mmBodyDetailCoordinate",
        (470, -60),
    )
    link(group, detail_x, "Value", detail_coordinate, "X")
    link(group, detail_y, "Value", detail_coordinate, "Y")
    decorrelated_coordinate = add_node(
        group,
        "ShaderNodeVectorMath",
        "IGGY_91mmBodyDetailCoordinate",
        (650, -60),
    )
    decorrelated_coordinate.operation = "SCALE"
    decorrelated_coordinate.inputs["Scale"].default_value = 0.064 / 0.091
    link(
        group,
        detail_coordinate,
        "Vector",
        decorrelated_coordinate,
        "Vector",
    )

    tool_angle_degrees = add_node(
        group,
        "ShaderNodeAttribute",
        "IGGY_ToolAngleDegrees",
        (-1200, -1260),
    )
    tool_angle_degrees.attribute_name = "iggy_tool_angle_deg"
    tool_angle_radians = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolAngleRadians",
        (-1000, -1260),
    )
    tool_angle_radians.operation = "MULTIPLY"
    tool_angle_radians.inputs[1].default_value = math.pi / 180.0
    link(group, tool_angle_degrees, "Fac", tool_angle_radians, "Value")
    tool_cosine = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolAngleCosine",
        (-810, -1200),
    )
    tool_cosine.operation = "COSINE"
    tool_sine = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolAngleSine",
        (-810, -1320),
    )
    tool_sine.operation = "SINE"
    link(group, tool_angle_radians, "Value", tool_cosine, "Value")
    link(group, tool_angle_radians, "Value", tool_sine, "Value")
    tool_x_cos = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolWorldXCos",
        (-620, -1160),
    )
    tool_x_cos.operation = "MULTIPLY"
    tool_z_sin = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolWorldZSin",
        (-620, -1240),
    )
    tool_z_sin.operation = "MULTIPLY"
    tool_z_cos = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolWorldZCos",
        (-620, -1320),
    )
    tool_z_cos.operation = "MULTIPLY"
    tool_x_sin = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolWorldXSin",
        (-620, -1400),
    )
    tool_x_sin.operation = "MULTIPLY"
    link(group, separate, "X", tool_x_cos, "Value")
    link(group, tool_cosine, "Value", tool_x_cos, "Value_001")
    link(group, separate, "Z", tool_z_sin, "Value")
    link(group, tool_sine, "Value", tool_z_sin, "Value_001")
    link(group, separate, "Z", tool_z_cos, "Value")
    link(group, tool_cosine, "Value", tool_z_cos, "Value_001")
    link(group, separate, "X", tool_x_sin, "Value")
    link(group, tool_sine, "Value", tool_x_sin, "Value_001")
    tool_rotated_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolRotatedX",
        (-430, -1200),
    )
    tool_rotated_x.operation = "ADD"
    tool_rotated_y = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolRotatedY",
        (-430, -1360),
    )
    tool_rotated_y.operation = "SUBTRACT"
    link(group, tool_x_cos, "Value", tool_rotated_x, "Value")
    link(group, tool_z_sin, "Value", tool_rotated_x, "Value_001")
    link(group, tool_z_cos, "Value", tool_rotated_y, "Value")
    link(group, tool_x_sin, "Value", tool_rotated_y, "Value_001")
    tool_scale_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_256mmToolingScaleX",
        (-240, -1200),
    )
    tool_scale_x.operation = "MULTIPLY"
    tool_scale_x.inputs[1].default_value = 1.0 / 0.256
    tool_scale_y = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_256mmToolingScaleY",
        (-240, -1360),
    )
    tool_scale_y.operation = "MULTIPLY"
    tool_scale_y.inputs[1].default_value = 1.0 / 0.256
    link(group, tool_rotated_x, "Value", tool_scale_x, "Value")
    link(group, tool_rotated_y, "Value", tool_scale_y, "Value")
    tool_phase_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolingPhaseX",
        (-240, -1480),
    )
    tool_phase_x.operation = "MULTIPLY"
    tool_phase_x.inputs[1].default_value = 13.0
    tool_phase_y = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolingPhaseY",
        (-240, -1560),
    )
    tool_phase_y.operation = "MULTIPLY"
    tool_phase_y.inputs[1].default_value = 29.0
    link(group, phase, "Fac", tool_phase_x, "Value")
    link(group, phase, "Fac", tool_phase_y, "Value")
    phased_tool_x = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_PhasedToolingX",
        (-40, -1200),
    )
    phased_tool_x.operation = "ADD"
    phased_tool_y = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_PhasedToolingY",
        (-40, -1360),
    )
    phased_tool_y.operation = "ADD"
    link(group, tool_scale_x, "Value", phased_tool_x, "Value")
    link(group, tool_phase_x, "Value", phased_tool_x, "Value_001")
    link(group, tool_scale_y, "Value", phased_tool_y, "Value")
    link(group, tool_phase_y, "Value", phased_tool_y, "Value_001")
    tooling_coordinate = add_node(
        group,
        "ShaderNodeCombineXYZ",
        "IGGY_256mmToolingCoordinate",
        (160, -1280),
    )
    link(group, phased_tool_x, "Value", tooling_coordinate, "X")
    link(group, phased_tool_y, "Value", tooling_coordinate, "Y")

    textures = (
        (
            "IGGY_BaseColor",
            "cathedral_stone_v1_basecolor.png",
            False,
            650,
            coordinate,
        ),
        ("IGGY_Normal", "cathedral_stone_v1_normal.png", True, 460, coordinate),
        ("IGGY_ORM", "cathedral_stone_v1_orm.png", True, 270, coordinate),
        (
            "IGGY_Height",
            "cathedral_stone_v1_height.png",
            True,
            80,
            coordinate,
        ),
        (
            "IGGY_Stylization",
            "cathedral_stone_v1_stylization.png",
            True,
            -110,
            coordinate,
        ),
        (
            "IGGY_IdentityMasks",
            "cathedral_stone_v1_identity_masks.png",
            True,
            -300,
            coordinate,
        ),
        (
            "IGGY_StoneBodyMasks",
            "cathedral_stone_v1_stone_body_masks.png",
            True,
            -490,
            detail_coordinate,
        ),
        (
            "IGGY_StoneBodyMasksDecorrelated",
            "cathedral_stone_v1_stone_body_masks.png",
            True,
            -680,
            decorrelated_coordinate,
        ),
        (
            "IGGY_StoneBodyHeight",
            "cathedral_stone_v1_stone_body_height.png",
            True,
            -870,
            detail_coordinate,
        ),
        (
            "IGGY_StoneBodyHeightDecorrelated",
            "cathedral_stone_v1_stone_body_height.png",
            True,
            -1060,
            decorrelated_coordinate,
        ),
        (
            "IGGY_ToolingDetail",
            "cathedral_stone_v1_tooling_detail.png",
            True,
            -1250,
            tooling_coordinate,
        ),
    )
    nodes: dict[str, bpy.types.Node] = {}
    for name, filename, non_color, y, texture_coordinate in textures:
        node = add_node(group, "ShaderNodeTexImage", name, (-350, y))
        node.image = load_image(output_root / filename, non_color=non_color)
        node.extension = "REPEAT"
        node.interpolation = "Linear"
        link(group, texture_coordinate, "Vector", node, "Vector")
        nodes[name] = node

    orm = add_node(
        group,
        "ShaderNodeSeparateColor",
        "IGGY_SeparateORM",
        (-90, 270),
    )
    orm.mode = "RGB"
    link(group, nodes["IGGY_ORM"], "Color", orm, "Color")
    height = add_node(
        group,
        "ShaderNodeRGBToBW",
        "IGGY_HeightValue",
        (-90, 80),
    )
    link(group, nodes["IGGY_Height"], "Color", height, "Color")
    stylization_channels = add_node(
        group,
        "ShaderNodeSeparateColor",
        "IGGY_SeparateStylizationLanes",
        (-90, -110),
    )
    stylization_channels.mode = "RGB"
    link(
        group,
        nodes["IGGY_Stylization"],
        "Color",
        stylization_channels,
        "Color",
    )
    camera = add_node(
        group,
        "ShaderNodeCameraData",
        "IGGY_CameraDistance",
        (-90, -1680),
    )
    detail_fade = add_node(
        group,
        "ShaderNodeMapRange",
        "IGGY_DetailFullAt030mZeroAt3p5m",
        (120, -1680),
    )
    detail_fade.clamp = True
    detail_fade.interpolation_type = "SMOOTHERSTEP"
    detail_fade.inputs["From Min"].default_value = 0.30
    detail_fade.inputs["From Max"].default_value = 3.5
    detail_fade.inputs["To Min"].default_value = 1.0
    detail_fade.inputs["To Max"].default_value = 0.0
    link(group, camera, "View Distance", detail_fade, "Value")
    body_mask_blend = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_Blend64mmAnd91mmBodyMasks",
        (-90, -570),
    )
    body_mask_blend.blend_type = "MIX"
    body_mask_blend.inputs["Fac"].default_value = 0.34
    link(
        group,
        nodes["IGGY_StoneBodyMasks"],
        "Color",
        body_mask_blend,
        "Color1",
    )
    link(
        group,
        nodes["IGGY_StoneBodyMasksDecorrelated"],
        "Color",
        body_mask_blend,
        "Color2",
    )
    body_masks = add_node(
        group,
        "ShaderNodeSeparateColor",
        "IGGY_SeparateStoneBodyMasks",
        (-90, -490),
    )
    body_masks.mode = "RGB"
    link(
        group,
        body_mask_blend,
        "Color",
        body_masks,
        "Color",
    )
    body_height_blend = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_Blend64mmAnd91mmBodyHeight",
        (120, -860),
    )
    body_height_blend.blend_type = "MIX"
    body_height_blend.inputs["Fac"].default_value = 0.34
    link(
        group,
        nodes["IGGY_StoneBodyHeight"],
        "Color",
        body_height_blend,
        "Color1",
    )
    link(
        group,
        nodes["IGGY_StoneBodyHeightDecorrelated"],
        "Color",
        body_height_blend,
        "Color2",
    )
    body_height_value = add_node(
        group,
        "ShaderNodeRGBToBW",
        "IGGY_StoneBodyHeightValue",
        (330, -860),
    )
    link(
        group,
        body_height_blend,
        "Color",
        body_height_value,
        "Color",
    )
    visible_body_height = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DistanceFadedStoneBodyHeight",
        (540, -860),
    )
    visible_body_height.operation = "MULTIPLY"
    link(group, body_height_value, "Val", visible_body_height, "Value")
    link(group, detail_fade, "Result", visible_body_height, "Value_001")
    tooling_channels = add_node(
        group,
        "ShaderNodeSeparateColor",
        "IGGY_SeparateToolingDensityVariants",
        (360, -1210),
    )
    tooling_channels.mode = "RGB"
    link(
        group,
        nodes["IGGY_ToolingDetail"],
        "Color",
        tooling_channels,
        "Color",
    )
    tooling_density = add_node(
        group,
        "ShaderNodeAttribute",
        "IGGY_ToolDensityVariant",
        (360, -1430),
    )
    tooling_density.attribute_name = "iggy_tool_density_variant"
    tooling_density_modulo = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolDensityModulo",
        (550, -1430),
    )
    tooling_density_modulo.operation = "MODULO"
    tooling_density_modulo.inputs[1].default_value = 3.0
    link(
        group,
        tooling_density,
        "Fac",
        tooling_density_modulo,
        "Value",
    )
    use_average_tooling = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_UseAverageTooling",
        (740, -1390),
    )
    use_average_tooling.operation = "COMPARE"
    use_average_tooling.inputs[1].default_value = 1.0
    use_average_tooling.inputs[2].default_value = 0.1
    use_dense_tooling = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_UseDenseTooling",
        (740, -1500),
    )
    use_dense_tooling.operation = "COMPARE"
    use_dense_tooling.inputs[1].default_value = 2.0
    use_dense_tooling.inputs[2].default_value = 0.1
    link(
        group,
        tooling_density_modulo,
        "Value",
        use_average_tooling,
        "Value",
    )
    link(
        group,
        tooling_density_modulo,
        "Value",
        use_dense_tooling,
        "Value",
    )
    sparse_or_average = add_node(
        group,
        "ShaderNodeMix",
        "IGGY_SelectSparseOrAverageTooling",
        (940, -1370),
    )
    sparse_or_average.data_type = "FLOAT"
    link(
        group,
        use_average_tooling,
        "Value",
        sparse_or_average,
        "Factor",
    )
    link(
        group,
        tooling_channels,
        "Red",
        sparse_or_average,
        "A",
    )
    link(
        group,
        tooling_channels,
        "Green",
        sparse_or_average,
        "B",
    )
    selected_tooling = add_node(
        group,
        "ShaderNodeMix",
        "IGGY_SelectToolingDensityVariant",
        (1140, -1370),
    )
    selected_tooling.data_type = "FLOAT"
    link(
        group,
        use_dense_tooling,
        "Value",
        selected_tooling,
        "Factor",
    )
    link(
        group,
        sparse_or_average,
        "Result",
        selected_tooling,
        "A",
    )
    link(
        group,
        tooling_channels,
        "Blue",
        selected_tooling,
        "B",
    )
    fossil_factor = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_FossilColorAmount",
        (120, 650),
    )
    fossil_factor.operation = "MULTIPLY"
    fossil_factor.inputs[1].default_value = 0.22
    link(group, body_masks, "Red", fossil_factor, "Value")
    visible_fossil_factor = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DistanceFadedFossilColor",
        (300, 720),
    )
    visible_fossil_factor.operation = "MULTIPLY"
    link(group, fossil_factor, "Value", visible_fossil_factor, "Value")
    link(group, detail_fade, "Result", visible_fossil_factor, "Value_001")
    fossil_mix = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_CalcareousFossilColor",
        (500, 650),
    )
    fossil_mix.blend_type = "MIX"
    fossil_mix.inputs["Color2"].default_value = (
        0.82,
        0.68,
        0.48,
        1.0,
    )
    link(group, visible_fossil_factor, "Value", fossil_mix, "Fac")
    link(group, nodes["IGGY_BaseColor"], "Color", fossil_mix, "Color1")

    silicate_factor = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_SilicateColorAmount",
        (500, 570),
    )
    silicate_factor.operation = "MULTIPLY"
    silicate_factor.inputs[1].default_value = 0.14
    link(group, body_masks, "Green", silicate_factor, "Value")
    visible_silicate_factor = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DistanceFadedSilicateColor",
        (680, 700),
    )
    visible_silicate_factor.operation = "MULTIPLY"
    link(group, silicate_factor, "Value", visible_silicate_factor, "Value")
    link(group, detail_fade, "Result", visible_silicate_factor, "Value_001")
    silicate_mix = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_CoolSilicateColor",
        (880, 590),
    )
    silicate_mix.blend_type = "MIX"
    silicate_mix.inputs["Color2"].default_value = (
        0.52,
        0.48,
        0.41,
        1.0,
    )
    link(group, visible_silicate_factor, "Value", silicate_mix, "Fac")
    link(group, fossil_mix, "Color", silicate_mix, "Color1")

    pore_factor = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_VisiblePoreColorAmount",
        (880, 500),
    )
    pore_factor.operation = "MULTIPLY"
    pore_factor.inputs[1].default_value = 0.42
    link(group, body_masks, "Blue", pore_factor, "Value")
    visible_pore_factor = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DistanceFadedPoreColor",
        (1060, 700),
    )
    visible_pore_factor.operation = "MULTIPLY"
    link(group, pore_factor, "Value", visible_pore_factor, "Value")
    link(group, detail_fade, "Result", visible_pore_factor, "Value_001")
    pore_mix = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_VisiblePoreColor",
        (1260, 570),
    )
    pore_mix.blend_type = "MULTIPLY"
    pore_mix.inputs["Color2"].default_value = (
        0.55,
        0.57,
        0.55,
        1.0,
    )
    link(group, visible_pore_factor, "Value", pore_mix, "Fac")
    link(group, silicate_mix, "Color", pore_mix, "Color1")

    visible_tooling = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DistanceFadedTooling",
        (1260, 360),
    )
    visible_tooling.operation = "MULTIPLY"
    link(group, selected_tooling, "Result", visible_tooling, "Value")
    link(group, detail_fade, "Result", visible_tooling, "Value_001")
    tooling_color_amount = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolingColorAmount",
        (1460, 440),
    )
    tooling_color_amount.operation = "MULTIPLY"
    tooling_color_amount.inputs[1].default_value = 0.28
    link(
        group,
        visible_tooling,
        "Value",
        tooling_color_amount,
        "Value",
    )
    tooling_color = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_ToolingColorLinework",
        (1660, 560),
    )
    tooling_color.blend_type = "MULTIPLY"
    tooling_color.inputs["Color2"].default_value = (
        0.68,
        0.71,
        0.72,
        1.0,
    )
    link(
        group,
        tooling_color_amount,
        "Value",
        tooling_color,
        "Fac",
    )
    link(group, pore_mix, "Color", tooling_color, "Color1")

    fossil_roughness = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_FossilRoughness",
        (120, 300),
    )
    fossil_roughness.operation = "MULTIPLY"
    fossil_roughness.inputs[1].default_value = 0.015
    silicate_roughness = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_SilicateRoughness",
        (120, 220),
    )
    silicate_roughness.operation = "MULTIPLY"
    silicate_roughness.inputs[1].default_value = -0.025
    pore_roughness = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_VisiblePoreRoughness",
        (120, 140),
    )
    pore_roughness.operation = "MULTIPLY"
    pore_roughness.inputs[1].default_value = 0.045
    link(group, body_masks, "Red", fossil_roughness, "Value")
    link(group, body_masks, "Green", silicate_roughness, "Value")
    link(group, body_masks, "Blue", pore_roughness, "Value")
    roughness_fossil = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_RoughnessPlusFossil",
        (340, 280),
    )
    roughness_fossil.operation = "ADD"
    link(group, orm, "Green", roughness_fossil, "Value")
    link(group, fossil_roughness, "Value", roughness_fossil, "Value_001")
    roughness_silicate = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_RoughnessPlusSilicate",
        (530, 270),
    )
    roughness_silicate.operation = "ADD"
    link(group, roughness_fossil, "Value", roughness_silicate, "Value")
    link(group, silicate_roughness, "Value", roughness_silicate, "Value_001")
    roughness_final = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_RoughnessPlusVisiblePore",
        (720, 260),
    )
    roughness_final.operation = "ADD"
    roughness_final.use_clamp = True
    link(group, roughness_silicate, "Value", roughness_final, "Value")
    link(group, pore_roughness, "Value", roughness_final, "Value_001")
    body_roughness_delta = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_StoneBodyRoughnessDelta",
        (900, 80),
    )
    body_roughness_delta.operation = "SUBTRACT"
    link(group, roughness_final, "Value", body_roughness_delta, "Value")
    link(group, orm, "Green", body_roughness_delta, "Value_001")
    faded_body_roughness_delta = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_DistanceFadedStoneBodyRoughness",
        (1080, 80),
    )
    faded_body_roughness_delta.operation = "MULTIPLY"
    link(
        group,
        body_roughness_delta,
        "Value",
        faded_body_roughness_delta,
        "Value",
    )
    link(
        group,
        detail_fade,
        "Result",
        faded_body_roughness_delta,
        "Value_001",
    )
    visible_body_roughness = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_VisibleStoneBodyRoughness",
        (1260, 80),
    )
    visible_body_roughness.operation = "ADD"
    link(group, orm, "Green", visible_body_roughness, "Value")
    link(
        group,
        faded_body_roughness_delta,
        "Value",
        visible_body_roughness,
        "Value_001",
    )

    tooling_roughness_amount = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolingRoughnessAmount",
        (920, 170),
    )
    tooling_roughness_amount.operation = "MULTIPLY"
    tooling_roughness_amount.inputs[1].default_value = 0.024
    link(
        group,
        visible_tooling,
        "Value",
        tooling_roughness_amount,
        "Value",
    )
    tooling_roughness = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_ToolingRoughnessLinework",
        (1120, 250),
    )
    tooling_roughness.operation = "ADD"
    tooling_roughness.use_clamp = True
    link(
        group,
        visible_body_roughness,
        "Value",
        tooling_roughness,
        "Value",
    )
    link(
        group,
        tooling_roughness_amount,
        "Value",
        tooling_roughness,
        "Value_001",
    )

    ink_amount = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_SelectiveStructuralInkAmount",
        (1850, 520),
    )
    ink_amount.operation = "MULTIPLY"
    ink_amount.inputs[1].default_value = 0.72
    link(group, stylization_channels, "Red", ink_amount, "Value")
    structural_ink = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_SelectiveStructuralInk",
        (2050, 560),
    )
    structural_ink.blend_type = "MULTIPLY"
    structural_ink.inputs["Color2"].default_value = (
        0.29,
        0.25,
        0.21,
        1.0,
    )
    link(group, ink_amount, "Value", structural_ink, "Fac")
    link(group, tooling_color, "Color", structural_ink, "Color1")
    highlight_amount = add_node(
        group,
        "ShaderNodeMath",
        "IGGY_SelectiveArrisHighlightAmount",
        (2050, 430),
    )
    highlight_amount.operation = "MULTIPLY"
    highlight_amount.inputs[1].default_value = 0.32
    link(group, stylization_channels, "Green", highlight_amount, "Value")
    arris_highlight = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_SelectiveArrisHighlight",
        (2250, 560),
    )
    arris_highlight.blend_type = "MIX"
    arris_highlight.inputs["Color2"].default_value = (
        0.78,
        0.69,
        0.54,
        1.0,
    )
    link(group, highlight_amount, "Value", arris_highlight, "Fac")
    link(group, structural_ink, "Color", arris_highlight, "Color1")

    link(group, arris_highlight, "Color", output, "Combined Color")
    link(group, tooling_roughness, "Value", output, "Combined Roughness")
    link(
        group,
        visible_body_height,
        "Value",
        output,
        "Combined Height",
    )
    link(
        group,
        body_mask_blend,
        "Color",
        output,
        "Stone Body Proof",
    )
    link(
        group,
        selected_tooling,
        "Result",
        output,
        "Tooling Proof",
    )
    link(
        group,
        nodes["IGGY_Stylization"],
        "Color",
        output,
        "Stylization Proof",
    )
    link(
        group,
        detail_fade,
        "Result",
        output,
        "Detail Distance Fade",
    )
    return group


def configure_material(group: bpy.types.NodeTree) -> bpy.types.Material:
    material = bpy.data.materials.get(MATERIAL_NAME)
    if material is None:
        material = bpy.data.materials.new(MATERIAL_NAME)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (650, 180),
    )
    principled = add_node(
        tree,
        "ShaderNodeBsdfPrincipled",
        "Cathedral_Stone_BSDF",
        (390, 180),
    )
    system = add_node(
        tree,
        "ShaderNodeGroup",
        "Cathedral_Stone_System",
        (-230, 270),
    )
    system.node_tree = group
    bump = add_node(
        tree,
        "ShaderNodeBump",
        "Metre_Scale_Stone_Height",
        (130, -20),
    )
    bump.inputs["Strength"].default_value = 1.0
    bump.inputs["Distance"].default_value = 0.00113
    link(tree, system, "Combined Color", principled, "Base Color")
    link(tree, system, "Combined Roughness", principled, "Roughness")
    link(tree, system, "Combined Height", bump, "Height")
    link(tree, bump, "Normal", principled, "Normal")
    principled.inputs["Metallic"].default_value = 0.0
    principled.inputs["IOR"].default_value = 1.46
    link(tree, principled, "BSDF", output, "Surface")
    material["iggy_material_schema"] = (
        "iggy3d.material.cathedral_stone_v1.v6"
    )
    material["iggy_tile_size_m"] = 4.0
    material["iggy_stone_body_detail_span_m"] = 0.064
    material["iggy_tooling_detail_span_m"] = 0.256
    material["iggy_detail_full_distance_m"] = 0.30
    material["iggy_detail_zero_distance_m"] = 3.5
    material["iggy_stylization_lanes_live"] = True
    material["iggy_stone_body_relief_amplitude_m"] = 0.00113
    material["iggy_relief_proxy_measurement_id"] = (
        "sabucina_calcarenite_surface_topography_proxy"
    )
    material["iggy_surface_relief_depth_authored"] = True
    material["iggy_unmeasured_block_plane_depth_authored"] = False
    material["iggy_unmeasured_tool_depth_authored"] = False
    material["iggy_unmeasured_arris_depth_authored"] = False
    material["iggy_unmeasured_mortar_depth_authored"] = False
    material["iggy_unmeasured_damage_depth_authored"] = False
    material["iggy_default_fracture"] = 0.0
    material["iggy_default_lichen"] = 0.0
    material["iggy_default_damp"] = 0.0
    return material


def ensure_collection() -> bpy.types.Collection:
    existing = bpy.data.collections.get(COLLECTION_NAME)
    if existing is not None:
        for obj in list(existing.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.collections.remove(existing)
    collection = bpy.data.collections.new(COLLECTION_NAME)
    bpy.context.scene.collection.children.link(collection)
    return collection


def write_float_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    value: float,
) -> None:
    existing = mesh.attributes.get(name)
    if existing is not None:
        mesh.attributes.remove(existing)
    attribute = mesh.attributes.new(name, "FLOAT", "POINT")
    attribute.data.foreach_set(
        "value",
        [float(value)] * len(mesh.vertices),
    )


def add_contract_attributes(
    mesh: bpy.types.Mesh,
    *,
    phase: float,
    variant: float,
    carved_trim: float = 0.0,
    block_id: float = 0.0,
    tooling_id: float = 0.0,
    face_length_m: float = 0.0,
    face_height_m: float = 0.0,
    depth_m: float = 0.0,
    tool_angle_deg: float = 0.0,
    tool_spacing_m: float = 0.0,
    tool_density_variant: float = 0.0,
) -> None:
    values = {
        "iggy_material_phase": phase,
        "iggy_material_variant": variant,
        "iggy_stone_block_id": block_id,
        "iggy_stone_tooling_id": tooling_id,
        "iggy_fracture_interior": 0.0,
        "iggy_carved_trim": carved_trim,
        "iggy_lichen_mask": 0.0,
        "iggy_damp_mask": 0.0,
        "iggy_traversal_id": 0.0,
        "iggy_stone_face_length_m": face_length_m,
        "iggy_stone_face_height_m": face_height_m,
        "iggy_stone_depth_m": depth_m,
        "iggy_tool_angle_deg": tool_angle_deg,
        "iggy_tool_spacing_m": tool_spacing_m,
        "iggy_tool_density_variant": tool_density_variant,
    }
    for name, value in values.items():
        write_float_attribute(mesh, name, value)


def create_box(
    collection: bpy.types.Collection,
    name: str,
    bounds: tuple[float, float, float, float, float, float],
    material: bpy.types.Material,
    *,
    phase: float = 0.0,
    variant: float = 0.0,
    carved_trim: float = 0.0,
    bevel_width: float = 0.012,
    block_id: float = 0.0,
    tooling_id: float = 0.0,
    face_length_m: float = 0.0,
    face_height_m: float = 0.0,
    depth_m: float = 0.0,
    tool_angle_deg: float = 0.0,
    tool_spacing_m: float = 0.0,
    tool_density_variant: float = 0.0,
) -> bpy.types.Object:
    x0, x1, y0, y1, z0, z1 = bounds
    vertices = [
        (x0, y0, z0),
        (x1, y0, z0),
        (x1, y1, z0),
        (x0, y1, z0),
        (x0, y0, z1),
        (x1, y0, z1),
        (x1, y1, z1),
        (x0, y1, z1),
    ]
    faces = [
        (0, 3, 2, 1),
        (4, 5, 6, 7),
        (0, 1, 5, 4),
        (1, 2, 6, 5),
        (2, 3, 7, 6),
        (3, 0, 4, 7),
    ]
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.materials.append(material)
    add_contract_attributes(
        mesh,
        phase=phase,
        variant=variant,
        carved_trim=carved_trim,
        block_id=block_id,
        tooling_id=tooling_id,
        face_length_m=face_length_m,
        face_height_m=face_height_m,
        depth_m=depth_m,
        tool_angle_deg=tool_angle_deg,
        tool_spacing_m=tool_spacing_m,
        tool_density_variant=tool_density_variant,
    )
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj["iggy_stone_block_id"] = float(block_id)
    obj["iggy_stone_tooling_id"] = float(tooling_id)
    obj["iggy_stone_face_length_m"] = float(face_length_m)
    obj["iggy_stone_face_height_m"] = float(face_height_m)
    obj["iggy_stone_depth_m"] = float(depth_m)
    obj["iggy_tool_angle_deg"] = float(tool_angle_deg)
    obj["iggy_tool_spacing_m"] = float(tool_spacing_m)
    obj["iggy_tool_density_variant"] = float(tool_density_variant)
    if bevel_width > 0.0:
        bevel = obj.modifiers.new("Dressed_Stone_Arris", "BEVEL")
        bevel.width = bevel_width
        bevel.segments = 2
    return obj


def create_measured_stone_materials() -> list[bpy.types.Material]:
    """Neutral limestone value variants for geometry acceptance.

    These are proof materials, not the runtime surface package. They make
    individual volumes readable without painting fake joints onto large slabs.
    """
    colors = (
        (0.47, 0.43, 0.35, 1.0),
        (0.52, 0.48, 0.40, 1.0),
        (0.56, 0.52, 0.44, 1.0),
        (0.50, 0.47, 0.41, 1.0),
        (0.58, 0.54, 0.46, 1.0),
    )
    return [
        flat_material(
            f"IGGY_MAT_MeasuredAshlarNeutral_{index:02d}",
            color,
            roughness=0.72,
        )
        for index, color in enumerate(colors, start=1)
    ]


def create_measured_ashlar_wall(
    collection: bpy.types.Collection,
    recipe: Any,
    stone_materials: list[bpy.types.Material],
    mortar_material: bpy.types.Material,
) -> tuple[list[bpy.types.Object], list[bpy.types.Object], dict[str, Any]]:
    """Build one surveyed-envelope facing wythe as individual stone volumes."""
    tile = float(recipe.tile_size_m)
    bed = float(recipe.mortar["bed_joint_m"])
    perpend = float(recipe.mortar["perpend_joint_m"])
    display_min_x = -3.0
    display_max_x = 3.0
    pattern_shift_x = -tile * 0.5
    front_y = 0.0
    block_objects: list[bpy.types.Object] = []
    mortar_objects: list[bpy.types.Object] = []
    physical_id = 1
    all_face_lengths: list[float] = []
    all_face_heights: list[float] = []
    all_depths: list[float] = []

    for course_index, course in enumerate(recipe.courses):
        z0 = float(course["bottom_m"]) + bed * 0.5
        z1 = float(course["top_m"]) - bed * 0.5
        face_height = z1 - z0
        cursor = float(course["bond_offset_m"])
        origins: list[tuple[float, dict[str, Any], int]] = []
        for design_index, specification in enumerate(course["blocks"]):
            origins.append((cursor, specification, design_index))
            cursor += float(specification["face_length_m"]) + perpend

        joint_centres: set[float] = set()
        for origin, specification, design_index in origins:
            pitch = float(specification["face_length_m"]) + perpend
            for repeat in range(-3, 4):
                interval_x0 = origin + repeat * tile + pattern_shift_x
                interval_x1 = interval_x0 + pitch
                face_x0 = interval_x0 + perpend * 0.5
                face_x1 = interval_x1 - perpend * 0.5
                if face_x1 <= display_min_x or face_x0 >= display_max_x:
                    continue
                depth = float(specification["depth_m"])
                phase = (
                    int(specification["face_seed"]) * 0.61803398875
                ) % 1.0
                variant = float(int(specification["face_seed"]) % 8)
                obj = create_box(
                    collection,
                    (
                        f"IGGY_MeasuredAshlar_C{course_index + 1:02d}"
                        f"_B{physical_id:03d}"
                    ),
                    (face_x0, face_x1, front_y, front_y + depth, z0, z1),
                    stone_materials[
                        int(specification["face_seed"]) % len(stone_materials)
                    ],
                    phase=phase,
                    variant=variant,
                    bevel_width=0.0,
                    block_id=float(physical_id),
                    tooling_id=1.0,
                    face_length_m=float(specification["face_length_m"]),
                    face_height_m=face_height,
                    depth_m=depth,
                    tool_angle_deg=float(specification["tool_angle_deg"]),
                    tool_spacing_m=float(specification["tool_spacing_m"]),
                    tool_density_variant=float(
                        int(specification["face_seed"]) % 3
                    ),
                )
                obj["iggy_source_design_index"] = float(
                    course_index * len(course["blocks"]) + design_index + 1
                )
                obj["iggy_course_index"] = float(course_index)
                block_objects.append(obj)
                all_face_lengths.append(float(specification["face_length_m"]))
                all_face_heights.append(face_height)
                all_depths.append(depth)
                physical_id += 1
                joint_centres.add(round(interval_x0, 6))

        for joint_index, centre_x in enumerate(sorted(joint_centres), start=1):
            if not display_min_x <= centre_x <= display_max_x:
                continue
            mortar_objects.append(
                create_box(
                    collection,
                    (
                        f"IGGY_MeasuredAshlarMortar_C{course_index + 1:02d}"
                        f"_P{joint_index:02d}"
                    ),
                    (
                        centre_x - perpend * 0.5,
                        centre_x + perpend * 0.5,
                        front_y,
                        front_y + 0.04,
                        z0,
                        z1,
                    ),
                    mortar_material,
                    bevel_width=0.0,
                )
            )

    for course_index in range(1, len(recipe.courses)):
        centre_z = float(recipe.courses[course_index]["bottom_m"])
        mortar_objects.append(
            create_box(
                collection,
                f"IGGY_MeasuredAshlarMortar_Bed{course_index:02d}",
                (
                    display_min_x,
                    display_max_x,
                    front_y,
                    front_y + 0.04,
                    centre_z - bed * 0.5,
                    centre_z + bed * 0.5,
                ),
                mortar_material,
                bevel_width=0.0,
            )
        )

    return block_objects, mortar_objects, {
        "physical_block_count": len(block_objects),
        "mortar_body_count": len(mortar_objects),
        "face_length_range_m": [min(all_face_lengths), max(all_face_lengths)],
        "face_height_range_m": [min(all_face_heights), max(all_face_heights)],
        "depth_range_m": [min(all_depths), max(all_depths)],
        "bed_joint_m": bed,
        "perpend_joint_m": perpend,
        "mortar_recess_m": float(recipe.mortar["recess_m"]),
        "bevel_radius_status": "unknown; no bevel modifier authored",
        "construction_scope": recipe.measurement_authority[
            "construction_scope"
        ],
    }


def create_arch_voussoir(
    collection: bpy.types.Collection,
    material: bpy.types.Material,
    index: int,
    *,
    count: int,
    center_z: float,
    inner_radius: float,
    outer_radius: float,
) -> bpy.types.Object:
    start = math.pi * index / count
    end = math.pi * (index + 1) / count
    gap = math.radians(0.55)
    start += gap
    end -= gap
    y_front = -0.245
    y_back = -0.075
    points = [
        (math.cos(start) * inner_radius, center_z + math.sin(start) * inner_radius),
        (math.cos(end) * inner_radius, center_z + math.sin(end) * inner_radius),
        (math.cos(end) * outer_radius, center_z + math.sin(end) * outer_radius),
        (math.cos(start) * outer_radius, center_z + math.sin(start) * outer_radius),
    ]
    vertices = [(x, y_back, z) for x, z in points]
    vertices.extend((x, y_front, z) for x, z in points)
    faces = [
        (0, 1, 2, 3),
        (7, 6, 5, 4),
        (0, 4, 5, 1),
        (1, 5, 6, 2),
        (2, 6, 7, 3),
        (3, 7, 4, 0),
    ]
    name = f"IGGY_CathedralArch_Voussoir_{index + 1:02d}"
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.materials.append(material)
    add_contract_attributes(
        mesh,
        phase=(index + 1) / count,
        variant=(index % 4) / 4.0,
        carved_trim=1.0,
    )
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    bevel = obj.modifiers.new("Hand_Dressed_Voussoir_Arris", "BEVEL")
    bevel.width = 0.016
    bevel.segments = 2
    return obj


def flat_material(
    name: str,
    color: tuple[float, float, float, float],
    *,
    roughness: float,
) -> bpy.types.Material:
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.diffuse_color = color
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = color
    principled.inputs["Roughness"].default_value = roughness
    return material


def create_arch_shadow(
    collection: bpy.types.Collection,
) -> bpy.types.Object:
    material = flat_material(
        "IGGY_MAT_CathedralArchShadow",
        (0.012, 0.015, 0.019, 1.0),
        roughness=1.0,
    )
    center_z = 2.0
    radius = 1.03
    segments = 24
    vertices = [
        (-radius, -0.255, 0.0),
        (radius, -0.255, 0.0),
        (radius, -0.255, center_z),
        (-radius, -0.255, center_z),
        (0.0, -0.255, center_z),
    ]
    for index in range(segments + 1):
        angle = math.pi * index / segments
        vertices.append(
            (
                math.cos(angle) * radius,
                -0.255,
                center_z + math.sin(angle) * radius,
            )
        )
    faces: list[tuple[int, ...]] = [(0, 1, 2, 3)]
    for index in range(segments):
        faces.append((4, 5 + index, 6 + index))
    mesh = bpy.data.meshes.new("IGGY_CathedralArchShadow_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.materials.append(material)
    mesh.update()
    obj = bpy.data.objects.new("IGGY_CathedralArchShadow", mesh)
    collection.objects.link(obj)
    return obj


def point_object(
    obj: bpy.types.Object,
    target: tuple[float, float, float],
) -> None:
    obj.rotation_euler = (
        Vector(target) - obj.location
    ).to_track_quat("-Z", "Y").to_euler()


def create_camera(
    collection: bpy.types.Collection,
    name: str,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    *,
    lens: float,
) -> bpy.types.Object:
    data = bpy.data.cameras.new(name)
    data.lens = lens
    camera = bpy.data.objects.new(name, data)
    collection.objects.link(camera)
    camera.location = location
    point_object(camera, target)
    return camera


def add_area_light(
    collection: bpy.types.Collection,
    name: str,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    *,
    energy: float,
    size: float,
    color: tuple[float, float, float],
) -> bpy.types.Object:
    data = bpy.data.lights.new(name, "AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = color
    light = bpy.data.objects.new(name, data)
    collection.objects.link(light)
    light.location = location
    point_object(light, target)
    return light


def configure_scene(
    collection: bpy.types.Collection,
) -> tuple[dict[str, bpy.types.Object], dict[str, bpy.types.Object]]:
    scene = bpy.context.scene
    cameras = {
        "hero": create_camera(
            collection,
            "IGGY_CAM_CathedralStoneHero",
            (5.5, -8.8, 4.6),
            (0.0, -0.02, 2.1),
            lens=58.0,
        ),
        "grazing": create_camera(
            collection,
            "IGGY_CAM_CathedralStoneGrazing",
            (3.6, -6.3, 2.9),
            (-0.2, -0.05, 2.0),
            lens=74.0,
        ),
        "front": create_camera(
            collection,
            "IGGY_CAM_CathedralStoneFront",
            (0.0, -10.5, 2.1),
            (0.0, -0.02, 2.1),
            lens=64.0,
        ),
        "distance": create_camera(
            collection,
            "IGGY_CAM_CathedralStoneDistance",
            (0.0, -15.2, 3.0),
            (0.0, -0.02, 2.0),
            lens=58.0,
        ),
        "close": create_camera(
            collection,
            "IGGY_CAM_MeasuredAshlarClose",
            (0.62, -2.35, 1.45),
            (0.48, 0.02, 1.42),
            lens=72.0,
        ),
        "surface_detail": create_camera(
            collection,
            "IGGY_CAM_BiocalcareniteBodyDetail",
            (0.48, -0.52, 1.42),
            (0.48, 0.02, 1.42),
            lens=82.0,
        ),
        "depth": create_camera(
            collection,
            "IGGY_CAM_MeasuredAshlarDepth",
            (3.8, 5.2, 3.25),
            (0.0, 0.14, 2.0),
            lens=62.0,
        ),
    }
    lights = {
        "key": add_area_light(
            collection,
            "IGGY_CathedralStone_Key",
            (-5.8, -4.6, 7.2),
            (0.0, 0.0, 2.1),
            energy=880.0,
            size=4.2,
            color=(1.0, 0.91, 0.78),
        ),
        "fill": add_area_light(
            collection,
            "IGGY_CathedralStone_Fill",
            (5.4, -2.7, 4.1),
            (0.0, 0.0, 2.0),
            energy=370.0,
            size=3.8,
            color=(0.68, 0.79, 1.0),
        ),
        "rim": add_area_light(
            collection,
            "IGGY_CathedralStone_Rim",
            (-3.5, 2.5, 5.8),
            (0.0, 0.0, 2.4),
            energy=460.0,
            size=3.0,
            color=(0.78, 0.88, 1.0),
        ),
    }
    world = scene.world or bpy.data.worlds.new("IGGY_CathedralStoneWorld")
    scene.world = world
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.055, 0.060, 0.067, 1.0)
    background.inputs["Strength"].default_value = 0.30
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.look = "AgX - Medium High Contrast"
    return cameras, lights


def set_material_mode(
    material: bpy.types.Material,
    output_name: str,
) -> None:
    tree = material.node_tree
    system = tree.nodes["Cathedral_Stone_System"]
    principled = tree.nodes["Cathedral_Stone_BSDF"]
    base = principled.inputs["Base Color"]
    for existing in list(base.links):
        tree.links.remove(existing)
    tree.links.new(system.outputs[output_name], base)
    normal = principled.inputs["Normal"]
    for existing in list(normal.links):
        tree.links.remove(existing)
    if output_name == "Combined Color":
        tree.links.new(
            tree.nodes["Metre_Scale_Stone_Height"].outputs["Normal"],
            normal,
        )
    principled.inputs["Roughness"].default_value = (
        0.62 if output_name == "Combined Color" else 1.0
    )


def render_still(
    scene: bpy.types.Scene,
    camera: bpy.types.Object,
    path: Path,
    width: int,
    height: int,
) -> dict[str, Any]:
    scene.camera = camera
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def render_proofs(
    scene: bpy.types.Scene,
    material: bpy.types.Material,
    cameras: dict[str, bpy.types.Object],
    lights: dict[str, bpy.types.Object],
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    renders: dict[str, Any] = {}
    set_material_mode(material, "Combined Color")
    renders["hero"] = render_still(
        scene,
        cameras["hero"],
        output_root / "cathedral_stone_v1_surface_hero.png",
        args.render_width,
        args.render_height,
    )
    renders["close"] = render_still(
        scene,
        cameras["close"],
        output_root / "cathedral_stone_v1_surface_close.png",
        args.render_width,
        args.render_height,
    )
    renders["body_detail"] = render_still(
        scene,
        cameras["surface_detail"],
        output_root / "cathedral_stone_v1_surface_body_detail.png",
        args.render_width,
        args.render_height,
    )
    lights["key"].location = (-6.8, -1.8, 3.2)
    point_object(lights["key"], (0.0, 0.0, 2.0))
    lights["key"].data.energy = 1040.0
    lights["fill"].data.energy = 180.0
    renders["grazing"] = render_still(
        scene,
        cameras["grazing"],
        output_root / "cathedral_stone_v1_surface_grazing.png",
        args.render_width,
        args.render_height,
    )
    lights["key"].location = (-5.8, -4.6, 7.2)
    point_object(lights["key"], (0.0, 0.0, 2.1))
    lights["key"].data.energy = 880.0
    lights["fill"].data.energy = 370.0
    renders["distance"] = render_still(
        scene,
        cameras["distance"],
        output_root / "cathedral_stone_v1_surface_distance.png",
        args.render_width,
        args.render_height,
    )
    set_material_mode(material, "Stone Body Proof")
    renders["body_masks"] = render_still(
        scene,
        cameras["surface_detail"],
        output_root / "cathedral_stone_v1_surface_body_masks.png",
        args.render_width,
        args.render_height,
    )
    set_material_mode(material, "Tooling Proof")
    renders["tooling_masks"] = render_still(
        scene,
        cameras["surface_detail"],
        output_root / "cathedral_stone_v1_surface_tooling_masks.png",
        args.render_width,
        args.render_height,
    )
    set_material_mode(material, "Combined Color")
    return renders


def render_measured_geometry_proofs(
    scene: bpy.types.Scene,
    cameras: dict[str, bpy.types.Object],
    lights: dict[str, bpy.types.Object],
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    renders: dict[str, Any] = {}
    renders["hero"] = render_still(
        scene,
        cameras["hero"],
        output_root / "cathedral_stone_v1_hero.png",
        args.render_width,
        args.render_height,
    )
    renders["close"] = render_still(
        scene,
        cameras["close"],
        output_root / "cathedral_stone_v1_measured_close.png",
        args.render_width,
        args.render_height,
    )
    lights["rim"].data.energy = 980.0
    renders["depth"] = render_still(
        scene,
        cameras["depth"],
        output_root / "cathedral_stone_v1_depths.png",
        args.render_width,
        args.render_height,
    )
    lights["rim"].data.energy = 460.0
    lights["key"].location = (-5.8, -1.15, 3.1)
    point_object(lights["key"], (0.0, 0.0, 2.0))
    lights["key"].data.energy = 1120.0
    lights["fill"].data.energy = 150.0
    renders["grazing"] = render_still(
        scene,
        cameras["grazing"],
        output_root / "cathedral_stone_v1_grazing.png",
        args.render_width,
        args.render_height,
    )
    lights["key"].location = (-5.8, -4.6, 7.2)
    point_object(lights["key"], (0.0, 0.0, 2.1))
    lights["key"].data.energy = 880.0
    lights["fill"].data.energy = 370.0
    renders["distance"] = render_still(
        scene,
        cameras["distance"],
        output_root / "cathedral_stone_v1_distance.png",
        args.render_width,
        args.render_height,
    )
    return renders


def validate_contract(
    profile: dict[str, Any],
    recipe: Any,
    block_objects: list[bpy.types.Object],
    mortar_objects: list[bpy.types.Object],
    measurement_metrics: dict[str, Any],
) -> dict[str, Any]:
    for obj in block_objects:
        missing = [
            name
            for name in ATTRIBUTE_NAMES
            if obj.data.attributes.get(name) is None
        ]
        if missing:
            raise AssertionError(f"{obj.name} missing attributes: {missing}")
        if len(obj.data.polygons) != 6:
            raise AssertionError(f"{obj.name} is not a complete six-face volume")
        if obj.modifiers:
            raise AssertionError(
                f"{obj.name} invents an unmeasured bevel or modifier"
            )
        measured = (
            float(obj["iggy_stone_face_length_m"]),
            float(obj["iggy_stone_depth_m"]),
            float(obj["iggy_stone_face_height_m"]),
        )
        actual = (float(obj.dimensions.x), float(obj.dimensions.y), float(obj.dimensions.z))
        if any(
            not math.isclose(a, b, abs_tol=1.0e-6)
            for a, b in zip(measured, actual)
        ):
            raise AssertionError(
                f"{obj.name} dimensions do not match its measurement attributes"
            )
        if not 0.78 <= measured[0] <= 1.13:
            raise AssertionError(f"{obj.name} length exceeds surveyed range")
        if not 0.17 <= measured[1] <= 0.30:
            raise AssertionError(f"{obj.name} depth exceeds surveyed range")
        if not 0.32 <= measured[2] <= 0.43:
            raise AssertionError(f"{obj.name} height exceeds surveyed range")
        tool_spacing = float(obj["iggy_tool_spacing_m"])
        tool_angle = float(obj["iggy_tool_angle_deg"])
        tool_density_variant = int(obj["iggy_tool_density_variant"])
        if not 0.001 <= tool_spacing <= 0.004:
            raise AssertionError(f"{obj.name} tooling spacing exceeds measured range")
        if not 35.0 <= abs(tool_angle) <= 55.0:
            raise AssertionError(f"{obj.name} tooling angle is not diagonal")
        if tool_density_variant not in (0, 1, 2):
            raise AssertionError(f"{obj.name} tooling density variant is invalid")
        for name in (
            "iggy_fracture_interior",
            "iggy_lichen_mask",
            "iggy_damp_mask",
            "iggy_traversal_id",
        ):
            if any(
                float(entry.value) != 0.0
                for entry in obj.data.attributes[name].data
            ):
                raise AssertionError(f"{name} must default off on {obj.name}")
    if len({int(obj["iggy_source_design_index"]) for obj in block_objects}) != 40:
        raise AssertionError("The proof must exercise all forty block designs")
    if not mortar_objects:
        raise AssertionError("The measured joint bodies are missing")
    if float(recipe.mortar["recess_m"]) != 0.0:
        raise AssertionError("The intact ashlar base must remain flush")
    if not math.isclose(
        measurement_metrics["bed_joint_m"], 0.003, abs_tol=1.0e-9
    ) or not math.isclose(
        measurement_metrics["perpend_joint_m"], 0.003, abs_tol=1.0e-9
    ):
        raise AssertionError("The working joint width changed without authority")
    if any(
        float(profile["default_overlays"][name]) != 0.0
        for name in ("fracture", "lichen", "damp", "soot")
    ):
        raise AssertionError("Damage overlays must default off")
    return {
        "construction_representation": "individual six-face stone volumes",
        "physical_block_count": len(block_objects),
        "source_design_count": 40,
        "course_count": len(recipe.courses),
        "mortar_body_count": len(mortar_objects),
        "measurement_metrics": measurement_metrics,
        "joint_profile": recipe.mortar["profile"],
        "unmeasured_arris_radius_authored": False,
        "unmeasured_tool_depth_authored": False,
        "finite_tooling_linework_authored": True,
        "tooling_detail_span_m": 0.256,
        "excluded_from_intact_core": list(recipe.excluded_from_intact_core),
        "damage_overlays_default_off": True,
        "geometry_attributes": list(ATTRIBUTE_NAMES),
    }


def validate_runtime_material(
    group: bpy.types.NodeTree,
    material: bpy.types.Material,
) -> dict[str, Any]:
    required_nodes = {
        "IGGY_WorldXZTileCoordinate",
        "IGGY_ObjectCoordinateAnchor",
        "IGGY_64mmBodyDetailCoordinate",
        "IGGY_91mmBodyDetailCoordinate",
        "IGGY_PerBlockPhase",
        "IGGY_PerBlockVariant",
        "IGGY_VariantQuarterTurn",
        "IGGY_VariantMirrorSign",
        "IGGY_StoneBodyMasks",
        "IGGY_StoneBodyMasksDecorrelated",
        "IGGY_Blend64mmAnd91mmBodyMasks",
        "IGGY_StoneBodyHeight",
        "IGGY_StoneBodyHeightDecorrelated",
        "IGGY_Blend64mmAnd91mmBodyHeight",
        "IGGY_StoneBodyHeightValue",
        "IGGY_256mmToolingCoordinate",
        "IGGY_ToolAngleDegrees",
        "IGGY_ToolDensityVariant",
        "IGGY_ToolingDetail",
        "IGGY_SelectToolingDensityVariant",
        "IGGY_ToolingColorLinework",
        "IGGY_ToolingRoughnessLinework",
        "IGGY_CalcareousFossilColor",
        "IGGY_CoolSilicateColor",
        "IGGY_VisiblePoreColor",
        "IGGY_RoughnessPlusVisiblePore",
        "IGGY_SeparateStylizationLanes",
        "IGGY_CameraDistance",
        "IGGY_DetailFullAt030mZeroAt3p5m",
        "IGGY_DistanceFadedStoneBodyHeight",
        "IGGY_DistanceFadedTooling",
        "IGGY_SelectiveStructuralInk",
        "IGGY_SelectiveArrisHighlight",
    }
    missing = sorted(required_nodes - set(group.nodes.keys()))
    if missing:
        raise AssertionError(f"Runtime stone material is missing nodes: {missing}")
    coordinate_node = group.nodes["IGGY_ObjectCoordinateAnchor"]
    if (
        coordinate_node.bl_idname != "ShaderNodeTexCoord"
        or not any(
            link.from_node == coordinate_node
            and link.from_socket.name == "Object"
            and link.to_node.name == "IGGY_WorldXZMetres"
            for link in group.links
        )
    ):
        raise AssertionError("Cathedral stone must use stable object-space metres")
    camera_nodes = [
        node
        for node in group.nodes
        if node.bl_idname == "ShaderNodeCameraData"
    ]
    forbidden_noise = [
        node
        for node in group.nodes
        if node.bl_idname in {"ShaderNodeTexNoise", "ShaderNodeTexVoronoi"}
    ]
    if len(camera_nodes) != 1 or forbidden_noise:
        raise AssertionError("Distance hierarchy or authored-map contract drifted")
    if material.get("iggy_material_schema") != (
        "iggy3d.material.cathedral_stone_v1.v6"
    ):
        raise AssertionError("Runtime stone material schema is stale")
    if not math.isclose(
        float(material.get("iggy_stone_body_detail_span_m", 0.0)),
        0.064,
        abs_tol=1.0e-9,
    ):
        raise AssertionError("Runtime stone body detail scale is not 64 mm")
    if not math.isclose(
        float(material.get("iggy_tooling_detail_span_m", 0.0)),
        0.256,
        abs_tol=1.0e-9,
    ):
        raise AssertionError("Runtime tooling detail scale is not 256 mm")
    if not math.isclose(
        float(material.get("iggy_detail_full_distance_m", 0.0)),
        0.30,
        abs_tol=1.0e-9,
    ) or not math.isclose(
        float(material.get("iggy_detail_zero_distance_m", 0.0)),
        3.5,
        abs_tol=1.0e-9,
    ):
        raise AssertionError("Stone detail distance hierarchy drifted")
    if not bool(material.get("iggy_stylization_lanes_live")):
        raise AssertionError("Authored ink and highlight lanes are not live")
    principled = material.node_tree.nodes["Cathedral_Stone_BSDF"]
    if float(principled.inputs["Metallic"].default_value) != 0.0:
        raise AssertionError("Architectural stone must remain non-metallic")
    bump = material.node_tree.nodes["Metre_Scale_Stone_Height"]
    if not math.isclose(
        float(bump.inputs["Strength"].default_value),
        1.0,
        abs_tol=1.0e-9,
    ) or not math.isclose(
        float(bump.inputs["Distance"].default_value),
        0.00113,
        abs_tol=1.0e-9,
    ):
        raise AssertionError("Measured stone-body relief amplitude is not preserved")
    if material.get("iggy_relief_proxy_measurement_id") != (
        "sabucina_calcarenite_surface_topography_proxy"
    ):
        raise AssertionError("Runtime relief proxy provenance is missing")
    for name in (
        "iggy_unmeasured_block_plane_depth_authored",
        "iggy_unmeasured_tool_depth_authored",
        "iggy_unmeasured_arris_depth_authored",
        "iggy_unmeasured_mortar_depth_authored",
        "iggy_unmeasured_damage_depth_authored",
    ):
        if bool(material.get(name)):
            raise AssertionError(f"Unsupported relief lane enabled: {name}")
    body_texture = group.nodes["IGGY_StoneBodyMasks"].image
    if body_texture is None or not body_texture.packed_file:
        raise AssertionError("Stone body masks are not packed into the Blender asset")
    body_height_texture = group.nodes["IGGY_StoneBodyHeight"].image
    if (
        body_height_texture is None
        or not body_height_texture.packed_file
    ):
        raise AssertionError("Stone body height is not packed into the Blender asset")
    tooling_texture = group.nodes["IGGY_ToolingDetail"].image
    if tooling_texture is None or not tooling_texture.packed_file:
        raise AssertionError("Finite tooling masks are not packed into the Blender asset")
    stylization_texture = group.nodes["IGGY_Stylization"].image
    if stylization_texture is None or not stylization_texture.packed_file:
        raise AssertionError("Authored stylization lanes are not packed")
    return {
        "schema": material["iggy_material_schema"],
        "construction_macro_span_m": float(material["iggy_tile_size_m"]),
        "material_body_detail_span_m": float(
            material["iggy_stone_body_detail_span_m"]
        ),
        "material_body_decorrelation_span_m": 0.091,
        "tooling_detail_span_m": float(
            material["iggy_tooling_detail_span_m"]
        ),
        "per_block_phase": True,
        "quarter_turn_variation": True,
        "mirror_variation": True,
        "metallic": 0.0,
        "relief_proxy_measurement_id": material[
            "iggy_relief_proxy_measurement_id"
        ],
        "stone_body_relief_amplitude_m": float(
            material["iggy_stone_body_relief_amplitude_m"]
        ),
        "measured_stone_body_relief_authored": True,
        "unmeasured_relief_authored": False,
        "stone_body_masks_packed": True,
        "stone_body_height_packed": True,
        "tooling_detail_packed": True,
        "tooling_depth_authored": False,
        "coordinate_space": "object_position_xz_metres",
        "detail_full_distance_m": float(
            material["iggy_detail_full_distance_m"]
        ),
        "detail_zero_distance_m": float(
            material["iggy_detail_zero_distance_m"]
        ),
        "stylization_lanes_live": True,
        "camera_distance_count": len(camera_nodes),
        "generic_noise_count": len(forbidden_noise),
        "required_node_count": len(required_nodes),
    }


def main() -> None:
    args = parse_args()
    profile = load_profile()
    generator = load_generator()
    args.output_root.mkdir(parents=True, exist_ok=True)
    material_data = generator.generate_material(
        resolution=args.texture_resolution,
        seed=823451,
        pattern_variation=0,
    )
    texture_manifest = generator.write_material_package(
        material_data,
        output_root=args.output_root,
        pattern_path=generator.DEFAULT_PATTERN,
        profile_path=PROFILE_PATH,
    )

    bpy.ops.wm.read_factory_settings(use_empty=True)
    collection = ensure_collection()
    runtime_group = create_material_group(args.output_root)
    runtime_material = configure_material(runtime_group)
    recipe = generator.load_ashlar_recipe()
    stone_materials = create_measured_stone_materials()
    mortar_material = flat_material(
        "IGGY_MAT_MeasuredAshlarMortar",
        (0.38, 0.35, 0.30, 1.0),
        roughness=0.82,
    )
    block_objects, mortar_objects, measurement_metrics = (
        create_measured_ashlar_wall(
            collection,
            recipe,
            stone_materials,
            mortar_material,
        )
    )
    floor_material = flat_material(
        "IGGY_MAT_CathedralProofFloor",
        (0.035, 0.043, 0.052, 1.0),
        roughness=0.86,
    )
    create_box(
        collection,
        "IGGY_CathedralProofFloor",
        (-5.5, 5.5, -3.0, 3.0, -0.14, -0.04),
        floor_material,
        bevel_width=0.0,
    )
    cameras, lights = configure_scene(collection)
    geometry_renders = render_measured_geometry_proofs(
        bpy.context.scene,
        cameras,
        lights,
        args.output_root,
        args,
    )
    for obj in block_objects:
        obj.data.materials[0] = runtime_material
    surface_renders = render_proofs(
        bpy.context.scene,
        runtime_material,
        cameras,
        lights,
        args.output_root,
        args,
    )
    validation = validate_contract(
        profile,
        recipe,
        block_objects,
        mortar_objects,
        measurement_metrics,
    )
    validation["runtime_material"] = validate_runtime_material(
        runtime_group,
        runtime_material,
    )
    blend_path = args.output_root / "cathedral_stone_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    manifest = {
        "schema": "iggy3d.material.cathedral_stone_v1.blender.v6",
        "output_blend": {
            "path": str(blend_path),
            "bytes": blend_path.stat().st_size,
            "sha256": sha256_file(blend_path),
        },
        "materials": {
            "stone_geometry_proof": [
                material.name for material in stone_materials
            ],
            "mortar_geometry_proof": mortar_material.name,
            "runtime_surface": runtime_material.name,
            "runtime_surface_group": runtime_group.name,
            "runtime_surface_status": (
                "assigned only after the neutral geometry proof was rendered; "
                "uses independent four-metre macro and 64 mm/91 mm body-detail "
                "coordinates with a measured comparable-calcarenite relief proxy, "
                "plus per-block 256 mm finite-impact tooling linework"
            ),
        },
        "texture_manifest": texture_manifest,
        "renders": {
            "neutral_geometry": geometry_renders,
            "layered_surface": surface_renders,
        },
        "validation": validation,
        "constraints": {
            "uses_object_space_metre_coordinates": True,
            "uses_twenty_shade_block_families": True,
            "uses_authored_graphic_plane_values": True,
            "uses_live_ink_and_highlight_lanes": True,
            "uses_distance_faded_body_and_tooling_detail": True,
            "unmeasured_arris_radius_authored": False,
            "unmeasured_tool_depth_authored": False,
            "unreal_runtime_parity_verified": False,
        },
        "source_policy": {
            "ai_generated_reference_capture": False,
            "photogrammetry_runtime_texture": False,
            "raw_reference_pixels_used_as_runtime_texture": False,
            "licensed_facade_color_capture": True,
            "facade_capture_license": "CC BY-SA 4.0",
            "facade_capture_attribution": (
                "Benjamin Smith / Wikimedia Commons"
            ),
            "dimension_measurement_authority": (
                recipe.measurement_authority
            ),
            "unmeasured_wall_core_inferred": False,
            "unmeasured_arris_radius_authored": False,
            "unmeasured_tool_depth_authored": False,
        },
    }
    manifest_path = (
        args.output_root / "cathedral_stone_v1_blender_manifest.json"
    )
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    )
    print(
        "cathedral_stone_v1 built:",
        len(block_objects),
        "full-volume measured blocks,",
        len(mortar_objects),
        "joint bodies,",
        len(geometry_renders) + len(surface_renders),
        "neutral and surface renders",
    )


if __name__ == "__main__":
    main()
