#!/usr/bin/env python3
"""Generate the measured chevron-voussoir material texture family."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
from typing import Any

import numpy as np


SCRIPT_ROOT = Path(__file__).resolve().parent
MATERIALS_ROOT = SCRIPT_ROOT.parent
if str(MATERIALS_ROOT) not in sys.path:
    sys.path.insert(0, str(MATERIALS_ROOT))

from pattern_lab_common import (  # noqa: E402
    normalized_range,
    periodic_fbm_rect,
    periodic_gaussian_blur,
    write_png_rgb8,
)


PROFILE_SCHEMA = "iggy3d.material.cathedral_stone_trim_fracture_v1.profile.v2"
PATTERN_SCHEMA = "iggy3d.pattern.chevron_voussoir_portal.v2"
MANIFEST_SCHEMA = "iggy3d.material.cathedral_stone_trim_fracture_v1.manifest.v2"
DEFAULT_PROFILE = SCRIPT_ROOT / "profiles" / "cathedral_stone_trim_fracture_v1.json"
DEFAULT_PATTERN = SCRIPT_ROOT / "patterns" / "cathedral_trim_fracture_atlas_v1.json"
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"
CORE_OUTPUT = MATERIALS_ROOT / "cathedral_stone_v1" / "output"
CORE_MANIFEST = CORE_OUTPUT / "cathedral_stone_v1_manifest.json"
RESOLUTION = 1024


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_contract(
    profile: dict[str, Any],
    pattern: dict[str, Any],
) -> None:
    if profile.get("schema") != PROFILE_SCHEMA:
        raise ValueError("Unexpected chevron material profile schema")
    if pattern.get("schema") != PATTERN_SCHEMA:
        raise ValueError("Unexpected chevron pattern schema")
    construction = profile["construction_contract"]
    completion = pattern["authored_completion"]
    exact_pairs = (
        ("count", 16),
        ("pitch_angle_deg", 11.25),
        ("joint_gap_centerline_m", 0.003),
        ("body_angle_deg", 11.042294066),
        ("inner_radius_m", 0.7275514105),
        ("outer_radius_m", 0.9275514105),
        ("derived_outer_chord_m", 0.1784852529),
    )
    for key, expected in exact_pairs:
        actual = construction[key]
        if isinstance(expected, int):
            if actual != expected or completion[key] != expected:
                raise ValueError(f"{key} drifted from the measured chevron contract")
        elif not (
            abs(float(actual) - expected) <= 1.0e-10
            and abs(float(completion[key]) - expected) <= 1.0e-10
        ):
            raise ValueError(f"{key} drifted from the measured chevron contract")
    if construction["arris_rule"] != (
        "Unknown in the measured source; no bevel is authored."
    ):
        raise ValueError("Unknown arris radius must remain unbevelled")
    exclusions = set(pattern["exclusions"])
    required_exclusions = {
        "jamb reconstruction",
        "fracture",
        "edge damage",
        "wear",
        "weathering",
        "soot",
        "damp",
        "lichen",
        "Unreal parity",
    }
    if exclusions != required_exclusions:
        raise ValueError("Capability exclusions changed without new evidence")
    inventory = profile["texture_inventory"]
    if set(inventory) != {
        "basecolor",
        "orm",
        "body_masks",
        "body_normal",
        "body_height",
    }:
        raise ValueError("Texture inventory must contain exactly five consumed lanes")
    for lane, entry in inventory.items():
        if entry["resolution"] != [RESOLUTION, RESOLUTION]:
            raise ValueError(f"{lane} must be authored at 1024 square")
        expected_mpt = entry["physical_span_m"] / RESOLUTION
        if abs(entry["metres_per_texel"] - expected_mpt) > 1.0e-12:
            raise ValueError(f"{lane} metres-per-texel is inconsistent")


def stone_palette() -> np.ndarray:
    values = (
        (161, 132, 101),
        (166, 137, 104),
        (171, 142, 108),
        (175, 146, 112),
        (179, 150, 116),
        (183, 154, 119),
        (186, 158, 123),
        (189, 161, 126),
        (192, 164, 130),
        (194, 167, 134),
        (196, 170, 138),
        (198, 172, 141),
        (200, 175, 145),
        (202, 177, 148),
        (204, 179, 151),
        (206, 181, 154),
        (208, 184, 158),
        (210, 186, 161),
        (212, 188, 164),
        (214, 191, 168),
    )
    return np.asarray(values, dtype=np.float32) / 255.0


def build_broad_fields(seed: int) -> tuple[np.ndarray, np.ndarray]:
    broad = periodic_fbm_rect(
        RESOLUTION,
        cells_x=4,
        cells_y=3,
        seed=seed,
        octaves=4,
        persistence=0.52,
    )
    broad = periodic_gaussian_blur(broad, sigma_px=18.0)
    broad = normalized_range(broad)
    wash = periodic_fbm_rect(
        RESOLUTION,
        cells_x=7,
        cells_y=5,
        seed=seed + 101,
        octaves=3,
        persistence=0.45,
    )
    wash = periodic_gaussian_blur(wash, sigma_px=8.0)
    wash = normalized_range(wash)
    combined = np.clip(0.72 * broad + 0.28 * wash, 0.0, 1.0)
    return combined.astype(np.float32), wash.astype(np.float32)


def quantized_palette_field(
    field: np.ndarray,
    palette: np.ndarray,
) -> np.ndarray:
    position = np.clip(field, 0.0, 1.0) * (len(palette) - 1)
    lower = np.floor(position).astype(np.int32)
    upper = np.minimum(lower + 1, len(palette) - 1)
    blend = (position - lower)[..., np.newaxis]
    return (
        palette[lower] * (1.0 - blend) + palette[upper] * blend
    ).astype(np.float32)


def build_basecolor(seed: int) -> tuple[np.ndarray, np.ndarray]:
    broad, wash = build_broad_fields(seed)
    color = quantized_palette_field(broad, stone_palette())
    x = np.arange(RESOLUTION, dtype=np.float32)[np.newaxis, :] / RESOLUTION
    y = np.arange(RESOLUTION, dtype=np.float32)[:, np.newaxis] / RESOLUTION
    directional = (
        0.5
        + 0.5
        * np.sin(
            2.0 * np.pi * (2.0 * x + y)
            + 0.35 * np.sin(2.0 * np.pi * 3.0 * y)
        )
    )
    glaze = (directional - 0.5) * 0.022 + (wash - 0.5) * 0.028
    warm = np.asarray([1.0, 0.79, 0.57], dtype=np.float32)
    cool = np.asarray([0.69, 0.72, 0.70], dtype=np.float32)
    color = color * (1.0 + glaze[..., np.newaxis])
    color = color * (
        1.0
        + (wash - 0.5)[..., np.newaxis]
        * (warm - cool)[np.newaxis, np.newaxis, :]
        * 0.055
    )
    return np.clip(color, 0.0, 1.0), broad


def build_orm(broad: np.ndarray) -> np.ndarray:
    roughness = np.clip(0.72 + (broad - 0.5) * 0.11, 0.66, 0.82)
    orm = np.empty((RESOLUTION, RESOLUTION, 3), dtype=np.float32)
    orm[..., 0] = 1.0
    orm[..., 1] = roughness
    orm[..., 2] = 0.0
    return orm


def copy_measured_body_lanes(output: Path) -> dict[str, Path]:
    required = {
        "body_masks": CORE_OUTPUT / "cathedral_stone_v1_stone_body_masks.png",
        "body_normal": CORE_OUTPUT / "cathedral_stone_v1_stone_body_normal.png",
        "body_height": CORE_OUTPUT / "cathedral_stone_v1_stone_body_height.png",
    }
    destinations: dict[str, Path] = {}
    for lane, source in required.items():
        if not source.is_file():
            raise FileNotFoundError(
                f"Accepted cathedral stone body lane is missing: {source}"
            )
        destination = output / f"cathedral_stone_trim_fracture_v1_{lane}.png"
        shutil.copyfile(source, destination)
        destinations[lane] = destination
    return destinations


def generate(
    profile_path: Path,
    pattern_path: Path,
    output: Path,
) -> dict[str, Any]:
    profile = read_json(profile_path)
    pattern = read_json(pattern_path)
    validate_contract(profile, pattern)
    output.mkdir(parents=True, exist_ok=True)
    seed = int(profile["seed"])
    basecolor, broad = build_basecolor(seed)
    orm = build_orm(broad)
    files = {
        "basecolor": output / "cathedral_stone_trim_fracture_v1_basecolor.png",
        "orm": output / "cathedral_stone_trim_fracture_v1_orm.png",
    }
    write_png_rgb8(files["basecolor"], basecolor)
    write_png_rgb8(files["orm"], orm)
    files.update(copy_measured_body_lanes(output))
    core_manifest = read_json(CORE_MANIFEST)
    manifest = {
        "schema": MANIFEST_SCHEMA,
        "profile_schema": profile["schema"],
        "pattern_schema": pattern["schema"],
        "profile_id": profile["profile_id"],
        "pattern_id": pattern["pattern_id"],
        "seed": seed,
        "source_measurement": pattern["source_measurement"],
        "authored_completion": pattern["authored_completion"],
        "moulding": pattern["moulding"],
        "texture_inventory": profile["texture_inventory"],
        "source_policy": profile["source_policy"],
        "inherited_body_contract": {
            "source_package": "cathedral_stone_v1",
            "source_manifest_schema": core_manifest["schema"],
            "relief_proxy_measurement_id": (
                "sabucina_calcarenite_surface_topography_proxy"
            ),
            "body_relief_range_m": 0.00113,
            "raw_reference_pixels_used": False,
            "ashlar_construction_pattern_inherited": False,
        },
        "files": {
            lane: {
                "filename": path.name,
                "sha256": sha256(path),
                "bytes": path.stat().st_size,
            }
            for lane, path in sorted(files.items())
        },
        "validation": {
            "texture_lane_count": len(files),
            "twenty_shade_palette": len(stone_palette()) == 20,
            "orm_ao_constant_one": True,
            "orm_metallic_constant_zero": True,
            "broad_color_not_one_block_one_color": (
                float(np.std(basecolor)) > 0.025
            ),
            "accepted_body_maps_copied_byte_exact": all(
                sha256(files[lane])
                == sha256(
                    CORE_OUTPUT
                    / f"cathedral_stone_v1_stone_{lane}.png"
                )
                for lane in ("body_masks", "body_normal", "body_height")
            ),
            "damage_authored": False,
            "fracture_authored": False,
        },
    }
    manifest_path = output / "cathedral_stone_trim_fracture_v1_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    return manifest


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", type=Path, default=DEFAULT_PROFILE)
    parser.add_argument("--pattern", type=Path, default=DEFAULT_PATTERN)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args(argv)


def main() -> int:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    args = parse_args(argv)
    manifest = generate(args.profile, args.pattern, args.output)
    print(
        json.dumps(
            {
                "status": "ok",
                "schema": manifest["schema"],
                "files": sorted(manifest["files"]),
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
