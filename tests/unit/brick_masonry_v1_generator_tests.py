from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
import unittest
import zlib

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATOR_PATH = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "brick_masonry_v1"
    / "generate_brick_masonry_v1.py"
)
RECIPE_PATH = (
    GENERATOR_PATH.parent
    / "patterns"
    / "kiln_fired_running_bond_v1.json"
)


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "brick_masonry_v1_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load generator from {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


generator = _load_generator()


class BrickMasonryV1GeneratorTests(unittest.TestCase):
    def test_recipe_owns_bond_courses_champions_and_placement(self):
        recipe = generator.load_brick_recipe(RECIPE_PATH)

        self.assertEqual(
            recipe.schema,
            "iggy3d.pattern.brick_masonry.v1",
        )
        self.assertEqual(recipe.name, "kiln_fired_running_bond_v1")
        self.assertEqual(recipe.columns, 7)
        self.assertEqual(len(recipe.courses), 20)
        self.assertEqual(len(recipe.champions), 20)
        self.assertEqual(len(recipe.placement), 20)
        self.assertTrue(
            all(len(course) == recipe.columns for course in recipe.placement)
        )
        self.assertEqual(
            {champion["face_role"] for champion in recipe.champions},
            {
                "calm",
                "compressed",
                "laminated",
                "pitted",
                "overfired",
            },
        )
        placement = np.asarray(recipe.placement, dtype=np.int32)
        self.assertEqual(np.unique(placement).size, 20)
        self.assertLessEqual(int(np.bincount(placement.ravel()).max()), 9)

        baseline = generator.generate_brick_layout(recipe, variation=0)
        first = generator.generate_brick_layout(recipe, variation=4)
        second = generator.generate_brick_layout(recipe, variation=4)
        other = generator.generate_brick_layout(recipe, variation=5)
        np.testing.assert_array_equal(
            first["champion_id"],
            second["champion_id"],
        )
        np.testing.assert_array_equal(
            first["course_offsets"],
            second["course_offsets"],
        )
        self.assertFalse(
            np.array_equal(
                baseline["course_offsets"],
                first["course_offsets"],
            )
        )
        self.assertFalse(
            np.array_equal(
                first["champion_id"],
                other["champion_id"],
            )
        )

    def test_raster_preserves_every_brick_and_wraps(self):
        recipe = generator.load_brick_recipe(RECIPE_PATH)
        layout = generator.generate_brick_layout(recipe, variation=0)
        raster = generator.rasterize_brick_pattern(
            layout,
            recipe,
            resolution=320,
        )
        brick_count = len(recipe.courses) * recipe.columns

        self.assertEqual(raster["brick_count"], brick_count)
        self.assertEqual(np.unique(raster["brick_id"]).size, brick_count)
        self.assertEqual(raster["brick_mask"].shape, (320, 320))
        self.assertTrue(np.isfinite(raster["edge_distance_m"]).all())
        self.assertGreater(float(raster["brick_mask"].mean()), 0.70)
        self.assertLess(float(raster["brick_mask"].mean()), 0.91)
        self.assertGreater(float(raster["mortar_mask"].mean()), 0.09)

        for name in ("brick_mask", "edge_distance_m", "mortar_mask"):
            seam = _wrapped_seam_delta(raster[name])
            interior = _interior_neighbor_delta(raster[name])
            self.assertLess(seam, interior * 4.5 + 1.0e-6, name)

    def test_each_brick_owns_and_uses_a_twenty_shade_family(self):
        material = generator.generate_material(
            resolution=384,
            seed=88421,
            pattern_variation=0,
        )
        self.assertEqual(material["shade_family_size"], 20)
        self.assertEqual(
            material["brick_shade_palettes_linear"].shape,
            (material["brick_count"], 20, 3),
        )
        palette_luminance = np.sum(
            material["brick_shade_palettes_linear"]
            * np.asarray([0.2126, 0.7152, 0.0722], dtype=np.float32),
            axis=-1,
        )
        self.assertTrue(
            np.all(
                palette_luminance.max(axis=1)
                - palette_luminance.min(axis=1)
                > 0.11
            )
        )
        self.assertEqual(
            set(material["shade_drivers"]),
            {
                "kiln_gradient",
                "heat_cloud",
                "clay_body",
                "fired_skin",
            },
        )

        focal_id = material["proof_brick_id"]
        focal = np.logical_and(
            material["brick_id"] == focal_id,
            material["brick_mask"] > 0.90,
        )
        coordinate = material["shade_coordinate"]
        self.assertGreater(
            float(
                np.percentile(coordinate[focal], 95)
                - np.percentile(coordinate[focal], 5)
            ),
            0.76,
        )
        occupied = np.unique(
            np.rint(material["shade_index_continuous"][focal]).astype(
                np.int32
            )
        )
        self.assertGreaterEqual(occupied.size, 16)

        neighbor_deltas = _masked_neighbor_deltas(coordinate, focal)
        self.assertLess(float(np.percentile(neighbor_deltas, 95)), 0.14)
        brick_luminance = np.sum(
            material["brick_pigment_linear"]
            * np.asarray([0.2126, 0.7152, 0.0722], dtype=np.float32),
            axis=-1,
        )
        self.assertGreater(
            float(
                np.percentile(brick_luminance[focal], 95)
                - np.percentile(brick_luminance[focal], 5)
            ),
            0.075,
        )

    def test_clay_mortar_and_linework_have_separate_causal_fields(self):
        material = generator.generate_material(
            resolution=384,
            seed=88421,
            pattern_variation=0,
        )
        expected = {
            "clay_lamination_mask",
            "compression_drag_mask",
            "mineral_inclusion_mask",
            "pit_mask",
            "fired_skin_mask",
            "mortar_body",
            "mortar_aggregate_mask",
            "mortar_trowel_mask",
            "primary_line_mask",
            "secondary_line_mask",
            "tertiary_line_mask",
            "ink_mask",
            "highlight_stroke_mask",
            "detail_priority",
            "brush_direction",
        }
        self.assertTrue(expected.issubset(material))
        for name in expected - {"brush_direction"}:
            self.assertEqual(material[name].shape, (384, 384), name)
            self.assertTrue(np.isfinite(material[name]).all(), name)
        self.assertEqual(material["brush_direction"].shape, (384, 384, 2))
        np.testing.assert_allclose(
            np.linalg.norm(material["brush_direction"], axis=-1),
            1.0,
            atol=2.0e-4,
        )

        brick = material["brick_mask"] > 0.90
        mortar = material["brick_mask"] < 0.10
        near_edge = np.logical_and(
            brick,
            material["edge_distance_m"] < 0.007,
        )
        interior = np.logical_and(
            brick,
            material["edge_distance_m"] > 0.018,
        )
        self.assertGreater(
            float(material["ink_mask"][near_edge].mean()),
            float(material["ink_mask"][interior].mean()) * 1.35,
        )
        ink_coverage = float((material["ink_mask"][brick] > 0.12).mean())
        self.assertGreater(ink_coverage, 0.025)
        self.assertLess(ink_coverage, 0.24)
        strong_edge_fraction = float(
            (material["primary_line_mask"][near_edge] > 0.35).mean()
        )
        self.assertGreater(strong_edge_fraction, 0.05)
        self.assertLess(strong_edge_fraction, 0.72)
        self.assertGreater(
            float(material["mortar_aggregate_mask"][mortar].mean()),
            float(material["mortar_aggregate_mask"][brick].mean()) * 3.0
            + 0.005,
        )

    def test_material_maps_are_coherent_and_deterministic(self):
        first = generator.generate_material(
            resolution=192,
            seed=88421,
            pattern_variation=3,
        )
        second = generator.generate_material(
            resolution=192,
            seed=88421,
            pattern_variation=3,
        )
        np.testing.assert_array_equal(first["height_m"], second["height_m"])
        np.testing.assert_array_equal(
            first["base_color_linear"],
            second["base_color_linear"],
        )
        np.testing.assert_array_equal(first["ink_mask"], second["ink_mask"])

        brick = first["brick_mask"] > 0.90
        mortar = first["brick_mask"] < 0.10
        self.assertGreater(
            float(first["height_m"][brick].mean()),
            float(first["height_m"][mortar].mean()) + 0.005,
        )
        self.assertGreater(
            float(first["roughness"][mortar].mean()),
            float(first["roughness"][brick].mean()),
        )
        np.testing.assert_allclose(
            np.linalg.norm(first["normal"], axis=-1),
            1.0,
            atol=2.0e-5,
        )
        self.assertTrue(((0.0 <= first["orm"]) & (first["orm"] <= 1.0)).all())
        self.assertTrue(np.allclose(first["orm"][..., 2], 0.0))

        for name in ("height_m", "roughness", "base_color_linear"):
            seam = _wrapped_seam_delta(first[name])
            interior_delta = _interior_neighbor_delta(first[name])
            self.assertLess(
                seam,
                interior_delta * 4.5 + 1.0e-6,
                name,
            )

    def test_package_writes_maps_and_adversarial_proofs(self):
        with TemporaryDirectory() as temporary:
            output = Path(temporary)
            material = generator.generate_material(
                resolution=128,
                seed=88421,
                pattern_variation=0,
            )
            manifest = generator.write_material_package(material, output)
            expected = {
                "base_color",
                "normal",
                "orm",
                "height",
                "stylization",
                "brush_direction",
                "construction_proof",
                "color_proof",
                "linework_proof",
                "response_proof",
                "tiling_proof",
                "variation_proof",
                "manifest",
            }
            self.assertEqual(set(manifest["outputs"]), expected)
            for path in manifest["outputs"].values():
                self.assertTrue((output / path).is_file(), path)
            self.assertEqual(
                _png_ihdr(output / manifest["outputs"]["height"])[
                    "bit_depth"
                ],
                16,
            )
            height_pixels = _read_gray16_png(
                output / manifest["outputs"]["height"]
            )
            self.assertEqual(int(height_pixels.min()), 0)
            self.assertEqual(int(height_pixels.max()), 65535)
            self.assertGreater(np.unique(height_pixels).size, 256)
            self.assertEqual(
                manifest["pattern"]["brick_count"],
                140,
            )
            self.assertEqual(
                manifest["stylization"]["shade_family_size_per_brick"],
                20,
            )
            self.assertEqual(
                manifest["stylization"]["line_families"],
                ["primary", "secondary", "tertiary"],
            )


