#!/usr/bin/env python3
"""Build and prove the reference-audited forged-iron face material."""

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


SCRIPT_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "forged_iron_v1.json"
SAMPLES_PATH = (
    SCRIPT_ROOT
    / "references"
    / "met_468492_front_iron_palette_samples.json"
)
GENERATOR_PATH = SCRIPT_ROOT / "generate_forged_iron_patterns_v1.py"
DEFAULT_SOURCE = (
    SCRIPT_ROOT.parent
    / "structural_oak_door_v1"
    / "output"
    / "structural_oak_door_v1.blend"
)
DEFAULT_OUTPUT_ROOT = SCRIPT_ROOT / "output"
ACTUAL_HINGE_SOURCE = (
    SCRIPT_ROOT / "output" / "openwork_strap_hinge_geometry_v1.blend"
)
ACTUAL_HINGE_OBJECT_NAMES = (
    "SM_GH018_FixedLeaf",
    "SM_GH018_MovingLeaf_Openwork",
    "SM_GH018_Pintle",
    "MovingKnuckle_01",
    "FixedKnuckle_02",
    "MovingKnuckle_03",
    "FixedKnuckle_04",
    "MovingKnuckle_05",
)

DOOR_OBJECT_NAME = "SINC_DemoDoorAssembly"
SOURCE_IRON_MATERIAL_NAME = "IGGY_MAT_NeutralIronProof_v001"
WOOD_MATERIAL_NAME = "IGGY_MAT_ReferenceStructuralOakDoor_v001"
IRON_MATERIAL_NAME = "IGGY_MAT_ReferenceForgedIron_v003"
IRON_GROUP_NAME = "IGGY_SH_ReferenceForgedIron_v003"
COORDINATE_GROUP_NAME = "IGGY_SH_IronCoordinates_v004"
LANE_GROUP_NAME = "IGGY_SH_AuthoredIronLanes_v004"
SCALE_GROUP_NAME = "IGGY_SH_ForgeScaleLayer_v002"
HAMMER_GROUP_NAME = "IGGY_SH_HammerPlanes_v003"
MICRO_GROUP_NAME = "IGGY_SH_IronMicroSurface_v002"
WORKED_GROUP_NAME = "IGGY_SH_WorkedIronSurface_v001"
NORMAL_GROUP_NAME = "IGGY_SH_NormalCombine_v003"
HEIGHT_GROUP_NAME = "IGGY_SH_SurfaceHeight_v003"
PROOF_COLLECTION_NAME = "IGGY_ForgedIronProof"
TARGET_COLLECTION_NAME = "IGGY_ForgedIronActualHingeProof"

PROOF_MODES = {
    "combined": 0.0,
    "base_colour": 1.0,
    "roughness": 2.0,
    "metalness": 3.0,
    "masks": 4.0,
    "normal": 5.0,
    "height": 6.0,
    "worked_response": 7.0,
}


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument(
        "--output-root",
        type=Path,
        default=DEFAULT_OUTPUT_ROOT,
    )
    parser.add_argument("--atlas-resolution-x", type=int, default=1536)
    parser.add_argument("--atlas-resolution-y", type=int, default=256)
    parser.add_argument("--door-resolution-x", type=int, default=900)
    parser.add_argument("--door-resolution-y", type=int, default=1080)
    parser.add_argument("--proof-resolution-x", type=int, default=1200)
    parser.add_argument("--proof-resolution-y", type=int, default=760)
    parser.add_argument("--target-resolution-x", type=int, default=1400)
    parser.add_argument("--target-resolution-y", type=int, default=560)
    return parser.parse_args(argv)


def load_contract() -> tuple[dict[str, Any], dict[str, Any]]:
    profile = json.loads(PROFILE_PATH.read_text())
    samples = json.loads(SAMPLES_PATH.read_text())
    observed = [
        sample["hex"]
        for sample in samples["eyedropper_samples"]["samples"]
    ]
    roles = [
        sample["material_role"]
        for sample in samples["eyedropper_samples"]["samples"]
    ]
    assert profile["profile_id"] == "forged_iron_v1"
    assert profile["palette"]["observed_reference_twenty"] == observed
    assert len(observed) == len(set(observed)) == 20
    assert roles.count("base_hue_only") == 11
    assert roles.count("oxidation_overlay_only") == 3
    assert roles.count("excluded") == 6
    return profile, samples


