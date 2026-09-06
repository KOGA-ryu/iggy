#!/usr/bin/env python3
"""Cheap four-way directional-grind candidate board on one sword crop."""

from __future__ import annotations

import argparse
import copy
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
from mathutils import Vector


PACKAGE = Path(__file__).resolve().parent
SOURCE = PACKAGE / "output" / "historical_swords_set_study_v1" / "source" / "historical_swords_set.blend"
CANONICAL_PATH = PACKAGE / "build_sword_steel_v1.py"
STUDY_PATH = PACKAGE / "build_historical_swords_set_study_v1.py"
RESPONSE_PATH = PACKAGE / "build_historical_swords_clean_steel_response_v1.py"
COMPARE_PATH = PACKAGE / "build_bastard_sword_original_vs_canonical.py"
DEFAULT_OUTPUT = PACKAGE / "output" / "bastard_sword_grind_candidates"
PANEL_WIDTH = 600
PANEL_HEIGHT = 300
CANDIDATES = (
    ("quiet", "A QUIET CONTROL"),
    ("current", "B CURRENT 47-TRACK"),
    ("segmented", "C SEGMENTED BELT"),
    ("source_spectrum", "D SOURCE SPECTRUM"),
)


def args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--samples", type=int, default=16)
    return parser.parse_args(argv)


def load_module(path: Path, name: str) -> Any:
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def artifact(path: Path) -> dict[str, Any]:
    return {"path": str(path), "bytes": path.stat().st_size, "sha256": sha256_file(path)}


def normalize_signed(field: np.ndarray) -> np.ndarray:
    field = field.astype(np.float32)
    field -= float(np.median(field))
    scale = float(np.percentile(np.abs(field), 99.2))
    if scale <= 1.0e-8:
        return np.full(field.shape, 0.5, dtype=np.float32)
    signed = np.clip(field / scale, -1.0, 1.0)
    return (0.5 + 0.5 * signed).astype(np.float32)


def segmented_belt_field(shape: tuple[int, int]) -> np.ndarray:
    height, width = shape
    rng = np.random.default_rng(190521)
    u = np.linspace(0.0, 1.0, width, dtype=np.float32)
    v = np.linspace(-1.0, 1.0, height, dtype=np.float32)
    u_field, v_field = np.meshgrid(u, v)
    signed = np.zeros(shape, dtype=np.float32)
    track_count = 58
    centers = np.sort(rng.uniform(-0.96, 0.96, track_count))
    pass_slopes = (-0.018, -0.005, 0.010, 0.023)
    for index, nominal_v in enumerate(centers):
        if rng.random() < 0.12:
            continue
        center_v = float(nominal_v + rng.normal(0.0, 0.004))
        width_v = float(rng.uniform(0.0015, 0.0085))
        slope = float(pass_slopes[index % len(pass_slopes)] + rng.normal(0.0, 0.0025))
        bow = float(rng.normal(0.0, 0.004))
        pressure = float(rng.choice((-1.0, 1.0)) * rng.uniform(0.20, 1.0))
        segment_count = int(rng.integers(1, 5))
        for _ in range(segment_count):
            segment_center = float(rng.uniform(-0.05, 1.05))
            segment_half = float(rng.uniform(0.055, 0.27))
            along = np.exp(-((u_field - segment_center) / segment_half) ** 4)
            offset_u = u_field - segment_center
            centerline = center_v + slope * offset_u + bow * offset_u * offset_u
            across = np.exp(-0.5 * ((v_field - centerline) / width_v) ** 2)
            signed += pressure * float(rng.uniform(0.35, 1.0)) * along * across
    # Three broad pressure passes join the fragments without filling the quiet gaps.
    for center_v, strength, width_v in ((-0.48, 0.14, 0.020), (0.08, -0.11, 0.026), (0.63, 0.09, 0.017)):
        envelope = np.exp(-((u_field - 0.53) / 0.46) ** 6)
        signed += strength * envelope * np.exp(-0.5 * ((v_field - center_v) / width_v) ** 2)
    return normalize_signed(signed)


