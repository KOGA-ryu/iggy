#!/usr/bin/env python3
"""Compare bounded palette, roughness, and broad-normal calibrations for v004."""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from typing import Any

import bpy
from mathutils import Vector
import numpy as np


MATERIAL_ROOT = Path(__file__).resolve().parent
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004.py"
PROOF_SCRIPT = MATERIAL_ROOT / "render_connected_oxide_candidate_cycles_v004.py"
PROFILE_PATH = MATERIAL_ROOT / "profiles" / "forged_iron_v1.json"
SOURCE_BLEND = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_connected_oxide_candidate_v004"
    / "forged_iron_connected_oxide_candidate_v004.blend"
)
DEFAULT_OUTPUT = MATERIAL_ROOT / "output" / "forged_iron_v004_optical_calibration_v1"
EXPOSED_CONDUCTOR = 0.0

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
    "independent_roughness": "ROUGHNESS",
    "normal": "NORMAL",
}


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


builder = _load_module("iggy_v004_calibration_builder", BUILD_SCRIPT)
proof = _load_module("iggy_v004_calibration_proof", PROOF_SCRIPT)
selection = builder.selection
base = builder.base
common = selection.common
presentation = selection.presentation


@dataclass(frozen=True)
class CalibrationRecipe:
    candidate_id: str
    label: str
    thin_hex: str | None
    thick_hex: str | None
    roughness_base: float
    thermal_amplitude: float
    medium_amplitude: float
    grain_amplitude: float
    roughness_minimum: float
    roughness_maximum: float
    broad_normal_multiplier: float


CANDIDATES = (
    CalibrationRecipe(
        "A_current_v004_control",
        "A  CURRENT v004 CONTROL",
        None,
        None,
        0.715,
        0.040,
        0.032,
        0.010,
        0.665,
        0.765,
        1.00,
    ),
    CalibrationRecipe(
        "B_audited_dark_half",
        "B  AUDITED DARK HALF",
        "#252a31",
        "#41464f",
        0.715,
        0.040,
        0.040,
        0.010,
        0.640,
        0.800,
        1.35,
    ),
    CalibrationRecipe(
        "C_audited_compressed_cool",
        "C  AUDITED COMPRESSED COOL",
        "#252a31",
        "#353b46",
        0.680,
        0.035,
        0.055,
        0.012,
        0.600,
        0.760,
        1.75,
    ),
    CalibrationRecipe(
        "D_audited_cool_to_warm",
        "D  AUDITED COOL TO WARM",
        "#292e36",
        "#574f4b",
        0.735,
        0.050,
        0.045,
        0.010,
        0.660,
        0.820,
        1.50,
    ),
)

VISUAL_REVIEW = {
    "decision": "select_C",
    "ranking": [
        "C_audited_compressed_cool",
        "B_audited_dark_half",
        "D_audited_cool_to_warm",
        "A_current_v004_control",
    ],
    "recommended_candidate": "C_audited_compressed_cool",
    "advancement_scope": "calibration_recipe_only_not_material_acceptance",
    "defect_ledger": [
        "A remains too close to black in the base-colour proof and leaves the lit material excessively dependent on the lamps.",
        "B restores the audited cool forge-skin range and remains quiet, but its roughness and normal handoff are less decisive than C.",
        "C preserves a compressed audited cool range while supplying the strongest bounded independent roughness and broad-normal response without spots, stripes, symbols, or warm condition cues.",
        "D introduces a localized warm passage around the hinge and moving leaf that reads closer to an oxidation hotspot or broad cloud than the requested clean intact core.",
        "All physical rows remain restrained at gameplay distance; none introduces a pattern that reads before geometry and the broad dark material group.",
        "C still leaves the pivot smoother than the target; selection does not establish finished component-scale forging response.",
        "The first C render was invalidated and rerun because Cycles persistent data retained temporary material-image bindings between rows; the corrected harness disables persistent data and uses unique candidate material names.",
    ],
    "manual_acceptance_established": False,
}


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=640)
    parser.add_argument("--resolution-y", type=int, default=220)
    parser.add_argument("--samples", type=int, default=32)
    parser.add_argument("--board-resolution-x", type=int, default=4480)
    parser.add_argument("--board-resolution-y", type=int, default=880)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def hex_to_srgb(value: str) -> np.ndarray:
    text = value.removeprefix("#")
    if len(text) != 6:
        raise ValueError(f"Invalid RGB anchor {value}")
    return np.asarray(
        [int(text[index : index + 2], 16) / 255.0 for index in (0, 2, 4)],
        dtype=np.float32,
    )


