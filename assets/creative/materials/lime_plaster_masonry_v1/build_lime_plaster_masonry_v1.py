#!/usr/bin/env python3
"""Build and render the geometry-aware lime-plaster masonry acceptance asset."""

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
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "lime_plaster_masonry_v1.json"
GENERATOR_PATH = SCRIPT_ROOT / "generate_lime_plaster_masonry_v1.py"
DEFAULT_SOURCE = (
    SCRIPT_ROOT.parent
    / "forged_iron_v1"
    / "output"
    / "forged_iron_v1.blend"
)
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"

GROUP_NAME = "IGGY_SH_LimePlasterMasonry_v001"
NORMAL_GROUP_NAME = "IGGY_SH_PlasterMasonryNormalCombine_v002"
HEIGHT_GROUP_NAME = "IGGY_SH_PlasterMasonryHeight_v002"
SURFACE_GROUP_NAME = "IGGY_SH_PlasterMasonrySurfaceData_v002"
MATERIAL_NAME = "IGGY_MAT_LimePlasterMasonry_v001"
COLLECTION_NAME = "IGGY_LimePlasterMasonryProof"
WALL_SURFACE_NAME = "IGGY_GiantHouse_PlasterMasonrySurface"
PRIOR_GROUPS = (
    "IGGY_SH_ReferenceStructuralOak_v001",
    "IGGY_SH_ReferenceForgedIron_v001",
)
PRIOR_MATERIALS = (
    "IGGY_MAT_ReferenceStructuralOakDoor_v001",
    "IGGY_MAT_ReferenceForgedIron_v001",
)
ATTRIBUTE_NAMES = (
    "iggy_material_phase",
    "iggy_material_variant",
    "iggy_plaster_coverage",
    "iggy_plaster_layer_state",
    "iggy_transition_edge",
    "iggy_traversal_id",
    "iggy_damp_mask",
    "iggy_soot_mask",
)


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
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
    if profile["profile_id"] != "lime_plaster_masonry_v1":
        raise ValueError("Unexpected lime-plaster masonry profile")
    source_policy = profile["source_policy"]
    if source_policy["ai_generated_reference_capture"]:
        raise ValueError("Canonical material intent may not use AI imagery")
    if source_policy["ai_generated_runtime_texture"]:
        raise ValueError("AI imagery cannot be used as a runtime texture")
    if source_policy["raw_reference_pixels_used_as_runtime_texture"]:
        raise ValueError("Raw reference pixels cannot enter runtime maps")
    if source_policy["legacy_ai_experiments_are_build_inputs"]:
        raise ValueError("Legacy AI experiments may not be build inputs")
    if any(
        float(profile["default_overlays"][name]) != 0.0
        for name in ("damp", "soot", "edge_accumulation")
    ):
        raise ValueError("Narrative overlays must default off")
    return profile


