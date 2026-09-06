#!/usr/bin/env python3
"""Build the curve-native three-strand plant-fibre rope source and proofs.

This script is intended to run inside Blender:

    Blender --background --factory-startup \
      --python build_rope_plant_fibre_v1.py

The existing texture generator remains responsible for authored colour and
fine surface maps.  This builder owns the structural hierarchy that a flat
texture cannot provide: three right-laid strands, seven counter-laid yarns per
strand, curve-stable metre coordinates, profile coordinates, and optional
sparse flyaway geometry.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import sys
from typing import Any, Callable

import bpy
from mathutils import Vector


SCRIPT_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "rope_plant_fibre_v1.json"
PATTERN_PATH = SCRIPT_ROOT / "patterns" / "three_strand_regular_lay_v1.json"
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"
UTILITY_FIBRE_MASK_PATH = (
    DEFAULT_OUTPUT
    / "rope_plant_fibre_v1_utility_line_fibre_detail_masks.png"
)
UTILITY_BASECOLOUR_PATH = (
    DEFAULT_OUTPUT
    / "rope_plant_fibre_v1_utility_line_basecolor.png"
)

GROUP_NAME = "IGGY_GN_RopePlantFibre_v001"
MATERIAL_NAME = "IGGY_MAT_RopePlantFibre_v001"
FLYAWAY_MATERIAL_NAME = "IGGY_MAT_RopeFlyaway_v001"
FLOOR_MATERIAL_NAME = "IGGY_MAT_RopeProofFloor_v001"
COLLECTION_NAME = "IGGY_RopePlantFibreProof"

TAU = math.tau
STRAND_COUNT = 3
YARNS_PER_STRAND = 7
TOTAL_YARNS = STRAND_COUNT * YARNS_PER_STRAND


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--render-width", type=int, default=1400)
    parser.add_argument("--render-height", type=int, default=840)
    parser.add_argument("--skip-renders", action="store_true")
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def srgb_channel_to_linear(value: float) -> float:
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def hex_to_linear_rgba(value: str) -> tuple[float, float, float, float]:
    digits = value.removeprefix("#")
    if len(digits) != 6:
        raise ValueError(f"Expected six-digit color, got {value}")
    channels = [int(digits[index : index + 2], 16) / 255.0 for index in (0, 2, 4)]
    return tuple(srgb_channel_to_linear(channel) for channel in channels) + (1.0,)


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (
        bpy.data.curves,
        bpy.data.meshes,
        bpy.data.cameras,
        bpy.data.lights,
        bpy.data.materials,
        bpy.data.node_groups,
    ):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)
    for collection in list(bpy.data.collections):
        if collection.users == 0:
            bpy.data.collections.remove(collection)


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


def socket(
    sockets: bpy.types.NodeInputs | bpy.types.NodeOutputs,
    key: str | int,
) -> bpy.types.NodeSocket:
    return sockets[key]


def link(
    tree: bpy.types.NodeTree,
    source: bpy.types.Node,
    source_socket: str | int,
    target: bpy.types.Node,
    target_socket: str | int,
) -> None:
    tree.links.new(
        socket(source.outputs, source_socket),
        socket(target.inputs, target_socket),
    )


def make_math(
    tree: bpy.types.NodeTree,
    name: str,
    operation: str,
    location: tuple[float, float],
    *,
    second: float | None = None,
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMath", name, location)
    node.operation = operation
    if second is not None:
        node.inputs[1].default_value = second
    return node


def make_integer_math(
    tree: bpy.types.NodeTree,
    name: str,
    operation: str,
    location: tuple[float, float],
    *,
    second: int | None = None,
) -> bpy.types.Node:
    node = add_node(tree, "FunctionNodeIntegerMath", name, location)
    node.operation = operation
    if second is not None:
        node.inputs[1].default_value = second
    return node


def store_attribute(
    tree: bpy.types.NodeTree,
    name: str,
    location: tuple[float, float],
    *,
    attribute_name: str,
    data_type: str,
    domain: str,
) -> bpy.types.Node:
    node = add_node(
        tree,
        "GeometryNodeStoreNamedAttribute",
        name,
        location,
    )
    node.data_type = data_type
    node.domain = domain
    node.inputs["Name"].default_value = attribute_name
    return node


def named_attribute(
    tree: bpy.types.NodeTree,
    name: str,
    location: tuple[float, float],
    *,
    attribute_name: str,
    data_type: str,
) -> bpy.types.Node:
    node = add_node(
        tree,
        "GeometryNodeInputNamedAttribute",
        name,
        location,
    )
    node.data_type = data_type
    node.inputs["Name"].default_value = attribute_name
    return node


def new_interface_socket(
    group: bpy.types.NodeTree,
    name: str,
    in_out: str,
    socket_type: str,
    *,
    default: Any | None = None,
    minimum: float | int | None = None,
    maximum: float | int | None = None,
) -> bpy.types.NodeTreeInterfaceSocket:
    interface_socket = group.interface.new_socket(
        name=name,
        in_out=in_out,
        socket_type=socket_type,
    )
    if default is not None:
        interface_socket.default_value = default
    if minimum is not None:
        interface_socket.min_value = minimum
    if maximum is not None:
        interface_socket.max_value = maximum
    return interface_socket


def create_rope_material(profile: dict[str, Any]) -> bpy.types.Material:
    material = bpy.data.materials.get(MATERIAL_NAME)
    if material is not None:
        bpy.data.materials.remove(material, do_unlink=True)
    material = bpy.data.materials.new(MATERIAL_NAME)
    material.use_nodes = True
    material.diffuse_color = hex_to_linear_rgba("#8b6b46")
    material["iggy_material_schema"] = "iggy3d.material.rope_plant_fibre_v1.v1"
    material["iggy_uses_ai_generated_imagery"] = False
    material["iggy_uses_baked_ao_in_base_colour"] = False
    material["iggy_structural_relief_owner"] = "geometry_nodes"
    material["iggy_fine_relief_owner"] = "shader"
    material["iggy_palette_tone_count"] = len(
        profile["palette"]["dry_warm_srgb"]
    )

    tree = material.node_tree
    tree.nodes.clear()
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "IGGY_RopeMaterialOutput",
        (1180, 120),
    )
    principled = add_node(
        tree,
        "ShaderNodeBsdfPrincipled",
        "IGGY_RopePrincipled",
        (880, 120),
    )
    principled.inputs["Metallic"].default_value = 0.0
    principled.inputs["IOR"].default_value = 1.47
    principled.inputs["Anisotropic"].default_value = 0.18
    principled.inputs["Sheen Roughness"].default_value = 0.62
    principled.inputs["Sheen Tint"].default_value = hex_to_linear_rgba(
        "#d8c3a2"
    )
    link(tree, principled, "BSDF", output, "Surface")

    u_attribute = add_node(
        tree,
        "ShaderNodeAttribute",
        "IGGY_LongitudinalMetres",
        (-1220, 500),
    )
    u_attribute.attribute_name = "sinc_rope_u_m"
    v_attribute = add_node(
        tree,
        "ShaderNodeAttribute",
        "IGGY_ProfileTurn",
        (-1220, 340),
    )
    v_attribute.attribute_name = "sinc_rope_v_turn"
    strand_attribute = add_node(
        tree,
        "ShaderNodeAttribute",
        "IGGY_StrandIdentity",
        (-1220, 180),
    )
    strand_attribute.attribute_name = "sinc_rope_strand_id"
    yarn_attribute = add_node(
        tree,
        "ShaderNodeAttribute",
        "IGGY_YarnIdentity",
        (-1220, 20),
    )
    yarn_attribute.attribute_name = "sinc_rope_yarn_id"
    diameter_attribute = add_node(
        tree,
        "ShaderNodeAttribute",
        "IGGY_RopeDiameterMetres",
        (-1220, -140),
    )
    diameter_attribute.attribute_name = "sinc_rope_diameter_m"
    rope_uv_attribute = add_node(
        tree,
        "ShaderNodeAttribute",
        "IGGY_RopeFaceCornerUV",
        (-1220, -300),
    )
    rope_uv_attribute.attribute_name = "rope_uv"

    missing_atlases = [
        path
        for path in (UTILITY_FIBRE_MASK_PATH, UTILITY_BASECOLOUR_PATH)
        if not path.is_file()
    ]
    if missing_atlases:
        raise FileNotFoundError(
            "Missing authored rope atlases. Run "
            "generate_rope_plant_fibre_v1.py before the Blender build: "
            + ", ".join(str(path) for path in missing_atlases)
        )
    fibre_image = bpy.data.images.load(
        str(UTILITY_FIBRE_MASK_PATH),
        check_existing=False,
    )
    fibre_image.name = "IGGY_IMG_RopeFibreDetailMasks_Utility_v001"
    fibre_image.colorspace_settings.name = "Non-Color"
    fibre_image.pack()
    basecolour_image = bpy.data.images.load(
        str(UTILITY_BASECOLOUR_PATH),
        check_existing=False,
    )
    basecolour_image.name = "IGGY_IMG_RopeBaseColour_Utility_v001"
    basecolour_image.colorspace_settings.name = "sRGB"
    basecolour_image.pack()

    uv_separate = add_node(
        tree,
        "ShaderNodeSeparateXYZ",
        "IGGY_SeparateRopeUV",
        (-1030, -500),
    )
    link(tree, rope_uv_attribute, "Vector", uv_separate, "Vector")
    atlas_yarn_phase = make_math(
        tree,
        "IGGY_FibreAtlasYarnPhase",
        "MULTIPLY",
        (-1020, -620),
        second=3.0,
    )
    atlas_identity = make_math(
        tree,
        "IGGY_FibreAtlasIdentity",
        "ADD",
        (-830, -590),
    )
    atlas_row = make_math(
        tree,
        "IGGY_FibreAtlasVariationRow",
        "MODULO",
        (-640, -590),
        second=4.0,
    )
    atlas_y = make_math(
        tree,
        "IGGY_FibreAtlasRowCoordinate",
        "ADD",
        (-450, -590),
    )
    atlas_y_scale = make_math(
        tree,
        "IGGY_FibreAtlasFourRowScale",
        "MULTIPLY",
        (-260, -590),
        second=0.25,
    )
    link(tree, yarn_attribute, "Fac", atlas_yarn_phase, 0)
    link(tree, strand_attribute, "Fac", atlas_identity, 0)
    link(tree, atlas_yarn_phase, "Value", atlas_identity, 1)
    link(tree, atlas_identity, "Value", atlas_row, 0)
    link(tree, uv_separate, "Y", atlas_y, 0)
    link(tree, atlas_row, "Value", atlas_y, 1)
    link(tree, atlas_y, "Value", atlas_y_scale, 0)
    atlas_uv = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        "IGGY_CombineFibreAtlasUV",
        (-40, -520),
    )
    link(tree, uv_separate, "X", atlas_uv, "X")
    link(tree, atlas_y_scale, "Value", atlas_uv, "Y")
    fibre_atlas = add_node(
        tree,
        "ShaderNodeTexImage",
        "IGGY_FibreAtlas_LongShortSpawn",
        (150, -520),
    )
    fibre_atlas.image = fibre_image
    fibre_atlas.interpolation = "Linear"
    fibre_atlas.extension = "REPEAT"
    link(tree, atlas_uv, "Vector", fibre_atlas, "Vector")
    fibre_channels = add_node(
        tree,
        "ShaderNodeSeparateColor",
        "IGGY_SeparateFibreDetailMasks",
        (360, -520),
    )
    fibre_channels.mode = "RGB"
    link(tree, fibre_atlas, "Color", fibre_channels, "Color")
    basecolour_atlas = add_node(
        tree,
        "ShaderNodeTexImage",
        "IGGY_AuthoredFibreBaseColourAtlas",
        (150, -760),
    )
    basecolour_atlas.image = basecolour_image
    basecolour_atlas.interpolation = "Linear"
    basecolour_atlas.extension = "REPEAT"
    link(tree, atlas_uv, "Vector", basecolour_atlas, "Vector")

    u_scale = make_math(
        tree,
        "IGGY_ColourLengthScale",
        "MULTIPLY",
        (-1020, 510),
        second=4.25,
    )
    strand_scale = make_math(
        tree,
        "IGGY_ColourStrandPhase",
        "MULTIPLY",
        (-1020, 190),
        second=2.173,
    )
    yarn_scale = make_math(
        tree,
        "IGGY_ColourYarnPhase",
        "MULTIPLY",
        (-1020, 30),
        second=0.731,
    )
    link(tree, u_attribute, "Fac", u_scale, 0)
    link(tree, strand_attribute, "Fac", strand_scale, 0)
    link(tree, yarn_attribute, "Fac", yarn_scale, 0)

    phase_sum = make_math(
        tree,
        "IGGY_ColourIdentityPhase",
        "ADD",
        (-830, 140),
    )
    link(tree, strand_scale, "Value", phase_sum, 0)
    link(tree, yarn_scale, "Value", phase_sum, 1)
    colour_coordinate = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        "IGGY_ColourConstructionCoordinate",
        (-620, 390),
    )
    link(tree, u_scale, "Value", colour_coordinate, "X")
    link(tree, phase_sum, "Value", colour_coordinate, "Y")
    link(tree, v_attribute, "Fac", colour_coordinate, "Z")

    broad_noise = add_node(
        tree,
        "ShaderNodeTexNoise",
        "IGGY_Colour_BroadNoise",
        (-420, 520),
    )
    broad_noise.noise_dimensions = "3D"
    broad_noise.inputs["Scale"].default_value = 0.72
    broad_noise.inputs["Detail"].default_value = 2.4
    broad_noise.inputs["Roughness"].default_value = 0.48
    link(tree, colour_coordinate, "Vector", broad_noise, "Vector")
    broad_ramp = add_node(
        tree,
        "ShaderNodeValToRGB",
        "IGGY_Colour_BroadPassages",
        (-190, 520),
    )
    broad_ramp.color_ramp.interpolation = "EASE"
    broad_colours = (
        (0.00, "#453426"),
        (0.13, "#5d4630"),
        (0.28, "#765a3b"),
        (0.43, "#8e6e47"),
        (0.58, "#a78358"),
        (0.72, "#c09d70"),
        (0.86, "#9b7a55"),
        (1.00, "#d1b58b"),
    )
    broad_ramp.color_ramp.elements.remove(
        broad_ramp.color_ramp.elements[1]
    )
    first = broad_ramp.color_ramp.elements[0]
    first.position = broad_colours[0][0]
    first.color = hex_to_linear_rgba(broad_colours[0][1])
    for position, colour in broad_colours[1:]:
        element = broad_ramp.color_ramp.elements.new(position)
        element.color = hex_to_linear_rgba(colour)
    link(tree, broad_noise, "Fac", broad_ramp, "Fac")

    yarn_noise = add_node(
        tree,
        "ShaderNodeTexNoise",
        "IGGY_Colour_YarnNoise",
        (-420, 260),
    )
    yarn_noise.noise_dimensions = "3D"
    yarn_noise.inputs["Scale"].default_value = 3.6
    yarn_noise.inputs["Detail"].default_value = 3.0
    yarn_noise.inputs["Roughness"].default_value = 0.57
    link(tree, colour_coordinate, "Vector", yarn_noise, "Vector")
    yarn_ramp = add_node(
        tree,
        "ShaderNodeValToRGB",
        "IGGY_Colour_YarnVariation",
        (-190, 260),
    )
    yarn_ramp.color_ramp.interpolation = "EASE"
    yarn_colours = (
        (0.00, "#574331"),
        (0.22, "#806244"),
        (0.44, "#b08e63"),
        (0.64, "#d0b68d"),
        (0.82, "#967552"),
        (1.00, "#c4a67c"),
    )
    yarn_ramp.color_ramp.elements.remove(yarn_ramp.color_ramp.elements[1])
    yarn_first = yarn_ramp.color_ramp.elements[0]
    yarn_first.position = yarn_colours[0][0]
    yarn_first.color = hex_to_linear_rgba(yarn_colours[0][1])
    for position, colour in yarn_colours[1:]:
        element = yarn_ramp.color_ramp.elements.new(position)
        element.color = hex_to_linear_rgba(colour)
    link(tree, yarn_noise, "Fac", yarn_ramp, "Fac")

    colour_mix = add_node(
        tree,
        "ShaderNodeMixRGB",
        "IGGY_Colour_TwentyToneBlend",
        (120, 420),
    )
    colour_mix.blend_type = "SOFT_LIGHT"
    colour_mix.inputs["Fac"].default_value = 0.34
    link(tree, broad_ramp, "Color", colour_mix, "Color1")
    link(tree, yarn_ramp, "Color", colour_mix, "Color2")
    link(tree, colour_mix, "Color", principled, "Base Color")
    authored_colour_mix = add_node(
        tree,
        "ShaderNodeMixRGB",
        "IGGY_AuthoredAndProceduralColour",
        (330, 360),
    )
    authored_colour_mix.blend_type = "MIX"
    authored_colour_mix.inputs["Fac"].default_value = 0.78
    link(tree, colour_mix, "Color", authored_colour_mix, "Color1")
    link(tree, basecolour_atlas, "Color", authored_colour_mix, "Color2")

    v_frequency = make_math(
        tree,
        "IGGY_FibreCircumferenceFrequency",
        "MULTIPLY",
        (-610, -40),
        # Absolute sine doubles the visible ridge count.  Forty-three cycles
        # therefore resolve eighty-six fibre ribbons around each yarn/hull
        # instead of reading as a handful of moulded cable grooves.
        second=TAU * 43.0,
    )
    u_wobble_frequency = make_math(
        tree,
        "IGGY_FibreWobbleFrequency",
        "MULTIPLY",
        (-610, -180),
        second=151.0,
    )
    u_wobble_sine = make_math(
        tree,
        "IGGY_FibreWobble",
        "SINE",
        (-420, -180),
    )
    u_wobble_amplitude = make_math(
        tree,
        "IGGY_FibreWobbleAmplitude",
        "MULTIPLY",
        (-240, -180),
        second=0.62,
    )
    fibre_phase = make_math(
        tree,
        "IGGY_FibrePhase",
        "ADD",
        (-30, -40),
    )
    fibre_sine = make_math(
        tree,
        "IGGY_FibreSine",
        "SINE",
        (150, -40),
    )
    fibre_absolute = make_math(
        tree,
        "IGGY_FibreAbsolute",
        "ABSOLUTE",
        (330, -40),
    )
    fibre_periodic_guide = make_math(
        tree,
        "IGGY_FibrePeriodicGuide",
        "POWER",
        (510, -40),
        second=4.2,
    )
    link(tree, v_attribute, "Fac", v_frequency, 0)
    link(tree, u_attribute, "Fac", u_wobble_frequency, 0)
    link(tree, u_wobble_frequency, "Value", u_wobble_sine, 0)
    link(tree, u_wobble_sine, "Value", u_wobble_amplitude, 0)
    link(tree, v_frequency, "Value", fibre_phase, 0)
    link(tree, u_wobble_amplitude, "Value", fibre_phase, 1)
    link(tree, fibre_phase, "Value", fibre_sine, 0)
    link(tree, fibre_sine, "Value", fibre_absolute, 0)
    link(tree, fibre_absolute, "Value", fibre_periodic_guide, 0)

    short_fibre_weight = make_math(
        tree,
        "IGGY_ShortFibreWeight",
        "MULTIPLY",
        (520, -470),
        second=0.78,
    )
    fibre_ridges = make_math(
        tree,
        "IGGY_Fibre_LongitudinalRidges",
        "MAXIMUM",
        (700, -430),
    )
    link(tree, fibre_channels, "Green", short_fibre_weight, 0)
    link(tree, fibre_channels, "Red", fibre_ridges, 0)
    link(tree, short_fibre_weight, "Value", fibre_ridges, 1)

    bump_distance = make_math(
        tree,
        "IGGY_DiameterRelativeFibreRelief",
        "MULTIPLY",
        (310, -190),
        second=0.0018,
    )
    link(tree, diameter_attribute, "Fac", bump_distance, 0)
    bump = add_node(
        tree,
        "ShaderNodeBump",
        "IGGY_FibreBump",
        (690, -60),
    )
    bump.inputs["Strength"].default_value = 0.46
    link(tree, fibre_ridges, "Value", bump, "Height")
    link(tree, bump_distance, "Value", bump, "Distance")
    link(tree, bump, "Normal", principled, "Normal")

    fibre_colour_ramp = add_node(
        tree,
        "ShaderNodeValToRGB",
        "IGGY_FibreColourLinework",
        (500, 185),
    )
    fibre_colour_ramp.color_ramp.interpolation = "EASE"
    fibre_colour_ramp.color_ramp.elements[0].position = 0.08
    fibre_colour_ramp.color_ramp.elements[0].color = hex_to_linear_rgba(
        "#5a402b"
    )
    fibre_colour_ramp.color_ramp.elements[1].position = 0.92
    fibre_colour_ramp.color_ramp.elements[1].color = hex_to_linear_rgba(
        "#c5a276"
    )
    fibre_mid = fibre_colour_ramp.color_ramp.elements.new(0.52)
    fibre_mid.color = hex_to_linear_rgba("#8d6745")
    link(tree, fibre_ridges, "Value", fibre_colour_ramp, "Fac")
    fibre_colour_mix = add_node(
        tree,
        "ShaderNodeMixRGB",
        "IGGY_FibreColourLayer",
        (700, 240),
    )
    fibre_colour_mix.blend_type = "SOFT_LIGHT"
    fibre_colour_mix.inputs["Fac"].default_value = 0.44
    link(
        tree,
        authored_colour_mix,
        "Color",
        fibre_colour_mix,
        "Color1",
    )
    link(tree, fibre_colour_ramp, "Color", fibre_colour_mix, "Color2")
    link(tree, fibre_colour_mix, "Color", principled, "Base Color")

    roughness_noise = add_node(
        tree,
        "ShaderNodeTexNoise",
        "IGGY_RoughnessConstructionNoise",
        (-190, -360),
    )
    roughness_noise.noise_dimensions = "3D"
    roughness_noise.inputs["Scale"].default_value = 2.3
    roughness_noise.inputs["Detail"].default_value = 2.0
    roughness_noise.inputs["Roughness"].default_value = 0.42
    link(tree, colour_coordinate, "Vector", roughness_noise, "Vector")
    roughness_ramp = add_node(
        tree,
        "ShaderNodeValToRGB",
        "IGGY_IndependentRoughness",
        (60, -360),
    )
    roughness_ramp.color_ramp.interpolation = "EASE"
    roughness_ramp.color_ramp.elements[0].position = 0.20
    roughness_ramp.color_ramp.elements[0].color = (0.67, 0.67, 0.67, 1.0)
    roughness_ramp.color_ramp.elements[1].position = 0.82
    roughness_ramp.color_ramp.elements[1].color = (0.86, 0.86, 0.86, 1.0)
    link(tree, roughness_noise, "Fac", roughness_ramp, "Fac")
    link(tree, roughness_ramp, "Color", principled, "Roughness")

    sheen = add_node(
        tree,
        "ShaderNodeValue",
        "IGGY_FibreSheen",
        (600, -300),
    )
    sheen.outputs["Value"].default_value = 0.28
    link(tree, sheen, "Value", principled, "Sheen Weight")
    return material


def create_flyaway_material() -> bpy.types.Material:
    material = bpy.data.materials.new(FLYAWAY_MATERIAL_NAME)
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = hex_to_linear_rgba(
        "#b89569"
    )
    principled.inputs["Roughness"].default_value = 0.78
    principled.inputs["IOR"].default_value = 1.47
    principled.inputs["Sheen Weight"].default_value = 0.34
    principled.inputs["Sheen Roughness"].default_value = 0.68
    return material


def create_floor_material() -> bpy.types.Material:
    material = bpy.data.materials.new(FLOOR_MATERIAL_NAME)
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = (0.012, 0.015, 0.019, 1.0)
    principled.inputs["Roughness"].default_value = 0.82
    principled.inputs["Metallic"].default_value = 0.0
    return material


def create_geometry_group(
    material: bpy.types.Material,
    geometry_contract: dict[str, Any],
) -> bpy.types.NodeTree:
    existing = bpy.data.node_groups.get(GROUP_NAME)
    if existing is not None:
        bpy.data.node_groups.remove(existing, do_unlink=True)
    group = bpy.data.node_groups.new(GROUP_NAME, "GeometryNodeTree")
    group.color_tag = "GEOMETRY"
    group.description = (
        "Metre-stable three-strand plant-fibre rope. Twenty-one yarn curves "
        "are packed as three tangent one-plus-six bundles. Rope and yarn lays "
        "use opposite signs and physical Spline Parameter length."
    )

    new_interface_socket(
        group,
        "Geometry",
        "INPUT",
        "NodeSocketGeometry",
    )
    new_interface_socket(
        group,
        "Diameter",
        "INPUT",
        "NodeSocketFloat",
        default=0.032,
        minimum=0.004,
        maximum=0.16,
    )
    new_interface_socket(
        group,
        "Lay Length",
        "INPUT",
        "NodeSocketFloat",
        default=0.09,
        minimum=0.012,
        maximum=0.60,
    )
    new_interface_socket(
        group,
        "Yarn Counterturn",
        "INPUT",
        "NodeSocketFloat",
        default=geometry_contract["yarn_counterturn_ratio_default"],
        minimum=geometry_contract["yarn_counterturn_ratio_min"],
        maximum=geometry_contract["yarn_counterturn_ratio_max"],
    )
    new_interface_socket(
        group,
        "Sample Length",
        "INPUT",
        "NodeSocketFloat",
        default=0.003,
        minimum=0.0004,
        maximum=0.02,
    )
    new_interface_socket(
        group,
        "Profile Resolution",
        "INPUT",
        "NodeSocketInt",
        default=10,
        minimum=6,
        maximum=24,
    )
    new_interface_socket(
        group,
        "Material",
        "INPUT",
        "NodeSocketMaterial",
        default=material,
    )
    new_interface_socket(
        group,
        "Geometry",
        "OUTPUT",
        "NodeSocketGeometry",
    )

    tree = group
    group_input = add_node(
        tree,
        "NodeGroupInput",
        "IGGY_RopeInputs",
        (-2380, 360),
    )
    group_output = add_node(
        tree,
        "NodeGroupOutput",
        "IGGY_RopeOutput",
        (2440, 320),
    )

    resample = add_node(
        tree,
        "GeometryNodeResampleCurve",
        "IGGY_ResampleByMetres",
        (-2160, 560),
    )
    # Blender 5 exposes Resample Curve's choice as a menu socket rather than
    # the legacy node.mode RNA property.  Choosing it explicitly keeps the
    # physical sampling step in metres instead of silently falling back to a
    # point count.
    resample.inputs["Mode"].default_value = "Length"
    link(tree, group_input, "Geometry", resample, "Curve")
    link(tree, group_input, "Sample Length", resample, "Length")

    path_parameter = add_node(
        tree,
        "GeometryNodeSplineParameter",
        "IGGY_PathLengthMetres",
        (-2150, 820),
    )
    store_u = store_attribute(
        tree,
        "IGGY_StoreLongitudinalMetres",
        (-1920, 560),
        attribute_name="sinc_rope_u_m",
        data_type="FLOAT",
        domain="POINT",
    )
    link(tree, resample, "Curve", store_u, "Geometry")
    link(tree, path_parameter, "Length", store_u, "Value")

    duplicate = add_node(
        tree,
        "GeometryNodeDuplicateElements",
        "IGGY_Duplicate21Yarns",
        (-1680, 560),
    )
    duplicate.domain = "SPLINE"
    duplicate.inputs["Amount"].default_value = TOTAL_YARNS
    link(tree, store_u, "Geometry", duplicate, "Geometry")

    strand_index = make_integer_math(
        tree,
        "IGGY_StrandIndex",
        "DIVIDE_FLOOR",
        (-1660, 1020),
        second=YARNS_PER_STRAND,
    )
    yarn_index = make_integer_math(
        tree,
        "IGGY_YarnIndex",
        "MODULO",
        (-1660, 900),
        second=YARNS_PER_STRAND,
    )
    link(tree, duplicate, "Duplicate Index", strand_index, 0)
    link(tree, duplicate, "Duplicate Index", yarn_index, 0)

    store_strand = store_attribute(
        tree,
        "IGGY_StoreStrandIdentity",
        (-1430, 600),
        attribute_name="sinc_rope_strand_id",
        data_type="INT",
        domain="CURVE",
    )
    store_yarn = store_attribute(
        tree,
        "IGGY_StoreYarnIdentity",
        (-1210, 600),
        attribute_name="sinc_rope_yarn_id",
        data_type="INT",
        domain="CURVE",
    )
    store_diameter = store_attribute(
        tree,
        "IGGY_StoreDiameterMetres",
        (-990, 600),
        attribute_name="sinc_rope_diameter_m",
        data_type="FLOAT",
        domain="CURVE",
    )
    link(tree, duplicate, "Geometry", store_strand, "Geometry")
    link(tree, strand_index, "Value", store_strand, "Value")
    link(tree, store_strand, "Geometry", store_yarn, "Geometry")
    link(tree, yarn_index, "Value", store_yarn, "Value")
    link(tree, store_yarn, "Geometry", store_diameter, "Geometry")
    link(tree, group_input, "Diameter", store_diameter, "Value")

    u_attribute = named_attribute(
        tree,
        "IGGY_ReadLongitudinalMetres",
        (-1430, 1230),
        attribute_name="sinc_rope_u_m",
        data_type="FLOAT",
    )
    rope_rate = make_math(
        tree,
        "IGGY_RopeRadiansPerMetre",
        "DIVIDE",
        (-1210, 1320),
    )
    rope_rate.inputs[0].default_value = TAU
    link(tree, group_input, "Lay Length", rope_rate, 1)
    rope_turn = make_math(
        tree,
        "IGGY_RopeTurnFromDistance",
        "MULTIPLY",
        (-1010, 1260),
    )
    link(tree, u_attribute, "Attribute", rope_turn, 0)
    link(tree, rope_rate, "Value", rope_turn, 1)
    strand_phase = make_math(
        tree,
        "IGGY_ThreeStrandPhase",
        "MULTIPLY",
        (-1210, 1100),
        second=TAU / STRAND_COUNT,
    )
    link(tree, strand_index, "Value", strand_phase, 0)
    rope_angle_regular = make_math(
        tree,
        "IGGY_RopeAngleRegular",
        "ADD",
        (-810, 1190),
    )
    link(tree, rope_turn, "Value", rope_angle_regular, 0)
    link(tree, strand_phase, "Value", rope_angle_regular, 1)
    drift_slow_rate = make_math(
        tree,
        "IGGY_LayDriftSlowRate",
        "MULTIPLY",
        (-1010, 1510),
        second=0.173,
    )
    drift_slow_phase = make_math(
        tree,
        "IGGY_LayDriftSlowStrandPhase",
        "MULTIPLY",
        (-1010, 1410),
        second=1.317,
    )
    drift_slow_sum = make_math(
        tree,
        "IGGY_LayDriftSlowCoordinate",
        "ADD",
        (-810, 1480),
    )
    drift_slow_sine = make_math(
        tree,
        "IGGY_LayDriftSlowSine",
        "SINE",
        (-620, 1480),
    )
    drift_slow = make_math(
        tree,
        "IGGY_LayDriftSlowAmplitude",
        "MULTIPLY",
        (-440, 1480),
        second=0.038,
    )
    link(tree, rope_turn, "Value", drift_slow_rate, 0)
    link(tree, strand_index, "Value", drift_slow_phase, 0)
    link(tree, drift_slow_rate, "Value", drift_slow_sum, 0)
    link(tree, drift_slow_phase, "Value", drift_slow_sum, 1)
    link(tree, drift_slow_sum, "Value", drift_slow_sine, 0)
    link(tree, drift_slow_sine, "Value", drift_slow, 0)

    drift_fast_rate = make_math(
        tree,
        "IGGY_LayDriftFastRate",
        "MULTIPLY",
        (-810, 1590),
        second=0.619,
    )
    drift_fast_phase = make_math(
        tree,
        "IGGY_LayDriftFastStrandPhase",
        "MULTIPLY",
        (-810, 1690),
        second=2.113,
    )
    drift_fast_sum = make_math(
        tree,
        "IGGY_LayDriftFastCoordinate",
        "ADD",
        (-620, 1640),
    )
    drift_fast_sine = make_math(
        tree,
        "IGGY_LayDriftFastSine",
        "SINE",
        (-440, 1640),
    )
    drift_fast = make_math(
        tree,
        "IGGY_LayDriftFastAmplitude",
        "MULTIPLY",
        (-260, 1640),
        second=0.014,
    )
    link(tree, rope_turn, "Value", drift_fast_rate, 0)
    link(tree, strand_index, "Value", drift_fast_phase, 0)
    link(tree, drift_fast_rate, "Value", drift_fast_sum, 0)
    link(tree, drift_fast_phase, "Value", drift_fast_sum, 1)
    link(tree, drift_fast_sum, "Value", drift_fast_sine, 0)
    link(tree, drift_fast_sine, "Value", drift_fast, 0)
    drift_sum = make_math(
        tree,
        "IGGY_LayDriftCombined",
        "ADD",
        (-80, 1550),
    )
    rope_angle = make_math(
        tree,
        "IGGY_RopeAngleFromMetres",
        "ADD",
        (100, 1440),
    )
    link(tree, drift_slow, "Value", drift_sum, 0)
    link(tree, drift_fast, "Value", drift_sum, 1)
    link(tree, rope_angle_regular, "Value", rope_angle, 0)
    link(tree, drift_sum, "Value", rope_angle, 1)

    counterturn = make_math(
        tree,
        "IGGY_YarnCounterturnAmount",
        "MULTIPLY",
        (-810, 1030),
    )
    link(tree, rope_turn, "Value", counterturn, 0)
    link(tree, group_input, "Yarn Counterturn", counterturn, 1)
    opposed_turn = make_math(
        tree,
        "IGGY_OpposedYarnTurn",
        "SUBTRACT",
        (-610, 1110),
    )
    link(tree, rope_angle, "Value", opposed_turn, 0)
    link(tree, counterturn, "Value", opposed_turn, 1)
    outer_yarn_index = make_integer_math(
        tree,
        "IGGY_OuterYarnIndex",
        "SUBTRACT",
        (-1010, 900),
        second=1,
    )
    link(tree, yarn_index, "Value", outer_yarn_index, 0)
    yarn_phase = make_math(
        tree,
        "IGGY_SixYarnRingPhase",
        "MULTIPLY",
        (-810, 900),
        second=TAU / 6.0,
    )
    link(tree, outer_yarn_index, "Value", yarn_phase, 0)
    opposed_yarn_angle = make_math(
        tree,
        "IGGY_OpposedYarnAngle",
        "ADD",
        (-410, 1040),
    )
    link(tree, opposed_turn, "Value", opposed_yarn_angle, 0)
    link(tree, yarn_phase, "Value", opposed_yarn_angle, 1)

    strand_radius_fraction = geometry_contract["packing"][
        "strand_radius_diameter_fraction"
    ]
    strand_center_fraction = geometry_contract["packing"][
        "strand_center_diameter_fraction"
    ]
    yarn_radius_fraction = (
        strand_radius_fraction
        * geometry_contract["packing"][
            "yarn_radius_strand_radius_fraction"
        ]
    )
    yarn_ring_fraction = (
        strand_radius_fraction
        * geometry_contract["packing"][
            "outer_yarn_center_strand_radius_fraction"
        ]
    )
    strand_center_radius = make_math(
        tree,
        "IGGY_TangentStrandCentreRadius",
        "MULTIPLY",
        (-610, 830),
        second=strand_center_fraction,
    )
    yarn_ring_radius = make_math(
        tree,
        "IGGY_OuterYarnRingRadius",
        "MULTIPLY",
        (-610, 710),
        second=yarn_ring_fraction,
    )
    profile_radius = make_math(
        tree,
        "IGGY_YarnProfileRadius",
        "MULTIPLY",
        (610, 10),
        second=yarn_radius_fraction,
    )
    link(tree, group_input, "Diameter", strand_center_radius, 0)
    link(tree, group_input, "Diameter", yarn_ring_radius, 0)
    link(tree, group_input, "Diameter", profile_radius, 0)

    non_core = add_node(
        tree,
        "FunctionNodeCompare",
        "IGGY_OuterYarnSelection",
        (-610, 570),
    )
    non_core.data_type = "INT"
    non_core.operation = "GREATER_THAN"
    non_core.inputs[3].default_value = 0
    link(tree, yarn_index, "Value", non_core, 2)
    ring_switch = add_node(
        tree,
        "GeometryNodeSwitch",
        "IGGY_CoreOrOuterYarnRadius",
        (-380, 710),
    )
    ring_switch.input_type = "FLOAT"
    ring_switch.inputs["False"].default_value = 0.0
    link(tree, non_core, "Result", ring_switch, "Switch")
    link(tree, yarn_ring_radius, "Value", ring_switch, "True")

    id_phase = make_math(
        tree,
        "IGGY_YarnPackingVariationPhase",
        "MULTIPLY",
        (-610, 430),
        second=1.61803398875,
    )
    id_sine = make_math(
        tree,
        "IGGY_YarnPackingVariationSine",
        "SINE",
        (-410, 430),
    )
    id_amplitude = make_math(
        tree,
        "IGGY_YarnPackingVariationAmplitude",
        "MULTIPLY",
        (-210, 430),
        second=0.022,
    )
    id_multiplier = make_math(
        tree,
        "IGGY_YarnPackingVariationMultiplier",
        "ADD",
        (-20, 430),
        second=1.0,
    )
    link(tree, duplicate, "Duplicate Index", id_phase, 0)
    link(tree, id_phase, "Value", id_sine, 0)
    link(tree, id_sine, "Value", id_amplitude, 0)
    link(tree, id_amplitude, "Value", id_multiplier, 0)
    varied_ring_radius = make_math(
        tree,
        "IGGY_VariedOuterYarnRadius",
        "MULTIPLY",
        (180, 680),
    )
    link(tree, ring_switch, "Output", varied_ring_radius, 0)
    link(tree, id_multiplier, "Value", varied_ring_radius, 1)

    rope_cos = make_math(
        tree,
        "IGGY_RopeAngleCosine",
        "COSINE",
        (-200, 1260),
    )
    rope_sin = make_math(
        tree,
        "IGGY_RopeAngleSine",
        "SINE",
        (-200, 1160),
    )
    yarn_cos = make_math(
        tree,
        "IGGY_YarnAngleCosine",
        "COSINE",
        (-200, 1020),
    )
    yarn_sin = make_math(
        tree,
        "IGGY_YarnAngleSine",
        "SINE",
        (-200, 920),
    )
    link(tree, rope_angle, "Value", rope_cos, 0)
    link(tree, rope_angle, "Value", rope_sin, 0)
    link(tree, opposed_yarn_angle, "Value", yarn_cos, 0)
    link(tree, opposed_yarn_angle, "Value", yarn_sin, 0)

    strand_x = make_math(
        tree,
        "IGGY_StrandOffsetNormal",
        "MULTIPLY",
        (20, 1260),
    )
    strand_y = make_math(
        tree,
        "IGGY_StrandOffsetBinormal",
        "MULTIPLY",
        (20, 1160),
    )
    yarn_x = make_math(
        tree,
        "IGGY_YarnOffsetNormal",
        "MULTIPLY",
        (20, 1020),
    )
    yarn_y = make_math(
        tree,
        "IGGY_YarnOffsetBinormal",
        "MULTIPLY",
        (20, 920),
    )
    link(tree, rope_cos, "Value", strand_x, 0)
    link(tree, strand_center_radius, "Value", strand_x, 1)
    link(tree, rope_sin, "Value", strand_y, 0)
    link(tree, strand_center_radius, "Value", strand_y, 1)
    link(tree, yarn_cos, "Value", yarn_x, 0)
    link(tree, varied_ring_radius, "Value", yarn_x, 1)
    link(tree, yarn_sin, "Value", yarn_y, 0)
    link(tree, varied_ring_radius, "Value", yarn_y, 1)
    offset_x = make_math(
        tree,
        "IGGY_CombinedNormalOffset",
        "ADD",
        (230, 1200),
    )
    offset_y = make_math(
        tree,
        "IGGY_CombinedBinormalOffset",
        "ADD",
        (230, 990),
    )
    link(tree, strand_x, "Value", offset_x, 0)
    link(tree, yarn_x, "Value", offset_x, 1)
    link(tree, strand_y, "Value", offset_y, 0)
    link(tree, yarn_y, "Value", offset_y, 1)

    tangent = add_node(
        tree,
        "GeometryNodeInputTangent",
        "IGGY_PathTangent",
        (210, 780),
    )
    normal = add_node(
        tree,
        "GeometryNodeInputNormal",
        "IGGY_PathNormal",
        (210, 650),
    )
    binormal = add_node(
        tree,
        "ShaderNodeVectorMath",
        "IGGY_ParallelTransportBinormal",
        (440, 720),
    )
    binormal.operation = "CROSS_PRODUCT"
    link(tree, tangent, "Tangent", binormal, 0)
    link(tree, normal, "Normal", binormal, 1)
    normal_scale = add_node(
        tree,
        "ShaderNodeVectorMath",
        "IGGY_ScalePathNormal",
        (450, 1190),
    )
    normal_scale.operation = "SCALE"
    binormal_scale = add_node(
        tree,
        "ShaderNodeVectorMath",
        "IGGY_ScalePathBinormal",
        (450, 990),
    )
    binormal_scale.operation = "SCALE"
    link(tree, normal, "Normal", normal_scale, "Vector")
    link(tree, offset_x, "Value", normal_scale, "Scale")
    link(tree, binormal, "Vector", binormal_scale, "Vector")
    link(tree, offset_y, "Value", binormal_scale, "Scale")
    combined_offset = add_node(
        tree,
        "ShaderNodeVectorMath",
        "IGGY_NestedHelixOffset",
        (680, 1110),
    )
    combined_offset.operation = "ADD"
    link(tree, normal_scale, "Vector", combined_offset, 0)
    link(tree, binormal_scale, "Vector", combined_offset, 1)

    set_position = add_node(
        tree,
        "GeometryNodeSetPosition",
        "IGGY_PositionNestedYarns",
        (700, 610),
    )
    link(tree, store_diameter, "Geometry", set_position, "Geometry")
    link(tree, combined_offset, "Vector", set_position, "Offset")
    set_radius = add_node(
        tree,
        "GeometryNodeSetCurveRadius",
        "IGGY_VaryYarnRadius",
        (920, 610),
    )
    link(tree, set_position, "Geometry", set_radius, "Curve")
    link(tree, id_multiplier, "Value", set_radius, "Radius")

    profile_circle = add_node(
        tree,
        "GeometryNodeCurvePrimitiveCircle",
        "IGGY_YarnProfileCircle",
        (700, 160),
    )
    profile_circle.mode = "RADIUS"
    link(tree, group_input, "Profile Resolution", profile_circle, "Resolution")
    link(tree, profile_radius, "Value", profile_circle, "Radius")
    profile_parameter = add_node(
        tree,
        "GeometryNodeSplineParameter",
        "IGGY_ProfileParameter",
        (700, -80),
    )
    store_profile_turn = store_attribute(
        tree,
        "IGGY_StoreProfileTurn",
        (930, 160),
        attribute_name="sinc_rope_v_turn",
        data_type="FLOAT",
        domain="POINT",
    )
    link(tree, profile_circle, "Curve", store_profile_turn, "Geometry")
    link(tree, profile_parameter, "Factor", store_profile_turn, "Value")

    curve_to_mesh = add_node(
        tree,
        "GeometryNodeCurveToMesh",
        "IGGY_CurveToYarnMesh",
        (1170, 560),
    )
    curve_to_mesh.inputs["Fill Caps"].default_value = True
    link(tree, set_radius, "Curve", curve_to_mesh, "Curve")
    link(tree, store_profile_turn, "Geometry", curve_to_mesh, "Profile Curve")

    mesh_u = named_attribute(
        tree,
        "IGGY_ReadMeshLongitudinalMetres",
        (1170, 300),
        attribute_name="sinc_rope_u_m",
        data_type="FLOAT",
    )
    mesh_v = named_attribute(
        tree,
        "IGGY_ReadMeshProfileTurn",
        (1170, 170),
        attribute_name="sinc_rope_v_turn",
        data_type="FLOAT",
    )
    uv_u = make_math(
        tree,
        "IGGY_UVTurnsPerLay",
        "DIVIDE",
        (1380, 300),
    )
    link(tree, mesh_u, "Attribute", uv_u, 0)
    link(tree, group_input, "Lay Length", uv_u, 1)
    uv_vector = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        "IGGY_CombineRopeUV",
        (1580, 240),
    )
    link(tree, uv_u, "Value", uv_vector, "X")
    link(tree, mesh_v, "Attribute", uv_vector, "Y")
    store_uv = store_attribute(
        tree,
        "IGGY_StoreFaceCornerUV",
        (1580, 560),
        attribute_name="rope_uv",
        data_type="FLOAT2",
        domain="CORNER",
    )
    link(tree, curve_to_mesh, "Mesh", store_uv, "Geometry")
    link(tree, uv_vector, "Vector", store_uv, "Value")

    set_material = add_node(
        tree,
        "GeometryNodeSetMaterial",
        "IGGY_AssignPlantFibreMaterial",
        (1810, 560),
    )
    link(tree, store_uv, "Geometry", set_material, "Geometry")
    link(tree, group_input, "Material", set_material, "Material")
    shade_smooth = add_node(
        tree,
        "GeometryNodeSetShadeSmooth",
        "IGGY_SmoothYarnFaces",
        (2040, 560),
    )
    shade_smooth.domain = "FACE"
    shade_smooth.inputs["Shade Smooth"].default_value = True
    link(tree, set_material, "Geometry", shade_smooth, "Mesh")

    # A laid strand is a compacted body, not seven independent round wires.
    # The yarn branch above is retained for crown breakup and identity, while
    # these three continuous hulls occupy the compressed mass beneath it.
    # Their radius is deliberately just below the yarn crowns so the final
    # silhouette reads as three strands with fine bundled relief.
    strand_hulls = add_node(
        tree,
        "GeometryNodeDuplicateElements",
        "IGGY_Duplicate3StrandHulls",
        (-1030, -760),
    )
    strand_hulls.domain = "SPLINE"
    strand_hulls.inputs["Amount"].default_value = STRAND_COUNT
    link(tree, store_u, "Geometry", strand_hulls, "Geometry")

    hull_store_strand = store_attribute(
        tree,
        "IGGY_StoreHullStrandIdentity",
        (-820, -760),
        attribute_name="sinc_rope_strand_id",
        data_type="INT",
        domain="CURVE",
    )
    hull_store_diameter = store_attribute(
        tree,
        "IGGY_StoreHullDiameterMetres",
        (-610, -760),
        attribute_name="sinc_rope_diameter_m",
        data_type="FLOAT",
        domain="CURVE",
    )
    link(tree, strand_hulls, "Geometry", hull_store_strand, "Geometry")
    link(
        tree,
        strand_hulls,
        "Duplicate Index",
        hull_store_strand,
        "Value",
    )
    link(
        tree,
        hull_store_strand,
        "Geometry",
        hull_store_diameter,
        "Geometry",
    )
    link(tree, group_input, "Diameter", hull_store_diameter, "Value")

    hull_phase = make_math(
        tree,
        "IGGY_HullThreeStrandPhase",
        "MULTIPLY",
        (-810, -1010),
        second=TAU / STRAND_COUNT,
    )
    hull_angle_regular = make_math(
        tree,
        "IGGY_HullRopeAngleRegular",
        "ADD",
        (-590, -1010),
    )
    link(tree, strand_hulls, "Duplicate Index", hull_phase, 0)
    link(tree, rope_turn, "Value", hull_angle_regular, 0)
    link(tree, hull_phase, "Value", hull_angle_regular, 1)
    hull_drift_slow_phase = make_math(
        tree,
        "IGGY_HullLayDriftSlowStrandPhase",
        "MULTIPLY",
        (-810, -1370),
        second=1.317,
    )
    hull_drift_slow_sum = make_math(
        tree,
        "IGGY_HullLayDriftSlowCoordinate",
        "ADD",
        (-590, -1370),
    )
    hull_drift_slow_sine = make_math(
        tree,
        "IGGY_HullLayDriftSlowSine",
        "SINE",
        (-390, -1370),
    )
    hull_drift_slow = make_math(
        tree,
        "IGGY_HullLayDriftSlowAmplitude",
        "MULTIPLY",
        (-190, -1370),
        second=0.038,
    )
    link(
        tree,
        strand_hulls,
        "Duplicate Index",
        hull_drift_slow_phase,
        0,
    )
    link(tree, drift_slow_rate, "Value", hull_drift_slow_sum, 0)
    link(
        tree,
        hull_drift_slow_phase,
        "Value",
        hull_drift_slow_sum,
        1,
    )
    link(
        tree,
        hull_drift_slow_sum,
        "Value",
        hull_drift_slow_sine,
        0,
    )
    link(
        tree,
        hull_drift_slow_sine,
        "Value",
        hull_drift_slow,
        0,
    )
    hull_drift_fast_phase = make_math(
        tree,
        "IGGY_HullLayDriftFastStrandPhase",
        "MULTIPLY",
        (20, -1450),
        second=2.113,
    )
    hull_drift_fast_sum = make_math(
        tree,
        "IGGY_HullLayDriftFastCoordinate",
        "ADD",
        (220, -1450),
    )
    hull_drift_fast_sine = make_math(
        tree,
        "IGGY_HullLayDriftFastSine",
        "SINE",
        (420, -1450),
    )
    hull_drift_fast = make_math(
        tree,
        "IGGY_HullLayDriftFastAmplitude",
        "MULTIPLY",
        (620, -1450),
        second=0.014,
    )
    link(
        tree,
        strand_hulls,
        "Duplicate Index",
        hull_drift_fast_phase,
        0,
    )
    link(tree, drift_fast_rate, "Value", hull_drift_fast_sum, 0)
    link(
        tree,
        hull_drift_fast_phase,
        "Value",
        hull_drift_fast_sum,
        1,
    )
    link(
        tree,
        hull_drift_fast_sum,
        "Value",
        hull_drift_fast_sine,
        0,
    )
    link(
        tree,
        hull_drift_fast_sine,
        "Value",
        hull_drift_fast,
        0,
    )
    hull_drift_sum = make_math(
        tree,
        "IGGY_HullLayDriftCombined",
        "ADD",
        (820, -1390),
    )
    hull_angle = make_math(
        tree,
        "IGGY_HullRopeAngleFromMetres",
        "ADD",
        (1020, -1370),
    )
    link(tree, hull_drift_slow, "Value", hull_drift_sum, 0)
    link(tree, hull_drift_fast, "Value", hull_drift_sum, 1)
    link(tree, hull_angle_regular, "Value", hull_angle, 0)
    link(tree, hull_drift_sum, "Value", hull_angle, 1)

    hull_cos = make_math(
        tree,
        "IGGY_HullAngleCosine",
        "COSINE",
        (-390, -1050),
    )
    hull_sin = make_math(
        tree,
        "IGGY_HullAngleSine",
        "SINE",
        (-390, -930),
    )
    link(tree, hull_angle, "Value", hull_cos, 0)
    link(tree, hull_angle, "Value", hull_sin, 0)
    hull_offset_normal = make_math(
        tree,
        "IGGY_HullOffsetNormal",
        "MULTIPLY",
        (-190, -1050),
    )
    hull_offset_binormal = make_math(
        tree,
        "IGGY_HullOffsetBinormal",
        "MULTIPLY",
        (-190, -930),
    )
    link(tree, hull_cos, "Value", hull_offset_normal, 0)
    link(
        tree,
        strand_center_radius,
        "Value",
        hull_offset_normal,
        1,
    )
    link(tree, hull_sin, "Value", hull_offset_binormal, 0)
    link(
        tree,
        strand_center_radius,
        "Value",
        hull_offset_binormal,
        1,
    )

    hull_normal_scale = add_node(
        tree,
        "ShaderNodeVectorMath",
        "IGGY_ScaleHullNormal",
        (20, -1050),
    )
    hull_normal_scale.operation = "SCALE"
    hull_binormal_scale = add_node(
        tree,
        "ShaderNodeVectorMath",
        "IGGY_ScaleHullBinormal",
        (20, -930),
    )
    hull_binormal_scale.operation = "SCALE"
    link(tree, normal, "Normal", hull_normal_scale, "Vector")
    link(tree, hull_offset_normal, "Value", hull_normal_scale, "Scale")
    link(tree, binormal, "Vector", hull_binormal_scale, "Vector")
    link(
        tree,
        hull_offset_binormal,
        "Value",
        hull_binormal_scale,
        "Scale",
    )
    hull_offset = add_node(
        tree,
        "ShaderNodeVectorMath",
        "IGGY_CompactStrandHullOffset",
        (250, -990),
    )
    hull_offset.operation = "ADD"
    link(tree, hull_normal_scale, "Vector", hull_offset, 0)
    link(tree, hull_binormal_scale, "Vector", hull_offset, 1)

    hull_position = add_node(
        tree,
        "GeometryNodeSetPosition",
        "IGGY_PositionStrandHulls",
        (260, -760),
    )
    link(tree, hull_store_diameter, "Geometry", hull_position, "Geometry")
    link(tree, hull_offset, "Vector", hull_position, "Offset")

    hull_profile_resolution = make_integer_math(
        tree,
        "IGGY_HullProfileResolution",
        "MULTIPLY",
        (470, -1210),
        second=2,
    )
    link(
        tree,
        group_input,
        "Profile Resolution",
        hull_profile_resolution,
        0,
    )
    hull_profile_radius = make_math(
        tree,
        "IGGY_CompactStrandHullRadius",
        "MULTIPLY",
        (470, -1110),
        second=geometry_contract["packing"][
            "strand_hull_radius_diameter_fraction"
        ],
    )
    link(tree, group_input, "Diameter", hull_profile_radius, 0)
    hull_profile = add_node(
        tree,
        "GeometryNodeCurvePrimitiveCircle",
        "IGGY_StrandHullProfile",
        (680, -1110),
    )
    hull_profile.mode = "RADIUS"
    link(
        tree,
        hull_profile_resolution,
        "Value",
        hull_profile,
        "Resolution",
    )
    link(tree, hull_profile_radius, "Value", hull_profile, "Radius")
    hull_profile_parameter = add_node(
        tree,
        "GeometryNodeSplineParameter",
        "IGGY_HullProfileParameter",
        (680, -1310),
    )
    hull_store_profile_turn = store_attribute(
        tree,
        "IGGY_StoreHullProfileTurn",
        (900, -1110),
        attribute_name="sinc_rope_v_turn",
        data_type="FLOAT",
        domain="POINT",
    )
    link(
        tree,
        hull_profile,
        "Curve",
        hull_store_profile_turn,
        "Geometry",
    )
    link(
        tree,
        hull_profile_parameter,
        "Factor",
        hull_store_profile_turn,
        "Value",
    )
    hull_mesh = add_node(
        tree,
        "GeometryNodeCurveToMesh",
        "IGGY_CurveToStrandHull",
        (1120, -760),
    )
    hull_mesh.inputs["Fill Caps"].default_value = True
    link(tree, hull_position, "Geometry", hull_mesh, "Curve")
    link(
        tree,
        hull_store_profile_turn,
        "Geometry",
        hull_mesh,
        "Profile Curve",
    )
    hull_store_uv = store_attribute(
        tree,
        "IGGY_StoreHullFaceCornerUV",
        (1340, -760),
        attribute_name="rope_uv",
        data_type="FLOAT2",
        domain="CORNER",
    )
    link(tree, hull_mesh, "Mesh", hull_store_uv, "Geometry")
    link(tree, uv_vector, "Vector", hull_store_uv, "Value")
    hull_material = add_node(
        tree,
        "GeometryNodeSetMaterial",
        "IGGY_AssignStrandHullMaterial",
        (1560, -760),
    )
    link(tree, hull_store_uv, "Geometry", hull_material, "Geometry")
    link(tree, group_input, "Material", hull_material, "Material")
    hull_smooth = add_node(
        tree,
        "GeometryNodeSetShadeSmooth",
        "IGGY_SmoothStrandHullFaces",
        (1780, -760),
    )
    hull_smooth.domain = "FACE"
    hull_smooth.inputs["Shade Smooth"].default_value = True
    link(tree, hull_material, "Geometry", hull_smooth, "Mesh")

    join_geometry = add_node(
        tree,
        "GeometryNodeJoinGeometry",
        "IGGY_JoinStrandHullAndYarns",
        (2240, 380),
    )
    link(tree, shade_smooth, "Mesh", join_geometry, "Geometry")
    link(tree, hull_smooth, "Mesh", join_geometry, "Geometry")
    link(tree, join_geometry, "Geometry", group_output, "Geometry")
    return group


def preset_by_id(pattern: dict[str, Any], preset_id: str) -> dict[str, Any]:
    return next(
        preset for preset in pattern["presets"] if preset["id"] == preset_id
    )


def rope_path_function(
    preset_id: str,
    length: float,
) -> Callable[[float], Vector]:
    if preset_id == "fine_lashing":
        def point(t: float) -> Vector:
            x = (t - 0.5) * length
            y = 0.010 * math.sin(TAU * (t + 0.08))
            z = 0.013 * math.sin(math.pi * t) + 0.004 * math.sin(TAU * 1.7 * t)
            return Vector((x, y, z))
        return point
    if preset_id == "heavy_hawser":
        def point(t: float) -> Vector:
            x = (t - 0.5) * length
            y = 0.026 * math.sin(TAU * (t + 0.14))
            z = 0.045 * math.sin(math.pi * t) + 0.012 * math.sin(TAU * 1.3 * t)
            return Vector((x, y, z))
        return point

    def point(t: float) -> Vector:
        x = (t - 0.5) * length
        y = 0.021 * math.sin(TAU * (t + 0.06))
        z = 0.034 * math.sin(math.pi * t) + 0.009 * math.sin(TAU * 1.5 * t)
        return Vector((x, y, z))
    return point


def create_curve_object(
    collection: bpy.types.Collection,
    name: str,
    point_function: Callable[[float], Vector],
    *,
    sample_count: int = 72,
) -> bpy.types.Object:
    curve = bpy.data.curves.new(name + "_Curve", "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 8
    curve.render_resolution_u = 10
    curve.twist_mode = "MINIMUM"
    spline = curve.splines.new("POLY")
    spline.points.add(sample_count - 1)
    for index, point in enumerate(spline.points):
        coordinate = point_function(index / (sample_count - 1))
        point.co = (*coordinate, 1.0)
    obj = bpy.data.objects.new(name, curve)
    collection.objects.link(obj)
    return obj


def interface_input(
    group: bpy.types.NodeTree,
    name: str,
) -> bpy.types.NodeTreeInterfaceSocket:
    return next(
        item
        for item in group.interface.items_tree
        if item.item_type == "SOCKET"
        and item.in_out == "INPUT"
        and item.name == name
    )


def set_modifier_input(
    modifier: bpy.types.Modifier,
    group: bpy.types.NodeTree,
    name: str,
    value: Any,
) -> None:
    modifier[interface_input(group, name).identifier] = value


def attach_rope_group(
    obj: bpy.types.Object,
    group: bpy.types.NodeTree,
    material: bpy.types.Material,
    preset: dict[str, Any],
    *,
    profile_resolution: int,
) -> None:
    diameter = float(preset["diameter_m"])
    lay_length = float(preset["lay_length_m"])
    samples_per_lay = 34.0
    sample_length = min(lay_length / samples_per_lay, diameter / 9.0)
    modifier = obj.modifiers.new("IGGY_RopePlantFibre", "NODES")
    modifier.node_group = group
    set_modifier_input(modifier, group, "Diameter", diameter)
    set_modifier_input(modifier, group, "Lay Length", lay_length)
    set_modifier_input(modifier, group, "Yarn Counterturn", 1.14)
    set_modifier_input(modifier, group, "Sample Length", sample_length)
    set_modifier_input(
        modifier,
        group,
        "Profile Resolution",
        profile_resolution,
    )
    set_modifier_input(modifier, group, "Material", material)
    obj["iggy_geometry_schema"] = "iggy3d.geometry.rope_plant_fibre_v1.v1"
    obj["sinc_rope_preset_id"] = preset["id"]
    obj["sinc_rope_diameter_m"] = diameter
    obj["sinc_rope_lay_length_m"] = lay_length
    obj["sinc_rope_lay_sign"] = 1
    obj["sinc_yarn_lay_sign"] = -1
    obj["sinc_yarn_counterturn_ratio"] = 1.14
    obj["sinc_rope_strand_count"] = STRAND_COUNT
    obj["sinc_rope_yarns_per_strand"] = YARNS_PER_STRAND
    obj["sinc_rope_sample_length_m"] = sample_length
    obj.data.materials.append(material)


def path_frame(
    point_function: Callable[[float], Vector],
    t: float,
) -> tuple[Vector, Vector, Vector]:
    epsilon = 0.0005
    before = point_function(max(0.0, t - epsilon))
    after = point_function(min(1.0, t + epsilon))
    tangent = (after - before).normalized()
    up = Vector((0.0, 0.0, 1.0))
    if abs(tangent.dot(up)) > 0.94:
        up = Vector((0.0, 1.0, 0.0))
    normal = tangent.cross(up).normalized()
    binormal = tangent.cross(normal).normalized()
    return tangent, normal, binormal


def create_sparse_flyaways(
    collection: bpy.types.Collection,
    point_function: Callable[[float], Vector],
    *,
    diameter: float,
    material: bpy.types.Material,
    seed: int,
) -> bpy.types.Object:
    rng = random.Random(seed)
    curve = bpy.data.curves.new("IGGY_RopeFlyaways_UtilityHero_Curve", "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 2
    curve.render_resolution_u = 3
    curve.bevel_depth = diameter * 0.0032
    curve.bevel_resolution = 1
    curve.resolution_u = 2
    curve.materials.append(material)
    for fibre_index in range(34):
        start_t = rng.uniform(0.04, 0.94)
        span = rng.uniform(0.006, 0.022)
        direction = -1.0 if fibre_index % 5 == 0 else 1.0
        angle = rng.uniform(0.0, TAU)
        protrusion = diameter * rng.uniform(0.07, 0.28)
        sweep = rng.uniform(-0.22, 0.22)
        spline = curve.splines.new("POLY")
        point_count = 5
        spline.points.add(point_count - 1)
        for point_index, curve_point in enumerate(spline.points):
            factor = point_index / (point_count - 1)
            t = max(
                0.0,
                min(1.0, start_t + direction * span * factor),
            )
            _, normal, binormal = path_frame(point_function, t)
            local_angle = angle + sweep * factor
            radial = (
                normal * math.cos(local_angle)
                + binormal * math.sin(local_angle)
            )
            lift = protrusion * factor * factor
            coordinate = point_function(t) + radial * (diameter * 0.49 + lift)
            curve_point.co = (*coordinate, 1.0)
            curve_point.radius = max(0.15, 0.85 - factor * 0.67) * rng.uniform(
                0.82,
                1.18,
            )
    obj = bpy.data.objects.new("IGGY_Rope_Flyaways_UtilityHero", curve)
    collection.objects.link(obj)
    obj["iggy_flyaway_schema"] = "iggy3d.rope_flyaway.v1"
    obj["iggy_flyaway_count"] = 34
    obj["iggy_default_damage"] = False
    return obj


def create_floor(
    collection: bpy.types.Collection,
    material: bpy.types.Material,
) -> bpy.types.Object:
    mesh = bpy.data.meshes.new("IGGY_RopeProofFloor_Mesh")
    vertices = [
        (-2.5, -1.4, 0.0),
        (2.5, -1.4, 0.0),
        (2.5, 1.4, 0.0),
        (-2.5, 1.4, 0.0),
    ]
    mesh.from_pydata(vertices, [], [(0, 1, 2, 3)])
    mesh.materials.append(material)
    floor = bpy.data.objects.new("IGGY_RopeProofFloor", mesh)
    floor.location.z = -0.43
    collection.objects.link(floor)
    return floor


def create_camera(collection: bpy.types.Collection) -> bpy.types.Object:
    camera_data = bpy.data.cameras.new("IGGY_RopeProofCamera")
    camera_data.lens = 62.0
    camera_data.sensor_width = 36.0
    camera_data.dof.use_dof = False
    camera = bpy.data.objects.new("IGGY_RopeProofCamera", camera_data)
    collection.objects.link(camera)
    return camera


def aim_camera(
    camera: bpy.types.Object,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    *,
    lens: float,
) -> None:
    camera.location = location
    camera.data.lens = lens
    direction = Vector(target) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def create_area_light(
    collection: bpy.types.Collection,
    name: str,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    *,
    energy: float,
    size: float,
    colour: tuple[float, float, float],
) -> bpy.types.Object:
    light_data = bpy.data.lights.new(name, "AREA")
    light_data.energy = energy
    light_data.shape = "DISK"
    light_data.size = size
    light_data.color = colour
    light = bpy.data.objects.new(name, light_data)
    collection.objects.link(light)
    light.location = location
    direction = Vector(target) - light.location
    light.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    return light


def configure_scene(
    width: int,
    height: int,
) -> bpy.types.Scene:
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.image_settings.compression = 20
    scene.render.film_transparent = False
    scene.render.use_file_extension = True
    scene.render.resolution_percentage = 100
    scene.render.image_settings.color_management = "FOLLOW_SCENE"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.world.color = (0.004, 0.006, 0.009)
    scene.camera = None
    return scene


def set_render_visibility(
    rope_objects: dict[str, bpy.types.Object],
    flyaways: bpy.types.Object,
    visible: set[str],
    *,
    show_flyaways: bool,
) -> None:
    for key, obj in rope_objects.items():
        obj.hide_render = key not in visible
    flyaways.hide_render = not show_flyaways


def render_proof(
    scene: bpy.types.Scene,
    output_root: Path,
    name: str,
    camera: bpy.types.Object,
    *,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    lens: float,
    lights: dict[str, bpy.types.Object],
    light_energies: dict[str, float],
) -> dict[str, Any]:
    aim_camera(camera, location, target, lens=lens)
    scene.camera = camera
    for light_name, light in lights.items():
        light.data.energy = light_energies[light_name]
    path = output_root / f"rope_plant_fibre_v1_{name}.png"
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "sha256": sha256_file(path),
        "width": scene.render.resolution_x,
        "height": scene.render.resolution_y,
    }


def build(args: argparse.Namespace) -> dict[str, Any]:
    profile = load_json(PROFILE_PATH)
    pattern = load_json(PATTERN_PATH)
    geometry_contract = profile["geometry_source"]
    if geometry_contract["strand_count"] != STRAND_COUNT:
        raise ValueError("The v1 source requires exactly three strands")
    if geometry_contract["yarns_per_strand"] != YARNS_PER_STRAND:
        raise ValueError("The v1 source requires one-plus-six yarn packing")
    if geometry_contract["rope_lay_direction"] != "right_hand":
        raise ValueError("Rope lay must be right-hand")
    if geometry_contract["yarn_lay_direction"] != "left_hand":
        raise ValueError("Yarn lay must oppose rope lay")

    output_root = args.output_root.resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    clear_scene()
    collection = bpy.data.collections.new(COLLECTION_NAME)
    bpy.context.scene.collection.children.link(collection)

    rope_material = create_rope_material(profile)
    flyaway_material = create_flyaway_material()
    floor_material = create_floor_material()
    group = create_geometry_group(rope_material, geometry_contract)

    object_specs = {
        "fine_lashing": {
            "name": "IGGY_Rope_FineLashing",
            "length": 0.90,
            "location": (0.0, 0.0, 0.25),
            "profile_resolution": 8,
        },
        "utility_line": {
            "name": "IGGY_Rope_UtilityHero",
            "length": 1.14,
            "location": (0.0, 0.0, 0.0),
            "profile_resolution": 10,
        },
        "heavy_hawser": {
            "name": "IGGY_Rope_HeavyHawser",
            "length": 1.32,
            "location": (0.0, 0.0, -0.28),
            "profile_resolution": 12,
        },
    }
    rope_objects: dict[str, bpy.types.Object] = {}
    path_functions: dict[str, Callable[[float], Vector]] = {}
    for preset_id, spec in object_specs.items():
        preset = preset_by_id(pattern, preset_id)
        point_function = rope_path_function(preset_id, spec["length"])
        path_functions[preset_id] = point_function
        obj = create_curve_object(
            collection,
            spec["name"],
            point_function,
        )
        obj.location = spec["location"]
        attach_rope_group(
            obj,
            group,
            rope_material,
            preset,
            profile_resolution=spec["profile_resolution"],
        )
        rope_objects[preset_id] = obj

    straight_preset = preset_by_id(pattern, "utility_line")
    straight_length = 0.45
    straight = create_curve_object(
        collection,
        "IGGY_Rope_UtilityStraight",
        lambda t: Vector(((t - 0.5) * straight_length, 0.0, 0.0)),
        sample_count=40,
    )
    straight.location = (0.0, 0.0, -2.0)
    attach_rope_group(
        straight,
        group,
        rope_material,
        straight_preset,
        profile_resolution=10,
    )
    straight.hide_render = True
    rope_objects["utility_straight"] = straight

    flyaways = create_sparse_flyaways(
        collection,
        path_functions["utility_line"],
        diameter=float(straight_preset["diameter_m"]),
        material=flyaway_material,
        seed=19437,
    )
    flyaways.location = object_specs["utility_line"]["location"]
    floor = create_floor(collection, floor_material)
    camera = create_camera(collection)
    lights = {
        "key": create_area_light(
            collection,
            "IGGY_RopeKey",
            (-1.15, -1.05, 1.45),
            (0.0, 0.0, 0.0),
            energy=48.0,
            size=1.15,
            colour=(1.0, 0.93, 0.84),
        ),
        "fill": create_area_light(
            collection,
            "IGGY_RopeFill",
            (1.15, -0.45, 0.65),
            (0.0, 0.0, 0.0),
            energy=22.0,
            size=1.05,
            colour=(0.72, 0.82, 1.0),
        ),
        "rim": create_area_light(
            collection,
            "IGGY_RopeRim",
            (0.35, 0.70, 0.82),
            (0.0, 0.0, 0.0),
            energy=34.0,
            size=0.72,
            colour=(1.0, 0.72, 0.55),
        ),
    }
    scene = configure_scene(args.render_width, args.render_height)
    scene.camera = camera

    blend_path = output_root / "rope_plant_fibre_v1.blend"
    renders: dict[str, dict[str, Any]] = {}
    if not args.skip_renders:
        set_render_visibility(
            rope_objects,
            flyaways,
            {"utility_line"},
            show_flyaways=True,
        )
        renders["hero_oblique"] = render_proof(
            scene,
            output_root,
            "hero_oblique",
            camera,
            location=(0.035, -0.31, 0.072),
            target=(0.015, 0.0, 0.018),
            lens=62.0,
            lights=lights,
            light_energies={"key": 48.0, "fill": 18.0, "rim": 36.0},
        )

        set_render_visibility(
            rope_objects,
            flyaways,
            {"utility_line"},
            show_flyaways=False,
        )
        renders["construction_side"] = render_proof(
            scene,
            output_root,
            "construction_side",
            camera,
            location=(1.02, -0.62, 0.21),
            target=(0.40, 0.0, 0.01),
            lens=76.0,
            lights=lights,
            light_energies={"key": 44.0, "fill": 25.0, "rim": 28.0},
        )

        set_render_visibility(
            rope_objects,
            flyaways,
            {"utility_line"},
            show_flyaways=True,
        )
        renders["grazing_response"] = render_proof(
            scene,
            output_root,
            "grazing_response",
            camera,
            location=(-0.025, -0.27, 0.045),
            target=(0.005, 0.0, 0.012),
            lens=72.0,
            lights=lights,
            light_energies={"key": 60.0, "fill": 6.0, "rim": 44.0},
        )

        set_render_visibility(
            rope_objects,
            flyaways,
            {"fine_lashing", "utility_line", "heavy_hawser"},
            show_flyaways=False,
        )
        renders["scale_family"] = render_proof(
            scene,
            output_root,
            "scale_family",
            camera,
            location=(0.0, -2.05, 0.22),
            target=(0.0, 0.0, -0.01),
            lens=70.0,
            lights=lights,
            light_energies={"key": 50.0, "fill": 22.0, "rim": 32.0},
        )

    set_render_visibility(
        rope_objects,
        flyaways,
        {"fine_lashing", "utility_line", "heavy_hawser"},
        show_flyaways=True,
    )
    floor.hide_render = False
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))

    manifest = {
        "schema": "iggy3d.material.rope_plant_fibre_blender_build.v1",
        "profile_id": profile["profile_id"],
        "blend": {
            "path": str(blend_path),
            "sha256": sha256_file(blend_path),
            "blender_version": bpy.app.version_string,
        },
        "source": {
            "builder": {
                "path": str(Path(__file__).resolve()),
                "sha256": sha256_file(Path(__file__).resolve()),
            },
            "profile": {
                "path": str(PROFILE_PATH),
                "sha256": sha256_file(PROFILE_PATH),
            },
            "pattern": {
                "path": str(PATTERN_PATH),
                "sha256": sha256_file(PATTERN_PATH),
            },
        },
        "hierarchy": {
            "strand_count": STRAND_COUNT,
            "yarns_per_strand": YARNS_PER_STRAND,
            "total_yarns": TOTAL_YARNS,
            "rope_lay_direction": "right_hand",
            "yarn_lay_direction": "left_hand",
        },
        "packing": geometry_contract["packing"],
        "coordinate_contract": {
            "space": "curve_local",
            "unit": "metre",
            "twist_driver": "Spline Parameter Length",
            "attributes": geometry_contract["coordinate_attributes"],
        },
        "node_group": {
            "name": group.name,
            "node_count": len(group.nodes),
            "shared_by": sorted(
                obj.name
                for obj in rope_objects.values()
            ),
        },
        "material": {
            "name": rope_material.name,
            "schema": rope_material["iggy_material_schema"],
            "palette_tone_count": rope_material["iggy_palette_tone_count"],
            "baked_ao_in_base_colour": False,
        },
        "presets": {
            preset_id: {
                "object": rope_objects[preset_id].name,
                "diameter_m": float(
                    rope_objects[preset_id]["sinc_rope_diameter_m"]
                ),
                "lay_length_m": float(
                    rope_objects[preset_id]["sinc_rope_lay_length_m"]
                ),
                "sample_length_m": float(
                    rope_objects[preset_id]["sinc_rope_sample_length_m"]
                ),
            }
            for preset_id in ("fine_lashing", "utility_line", "heavy_hawser")
        },
        "flyaways": {
            "object": flyaways.name,
            "count": int(flyaways["iggy_flyaway_count"]),
            "optional_geometry": True,
            "damage": False,
        },
        "renders": renders,
        "constraints": {
            "uses_ai_generated_imagery": False,
            "uses_index_as_twist_coordinate": False,
            "uses_baked_directional_light": False,
            "uses_baked_ao_in_base_colour": False,
            "damage_default": False,
            "dirt_default": False,
            "wetness_default": False,
        },
    }
    manifest_path = output_root / "rope_plant_fibre_v1_blender_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def main() -> None:
    manifest = build(parse_args())
    print("IGGY_ROPE_BUILD=" + json.dumps({
        "blend": manifest["blend"]["path"],
        "node_group": manifest["node_group"],
        "hierarchy": manifest["hierarchy"],
        "renders": sorted(manifest["renders"]),
    }))


if __name__ == "__main__":
    main()