def audited_palette() -> set[str]:
    profile = json.loads(PROFILE_PATH.read_text())
    return {value.lower() for value in profile["palette"]["forge_skin_srgb"]}


def selected_recipe():
    return next(
        recipe
        for recipe in selection.CANDIDATES
        if recipe.candidate_id == builder.SELECTED_RECIPE_ID
    )


def build_calibrated_fields(
    frame: dict[str, Any],
    component_name: str,
    recipe: CalibrationRecipe,
) -> dict[str, np.ndarray]:
    fields = selection.build_candidate_fields(frame, component_name, selected_recipe())
    if recipe.candidate_id == "A_current_v004_control":
        return fields
    if recipe.thin_hex is None or recipe.thick_hex is None:
        raise RuntimeError("calibrated row is missing audited palette anchors")
    palette = audited_palette()
    for anchor in (recipe.thin_hex.lower(), recipe.thick_hex.lower()):
        if anchor not in palette:
            raise RuntimeError(f"Calibration anchor is not audited: {anchor}")
    thermal_response = fields["thermal_response"]
    connected_medium = fields["connected_medium"]
    aggregate_grain = fields["aggregate_grain"]
    thin = hex_to_srgb(recipe.thin_hex)
    thick = hex_to_srgb(recipe.thick_hex)
    oxide_srgb = common.mix(
        np.broadcast_to(thin, thermal_response.shape + (3,)),
        np.broadcast_to(thick, thermal_response.shape + (3,)),
        thermal_response,
    )
    fields["oxide_base_linear"] = common.srgb_to_linear(oxide_srgb).astype(np.float32)
    fields["oxide_roughness"] = np.clip(
        recipe.roughness_base
        + (thermal_response - 0.5) * recipe.thermal_amplitude
        + connected_medium * recipe.medium_amplitude
        + aggregate_grain * recipe.grain_amplitude,
        recipe.roughness_minimum,
        recipe.roughness_maximum,
    ).astype(np.float32)
    length_m, width_m = frame["span_m"]
    resolution_y, resolution_x = fields["oxide_roughness"].shape
    fields["combined_normal"] = common.height_to_normal_nonperiodic(
        fields["broad_forging_height_m"] * recipe.broad_normal_multiplier,
        meters_per_pixel_x=length_m / (resolution_x - 1),
        meters_per_pixel_y=width_m / (resolution_y - 1),
    )
    if float(fields["oxide_coverage"].min()) != 1.0:
        raise RuntimeError("calibration lost complete oxide coverage")
    if float(fields["oxide_metalness"].max()) != 0.0:
        raise RuntimeError("calibration made intact oxide conductive")
    if float(np.abs(fields["oxide_surface_height_m"]).max()) != 0.0:
        raise RuntimeError("calibration changed oxide height")
    if float(fields["luster_control"][..., 0].max()) != 0.0:
        raise RuntimeError("calibration enabled worked luster")
    return fields


def configure_scene(scene: bpy.types.Scene, args: argparse.Namespace) -> None:
    scene.render.engine = "CYCLES"
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
    # Candidate datablocks are deliberately created and removed between rows.
    # Persistent render caches may retain a deleted material/image binding and
    # launder it into the next row, so this disposable matrix must rebuild the
    # render dependency state for every proof.
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


def render_physical_panels(
    scene: bpy.types.Scene,
    lights: dict[str, bpy.types.Object],
    center: Vector,
    close_center: Vector,
    span: Vector,
    output_root: Path,
) -> dict[str, dict[str, Any]]:
    renders: dict[str, dict[str, Any]] = {}
    presentation.set_neutral_lights(lights, close_center, close=True)
    presentation.front_camera(scene.camera, close_center, 2.05)
    renders["close_neutral"] = render_path(scene, output_root / "close_neutral.png")

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