def load_generator_module():
    spec = importlib.util.spec_from_file_location(
        "generate_forged_iron_patterns_v1",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load pattern generator: {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def srgb_channel_to_linear(value: float) -> float:
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def hex_colour(value: str) -> tuple[float, float, float, float]:
    value = value.removeprefix("#")
    srgb = [
        int(value[index : index + 2], 16) / 255.0
        for index in (0, 2, 4)
    ]
    return tuple(srgb_channel_to_linear(channel) for channel in srgb) + (
        1.0,
    )


def add_node(
    tree: bpy.types.NodeTree,
    node_type: str,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = tree.nodes.new(node_type)
    node.name = name
    node.label = name.replace("_", " ").title()
    node.location = location
    return node


def set_input(
    node: bpy.types.Node,
    socket_name: str,
    value,
) -> None:
    socket = node.inputs.get(socket_name)
    if socket is None:
        raise KeyError(f"{node.bl_idname} has no {socket_name!r} input")
    socket.default_value = value


def set_first_available_input(
    node: bpy.types.Node,
    names: tuple[str, ...],
    value,
) -> None:
    for name in names:
        socket = node.inputs.get(name)
        if socket is not None:
            socket.default_value = value
            return
    raise KeyError(f"{node.bl_idname} has none of {names!r}")


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


def add_math(
    tree: bpy.types.NodeTree,
    name: str,
    operation: str,
    location: tuple[float, float],
    *,
    second: float | None = None,
    clamp: bool = False,
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMath", name, location)
    node.operation = operation
    node.use_clamp = clamp
    if second is not None:
        node.inputs[1].default_value = second
    return node


def add_mix(
    tree: bpy.types.NodeTree,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMixRGB", name, location)
    node.blend_type = "MIX"
    node.use_clamp = True
    return node


def node_contract(tree: bpy.types.NodeTree) -> dict[str, Any]:
    return {
        "node_names": sorted(node.name for node in tree.nodes),
        "node_types": sorted(node.bl_idname for node in tree.nodes),
        "link_count": len(tree.links),
    }


def load_authored_image(
    path: Path,
    name: str,
    *,
    non_color: bool,
) -> bpy.types.Image:
    image = bpy.data.images.load(str(path), check_existing=False)
    image.name = name
    image.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    return image


def _component_vertex_sets(
    mesh: bpy.types.Mesh,
    material_index: int,
) -> list[list[int]]:
    iron_polygons = [
        polygon
        for polygon in mesh.polygons
        if polygon.material_index == material_index
    ]
    iron_vertices = {
        vertex_index
        for polygon in iron_polygons
        for vertex_index in polygon.vertices
    }
    adjacency = {vertex_index: set() for vertex_index in iron_vertices}
    for polygon in iron_polygons:
        indices = list(polygon.vertices)
        for index, vertex_index in enumerate(indices):
            next_index = indices[(index + 1) % len(indices)]
            adjacency[vertex_index].add(next_index)
            adjacency[next_index].add(vertex_index)
    components = []
    remaining = set(iron_vertices)
    while remaining:
        start = min(remaining)
        stack = [start]
        component = []
        remaining.remove(start)
        while stack:
            current = stack.pop()
            component.append(current)
            for neighbor in adjacency[current]:
                if neighbor in remaining:
                    remaining.remove(neighbor)
                    stack.append(neighbor)
        components.append(sorted(component))
    return components


def realize_door_for_material_proof(
    door: bpy.types.Object,
) -> dict[str, Any]:
    """Freeze the evaluated acceptance fixture without editing its donor file."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = door.evaluated_get(depsgraph)
    realized = bpy.data.meshes.new_from_object(
        evaluated,
        preserve_all_data_layers=True,
        depsgraph=depsgraph,
    )
    previous = door.data
    door.data = realized
    door.modifiers.clear()
    if previous.users == 0:
        bpy.data.meshes.remove(previous)
    return {
        "vertices": len(realized.vertices),
        "polygons": len(realized.polygons),
        "materials": [
            material.name if material is not None else None
            for material in realized.materials
        ],
        "attributes_preserved": sorted(
            attribute.name for attribute in realized.attributes
        ),
    }


def ensure_component_local_iron_coordinates(
    door: bpy.types.Object,
    iron_material: bpy.types.Material,
    profile: dict[str, Any],
) -> dict[str, Any]:
    """Emit explicit metre coordinates; shader projection never guesses axes."""
    mesh = door.data
    material_index = next(
        (
            index
            for index, material in enumerate(mesh.materials)
            if material is iron_material
        ),
        None,
    )
    if material_index is None:
        raise RuntimeError("Door has no neutral iron material slot")
    components = _component_vertex_sets(mesh, material_index)
    contract = profile["coordinate_contract"]
    u_name = contract["longitudinal_attribute"]
    v_name = contract["cross_stock_attribute"]
    u_attribute = mesh.attributes.get(u_name) or mesh.attributes.new(
        u_name,
        "FLOAT",
        "POINT",
    )
    v_attribute = mesh.attributes.get(v_name) or mesh.attributes.new(
        v_name,
        "FLOAT",
        "POINT",
    )
    fabrication_name = contract["fabrication_scale_attribute"]
    fabrication_attribute = (
        mesh.attributes.get(fabrication_name)
        or mesh.attributes.new(fabrication_name, "FLOAT", "POINT")
    )
    spans = []
    for component in components:
        positions = np.asarray(
            [mesh.vertices[index].co[:] for index in component],
            dtype=np.float64,
        )
        centre = positions.mean(axis=0)
        centered = positions - centre
        covariance = centered.T @ centered
        eigenvalues, eigenvectors = np.linalg.eigh(covariance)
        order = np.argsort(eigenvalues)[::-1]
        longitudinal_axis = eigenvectors[:, order[0]]
        cross_axis = eigenvectors[:, order[1]]
        longitudinal = centered @ longitudinal_axis
        cross_stock = centered @ cross_axis
        longitudinal -= longitudinal.min()
        cross_stock -= cross_stock.min()
        for local_index, vertex_index in enumerate(component):
            u_attribute.data[vertex_index].value = float(
                longitudinal[local_index]
            )
            v_attribute.data[vertex_index].value = float(
                cross_stock[local_index]
            )
            fabrication_attribute.data[vertex_index].value = 1.0
        spans.append(
            {
                "vertices": len(component),
                "length_m": float(np.ptp(longitudinal)),
                "cross_stock_m": float(np.ptp(cross_stock)),
            }
        )
    tangent_uv = mesh.uv_layers.get(contract["tangent_uv"])
    if tangent_uv is None:
        tangent_uv = mesh.uv_layers.new(name=contract["tangent_uv"])
    for loop in mesh.loops:
        tangent_uv.data[loop.index].uv = (
            float(u_attribute.data[loop.vertex_index].value),
            float(v_attribute.data[loop.vertex_index].value),
        )
    return {
        "material_index": material_index,
        "component_count": len(components),
        "components": spans,
        "attributes": [u_name, v_name, fabrication_name],
        "tangent_uv": tangent_uv.name,
    }


def _profile_layer(profile: dict[str, Any], layer_id: str) -> dict[str, Any]:
    return next(layer for layer in profile["layers"] if layer["id"] == layer_id)


def _new_shader_group(
    name: str,
    description: str,
    *,
    color_tag: str = "CONVERTER",
) -> bpy.types.NodeTree:
    previous = bpy.data.node_groups.get(name)
    if previous is not None:
        bpy.data.node_groups.remove(previous, do_unlink=True)
    group = bpy.data.node_groups.new(name, "ShaderNodeTree")
    group.color_tag = color_tag
    group.description = description
    return group


def _new_socket(
    group: bpy.types.NodeTree,
    name: str,
    in_out: str,
    socket_type: str,
    *,
    default: float | None = None,
    minimum: float | None = None,
    maximum: float | None = None,
) -> None:
    socket = group.interface.new_socket(
        name=name,
        in_out=in_out,
        socket_type=socket_type,
    )
    if default is not None:
        socket.default_value = default
    if minimum is not None:
        socket.min_value = minimum
    if maximum is not None:
        socket.max_value = maximum


def _build_coordinate_group(profile: dict[str, Any]) -> bpy.types.NodeTree:
    group = _new_shader_group(
        COORDINATE_GROUP_NAME,
        "Separate fabrication-scaled macro, world-metre surface, and larger finite-work coordinates.",
        color_tag="INPUT",
    )
    for name, socket_type in (
        ("Macro Atlas Coordinate", "NodeSocketVector"),
        ("Surface Atlas Coordinate", "NodeSocketVector"),
        ("Worked Atlas Coordinate", "NodeSocketVector"),
        ("Edge Mask", "NodeSocketFloat"),
        ("Variation Index", "NodeSocketFloat"),
        ("Fabrication Scale", "NodeSocketFloat"),
    ):
        _new_socket(group, name, "OUTPUT", socket_type)
    output = add_node(group, "NodeGroupOutput", "Group_Output", (1040, 220))
    contract = profile["coordinate_contract"]
    u_attribute = add_node(
        group,
        "ShaderNodeAttribute",
        "Component_Longitudinal_Metres",
        (-1160, 720),
    )
    u_attribute.attribute_name = contract["longitudinal_attribute"]
    v_attribute = add_node(
        group,
        "ShaderNodeAttribute",
        "Component_Cross_Stock_Metres",
        (-1160, 520),
    )
    v_attribute.attribute_name = contract["cross_stock_attribute"]
    seed_attribute = add_node(
        group,
        "ShaderNodeAttribute",
        "Authored_Variation_Seed",
        (-1160, 190),
    )
    seed_attribute.attribute_name = contract["variant_attribute"]
    edge_attribute = add_node(
        group,
        "ShaderNodeAttribute",
        "Geometry_Authored_Contact_Edge",
        (-1160, -30),
    )
    edge_attribute.attribute_name = contract["edge_attribute"]
    fabrication_attribute = add_node(
        group,
        "ShaderNodeAttribute",
        "Authored_Fabrication_Scale",
        (-1160, 940),
    )
    fabrication_attribute.attribute_name = contract[
        "fabrication_scale_attribute"
    ]
    safe_fabrication = add_math(
        group,
        "Safe_Fabrication_Scale",
        "MAXIMUM",
        (-930, 940),
        second=1.0,
    )
    macro_u_metres = add_math(
        group,
        "Macro_Longitudinal_By_Fabrication",
        "DIVIDE",
        (-700, 760),
    )
    macro_v_metres = add_math(
        group,
        "Macro_Cross_Stock_By_Fabrication",
        "DIVIDE",
        (-700, 570),
    )
    link(group, fabrication_attribute, "Fac", safe_fabrication, "Value")
    link(group, u_attribute, "Fac", macro_u_metres, "Value")
    link(
        group,
        safe_fabrication,
        "Value",
        macro_u_metres,
        "Value_001",
    )
    link(group, v_attribute, "Fac", macro_v_metres, "Value")
    link(
        group,
        safe_fabrication,
        "Value",
        macro_v_metres,
        "Value_001",
    )
    tile = profile["pattern"]["physical_tile_m"]
    worked_tile = profile["pattern"]["worked_physical_tile_m"]

    repeated = {}
    coordinate_sources = {
        "Macro": (macro_u_metres, macro_v_metres, tile),
        "Surface": (u_attribute, v_attribute, tile),
        "Worked": (u_attribute, v_attribute, worked_tile),
    }
    for lane_index, (lane, (u_source, v_source, lane_tile)) in enumerate(
        coordinate_sources.items()
    ):
        u_scale = add_math(
            group,
            f"{lane}_Longitudinal_Metres_To_Tile",
            "MULTIPLY",
            (-450, 820 - lane_index * 330),
            second=1.0 / float(lane_tile["length"]),
        )
        v_scale = add_math(
            group,
            f"{lane}_Cross_Stock_Metres_To_Tile",
            "MULTIPLY",
            (-450, 690 - lane_index * 330),
            second=1.0 / float(lane_tile["width"]),
        )
        u_repeat = add_math(
            group,
            f"{lane}_Longitudinal_NonSquare_Repeat",
            "FRACT",
            (-220, 820 - lane_index * 330),
        )
        v_repeat = add_math(
            group,
            f"{lane}_Cross_Stock_NonSquare_Repeat",
            "FRACT",
            (-220, 690 - lane_index * 330),
        )
        link(group, u_source, "Value" if lane == "Macro" else "Fac", u_scale, "Value")
        link(group, v_source, "Value" if lane == "Macro" else "Fac", v_scale, "Value")
        link(group, u_scale, "Value", u_repeat, "Value")
        link(group, v_scale, "Value", v_repeat, "Value")
        repeated[lane] = (u_repeat, v_repeat)

    seed_abs = add_math(
        group,
        "Positive_Variation_Seed",
        "ABSOLUTE",
        (-930, 190),
    )
    seed_modulo = add_math(
        group,
        "Four_Authored_Variations",
        "MODULO",
        (-700, 190),
        second=4.0,
    )
    seed_floor = add_math(
        group,
        "Integer_Variation_Index",
        "FLOOR",
        (-470, 190),
    )
    row_from_bottom = add_math(
        group,
        "Atlas_Row_From_Bottom",
        "SUBTRACT",
        (-240, 190),
    )
    row_from_bottom.inputs[0].default_value = 3.0
    link(group, seed_attribute, "Fac", seed_abs, "Value")
    link(group, seed_abs, "Value", seed_modulo, "Value")
    link(group, seed_modulo, "Value", seed_floor, "Value")
    link(group, seed_floor, "Value", row_from_bottom, "Value_001")

    for lane_index, lane in enumerate(("Macro", "Surface", "Worked")):
        u_repeat, v_repeat = repeated[lane]
        row_plus_v = add_math(
            group,
            f"{lane}_Variation_Row_Plus_Local_V",
            "ADD",
            (40, 700 - lane_index * 330),
        )
        atlas_v = add_math(
            group,
            f"{lane}_Four_Row_Atlas_V",
            "DIVIDE",
            (270, 700 - lane_index * 330),
            second=4.0,
        )
        atlas_coordinate = add_node(
            group,
            "ShaderNodeCombineXYZ",
            f"{lane}_Measured_Authored_Atlas_Coordinate",
            (500, 760 - lane_index * 330),
        )
        link(group, row_from_bottom, "Value", row_plus_v, "Value")
        link(group, v_repeat, "Value", row_plus_v, "Value_001")
        link(group, row_plus_v, "Value", atlas_v, "Value")
        link(group, u_repeat, "Value", atlas_coordinate, "X")
        link(group, atlas_v, "Value", atlas_coordinate, "Y")
        link(
            group,
            atlas_coordinate,
            "Vector",
            output,
            f"{lane} Atlas Coordinate",
        )
    link(group, edge_attribute, "Fac", output, "Edge Mask")
    link(group, seed_floor, "Value", output, "Variation Index")
    link(
        group,
        safe_fabrication,
        "Value",
        output,
        "Fabrication Scale",
    )
    return group


def _build_lane_group(
    *,
    scale_color_image: bpy.types.Image,
    iron_color_image: bpy.types.Image,
    layer_response_image: bpy.types.Image,
    macro_normal_image: bpy.types.Image,
    scale_normal_image: bpy.types.Image,
    micro_normal_image: bpy.types.Image,
    worked_normal_image: bpy.types.Image,
    worked_response_image: bpy.types.Image,
    macro_height_image: bpy.types.Image,
    scale_height_image: bpy.types.Image,
    micro_height_image: bpy.types.Image,
    mask_image: bpy.types.Image,
) -> bpy.types.NodeTree:
    group = _new_shader_group(
        LANE_GROUP_NAME,
        "The only image-sampling boundary for twelve authored physical lanes.",
        color_tag="TEXTURE",
    )
    _new_socket(group, "Macro Atlas Coordinate", "INPUT", "NodeSocketVector")
    _new_socket(group, "Surface Atlas Coordinate", "INPUT", "NodeSocketVector")
    _new_socket(group, "Worked Atlas Coordinate", "INPUT", "NodeSocketVector")
    for name, socket_type in (
        ("Scale Color", "NodeSocketColor"),
        ("Iron Color", "NodeSocketColor"),
        ("Layer Response", "NodeSocketColor"),
        ("Macro Normal Color", "NodeSocketColor"),
        ("Scale Normal Color", "NodeSocketColor"),
        ("Micro Normal Color", "NodeSocketColor"),
        ("Worked Normal Color", "NodeSocketColor"),
        ("Worked Response", "NodeSocketColor"),
        ("Macro Height Encoded", "NodeSocketFloat"),
        ("Scale Height Encoded", "NodeSocketFloat"),
        ("Micro Height Encoded", "NodeSocketFloat"),
        ("Mask Color", "NodeSocketColor"),
    ):
        _new_socket(group, name, "OUTPUT", socket_type)
    group_input = add_node(
        group,
        "NodeGroupInput",
        "Group_Input",
        (-760, 250),
    )
    group_output = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (520, 250),
    )
    specifications = (
        ("Scale_Color_Atlas", scale_color_image, "Scale Color", "Surface"),
        ("Iron_Color_Atlas", iron_color_image, "Iron Color", "Surface"),
        ("Layer_Response_Atlas", layer_response_image, "Layer Response", "Surface"),
        ("Macro_Normal_Atlas", macro_normal_image, "Macro Normal Color", "Macro"),
        ("Scale_Normal_Atlas", scale_normal_image, "Scale Normal Color", "Surface"),
        ("Micro_Normal_Atlas", micro_normal_image, "Micro Normal Color", "Surface"),
        ("Worked_Normal_Atlas", worked_normal_image, "Worked Normal Color", "Worked"),
        ("Worked_Response_Atlas", worked_response_image, "Worked Response", "Worked"),
        ("Macro_Height_Atlas", macro_height_image, "Macro Height Encoded", "Macro"),
        ("Scale_Height_Atlas", scale_height_image, "Scale Height Encoded", "Surface"),
        ("Micro_Height_Atlas", micro_height_image, "Micro Height Encoded", "Surface"),
        ("Semantic_Mask_Atlas", mask_image, "Mask Color", "Macro"),
    )
    for index, (name, image, output_name, coordinate_lane) in enumerate(
        specifications
    ):
        texture = add_node(
            group,
            "ShaderNodeTexImage",
            name,
            (-500, 760 - index * 140),
        )
        texture.image = image
        texture.extension = "EXTEND"
        texture.interpolation = "Linear"
        link(
            group,
            group_input,
            f"{coordinate_lane} Atlas Coordinate",
            texture,
            "Vector",
        )
        if output_name.endswith("Encoded"):
            grayscale = add_node(
                group,
                "ShaderNodeRGBToBW",
                f"{name}_Value",
                (-230, 760 - index * 140),
            )
            link(group, texture, "Color", grayscale, "Color")
            link(group, grayscale, "Val", group_output, output_name)
        else:
            link(group, texture, "Color", group_output, output_name)
    return group


def _build_scale_group() -> bpy.types.NodeTree:
    group = _new_shader_group(
        SCALE_GROUP_NAME,
        "Dielectric compact forge scale with independent colour and roughness.",
        color_tag="SHADER",
    )
    for name, socket_type in (
        ("Scale Color", "NodeSocketColor"),
        ("Layer Response", "NodeSocketColor"),
        ("Mask Color", "NodeSocketColor"),
    ):
        _new_socket(group, name, "INPUT", socket_type)
    for name, socket_type in (
        ("Color", "NodeSocketColor"),
        ("Roughness", "NodeSocketFloat"),
        ("Scale Tone Mask", "NodeSocketFloat"),
    ):
        _new_socket(group, name, "OUTPUT", socket_type)
    group_input = add_node(
        group,
        "NodeGroupInput",
        "Group_Input",
        (-460, 160),
    )
    group_output = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (300, 160),
    )
    response = add_node(
        group,
        "ShaderNodeSeparateColor",
        "Separate_Scale_And_Iron_Response",
        (-180, 80),
    )
    response.mode = "RGB"
    masks = add_node(
        group,
        "ShaderNodeSeparateColor",
        "Separate_Semantic_Masks",
        (-180, -130),
    )
    masks.mode = "RGB"
    link(group, group_input, "Layer Response", response, "Color")
    link(group, group_input, "Mask Color", masks, "Color")
    link(group, group_input, "Scale Color", group_output, "Color")
    link(group, response, "Red", group_output, "Roughness")
    link(group, masks, "Blue", group_output, "Scale Tone Mask")
    return group


def _build_hammer_group() -> bpy.types.NodeTree:
    group = _new_shader_group(
        HAMMER_GROUP_NAME,
        "Finite measured-tool hammer masks and conductive iron response.",
        color_tag="SHADER",
    )
    for name, socket_type in (
        ("Iron Color", "NodeSocketColor"),
        ("Layer Response", "NodeSocketColor"),
        ("Mask Color", "NodeSocketColor"),
    ):
        _new_socket(group, name, "INPUT", socket_type)
    for name, socket_type in (
        ("Iron Color", "NodeSocketColor"),
        ("Iron Roughness", "NodeSocketFloat"),
        ("Exposure Mask", "NodeSocketFloat"),
        ("Planishing Mask", "NodeSocketFloat"),
        ("Cross Peen Mask", "NodeSocketFloat"),
        ("Quiet Mask", "NodeSocketFloat"),
    ):
        _new_socket(group, name, "OUTPUT", socket_type)
    group_input = add_node(
        group,
        "NodeGroupInput",
        "Group_Input",
        (-520, 170),
    )
    group_output = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (440, 170),
    )
    response = add_node(
        group,
        "ShaderNodeSeparateColor",
        "Separate_Layer_Response",
        (-270, 90),
    )
    response.mode = "RGB"
    masks = add_node(
        group,
        "ShaderNodeSeparateColor",
        "Separate_Hammer_Masks",
        (-270, -120),
    )
    masks.mode = "RGB"
    activity = add_math(
        group,
        "Finite_Hammer_Activity",
        "MAXIMUM",
        (-20, -120),
    )
    quiet = add_math(
        group,
        "Quiet_Outside_Hammer_Faces",
        "SUBTRACT",
        (190, -120),
        clamp=True,
    )
    quiet.inputs[0].default_value = 1.0
    link(group, group_input, "Layer Response", response, "Color")
    link(group, group_input, "Mask Color", masks, "Color")
    link(group, masks, "Red", activity, "Value")
    link(group, masks, "Green", activity, "Value_001")
    link(group, activity, "Value", quiet, "Value_001")
    link(group, group_input, "Iron Color", group_output, "Iron Color")
    link(group, response, "Green", group_output, "Iron Roughness")
    link(group, response, "Blue", group_output, "Exposure Mask")
    link(group, masks, "Red", group_output, "Planishing Mask")
    link(group, masks, "Green", group_output, "Cross Peen Mask")
    link(group, quiet, "Value", group_output, "Quiet Mask")
    return group


def _build_worked_group(profile: dict[str, Any]) -> bpy.types.NodeTree:
    group = _new_shader_group(
        WORKED_GROUP_NAME,
        "Finite worked-face normal, direction, protected rest, and anisotropy.",
        color_tag="VECTOR",
    )
    _new_socket(group, "Worked Normal Color", "INPUT", "NodeSocketColor")
    _new_socket(group, "Worked Response", "INPUT", "NodeSocketColor")
    for name, socket_type in (
        ("Normal Color", "NodeSocketColor"),
        ("Worked Mask", "NodeSocketFloat"),
        ("Direction", "NodeSocketFloat"),
        ("Protected Rest", "NodeSocketFloat"),
        ("Anisotropy", "NodeSocketFloat"),
    ):
        _new_socket(group, name, "OUTPUT", socket_type)
    group_input = add_node(
        group,
        "NodeGroupInput",
        "Group_Input",
        (-430, 100),
    )
    group_output = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (360, 100),
    )
    response = add_node(
        group,
        "ShaderNodeSeparateColor",
        "Separate_Worked_Response",
        (-170, -70),
    )
    response.mode = "RGB"
    anisotropy = add_math(
        group,
        "Worked_Mask_To_Anisotropy",
        "MULTIPLY",
        (90, -180),
        second=float(profile["surface_response"]["worked_anisotropy_max"]),
        clamp=True,
    )
    link(group, group_input, "Worked Response", response, "Color")
    link(group, response, "Red", anisotropy, "Value")
    link(
        group,
        group_input,
        "Worked Normal Color",
        group_output,
        "Normal Color",
    )
    link(group, response, "Red", group_output, "Worked Mask")
    link(group, response, "Green", group_output, "Direction")
    link(group, response, "Blue", group_output, "Protected Rest")
    link(group, anisotropy, "Value", group_output, "Anisotropy")
    return group


def _build_micro_group(profile: dict[str, Any]) -> bpy.types.NodeTree:
    group = _new_shader_group(
        MICRO_GROUP_NAME,
        "Measured-proxy micro normal with camera-distance fade.",
        color_tag="VECTOR",
    )
    _new_socket(group, "Micro Normal Color", "INPUT", "NodeSocketColor")
    _new_socket(group, "Normal Color", "OUTPUT", "NodeSocketColor")
    _new_socket(group, "Distance Fade", "OUTPUT", "NodeSocketFloat")
    group_input = add_node(
        group,
        "NodeGroupInput",
        "Group_Input",
        (-520, 120),
    )
    group_output = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (370, 120),
    )
    camera = add_node(
        group,
        "ShaderNodeCameraData",
        "Camera_Distance_Metres",
        (-520, -130),
    )
    distance = profile["surface_response"]["distance_response_m"]
    fade = add_node(
        group,
        "ShaderNodeMapRange",
        "Micro_Fade_By_View_Distance",
        (-270, -100),
    )
    fade.clamp = True
    fade.interpolation_type = "SMOOTHERSTEP"
    fade.inputs["From Min"].default_value = float(
        distance["micro_full_until"]
    )
    fade.inputs["From Max"].default_value = float(
        distance["micro_zero_after"]
    )
    fade.inputs["To Min"].default_value = 1.0
    fade.inputs["To Max"].default_value = 0.0
    neutral = (0.5, 0.5, 1.0, 1.0)
    normal_mix = add_mix(
        group,
        "Distance_Faded_Micro_Normal",
        (30, 170),
    )
    normal_mix.inputs["Color1"].default_value = neutral
    link(group, camera, "View Distance", fade, "Value")
    link(group, fade, "Result", normal_mix, "Fac")
    link(
        group,
        group_input,
        "Micro Normal Color",
        normal_mix,
        "Color2",
    )
    link(group, normal_mix, "Color", group_output, "Normal Color")
    link(group, fade, "Result", group_output, "Distance Fade")
    return group


def _decode_tangent_normal(
    tree: bpy.types.NodeTree,
    source: bpy.types.Node,
    source_socket: str,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    multiply = add_node(
        tree,
        "ShaderNodeVectorMath",
        f"{name}_Decode_Times_Two",
        location,
    )
    multiply.operation = "MULTIPLY"
    multiply.inputs[1].default_value = (2.0, 2.0, 2.0)
    offset = add_node(
        tree,
        "ShaderNodeVectorMath",
        f"{name}_Decode_Minus_One",
        (location[0] + 190, location[1]),
    )
    offset.operation = "ADD"
    offset.inputs[1].default_value = (-1.0, -1.0, -1.0)
    link(tree, source, source_socket, multiply, "Vector")
    link(tree, multiply, "Vector", offset, "Vector")
    return offset


def _strengthen_tangent_normal(
    tree: bpy.types.NodeTree,
    normal: bpy.types.Node,
    strength_source: bpy.types.Node,
    strength_socket: str,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    strength_vector = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        f"{name}_XY_Strength",
        location,
    )
    strength_vector.inputs["Z"].default_value = 1.0
    link(tree, strength_source, strength_socket, strength_vector, "X")
    link(tree, strength_source, strength_socket, strength_vector, "Y")
    multiply = add_node(
        tree,
        "ShaderNodeVectorMath",
        f"{name}_Apply_XY_Strength",
        (location[0] + 190, location[1]),
    )
    multiply.operation = "MULTIPLY"
    link(tree, normal, "Vector", multiply, "Vector")
    tree.links.new(strength_vector.outputs["Vector"], multiply.inputs[1])
    return multiply


def _whiteout_combine(
    tree: bpy.types.NodeTree,
    first: bpy.types.Node,
    second: bpy.types.Node,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    first_separate = add_node(
        tree,
        "ShaderNodeSeparateXYZ",
        f"{name}_First_XYZ",
        location,
    )
    second_separate = add_node(
        tree,
        "ShaderNodeSeparateXYZ",
        f"{name}_Second_XYZ",
        (location[0], location[1] - 180),
    )
    x_add = add_math(
        tree,
        f"{name}_Add_X",
        "ADD",
        (location[0] + 190, location[1] + 40),
    )
    y_add = add_math(
        tree,
        f"{name}_Add_Y",
        "ADD",
        (location[0] + 190, location[1] - 50),
    )
    z_multiply = add_math(
        tree,
        f"{name}_Multiply_Z",
        "MULTIPLY",
        (location[0] + 190, location[1] - 140),
    )
    combine = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        f"{name}_Whiteout_Vector",
        (location[0] + 390, location[1] - 30),
    )
    normalize = add_node(
        tree,
        "ShaderNodeVectorMath",
        f"{name}_Normalize",
        (location[0] + 590, location[1] - 30),
    )
    normalize.operation = "NORMALIZE"
    link(tree, first, "Vector", first_separate, "Vector")
    link(tree, second, "Vector", second_separate, "Vector")
    link(tree, first_separate, "X", x_add, "Value")
    link(tree, second_separate, "X", x_add, "Value_001")
    link(tree, first_separate, "Y", y_add, "Value")
    link(tree, second_separate, "Y", y_add, "Value_001")
    link(tree, first_separate, "Z", z_multiply, "Value")
    link(tree, second_separate, "Z", z_multiply, "Value_001")
    link(tree, x_add, "Value", combine, "X")
    link(tree, y_add, "Value", combine, "Y")
    link(tree, z_multiply, "Value", combine, "Z")
    link(tree, combine, "Vector", normalize, "Vector")
    return normalize


def _build_normal_group() -> bpy.types.NodeTree:
    group = _new_shader_group(
        NORMAL_GROUP_NAME,
        "Whiteout blending for independent macro, scale, micro, and worked normals.",
        color_tag="VECTOR",
    )
    for name, socket_type, default in (
        ("Macro Normal Color", "NodeSocketColor", None),
        ("Scale Normal Color", "NodeSocketColor", None),
        ("Micro Normal Color", "NodeSocketColor", None),
        ("Worked Normal Color", "NodeSocketColor", None),
        ("Macro Strength", "NodeSocketFloat", 1.0),
        ("Scale Strength", "NodeSocketFloat", 1.0),
        ("Micro Strength", "NodeSocketFloat", 1.0),
        ("Worked Strength", "NodeSocketFloat", 1.0),
    ):
        _new_socket(
            group,
            name,
            "INPUT",
            socket_type,
            default=default,
            minimum=0.0 if default is not None else None,
            maximum=2.0 if default is not None else None,
        )
    _new_socket(group, "Normal Color", "OUTPUT", "NodeSocketColor")
    group_input = add_node(
        group,
        "NodeGroupInput",
        "Group_Input",
        (-1320, 250),
    )
    group_output = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (1230, 250),
    )
    decoded = []
    for index, lane in enumerate(("Macro", "Scale", "Micro", "Worked")):
        normal = _decode_tangent_normal(
            group,
            group_input,
            f"{lane} Normal Color",
            lane,
            (-1120, 510 - index * 330),
        )
        strengthened = _strengthen_tangent_normal(
            group,
            normal,
            group_input,
            f"{lane} Strength",
            lane,
            (-700, 510 - index * 330),
        )
        decoded.append(strengthened)
    macro_scale = _whiteout_combine(
        group,
        decoded[0],
        decoded[1],
        "Macro_And_Scale",
        (-300, 450),
    )
    combined = _whiteout_combine(
        group,
        macro_scale,
        decoded[2],
        "Add_Micro",
        (340, 340),
    )
    final_combined = _whiteout_combine(
        group,
        combined,
        decoded[3],
        "Add_Worked",
        (610, 170),
    )
    encode_scale = add_node(
        group,
        "ShaderNodeVectorMath",
        "Encode_Normal_Times_Half",
        (1120, 300),
    )
    encode_scale.operation = "MULTIPLY"
    encode_scale.inputs[1].default_value = (0.5, 0.5, 0.5)
    encode_offset = add_node(
        group,
        "ShaderNodeVectorMath",
        "Encode_Normal_Plus_Half",
        (1280, 300),
    )
    encode_offset.operation = "ADD"
    encode_offset.inputs[1].default_value = (0.5, 0.5, 0.5)
    link(group, final_combined, "Vector", encode_scale, "Vector")
    link(group, encode_scale, "Vector", encode_offset, "Vector")
    link(group, encode_offset, "Vector", group_output, "Normal Color")
    return group


def _build_height_group(profile: dict[str, Any]) -> bpy.types.NodeTree:
    group = _new_shader_group(
        HEIGHT_GROUP_NAME,
        "Decode and combine three signed metre-valued height frequencies.",
        color_tag="CONVERTER",
    )
    for name, default in (
        ("Macro Height Encoded", None),
        ("Scale Height Encoded", None),
        ("Micro Height Encoded", None),
        ("Micro Distance Fade", 1.0),
        ("Fabrication Scale", 1.0),
        ("Displacement Strength", 1.0),
    ):
        _new_socket(
            group,
            name,
            "INPUT",
            "NodeSocketFloat",
            default=default,
            minimum=0.0 if default is not None else None,
            maximum=2.0 if default is not None else None,
        )
    _new_socket(group, "Height M", "OUTPUT", "NodeSocketFloat")
    group_input = add_node(
        group,
        "NodeGroupInput",
        "Group_Input",
        (-920, 240),
    )
    group_output = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (600, 240),
    )
    encodings = profile["surface_response"]["layer_height_encoding"]
    decoded = {}
    for index, (lane, socket_name, encoding_name) in enumerate(
        (
            ("Macro", "Macro Height Encoded", "macro_hammer"),
            ("Scale", "Scale Height Encoded", "scale_lip"),
            ("Micro", "Micro Height Encoded", "micro_surface"),
        )
    ):
        encoding = encodings[encoding_name]
        span = float(encoding["maximum_m"]) - float(
            encoding["minimum_m"]
        )
        multiply = add_math(
            group,
            f"Decode_{lane}_Height_Range",
            "MULTIPLY",
            (-660, 500 - index * 220),
            second=span,
        )
        offset = add_math(
            group,
            f"Decode_{lane}_Height_Offset",
            "ADD",
            (-430, 500 - index * 220),
            second=float(encoding["minimum_m"]),
        )
        link(group, group_input, socket_name, multiply, "Value")
        link(group, multiply, "Value", offset, "Value")
        decoded[lane] = offset
    micro_faded = add_math(
        group,
        "Distance_Faded_Micro_Height",
        "MULTIPLY",
        (-160, 30),
    )
    macro_scaled = add_math(
        group,
        "Fabrication_Scaled_Macro_Height",
        "MULTIPLY",
        (-160, 440),
    )
    macro_plus_scale = add_math(
        group,
        "Macro_Plus_Scale_Height",
        "ADD",
        (-160, 330),
    )
    complete_height = add_math(
        group,
        "Complete_Physical_Height",
        "ADD",
        (70, 250),
    )
    strength = add_math(
        group,
        "Displacement_Strength_Multiplier",
        "MULTIPLY",
        (310, 250),
    )
    link(group, decoded["Micro"], "Value", micro_faded, "Value")
    link(group, decoded["Macro"], "Value", macro_scaled, "Value")
    link(
        group,
        group_input,
        "Fabrication Scale",
        macro_scaled,
        "Value_001",
    )
    link(
        group,
        group_input,
        "Micro Distance Fade",
        micro_faded,
        "Value_001",
    )
    link(group, macro_scaled, "Value", macro_plus_scale, "Value")
    link(group, decoded["Scale"], "Value", macro_plus_scale, "Value_001")
    link(group, macro_plus_scale, "Value", complete_height, "Value")
    link(group, micro_faded, "Value", complete_height, "Value_001")
    link(group, complete_height, "Value", strength, "Value")
    link(
        group,
        group_input,
        "Displacement Strength",
        strength,
        "Value_001",
    )
    link(group, strength, "Value", group_output, "Height M")
    return group


def create_iron_group(
    profile: dict[str, Any],
    *,
    scale_color_image: bpy.types.Image,
    iron_color_image: bpy.types.Image,
    layer_response_image: bpy.types.Image,
    macro_normal_image: bpy.types.Image,
    scale_normal_image: bpy.types.Image,
    micro_normal_image: bpy.types.Image,
    worked_normal_image: bpy.types.Image,
    worked_response_image: bpy.types.Image,
    macro_height_image: bpy.types.Image,
    scale_height_image: bpy.types.Image,
    micro_height_image: bpy.types.Image,
    mask_image: bpy.types.Image,
) -> bpy.types.NodeTree:
    coordinate_group = _build_coordinate_group(profile)
    lane_group = _build_lane_group(
        scale_color_image=scale_color_image,
        iron_color_image=iron_color_image,
        layer_response_image=layer_response_image,
        macro_normal_image=macro_normal_image,
        scale_normal_image=scale_normal_image,
        micro_normal_image=micro_normal_image,
        worked_normal_image=worked_normal_image,
        worked_response_image=worked_response_image,
        macro_height_image=macro_height_image,
        scale_height_image=scale_height_image,
        micro_height_image=micro_height_image,
        mask_image=mask_image,
    )
    scale_group = _build_scale_group()
    hammer_group = _build_hammer_group()
    worked_group = _build_worked_group(profile)
    micro_group = _build_micro_group(profile)
    normal_group = _build_normal_group()
    height_group = _build_height_group(profile)
    group = _new_shader_group(
        IRON_GROUP_NAME,
        "Intact compact scale with optional contact iron and four independent normal lanes.",
        color_tag="SHADER",
    )
    oxidation = _profile_layer(profile, "brown_oxidation_overlay")
    polish = _profile_layer(profile, "contact_polish_overlay")
    strengths = profile["surface_response"]["normal_strengths"]
    for name, default, maximum in (
        ("Brown Oxidation Amount", float(oxidation["amount"]), 1.0),
        ("Contact Polish Amount", float(polish["amount"]), 1.0),
        (
            "Surface Strength",
            float(profile["surface_response"]["maximum_bump_strength"]),
            1.0,
        ),
        ("Macro Normal Strength", float(strengths["macro_hammer"]), 2.0),
        ("Scale Normal Strength", float(strengths["scale_lip"]), 2.0),
        ("Micro Normal Strength", float(strengths["micro_surface"]), 2.0),
        ("Worked Normal Strength", float(strengths["worked_surface"]), 2.0),
        ("Displacement Strength", 1.0, 2.0),
    ):
        _new_socket(
            group,
            name,
            "INPUT",
            "NodeSocketFloat",
            default=default,
            minimum=0.0,
            maximum=maximum,
        )
    for name, socket_type in (
        ("Shader", "NodeSocketShader"),
        ("Base Color", "NodeSocketColor"),
        ("Roughness", "NodeSocketFloat"),
        ("Metalness", "NodeSocketFloat"),
        ("Normal", "NodeSocketVector"),
        ("Normal Color", "NodeSocketColor"),
        ("Height M", "NodeSocketFloat"),
        ("Height Normalized", "NodeSocketFloat"),
        ("Planishing Mask", "NodeSocketFloat"),
        ("Cross Peen Mask", "NodeSocketFloat"),
        ("Quiet Mask", "NodeSocketFloat"),
        ("Scale Mask", "NodeSocketFloat"),
        ("Oxidation Mask", "NodeSocketFloat"),
        ("Contact Mask", "NodeSocketFloat"),
        ("Mask Color", "NodeSocketColor"),
        ("Micro Distance Fade", "NodeSocketFloat"),
        ("Worked Response", "NodeSocketColor"),
    ):
        _new_socket(group, name, "OUTPUT", socket_type)
    group_input = add_node(
        group,
        "NodeGroupInput",
        "Group_Input",
        (-2100, -520),
    )
    group_output = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (2180, 250),
    )
    coordinates = add_node(
        group,
        "ShaderNodeGroup",
        "Component_Metre_And_Fabrication_Coordinates",
        (-2100, 730),
    )
    coordinates.node_tree = coordinate_group
    lanes = add_node(
        group,
        "ShaderNodeGroup",
        "Twelve_Authored_Physical_Lanes",
        (-1830, 670),
    )
    lanes.node_tree = lane_group
    link(
        group,
        coordinates,
        "Macro Atlas Coordinate",
        lanes,
        "Macro Atlas Coordinate",
    )
    link(
        group,
        coordinates,
        "Surface Atlas Coordinate",
        lanes,
        "Surface Atlas Coordinate",
    )
    link(
        group,
        coordinates,
        "Worked Atlas Coordinate",
        lanes,
        "Worked Atlas Coordinate",
    )
    scale = add_node(
        group,
        "ShaderNodeGroup",
        "Continuous_Compact_Scale",
        (-1470, 790),
    )
    scale.node_tree = scale_group
    hammer = add_node(
        group,
        "ShaderNodeGroup",
        "Finite_Hammer_And_Iron",
        (-1470, 420),
    )
    hammer.node_tree = hammer_group
    for source, target in (
        ("Scale Color", "Scale Color"),
        ("Layer Response", "Layer Response"),
        ("Mask Color", "Mask Color"),
    ):
        link(group, lanes, source, scale, target)
    for source, target in (
        ("Iron Color", "Iron Color"),
        ("Layer Response", "Layer Response"),
        ("Mask Color", "Mask Color"),
    ):
        link(group, lanes, source, hammer, target)
    worked = add_node(
        group,
        "ShaderNodeGroup",
        "Finite_Worked_Surface",
        (-1470, 190),
    )
    worked.node_tree = worked_group
    link(
        group,
        lanes,
        "Worked Normal Color",
        worked,
        "Worked Normal Color",
    )
    link(
        group,
        lanes,
        "Worked Response",
        worked,
        "Worked Response",
    )
    micro = add_node(
        group,
        "ShaderNodeGroup",
        "Distance_Aware_Micro_Surface",
        (-1470, -60),
    )
    micro.node_tree = micro_group
    link(
        group,
        lanes,
        "Micro Normal Color",
        micro,
        "Micro Normal Color",
    )
    normal_strength_nodes = {}
    for index, socket_name in enumerate(
        (
            "Macro Normal Strength",
            "Scale Normal Strength",
            "Micro Normal Strength",
            "Worked Normal Strength",
        )
    ):
        multiplier = add_math(
            group,
            f"Global_Times_{socket_name.replace(' ', '_')}",
            "MULTIPLY",
            (-1110, 80 - index * 120),
        )
        link(
            group,
            group_input,
            "Surface Strength",
            multiplier,
            "Value",
        )
        link(
            group,
            group_input,
            socket_name,
            multiplier,
            "Value_001",
        )
        normal_strength_nodes[socket_name] = multiplier
    normal_combine = add_node(
        group,
        "ShaderNodeGroup",
        "Whiteout_Four_Frequency_Normal",
        (-820, 50),
    )
    normal_combine.node_tree = normal_group
    link(
        group,
        lanes,
        "Macro Normal Color",
        normal_combine,
        "Macro Normal Color",
    )
    link(
        group,
        lanes,
        "Scale Normal Color",
        normal_combine,
        "Scale Normal Color",
    )
    link(
        group,
        micro,
        "Normal Color",
        normal_combine,
        "Micro Normal Color",
    )
    link(
        group,
        worked,
        "Normal Color",
        normal_combine,
        "Worked Normal Color",
    )
    for socket_name, multiplier in normal_strength_nodes.items():
        link(
            group,
            multiplier,
            "Value",
            normal_combine,
            socket_name.replace(" Normal", ""),
        )
    normal_map = add_node(
        group,
        "ShaderNodeNormalMap",
        "Combined_Physical_Normal",
        (-470, 80),
    )
    normal_map.space = "TANGENT"
    normal_map.inputs["Strength"].default_value = 1.0
    link(
        group,
        normal_combine,
        "Normal Color",
        normal_map,
        "Color",
    )
    height = add_node(
        group,
        "ShaderNodeGroup",
        "Three_Frequency_Physical_Height",
        (-820, -420),
    )
    height.node_tree = height_group
    for source, target in (
        ("Macro Height Encoded", "Macro Height Encoded"),
        ("Scale Height Encoded", "Scale Height Encoded"),
        ("Micro Height Encoded", "Micro Height Encoded"),
    ):
        link(group, lanes, source, height, target)
    link(
        group,
        micro,
        "Distance Fade",
        height,
        "Micro Distance Fade",
    )
    link(
        group,
        group_input,
        "Displacement Strength",
        height,
        "Displacement Strength",
    )
    link(
        group,
        coordinates,
        "Fabrication Scale",
        height,
        "Fabrication Scale",
    )
    inverse_edge = add_math(
        group,
        "Sheltered_Not_Contact_Edge",
        "SUBTRACT",
        (-1110, 660),
        clamp=True,
    )
    inverse_edge.inputs[0].default_value = 1.0
    oxidation_potential = add_math(
        group,
        "Scale_Tone_Sheltered_Oxidation_Potential",
        "MULTIPLY",
        (-880, 660),
    )
    oxidation_mask = add_math(
        group,
        "Default_Off_Brown_Oxidation",
        "MULTIPLY",
        (-650, 660),
        clamp=True,
    )
    contact_mask = add_math(
        group,
        "Default_Off_Contact_Polish",
        "MULTIPLY",
        (-880, 530),
        clamp=True,
    )
    link(group, coordinates, "Edge Mask", inverse_edge, "Value_001")
    link(group, scale, "Scale Tone Mask", oxidation_potential, "Value")
    link(
        group,
        inverse_edge,
        "Value",
        oxidation_potential,
        "Value_001",
    )
    link(
        group,
        oxidation_potential,
        "Value",
        oxidation_mask,
        "Value",
    )
    link(
        group,
        group_input,
        "Brown Oxidation Amount",
        oxidation_mask,
        "Value_001",
    )
    link(group, coordinates, "Edge Mask", contact_mask, "Value")
    link(
        group,
        group_input,
        "Contact Polish Amount",
        contact_mask,
        "Value_001",
    )
    inverse_oxidation = add_math(
        group,
        "Outside_Optional_Oxidation",
        "SUBTRACT",
        (-390, 650),
        clamp=True,
    )
    inverse_oxidation.inputs[0].default_value = 1.0
    inverse_contact = add_math(
        group,
        "Outside_Optional_Contact",
        "SUBTRACT",
        (-390, 520),
        clamp=True,
    )
    inverse_contact.inputs[0].default_value = 1.0
    exposed_not_oxidized = add_math(
        group,
        "Iron_Exposure_Outside_Oxidation",
        "MULTIPLY",
        (-150, 620),
    )
    retained_exposure = add_math(
        group,
        "Retain_Exposure_Outside_Contact",
        "MULTIPLY",
        (80, 620),
    )
    final_exposure = add_math(
        group,
        "Final_Physical_Iron_Exposure",
        "ADD",
        (310, 620),
        clamp=True,
    )
    link(group, oxidation_mask, "Value", inverse_oxidation, "Value_001")
    link(group, contact_mask, "Value", inverse_contact, "Value_001")
    link(
        group,
        hammer,
        "Exposure Mask",
        exposed_not_oxidized,
        "Value",
    )
    link(
        group,
        inverse_oxidation,
        "Value",
        exposed_not_oxidized,
        "Value_001",
    )
    link(
        group,
        exposed_not_oxidized,
        "Value",
        retained_exposure,
        "Value",
    )
    link(
        group,
        inverse_contact,
        "Value",
        retained_exposure,
        "Value_001",
    )
    link(group, retained_exposure, "Value", final_exposure, "Value")
    link(group, contact_mask, "Value", final_exposure, "Value_001")
    oxide_color = add_mix(
        group,
        "Scale_With_Optional_Brown_Oxide",
        (-150, 950),
    )
    oxide_color.inputs["Color2"].default_value = hex_colour(
        profile["palette"]["brown_oxide_srgb"][3]
    )
    link(group, oxidation_mask, "Value", oxide_color, "Fac")
    link(group, scale, "Color", oxide_color, "Color1")
    scale_rough_base = add_math(
        group,
        "Scale_Roughness_Outside_Oxide",
        "MULTIPLY",
        (-150, 820),
    )
    oxide_rough = add_math(
        group,
        "Oxidation_Roughness",
        "MULTIPLY",
        (-150, 750),
        second=float(oxidation["roughness"]),
    )
    final_scale_rough = add_math(
        group,
        "Final_Scale_Roughness",
        "ADD",
        (80, 800),
        clamp=True,
    )
    link(group, scale, "Roughness", scale_rough_base, "Value")
    link(
        group,
        inverse_oxidation,
        "Value",
        scale_rough_base,
        "Value_001",
    )
    link(group, oxidation_mask, "Value", oxide_rough, "Value")
    link(group, scale_rough_base, "Value", final_scale_rough, "Value")
    link(group, oxide_rough, "Value", final_scale_rough, "Value_001")
    worked_luster = add_math(
        group,
        "Worked_Surface_Luster_Amount",
        "MULTIPLY",
        (310, 800),
        second=float(
            _profile_layer(profile, "worked_surface")[
                "roughness_reduction_max"
            ]
        ),
    )
    worked_scale_rough = add_math(
        group,
        "Worked_Surface_Final_Scale_Roughness",
        "SUBTRACT",
        (520, 800),
        clamp=True,
    )
    link(group, worked, "Worked Mask", worked_luster, "Value")
    link(group, final_scale_rough, "Value", worked_scale_rough, "Value")
    link(
        group,
        worked_luster,
        "Value",
        worked_scale_rough,
        "Value_001",
    )
    iron_rough_base = add_math(
        group,
        "Iron_Roughness_Outside_Contact",
        "MULTIPLY",
        (80, 460),
    )
    contact_rough = add_math(
        group,
        "Contact_Roughness",
        "MULTIPLY",
        (80, 390),
        second=float(polish["roughness"]),
    )
    final_iron_rough = add_math(
        group,
        "Final_Iron_Roughness",
        "ADD",
        (310, 450),
        clamp=True,
    )
    link(group, hammer, "Iron Roughness", iron_rough_base, "Value")
    link(
        group,
        inverse_contact,
        "Value",
        iron_rough_base,
        "Value_001",
    )
    link(group, contact_mask, "Value", contact_rough, "Value")
    link(group, iron_rough_base, "Value", final_iron_rough, "Value")
    link(group, contact_rough, "Value", final_iron_rough, "Value_001")
    oxide_bsdf = add_node(
        group,
        "ShaderNodeBsdfPrincipled",
        "Dielectric_Compact_Forge_Scale",
        (600, 870),
    )
    oxide_bsdf.inputs["Metallic"].default_value = 0.0
    iron_bsdf = add_node(
        group,
        "ShaderNodeBsdfPrincipled",
        "Conductive_Exposed_Iron",
        (600, 510),
    )
    iron_bsdf.inputs["Metallic"].default_value = 1.0
    tangent = add_node(
        group,
        "ShaderNodeTangent",
        "Component_Local_Worked_Tangent",
        (330, 1080),
    )
    tangent.direction_type = "UV_MAP"
    tangent.uv_map = profile["coordinate_contract"]["tangent_uv"]
    for shader in (oxide_bsdf, iron_bsdf):
        link(group, worked, "Anisotropy", shader, "Anisotropic")
        link(
            group,
            worked,
            "Direction",
            shader,
            "Anisotropic Rotation",
        )
        link(group, tangent, "Tangent", shader, "Tangent")
    for shader, color_source, color_socket, rough_source in (
        (
            oxide_bsdf,
            oxide_color,
            "Color",
            worked_scale_rough,
        ),
        (
            iron_bsdf,
            hammer,
            "Iron Color",
            final_iron_rough,
        ),
    ):
        link(group, color_source, color_socket, shader, "Base Color")
        link(group, rough_source, "Value", shader, "Roughness")
        link(group, normal_map, "Normal", shader, "Normal")
    material_mix = add_node(
        group,
        "ShaderNodeMixShader",
        "Physical_Oxide_And_Optional_Contact_Iron",
        (1020, 720),
    )
    link(group, final_exposure, "Value", material_mix, "Fac")
    link(group, oxide_bsdf, "BSDF", material_mix, "Shader")
    group.links.new(iron_bsdf.outputs["BSDF"], material_mix.inputs[2])
    final_color = add_mix(
        group,
        "Data_Base_Color",
        (600, 250),
    )
    link(group, final_exposure, "Value", final_color, "Fac")
    link(group, oxide_color, "Color", final_color, "Color1")
    link(group, hammer, "Iron Color", final_color, "Color2")
    inverse_exposure = add_math(
        group,
        "Outside_Iron_Exposure",
        "SUBTRACT",
        (600, 130),
        clamp=True,
    )
    inverse_exposure.inputs[0].default_value = 1.0
    rough_scale_part = add_math(
        group,
        "Scale_Roughness_Part",
        "MULTIPLY",
        (830, 140),
    )
    rough_iron_part = add_math(
        group,
        "Iron_Roughness_Part",
        "MULTIPLY",
        (830, 70),
    )
    final_rough = add_math(
        group,
        "Data_Final_Roughness",
        "ADD",
        (1050, 110),
        clamp=True,
    )
    link(group, final_exposure, "Value", inverse_exposure, "Value_001")
    link(group, worked_scale_rough, "Value", rough_scale_part, "Value")
    link(
        group,
        inverse_exposure,
        "Value",
        rough_scale_part,
        "Value_001",
    )
    link(group, final_iron_rough, "Value", rough_iron_part, "Value")
    link(
        group,
        final_exposure,
        "Value",
        rough_iron_part,
        "Value_001",
    )
    link(group, rough_scale_part, "Value", final_rough, "Value")
    link(group, rough_iron_part, "Value", final_rough, "Value_001")
    height_encoding = profile["surface_response"]["height_encoding"]
    height_offset = add_math(
        group,
        "Height_Proof_Subtract_Minimum",
        "SUBTRACT",
        (600, -280),
        second=float(height_encoding["minimum_m"]),
    )
    height_normalized = add_math(
        group,
        "Height_Proof_Normalized",
        "DIVIDE",
        (830, -280),
        second=(
            float(height_encoding["maximum_m"])
            - float(height_encoding["minimum_m"])
        ),
        clamp=True,
    )
    link(group, height, "Height M", height_offset, "Value")
    link(group, height_offset, "Value", height_normalized, "Value")
    link(group, material_mix, "Shader", group_output, "Shader")
    link(group, final_color, "Color", group_output, "Base Color")
    link(group, final_rough, "Value", group_output, "Roughness")
    link(group, final_exposure, "Value", group_output, "Metalness")
    link(group, normal_map, "Normal", group_output, "Normal")
    link(
        group,
        normal_combine,
        "Normal Color",
        group_output,
        "Normal Color",
    )
    link(group, height, "Height M", group_output, "Height M")
    link(
        group,
        height_normalized,
        "Value",
        group_output,
        "Height Normalized",
    )
    link(group, hammer, "Planishing Mask", group_output, "Planishing Mask")
    link(
        group,
        hammer,
        "Cross Peen Mask",
        group_output,
        "Cross Peen Mask",
    )
    link(group, hammer, "Quiet Mask", group_output, "Quiet Mask")
    link(group, scale, "Scale Tone Mask", group_output, "Scale Mask")
    link(group, oxidation_mask, "Value", group_output, "Oxidation Mask")
    link(group, contact_mask, "Value", group_output, "Contact Mask")
    link(group, lanes, "Mask Color", group_output, "Mask Color")
    link(
        group,
        lanes,
        "Worked Response",
        group_output,
        "Worked Response",
    )
    link(
        group,
        micro,
        "Distance Fade",
        group_output,
        "Micro Distance Fade",
    )
    return group


def configure_iron_material(
    material: bpy.types.Material,
    group: bpy.types.NodeTree,
    profile: dict[str, Any],
) -> bpy.types.Node:
    material.name = IRON_MATERIAL_NAME
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    iron = add_node(
        tree,
        "ShaderNodeGroup",
        "Reference_Audited_Forged_Iron",
        (-900, 250),
    )
    iron.node_tree = group
    mode = add_node(
        tree,
        "ShaderNodeValue",
        "IGGY_IRON_PROOF_MODE",
        (-900, -330),
    )
    mode.outputs[0].default_value = PROOF_MODES["combined"]
    selectors = {}
    for index, (proof_name, threshold) in enumerate(
        (
            ("roughness", 1.5),
            ("metalness", 2.5),
            ("masks", 3.5),
            ("normal", 4.5),
            ("height", 5.5),
            ("worked_response", 6.5),
        )
    ):
        selector = add_math(
            tree,
            f"Select_{proof_name.title()}_Proof",
            "GREATER_THAN",
            (-650, -250 - index * 110),
            second=threshold,
        )
        link(tree, mode, "Value", selector, "Value")
        selectors[proof_name] = selector
    data_mix = add_mix(
        tree,
        "Base_Or_Roughness_Proof",
        (-410, 10),
    )
    link(tree, selectors["roughness"], "Value", data_mix, "Fac")
    link(tree, iron, "Base Color", data_mix, "Color1")
    link(tree, iron, "Roughness", data_mix, "Color2")
    for index, (proof_name, output_name) in enumerate(
        (
            ("metalness", "Metalness"),
            ("masks", "Mask Color"),
            ("normal", "Normal Color"),
            ("height", "Height Normalized"),
            ("worked_response", "Worked Response"),
        )
    ):
        next_mix = add_mix(
            tree,
            f"Then_{proof_name.title()}_Proof",
            (-180 + index * 230, 10),
        )
        link(tree, selectors[proof_name], "Value", next_mix, "Fac")
        link(tree, data_mix, "Color", next_mix, "Color1")
        link(tree, iron, output_name, next_mix, "Color2")
        data_mix = next_mix
    emission = add_node(
        tree,
        "ShaderNodeEmission",
        "Data_Proof_Emission",
        (760, 0),
    )
    set_input(emission, "Strength", 1.0)
    link(tree, data_mix, "Color", emission, "Color")
    use_proof = add_math(
        tree,
        "Use_Data_Proof",
        "GREATER_THAN",
        (760, -180),
        second=0.5,
    )
    link(tree, mode, "Value", use_proof, "Value")
    shader_select = add_node(
        tree,
        "ShaderNodeMixShader",
        "Combined_Or_Data_Proof",
        (1010, 250),
    )
    link(tree, use_proof, "Value", shader_select, "Fac")
    link(tree, iron, "Shader", shader_select, "Shader")
    tree.links.new(emission.outputs["Emission"], shader_select.inputs[2])
    displacement = add_node(
        tree,
        "ShaderNodeDisplacement",
        "Physical_Metre_Displacement",
        (1010, -120),
    )
    displacement.space = "OBJECT"
    displacement.inputs["Midlevel"].default_value = 0.0
    displacement.inputs["Scale"].default_value = 1.0
    link(tree, iron, "Height M", displacement, "Height")
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (1260, 250),
    )
    link(tree, shader_select, "Shader", output, "Surface")
    link(tree, displacement, "Displacement", output, "Displacement")
    material.diffuse_color = hex_colour(
        profile["palette"]["forge_skin_srgb"][6]
    )
    material.metallic = 0.0
    material.roughness = 0.71
    return mode


def configure_flat_material(name: str, colour: str) -> bpy.types.Material:
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    diffuse = add_node(
        tree,
        "ShaderNodeBsdfDiffuse",
        "Uniform_Diffuse",
        (-100, 0),
    )
    diffuse.inputs["Color"].default_value = hex_colour(colour)
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (140, 0),
    )
    link(tree, diffuse, "BSDF", output, "Surface")
    return material


def set_mesh_attributes(
    obj: bpy.types.Object,
    *,
    seed: int,
    length_m: float,
    width_m: float,
    fabrication_scale: float = 1.0,
) -> None:
    mesh = obj.data
    attributes = {
        "sinc_iron_edge_mask": mesh.attributes.get("sinc_iron_edge_mask")
        or mesh.attributes.new("sinc_iron_edge_mask", "FLOAT", "POINT"),
        "sinc_seed": mesh.attributes.get("sinc_seed")
        or mesh.attributes.new("sinc_seed", "INT", "POINT"),
        "sinc_iron_u_m": mesh.attributes.get("sinc_iron_u_m")
        or mesh.attributes.new("sinc_iron_u_m", "FLOAT", "POINT"),
        "sinc_iron_v_m": mesh.attributes.get("sinc_iron_v_m")
        or mesh.attributes.new("sinc_iron_v_m", "FLOAT", "POINT"),
        "sinc_iron_fabrication_scale": (
            mesh.attributes.get("sinc_iron_fabrication_scale")
            or mesh.attributes.new(
                "sinc_iron_fabrication_scale",
                "FLOAT",
                "POINT",
            )
        ),
    }
    edge_band = min(0.003, width_m * 0.18)
    for index, vertex in enumerate(mesh.vertices):
        u = min(max(float(vertex.co.x) + length_m * 0.5, 0.0), length_m)
        v = min(max(float(vertex.co.z) + width_m * 0.5, 0.0), width_m)
        distance = min(u, length_m - u, v, width_m - v)
        edge = 1.0 - min(max(distance / edge_band, 0.0), 1.0)
        attributes["sinc_iron_edge_mask"].data[index].value = edge
        attributes["sinc_seed"].data[index].value = int(seed)
        attributes["sinc_iron_u_m"].data[index].value = u
        attributes["sinc_iron_v_m"].data[index].value = v
        attributes["sinc_iron_fabrication_scale"].data[
            index
        ].value = float(fabrication_scale)
    tangent_uv = mesh.uv_layers.get("IGGY_IronUV")
    if tangent_uv is None:
        tangent_uv = mesh.uv_layers.new(name="IGGY_IronUV")
    for loop in mesh.loops:
        vertex_index = loop.vertex_index
        tangent_uv.data[loop.index].uv = (
            float(attributes["sinc_iron_u_m"].data[vertex_index].value),
            float(attributes["sinc_iron_v_m"].data[vertex_index].value),
        )


def _set_target_hinge_attributes(
    obj: bpy.types.Object,
    *,
    seed: int,
) -> dict[str, Any]:
    mesh = obj.data
    coordinates = np.asarray(
        [vertex.co[:] for vertex in mesh.vertices],
        dtype=np.float64,
    )
    if len(coordinates) == 0:
        raise RuntimeError(f"Target hinge object {obj.name} has no vertices")
    u_values = coordinates[:, 0] - float(coordinates[:, 0].min())
    v_values = coordinates[:, 2] - float(coordinates[:, 2].min())
    length = max(float(np.ptp(u_values)), 1.0e-6)
    width = max(float(np.ptp(v_values)), 1.0e-6)
    names = (
        ("sinc_iron_u_m", "FLOAT"),
        ("sinc_iron_v_m", "FLOAT"),
        ("sinc_iron_edge_mask", "FLOAT"),
        ("sinc_seed", "INT"),
        ("sinc_iron_fabrication_scale", "FLOAT"),
    )
    attributes = {
        name: mesh.attributes.get(name)
        or mesh.attributes.new(name, data_type, "POINT")
        for name, data_type in names
    }
    edge_band = max(min(0.018, width * 0.18), 0.002)
    for vertex_index in range(len(mesh.vertices)):
        u = float(u_values[vertex_index])
        v = float(v_values[vertex_index])
        distance = min(u, length - u, v, width - v)
        edge = 1.0 - min(max(distance / edge_band, 0.0), 1.0)
        attributes["sinc_iron_u_m"].data[vertex_index].value = u
        attributes["sinc_iron_v_m"].data[vertex_index].value = v
        attributes["sinc_iron_edge_mask"].data[vertex_index].value = edge
        attributes["sinc_seed"].data[vertex_index].value = int(seed)
        attributes["sinc_iron_fabrication_scale"].data[
            vertex_index
        ].value = 10.0
    tangent_uv = mesh.uv_layers.get("IGGY_IronUV")
    if tangent_uv is None:
        tangent_uv = mesh.uv_layers.new(name="IGGY_IronUV")
    for loop in mesh.loops:
        vertex_index = loop.vertex_index
        tangent_uv.data[loop.index].uv = (
            float(u_values[vertex_index]),
            float(v_values[vertex_index]),
        )
    return {
        "dimensions_m": [float(value) for value in obj.dimensions],
        "vertices": len(mesh.vertices),
        "polygons": len(mesh.polygons),
        "fabrication_scale": 10.0,
        "attributes": sorted(attribute.name for attribute in mesh.attributes),
        "uv_layers": sorted(layer.name for layer in mesh.uv_layers),
    }


def append_actual_hinge(
    material: bpy.types.Material,
) -> tuple[list[bpy.types.Object], dict[str, Any]]:
    if not ACTUAL_HINGE_SOURCE.is_file():
        raise FileNotFoundError(
            f"Approved openwork hinge source missing: {ACTUAL_HINGE_SOURCE}"
        )
    old = bpy.data.collections.get(TARGET_COLLECTION_NAME)
    if old is not None:
        for obj in list(old.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.collections.remove(old)
    collection = bpy.data.collections.new(TARGET_COLLECTION_NAME)
    bpy.context.scene.collection.children.link(collection)
    for name in ACTUAL_HINGE_OBJECT_NAMES:
        existing = bpy.data.objects.get(name)
        if existing is not None:
            bpy.data.objects.remove(existing, do_unlink=True)
    with bpy.data.libraries.load(
        str(ACTUAL_HINGE_SOURCE),
        link=False,
    ) as (data_from, data_to):
        missing = sorted(
            set(ACTUAL_HINGE_OBJECT_NAMES) - set(data_from.objects)
        )
        if missing:
            raise RuntimeError(f"Openwork hinge source is missing: {missing}")
        data_to.objects = list(ACTUAL_HINGE_OBJECT_NAMES)
    target_objects = []
    contract = {}
    for seed, obj in enumerate(data_to.objects):
        if obj is None or obj.type != "MESH":
            raise RuntimeError("Openwork hinge import returned a non-mesh")
        collection.objects.link(obj)
        obj.data.materials.clear()
        obj.data.materials.append(material)
        contract[obj.name] = _set_target_hinge_attributes(obj, seed=seed)
        target_objects.append(obj)
    assembly_points = [
        obj.matrix_world @ Vector(corner)
        for obj in target_objects
        for corner in obj.bound_box
    ]
    assembly_min = [
        min(getattr(point, axis) for point in assembly_points)
        for axis in ("x", "y", "z")
    ]
    assembly_max = [
        max(getattr(point, axis) for point in assembly_points)
        for axis in ("x", "y", "z")
    ]
    return target_objects, {
        "source": str(ACTUAL_HINGE_SOURCE),
        "sha256": sha256_file(ACTUAL_HINGE_SOURCE),
        "assembly_bounds_m": {
            "minimum": assembly_min,
            "maximum": assembly_max,
            "span": [
                maximum - minimum
                for minimum, maximum in zip(assembly_min, assembly_max)
            ],
        },
        "objects": contract,
    }


def apply_scale_and_bevel(
    obj: bpy.types.Object,
    *,
    width: float,
) -> None:
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(
        location=False,
        rotation=False,
        scale=True,
    )
    bevel = obj.modifiers.new("Measured_Specimen_Arris", "BEVEL")
    bevel.width = width
    bevel.segments = 3
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    obj.select_set(False)


def _bevelled_rectangle_ring(
    y: float,
    half_x: float,
    half_z: float,
    chamfer: float,
) -> list[tuple[float, float, float]]:
    return [
        (-half_x + chamfer, y, -half_z),
        (half_x - chamfer, y, -half_z),
        (half_x, y, -half_z + chamfer),
        (half_x, y, half_z - chamfer),
        (half_x - chamfer, y, half_z),
        (-half_x + chamfer, y, half_z),
        (-half_x, y, half_z - chamfer),
        (-half_x, y, -half_z + chamfer),
    ]


def _ring_mesh(
    name: str,
    rings: list[list[tuple[float, float, float]]],
) -> bpy.types.Mesh:
    side_count = len(rings[0])
    if any(len(ring) != side_count for ring in rings):
        raise ValueError("forged fastener rings must share one side count")
    vertices = [coordinate for ring in rings for coordinate in ring]
    faces: list[tuple[int, ...]] = []
    for ring_index in range(len(rings) - 1):
        start = ring_index * side_count
        following = (ring_index + 1) * side_count
        for side in range(side_count):
            next_side = (side + 1) % side_count
            faces.append(
                (
                    start + side,
                    start + next_side,
                    following + next_side,
                    following + side,
                )
            )
    faces.append(tuple(reversed(range(side_count))))
    top_start = (len(rings) - 1) * side_count
    faces.append(tuple(top_start + index for index in range(side_count)))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    return mesh


def _create_square_nail(
    collection: bpy.types.Collection,
    material: bpy.types.Material,
    profile: dict[str, Any],
    *,
    bearing_y: float,
) -> bpy.types.Object:
    dimensions = profile["proof_geometry"]["square_nail"]
    half_x = float(dimensions["head_width_m"]) * 0.5
    half_z = float(dimensions["head_depth_m"]) * 0.5
    head_height = float(dimensions["head_height_m"])
    shank = float(dimensions["visible_shank_width_m"]) * 0.5
    rings = [
        _bevelled_rectangle_ring(-0.036, shank, shank, shank * 0.15),
        _bevelled_rectangle_ring(0.0, shank, shank, shank * 0.15),
        _bevelled_rectangle_ring(0.0007, half_x, half_z, 0.0016),
        _bevelled_rectangle_ring(
            head_height * 0.48,
            half_x * 0.88,
            half_z * 0.86,
            0.0022,
        ),
        _bevelled_rectangle_ring(
            head_height,
            half_x * 0.34,
            half_z * 0.30,
            0.0012,
        ),
    ]
    obj = bpy.data.objects.new(
        "IGGY_IronProof_FacetedSquareNail",
        _ring_mesh("IGGY_IronProof_FacetedSquareNailMesh", rings),
    )
    collection.objects.link(obj)
    obj.location = (-0.118, bearing_y, 0.0)
    obj.data.materials.append(material)
    set_mesh_attributes(
        obj,
        seed=1,
        length_m=float(dimensions["head_width_m"]),
        width_m=float(dimensions["head_depth_m"]),
    )
    obj["iggy_fastener_role"] = "faceted_square_nail"
    return obj


def _create_domed_rivet(
    collection: bpy.types.Collection,
    material: bpy.types.Material,
    profile: dict[str, Any],
    *,
    bearing_y: float,
) -> bpy.types.Object:
    dimensions = profile["proof_geometry"]["domed_rivet"]
    radius = float(dimensions["head_diameter_m"]) * 0.5
    shank_radius = float(dimensions["shank_diameter_m"]) * 0.5
    height = float(dimensions["head_height_m"])
    sides = 16

    def circle_ring(y: float, ring_radius: float):
        return [
            (
                math.cos(math.tau * index / sides) * ring_radius,
                y,
                math.sin(math.tau * index / sides) * ring_radius,
            )
            for index in range(sides)
        ]

    rings = [
        circle_ring(-0.022, shank_radius),
        circle_ring(0.0, shank_radius),
        circle_ring(0.0006, radius * 0.96),
        circle_ring(height * 0.35, radius),
        circle_ring(height * 0.68, radius * 0.77),
        circle_ring(height * 0.90, radius * 0.43),
        circle_ring(height, radius * 0.10),
    ]
    obj = bpy.data.objects.new(
        "IGGY_IronProof_FacetedDomedRivet",
        _ring_mesh("IGGY_IronProof_FacetedDomedRivetMesh", rings),
    )
    collection.objects.link(obj)
    obj.location = (0.119, bearing_y, 0.0)
    obj.rotation_euler.y = math.radians(7.0)
    obj.data.materials.append(material)
    set_mesh_attributes(
        obj,
        seed=2,
        length_m=float(dimensions["head_diameter_m"]),
        width_m=float(dimensions["head_diameter_m"]),
    )
    obj["iggy_fastener_role"] = "faceted_domed_rivet"
    return obj


def create_proof_fixture(
    material: bpy.types.Material,
    profile: dict[str, Any],
) -> tuple[list[bpy.types.Object], list[bpy.types.Object]]:
    old = bpy.data.collections.get(PROOF_COLLECTION_NAME)
    if old is not None:
        for obj in list(old.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.collections.remove(old)
    collection = bpy.data.collections.new(PROOF_COLLECTION_NAME)
    bpy.context.scene.collection.children.link(collection)
    geometry: list[bpy.types.Object] = []

    measured_reference_strap = profile["proof_geometry"][
        "measured_reference_strap"
    ]
    length = float(measured_reference_strap["length_m"])
    width = float(measured_reference_strap["width_m"])
    thickness = float(measured_reference_strap["display_thickness_m"])
    bpy.ops.mesh.primitive_cube_add(location=(0.0, 0.0, 0.0))
    strap = bpy.context.object
    strap.name = "IGGY_IronProof_MeasuredReferenceStrap"
    strap.scale = (length * 0.5, thickness * 0.5, width * 0.5)
    apply_scale_and_bevel(strap, width=0.00075)
    for owner in list(strap.users_collection):
        owner.objects.unlink(strap)
    collection.objects.link(strap)
    strap.data.materials.append(material)
    set_mesh_attributes(
        strap,
        seed=0,
        length_m=length,
        width_m=width,
    )
    strap["iggy_reference_object"] = measured_reference_strap[
        "source_object_number"
    ]
    geometry.append(strap)

    bearing_y = thickness * 0.5
    geometry.append(
        _create_square_nail(
            collection,
            material,
            profile,
            bearing_y=bearing_y,
        )
    )
    geometry.append(
        _create_domed_rivet(
            collection,
            material,
            profile,
            bearing_y=bearing_y,
        )
    )
    bpy.ops.mesh.primitive_ico_sphere_add(
        subdivisions=4,
        radius=0.027,
        location=(-0.070, 0.0, -0.076),
    )
    response_sphere = bpy.context.object
    response_sphere.name = "IGGY_IronProof_ResponseSphere"
    for owner in list(response_sphere.users_collection):
        owner.objects.unlink(response_sphere)
    collection.objects.link(response_sphere)
    response_sphere.data.materials.append(material)
    set_mesh_attributes(
        response_sphere,
        seed=3,
        length_m=0.054,
        width_m=0.054,
    )
    bpy.context.view_layer.objects.active = response_sphere
    response_sphere.select_set(True)
    bpy.ops.object.shade_smooth()
    response_sphere.select_set(False)
    response_sphere["iggy_proof_role"] = "curved_reflection_response"
    geometry.append(response_sphere)

    bpy.ops.mesh.primitive_cylinder_add(
        vertices=64,
        radius=0.023,
        depth=0.040,
        end_fill_type="NGON",
        location=(0.030, 0.0, -0.076),
        rotation=(math.radians(90.0), 0.0, 0.0),
    )
    response_cylinder = bpy.context.object
    response_cylinder.name = "IGGY_IronProof_ResponseCylinder"
    apply_scale_and_bevel(response_cylinder, width=0.00045)
    for polygon in response_cylinder.data.polygons:
        polygon.use_smooth = abs(polygon.normal.z) < 0.90
    for owner in list(response_cylinder.users_collection):
        owner.objects.unlink(response_cylinder)
    collection.objects.link(response_cylinder)
    response_cylinder.data.materials.append(material)
    set_mesh_attributes(
        response_cylinder,
        seed=1,
        length_m=0.046,
        width_m=0.046,
    )
    response_cylinder["iggy_proof_role"] = "banded_roughness_response"
    geometry.append(response_cylinder)

    bpy.ops.mesh.primitive_plane_add(
        size=1.4,
        location=(0.0, -0.050, 0.0),
        rotation=(math.radians(90.0), 0.0, 0.0),
    )
    floor = bpy.context.object
    floor.name = "IGGY_IronProof_Floor"
    for owner in list(floor.users_collection):
        owner.objects.unlink(floor)
    collection.objects.link(floor)
    floor.data.materials.append(
        configure_flat_material("IGGY_MAT_IronProofFloor", "#111317")
    )
    geometry.append(floor)
    lights = create_specimen_lights(collection)
    return geometry, lights


def point_object(
    obj: bpy.types.Object,
    target: tuple[float, float, float],
) -> None:
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def add_area_light(
    collection: bpy.types.Collection,
    name: str,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    *,
    energy: float,
    size: float,
    colour: tuple[float, float, float],
) -> bpy.types.Object:
    data = bpy.data.lights.new(name=name, type="AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = colour
    light = bpy.data.objects.new(name=name, object_data=data)
    collection.objects.link(light)
    light.location = location
    point_object(light, target)
    return light


def create_specimen_lights(
    collection: bpy.types.Collection,
) -> list[bpy.types.Object]:
    lights = [
        add_area_light(
            collection,
            "IGGY_IronProof_GrazingKey",
            (-0.46, 0.20, 0.12),
            (0.0, 0.0, 0.0),
            energy=10.0,
            size=0.18,
            colour=(1.0, 0.91, 0.80),
        ),
        add_area_light(
            collection,
            "IGGY_IronProof_CoolFill",
            (0.50, 0.46, 0.12),
            (0.0, 0.0, 0.0),
            energy=2.7,
            size=0.42,
            colour=(0.68, 0.79, 1.0),
        ),
        add_area_light(
            collection,
            "IGGY_IronProof_Rim",
            (0.0, -0.16, 0.34),
            (0.0, 0.0, 0.0),
            energy=6.0,
            size=0.30,
            colour=(1.0, 0.70, 0.48),
        ),
    ]
    lights[0].data.shape = "RECTANGLE"
    lights[0].data.size = 0.18
    lights[0].data.size_y = 0.025
    lights[2].data.shape = "RECTANGLE"
    lights[2].data.size = 0.28
    lights[2].data.size_y = 0.045
    return lights


def create_target_lights(
    collection: bpy.types.Collection,
    target_objects: list[bpy.types.Object],
) -> list[bpy.types.Object]:
    target_points = [
        obj.matrix_world @ Vector(corner)
        for obj in target_objects
        for corner in obj.bound_box
    ]
    target = (
        (min(point.x for point in target_points)
         + max(point.x for point in target_points)) * 0.5,
        0.0,
        (min(point.z for point in target_points)
         + max(point.z for point in target_points)) * 0.5,
    )
    lights = [
        add_area_light(
            collection,
            "IGGY_IronTarget_NeutralKey",
            (0.55, 3.2, 1.55),
            target,
            energy=105.0,
            size=2.2,
            colour=(1.0, 0.96, 0.91),
        ),
        add_area_light(
            collection,
            "IGGY_IronTarget_CoolFill",
            (2.85, 2.1, -1.0),
            target,
            energy=30.0,
            size=1.8,
            colour=(0.72, 0.82, 1.0),
        ),
        add_area_light(
            collection,
            "IGGY_IronTarget_GrazingStrip",
            (-1.0, 0.65, 0.55),
            target,
            energy=72.0,
            size=2.4,
            colour=(1.0, 0.78, 0.58),
        ),
    ]
    lights[2].data.shape = "RECTANGLE"
    lights[2].data.size = 2.4
    lights[2].data.size_y = 0.10
    return lights


def set_visible(objects: list[bpy.types.Object], visible: bool) -> None:
    for obj in objects:
        obj.hide_render = not visible
        obj.hide_viewport = not visible


def scene_light_objects(prefix: str) -> list[bpy.types.Object]:
    return [
        obj
        for obj in bpy.data.objects
        if obj.type == "LIGHT" and obj.name.startswith(prefix)
    ]


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def render_still(scene: bpy.types.Scene, path: Path) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def render_proofs(
    scene: bpy.types.Scene,
    door: bpy.types.Object,
    proof_objects: list[bpy.types.Object],
    specimen_lights: list[bpy.types.Object],
    target_objects: list[bpy.types.Object],
    target_lights: list[bpy.types.Object],
    iron_material: bpy.types.Material,
    clay_material: bpy.types.Material,
    proof_mode: bpy.types.Node,
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    renders = {}
    camera = scene.camera
    camera_state = {
        "location": camera.location.copy(),
        "rotation": camera.rotation_euler.copy(),
        "type": camera.data.type,
        "ortho_scale": camera.data.ortho_scale,
        "lens": camera.data.lens,
    }
    door_lights = [
        light
        for light in scene_light_objects("IGGY_")
        if not light.name.startswith("IGGY_IronProof_")
        and not light.name.startswith("IGGY_IronTarget_")
    ]
    door_support_meshes = [
        obj
        for obj in bpy.data.objects
        if obj.type == "MESH"
        and obj is not door
        and obj not in proof_objects
        and obj not in target_objects
    ]
    set_visible(
        proof_objects + specimen_lights + target_objects + target_lights,
        False,
    )
    set_visible(door_lights, True)
    door.hide_render = False
    door.hide_viewport = False
    proof_mode.outputs[0].default_value = PROOF_MODES["combined"]
    scene.render.resolution_x = args.door_resolution_x
    scene.render.resolution_y = args.door_resolution_y
    renders["door_combined"] = render_still(
        scene,
        output_root / "forged_iron_v1_door_combined.png",
    )

    door.hide_render = True
    door.hide_viewport = True
    set_visible(door_support_meshes, False)
    set_visible(door_lights, False)
    set_visible(proof_objects + specimen_lights, True)
    camera.data.type = "PERSP"
    camera.data.lens = 62.0
    camera.location = (0.31, 0.56, 0.20)
    point_object(camera, (0.0, 0.0, -0.018))
    scene.render.resolution_x = args.proof_resolution_x
    scene.render.resolution_y = args.proof_resolution_y
    proof_mode.outputs[0].default_value = PROOF_MODES["combined"]
    renders["specimen_combined"] = render_still(
        scene,
        output_root / "forged_iron_v1_specimen_combined.png",
    )
    camera.data.lens = 70.0
    camera.location = (-0.29, 0.23, 0.075)
    point_object(camera, (0.01, 0.0, -0.018))
    renders["specimen_grazing"] = render_still(
        scene,
        output_root / "forged_iron_v1_specimen_grazing.png",
    )
    camera.data.lens = 70.0
    camera.location = (0.65, 1.15, 0.32)
    point_object(camera, (0.0, 0.0, -0.020))
    renders["specimen_gameplay_distance"] = render_still(
        scene,
        output_root / "forged_iron_v1_specimen_gameplay_distance.png",
    )

    camera.data.lens = 76.0
    camera.location = (0.0, 0.72, 0.0)
    point_object(camera, (0.0, 0.0, -0.018))
    for name in (
        "base_colour",
        "roughness",
        "metalness",
        "masks",
        "normal",
        "height",
    ):
        proof_mode.outputs[0].default_value = PROOF_MODES[name]
        renders[f"specimen_{name}"] = render_still(
            scene,
            output_root / f"forged_iron_v1_specimen_{name}.png",
        )

    proof_mode.outputs[0].default_value = PROOF_MODES["combined"]
    set_visible(proof_objects + specimen_lights, False)
    set_visible(target_objects + target_lights, True)
    target_bounds = [
        obj.matrix_world @ Vector(corner)
        for obj in target_objects
        for corner in obj.bound_box
    ]
    target_min_x = min(point.x for point in target_bounds)
    target_max_x = max(point.x for point in target_bounds)
    target_min_z = min(point.z for point in target_bounds)
    target_max_z = max(point.z for point in target_bounds)
    target_center = (
        (target_min_x + target_max_x) * 0.5,
        0.0,
        (target_min_z + target_max_z) * 0.5,
    )
    target_span_x = target_max_x - target_min_x
    target_span_z = target_max_z - target_min_z
    target_ortho_scale = max(
        target_span_z * 1.35,
        target_span_x * 1.15,
    )
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = target_ortho_scale
    camera.location = (
        target_center[0],
        max(4.8, target_span_x * 1.25),
        target_center[2],
    )
    point_object(camera, target_center)
    scene.render.resolution_x = args.target_resolution_x
    scene.render.resolution_y = args.target_resolution_y
    for obj in target_objects:
        obj.data.materials.clear()
        obj.data.materials.append(clay_material)
    renders["actual_hinge_clay_front"] = render_still(
        scene,
        output_root / "forged_iron_v1_actual_hinge_clay_front.png",
    )
    for obj in target_objects:
        obj.data.materials.clear()
        obj.data.materials.append(iron_material)
    proof_mode.outputs[0].default_value = PROOF_MODES["combined"]
    renders["actual_hinge_front"] = render_still(
        scene,
        output_root / "forged_iron_v1_actual_hinge_front.png",
    )
    camera.data.type = "PERSP"
    camera.data.lens = 64.0
    camera.location = (
        target_center[0] - target_span_x * 0.04,
        max(4.2, target_span_x * 1.08),
        target_center[2] + max(1.2, target_span_x * 0.36),
    )
    point_object(camera, target_center)
    renders["actual_hinge_grazing"] = render_still(
        scene,
        output_root / "forged_iron_v1_actual_hinge_grazing.png",
    )
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = target_ortho_scale
    camera.location = (
        target_center[0],
        max(4.8, target_span_x * 1.25),
        target_center[2],
    )
    point_object(camera, target_center)
    proof_mode.outputs[0].default_value = PROOF_MODES["base_colour"]
    renders["actual_hinge_base_colour"] = render_still(
        scene,
        output_root / "forged_iron_v1_actual_hinge_base_colour.png",
    )
    proof_mode.outputs[0].default_value = PROOF_MODES["worked_response"]
    renders["actual_hinge_worked_response"] = render_still(
        scene,
        output_root / "forged_iron_v1_actual_hinge_worked_response.png",
    )
    proof_mode.outputs[0].default_value = PROOF_MODES["combined"]
    set_visible(target_objects + target_lights, False)
    set_visible(door_support_meshes, True)
    set_visible(door_lights, True)
    door.hide_render = False
    door.hide_viewport = False
    camera.location = camera_state["location"]
    camera.rotation_euler = camera_state["rotation"]
    camera.data.type = camera_state["type"]
    camera.data.ortho_scale = camera_state["ortho_scale"]
    camera.data.lens = camera_state["lens"]
    scene.render.resolution_x = args.door_resolution_x
    scene.render.resolution_y = args.door_resolution_y
    return renders


def evaluated_door_contract(door: bpy.types.Object) -> dict[str, Any]:
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = door.evaluated_get(depsgraph)
    mesh = evaluated.to_mesh()
    try:
        return {
            "vertices": len(mesh.vertices),
            "polygons": len(mesh.polygons),
            "attributes": {
                attribute.name: {
                    "domain": attribute.domain,
                    "data_type": attribute.data_type,
                }
                for attribute in mesh.attributes
            },
            "materials": [
                material.name if material is not None else None
                for material in mesh.materials
            ],
        }
    finally:
        evaluated.to_mesh_clear()


def dependent_shader_groups(
    root: bpy.types.NodeTree,
) -> list[bpy.types.NodeTree]:
    ordered = []
    pending = [root]
    seen = set()
    while pending:
        tree = pending.pop()
        if tree.name in seen:
            continue
        seen.add(tree.name)
        ordered.append(tree)
        pending.extend(
            node.node_tree
            for node in tree.nodes
            if node.bl_idname == "ShaderNodeGroup"
            and node.node_tree is not None
        )
    return ordered


def validate_contract(
    profile: dict[str, Any],
    samples: dict[str, Any],
    group: bpy.types.NodeTree,
    material: bpy.types.Material,
    door: bpy.types.Object,
    wood_before: dict[str, Any],
    measured_strap: bpy.types.Object,
) -> dict[str, Any]:
    door_contract = evaluated_door_contract(door)
    contract = profile["coordinate_contract"]
    required = {
        contract["variant_attribute"],
        contract["edge_attribute"],
        contract["longitudinal_attribute"],
        contract["cross_stock_attribute"],
        contract["fabrication_scale_attribute"],
    }
    missing = sorted(required - set(door_contract["attributes"]))
    if missing:
        raise RuntimeError(f"Door is missing iron attributes: {missing}")
    wood = bpy.data.materials.get(WOOD_MATERIAL_NAME)
    if wood is None or node_contract(wood.node_tree) != wood_before:
        raise RuntimeError("Approved structural-oak material was modified")
    node_groups = dependent_shader_groups(group)
    all_group_nodes = [
        node for node_group in node_groups for node in node_group.nodes
    ]
    shader_attributes = {
        node.attribute_name
        for node in all_group_nodes
        if node.bl_idname == "ShaderNodeAttribute"
    }
    if shader_attributes != required:
        raise RuntimeError(
            f"Unexpected iron shader attributes: {sorted(shader_attributes)}"
        )
    forbidden_text = ("damage", "scratch", "pit", "blood", "dirt", "soot")
    named = " ".join(node.name.lower() for node in all_group_nodes)
    found_forbidden = sorted(
        token for token in forbidden_text if token in named
    )
    if found_forbidden:
        raise RuntimeError(
            f"Forbidden lanes entered the iron group: {found_forbidden}"
        )
    image_nodes = [
        node
        for node in all_group_nodes
        if node.bl_idname == "ShaderNodeTexImage"
    ]
    noise_nodes = [
        node
        for node in all_group_nodes
        if node.bl_idname in {"ShaderNodeTexNoise", "ShaderNodeTexVoronoi"}
    ]
    principled_nodes = [
        node
        for node in all_group_nodes
        if node.bl_idname == "ShaderNodeBsdfPrincipled"
    ]
    normal_map_nodes = [
        node
        for node in all_group_nodes
        if node.bl_idname == "ShaderNodeNormalMap"
    ]
    displacement_nodes = [
        node
        for node in material.node_tree.nodes
        if node.bl_idname == "ShaderNodeDisplacement"
    ]
    material_group = next(
        node
        for node in material.node_tree.nodes
        if node.bl_idname == "ShaderNodeGroup"
        and node.node_tree == group
    )
    expected_groups = {
        IRON_GROUP_NAME,
        COORDINATE_GROUP_NAME,
        LANE_GROUP_NAME,
        SCALE_GROUP_NAME,
        HAMMER_GROUP_NAME,
        MICRO_GROUP_NAME,
        WORKED_GROUP_NAME,
        NORMAL_GROUP_NAME,
        HEIGHT_GROUP_NAME,
    }
    actual_groups = {node_group.name for node_group in node_groups}
    if actual_groups != expected_groups:
        raise RuntimeError(
            f"Layered node contract drifted: {sorted(actual_groups)}"
        )
    if len(image_nodes) != 12 or noise_nodes:
        raise RuntimeError("Authored map contract was not preserved")
    if (
        len(principled_nodes) != 2
        or len(normal_map_nodes) != 1
        or len(displacement_nodes) != 1
    ):
        raise RuntimeError(
            "Expected two physical layer shaders, one normal-map node, and one displacement node"
        )
    reference = profile["proof_geometry"]["measured_reference_strap"]
    expected = [
        float(reference["length_m"]),
        float(reference["display_thickness_m"]),
        float(reference["width_m"]),
    ]
    actual = [float(value) for value in measured_strap.dimensions]
    if any(abs(left - right) > 1.0e-5 for left, right in zip(actual, expected)):
        raise RuntimeError(
            f"Measured strap proof drifted: actual={actual}, expected={expected}"
        )
    audit = samples["eyedropper_samples"]["audit"]
    return {
        "door": door_contract,
        "shader_attributes": sorted(shader_attributes),
        "node_groups": sorted(actual_groups),
        "group_node_count": len(group.nodes),
        "material_node_count": len(material.node_tree.nodes),
        "image_texture_count": len(image_nodes),
        "generic_noise_node_count": len(noise_nodes),
        "principled_count": len(principled_nodes),
        "normal_map_count": len(normal_map_nodes),
        "displacement_node_count": len(displacement_nodes),
        "normal_strength": material_group.inputs[
            "Surface Strength"
        ].default_value,
        "measured_reference_strap_dimensions_m": actual,
        "sample_audit": {
            "base_hue": audit["base_hue_sample_count"],
            "oxidation_overlay": audit["oxidation_overlay_sample_count"],
            "excluded": audit["excluded_sample_count"],
        },
        "forbidden_lanes_found": found_forbidden,
        "approved_wood_unchanged": True,
    }


def main() -> None:
    args = parse_args()
    profile, samples = load_contract()
    source = args.source.expanduser().resolve()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if not source.is_file():
        raise FileNotFoundError(f"Approved door source missing: {source}")

    generator = load_generator_module()
    pattern_build = generator.generate(
        output_root,
        resolution_x=args.atlas_resolution_x,
        resolution_y=args.atlas_resolution_y,
    )
    outputs = pattern_build["outputs"]

    bpy.ops.wm.open_mainfile(filepath=str(source))
    door = bpy.data.objects.get(DOOR_OBJECT_NAME)
    iron_material = bpy.data.materials.get(SOURCE_IRON_MATERIAL_NAME)
    wood_material = bpy.data.materials.get(WOOD_MATERIAL_NAME)
    if door is None or iron_material is None or wood_material is None:
        raise RuntimeError("Approved door material contract is incomplete")
    wood_before = node_contract(wood_material.node_tree)
    realization = realize_door_for_material_proof(door)
    coordinate_build = ensure_component_local_iron_coordinates(
        door,
        iron_material,
        profile,
    )

    scale_color_image = load_authored_image(
        Path(outputs["scale_color"]["path"]),
        "IGGY_IMG_AuthoredForgeScaleColor_v002",
        non_color=False,
    )
    iron_color_image = load_authored_image(
        Path(outputs["iron_color"]["path"]),
        "IGGY_IMG_AuthoredExposedIronColor_v002",
        non_color=False,
    )
    layer_response_image = load_authored_image(
        Path(outputs["layer_response"]["path"]),
        "IGGY_IMG_AuthoredIronLayerResponse_v002",
        non_color=True,
    )
    normal_images = {
        lane: load_authored_image(
            Path(outputs[f"{lane}_normal"]["path"]),
            f"IGGY_IMG_AuthoredIron{lane.title()}Normal_v002",
            non_color=True,
        )
        for lane in ("macro", "scale", "micro")
    }
    worked_normal_image = load_authored_image(
        Path(outputs["worked_normal"]["path"]),
        "IGGY_IMG_AuthoredIronWorkedNormal_v001",
        non_color=True,
    )
    worked_response_image = load_authored_image(
        Path(outputs["worked_response"]["path"]),
        "IGGY_IMG_AuthoredIronWorkedResponse_v001",
        non_color=True,
    )
    height_images = {
        lane: load_authored_image(
            Path(outputs[f"{lane}_height"]["path"]),
            f"IGGY_IMG_AuthoredIron{lane.title()}Height_v002",
            non_color=True,
        )
        for lane in ("macro", "scale", "micro")
    }
    mask_image = load_authored_image(
        Path(outputs["masks"]["path"]),
        "IGGY_IMG_AuthoredForgedIronMasks_v002",
        non_color=True,
    )
    authored_images = [
        scale_color_image,
        iron_color_image,
        layer_response_image,
        normal_images["macro"],
        normal_images["scale"],
        normal_images["micro"],
        worked_normal_image,
        worked_response_image,
        height_images["macro"],
        height_images["scale"],
        height_images["micro"],
        mask_image,
    ]

    group = create_iron_group(
        profile,
        scale_color_image=scale_color_image,
        iron_color_image=iron_color_image,
        layer_response_image=layer_response_image,
        macro_normal_image=normal_images["macro"],
        scale_normal_image=normal_images["scale"],
        micro_normal_image=normal_images["micro"],
        worked_normal_image=worked_normal_image,
        worked_response_image=worked_response_image,
        macro_height_image=height_images["macro"],
        scale_height_image=height_images["scale"],
        micro_height_image=height_images["micro"],
        mask_image=mask_image,
    )
    proof_mode = configure_iron_material(iron_material, group, profile)
    proof_objects, specimen_lights = create_proof_fixture(
        iron_material,
        profile,
    )
    target_objects, target_contract = append_actual_hinge(iron_material)
    target_collection = bpy.data.collections[TARGET_COLLECTION_NAME]
    target_lights = create_target_lights(target_collection, target_objects)
    clay_material = configure_flat_material(
        "IGGY_MAT_ActualHingeNeutralClay",
        "#77736d",
    )
    measured_strap = bpy.data.objects[
        "IGGY_IronProof_MeasuredReferenceStrap"
    ]

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.018, 0.024, 0.034, 1.0)
    background.inputs["Strength"].default_value = 0.16
    validation = validate_contract(
        profile,
        samples,
        group,
        iron_material,
        door,
        wood_before,
        measured_strap,
    )
    renders = render_proofs(
        scene,
        door,
        proof_objects,
        specimen_lights,
        target_objects,
        target_lights,
        iron_material,
        clay_material,
        proof_mode,
        output_root,
        args,
    )

    for image in authored_images:
        image.pack()
    blend_path = output_root / "forged_iron_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    manifest = {
        "schema": "iggy-forged-iron-build/4.0",
        "status": "LAYERED_NODE_FORGED_IRON_BUILT",
        "profile": str(PROFILE_PATH),
        "reference_sample_audit": {
            "path": str(SAMPLES_PATH),
            "sha256": sha256_file(SAMPLES_PATH),
            "base_hue_count": 11,
            "oxidation_overlay_count": 3,
            "excluded_count": 6,
        },
        "pattern_build": pattern_build,
        "acceptance_fixture_realization": realization,
        "coordinate_build": coordinate_build,
        "actual_hinge": target_contract,
        "source_fixture": {
            "path": str(source),
            "sha256": sha256_file(source),
        },
        "saved_blend": {
            "path": str(blend_path),
            "bytes": blend_path.stat().st_size,
            "sha256": sha256_file(blend_path),
            "authored_images_packed": True,
        },
        "constraints": {
            "uses_ai_generated_imagery": False,
            "uses_real_door_reference": True,
            "uses_measured_stock": True,
            "uses_measured_tool_envelopes": True,
            "uses_generic_hammer_noise": False,
            "uses_active_orange_corrosion": False,
            "uses_uniform_rust": False,
            "uses_damage": False,
            "uses_layered_principled_responses": True,
            "uses_three_frequency_normals": True,
            "uses_four_frequency_normals": True,
            "uses_actual_openwork_hinge": True,
            "uses_metre_displacement": True,
            "unreal_runtime_parity_verified": False,
            "brown_oxidation_default": 0.0,
            "contact_polish_default": 0.0,
            "approved_wood_changed": False,
        },
        "validation": validation,
        "renders": renders,
    }
    manifest_path = output_root / "forged_iron_v1_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
