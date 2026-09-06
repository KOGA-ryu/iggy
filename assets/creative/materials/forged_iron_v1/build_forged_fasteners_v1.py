#!/usr/bin/env python3
"""Build the measured, reusable forged-fastener Geometry Nodes family."""

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
from mathutils import Euler, Vector


SCRIPT_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "forged_fasteners_v1.json"
IRON_PROFILE_PATH = SCRIPT_ROOT / "profiles" / "forged_iron_v1.json"
DEFAULT_SOURCE = (
    SCRIPT_ROOT.parent
    / "structural_oak_door_v1"
    / "output"
    / "structural_oak_door_v1.blend"
)
IRON_LIBRARY = SCRIPT_ROOT / "output" / "forged_iron_v1.blend"
IRON_BUILDER_PATH = SCRIPT_ROOT / "build_forged_iron_v1.py"
DEFAULT_OUTPUT_ROOT = SCRIPT_ROOT / "output"

DOOR_OBJECT_NAME = "SINC_DemoDoorAssembly"
DOOR_GROUP_NAME = "SINC_GN_DoorAssembly_v001"
OLD_FASTENER_GROUP_NAME = "SINC_GN_ForgedFasteners_v003"
FASTENER_GROUP_NAME = "IGGY_GN_ForgedFasteners_v001"
IRON_MATERIAL_NAME = "IGGY_MAT_ReferenceForgedIron_v001"
WOOD_MATERIAL_NAME = "IGGY_MAT_ReferenceStructuralOakDoor_v001"
MASTER_COLLECTION_NAME = "IGGY_ForgedFastenerMasters"
PROOF_COLLECTION_NAME = "IGGY_ForgedFastenerProof"


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--door-resolution-x", type=int, default=900)
    parser.add_argument("--door-resolution-y", type=int, default=1080)
    parser.add_argument("--proof-resolution-x", type=int, default=1200)
    parser.add_argument("--proof-resolution-y", type=int, default=760)
    return parser.parse_args(argv)


def load_profile() -> dict[str, Any]:
    profile = json.loads(PROFILE_PATH.read_text())
    assert profile["profile_id"] == "forged_fasteners_v1"
    measured = profile["reference_authority"]["measured_door_nail"]
    assert measured["overall_m"] == {
        "width": 0.074,
        "depth": 0.075,
        "length": 0.156,
    }
    assert measured["installed_m"]["projection"] == 0.017
    assert len(profile["families"]) == 4
    assert not any(
        family["uses_noise_displacement"]
        for family in profile["families"].values()
    )
    return profile


