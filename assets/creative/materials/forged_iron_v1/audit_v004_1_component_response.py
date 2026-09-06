#!/usr/bin/env python3
"""Audit v004.1 response ownership across leaf, rolled-eye, and pintle stock."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Any

import bpy
import numpy as np


MATERIAL_ROOT = Path(__file__).resolve().parent
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004_1.py"
RESEARCH_PATH = MATERIAL_ROOT / "COMPONENT_RESPONSE_AUDIT.md"
SOURCE_BLEND = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_connected_oxide_candidate_v004_1"
    / "forged_iron_connected_oxide_candidate_v004_1.blend"
)
DEFAULT_OUTPUT = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_v004_1_component_response_audit_v1"
)

STOCK_LINEAGES = {
    "moving_leaf_stock": (
        "SM_GH018_MovingLeaf_Openwork",
        "MovingKnuckle_01",
        "MovingKnuckle_03",
        "MovingKnuckle_05",
    ),
    "fixed_leaf_stock": (
        "SM_GH018_FixedLeaf",
        "FixedKnuckle_02",
        "FixedKnuckle_04",
    ),
    "pintle_stock": ("SM_GH018_Pintle",),
}

CONSTRUCTION_CLASSES = {
    "SM_GH018_FixedLeaf": "forged_flat_strap",
    "SM_GH018_MovingLeaf_Openwork": "forged_flat_strap",
    "SM_GH018_Pintle": "separate_forged_round_pin",
    "MovingKnuckle_01": "rolled_eye_from_parent_leaf",
    "FixedKnuckle_02": "rolled_eye_from_parent_leaf",
    "MovingKnuckle_03": "rolled_eye_from_parent_leaf",
    "FixedKnuckle_04": "rolled_eye_from_parent_leaf",
    "MovingKnuckle_05": "rolled_eye_from_parent_leaf",
}

SOURCE_LEDGER = (
    {
        "title": "Forge Work",
        "author": "William L. Ilgen",
        "year": 1912,
        "url": "https://www.gutenberg.org/files/53854/53854-h/53854-h.htm",
        "authority": "public-domain forging-instruction manual",
        "observation": (
            "Upright blows draw flat stock and make smooth surfaces; round stock "
            "is progressively worked square, octagonal, and round, then finished "
            "with light blows and continuous rotation or swages."
        ),
        "eligible_transfer": (
            "Mostly smoothed overlapping flat-stock response and restrained axial "
            "or progressive-facet pintle response; no periodic rings."
        ),
        "prohibited_inference": (
            "No universal mark size, spacing, residual depth, or visible facet count."
        ),
    },
    {
        "title": "Practical Forging and Art Smithing",
        "author": "Thomas F. Googerty",
        "year": 1915,
        "url": "https://www.bamsite.org/books/cu31924003588534.pdf",
        "authority": "public-domain master-craftsman wrought-metal manual",
        "observation": (
            "The eye is made first and trued with an eye-pin; a weld lap belongs "
            "on the back. Heavy joints are marked and split while flat, then the "
            "eye is formed and alternating projections are cut and fitted."
        ),
        "eligible_transfer": (
            "Knuckles inherit parent-leaf rest-stock coordinates; rear seam and "
            "bearing faces are optional later semantic owners."
        ),
        "prohibited_inference": (
            "Does not prove every hinge is welded or authorize a universal seam mark."
        ),
    },
    {
        "title": "Basic Blacksmithing: A Training Manual",
        "author": "J. B. Stokes / Food and Agriculture Organization",
        "year": 1992,
        "url": "https://www.fao.org/4/ah637e/ah637e00.htm",
        "authority": "written UN training manual",
        "observation": (
            "A flat strap end is chamfered, bent into an eye, fitted to a drift, "
            "and flattened in a bottom swage; the mating hinge pin uses separate "
            "round stock and bottom swages."
        ),
        "eligible_transfer": (
            "Deform a parent strap field through the roll; reserve drift/swage "
            "compression as an eye-local overlay; give the pin separate ownership."
        ),
        "prohibited_inference": (
            "Human-scale job dimensions do not directly specify giant texture sizes."
        ),
    },
    {
        "title": "Hinges, Pintles and Gudgeons",
        "author": "Field's Blacksmith Shop at The Farmers' Museum",
        "year": 2009,
        "url": (
            "https://ruralblacksmith.blogspot.com/2009/09/"
            "fields-blacksmith-shop-is-making.html"
        ),
        "authority": "practicing historical-reproduction museum shop",
        "observation": (
            "One bar supplies the rolled hinge eye and drawn strap; a sizing pin "
            "trues the gudgeon, while the pintle is forged separately."
        ),
        "eligible_transfer": "Shared strap/eye lineage and separate pintle lineage.",
        "prohibited_inference": (
            "One shop example does not establish universal finish or wear."
        ),
    },
    {
        "title": "Recreating and Installing a Seventeenth-Century Door",
        "author": "Cloverfields Preservation Foundation",
        "year": 2021,
        "url": (
            "https://www.cloverfieldspreservationfoundation.org/newsletters/"
            "cellar-door-hardware"
        ),
        "authority": "multidisciplinary historical reconstruction",
        "observation": (
            "The strap is wrapped around the eye and forge welded on the rear, "
            "leaving at most a slight feathered seam; pintles are separately forged."
        ),
        "eligible_transfer": (
            "Optional sparse rear seam identity after geometry/reference confirmation."
        ),
        "prohibited_inference": (
            "Does not prove the current fantasy hinge's exact weld state or seam path."
        ),
    },
)

LINEAGE_COLOURS = {
    "moving_leaf_stock": (0.055, 0.270, 0.780, 1.0),
    "fixed_leaf_stock": (0.900, 0.310, 0.045, 1.0),
    "pintle_stock": (0.055, 0.690, 0.280, 1.0),
}

TILE_ORDER = (
    "stock_lineage",
    "current_field_frame",
    "existing_broad_height",
    "integrated_normal_angle",
)
TILE_LABELS = {
    "stock_lineage": "STOCK LINEAGE",
    "current_field_frame": "PER-OBJECT U / V RESTART",
    "existing_broad_height": "GLOBAL-SCALE BROAD HEIGHT",
    "integrated_normal_angle": "NORMAL ANGLE — 0 TO 2 DEG",
}


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


build = _load_module("iggy_v004_1_component_audit_build", BUILD_SCRIPT)
calibration = build.calibration
base = build.base
presentation = calibration.presentation


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--resolution-x", type=int, default=960)
    parser.add_argument("--resolution-y", type=int, default=320)
    parser.add_argument("--board-resolution-x", type=int, default=1920)
    parser.add_argument("--board-resolution-y", type=int, default=640)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def stock_lineage(component_name: str) -> str:
    matches = [
        lineage
        for lineage, members in STOCK_LINEAGES.items()
        if component_name in members
    ]
    if len(matches) != 1:
        raise RuntimeError(f"Component lineage is ambiguous: {component_name} -> {matches}")
    return matches[0]


def finite(value: float) -> float:
    result = float(value)
    if not math.isfinite(result):
        raise RuntimeError(f"Audit produced non-finite metric: {result}")
    return result


def descriptive(value: np.ndarray) -> dict[str, float]:
    field = np.asarray(value, dtype=np.float64).reshape(-1)
    return {
        "minimum": finite(field.min()),
        "p05": finite(np.percentile(field, 5.0)),
        "median": finite(np.percentile(field, 50.0)),
        "p95": finite(np.percentile(field, 95.0)),
        "maximum": finite(field.max()),
        "mean": finite(field.mean()),
        "standard_deviation": finite(field.std()),
        "peak_to_peak": finite(np.ptp(field)),
    }


def normal_angle_degrees(normal: np.ndarray) -> np.ndarray:
    vectors = np.asarray(normal, dtype=np.float64)
    magnitude = np.linalg.norm(vectors, axis=-1, keepdims=True)
    if np.any(magnitude <= 1.0e-12):
        raise RuntimeError("Normal audit found a zero vector")
    unit = vectors / magnitude
    return np.degrees(np.arccos(np.clip(unit[..., 2], -1.0, 1.0))).astype(np.float32)


def normal_angle_metrics(angle_degrees: np.ndarray) -> dict[str, Any]:
    result: dict[str, Any] = descriptive(angle_degrees)
    result["texel_fraction_above_degrees"] = {
        f"{threshold:.2f}": finite(np.mean(angle_degrees > threshold))
        for threshold in (0.25, 0.50, 1.00, 2.00)
    }
    return result


def axis_frequency_metrics(
    field: np.ndarray,
    pitch_m: float,
    axis: int,
) -> dict[str, float | None]:
    values = np.asarray(field, dtype=np.float64)
    profile = values.mean(axis=0 if axis == 1 else 1)
    coordinate = np.arange(len(profile), dtype=np.float64)
    linear = np.polyfit(coordinate, profile, 1)
    detrended = profile - np.polyval(linear, coordinate)
    if len(profile) > 2:
        detrended *= np.hanning(len(profile))
    power = np.abs(np.fft.rfft(detrended)) ** 2
    frequency = np.fft.rfftfreq(len(profile), d=pitch_m)
    if len(power) <= 1 or float(power[1:].sum()) <= 1.0e-24:
        return {
            "dominant_frequency_cycles_per_m": None,
            "dominant_wavelength_m": None,
            "power_centroid_frequency_cycles_per_m": None,
            "power_centroid_wavelength_m": None,
        }
    power[0] = 0.0
    dominant_index = int(np.argmax(power))
    dominant_frequency = finite(frequency[dominant_index])
    centroid_frequency = finite(np.sum(frequency * power) / np.sum(power))
    return {
        "dominant_frequency_cycles_per_m": dominant_frequency,
        "dominant_wavelength_m": finite(1.0 / dominant_frequency),
        "power_centroid_frequency_cycles_per_m": centroid_frequency,
        "power_centroid_wavelength_m": finite(1.0 / centroid_frequency),
    }


def dominant_wavelength(
    field: np.ndarray,
    pitch_x_m: float,
    pitch_y_m: float,
) -> dict[str, Any]:
    return {
        "local_u": axis_frequency_metrics(field, pitch_x_m, axis=1),
        "local_v": axis_frequency_metrics(field, pitch_y_m, axis=0),
    }


def resample_bilinear(
    field: np.ndarray,
    output_height: int = 64,
    output_width: int = 128,
) -> np.ndarray:
    source = np.asarray(field, dtype=np.float64)
    source_x = np.linspace(0.0, 1.0, source.shape[1])
    target_x = np.linspace(0.0, 1.0, output_width)
    horizontal = np.stack(
        [np.interp(target_x, source_x, row) for row in source],
        axis=0,
    )
    source_y = np.linspace(0.0, 1.0, source.shape[0])
    target_y = np.linspace(0.0, 1.0, output_height)
    return np.stack(
        [np.interp(target_y, source_y, horizontal[:, column]) for column in range(output_width)],
        axis=1,
    ).astype(np.float32)


def standardized(field: np.ndarray) -> np.ndarray:
    value = np.asarray(field, dtype=np.float64)
    deviation = float(value.std())
    if deviation <= 1.0e-12:
        raise RuntimeError("Cannot standardize a constant broad-height field")
    return ((value - value.mean()) / deviation).astype(np.float32)


def correlation_matrix(
    component_order: list[str],
    fields: dict[str, dict[str, np.ndarray]],
) -> dict[str, Any]:
    samples = np.stack(
        [
            standardized(
                resample_bilinear(fields[name]["broad_forging_height_m"])
            ).reshape(-1)
            for name in component_order
        ],
        axis=0,
    )
    values = np.corrcoef(samples)
    off_diagonal = values[~np.eye(len(values), dtype=bool)]
    return {
        "component_order": component_order,
        "audit_grid": [128, 64],
        "values": [[finite(value) for value in row] for row in values],
        "off_diagonal_minimum": finite(off_diagonal.min()),
        "off_diagonal_median": finite(np.median(off_diagonal)),
        "off_diagonal_maximum": finite(off_diagonal.max()),
        "interpretation": (
            "Near-unity correlation after per-component resampling demonstrates "
            "that one normalized broad-rail gesture restarts on unlike host parts."
        ),
    }


def material_contract(obj: bpy.types.Object) -> dict[str, Any]:
    if len(obj.data.materials) != 1:
        raise RuntimeError(f"{obj.name} does not have one source material")
    material = obj.data.materials[0]
    images = {}
    for node in material.node_tree.nodes:
        if node.bl_idname == "ShaderNodeTexImage":
            images[node.name] = {
                "image": node.image.name if node.image else None,
                "packed": bool(node.image and node.image.packed_file),
            }
    return {
        "material": material.name,
        "images": images,
        "all_images_packed": len(images) == 4 and all(
            item["packed"] for item in images.values()
        ),
        "uv_layers": sorted(layer.name for layer in obj.data.uv_layers),
    }


def current_recipe_contract(frame: dict[str, Any]) -> dict[str, Any]:
    length_m, width_m = frame["span_m"]
    amplitude_m = base.GIANT_LEAF_FORGING_AMPLITUDE_M * min(
        1.0,
        max(width_m, 1.0e-6) / 0.410,
    )
    rails = base.forging_rails(
        frame["minimum_m"][0],
        frame["maximum_m"][0],
        frame["minimum_m"][1],
        frame["maximum_m"][1],
        amplitude_m,
    )
    return {
        "amplitude_envelope_m": finite(amplitude_m),
        "amplitude_rule": (
            "0.006 m times min(1, second_field_span_m / 0.410 m)"
        ),
        "rail_count": len(rails),
        "knot_counts": [len(rail["knots_m"]) for rail in rails],
        "total_knot_count": sum(len(rail["knots_m"]) for rail in rails),
        "normal_multiplier": build.BROAD_NORMAL_MULTIPLIER,
        "component_name_seeded_optical_fields": [
            "connected_medium_34mm",
            "connected_medium_13mm",
            "aggregate_grain_8mm",
        ],
    }


def configure_diagnostic_scene(scene: bpy.types.Scene, args: argparse.Namespace) -> None:
    try:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE"
    scene.render.use_persistent_data = False
    scene.render.resolution_x = args.resolution_x
    scene.render.resolution_y = args.resolution_y
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
    scene.view_settings.exposure = 0.0
    presentation.configure_world(scene, strength=0.0)


def render_path(scene: bpy.types.Scene, path: Path) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def heatmap_normal_angle(angle: np.ndarray) -> np.ndarray:
    t = np.clip(np.asarray(angle, dtype=np.float32) / 2.0, 0.0, 1.0)
    red = np.clip((t - 0.45) / 0.55, 0.0, 1.0)
    green = np.clip(1.0 - np.abs(t - 0.55) / 0.55, 0.0, 1.0)
    blue = np.clip(1.0 - t * 1.15, 0.0, 1.0)
    return np.stack((red, green, blue), axis=-1).astype(np.float32)


def component_image_materials(
    fields: dict[str, dict[str, np.ndarray]],
    global_height_absolute_maximum_m: float,
) -> dict[str, dict[str, bpy.types.Material]]:
    result: dict[str, dict[str, bpy.types.Material]] = {}
    for name, component_fields in fields.items():
        height, width = component_fields["broad_forging_height_m"].shape
        u = np.linspace(0.0, 1.0, width, dtype=np.float32)
        v = np.linspace(0.0, 1.0, height, dtype=np.float32)
        u_field, v_field = np.meshgrid(u, v)
        frame_rgb = np.stack(
            (u_field, v_field, np.full_like(u_field, 0.16)),
            axis=-1,
        )
        integrated_height = (
            component_fields["broad_forging_height_m"]
            * build.BROAD_NORMAL_MULTIPLIER
        )
        height_gray = np.clip(
            0.5
            + integrated_height
            / max(2.0 * global_height_absolute_maximum_m, 1.0e-12),
            0.0,
            1.0,
        )
        angle = normal_angle_degrees(component_fields["combined_normal"])
        images = {
            "current_field_frame": base.make_float_image(
                f"IGGY_IMG_Audit_Frame_{name}",
                frame_rgb,
                color=True,
            ),
            "existing_broad_height": base.make_float_image(
                f"IGGY_IMG_Audit_Height_{name}",
                height_gray,
                color=False,
            ),
            "integrated_normal_angle": base.make_float_image(
                f"IGGY_IMG_Audit_Angle_{name}",
                heatmap_normal_angle(angle),
                color=True,
            ),
        }
        result[name] = {
            panel_id: base.build_lane_material(
                f"IGGY_MAT_Audit_{panel_id}_{name}",
                image,
            )
            for panel_id, image in images.items()
        }
    return result


def render_diagnostic_tiles(
    scene: bpy.types.Scene,
    targets: list[bpy.types.Object],
    fields: dict[str, dict[str, np.ndarray]],
    output_root: Path,
) -> dict[str, dict[str, Any]]:
    minimum, maximum = presentation.object_bounds(targets)
    center = (minimum + maximum) * 0.5
    span = maximum - minimum
    presentation.front_camera(scene.camera, center, span.x * 1.12)
    global_height_absolute_maximum_m = max(
        float(
            np.abs(component["broad_forging_height_m"] * build.BROAD_NORMAL_MULTIPLIER).max()
        )
        for component in fields.values()
    )
    source_materials = {obj.name: obj.data.materials[0] for obj in targets}
    lineage_materials = {
        lineage: presentation.make_emission_material(
            f"IGGY_MAT_Audit_Lineage_{lineage}",
            colour=colour,
        )
        for lineage, colour in LINEAGE_COLOURS.items()
    }
    for obj in targets:
        base.set_material(obj, lineage_materials[stock_lineage(obj.name)])
    tiles = {
        "stock_lineage": render_path(scene, output_root / "stock_lineage.png")
    }

    audit_materials = component_image_materials(
        fields,
        global_height_absolute_maximum_m,
    )
    for panel_id in (
        "current_field_frame",
        "existing_broad_height",
        "integrated_normal_angle",
    ):
        for obj in targets:
            base.set_material(obj, audit_materials[obj.name][panel_id])
        tiles[panel_id] = render_path(scene, output_root / f"{panel_id}.png")
    for obj in targets:
        base.set_material(obj, source_materials[obj.name])
    return tiles


NUMBER_GLYPHS = {
    "1": ("00100", "01100", "00100", "00100", "00100", "00100", "01110"),
    "2": ("01110", "10001", "00001", "00010", "00100", "01000", "11111"),
    "3": ("11110", "00001", "00001", "01110", "00001", "00001", "11110"),
    "4": ("00010", "00110", "01010", "10010", "11111", "00010", "00010"),
}


def write_numbered_overlay(path: Path, width: int, height: int) -> None:
    overlay = np.zeros((height, width, 4), dtype=np.float32)
    badge_size = 52
    glyph_scale = 5
    for index, _panel_id in enumerate(TILE_ORDER):
        column = index % 2
        row = index // 2
        x0 = column * (width // 2) + 12
        y0 = row * (height // 2) + 12
        overlay[y0 : y0 + badge_size, x0 : x0 + badge_size, :3] = 0.008
        overlay[y0 : y0 + badge_size, x0 : x0 + badge_size, 3] = 0.92
        glyph = NUMBER_GLYPHS[str(index + 1)]
        glyph_width = len(glyph[0]) * glyph_scale
        glyph_height = len(glyph) * glyph_scale
        glyph_x = x0 + (badge_size - glyph_width) // 2
        glyph_y = y0 + (badge_size - glyph_height) // 2
        for source_y, line in enumerate(glyph):
            for source_x, active in enumerate(line):
                if active != "1":
                    continue
                ys = slice(glyph_y + source_y * glyph_scale, glyph_y + (source_y + 1) * glyph_scale)
                xs = slice(glyph_x + source_x * glyph_scale, glyph_x + (source_x + 1) * glyph_scale)
                overlay[ys, xs, :3] = 0.95
                overlay[ys, xs, 3] = 1.0
    image = bpy.data.images.new(
        "IGGY_IMG_ComponentResponseAuditNumberOverlay",
        width=width,
        height=height,
        alpha=True,
        float_buffer=True,
    )
    image.pixels.foreach_set(np.flipud(overlay).reshape(-1))
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()


def assemble_board(
    scene: bpy.types.Scene,
    output_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for the diagnostic mosaic")
    raw_board = output_root / "_component_response_raw_mosaic.png"
    label_overlay = output_root / "_component_response_labels.png"
    stack = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            *sum(
                (["-i", str(output_root / f"{panel_id}.png")] for panel_id in TILE_ORDER),
                [],
            ),
            "-filter_complex",
            (
                "[0:v][1:v]hstack=inputs=2[top];"
                "[2:v][3:v]hstack=inputs=2[bottom];"
                "[top][bottom]vstack=inputs=2[board]"
            ),
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(raw_board),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if stack.returncode != 0:
        raise RuntimeError("Diagnostic mosaic assembly failed: " + stack.stderr.strip())

    board_path = output_root / "forged_iron_v004_1_component_response_audit_board.png"
    write_numbered_overlay(
        label_overlay,
        args.board_resolution_x,
        args.board_resolution_y,
    )
    overlay = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(raw_board),
            "-i",
            str(label_overlay),
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
        raise RuntimeError("Diagnostic label overlay failed: " + overlay.stderr.strip())
    raw_board.unlink(missing_ok=True)
    label_overlay.unlink(missing_ok=True)
    return {
        "path": str(board_path),
        "resolution": [args.board_resolution_x, args.board_resolution_y],
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "panel_order": list(TILE_ORDER),
        "panel_legend": {
            str(index + 1): {"id": panel_id, "label": TILE_LABELS[panel_id]}
            for index, panel_id in enumerate(TILE_ORDER)
        },
        "assembly": "ffmpeg 2x2 tile stack plus deterministic numbered RGBA overlay",
    }


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    for required in (SOURCE_BLEND, BUILD_SCRIPT, RESEARCH_PATH):
        if not required.is_file():
            raise FileNotFoundError(required)

    source_hash_before = sha256_file(SOURCE_BLEND)
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))
    scene = bpy.context.scene
    configure_diagnostic_scene(scene, args)
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    lights = {name: bpy.data.objects[name] for name in presentation.TARGET_LIGHT_NAMES}
    presentation.isolate_actual_hinge(targets, lights)
    frames = {obj.name: base.install_component_frames(obj) for obj in targets}
    recipe = build.selected_recipe()
    fields = {
        obj.name: calibration.build_calibrated_fields(
            frames[obj.name],
            obj.name,
            recipe,
        )
        for obj in targets
    }

    component_manifest: dict[str, Any] = {}
    for obj in targets:
        name = obj.name
        frame = frames[name]
        field = fields[name]
        resolution_y, resolution_x = field["broad_forging_height_m"].shape
        pitch_x_m = float(frame["span_m"][0]) / (resolution_x - 1)
        pitch_y_m = float(frame["span_m"][1]) / (resolution_y - 1)
        angle = normal_angle_degrees(field["combined_normal"])
        component_manifest[name] = {
            "construction_class": CONSTRUCTION_CLASSES[name],
            "stock_lineage": stock_lineage(name),
            "parent_leaf": (
                "SM_GH018_MovingLeaf_Openwork"
                if stock_lineage(name) == "moving_leaf_stock"
                else "SM_GH018_FixedLeaf"
                if stock_lineage(name) == "fixed_leaf_stock"
                else None
            ),
            "frame": frame,
            "field_resolution": [resolution_x, resolution_y],
            "pitch_m": {"local_u": pitch_x_m, "local_v": pitch_y_m},
            "height_m": descriptive(field["broad_forging_height_m"]),
            "integrated_height_m": descriptive(
                field["broad_forging_height_m"] * build.BROAD_NORMAL_MULTIPLIER
            ),
            "normal_angle_degrees": normal_angle_metrics(angle),
            "feature_wavelength_m": dominant_wavelength(
                field["broad_forging_height_m"],
                pitch_x_m,
                pitch_y_m,
            ),
            "current_recipe": current_recipe_contract(frame),
            "source_material": material_contract(obj),
        }

    component_order = list(base.TARGET_OBJECTS)
    correlations = correlation_matrix(component_order, fields)
    tiles = render_diagnostic_tiles(scene, targets, fields, output_root)
    board = assemble_board(scene, output_root, args)
    source_hash_after = sha256_file(SOURCE_BLEND)
    source_unchanged = source_hash_before == source_hash_after
    if not source_unchanged:
        raise RuntimeError("Component-response audit changed the saved v004.1 source")

    lineage_continuity = {
        lineage: {
            "members": list(members),
            "construction_owner": (
                "one_parent_flat_strap_deformed_through_rolled_eye"
                if lineage != "pintle_stock"
                else "separate_round_pin_stock"
            ),
            "current_field_frames": [frames[name]["field_coordinate_mode"] for name in members],
            "current_materials": [
                component_manifest[name]["source_material"]["material"] for name in members
            ],
            "current_state": (
                "broken_by_per_object_normalization_and_component_name_seed"
                if lineage != "pintle_stock"
                else "separate_as_required_but_reuses_leaf_broad_rail_recipe"
            ),
        }
        for lineage, members in STOCK_LINEAGES.items()
    }
    findings = {
        "normalized_recipe_restarts": True,
        "correlation_evidence": {
            "off_diagonal_minimum": correlations["off_diagonal_minimum"],
            "off_diagonal_median": correlations["off_diagonal_median"],
            "off_diagonal_maximum": correlations["off_diagonal_maximum"],
        },
        "leaf_knuckle_seed_continuity_broken": True,
        "reason": (
            "Every object normalizes its own frame and receives the same three-rail, "
            "sixteen-knot broad-height recipe. Optical medium/grain seeds additionally "
            "include component_name, so each rolled eye loses parent-leaf phase."
        ),
        "locally_plausible_but_incomplete_frames": {
            "rolled_eye": (
                "circumferential_barrel follows the local roll but does not carry an "
                "unrolled parent-stock coordinate or stock-lineage identity"
            ),
            "pintle": (
                "axial_pin is a plausible local frame but currently receives the same "
                "whole-host forging-rail grammar as the leaves"
            ),
        },
    }
    manifest = {
        "schema": "iggy3d.forged_iron_v004_1_component_response_audit.v1",
        "status": "DIAGNOSTIC_AUDIT_NO_MATERIAL_CHANGE",
        "source": {
            "path": str(SOURCE_BLEND),
            "sha256_before": source_hash_before,
            "sha256_after": source_hash_after,
        },
        "script": {
            "path": str(Path(__file__).resolve()),
            "sha256": sha256_file(Path(__file__).resolve()),
        },
        "research": {
            "path": str(RESEARCH_PATH),
            "sha256": sha256_file(RESEARCH_PATH),
            "sources": list(SOURCE_LEDGER),
        },
        "stock_lineages": lineage_continuity,
        "components": component_manifest,
        "correlation_matrix": correlations,
        "diagnostic_tiles": tiles,
        "board": board,
        "findings": findings,
        "decision": {
            "result": "reject_shape_only_global_normal_owner",
            "accepted_for_next_gate": [
                "v004_1_colour_and_full_coverage_oxide_response",
                "moving_leaf_stock_lineage",
                "fixed_leaf_stock_lineage",
                "separate_pintle_stock_lineage",
                "circumferential_local_eye_frame_as_secondary_coordinate_only",
                "axial_local_pintle_frame",
            ],
            "rejected": [
                "one_normalized_three_rail_recipe_for_all_host_shapes",
                "per_object_leaf_to_knuckle_field_reset",
                "component_name_seed_for_shared_parent_stock",
                "global_amplitude_increase_as_pivot_repair",
            ],
            "next_gate": (
                "Author no integrated material yet. Build a reduced diagnostic with "
                "one parent-rest-stock leaf field carried through rolled knuckles and "
                "one separately evidenced axial pintle response; compare each isolated "
                "and cumulative against unchanged v004.1."
            ),
        },
        "source_unchanged": source_unchanged,
        "material_candidate_created": False,
        "uses_ai_generated_imagery": False,
        "uses_condition": False,
        "manual_acceptance_established": False,
        "unreal_parity_verified": False,
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    )
    print(
        "FORGED_IRON_V004_1_COMPONENT_AUDIT="
        f"components:{len(component_manifest)},"
        f"lineages:{len(STOCK_LINEAGES)},"
        f"source_unchanged:{source_unchanged}"
    )
    print(f"FORGED_IRON_V004_1_COMPONENT_AUDIT_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
