#!/usr/bin/env python3
"""Render frozen v003 beside the B-derived v004 candidate in Cycles."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Any

import bpy
from mathutils import Vector


MATERIAL_ROOT = Path(__file__).resolve().parent
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004.py"
SOURCE_BLEND = MATERIAL_ROOT / "output" / "forged_iron_v1.blend"
CANDIDATE_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004"
CANDIDATE_BLEND = CANDIDATE_ROOT / "forged_iron_connected_oxide_candidate_v004.blend"
CANDIDATE_MANIFEST = CANDIDATE_ROOT / "manifest.json"
DEFAULT_OUTPUT = CANDIDATE_ROOT / "cycles_actual_hinge_proof_v1"

PANEL_ORDER = (
    "full_front",
    "close_neutral",
    "opposed_light_difference",
    "grazing",
    "pivot_close",
    "gameplay",
    "base_colour",
    "independent_roughness",
    "normal",
)
PANEL_LABELS = {
    "full_front": "FULL FRONT",
    "close_neutral": "CLOSE NEUTRAL",
    "opposed_light_difference": "OPPOSED LIGHT DIFFERENCE",
    "grazing": "GRAZING",
    "pivot_close": "PIVOT CLOSE",
    "gameplay": "GAMEPLAY DISTANCE",
    "base_colour": "BASE COLOUR",
    "independent_roughness": "INDEPENDENT ROUGHNESS",
    "normal": "NORMAL",
}
ROWS = (
    ("v003_frozen_root", "v003 FROZEN ROOT", SOURCE_BLEND),
    ("v004_connected_oxide", "v004 CONNECTED OXIDE", CANDIDATE_BLEND),
)

# Replaced only after the complete board and decisive source tiles are viewed.
VISUAL_REVIEW = {
    "result": "repair_requested",
    "preferred_row": "v004_connected_oxide",
    "advancement_scope": "preferred_repair_base_not_material_acceptance",
    "defect_ledger": [
        "v004 removes the v003 soft cloud colour field and the finite striped worked-response language in every physical view.",
        "v004 base colour is too close to black in the isolated proof, so the beauty render is carried primarily by the lamps rather than a readable dark oxide colour hierarchy.",
        "v004 independent roughness remains nearly uniform at close and gameplay scale; it does not yet establish a broad-medium-micro manufacturing handoff.",
        "v003 and v004 normal proofs are both nearly flat at the actual-consumer scale, so the broad forging-plane owner remains underpowered.",
        "v004 pivot is cleaner than v003 but still reads as a smooth coated cylinder with no component-scale forged response.",
        "The opposed-light comparison is still dominated by geometry-owned bevels, apertures, and barrel segmentation rather than a convincing intact forged surface.",
        "No symbolic pattern, visible luster rail, rust, contact polish, or condition was introduced by v004.",
    ],
    "manual_acceptance_established": False,
}


def _load_builder():
    spec = importlib.util.spec_from_file_location("iggy_v004_candidate_builder", BUILD_SCRIPT)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load v004 builder: {BUILD_SCRIPT}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


builder = _load_builder()
base = builder.base
presentation = builder.selection.presentation


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=720)
    parser.add_argument("--resolution-y", type=int, default=240)
    parser.add_argument("--samples", type=int, default=32)
    parser.add_argument("--board-resolution-x", type=int, default=4320)
    parser.add_argument("--board-resolution-y", type=int, default=720)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def configure_cycles(scene: bpy.types.Scene, args: argparse.Namespace) -> None:
    scene.render.engine = "CYCLES"
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
    scene.render.use_persistent_data = True
    scene.render.resolution_x = args.resolution_x
    scene.render.resolution_y = args.resolution_y
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = 0.0
    presentation.configure_world(scene)


def render_path(scene: bpy.types.Scene, path: Path) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def render_opposed_difference(
    scene: bpy.types.Scene,
    lights: dict[str, bpy.types.Object],
    close_center: Vector,
    output_path: Path,
) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for the opposed-light diagnostic")
    left = output_path.with_name("_close_light_left.png")
    right = output_path.with_name("_close_light_right.png")
    presentation.front_camera(scene.camera, close_center, 2.05)
    presentation.set_moving_strip(lights, close_center, side="left")
    render_path(scene, left)
    presentation.set_moving_strip(lights, close_center, side="right")
    render_path(scene, right)
    completed = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(left),
            "-i",
            str(right),
            "-filter_complex",
            "blend=all_mode=difference",
            "-frames:v",
            "1",
            str(output_path),
        ],
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError("opposed-light assembly failed: " + completed.stderr.strip())
    left.unlink(missing_ok=True)
    right.unlink(missing_ok=True)
    return {
        "path": str(output_path),
        "bytes": output_path.stat().st_size,
        "sha256": sha256_file(output_path),
        "operation": "absolute sRGB difference of identical Cycles left/right close renders",
    }


def render_physical_panels(
    scene: bpy.types.Scene,
    lights: dict[str, bpy.types.Object],
    targets: list[bpy.types.Object],
    center: Vector,
    close_center: Vector,
    span: Vector,
    output_root: Path,
) -> dict[str, dict[str, Any]]:
    renders: dict[str, dict[str, Any]] = {}
    presentation.set_neutral_lights(lights, center)
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    renders["full_front"] = render_path(scene, output_root / "full_front.png")

    presentation.set_neutral_lights(lights, close_center, close=True)
    presentation.front_camera(scene.camera, close_center, 2.05)
    renders["close_neutral"] = render_path(scene, output_root / "close_neutral.png")
    renders["opposed_light_difference"] = render_opposed_difference(
        scene,
        lights,
        close_center,
        output_root / "opposed_light_difference.png",
    )

    presentation.set_moving_strip(lights, center, side="left")
    presentation.oblique_camera(
        scene.camera,
        center,
        span.x * 1.14,
        x_offset=-0.18,
        height=0.58,
    )
    renders["grazing"] = render_path(scene, output_root / "grazing.png")

    pivot_names = (
        "SM_GH018_Pintle",
        "MovingKnuckle_01",
        "FixedKnuckle_02",
        "MovingKnuckle_03",
        "FixedKnuckle_04",
        "MovingKnuckle_05",
    )
    pivot_minimum, pivot_maximum = presentation.object_bounds(
        [bpy.data.objects[name] for name in pivot_names]
    )
    pivot_center = (pivot_minimum + pivot_maximum) * 0.5
    presentation.set_neutral_lights(lights, pivot_center, close=True)
    presentation.oblique_camera(
        scene.camera,
        pivot_center,
        0.78,
        x_offset=-0.05,
        height=0.18,
    )
    renders["pivot_close"] = render_path(scene, output_root / "pivot_close.png")

    presentation.set_neutral_lights(lights, center)
    presentation.front_camera(scene.camera, center, span.x * 1.62)
    renders["gameplay"] = render_path(scene, output_root / "gameplay.png")
    return renders


def root_row(
    args: argparse.Namespace,
    output_root: Path,
) -> tuple[dict[str, Any], dict[str, Any]]:
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
    scene, material, proof_mode, targets, lights = presentation.validate_root()
    presentation.isolate_actual_hinge(targets, lights)
    configure_cycles(scene, args)
    minimum, maximum = presentation.object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    close_center = Vector((0.72, center.y, center.z))
    for obj in targets:
        base.set_material(obj, material)
    proof_mode.outputs[0].default_value = presentation.PROOF_MODES["combined"]
    renders = render_physical_panels(
        scene,
        lights,
        targets,
        center,
        close_center,
        span,
        output_root,
    )
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    presentation.set_neutral_lights(lights, center)
    for panel_id, proof_name in (
        ("base_colour", "base_colour"),
        ("independent_roughness", "roughness"),
        ("normal", "normal"),
    ):
        proof_mode.outputs[0].default_value = presentation.PROOF_MODES[proof_name]
        renders[panel_id] = render_path(scene, output_root / f"{panel_id}.png")
    proof_mode.outputs[0].default_value = presentation.PROOF_MODES["combined"]
    return (
        {
            "label": "v003 FROZEN ROOT",
            "blend": str(SOURCE_BLEND),
            "blend_sha256": sha256_file(SOURCE_BLEND),
            "material": material.name,
            "renders": {panel: renders[panel] for panel in PANEL_ORDER},
        },
        {"center": list(center), "span": list(span), "close_center": list(close_center)},
    )


def candidate_lane_materials(
    targets: list[bpy.types.Object],
) -> tuple[dict[str, dict[str, bpy.types.Material]], list[bpy.types.Material]]:
    lanes: dict[str, dict[str, bpy.types.Material]] = {}
    tracked: list[bpy.types.Material] = []
    node_names = {
        "base_colour": "Continuous_Oxide_Optical_Colour",
        "independent_roughness": "Independent_Oxide_Roughness",
        "normal": "Broad_Forging_Normal_Only",
    }
    for obj in targets:
        material = obj.data.materials[0]
        lanes[obj.name] = {}
        for panel_id, node_name in node_names.items():
            image_node = material.node_tree.nodes.get(node_name)
            if image_node is None or image_node.image is None:
                raise RuntimeError(f"Missing {node_name} on {material.name}")
            lane = base.build_lane_material(
                f"IGGY_MAT_v004_{obj.name}_{panel_id}_Proof",
                image_node.image,
            )
            lanes[obj.name][panel_id] = lane
            tracked.append(lane)
    return lanes, tracked


def candidate_row(
    args: argparse.Namespace,
    output_root: Path,
) -> tuple[dict[str, Any], dict[str, Any]]:
    bpy.ops.wm.open_mainfile(filepath=str(CANDIDATE_BLEND))
    scene = bpy.context.scene
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    lights = {name: bpy.data.objects[name] for name in presentation.TARGET_LIGHT_NAMES}
    presentation.isolate_actual_hinge(targets, lights)
    configure_cycles(scene, args)
    group = bpy.data.node_groups.get(builder.GROUP_NAME)
    if group is None:
        raise RuntimeError("v004 shared response group is missing")
    for obj in targets:
        material = obj.data.materials[0]
        if not material.name.startswith(builder.MATERIAL_PREFIX):
            raise RuntimeError(f"Target {obj.name} is not using the v004 candidate")
    minimum, maximum = presentation.object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    close_center = Vector((0.72, center.y, center.z))
    physical_materials = {obj.name: obj.data.materials[0] for obj in targets}
    renders = render_physical_panels(
        scene,
        lights,
        targets,
        center,
        close_center,
        span,
        output_root,
    )
    lane_materials, _tracked = candidate_lane_materials(targets)
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    presentation.set_neutral_lights(lights, center)
    for panel_id in ("base_colour", "independent_roughness", "normal"):
        for obj in targets:
            base.set_material(obj, lane_materials[obj.name][panel_id])
        renders[panel_id] = render_path(scene, output_root / f"{panel_id}.png")
    for obj in targets:
        base.set_material(obj, physical_materials[obj.name])
    return (
        {
            "label": "v004 CONNECTED OXIDE",
            "blend": str(CANDIDATE_BLEND),
            "blend_sha256": sha256_file(CANDIDATE_BLEND),
            "shared_group": group.name,
            "materials": [physical_materials[obj.name].name for obj in targets],
            "renders": {panel: renders[panel] for panel in PANEL_ORDER},
        },
        {"center": list(center), "span": list(span), "close_center": list(close_center)},
    )


def configure_board_scene(scene: bpy.types.Scene, width: int, height: int) -> None:
    try:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "Standard"
    try:
        scene.view_settings.look = "None"
    except TypeError:
        pass
    presentation.configure_world(scene, strength=0.0)


def build_comparison_board(
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    scene = bpy.context.scene
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    camera_data = bpy.data.cameras.new("IGGY_CAM_v003_v004_CyclesBoard")
    camera = bpy.data.objects.new("IGGY_CAM_v003_v004_CyclesBoard", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 24.0
    camera.location = (0.0, 0.0, 10.0)
    presentation.point_object(camera, Vector((0.0, 0.0, 0.0)))
    configure_board_scene(scene, args.board_resolution_x, args.board_resolution_y)
    label_background = presentation.make_emission_material(
        "IGGY_MAT_v004BoardLabel",
        colour=(0.004, 0.005, 0.007, 1.0),
    )
    label_text = presentation.make_emission_material(
        "IGGY_MAT_v004BoardText",
        colour=(0.86, 0.90, 0.96, 1.0),
    )
    panel_width = 4.0
    panel_height = 4.0 / 3.0
    for pair_index, panel_id in enumerate(PANEL_ORDER):
        pair_row = pair_index // 3
        pair_column = pair_index % 3
        for row_index, (row_id, row_label, _blend) in enumerate(ROWS):
            column = pair_column * 2 + row_index
            center_x = (column - 2.5) * panel_width
            center_y = (1.0 - pair_row) * panel_height
            panel_path = output_root / row_id / f"{panel_id}.png"
            if not panel_path.is_file():
                raise FileNotFoundError(panel_path)
            image = bpy.data.images.load(str(panel_path), check_existing=False)
            image_material = presentation.make_emission_material(
                f"IGGY_MAT_Board_{row_id}_{panel_id}",
                image=image,
            )
            presentation.add_panel_plane(
                f"Board_{row_id}_{panel_id}",
                center_x,
                center_y,
                panel_width - 0.035,
                panel_height - 0.035,
                0.0,
                image_material,
            )
            presentation.add_panel_plane(
                f"BoardLabel_{row_id}_{panel_id}",
                center_x,
                center_y + panel_height * 0.40,
                panel_width - 0.10,
                panel_height * 0.16,
                0.10,
                label_background,
            )
            font_curve = bpy.data.curves.new(
                f"BoardText_{row_id}_{panel_id}",
                type="FONT",
            )
            version_label = "v003" if row_index == 0 else "v004"
            font_curve.body = f"{version_label}  {PANEL_LABELS[panel_id]}"
            font_curve.align_x = "LEFT"
            font_curve.align_y = "CENTER"
            font_curve.size = 0.105
            font_obj = bpy.data.objects.new(font_curve.name, font_curve)
            scene.collection.objects.link(font_obj)
            font_obj.location = (
                center_x - panel_width * 0.46,
                center_y + panel_height * 0.40,
                0.20,
            )
            font_obj.data.materials.append(label_text)
    board_path = output_root / "forged_iron_v003_v004_cycles_comparison.png"
    scene.render.filepath = str(board_path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(board_path),
        "resolution": [args.board_resolution_x, args.board_resolution_y],
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "layout": "three proof pairs per row; frozen v003 immediately left of v004",
        "panel_order": list(PANEL_ORDER),
    }


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if not SOURCE_BLEND.is_file() or not CANDIDATE_BLEND.is_file() or not CANDIDATE_MANIFEST.is_file():
        raise FileNotFoundError("v003 source or v004 candidate package is missing")
    source_hash_before = sha256_file(SOURCE_BLEND)
    candidate_hash_before = sha256_file(CANDIDATE_BLEND)
    script_hash = sha256_file(Path(__file__).resolve())
    rows: dict[str, Any] = {}
    root_id = ROWS[0][0]
    candidate_id = ROWS[1][0]
    (output_root / root_id).mkdir(parents=True, exist_ok=True)
    (output_root / candidate_id).mkdir(parents=True, exist_ok=True)
    (rows[root_id], root_consumer) = root_row(args, output_root / root_id)
    (rows[candidate_id], candidate_consumer) = candidate_row(
        args,
        output_root / candidate_id,
    )
    if root_consumer != candidate_consumer:
        raise RuntimeError(
            f"v003/v004 consumer framing drifted: {root_consumer} != {candidate_consumer}"
        )
    source_hash_after = sha256_file(SOURCE_BLEND)
    candidate_hash_after = sha256_file(CANDIDATE_BLEND)
    if source_hash_before != source_hash_after or candidate_hash_before != candidate_hash_after:
        raise RuntimeError("Cycles proof changed a source blend")
    board = build_comparison_board(output_root, args)
    manifest = {
        "schema": "iggy3d.forged_iron_v003_v004_cycles_proof.v1",
        "status": "USER_NOT_ACCEPTED_PROOF_ONLY",
        "render_engine": "CYCLES",
        "cycles_samples": args.samples,
        "tile_resolution": [args.resolution_x, args.resolution_y],
        "consumer": root_consumer,
        "rows": rows,
        "board": board,
        "visual_review": VISUAL_REVIEW,
        "source_hashes": {
            "v003_before": source_hash_before,
            "v003_after": source_hash_after,
            "v004_before": candidate_hash_before,
            "v004_after": candidate_hash_after,
        },
        "script": {"path": str(Path(__file__).resolve()), "sha256": script_hash},
        "manual_acceptance_required": True,
        "uses_ai_generated_imagery": False,
        "uses_condition": False,
        "unreal_parity_verified": False,
        "limitations": [
            "This board compares intact clean-core response only.",
            "v004 is a noncanonical candidate and v003 remains frozen current work.",
            "Cycles proof does not establish Unreal parity or user acceptance.",
        ],
    }
    manifest_path = output_root / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(
        "FORGED_IRON_V004_CYCLES_PROOF="
        f"rows:{len(rows)},panels:{len(PANEL_ORDER)},sources_unchanged:true"
    )
    print(f"FORGED_IRON_V004_CYCLES_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
