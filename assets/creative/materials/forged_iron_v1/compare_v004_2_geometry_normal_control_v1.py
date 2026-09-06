#!/usr/bin/env python3
"""Compare saved v004.2 broad-normal response with geometry normals only."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Any

import bpy
from mathutils import Vector


MATERIAL_ROOT = Path(__file__).resolve().parent
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004_2.py"
SWEEP_SCRIPT = MATERIAL_ROOT / "compare_v004_1_reflection_routing_v1.py"
SOURCE_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004_2"
SOURCE_BLEND = SOURCE_ROOT / "forged_iron_connected_oxide_candidate_v004_2.blend"
SOURCE_MANIFEST = SOURCE_ROOT / "manifest.json"
INHERITED_CLAY_BOARD = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_v004_1_reflection_routing_sweep_v1"
    / "forged_iron_v004_1_geometry_moving_strip_board.png"
)
DEFAULT_OUTPUT = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_v004_2_geometry_normal_control_v1"
)
REVIEW_PATH = MATERIAL_ROOT / "V004_2_GEOMETRY_NORMAL_CONTROL_REVIEW.md"

SOURCE_GROUP = "IGGY_SH_ConnectedOxideForgedIron_v004_2"
SOURCE_MATERIAL_PREFIX = "IGGY_MAT_ConnectedOxideForgedIron_v004_2"
NORMAL_NODE = "Broad_Forging_Normal_Decode"
CONTROL_NORMAL_STRENGTH = 1.0
GEOMETRY_NORMAL_STRENGTH = 0.0
DIRECTION_MULTIPLIER_NODE = "Worked_Luster_Disabled"
DIRECTION_MULTIPLIER = 1.0
DIRECTION_AMOUNT = 0.28

ROWS = (
    {
        "row_id": "A_v004_2_control",
        "label": "A  V004.2 BROAD NORMAL",
        "normal_strength": CONTROL_NORMAL_STRENGTH,
    },
    {
        "row_id": "B_geometry_normals_only",
        "label": "B  GEOMETRY NORMALS ONLY",
        "normal_strength": GEOMETRY_NORMAL_STRENGTH,
    },
)
PANEL_ORDER = (
    "close_neutral",
    "grazing_left",
    "grazing_right",
    "pivot_close",
    "gameplay",
    "applied_normal_amount",
    "source_normal",
)
PANEL_LABELS = {
    "close_neutral": "CLOSE NEUTRAL",
    "grazing_left": "MOVING STRIP LEFT",
    "grazing_right": "MOVING STRIP RIGHT",
    "pivot_close": "PIVOT CLOSE",
    "gameplay": "GAMEPLAY DISTANCE",
    "applied_normal_amount": "APPLIED BROAD NORMAL",
    "source_normal": "FROZEN SOURCE NORMAL",
}
PHYSICAL_PANELS = (
    "close_neutral",
    "grazing_left",
    "grazing_right",
    "pivot_close",
    "gameplay",
)

# Replaced only after inspecting the hash-locked paired proof.
VISUAL_REVIEW = {
    "decision": "select_geometry_normals_only_recipe",
    "preferred_row": "B_geometry_normals_only",
    "advancement_scope": "disposable_normal_ownership_decision_only",
    "defect_ledger": [
        "Row B preserves the v004.2 broad intact-oxide value organization and direction-dependent highlight travel while removing faint cloudy and dent-like modulation from the long leaf planes.",
        "The opposed moving-strip views retain the same large highlight placement from both directions. B therefore removes a surface-normal owner rather than baking a preferred light direction or flattening the metal response.",
        "Aperture boundaries, round holes, leaf joins, bevel lands, and the moving-versus-fixed leaf division remain equally legible. Their response stays owned by geometry, not by the removed normal field.",
        "The pivot is cleaner in B: cylindrical highlight continuity remains, while the low-amplitude vertical mottling from the broad normal disappears. Bearing gaps and the smooth-cylinder construction limitation remain geometry-owned and unchanged.",
        "At gameplay distance the change nearly disappears, as intended. B is not dead or sterile there because base colour, roughness, metallic response, and the selected 0.28 construction-tangent response are still live and unchanged.",
        "The five physical comparisons measure 44.862448 dB close, 50.282430 dB strip-left, 48.363634 dB strip-right, 44.721258 dB pivot, and 51.861939 dB gameplay. This is a bounded cleanup, not a replacement appearance pass.",
        "All eight base-colour, roughness, source-normal, and direction-control float hashes match between rows. The decoded source-normal proof also matches; only its applied Strength changes from one to zero.",
        "The openwork silhouette can still produce repeated face-like readings, but it is literal geometry in this consumer and is outside this shader decision. The material does not add any diamond, face, stamp, scratch, pit, or condition motif.",
        "No matched professional-reference acceptance plate or Unreal parity exists. B selects only a normal-ownership recipe and cannot promote v004.2, establish acceptance, or register a donor.",
    ],
    "physical_panel_psnr_db": {
        "close_neutral": 44.862448,
        "grazing_left": 50.282430,
        "grazing_right": 48.363634,
        "pivot_close": 44.721258,
        "gameplay": 51.861939,
    },
    "strongest_visible_achievement": "cleaner geometry-owned leaf and pivot response without losing directional intact-metal behavior",
    "weakest_visible_area": "the actual hinge geometry still supplies smooth broad planes and repeated openwork silhouettes that the shader must not disguise",
    "repair_pass_count_since_v004_2": 1,
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


build = _load_module("iggy_v004_2_normal_control_build", BUILD_SCRIPT)
sweep = _load_module("iggy_v004_2_normal_control_sweep", SWEEP_SCRIPT)
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


def normal_strength(group: bpy.types.NodeTree) -> float:
    node = group.nodes.get(NORMAL_NODE)
    if node is None or node.bl_idname != "ShaderNodeNormalMap":
        raise RuntimeError(f"{group.name} lost Normal Map owner {NORMAL_NODE}")
    return float(node.inputs["Strength"].default_value)


def direction_multiplier(group: bpy.types.NodeTree) -> float:
    node = group.nodes.get(DIRECTION_MULTIPLIER_NODE)
    if node is None:
        raise RuntimeError(
            f"{group.name} lost direction multiplier {DIRECTION_MULTIPLIER_NODE}"
        )
    return float(node.inputs[1].default_value)


def topology_matches(
    source_signature: dict[str, Any],
    candidate_signature: dict[str, Any],
) -> bool:
    return all(
        source_signature[key] == candidate_signature[key]
        for key in (
            "node_count",
            "link_count",
            "interface_socket_count",
            "topology_sha256",
        )
    )


def build_disposable_geometry_normal_group(
    source_group: bpy.types.NodeTree,
) -> tuple[bpy.types.NodeTree, dict[str, Any]]:
    if normal_strength(source_group) != CONTROL_NORMAL_STRENGTH:
        raise RuntimeError("Saved v004.2 broad-normal strength is not one")
    if direction_multiplier(source_group) != DIRECTION_MULTIPLIER:
        raise RuntimeError("Saved v004.2 direction-response multiplier is not one")
    source_signature = build.validate_group(source_group, DIRECTION_MULTIPLIER)
    candidate_group = source_group.copy()
    candidate_group.name = "IGGY_SH_B_geometry_normals_only_Disposable"
    candidate_group["iggy_status"] = "EXPLORATORY_BROAD_NORMAL_REMOVAL_NOT_CANONICAL"
    candidate_group.nodes[NORMAL_NODE].inputs["Strength"].default_value = (
        GEOMETRY_NORMAL_STRENGTH
    )
    candidate_signature = build.validate_group(candidate_group, DIRECTION_MULTIPLIER)
    differences = build.group_default_differences(source_group, candidate_group)
    expected = [
        {
            "socket": "Broad_Forging_Normal_Decode:0:Strength",
            "source": CONTROL_NORMAL_STRENGTH,
            "candidate": GEOMETRY_NORMAL_STRENGTH,
        }
    ]
    if differences != expected:
        raise RuntimeError(f"Geometry-normal control changed unexpected defaults: {differences}")
    if not topology_matches(source_signature, candidate_signature):
        raise RuntimeError("Geometry-normal control changed shared-group topology")
    return candidate_group, {
        "source_group": source_group.name,
        "candidate_group": candidate_group.name,
        "source_signature": source_signature,
        "candidate_signature": candidate_signature,
        "topology_matches_source": True,
        "default_differences": differences,
    }


def component_record(
    obj: bpy.types.Object,
    material: bpy.types.Material,
) -> dict[str, Any]:
    images = build.material_images(material)
    luster_values = build.image_values(images["luster_control"])
    return {
        "object": obj.name,
        "material": material.name,
        "mesh_sha256": build.mesh_sha256(obj),
        "uv_layers": sorted(layer.name for layer in obj.data.uv_layers),
        "image_pixel_sha256": {
            lane: build.image_pixel_sha256(image) for lane, image in images.items()
        },
        "images": {lane: image.name for lane, image in images.items()},
        "all_images_packed": all(
            image.packed_file is not None for image in images.values()
        ),
        "luster_amount_range": [
            round(float(luster_values[..., 0].min()), 6),
            round(float(luster_values[..., 0].max()), 6),
        ],
    }


def retarget_material_copy(
    obj: bpy.types.Object,
    source_group: bpy.types.NodeTree,
    candidate_group: bpy.types.NodeTree,
) -> bpy.types.Material:
    source_material = obj.data.materials[0]
    if not source_material.name.startswith(SOURCE_MATERIAL_PREFIX):
        raise RuntimeError(
            f"{obj.name} has unexpected saved material {source_material.name}"
        )
    material = source_material.copy()
    material.name = f"IGGY_MAT_B_GeometryNormalsOnly_{obj.name}_Disposable"
    group_instances = [
        node
        for node in material.node_tree.nodes
        if node.bl_idname == "ShaderNodeGroup" and node.node_tree == source_group
    ]
    if len(group_instances) != 1:
        raise RuntimeError(
            f"{material.name} has {len(group_instances)} saved-group owners instead of one"
        )
    group_instances[0].node_tree = candidate_group
    base.set_material(obj, material)
    return material


def build_constant_scalar_material(name: str, value: float) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    output.name = "Output"
    emission = tree.nodes.new("ShaderNodeEmission")
    emission.name = "Absolute_Zero_To_One_Emission"
    emission.inputs["Color"].default_value = (value, value, value, 1.0)
    emission.inputs["Strength"].default_value = 1.0
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def render_row(
    row: dict[str, Any],
    output_root: Path,
    args: argparse.Namespace,
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any] | None]:
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
    scene = bpy.context.scene
    sweep.configure_scene(scene, args)
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    lights = {name: bpy.data.objects[name] for name in presentation.TARGET_LIGHT_NAMES}
    presentation.isolate_actual_hinge(targets, lights)

    source_group = bpy.data.node_groups.get(SOURCE_GROUP)
    if source_group is None:
        raise RuntimeError(f"Saved v004.2 lost {SOURCE_GROUP}")
    source_signature = build.validate_group(source_group, DIRECTION_MULTIPLIER)
    if normal_strength(source_group) != CONTROL_NORMAL_STRENGTH:
        raise RuntimeError("Saved v004.2 source normal strength drifted")

    group_contract: dict[str, Any] | None = None
    physical_materials: dict[str, bpy.types.Material] = {}
    if row["normal_strength"] == GEOMETRY_NORMAL_STRENGTH:
        candidate_group, group_contract = build_disposable_geometry_normal_group(
            source_group
        )
        for obj in targets:
            physical_materials[obj.name] = retarget_material_copy(
                obj,
                source_group,
                candidate_group,
            )
    else:
        for obj in targets:
            material = obj.data.materials[0]
            if not material.name.startswith(SOURCE_MATERIAL_PREFIX):
                raise RuntimeError(
                    f"{obj.name} has unexpected saved material {material.name}"
                )
            physical_materials[obj.name] = material

    components = {
        obj.name: component_record(obj, physical_materials[obj.name]) for obj in targets
    }
    if any(
        component["luster_amount_range"] != [DIRECTION_AMOUNT, DIRECTION_AMOUNT]
        for component in components.values()
    ):
        raise RuntimeError(f"{row['row_id']} changed v004.2 direction amount")

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
    amount_material = build_constant_scalar_material(
        f"IGGY_MAT_{row['row_id']}_AppliedNormalAmount",
        row["normal_strength"],
    )
    for obj in targets:
        base.set_material(obj, amount_material)
    renders["applied_normal_amount"] = render_path(
        scene,
        output_root / "applied_normal_amount.png",
    )

    for obj in targets:
        normal = build.material_images(physical_materials[obj.name])["combined_normal"]
        base.set_material(
            obj,
            base.build_lane_material(
                f"IGGY_MAT_{row['row_id']}_{obj.name}_SourceNormalProof",
                normal,
            ),
        )
    renders["source_normal"] = render_path(scene, output_root / "source_normal.png")

    if normal_strength(source_group) != CONTROL_NORMAL_STRENGTH:
        raise RuntimeError(f"{row['row_id']} mutated the saved source group")
    if direction_multiplier(source_group) != DIRECTION_MULTIPLIER:
        raise RuntimeError(f"{row['row_id']} changed direction response")

    framing = {
        "center": list(center),
        "span": list(span),
        "close_center": list(close_center),
        "pivot_center": list(pivot_center),
    }
    return (
        {
            "row_id": row["row_id"],
            "label": row["label"],
            "normal_strength": row["normal_strength"],
            "source_group": source_group.name,
            "source_group_signature": source_signature,
            "source_group_strength_after_row": normal_strength(source_group),
            "direction_multiplier_after_row": direction_multiplier(source_group),
            "components": components,
            "renders": {panel: renders[panel] for panel in PANEL_ORDER},
        },
        framing,
        group_contract,
    )


def assemble_board(output_root: Path, args: argparse.Namespace) -> dict[str, Any]:
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
    raw_path = output_root / "_geometry_normal_control_raw.png"
    badge_path = output_root / "_geometry_normal_control_badges.png"
    board_path = output_root / "forged_iron_v004_2_geometry_normal_control_board.png"
    layout = [
        f"{column_index * args.resolution_x}_{row_index * args.resolution_y}"
        for row_index in range(len(ROWS))
        for column_index in range(len(PANEL_ORDER))
    ]
    command = [
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
    ]
    stack = subprocess.run(command, capture_output=True, text=True, check=False)
    if stack.returncode != 0:
        raise RuntimeError("Geometry-normal proof stack failed: " + stack.stderr.strip())
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
        raise RuntimeError(
            "Geometry-normal proof badge overlay failed: " + overlay.stderr.strip()
        )
    raw_path.unlink(missing_ok=True)
    badge_path.unlink(missing_ok=True)
    return {
        "path": str(board_path),
        "resolution": [width, height],
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "layout": "v004.2 broad-normal row A above geometry-normals-only row B",
        "row_legend": {"A": ROWS[0]["row_id"], "B": ROWS[1]["row_id"]},
        "column_legend": {
            str(index + 1): {"id": panel, "label": PANEL_LABELS[panel]}
            for index, panel in enumerate(PANEL_ORDER)
        },
    }


def psnr_db(reference: Path, candidate: Path) -> float | None:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for physical-panel comparison")
    result = subprocess.run(
        [
            ffmpeg,
            "-hide_banner",
            "-i",
            str(reference),
            "-i",
            str(candidate),
            "-lavfi",
            "psnr",
            "-f",
            "null",
            "-",
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError("PSNR comparison failed: " + result.stderr.strip())
    match = re.search(r"average:([0-9.]+|inf)", result.stderr)
    if match is None:
        raise RuntimeError("PSNR output did not contain an average")
    return None if match.group(1) == "inf" else float(match.group(1))


def verify_frozen_lanes(rows: dict[str, Any]) -> dict[str, Any]:
    control = rows[ROWS[0]["row_id"]]
    candidate = rows[ROWS[1]["row_id"]]
    component_matches: dict[str, Any] = {}
    all_match = True
    for name in base.TARGET_OBJECTS:
        control_component = control["components"][name]
        candidate_component = candidate["components"][name]
        lane_matches = {
            lane: (
                control_component["image_pixel_sha256"][lane]
                == candidate_component["image_pixel_sha256"][lane]
            )
            for lane in (
                "oxide_base",
                "oxide_roughness",
                "combined_normal",
                "luster_control",
            )
        }
        mesh_matches = control_component["mesh_sha256"] == candidate_component["mesh_sha256"]
        uv_matches = control_component["uv_layers"] == candidate_component["uv_layers"]
        all_match = all_match and all(lane_matches.values()) and mesh_matches and uv_matches
        component_matches[name] = {
            "lanes": lane_matches,
            "mesh_matches": mesh_matches,
            "uv_layers_match": uv_matches,
        }
    source_normal_proof_match = (
        control["renders"]["source_normal"]["decoded_pixel_sha256"]
        == candidate["renders"]["source_normal"]["decoded_pixel_sha256"]
    )
    if not all_match or not source_normal_proof_match:
        raise RuntimeError("Geometry-normal control changed a frozen lane or consumer")
    return {
        "all_component_lane_hashes_match": all_match,
        "source_normal_proof_decoded_pixels_identical": source_normal_proof_match,
        "components": component_matches,
    }


def review_only(output_root: Path) -> None:
    manifest_path = output_root / "manifest.json"
    if not manifest_path.is_file():
        raise FileNotFoundError(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    expected_hashes = {manifest["board"]["path"]: manifest["board"]["sha256"]}
    for row in manifest["rows"].values():
        expected_hashes.update(
            {render["path"]: render["sha256"] for render in row["renders"].values()}
        )
    for path_string, expected in expected_hashes.items():
        path = Path(path_string)
        if not path.is_file() or sha256_file(path) != expected:
            raise RuntimeError(f"Proof changed before review registration: {path}")
    if sha256_file(SOURCE_BLEND) != manifest["source_hashes"]["after"]:
        raise RuntimeError("Saved v004.2 changed before review registration")
    if not REVIEW_PATH.is_file():
        raise FileNotFoundError(REVIEW_PATH)
    manifest["script"]["sha256"] = sha256_file(Path(__file__).resolve())
    manifest["review_document"] = {
        "path": str(REVIEW_PATH),
        "sha256": sha256_file(REVIEW_PATH),
    }
    manifest["visual_review"] = VISUAL_REVIEW
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(f"FORGED_IRON_V004_2_NORMAL_REVIEW={VISUAL_REVIEW['decision']}")


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if args.review_only:
        review_only(output_root)
        return
    for required in (SOURCE_BLEND, SOURCE_MANIFEST, INHERITED_CLAY_BOARD):
        if not required.is_file():
            raise FileNotFoundError(required)
    source_hash_before = sha256_file(SOURCE_BLEND)
    rows: dict[str, Any] = {}
    framings: dict[str, Any] = {}
    group_contract: dict[str, Any] | None = None
    for row in ROWS:
        row_root = output_root / row["row_id"]
        row_root.mkdir(parents=True, exist_ok=True)
        row_record, framing, row_group_contract = render_row(row, row_root, args)
        rows[row["row_id"]] = row_record
        framings[row["row_id"]] = framing
        if row_group_contract is not None:
            group_contract = row_group_contract
    if group_contract is None:
        raise RuntimeError("Geometry-normal control did not construct its disposable group")
    if framings[ROWS[0]["row_id"]] != framings[ROWS[1]["row_id"]]:
        raise RuntimeError(f"Geometry-normal consumer framing drifted: {framings}")
    source_hash_after = sha256_file(SOURCE_BLEND)
    if source_hash_before != source_hash_after:
        raise RuntimeError("Geometry-normal control changed saved v004.2")

    frozen_lane_contract = verify_frozen_lanes(rows)
    physical_psnr = {
        panel: psnr_db(
            output_root / ROWS[0]["row_id"] / f"{panel}.png",
            output_root / ROWS[1]["row_id"] / f"{panel}.png",
        )
        for panel in PHYSICAL_PANELS
    }
    board = assemble_board(output_root, args)
    manifest = {
        "schema": "iggy3d.forged_iron_v004_2_geometry_normal_control.v1",
        "status": "EXPLORATORY_BROAD_NORMAL_REMOVAL_NOT_CANONICAL",
        "render_engine": "CYCLES",
        "cycles_samples": args.samples,
        "persistent_render_data": False,
        "tile_resolution": [args.resolution_x, args.resolution_y],
        "consumer": framings[ROWS[0]["row_id"]],
        "rows": rows,
        "group_contract": group_contract,
        "frozen_lane_contract": frozen_lane_contract,
        "physical_panel_psnr_db": physical_psnr,
        "inherited_geometry_proof": {
            "path": str(INHERITED_CLAY_BOARD),
            "sha256": sha256_file(INHERITED_CLAY_BOARD),
            "regenerated": False,
            "claim": "hash_registered_geometry_baseline_only",
        },
        "board": board,
        "visual_review": VISUAL_REVIEW,
        "review_document": None,
        "source_hashes": {
            "before": source_hash_before,
            "after": source_hash_after,
            "source_manifest_sha256": sha256_file(SOURCE_MANIFEST),
            "sources_unchanged": source_hash_before == source_hash_after,
        },
        "script": {
            "path": str(Path(__file__).resolve()),
            "sha256": sha256_file(Path(__file__).resolve()),
        },
        "saved_blend_created": False,
        "manual_acceptance_established": False,
        "uses_ai_generated_imagery": False,
        "uses_new_texture_or_pattern": False,
        "uses_condition": False,
        "unreal_parity_verified": False,
        "limitations": [
            "This proof changes only one in-memory Normal Map Strength default.",
            "The packed source normal remains present and hash-identical in both rows.",
            "No saved candidate or material catalog promotion is created.",
            "The inherited clay proof is hash-registered, not regenerated.",
            "Cycles proof does not establish Unreal parity, donor status, or user acceptance.",
        ],
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    )
    print(
        "FORGED_IRON_V004_2_GEOMETRY_NORMAL_CONTROL="
        f"rows:{len(rows)},panels:{len(PANEL_ORDER)},source_unchanged:true,"
        "frozen_lanes_match:true,saved_blend:false"
    )
    print(f"FORGED_IRON_V004_2_GEOMETRY_NORMAL_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
