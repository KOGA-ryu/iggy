#!/usr/bin/env python3
"""Build and prove the measured Old Sarum chevron-voussoir portal order."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import sys
from typing import Any, Iterable

import bmesh
import bpy
from mathutils import Vector


SCRIPT_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "cathedral_stone_trim_fracture_v1.json"
PATTERN_PATH = SCRIPT_ROOT / "patterns" / "cathedral_trim_fracture_atlas_v1.json"
GENERATOR_PATH = SCRIPT_ROOT / "generate_cathedral_stone_trim_fracture_v1.py"
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"
GROUP_NAME = "IGGY_SH_ChevronVoussoir_v002"
MATERIAL_NAME = "IGGY_MAT_ChevronVoussoir_v002"
CLAY_MATERIAL_NAME = "IGGY_MAT_ChevronVoussoir_ClayProof"
MOULDING_PROOF_MATERIAL_NAME = "IGGY_MAT_ChevronMoulding_Proof"
IDENTITY_PROOF_MATERIAL_NAME = "IGGY_MAT_ChevronIdentity_Proof"
WIREFRAME_PROOF_MATERIAL_NAME = "IGGY_MAT_ChevronWireframe_Proof"
PRODUCT_COLLECTION_NAME = "IGGY_ChevronVoussoirPortal_Product"
PROOF_COLLECTION_NAME = "IGGY_ChevronVoussoirPortal_Proof"
PROOF_IDS = (
    "neutral_clay_front",
    "neutral_clay_grazing",
    "live_material_front",
    "live_material_grazing",
    "measured_close",
    "distance_read",
    "moulding_proof",
    "identity_proof",
    "wireframe_proof",
)
BLENDER_MANIFEST_SCHEMA = (
    "iggy3d.material.cathedral_stone_trim_fracture_v1.blender_manifest.v2"
)


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_generator() -> Any:
    spec = importlib.util.spec_from_file_location(
        "iggy_chevron_texture_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("Cannot load chevron texture generator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def ensure_texture_family(output: Path) -> dict[str, Any]:
    generator = load_generator()
    return generator.generate(PROFILE_PATH, PATTERN_PATH, output)


def reset_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for block in (
        bpy.data.meshes,
        bpy.data.curves,
        bpy.data.cameras,
        bpy.data.lights,
        bpy.data.materials,
        bpy.data.node_groups,
    ):
        for item in list(block):
            block.remove(item, do_unlink=True)
    for collection in list(bpy.data.collections):
        if collection.name != bpy.context.scene.collection.name:
            bpy.data.collections.remove(collection)


def collection(name: str) -> bpy.types.Collection:
    result = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(result)
    return result


def add_node(
    tree: bpy.types.NodeTree,
    bl_idname: str,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = tree.nodes.new(bl_idname)
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
    tree.links.new(
        source.outputs[source_socket],
        target.inputs[target_socket],
    )


def math_node(
    tree: bpy.types.NodeTree,
    name: str,
    operation: str,
    location: tuple[float, float],
    *,
    value_1: float | None = None,
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMath", name, location)
    node.operation = operation
    if value_1 is not None:
        node.inputs[1].default_value = value_1
    return node


def vector_scale_node(
    tree: bpy.types.NodeTree,
    name: str,
    scale: float,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeVectorMath", name, location)
    node.operation = "SCALE"
    node.inputs["Scale"].default_value = scale
    return node


def mix_color_node(
    tree: bpy.types.NodeTree,
    name: str,
    color: tuple[float, float, float, float],
    location: tuple[float, float],
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMixRGB", name, location)
    node.blend_type = "MIX"
    node.inputs["Color2"].default_value = color
    return node


def srgb_channel(value: float) -> float:
    value = max(0.0, min(1.0, value))
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def linear_rgba(
    red: int,
    green: int,
    blue: int,
    alpha: float = 1.0,
) -> tuple[float, float, float, float]:
    return (
        srgb_channel(red / 255.0),
        srgb_channel(green / 255.0),
        srgb_channel(blue / 255.0),
        alpha,
    )


def load_image(path: Path, *, non_color: bool) -> bpy.types.Image:
    if not path.is_file():
        raise FileNotFoundError(path)
    image = bpy.data.images.load(str(path), check_existing=True)
    image.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    image.pack()
    return image


def create_material_group(output: Path) -> bpy.types.NodeTree:
    group = bpy.data.node_groups.new(GROUP_NAME, "ShaderNodeTree")
    group.color_tag = "SHADER"
    group.description = (
        "Measured chevron-voussoir shading: metre-stable local UVs, broad "
        "twenty-shade calcarenite, two-scale body anatomy, live roll/hollow "
        "identity, and distance-gated microdetail."
    )
    for name, socket_type in (
        ("Combined Color", "NodeSocketColor"),
        ("Combined Roughness", "NodeSocketFloat"),
        ("Combined Normal", "NodeSocketVector"),
        ("Moulding Proof", "NodeSocketColor"),
        ("Identity Proof", "NodeSocketColor"),
        ("Detail Distance Fade", "NodeSocketFloat"),
    ):
        group.interface.new_socket(
            name=name,
            in_out="OUTPUT",
            socket_type=socket_type,
        )
    output_node = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (2480, 120),
    )

    uv_a = add_node(
        group,
        "ShaderNodeUVMap",
        "IGGY_StoneUV_A",
        (-2420, 680),
    )
    uv_a.uv_map = "IGGY_StoneUV_A"
    uv_b = add_node(
        group,
        "ShaderNodeUVMap",
        "IGGY_StoneUV_B",
        (-2420, 300),
    )
    uv_b.uv_map = "IGGY_StoneUV_B"
    base_coordinate = vector_scale_node(
        group,
        "IGGY_BaseCoordinate_512mm",
        1.0 / 0.512,
        (-2190, 720),
    )
    primary_coordinate = vector_scale_node(
        group,
        "IGGY_PrimaryCoordinate_64mm",
        1.0 / 0.064,
        (-2190, 520),
    )
    secondary_coordinate = vector_scale_node(
        group,
        "IGGY_SecondaryCoordinate_91mm",
        1.0 / 0.091,
        (-2190, 280),
    )
    link(group, uv_a, "UV", base_coordinate, "Vector")
    link(group, uv_a, "UV", primary_coordinate, "Vector")
    link(group, uv_b, "UV", secondary_coordinate, "Vector")

    image_specs = (
        (
            "IGGY_TrimBaseColor",
            "cathedral_stone_trim_fracture_v1_basecolor.png",
            False,
            base_coordinate,
            900,
        ),
        (
            "IGGY_TrimORM",
            "cathedral_stone_trim_fracture_v1_orm.png",
            True,
            base_coordinate,
            670,
        ),
        (
            "IGGY_BodyMasks64",
            "cathedral_stone_trim_fracture_v1_body_masks.png",
            True,
            primary_coordinate,
            420,
        ),
        (
            "IGGY_BodyMasks91",
            "cathedral_stone_trim_fracture_v1_body_masks.png",
            True,
            secondary_coordinate,
            190,
        ),
        (
            "IGGY_BodyNormal64",
            "cathedral_stone_trim_fracture_v1_body_normal.png",
            True,
            primary_coordinate,
            -60,
        ),
        (
            "IGGY_BodyHeight91",
            "cathedral_stone_trim_fracture_v1_body_height.png",
            True,
            secondary_coordinate,
            -300,
        ),
    )
    images: dict[str, bpy.types.Node] = {}
    for name, filename, non_color, coordinate, y in image_specs:
        node = add_node(group, "ShaderNodeTexImage", name, (-1930, y))
        node.image = load_image(output / filename, non_color=non_color)
        node.extension = "REPEAT"
        node.interpolation = "Linear"
        link(group, coordinate, "Vector", node, "Vector")
        images[name] = node

    orm = add_node(
        group,
        "ShaderNodeSeparateColor",
        "IGGY_SeparateORM",
        (-1680, 670),
    )
    orm.mode = "RGB"
    link(group, images["IGGY_TrimORM"], "Color", orm, "Color")
    masks_blend = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_Blend64mmAnd91mmBodyMasks",
        (-1680, 320),
    )
    masks_blend.blend_type = "MIX"
    masks_blend.inputs["Factor"].default_value = 0.34
    link(group, images["IGGY_BodyMasks64"], "Color", masks_blend, "Color1")
    link(group, images["IGGY_BodyMasks91"], "Color", masks_blend, "Color2")
    masks = add_node(
        group,
        "ShaderNodeSeparateColor",
        "IGGY_SeparateBodyMasks",
        (-1440, 320),
    )
    masks.mode = "RGB"
    link(group, masks_blend, "Color", masks, "Color")

    camera = add_node(
        group,
        "ShaderNodeCameraData",
        "IGGY_CameraDistance",
        (-1680, -580),
    )
    detail_fade = add_node(
        group,
        "ShaderNodeMapRange",
        "IGGY_DetailFullAt030mZeroAt3p5m",
        (-1440, -580),
    )
    detail_fade.clamp = True
    detail_fade.interpolation_type = "SMOOTHERSTEP"
    detail_fade.inputs["From Min"].default_value = 0.3
    detail_fade.inputs["From Max"].default_value = 3.5
    detail_fade.inputs["To Min"].default_value = 1.0
    detail_fade.inputs["To Max"].default_value = 0.0
    link(group, camera, "View Distance", detail_fade, "Value")
    line_fade = add_node(
        group,
        "ShaderNodeMapRange",
        "IGGY_LineFullAt030mZeroAt6m",
        (-1440, -730),
    )
    line_fade.clamp = True
    line_fade.interpolation_type = "SMOOTHERSTEP"
    line_fade.inputs["From Min"].default_value = 0.3
    line_fade.inputs["From Max"].default_value = 6.0
    line_fade.inputs["To Min"].default_value = 1.0
    line_fade.inputs["To Max"].default_value = 0.0
    link(group, camera, "View Distance", line_fade, "Value")

    attributes: dict[str, bpy.types.Node] = {}
    attribute_specs = (
        ("roll", "iggy_chevron_roll", -1180, 80),
        ("hollow", "iggy_chevron_hollow", -1180, -50),
        ("quiet", "iggy_chevron_quiet", -1180, -180),
        ("ink", "iggy_chevron_ink", -1180, -310),
        ("highlight", "iggy_chevron_highlight", -1180, -440),
        ("front", "iggy_chevron_front", -1180, -570),
        ("carved", "iggy_carved_trim", -1180, -700),
        ("variant", "iggy_material_variant", -1180, -830),
        ("stone_id", "iggy_voussoir_id", -1180, -960),
        ("phase", "iggy_material_phase", -1180, -1090),
    )
    for key, attribute_name, x, y in attribute_specs:
        node = add_node(
            group,
            "ShaderNodeAttribute",
            f"IGGY_Attribute_{attribute_name}",
            (x, y),
        )
        node.attribute_name = attribute_name
        attributes[key] = node

    front_carved = math_node(
        group,
        "IGGY_FrontTimesCarved",
        "MULTIPLY",
        (-900, -620),
    )
    link(group, attributes["front"], "Factor", front_carved, "Value")
    link(group, attributes["carved"], "Factor", front_carved, "Value_001")

    live_masks: dict[str, bpy.types.Node] = {}
    for index, key in enumerate(("roll", "hollow", "quiet", "ink", "highlight")):
        node = math_node(
            group,
            f"IGGY_Live{key.title()}Mask",
            "MULTIPLY",
            (-680, -100 - index * 130),
        )
        link(group, attributes[key], "Factor", node, "Value")
        link(group, front_carved, "Value", node, "Value_001")
        live_masks[key] = node

    parity = math_node(
        group,
        "IGGY_VariantParity",
        "MODULO",
        (-900, 920),
        value_1=2.0,
    )
    link(group, attributes["variant"], "Factor", parity, "Value")
    warm_factor = math_node(
        group,
        "IGGY_WarmVariantFactor",
        "MULTIPLY",
        (-680, 920),
        value_1=0.07,
    )
    link(group, parity, "Value", warm_factor, "Value")
    high_variant = math_node(
        group,
        "IGGY_HighVariant",
        "GREATER_THAN",
        (-900, 790),
        value_1=1.5,
    )
    link(group, attributes["variant"], "Factor", high_variant, "Value")
    cool_factor = math_node(
        group,
        "IGGY_CoolVariantFactor",
        "MULTIPLY",
        (-680, 790),
        value_1=0.055,
    )
    link(group, high_variant, "Value", cool_factor, "Value")

    warm_mix = mix_color_node(
        group,
        "IGGY_PerStoneWarmWash",
        linear_rgba(211, 172, 118),
        (-410, 920),
    )
    link(group, images["IGGY_TrimBaseColor"], "Color", warm_mix, "Color1")
    link(group, warm_factor, "Value", warm_mix, "Factor")
    cool_mix = mix_color_node(
        group,
        "IGGY_PerStoneCoolWash",
        linear_rgba(157, 158, 149),
        (-170, 920),
    )
    link(group, warm_mix, "Color", cool_mix, "Color1")
    link(group, cool_factor, "Value", cool_mix, "Factor")

    color_factors: dict[str, bpy.types.Node] = {}
    for index, (key, channel, amount) in enumerate(
        (
            ("fossil", "Red", 0.13),
            ("silicate", "Green", 0.09),
            ("pore", "Blue", 0.17),
        )
    ):
        faded = math_node(
            group,
            f"IGGY_{key.title()}TimesDetailFade",
            "MULTIPLY",
            (-1130 + index * 40, 570 - index * 120),
        )
        link(group, masks, channel, faded, "Value")
        link(group, detail_fade, "Result", faded, "Value_001")
        amount_node = math_node(
            group,
            f"IGGY_{key.title()}ColorAmount",
            "MULTIPLY",
            (-890 + index * 40, 570 - index * 120),
            value_1=amount,
        )
        link(group, faded, "Value", amount_node, "Value")
        color_factors[key] = amount_node

    fossil_mix = mix_color_node(
        group,
        "IGGY_FossilFragmentColor",
        linear_rgba(224, 202, 166),
        (80, 810),
    )
    link(group, cool_mix, "Color", fossil_mix, "Color1")
    link(group, color_factors["fossil"], "Value", fossil_mix, "Factor")
    silicate_mix = mix_color_node(
        group,
        "IGGY_SilicateGrainColor",
        linear_rgba(150, 153, 146),
        (310, 760),
    )
    link(group, fossil_mix, "Color", silicate_mix, "Color1")
    link(group, color_factors["silicate"], "Value", silicate_mix, "Factor")
    pore_mix = mix_color_node(
        group,
        "IGGY_PoreIdentityColor",
        linear_rgba(111, 94, 76),
        (540, 710),
    )
    link(group, silicate_mix, "Color", pore_mix, "Color1")
    link(group, color_factors["pore"], "Value", pore_mix, "Factor")

    moulding_specs = (
        (
            "roll",
            "IGGY_RollWarmth",
            linear_rgba(218, 186, 139),
            0.16,
        ),
        (
            "hollow",
            "IGGY_HollowCoolness",
            linear_rgba(123, 113, 102),
            0.22,
        ),
    )
    previous_color: bpy.types.Node = pore_mix
    for index, (key, name, tint, amount) in enumerate(moulding_specs):
        factor = math_node(
            group,
            f"{name}Factor",
            "MULTIPLY",
            (610, 420 - index * 130),
            value_1=amount,
        )
        link(group, live_masks[key], "Value", factor, "Value")
        mix_node = mix_color_node(
            group,
            name,
            tint,
            (840, 570 - index * 70),
        )
        link(group, previous_color, "Color", mix_node, "Color1")
        link(group, factor, "Value", mix_node, "Factor")
        previous_color = mix_node

    ink_line = math_node(
        group,
        "IGGY_InkTimesLineFade",
        "MULTIPLY",
        (800, 190),
    )
    link(group, live_masks["ink"], "Value", ink_line, "Value")
    link(group, line_fade, "Result", ink_line, "Value_001")
    ink_amount = math_node(
        group,
        "IGGY_SelectiveInkAmount",
        "MULTIPLY",
        (1020, 190),
        value_1=0.30,
    )
    link(group, ink_line, "Value", ink_amount, "Value")
    ink_mix = mix_color_node(
        group,
        "IGGY_SelectiveChevronInk",
        linear_rgba(88, 76, 65),
        (1240, 500),
    )
    link(group, previous_color, "Color", ink_mix, "Color1")
    link(group, ink_amount, "Value", ink_mix, "Factor")

    highlight_line = math_node(
        group,
        "IGGY_HighlightTimesLineFade",
        "MULTIPLY",
        (1020, 40),
    )
    link(group, live_masks["highlight"], "Value", highlight_line, "Value")
    link(group, line_fade, "Result", highlight_line, "Value_001")
    highlight_amount = math_node(
        group,
        "IGGY_SelectiveHighlightAmount",
        "MULTIPLY",
        (1240, 40),
        value_1=0.14,
    )
    link(group, highlight_line, "Value", highlight_amount, "Value")
    highlight_mix = mix_color_node(
        group,
        "IGGY_SelectiveCrestHighlight",
        linear_rgba(231, 211, 176),
        (1470, 500),
    )
    link(group, ink_mix, "Color", highlight_mix, "Color1")
    link(group, highlight_amount, "Value", highlight_mix, "Factor")

    pore_rough = math_node(
        group,
        "IGGY_PoreRoughnessDelta",
        "MULTIPLY",
        (250, -20),
        value_1=0.09,
    )
    link(group, color_factors["pore"], "Value", pore_rough, "Value")
    silicate_rough = math_node(
        group,
        "IGGY_SilicateRoughnessDelta",
        "MULTIPLY",
        (250, -150),
        value_1=-0.04,
    )
    link(
        group,
        color_factors["silicate"],
        "Value",
        silicate_rough,
        "Value",
    )
    add_pore = math_node(
        group,
        "IGGY_AddPoreRoughness",
        "ADD",
        (500, -20),
    )
    link(group, orm, "Green", add_pore, "Value")
    link(group, pore_rough, "Value", add_pore, "Value_001")
    add_silicate = math_node(
        group,
        "IGGY_AddSilicateRoughness",
        "ADD",
        (720, -20),
    )
    link(group, add_pore, "Value", add_silicate, "Value")
    link(group, silicate_rough, "Value", add_silicate, "Value_001")
    roll_rough = math_node(
        group,
        "IGGY_RollRoughnessDelta",
        "MULTIPLY",
        (500, -250),
        value_1=-0.018,
    )
    link(group, live_masks["roll"], "Value", roll_rough, "Value")
    hollow_rough = math_node(
        group,
        "IGGY_HollowRoughnessDelta",
        "MULTIPLY",
        (500, -380),
        value_1=0.028,
    )
    link(group, live_masks["hollow"], "Value", hollow_rough, "Value")
    add_roll = math_node(
        group,
        "IGGY_AddRollRoughness",
        "ADD",
        (940, -20),
    )
    link(group, add_silicate, "Value", add_roll, "Value")
    link(group, roll_rough, "Value", add_roll, "Value_001")
    add_hollow = math_node(
        group,
        "IGGY_AddHollowRoughness",
        "ADD",
        (1160, -20),
    )
    link(group, add_roll, "Value", add_hollow, "Value")
    link(group, hollow_rough, "Value", add_hollow, "Value_001")
    rough_min = math_node(
        group,
        "IGGY_RoughnessMinimum",
        "MAXIMUM",
        (1380, -20),
        value_1=0.62,
    )
    link(group, add_hollow, "Value", rough_min, "Value")
    rough_max = math_node(
        group,
        "IGGY_RoughnessMaximum",
        "MINIMUM",
        (1600, -20),
        value_1=0.90,
    )
    link(group, rough_min, "Value", rough_max, "Value")

    normal_map = add_node(
        group,
        "ShaderNodeNormalMap",
        "IGGY_OpenGLBodyNormal64mm",
        (250, -560),
    )
    normal_map.space = "TANGENT"
    primary_normal_strength = math_node(
        group,
        "IGGY_PrimaryNormalStrength66Percent",
        "MULTIPLY",
        (0, -560),
        value_1=0.66,
    )
    link(
        group,
        detail_fade,
        "Result",
        primary_normal_strength,
        "Value",
    )
    link(
        group,
        primary_normal_strength,
        "Value",
        normal_map,
        "Strength",
    )
    link(group, images["IGGY_BodyNormal64"], "Color", normal_map, "Color")
    height_value = add_node(
        group,
        "ShaderNodeRGBToBW",
        "IGGY_BodyHeight91mmValue",
        (250, -760),
    )
    link(group, images["IGGY_BodyHeight91"], "Color", height_value, "Color")
    bump = add_node(
        group,
        "ShaderNodeBump",
        "IGGY_DecorrelatedBodyHeight91mm",
        (560, -650),
    )
    bump.invert = False
    bump.inputs["Distance"].default_value = 0.00113 * 0.34
    link(group, detail_fade, "Result", bump, "Strength")
    link(group, height_value, "Val", bump, "Height")
    link(group, normal_map, "Normal", bump, "Normal")

    moulding_proof = add_node(
        group,
        "ShaderNodeCombineColor",
        "IGGY_MouldingProofRGB",
        (1730, -300),
    )
    moulding_proof.mode = "RGB"
    link(group, live_masks["roll"], "Value", moulding_proof, "Red")
    link(group, live_masks["hollow"], "Value", moulding_proof, "Green")
    link(group, live_masks["quiet"], "Value", moulding_proof, "Blue")
    id_normalized = math_node(
        group,
        "IGGY_VoussoirIdNormalized",
        "DIVIDE",
        (1040, -1000),
        value_1=15.0,
    )
    link(group, attributes["stone_id"], "Factor", id_normalized, "Value")
    phase_length = add_node(
        group,
        "ShaderNodeVectorMath",
        "IGGY_PhaseVectorLength",
        (1040, -1130),
    )
    phase_length.operation = "LENGTH"
    link(group, attributes["phase"], "Vector", phase_length, "Vector")
    phase_normalized = add_node(
        group,
        "ShaderNodeMapRange",
        "IGGY_PhaseLengthNormalized",
        (1270, -1130),
    )
    phase_normalized.clamp = True
    phase_normalized.inputs["From Min"].default_value = 0.0
    phase_normalized.inputs["From Max"].default_value = 0.75
    phase_normalized.inputs["To Min"].default_value = 0.0
    phase_normalized.inputs["To Max"].default_value = 1.0
    link(group, phase_length, "Value", phase_normalized, "Value")
    variant_normalized = math_node(
        group,
        "IGGY_VariantNormalized",
        "DIVIDE",
        (1040, -1260),
        value_1=3.0,
    )
    link(
        group,
        attributes["variant"],
        "Factor",
        variant_normalized,
        "Value",
    )
    identity_proof = add_node(
        group,
        "ShaderNodeCombineColor",
        "IGGY_IdentityProofRGB",
        (1730, -1040),
    )
    identity_proof.mode = "RGB"
    link(group, id_normalized, "Value", identity_proof, "Red")
    link(group, phase_normalized, "Result", identity_proof, "Green")
    link(group, variant_normalized, "Value", identity_proof, "Blue")

    link(group, highlight_mix, "Color", output_node, "Combined Color")
    link(group, rough_max, "Value", output_node, "Combined Roughness")
    link(group, bump, "Normal", output_node, "Combined Normal")
    link(group, moulding_proof, "Color", output_node, "Moulding Proof")
    link(group, identity_proof, "Color", output_node, "Identity Proof")
    link(
        group,
        detail_fade,
        "Result",
        output_node,
        "Detail Distance Fade",
    )
    group["iggy_schema"] = (
        "iggy3d.shader.chevron_voussoir.measured_roll_hollow_roll.v2"
    )
    group["iggy_primary_body_span_m"] = 0.064
    group["iggy_secondary_body_span_m"] = 0.091
    group["iggy_body_relief_range_m"] = 0.00113
    group["iggy_damage_enabled"] = False
    return group


def configure_material(group: bpy.types.NodeTree) -> bpy.types.Material:
    material = bpy.data.materials.new(MATERIAL_NAME)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (580, 160),
    )
    principled = add_node(
        tree,
        "ShaderNodeBsdfPrincipled",
        "Chevron_Voussoir_BSDF",
        (300, 160),
    )
    system = add_node(
        tree,
        "ShaderNodeGroup",
        "Chevron_Voussoir_System",
        (-100, 220),
    )
    system.node_tree = group
    link(tree, system, "Combined Color", principled, "Base Color")
    link(tree, system, "Combined Roughness", principled, "Roughness")
    link(tree, system, "Combined Normal", principled, "Normal")
    principled.inputs["Metallic"].default_value = 0.0
    principled.inputs["IOR"].default_value = 1.46
    principled.inputs["Diffuse Roughness"].default_value = 0.16
    link(tree, principled, "BSDF", output, "Surface")
    material["iggy_schema"] = (
        "iggy3d.material.cathedral_stone_trim_fracture_v1.blender.v2"
    )
    material["iggy_measured_source"] = "S01_old_sarum_catalogue_item_55"
    material["iggy_fracture_enabled"] = False
    material["iggy_damage_enabled"] = False
    return material


def simple_material(
    name: str,
    color: tuple[float, float, float, float],
    roughness: float,
) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.diffuse_color = color
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    if principled is None:
        raise RuntimeError("Default Principled BSDF is missing")
    principled.inputs["Base Color"].default_value = color
    principled.inputs["Roughness"].default_value = roughness
    principled.inputs["Metallic"].default_value = 0.0
    return material


def group_proof_material(
    name: str,
    group: bpy.types.NodeTree,
    output_name: str,
) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (430, 80),
    )
    principled = add_node(
        tree,
        "ShaderNodeBsdfPrincipled",
        f"{name}_BSDF",
        (170, 80),
    )
    system = add_node(
        tree,
        "ShaderNodeGroup",
        f"{name}_System",
        (-180, 120),
    )
    system.node_tree = group
    link(tree, system, output_name, principled, "Base Color")
    principled.inputs["Roughness"].default_value = 0.72
    principled.inputs["Metallic"].default_value = 0.0
    link(tree, principled, "BSDF", output, "Surface")
    return material


def wireframe_material() -> bpy.types.Material:
    material = bpy.data.materials.new(WIREFRAME_PROOF_MATERIAL_NAME)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (500, 80),
    )
    principled = add_node(
        tree,
        "ShaderNodeBsdfPrincipled",
        "Wireframe_Proof_BSDF",
        (240, 80),
    )
    wire = add_node(
        tree,
        "ShaderNodeWireframe",
        "Measured_Mesh_Wireframe",
        (-260, 80),
    )
    wire.use_pixel_size = True
    wire.inputs["Size"].default_value = 0.65
    mix_node = add_node(
        tree,
        "ShaderNodeMixRGB",
        "Wire_Over_Clay",
        (-20, 130),
    )
    mix_node.inputs["Color1"].default_value = linear_rgba(204, 196, 182)
    mix_node.inputs["Color2"].default_value = linear_rgba(24, 27, 30)
    link(tree, wire, "Fac", mix_node, "Factor")
    link(tree, mix_node, "Color", principled, "Base Color")
    principled.inputs["Roughness"].default_value = 0.82
    link(tree, principled, "BSDF", output, "Surface")
    return material


def compact_band(distance_m: float, half_width_m: float) -> float:
    normalized = abs(distance_m) / half_width_m
    if normalized >= 1.0:
        return 0.0
    return math.cos(0.5 * math.pi * normalized) ** 2


def chevron_masks(
    u: float,
    radial_m: float,
    moulding: dict[str, Any],
) -> dict[str, float]:
    chevron_line_m = 0.048 + 0.104 * abs(u)
    signed = radial_m - chevron_line_m
    half_width = float(moulding["band_half_width_m"])
    roll_a = compact_band(
        signed - float(moulding["roll_center_offsets_m"][0]),
        half_width,
    )
    roll_b = compact_band(
        signed - float(moulding["roll_center_offsets_m"][1]),
        half_width,
    )
    roll = max(roll_a, roll_b)
    hollow = compact_band(
        signed - float(moulding["hollow_center_offset_m"]),
        half_width,
    )
    quiet = max(0.0, 1.0 - max(roll, hollow))
    ink = max(
        hollow,
        0.38 * compact_band(signed + 0.025, half_width * 0.55),
    )
    highlight = max(
        roll_a * (0.74 + 0.26 * max(0.0, -u)),
        roll_b * (0.74 + 0.26 * max(0.0, u)),
    )
    return {
        "roll": min(1.0, roll),
        "hollow": min(1.0, hollow),
        "quiet": min(1.0, quiet),
        "ink": min(1.0, ink),
        "highlight": min(1.0, highlight),
    }


def rotate_uv(
    value: tuple[float, float],
    quarter_turns: int,
    mirror_u: bool,
    mirror_v: bool,
) -> tuple[float, float]:
    u, v = value
    if mirror_u:
        u = -u
    if mirror_v:
        v = -v
    for _ in range(quarter_turns % 4):
        u, v = -v, u
    return u, v


def set_point_float_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: Iterable[float],
) -> None:
    attribute = mesh.attributes.new(name=name, type="FLOAT", domain="POINT")
    for item, value in zip(attribute.data, values, strict=True):
        item.value = float(value)


def set_point_int_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: Iterable[int],
) -> None:
    attribute = mesh.attributes.new(name=name, type="INT", domain="POINT")
    for item, value in zip(attribute.data, values, strict=True):
        item.value = int(value)


def set_point_vector_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: Iterable[tuple[float, float, float]],
) -> None:
    attribute = mesh.attributes.new(
        name=name,
        type="FLOAT_VECTOR",
        domain="POINT",
    )
    for item, value in zip(attribute.data, values, strict=True):
        item.vector = value


def set_face_boolean_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: Iterable[bool],
) -> None:
    attribute = mesh.attributes.new(name=name, type="BOOLEAN", domain="FACE")
    for item, value in zip(attribute.data, values, strict=True):
        item.value = bool(value)


def build_voussoir_mesh(
    stone_id: int,
    pattern: dict[str, Any],
) -> bpy.types.Mesh:
    completion = pattern["authored_completion"]
    moulding = pattern["moulding"]
    source = pattern["source_measurement"]
    segments_u = int(moulding["front_segments"]["tangential"])
    segments_v = int(moulding["front_segments"]["radial"])
    body_angle = math.radians(float(completion["body_angle_deg"]))
    inner_radius = float(completion["inner_radius_m"])
    radial_height = float(source["height_m"])
    depth = float(source["depth_m"])
    rng = random.Random(int(pattern["seed"]) + stone_id * 104729)
    variant = stone_id % int(pattern["material_variation"]["variants"])
    transform_a = pattern["material_variation"]["uv_transforms"][variant]
    transform_b = pattern["material_variation"]["uv_transforms"][
        (variant + 1) % int(pattern["material_variation"]["variants"])
    ]
    phase_a = (rng.random() * 0.512, rng.random() * 0.512)
    phase_b = (rng.random() * 0.091, rng.random() * 0.091)

    vertices: list[tuple[float, float, float]] = []
    uv_a_values: list[tuple[float, float]] = []
    uv_b_values: list[tuple[float, float]] = []
    roll_values: list[float] = []
    hollow_values: list[float] = []
    quiet_values: list[float] = []
    ink_values: list[float] = []
    highlight_values: list[float] = []

    for front in (True, False):
        for radial_index in range(segments_v + 1):
            v = radial_index / segments_v
            radius = inner_radius + radial_height * v
            radial_m = radial_height * v
            for tangent_index in range(segments_u + 1):
                u = -1.0 + 2.0 * tangent_index / segments_u
                angle = 0.5 * body_angle * u
                masks = chevron_masks(u, radial_m, moulding)
                relief = (
                    float(moulding["roll_crest_m"]) * masks["roll"]
                    - float(moulding["hollow_depression_m"]) * masks["hollow"]
                    if front
                    else 0.0
                )
                x = radius * math.cos(angle)
                z = radius * math.sin(angle)
                y = -0.5 * depth - relief if front else 0.5 * depth
                vertices.append((x, y, z))
                tangent_m = (
                    u
                    * 0.5
                    * body_angle
                    * (inner_radius + 0.5 * radial_height)
                )
                centred = (tangent_m, radial_m - 0.5 * radial_height)
                transformed_a = rotate_uv(
                    centred,
                    int(transform_a["quarter_turn"]),
                    bool(transform_a["mirror_u"]),
                    bool(transform_a["mirror_v"]),
                )
                transformed_b = rotate_uv(
                    centred,
                    int(transform_b["quarter_turn"]),
                    bool(transform_b["mirror_u"]),
                    bool(transform_b["mirror_v"]),
                )
                uv_a_values.append(
                    (
                        transformed_a[0] + phase_a[0],
                        transformed_a[1] + phase_a[1],
                    )
                )
                uv_b_values.append(
                    (
                        transformed_b[0] + phase_b[0],
                        transformed_b[1] + phase_b[1],
                    )
                )
                roll_values.append(masks["roll"] if front else 0.0)
                hollow_values.append(masks["hollow"] if front else 0.0)
                quiet_values.append(masks["quiet"] if front else 0.0)
                ink_values.append(masks["ink"] if front else 0.0)
                highlight_values.append(
                    masks["highlight"] if front else 0.0
                )

    row = segments_u + 1
    grid = row * (segments_v + 1)

    def index(front: bool, radial: int, tangent: int) -> int:
        return (0 if front else grid) + radial * row + tangent

    faces: list[tuple[int, int, int, int]] = []
    front_flags: list[bool] = []
    for radial in range(segments_v):
        for tangent in range(segments_u):
            faces.append(
                (
                    index(True, radial, tangent),
                    index(True, radial + 1, tangent),
                    index(True, radial + 1, tangent + 1),
                    index(True, radial, tangent + 1),
                )
            )
            front_flags.append(True)
    front_face_count = len(faces)
    for radial in range(segments_v):
        for tangent in range(segments_u):
            faces.append(
                (
                    index(False, radial, tangent),
                    index(False, radial, tangent + 1),
                    index(False, radial + 1, tangent + 1),
                    index(False, radial + 1, tangent),
                )
            )
            front_flags.append(False)
    for radial in range(segments_v):
        faces.append(
            (
                index(True, radial, 0),
                index(False, radial, 0),
                index(False, radial + 1, 0),
                index(True, radial + 1, 0),
            )
        )
        front_flags.append(False)
        faces.append(
            (
                index(True, radial, segments_u),
                index(True, radial + 1, segments_u),
                index(False, radial + 1, segments_u),
                index(False, radial, segments_u),
            )
        )
        front_flags.append(False)
    for tangent in range(segments_u):
        faces.append(
            (
                index(True, 0, tangent),
                index(True, 0, tangent + 1),
                index(False, 0, tangent + 1),
                index(False, 0, tangent),
            )
        )
        front_flags.append(False)
        faces.append(
            (
                index(True, segments_v, tangent),
                index(False, segments_v, tangent),
                index(False, segments_v, tangent + 1),
                index(True, segments_v, tangent + 1),
            )
        )
        front_flags.append(False)

    mesh = bpy.data.meshes.new(f"IGGY_ChevronVoussoir_{stone_id:02d}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.validate(verbose=True, clean_customdata=False)
    mesh.update(calc_edges=True)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons[:front_face_count]:
        polygon.use_smooth = True

    uv_a = mesh.uv_layers.new(name="IGGY_StoneUV_A")
    uv_b = mesh.uv_layers.new(name="IGGY_StoneUV_B")
    for loop in mesh.loops:
        uv_a.data[loop.index].uv = uv_a_values[loop.vertex_index]
        uv_b.data[loop.index].uv = uv_b_values[loop.vertex_index]

    point_count = len(vertices)
    set_point_int_attribute(
        mesh,
        "iggy_voussoir_id",
        [stone_id] * point_count,
    )
    set_point_int_attribute(
        mesh,
        "iggy_material_variant",
        [variant] * point_count,
    )
    set_point_vector_attribute(
        mesh,
        "iggy_material_phase",
        [(phase_a[0], phase_a[1], phase_b[0])] * point_count,
    )
    set_point_float_attribute(mesh, "iggy_chevron_roll", roll_values)
    set_point_float_attribute(mesh, "iggy_chevron_hollow", hollow_values)
    set_point_float_attribute(mesh, "iggy_chevron_quiet", quiet_values)
    set_point_float_attribute(mesh, "iggy_chevron_ink", ink_values)
    set_point_float_attribute(
        mesh,
        "iggy_chevron_highlight",
        highlight_values,
    )
    set_face_boolean_attribute(
        mesh,
        "iggy_carved_trim",
        [True] * len(faces),
    )
    set_face_boolean_attribute(
        mesh,
        "iggy_chevron_front",
        front_flags,
    )
    set_face_boolean_attribute(
        mesh,
        "iggy_fracture_interior",
        [False] * len(faces),
    )
    mesh["iggy_front_face_count"] = front_face_count
    mesh["iggy_closed_volume"] = True
    mesh["iggy_bevel_m"] = 0.0
    return mesh


def build_product(
    pattern: dict[str, Any],
    product_collection: bpy.types.Collection,
    material: bpy.types.Material,
) -> list[bpy.types.Object]:
    count = int(pattern["authored_completion"]["count"])
    pitch = math.radians(
        float(pattern["authored_completion"]["pitch_angle_deg"])
    )
    objects: list[bpy.types.Object] = []
    for stone_id in range(count):
        mesh = build_voussoir_mesh(stone_id, pattern)
        obj = bpy.data.objects.new(
            f"IGGY_ChevronVoussoir_{stone_id:02d}",
            mesh,
        )
        center_angle = (stone_id + 0.5) * pitch
        obj.rotation_euler[1] = -center_angle
        obj.data.materials.append(material)
        obj["iggy_role"] = "measured_chevron_voussoir"
        obj["iggy_source_id"] = "S01_old_sarum_catalogue_item_55"
        obj["iggy_catalogue_item"] = 55
        obj["iggy_voussoir_id"] = stone_id
        obj["iggy_center_angle_deg"] = math.degrees(center_angle)
        obj["iggy_catalogue_radial_height_m"] = 0.2
        obj["iggy_catalogue_inner_chord_m"] = 0.14
        obj["iggy_catalogue_outer_chord_m"] = 0.18
        obj["iggy_catalogue_depth_m"] = 0.27
        obj["iggy_body_angle_deg"] = float(
            pattern["authored_completion"]["body_angle_deg"]
        )
        obj["iggy_joint_gap_centerline_m"] = 0.003
        obj["iggy_damage_enabled"] = False
        obj["iggy_fracture_enabled"] = False
        product_collection.objects.link(obj)
        objects.append(obj)
    return objects


def add_cube(
    name: str,
    location: tuple[float, float, float],
    dimensions: tuple[float, float, float],
    target_collection: bpy.types.Collection,
    material: bpy.types.Material,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for source_collection in list(obj.users_collection):
        source_collection.objects.unlink(obj)
    target_collection.objects.link(obj)
    obj.data.materials.append(material)
    obj["iggy_role"] = "proof_context_only"
    return obj


def build_proof_context(
    proof_collection: bpy.types.Collection,
) -> list[bpy.types.Object]:
    backdrop_material = simple_material(
        "IGGY_MAT_ProofBackdrop",
        linear_rgba(68, 72, 76),
        0.9,
    )
    support_material = simple_material(
        "IGGY_MAT_ProofSupport",
        linear_rgba(126, 118, 106),
        0.82,
    )
    backdrop = add_cube(
        "IGGY_ProofBackdrop",
        (0.0, 0.30, 0.72),
        (2.70, 0.10, 1.75),
        proof_collection,
        backdrop_material,
    )
    left = add_cube(
        "IGGY_ProofSpringing_Left",
        (-0.83, 0.02, -0.12),
        (0.34, 0.31, 0.24),
        proof_collection,
        support_material,
    )
    right = add_cube(
        "IGGY_ProofSpringing_Right",
        (0.83, 0.02, -0.12),
        (0.34, 0.31, 0.24),
        proof_collection,
        support_material,
    )
    return [backdrop, left, right]


def add_area_light(
    name: str,
    location: tuple[float, float, float],
    energy: float,
    size: float,
    color: tuple[float, float, float],
    target: tuple[float, float, float],
) -> bpy.types.Object:
    data = bpy.data.lights.new(name=name, type="AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = color
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    obj.location = location
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    return obj


def configure_lighting() -> None:
    add_area_light(
        "IGGY_Key_LargeNeutral",
        (-2.7, -3.2, 3.8),
        620.0,
        3.0,
        (1.0, 0.91, 0.82),
        (0.0, 0.0, 0.72),
    )
    add_area_light(
        "IGGY_Fill_Cool",
        (3.0, -1.1, 2.1),
        260.0,
        2.5,
        (0.72, 0.82, 1.0),
        (0.0, 0.0, 0.70),
    )
    add_area_light(
        "IGGY_Grazing_Rim",
        (-2.0, -0.25, 1.0),
        340.0,
        1.0,
        (1.0, 0.76, 0.56),
        (-0.1, -0.1, 0.72),
    )
    world = bpy.context.scene.world
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    if background is not None:
        background.inputs["Color"].default_value = (0.018, 0.022, 0.028, 1.0)
        background.inputs["Strength"].default_value = 0.18


def create_camera() -> bpy.types.Object:
    data = bpy.data.cameras.new("IGGY_ChevronProofCamera")
    data.lens = 58.0
    data.sensor_width = 36.0
    obj = bpy.data.objects.new("IGGY_ChevronProofCamera", data)
    bpy.context.scene.collection.objects.link(obj)
    bpy.context.scene.camera = obj
    return obj


def aim_camera(
    camera: bpy.types.Object,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    lens: float,
) -> None:
    camera.location = location
    camera.data.lens = lens
    direction = Vector(target) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def configure_scene() -> None:
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 900
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.film_transparent = False
    scene.render.use_file_extension = True
    scene.render.image_settings.color_depth = "8"
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = -0.35
    scene.view_settings.gamma = 1.0
    scene.render.resolution_percentage = 100


def assign_product_material(
    product: list[bpy.types.Object],
    material: bpy.types.Material,
) -> None:
    for obj in product:
        obj.data.materials.clear()
        obj.data.materials.append(material)


def render_view(
    output: Path,
    filename: str,
    camera: bpy.types.Object,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    lens: float,
) -> Path:
    aim_camera(camera, location, target, lens)
    path = output / filename
    bpy.context.scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    if not path.is_file():
        raise RuntimeError(f"Render was not written: {path}")
    return path


def render_proofs(
    output: Path,
    camera: bpy.types.Object,
    product: list[bpy.types.Object],
    live: bpy.types.Material,
    clay: bpy.types.Material,
    moulding_proof: bpy.types.Material,
    identity_proof: bpy.types.Material,
    wireframe: bpy.types.Material,
    selected_proofs: set[str],
) -> dict[str, Path]:
    proofs = output / "proofs"
    proofs.mkdir(parents=True, exist_ok=True)
    views: dict[str, Path] = {}
    if selected_proofs & {"neutral_clay_front", "neutral_clay_grazing"}:
        assign_product_material(product, clay)
    if "neutral_clay_front" in selected_proofs:
        views["neutral_clay_front"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_neutral_clay_front.png",
            camera,
            (0.0, -4.0, 0.67),
            (0.0, 0.0, 0.56),
            62.0,
        )
    if "neutral_clay_grazing" in selected_proofs:
        views["neutral_clay_grazing"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_neutral_clay_grazing.png",
            camera,
            (-2.25, -2.55, 1.08),
            (0.0, -0.02, 0.62),
            58.0,
        )
    if selected_proofs & {
        "live_material_front",
        "live_material_grazing",
        "measured_close",
        "distance_read",
    }:
        assign_product_material(product, live)
    if "live_material_front" in selected_proofs:
        views["live_material_front"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_live_material_front.png",
            camera,
            (0.0, -4.0, 0.67),
            (0.0, 0.0, 0.56),
            62.0,
        )
    if "live_material_grazing" in selected_proofs:
        views["live_material_grazing"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_live_material_grazing.png",
            camera,
            (-2.25, -2.55, 1.08),
            (0.0, -0.02, 0.62),
            58.0,
        )
    if "measured_close" in selected_proofs:
        views["measured_close"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_measured_close.png",
            camera,
            (0.0, -1.02, 1.02),
            (0.0, -0.08, 0.84),
            72.0,
        )
    if "distance_read" in selected_proofs:
        views["distance_read"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_distance_read.png",
            camera,
            (0.0, -7.2, 0.72),
            (0.0, 0.0, 0.56),
            72.0,
        )
    if "moulding_proof" in selected_proofs:
        assign_product_material(product, moulding_proof)
        views["moulding_proof"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_moulding_proof.png",
            camera,
            (0.0, -3.85, 0.67),
            (0.0, 0.0, 0.56),
            62.0,
        )
    if "identity_proof" in selected_proofs:
        assign_product_material(product, identity_proof)
        views["identity_proof"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_identity_proof.png",
            camera,
            (0.0, -3.85, 0.67),
            (0.0, 0.0, 0.56),
            62.0,
        )
    if "wireframe_proof" in selected_proofs:
        assign_product_material(product, wireframe)
        views["wireframe_proof"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_wireframe_proof.png",
            camera,
            (-1.45, -3.1, 1.18),
            (0.0, 0.0, 0.62),
            60.0,
        )
    assign_product_material(product, live)
    return views


def resolve_proof_selection(
    output: Path,
    proof_set: str,
    requested_proofs: Iterable[str],
) -> set[str]:
    requested = set(requested_proofs)
    unknown = requested - set(PROOF_IDS)
    if unknown:
        raise ValueError("Unknown proof ids: " + ", ".join(sorted(unknown)))
    if proof_set == "all":
        if requested:
            raise ValueError("--proof is only valid with --proof-set changed")
        selected = set(PROOF_IDS)
    elif proof_set == "changed":
        if not requested:
            raise ValueError("--proof-set changed requires at least one --proof")
        selected = requested
    elif proof_set == "none":
        if requested:
            raise ValueError("--proof-set none cannot receive --proof")
        selected = set()
    else:
        raise ValueError(f"Unsupported proof set: {proof_set}")
    if proof_set != "all" and output.resolve() == DEFAULT_OUTPUT.resolve():
        raise ValueError(
            "Fast or changed proof builds require an explicit non-canonical "
            "--output directory"
        )
    return selected


def edge_manifold(mesh: bpy.types.Mesh) -> bool:
    usage = [0] * len(mesh.edges)
    edge_lookup = {
        tuple(sorted(edge.vertices)): edge.index
        for edge in mesh.edges
    }
    for polygon in mesh.polygons:
        for edge_index in polygon.edge_keys:
            usage[edge_lookup[tuple(sorted(edge_index))]] += 1
    return all(count == 2 for count in usage)


def node_inventory(group: bpy.types.NodeTree) -> list[dict[str, Any]]:
    def socket_default(socket: bpy.types.NodeSocket) -> Any:
        if not hasattr(socket, "default_value"):
            return None
        value = socket.default_value
        if isinstance(value, (str, bool, int, float)):
            return value
        try:
            return [float(component) for component in value]
        except (TypeError, ValueError):
            return str(value)

    result = []
    for node in sorted(group.nodes, key=lambda item: item.name):
        entry: dict[str, Any] = {
            "name": node.name,
            "bl_idname": node.bl_idname,
            "inputs": [
                {
                    "name": socket.name,
                    "identifier": socket.identifier,
                    "type": socket.type,
                    "linked": socket.is_linked,
                    "default": (
                        None if socket.is_linked else socket_default(socket)
                    ),
                }
                for socket in node.inputs
            ],
            "outputs": [
                {
                    "name": socket.name,
                    "identifier": socket.identifier,
                    "type": socket.type,
                    "linked": socket.is_linked,
                }
                for socket in node.outputs
            ],
        }
        if node.bl_idname == "ShaderNodeTexImage":
            entry["image"] = node.image.name if node.image is not None else None
            entry["image_packed"] = bool(
                node.image is not None and node.image.packed_file is not None
            )
            entry["color_space"] = (
                node.image.colorspace_settings.name
                if node.image is not None
                else None
            )
            entry["extension"] = node.extension
            entry["interpolation"] = node.interpolation
            entry["projection"] = node.projection
        if node.bl_idname == "ShaderNodeAttribute":
            entry["attribute_name"] = node.attribute_name
        if node.bl_idname == "ShaderNodeMath":
            entry["operation"] = node.operation
        if node.bl_idname == "ShaderNodeVectorMath":
            entry["operation"] = node.operation
        if node.bl_idname == "ShaderNodeMixRGB":
            entry["blend_type"] = node.blend_type
            entry["use_clamp"] = node.use_clamp
        if node.bl_idname in {
            "ShaderNodeSeparateColor",
            "ShaderNodeCombineColor",
        }:
            entry["mode"] = node.mode
        if node.bl_idname == "ShaderNodeNormalMap":
            entry["space"] = node.space
            entry["uv_map"] = node.uv_map
        if node.bl_idname == "ShaderNodeBump":
            entry["invert"] = node.invert
        if node.bl_idname == "ShaderNodeUVMap":
            entry["uv_map"] = node.uv_map
        if node.bl_idname == "ShaderNodeMapRange":
            entry["interpolation_type"] = node.interpolation_type
            entry["clamp"] = node.clamp
        result.append(entry)
    return result


def link_inventory(group: bpy.types.NodeTree) -> list[dict[str, str]]:
    return sorted(
        (
            {
                "from_node": item.from_node.name,
                "from_socket": item.from_socket.name,
                "to_node": item.to_node.name,
                "to_socket": item.to_socket.name,
            }
            for item in group.links
        ),
        key=lambda item: (
            item["from_node"],
            item["from_socket"],
            item["to_node"],
            item["to_socket"],
        ),
    )


def geometry_summary(
    product: list[bpy.types.Object],
    pattern: dict[str, Any],
) -> dict[str, Any]:
    first = product[0]
    inner_radius = float(pattern["authored_completion"]["inner_radius_m"])
    outer_radius = float(pattern["authored_completion"]["outer_radius_m"])
    body_angle = math.radians(
        float(pattern["authored_completion"]["body_angle_deg"])
    )
    inner_chord = 2.0 * inner_radius * math.sin(0.5 * body_angle)
    outer_chord = 2.0 * outer_radius * math.sin(0.5 * body_angle)
    mean_radius = 0.5 * (inner_radius + outer_radius)
    pitch = math.radians(
        float(pattern["authored_completion"]["pitch_angle_deg"])
    )
    centerline_gap = mean_radius * (pitch - body_angle)
    return {
        "object_count": len(product),
        "all_separate_mesh_datablocks": (
            len({obj.data.name for obj in product}) == len(product)
        ),
        "vertices_per_object": len(first.data.vertices),
        "polygons_per_object": len(first.data.polygons),
        "front_polygons_per_object": int(first.data["iggy_front_face_count"]),
        "all_meshes_manifold": all(edge_manifold(obj.data) for obj in product),
        "all_object_scales_unit": all(
            all(abs(component - 1.0) < 1.0e-9 for component in obj.scale)
            for obj in product
        ),
        "catalogue_inner_chord_m": inner_chord,
        "derived_outer_chord_m": outer_chord,
        "centerline_joint_gap_m": centerline_gap,
        "clear_diameter_m": 2.0 * inner_radius,
        "outer_diameter_m": 2.0 * outer_radius,
        "bevel_modifier_count": sum(
            1
            for obj in product
            for modifier in obj.modifiers
            if modifier.type == "BEVEL"
        ),
        "boolean_modifier_count": sum(
            1
            for obj in product
            for modifier in obj.modifiers
            if modifier.type == "BOOLEAN"
        ),
    }


def write_blender_manifest(
    output: Path,
    texture_manifest: dict[str, Any],
    pattern: dict[str, Any],
    product: list[bpy.types.Object],
    group: bpy.types.NodeTree,
    views: dict[str, Path],
    blend_path: Path,
    proof_set: str,
) -> dict[str, Any]:
    geometry = geometry_summary(product, pattern)
    node_data = node_inventory(group)
    links = link_inventory(group)
    required_attributes = {
        "iggy_voussoir_id",
        "iggy_material_phase",
        "iggy_material_variant",
        "iggy_chevron_roll",
        "iggy_chevron_hollow",
        "iggy_chevron_quiet",
        "iggy_chevron_ink",
        "iggy_chevron_highlight",
        "iggy_carved_trim",
        "iggy_chevron_front",
        "iggy_fracture_interior",
    }
    attribute_names = {
        attribute.name
        for obj in product
        for attribute in obj.data.attributes
        if not attribute.name.startswith(".")
        and attribute.name
        not in {"position", "sharp_face", "IGGY_StoneUV_A", "IGGY_StoneUV_B"}
    }
    manifest = {
        "schema": BLENDER_MANIFEST_SCHEMA,
        "blender_version": bpy.app.version_string,
        "texture_manifest_schema": texture_manifest["schema"],
        "pattern_schema": pattern["schema"],
        "build_mode": {
            "proof_set": proof_set,
            "selected_proofs": sorted(views),
            "canonical_candidate_complete": (
                proof_set == "all" and set(views) == set(PROOF_IDS)
            ),
        },
        "source_measurement": pattern["source_measurement"],
        "authored_completion": pattern["authored_completion"],
        "geometry": geometry,
        "attributes": {
            "required": sorted(required_attributes),
            "present": sorted(attribute_names),
            "all_required_present": required_attributes <= attribute_names,
            "fracture_values_all_zero": all(
                not item.value
                for obj in product
                for item in obj.data.attributes[
                    "iggy_fracture_interior"
                ].data
            ),
        },
        "uv_layers": {
            obj.name: sorted(layer.name for layer in obj.data.uv_layers)
            for obj in product
        },
        "shader": {
            "group_name": group.name,
            "node_count": len(group.nodes),
            "link_count": len(group.links),
            "nodes": node_data,
            "links": links,
            "all_image_nodes_packed": all(
                entry.get("image_packed", True) for entry in node_data
            ),
            "normal_map_nodes": [
                entry["name"]
                for entry in node_data
                if entry["bl_idname"] == "ShaderNodeNormalMap"
            ],
            "bump_nodes": [
                entry["name"]
                for entry in node_data
                if entry["bl_idname"] == "ShaderNodeBump"
            ],
            "attribute_nodes": sorted(
                entry["attribute_name"]
                for entry in node_data
                if entry["bl_idname"] == "ShaderNodeAttribute"
            ),
        },
        "proofs": {
            name: {
                "filename": path.name,
                "sha256": sha256(path),
                "bytes": path.stat().st_size,
            }
            for name, path in sorted(views.items())
        },
        "blend": {
            "filename": blend_path.name,
            "sha256": sha256(blend_path),
            "bytes": blend_path.stat().st_size,
        },
        "validation": {
            "individual_voussoirs": geometry["object_count"] == 16,
            "closed_manifold_stones": geometry["all_meshes_manifold"],
            "catalogue_inner_chord_within_0p005m": (
                abs(geometry["catalogue_inner_chord_m"] - 0.14) <= 0.005
            ),
            "catalogue_outer_chord_within_0p005m": (
                abs(geometry["derived_outer_chord_m"] - 0.18) <= 0.005
            ),
            "centerline_joint_gap_is_0p003m": (
                abs(geometry["centerline_joint_gap_m"] - 0.003) <= 1.0e-9
            ),
            "no_bevel_modifiers": geometry["bevel_modifier_count"] == 0,
            "no_boolean_modifiers": geometry["boolean_modifier_count"] == 0,
            "required_attributes_present": required_attributes <= attribute_names,
            "all_texture_nodes_packed": all(
                entry.get("image_packed", True) for entry in node_data
            ),
            "normal_map_live": any(
                link_item["from_node"] == "IGGY_OpenGLBodyNormal64mm"
                and link_item["to_node"] == "IGGY_DecorrelatedBodyHeight91mm"
                for link_item in links
            ),
            "height_not_inverted": (
                group.nodes["IGGY_DecorrelatedBodyHeight91mm"].invert is False
            ),
            "damage_authored": False,
            "fracture_authored": False,
            "proof_count": len(views),
            "complete_proof_set": set(views) == set(PROOF_IDS),
        },
    }
    manifest_path = (
        output / "cathedral_stone_trim_fracture_v1_blender_manifest.json"
    )
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    return manifest


def build(
    output: Path,
    proof_set: str = "all",
    requested_proofs: Iterable[str] = (),
) -> dict[str, Any]:
    profile = read_json(PROFILE_PATH)
    pattern = read_json(PATTERN_PATH)
    if profile["schema"] != (
        "iggy3d.material.cathedral_stone_trim_fracture_v1.profile.v2"
    ):
        raise ValueError("Unexpected chevron profile schema")
    if pattern["schema"] != "iggy3d.pattern.chevron_voussoir_portal.v2":
        raise ValueError("Unexpected chevron pattern schema")
    selected_proofs = resolve_proof_selection(
        output,
        proof_set,
        requested_proofs,
    )
    texture_manifest = ensure_texture_family(output)
    reset_scene()
    configure_scene()
    product_collection = collection(PRODUCT_COLLECTION_NAME)
    group = create_material_group(output)
    live = configure_material(group)
    product = build_product(pattern, product_collection, live)
    views: dict[str, Path] = {}
    if selected_proofs:
        proof_collection = collection(PROOF_COLLECTION_NAME)
        clay = simple_material(
            CLAY_MATERIAL_NAME,
            linear_rgba(186, 181, 171),
            0.86,
        )
        moulding_proof = group_proof_material(
            MOULDING_PROOF_MATERIAL_NAME,
            group,
            "Moulding Proof",
        )
        identity_proof = group_proof_material(
            IDENTITY_PROOF_MATERIAL_NAME,
            group,
            "Identity Proof",
        )
        wireframe = wireframe_material()
        build_proof_context(proof_collection)
        configure_lighting()
        camera = create_camera()
        views = render_proofs(
            output,
            camera,
            product,
            live,
            clay,
            moulding_proof,
            identity_proof,
            wireframe,
            selected_proofs,
        )
    scene = bpy.context.scene
    scene["iggy_asset_schema"] = (
        "iggy3d.asset.chevron_voussoir_portal.measured.v2"
    )
    scene["iggy_source_measurement"] = (
        "S01_old_sarum_catalogue_item_55"
    )
    scene["iggy_product_object_count"] = len(product)
    scene["iggy_proof_set"] = proof_set
    scene["iggy_selected_proofs"] = json.dumps(sorted(views))
    scene["iggy_fracture_enabled"] = False
    scene["iggy_damage_enabled"] = False
    blend_path = output / "cathedral_stone_trim_fracture_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path), check_existing=False)
    manifest = write_blender_manifest(
        output,
        texture_manifest,
        pattern,
        product,
        group,
        views,
        blend_path,
        proof_set,
    )
    return manifest


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--proof-set",
        choices=("none", "changed", "all"),
        default="all",
    )
    parser.add_argument("--proof", action="append", default=[])
    return parser.parse_args(argv)


def main() -> int:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    args = parse_args(argv)
    manifest = build(args.output, args.proof_set, args.proof)
    print(
        json.dumps(
            {
                "status": "ok",
                "schema": manifest["schema"],
                "object_count": manifest["geometry"]["object_count"],
                "proof_count": manifest["validation"]["proof_count"],
                "proof_set": manifest["build_mode"]["proof_set"],
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