def load_iron_builder():
    spec = importlib.util.spec_from_file_location(
        "build_forged_iron_v1",
        IRON_BUILDER_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load {IRON_BUILDER_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def add_node(
    tree: bpy.types.NodeTree,
    node_type: str,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = tree.nodes.new(node_type)
    node.name = name
    node.label = name.replace("_", " ")
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


def new_collection(name: str) -> bpy.types.Collection:
    old = bpy.data.collections.get(name)
    if old is not None:
        for obj in list(old.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.collections.remove(old)
    collection = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(collection)
    return collection


def set_point_float_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: list[float],
) -> None:
    attribute = mesh.attributes.get(name)
    if attribute is None:
        attribute = mesh.attributes.new(name, "FLOAT", "POINT")
    for index, value in enumerate(values):
        attribute.data[index].value = float(value)


def set_point_int_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: list[int],
) -> None:
    attribute = mesh.attributes.get(name)
    if attribute is None:
        attribute = mesh.attributes.new(name, "INT", "POINT")
    for index, value in enumerate(values):
        attribute.data[index].value = int(value)


def square_perimeter(segment: int) -> tuple[float, float]:
    points = (
        (1.0, 0.70),
        (0.70, 1.0),
        (-0.70, 1.0),
        (-1.0, 0.70),
        (-1.0, -0.70),
        (-0.70, -1.0),
        (0.70, -1.0),
        (1.0, -0.70),
    )
    return points[segment]


def create_head_master(
    collection: bpy.types.Collection,
    family_name: str,
    family: dict[str, Any],
) -> bpy.types.Object:
    display = {
        "rosehead": "Rosehead",
        "faceted_square_peen": "FacetedSquarePeen",
        "domed_rivet": "DomedRivet",
        "flattened_peen": "FlattenedPeen",
    }[family_name]
    object_name = f"IGGY_FastenerMaster_{display}"
    old = bpy.data.objects.get(object_name)
    if old is not None:
        bpy.data.objects.remove(old, do_unlink=True)

    segments = int(family["perimeter_segments"])
    square = segments == 8
    aspect = family.get(
        "ellipticity",
        [0.9866666666666667, 1.0] if family_name == "rosehead" else [1.0, 1.0],
    )
    ring_count = len(family["axial_rings"])
    vertices: list[tuple[float, float, float]] = []
    edge_values: list[float] = []
    for ring_index, ring in enumerate(family["axial_rings"]):
        radius = float(ring["radius"])
        offset_x, offset_y = (float(value) for value in ring["offset"])
        for segment in range(segments):
            if square:
                base_x, base_y = square_perimeter(segment)
                if segment in (1, 5):
                    base_x *= 0.975
                if segment in (3, 7):
                    base_y *= 0.985
            else:
                angle = 2.0 * math.pi * segment / segments
                base_x = math.cos(angle)
                base_y = math.sin(angle)
                if family_name == "flattened_peen":
                    authored_lobe = (
                        1.0
                        + 0.014 * math.cos(2.0 * angle + 0.3)
                        + 0.008 * math.sin(3.0 * angle - 0.2)
                    )
                    base_x *= authored_lobe
                    base_y *= authored_lobe
            vertices.append(
                (
                    (base_x * radius + offset_x) * float(aspect[0]),
                    (base_y * radius + offset_y) * float(aspect[1]),
                    float(ring["z"]),
                )
            )
            if ring_index in (0, ring_count - 1):
                edge_values.append(1.0)
            elif ring_index in (1, ring_count - 2):
                edge_values.append(0.58)
            else:
                edge_values.append(0.12)

    faces: list[tuple[int, ...]] = []
    faces.append(tuple(reversed(range(segments))))
    for ring_index in range(ring_count - 1):
        base = ring_index * segments
        next_base = (ring_index + 1) * segments
        for segment in range(segments):
            following = (segment + 1) % segments
            faces.append(
                (
                    base + segment,
                    base + following,
                    next_base + following,
                    next_base + segment,
                )
            )
    top_base = (ring_count - 1) * segments
    faces.append(tuple(top_base + index for index in range(segments)))

    mesh = bpy.data.meshes.new(f"{object_name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    smooth_family = family_name in {"domed_rivet", "flattened_peen"}
    for polygon in mesh.polygons:
        polygon.use_smooth = smooth_family and len(polygon.vertices) == 4
    set_point_float_attribute(mesh, "sinc_iron_edge_mask", edge_values)
    set_point_int_attribute(
        mesh,
        "sinc_fastener_style",
        [int(family["style_id"])] * len(vertices),
    )
    set_point_int_attribute(
        mesh,
        "sinc_fastener_surface_role",
        [0] * len(vertices),
    )

    obj = bpy.data.objects.new(object_name, mesh)
    collection.objects.link(obj)
    obj.hide_render = True
    obj.hide_set(True)
    obj["iggy_fastener_family"] = family_name
    obj["iggy_dimension_status"] = family["dimension_status"]
    return obj


def create_punched_rim_master(
    collection: bpy.types.Collection,
    profile: dict[str, Any],
) -> bpy.types.Object:
    name = "IGGY_PunchedSeatRimMaster"
    old = bpy.data.objects.get(name)
    if old is not None:
        bpy.data.objects.remove(old, do_unlink=True)
    contract = profile["seat_contract"]["iron_plate"]
    outer = float(contract["outer_radius_over_shank_radius"])
    height = float(contract["height_over_shank_radius"])
    segments = 24
    ring_specs = (
        (1.0, 0.0),
        (1.16, height * 0.72),
        (outer, height),
        (outer * 1.08, 0.0),
    )
    vertices: list[tuple[float, float, float]] = []
    for ring_index, (radius, z) in enumerate(ring_specs):
        for segment in range(segments):
            angle = 2.0 * math.pi * segment / segments
            four_lobe = 1.0 + 0.018 * math.cos(4.0 * angle + 0.4)
            if ring_index in (0, len(ring_specs) - 1):
                four_lobe = 1.0
            vertices.append(
                (
                    math.cos(angle) * radius * four_lobe,
                    math.sin(angle) * radius * four_lobe,
                    z,
                )
            )
    faces: list[tuple[int, ...]] = []
    for ring_index in range(len(ring_specs) - 1):
        base = ring_index * segments
        next_base = (ring_index + 1) * segments
        for segment in range(segments):
            following = (segment + 1) % segments
            faces.append(
                (
                    base + segment,
                    base + following,
                    next_base + following,
                    next_base + segment,
                )
            )
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    set_point_float_attribute(
        mesh,
        "sinc_iron_edge_mask",
        [
            1.0 if index // segments in (0, len(ring_specs) - 1) else 0.52
            for index in range(len(vertices))
        ],
    )
    set_point_int_attribute(
        mesh,
        "sinc_fastener_surface_role",
        [3] * len(vertices),
    )
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj.hide_render = True
    obj.hide_set(True)
    return obj


def create_tip_master(
    collection: bpy.types.Collection,
    *,
    square: bool,
) -> bpy.types.Object:
    name = (
        "IGGY_FastenerTip_SquareMaster"
        if square
        else "IGGY_FastenerTip_RoundMaster"
    )
    old = bpy.data.objects.get(name)
    if old is not None:
        bpy.data.objects.remove(old, do_unlink=True)
    segments = 4 if square else 12
    vertices: list[tuple[float, float, float]] = []
    for segment in range(segments):
        angle = 2.0 * math.pi * segment / segments
        vertices.append((math.cos(angle), math.sin(angle), 0.0))
    tip_index = len(vertices)
    vertices.append((0.0, 0.0, -1.0))
    faces: list[tuple[int, ...]] = [
        tuple(reversed(range(segments))),
    ]
    for segment in range(segments):
        faces.append(
            (
                segment,
                (segment + 1) % segments,
                tip_index,
            )
        )
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    set_point_float_attribute(
        mesh,
        "sinc_iron_edge_mask",
        [0.76] * segments + [1.0],
    )
    set_point_int_attribute(
        mesh,
        "sinc_fastener_surface_role",
        [2] * len(vertices),
    )
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj.hide_render = True
    obj.hide_set(True)
    return obj


def create_masters(
    profile: dict[str, Any],
) -> tuple[
    dict[str, bpy.types.Object],
    bpy.types.Object,
    dict[bool, bpy.types.Object],
]:
    collection = new_collection(MASTER_COLLECTION_NAME)
    masters = {
        name: create_head_master(collection, name, family)
        for name, family in profile["families"].items()
    }
    rim = create_punched_rim_master(collection, profile)
    tips = {
        True: create_tip_master(collection, square=True),
        False: create_tip_master(collection, square=False),
    }
    return masters, rim, tips


def new_interface_socket(
    group: bpy.types.NodeTree,
    name: str,
    socket_type: str,
    *,
    in_out: str = "INPUT",
    default: Any = None,
    minimum: float | int | None = None,
    maximum: float | int | None = None,
) -> Any:
    socket = group.interface.new_socket(
        name=name,
        in_out=in_out,
        socket_type=socket_type,
    )
    if default is not None and hasattr(socket, "default_value"):
        socket.default_value = default
    if minimum is not None and hasattr(socket, "min_value"):
        socket.min_value = minimum
    if maximum is not None and hasattr(socket, "max_value"):
        socket.max_value = maximum
    return socket


def store_attribute(
    tree: bpy.types.NodeTree,
    source: bpy.types.Node,
    source_socket: str,
    name: str,
    data_type: str,
    value_source: bpy.types.Node | None,
    value_socket: str | None,
    default: float | int,
    location: tuple[float, float],
    label: str,
) -> bpy.types.Node:
    node = add_node(
        tree,
        "GeometryNodeStoreNamedAttribute",
        label,
        location,
    )
    node.domain = "POINT"
    node.data_type = data_type
    node.inputs["Name"].default_value = name
    link(tree, source, source_socket, node, "Geometry")
    if value_source is not None and value_socket is not None:
        link(tree, value_source, value_socket, node, "Value")
    else:
        node.inputs["Value"].default_value = default
    return node


def create_piece_material_node(
    tree: bpy.types.NodeTree,
    source: bpy.types.Node,
    source_socket: str,
    group_input: bpy.types.Node,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    material = add_node(
        tree,
        "GeometryNodeSetMaterial",
        name,
        location,
    )
    link(tree, source, source_socket, material, "Geometry")
    link(tree, group_input, "Iron Material", material, "Material")
    return material


def create_shank_piece(
    tree: bpy.types.NodeTree,
    group_input: bpy.types.Node,
    *,
    square: bool,
    y: float,
    style_id: int,
) -> bpy.types.Node:
    cylinder = add_node(
        tree,
        "GeometryNodeMeshCylinder",
        f"Style_{style_id}_Tapered_Shank",
        (-880, y),
    )
    cylinder.inputs["Vertices"].default_value = 4 if square else 12
    cylinder.inputs["Side Segments"].default_value = 1
    cylinder.inputs["Fill Segments"].default_value = 1
    link(tree, group_input, "Shank Radius", cylinder, "Radius")
    link(tree, group_input, "Shank Length", cylinder, "Depth")

    half = add_node(
        tree,
        "ShaderNodeMath",
        f"Style_{style_id}_Half_Shank_Negative",
        (-880, y - 135),
    )
    half.operation = "MULTIPLY"
    half.inputs[1].default_value = -0.5
    link(tree, group_input, "Shank Length", half, "Value")
    translation = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        f"Style_{style_id}_Shank_Translation",
        (-660, y - 110),
    )
    link(tree, half, "Value", translation, "Z")
    transform = add_node(
        tree,
        "GeometryNodeTransform",
        f"Style_{style_id}_Seat_Shank",
        (-430, y),
    )
    link(tree, cylinder, "Mesh", transform, "Geometry")
    link(tree, translation, "Vector", transform, "Translation")
    if square:
        transform.inputs["Rotation"].default_value[2] = math.radians(45.0)
    role = store_attribute(
        tree,
        transform,
        "Geometry",
        "sinc_fastener_surface_role",
        "INT",
        None,
        None,
        1,
        (-210, y),
        f"Style_{style_id}_Store_Shank_Role",
    )
    edge = store_attribute(
        tree,
        role,
        "Geometry",
        "sinc_iron_edge_mask",
        "FLOAT",
        None,
        None,
        0.38 if square else 0.18,
        (0, y),
        f"Style_{style_id}_Store_Shank_Arris",
    )
    return create_piece_material_node(
        tree,
        edge,
        "Geometry",
        group_input,
        f"Style_{style_id}_Material_Shank",
        (210, y),
    )


def create_tip_piece(
    tree: bpy.types.NodeTree,
    group_input: bpy.types.Node,
    tip_master: bpy.types.Object,
    *,
    square: bool,
    y: float,
    style_id: int,
) -> bpy.types.Node:
    info = add_node(
        tree,
        "GeometryNodeObjectInfo",
        f"Style_{style_id}_Authored_Drawn_Tip",
        (-880, y),
    )
    info.transform_space = "ORIGINAL"
    info.inputs["Object"].default_value = tip_master
    info.inputs["As Instance"].default_value = False

    shank_translation = add_node(
        tree,
        "ShaderNodeVectorMath",
        f"Style_{style_id}_Negative_Shank_Vector",
        (-880, y - 130),
    )
    shank_translation.operation = "SCALE"
    shank_translation.inputs["Vector"].default_value = (0.0, 0.0, -1.0)
    link(tree, group_input, "Shank Length", shank_translation, "Scale")
    scale = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        f"Style_{style_id}_Tip_Metre_Scale",
        (-660, y - 135),
    )
    link(tree, group_input, "Shank Radius", scale, "X")
    link(tree, group_input, "Shank Radius", scale, "Y")
    link(tree, group_input, "Tip Length", scale, "Z")
    transform = add_node(
        tree,
        "GeometryNodeTransform",
        f"Style_{style_id}_Seat_Tip",
        (-430, y),
    )
    link(tree, info, "Geometry", transform, "Geometry")
    link(tree, scale, "Vector", transform, "Scale")
    link(tree, shank_translation, "Vector", transform, "Translation")
    if square:
        transform.inputs["Rotation"].default_value[2] = math.radians(45.0)
    role = store_attribute(
        tree,
        transform,
        "Geometry",
        "sinc_fastener_surface_role",
        "INT",
        None,
        None,
        2,
        (-210, y),
        f"Style_{style_id}_Store_Tip_Role",
    )
    edge = store_attribute(
        tree,
        role,
        "Geometry",
        "sinc_iron_edge_mask",
        "FLOAT",
        None,
        None,
        0.78,
        (0, y),
        f"Style_{style_id}_Store_Tip_Arris",
    )
    return create_piece_material_node(
        tree,
        edge,
        "Geometry",
        group_input,
        f"Style_{style_id}_Material_Tip",
        (210, y),
    )


def create_head_piece(
    tree: bpy.types.NodeTree,
    group_input: bpy.types.Node,
    master: bpy.types.Object,
    *,
    y: float,
    style_id: int,
) -> bpy.types.Node:
    info = add_node(
        tree,
        "GeometryNodeObjectInfo",
        f"Style_{style_id}_Authored_Head_Master",
        (-880, y),
    )
    info.transform_space = "ORIGINAL"
    info.inputs["Object"].default_value = master
    info.inputs["As Instance"].default_value = False
    scale = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        f"Style_{style_id}_Head_Metre_Scale",
        (-660, y - 120),
    )
    link(tree, group_input, "Head Radius", scale, "X")
    link(tree, group_input, "Head Radius", scale, "Y")
    link(tree, group_input, "Head Height", scale, "Z")
    transform = add_node(
        tree,
        "GeometryNodeTransform",
        f"Style_{style_id}_Scale_Authored_Head",
        (-430, y),
    )
    link(tree, info, "Geometry", transform, "Geometry")
    link(tree, scale, "Vector", transform, "Scale")
    role = store_attribute(
        tree,
        transform,
        "Geometry",
        "sinc_fastener_surface_role",
        "INT",
        None,
        None,
        0,
        (-210, y),
        f"Style_{style_id}_Store_Head_Role",
    )
    return create_piece_material_node(
        tree,
        role,
        "Geometry",
        group_input,
        f"Style_{style_id}_Material_Head",
        (0, y),
    )


def create_rim_piece(
    tree: bpy.types.NodeTree,
    group_input: bpy.types.Node,
    rim_master: bpy.types.Object,
    *,
    y: float,
    style_id: int,
) -> bpy.types.Node:
    info = add_node(
        tree,
        "GeometryNodeObjectInfo",
        f"Style_{style_id}_Punched_Rim_Master",
        (-880, y),
    )
    info.transform_space = "ORIGINAL"
    info.inputs["Object"].default_value = rim_master
    info.inputs["As Instance"].default_value = False
    scale = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        f"Style_{style_id}_Rim_Metre_Scale",
        (-660, y - 110),
    )
    link(tree, group_input, "Shank Radius", scale, "X")
    link(tree, group_input, "Shank Radius", scale, "Y")
    link(tree, group_input, "Shank Radius", scale, "Z")
    transform = add_node(
        tree,
        "GeometryNodeTransform",
        f"Style_{style_id}_Scale_Punched_Rim",
        (-430, y),
    )
    link(tree, info, "Geometry", transform, "Geometry")
    link(tree, scale, "Vector", transform, "Scale")
    material = create_piece_material_node(
        tree,
        transform,
        "Geometry",
        group_input,
        f"Style_{style_id}_Material_Rim",
        (-210, y),
    )
    switch = add_node(
        tree,
        "GeometryNodeSwitch",
        f"Style_{style_id}_Optional_Punched_Seat",
        (20, y),
    )
    switch.input_type = "GEOMETRY"
    link(tree, group_input, "Punched Seat", switch, "Switch")
    link(tree, material, "Geometry", switch, "True")
    return switch


