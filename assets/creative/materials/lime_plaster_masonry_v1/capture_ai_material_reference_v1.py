#!/usr/bin/env python3
"""Capture material intent from an AI comparison image without copying a lit crop."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
from typing import Any

import numpy as np
from PIL import Image, ImageDraw, ImageFilter


SCRIPT_ROOT = Path(__file__).resolve().parent
DEFAULT_REGIONS = (
    SCRIPT_ROOT / "references" / "attempt_01_capture_regions.json"
)
DEFAULT_OUTPUT_ROOT = SCRIPT_ROOT / "references" / "attempt_01_capture"
DEFAULT_INKBLOTTER_ROOT = Path("/Users/kogaryu/font")
CAPTURE_SCHEMA = "iggy3d.material_reference_capture.v1"


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture semantic material evidence from an AI reference."
    )
    parser.add_argument("--regions", type=Path, default=DEFAULT_REGIONS)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument(
        "--inkblotter-root",
        type=Path,
        default=DEFAULT_INKBLOTTER_ROOT,
    )
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_regions(path: Path) -> tuple[dict[str, Any], Path]:
    payload = json.loads(path.read_text())
    if payload.get("schema") != "iggy3d.material_reference_capture_regions.v1":
        raise ValueError("Unexpected capture-region schema")
    source = (path.parent / payload["source"]).resolve()
    if not source.is_file():
        raise FileNotFoundError(source)
    return payload, source


def luminance(rgb: tuple[int, int, int] | list[int]) -> float:
    return (
        float(rgb[0]) * 0.2126
        + float(rgb[1]) * 0.7152
        + float(rgb[2]) * 0.0722
    )


def hex_rgb(rgb: tuple[int, int, int] | list[int]) -> str:
    return "#" + "".join(f"{int(channel):02x}" for channel in rgb)


def normalize_box(
    box: list[int],
    width: int,
    height: int,
) -> list[float]:
    x0, y0, x1, y1 = box
    return [
        x0 / width,
        y0 / height,
        x1 / width,
        y1 / height,
    ]


def sample_region(
    image: Image.Image,
    box: list[int],
    *,
    material: str,
) -> tuple[np.ndarray, Image.Image]:
    x0, y0, x1, y1 = box
    crop = image.crop((x0, y0, x1, y1)).convert("RGB")
    pixels = np.asarray(crop, dtype=np.uint8).reshape(-1, 3)
    value = (
        pixels[:, 0].astype(np.float32) * 0.2126
        + pixels[:, 1].astype(np.float32) * 0.7152
        + pixels[:, 2].astype(np.float32) * 0.0722
    )
    if material == "plaster":
        keep = np.logical_and(value > 92, value < 246)
    elif material == "mortar":
        keep = np.logical_and(value > 58, value < 204)
    elif material == "linework":
        lower = np.percentile(value, 4.0)
        upper = np.percentile(value, 16.0)
        keep = np.logical_and(value >= lower, value <= upper)
    else:
        keep = np.logical_and(value > 36, value < 238)
    filtered = pixels[keep]
    if len(filtered) < 64:
        raise ValueError(f"Region {box} retained too little {material} evidence")
    stride = max(1, len(filtered) // 60000)
    return filtered[::stride], crop


def reduce_colors(
    colors: np.ndarray,
    palette_size: int,
    reduce_palette,
) -> list[list[int]]:
    tuples = [tuple(int(channel) for channel in color) for color in colors]
    reduced = reduce_palette(tuples, palette_size)
    return [list(color) for color in sorted(reduced, key=luminance)]


def interpolate_palette(
    palette: list[list[int]],
    count: int,
) -> list[list[int]]:
    source = np.asarray(palette, dtype=np.float32)
    if len(source) == count:
        return source.astype(np.uint8).tolist()
    positions = np.linspace(0.0, 1.0, len(source), dtype=np.float32)
    targets = np.linspace(0.0, 1.0, count, dtype=np.float32)
    result = np.stack(
        [
            np.interp(targets, positions, source[:, channel])
            for channel in range(3)
        ],
        axis=-1,
    )
    return np.clip(np.rint(result), 0, 255).astype(np.uint8).tolist()


def frequency_metrics(crop: Image.Image) -> dict[str, Any]:
    gray = crop.convert("L")
    array = np.asarray(gray, dtype=np.float32) / 255.0
    blur_4 = np.asarray(
        gray.filter(ImageFilter.GaussianBlur(radius=4.0)),
        dtype=np.float32,
    ) / 255.0
    blur_16 = np.asarray(
        gray.filter(ImageFilter.GaussianBlur(radius=16.0)),
        dtype=np.float32,
    ) / 255.0
    low = blur_16
    middle = blur_4 - blur_16
    high = array - blur_4
    gradient_y, gradient_x = np.gradient(array)
    magnitude = np.sqrt(gradient_x * gradient_x + gradient_y * gradient_y)
    tensor_xx = float(np.mean(gradient_x * gradient_x))
    tensor_yy = float(np.mean(gradient_y * gradient_y))
    tensor_xy = float(np.mean(gradient_x * gradient_y))
    angle = 0.5 * math.atan2(2.0 * tensor_xy, tensor_xx - tensor_yy)
    return {
        "low_frequency_std": float(np.std(low)),
        "middle_frequency_std": float(np.std(middle)),
        "high_frequency_std": float(np.std(high)),
        "quiet_pixel_ratio": float(
            np.logical_and(np.abs(middle) < 0.025, np.abs(high) < 0.018).mean()
        ),
        "gradient_percentiles": {
            "p50": float(np.percentile(magnitude, 50)),
            "p90": float(np.percentile(magnitude, 90)),
            "p99": float(np.percentile(magnitude, 99)),
        },
        "dominant_gradient_angle_deg": math.degrees(angle),
    }


def color_metrics(colors: np.ndarray) -> dict[str, Any]:
    value = (
        colors[:, 0].astype(np.float32) * 0.2126
        + colors[:, 1].astype(np.float32) * 0.7152
        + colors[:, 2].astype(np.float32) * 0.0722
    )
    chroma = colors.max(axis=1).astype(np.float32) - colors.min(axis=1)
    return {
        "sample_count": int(len(colors)),
        "mean_rgb": np.rint(colors.mean(axis=0)).astype(np.uint8).tolist(),
        "median_rgb": np.rint(np.median(colors, axis=0)).astype(np.uint8).tolist(),
        "luminance_percentiles": {
            "p05": float(np.percentile(value, 5)),
            "p50": float(np.percentile(value, 50)),
            "p95": float(np.percentile(value, 95)),
        },
        "facet_contrast": float(
            (np.percentile(value, 95) - np.percentile(value, 5)) / 255.0
        ),
        "chroma_median": float(np.median(chroma) / 255.0),
    }


def combine_crops(crops: list[Image.Image]) -> Image.Image:
    if not crops:
        raise ValueError("No crops to combine")
    width = max(crop.width for crop in crops)
    height = sum(crop.height for crop in crops)
    sheet = Image.new("RGB", (width, height), "black")
    y = 0
    for crop in crops:
        sheet.paste(crop, (0, y))
        y += crop.height
    return sheet


def draw_region_overlay(
    image: Image.Image,
    regions: list[dict[str, Any]],
    path: Path,
) -> None:
    overlay = image.convert("RGB").copy()
    draw = ImageDraw.Draw(overlay)
    colors = {
        "plaster": "#f0d36f",
        "stone": "#5bc0eb",
        "mortar": "#a5d66a",
        "finish_coat": "#fff1ba",
        "brown_coat": "#d68a50",
        "scratch_coat": "#b3573f",
        "linework": "#d754d7",
    }
    for region in regions:
        box = tuple(region["box_px"])
        color = colors[region["material"]]
        draw.rectangle(box, outline=color, width=3)
        draw.rectangle(
            (box[0], box[1], min(box[2], box[0] + 180), box[1] + 19),
            fill="#111111",
        )
        draw.text((box[0] + 3, box[1] + 3), region["name"], fill=color)
    overlay.save(path)


def draw_crop_sheet(
    records: list[dict[str, Any]],
    crops: dict[str, Image.Image],
    path: Path,
) -> None:
    thumb_w = 220
    thumb_h = 128
    label_h = 34
    columns = 4
    rows = math.ceil(len(records) / columns)
    sheet = Image.new(
        "RGB",
        (columns * thumb_w, rows * (thumb_h + label_h)),
        "#161719",
    )
    draw = ImageDraw.Draw(sheet)
    for index, record in enumerate(records):
        column = index % columns
        row = index // columns
        x = column * thumb_w
        y = row * (thumb_h + label_h)
        crop = crops[record["name"]].copy()
        crop.thumbnail((thumb_w - 8, thumb_h - 8), Image.Resampling.LANCZOS)
        sheet.paste(crop, (x + 4, y + label_h + 4))
        draw.text((x + 4, y + 4), record["name"], fill="#f2d16d")
        draw.text(
            (x + 4, y + 18),
            record["material"],
            fill="#b8c0c8",
        )
    sheet.save(path)


def draw_palette_sheet(
    materials: dict[str, Any],
    path: Path,
) -> None:
    rows: list[tuple[str, list[list[int]]]] = []
    for material_name in (
        "plaster",
        "mortar",
        "finish_coat",
        "brown_coat",
        "scratch_coat",
        "linework",
    ):
        entry = materials.get(material_name)
        if entry:
            rows.append((material_name, entry["palette_20_rgb"]))
    for family, entry in materials["stone"]["families"].items():
        rows.append((f"stone/{family}", entry["palette_20_rgb"]))
    swatch = 42
    label_w = 210
    height = len(rows) * swatch
    sheet = Image.new("RGB", (label_w + 20 * swatch, height), "#111214")
    draw = ImageDraw.Draw(sheet)
    for row, (label, palette) in enumerate(rows):
        y = row * swatch
        draw.text((8, y + 13), label, fill="#e7dfca")
        for index, color in enumerate(palette):
            x = label_w + index * swatch
            draw.rectangle(
                (x, y, x + swatch, y + swatch),
                fill=tuple(color),
            )
    sheet.save(path)


def main() -> None:
    argv = sys.argv[1:]
    args = parse_args(argv)
    regions_payload, source_path = load_regions(args.regions)
    if not args.inkblotter_root.is_dir():
        raise FileNotFoundError(args.inkblotter_root)
    sys.path.insert(0, str(args.inkblotter_root))
    from glyph_lab.image_probe import probe_image
    from glyph_lab.reference_style import reduce_palette

    args.output_root.mkdir(parents=True, exist_ok=True)
    image = Image.open(source_path).convert("RGB")
    width, height = image.size
    scale = regions_payload["scale_reference"]
    door_height_px = scale["bbox_px"][3] - scale["bbox_px"][1]
    pixels_per_meter = door_height_px / float(scale["height_m"])

    records: list[dict[str, Any]] = []
    samples_by_material: dict[str, list[np.ndarray]] = {}
    samples_by_family: dict[str, list[np.ndarray]] = {}
    crops_by_material: dict[str, list[Image.Image]] = {}
    crops_by_name: dict[str, Image.Image] = {}
    for region in regions_payload["regions"]:
        colors, crop = sample_region(
            image,
            region["box_px"],
            material=region["material"],
        )
        samples_by_material.setdefault(region["material"], []).append(colors)
        crops_by_material.setdefault(region["material"], []).append(crop)
        if region.get("family"):
            samples_by_family.setdefault(region["family"], []).append(colors)
        crops_by_name[region["name"]] = crop
        palette = interpolate_palette(
            reduce_colors(colors, min(8, len(colors)), reduce_palette),
            20,
        )
        metrics = color_metrics(colors)
        records.append(
            {
                **region,
                "box_normalized": normalize_box(
                    region["box_px"], width, height
                ),
                "palette_20_rgb": palette,
                "palette_20_hex": [hex_rgb(color) for color in palette],
                "metrics": metrics,
                "frequency": frequency_metrics(crop),
            }
        )

    materials: dict[str, Any] = {}
    for material_name, arrays in samples_by_material.items():
        combined = np.concatenate(arrays, axis=0)
        palette = interpolate_palette(
            reduce_colors(combined, 12, reduce_palette), 20
        )
        material_crop = combine_crops(crops_by_material[material_name])
        materials[material_name] = {
            "palette_20_rgb": palette,
            "palette_20_hex": [hex_rgb(color) for color in palette],
            "metrics": color_metrics(combined),
            "frequency": frequency_metrics(material_crop),
            "region_names": [
                record["name"]
                for record in records
                if record["material"] == material_name
            ],
        }

    stone_families: dict[str, Any] = {}
    for family, arrays in samples_by_family.items():
        combined = np.concatenate(arrays, axis=0)
        palette = interpolate_palette(
            reduce_colors(combined, 12, reduce_palette), 20
        )
        stone_families[family] = {
            "base_rgb": palette[10],
            "base_hex": hex_rgb(palette[10]),
            "palette_20_rgb": palette,
            "palette_20_hex": [hex_rgb(color) for color in palette],
            "metrics": color_metrics(combined),
            "region_names": [
                record["name"]
                for record in records
                if record.get("family") == family
            ],
        }
    materials["stone"]["families"] = stone_families

    line_palette = materials["linework"]["palette_20_rgb"]
    line_color = line_palette[max(0, min(4, len(line_palette) - 1))]
    plaster_metrics = materials["plaster"]["frequency"]
    stone_metrics = materials["stone"]["frequency"]
    capture = {
        "schema": CAPTURE_SCHEMA,
        "source": {
            "path": str(source_path),
            "sha256": sha256_file(source_path),
            "width": width,
            "height": height,
            "kind": "ai_generated_comparison_reference",
        },
        "capture_regions": {
            "path": str(args.regions),
            "sha256": sha256_file(args.regions),
        },
        "inkblotter": {
            "root": str(args.inkblotter_root),
            "palette_reducer": "glyph_lab.reference_style.reduce_palette",
            "probe": probe_image(source_path, grid_width=64, grid_height=48),
        },
        "scale": {
            **scale,
            "pixels_per_meter": pixels_per_meter,
        },
        "materials": materials,
        "style_intent": {
            "line_color_rgb": line_color,
            "line_color_hex": hex_rgb(line_color),
            "plaster_quiet_ratio": plaster_metrics["quiet_pixel_ratio"],
            "stone_quiet_ratio": stone_metrics["quiet_pixel_ratio"],
            "plaster_low_to_high_frequency_ratio": (
                plaster_metrics["low_frequency_std"]
                / max(plaster_metrics["high_frequency_std"], 1.0e-8)
            ),
            "stone_middle_to_high_frequency_ratio": (
                stone_metrics["middle_frequency_std"]
                / max(stone_metrics["high_frequency_std"], 1.0e-8)
            ),
            "plaster_dominant_gradient_angle_deg": plaster_metrics[
                "dominant_gradient_angle_deg"
            ],
            "stone_facet_contrast": materials["stone"]["metrics"][
                "facet_contrast"
            ],
            "mortar_stone_luminance_delta": (
                materials["stone"]["metrics"]["luminance_percentiles"]["p50"]
                - materials["mortar"]["metrics"]["luminance_percentiles"]["p50"]
            )
            / 255.0,
        },
        "regions": records,
        "production_translation": {
            "consume": [
                "semantic palettes",
                "broad-to-fine frequency ratios",
                "quiet-area ratios",
                "facet contrast",
                "line color",
                "coat color order",
            ],
            "do_not_consume": [
                "perspective",
                "directional lighting",
                "cast shadows",
                "raw crop pixels",
                "invented cracks or damage",
            ],
            "raw_pixels_used_as_runtime_texture": False,
        },
    }
    capture_path = args.output_root / "attempt_01_material_capture.json"
    capture_path.write_text(json.dumps(capture, indent=2) + "\n")
    draw_region_overlay(
        image,
        regions_payload["regions"],
        args.output_root / "attempt_01_capture_regions.png",
    )
    draw_crop_sheet(
        records,
        crops_by_name,
        args.output_root / "attempt_01_capture_crops.png",
    )
    draw_palette_sheet(
        materials,
        args.output_root / "attempt_01_capture_palettes.png",
    )
    manifest = {
        "schema": "iggy3d.material_reference_capture.outputs.v1",
        "capture": str(capture_path),
        "regions_proof": str(
            args.output_root / "attempt_01_capture_regions.png"
        ),
        "crops_proof": str(
            args.output_root / "attempt_01_capture_crops.png"
        ),
        "palettes_proof": str(
            args.output_root / "attempt_01_capture_palettes.png"
        ),
    }
    (args.output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n"
    )
    print(
        "attempt_01 material captured:",
        len(records),
        "semantic regions,",
        len(stone_families),
        "stone families,",
        f"{pixels_per_meter:.2f} px/m",
    )


if __name__ == "__main__":
    main()
