#!/usr/bin/env python3
"""Bake clean joinery faces as physical slices of the timber growth volume."""

from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import sys
from typing import Any

import numpy as np


SCRIPT_ROOT = Path(__file__).resolve().parent
MATERIALS_ROOT = SCRIPT_ROOT.parent
if str(MATERIALS_ROOT) not in sys.path:
    sys.path.insert(0, str(MATERIALS_ROOT))

from pattern_lab_common import (  # noqa: E402
    compose_material_chapter,
    linear_to_srgb,
    mix,
    smoothstep,
    write_png_gray16,
    write_png_rgb8,
)


PROFILE_PATH = SCRIPT_ROOT / "profiles" / "structural_oak_joinery_v1.json"
PATTERN_PATH = (
    SCRIPT_ROOT / "patterns" / "structural_oak_endgrain_variants_v1.json"
)
DEFAULT_OUTPUT_ROOT = SCRIPT_ROOT / "output"
PROFILE_SCHEMA = "iggy3d.material.structural_oak_joinery.v2"
PATTERN_SCHEMA = "iggy3d.pattern.structural_oak_joinery_cut_atlas.v2"
PREFIX = "structural_oak_joinery_v1"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", type=Path, default=PROFILE_PATH)
    parser.add_argument("--pattern", type=Path, default=PATTERN_PATH)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--tile-resolution", type=int)
    return parser.parse_args()


def _resolve(profile_path: Path, relative: str) -> Path:
    return (profile_path.parent.parent / relative).resolve()