def control_images(obj: bpy.types.Object) -> dict[str, bpy.types.Image]:
    material = obj.data.materials[0]
    names = {
        "oxide_base": "Continuous_Oxide_Optical_Colour",
        "oxide_roughness": "Independent_Oxide_Roughness",
        "combined_normal": "Broad_Forging_Normal_Only",
        "luster_control": "Explicit_Zero_Worked_Luster_Control",
    }
    result = {}
    for key, node_name in names.items():
        node = material.node_tree.nodes.get(node_name)
        if node is None or node.image is None:
            raise RuntimeError(f"Control material lost {node_name}")
        result[key] = node.image
    return result


def make_images(
    component_name: str,
    recipe: CalibrationRecipe,
    fields: dict[str, np.ndarray],
) -> dict[str, bpy.types.Image]:
    prefix = f"IGGY_IMG_{recipe.candidate_id}_{component_name.replace('.', '_')}"
    return {
        "oxide_base": base.make_float_image(
            f"{prefix}_OxideBase",
            fields["oxide_base_linear"],
            color=True,
        ),
        "oxide_roughness": base.make_float_image(
            f"{prefix}_OxideRoughness",
            fields["oxide_roughness"],
            color=False,
        ),
        "combined_normal": base.make_float_image(
            f"{prefix}_BroadForgingNormal",
            fields["combined_normal"] * 0.5 + 0.5,
            color=True,
        ),
        "luster_control": base.make_float_image(
            f"{prefix}_LusterControl",
            fields["luster_control"],
            color=True,
        ),
    }


def lane_materials_for_images(
    component_name: str,
    recipe: CalibrationRecipe,
    images: dict[str, bpy.types.Image],
) -> dict[str, bpy.types.Material]:
    return {
        "base_colour": base.build_lane_material(
            f"IGGY_MAT_{recipe.candidate_id}_{component_name}_BaseProof",
            images["oxide_base"],
        ),
        "independent_roughness": base.build_lane_material(
            f"IGGY_MAT_{recipe.candidate_id}_{component_name}_RoughProof",
            images["oxide_roughness"],
        ),
        "normal": base.build_lane_material(
            f"IGGY_MAT_{recipe.candidate_id}_{component_name}_NormalProof",
            images["combined_normal"],
        ),
    }


def render_diagnostics(
    scene: bpy.types.Scene,
    lights: dict[str, bpy.types.Object],
    targets: list[bpy.types.Object],
    center: Vector,
    span: Vector,
    lanes: dict[str, dict[str, bpy.types.Material]],
    output_root: Path,
) -> dict[str, dict[str, Any]]:
    renders: dict[str, dict[str, Any]] = {}
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    presentation.set_neutral_lights(lights, center)
    for panel_id in ("base_colour", "independent_roughness", "normal"):
        for obj in targets:
            base.set_material(obj, lanes[obj.name][panel_id])
        renders[panel_id] = render_path(scene, output_root / f"{panel_id}.png")
    return renders


def cleanup_candidate_resources(
    targets: list[bpy.types.Object],
    control_materials: dict[str, bpy.types.Material],
    materials: list[bpy.types.Material],
    images: list[bpy.types.Image],
) -> None:
    material_names = [material.name for material in materials]
    image_names = [image.name for image in images]
    for obj in targets:
        base.set_material(obj, control_materials[obj.name])
    for material in materials:
        if material.name in bpy.data.materials:
            bpy.data.materials.remove(material, do_unlink=True)
    for image in images:
        if image.name in bpy.data.images:
            bpy.data.images.remove(image, do_unlink=True)
    if any(name in bpy.data.materials for name in material_names):
        raise RuntimeError("calibration material cleanup failed")
    if any(name in bpy.data.images for name in image_names):
        raise RuntimeError("calibration image cleanup failed")


def scalar_values(
    fields: dict[str, dict[str, np.ndarray]],
    key: str,
) -> np.ndarray:
    return np.concatenate([value[key].reshape(-1) for value in fields.values()])


