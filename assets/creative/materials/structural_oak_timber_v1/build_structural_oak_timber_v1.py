#!/usr/bin/env python3
"""Apply the structural-oak growth atlas to the approved rough-hewn beam."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import sys
from typing import Any

import bpy
from mathutils import Vector


SCRIPT_ROOT = Path(__file__).resolve().parent
BEAM_ROOT = (
    SCRIPT_ROOT.parent.parent
    / "architecture"
    / "structural"
    / "rough_hewn_timber_beam_v1"
)
BEAM_BLEND = BEAM_ROOT / "output" / "rough_hewn_timber_beam_v1.blend"
OUTPUT_ROOT = SCRIPT_ROOT / "output"
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "structural_oak_timber_v1.json"
MASTER_NAME = "IGGY_WA001_RoughHewnTimberBeam_Master"
MATERIAL_NAME = "IGGY_MAT_StructuralOakTimberV1"
KNOT_MATERIAL_NAME = "IGGY_MAT_StructuralOakKnotV1"
PREFIX = "structural_oak_timber_v1"


def parse_args() -> argparse.Namespace:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-blend", type=Path, default=BEAM_BLEND)
    parser.add_argument("--output-root", type=Path, default=OUTPUT_ROOT)
    parser.add_argument("--render-width", type=int, default=1440)
    parser.add_argument("--render-height", type=int, default=810)
    parser.add_argument("--skip-renders", action="store_true")
    return parser.parse_args(arguments)


def _clear_nodes(tree: bpy.types.NodeTree) -> None:
    for node in list(tree.nodes):
        tree.nodes.remove(node)


def _math(
    tree: bpy.types.NodeTree,
    operation: str,
    name: str,
    x: float,
    y: float,
) -> bpy.types.Node:
    node = tree.nodes.new("ShaderNodeMath")
    node.name = name
    node.label = name
    node.operation = operation
    node.inputs[0].default_value = x
    node.inputs[1].default_value = y
    return node


def _load_image(path: Path, *, non_color: bool) -> bpy.types.Image:
    image = bpy.data.images.load(str(path.resolve()), check_existing=True)
    image.name = f"IGGY_IMG_{path.stem}"
    image.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    image.pack()
    return image


def _srgb_channel_to_linear(value: float) -> float:
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def _hex_to_linear_rgba(hex_color: str) -> tuple[float, float, float, float]:
    stripped = hex_color.lstrip("#")
    srgb = tuple(
        int(stripped[index : index + 2], 16) / 255.0
        for index in (0, 2, 4)
    )
    return (*(_srgb_channel_to_linear(value) for value in srgb), 1.0)


def _image_node(
    tree: bpy.types.NodeTree,
    image: bpy.types.Image,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = tree.nodes.new("ShaderNodeTexImage")
    node.name = name
    node.label = name
    node.image = image
    node.interpolation = "Linear"
    node.extension = "CLIP"
    node.location = location
    return node


def create_timber_material(
    profile: dict[str, Any],
    output_root: Path,
) -> bpy.types.Material:
    material = bpy.data.materials.get(MATERIAL_NAME)
    if material is None:
        material = bpy.data.materials.new(MATERIAL_NAME)
    material.use_nodes = True
    material.diffuse_color = (0.44, 0.32, 0.21, 1.0)
    material["iggy_material_profile"] = profile["profile_id"]
    material["iggy_coordinate_contract"] = "shared_growth_volume"
    material["iggy_roughness_lane"] = "uniform_constant_only"
    material["iggy_damage_lane"] = "excluded"
    tree = material.node_tree
    _clear_nodes(tree)
    nodes = tree.nodes
    links = tree.links

    output = nodes.new("ShaderNodeOutputMaterial")
    output.name = "Material Output"
    output.location = (1160, 80)
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    shader.name = "Structural Oak Principled"
    shader.location = (900, 80)
    shader.inputs["Metallic"].default_value = 0.0
    shader.inputs["Roughness"].default_value = float(
        profile["surface_response"]["uniform_roughness"]
    )
    shader.inputs["IOR"].default_value = float(profile["surface_response"]["ior"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])

    uv = nodes.new("ShaderNodeUVMap")
    uv.name = "Timber Metre UV"
    uv.label = "IGGY_TimberUV / metres"
    uv.uv_map = profile["coordinate_contract"]["uv_attribute"]
    uv.location = (-1220, 260)
    separate_uv = nodes.new("ShaderNodeSeparateXYZ")
    separate_uv.name = "Separate Timber UV"
    separate_uv.location = (-1040, 260)
    links.new(uv.outputs["UV"], separate_uv.inputs["Vector"])

    face = nodes.new("ShaderNodeAttribute")
    face.name = "Timber Face Identity"
    face.label = profile["coordinate_contract"]["face_attribute"]
    face.attribute_name = profile["coordinate_contract"]["face_attribute"]
    face.attribute_type = "GEOMETRY"
    face.location = (-1220, -80)

    # Side U = longitudinal metres / 4.2.
    side_u = _math(tree, "DIVIDE", "Side U / 4.2 m", 0.0, 4.2)
    side_u.location = (-850, 380)
    links.new(separate_uv.outputs["X"], side_u.inputs[0])

    # Cross-normalized coordinate: the beam source stores side cross as
    # (physical cross + 0.5 m), while the atlas owns [-0.2, +0.2] m.
    side_cross_offset = _math(
        tree, "SUBTRACT", "Side Cross - 0.3 m", 0.0, 0.3
    )
    side_cross_offset.location = (-850, 250)
    links.new(separate_uv.outputs["Y"], side_cross_offset.inputs[0])
    side_cross = _math(tree, "DIVIDE", "Side Cross / 0.4 m", 0.0, 0.4)
    side_cross.location = (-670, 250)
    links.new(side_cross_offset.outputs[0], side_cross.inputs[0])

    # PNG row zero is the top row.  The atlas is front/back/top/bottom from
    # top to bottom, so Blender's bottom-origin V must reverse both row and
    # cross direction: (4 - face_id - cross_normalized) / 4.
    side_row = _math(tree, "SUBTRACT", "Side 4 - Face", 4.0, 0.0)
    side_row.location = (-670, 90)
    links.new(face.outputs["Fac"], side_row.inputs[1])
    side_v_pre = _math(tree, "SUBTRACT", "Side Row - Cross", 0.0, 0.0)
    side_v_pre.location = (-490, 160)
    links.new(side_row.outputs[0], side_v_pre.inputs[0])
    links.new(side_cross.outputs[0], side_v_pre.inputs[1])
    side_v = _math(tree, "DIVIDE", "Side V / 4 Faces", 0.0, 4.0)
    side_v.location = (-310, 160)
    links.new(side_v_pre.outputs[0], side_v.inputs[0])
    side_vector = nodes.new("ShaderNodeCombineXYZ")
    side_vector.name = "Side Atlas Coordinate"
    side_vector.location = (-120, 310)
    links.new(side_u.outputs[0], side_vector.inputs["X"])
    links.new(side_v.outputs[0], side_vector.inputs["Y"])

    # End U packs left/right into two columns. End V reverses PNG row order.
    end_y_offset = _math(tree, "SUBTRACT", "End Y - 0.3 m", 0.0, 0.3)
    end_y_offset.location = (-850, -180)
    links.new(separate_uv.outputs["X"], end_y_offset.inputs[0])
    end_y_norm = _math(tree, "DIVIDE", "End Y / 0.4 m", 0.0, 0.4)
    end_y_norm.location = (-670, -180)
    links.new(end_y_offset.outputs[0], end_y_norm.inputs[0])
    end_column = _math(tree, "SUBTRACT", "End Face - 4", 0.0, 4.0)
    end_column.location = (-850, -320)
    links.new(face.outputs["Fac"], end_column.inputs[0])
    end_u_sum = _math(tree, "ADD", "End Column + Y", 0.0, 0.0)
    end_u_sum.location = (-490, -240)
    links.new(end_column.outputs[0], end_u_sum.inputs[0])
    links.new(end_y_norm.outputs[0], end_u_sum.inputs[1])
    end_u = _math(tree, "DIVIDE", "End U / 2 Faces", 0.0, 2.0)
    end_u.location = (-310, -240)
    links.new(end_u_sum.outputs[0], end_u.inputs[0])

    end_z_offset = _math(tree, "SUBTRACT", "End Z - 0.3 m", 0.0, 0.3)
    end_z_offset.location = (-850, -470)
    links.new(separate_uv.outputs["Y"], end_z_offset.inputs[0])
    end_z_norm = _math(tree, "DIVIDE", "End Z / 0.4 m", 0.0, 0.4)
    end_z_norm.location = (-670, -470)
    links.new(end_z_offset.outputs[0], end_z_norm.inputs[0])
    end_v = _math(tree, "SUBTRACT", "End V Reverse", 1.0, 0.0)
    end_v.location = (-490, -470)
    links.new(end_z_norm.outputs[0], end_v.inputs[1])
    end_vector = nodes.new("ShaderNodeCombineXYZ")
    end_vector.name = "End Atlas Coordinate"
    end_vector.location = (-120, -300)
    links.new(end_u.outputs[0], end_vector.inputs["X"])
    links.new(end_v.outputs[0], end_vector.inputs["Y"])

    side_base = _load_image(
        output_root / f"{PREFIX}_side_basecolor.png", non_color=False
    )
    end_base = _load_image(
        output_root / f"{PREFIX}_end_basecolor.png", non_color=False
    )
    side_normal = _load_image(
        output_root / f"{PREFIX}_side_normal.png", non_color=True
    )
    end_normal = _load_image(
        output_root / f"{PREFIX}_end_normal.png", non_color=True
    )
    side_base_node = _image_node(
        tree, side_base, "Side Growth Base Color", (120, 430)
    )
    end_base_node = _image_node(
        tree, end_base, "End Growth Base Color", (120, 170)
    )
    side_normal_node = _image_node(
        tree, side_normal, "Side Growth Normal", (120, -120)
    )
    end_normal_node = _image_node(
        tree, end_normal, "End Growth Normal", (120, -390)
    )
    for node in (side_base_node, side_normal_node):
        links.new(side_vector.outputs["Vector"], node.inputs["Vector"])
    for node in (end_base_node, end_normal_node):
        links.new(end_vector.outputs["Vector"], node.inputs["Vector"])

    end_min = _math(tree, "GREATER_THAN", "Face Is End Minimum", 0.0, 3.5)
    end_min.location = (-100, -600)
    links.new(face.outputs["Fac"], end_min.inputs[0])
    end_max = _math(tree, "LESS_THAN", "Face Is End Maximum", 0.0, 5.5)
    end_max.location = (-100, -690)
    links.new(face.outputs["Fac"], end_max.inputs[0])
    is_end = _math(tree, "MULTIPLY", "End Face Mask", 0.0, 0.0)
    is_end.location = (100, -620)
    links.new(end_min.outputs[0], is_end.inputs[0])
    links.new(end_max.outputs[0], is_end.inputs[1])

    color_mix = nodes.new("ShaderNodeMixRGB")
    color_mix.name = "Side End Color Continuity"
    color_mix.blend_type = "MIX"
    color_mix.location = (440, 300)
    links.new(is_end.outputs[0], color_mix.inputs["Fac"])
    links.new(side_base_node.outputs["Color"], color_mix.inputs[1])
    links.new(end_base_node.outputs["Color"], color_mix.inputs[2])

    normal_mix = nodes.new("ShaderNodeMixRGB")
    normal_mix.name = "Side End Normal Continuity"
    normal_mix.blend_type = "MIX"
    normal_mix.location = (440, -130)
    links.new(is_end.outputs[0], normal_mix.inputs["Fac"])
    links.new(side_normal_node.outputs["Color"], normal_mix.inputs[1])
    links.new(end_normal_node.outputs["Color"], normal_mix.inputs[2])

    # The source mesh owns a fifth side class for the four narrow hewn
    # arrises.  It is intentionally not stretched through one of the planar
    # face rows.  Give it a quiet accepted-palette color and a flat normal;
    # this keeps the chamfer as wood instead of an out-of-bounds black rail.
    is_chamfer = _math(
        tree, "GREATER_THAN", "Chamfer Face Mask", 0.0, 5.5
    )
    is_chamfer.location = (280, -660)
    links.new(face.outputs["Fac"], is_chamfer.inputs[0])
    chamfer_color = nodes.new("ShaderNodeRGB")
    chamfer_color.name = "Quiet Chamfer Wood"
    chamfer_color.outputs["Color"].default_value = _hex_to_linear_rgba(
        "#6c5648"
    )
    chamfer_color.location = (450, 110)
    chamfer_color_mix = nodes.new("ShaderNodeMixRGB")
    chamfer_color_mix.name = "Explicit Chamfer Color"
    chamfer_color_mix.blend_type = "MIX"
    chamfer_color_mix.location = (670, 280)
    links.new(is_chamfer.outputs[0], chamfer_color_mix.inputs["Fac"])
    links.new(color_mix.outputs["Color"], chamfer_color_mix.inputs[1])
    links.new(chamfer_color.outputs["Color"], chamfer_color_mix.inputs[2])
    links.new(chamfer_color_mix.outputs["Color"], shader.inputs["Base Color"])

    flat_normal = nodes.new("ShaderNodeRGB")
    flat_normal.name = "Flat Chamfer Tangent Normal"
    flat_normal.outputs["Color"].default_value = (0.5, 0.5, 1.0, 1.0)
    flat_normal.location = (450, -320)
    chamfer_normal_mix = nodes.new("ShaderNodeMixRGB")
    chamfer_normal_mix.name = "Explicit Chamfer Normal"
    chamfer_normal_mix.blend_type = "MIX"
    chamfer_normal_mix.location = (660, -130)
    links.new(is_chamfer.outputs[0], chamfer_normal_mix.inputs["Fac"])
    links.new(normal_mix.outputs["Color"], chamfer_normal_mix.inputs[1])
    links.new(flat_normal.outputs["Color"], chamfer_normal_mix.inputs[2])
    normal_map = nodes.new("ShaderNodeNormalMap")
    normal_map.name = "Measured Timber Normal"
    normal_map.space = "TANGENT"
    normal_map.inputs["Strength"].default_value = 1.0
    normal_map.location = (820, -100)
    links.new(chamfer_normal_mix.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], shader.inputs["Normal"])
    return material


def create_knot_material(
    profile: dict[str, Any],
    door_profile: dict[str, Any],
) -> bpy.types.Material:
    material = bpy.data.materials.get(KNOT_MATERIAL_NAME)
    if material is None:
        material = bpy.data.materials.new(KNOT_MATERIAL_NAME)
    material.use_nodes = True
    material["iggy_material_role"] = "recessed_knot_body"
    material["iggy_damage_lane"] = "excluded"
    principled = material.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = _hex_to_linear_rgba(
        door_profile["palette"]["observed_reference_twenty"][0]
    )
    principled.inputs["Roughness"].default_value = float(
        profile["surface_response"]["uniform_roughness"]
    )
    return material


def _aim(obj: bpy.types.Object, target: tuple[float, float, float]) -> None:
    obj.rotation_euler = (
        Vector(target) - obj.location
    ).to_track_quat("-Z", "Y").to_euler()


def _set_camera(
    camera: bpy.types.Object,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    *,
    lens: float,
) -> None:
    camera.data.type = "PERSP"
    camera.data.lens = lens
    camera.location = location
    _aim(camera, target)


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
    key.location = (-1.45, -3.7, 4.1)
    key.data.energy = 650.0
    key.data.size = 3.4
    key.data.color = (1.0, 0.95, 0.88)
    _aim(key, (0.0, 0.0, 0.0))
    fill.location = (2.8, 3.1, 2.0)
    fill.data.energy = 250.0
    fill.data.size = 3.0
    fill.data.color = (0.82, 0.90, 1.0)
    _aim(fill, (0.0, 0.0, 0.0))
    rim.location = (-3.0, 1.6, 2.8)
    rim.data.energy = 330.0
    rim.data.size = 2.2
    rim.data.color = (0.90, 0.94, 1.0)
    _aim(rim, (0.0, 0.0, 0.0))
    return {
        "camera": camera,
        "ground": ground,
        "key": key,
        "fill": fill,
        "rim": rim,
    }


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
) -> dict[str, Any]:
    render_root = output_root / "renders"
    render_root.mkdir(parents=True, exist_ok=True)
    camera = stage["camera"]
    ground = stage["ground"]
    key = stage["key"]
    fill = stage["fill"]
    rim = stage["rim"]
    proofs: dict[str, Any] = {}

    ground.hide_render = False
    _set_camera(camera, (4.05, -4.25, 2.55), (0.1, 0.0, 0.0), lens=62.0)
    proofs["hero"] = _render(render_root / f"{PREFIX}_beam_hero.png")

    _set_camera(camera, (-3.85, 3.65, 2.15), (-0.08, 0.0, 0.0), lens=66.0)
    proofs["reverse"] = _render(render_root / f"{PREFIX}_beam_reverse.png")

    ground.hide_render = True
    _set_camera(camera, (-3.02, -1.33, 0.55), (-2.04, 0.0, 0.0), lens=78.0)
    key.location = (-3.2, -1.8, 2.0)
    key.data.energy = 520.0
    key.data.size = 1.7
    _aim(key, (-2.04, 0.0, 0.0))
    fill.data.energy = 150.0
    rim.data.energy = 220.0
    proofs["end_growth"] = _render(
        render_root / f"{PREFIX}_beam_end_growth.png"
    )

    ground.hide_render = False
    key.location = (-3.45, -1.02, 0.38)
    key.data.energy = 450.0
    key.data.size = 0.55
    _aim(key, (0.2, -0.14, -0.01))
    fill.data.energy = 120.0
    rim.data.energy = 210.0
    _set_camera(camera, (0.3, -3.55, 0.82), (0.18, -0.14, -0.01), lens=59.0)
    proofs["grazing"] = _render(
        render_root / f"{PREFIX}_beam_grazing.png"
    )
    return proofs


def main() -> None:
    args = parse_args()
    input_blend = args.input_blend.resolve()
    output_root = args.output_root.resolve()
    if not input_blend.exists():
        raise FileNotFoundError(f"approved beam blend is absent: {input_blend}")
    required_maps = [
        output_root / f"{PREFIX}_{lane}.png"
        for lane in (
            "side_basecolor",
            "side_normal",
            "end_basecolor",
            "end_normal",
        )
    ]
    absent = [str(path) for path in required_maps if not path.exists()]
    if absent:
        raise FileNotFoundError(f"generate material maps first: {absent}")

    profile = json.loads(PROFILE_PATH.read_text())
    door_profile_path = (
        PROFILE_PATH.parent.parent
        / profile["source_dependencies"]["door_profile"]
    ).resolve()
    door_profile = json.loads(door_profile_path.read_text())
    bpy.ops.wm.open_mainfile(filepath=str(input_blend))
    master = bpy.data.objects[MASTER_NAME]
    timber = create_timber_material(profile, output_root)
    knot = create_knot_material(profile, door_profile)
    master.data.materials.clear()
    master.data.materials.append(timber)
    master["iggy_material_status"] = "structural_oak_timber_v1"
    master["iggy_growth_volume_continuity"] = True
    master["iggy_authored_roughness_map"] = False
    master["iggy_material_damage"] = False
    knot_bodies = [
        obj
        for obj in bpy.data.objects
        if obj.get("iggy_anatomy_role") == "recessed_knot_body"
    ]
    for body in knot_bodies:
        body.data.materials.clear()
        body.data.materials.append(knot)

    stage = _configure_stage(args.render_width, args.render_height)
    proofs = (
        {}
        if args.skip_renders
        else render_proofs(output_root, stage)
    )
    blend_path = output_root / f"{PREFIX}_beam.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    manifest = {
        "schema": "iggy3d.material.structural_oak_timber.blender.v1",
        "profile_id": profile["profile_id"],
        "source_beam": str(input_blend),
        "blend": str(blend_path.resolve()),
        "master_object": MASTER_NAME,
        "material": MATERIAL_NAME,
        "knot_material": KNOT_MATERIAL_NAME,
        "packed_images": [path.name for path in required_maps],
        "coordinate_mapping": {
            "side": "IGGY_TimberUV metre values + sinc_timber_face_id rows",
            "end": "independent left/right columns selected by sinc_timber_face_id",
            "boolean_policy": (
                "original face attributes remain authoritative; Boolean bearing "
                "faces inherit end/adjacent identity and never use a triangle mask"
            ),
        },
        "surface_response": {
            "uniform_roughness": profile["surface_response"]["uniform_roughness"],
            "authored_roughness_map": False,
            "damage": False,
        },
        "proofs": proofs,
    }
    manifest_path = output_root / f"{PREFIX}_blender_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(
        json.dumps(
            {
                "blend": str(blend_path),
                "material": MATERIAL_NAME,
                "proof_count": len(proofs),
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