def load_generator():
    spec = importlib.util.spec_from_file_location(
        "iggy_lime_plaster_masonry_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("Unable to load lime-plaster masonry generator")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def srgb_channel_to_linear(value: float) -> float:
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def hex_colour(value: str) -> tuple[float, float, float, float]:
    value = value.removeprefix("#")
    channels = [
        int(value[index : index + 2], 16) / 255.0 for index in (0, 2, 4)
    ]
    return tuple(srgb_channel_to_linear(channel) for channel in channels) + (
        1.0,
    )


def node_contract(tree: bpy.types.NodeTree | None) -> dict[str, Any] | None:
    if tree is None:
        return None
    nodes = []
    for node in tree.nodes:
        payload: dict[str, Any] = {
            "name": node.name,
            "type": node.bl_idname,
        }
        if node.bl_idname == "ShaderNodeTexImage":
            payload["image"] = node.image.name if node.image else None
        if node.bl_idname == "ShaderNodeAttribute":
            payload["attribute"] = node.attribute_name
        if node.bl_idname == "ShaderNodeGroup":
            payload["group"] = node.node_tree.name if node.node_tree else None
        nodes.append(payload)
    links = sorted(
        (
            link.from_node.name,
            link.from_socket.name,
            link.to_node.name,
            link.to_socket.name,
        )
        for link in tree.links
    )
    return {
        "name": tree.name,
        "nodes": sorted(nodes, key=lambda entry: entry["name"]),
        "links": links,
    }


def prior_contracts() -> dict[str, Any]:
    contracts: dict[str, Any] = {"groups": {}, "materials": {}}
    for name in PRIOR_GROUPS:
        contracts["groups"][name] = node_contract(bpy.data.node_groups.get(name))
    for name in PRIOR_MATERIALS:
        material = bpy.data.materials.get(name)
        contracts["materials"][name] = node_contract(
            material.node_tree if material else None
        )
    return contracts


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
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMath", name, location)
    node.operation = operation
    if second is not None:
        node.inputs[1].default_value = second
    return node


def add_mix(
    tree: bpy.types.NodeTree,
    name: str,
    location: tuple[float, float],
    *,
    blend_type: str = "MIX",
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMixRGB", name, location)
    node.blend_type = blend_type
    node.use_clamp = True
    return node


def load_image(path: Path, *, non_color: bool) -> bpy.types.Image:
    image = bpy.data.images.load(str(path), check_existing=True)
    image.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    image.pack()
    return image


def add_smooth_weight(
    tree: bpy.types.NodeTree,
    name: str,
    state: bpy.types.Node,
    start: float,
    end: float,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMapRange", name, location)
    node.clamp = True
    node.interpolation_type = "SMOOTHERSTEP"
    node.inputs["From Min"].default_value = start
    node.inputs["From Max"].default_value = end
    node.inputs["To Min"].default_value = 0.0
    node.inputs["To Max"].default_value = 1.0
    link(tree, state, "Fac", node, "Value")
    return node


def new_shader_group(
    name: str,
    description: str,
    *,
    color_tag: str,
) -> bpy.types.NodeTree:
    previous = bpy.data.node_groups.get(name)
    if previous is not None:
        bpy.data.node_groups.remove(previous, do_unlink=True)
    group = bpy.data.node_groups.new(name, "ShaderNodeTree")
    group.description = description
    group.color_tag = color_tag
    return group


def new_socket(
    group: bpy.types.NodeTree,
    name: str,
    direction: str,
    socket_type: str,
    *,
    default: float | None = None,
) -> None:
    socket = group.interface.new_socket(
        name=name,
        in_out=direction,
        socket_type=socket_type,
    )
    if default is not None:
        socket.default_value = default


def decode_tangent_normal(
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
        (location[0] + 180, location[1]),
    )
    offset.operation = "ADD"
    offset.inputs[1].default_value = (-1.0, -1.0, -1.0)
    link(tree, source, source_socket, multiply, "Vector")
    link(tree, multiply, "Vector", offset, "Vector")
    return offset


def strengthen_tangent_normal(
    tree: bpy.types.NodeTree,
    normal: bpy.types.Node,
    strength_source: bpy.types.Node,
    strength_socket: str,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    vector = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        f"{name}_XY_Strength",
        location,
    )
    vector.inputs["Z"].default_value = 1.0
    link(tree, strength_source, strength_socket, vector, "X")
    link(tree, strength_source, strength_socket, vector, "Y")
    multiply = add_node(
        tree,
        "ShaderNodeVectorMath",
        f"{name}_Apply_XY_Strength",
        (location[0] + 180, location[1]),
    )
    multiply.operation = "MULTIPLY"
    link(tree, normal, "Vector", multiply, "Vector")
    tree.links.new(vector.outputs["Vector"], multiply.inputs[1])
    return multiply


def whiteout_combine(
    tree: bpy.types.NodeTree,
    first: bpy.types.Node,
    second: bpy.types.Node,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    first_xyz = add_node(
        tree,
        "ShaderNodeSeparateXYZ",
        f"{name}_First_XYZ",
        location,
    )
    second_xyz = add_node(
        tree,
        "ShaderNodeSeparateXYZ",
        f"{name}_Second_XYZ",
        (location[0], location[1] - 170),
    )
    x_add = add_math(
        tree,
        f"{name}_Add_X",
        "ADD",
        (location[0] + 180, location[1] + 35),
    )
    y_add = add_math(
        tree,
        f"{name}_Add_Y",
        "ADD",
        (location[0] + 180, location[1] - 45),
    )
    z_multiply = add_math(
        tree,
        f"{name}_Multiply_Z",
        "MULTIPLY",
        (location[0] + 180, location[1] - 125),
    )
    combined = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        f"{name}_Whiteout_Vector",
        (location[0] + 360, location[1] - 25),
    )
    normalized = add_node(
        tree,
        "ShaderNodeVectorMath",
        f"{name}_Normalize",
        (location[0] + 540, location[1] - 25),
    )
    normalized.operation = "NORMALIZE"
    link(tree, first, "Vector", first_xyz, "Vector")
    link(tree, second, "Vector", second_xyz, "Vector")
    link(tree, first_xyz, "X", x_add, "Value")
    link(tree, second_xyz, "X", x_add, "Value_001")
    link(tree, first_xyz, "Y", y_add, "Value")
    link(tree, second_xyz, "Y", y_add, "Value_001")
    link(tree, first_xyz, "Z", z_multiply, "Value")
    link(tree, second_xyz, "Z", z_multiply, "Value_001")
    link(tree, x_add, "Value", combined, "X")
    link(tree, y_add, "Value", combined, "Y")
    link(tree, z_multiply, "Value", combined, "Z")
    link(tree, combined, "Vector", normalized, "Vector")
    return normalized


def create_normal_group() -> bpy.types.NodeTree:
    group = new_shader_group(
        NORMAL_GROUP_NAME,
        "Whiteout-combine form and distance-faded detail normals per surface.",
        color_tag="VECTOR",
    )
    for name, socket_type, default in (
        ("Plaster Form Normal", "NodeSocketColor", None),
        ("Plaster Detail Normal", "NodeSocketColor", None),
        ("Masonry Form Normal", "NodeSocketColor", None),
        ("Masonry Detail Normal", "NodeSocketColor", None),
        ("Plaster Weight", "NodeSocketFloat", None),
        ("Form Strength", "NodeSocketFloat", 1.0),
        ("Detail Strength", "NodeSocketFloat", 0.58),
    ):
        new_socket(
            group,
            name,
            "INPUT",
            socket_type,
            default=default,
        )
    new_socket(group, "Normal Color", "OUTPUT", "NodeSocketColor")
    new_socket(group, "Detail Distance Fade", "OUTPUT", "NodeSocketFloat")
    inputs = add_node(group, "NodeGroupInput", "Group_Input", (-1700, 280))
    outputs = add_node(group, "NodeGroupOutput", "Group_Output", (1450, 280))
    camera = add_node(
        group,
        "ShaderNodeCameraData",
        "Camera_Distance",
        (-1700, -520),
    )
    fade = add_node(
        group,
        "ShaderNodeMapRange",
        "Detail_Full_At_One_Metre_Zero_At_Eight",
        (-1480, -520),
    )
    fade.clamp = True
    fade.interpolation_type = "SMOOTHERSTEP"
    fade.inputs["From Min"].default_value = 1.0
    fade.inputs["From Max"].default_value = 8.0
    fade.inputs["To Min"].default_value = 1.0
    fade.inputs["To Max"].default_value = 0.0
    link(group, camera, "View Distance", fade, "Value")
    detail_strength = add_math(
        group,
        "Distance_Faded_Detail_Strength",
        "MULTIPLY",
        (-1260, -440),
    )
    link(group, inputs, "Detail Strength", detail_strength, "Value")
    link(group, fade, "Result", detail_strength, "Value_001")

    combined_normals: dict[str, bpy.types.Node] = {}
    for row, surface in enumerate(("Plaster", "Masonry")):
        y = 650 - row * 610
        form = decode_tangent_normal(
            group,
            inputs,
            f"{surface} Form Normal",
            f"{surface}_Form",
            (-1450, y),
        )
        detail = decode_tangent_normal(
            group,
            inputs,
            f"{surface} Detail Normal",
            f"{surface}_Detail",
            (-1450, y - 260),
        )
        form_strengthened = strengthen_tangent_normal(
            group,
            form,
            inputs,
            "Form Strength",
            f"{surface}_Form",
            (-1050, y),
        )
        detail_strengthened = strengthen_tangent_normal(
            group,
            detail,
            detail_strength,
            "Value",
            f"{surface}_Detail",
            (-1050, y - 260),
        )
        combined_normals[surface] = whiteout_combine(
            group,
            form_strengthened,
            detail_strengthened,
            f"{surface}_Form_And_Detail",
            (-650, y - 20),
        )

    surface_mix = add_node(
        group,
        "ShaderNodeMixRGB",
        "Masonry_To_Plaster_Normal",
        (380, 260),
    )
    link(group, inputs, "Plaster Weight", surface_mix, "Fac")
    link(group, combined_normals["Masonry"], "Vector", surface_mix, "Color1")
    link(group, combined_normals["Plaster"], "Vector", surface_mix, "Color2")
    normalize = add_node(
        group,
        "ShaderNodeVectorMath",
        "Normalize_Selected_Surface",
        (580, 260),
    )
    normalize.operation = "NORMALIZE"
    link(group, surface_mix, "Color", normalize, "Vector")
    encode_scale = add_node(
        group,
        "ShaderNodeVectorMath",
        "Encode_Normal_Times_Half",
        (780, 260),
    )
    encode_scale.operation = "MULTIPLY"
    encode_scale.inputs[1].default_value = (0.5, 0.5, 0.5)
    encode_offset = add_node(
        group,
        "ShaderNodeVectorMath",
        "Encode_Normal_Plus_Half",
        (980, 260),
    )
    encode_offset.operation = "ADD"
    encode_offset.inputs[1].default_value = (0.5, 0.5, 0.5)
    link(group, normalize, "Vector", encode_scale, "Vector")
    link(group, encode_scale, "Vector", encode_offset, "Vector")
    link(group, encode_offset, "Vector", outputs, "Normal Color")
    link(group, fade, "Result", outputs, "Detail Distance Fade")
    return group


def create_height_group(
    height_ranges: dict[str, list[float]],
) -> bpy.types.NodeTree:
    group = new_shader_group(
        HEIGHT_GROUP_NAME,
        "Decode 16-bit plaster and masonry heights into metres.",
        color_tag="CONVERTER",
    )
    for name in (
        "Plaster Height Encoded",
        "Masonry Height Encoded",
        "Plaster Weight",
    ):
        new_socket(group, name, "INPUT", "NodeSocketFloat")
    new_socket(group, "Height M", "OUTPUT", "NodeSocketFloat")
    inputs = add_node(group, "NodeGroupInput", "Group_Input", (-700, 160))
    outputs = add_node(group, "NodeGroupOutput", "Group_Output", (450, 160))
    decoded: dict[str, bpy.types.Node] = {}
    for index, surface in enumerate(("Plaster", "Masonry")):
        node = add_node(
            group,
            "ShaderNodeMapRange",
            f"Decode_{surface}_Height_To_Metres",
            (-420, 300 - index * 240),
        )
        node.clamp = True
        minimum, maximum = height_ranges[surface.lower()]
        node.inputs["From Min"].default_value = 0.0
        node.inputs["From Max"].default_value = 1.0
        node.inputs["To Min"].default_value = float(minimum)
        node.inputs["To Max"].default_value = float(maximum)
        link(group, inputs, f"{surface} Height Encoded", node, "Value")
        decoded[surface] = node
    mix_height = add_math(
        group,
        "Select_Surface_Height",
        "MULTIPLY_ADD",
        (40, 160),
    )
    difference = add_math(
        group,
        "Plaster_Minus_Masonry",
        "SUBTRACT",
        (-180, 220),
    )
    link(group, decoded["Plaster"], "Result", difference, "Value")
    link(group, decoded["Masonry"], "Result", difference, "Value_001")
    link(group, difference, "Value", mix_height, "Value")
    link(group, inputs, "Plaster Weight", mix_height, "Value_001")
    link(group, decoded["Masonry"], "Result", mix_height, "Value_002")
    link(group, mix_height, "Value", outputs, "Height M")
    return group


def create_surface_data_group(
    profile: dict[str, Any],
    output_root: Path,
) -> bpy.types.NodeTree:
    previous = bpy.data.node_groups.get(SURFACE_GROUP_NAME)
    if previous is not None:
        bpy.data.node_groups.remove(previous, do_unlink=True)
    group = bpy.data.node_groups.new(SURFACE_GROUP_NAME, "ShaderNodeTree")
    group.color_tag = "SHADER"
    group.description = (
        "Authored plaster, mortar, and stone data reconstructed from "
        "face-owned construction state."
    )
    for name, socket_type in (
        ("Combined Color", "NodeSocketColor"),
        ("Combined Roughness", "NodeSocketFloat"),
        ("Height M", "NodeSocketFloat"),
        ("Height Proof", "NodeSocketFloat"),
        ("Normal Color", "NodeSocketColor"),
        ("Combined AO", "NodeSocketFloat"),
        ("Stone Color", "NodeSocketColor"),
        ("Mortar Color", "NodeSocketColor"),
        ("Plaster Color", "NodeSocketColor"),
        ("Stone Roughness", "NodeSocketFloat"),
        ("Mortar Roughness", "NodeSocketFloat"),
        ("Plaster Roughness", "NodeSocketFloat"),
        ("Stone Weight", "NodeSocketFloat"),
        ("Mortar Weight", "NodeSocketFloat"),
        ("Plaster Weight", "NodeSocketFloat"),
        ("Detail Distance Fade", "NodeSocketFloat"),
        ("Layer State Proof", "NodeSocketColor"),
        ("Transition Edge Proof", "NodeSocketColor"),
        ("Traversal Proof", "NodeSocketColor"),
    ):
        group.interface.new_socket(
            name=name,
            in_out="OUTPUT",
            socket_type=socket_type,
        )
    output = add_node(group, "NodeGroupOutput", "Group_Output", (2140, 300))
    texture_manifest = json.loads(
        (output_root / "lime_plaster_masonry_v1_manifest.json").read_text()
    )
    normal_group = create_normal_group()
    height_group = create_height_group(
        texture_manifest["height_range_m"],
    )

    coordinates = add_node(
        group,
        "ShaderNodeTexCoord",
        "Object_Metre_Position",
        (-2200, 1180),
    )
    separate_position = add_node(
        group, "ShaderNodeSeparateXYZ", "Wall_XZ_Metres", (-2000, 1180)
    )
    link(group, coordinates, "Object", separate_position, "Vector")
    phase = add_node(
        group, "ShaderNodeAttribute", "Per_Instance_Material_Phase", (-2200, 930)
    )
    phase.attribute_name = profile["coordinate_contract"]["phase_attribute"]
    phase_scale = add_math(
        group, "Phase_In_Metres", "MULTIPLY", (-1990, 930), second=4.0
    )
    link(group, phase, "Fac", phase_scale, "Value")
    phase_x = add_math(group, "Phase_Shifted_X", "ADD", (-1780, 1150))
    link(group, separate_position, "X", phase_x, "Value")
    link(group, phase_scale, "Value", phase_x, "Value_001")
    scale_x = add_math(
        group, "Four_Metre_Tile_X", "MULTIPLY", (-1570, 1150), second=0.25
    )
    scale_z = add_math(
        group, "Four_Metre_Tile_Z", "MULTIPLY", (-1570, 1030), second=0.25
    )
    link(group, phase_x, "Value", scale_x, "Value")
    link(group, separate_position, "Z", scale_z, "Value")
    coordinate = add_node(
        group, "ShaderNodeCombineXYZ", "Object_XZ_Tile_Coordinate", (-1360, 1100)
    )
    link(group, scale_x, "Value", coordinate, "X")
    link(group, scale_z, "Value", coordinate, "Y")

    texture_specs = (
        ("Plaster_Base_Colour", "lime_plaster_v1_basecolor.png", False, 980),
        ("Plaster_ORM", "lime_plaster_v1_orm.png", True, 820),
        ("Plaster_Height", "lime_plaster_v1_height.png", True, 660),
        ("Plaster_Form_Normal", "lime_plaster_v1_form_normal.png", True, 500),
        ("Plaster_Detail_Normal", "lime_plaster_v1_detail_normal.png", True, 340),
        ("Masonry_Base_Colour", "giant_masonry_v1_basecolor.png", False, 160),
        ("Masonry_ORM", "giant_masonry_v1_orm.png", True, 0),
        ("Masonry_Height", "giant_masonry_v1_height.png", True, -160),
        ("Masonry_Form_Normal", "giant_masonry_v1_form_normal.png", True, -320),
        ("Masonry_Detail_Normal", "giant_masonry_v1_detail_normal.png", True, -480),
        ("Masonry_Masks", "giant_masonry_v1_masks.png", True, -640),
        (
            "Stylization_Lanes",
            "lime_plaster_masonry_v1_stylization.png",
            True,
            -800,
        ),
    )
    textures: dict[str, bpy.types.Node] = {}
    for name, filename, non_color, y in texture_specs:
        node = add_node(group, "ShaderNodeTexImage", name, (-1120, y))
        node.image = load_image(output_root / filename, non_color=non_color)
        node.extension = "REPEAT"
        node.interpolation = "Linear"
        link(group, coordinate, "Vector", node, "Vector")
        textures[name] = node

    plaster_orm = add_node(
        group, "ShaderNodeSeparateColor", "Separate_Plaster_ORM", (-870, 630)
    )
    plaster_orm.mode = "RGB"
    link(group, textures["Plaster_ORM"], "Color", plaster_orm, "Color")
    masonry_orm = add_node(
        group, "ShaderNodeSeparateColor", "Separate_Masonry_ORM", (-870, 80)
    )
    masonry_orm.mode = "RGB"
    link(group, textures["Masonry_ORM"], "Color", masonry_orm, "Color")
    masks = add_node(
        group, "ShaderNodeSeparateColor", "Separate_Mortar_Ink_Traversal", (-870, -250)
    )
    masks.mode = "RGB"
    link(group, textures["Masonry_Masks"], "Color", masks, "Color")
    plaster_height = add_node(
        group, "ShaderNodeRGBToBW", "Plaster_Height_Value", (-850, 450)
    )
    masonry_height = add_node(
        group, "ShaderNodeRGBToBW", "Masonry_Height_Value", (-850, -90)
    )
    link(group, textures["Plaster_Height"], "Color", plaster_height, "Color")
    link(group, textures["Masonry_Height"], "Color", masonry_height, "Color")

    attributes: dict[str, bpy.types.Node] = {}
    for index, (label, key) in enumerate(
        (
            ("Plaster_Coverage", "plaster_coverage_attribute"),
            ("Construction_Layer_State", "layer_state_attribute"),
            ("Authored_Transition_Edge", "transition_edge_attribute"),
            ("Traversal_Material_ID", "traversal_id_attribute"),
            ("Optional_Damp_Overlay", "damp_attribute"),
            ("Optional_Soot_Overlay", "soot_attribute"),
        )
    ):
        node = add_node(
            group,
            "ShaderNodeAttribute",
            label,
            (-2200, 660 - index * 160),
        )
        node.attribute_name = profile["coordinate_contract"][key]
        attributes[key] = node
    state = attributes["layer_state_attribute"]
    weights = {
        "mortar": add_smooth_weight(
            group, "Stone_To_Flush_Mortar", state, 0.15, 0.21, (-600, 1140)
        ),
        "scratch": add_smooth_weight(
            group, "Mortar_To_Scratch_Coat", state, 0.27, 0.34, (-600, 1030)
        ),
        "brown": add_smooth_weight(
            group, "Scratch_To_Brown_Coat", state, 0.48, 0.56, (-600, 920)
        ),
        "finish": add_smooth_weight(
            group, "Brown_To_Lime_Finish", state, 0.72, 0.80, (-600, 810)
        ),
    }
    not_plaster = add_math(
        group,
        "One_Minus_Plaster_State",
        "SUBTRACT",
        (-360, 1050),
        second=0.0,
    )
    not_plaster.inputs[0].default_value = 1.0
    link(group, weights["scratch"], "Result", not_plaster, "Value_001")
    mortar_union = add_math(
        group,
        "Authored_Mortar_Or_Flush_Mortar_State",
        "MAXIMUM",
        (-360, 930),
    )
    link(group, masks, "Red", mortar_union, "Value")
    link(group, weights["mortar"], "Result", mortar_union, "Value_001")
    mortar_weight = add_math(
        group,
        "Physical_Mortar_Weight",
        "MULTIPLY",
        (-130, 1010),
    )
    link(group, not_plaster, "Value", mortar_weight, "Value")
    link(group, mortar_union, "Value", mortar_weight, "Value_001")
    not_mortar = add_math(
        group,
        "One_Minus_Mortar",
        "SUBTRACT",
        (-130, 900),
        second=0.0,
    )
    not_mortar.inputs[0].default_value = 1.0
    link(group, mortar_union, "Value", not_mortar, "Value_001")
    stone_weight = add_math(
        group,
        "Physical_Stone_Weight",
        "MULTIPLY",
        (100, 960),
    )
    link(group, not_plaster, "Value", stone_weight, "Value")
    link(group, not_mortar, "Value", stone_weight, "Value_001")

    mortar_color = add_mix(
        group, "Lime_Mortar_From_Plaster_Body", (-560, 640), blend_type="MULTIPLY"
    )
    mortar_color.inputs["Fac"].default_value = 1.0
    mortar_color.inputs["Color2"].default_value = hex_colour("#8f8268")
    link(group, textures["Plaster_Base_Colour"], "Color", mortar_color, "Color1")
    scratch_color = add_mix(
        group, "Sand_Rich_Scratch_Coat", (-560, 500), blend_type="MULTIPLY"
    )
    scratch_color.inputs["Fac"].default_value = 1.0
    scratch_color.inputs["Color2"].default_value = hex_colour("#a9875c")
    link(group, textures["Plaster_Base_Colour"], "Color", scratch_color, "Color1")
    brown_color = add_mix(
        group, "Floated_Brown_Coat", (-560, 360), blend_type="MULTIPLY"
    )
    brown_color.inputs["Fac"].default_value = 1.0
    brown_color.inputs["Color2"].default_value = hex_colour("#c2ad85")
    link(group, textures["Plaster_Base_Colour"], "Color", brown_color, "Color1")

    color_mortar = add_mix(group, "Masonry_To_Mortar_State", (-250, 700))
    link(group, weights["mortar"], "Result", color_mortar, "Factor")
    link(group, textures["Masonry_Base_Colour"], "Color", color_mortar, "Color1")
    link(group, mortar_color, "Color", color_mortar, "Color2")
    color_scratch = add_mix(group, "Mortar_To_Scratch_State", (0, 700))
    link(group, weights["scratch"], "Result", color_scratch, "Factor")
    link(group, color_mortar, "Color", color_scratch, "Color1")
    link(group, scratch_color, "Color", color_scratch, "Color2")
    color_brown = add_mix(group, "Scratch_To_Brown_State", (250, 700))
    link(group, weights["brown"], "Result", color_brown, "Factor")
    link(group, color_scratch, "Color", color_brown, "Color1")
    link(group, brown_color, "Color", color_brown, "Color2")
    combined_color = add_mix(group, "Brown_To_Finish_State", (500, 700))
    link(group, weights["finish"], "Result", combined_color, "Factor")
    link(group, color_brown, "Color", combined_color, "Color1")
    link(group, textures["Plaster_Base_Colour"], "Color", combined_color, "Color2")

    rough_mortar = add_mix(group, "Masonry_To_Mortar_Roughness", (-250, 280))
    rough_mortar.inputs["Color2"].default_value = (0.86, 0.86, 0.86, 1.0)
    link(group, weights["mortar"], "Result", rough_mortar, "Factor")
    link(group, masonry_orm, "Green", rough_mortar, "Color1")
    rough_scratch = add_mix(group, "Mortar_To_Scratch_Roughness", (0, 280))
    rough_scratch.inputs["Color2"].default_value = (0.82, 0.82, 0.82, 1.0)
    link(group, weights["scratch"], "Result", rough_scratch, "Factor")
    link(group, rough_mortar, "Color", rough_scratch, "Color1")
    rough_brown = add_mix(group, "Scratch_To_Brown_Roughness", (250, 280))
    rough_brown.inputs["Color2"].default_value = (0.77, 0.77, 0.77, 1.0)
    link(group, weights["brown"], "Result", rough_brown, "Factor")
    link(group, rough_scratch, "Color", rough_brown, "Color1")
    rough_finish = add_mix(group, "Brown_To_Finish_Roughness", (500, 280))
    link(group, weights["finish"], "Result", rough_finish, "Factor")
    link(group, rough_brown, "Color", rough_finish, "Color1")
    link(group, plaster_orm, "Green", rough_finish, "Color2")
    rough_value = add_node(
        group, "ShaderNodeRGBToBW", "Combined_Roughness_Value", (750, 280)
    )
    link(group, rough_finish, "Color", rough_value, "Color")

    height_mix = add_mix(group, "Masonry_To_Plaster_Surface_Height", (500, 50))
    link(group, weights["scratch"], "Result", height_mix, "Factor")
    link(group, masonry_height, "Val", height_mix, "Color1")
    link(group, plaster_height, "Val", height_mix, "Color2")
    height_value = add_node(
        group, "ShaderNodeRGBToBW", "Combined_Height_Value", (750, 50)
    )
    link(group, height_mix, "Color", height_value, "Color")
    normal_system = add_node(
        group,
        "ShaderNodeGroup",
        "Layered_Form_And_Detail_Normals",
        (760, 520),
    )
    normal_system.node_tree = normal_group
    link(
        group,
        textures["Plaster_Form_Normal"],
        "Color",
        normal_system,
        "Plaster Form Normal",
    )
    link(
        group,
        textures["Plaster_Detail_Normal"],
        "Color",
        normal_system,
        "Plaster Detail Normal",
    )
    link(
        group,
        textures["Masonry_Form_Normal"],
        "Color",
        normal_system,
        "Masonry Form Normal",
    )
    link(
        group,
        textures["Masonry_Detail_Normal"],
        "Color",
        normal_system,
        "Masonry Detail Normal",
    )
    link(
        group,
        weights["scratch"],
        "Result",
        normal_system,
        "Plaster Weight",
    )
    normal_system.inputs["Form Strength"].default_value = 0.72
    normal_system.inputs["Detail Strength"].default_value = 0.52

    height_system = add_node(
        group,
        "ShaderNodeGroup",
        "Decode_Authored_Height_To_Metres",
        (980, 40),
    )
    height_system.node_tree = height_group
    link(
        group,
        plaster_height,
        "Val",
        height_system,
        "Plaster Height Encoded",
    )
    link(
        group,
        masonry_height,
        "Val",
        height_system,
        "Masonry Height Encoded",
    )
    link(
        group,
        weights["scratch"],
        "Result",
        height_system,
        "Plaster Weight",
    )
    ao_mix = add_mix(
        group,
        "Masonry_To_Plaster_AO",
        (1000, 210),
    )
    link(group, weights["scratch"], "Result", ao_mix, "Fac")
    link(group, masonry_orm, "Red", ao_mix, "Color1")
    link(group, plaster_orm, "Red", ao_mix, "Color2")
    ao_value = add_node(
        group,
        "ShaderNodeRGBToBW",
        "Combined_AO_Value",
        (1200, 210),
    )
    link(group, ao_mix, "Color", ao_value, "Color")
    mortar_roughness = add_node(
        group,
        "ShaderNodeValue",
        "Brushed_Lime_Mortar_Roughness",
        (1000, -80),
    )
    mortar_roughness.outputs["Value"].default_value = 0.88

    state_proof = add_mix(group, "State_Proof_Stone_To_Mortar", (750, -190))
    state_proof.inputs["Color1"].default_value = hex_colour("#284457")
    state_proof.inputs["Color2"].default_value = hex_colour("#7b6848")
    link(group, weights["mortar"], "Result", state_proof, "Factor")
    state_scratch = add_mix(group, "State_Proof_Mortar_To_Scratch", (980, -190))
    state_scratch.inputs["Color2"].default_value = hex_colour("#a65b32")
    link(group, weights["scratch"], "Result", state_scratch, "Factor")
    link(group, state_proof, "Color", state_scratch, "Color1")
    state_brown = add_mix(group, "State_Proof_Scratch_To_Brown", (1210, -190))
    state_brown.inputs["Color2"].default_value = hex_colour("#c69543")
    link(group, weights["brown"], "Result", state_brown, "Factor")
    link(group, state_scratch, "Color", state_brown, "Color1")
    state_finish = add_mix(group, "State_Proof_Brown_To_Finish", (1440, -190))
    state_finish.inputs["Color2"].default_value = hex_colour("#e5d7a7")
    link(group, weights["finish"], "Result", state_finish, "Factor")
    link(group, state_brown, "Color", state_finish, "Color1")

    edge_proof = add_mix(group, "Transition_Edge_Proof", (1440, -380))
    edge_proof.inputs["Color1"].default_value = hex_colour("#101417")
    edge_proof.inputs["Color2"].default_value = hex_colour("#f0b63e")
    link(
        group,
        attributes["transition_edge_attribute"],
        "Fac",
        edge_proof,
        "Factor",
    )
    traversal_proof = add_mix(group, "Traversal_Readability_Proof", (1440, -540))
    traversal_proof.inputs["Color1"].default_value = hex_colour("#101417")
    traversal_proof.inputs["Color2"].default_value = hex_colour("#35c8c3")
    link(
        group,
        attributes["traversal_id_attribute"],
        "Fac",
        traversal_proof,
        "Factor",
    )

    link(group, combined_color, "Color", output, "Combined Color")
    link(group, rough_value, "Val", output, "Combined Roughness")
    link(group, height_system, "Height M", output, "Height M")
    link(group, height_value, "Val", output, "Height Proof")
    link(group, normal_system, "Normal Color", output, "Normal Color")
    link(group, ao_value, "Val", output, "Combined AO")
    link(
        group,
        textures["Masonry_Base_Colour"],
        "Color",
        output,
        "Stone Color",
    )
    link(group, mortar_color, "Color", output, "Mortar Color")
    link(group, combined_color, "Color", output, "Plaster Color")
    link(group, masonry_orm, "Green", output, "Stone Roughness")
    link(
        group,
        mortar_roughness,
        "Value",
        output,
        "Mortar Roughness",
    )
    link(group, rough_value, "Val", output, "Plaster Roughness")
    link(group, stone_weight, "Value", output, "Stone Weight")
    link(group, mortar_weight, "Value", output, "Mortar Weight")
    link(
        group,
        weights["scratch"],
        "Result",
        output,
        "Plaster Weight",
    )
    link(
        group,
        normal_system,
        "Detail Distance Fade",
        output,
        "Detail Distance Fade",
    )
    link(group, state_finish, "Color", output, "Layer State Proof")
    link(group, edge_proof, "Color", output, "Transition Edge Proof")
    link(group, traversal_proof, "Color", output, "Traversal Proof")
    return group


def create_material_group(
    surface_group: bpy.types.NodeTree,
) -> bpy.types.NodeTree:
    group = new_shader_group(
        GROUP_NAME,
        "Three physical dielectric responses selected by geometry-owned construction state.",
        color_tag="SHADER",
    )
    for name, socket_type in (
        ("Shader", "NodeSocketShader"),
        ("Combined Color", "NodeSocketColor"),
        ("Combined Roughness", "NodeSocketFloat"),
        ("Height M", "NodeSocketFloat"),
        ("Height Proof", "NodeSocketFloat"),
        ("Normal Color", "NodeSocketColor"),
        ("Combined AO", "NodeSocketFloat"),
        ("Stone Weight", "NodeSocketFloat"),
        ("Mortar Weight", "NodeSocketFloat"),
        ("Plaster Weight", "NodeSocketFloat"),
        ("Detail Distance Fade", "NodeSocketFloat"),
        ("Layer State Proof", "NodeSocketColor"),
        ("Transition Edge Proof", "NodeSocketColor"),
        ("Traversal Proof", "NodeSocketColor"),
    ):
        new_socket(group, name, "OUTPUT", socket_type)
    output = add_node(group, "NodeGroupOutput", "Group_Output", (1250, 280))
    surface = add_node(
        group,
        "ShaderNodeGroup",
        "Authored_Surface_Data",
        (-900, 300),
    )
    surface.node_tree = surface_group
    normal_map = add_node(
        group,
        "ShaderNodeNormalMap",
        "Combined_Tangent_Normal",
        (-430, -120),
    )
    normal_map.space = "TANGENT"
    normal_map.inputs["Strength"].default_value = 0.85
    link(group, surface, "Normal Color", normal_map, "Color")

    shaders: dict[str, bpy.types.Node] = {}
    for index, (name, ior) in enumerate(
        (
            ("Stone", 1.52),
            ("Mortar", 1.48),
            ("Plaster", 1.46),
        )
    ):
        node = add_node(
            group,
            "ShaderNodeBsdfPrincipled",
            f"{name}_Dielectric_Response",
            (-120, 560 - index * 300),
        )
        node.inputs["Metallic"].default_value = 0.0
        node.inputs["IOR"].default_value = ior
        link(group, surface, f"{name} Color", node, "Base Color")
        link(group, surface, f"{name} Roughness", node, "Roughness")
        link(group, normal_map, "Normal", node, "Normal")
        shaders[name] = node
    stone_to_mortar = add_node(
        group,
        "ShaderNodeMixShader",
        "Stone_And_Mortar_Physical_Layers",
        (260, 390),
    )
    link(group, surface, "Mortar Weight", stone_to_mortar, "Fac")
    link(group, shaders["Stone"], "BSDF", stone_to_mortar, "Shader")
    group.links.new(
        shaders["Mortar"].outputs["BSDF"],
        stone_to_mortar.inputs[2],
    )
    masonry_to_plaster = add_node(
        group,
        "ShaderNodeMixShader",
        "Masonry_To_Plaster_Construction_Layer",
        (560, 310),
    )
    link(group, surface, "Plaster Weight", masonry_to_plaster, "Fac")
    link(group, stone_to_mortar, "Shader", masonry_to_plaster, "Shader")
    group.links.new(
        shaders["Plaster"].outputs["BSDF"],
        masonry_to_plaster.inputs[2],
    )
    link(group, masonry_to_plaster, "Shader", output, "Shader")
    for name in (
        "Combined Color",
        "Combined Roughness",
        "Height M",
        "Height Proof",
        "Normal Color",
        "Combined AO",
        "Stone Weight",
        "Mortar Weight",
        "Plaster Weight",
        "Detail Distance Fade",
        "Layer State Proof",
        "Transition Edge Proof",
        "Traversal Proof",
    ):
        link(group, surface, name, output, name)
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
        (780, 260),
    )
    material_group = add_node(
        tree,
        "ShaderNodeGroup",
        "Lime_Plaster_Masonry_System",
        (-420, 320),
    )
    material_group.node_tree = group
    proof_emission = add_node(
        tree,
        "ShaderNodeEmission",
        "Material_Data_Proof",
        (160, 20),
    )
    proof_emission.inputs["Strength"].default_value = 1.0
    proof_mode = add_node(
        tree,
        "ShaderNodeValue",
        "Proof_Mode",
        (-80, 220),
    )
    proof_mode.outputs["Value"].default_value = 0.0
    proof_mix = add_node(
        tree,
        "ShaderNodeMixShader",
        "Physical_Surface_Or_Data_Proof",
        (500, 280),
    )
    link(tree, proof_mode, "Value", proof_mix, "Fac")
    link(tree, material_group, "Shader", proof_mix, "Shader")
    tree.links.new(
        proof_emission.outputs["Emission"],
        proof_mix.inputs[2],
    )
    displacement = add_node(
        tree,
        "ShaderNodeDisplacement",
        "Physical_Metre_Displacement",
        (500, -60),
    )
    displacement.space = "OBJECT"
    displacement.inputs["Midlevel"].default_value = 0.0
    displacement.inputs["Scale"].default_value = 1.0
    link(tree, material_group, "Height M", displacement, "Height")
    link(tree, proof_mix, "Shader", output, "Surface")
    link(tree, displacement, "Displacement", output, "Displacement")
    link(
        tree,
        material_group,
        "Combined Color",
        proof_emission,
        "Color",
    )
    material["iggy_material_schema"] = "iggy3d.material.lime_plaster_masonry_v1.v2"
    material["iggy_tile_size_m"] = 4.0
    material["iggy_default_damp"] = 0.0
    material["iggy_default_soot"] = 0.0
    material["iggy_ai_reference_used"] = False
    return material


def ensure_collection() -> bpy.types.Collection:
    previous = bpy.data.collections.get(COLLECTION_NAME)
    if previous is not None:
        for obj in list(previous.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.collections.remove(previous)
    collection = bpy.data.collections.new(COLLECTION_NAME)
    bpy.context.scene.collection.children.link(collection)
    return collection


def write_float_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: list[float],
) -> None:
    attribute = mesh.attributes.get(name)
    if attribute is not None:
        mesh.attributes.remove(attribute)
    attribute = mesh.attributes.new(name, "FLOAT", "POINT")
    attribute.data.foreach_set("value", values)


def write_face_float_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: list[float],
) -> None:
    attribute = mesh.attributes.get(name)
    if attribute is not None:
        mesh.attributes.remove(attribute)
    attribute = mesh.attributes.new(name, "FLOAT", "FACE")
    attribute.data.foreach_set("value", values)


def reveal_signed_distance(x: float, z: float) -> float:
    angle = -0.16
    dx = x - 2.38
    dz = z - 2.72
    rotated_x = dx * math.cos(angle) - dz * math.sin(angle)
    rotated_z = dx * math.sin(angle) + dz * math.cos(angle)
    radius = math.sqrt((rotated_x / 1.62) ** 2 + (rotated_z / 2.18) ** 2)
    irregularity = (
        math.sin(x * 2.35 + z * 1.18) * 0.060
        + math.sin(x * 5.3 - z * 3.1) * 0.029
        + math.sin(x * 9.1 + z * 6.4) * 0.013
    )
    primary = radius - 1.0 + irregularity
    small = math.sqrt(((x + 2.72) / 0.78) ** 2 + ((z - 4.84) / 0.91) ** 2) - 1.0
    small += math.sin(x * 5.8 + z * 4.2) * 0.042
    return min(primary, small)


def material_state(distance: float) -> float:
    return max(
        0.07,
        min(0.94, 0.07 + ((distance + 0.20) / 0.30) * 0.87),
    )


def categorical_material_state(distance: float) -> float:
    if distance < -0.175:
        return 0.07
    if distance < -0.090:
        return 0.25
    if distance < -0.005:
        return 0.46
    if distance < 0.075:
        return 0.68
    return 0.94


def layer_depth_from_state(state: float) -> float:
    controls = (
        (0.07, 0.000),
        (0.25, 0.000),
        (0.46, 0.009525),
        (0.68, 0.019050),
        (0.94, 0.022225),
    )
    if state <= controls[0][0]:
        return controls[0][1]
    for (state_a, depth_a), (state_b, depth_b) in zip(
        controls, controls[1:]
    ):
        if state <= state_b:
            fraction = (state - state_a) / (state_b - state_a)
            return depth_a * (1.0 - fraction) + depth_b * fraction
    return controls[-1][1]


FOOTHOLDS = (
    (2.30, 0.82, 0.92, 0.46, 0.14, 0.04),
    (2.78, 1.76, 0.70, 0.54, 0.17, -0.10),
    (1.75, 2.70, 0.98, 0.50, 0.13, 0.08),
    (2.69, 3.54, 0.78, 0.48, 0.16, -0.06),
    (1.98, 4.28, 0.68, 0.43, 0.12, 0.11),
)


def nearest_foothold(x: float, z: float) -> float:
    for center_x, center_z, width, height, _depth, _rotation in FOOTHOLDS:
        if ((x - center_x) / (width * 0.62)) ** 2 + (
            (z - center_z) / (height * 0.66)
        ) ** 2 < 1.0:
            return 1.0
    return 0.0


def create_wall_surface(
    collection: bpy.types.Collection,
    material: bpy.types.Material,
) -> bpy.types.Object:
    width = 8.8
    height = 6.4
    columns = 704
    rows = 512
    vertices: list[tuple[float, float, float]] = []
    coverage_values: list[float] = []
    edge_values: list[float] = []
    traversal_values: list[float] = []
    phase_values: list[float] = []
    variant_values: list[float] = []
    zero_values: list[float] = []
    for row in range(rows + 1):
        z = height * row / rows
        for column in range(columns + 1):
            x = -width * 0.5 + width * column / columns
            distance = reveal_signed_distance(x, z)
            state = material_state(distance)
            layer_depth = layer_depth_from_state(state)
            y = 0.122 + layer_depth
            vertices.append((x, y, z))
            coverage_values.append(
                max(0.0, min(1.0, (distance + 0.12) / 0.22))
            )
            edge_values.append(max(0.0, 1.0 - abs(distance) / 0.17))
            traversal_values.append(0.0)
            phase_values.append(0.037)
            variant_values.append(0.18)
            zero_values.append(0.0)

    def index(column: int, row: int) -> int:
        return row * (columns + 1) + column

    faces: list[tuple[int, int, int, int]] = []
    face_state_values: list[float] = []
    for row in range(rows):
        z_center = height * (row + 0.5) / rows
        for column in range(columns):
            x_center = -width * 0.5 + width * (column + 0.5) / columns
            inside_door = (
                -1.11 < x_center < 1.11 and 0.0 <= z_center < 4.82
            )
            if inside_door:
                continue
            faces.append(
                (
                    index(column, row),
                    index(column + 1, row),
                    index(column + 1, row + 1),
                    index(column, row + 1),
                )
            )
            face_state_values.append(
                categorical_material_state(
                    reveal_signed_distance(x_center, z_center)
                )
            )
    mesh = bpy.data.meshes.new(f"{WALL_SURFACE_NAME}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.materials.append(material)
    write_face_float_attribute(
        mesh,
        "iggy_plaster_layer_state",
        face_state_values,
    )
    write_float_attribute(mesh, "iggy_plaster_coverage", coverage_values)
    write_float_attribute(mesh, "iggy_transition_edge", edge_values)
    write_float_attribute(mesh, "iggy_traversal_id", traversal_values)
    write_float_attribute(mesh, "iggy_material_phase", phase_values)
    write_float_attribute(mesh, "iggy_material_variant", variant_values)
    write_float_attribute(mesh, "iggy_damp_mask", zero_values)
    write_float_attribute(mesh, "iggy_soot_mask", zero_values)
    mesh.update()
    obj = bpy.data.objects.new(WALL_SURFACE_NAME, mesh)
    collection.objects.link(obj)
    obj["iggy_surface_role"] = "geometry_owned_plaster_transition"
    obj["iggy_tile_size_m"] = 4.0
    return obj


def create_box_mesh(
    collection: bpy.types.Collection,
    name: str,
    bounds: tuple[float, float, float, float, float, float],
    material: bpy.types.Material,
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
    for attribute_name in ATTRIBUTE_NAMES:
        value = 0.07 if attribute_name == "iggy_plaster_layer_state" else 0.0
        if attribute_name == "iggy_plaster_layer_state":
            write_face_float_attribute(
                mesh,
                attribute_name,
                [value] * len(faces),
            )
        else:
            write_float_attribute(
                mesh,
                attribute_name,
                [value] * len(vertices),
            )
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    bevel = obj.modifiers.new("Restrained_Masonry_Arris", "BEVEL")
    bevel.width = 0.018
    bevel.segments = 2
    return obj


def create_foothold(
    collection: bpy.types.Collection,
    material: bpy.types.Material,
    index: int,
    specification: tuple[float, float, float, float, float, float],
) -> bpy.types.Object:
    center_x, center_z, width, height, depth, rotation = specification
    count = 10
    back_y = 0.137
    front_y = back_y + depth
    outline: list[tuple[float, float]] = []
    for point in range(count):
        angle = math.tau * point / count
        radius = (
            1.0
            + 0.10 * math.sin(angle * 3.0 + index * 0.73)
            + 0.055 * math.cos(angle * 5.0 - index * 0.41)
        )
        local_x = math.cos(angle) * width * 0.5 * radius
        local_z = math.sin(angle) * height * 0.5 * radius
        x = (
            center_x
            + local_x * math.cos(rotation)
            - local_z * math.sin(rotation)
        )
        z = (
            center_z
            + local_x * math.sin(rotation)
            + local_z * math.cos(rotation)
        )
        outline.append((x, z))
    vertices = [(x, back_y, z) for x, z in outline]
    vertices.extend((x, front_y, z) for x, z in outline)
    faces: list[tuple[int, ...]] = [
        tuple(range(count - 1, -1, -1)),
        tuple(range(count, count * 2)),
    ]
    for point in range(count):
        next_point = (point + 1) % count
        faces.append(
            (point, next_point, next_point + count, point + count)
        )
    mesh = bpy.data.meshes.new(f"IGGY_TraversalStone_{index:02d}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.materials.append(material)
    for attribute_name in ATTRIBUTE_NAMES:
        if attribute_name == "iggy_plaster_layer_state":
            value = 0.07
        elif attribute_name == "iggy_traversal_id":
            value = 1.0
        elif attribute_name == "iggy_material_phase":
            value = index * 0.137
        elif attribute_name == "iggy_material_variant":
            value = index / len(FOOTHOLDS)
        else:
            value = 0.0
        if attribute_name == "iggy_plaster_layer_state":
            write_face_float_attribute(
                mesh,
                attribute_name,
                [value] * len(faces),
            )
        else:
            write_float_attribute(
                mesh,
                attribute_name,
                [value] * len(vertices),
            )
    mesh.update()
    obj = bpy.data.objects.new(f"IGGY_TraversalStone_{index:02d}", mesh)
    collection.objects.link(obj)
    obj["iggy_surface_role"] = "projecting_traversal_stone"
    obj["iggy_traversal_id"] = index
    bevel = obj.modifiers.new("Hand_Dressed_Foothold_Arris", "BEVEL")
    bevel.width = 0.022
    bevel.segments = 2
    return obj


def create_flat_material(
    name: str,
    color: str,
    *,
    roughness: float,
) -> bpy.types.Material:
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = add_node(tree, "ShaderNodeOutputMaterial", "Output", (300, 0))
    principled = add_node(tree, "ShaderNodeBsdfPrincipled", "Surface", (0, 0))
    principled.inputs["Base Color"].default_value = hex_colour(color)
    principled.inputs["Roughness"].default_value = roughness
    link(tree, principled, "BSDF", output, "Surface")
    return material


def create_floor(
    collection: bpy.types.Collection,
) -> bpy.types.Object:
    material = create_flat_material(
        "IGGY_MAT_PlasterMasonryProofFloor", "#242629", roughness=0.84
    )
    return create_box_mesh(
        collection,
        "IGGY_PlasterMasonryProofFloor",
        (-7.0, 7.0, -4.0, 8.0, -0.12, -0.04),
        material,
    )


def point_object(
    obj: bpy.types.Object,
    target: tuple[float, float, float],
) -> None:
    obj.rotation_euler = (
        Vector(target) - obj.location
    ).to_track_quat("-Z", "Y").to_euler()


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


def configure_scene(
    collection: bpy.types.Collection,
) -> tuple[dict[str, bpy.types.Object], dict[str, bpy.types.Object]]:
    scene = bpy.context.scene
    for obj in bpy.data.objects:
        if obj.type == "LIGHT":
            obj.hide_render = True
    cameras = {
        "hero": create_camera(
            collection,
            "IGGY_CAM_PlasterMasonryHero",
            (7.6, 11.2, 6.7),
            (0.0, 0.08, 3.08),
            lens=58.0,
        ),
        "close": create_camera(
            collection,
            "IGGY_CAM_PlasterMasonryTransition",
            (5.1, 7.6, 4.2),
            (2.25, 0.10, 2.70),
            lens=72.0,
        ),
        "front": create_camera(
            collection,
            "IGGY_CAM_PlasterMasonryFront",
            (0.0, 11.5, 3.20),
            (0.0, 0.10, 3.20),
            lens=58.0,
        ),
    }
    lights = {
        "key": add_area_light(
            collection,
            "IGGY_PlasterMasonry_Key",
            (-4.2, 5.8, 8.6),
            (0.4, 0.0, 3.0),
            energy=820.0,
            size=5.0,
            color=(1.0, 0.91, 0.78),
        ),
        "fill": add_area_light(
            collection,
            "IGGY_PlasterMasonry_Fill",
            (5.8, 4.2, 5.0),
            (1.5, 0.0, 2.8),
            energy=440.0,
            size=4.4,
            color=(0.68, 0.78, 1.0),
        ),
        "rim": add_area_light(
            collection,
            "IGGY_PlasterMasonry_Rim",
            (-3.2, -2.2, 6.4),
            (0.0, 0.0, 3.1),
            energy=520.0,
            size=3.4,
            color=(0.82, 0.90, 1.0),
        ),
    }
    for light in lights.values():
        light.hide_render = False
    world = scene.world or bpy.data.worlds.new("IGGY_PlasterMasonryWorld")
    scene.world = world
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = hex_colour("#22262b")
    background.inputs["Strength"].default_value = 0.16
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.look = "AgX - Medium High Contrast"
    return cameras, lights


def set_material_mode(material: bpy.types.Material, output_name: str) -> None:
    tree = material.node_tree
    group = tree.nodes["Lime_Plaster_Masonry_System"]
    proof_mode = tree.nodes["Proof_Mode"]
    proof_emission = tree.nodes["Material_Data_Proof"]
    color_input = proof_emission.inputs["Color"]
    for existing in list(color_input.links):
        tree.links.remove(existing)
    if output_name == "Combined Color":
        proof_mode.outputs["Value"].default_value = 0.0
        tree.links.new(group.outputs["Combined Color"], color_input)
    else:
        proof_mode.outputs["Value"].default_value = 1.0
        tree.links.new(group.outputs[output_name], color_input)


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
    hero_path = output_root / "lime_plaster_masonry_v1_door_hero.png"
    renders["door_hero"] = render_still(
        scene, cameras["hero"], hero_path, args.render_width, args.render_height
    )
    close_path = output_root / "lime_plaster_masonry_v1_transition_close.png"
    renders["transition_close"] = render_still(
        scene, cameras["close"], close_path, args.render_width, args.render_height
    )
    gameplay_path = (
        output_root / "lime_plaster_masonry_v1_gameplay_distance.png"
    )
    renders["gameplay_distance"] = render_still(
        scene,
        cameras["front"],
        gameplay_path,
        args.render_width,
        args.render_height,
    )
    key_location = lights["key"].location.copy()
    key_energy = lights["key"].data.energy
    key_size = lights["key"].data.size
    fill_energy = lights["fill"].data.energy
    lights["key"].location = (5.8, 1.15, 4.4)
    point_object(lights["key"], (1.7, 0.10, 2.65))
    lights["key"].data.energy = 1180.0
    lights["key"].data.size = 1.6
    lights["fill"].data.energy = 120.0
    grazing_path = output_root / "lime_plaster_masonry_v1_grazing.png"
    renders["grazing"] = render_still(
        scene,
        cameras["close"],
        grazing_path,
        args.render_width,
        args.render_height,
    )
    lights["key"].location = key_location
    point_object(lights["key"], (0.4, 0.0, 3.0))
    lights["key"].data.energy = key_energy
    lights["key"].data.size = key_size
    lights["fill"].data.energy = fill_energy

    set_material_mode(material, "Layer State Proof")
    layer_path = output_root / "lime_plaster_masonry_v1_layer_states.png"
    renders["layer_states"] = render_still(
        scene, cameras["front"], layer_path, args.render_width, args.render_height
    )

    set_material_mode(material, "Traversal Proof")
    traversal_path = output_root / "lime_plaster_masonry_v1_traversal.png"
    renders["traversal"] = render_still(
        scene, cameras["close"], traversal_path, args.render_width, args.render_height
    )
    set_material_mode(material, "Normal Color")
    normal_path = output_root / "lime_plaster_masonry_v1_normal.png"
    renders["normal"] = render_still(
        scene,
        cameras["close"],
        normal_path,
        args.render_width,
        args.render_height,
    )
    set_material_mode(material, "Height Proof")
    height_path = output_root / "lime_plaster_masonry_v1_height.png"
    renders["height"] = render_still(
        scene,
        cameras["close"],
        height_path,
        args.render_width,
        args.render_height,
    )

    set_material_mode(material, "Combined Color")
    lights["key"].data.color = (1.0, 0.73, 0.48)
    lights["key"].data.energy = 980.0
    lights["fill"].data.color = (0.40, 0.56, 1.0)
    lights["fill"].data.energy = 520.0
    game_path = output_root / "lime_plaster_masonry_v1_game_light.png"
    renders["game_light"] = render_still(
        scene, cameras["hero"], game_path, args.render_width, args.render_height
    )
    lights["key"].data.color = (1.0, 0.91, 0.78)
    lights["key"].data.energy = 820.0
    lights["fill"].data.color = (0.68, 0.78, 1.0)
    lights["fill"].data.energy = 440.0
    return renders


def dependent_shader_groups(
    root: bpy.types.NodeTree,
) -> list[bpy.types.NodeTree]:
    result: list[bpy.types.NodeTree] = []
    pending = [root]
    seen: set[str] = set()
    while pending:
        tree = pending.pop()
        if tree.name in seen:
            continue
        seen.add(tree.name)
        result.append(tree)
        pending.extend(
            node.node_tree
            for node in tree.nodes
            if node.bl_idname == "ShaderNodeGroup"
            and node.node_tree is not None
        )
    return result


def validate_contract(
    profile: dict[str, Any],
    wall: bpy.types.Object,
    footholds: list[bpy.types.Object],
    material: bpy.types.Material,
    before: dict[str, Any],
) -> dict[str, Any]:
    after = prior_contracts()
    if before != after:
        raise AssertionError("Approved oak, joinery, or forged-iron contracts changed")
    missing_attributes = [
        name for name in ATTRIBUTE_NAMES if wall.data.attributes.get(name) is None
    ]
    if missing_attributes:
        raise AssertionError(f"Wall is missing attributes: {missing_attributes}")
    state_attribute = wall.data.attributes["iggy_plaster_layer_state"]
    if state_attribute.domain != "FACE":
        raise AssertionError("Categorical plaster state must use the face domain")
    state_values = [float(entry.value) for entry in state_attribute.data]
    state_range = [min(state_values), max(state_values)]
    if state_range[0] > 0.071 or state_range[1] < 0.939:
        raise AssertionError(f"Unexpected material-state range: {state_range}")
    occupied_state_bands = []
    for entry in profile["layer_states"]:
        lower, upper = entry["range"]
        occupied = sum(lower <= value <= upper for value in state_values)
        if occupied == 0:
            raise AssertionError(
                f"Material-state band is unoccupied: {entry['name']}"
            )
        occupied_state_bands.append(
            {"name": entry["name"], "face_count": occupied}
        )
    if len(footholds) != 5:
        raise AssertionError("Acceptance fixture needs five projecting footholds")
    if any(
        obj.data.attributes.get("iggy_traversal_id") is None
        for obj in footholds
    ):
        raise AssertionError("Every foothold needs traversal identity")
    group = bpy.data.node_groups[GROUP_NAME]
    shader_groups = dependent_shader_groups(group)
    expected_groups = {
        GROUP_NAME,
        SURFACE_GROUP_NAME,
        NORMAL_GROUP_NAME,
        HEIGHT_GROUP_NAME,
    }
    actual_groups = {tree.name for tree in shader_groups}
    if actual_groups != expected_groups:
        raise AssertionError(
            f"Layered shader group contract drifted: {sorted(actual_groups)}"
        )
    attribute_nodes = {
        node.attribute_name
        for tree in shader_groups
        for node in tree.nodes
        if node.bl_idname == "ShaderNodeAttribute"
    }
    required = {
        profile["coordinate_contract"][key]
        for key in (
            "phase_attribute",
            "plaster_coverage_attribute",
            "layer_state_attribute",
            "transition_edge_attribute",
            "traversal_id_attribute",
            "damp_attribute",
            "soot_attribute",
        )
    }
    if not required.issubset(attribute_nodes):
        raise AssertionError("Shader is missing geometry-aware attribute lanes")
    surface_tree = bpy.data.node_groups[SURFACE_GROUP_NAME]
    coordinate_node = surface_tree.nodes.get("Object_Metre_Position")
    if (
        coordinate_node is None
        or coordinate_node.bl_idname != "ShaderNodeTexCoord"
        or not any(
            link.from_node == coordinate_node
            and link.from_socket.name == "Object"
            and link.to_node.name == "Wall_XZ_Metres"
            for link in surface_tree.links
        )
    ):
        raise AssertionError("Material coordinates must come from object-space metres")
    shader_nodes = [
        node for tree in shader_groups for node in tree.nodes
    ]
    image_nodes = [
        node
        for node in shader_nodes
        if node.bl_idname == "ShaderNodeTexImage"
    ]
    principled_nodes = [
        node
        for node in shader_nodes
        if node.bl_idname == "ShaderNodeBsdfPrincipled"
    ]
    normal_nodes = [
        node
        for node in shader_nodes
        if node.bl_idname == "ShaderNodeNormalMap"
    ]
    camera_nodes = [
        node
        for node in shader_nodes
        if node.bl_idname == "ShaderNodeCameraData"
    ]
    forbidden_noise = [
        node
        for node in shader_nodes
        if node.bl_idname in {"ShaderNodeTexNoise", "ShaderNodeTexVoronoi"}
    ]
    bump_nodes = [
        node
        for node in material.node_tree.nodes
        if node.bl_idname == "ShaderNodeBump"
    ]
    displacement_nodes = [
        node
        for node in material.node_tree.nodes
        if node.bl_idname == "ShaderNodeDisplacement"
    ]
    if (
        len(image_nodes) != 12
        or len(principled_nodes) != 3
        or len(normal_nodes) != 1
        or len(camera_nodes) != 1
        or len(displacement_nodes) != 1
        or bump_nodes
        or forbidden_noise
    ):
        raise AssertionError("Layered shader node counts drifted")
    if material["iggy_default_damp"] != 0.0 or material["iggy_default_soot"] != 0.0:
        raise AssertionError("Narrative overlays must remain off")
    packed_images = [
        image.name
        for image in bpy.data.images
        if image.packed_file is not None
        and (
            image.name.startswith("lime_plaster")
            or image.name.startswith("giant_masonry")
        )
    ]
    if len(packed_images) < 12:
        raise AssertionError("Material textures were not packed")
    return {
        "prior_contracts_unchanged": True,
        "wall_vertex_count": len(wall.data.vertices),
        "wall_polygon_count": len(wall.data.polygons),
        "material_state_range": state_range,
        "material_state_domain": state_attribute.domain,
        "occupied_state_bands": occupied_state_bands,
        "attribute_names": list(ATTRIBUTE_NAMES),
        "projecting_footholds": len(footholds),
        "packed_images": sorted(packed_images),
        "node_groups": sorted(actual_groups),
        "image_texture_count": len(image_nodes),
        "principled_count": len(principled_nodes),
        "normal_map_count": len(normal_nodes),
        "camera_distance_count": len(camera_nodes),
        "displacement_count": len(displacement_nodes),
        "bump_count": len(bump_nodes),
        "generic_noise_count": len(forbidden_noise),
        "coordinate_space": "object_position_xz_metres",
        "geometry_transition": True,
        "narrative_overlays_default_off": True,
    }


def main() -> None:
    args = parse_args()
    profile = load_profile()
    args.output_root.mkdir(parents=True, exist_ok=True)
    generator = load_generator()
    material_data = generator.generate_material(
        resolution=args.texture_resolution,
        seed=73129,
        pattern_variation=0,
    )
    texture_manifest = generator.write_material_package(
        material_data,
        output_root=args.output_root,
        pattern_path=generator.DEFAULT_PATTERN,
        profile_path=PROFILE_PATH,
    )

    bpy.ops.wm.open_mainfile(filepath=str(args.source))
    before = prior_contracts()
    collection = ensure_collection()
    for name in ("IGGY_ForgedIronProof", "IGGY_StructuralOakJoineryProof"):
        existing = bpy.data.collections.get(name)
        if existing is not None:
            existing.hide_render = True
    for name in ("IGGY_ProofFloor", "IGGY_IronProof_Floor", "IGGY_JoineryProofFloor"):
        existing = bpy.data.objects.get(name)
        if existing is not None:
            existing.hide_render = True

    surface_group = create_surface_data_group(profile, args.output_root)
    group = create_material_group(surface_group)
    material = configure_material(group)
    create_box_mesh(
        collection,
        "IGGY_GiantHouseWall_LeftCore",
        (-4.4, -1.11, -0.42, 0.115, 0.0, 6.4),
        material,
    )
    create_box_mesh(
        collection,
        "IGGY_GiantHouseWall_RightCore",
        (1.11, 4.4, -0.42, 0.115, 0.0, 6.4),
        material,
    )
    create_box_mesh(
        collection,
        "IGGY_GiantHouseWall_LintelCore",
        (-1.11, 1.11, -0.42, 0.115, 4.82, 6.4),
        material,
    )
    wall = create_wall_surface(collection, material)
    footholds = [
        create_foothold(collection, material, index + 1, specification)
        for index, specification in enumerate(FOOTHOLDS)
    ]
    create_floor(collection)
    door = bpy.data.objects.get("SINC_DemoDoorAssembly")
    if door is None:
        raise AssertionError("Approved giant-house door is missing")
    door.hide_render = False
    cameras, lights = configure_scene(collection)
    scene = bpy.context.scene
    renders = render_proofs(
        scene,
        material,
        cameras,
        lights,
        args.output_root,
        args,
    )
    validation = validate_contract(
        profile,
        wall,
        footholds,
        material,
        before,
    )
    set_material_mode(material, "Combined Color")
    blend_path = args.output_root / "lime_plaster_masonry_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    manifest = {
        "schema": "iggy3d.material.lime_plaster_masonry_v1.blender.v2",
        "source_blend": {
            "path": str(args.source),
            "sha256": sha256_file(args.source),
        },
        "output_blend": {
            "path": str(blend_path),
            "bytes": blend_path.stat().st_size,
            "sha256": sha256_file(blend_path),
        },
        "material": MATERIAL_NAME,
        "node_group": GROUP_NAME,
        "texture_manifest": texture_manifest,
        "renders": renders,
        "validation": validation,
        "constraints": {
            "uses_three_physical_dielectrics": True,
            "uses_form_and_detail_normals": True,
            "uses_distance_faded_detail": True,
            "uses_metre_displacement": True,
            "categorical_state_is_face_domain": True,
            "uses_object_space_metre_coordinates": True,
            "unreal_runtime_parity_verified": False,
        },
        "source_policy": {
            "ai_generated_reference_capture": False,
            "ai_generated_runtime_texture": False,
            "raw_reference_pixels_used_as_runtime_texture": False,
            "third_party_pixels_stored": False,
            "legacy_ai_experiments_are_build_inputs": False,
        },
    }
    manifest_path = args.output_root / "lime_plaster_masonry_v1_blender_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(
        "lime_plaster_masonry_v1 built:",
        validation["wall_polygon_count"],
        "wall polygons,",
        validation["projecting_footholds"],
        "projecting footholds,",
        len(renders),
        "renders",
    )


if __name__ == "__main__":
    main()
