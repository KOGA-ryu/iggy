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
    / "stone_rough_v2"
    / "generate_stone_rough_v2.py"
)
PATTERN_RECIPE_PATH = (
    GENERATOR_PATH.parent
    / "patterns"
    / "fieldstone_hand_authored_v1.json"
)


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "stone_rough_v2_generator", GENERATOR_PATH
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load generator from {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


generator = _load_generator()


class StoneRoughV2GeneratorTests(unittest.TestCase):
    def test_hand_authored_pattern_generates_diverse_deterministic_variants(self):
        recipe = generator.load_pattern_recipe(PATTERN_RECIPE_PATH)

        self.assertEqual(recipe.schema, "iggy3d.pattern.site_layout.v1")
        self.assertEqual(recipe.name, "fieldstone_hand_authored_v1")
        self.assertEqual(
            {site["role"] for site in recipe.sites},
            {"anchor", "medium", "infill"},
        )
        self.assertGreaterEqual(len(recipe.sites), 24)

        baseline = generator.generate_pattern_layout(recipe, variation=0)
        first = generator.generate_pattern_layout(recipe, variation=5)
        second = generator.generate_pattern_layout(recipe, variation=5)
        other = generator.generate_pattern_layout(recipe, variation=6)

        np.testing.assert_array_equal(first["positions"], second["positions"])
        np.testing.assert_array_equal(first["influences"], second["influences"])
        self.assertEqual(first["site_ids"], baseline["site_ids"])
        self.assertFalse(np.array_equal(first["positions"], baseline["positions"]))
        self.assertFalse(np.array_equal(first["positions"], other["positions"]))
        self.assertTrue(((0.0 <= first["positions"]) & (first["positions"] < 1.0)).all())
        self.assertTrue((first["influences"] > 0.0).all())

    def test_authored_pattern_raster_has_varied_areas_and_wraps(self):
        recipe = generator.load_pattern_recipe(PATTERN_RECIPE_PATH)
        layout = generator.generate_pattern_layout(recipe, variation=5)
        raster = generator.rasterize_pattern(
            layout,
            resolution=192,
            tile_size_m=1.6,
        )

        counts = np.bincount(
            raster["cell_id"].ravel(), minlength=len(recipe.sites)
        )
        occupied = counts[counts > 0]
        self.assertEqual(occupied.size, len(recipe.sites))
        self.assertGreater(float(np.std(occupied) / np.mean(occupied)), 0.28)
        self.assertGreater(
            float(np.percentile(occupied, 90) / np.percentile(occupied, 10)),
            1.75,
        )
        self.assertTrue(np.isfinite(raster["edge_distance"]).all())
        self.assertGreaterEqual(float(raster["edge_distance"].min()), 0.0)

        seam = _wrapped_seam_delta(raster["edge_distance"])
        interior = _interior_neighbor_delta(raster["edge_distance"])
        self.assertLess(seam, interior * 4.0 + 1.0e-7)

    def test_generation_is_deterministic_and_channel_complete(self):
        first = generator.generate_material(
            resolution=128, seed=240619, tile_size_m=1.6
        )
        second = generator.generate_material(
            resolution=128, seed=240619, tile_size_m=1.6
        )

        expected_scalar = {
            "edge_distance",
            "stone_mask",
            "chip_mask",
            "height_m",
            "roughness",
            "ao",
        }
        for name in expected_scalar:
            self.assertEqual(first[name].shape, (128, 128), name)
            self.assertTrue(np.isfinite(first[name]).all(), name)
        self.assertEqual(first["base_color_linear"].shape, (128, 128, 3))
        self.assertEqual(first["normal"].shape, (128, 128, 3))
        self.assertEqual(first["orm"].shape, (128, 128, 3))

        np.testing.assert_array_equal(first["height_m"], second["height_m"])
        np.testing.assert_array_equal(
            first["base_color_linear"], second["base_color_linear"]
        )
        np.testing.assert_array_equal(first["roughness"], second["roughness"])

    def test_height_normal_and_layer_relationships_are_coherent(self):
        material = generator.generate_material(
            resolution=160, seed=240619, tile_size_m=1.6
        )
        stone = material["stone_mask"] >= 0.9
        mortar = material["stone_mask"] <= 0.1

        self.assertGreater(
            float(material["height_m"][stone].mean()),
            float(material["height_m"][mortar].mean()) + 0.004,
        )
        self.assertGreater(
            float(material["roughness"][mortar].mean()),
            float(material["roughness"][stone].mean()),
        )
        normal_lengths = np.linalg.norm(material["normal"], axis=-1)
        np.testing.assert_allclose(normal_lengths, 1.0, atol=2.0e-5)
        self.assertTrue((material["normal"][..., 2] > 0.0).all())
        self.assertTrue(((0.0 <= material["orm"]) & (material["orm"] <= 1.0)).all())
        self.assertTrue(np.allclose(material["orm"][..., 2], 0.0))

    def test_periodic_seams_are_no_harsher_than_interior_neighbors(self):
        material = generator.generate_material(
            resolution=192, seed=240619, tile_size_m=1.6
        )
        for name in ("height_m", "roughness", "base_color_linear"):
            image = material[name]
            seam = _wrapped_seam_delta(image)
            interior = _interior_neighbor_delta(image)
            self.assertLess(
                seam,
                interior * 4.0 + 1.0e-7,
                f"{name}: seam {seam} interior {interior}",
            )

    def test_inkblotter_palette_rejects_lighting_extremes(self):
        with TemporaryDirectory() as temporary:
            palette_path = Path(temporary) / "reference_style_recipe.json"
            palette_path.write_text(
                json.dumps(
                    {
                        "schema": "glyph_lab.reference_style_recipe.v0",
                        "layers": [
                            {
                                "name": "outline",
                                "role": "linework",
                                "palette": ["#000000"],
                                "pixel_count": 80,
                            },
                            {
                                "name": "gray",
                                "role": "color_fill",
                                "palette": [
                                    "#4b4942",
                                    "#716d61",
                                    "#9a9483",
                                ],
                                "pixel_count": 420,
                            },
                            {
                                "name": "highlight",
                                "role": "highlight",
                                "palette": ["#ffffff"],
                                "pixel_count": 30,
                            },
                        ],
                    }
                ),
                encoding="utf-8",
            )

            palette = generator.load_reference_palette(palette_path)

        self.assertEqual(palette.source_schema, "glyph_lab.reference_style_recipe.v0")
        self.assertEqual(len(palette.colors_srgb), 3)
        self.assertNotIn((0, 0, 0), palette.colors_srgb)
        self.assertNotIn((255, 255, 255), palette.colors_srgb)
        self.assertGreater(palette.weights[0], 0.0)

    def test_reference_palette_luminance_is_delit_before_material_use(self):
        palette = generator.srgb_to_linear(
            np.asarray(
                [
                    [55, 48, 36],
                    [112, 101, 84],
                    [165, 155, 137],
                ],
                dtype=np.float32,
            )
            / 255.0
        )
        luminance = np.sum(
            palette * np.array([0.2126, 0.7152, 0.0722]), axis=1
        )

        compressed = generator._compress_palette_luminance(
            palette, [0.2, 0.6, 0.2], contrast=0.45
        )
        compressed_luminance = np.sum(
            compressed * np.array([0.2126, 0.7152, 0.0722]), axis=1
        )

        self.assertLess(
            float(np.ptp(compressed_luminance)),
            float(np.ptp(luminance)) * 0.5,
        )
        self.assertAlmostEqual(
            float(compressed_luminance[1]), float(luminance[1]), places=5
        )

    def test_package_writes_maps_manifest_and_visual_proofs(self):
        with TemporaryDirectory() as temporary:
            output = Path(temporary)
            material = generator.generate_material(
                resolution=128, seed=240619, tile_size_m=1.6
            )
            manifest = generator.write_material_package(material, output)

            expected = {
                "base_color",
                "normal",
                "orm",
                "height",
                "breakdown",
                "tiling",
                "pattern_variations",
                "book_text",
                "manifest",
            }
            self.assertEqual(set(manifest["outputs"]), expected)
            for path in manifest["outputs"].values():
                self.assertTrue((output / path).is_file(), path)
            self.assertEqual(
                _png_ihdr(output / manifest["outputs"]["height"])["bit_depth"],
                16,
            )
            height_pixels = _read_gray16_png(
                output / manifest["outputs"]["height"]
            )
            self.assertEqual(height_pixels.shape, (128, 128))
            self.assertEqual(int(height_pixels.min()), 0)
            self.assertEqual(int(height_pixels.max()), 65535)
            self.assertGreater(np.unique(height_pixels).size, 256)
            self.assertEqual(
                _png_ihdr(output / manifest["outputs"]["base_color"])[
                    "color_type"
                ],
                2,
            )
            self.assertEqual(
                manifest["pattern"]["schema"],
                "iggy3d.pattern.site_layout.v1",
            )
            self.assertEqual(manifest["pattern"]["variation"], 0)
            self.assertEqual(len(manifest["material_book"]["chapters"]), 8)
            for chapter in manifest["material_book"]["chapters"]:
                chapter_path = output / chapter["path"]
                self.assertTrue(chapter_path.is_file(), chapter["path"])
                chapter_header = _png_ihdr(chapter_path)
                self.assertGreater(chapter_header["width"], 1200)
                self.assertGreater(chapter_header["height"], 850)
            book_text = (
                output / manifest["outputs"]["book_text"]
            ).read_text(encoding="utf-8")
            self.assertIn("# Stone Rough V2 Material Book", book_text)
            self.assertIn("## Chapter 8", book_text)


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
    ihdr = _png_ihdr(path)
    if ihdr["bit_depth"] != 16 or ihdr["color_type"] != 0:
        raise AssertionError(f"{path} is not a 16-bit grayscale PNG")

    width, height = struct.unpack(">II", data[16:24])
    chunks: list[bytes] = []
    offset = 8
    while offset < len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        chunk_type = data[offset + 4 : offset + 8]
        chunk_data = data[offset + 8 : offset + 8 + length]
        if chunk_type == b"IDAT":
            chunks.append(chunk_data)
        offset += length + 12
        if chunk_type == b"IEND":
            break

    raw = zlib.decompress(b"".join(chunks))
    row_stride = width * 2
    expected_size = height * (row_stride + 1)
    if len(raw) != expected_size:
        raise AssertionError(
            f"{path} decoded to {len(raw)} bytes, expected {expected_size}"
        )

    pixels = np.empty((height, width), dtype=np.uint16)
    for row_index in range(height):
        row_start = row_index * (row_stride + 1)
        if raw[row_start] != 0:
            raise AssertionError(
                f"{path} row {row_index} uses unsupported PNG filter"
            )
        row = raw[row_start + 1 : row_start + 1 + row_stride]
        pixels[row_index] = np.frombuffer(row, dtype=">u2")
    return pixels


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
