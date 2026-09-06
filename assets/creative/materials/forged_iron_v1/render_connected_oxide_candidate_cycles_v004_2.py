#!/usr/bin/env python3
"""Render saved v004.1 and v004.2 as a fixed paired Cycles proof."""

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
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004_2.py"
SWEEP_SCRIPT = MATERIAL_ROOT / "compare_v004_1_reflection_routing_v1.py"
V004_1_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004_1"
V004_1_BLEND = V004_1_ROOT / "forged_iron_connected_oxide_candidate_v004_1.blend"
V004_1_MANIFEST = V004_1_ROOT / "manifest.json"
V004_2_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004_2"
V004_2_BLEND = V004_2_ROOT / "forged_iron_connected_oxide_candidate_v004_2.blend"
V004_2_MANIFEST = V004_2_ROOT / "manifest.json"
DEFAULT_OUTPUT = V004_2_ROOT / "cycles_v004_1_v004_2_proof_v1"
REVIEW_PATH = MATERIAL_ROOT / "V004_2_INTEGRATION_REVIEW.md"

PANEL_ORDER = (
    "close_neutral",
    "grazing_left",
    "grazing_right",
    "pivot_close",
    "gameplay",
    "direction_amount",
    "source_normal",
)
PANEL_LABELS = {
    "close_neutral": "CLOSE NEUTRAL",
    "grazing_left": "MOVING STRIP LEFT",
    "grazing_right": "MOVING STRIP RIGHT",
    "pivot_close": "PIVOT CLOSE",
    "gameplay": "GAMEPLAY DISTANCE",
    "direction_amount": "LIVE DIRECTION AMOUNT",
    "source_normal": "FROZEN SOURCE NORMAL",
}
ROWS = (
    {
        "row_id": "A_v004_1_control",
        "label": "A  V004.1 CONTROL",
        "blend": V004_1_BLEND,
        "manifest": V004_1_MANIFEST,
        "material_prefix": "IGGY_MAT_ConnectedOxideForgedIron_v004_1",
        "group": "IGGY_SH_ConnectedOxideForgedIron_v004",
        "multiplier": 0.0,
    },
    {
        "row_id": "B_v004_2_direction_response",
        "label": "B  V004.2 DIRECTION RESPONSE",
        "blend": V004_2_BLEND,
        "manifest": V004_2_MANIFEST,
        "material_prefix": "IGGY_MAT_ConnectedOxideForgedIron_v004_2",
        "group": "IGGY_SH_ConnectedOxideForgedIron_v004_2",
        "multiplier": 1.0,
    },
)

# Replaced after the hash-locked board and decisive source tiles are inspected.
VISUAL_REVIEW = {
    "decision": "prefer_v004_2_repair_candidate",
    "preferred_row": "B_v004_2_direction_response",
    "advancement_scope": "preferred_forged_iron_repair_candidate_not_acceptance",
    "defect_ledger": [
        "The v004.2 row changes highlight travel without drawing a surface pattern. Close neutral remains broad and quiet; neither opposed strip view develops brushing lines, grooves, repeated symbols, tangent bands, or a baked light direction.",
        "The response is genuinely angle dependent and balanced: v004.2 differs from v004.1 by 44.03 dB close, 39.28 dB strip-left, 39.47 dB strip-right, 44.40 dB pivot, and 49.52 dB gameplay. These reproduce the selected disposable-C calibration rather than a new visual guess.",
        "The live direction-amount proof changes from zero to uniform 0.28 while all eight base-colour, roughness, and source-normal float hashes remain identical. The decoded source-normal proof pixels are also identical.",
        "No tangent seam is visible on the long leaves, rolled eyes, or pin. The strongest normalized differences remain broad highlight redistribution around real silhouette, apertures, joins, and cylindrical curvature rather than a finite motif.",
        "The improvement is intentionally subordinate and nearly disappears at gameplay distance. It improves clean-metal plausibility but does not by itself create a complete forged-surface identity.",
        "The inherited broad normal remains the previously rejected shape-normalized owner. This integration freezes it for isolation; it does not validate it.",
        "The pivot remains a smooth coated cylinder with geometry-owned bearing gaps and highlight continuity. v004.2 does not repair or disguise that construction limitation.",
        "A matched professional-reference acceptance plate and Unreal parity are still absent, so this row may become only the preferred repair base.",
    ],
    "physical_panel_psnr_db": {
        "close_neutral": 44.030040,
        "grazing_left": 39.282581,
        "grazing_right": 39.474193,
        "pivot_close": 44.397690,
        "gameplay": 49.521032,
    },
    "strongest_visible_achievement": "quiet construction-oriented highlight response with no new texture motif",
    "weakest_visible_area": "the inherited rejected broad normal and smooth pivot still limit forged-surface identity",
    "repair_pass_count_since_v004_1": 1,
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


build = _load_module("iggy_v004_2_cycles_build", BUILD_SCRIPT)
sweep = _load_module("iggy_v004_2_cycles_sweep", SWEEP_SCRIPT)
base = build.base
presentation = sweep.presentation


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=640)
    parser.add_argument("--resolution-y", type=int, default=220)
    parser.add_argument("--samples", type=int, default=32)
    parser.add_argument("--review-only", action="store_true")
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def render_path(scene: bpy.types.Scene, path: Path) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    decoded = bpy.data.images.load(str(path), check_existing=False)
    pixel_sha256 = build.image_pixel_sha256(decoded)
    bpy.data.images.remove(decoded)
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
        "decoded_pixel_sha256": pixel_sha256,
    }


