#!/usr/bin/env python3
"""Build the Boolean-safe structural-oak joinery acceptance fixture."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import sys
from typing import Any

import bpy
import bmesh
from mathutils import Vector


SCRIPT_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "structural_oak_joinery_v1.json"
PATTERN_PATH = (
    SCRIPT_ROOT / "patterns" / "structural_oak_endgrain_variants_v1.json"
)
OUTPUT_ROOT = SCRIPT_ROOT / "output"
PREFIX = "structural_oak_joinery_v1"
TIMBER_MATERIAL_NAME = "IGGY_MAT_StructuralOakTimberV1"
CUT_MATERIAL_NAME = "IGGY_MAT_StructuralOakJoineryCutsV1"
PEG_MATERIAL_NAME = "IGGY_MAT_RivenOakPegV1"
MARKER_MATERIAL_NAME = "IGGY_MAT_JoineryBooleanTransferMarker"
RECEIVING_NAME = "IGGY_Joinery_ReceivingTimber"
ENTERING_NAME = "IGGY_Joinery_EnteringTenon"
PEG_NAME = "IGGY_Joinery_OctagonalDrawPeg"
COLLECTION_NAME = "IGGY_StructuralOakJoineryProof"


def parse_args() -> argparse.Namespace:
    values = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=OUTPUT_ROOT)
    parser.add_argument("--render-width", type=int, default=1440)
    parser.add_argument("--render-height", type=int, default=810)
    parser.add_argument("--skip-renders", action="store_true")
    return parser.parse_args(values)


def _resolve(profile_path: Path, relative: str) -> Path:
    return (profile_path.parent.parent / relative).resolve()


def _load_contract() -> tuple[dict[str, Any], dict[str, Any], Path]:
    profile = json.loads(PROFILE_PATH.read_text())
    pattern = json.loads(PATTERN_PATH.read_text())
    if profile.get("schema") != "iggy3d.material.structural_oak_joinery.v2":
        raise ValueError("unexpected joinery profile schema")
    if pattern.get("schema") != (
        "iggy3d.pattern.structural_oak_joinery_cut_atlas.v2"
    ):
        raise ValueError("unexpected joinery cut-atlas schema")
    timber_blend = _resolve(
        PROFILE_PATH, profile["source_dependencies"]["timber_blend"]
    )
    if not timber_blend.is_file():
        raise FileNotFoundError(f"built structural timber is absent: {timber_blend}")
    return profile, pattern, timber_blend


def _hex_to_linear(hex_color: str) -> tuple[float, float, float, float]:
    stripped = hex_color.lstrip("#")
    channels = [
        int(stripped[index : index + 2], 16) / 255.0
        for index in (0, 2, 4)
    ]

    def linear(value: float) -> float:
        if value <= 0.04045:
            return value / 12.92
        return ((value + 0.055) / 1.055) ** 2.4

    return (linear(channels[0]), linear(channels[1]), linear(channels[2]), 1.0)


def _clear_nodes(tree: bpy.types.NodeTree) -> None:
    for node in list(tree.nodes):
        tree.nodes.remove(node)


def _load_image(path: Path, *, non_color: bool) -> bpy.types.Image:
    if not path.is_file():
        raise FileNotFoundError(f"generate joinery atlas first: {path}")
    image = bpy.data.images.load(str(path.resolve()), check_existing=True)
    image.name = f"IGGY_IMG_{path.stem}"
    image.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    image.pack()
    return image


def create_cut_material(
    profile: dict[str, Any],
    output_root: Path,
) -> bpy.types.Material:
    material = bpy.data.materials.get(CUT_MATERIAL_NAME)
    if material is None:
        material = bpy.data.materials.new(CUT_MATERIAL_NAME)
    material.use_nodes = True
    material["iggy_material_role"] = "clean_joinery_cut_faces"
    material["iggy_surface_class_attribute"] = profile["coordinate_contract"][
        "surface_class_attribute"
    ]
    material["iggy_cut_uv_attribute"] = profile["coordinate_contract"][
        "cut_uv_attribute"
    ]
    material["iggy_damage"] = False
    material["iggy_roughness_map"] = False
    tree = material.node_tree
    _clear_nodes(tree)
    nodes = tree.nodes
    links = tree.links

    output = nodes.new("ShaderNodeOutputMaterial")
    output.name = "Material Output"
    output.location = (660, 80)
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    shader.name = "Clean Joinery Principled"
    shader.location = (420, 80)
    shader.inputs["Metallic"].default_value = 0.0
    shader.inputs["Roughness"].default_value = float(
        profile["representation"]["uniform_roughness"]
    )
    shader.inputs["IOR"].default_value = 1.5
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])

    uv = nodes.new("ShaderNodeUVMap")
    uv.name = "Explicit Joinery Cut UV"
    uv.uv_map = profile["coordinate_contract"]["cut_uv_attribute"]
    uv.location = (-520, 140)
    surface_class = nodes.new("ShaderNodeAttribute")
    surface_class.name = "Explicit Corner Surface Class"
    surface_class.attribute_name = profile["coordinate_contract"][
        "surface_class_attribute"
    ]
    surface_class.attribute_type = "GEOMETRY"
    surface_class.location = (-520, -160)

    base_image = _load_image(
        output_root / f"{PREFIX}_cut_basecolor.png",
        non_color=False,
    )
    normal_image = _load_image(
        output_root / f"{PREFIX}_cut_normal.png",
        non_color=True,
    )
    base = nodes.new("ShaderNodeTexImage")
    base.name = "Physical Cut Base Color Atlas"
    base.image = base_image
    base.interpolation = "Linear"
    base.extension = "CLIP"
    base.location = (-260, 220)
    normal = nodes.new("ShaderNodeTexImage")
    normal.name = "Physical Cut Normal Atlas"
    normal.image = normal_image
    normal.interpolation = "Linear"
    normal.extension = "CLIP"
    normal.location = (-260, -50)
    links.new(uv.outputs["UV"], base.inputs["Vector"])
    links.new(uv.outputs["UV"], normal.inputs["Vector"])
    links.new(base.outputs["Color"], shader.inputs["Base Color"])
    normal_map = nodes.new("ShaderNodeNormalMap")
    normal_map.name = "Measured Cut Face Normal"
    normal_map.space = "TANGENT"
    normal_map.inputs["Strength"].default_value = 1.0
    normal_map.location = (170, -70)
    links.new(normal.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], shader.inputs["Normal"])
    return material


def create_marker_material() -> bpy.types.Material:
    material = bpy.data.materials.get(MARKER_MATERIAL_NAME)
    if material is None:
        material = bpy.data.materials.new(MARKER_MATERIAL_NAME)
    material.use_nodes = True
    material.diffuse_color = (1.0, 0.0, 1.0, 1.0)
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = (1.0, 0.0, 1.0, 1.0)
    principled.inputs["Roughness"].default_value = 1.0
    return material


def create_peg_material(profile: dict[str, Any]) -> bpy.types.Material:
    material = bpy.data.materials.get(PEG_MATERIAL_NAME)
    if material is None:
        material = bpy.data.materials.new(PEG_MATERIAL_NAME)
    material.use_nodes = True
    material["iggy_material_role"] = "riven_octagonal_white_oak_peg"
    tree = material.node_tree
    _clear_nodes(tree)
    nodes = tree.nodes
    links = tree.links
    output = nodes.new("ShaderNodeOutputMaterial")
    output.location = (430, 0)
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    shader.name = "Riven Peg Principled"
    shader.location = (170, 0)
    shader.inputs["Base Color"].default_value = _hex_to_linear("#795c43")
    shader.inputs["Roughness"].default_value = float(
        profile["representation"]["uniform_roughness"]
    )
    geometry = nodes.new("ShaderNodeTexCoord")
    geometry.name = "Peg Object Coordinates"
    geometry.location = (-520, 0)
    separate = nodes.new("ShaderNodeSeparateXYZ")
    separate.location = (-330, 0)
    links.new(geometry.outputs["Generated"], separate.inputs["Vector"])
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.name = "Finite Riven Peg Colour Passages"
    ramp.location = (-80, 50)
    ramp.color_ramp.interpolation = "EASE"
    ramp.color_ramp.elements[0].position = 0.0
    ramp.color_ramp.elements[0].color = _hex_to_linear("#5c4b43")
    ramp.color_ramp.elements[1].position = 1.0
    ramp.color_ramp.elements[1].color = _hex_to_linear("#a28259")
    middle = ramp.color_ramp.elements.new(0.43)
    middle.color = _hex_to_linear("#87735b")
    links.new(separate.outputs["Y"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def _new_collection() -> bpy.types.Collection:
    previous = bpy.data.collections.get(COLLECTION_NAME)
    if previous is not None:
        for obj in list(previous.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.collections.remove(previous)
    collection = bpy.data.collections.new(COLLECTION_NAME)
    bpy.context.scene.collection.children.link(collection)
    return collection


def _move_to_collection(
    obj: bpy.types.Object,
    collection: bpy.types.Collection,
) -> None:
    for owner in list(obj.users_collection):
        owner.objects.unlink(obj)
    collection.objects.link(obj)


def _ensure_corner_int(
    mesh: bpy.types.Mesh,
    name: str,
) -> bpy.types.Attribute:
    attribute = mesh.attributes.get(name)
    if attribute is not None and (
        attribute.domain != "CORNER" or attribute.data_type != "INT"
    ):
        mesh.attributes.remove(attribute)
        attribute = None
    if attribute is None:
        attribute = mesh.attributes.new(name=name, type="INT", domain="CORNER")
    return attribute


def _ensure_cut_uv(mesh: bpy.types.Mesh, name: str) -> bpy.types.MeshUVLoopLayer:
    layer = mesh.uv_layers.get(name)
    if layer is None:
        layer = mesh.uv_layers.new(name=name)
    return layer


def _initialize_receiving_semantics(
    obj: bpy.types.Object,
    profile: dict[str, Any],
) -> None:
    mesh = obj.data
    class_name = profile["coordinate_contract"]["surface_class_attribute"]
    cut_uv_name = profile["coordinate_contract"]["cut_uv_attribute"]
    surface_class = _ensure_corner_int(mesh, class_name)
    _ensure_cut_uv(mesh, cut_uv_name)
    face_attribute = mesh.attributes.get("sinc_timber_face_id")
    for polygon in mesh.polygons:
        face_id = (
            int(face_attribute.data[polygon.index].value)
            if face_attribute is not None
            else 0
        )
        value = 1 if face_id in {4, 5} else 0
        for loop_index in polygon.loop_indices:
            surface_class.data[loop_index].value = value
    mesh.update()


def _tile(
    pattern: dict[str, Any],
    index: int,
) -> dict[str, Any]:
    return next(tile for tile in pattern["tiles"] if int(tile["index"]) == index)


def _atlas_uv(
    pattern: dict[str, Any],
    tile_index: int,
    s: float,
    t: float,
) -> tuple[float, float]:
    tile = _tile(pattern, tile_index)
    if tile["projection"] in {"Y_THETA", "TENON_THETA"}:
        t_range = tile["theta_range_rad"]
    else:
        t_range = tile["axis_v_range_m"]
    s_range = tile["axis_u_range_m"]
    normalized_s = max(
        0.002,
        min(
            0.998,
            (s - float(s_range[0]))
            / (float(s_range[1]) - float(s_range[0])),
        ),
    )
    normalized_t = max(
        0.002,
        min(
            0.998,
            (t - float(t_range[0]))
            / (float(t_range[1]) - float(t_range[0])),
        ),
    )
    columns = int(pattern["atlas_columns"])
    rows = int(pattern["atlas_rows"])
    column = tile_index % columns
    row = tile_index // columns
    u = (column + normalized_s) / columns
    v = 1.0 - (row + normalized_t) / rows
    return (u, v)


def _polygon_cut_projection(
    polygon: bpy.types.MeshPolygon,
    coordinate: Vector,
    *,
    cut_kind: str,
    profile: dict[str, Any],
    pattern: dict[str, Any],
) -> tuple[int, int, float, float]:
    normal = polygon.normal
    abs_normal = (abs(normal.x), abs(normal.y), abs(normal.z))
    if cut_kind == "housing":
        if abs_normal[2] >= max(abs_normal[0], abs_normal[1]):
            return (6, 2, coordinate.x, coordinate.y)
        if abs_normal[0] >= abs_normal[1]:
            return (5, 0, coordinate.y, coordinate.z)
        return (5, 1, coordinate.x, coordinate.z)
    if cut_kind == "mortise":
        if abs_normal[2] >= max(abs_normal[0], abs_normal[1]):
            return (4, 2, coordinate.x, coordinate.y)
        if abs_normal[0] >= abs_normal[1]:
            return (3, 0, coordinate.y, coordinate.z)
        return (3, 1, coordinate.x, coordinate.z)
    peg = profile["joint_fixture_m"]["peg"]
    if cut_kind == "receiving_bore":
        center_x = float(profile["joint_fixture_m"]["joint_center_x"])
        center_z = (
            float(profile["joint_fixture_m"]["receiving_timber"]["height"]) * 0.5
            - float(peg["mortise_hole_depth_from_reference_face"])
        )
        theta = math.atan2(coordinate.z - center_z, coordinate.x - center_x)
        return (7, 3, coordinate.y, theta)
    if cut_kind == "tenon_bore":
        fixture = profile["joint_fixture_m"]
        center_x = (
            float(fixture["entering_timber"]["length"])
            - float(fixture["tenon"]["length"])
            + float(peg["mortise_hole_depth_from_reference_face"])
            - float(peg["drawbore_offset"])
        )
        theta = math.atan2(coordinate.z, coordinate.x - center_x)
        return (7, 7, coordinate.y, theta)
    raise ValueError(f"unsupported Boolean cut kind {cut_kind!r}")


def _tag_transferred_faces(
    obj: bpy.types.Object,
    *,
    marker_index: int,
    cut_material_index: int,
    cut_kind: str,
    profile: dict[str, Any],
    pattern: dict[str, Any],
) -> dict[str, Any]:
    mesh = obj.data
    mesh.update()
    class_attribute = _ensure_corner_int(
        mesh, profile["coordinate_contract"]["surface_class_attribute"]
    )
    cut_uv = _ensure_cut_uv(
        mesh, profile["coordinate_contract"]["cut_uv_attribute"]
    )
    marker_faces = []
    class_counts: dict[int, int] = {}
    for polygon in mesh.polygons:
        if polygon.material_index != marker_index:
            continue
        marker_faces.append(polygon.index)
        for loop_index in polygon.loop_indices:
            coordinate = mesh.vertices[
                mesh.loops[loop_index].vertex_index
            ].co
            surface_class, tile_index, s, t = _polygon_cut_projection(
                polygon,
                coordinate,
                cut_kind=cut_kind,
                profile=profile,
                pattern=pattern,
            )
            class_attribute.data[loop_index].value = surface_class
            cut_uv.data[loop_index].uv = _atlas_uv(
                pattern, tile_index, float(s), float(t)
            )
            class_counts[surface_class] = class_counts.get(surface_class, 0) + 1
        polygon.material_index = cut_material_index
    if not marker_faces:
        raise RuntimeError(
            f"Exact Boolean transfer produced no marker faces for {cut_kind}"
        )
    mesh.update()
    return {
        "kind": cut_kind,
        "marker_face_count": len(marker_faces),
        "surface_corner_counts": {
            str(key): value for key, value in sorted(class_counts.items())
        },
    }


def _cube_cutter(
    collection: bpy.types.Collection,
    marker: bpy.types.Material,
    name: str,
    *,
    location: tuple[float, float, float],
    dimensions: tuple[float, float, float],
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(location=location)
    cutter = bpy.context.object
    cutter.name = name
    _move_to_collection(cutter, collection)
    cutter.dimensions = dimensions
    bpy.context.view_layer.objects.active = cutter
    cutter.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    cutter.data.materials.append(marker)
    return cutter


def _bore_cutter(
    collection: bpy.types.Collection,
    marker: bpy.types.Material,
    name: str,
    *,
    location: tuple[float, float, float],
    radius: float,
    depth: float,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=48,
        radius=radius,
        depth=depth,
        location=location,
        rotation=(math.radians(90.0), 0.0, 0.0),
    )
    cutter = bpy.context.object
    cutter.name = name
    _move_to_collection(cutter, collection)
    bpy.context.view_layer.objects.active = cutter
    cutter.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    cutter.data.materials.append(marker)
    return cutter


def _apply_exact_boolean(
    target: bpy.types.Object,
    cutter: bpy.types.Object,
    *,
    cut_kind: str,
    marker: bpy.types.Material,
    cut_material: bpy.types.Material,
    profile: dict[str, Any],
    pattern: dict[str, Any],
) -> dict[str, Any]:
    if marker.name not in [material.name for material in target.data.materials]:
        target.data.materials.append(marker)
    marker_index = next(
        index
        for index, material in enumerate(target.data.materials)
        if material is not None and material.name == marker.name
    )
    cut_material_index = next(
        index
        for index, material in enumerate(target.data.materials)
        if material is not None and material.name == cut_material.name
    )
    modifier = target.modifiers.new(f"IGGY_CUT_{cut_kind}", "BOOLEAN")
    modifier.operation = "DIFFERENCE"
    modifier.solver = "EXACT"
    modifier.operand_type = "OBJECT"
    modifier.object = cutter
    if hasattr(modifier, "material_mode"):
        modifier.material_mode = "TRANSFER"
    bpy.context.view_layer.objects.active = target
    target.select_set(True)
    cutter.select_set(False)
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    evidence = _tag_transferred_faces(
        target,
        marker_index=marker_index,
        cut_material_index=cut_material_index,
        cut_kind=cut_kind,
        profile=profile,
        pattern=pattern,
    )
    cutter_mesh = cutter.data
    bpy.data.objects.remove(cutter, do_unlink=True)
    if cutter_mesh.users == 0:
        bpy.data.meshes.remove(cutter_mesh)
    target.data.materials.pop(index=marker_index)
    return evidence


def create_receiving_timber(
    collection: bpy.types.Collection,
    timber_material: bpy.types.Material,
    cut_material: bpy.types.Material,
    marker: bpy.types.Material,
    profile: dict[str, Any],
    pattern: dict[str, Any],
) -> tuple[bpy.types.Object, list[dict[str, Any]]]:
    clean = bpy.data.objects[
        "IGGY_WA001_RoughHewnTimberBeam_CleanSource"
    ]
    receiving = clean.copy()
    receiving.data = clean.data.copy()
    receiving.name = RECEIVING_NAME
    collection.objects.link(receiving)
    receiving.hide_viewport = False
    receiving.hide_render = False
    receiving.location = (0.0, 0.0, 0.0)
    receiving.rotation_euler = (0.0, 0.0, 0.0)
    receiving.data.materials.clear()
    receiving.data.materials.append(timber_material)
    receiving.data.materials.append(cut_material)
    receiving["iggy_joinery_role"] = "receiving_timber"
    receiving["iggy_joint_center_x_m"] = float(
        profile["joint_fixture_m"]["joint_center_x"]
    )
    _initialize_receiving_semantics(receiving, profile)

    fixture = profile["joint_fixture_m"]
    top_z = float(fixture["receiving_timber"]["height"]) * 0.5
    center_x = float(fixture["joint_center_x"])
    housing = fixture["housing"]
    housing_cutter = _cube_cutter(
        collection,
        marker,
        "IGGY_CUT_FullyHousedSeat",
        location=(
            center_x,
            0.0,
            top_z - float(housing["depth_z"]) * 0.5 + 0.02,
        ),
        dimensions=(
            float(housing["length_x"]),
            float(housing["width_y"]),
            float(housing["depth_z"]) + 0.04,
        ),
    )
    evidence = [
        _apply_exact_boolean(
            receiving,
            housing_cutter,
            cut_kind="housing",
            marker=marker,
            cut_material=cut_material,
            profile=profile,
            pattern=pattern,
        )
    ]

    mortise = fixture["mortise"]
    mortise_cutter = _cube_cutter(
        collection,
        marker,
        "IGGY_CUT_BlindMortise",
        location=(
            center_x,
            0.0,
            top_z - float(mortise["depth_z"]) * 0.5 + 0.02,
        ),
        dimensions=(
            float(mortise["length_x"]),
            float(mortise["width_y"]),
            float(mortise["depth_z"]) + 0.04,
        ),
    )
    evidence.append(
        _apply_exact_boolean(
            receiving,
            mortise_cutter,
            cut_kind="mortise",
            marker=marker,
            cut_material=cut_material,
            profile=profile,
            pattern=pattern,
        )
    )

    peg = fixture["peg"]
    bore_center_z = (
        top_z - float(peg["mortise_hole_depth_from_reference_face"])
    )
    bore = _bore_cutter(
        collection,
        marker,
        "IGGY_CUT_ReceivingDrawbore",
        location=(center_x, 0.0, bore_center_z),
        radius=float(peg["bore_diameter"]) * 0.5,
        depth=float(fixture["receiving_timber"]["width"]) + 0.08,
    )
    evidence.append(
        _apply_exact_boolean(
            receiving,
            bore,
            cut_kind="receiving_bore",
            marker=marker,
            cut_material=cut_material,
            profile=profile,
            pattern=pattern,
        )
    )
    return receiving, evidence


def _create_tenon_mesh(
    profile: dict[str, Any],
    pattern: dict[str, Any],
    timber_material: bpy.types.Material,
    cut_material: bpy.types.Material,
) -> bpy.types.Mesh:
    fixture = profile["joint_fixture_m"]
    member = fixture["entering_timber"]
    tenon = fixture["tenon"]
    shoulder_x = float(member["length"]) - float(tenon["length"])
    end_x = float(member["length"])
    outer_y = float(member["width"]) * 0.5
    outer_z = float(member["height"]) * 0.5
    inner_y = float(tenon["width"]) * 0.5
    inner_z = float(tenon["height"]) * 0.5

    def ring(x: float, half_y: float, half_z: float):
        return [
            (x, -half_y, -half_z),
            (x, half_y, -half_z),
            (x, half_y, half_z),
            (x, -half_y, half_z),
        ]

    vertices = [
        *ring(0.0, outer_y, outer_z),
        *ring(shoulder_x, outer_y, outer_z),
        *ring(shoulder_x, inner_y, inner_z),
        *ring(end_x, inner_y, inner_z),
    ]
    faces: list[tuple[int, ...]] = []
    roles: list[tuple[int, int, int]] = []

    def add(face: tuple[int, ...], material: int, face_id: int, surface: int):
        faces.append(face)
        roles.append((material, face_id, surface))

    add((0, 3, 2, 1), 0, 4, 1)
    for face, face_id in (
        ((0, 1, 5, 4), 3),
        ((1, 2, 6, 5), 1),
        ((2, 3, 7, 6), 2),
        ((3, 0, 4, 7), 0),
    ):
        add(face, 0, face_id, 0)
    for face in (
        (4, 5, 9, 8),
        (5, 6, 10, 9),
        (6, 7, 11, 10),
        (7, 4, 8, 11),
    ):
        add(face, 1, 5, 9)
    for face in (
        (8, 9, 13, 12),
        (9, 10, 14, 13),
        (10, 11, 15, 14),
        (11, 8, 12, 15),
    ):
        add(face, 1, 0, 8)
    add((12, 13, 14, 15), 1, 5, 2)

    mesh = bpy.data.meshes.new("IGGY_Joinery_EnteringTenonMesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    mesh.materials.append(timber_material)
    mesh.materials.append(cut_material)
    face_attribute = mesh.attributes.new(
        "sinc_timber_face_id", type="INT", domain="FACE"
    )
    surface_attribute = mesh.attributes.new(
        profile["coordinate_contract"]["surface_class_attribute"],
        type="INT",
        domain="CORNER",
    )
    timber_uv = mesh.uv_layers.new(name="IGGY_TimberUV")
    cut_uv = mesh.uv_layers.new(
        name=profile["coordinate_contract"]["cut_uv_attribute"]
    )
    u_attribute = mesh.attributes.new(
        "sinc_timber_u_m", type="FLOAT", domain="POINT"
    )
    end_distance = mesh.attributes.new(
        "sinc_end_distance_m", type="FLOAT", domain="POINT"
    )
    for index, vertex in enumerate(mesh.vertices):
        u_attribute.data[index].value = float(vertex.co.x)
        end_distance.data[index].value = min(
            float(vertex.co.x), end_x - float(vertex.co.x)
        )

    for polygon, (material_index, face_id, surface_class) in zip(
        mesh.polygons, roles, strict=True
    ):
        polygon.material_index = material_index
        face_attribute.data[polygon.index].value = face_id
        for loop_index in polygon.loop_indices:
            coordinate = mesh.vertices[
                mesh.loops[loop_index].vertex_index
            ].co
            surface_attribute.data[loop_index].value = surface_class
            if face_id in {0, 1}:
                timber_uv.data[loop_index].uv = (
                    coordinate.x,
                    coordinate.z + 0.5,
                )
            elif face_id in {2, 3}:
                timber_uv.data[loop_index].uv = (
                    coordinate.x,
                    coordinate.y + 0.5,
                )
            else:
                timber_uv.data[loop_index].uv = (
                    coordinate.y + 0.5,
                    coordinate.z + 0.5,
                )
            if surface_class in {2, 9}:
                cut_uv.data[loop_index].uv = _atlas_uv(
                    pattern, 4, coordinate.y, coordinate.z
                )
            elif surface_class == 8:
                if abs(polygon.normal.y) >= abs(polygon.normal.z):
                    cut_uv.data[loop_index].uv = _atlas_uv(
                        pattern, 5, coordinate.x, coordinate.z
                    )
                else:
                    cut_uv.data[loop_index].uv = _atlas_uv(
                        pattern, 6, coordinate.x, coordinate.y
                    )
    return mesh


def create_entering_tenon(
    collection: bpy.types.Collection,
    timber_material: bpy.types.Material,
    cut_material: bpy.types.Material,
    marker: bpy.types.Material,
    profile: dict[str, Any],
    pattern: dict[str, Any],
) -> tuple[bpy.types.Object, dict[str, Any]]:
    mesh = _create_tenon_mesh(profile, pattern, timber_material, cut_material)
    entering = bpy.data.objects.new(ENTERING_NAME, mesh)
    collection.objects.link(entering)
    entering["iggy_joinery_role"] = "entering_tenon"
    fixture = profile["joint_fixture_m"]
    peg = fixture["peg"]
    shoulder = (
        float(fixture["entering_timber"]["length"])
        - float(fixture["tenon"]["length"])
    )
    bore_x = (
        shoulder
        + float(peg["mortise_hole_depth_from_reference_face"])
        - float(peg["drawbore_offset"])
    )
    bore = _bore_cutter(
        collection,
        marker,
        "IGGY_CUT_TenonDrawbore",
        location=(bore_x, 0.0, 0.0),
        radius=float(peg["bore_diameter"]) * 0.5,
        depth=float(fixture["entering_timber"]["width"]) + 0.08,
    )
    evidence = _apply_exact_boolean(
        entering,
        bore,
        cut_kind="tenon_bore",
        marker=marker,
        cut_material=cut_material,
        profile=profile,
        pattern=pattern,
    )
    entering.rotation_euler = (0.0, math.radians(90.0), 0.0)
    entering.location = (
        float(fixture["joint_center_x"]),
        -0.50,
        1.56,
    )
    return entering, evidence


def create_octagonal_peg(
    collection: bpy.types.Collection,
    material: bpy.types.Material,
    profile: dict[str, Any],
) -> bpy.types.Object:
    fixture = profile["joint_fixture_m"]
    peg = fixture["peg"]
    half_length = float(peg["length"]) * 0.5
    taper_start = half_length - float(peg["taper_length"])
    radii = (
        float(peg["diameter"]) * 0.5,
        float(peg["diameter"]) * 0.5,
        float(peg["tip_diameter"]) * 0.5,
    )
    y_positions = (-half_length, taper_start, half_length)
    sides = int(peg["sides"])
    vertices = []
    for y, radius in zip(y_positions, radii, strict=True):
        for index in range(sides):
            angle = math.tau * index / sides + math.pi / 8.0
            vertices.append(
                (math.cos(angle) * radius, y, math.sin(angle) * radius)
            )
    faces = []
    surface_roles = []
    for ring_index in range(2):
        start = ring_index * sides
        end = (ring_index + 1) * sides
        for index in range(sides):
            faces.append(
                (
                    start + index,
                    start + (index + 1) % sides,
                    end + (index + 1) % sides,
                    end + index,
                )
            )
            surface_roles.append(10)
    faces.append(tuple(reversed(range(sides))))
    surface_roles.append(11)
    faces.append(tuple(2 * sides + index for index in range(sides)))
    surface_roles.append(11)
    mesh = bpy.data.meshes.new("IGGY_Joinery_OctagonalDrawPegMesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.materials.append(material)
    mesh.update()
    surface = mesh.attributes.new(
        profile["coordinate_contract"]["surface_class_attribute"],
        type="INT",
        domain="CORNER",
    )
    for polygon, role in zip(mesh.polygons, surface_roles, strict=True):
        for loop_index in polygon.loop_indices:
            surface.data[loop_index].value = role
    obj = bpy.data.objects.new(PEG_NAME, mesh)
    collection.objects.link(obj)
    obj.location = (0.62, -0.50, 0.76)
    obj.rotation_euler = (
        math.radians(78.0),
        math.radians(8.0),
        math.radians(-12.0),
    )
    obj["iggy_joinery_role"] = "drawbore_peg"
    obj["iggy_drawbore_offset_m"] = float(peg["drawbore_offset"])
    return obj


def _manifold_payload(obj: bpy.types.Object) -> dict[str, Any]:
    mesh = obj.data
    topology = bmesh.new()
    topology.from_mesh(mesh)
    payload = {
        "vertices": len(mesh.vertices),
        "polygons": len(mesh.polygons),
        "manifold": all(edge.is_manifold for edge in topology.edges),
    }
    topology.free()
    return payload


def _aim(obj: bpy.types.Object, target: tuple[float, float, float]) -> None:
    obj.rotation_euler = (
        Vector(target) - obj.location
    ).to_track_quat("-Z", "Y").to_euler()


def _configure_stage(width: int, height: int) -> dict[str, bpy.types.Object]:
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.look = "AgX - Medium High Contrast"
    camera = bpy.data.objects["IGGY_WA001_ReviewCamera"]
    scene.camera = camera
    ground = bpy.data.objects["IGGY_WA001_ReviewGround"]
    ground.hide_render = False
    key = bpy.data.objects["IGGY_WA001_Key"]
    fill = bpy.data.objects["IGGY_WA001_Fill"]
    rim = bpy.data.objects["IGGY_WA001_Rim"]
    for light in (key, fill, rim):
        light.hide_render = False
    key.location = (-2.4, -3.6, 4.5)
    key.data.energy = 760.0
    key.data.size = 3.1
    key.data.color = (1.0, 0.94, 0.86)
    _aim(key, (0.1, -0.15, 0.45))
    fill.location = (3.1, 2.8, 2.6)
    fill.data.energy = 280.0
    fill.data.size = 2.8
    fill.data.color = (0.82, 0.90, 1.0)
    _aim(fill, (0.1, -0.1, 0.35))
    rim.location = (-3.2, 1.8, 3.4)
    rim.data.energy = 390.0
    rim.data.size = 2.2
    rim.data.color = (0.90, 0.94, 1.0)
    _aim(rim, (0.0, 0.0, 0.45))
    return {
        "camera": camera,
        "ground": ground,
        "key": key,
        "fill": fill,
        "rim": rim,
    }


def _set_camera(
    camera: bpy.types.Object,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    lens: float,
) -> None:
    camera.data.type = "PERSP"
    camera.data.lens = lens
    camera.location = location
    _aim(camera, target)


def _render(path: Path) -> dict[str, Any]:
    scene = bpy.context.scene
    scene.render.filepath = str(path.resolve())
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path.resolve()),
        "width": scene.render.resolution_x,
        "height": scene.render.resolution_y,
        "file_size_bytes": path.stat().st_size,
    }


def render_proofs(
    output_root: Path,
    stage: dict[str, bpy.types.Object],
    receiving: bpy.types.Object,
    entering: bpy.types.Object,
    peg: bpy.types.Object,
) -> dict[str, Any]:
    render_root = output_root / "renders"
    render_root.mkdir(parents=True, exist_ok=True)
    camera = stage["camera"]
    ground = stage["ground"]
    proofs = {}
    _set_camera(camera, (3.9, -4.9, 3.2), (0.1, -0.1, 0.55), 62.0)
    proofs["assembly_exploded"] = _render(
        render_root / f"{PREFIX}_assembly_exploded.png"
    )

    entering.hide_render = True
    peg.hide_render = True
    ground.hide_render = False
    _set_camera(camera, (1.42, -2.15, 1.22), (0.15, 0.0, 0.10), 72.0)
    proofs["mortise_housing_bore"] = _render(
        render_root / f"{PREFIX}_mortise_housing_bore.png"
    )
    entering.hide_render = False
    peg.hide_render = False

    receiving.hide_render = True
    peg.hide_render = True
    ground.hide_render = True
    _set_camera(camera, (0.76, -1.48, 0.74), (0.15, -0.50, 0.49), 68.0)
    proofs["tenon_shoulder_bore"] = _render(
        render_root / f"{PREFIX}_tenon_shoulder_bore.png"
    )
    receiving.hide_render = False
    peg.hide_render = False
    ground.hide_render = False

    _set_camera(camera, (-0.9, -2.4, 0.92), (0.15, -0.08, 0.08), 84.0)
    proofs["cutface_closeup"] = _render(
        render_root / f"{PREFIX}_cutface_closeup.png"
    )
    return proofs


def _collect_surface_classes(
    obj: bpy.types.Object,
    profile: dict[str, Any],
) -> list[int]:
    attribute = obj.data.attributes[
        profile["coordinate_contract"]["surface_class_attribute"]
    ]
    return sorted({int(value.value) for value in attribute.data})


def main() -> None:
    args = parse_args()
    output_root = args.output_root.resolve()
    profile, pattern, timber_blend = _load_contract()
    bpy.ops.wm.open_mainfile(filepath=str(timber_blend))
    timber_material = bpy.data.materials[TIMBER_MATERIAL_NAME]
    cut_material = create_cut_material(profile, output_root)
    marker = create_marker_material()
    peg_material = create_peg_material(profile)

    for obj in bpy.data.objects:
        if obj.type == "MESH" and obj.name not in {"IGGY_WA001_ReviewGround"}:
            obj.hide_render = True
    collection = _new_collection()
    receiving, receiving_booleans = create_receiving_timber(
        collection,
        timber_material,
        cut_material,
        marker,
        profile,
        pattern,
    )
    entering, tenon_boolean = create_entering_tenon(
        collection,
        timber_material,
        cut_material,
        marker,
        profile,
        pattern,
    )
    peg = create_octagonal_peg(collection, peg_material, profile)
    retained_meshes = {
        receiving.name,
        entering.name,
        peg.name,
        "IGGY_WA001_ReviewGround",
    }
    for obj in list(bpy.data.objects):
        if obj.type != "MESH" or obj.name in retained_meshes:
            continue
        inherited_mesh = obj.data
        bpy.data.objects.remove(obj, do_unlink=True)
        if inherited_mesh.users == 0:
            bpy.data.meshes.remove(inherited_mesh)
    if marker.users != 0:
        raise RuntimeError(
            "temporary Boolean transfer marker still has datablock users "
            f"after cutter cleanup: {marker.users}"
        )
    bpy.data.materials.remove(marker)

    stage = _configure_stage(args.render_width, args.render_height)
    proofs = (
        {}
        if args.skip_renders
        else render_proofs(
            output_root,
            stage,
            receiving,
            entering,
            peg,
        )
    )
    geometry = {
        receiving.name: _manifold_payload(receiving),
        entering.name: _manifold_payload(entering),
        peg.name: _manifold_payload(peg),
    }
    if not all(item["manifold"] for item in geometry.values()):
        raise RuntimeError(f"joinery proof contains nonmanifold geometry: {geometry}")
    surface_classes = {
        receiving.name: _collect_surface_classes(receiving, profile),
        entering.name: _collect_surface_classes(entering, profile),
        peg.name: _collect_surface_classes(peg, profile),
    }
    required = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}
    present = set().union(*[set(values) for values in surface_classes.values()])
    absent = sorted(required - present)
    if absent:
        raise RuntimeError(f"semantic cut-face classes are absent: {absent}")

    blend_path = output_root / f"{PREFIX}.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    fixture = profile["joint_fixture_m"]
    manifest = {
        "schema": "iggy3d.material.structural_oak_joinery.blender.v2",
        "profile_id": profile["profile_id"],
        "blend": str(blend_path.resolve()),
        "source_timber_blend": str(timber_blend),
        "objects": {
            "receiving": receiving.name,
            "entering": entering.name,
            "peg": peg.name,
        },
        "materials": {
            "outer": TIMBER_MATERIAL_NAME,
            "cut": CUT_MATERIAL_NAME,
            "peg": PEG_MATERIAL_NAME,
        },
        "measurements_m": fixture,
        "boolean_contract": {
            "solver": "EXACT",
            "material_mode": "TRANSFER",
            "cuts": [*receiving_booleans, tenon_boolean],
            "marker_material_present_after_build": (
                MARKER_MATERIAL_NAME in bpy.data.materials
            ),
            "surface_class_domain": "CORNER",
            "cut_uv_domain": "CORNER",
        },
        "surface_classes": surface_classes,
        "geometry": geometry,
        "surface_response": {
            "uniform_roughness": profile["representation"][
                "uniform_roughness"
            ],
            "roughness_map": False,
            "damage": False,
        },
        "proofs": proofs,
    }
    manifest_path = output_root / f"{PREFIX}_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(
        json.dumps(
            {
                "blend": str(blend_path),
                "surface_classes": sorted(present),
                "proof_count": len(proofs),
                "boolean_cut_count": len(receiving_booleans) + 1,
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
