#!/usr/bin/env python3
"""Render an adversarial proof board from the frozen forged-iron root blend.

This script is intentionally presentation-only. Blender must open the canonical
``forged_iron_v1.blend`` before the script runs. The script changes cameras,
lights, visibility, and the existing proof selector in memory, writes PNG
proofs, assembles them into one board, and exits without saving a .blend.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
from typing import Any

import bpy
from mathutils import Vector


SCRIPT_ROOT = Path(__file__).resolve().parent
SOURCE_BLEND = SCRIPT_ROOT / "output" / "forged_iron_v1.blend"
SOURCE_MANIFEST = SCRIPT_ROOT / "output" / "forged_iron_v1_manifest.json"
DEFAULT_OUTPUT_ROOT = (
    SCRIPT_ROOT / "output" / "actual_hinge_acceptance_board_v1"
)

MATERIAL_NAME = "IGGY_MAT_ReferenceForgedIron_v003"
MATERIAL_GROUP_NAME = "IGGY_SH_ReferenceForgedIron_v003"
PROOF_NODE_NAME = "IGGY_IRON_PROOF_MODE"
TARGET_NAMES = (
    "SM_GH018_FixedLeaf",
    "SM_GH018_MovingLeaf_Openwork",
    "SM_GH018_Pintle",
    "MovingKnuckle_01",
    "FixedKnuckle_02",
    "MovingKnuckle_03",
    "FixedKnuckle_04",
    "MovingKnuckle_05",
)
TARGET_LIGHT_NAMES = (
    "IGGY_IronTarget_NeutralKey",
    "IGGY_IronTarget_CoolFill",
    "IGGY_IronTarget_GrazingStrip",
)
PROOF_MODES = {
    "combined": 0.0,
    "base_colour": 1.0,
    "roughness": 2.0,
    "normal": 5.0,
    "worked_response": 7.0,
}
LOCKED_ABSENCES = (
    "rust",
    "damage",
    "contact_polish",
    "soot",
    "dirt",
    "blood",
    "cat_face_motifs",
    "diamond_stamps",
)
PANEL_SPECS = (
    ("front", "01 ROOT FRONT"),
    ("three_quarter", "02 ROOT THREE QUARTER"),
    ("grazing", "03 ROOT GRAZING"),
    ("gameplay", "04 GAMEPLAY DISTANCE"),
    ("close_neutral", "05 CLOSE NEUTRAL"),
    ("close_light_left", "06 CLOSE LIGHT LEFT"),
    ("close_light_right", "07 CLOSE LIGHT RIGHT"),
    ("pivot_close", "08 PIVOT AND TANGENT"),
    ("base_colour", "09 UNLIT BASE COLOUR"),
    ("roughness", "10 ROUGHNESS"),
    ("normal", "11 COMBINED NORMAL"),
    ("worked_response", "12 WORKED RESPONSE"),
)


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--tile-resolution-x", type=int, default=900)
    parser.add_argument("--tile-resolution-y", type=int, default=360)
    parser.add_argument("--board-resolution-x", type=int, default=3600)
    parser.add_argument("--board-resolution-y", type=int, default=1080)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def point_object(obj: bpy.types.Object, target: Vector) -> None:
    direction = target - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def object_bounds(objects: list[bpy.types.Object]) -> tuple[Vector, Vector]:
    points = [
        obj.matrix_world @ Vector(corner)
        for obj in objects
        for corner in obj.bound_box
    ]
    return (
        Vector((
            min(point.x for point in points),
            min(point.y for point in points),
            min(point.z for point in points),
        )),
        Vector((
            max(point.x for point in points),
            max(point.y for point in points),
            max(point.z for point in points),
        )),
    )


def configure_render(scene: bpy.types.Scene, width: int, height: int) -> None:
    if not scene.render.engine.startswith("BLENDER_EEVEE"):
        raise RuntimeError(
            f"Frozen proof blend must use Eevee, found {scene.render.engine}"
        )
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.render.use_file_extension = True
    if hasattr(scene, "eevee"):
        scene.eevee.taa_render_samples = 64


def configure_world(scene: bpy.types.Scene, strength: float = 0.025) -> None:
    world = scene.world
    if world is None:
        world = bpy.data.worlds.new("IGGY_AcceptanceBoardWorld")
        scene.world = world
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    if background is None:
        background = world.node_tree.nodes.new("ShaderNodeBackground")
    background.inputs["Color"].default_value = (0.008, 0.010, 0.014, 1.0)
    background.inputs["Strength"].default_value = strength


def validate_root() -> tuple[
    bpy.types.Scene,
    bpy.types.Material,
    bpy.types.Node,
    list[bpy.types.Object],
    dict[str, bpy.types.Object],
]:
    opened = Path(bpy.data.filepath).resolve()
    if opened != SOURCE_BLEND.resolve():
        raise RuntimeError(
            f"Open {SOURCE_BLEND} before running this proof; found {opened}"
        )
    if not SOURCE_MANIFEST.is_file():
        raise FileNotFoundError(SOURCE_MANIFEST)
    source_payload = json.loads(SOURCE_MANIFEST.read_text())
    if source_payload.get("status") != "LAYERED_NODE_FORGED_IRON_BUILT":
        raise RuntimeError("Root forged-iron manifest is not the frozen build")

    scene = bpy.context.scene
    if scene.camera is None:
        raise RuntimeError("Frozen proof blend has no scene camera")
    material = bpy.data.materials.get(MATERIAL_NAME)
    if material is None or not material.use_nodes:
        raise RuntimeError(f"Missing canonical material {MATERIAL_NAME}")
    group_nodes = [
        node
        for node in material.node_tree.nodes
        if node.bl_idname == "ShaderNodeGroup" and node.node_tree is not None
    ]
    if [node.node_tree.name for node in group_nodes] != [MATERIAL_GROUP_NAME]:
        raise RuntimeError("Canonical public material group drifted")
    proof_mode = material.node_tree.nodes.get(PROOF_NODE_NAME)
    if proof_mode is None or proof_mode.bl_idname != "ShaderNodeValue":
        raise RuntimeError(f"Missing proof selector {PROOF_NODE_NAME}")

    missing_targets = [name for name in TARGET_NAMES if bpy.data.objects.get(name) is None]
    if missing_targets:
        raise RuntimeError(f"Missing actual-hinge targets: {missing_targets}")
    targets = [bpy.data.objects[name] for name in TARGET_NAMES]
    for target in targets:
        if target.type != "MESH":
            raise RuntimeError(f"Actual-hinge target is not a mesh: {target.name}")
        materials = [slot.material.name if slot.material else None for slot in target.material_slots]
        if materials != [MATERIAL_NAME]:
            raise RuntimeError(
                f"Actual-hinge material drift on {target.name}: {materials}"
            )
        if "IGGY_IronUV" not in target.data.uv_layers:
            raise RuntimeError(f"Missing IGGY_IronUV on {target.name}")

    lights: dict[str, bpy.types.Object] = {}
    for name in TARGET_LIGHT_NAMES:
        light = bpy.data.objects.get(name)
        if light is None or light.type != "LIGHT":
            raise RuntimeError(f"Missing target proof light {name}")
        lights[name] = light
    return scene, material, proof_mode, targets, lights


def isolate_actual_hinge(
    targets: list[bpy.types.Object],
    lights: dict[str, bpy.types.Object],
) -> None:
    target_set = set(targets)
    light_set = set(lights.values())
    for obj in bpy.data.objects:
        visible = obj in target_set or obj in light_set or obj is bpy.context.scene.camera
        obj.hide_render = not visible
        obj.hide_viewport = not visible


def configure_light(
    light: bpy.types.Object,
    *,
    location: Vector,
    target: Vector,
    energy: float,
    size: float,
    size_y: float,
    colour: tuple[float, float, float],
) -> None:
    light.location = location
    point_object(light, target)
    light.data.energy = energy
    light.data.color = colour
    light.data.shape = "RECTANGLE"
    light.data.size = size
    light.data.size_y = size_y
    light.hide_render = False
    light.hide_viewport = False


def set_neutral_lights(
    lights: dict[str, bpy.types.Object],
    target: Vector,
    *,
    close: bool = False,
) -> None:
    scale = 0.72 if close else 1.0
    configure_light(
        lights["IGGY_IronTarget_NeutralKey"],
        location=target + Vector((-0.75, 3.0, 1.45)),
        target=target,
        energy=115.0 * scale,
        size=1.7 if close else 2.2,
        size_y=1.7 if close else 2.2,
        colour=(1.0, 0.97, 0.93),
    )
    configure_light(
        lights["IGGY_IronTarget_CoolFill"],
        location=target + Vector((2.15, 2.0, -0.85)),
        target=target,
        energy=28.0 * scale,
        size=1.5,
        size_y=1.5,
        colour=(0.74, 0.84, 1.0),
    )
    configure_light(
        lights["IGGY_IronTarget_GrazingStrip"],
        location=target + Vector((-2.0, 0.70, 0.48)),
        target=target,
        energy=68.0 * scale,
        size=2.0 if close else 2.6,
        size_y=0.08,
        colour=(1.0, 0.82, 0.66),
    )


def set_moving_strip(
    lights: dict[str, bpy.types.Object],
    target: Vector,
    *,
    side: str,
) -> None:
    sign = -1.0 if side == "left" else 1.0
    configure_light(
        lights["IGGY_IronTarget_NeutralKey"],
        location=target + Vector((0.0, 2.8, 1.4)),
        target=target,
        energy=10.0,
        size=2.0,
        size_y=2.0,
        colour=(1.0, 1.0, 1.0),
    )
    configure_light(
        lights["IGGY_IronTarget_CoolFill"],
        location=target + Vector((-sign * 1.1, 1.5, -0.55)),
        target=target,
        energy=12.0,
        size=1.2,
        size_y=1.2,
        colour=(0.76, 0.85, 1.0),
    )
    configure_light(
        lights["IGGY_IronTarget_GrazingStrip"],
        location=target + Vector((sign * 1.7, 0.52, 0.28)),
        target=target,
        energy=230.0,
        size=0.95,
        size_y=0.055,
        colour=(1.0, 0.86, 0.72),
    )


def front_camera(
    camera: bpy.types.Object,
    target: Vector,
    ortho_scale: float,
    *,
    height: float = 0.0,
) -> None:
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = ortho_scale
    camera.location = Vector((target.x, 5.2, target.z + height))
    point_object(camera, target)


def oblique_camera(
    camera: bpy.types.Object,
    target: Vector,
    ortho_scale: float,
    *,
    x_offset: float,
    height: float,
) -> None:
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = ortho_scale
    camera.location = target + Vector((x_offset, 5.0, height))
    point_object(camera, target)


def render_tile(
    scene: bpy.types.Scene,
    output_root: Path,
    panel_id: str,
) -> dict[str, Any]:
    path = output_root / f"{panel_id}.png"
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "id": panel_id,
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def make_emission_material(
    name: str,
    *,
    colour: tuple[float, float, float, float] | None = None,
    image: bpy.types.Image | None = None,
) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    emission = nodes.new("ShaderNodeEmission")
    emission.inputs["Strength"].default_value = 1.0
    links.new(emission.outputs["Emission"], output.inputs["Surface"])
    if image is not None:
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = image
        texture.interpolation = "Linear"
        links.new(texture.outputs["Color"], emission.inputs["Color"])
    elif colour is not None:
        emission.inputs["Color"].default_value = colour
    else:
        raise ValueError("Emission material needs a colour or image")
    return material


def add_panel_plane(
    name: str,
    center_x: float,
    center_y: float,
    width: float,
    height: float,
    z: float,
    material: bpy.types.Material,
) -> bpy.types.Object:
    half_w = width * 0.5
    half_h = height * 0.5
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(
        [
            (center_x - half_w, center_y - half_h, z),
            (center_x + half_w, center_y - half_h, z),
            (center_x + half_w, center_y + half_h, z),
            (center_x - half_w, center_y + half_h, z),
        ],
        [],
        [(0, 1, 2, 3)],
    )
    mesh.uv_layers.new(name="UVMap")
    for loop, uv in zip(mesh.uv_layers[0].data, ((0, 0), (1, 0), (1, 1), (0, 1))):
        loop.uv = uv
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(material)
    return obj


def assemble_board(
    scene: bpy.types.Scene,
    output_root: Path,
    panels: list[dict[str, Any]],
    width: int,
    height: int,
) -> dict[str, Any]:
    if len(PANEL_SPECS) != 12 or len(panels) != len(PANEL_SPECS):
        raise RuntimeError("Acceptance board requires exactly twelve panels")
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)

    scene.camera = None
    camera_data = bpy.data.cameras.new("IGGY_CAM_IronAcceptanceBoard")
    camera = bpy.data.objects.new("IGGY_CAM_IronAcceptanceBoard", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type = "ORTHO"
    # Blender's orthographic scale is the horizontal span for this camera.
    # The board is 4 panels x 4.0 units, with a 3.333:1 render aspect that
    # produces the required 4.8-unit vertical span.
    camera.data.ortho_scale = 16.0
    camera.location = (0.0, 0.0, 10.0)
    point_object(camera, Vector((0.0, 0.0, 0.0)))

    configure_render(scene, width, height)
    configure_world(scene, strength=0.0)
    scene.view_settings.view_transform = "Standard"
    try:
        scene.view_settings.look = "None"
    except TypeError:
        pass

    black = make_emission_material(
        "IGGY_MAT_AcceptanceBoardLabel",
        colour=(0.004, 0.005, 0.007, 1.0),
    )
    white = make_emission_material(
        "IGGY_MAT_AcceptanceBoardText",
        colour=(0.86, 0.90, 0.96, 1.0),
    )
    for index, ((panel_id, label), panel) in enumerate(zip(PANEL_SPECS, panels)):
        if panel["id"] != panel_id:
            raise RuntimeError(f"Panel order drift: {panel['id']} != {panel_id}")
        row = index // 4
        column = index % 4
        center_x = (column - 1.5) * 4.0
        center_y = (1.0 - row) * 1.6
        image = bpy.data.images.load(panel["path"], check_existing=False)
        image_material = make_emission_material(
            f"IGGY_MAT_Board_{panel_id}",
            image=image,
        )
        add_panel_plane(
            f"BoardPanel_{panel_id}",
            center_x,
            center_y,
            3.96,
            1.56,
            0.0,
            image_material,
        )
        add_panel_plane(
            f"BoardLabel_{panel_id}",
            center_x,
            center_y + 0.65,
            3.88,
            0.25,
            0.10,
            black,
        )
        font_curve = bpy.data.curves.new(
            f"BoardText_{panel_id}",
            type="FONT",
        )
        font_curve.body = label
        font_curve.align_x = "LEFT"
        font_curve.align_y = "CENTER"
        font_curve.size = 0.155
        font_curve.space_character = 1.05
        font_obj = bpy.data.objects.new(f"BoardText_{panel_id}", font_curve)
        scene.collection.objects.link(font_obj)
        font_obj.location = (center_x - 1.83, center_y + 0.65, 0.20)
        font_obj.data.materials.append(white)

    path = output_root / "forged_iron_v1_actual_hinge_acceptance_board.png"
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "resolution": [width, height],
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def main() -> None:
    args = parse_args()
    output_root = args.output_root.resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    source_hash_before = sha256_file(SOURCE_BLEND)
    source_manifest_hash = sha256_file(SOURCE_MANIFEST)
    script_hash = sha256_file(Path(__file__).resolve())

    scene, material, proof_mode, targets, lights = validate_root()
    isolate_actual_hinge(targets, lights)
    configure_render(scene, args.tile_resolution_x, args.tile_resolution_y)
    configure_world(scene)
    camera = scene.camera
    minimum, maximum = object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    full_scale = span.x * 1.12
    close_center = Vector((0.72, center.y, center.z))
    pivot_minimum, pivot_maximum = object_bounds([
        bpy.data.objects["SM_GH018_Pintle"],
        bpy.data.objects["MovingKnuckle_01"],
        bpy.data.objects["FixedKnuckle_02"],
        bpy.data.objects["MovingKnuckle_03"],
        bpy.data.objects["FixedKnuckle_04"],
        bpy.data.objects["MovingKnuckle_05"],
    ])
    pivot_center = (pivot_minimum + pivot_maximum) * 0.5

    panels: list[dict[str, Any]] = []
    proof_mode.outputs[0].default_value = PROOF_MODES["combined"]
    set_neutral_lights(lights, center)
    front_camera(camera, center, full_scale)
    panels.append(render_tile(scene, output_root, "front"))

    set_neutral_lights(lights, center)
    oblique_camera(
        camera,
        center,
        full_scale * 1.03,
        x_offset=-0.26,
        height=0.92,
    )
    panels.append(render_tile(scene, output_root, "three_quarter"))

    set_moving_strip(lights, center, side="left")
    oblique_camera(
        camera,
        center,
        full_scale * 1.02,
        x_offset=-0.18,
        height=0.58,
    )
    panels.append(render_tile(scene, output_root, "grazing"))

    set_neutral_lights(lights, center)
    front_camera(camera, center, span.x * 1.62)
    panels.append(render_tile(scene, output_root, "gameplay"))

    set_neutral_lights(lights, close_center, close=True)
    front_camera(camera, close_center, 2.05)
    panels.append(render_tile(scene, output_root, "close_neutral"))

    set_moving_strip(lights, close_center, side="left")
    front_camera(camera, close_center, 2.05)
    panels.append(render_tile(scene, output_root, "close_light_left"))

    set_moving_strip(lights, close_center, side="right")
    front_camera(camera, close_center, 2.05)
    panels.append(render_tile(scene, output_root, "close_light_right"))

    set_neutral_lights(lights, pivot_center, close=True)
    oblique_camera(
        camera,
        pivot_center,
        0.78,
        x_offset=-0.05,
        height=0.18,
    )
    panels.append(render_tile(scene, output_root, "pivot_close"))

    front_camera(camera, center, full_scale)
    for panel_id, proof_name in (
        ("base_colour", "base_colour"),
        ("roughness", "roughness"),
        ("normal", "normal"),
        ("worked_response", "worked_response"),
    ):
        proof_mode.outputs[0].default_value = PROOF_MODES[proof_name]
        panels.append(render_tile(scene, output_root, panel_id))
    proof_mode.outputs[0].default_value = PROOF_MODES["combined"]

    board = assemble_board(
        scene,
        output_root,
        panels,
        args.board_resolution_x,
        args.board_resolution_y,
    )
    source_hash_after = sha256_file(SOURCE_BLEND)
    if source_hash_before != source_hash_after:
        raise RuntimeError("Canonical forged-iron blend changed during proof")

    manifest = {
        "schema": "iggy3d.forged_iron_actual_hinge_acceptance_board.v1",
        "status": "USER_NOT_ACCEPTED_PROOF_ONLY",
        "candidate": {
            "id": "forged_iron_v1",
            "material": material.name,
            "public_group": MATERIAL_GROUP_NAME,
            "source_blend": str(SOURCE_BLEND),
            "source_blend_sha256_before": source_hash_before,
            "source_blend_sha256_after": source_hash_after,
            "source_unchanged": source_hash_before == source_hash_after,
            "source_manifest": str(SOURCE_MANIFEST),
            "source_manifest_sha256": source_manifest_hash,
        },
        "consumer": {
            "name": "approved eight-part openwork strap hinge",
            "objects": list(TARGET_NAMES),
            "assembly_bounds_m": {
                "minimum": list(minimum),
                "maximum": list(maximum),
                "span": list(span),
            },
        },
        "render_contract": {
            "engine": "BLENDER_EEVEE",
            "tile_resolution": [
                args.tile_resolution_x,
                args.tile_resolution_y,
            ],
            "board_resolution": [
                args.board_resolution_x,
                args.board_resolution_y,
            ],
            "proof_modes": PROOF_MODES,
            "same_geometry_for_all_panels": True,
            "same_material_for_all_combined_panels": True,
            "no_saved_blend": True,
            "uses_ai_generated_imagery": False,
        },
        "panels": panels,
        "board": board,
        "locked_absences": list(LOCKED_ABSENCES),
        "review_contract": {
            "reject_if": [
                "worked-response masks read as stamps, lozenges, bars, faces, or symbols",
                "broad colour reads as a detached cloud overlay instead of coherent oxide",
                "roughness merely copies colour or height",
                "moving light reveals equal-frequency noise or repeated rows",
                "surface reads as gray plastic, painted stone, or brushed aluminium",
                "detail disappears before the close-to-gameplay handoff",
            ],
            "manual_acceptance_required": True,
            "unreal_parity_verified": False,
        },
        "visual_decision": {
            "result": "repair_requested",
            "strongest_surviving_result": (
                "quiet dark macro value and construction readability survive "
                "on the complete hinge through gameplay distance"
            ),
            "failed_causal_owners": [
                "soft closed-cloud base-colour morphology",
                "large rounded striped worked-response stamps",
                "near-flat roughness hierarchy",
                "weak broad-to-medium normal handoff",
                "smooth coated-plastic pivot response",
            ],
            "next_gate": (
                "reduced A-D connected-oxide and open-luster comparison on "
                "the same hinge; no canonical integration before selection"
            ),
        },
        "generator": {
            "script": str(Path(__file__).resolve()),
            "sha256": script_hash,
        },
    }
    manifest_path = output_root / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"IRON_ACCEPTANCE_BOARD={board['path']}")
    print(f"IRON_ACCEPTANCE_MANIFEST={manifest_path}")


if __name__ == "__main__":
    main()