def build_board(
    scene: bpy.types.Scene,
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    camera_data = bpy.data.cameras.new("IGGY_CAM_v004_OpticalCalibrationBoard")
    camera = bpy.data.objects.new("IGGY_CAM_v004_OpticalCalibrationBoard", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 28.0
    camera.location = (0.0, 0.0, 10.0)
    presentation.point_object(camera, Vector((0.0, 0.0, 0.0)))
    proof.configure_board_scene(scene, args.board_resolution_x, args.board_resolution_y)
    black = presentation.make_emission_material(
        "IGGY_MAT_OpticalCalibrationLabel",
        colour=(0.004, 0.005, 0.007, 1.0),
    )
    white = presentation.make_emission_material(
        "IGGY_MAT_OpticalCalibrationText",
        colour=(0.86, 0.90, 0.96, 1.0),
    )
    panel_width = 4.0
    panel_height = 28.0 / (args.board_resolution_x / args.board_resolution_y) / 4.0
    for row, recipe in enumerate(CANDIDATES):
        for column, panel_id in enumerate(PANEL_ORDER):
            panel_path = output_root / recipe.candidate_id / f"{panel_id}.png"
            if not panel_path.is_file():
                raise FileNotFoundError(panel_path)
            center_x = (column - 3.0) * panel_width
            center_y = (1.5 - row) * panel_height
            image = bpy.data.images.load(str(panel_path), check_existing=False)
            image_material = presentation.make_emission_material(
                f"IGGY_MAT_Board_{recipe.candidate_id}_{panel_id}",
                image=image,
            )
            presentation.add_panel_plane(
                f"Board_{recipe.candidate_id}_{panel_id}",
                center_x,
                center_y,
                panel_width - 0.035,
                panel_height - 0.035,
                0.0,
                image_material,
            )
            presentation.add_panel_plane(
                f"BoardLabel_{recipe.candidate_id}_{panel_id}",
                center_x,
                center_y + panel_height * 0.40,
                panel_width - 0.10,
                panel_height * 0.16,
                0.10,
                black,
            )
            font_curve = bpy.data.curves.new(
                f"BoardText_{recipe.candidate_id}_{panel_id}",
                type="FONT",
            )
            font_curve.body = f"{recipe.candidate_id[0]}  {PANEL_LABELS[panel_id]}"
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
            font_obj.data.materials.append(white)
    board_path = output_root / "forged_iron_v004_optical_calibration_board.png"
    scene.render.filepath = str(board_path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(board_path),
        "resolution": [args.board_resolution_x, args.board_resolution_y],
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "row_order": [recipe.candidate_id for recipe in CANDIDATES],
        "column_order": list(PANEL_ORDER),
    }


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if not SOURCE_BLEND.is_file() or not PROFILE_PATH.is_file():
        raise FileNotFoundError("v004 candidate or audited profile is missing")
    source_hash_before = sha256_file(SOURCE_BLEND)
    script_hash = sha256_file(Path(__file__).resolve())
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
    scene = bpy.context.scene
    configure_scene(scene, args)
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    lights = {name: bpy.data.objects[name] for name in presentation.TARGET_LIGHT_NAMES}
    presentation.isolate_actual_hinge(targets, lights)
    frames = {obj.name: base.install_component_frames(obj) for obj in targets}
    control_materials = {obj.name: obj.data.materials[0] for obj in targets}
    shared_group = bpy.data.node_groups.get(builder.GROUP_NAME)
    if shared_group is None:
        raise RuntimeError("v004 shared group is missing")
    minimum, maximum = presentation.object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    close_center = Vector((0.72, center.y, center.z))
    candidate_manifest: dict[str, Any] = {}

    for recipe in CANDIDATES:
        candidate_root = output_root / recipe.candidate_id
        candidate_root.mkdir(parents=True, exist_ok=True)
        fields_by_object: dict[str, dict[str, np.ndarray]] = {}
        physical: dict[str, bpy.types.Material] = {}
        lanes: dict[str, dict[str, bpy.types.Material]] = {}
        tracked_materials: list[bpy.types.Material] = []
        tracked_images: list[bpy.types.Image] = []
        for obj in targets:
            fields = build_calibrated_fields(frames[obj.name], obj.name, recipe)
            fields_by_object[obj.name] = fields
            if recipe.candidate_id == "A_current_v004_control":
                images = control_images(obj)
                physical[obj.name] = control_materials[obj.name]
            else:
                images = make_images(obj.name, recipe, fields)
                tracked_images.extend(images.values())
                material, _counts = builder.build_component_material(obj, shared_group, images)
                material.name = f"IGGY_MAT_{recipe.candidate_id}__{obj.name}"
                physical[obj.name] = material
                tracked_materials.append(material)
            object_lanes = lane_materials_for_images(obj.name, recipe, images)
            lanes[obj.name] = object_lanes
            tracked_materials.extend(object_lanes.values())

        for obj in targets:
            base.set_material(obj, physical[obj.name])
        renders = render_physical_panels(
            scene,
            lights,
            center,
            close_center,
            span,
            candidate_root,
        )
        renders.update(
            render_diagnostics(
                scene,
                lights,
                targets,
                center,
                span,
                lanes,
                candidate_root,
            )
        )
        coverage = scalar_values(fields_by_object, "oxide_coverage")
        metalness = scalar_values(fields_by_object, "oxide_metalness")
        exposure = scalar_values(fields_by_object, "exposure")
        oxide_height = scalar_values(fields_by_object, "oxide_surface_height_m")
        roughness = scalar_values(fields_by_object, "oxide_roughness")
        luster = np.concatenate(
            [value["luster_control"][..., 0].reshape(-1) for value in fields_by_object.values()]
        )
        candidate_manifest[recipe.candidate_id] = {
            "label": recipe.label,
            "recipe": asdict(recipe),
            "renders": {panel: renders[panel] for panel in PANEL_ORDER},
            "coverage_range": [float(coverage.min()), float(coverage.max())],
            "metalness_maximum": float(metalness.max()),
            "exposure_maximum": float(exposure.max()),
            "oxide_height_absolute_maximum_m": float(np.abs(oxide_height).max()),
            "roughness_range": [float(roughness.min()), float(roughness.max())],
            "luster_amount_range": [float(luster.min()), float(luster.max())],
            "resources_released_after_render": True,
        }
        cleanup_candidate_resources(
            targets,
            control_materials,
            tracked_materials,
            tracked_images,
        )

    source_hash_after = sha256_file(SOURCE_BLEND)
    source_unchanged = source_hash_before == source_hash_after
    if not source_unchanged:
        raise RuntimeError("calibration changed the saved v004 candidate")
    board = build_board(scene, output_root, args)
    manifest = {
        "schema": "iggy3d.forged_iron_v004_optical_calibration.v1",
        "status": "EXPLORATORY_CALIBRATION_NOT_CANONICAL",
        "render_engine": "CYCLES",
        "cycles_samples": args.samples,
        "tile_resolution": [args.resolution_x, args.resolution_y],
        "source": {
            "path": str(SOURCE_BLEND),
            "sha256_before": source_hash_before,
            "sha256_after": source_hash_after,
        },
        "script": {"path": str(Path(__file__).resolve()), "sha256": script_hash},
        "profile": {"path": str(PROFILE_PATH), "sha256": sha256_file(PROFILE_PATH)},
        "candidates": candidate_manifest,
        "board": board,
        "visual_review": VISUAL_REVIEW,
        "source_unchanged": source_unchanged,
        "uses_ai_generated_imagery": False,
        "uses_condition": False,
        "unreal_parity_verified": False,
        "limitations": [
            "This board calibrates three existing response envelopes only.",
            "A selected row remains a recipe and does not modify the saved v004 candidate.",
            "Cycles selection does not establish user acceptance or Unreal parity.",
        ],
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    )
    print(
        "FORGED_IRON_V004_OPTICAL_CALIBRATION="
        f"rows:{len(CANDIDATES)},columns:{len(PANEL_ORDER)},source_unchanged:{source_unchanged}"
    )
    print(f"FORGED_IRON_V004_OPTICAL_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