def _masked_neighbor_deltas(
    image: np.ndarray,
    mask: np.ndarray,
) -> np.ndarray:
    horizontal = np.logical_and(mask[:, 1:], mask[:, :-1])
    vertical = np.logical_and(mask[1:, :], mask[:-1, :])
    return np.concatenate(
        [
            np.abs(image[:, 1:] - image[:, :-1])[horizontal],
            np.abs(image[1:, :] - image[:-1, :])[vertical],
        ]
    )


def _wrapped_seam_delta(image: np.ndarray) -> float:
    horizontal = np.mean(np.abs(image[:, 0] - image[:, -1]))
    vertical = np.mean(np.abs(image[0] - image[-1]))
    return float(max(horizontal, vertical))


def _interior_neighbor_delta(image: np.ndarray) -> float:
    horizontal = np.mean(np.abs(image[:, 1:] - image[:, :-1]))
    vertical = np.mean(np.abs(image[1:] - image[:-1]))
    return float(max(horizontal, vertical))


def _png_ihdr(path: Path) -> dict[str, int]:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise AssertionError(f"{path} is not a PNG with an IHDR first chunk")
    return {
        "width": struct.unpack(">I", data[16:20])[0],
        "height": struct.unpack(">I", data[20:24])[0],
        "bit_depth": data[24],
        "color_type": data[25],
    }


def _read_gray16_png(path: Path) -> np.ndarray:
    data = path.read_bytes()
    width, height = struct.unpack(">II", data[16:24])
    chunks: list[bytes] = []
    offset = 8
    while offset < len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        chunk_type = data[offset + 4 : offset + 8]
        chunks.append(
            data[offset + 8 : offset + 8 + length]
            if chunk_type == b"IDAT"
            else b""
        )
        offset += length + 12
        if chunk_type == b"IEND":
            break
    raw = zlib.decompress(b"".join(chunks))
    row_stride = width * 2
    pixels = np.empty((height, width), dtype=np.uint16)
    for row_index in range(height):
        row_start = row_index * (row_stride + 1)
        if raw[row_start] != 0:
            raise AssertionError("unsupported PNG row filter")
        pixels[row_index] = np.frombuffer(
            raw[row_start + 1 : row_start + 1 + row_stride],
            dtype=">u2",
        )
    return pixels


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