def create_fastener_group(
    profile: dict[str, Any],
    masters: dict[str, bpy.types.Object],
    rim_master: bpy.types.Object,
    tip_masters: dict[bool, bpy.types.Object],
) -> bpy.types.NodeTree:
    previous = bpy.data.node_groups.get(FASTENER_GROUP_NAME)
    if previous is not None:
        bpy.data.node_groups.remove(previous, do_unlink=True)
    group = bpy.data.node_groups.new(FASTENER_GROUP_NAME, "GeometryNodeTree")
    group.is_modifier = True
    new_interface_socket(
        group,
        "Geometry",
        "NodeSocketGeometry",
        in_out="OUTPUT",
    )
    new_interface_socket(group, "Points", "NodeSocketGeometry")
    new_interface_socket(
        group,
        "Rotation",
        "NodeSocketRotation",
        default=(0.0, 0.0, 0.0),
    )
    new_interface_socket(
        group,
        "Shank Radius",
        "NodeSocketFloat",
        default=0.012,
        minimum=0.001,
        maximum=0.05,
    )
    new_interface_socket(
        group,
        "Shank Length",
        "NodeSocketFloat",
        default=0.12,
        minimum=0.01,
        maximum=0.5,
    )
    new_interface_socket(
        group,
        "Head Radius",
        "NodeSocketFloat",
        default=0.0375,
        minimum=0.005,
        maximum=0.15,
    )
    new_interface_socket(
        group,
        "Head Height",
        "NodeSocketFloat",
        default=0.017,
        minimum=0.002,
        maximum=0.08,
    )
    new_interface_socket(
        group,
        "Rose Head",
        "NodeSocketBool",
        default=True,
    )
    new_interface_socket(
        group,
        "Head Irregularity",
        "NodeSocketFloat",
        default=0.0,
        minimum=0.0,
        maximum=0.0,
    )
    new_interface_socket(
        group,
        "Tip Length",
        "NodeSocketFloat",
        default=0.024,
        minimum=0.002,
        maximum=0.08,
    )
    new_interface_socket(
        group,
        "Radial Segments",
        "NodeSocketInt",
        default=16,
        minimum=4,
        maximum=32,
    )
    new_interface_socket(
        group,
        "Seed",
        "NodeSocketInt",
        default=0,
        minimum=0,
        maximum=65535,
    )
    new_interface_socket(
        group,
        "Punched Seat",
        "NodeSocketBool",
        default=False,
    )
    new_interface_socket(
        group,
        "Realize",
        "NodeSocketBool",
        default=False,
    )
    new_interface_socket(group, "Iron Material", "NodeSocketMaterial")

    tree = group
    group_input = add_node(tree, "NodeGroupInput", "Inputs", (-1750, 200))
    group_output = add_node(tree, "NodeGroupOutput", "Outputs", (1880, 200))
    ordered = (
        ("rosehead", True),
        ("faceted_square_peen", True),
        ("domed_rivet", False),
        ("flattened_peen", False),
    )
    authored_instances: list[bpy.types.Node] = []
    for style_id, (family_name, square) in enumerate(ordered):
        row_y = 1040 - style_id * 680
        head = create_head_piece(
            tree,
            group_input,
            masters[family_name],
            y=row_y,
            style_id=style_id,
        )
        shank = create_shank_piece(
            tree,
            group_input,
            square=square,
            y=row_y - 150,
            style_id=style_id,
        )
        tip = create_tip_piece(
            tree,
            group_input,
            tip_masters[square],
            square=square,
            y=row_y - 300,
            style_id=style_id,
        )
        rim = create_rim_piece(
            tree,
            group_input,
            rim_master,
            y=row_y - 450,
            style_id=style_id,
        )
        join = add_node(
            tree,
            "GeometryNodeJoinGeometry",
            f"Style_{style_id}_Head_Shank_Tip_Seat",
            (310, row_y - 100),
        )
        for source in (head, shank, tip, rim):
            tree.links.new(
                source.outputs["Output"]
                if source.bl_idname == "GeometryNodeSwitch"
                else source.outputs["Geometry"],
                join.inputs["Geometry"],
            )
        style = store_attribute(
            tree,
            join,
            "Geometry",
            "sinc_fastener_style",
            "INT",
            None,
            None,
            style_id,
            (520, row_y - 100),
            f"Style_{style_id}_Store_Family_Id",
        )
        seed = store_attribute(
            tree,
            style,
            "Geometry",
            "sinc_seed",
            "INT",
            group_input,
            "Seed",
            0,
            (700, row_y - 100),
            f"Style_{style_id}_Store_Seed",
        )
        to_instance = add_node(
            tree,
            "GeometryNodeGeometryToInstance",
            f"Style_{style_id}_Reusable_Instance",
            (760, row_y - 100),
        )
        link(tree, seed, "Geometry", to_instance, "Geometry")
        authored_instances.append(to_instance)

    point_index = add_node(
        tree,
        "GeometryNodeInputIndex",
        "Placement_Point_Index",
        (760, -1550),
    )
    seeded_index = add_node(
        tree,
        "ShaderNodeMath",
        "Seeded_Family_Variant_Index",
        (970, -1550),
    )
    seeded_index.operation = "ADD"
    link(tree, point_index, "Index", seeded_index, "Value")
    link(tree, group_input, "Seed", seeded_index, "Value_001")
    variant = add_node(
        tree,
        "ShaderNodeMath",
        "Two_Variants_Per_Construction",
        (1170, -1550),
    )
    variant.operation = "MODULO"
    variant.inputs[1].default_value = 2.0
    link(tree, seeded_index, "Value", variant, "Value")
    even_variant = add_node(
        tree,
        "FunctionNodeBooleanMath",
        "Select_Even_Variant",
        (1370, -1570),
    )
    even_variant.operation = "NOT"
    link(tree, variant, "Value", even_variant, "Boolean")
    rivet_family = add_node(
        tree,
        "FunctionNodeBooleanMath",
        "Select_Rivet_Family",
        (1160, -1360),
    )
    rivet_family.operation = "NOT"
    link(tree, group_input, "Rose Head", rivet_family, "Boolean")
    selections: list[bpy.types.Node] = []
    selection_specs = (
        (group_input, "Rose Head", even_variant, "Boolean"),
        (group_input, "Rose Head", variant, "Value"),
        (rivet_family, "Boolean", even_variant, "Boolean"),
        (rivet_family, "Boolean", variant, "Value"),
    )
    for style_id, (
        family_source,
        family_socket,
        variant_source,
        variant_socket,
    ) in enumerate(selection_specs):
        selection = add_node(
            tree,
            "FunctionNodeBooleanMath",
            f"Select_Style_{style_id}",
            (1450, -1360 + style_id * 90),
        )
        selection.operation = "AND"
        tree.links.new(
            family_source.outputs[family_socket],
            selection.inputs[0],
        )
        tree.links.new(
            variant_source.outputs[variant_socket],
            selection.inputs[1],
        )
        selections.append(selection)

    distributed_join = add_node(
        tree,
        "GeometryNodeJoinGeometry",
        "Join_Selected_Fastener_Instances",
        (1420, 390),
    )
    for style_id, (instance, selection) in enumerate(
        zip(authored_instances, selections)
    ):
        distribute = add_node(
            tree,
            "GeometryNodeInstanceOnPoints",
            f"Distribute_Style_{style_id}",
            (1160, 760 - style_id * 230),
        )
        distribute.inputs["Pick Instance"].default_value = False
        link(tree, group_input, "Points", distribute, "Points")
        link(tree, selection, "Boolean", distribute, "Selection")
        link(tree, instance, "Instances", distribute, "Instance")
        link(tree, group_input, "Rotation", distribute, "Rotation")
        tree.links.new(
            distribute.outputs["Instances"],
            distributed_join.inputs["Geometry"],
        )
    realize = add_node(
        tree,
        "GeometryNodeRealizeInstances",
        "Realize_For_Export",
        (1580, 390),
    )
    link(tree, distributed_join, "Geometry", realize, "Geometry")
    realize_switch = add_node(
        tree,
        "GeometryNodeSwitch",
        "Procedural_Or_Realized",
        (1780, 390),
    )
    realize_switch.input_type = "GEOMETRY"
    link(tree, group_input, "Realize", realize_switch, "Switch")
    link(tree, distributed_join, "Geometry", realize_switch, "False")
    link(tree, realize, "Geometry", realize_switch, "True")
    link(tree, realize_switch, "Output", group_output, "Geometry")
    return group