def image_nodes(material: bpy.types.Material) -> dict[str, bpy.types.Image]:
    return build.material_images(material)


def build_scalar_lane_material(
    name: str,
    image: bpy.types.Image,
) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    output.name = "Output"
    emission = tree.nodes.new("ShaderNodeEmission")
    emission.name = "Absolute_Zero_To_One_Emission"
    texture = tree.nodes.new("ShaderNodeTexImage")
    texture.name = "Live_Luster_Control"
    texture.image = image
    texture.interpolation = "Linear"
    texture.extension = "EXTEND"
    separate = tree.nodes.new("ShaderNodeSeparateColor")
    separate.name = "Direction_Amount_R"
    tree.links.new(texture.outputs["Color"], separate.inputs["Color"])
    tree.links.new(separate.outputs["Red"], emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def render_row(
    row: dict[str, Any],
    output_root: Path,
    args: argparse.Namespace,
) -> tuple[dict[str, Any], dict[str, Any]]:
    bpy.ops.wm.open_mainfile(filepath=str(row["blend"]))
    scene = bpy.context.scene
    sweep.configure_scene(scene, args)
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    lights = {name: bpy.data.objects[name] for name in presentation.TARGET_LIGHT_NAMES}
    presentation.isolate_actual_hinge(targets, lights)

    group = bpy.data.node_groups.get(row["group"])
    if group is None:
        raise RuntimeError(f"{row['row_id']} lost {row['group']}")
    group_signature = build.validate_group(group, row["multiplier"])
    physical_materials: dict[str, bpy.types.Material] = {}
    components: dict[str, Any] = {}
    for obj in targets:
        material = obj.data.materials[0]
        if not material.name.startswith(row["material_prefix"]):
            raise RuntimeError(
                f"{row['row_id']}/{obj.name} has unexpected material {material.name}"
            )
        images = image_nodes(material)
        physical_materials[obj.name] = material
        components[obj.name] = {
            "material": material.name,
            "base_colour_pixel_sha256": build.image_pixel_sha256(images["oxide_base"]),
            "roughness_pixel_sha256": build.image_pixel_sha256(images["oxide_roughness"]),
            "source_normal_pixel_sha256": build.image_pixel_sha256(images["combined_normal"]),
            "luster_control_pixel_sha256": build.image_pixel_sha256(images["luster_control"]),
            "luster_control_resolution": list(images["luster_control"].size),
        }

    minimum, maximum = presentation.object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    close_center = Vector((0.72, center.y, center.z))
    renders: dict[str, Any] = {}

    presentation.set_neutral_lights(lights, close_center, close=True)
    presentation.front_camera(scene.camera, close_center, 2.05)
    renders["close_neutral"] = render_path(scene, output_root / "close_neutral.png")

    presentation.oblique_camera(
        scene.camera,
        center,
        span.x * 1.14,
        x_offset=-0.18,
        height=0.58,
    )
    presentation.set_moving_strip(lights, center, side="left")
    renders["grazing_left"] = render_path(scene, output_root / "grazing_left.png")
    presentation.set_moving_strip(lights, center, side="right")
    renders["grazing_right"] = render_path(scene, output_root / "grazing_right.png")

    _pivot_minimum, _pivot_maximum, pivot_center = sweep.pivot_geometry()
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

    presentation.front_camera(scene.camera, center, span.x * 1.12)
    for obj in targets:
        luster = image_nodes(physical_materials[obj.name])["luster_control"]
        base.set_material(
            obj,
            build_scalar_lane_material(
                f"IGGY_MAT_{row['row_id']}_{obj.name}_DirectionAmountProof",
                luster,
            ),
        )
    renders["direction_amount"] = render_path(
        scene,
        output_root / "direction_amount.png",
    )

    for obj in targets:
        normal = image_nodes(physical_materials[obj.name])["combined_normal"]
        base.set_material(
            obj,
            base.build_lane_material(
                f"IGGY_MAT_{row['row_id']}_{obj.name}_SourceNormalProof",
                normal,
            ),
        )
    renders["source_normal"] = render_path(scene, output_root / "source_normal.png")

    for obj in targets:
        base.set_material(obj, physical_materials[obj.name])
    record = {
        "row_id": row["row_id"],
        "label": row["label"],
        "blend": str(row["blend"]),
        "blend_sha256": sha256_file(row["blend"]),
        "manifest": str(row["manifest"]),
        "manifest_sha256": sha256_file(row["manifest"]),
        "group": group.name,
        "group_contract": group_signature,
        "direction_multiplier": row["multiplier"],
        "components": components,
        "renders": {panel: renders[panel] for panel in PANEL_ORDER},
    }
    framing = {
        "center": list(center),
        "span": list(span),
        "close_center": list(close_center),
        "pivot_center": list(pivot_center),
    }
    return record, framing


def assemble_board(
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for paired proof assembly")
    paths = [
        output_root / row["row_id"] / f"{panel}.png"
        for row in ROWS
        for panel in PANEL_ORDER
    ]
    for path in paths:
        if not path.is_file():
            raise FileNotFoundError(path)
    raw_path = output_root / "_forged_iron_v004_1_v004_2_raw.png"
    badge_path = output_root / "_forged_iron_v004_1_v004_2_badges.png"
    board_path = output_root / "forged_iron_v004_1_v004_2_cycles_comparison.png"
    layout = []
    for row_index in range(len(ROWS)):
        for column_index in range(len(PANEL_ORDER)):
            layout.append(
                f"{column_index * args.resolution_x}_{row_index * args.resolution_y}"
            )
    stack = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            *sum((["-i", str(path)] for path in paths), []),
            "-filter_complex",
            f"xstack=inputs={len(paths)}:layout={'|'.join(layout)}[board]",
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(raw_path),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if stack.returncode != 0:
        raise RuntimeError("Paired proof stack failed: " + stack.stderr.strip())
    width = args.resolution_x * len(PANEL_ORDER)
    height = args.resolution_y * len(ROWS)
    sweep.write_grid_badges(
        badge_path,
        width,
        height,
        args.resolution_x,
        args.resolution_y,
        ("A", "B"),
        len(PANEL_ORDER),
    )
    overlay = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(raw_path),
            "-i",
            str(badge_path),
            "-filter_complex",
            "[0:v][1:v]overlay=0:0:format=auto[board]",
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(board_path),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if overlay.returncode != 0:
        raise RuntimeError("Paired proof badge overlay failed: " + overlay.stderr.strip())
    raw_path.unlink(missing_ok=True)
    badge_path.unlink(missing_ok=True)
    return {
        "path": str(board_path),
        "resolution": [width, height],
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "layout": "v004.1 row A above v004.2 row B; seven aligned columns",
        "row_legend": {
            "A": ROWS[0]["row_id"],
            "B": ROWS[1]["row_id"],
        },
        "column_legend": {
            str(index + 1): {"id": panel, "label": PANEL_LABELS[panel]}
            for index, panel in enumerate(PANEL_ORDER)
        },
    }


def verify_frozen_lanes(rows: dict[str, Any]) -> dict[str, Any]:
    control = rows[ROWS[0]["row_id"]]
    candidate = rows[ROWS[1]["row_id"]]
    components: dict[str, Any] = {}
    all_match = True
    for name in base.TARGET_OBJECTS:
        control_component = control["components"][name]
        candidate_component = candidate["components"][name]
        lane_matches = {
            lane: control_component[lane] == candidate_component[lane]
            for lane in (
                "base_colour_pixel_sha256",
                "roughness_pixel_sha256",
                "source_normal_pixel_sha256",
            )
        }
        all_match = all_match and all(lane_matches.values())
        components[name] = lane_matches
    normal_tile_match = (
        control["renders"]["source_normal"]["decoded_pixel_sha256"]
        == candidate["renders"]["source_normal"]["decoded_pixel_sha256"]
    )
    if not all_match or not normal_tile_match:
        raise RuntimeError("v004.2 paired proof found frozen lane drift")
    return {
        "all_component_frozen_lane_hashes_match": all_match,
        "source_normal_proof_decoded_pixels_identical": normal_tile_match,
        "components": components,
    }


def review_only(output_root: Path) -> None:
    manifest_path = output_root / "manifest.json"
    if not manifest_path.is_file():
        raise FileNotFoundError(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    expected_hashes: dict[str, str] = {
        manifest["board"]["path"]: manifest["board"]["sha256"],
    }
    for row in manifest["rows"].values():
        expected_hashes.update(
            {render["path"]: render["sha256"] for render in row["renders"].values()}
        )
    for path_string, expected in expected_hashes.items():
        path = Path(path_string)
        if not path.is_file() or sha256_file(path) != expected:
            raise RuntimeError(f"Proof changed before review registration: {path}")
    for row in ROWS:
        if sha256_file(row["blend"]) != manifest["source_hashes"]["after"][row["row_id"]]:
            raise RuntimeError(f"Source changed before review registration: {row['blend']}")
    if not REVIEW_PATH.is_file():
        raise FileNotFoundError(REVIEW_PATH)
    manifest["script"]["sha256"] = sha256_file(Path(__file__).resolve())
    manifest["review_document"] = {
        "path": str(REVIEW_PATH),
        "sha256": sha256_file(REVIEW_PATH),
    }
    manifest["visual_review"] = VISUAL_REVIEW
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(f"FORGED_IRON_V004_2_REVIEW={VISUAL_REVIEW['decision']}")


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if args.review_only:
        review_only(output_root)
        return
    for row in ROWS:
        for required in (row["blend"], row["manifest"]):
            if not required.is_file():
                raise FileNotFoundError(required)
    source_hashes_before = {row["row_id"]: sha256_file(row["blend"]) for row in ROWS}
    rows: dict[str, Any] = {}
    framings: dict[str, Any] = {}
    for row in ROWS:
        row_root = output_root / row["row_id"]
        row_root.mkdir(parents=True, exist_ok=True)
        rows[row["row_id"]], framings[row["row_id"]] = render_row(
            row,
            row_root,
            args,
        )
    if framings[ROWS[0]["row_id"]] != framings[ROWS[1]["row_id"]]:
        raise RuntimeError(f"v004.1/v004.2 consumer framing drifted: {framings}")
    source_hashes_after = {row["row_id"]: sha256_file(row["blend"]) for row in ROWS}
    if source_hashes_before != source_hashes_after:
        raise RuntimeError("Paired proof changed a source blend")
    frozen_lane_contract = verify_frozen_lanes(rows)
    board = assemble_board(output_root, args)
    manifest = {
        "schema": "iggy3d.forged_iron_v004_1_v004_2_cycles_proof.v1",
        "status": "USER_NOT_ACCEPTED_PROOF_ONLY",
        "render_engine": "CYCLES",
        "cycles_samples": args.samples,
        "persistent_render_data": False,
        "tile_resolution": [args.resolution_x, args.resolution_y],
        "consumer": framings[ROWS[0]["row_id"]],
        "rows": rows,
        "frozen_lane_contract": frozen_lane_contract,
        "board": board,
        "visual_review": VISUAL_REVIEW,
        "review_document": None,
        "source_hashes": {
            "before": source_hashes_before,
            "after": source_hashes_after,
            "sources_unchanged": source_hashes_before == source_hashes_after,
        },
        "script": {
            "path": str(Path(__file__).resolve()),
            "sha256": sha256_file(Path(__file__).resolve()),
        },
        "manual_acceptance_required": True,
        "uses_ai_generated_imagery": False,
        "uses_condition": False,
        "unreal_parity_verified": False,
        "limitations": [
            "This proof changes only the uniform direction-dependent intact-oxide response.",
            "The existing v004.1 broad normal remains frozen for comparison, not accepted.",
            "The pivot remains geometry-owned and is not repaired by this material response.",
            "Cycles proof does not establish Unreal parity, donor status, or user acceptance.",
        ],
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    )
    print(
        "FORGED_IRON_V004_2_CYCLES_PROOF="
        f"rows:{len(rows)},panels:{len(PANEL_ORDER)},sources_unchanged:true,"
        "frozen_lanes_match:true"
    )
    print(f"FORGED_IRON_V004_2_CYCLES_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
