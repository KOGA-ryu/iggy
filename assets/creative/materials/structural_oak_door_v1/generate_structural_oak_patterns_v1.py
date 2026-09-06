#!/usr/bin/env python3
"""Compile reference-derived door-wood colour fields into texture atlases."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

import numpy as np


SCRIPT_ROOT = Path(__file__).resolve().parent
MATERIALS_ROOT = SCRIPT_ROOT.parent
if str(MATERIALS_ROOT) not in sys.path:
    sys.path.insert(0, str(MATERIALS_ROOT))

from pattern_lab_common import (  # noqa: E402
    linear_to_srgb,
    mix,
    smoothstep,
    srgb_to_linear,
    write_png_rgb8,
)


PROFILE_PATH = SCRIPT_ROOT / "profiles" / "structural_oak_door_v1.json"
REFERENCE_FIELDS_PATH = (
    SCRIPT_ROOT / "patterns" / "structural_oak_reference_fields_v1.json"
)
DEFAULT_OUTPUT_ROOT = SCRIPT_ROOT / "output"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output-root",
        type=Path,
        default=DEFAULT_OUTPUT_ROOT,
    )
    parser.add_argument("--tile-resolution", type=int, default=512)
    return parser.parse_args()


def hex_linear(value: str) -> np.ndarray:
    value = value.removeprefix("#")
    srgb = np.array(
        [
            int(value[index : index + 2], 16) / 255.0
            for index in (0, 2, 4)
        ],
        dtype=np.float32,
    )
    return srgb_to_linear(srgb)


def interpolate_knots(
    values: list[float],
    x: np.ndarray,
) -> np.ndarray:
    authored = np.asarray(values, dtype=np.float32)
    position = np.clip(x, 0.0, 1.0) * (len(values) - 1)
    lower = np.floor(position).astype(np.int32)
    upper = np.minimum(lower + 1, len(values) - 1)
    blend = position - lower
    blend = blend * blend * (3.0 - 2.0 * blend)
    return (
        authored[lower] * (1.0 - blend)
        + authored[upper] * blend
    ).astype(np.float32)


def segment_envelope(
    x: np.ndarray,
    x0: float,
    x1: float,
) -> np.ndarray:
    fade = min(0.045, max((x1 - x0) * 0.16, 0.010))
    return (
        smoothstep(x0, x0 + fade, x)
        * (1.0 - smoothstep(x1 - fade, x1, x))
    ).astype(np.float32)


def path_mask(
    distance: np.ndarray,
    width: np.ndarray | float,
) -> np.ndarray:
    safe_width = np.maximum(width, 1.0e-5)
    return np.exp(-((distance / safe_width) ** 2)).astype(np.float32)


def broad_field_mask(
    distance: np.ndarray,
    width: np.ndarray,
) -> np.ndarray:
    safe_width = np.maximum(width, 1.0e-5)
    return (
        1.0
        - smoothstep(
            safe_width * np.float32(0.68),
            safe_width,
            distance,
        )
    ).astype(np.float32)


def load_inputs() -> tuple[dict, dict, dict, Path]:
    profile = json.loads(PROFILE_PATH.read_text())
    recipe = json.loads(REFERENCE_FIELDS_PATH.read_text())
    samples_path = (
        REFERENCE_FIELDS_PATH.parent
        / recipe["reference"]["palette_samples"]
    ).resolve()
    sample_document = json.loads(samples_path.read_text())
    return profile, recipe, sample_document, samples_path


def sample_table(
    sample_document: dict,
) -> tuple[dict[str, dict], list[str]]:
    samples = sample_document["eyedropper_samples"]["samples"]
    labels = [sample["label"] for sample in samples]
    if len(samples) != 20 or len(set(labels)) != 20:
        raise ValueError(
            "The locked reference palette must contain 20 unique samples"
        )
    return {sample["label"]: sample for sample in samples}, labels


def validate_reference_contract(
    profile: dict,
    recipe: dict,
    sample_document: dict,
) -> tuple[dict[str, dict], list[str]]:
    by_label, ordered_labels = sample_table(sample_document)
    sampled_hex = [by_label[label]["hex"] for label in ordered_labels]
    if sampled_hex != profile["palette"]["observed_reference_twenty"]:
        raise ValueError(
            "Profile palette has drifted from the locked eyedropper samples"
        )
    variants = recipe["variants"]
    expected = int(recipe["tile_columns"]) * int(recipe["tile_rows"])
    if len(variants) != expected:
        raise ValueError(
            "Reference-field variant count must fill the atlas grid"
        )
    for variant in variants:
        if variant["photo_crop"] not in recipe["photo_field"]["crops"]:
            raise ValueError(
                f"Unknown photo crop {variant['photo_crop']!r}"
            )
        for field in variant["colour_fields"]:
            if field["sample"] not in by_label:
                raise ValueError(
                    f"Unknown field sample {field['sample']!r}"
                )
        for fibre in variant["fibres"]:
            if fibre["sample"] not in by_label:
                raise ValueError(
                    f"Unknown fibre sample {fibre['sample']!r}"
                )
        unknown_emphasis = (
            set(variant["anchor_emphasis"]) - set(by_label)
        )
        if unknown_emphasis:
            raise ValueError(
                f"Unknown emphasized samples: {sorted(unknown_emphasis)}"
            )
    return by_label, ordered_labels


def load_reference_photo(recipe: dict) -> tuple[np.ndarray, Path]:
    try:
        import OpenImageIO as oiio
    except ImportError as error:
        raise RuntimeError(
            "OpenImageIO is required to read the locked real-door reference"
        ) from error
    path = Path(
        recipe["reference"]["local_reverse_path"]
    ).expanduser().resolve()
    if not path.is_file():
        raise FileNotFoundError(
            f"Locked real-door reference is missing: {path}"
        )
    image = oiio.ImageBuf(str(path))
    if image.has_error:
        raise RuntimeError(image.geterror())
    pixels = image.get_pixels(oiio.FLOAT)
    if pixels is None:
        raise RuntimeError(
            f"Could not read pixels from reference: {path}"
        )
    pixels = np.asarray(pixels, dtype=np.float32)
    if pixels.ndim != 3 or pixels.shape[2] < 3:
        raise RuntimeError(
            f"Unexpected reference image shape: {pixels.shape}"
        )
    return np.clip(pixels[..., :3], 0.0, 1.0), path


def hex_srgb(value: str) -> np.ndarray:
    value = value.removeprefix("#")
    return np.array(
        [
            int(value[index : index + 2], 16) / 255.0
            for index in (0, 2, 4)
        ],
        dtype=np.float32,
    )


def median_reduce(
    source: np.ndarray,
    output_height: int,
    output_width: int,
) -> np.ndarray:
    """Reduce a crop to robust broad cells without carrying fine damage."""

    source_height, source_width = source.shape[:2]
    y_edges = np.linspace(
        0,
        source_height,
        output_height + 1,
        dtype=np.int32,
    )
    x_edges = np.linspace(
        0,
        source_width,
        output_width + 1,
        dtype=np.int32,
    )
    reduced = np.zeros(
        (output_height, output_width, 3),
        dtype=np.float32,
    )
    for row in range(output_height):
        for column in range(output_width):
            cell = source[
                y_edges[row] : y_edges[row + 1],
                x_edges[column] : x_edges[column + 1],
            ]
            reduced[row, column] = np.median(
                cell.reshape(-1, 3),
                axis=0,
            )
    return reduced


def project_cells_to_reference_palette(
    cells: np.ndarray,
    samples: dict[str, dict],
    ordered_labels: list[str],
) -> np.ndarray:
    palette = np.stack(
        [
            hex_srgb(samples[label]["hex"])
            for label in ordered_labels
        ],
        axis=0,
    )
    flat = cells.reshape(-1, 3)
    distance = np.sum(
        (flat[:, np.newaxis, :] - palette[np.newaxis, :, :])
        ** 2,
        axis=2,
    )
    nearest = palette[np.argmin(distance, axis=1)]
    constrained = (
        flat * np.float32(0.35)
        + nearest * np.float32(0.65)
    )
    return constrained.reshape(cells.shape)


def resize_bilinear(
    source: np.ndarray,
    output_height: int,
    output_width: int,
) -> np.ndarray:
    source_height, source_width = source.shape[:2]
    source_x = np.linspace(0.0, 1.0, source_width)
    target_x = np.linspace(0.0, 1.0, output_width)
    horizontal = np.empty(
        (source_height, output_width, 3),
        dtype=np.float32,
    )
    for row in range(source_height):
        for channel in range(3):
            horizontal[row, :, channel] = np.interp(
                target_x,
                source_x,
                source[row, :, channel],
            )
    source_y = np.linspace(0.0, 1.0, source_height)
    target_y = np.linspace(0.0, 1.0, output_height)
    output = np.empty(
        (output_height, output_width, 3),
        dtype=np.float32,
    )
    for column in range(output_width):
        for channel in range(3):
            output[:, column, channel] = np.interp(
                target_y,
                source_y,
                horizontal[:, column, channel],
            )
    return output


def reference_photo_field(
    variant: dict,
    recipe: dict,
    reference_photo: np.ndarray,
    samples: dict[str, dict],
    ordered_labels: list[str],
    resolution: int,
) -> tuple[np.ndarray, np.ndarray]:
    crop = recipe["photo_field"]["crops"][variant["photo_crop"]]
    left, top, right, bottom = [
        int(value) for value in crop["box"]
    ]
    patch = reference_photo[top:bottom, left:right].copy()
    if not patch.size:
        raise ValueError(
            f"Empty photo crop for {variant['id']!r}: {crop['box']}"
        )
    if crop["long_axis"] == "vertical":
        patch = np.transpose(patch, (1, 0, 2))
    elif crop["long_axis"] != "horizontal":
        raise ValueError(
            f"Unknown crop long axis: {crop['long_axis']!r}"
        )
    if variant["photo_flip"]["length"]:
        patch = np.flip(patch, axis=1)
    if variant["photo_flip"]["width"]:
        patch = np.flip(patch, axis=0)
    reduced = median_reduce(
        patch,
        int(recipe["photo_field"]["transverse_cells"]),
        int(recipe["photo_field"]["longitudinal_cells"]),
    )
    constrained = project_cells_to_reference_palette(
        reduced,
        samples,
        ordered_labels,
    )
    cell_luminance = (
        0.2126 * constrained[..., 0]
        + 0.7152 * constrained[..., 1]
        + 0.0722 * constrained[..., 2]
    )
    transverse_gradient = np.abs(
        np.gradient(cell_luminance, axis=0)
    )
    row_score = np.mean(transverse_gradient, axis=1)
    selected_rows: list[int] = []
    for row in np.argsort(row_score)[::-1]:
        row = int(row)
        if all(abs(row - chosen) >= 4 for chosen in selected_rows):
            selected_rows.append(row)
        if len(selected_rows) == 5:
            break
    cell_ink = np.zeros_like(cell_luminance, dtype=np.float32)
    for row in selected_rows:
        along = transverse_gradient[row]
        along /= max(float(np.max(along)), 1.0e-5)
        line = np.clip(0.24 + along * 0.76, 0.0, 1.0)
        cell_ink[row] = np.maximum(cell_ink[row], line)
        if row > 0:
            cell_ink[row - 1] = np.maximum(
                cell_ink[row - 1],
                line * 0.34,
            )
        if row + 1 < cell_ink.shape[0]:
            cell_ink[row + 1] = np.maximum(
                cell_ink[row + 1],
                line * 0.34,
            )
    resized = resize_bilinear(
        constrained,
        resolution,
        resolution,
    )
    resized_ink = resize_bilinear(
        np.repeat(cell_ink[..., np.newaxis], 3, axis=-1),
        resolution,
        resolution,
    )[..., 0]
    return (
        srgb_to_linear(np.clip(resized, 0.0, 1.0)),
        np.clip(
            resized_ink
            * float(
                recipe["fibre_render"]["photo_ink_strength"]
            ),
            0.0,
            1.0,
        ),
    )


def reference_colour_field(
    variant: dict,
    recipe: dict,
    samples: dict[str, dict],
    ordered_labels: list[str],
    x: np.ndarray,
    y: np.ndarray,
) -> np.ndarray:
    """Reconstruct a smooth base field from the 20 measured door colours."""

    source_width, source_height = (
        recipe["_source_image_size"]
    )
    transform = variant["reference_transform"]
    reference_longitudinal = np.mod(
        x * float(transform["length_scale"])
        + float(transform["length_phase"]),
        1.0,
    )
    transverse = 1.0 - y if transform["mirror_width"] else y
    reference_transverse = np.clip(
        (transverse - 0.5) * float(transform["width_scale"])
        + 0.5
        + float(transform["width_shift"]),
        0.0,
        1.0,
    )
    radius_longitudinal = float(
        recipe["reference_field"]["longitudinal_radius"]
    )
    radius_transverse = float(
        recipe["reference_field"]["transverse_radius"]
    )
    weighted_colour = np.zeros(
        (*x.shape, 3),
        dtype=np.float32,
    )
    total_weight = np.zeros(x.shape, dtype=np.float32)
    emphasis = variant["anchor_emphasis"]
    for label in ordered_labels:
        sample = samples[label]
        sample_longitudinal = float(sample["y"]) / source_height
        sample_transverse = float(sample["x"]) / source_width
        longitudinal_distance = np.abs(
            reference_longitudinal - sample_longitudinal
        )
        longitudinal_distance = np.minimum(
            longitudinal_distance,
            1.0 - longitudinal_distance,
        )
        transverse_distance = np.abs(
            reference_transverse - sample_transverse
        )
        weight = np.exp(
            -(
                longitudinal_distance / radius_longitudinal
            )
            ** 2
            - (
                transverse_distance / radius_transverse
            )
            ** 2
        ).astype(np.float32)
        weight *= float(emphasis.get(label, 1.0))
        colour = hex_linear(sample["hex"])
        weighted_colour += weight[..., np.newaxis] * colour
        total_weight += weight
    return weighted_colour / np.maximum(
        total_weight[..., np.newaxis],
        1.0e-6,
    )


def apply_reference_glazes(
    colour: np.ndarray,
    variant: dict,
    recipe: dict,
    samples: dict[str, dict],
    ordered_labels: list[str],
    x: np.ndarray,
    y: np.ndarray,
) -> np.ndarray:
    source_width, source_height = recipe["_source_image_size"]
    transform = variant["reference_transform"]
    settings = recipe["reference_glazes"]
    minimum_width = float(settings["minimum_width"])
    maximum_width = float(settings["maximum_width"])
    opacity = float(settings["opacity"])
    for label in ordered_labels:
        sample = samples[label]
        reference_transverse = float(sample["x"]) / source_width
        local_centre = (
            reference_transverse
            - 0.5
            - float(transform["width_shift"])
        ) / float(transform["width_scale"]) + 0.5
        if transform["mirror_width"]:
            local_centre = 1.0 - local_centre
        sample_longitudinal = float(sample["y"]) / source_height
        direction = -1.0 if reference_transverse < 0.5 else 1.0
        drift = 0.010 + 0.028 * abs(sample_longitudinal - 0.5)
        centres = [
            local_centre,
            local_centre + direction * drift * 0.45,
            local_centre - direction * drift * 0.18,
            local_centre + direction * drift * 0.58,
            local_centre + direction * drift * 0.10,
        ]
        centre = interpolate_knots(centres, x)
        luminance = float(sample["luminance"]) / 255.0
        width = minimum_width + (
            maximum_width - minimum_width
        ) * luminance
        mask = path_mask(np.abs(y - centre), width)
        x0 = 0.02 + sample_longitudinal * 0.22
        x1 = 0.72 + sample_longitudinal * 0.24
        if variant["photo_flip"]["length"]:
            x0, x1 = 1.0 - x1, 1.0 - x0
        mask *= segment_envelope(x, x0, x1)
        mask *= opacity * (0.78 + 0.22 * luminance)
        target = np.broadcast_to(
            hex_linear(sample["hex"]),
            colour.shape,
        )
        colour = mix(colour, target, mask)
    return colour


def render_variant(
    variant: dict,
    recipe: dict,
    samples: dict[str, dict],
    ordered_labels: list[str],
    reference_photo: np.ndarray,
    resolution: int,
) -> tuple[np.ndarray, np.ndarray]:
    coordinate_x = np.linspace(
        0.0,
        1.0,
        resolution,
        dtype=np.float32,
    )
    coordinate_y = np.linspace(
        0.0,
        1.0,
        resolution,
        dtype=np.float32,
    )
    x = np.broadcast_to(
        coordinate_x[np.newaxis, :],
        (resolution, resolution),
    )
    y = np.broadcast_to(
        coordinate_y[:, np.newaxis],
        (resolution, resolution),
    )
    colour = reference_colour_field(
        variant,
        recipe,
        samples,
        ordered_labels,
        x,
        y,
    )
    photo_colour, photo_ink = reference_photo_field(
        variant,
        recipe,
        reference_photo,
        samples,
        ordered_labels,
        resolution,
    )
    photo_strength = np.float32(
        recipe["photo_field"]["blend_strength"]
    )
    colour = (
        colour * (np.float32(1.0) - photo_strength)
        + photo_colour * photo_strength
    )
    colour = apply_reference_glazes(
        colour,
        variant,
        recipe,
        samples,
        ordered_labels,
        x,
        y,
    )

    for field in variant["colour_fields"]:
        centre = interpolate_knots(field["centres"], x)
        width = interpolate_knots(field["widths"], x)
        mask = broad_field_mask(
            np.abs(y - centre),
            width * np.float32(0.58),
        )
        mask *= segment_envelope(
            x,
            float(field["x0"]),
            float(field["x1"]),
        )
        mask *= float(field["opacity"])
        target = np.broadcast_to(
            hex_linear(samples[field["sample"]]["hex"]),
            colour.shape,
        )
        colour = mix(colour, target, mask)

    ink = photo_ink
    for fibre in variant["fibres"]:
        centre = interpolate_knots(fibre["centres"], x)
        mask = path_mask(
            np.abs(y - centre),
            float(fibre["width"])
            * float(
                recipe["fibre_render"][
                    "authored_width_scale"
                ]
            ),
        )
        mask *= segment_envelope(
            x,
            float(fibre["x0"]),
            float(fibre["x1"]),
        )
        colour_mask = mask * float(fibre["opacity"])
        target = np.broadcast_to(
            hex_linear(samples[fibre["sample"]]["hex"]),
            colour.shape,
        )
        colour = mix(colour, target, colour_mask)
        ink = np.maximum(
            ink,
            mask * float(fibre["ink"]),
        )

    return (
        np.clip(colour, 0.0, 1.0),
        np.clip(
            ink * float(recipe["fibre_render"]["ink_gain"]),
            0.0,
            1.0,
        ),
    )


def render_palette_sheet(
    samples: dict[str, dict],
    ordered_labels: list[str],
    swatch_size: int = 128,
) -> np.ndarray:
    columns = 5
    rows = 4
    sheet = np.zeros(
        (rows * swatch_size, columns * swatch_size, 3),
        dtype=np.float32,
    )
    for index, label in enumerate(ordered_labels):
        row = rows - 1 - index // columns
        column = index % columns
        y0 = row * swatch_size
        x0 = column * swatch_size
        sheet[
            y0 : y0 + swatch_size,
            x0 : x0 + swatch_size,
        ] = hex_linear(samples[label]["hex"])
    return sheet


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def generate(
    output_root: Path = DEFAULT_OUTPUT_ROOT,
    *,
    tile_resolution: int = 512,
) -> dict:
    profile, recipe, sample_document, samples_path = load_inputs()
    samples, ordered_labels = validate_reference_contract(
        profile,
        recipe,
        sample_document,
    )
    source_size = sample_document["eyedropper_samples"]["image_size"]
    recipe["_source_image_size"] = source_size
    reference_photo, reference_photo_path = load_reference_photo(recipe)
    reference_height, reference_width = reference_photo.shape[:2]
    if [reference_width, reference_height] != source_size:
        raise ValueError(
            "Reference photo dimensions disagree with eyedropper samples"
        )
    columns = int(recipe["tile_columns"])
    rows = int(recipe["tile_rows"])
    variants = recipe["variants"]
    if tile_resolution < 128:
        raise ValueError("Tile resolution must be at least 128")

    colour_atlas = np.zeros(
        (
            rows * tile_resolution,
            columns * tile_resolution,
            3,
        ),
        dtype=np.float32,
    )
    ink_atlas = np.zeros_like(colour_atlas)
    variant_stats = []
    for index, variant in enumerate(variants):
        colour, ink = render_variant(
            variant,
            recipe,
            samples,
            ordered_labels,
            reference_photo,
            tile_resolution,
        )
        atlas_column = index % columns
        atlas_row_from_bottom = index // columns
        atlas_row = rows - 1 - atlas_row_from_bottom
        y0 = atlas_row * tile_resolution
        x0 = atlas_column * tile_resolution
        colour_atlas[
            y0 : y0 + tile_resolution,
            x0 : x0 + tile_resolution,
        ] = colour
        ink_atlas[
            y0 : y0 + tile_resolution,
            x0 : x0 + tile_resolution,
        ] = np.repeat(ink[..., np.newaxis], 3, axis=-1)
        luminance = (
            0.2126 * colour[..., 0]
            + 0.7152 * colour[..., 1]
            + 0.0722 * colour[..., 2]
        )
        variant_stats.append(
            {
                "id": variant["id"],
                "source_region": variant["source_region"],
                "reference_anchor_count": len(ordered_labels),
                "colour_field_count": len(
                    variant["colour_fields"]
                ),
                "fibre_count": len(variant["fibres"]),
                "ink_coverage": float(np.mean(ink > 0.08)),
                "linear_luminance_min": float(np.min(luminance)),
                "linear_luminance_max": float(np.max(luminance)),
            }
        )

    output_root = output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    colour_path = (
        output_root / "structural_oak_door_v1_colour_atlas.png"
    )
    ink_path = (
        output_root / "structural_oak_door_v1_ink_atlas.png"
    )
    palette_path = (
        output_root
        / "structural_oak_door_v1_reference_palette.png"
    )
    write_png_rgb8(colour_path, linear_to_srgb(colour_atlas))
    write_png_rgb8(ink_path, ink_atlas)
    palette_sheet = render_palette_sheet(samples, ordered_labels)
    write_png_rgb8(
        palette_path,
        linear_to_srgb(palette_sheet),
    )

    manifest = {
        "schema": "iggy-structural-oak-atlas-build/2.0",
        "status": "REFERENCE_FIELDS_COMPILED",
        "profile": str(PROFILE_PATH),
        "reference_fields": str(REFERENCE_FIELDS_PATH),
        "palette_samples": str(samples_path),
        "reference": {
            "authority": recipe["reference"]["authority"],
            "object_number": recipe["reference"]["object_number"],
            "object_url": recipe["reference"]["object_url"],
            "reverse_image_url": (
                recipe["reference"]["reverse_image_url"]
            ),
            "source_image_size": source_size,
            "sample_count": len(ordered_labels),
            "local_reverse_path": str(reference_photo_path),
            "local_reverse_sha256": sha256_file(
                reference_photo_path
            ),
            "palette_sample_sha256": sha256_file(samples_path),
            "reference_fields_sha256": sha256_file(
                REFERENCE_FIELDS_PATH
            ),
        },
        "tile_resolution": tile_resolution,
        "columns": columns,
        "rows": rows,
        "constraints": {
            "ai_generated_imagery": False,
            "floor_pattern_source": False,
            "periodic_grain_functions": False,
            "random_noise": False,
            "surface_variation_maps": False,
            "relief_maps": False,
            "damage_maps": False,
        },
        "variants": variant_stats,
        "outputs": {
            "colour": {
                "path": str(colour_path),
                "bytes": colour_path.stat().st_size,
                "sha256": sha256_file(colour_path),
            },
            "ink": {
                "path": str(ink_path),
                "bytes": ink_path.stat().st_size,
                "sha256": sha256_file(ink_path),
            },
            "reference_palette": {
                "path": str(palette_path),
                "bytes": palette_path.stat().st_size,
                "sha256": sha256_file(palette_path),
            },
        },
    }
    manifest_path = (
        output_root
        / "structural_oak_door_v1_atlas_manifest.json"
    )
    manifest_path.write_text(
        json.dumps(manifest, indent=2) + "\n"
    )
    return manifest


def main() -> None:
    args = parse_args()
    manifest = generate(
        args.output_root,
        tile_resolution=args.tile_resolution,
    )
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