def rebind_group_call(
    tree: bpy.types.NodeTree,
    node: bpy.types.Node,
    group: bpy.types.NodeTree,
) -> None:
    input_links = {
        socket.name: [link.from_socket for link in socket.links]
        for socket in node.inputs
        if socket.is_linked
    }
    output_links = {
        socket.name: [link.to_socket for link in socket.links]
        for socket in node.outputs
        if socket.is_linked
    }
    rotation = tuple(
        float(value)
        for value in node.inputs["Rotation"].default_value
    )
    node.node_tree = group
    node.inputs["Rotation"].default_value = rotation
    for socket_name, sources in input_links.items():
        socket = node.inputs.get(socket_name)
        if socket is None:
            continue
        for source in sources:
            if not any(link.from_socket == source for link in socket.links):
                tree.links.new(source, socket)
    for socket_name, targets in output_links.items():
        socket = node.outputs.get(socket_name)
        if socket is None:
            continue
        for target in targets:
            if not any(link.to_socket == target for link in socket.links):
                tree.links.new(socket, target)


def adapt_legacy_shank_center_points(
    tree: bpy.types.NodeTree,
    node: bpy.types.Node,
) -> dict[str, Any]:
    """Convert the donor door's centered-shank points to bearing-plane points."""
    point_links = list(node.inputs["Points"].links)
    if len(point_links) != 1:
        raise RuntimeError(
            f"{node.name} needs one point source for origin conversion"
        )
    source = point_links[0].from_socket
    legacy_shank_length = float(
        node.inputs["Shank Length"].default_value
    )
    rotation = Euler(
        tuple(float(value) for value in node.inputs["Rotation"].default_value),
        "XYZ",
    )
    local_offset = Vector((0.0, 0.0, legacy_shank_length * 0.5))
    world_offset = rotation.to_matrix() @ local_offset

    tree.links.remove(point_links[0])
    adapter = add_node(
        tree,
        "GeometryNodeTransform",
        "IGGY::brace_fastener_bearing_plane_adapter",
        (node.location.x - 220.0, node.location.y + 90.0),
    )
    adapter.label = "Legacy shank centre → bearing plane"
    adapter.inputs["Translation"].default_value = world_offset
    tree.links.new(source, adapter.inputs["Geometry"])
    tree.links.new(adapter.outputs["Geometry"], node.inputs["Points"])
    return {
        "node": adapter.name,
        "legacy_shank_length_m": legacy_shank_length,
        "world_offset_m": [float(value) for value in world_offset],
    }


