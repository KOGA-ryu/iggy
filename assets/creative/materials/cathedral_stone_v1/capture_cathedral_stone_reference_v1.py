#!/usr/bin/env python3
"""Capture Santa Marina biocalcarenite evidence without retaining source pixels."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import sys
from typing import Any
from urllib.request import Request, urlopen

import numpy as np
from PIL import Image, ImageDraw


SCRIPT_ROOT = Path(__file__).resolve().parent
DEFAULT_OUTPUT = (
    SCRIPT_ROOT / "references" / "santa_marina_biocalcarenite_capture.json"
)
DEFAULT_PROOF = (
    SCRIPT_ROOT / "references" / "santa_marina_biocalcarenite_palette_proof.png"
)
DEFAULT_INKBLOTTER_ROOT = Path("/Users/kogaryu/font")
SOURCE_PAGE = (
    "https://commons.wikimedia.org/wiki/"
    "File:C%C3%B3rdoba_-_Santa_Marina_de_Aguas_Santas.jpg"
)
SOURCE_URL = (
    "https://upload.wikimedia.org/wikipedia/commons/f/f4/"
    "C%C3%B3rdoba_-_Santa_Marina_de_Aguas_Santas.jpg"
)
LITHOLOGY_SOURCE = (
    "https://materconstrucc.revistas.csic.es/index.php/"
    "materconstrucc/article/download/123/162"
)
CAPTURE_SCHEMA = "iggy3d.material_reference_capture.cathedral_stone.v2"

# Small interiors of evenly sunlit upper-gable stones. The rectangles avoid
# mortar, sky, mouldings, cast shadow, obvious cracks, repairs, and eroded
# cavities. They are color observations, not texture crops.
SUNLIT_STONE_SAMPLES_PX = (
    (2626, 636, 2674, 684),
    (2986, 686, 3034, 734),
    (2366, 776, 2414, 824),
    (2966, 826, 3014, 874),
    (2516, 966, 2564, 1014),
    (2716, 986, 2764, 1034),
    (3156, 1026, 3204, 1074),
    (2466, 1146, 2514, 1194),
    (2676, 1166, 2724, 1214),
    (2916, 1196, 2964, 1244),
    (3176, 1206, 3224, 1254),
    (3276, 1406, 3324, 1454),
)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture relative color and material-body evidence for Santa Marina stone."
    )
    parser.add_argument("--source-image", type=Path)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--proof", type=Path, default=DEFAULT_PROOF)
    parser.add_argument(
        "--inkblotter-root",
        type=Path,
        default=DEFAULT_INKBLOTTER_ROOT,
    )
    return parser.parse_args(argv)


def sha256_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def read_source_image(path: Path | None) -> bytes:
    if path is not None:
        return path.read_bytes()
    request = Request(
        SOURCE_URL,
        headers={"User-Agent": "iggy3d-material-research/2.0"},
    )
    with urlopen(request, timeout=90) as response:
        return response.read()


def luminance(colors: np.ndarray) -> np.ndarray:
    colors = colors.astype(np.float32)
    return (
        colors[..., 0] * 0.2126
        + colors[..., 1] * 0.7152
        + colors[..., 2] * 0.0722
    )


def color_hex(color: list[int]) -> str:
    return "#" + "".join(f"{int(channel):02x}" for channel in color)


def interpolate_palette(
    colors: np.ndarray,
    reduce_palette,
    *,
    count: int = 20,
) -> list[list[int]]:
    stride = max(1, len(colors) // 120000)
    tuples = [
        tuple(int(channel) for channel in color)
        for color in colors[::stride]
    ]
    reduced = reduce_palette(tuples, min(12, count))
    ordered = np.asarray(
        sorted(reduced, key=lambda rgb: float(luminance(np.asarray(rgb)))),
        dtype=np.float32,
    )
    source = np.linspace(0.0, 1.0, len(ordered), dtype=np.float32)
    target = np.linspace(0.0, 1.0, count, dtype=np.float32)
    result = np.stack(
        [
            np.interp(target, source, ordered[:, channel])
            for channel in range(3)
        ],
        axis=-1,
    )
    return np.clip(np.rint(result), 0, 255).astype(np.uint8).tolist()


def color_metrics(colors: np.ndarray) -> dict[str, Any]:
    value = luminance(colors)
    chroma = (
        colors.max(axis=-1).astype(np.float32)
        - colors.min(axis=-1).astype(np.float32)
    )
    return {
        "sample_count": int(len(colors)),
        "mean_rgb": np.rint(colors.mean(axis=0)).astype(np.uint8).tolist(),
        "median_rgb": np.rint(
            np.median(colors, axis=0)
        ).astype(np.uint8).tolist(),
        "luminance_percentiles": {
            key: float(np.percentile(value, percentile))
            for key, percentile in (
                ("p05", 5),
                ("p25", 25),
                ("p50", 50),
                ("p75", 75),
                ("p95", 95),
            )
        },
        "chroma_median": float(np.median(chroma) / 255.0),
    }


def normalized_sunlit_samples(
    image: np.ndarray,
) -> tuple[np.ndarray, list[dict[str, Any]]]:
    samples = [
        image[top:bottom, left:right].reshape(-1, 3).astype(np.float32)
        for left, top, right, bottom in SUNLIT_STONE_SAMPLES_PX
    ]
    medians = [float(np.median(luminance(sample))) for sample in samples]
    target = float(np.median(medians))
    normalized: list[np.ndarray] = []
    summary: list[dict[str, Any]] = []
    for rectangle, sample, sample_median in zip(
        SUNLIT_STONE_SAMPLES_PX,
        samples,
        medians,
        strict=True,
    ):
        scale = np.clip(target / max(sample_median, 1.0), 0.88, 1.12)
        adjusted = np.clip(sample * scale, 0.0, 255.0)
        normalized.append(adjusted)
        summary.append(
            {
                "rectangle_px": list(rectangle),
                "raw_median_luminance": sample_median,
                "normalization_scale": float(scale),
            }
        )
    return np.concatenate(normalized).astype(np.uint8), summary


def classified_color_families(
    colors: np.ndarray,
) -> dict[str, np.ndarray]:
    channels = colors.astype(np.float32)
    value = luminance(channels)
    warmth = channels[..., 0] - channels[..., 2]
    middle_value = (
        (value >= np.percentile(value, 30))
        & (value <= np.percentile(value, 70))
    )
    middle_warmth = (
        (warmth >= np.percentile(warmth, 30))
        & (warmth <= np.percentile(warmth, 70))
    )
    return {
        "cool_buff": colors[warmth <= np.percentile(warmth, 24)],
        "mid_buff": colors[middle_value & middle_warmth],
        "warm_ochre": colors[warmth >= np.percentile(warmth, 76)],
        "pale_cream": colors[value >= np.percentile(value, 76)],
    }


def material_entry(
    colors: np.ndarray,
    reduce_palette,
) -> dict[str, Any]:
    palette = interpolate_palette(colors, reduce_palette)
    return {
        "palette_20_rgb": palette,
        "palette_20_hex": [color_hex(color) for color in palette],
        "metrics": color_metrics(colors),
    }


def draw_palette_proof(
    capture: dict[str, Any],
    path: Path,
) -> None:
    color = capture["photographic_color"]
    rows = [
        ("all sunlit stone", color["all_sunlit_stone"]["palette_20_rgb"]),
        *[
            (name, entry["palette_20_rgb"])
            for name, entry in color["families"].items()
        ],
    ]
    swatch = 44
    label_width = 220
    footer = 52
    sheet = Image.new(
        "RGB",
        (label_width + swatch * 20, swatch * len(rows) + footer),
        "#151719",
    )
    draw = ImageDraw.Draw(sheet)
    for row_index, (label, palette) in enumerate(rows):
        y = row_index * swatch
        draw.text((10, y + 14), label, fill="#ede3ce")
        for color_index, color_rgb in enumerate(palette):
            x = label_width + color_index * swatch
            draw.rectangle(
                (x, y, x + swatch, y + swatch),
                fill=tuple(color_rgb),
            )
    footer_y = swatch * len(rows) + 8
    draw.text(
        (10, footer_y),
        "Relative sunlit colour only - Benjamin Smith / Wikimedia Commons - CC BY-SA 4.0",
        fill="#ede3ce",
    )
    draw.text(
        (10, footer_y + 18),
        "No source pixels are retained or used as runtime texture.",
        fill="#b9c0c6",
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(path)


def build_capture(payload: bytes, reduce_palette) -> dict[str, Any]:
    with Image.open(io.BytesIO(payload)) as source:
        image = np.asarray(source.convert("RGB"), dtype=np.uint8)
    if [image.shape[1], image.shape[0]] != [5688, 4493]:
        raise ValueError(
            f"Expected the catalogued 5688x4493 source, got {image.shape[1]}x{image.shape[0]}"
        )
    colors, sample_summary = normalized_sunlit_samples(image)
    families = {
        name: material_entry(family_colors, reduce_palette)
        for name, family_colors in classified_color_families(colors).items()
    }
    return {
        "schema": CAPTURE_SCHEMA,
        "source": {
            "asset_id": "Santa_Marina_Aguas_Santas_2024_facade",
            "display_name": "Cordoba - Santa Marina de Aguas Santas",
            "page": SOURCE_PAGE,
            "download": SOURCE_URL,
            "license": "CC BY-SA 4.0",
            "attribution": "Benjamin Smith / Wikimedia Commons",
            "captured_date": "2024-04-18",
            "resolution": [5688, 4493],
            "image_sha256": sha256_bytes(payload),
            "photographic_limit": (
                "Unpolarized, manually white-balanced sunlit photograph; "
                "not a calibrated reflectance capture."
            ),
        },
        "stone_body": {
            "measurement_id": "santa_marina_naranjo_biocalcarenite_body",
            "lithology_source": LITHOLOGY_SOURCE,
            "lithology": "Tortonian biocalcarenite and biomicrite with carbonated micritic cement",
            "likely_quarry": "Naranjo",
            "active_lithotypes": [
                "clastic sandy yellowish biocalcarenite",
                "sandy fine-grained biomicrite",
            ],
            "explicit_variant_only_lithotypes": [
                "conglomeratic biocalcarenite",
                "biosparite",
            ],
            "porosity_fraction": 0.13,
            "fossil_component_fraction": [0.3, 0.4],
            "fossil_components": [
                "foraminifera",
                "algae",
                "bryozoans",
                "bivalves",
                "echinoid bristles and plates",
            ],
            "nondegraded_naranjo_mineral_fractions": {
                "calcite": 0.8,
                "quartz": 0.15,
                "feldspar": 0.03,
                "clay": 0.02,
            },
            "iron_oxyhydroxide_role": (
                "Sparse visible colorant responsible for the more saturated "
                "yellow Naranjo material; not a universal orange wash."
            ),
            "surface_figure_scale_bars_m": {
                "macrophotographs": 0.05,
                "thin_sections": 0.001,
            },
            "relief_depth_status": "unknown; no normal or height is inferred",
            "optical_roughness_status": (
                "unknown; numeric PBR roughness remains an authored light-calibration parameter"
            ),
        },
        "photographic_color": {
            "measurement_limit": (
                "relative sunlit color families only; not absolute albedo, "
                "roughness, height, or normal"
            ),
            "sample_rectangles": sample_summary,
            "sample_exclusions": [
                "mortar",
                "sky",
                "cast shadow",
                "mouldings",
                "obvious cracks",
                "repairs",
                "eroded cavities",
            ],
            "normalization_rule": (
                "Each 48px stone-interior sample is scaled to the median "
                "sample luminance with a clamp of 0.88-1.12; hue and local "
                "within-stone variation are retained."
            ),
            "all_sunlit_stone": material_entry(colors, reduce_palette),
            "families": families,
            "family_rules": {
                "cool_buff": "lowest 24 percent red-minus-blue warmth",
                "mid_buff": "middle 40 percent luminance and warmth intersection",
                "warm_ochre": "highest 24 percent red-minus-blue warmth",
                "pale_cream": "highest 24 percent luminance",
            },
        },
        "production_translation": {
            "raw_pixels_used_as_runtime_texture": False,
            "source_pixels_retained_in_repository": False,
            "runtime_maps_are_deterministic": True,
            "photo_palette_role": (
                "Relative warm/cool family and value-order evidence only. "
                "Lighting is removed from the authored Base Color hierarchy."
            ),
            "lithology_layer_order": [
                "micritic carbonate matrix",
                "sandy silicate grains",
                "fragmented calcareous fossils",
                "visible pore identity without invented depth",
                "sparse iron oxyhydroxide color",
            ],
        },
    }


def main() -> None:
    args = parse_args(sys.argv[1:])
    if not args.inkblotter_root.is_dir():
        raise FileNotFoundError(args.inkblotter_root)
    sys.path.insert(0, str(args.inkblotter_root))
    from glyph_lab.reference_style import reduce_palette

    payload = read_source_image(args.source_image)
    capture = build_capture(payload, reduce_palette)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(capture, indent=2, sort_keys=True) + "\n"
    )
    draw_palette_proof(capture, args.proof)
    print(
        "cathedral stone reference captured:",
        capture["source"]["asset_id"],
        capture["source"]["license"],
        len(capture["photographic_color"]["families"]),
        "relative color families; raw pixels discarded",
    )


if __name__ == "__main__":
    main()