def blade_uv_bounds(source: bpy.types.Object, component: dict[str, Any]) -> tuple[float, float, float, float]:
    vertex_ids = component["_vertex_ids"]
    uv_layer = source.data.uv_layers["UVMap"]
    values = []
    for polygon in source.data.polygons:
        if all(int(vertex) in vertex_ids for vertex in polygon.vertices):
            values.extend(tuple(uv_layer.data[index].uv) for index in polygon.loop_indices)
    return (
        min(value[0] for value in values),
        max(value[0] for value in values),
        min(value[1] for value in values),
        max(value[1] for value in values),
    )


def source_spectrum_field(
    image: bpy.types.Image,
    uv_bounds: tuple[float, float, float, float],
    shape: tuple[int, int],
) -> np.ndarray:
    width, height = image.size
    pixels = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    rgb = pixels.reshape(height, width, 4)[..., :3]
    luminance = 0.2126 * rgb[..., 0] + 0.7152 * rgb[..., 1] + 0.0722 * rgb[..., 2]
    u0, u1, v0, v1 = uv_bounds
    x0 = max(0, min(width - 2, int(math.floor(u0 * (width - 1)))))
    x1 = max(x0 + 2, min(width, int(math.ceil(u1 * (width - 1))) + 1))
    y0 = max(0, min(height - 2, int(math.floor(v0 * (height - 1)))))
    y1 = max(y0 + 2, min(height, int(math.ceil(v1 * (height - 1))) + 1))
    crop = luminance[y0:y1, x0:x1]
    target_y = np.linspace(0, crop.shape[0] - 1, shape[0]).astype(np.int32)
    target_x = np.linspace(0, crop.shape[1] - 1, shape[1]).astype(np.int32)
    sampled = crop[np.ix_(target_y, target_x)].astype(np.float32)
    sampled -= float(sampled.mean())
    window = np.outer(np.hanning(shape[0]), np.hanning(shape[1])).astype(np.float32)
    spectrum = np.fft.rfft2(sampled * window)
    amplitude = np.abs(spectrum)
    fy = np.fft.fftfreq(shape[0])[:, np.newaxis]
    fx = np.fft.rfftfreq(shape[1])[np.newaxis, :]
    high_pass = 1.0 - np.exp(-((fx / 0.012) ** 2 + (fy / 0.020) ** 2))
    # Remove almost-horizontal-frequency energy that would create transverse bars.
    longitudinal_bias = 0.28 + 0.72 * (np.abs(fy) / (np.abs(fx) + np.abs(fy) + 1.0e-6))
    amplitude *= high_pass * longitudinal_bias
    rng = np.random.default_rng(771903)
    phase = rng.uniform(-math.pi, math.pi, amplitude.shape)
    synthetic = np.fft.irfft2(amplitude * np.exp(1j * phase), s=shape).real
    return normalize_signed(synthetic)


def field_metrics(field: np.ndarray) -> dict[str, float]:
    centered = field.astype(np.float64) - 0.5
    gradient_v, gradient_u = np.gradient(centered)
    energy_u = float(np.mean(gradient_u * gradient_u))
    energy_v = float(np.mean(gradient_v * gradient_v))
    return {
        "minimum": float(field.min()),
        "maximum": float(field.max()),
        "standard_deviation": float(field.std()),
        "quiet_fraction_abs_lt_0_03": float(np.mean(np.abs(centered) < 0.03)),
        "longitudinal_mark_coherence": energy_v / max(energy_u + energy_v, 1.0e-12),
    }


def configure_hero_scene(response: Any, samples: int) -> tuple[Any, Any, dict[str, Any]]:
    scene, camera, lights = response.configure_scene(samples)
    scene.render.resolution_x = PANEL_WIDTH
    scene.render.resolution_y = PANEL_HEIGHT
    camera.data.ortho_scale = 0.105
    camera.location = Vector((0.0, -0.17, 0.48))
    response.point_at(camera, Vector((0.0, 0.0, 0.0)))
    lights["key"].hide_render = False
    lights["key"].data.energy = 6.0
    lights["fill"].hide_render = False
    lights["fill"].data.energy = 1.2
    lights["grazing"].hide_render = False
    lights["grazing"].data.energy = 8.0
    return scene, camera, lights