def install_group_on_door(
    profile: dict[str, Any],
    group: bpy.types.NodeTree,
) -> dict[str, Any]:
    door_group = bpy.data.node_groups.get(DOOR_GROUP_NAME)
    if door_group is None:
        raise RuntimeError(f"Door group absent: {DOOR_GROUP_NAME}")
    caller_manifest: dict[str, Any] = {}
    for role, call_spec in profile["acceptance_door_calls"].items():
        node = door_group.nodes.get(call_spec["node"])
        if node is None or node.bl_idname != "GeometryNodeGroup":
            raise RuntimeError(f"Fastener caller absent: {call_spec['node']}")
        point_origin = call_spec["source_point_origin"]
        point_adapter = None
        if point_origin == "legacy_shank_center":
            point_adapter = adapt_legacy_shank_center_points(
                door_group,
                node,
            )
        elif point_origin != "bearing_plane":
            raise RuntimeError(
                f"Unsupported source point origin: {point_origin}"
            )
        rebind_group_call(door_group, node, group)
        values = {
            "Shank Radius": call_spec["shank_radius_m"],
            "Shank Length": call_spec["shank_length_m"],
            "Head Radius": call_spec["head_radius_m"],
            "Head Height": call_spec["head_height_m"],
            "Rose Head": call_spec["rose_head"],
            "Head Irregularity": 0.0,
            "Tip Length": call_spec["tip_length_m"],
            "Radial Segments": 16,
            "Seed": call_spec["seed"],
            "Punched Seat": call_spec["punched_seat"],
            "Realize": True,
        }
        for name, value in values.items():
            node.inputs[name].default_value = value
        caller_manifest[role] = {
            "node": node.name,
            "label": node.label,
            "values": values,
            "source_point_origin": point_origin,
            "point_adapter": point_adapter,
            "points_linked": node.inputs["Points"].is_linked,
            "material_linked": node.inputs["Iron Material"].is_linked,
        }
    old = bpy.data.node_groups.get(OLD_FASTENER_GROUP_NAME)
    if old is not None:
        old.use_fake_user = False
        if old.users:
            raise RuntimeError(
                f"Obsolete fastener group still has {old.users} live users"
            )
        bpy.data.node_groups.remove(old, do_unlink=True)
    return caller_manifest


