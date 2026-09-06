#!/usr/bin/env python3
"""Build the reference-derived structural-oak acceptance scene in Blender."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector


SCRIPT_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = (
    SCRIPT_ROOT / "profiles" / "structural_oak_door_v1.json"
)
GENERATOR_PATH = SCRIPT_ROOT / "generate_structural_oak_patterns_v1.py"
DEFAULT_OUTPUT_ROOT = SCRIPT_ROOT / "output"
DEFAULT_SOURCE = Path(
    "/Users/kogaryu/Documents/Codex/2026-07-25/sinc/outputs/"
    "sinc-blender-node-library-v1/sinc_blender_node_library_v1.blend"
)

DOOR_OBJECT_NAME = "SINC_DemoDoorAssembly"
SOURCE_WOOD_MATERIAL_NAME = "SINC_MAT_WornOak_v001"
SOURCE_IRON_MATERIAL_NAME = "SINC_MAT_ForgedIron_v001"
OAK_GROUP_NAME = "IGGY_SH_ReferenceStructuralOak_v001"
OAK_MATERIAL_NAME = "IGGY_MAT_ReferenceStructuralOakDoor_v001"
IRON_MATERIAL_NAME = "IGGY_MAT_NeutralIronProof_v001"

PROOF_MODES = {
    "colour": 0.0,
    "linework": 1.0,
    "combined": 2.0,
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
    parser.add_argument("--resolution-x", type=int, default=900)
    parser.add_argument("--resolution-y", type=int, default=1080)
    parser.add_argument("--tile-resolution", type=int, default=512)
    return parser.parse_args(argv)


def load_profile() -> dict:
    profile = json.loads(PROFILE_PATH.read_text())
    assert profile["profile_id"] == "structural_oak_door_v1"
    assert (
        len(profile["palette"]["observed_reference_twenty"])
        == 20
    )
    assert profile["surface_response"]["model"] == "uniform_diffuse"
    assert not profile["surface_response"]["authored_roughness"]
    return profile


def load_generator_module():
    specification = importlib.util.spec_from_file_location(
        "iggy_structural_oak_pattern_generator",
        GENERATOR_PATH,
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"Cannot load generator: {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def srgb_channel_to_linear(value: float) -> float:
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def hex_colour(value: str) -> tuple[float, float, float, float]:
    value = value.removeprefix("#")
    channels = [
        int(value[index : index + 2], 16) / 255.0
        for index in (0, 2, 4)
    ]
    return tuple(
        srgb_channel_to_linear(channel) for channel in channels
    ) + (1.0,)


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
        raise KeyError(
            f"{node.bl_idname} has no {socket_name!r} input"
        )
    socket.default_value = value


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


def create_oak_group(
    profile: dict,
    colour_image: bpy.types.Image,
    ink_image: bpy.types.Image,
    *,
    tile_resolution: int,
) -> bpy.types.NodeTree:
    previous = bpy.data.node_groups.get(OAK_GROUP_NAME)
    if previous is not None:
        bpy.data.node_groups.remove(previous, do_unlink=True)

    group = bpy.data.node_groups.new(OAK_GROUP_NAME, "ShaderNodeTree")
    group.color_tag = "CONVERTER"
    group.description = (
        "Places real-door-derived colour fields and restrained authored "
        "fibre ink by timber seed; contributes no roughness or damage."
    )
    ink_socket = group.interface.new_socket(
        name="Ink Strength",
        in_out="INPUT",
        socket_type="NodeSocketFloat",
    )
    ink_socket.default_value = profile["layers"][4]["strength"]
    ink_socket.min_value = 0.0
    ink_socket.max_value = 1.0
    group.interface.new_socket(
        name="Base Color",
        in_out="OUTPUT",
        socket_type="NodeSocketColor",
    )
    group.interface.new_socket(
        name="Ink Mask",
        in_out="OUTPUT",
        socket_type="NodeSocketFloat",
    )
    group.interface.new_socket(
        name="Combined Color",
        in_out="OUTPUT",
        socket_type="NodeSocketColor",
    )
    group.interface.new_socket(
        name="Linework Proof",
        in_out="OUTPUT",
        socket_type="NodeSocketColor",
    )

    tree = group
    group_input = add_node(
        tree,
        "NodeGroupInput",
        "Group_Input",
        (-1700, -180),
    )
    group_output = add_node(
        tree,
        "NodeGroupOutput",
        "Group_Output",
        (1120, 180),
    )
    uv_attribute = add_node(
        tree,
        "ShaderNodeAttribute",
        "Construction_UV",
        (-1700, 420),
    )
    uv_attribute.attribute_name = profile["coordinate_contract"][
        "uv_attribute"
    ]
    seed_attribute = add_node(
        tree,
        "ShaderNodeAttribute",
        "Timber_Variant_Seed",
        (-1700, 80),
    )
    seed_attribute.attribute_name = profile["coordinate_contract"][
        "variant_attribute"
    ]
    separate_uv = add_node(
        tree,
        "ShaderNodeSeparateXYZ",
        "Separate_Construction_UV",
        (-1490, 420),
    )
    link(tree, uv_attribute, "Vector", separate_uv, "Vector")

    local_u_subtract = add_math(
        tree,
        "Local_Longitudinal_Origin",
        "SUBTRACT",
        (-1280, 520),
        second=0.25,
    )
    local_u_scale = add_math(
        tree,
        "Local_Longitudinal_Range",
        "MULTIPLY",
        (-1070, 520),
        second=4.0,
    )
    local_u_clamp = add_node(
        tree,
        "ShaderNodeClamp",
        "Clamp_Local_Longitudinal",
        (-860, 520),
    )
    set_input(local_u_clamp, "Min", 0.0)
    set_input(local_u_clamp, "Max", 1.0)
    link(tree, separate_uv, "X", local_u_subtract, "Value")
    link(tree, local_u_subtract, "Value", local_u_scale, "Value")
    link(tree, local_u_scale, "Value", local_u_clamp, "Value")

    local_v_subtract = add_math(
        tree,
        "Local_Transverse_Origin",
        "SUBTRACT",
        (-1280, 360),
        second=0.125,
    )
    local_v_scale = add_math(
        tree,
        "Local_Transverse_Range",
        "DIVIDE",
        (-1070, 360),
        second=0.75,
    )
    local_v_clamp = add_node(
        tree,
        "ShaderNodeClamp",
        "Clamp_Local_Transverse",
        (-860, 360),
    )
    set_input(local_v_clamp, "Min", 0.0)
    set_input(local_v_clamp, "Max", 1.0)
    link(tree, separate_uv, "Y", local_v_subtract, "Value")
    link(tree, local_v_subtract, "Value", local_v_scale, "Value")
    link(tree, local_v_scale, "Value", local_v_clamp, "Value")

    seed_scale = add_math(
        tree,
        "Hash_Seed_Scale",
        "MULTIPLY",
        (-1490, 80),
        second=12.9898,
    )
    seed_sine = add_math(
        tree,
        "Hash_Seed_Sine",
        "SINE",
        (-1280, 80),
    )
    seed_spread = add_math(
        tree,
        "Hash_Seed_Spread",
        "MULTIPLY",
        (-1070, 80),
        second=43758.5453,
    )
    seed_fraction = add_math(
        tree,
        "Hash_Seed_Fraction",
        "FRACT",
        (-860, 80),
    )
    tile_scale = add_math(
        tree,
        "Twelve_Authored_Variants",
        "MULTIPLY",
        (-650, 80),
        second=12.0,
    )
    tile_index = add_math(
        tree,
        "Authored_Variant_Index",
        "FLOOR",
        (-440, 80),
    )
    atlas_column = add_math(
        tree,
        "Atlas_Column",
        "MODULO",
        (-230, 80),
        second=4.0,
    )
    atlas_row_divide = add_math(
        tree,
        "Atlas_Row_Divide",
        "DIVIDE",
        (-230, -80),
        second=4.0,
    )
    atlas_row = add_math(
        tree,
        "Atlas_Row",
        "FLOOR",
        (-20, -80),
    )
    link(tree, seed_attribute, "Fac", seed_scale, "Value")
    link(tree, seed_scale, "Value", seed_sine, "Value")
    link(tree, seed_sine, "Value", seed_spread, "Value")
    link(tree, seed_spread, "Value", seed_fraction, "Value")
    link(tree, seed_fraction, "Value", tile_scale, "Value")
    link(tree, tile_scale, "Value", tile_index, "Value")
    link(tree, tile_index, "Value", atlas_column, "Value")
    link(tree, tile_index, "Value", atlas_row_divide, "Value")
    link(tree, atlas_row_divide, "Value", atlas_row, "Value")

    inset = 4.0 / float(tile_resolution)
    inner_scale = 1.0 - 2.0 * inset
    u_inner_scale = add_math(
        tree,
        "Inset_Longitudinal_Scale",
        "MULTIPLY",
        (-650, 560),
        second=inner_scale,
    )
    u_inner_offset = add_math(
        tree,
        "Inset_Longitudinal_Offset",
        "ADD",
        (-440, 560),
        second=inset,
    )
    v_inner_scale = add_math(
        tree,
        "Inset_Transverse_Scale",
        "MULTIPLY",
        (-650, 400),
        second=inner_scale,
    )
    v_inner_offset = add_math(
        tree,
        "Inset_Transverse_Offset",
        "ADD",
        (-440, 400),
        second=inset,
    )
    link(tree, local_u_clamp, "Result", u_inner_scale, "Value")
    link(tree, u_inner_scale, "Value", u_inner_offset, "Value")
    link(tree, local_v_clamp, "Result", v_inner_scale, "Value")
    link(tree, v_inner_scale, "Value", v_inner_offset, "Value")

    atlas_u_add = add_math(
        tree,
        "Atlas_U_Tile",
        "ADD",
        (-20, 500),
    )
    atlas_u_divide = add_math(
        tree,
        "Atlas_U",
        "DIVIDE",
        (190, 500),
        second=4.0,
    )
    atlas_v_add = add_math(
        tree,
        "Atlas_V_Tile",
        "ADD",
        (-20, 340),
    )
    atlas_v_divide = add_math(
        tree,
        "Atlas_V",
        "DIVIDE",
        (190, 340),
        second=3.0,
    )
    atlas_vector = add_node(
        tree,
        "ShaderNodeCombineXYZ",
        "Authored_Atlas_Coordinate",
        (400, 420),
    )
    link(tree, atlas_column, "Value", atlas_u_add, "Value")
    link(tree, u_inner_offset, "Value", atlas_u_add, "Value_001")
    link(tree, atlas_u_add, "Value", atlas_u_divide, "Value")
    link(tree, atlas_row, "Value", atlas_v_add, "Value")
    link(tree, v_inner_offset, "Value", atlas_v_add, "Value_001")
    link(tree, atlas_v_add, "Value", atlas_v_divide, "Value")
    link(tree, atlas_u_divide, "Value", atlas_vector, "X")
    link(tree, atlas_v_divide, "Value", atlas_vector, "Y")

    colour_texture = add_node(
        tree,
        "ShaderNodeTexImage",
        "Authored_Colour_Atlas",
        (610, 600),
    )
    colour_texture.image = colour_image
    colour_texture.interpolation = "Linear"
    colour_texture.extension = "EXTEND"
    ink_texture = add_node(
        tree,
        "ShaderNodeTexImage",
        "Authored_Ink_Atlas",
        (610, 300),
    )
    ink_texture.image = ink_image
    ink_texture.interpolation = "Linear"
    ink_texture.extension = "EXTEND"
    ink_value = add_node(
        tree,
        "ShaderNodeRGBToBW",
        "Authored_Ink_Value",
        (820, 300),
    )
    ink_strength = add_math(
        tree,
        "Authored_Ink_Strength",
        "MULTIPLY",
        (1020, 300),
        clamp=True,
    )
    combined = add_mix(
        tree,
        "Authored_Ink_Over_Colour",
        (810, 80),
    )
    combined.inputs["Color2"].default_value = hex_colour(
        profile["palette"]["ink"]
    )
    linework = add_mix(
        tree,
        "Authored_Linework_Proof",
        (810, -140),
    )
    linework.inputs["Color1"].default_value = hex_colour(
        profile["palette"]["linework_proof_field"]
    )
    linework.inputs["Color2"].default_value = hex_colour(
        profile["palette"]["ink"]
    )
    link(tree, atlas_vector, "Vector", colour_texture, "Vector")
    link(tree, atlas_vector, "Vector", ink_texture, "Vector")
    link(tree, ink_texture, "Color", ink_value, "Color")
    link(tree, ink_value, "Val", ink_strength, "Value")
    link(tree, group_input, "Ink Strength", ink_strength, "Value_001")
    link(tree, ink_strength, "Value", combined, "Fac")
    link(tree, colour_texture, "Color", combined, "Color1")
    link(tree, ink_strength, "Value", linework, "Fac")
    link(tree, colour_texture, "Color", group_output, "Base Color")
    link(tree, ink_strength, "Value", group_output, "Ink Mask")
    link(tree, combined, "Color", group_output, "Combined Color")
    link(tree, linework, "Color", group_output, "Linework Proof")
    return group


def configure_oak_material(
    material: bpy.types.Material,
    group: bpy.types.NodeTree,
) -> bpy.types.Node:
    material.name = OAK_MATERIAL_NAME
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()

    oak = add_node(
        tree,
        "ShaderNodeGroup",
        "Authored_Structural_Oak",
        (-620, 180),
    )
    oak.node_tree = group
    mode = add_node(
        tree,
        "ShaderNodeValue",
        "IGGY_PROOF_MODE",
        (-620, -220),
    )
    mode.outputs[0].default_value = PROOF_MODES["combined"]
    line_threshold = add_math(
        tree,
        "Use_Linework_Proof",
        "GREATER_THAN",
        (-390, -180),
        second=0.5,
    )
    combined_threshold = add_math(
        tree,
        "Use_Combined_Proof",
        "GREATER_THAN",
        (-390, -300),
        second=1.5,
    )
    first_select = add_mix(
        tree,
        "Select_Colour_Or_Linework",
        (-110, 160),
    )
    final_select = add_mix(
        tree,
        "Select_Combined",
        (130, 160),
    )
    diffuse = add_node(
        tree,
        "ShaderNodeBsdfDiffuse",
        "Uniform_Diffuse",
        (390, 160),
    )
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (620, 160),
    )
    link(tree, mode, "Value", line_threshold, "Value")
    link(tree, mode, "Value", combined_threshold, "Value")
    link(tree, line_threshold, "Value", first_select, "Fac")
    link(tree, oak, "Base Color", first_select, "Color1")
    link(tree, oak, "Linework Proof", first_select, "Color2")
    link(tree, combined_threshold, "Value", final_select, "Fac")
    link(tree, first_select, "Color", final_select, "Color1")
    link(tree, oak, "Combined Color", final_select, "Color2")
    link(tree, final_select, "Color", diffuse, "Color")
    link(tree, diffuse, "BSDF", output, "Surface")
    material.diffuse_color = hex_colour("#735A48")
    return mode


def configure_flat_diffuse_material(
    material: bpy.types.Material,
    name: str,
    colour: tuple[float, float, float, float],
) -> None:
    material.name = name
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    diffuse = add_node(
        tree,
        "ShaderNodeBsdfDiffuse",
        "Uniform_Diffuse",
        (-120, 0),
    )
    diffuse.inputs["Color"].default_value = colour
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (120, 0),
    )
    link(tree, diffuse, "BSDF", output, "Surface")
    material.diffuse_color = colour


def point_object(
    obj: bpy.types.Object,
    target: tuple[float, float, float],
) -> None:
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def add_area_light(
    name: str,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    *,
    energy: float,
    size: float,
    colour: tuple[float, float, float],
) -> None:
    data = bpy.data.lights.new(name=name, type="AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = colour
    light = bpy.data.objects.new(name=name, object_data=data)
    bpy.context.scene.collection.objects.link(light)
    light.location = location
    point_object(light, target)


def configure_scene(
    door: bpy.types.Object,
    resolution_x: int,
    resolution_y: int,
) -> bpy.types.Scene:
    scene = bpy.context.scene
    for obj in list(bpy.data.objects):
        if obj != door:
            bpy.data.objects.remove(obj, do_unlink=True)
    door.location = (0.0, 0.0, 0.0)
    door.hide_render = False
    door.hide_viewport = False

    bpy.ops.mesh.primitive_plane_add(
        size=18.0,
        location=(0.0, 0.0, -0.035),
    )
    floor = bpy.context.object
    floor.name = "IGGY_ProofFloor"
    floor_material = bpy.data.materials.new("IGGY_MAT_ProofFloor")
    configure_flat_diffuse_material(
        floor_material,
        "IGGY_MAT_ProofFloor",
        hex_colour("#555555"),
    )
    floor.data.materials.append(floor_material)

    camera_data = bpy.data.cameras.new(
        "IGGY_CAM_AuthoredStructuralOak",
    )
    camera = bpy.data.objects.new(
        "IGGY_CAM_AuthoredStructuralOak",
        camera_data,
    )
    scene.collection.objects.link(camera)
    camera.location = (3.85, 7.1, 3.25)
    point_object(camera, (0.0, 0.06, 2.40))
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 5.58
    scene.camera = camera

    add_area_light(
        "IGGY_Key",
        (-3.8, 4.5, 7.4),
        (0.0, 0.0, 2.4),
        energy=480.0,
        size=4.2,
        colour=(1.0, 1.0, 1.0),
    )
    add_area_light(
        "IGGY_Fill",
        (4.8, 3.5, 4.0),
        (0.0, 0.0, 2.25),
        energy=220.0,
        size=5.0,
        colour=(1.0, 1.0, 1.0),
    )
    add_area_light(
        "IGGY_Rim",
        (-2.0, -3.0, 5.8),
        (0.0, 0.0, 2.6),
        energy=250.0,
        size=3.0,
        colour=(1.0, 1.0, 1.0),
    )

    world = scene.world or bpy.data.worlds.new("IGGY_ProofWorld")
    scene.world = world
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = hex_colour("#383838")
    background.inputs["Strength"].default_value = 0.12

    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = resolution_x
    scene.render.resolution_y = resolution_y
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.look = "AgX - Medium High Contrast"
    return scene


def evaluated_door_contract(door: bpy.types.Object) -> dict:
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


def validate_contract(
    profile: dict,
    group: bpy.types.NodeTree,
    oak_material: bpy.types.Material,
    door_contract: dict,
) -> dict:
    required_attributes = {
        profile["coordinate_contract"]["uv_attribute"],
        profile["coordinate_contract"]["longitudinal_attribute"],
        profile["coordinate_contract"]["variant_attribute"],
        profile["coordinate_contract"]["end_grain_attribute"],
    }
    missing = sorted(
        required_attributes - set(door_contract["attributes"])
    )
    if missing:
        raise RuntimeError(f"Door is missing attributes: {missing}")

    forbidden_node_types = {
        "ShaderNodeBump",
        "ShaderNodeNormalMap",
        "ShaderNodeBsdfPrincipled",
    }
    all_nodes = list(group.nodes) + list(oak_material.node_tree.nodes)
    found_forbidden = sorted(
        {node.bl_idname for node in all_nodes} & forbidden_node_types
    )
    if found_forbidden:
        raise RuntimeError(
            f"Forbidden material nodes present: {found_forbidden}"
        )

    image_nodes = [
        node
        for node in group.nodes
        if node.bl_idname == "ShaderNodeTexImage"
    ]
    if len(image_nodes) != 2:
        raise RuntimeError("Expected exactly authored colour and ink atlases")
    attribute_names = {
        node.attribute_name
        for node in group.nodes
        if node.bl_idname == "ShaderNodeAttribute"
    }
    expected_shader_attributes = {
        profile["coordinate_contract"]["uv_attribute"],
        profile["coordinate_contract"]["variant_attribute"],
    }
    if attribute_names != expected_shader_attributes:
        raise RuntimeError(
            "Shader reads attributes outside placement contract: "
            f"{sorted(attribute_names)}"
        )
    return {
        "required_door_attributes": sorted(required_attributes),
        "shader_attributes": sorted(attribute_names),
        "group_node_count": len(group.nodes),
        "material_node_count": len(oak_material.node_tree.nodes),
        "image_texture_count": len(image_nodes),
        "forbidden_node_types_found": found_forbidden,
        "surface_variation_nodes": 0,
        "damage_inputs": 0,
    }


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def render_proofs(
    scene: bpy.types.Scene,
    proof_mode: bpy.types.Node,
    output_root: Path,
) -> dict:
    renders = {}
    for name, mode in PROOF_MODES.items():
        proof_mode.outputs[0].default_value = mode
        path = output_root / f"structural_oak_door_v1_{name}.png"
        scene.render.filepath = str(path)
        bpy.ops.render.render(write_still=True)
        renders[name] = {
            "path": str(path),
            "bytes": path.stat().st_size,
            "sha256": sha256_file(path),
        }
    proof_mode.outputs[0].default_value = PROOF_MODES["combined"]
    return renders


def main() -> None:
    args = parse_args()
    profile = load_profile()
    source = args.source.expanduser().resolve()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if not source.is_file():
        raise FileNotFoundError(f"Door source not found: {source}")

    generator = load_generator_module()
    atlas_manifest = generator.generate(
        output_root,
        tile_resolution=args.tile_resolution,
    )
    colour_path = Path(atlas_manifest["outputs"]["colour"]["path"])
    ink_path = Path(atlas_manifest["outputs"]["ink"]["path"])

    bpy.ops.wm.open_mainfile(filepath=str(source))
    door = bpy.data.objects.get(DOOR_OBJECT_NAME)
    if door is None:
        raise RuntimeError(
            f"Source does not contain {DOOR_OBJECT_NAME!r}"
        )
    oak_material = bpy.data.materials.get(SOURCE_WOOD_MATERIAL_NAME)
    iron_material = bpy.data.materials.get(SOURCE_IRON_MATERIAL_NAME)
    if oak_material is None or iron_material is None:
        raise RuntimeError("Expected donor materials are absent")

    colour_image = bpy.data.images.load(
        str(colour_path),
        check_existing=False,
    )
    colour_image.name = "IGGY_IMG_AuthoredOakColourAtlas_v001"
    colour_image.colorspace_settings.name = "sRGB"
    ink_image = bpy.data.images.load(
        str(ink_path),
        check_existing=False,
    )
    ink_image.name = "IGGY_IMG_AuthoredOakInkAtlas_v001"
    ink_image.colorspace_settings.name = "Non-Color"

    group = create_oak_group(
        profile,
        colour_image,
        ink_image,
        tile_resolution=args.tile_resolution,
    )
    proof_mode = configure_oak_material(oak_material, group)
    configure_flat_diffuse_material(
        iron_material,
        IRON_MATERIAL_NAME,
        hex_colour("#25282B"),
    )
    scene = configure_scene(
        door,
        resolution_x=args.resolution_x,
        resolution_y=args.resolution_y,
    )
    door_contract = evaluated_door_contract(door)
    shader_contract = validate_contract(
        profile,
        group,
        oak_material,
        door_contract,
    )
    renders = render_proofs(scene, proof_mode, output_root)

    colour_image.pack()
    ink_image.pack()
    blend_path = output_root / "structural_oak_door_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))

    manifest = {
        "schema": "iggy-structural-oak-build/2.0",
        "status": "REFERENCE_ATLAS_BUILT_AND_HEADLESSLY_VALIDATED",
        "profile": str(PROFILE_PATH),
        "source_fixture": str(source),
        "atlas_build": atlas_manifest,
        "saved_blend": {
            "path": str(blend_path),
            "bytes": blend_path.stat().st_size,
            "sha256": sha256_file(blend_path),
            "authored_images_packed": True,
        },
        "constraints": {
            "uses_floor_pattern": False,
            "uses_ai_generated_imagery": False,
            "uses_real_door_reference": True,
            "uses_surface_variation": False,
            "uses_relief": False,
            "uses_damage": False,
            "proof_surface": "uniform diffuse",
        },
        "door_contract": door_contract,
        "shader_contract": shader_contract,
        "renders": renders,
    }
    manifest_path = output_root / "structural_oak_door_v1_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