def _load_module(path: Path):
    spec = importlib.util.spec_from_file_location(
        "structural_oak_timber_v1_shared_generator",
        path,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load shared timber generator: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def load_contract(
    profile_path: str | Path = PROFILE_PATH,
    pattern_path: str | Path = PATTERN_PATH,
) -> dict[str, Any]:
    profile_source = Path(profile_path).resolve()
    pattern_source = Path(pattern_path).resolve()
    profile = json.loads(profile_source.read_text())
    pattern = json.loads(pattern_source.read_text())
    if profile.get("schema") != PROFILE_SCHEMA:
        raise ValueError(
            f"{profile_source} uses {profile.get('schema')!r}; "
            f"expected {PROFILE_SCHEMA!r}"
        )
    if pattern.get("schema") != PATTERN_SCHEMA:
        raise ValueError(
            f"{pattern_source} uses {pattern.get('schema')!r}; "
            f"expected {PATTERN_SCHEMA!r}"
        )
    if len(pattern["tiles"]) != (
        int(pattern["atlas_columns"]) * int(pattern["atlas_rows"])
    ):
        raise ValueError("cut-face tiles do not fill the declared atlas")

    dependencies = profile["source_dependencies"]
    timber_profile_path = _resolve(profile_source, dependencies["timber_profile"])
    timber_pattern_path = _resolve(profile_source, dependencies["timber_pattern"])
    timber_generator_path = _resolve(
        profile_source, dependencies["timber_generator"]
    )
    timber_blend_path = _resolve(profile_source, dependencies["timber_blend"])
    timber_generator = _load_module(timber_generator_path)
    timber_bundle = timber_generator.load_bundle(
        timber_profile_path,
        timber_pattern_path,
    )
    return {
        "profile_path": profile_source,
        "pattern_path": pattern_source,
        "timber_profile_path": timber_profile_path,
        "timber_pattern_path": timber_pattern_path,
        "timber_generator_path": timber_generator_path,
        "timber_blend_path": timber_blend_path,
        "profile": profile,
        "pattern": pattern,
        "timber_generator": timber_generator,
        "timber_bundle": timber_bundle,
    }


def compile_shared_growth(contract: dict[str, Any]) -> dict[str, Any]:
    table = contract["timber_generator"].compile_ring_table(
        contract["timber_bundle"]["pattern"]
    )
    return {
        "boundaries_m": table["boundaries_m"],
        "widths_m": table["widths_m"],
        "digest": table["digest"],
        "ring_count": len(table["widths_m"]),
    }


def _axes(
    tile: dict[str, Any],
    resolution: int,
) -> tuple[np.ndarray, np.ndarray]:
    u_min, u_max = tile["axis_u_range_m"]
    u_axis = np.linspace(
        float(u_min),
        float(u_max),
        resolution,
        endpoint=False,
        dtype=np.float32,
    )
    u_axis += (float(u_max) - float(u_min)) / resolution * 0.5
    if tile["projection"] in {"Y_THETA", "TENON_THETA"}:
        v_min, v_max = tile["theta_range_rad"]
    else:
        v_min, v_max = tile["axis_v_range_m"]
    v_axis = np.linspace(
        float(v_min),
        float(v_max),
        resolution,
        endpoint=False,
        dtype=np.float32,
    )
    v_axis += (float(v_max) - float(v_min)) / resolution * 0.5
    return np.meshgrid(u_axis, v_axis)


def _growth_coordinates(
    contract: dict[str, Any],
    tile: dict[str, Any],
    coordinate_u: np.ndarray,
    coordinate_v: np.ndarray,
) -> tuple[
    dict[str, Any],
    np.ndarray,
    np.ndarray,
    np.ndarray,
    dict[str, np.ndarray],
]:
    profile = contract["profile"]
    timber_bundle = contract["timber_bundle"]
    source = tile["source"]
    if source == "receiving_timber":
        growth_profile = timber_bundle["profile"]
        longitudinal_offset = (
            float(profile["joint_fixture_m"]["receiving_timber"]["length"]) * 0.5
        )
    elif source == "entering_timber":
        growth_profile = copy.deepcopy(timber_bundle["profile"])
        length = float(
            profile["joint_fixture_m"]["entering_timber"]["length"]
        )
        growth_profile["beam_fixture_m"]["length"] = length
        longitudinal_offset = 0.0
    else:
        raise ValueError(f"{tile['id']} has no tree growth coordinates")

    projection = tile["projection"]
    plane = float(tile.get("plane_m", 0.0))
    if projection == "YZ":
        x_m = np.full_like(coordinate_u, plane)
        y_m = coordinate_u
        z_m = coordinate_v
    elif projection == "XZ":
        x_m = coordinate_u
        y_m = np.full_like(coordinate_u, plane)
        z_m = coordinate_v
    elif projection == "XY":
        x_m = coordinate_u
        y_m = coordinate_v
        z_m = np.full_like(coordinate_u, plane)
    elif projection == "Y_THETA":
        peg = profile["joint_fixture_m"]["peg"]
        radius = float(peg["bore_diameter"]) * 0.5
        center_x = float(profile["joint_fixture_m"]["joint_center_x"])
        center_z = (
            float(profile["joint_fixture_m"]["receiving_timber"]["height"]) * 0.5
            - float(peg["mortise_hole_depth_from_reference_face"])
        )
        x_m = center_x + radius * np.cos(coordinate_v)
        y_m = coordinate_u
        z_m = center_z + radius * np.sin(coordinate_v)
    elif projection == "TENON_THETA":
        fixture = profile["joint_fixture_m"]
        radius = float(fixture["peg"]["bore_diameter"]) * 0.5
        center_x = (
            float(fixture["entering_timber"]["length"])
            - float(fixture["tenon"]["length"])
            + float(fixture["peg"]["mortise_hole_depth_from_reference_face"])
            - float(fixture["peg"]["drawbore_offset"])
        )
        x_m = center_x + radius * np.cos(coordinate_v)
        y_m = coordinate_u
        z_m = radius * np.sin(coordinate_v)
    else:
        raise ValueError(f"unsupported physical projection {projection!r}")

    u_m = x_m + longitudinal_offset
    growth = contract["timber_generator"].evaluate_growth_volume(
        growth_profile,
        timber_bundle["pattern"],
        contract["ring_table"],
        u_m=u_m,
        y_m=y_m,
        z_m=z_m,
    )
    return growth_profile, u_m, y_m, z_m, growth


def _physical_tile(
    contract: dict[str, Any],
    tile: dict[str, Any],
    coordinate_u: np.ndarray,
    coordinate_v: np.ndarray,
) -> dict[str, np.ndarray]:
    (
        growth_profile,
        _u_m,
        _y_m,
        _z_m,
        growth,
    ) = _growth_coordinates(contract, tile, coordinate_u, coordinate_v)
    timber_bundle = contract["timber_bundle"]
    pattern = timber_bundle["pattern"]
    palette = timber_bundle["palette_linear"]
    ring_index = growth["ring_index"]
    ring_phase = growth["ring_phase"]
    early_cycle = np.asarray(
        pattern["growth_volume"]["earlywood_fraction_cycle"],
        dtype=np.float32,
    )
    early_fraction = early_cycle[ring_index % len(early_cycle)]
    latewood = smoothstep(
        early_fraction,
        np.minimum(early_fraction + 0.16, 0.96),
        ring_phase,
    )
    color_cycle = np.asarray(
        pattern["growth_volume"]["ring_colour_palette_cycle"], dtype=np.int16
    )
    ring_palette = palette[color_cycle[ring_index % len(color_cycle)]]
    palette_indices = contract["pattern"]["fresh_cut_palette_indices"]
    base_index = int(palette_indices[int(tile["index"]) % len(palette_indices)])
    base = np.broadcast_to(palette[base_index], (*ring_phase.shape, 3)).copy()
    base = mix(base, ring_palette, 0.075 + latewood * 0.095)
    base = mix(
        base,
        np.broadcast_to(palette[17], base.shape),
        np.full_like(ring_phase, float(tile["fresh_cut_tint"])),
    )

    normalized_u = (
        coordinate_u - float(tile["axis_u_range_m"][0])
    ) / (
        float(tile["axis_u_range_m"][1])
        - float(tile["axis_u_range_m"][0])
    )
    if tile["projection"] in {"Y_THETA", "TENON_THETA"}:
        normalized_v = (
            coordinate_v - float(tile["theta_range_rad"][0])
        ) / (
            float(tile["theta_range_rad"][1])
            - float(tile["theta_range_rad"][0])
        )
    else:
        normalized_v = (
            coordinate_v - float(tile["axis_v_range_m"][0])
        ) / (
            float(tile["axis_v_range_m"][1])
            - float(tile["axis_v_range_m"][0])
        )
    for passage in contract["pattern"]["cut_colour_passages"]:
        if int(passage["tile"]) != int(tile["index"]):
            continue
        distance = (
            (normalized_u - float(passage["centre_uv"][0]))
            / float(passage["radius_uv"][0])
        ) ** 2 + (
            (normalized_v - float(passage["centre_uv"][1]))
            / float(passage["radius_uv"][1])
        ) ** 2
        mask = (
            1.0 - smoothstep(0.25, 1.0, distance)
        ) * float(passage["strength"])
        base = mix(
            base,
            np.broadcast_to(
                palette[int(passage["palette_index"])],
                base.shape,
            ),
            mask,
        )

    projection = tile["projection"]
    transverse = projection == "YZ"
    ring_boundary = np.maximum(
        1.0 - smoothstep(0.0, 0.12, ring_phase),
        smoothstep(0.86, 1.0, ring_phase),
    )
    if projection in {"Y_THETA", "TENON_THETA"}:
        meters_per_pixel = min(
            (
                float(tile["axis_u_range_m"][1])
                - float(tile["axis_u_range_m"][0])
            )
            / coordinate_u.shape[1],
            (
                float(contract["profile"]["joint_fixture_m"]["peg"][
                    "bore_diameter"
                ])
                * math.pi
            )
            / coordinate_u.shape[0],
        )
    else:
        meters_per_pixel = min(
            (
                float(tile["axis_u_range_m"][1])
                - float(tile["axis_u_range_m"][0])
            )
            / coordinate_u.shape[1],
            (
                float(tile["axis_v_range_m"][1])
                - float(tile["axis_v_range_m"][0])
            )
            / coordinate_u.shape[0],
        )
    if transverse:
        wide_ray, narrow_ray = contract["timber_generator"]._ray_mask(
            pattern,
            growth["radius_m"],
            growth["angle_rad"],
            meters_per_pixel,
        )
        ray = np.maximum(wide_ray, narrow_ray * 0.58)
        pore = contract["timber_generator"]._pore_mask(
            timber_bundle,
            growth,
            meters_per_pixel,
        )
        base = mix(
            base,
            np.broadcast_to(palette[13], base.shape),
            ray * 0.27,
        )
        base = mix(
            base,
            np.broadcast_to(palette[0], base.shape),
            pore * 0.58,
        )
        height = (
            ring_boundary * 0.000013
            + ray * 0.000021
            - pore * 0.000062
        )
    else:
        ray = np.zeros_like(ring_phase, dtype=np.float32)
        pore = np.zeros_like(ring_phase, dtype=np.float32)
        height = (latewood - 0.5) * (
            0.000007 + growth["ring_width_m"] * 0.005
        )

    maximum = 0.00012
    height = np.clip(height, -maximum, maximum).astype(np.float32)
    if projection in {"Y_THETA", "TENON_THETA"}:
        scale_y = (
            float(contract["profile"]["joint_fixture_m"]["peg"][
                "bore_diameter"
            ])
            * math.pi
        ) / coordinate_u.shape[0]
    else:
        scale_y = (
            float(tile["axis_v_range_m"][1])
            - float(tile["axis_v_range_m"][0])
        ) / coordinate_u.shape[0]
    scale_x = (
        float(tile["axis_u_range_m"][1])
        - float(tile["axis_u_range_m"][0])
    ) / coordinate_u.shape[1]
    normal = contract["timber_generator"]._height_to_normal_rect(
        height,
        meters_per_pixel_x=scale_x,
        meters_per_pixel_y=scale_y,
        strength=float(contract["profile"]["representation"]["normal_strength"]),
    )
    identity = np.stack([ring_boundary, ray, pore], axis=-1).astype(np.float32)
    return {
        "base_color_linear": np.clip(base, 0.0, 1.0).astype(np.float32),
        "normal": normal,
        "height_m": height,
        "identity": identity,
    }


def _edge_pad(image: np.ndarray, padding: int) -> np.ndarray:
    if padding <= 0:
        return image
    result = image.copy()
    result[:padding] = result[padding : padding + 1]
    result[-padding:] = result[-padding - 1 : -padding]
    result[:, :padding] = result[:, padding : padding + 1]
    result[:, -padding:] = result[:, -padding - 1 : -padding]
    return result


def generate_atlas(
    contract: dict[str, Any],
    *,
    tile_resolution: int,
) -> dict[str, Any]:
    if tile_resolution < 64:
        raise ValueError("cut atlas tiles must be at least sixty-four pixels")
    contract = dict(contract)
    shared = compile_shared_growth(contract)
    contract["ring_table"] = {
        "boundaries_m": shared["boundaries_m"],
        "widths_m": shared["widths_m"],
        "digest": shared["digest"],
    }
    padding = min(
        int(contract["profile"]["representation"]["tile_padding_px"]),
        max(1, tile_resolution // 16),
    )
    tile_results = []
    semantic_tiles = []
    for tile in contract["pattern"]["tiles"]:
        coordinate_u, coordinate_v = _axes(tile, tile_resolution)
        if tile["projection"] == "PEG":
            result = _peg_tile(contract, tile, coordinate_u, coordinate_v)
        else:
            result = _physical_tile(contract, tile, coordinate_u, coordinate_v)
        tile_results.append(
            {
                key: _edge_pad(value, padding)
                for key, value in result.items()
            }
        )
        first_class = int(tile["surface_classes"][0])
        projection_code = {
            "YZ": 0.2,
            "XZ": 0.4,
            "XY": 0.6,
            "Y_THETA": 0.8,
            "TENON_THETA": 1.0,
        }[tile["projection"]]
        semantic = np.zeros(
            (tile_resolution, tile_resolution, 3), dtype=np.float32
        )
        semantic[..., 0] = first_class / 11.0
        semantic[..., 1] = int(tile["index"]) / 7.0
        semantic[..., 2] = projection_code
        semantic_tiles.append(semantic)

    columns = int(contract["pattern"]["atlas_columns"])
    rows = int(contract["pattern"]["atlas_rows"])

    def compose(key: str) -> np.ndarray:
        row_images = []
        for row in range(rows):
            start = row * columns
            row_images.append(
                np.concatenate(
                    [
                        tile_results[index][key]
                        for index in range(start, start + columns)
                    ],
                    axis=1,
                )
            )
        return np.concatenate(row_images, axis=0)

    semantic_rows = [
        np.concatenate(
            semantic_tiles[row * columns : (row + 1) * columns],
            axis=1,
        )
        for row in range(rows)
    ]
    return {
        "base_color_linear": compose("base_color_linear"),
        "normal": compose("normal"),
        "height_m": compose("height_m"),
        "identity": compose("identity"),
        "semantic": np.concatenate(semantic_rows, axis=0),
        "tiles": tile_results,
        "shared_ring_digest": shared["digest"],
        "ring_count": shared["ring_count"],
    }


def _book(
    contract: dict[str, Any],
    atlas: dict[str, Any],
) -> np.ndarray:
    panel_labels = {
        "mortise_and_housing_bearing": "MORTISE + HOUSING BED",
        "tenon_drawbore_cylinder": "TENON DRAWBORE",
    }
    panels = [
        (
            panel_labels.get(tile["id"], tile["id"].upper()),
            linear_to_srgb(atlas["tiles"][int(tile["index"])]["base_color_linear"]),
        )
        for tile in contract["pattern"]["tiles"]
    ]
    return compose_material_chapter(
        "STRUCTURAL OAK JOINERY / CLEAN CUT ATLAS",
        "MORTISE  |  HOUSING  |  TENON  |  DRAWBORE  |  SHARED GROWTH VOLUME",
        panels,
        [
            "Every tile is evaluated at physical X/Y/Z coordinates against the timber champion's 832-ring table.",
            "Exact Boolean material transfer selects generated faces; corner semantics replace the temporary marker immediately.",
            "Tenon is 1/4 stock width, housing is 25.4 mm deep, peg is 38 mm octagonal, and draw is 2.4 mm.",
            "No damage, roughness map, repair, grime, fantasy pores, or decorative centimetre-scale growth rings.",
        ],
    )


def build_outputs(
    contract: dict[str, Any],
    output_root: str | Path,
    *,
    tile_resolution: int,
) -> dict[str, Any]:
    output = Path(output_root)
    output.mkdir(parents=True, exist_ok=True)
    atlas = generate_atlas(contract, tile_resolution=tile_resolution)
    maximum_height = 0.00012
    paths = {
        "basecolor": output / f"{PREFIX}_cut_basecolor.png",
        "normal": output / f"{PREFIX}_cut_normal.png",
        "height": output / f"{PREFIX}_cut_height.png",
        "identity": output / f"{PREFIX}_cut_identity.png",
        "semantic": output / f"{PREFIX}_cut_semantic.png",
        "book": output / f"{PREFIX}_cut_atlas_book.png",
    }
    write_png_rgb8(paths["basecolor"], linear_to_srgb(atlas["base_color_linear"]))
    write_png_rgb8(paths["normal"], atlas["normal"] * 0.5 + 0.5)
    write_png_gray16(
        paths["height"],
        np.clip(
            atlas["height_m"] / (2.0 * maximum_height) + 0.5,
            0.0,
            1.0,
        ),
    )
    write_png_rgb8(paths["identity"], atlas["identity"])
    write_png_rgb8(paths["semantic"], atlas["semantic"])
    write_png_rgb8(paths["book"], _book(contract, atlas))

    columns = int(contract["pattern"]["atlas_columns"])
    rows = int(contract["pattern"]["atlas_rows"])
    digest = str(atlas["shared_ring_digest"])
    manifest = {
        "schema": "iggy3d.material.structural_oak_joinery.atlas.v2",
        "profile_id": contract["profile"]["profile_id"],
        "pattern_id": contract["pattern"]["pattern_id"],
        "tile_resolution": tile_resolution,
        "atlas_resolution": [
            tile_resolution * columns,
            tile_resolution * rows,
        ],
        "continuity": {
            "ring_count": int(atlas["ring_count"]),
            "joinery_ring_table_digest": digest,
            "timber_ring_table_digest": digest,
            "rule": "All physical cut tiles evaluate the structural timber pith controls and ring table.",
        },
        "surface_response": {
            "uniform_roughness": contract["profile"]["representation"][
                "uniform_roughness"
            ],
            "roughness_map": False,
            "damage": False,
            "height_decode_m": [-maximum_height, maximum_height],
        },
        "tiles": [
            {
                "index": tile["index"],
                "id": tile["id"],
                "projection": tile["projection"],
                "surface_classes": tile["surface_classes"],
            }
            for tile in contract["pattern"]["tiles"]
        ],
        "source_sha256": {
            "profile": _sha256(contract["profile_path"]),
            "pattern": _sha256(contract["pattern_path"]),
            "timber_profile": _sha256(contract["timber_profile_path"]),
            "timber_pattern": _sha256(contract["timber_pattern_path"]),
        },
        "files": sorted(path.name for path in paths.values()),
    }
    manifest_path = output / f"{PREFIX}_atlas_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def main() -> None:
    args = parse_args()
    contract = load_contract(args.profile, args.pattern)
    resolution = (
        args.tile_resolution
        if args.tile_resolution is not None
        else int(contract["profile"]["representation"]["tile_resolution"])
    )
    manifest = build_outputs(
        contract,
        args.output_root,
        tile_resolution=resolution,
    )
    print(
        json.dumps(
            {
                "profile_id": manifest["profile_id"],
                "pattern_id": manifest["pattern_id"],
                "atlas_resolution": manifest["atlas_resolution"],
                "ring_count": manifest["continuity"]["ring_count"],
                "output_root": str(Path(args.output_root).resolve()),
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