def append_forged_iron_material() -> bpy.types.Material:
    existing = bpy.data.materials.get(IRON_MATERIAL_NAME)
    if existing is not None:
        return existing
    if not IRON_LIBRARY.is_file():
        raise FileNotFoundError(
            f"Build forged_iron_v1 before fasteners: {IRON_LIBRARY}"
        )
    with bpy.data.libraries.load(str(IRON_LIBRARY), link=False) as (
        data_from,
        data_to,
    ):
        if IRON_MATERIAL_NAME not in data_from.materials:
            raise RuntimeError(
                f"{IRON_LIBRARY} does not contain {IRON_MATERIAL_NAME}"
            )
        data_to.materials = [IRON_MATERIAL_NAME]
    material = bpy.data.materials.get(IRON_MATERIAL_NAME)
    if material is None:
        raise RuntimeError("Forged iron material failed to append")
    return material


def assign_modifier_material(
    door: bpy.types.Object,
    material: bpy.types.Material,
) -> None:
    modifier = next(
        (item for item in door.modifiers if item.type == "NODES"),
        None,
    )
    if modifier is None or modifier.node_group is None:
        raise RuntimeError("Door Geometry Nodes modifier is absent")
    socket = next(
        (
            item
            for item in modifier.node_group.interface.items_tree
            if item.item_type == "SOCKET"
            and item.in_out == "INPUT"
            and item.name == "Iron Material"
        ),
        None,
    )
    if socket is None:
        raise RuntimeError("Door has no Iron Material input")
    modifier[socket.identifier] = material


def point_object(
    obj: bpy.types.Object,
    target: tuple[float, float, float],
) -> None:
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def set_modifier_input(
    modifier: bpy.types.Modifier,
    group: bpy.types.NodeTree,
    name: str,
    value: Any,
) -> None:
    socket = next(
        (
            item
            for item in group.interface.items_tree
            if item.item_type == "SOCKET"
            and item.in_out == "INPUT"
            and item.name == name
        ),
        None,
    )
    if socket is None:
        raise RuntimeError(f"Fastener group input absent: {name}")
    modifier[socket.identifier] = value


def create_point_object(
    collection: bpy.types.Collection,
    name: str,
    location: tuple[float, float, float],
) -> bpy.types.Object:
    mesh = bpy.data.meshes.new(f"{name}_PointMesh")
    mesh.from_pydata([(0.0, 0.0, 0.0)], [], [])
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj.location = location
    return obj


def create_flat_material(
    name: str,
    colour: tuple[float, float, float, float],
    *,
    roughness: float,
) -> bpy.types.Material:
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    principled = add_node(
        tree,
        "ShaderNodeBsdfPrincipled",
        "Neutral_Proof_Surface",
        (0, 0),
    )
    principled.inputs["Base Color"].default_value = colour
    principled.inputs["Roughness"].default_value = roughness
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (240, 0),
    )
    link(tree, principled, "BSDF", output, "Surface")
    return material


