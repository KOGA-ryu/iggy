#!/usr/bin/env python3
"""Render saved v004 beside integrated selected-C v004.1 in Cycles."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from typing import Any

import bpy
from mathutils import Vector


MATERIAL_ROOT = Path(__file__).resolve().parent
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004_1.py"
V004_BLEND = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_connected_oxide_candidate_v004"
    / "forged_iron_connected_oxide_candidate_v004.blend"
)
V004_1_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004_1"
V004_1_BLEND = V004_1_ROOT / "forged_iron_connected_oxide_candidate_v004_1.blend"
V004_1_MANIFEST = V004_1_ROOT / "manifest.json"
DEFAULT_OUTPUT = V004_1_ROOT / "cycles_v004_v004_1_proof_v1"

PANEL_ORDER = (
    "close_neutral",
    "grazing",
    "pivot_close",
    "gameplay",
    "base_colour",
    "independent_roughness",
    "normal",
)
PANEL_LABELS = {
    "close_neutral": "CLOSE NEUTRAL",
    "grazing": "GRAZING",
    "pivot_close": "PIVOT CLOSE",
    "gameplay": "GAMEPLAY DISTANCE",
    "base_colour": "BASE COLOUR",
    "independent_roughness": "INDEPENDENT ROUGHNESS",
    "normal": "BROAD FORGING NORMAL",
}
ROWS = (
    (
        "v004_control",
        "v004 CONTROL",
        V004_BLEND,
        "IGGY_MAT_ConnectedOxideForgedIron_v004",
    ),
    (
        "v004_1_integrated_c",
        "v004.1 INTEGRATED C",
        V004_1_BLEND,
        "IGGY_MAT_ConnectedOxideForgedIron_v004_1",
    ),
)

# Replaced only after the assembled board and decisive source tiles are viewed.
VISUAL_REVIEW = {
    "result": "repair_requested",
    "preferred_row": "v004_1_integrated_c",
    "advancement_scope": "preferred_repair_base_not_material_acceptance",
    "defect_ledger": [
        "v004.1 repairs v004's worst optical failure: the isolated base colour now carries a readable compressed cool forge-skin range instead of collapsing to near-black.",
        "Close, grazing, and gameplay views retain quiet plate grouping; no stamp, stripe, diamond, cat-face, exposed-conductor island, worked-luster rail, or condition cue was introduced.",
        "The v004.1 close leaf contains restrained low-frequency optical modulation, but it remains connected and subordinate rather than closing into the rejected v003 cloud islands.",
        "The independent roughness lane is numerically wider but remains almost uniform at the actual-consumer framing, so it does not yet create a readable manufacturing-frequency handoff.",
        "The 1.75 broad-normal multiplier produces no decisive actual-asset improvement; the v004 and v004.1 normal proofs remain nearly indistinguishable at the board scale.",
        "The pivot remains a smooth coated cylinder. It needs a separately evidenced component-class response, not another global amplitude increase or a decorative barrel pattern.",
        "At gameplay distance v004.1 preserves the useful silhouette and broad value hierarchy, but its surface identity still depends mainly on geometry and light.",
    ],
    "manual_acceptance_established": False,
}


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


build = _load_module("iggy_v004_1_build", BUILD_SCRIPT)
calibration = build.calibration
base = build.base
presentation = calibration.presentation
board_helpers = calibration.proof


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=720)
    parser.add_argument("--resolution-y", type=int, default=240)
    parser.add_argument("--samples", type=int, default=32)
    parser.add_argument("--board-resolution-x", type=int, default=2880)
    parser.add_argument("--board-resolution-y", type=int, default=960)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def configure_cycles(scene: bpy.types.Scene, args: argparse.Namespace) -> None:
    scene.render.engine = "CYCLES"
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
    scene.render.use_persistent_data = False
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


def lane_materials(
    row_id: str,
    targets: list[bpy.types.Object],
) -> dict[str, dict[str, bpy.types.Material]]:
    node_names = {
        "base_colour": "Continuous_Oxide_Optical_Colour",
        "independent_roughness": "Independent_Oxide_Roughness",
        "normal": "Broad_Forging_Normal_Only",
    }
    lanes: dict[str, dict[str, bpy.types.Material]] = {}
    for obj in targets:
        material = obj.data.materials[0]
        lanes[obj.name] = {}
        for panel_id, node_name in node_names.items():
            image_node = material.node_tree.nodes.get(node_name)
            if image_node is None or image_node.image is None:
                raise RuntimeError(f"{material.name} lost {node_name}")
            lanes[obj.name][panel_id] = base.build_lane_material(
                f"IGGY_MAT_{row_id}_{obj.name}_{panel_id}_Proof",
                image_node.image,
            )
    return lanes


def render_row(
    args: argparse.Namespace,
    row_id: str,
    label: str,
    blend_path: Path,
    material_prefix: str,
    output_root: Path,
) -> tuple[dict[str, Any], dict[str, Any]]:
    bpy.ops.wm.open_mainfile(filepath=str(blend_path))
    scene = bpy.context.scene
    configure_cycles(scene, args)
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    lights = {name: bpy.data.objects[name] for name in presentation.TARGET_LIGHT_NAMES}
    presentation.isolate_actual_hinge(targets, lights)
    shared_group = bpy.data.node_groups.get(build.GROUP_NAME)
    if shared_group is None:
        raise RuntimeError(f"{row_id} lost {build.GROUP_NAME}")
    physical_materials: dict[str, bpy.types.Material] = {}
    for obj in targets:
        material = obj.data.materials[0]
        if not material.name.startswith(material_prefix):
            raise RuntimeError(
                f"{row_id}/{obj.name} has unexpected material {material.name}"
            )
        physical_materials[obj.name] = material

    minimum, maximum = presentation.object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    close_center = Vector((0.72, center.y, center.z))
    renders = calibration.render_physical_panels(
        scene,
        lights,
        center,
        close_center,
        span,
        output_root,
    )
    lanes = lane_materials(row_id, targets)
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    presentation.set_neutral_lights(lights, center)
    for panel_id in ("base_colour", "independent_roughness", "normal"):
        for obj in targets:
            base.set_material(obj, lanes[obj.name][panel_id])
        renders[panel_id] = render_path(scene, output_root / f"{panel_id}.png")
    for obj in targets:
        base.set_material(obj, physical_materials[obj.name])

    row = {
        "label": label,
        "blend": str(blend_path),
        "blend_sha256": sha256_file(blend_path),
        "shared_group": shared_group.name,
        "materials": [physical_materials[obj.name].name for obj in targets],
        "renders": {panel: renders[panel] for panel in PANEL_ORDER},
    }
    framing = {
        "center": list(center),
        "span": list(span),
        "close_center": list(close_center),
    }
    return row, framing


def add_label(
    scene: bpy.types.Scene,
    row_id: str,
    panel_id: str,
    text: str,
    center_x: float,
    center_y: float,
    panel_width: float,
    panel_height: float,
    background: bpy.types.Material,
    foreground: bpy.types.Material,
) -> None:
    presentation.add_panel_plane(
        f"BoardLabel_{row_id}_{panel_id}",
        center_x,
        center_y + panel_height * 0.40,
        panel_width - 0.10,
        panel_height * 0.16,
        0.10,
        background,
    )
    font_curve = bpy.data.curves.new(f"BoardText_{row_id}_{panel_id}", type="FONT")
    font_curve.body = text
    font_curve.align_x = "CENTER"
    font_curve.align_y = "CENTER"
    font_curve.size = 0.20
    font_obj = bpy.data.objects.new(font_curve.name, font_curve)
    scene.collection.objects.link(font_obj)
    font_obj.location = (
        center_x,
        center_y + panel_height * 0.40,
        0.20,
    )
    font_obj.data.materials.append(foreground)


def build_board(output_root: Path, args: argparse.Namespace) -> dict[str, Any]:
    scene = bpy.context.scene
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    camera_data = bpy.data.cameras.new("IGGY_CAM_v004_v004_1_CyclesBoard")
    camera = bpy.data.objects.new("IGGY_CAM_v004_v004_1_CyclesBoard", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 16.0
    camera.location = (0.0, 0.0, 10.0)
    presentation.point_object(camera, Vector((0.0, 0.0, 0.0)))
    board_helpers.configure_board_scene(
        scene,
        args.board_resolution_x,
        args.board_resolution_y,
    )
    background = presentation.make_emission_material(
        "IGGY_MAT_v004_1BoardLabel",
        colour=(0.004, 0.005, 0.007, 1.0),
    )
    foreground = presentation.make_emission_material(
        "IGGY_MAT_v004_1BoardText",
        colour=(0.86, 0.90, 0.96, 1.0),
    )
    panel_width = 12.0
    panel_height = 4.0
    for pair_index, panel_id in enumerate(PANEL_ORDER):
        pair_row = pair_index // 2
        pair_slot = pair_index % 2
        for candidate_index, (row_id, row_label, _blend, _prefix) in enumerate(ROWS):
            column = pair_slot * 2 + candidate_index
            center_x = (column - 1.5) * panel_width
            center_y = (1.5 - pair_row) * panel_height
            panel_path = output_root / row_id / f"{panel_id}.png"
            if not panel_path.is_file():
                raise FileNotFoundError(panel_path)
            image = bpy.data.images.load(str(panel_path), check_existing=False)
            material = presentation.make_emission_material(
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
                material,
            )
            add_label(
                scene,
                row_id,
                panel_id,
                f"{row_label}  {PANEL_LABELS[panel_id]}",
                center_x,
                center_y,
                panel_width,
                panel_height,
                background,
                foreground,
            )
    board_path = output_root / "forged_iron_v004_v004_1_cycles_comparison.png"
    scene.render.filepath = str(board_path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(board_path),
        "resolution": [args.board_resolution_x, args.board_resolution_y],
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "layout": "four-column paired board; v004 immediately left of v004.1",
        "panel_order": list(PANEL_ORDER),
    }


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    for required in (V004_BLEND, V004_1_BLEND, V004_1_MANIFEST):
        if not required.is_file():
            raise FileNotFoundError(required)
    hashes_before = {path.name: sha256_file(path) for path in (V004_BLEND, V004_1_BLEND)}
    script_hash = sha256_file(Path(__file__).resolve())
    rows: dict[str, Any] = {}
    framings: dict[str, Any] = {}
    for row_id, label, blend_path, material_prefix in ROWS:
        row_root = output_root / row_id
        row_root.mkdir(parents=True, exist_ok=True)
        rows[row_id], framings[row_id] = render_row(
            args,
            row_id,
            label,
            blend_path,
            material_prefix,
            row_root,
        )
    if framings[ROWS[0][0]] != framings[ROWS[1][0]]:
        raise RuntimeError(f"v004/v004.1 consumer framing drifted: {framings}")
    hashes_after = {path.name: sha256_file(path) for path in (V004_BLEND, V004_1_BLEND)}
    if hashes_before != hashes_after:
        raise RuntimeError("Paired Cycles proof changed a source blend")
    board = build_board(output_root, args)
    manifest = {
        "schema": "iggy3d.forged_iron_v004_v004_1_cycles_proof.v1",
        "status": "USER_NOT_ACCEPTED_PROOF_ONLY",
        "render_engine": "CYCLES",
        "cycles_samples": args.samples,
        "tile_resolution": [args.resolution_x, args.resolution_y],
        "consumer": framings[ROWS[0][0]],
        "rows": rows,
        "board": board,
        "visual_review": VISUAL_REVIEW,
        "source_hashes": {
            "before": hashes_before,
            "after": hashes_after,
            "sources_unchanged": hashes_before == hashes_after,
        },
        "script": {"path": str(Path(__file__).resolve()), "sha256": script_hash},
        "candidate_manifest": {
            "path": str(V004_1_MANIFEST),
            "sha256": sha256_file(V004_1_MANIFEST),
        },
        "manual_acceptance_required": True,
        "uses_ai_generated_imagery": False,
        "uses_condition": False,
        "unreal_parity_verified": False,
        "limitations": [
            "This proof compares clean intact-core response only.",
            "v004.1 changes only the selected colour, roughness, and broad-normal fields.",
            "The pivot remains an explicit unresolved component-response defect.",
            "Cycles proof does not establish Unreal parity or user acceptance.",
        ],
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    )
    print(
        "FORGED_IRON_V004_1_CYCLES_PROOF="
        f"rows:{len(rows)},panels:{len(PANEL_ORDER)},sources_unchanged:true"
    )
    print(f"FORGED_IRON_V004_1_CYCLES_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