def escape_xml(value: str) -> str:
    return value.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def assemble_board(
    output_root: Path,
    renders: dict[str, dict[str, Any]],
    maps: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    sips = shutil.which("sips")
    if ffmpeg is None or sips is None:
        raise RuntimeError("ffmpeg and sips are required")
    raw = output_root / "_raw.png"
    overlay_svg = output_root / "_overlay.svg"
    overlay_png = output_root / "_overlay.png"
    board = output_root / "bastard_sword_grind_candidate_board.png"
    keys = [key for key, _ in CANDIDATES]
    inputs = [Path(renders[key]["path"]) for key in keys] + [Path(maps[key]["path"]) for key in keys]
    filters = []
    for index in range(4, 8):
        filters.append(
            f"[{index}:v]scale={PANEL_WIDTH}:{PANEL_HEIGHT // 2}:flags=lanczos,"
            f"pad={PANEL_WIDTH}:{PANEL_HEIGHT}:0:{PANEL_HEIGHT // 4}:color=0x11141a[m{index - 4}]"
        )
    labels = [f"[{index}:v]" for index in range(4)] + [f"[m{index}]" for index in range(4)]
    layout = "|".join(
        [f"{index * PANEL_WIDTH}_0" for index in range(4)]
        + [f"{index * PANEL_WIDTH}_{PANEL_HEIGHT}" for index in range(4)]
    )
    filters.append("".join(labels) + f"xstack=inputs=8:layout={layout}[board]")
    command = [ffmpeg, "-y", "-hide_banner", "-loglevel", "error"]
    for path in inputs:
        command.extend(("-i", str(path)))
    command.extend(("-filter_complex", ";".join(filters), "-map", "[board]", "-frames:v", "1", str(raw)))
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode:
        raise RuntimeError(result.stderr)
    board_width = PANEL_WIDTH * 4
    board_height = PANEL_HEIGHT * 2
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{board_width}" height="{board_height}">']
    for index, (_, label) in enumerate(CANDIDATES):
        x = index * PANEL_WIDTH
        for row, suffix in ((0, "PHYSICAL HERO / 2.5X DIAGNOSTIC GAIN"), (1, "ISOLATED GRIND FIELD")):
            y = row * PANEL_HEIGHT
            svg.extend(
                (
                    f'<rect x="{x}" y="{y}" width="{PANEL_WIDTH}" height="38" fill="#090b0f" fill-opacity="0.94"/>',
                    f'<text x="{x + 13}" y="{y + 24}" fill="#f3f5f7" font-family="Arial" font-size="14" font-weight="bold">{escape_xml(label)} / {suffix}</text>',
                )
            )
    svg.append("</svg>")
    overlay_svg.write_text("\n".join(svg) + "\n")
    converted = subprocess.run(
        (sips, "-s", "format", "png", str(overlay_svg), "--out", str(overlay_png)),
        capture_output=True,
        text=True,
        check=False,
    )
    if converted.returncode:
        raise RuntimeError(converted.stderr)
    merged = subprocess.run(
        (
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(raw),
            "-i",
            str(overlay_png),
            "-filter_complex",
            "[0:v][1:v]overlay=0:0:format=auto[board]",
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(board),
        ),
        capture_output=True,
        text=True,
        check=False,
    )
    if merged.returncode:
        raise RuntimeError(merged.stderr)
    for temporary in (raw, overlay_svg, overlay_png):
        temporary.unlink(missing_ok=True)
    return artifact(board) | {"resolution": [board_width, board_height]}


def main() -> None:
    arguments = args()
    output_root = arguments.output_root.expanduser().resolve()
    renders_root = output_root / "renders"
    maps_root = output_root / "maps"
    renders_root.mkdir(parents=True, exist_ok=True)
    maps_root.mkdir(parents=True, exist_ok=True)
    for root in (renders_root, maps_root):
        for path in root.glob("*.png"):
            path.unlink()
    (output_root / "bastard_sword_grind_candidate_board.png").unlink(missing_ok=True)

    canonical = load_module(CANONICAL_PATH, "iggy_grind_canonical")
    study = load_module(STUDY_PATH, "iggy_grind_study")
    response = load_module(RESPONSE_PATH, "iggy_grind_response")
    compare = load_module(COMPARE_PATH, "iggy_grind_compare")
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE))
    source = bpy.data.objects["BastardSword"]
    component = study.classify_components(study.connected_components(source))["blade"][0]
    source_uv_bounds = blade_uv_bounds(source, component)
    source_diffuse = bpy.data.images["steel_diffuse.png"]
    blade, uv_record = compare.copy_blade_preserving_uv(source, component)
    region_record = response.author_blade_uv_and_regions(blade, "BastardSword", canonical)
    response.center_and_orient_blade(blade)
    for obj in list(bpy.data.objects):
        if obj is not blade:
            bpy.data.objects.remove(obj, do_unlink=True)

    profile, conductor, tangent = canonical.load_contracts()
    shape = (
        profile["finish"]["medium_grind"]["map_resolution"][1],
        profile["finish"]["medium_grind"]["map_resolution"][0],
    )
    fields = {
        "quiet": np.full(shape, 0.5, dtype=np.float32),
        "current": canonical.generate_grind_field(profile),
        "segmented": segmented_belt_field(shape),
        "source_spectrum": source_spectrum_field(source_diffuse, source_uv_bounds, shape),
    }
    macro = canonical.generate_macro_polish_field(profile)
    macro_path = output_root / "shared_macro.png"
    canonical.common.write_png_gray16(macro_path, macro)
    macro_image = bpy.data.images.load(str(macro_path), check_existing=False)
    macro_image.colorspace_settings.name = "Non-Color"

    candidate_images: dict[str, bpy.types.Image] = {}
    map_records: dict[str, dict[str, Any]] = {}
    metrics: dict[str, dict[str, float]] = {}
    for key, _ in CANDIDATES:
        path = maps_root / f"{key}.png"
        canonical.common.write_png_gray16(path, fields[key])
        image = bpy.data.images.load(str(path), check_existing=False)
        image.colorspace_settings.name = "Non-Color"
        candidate_images[key] = image
        map_records[key] = artifact(path)
        metrics[key] = field_metrics(fields[key])

    scene, _, _ = configure_hero_scene(response, arguments.samples)
    render_records: dict[str, dict[str, Any]] = {}
    materials: dict[str, bpy.types.Material] = {}
    for key, _ in CANDIDATES:
        diagnostic_profile = copy.deepcopy(profile)
        finish = diagnostic_profile["finish"]["medium_grind"]
        finish["roughness_amplitude"] = 0.045
        finish["bump_distance_m"] = 0.000010
        finish["bump_strength"] = 0.35
        material, _ = canonical.build_sword_steel_material(
            diagnostic_profile,
            conductor,
            tangent,
            macro_image,
            candidate_images[key],
        )
        material.name = f"IGGY_EXP_Grind_{key}"
        materials[key] = material
        compare.set_material(blade, material)
        render_path = renders_root / f"{key}.png"
        render_records[key] = response.render_panel(scene, render_path)

    board = assemble_board(output_root, render_records, map_records)
    manifest = {
        "schema": "iggy3d.bastard-sword-grind-candidates.v1",
        "status": "CHEAP_EXPLORATION_NOT_INTEGRATED",
        "candidate_order": [key for key, _ in CANDIDATES],
        "changed_layer": "medium directional grind only",
        "diagnostic_gain": {
            "roughness_amplitude": 0.045,
            "bump_distance_m": 0.000010,
            "bump_strength": 0.35,
        },
        "source_uv_bounds": list(source_uv_bounds),
        "blade": region_record | uv_record,
        "metrics": metrics,
        "maps": map_records,
        "renders": render_records,
        "board": board,
        "integration_performed": False,
        "manual_acceptance_established": False,
    }
    (output_root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"GRIND_CANDIDATE_BOARD={board['path']}")


if __name__ == "__main__":
    main()