def add_area_light(
    collection: bpy.types.Collection,
    name: str,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    energy: float,
    size: float,
    colour: tuple[float, float, float],
) -> bpy.types.Object:
    data = bpy.data.lights.new(name=name, type="AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = colour
    light = bpy.data.objects.new(name, data)
    collection.objects.link(light)
    light.location = location
    point_object(light, target)
    return light


def create_proof_fixture(
    profile: dict[str, Any],
    group: bpy.types.NodeTree,
    iron_material: bpy.types.Material,
) -> dict[str, list[bpy.types.Object] | bpy.types.Object]:
    collection = new_collection(PROOF_COLLECTION_NAME)
    proof_objects: list[bpy.types.Object] = []
    supports: list[bpy.types.Object] = []
    measured = profile["measured_proof"]
    definitions = (
        (
            "IGGY_FastenerProof_MetDoorNail_55_61_134",
            True,
            0,
            False,
            measured,
        ),
        (
            "IGGY_FastenerProof_FacetedSquarePeen",
            True,
            1,
            True,
            {
                "head_radius_m": 0.035,
                "head_height_m": 0.015,
                "shank_radius_m": 0.010,
                "straight_shank_length_m": 0.095,
                "tip_length_m": 0.022,
            },
        ),
        (
            "IGGY_FastenerProof_DomedRivet",
            False,
            0,
            True,
            {
                "head_radius_m": 0.032,
                "head_height_m": 0.015,
                "shank_radius_m": 0.009,
                "straight_shank_length_m": 0.080,
                "tip_length_m": 0.018,
            },
        ),
        (
            "IGGY_FastenerProof_FlattenedPeen",
            False,
            1,
            True,
            {
                "head_radius_m": 0.033,
                "head_height_m": 0.013,
                "shank_radius_m": 0.009,
                "straight_shank_length_m": 0.080,
                "tip_length_m": 0.018,
            },
        ),
    )
    x_positions = (-0.18, -0.06, 0.06, 0.18)
    for x, (name, rose, seed, punched, dimensions) in zip(
        x_positions,
        definitions,
    ):
        proof = create_point_object(collection, name, (x, 0.0, 0.0))
        modifier = proof.modifiers.new("IGGY_ForgedFastener", "NODES")
        modifier.node_group = group
        set_modifier_input(modifier, group, "Shank Radius", dimensions["shank_radius_m"])
        set_modifier_input(
            modifier,
            group,
            "Shank Length",
            dimensions["straight_shank_length_m"],
        )
        set_modifier_input(modifier, group, "Head Radius", dimensions["head_radius_m"])
        set_modifier_input(modifier, group, "Head Height", dimensions["head_height_m"])
        set_modifier_input(modifier, group, "Rose Head", rose)
        set_modifier_input(modifier, group, "Head Irregularity", 0.0)
        set_modifier_input(modifier, group, "Tip Length", dimensions["tip_length_m"])
        set_modifier_input(modifier, group, "Radial Segments", 16)
        set_modifier_input(modifier, group, "Seed", seed)
        set_modifier_input(modifier, group, "Punched Seat", punched)
        set_modifier_input(modifier, group, "Realize", True)
        set_modifier_input(modifier, group, "Iron Material", iron_material)
        proof_objects.append(proof)

        bpy.ops.mesh.primitive_cube_add(
            location=(x, 0.0, -0.055),
            scale=(0.052, 0.052, 0.055),
        )
        support = bpy.context.object
        support.name = f"{name}_BearingBlock"
        for owner in list(support.users_collection):
            owner.objects.unlink(support)
        collection.objects.link(support)
        support.data.materials.append(
            create_flat_material(
                "IGGY_MAT_FastenerProofBearing",
                (0.20, 0.145, 0.095, 1.0),
                roughness=0.72,
            )
        )
        supports.append(support)

    bpy.ops.mesh.primitive_plane_add(
        size=1.6,
        location=(0.0, 0.0, -0.155),
    )
    floor = bpy.context.object
    floor.name = "IGGY_FastenerProof_Floor"
    for owner in list(floor.users_collection):
        owner.objects.unlink(floor)
    collection.objects.link(floor)
    floor.data.materials.append(
        create_flat_material(
            "IGGY_MAT_FastenerProofFloor",
            (0.025, 0.03, 0.038, 1.0),
            roughness=0.64,
        )
    )
    supports.append(floor)

    lights = [
        add_area_light(
            collection,
            "IGGY_FastenerProof_RakingKey",
            (-0.38, -0.34, 0.34),
            (0.0, 0.0, 0.0),
            14.0,
            0.20,
            (1.0, 0.86, 0.70),
        ),
        add_area_light(
            collection,
            "IGGY_FastenerProof_CoolFill",
            (0.42, -0.18, 0.20),
            (0.0, 0.0, -0.02),
            5.5,
            0.28,
            (0.62, 0.76, 1.0),
        ),
        add_area_light(
            collection,
            "IGGY_FastenerProof_Rim",
            (0.0, 0.30, 0.26),
            (0.0, 0.0, -0.04),
            8.0,
            0.16,
            (1.0, 0.46, 0.24),
        ),
    ]
    return {
        "proofs": proof_objects,
        "supports": supports,
        "lights": lights,
    }


def realize_door(door: bpy.types.Object) -> dict[str, Any]:
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = door.evaluated_get(depsgraph)
    mesh = bpy.data.meshes.new_from_object(
        evaluated,
        preserve_all_data_layers=True,
        depsgraph=depsgraph,
    )
    old_mesh = door.data
    door.data = mesh
    door.data.name = "IGGY_MESH_DoorWithAuthoredFasteners_v001"
    door.modifiers.clear()
    if old_mesh.users == 0:
        bpy.data.meshes.remove(old_mesh)
    return {
        "vertices": len(mesh.vertices),
        "polygons": len(mesh.polygons),
        "attributes": sorted(attribute.name for attribute in mesh.attributes),
        "materials": [
            material.name if material else None for material in mesh.materials
        ],
    }


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


def set_objects_visible(
    objects: list[bpy.types.Object],
    visible: bool,
) -> None:
    for obj in objects:
        obj.hide_render = not visible


def render_acceptance(
    scene: bpy.types.Scene,
    door: bpy.types.Object,
    proof: dict[str, list[bpy.types.Object] | bpy.types.Object],
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    renders: dict[str, Any] = {}
    camera = scene.camera
    if camera is None:
        raise RuntimeError("Acceptance scene has no camera")
    proof_objects = (
        list(proof["proofs"])
        + list(proof["supports"])
        + list(proof["lights"])
    )
    master_collection = bpy.data.collections.get(MASTER_COLLECTION_NAME)
    master_objects = (
        list(master_collection.objects)
        if master_collection is not None
        else []
    )
    door_lights = [
        obj
        for obj in scene.objects
        if obj.type == "LIGHT" and obj not in proof["lights"]
    ]
    all_other_visible = [
        obj
        for obj in scene.objects
        if obj not in proof_objects
        and obj not in master_objects
        and obj != camera
    ]
    original = {
        obj.name: obj.hide_render
        for obj in scene.objects
    }
    camera_state = (
        camera.location.copy(),
        camera.rotation_euler.copy(),
        camera.data.type,
        camera.data.ortho_scale,
        camera.data.lens,
    )
    light_states = {
        obj.name: obj.location.copy()
        for obj in door_lights
    }

    set_objects_visible(proof_objects, False)
    set_objects_visible(master_objects, False)
    set_objects_visible(all_other_visible, True)
    scene.render.resolution_x = args.door_resolution_x
    scene.render.resolution_y = args.door_resolution_y
    scene.camera.data.type = "ORTHO"
    renders["door_reverse"] = render_still(
        scene,
        output_root / "forged_fasteners_v1_door_reverse.png",
    )

    camera.location.y = -abs(camera.location.y)
    point_object(camera, (0.0, 0.0, 2.35))
    for light in door_lights:
        light.location.y = -light.location.y
        point_object(light, (0.0, 0.0, 2.35))
    renders["door_front"] = render_still(
        scene,
        output_root / "forged_fasteners_v1_door_front.png",
    )

    set_objects_visible(all_other_visible, False)
    set_objects_visible(proof_objects, True)
    camera.data.type = "PERSP"
    camera.data.lens = 62.0
    camera.location = (0.38, -0.55, 0.30)
    point_object(camera, (0.0, 0.0, -0.025))
    scene.render.resolution_x = args.proof_resolution_x
    scene.render.resolution_y = args.proof_resolution_y
    renders["seated_heads"] = render_still(
        scene,
        output_root / "forged_fasteners_v1_seated_heads.png",
    )

    set_objects_visible(list(proof["supports"]), False)
    camera.location = (0.42, -0.62, -0.015)
    point_object(camera, (0.0, 0.0, -0.045))
    renders["full_anatomy"] = render_still(
        scene,
        output_root / "forged_fasteners_v1_full_anatomy.png",
    )

    camera.location = camera_state[0]
    camera.rotation_euler = camera_state[1]
    camera.data.type = camera_state[2]
    camera.data.ortho_scale = camera_state[3]
    camera.data.lens = camera_state[4]
    for light in door_lights:
        light.location = light_states[light.name]
        point_object(light, (0.0, 0.0, 2.35))
    for obj in scene.objects:
        obj.hide_render = original.get(obj.name, obj.hide_render)
    door.hide_render = False
    return renders


def evaluated_dimensions(obj: bpy.types.Object) -> list[float]:
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = obj.evaluated_get(depsgraph)
    return [float(value) for value in evaluated.dimensions]


def validate_contract(
    profile: dict[str, Any],
    group: bpy.types.NodeTree,
    masters: dict[str, bpy.types.Object],
    proof: dict[str, list[bpy.types.Object] | bpy.types.Object],
    door: bpy.types.Object,
    wood_before: tuple[Any, ...],
    wood_after: tuple[Any, ...],
) -> dict[str, Any]:
    forbidden = [
        node.bl_idname
        for node in group.nodes
        if node.bl_idname in {"ShaderNodeTexNoise", "ShaderNodeTexVoronoi"}
    ]
    if forbidden:
        raise RuntimeError(f"Generic deformation nodes found: {forbidden}")
    expected_master_names = {
        "IGGY_FastenerMaster_Rosehead",
        "IGGY_FastenerMaster_FacetedSquarePeen",
        "IGGY_FastenerMaster_DomedRivet",
        "IGGY_FastenerMaster_FlattenedPeen",
    }
    if {obj.name for obj in masters.values()} != expected_master_names:
        raise RuntimeError("Authored fastener master set drifted")
    proof_obj = bpy.data.objects["IGGY_FastenerProof_MetDoorNail_55_61_134"]
    measured = evaluated_dimensions(proof_obj)
    expected = profile["measured_proof"]["overall_envelope_m"]
    if any(abs(a - b) > 1.0e-4 for a, b in zip(measured, expected)):
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated_mesh = proof_obj.evaluated_get(depsgraph).to_mesh()
        style_attribute = evaluated_mesh.attributes.get("sinc_fastener_style")
        styles = (
            sorted({item.value for item in style_attribute.data})
            if style_attribute is not None
            else []
        )
        role_attribute = evaluated_mesh.attributes.get(
            "sinc_fastener_surface_role"
        )
        role_bounds: dict[int, list[float]] = {}
        if role_attribute is not None:
            for role in sorted({item.value for item in role_attribute.data}):
                z_values = [
                    evaluated_mesh.vertices[index].co.z
                    for index, item in enumerate(role_attribute.data)
                    if item.value == role
                ]
                if z_values:
                    role_bounds[int(role)] = [
                        float(min(z_values)),
                        float(max(z_values)),
                    ]
        raise RuntimeError(
            "Measured nail envelope drifted: "
            f"actual={measured}, expected={expected}, styles={styles}, "
            f"role_z_bounds={role_bounds}"
        )
    attributes = {attribute.name for attribute in door.data.attributes}
    required = {
        "sinc_fastener_style",
        "sinc_fastener_surface_role",
        "sinc_iron_edge_mask",
        "sinc_seed",
        "sinc_iron_u_m",
        "sinc_iron_v_m",
    }
    if not required.issubset(attributes):
        raise RuntimeError(
            f"Door fastener attributes missing: {sorted(required - attributes)}"
        )
    material_names = {
        material.name
        for material in door.data.materials
        if material is not None
    }
    if IRON_MATERIAL_NAME not in material_names:
        raise RuntimeError("Reference forged iron is not assigned to door")
    if wood_before != wood_after:
        raise RuntimeError("Approved structural oak material changed")
    stores = {
        node.inputs["Name"].default_value
        for node in group.nodes
        if node.bl_idname == "GeometryNodeStoreNamedAttribute"
    }
    return {
        "node_count": len(group.nodes),
        "generic_deformation_node_count": len(forbidden),
        "master_objects": sorted(expected_master_names),
        "store_named_attributes": sorted(stores),
        "measured_proof_dimensions_m": measured,
        "door_attributes": sorted(attributes),
        "door_materials": sorted(material_names),
        "approved_wood_unchanged": True,
    }


def main() -> None:
    args = parse_args()
    profile = load_profile()
    source = args.source.expanduser().resolve()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if not source.is_file():
        raise FileNotFoundError(f"Approved door source absent: {source}")

    bpy.ops.wm.open_mainfile(filepath=str(source))
    door = bpy.data.objects.get(DOOR_OBJECT_NAME)
    wood_material = bpy.data.materials.get(WOOD_MATERIAL_NAME)
    if door is None or wood_material is None:
        raise RuntimeError("Approved structural door contract is incomplete")
    iron_builder = load_iron_builder()
    wood_before = iron_builder.node_contract(wood_material.node_tree)
    iron_material = append_forged_iron_material()
    masters, rim_master, tip_masters = create_masters(profile)
    group = create_fastener_group(
        profile,
        masters,
        rim_master,
        tip_masters,
    )
    callers = install_group_on_door(profile, group)
    assign_modifier_material(door, iron_material)

    proof = create_proof_fixture(profile, group, iron_material)
    door_realization = realize_door(door)
    iron_profile = json.loads(IRON_PROFILE_PATH.read_text())
    coordinate_build = iron_builder.ensure_component_local_iron_coordinates(
        door,
        iron_material,
        iron_profile,
    )
    wood_after = iron_builder.node_contract(wood_material.node_tree)

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.view_settings.look = "AgX - Medium High Contrast"
    validation = validate_contract(
        profile,
        group,
        masters,
        proof,
        door,
        wood_before,
        wood_after,
    )
    renders = render_acceptance(
        scene,
        door,
        proof,
        output_root,
        args,
    )

    blend_path = output_root / "forged_fasteners_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    manifest = {
        "schema": "iggy-forged-fasteners-build/1.0",
        "status": "MEASURED_FORGED_FASTENERS_BUILT",
        "profile": str(PROFILE_PATH),
        "source_fixture": {
            "path": str(source),
            "sha256": sha256_file(source),
        },
        "material_library": {
            "path": str(IRON_LIBRARY),
            "sha256": sha256_file(IRON_LIBRARY),
            "material": IRON_MATERIAL_NAME,
        },
        "canonical_owner": {
            "node_group": FASTENER_GROUP_NAME,
            "profile": str(PROFILE_PATH),
            "obsolete_group_removed": OLD_FASTENER_GROUP_NAME,
        },
        "door_callers": callers,
        "door_realization": door_realization,
        "coordinate_build": coordinate_build,
        "validation": validation,
        "constraints": {
            "uses_authored_meshes": True,
            "uses_noise_displacement": False,
            "uses_ai_generated_imagery": False,
            "uses_damage": False,
            "uses_corrosion": False,
            "measured_nail_envelope_exact": True,
            "approved_wood_changed": False,
        },
        "renders": renders,
        "saved_blend": {
            "path": str(blend_path),
            "bytes": blend_path.stat().st_size,
            "sha256": sha256_file(blend_path),
        },
    }
    manifest_path = output_root / "forged_fasteners_v1_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
