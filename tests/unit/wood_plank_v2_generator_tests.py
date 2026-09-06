from __future__ import annotations

import importlib.util
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
    / "wood_plank_v2"
    / "generate_wood_plank_v2.py"
)
PATTERN_RECIPE_PATH = (
    GENERATOR_PATH.parent / "patterns" / "aged_oak_champion_v1.json"
)


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "wood_plank_v2_generator", GENERATOR_PATH
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load generator from {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


generator = _load_generator()


class WoodPlankV2GeneratorTests(unittest.TestCase):
    def test_authored_plank_recipe_generates_stable_bounded_variations(self):
        recipe = generator.load_plank_recipe(PATTERN_RECIPE_PATH)
        baseline = generator.generate_plank_layout(recipe, variation=0)
        first = generator.generate_plank_layout(recipe, variation=4)
        second = generator.generate_plank_layout(recipe, variation=4)
        other = generator.generate_plank_layout(recipe, variation=5)

        self.assertEqual(recipe.schema, "iggy3d.pattern.aged_oak.v1")
        self.assertEqual(recipe.name, "aged_oak_champion_v1")
        self.assertEqual(len(recipe.boards), 9)
        self.assertEqual(recipe.tile_size_m, 1.6)
        self.assertEqual(
            sum(len(board["knots"]) for board in recipe.boards),
            4,
        )
        self.assertEqual(
            sum(
                knot["intent"] == "focal_intergrown"
                for board in recipe.boards
                for knot in board["knots"]
            ),
            1,
        )
        damage_records = [
            damage
            for board in recipe.boards
            for damage in board["damage"]
        ]
        self.assertEqual(len(damage_records), 7)
        self.assertEqual(
            {damage["kind"] for damage in damage_records},
            {"knot_check", "end_check", "edge_splinter", "scratch"},
        )
        self.assertGreaterEqual(
            min(board["width_m"] for board in recipe.boards),
            0.15,
        )
        self.assertLessEqual(
            max(board["width_m"] for board in recipe.boards),
            0.20,
        )
        np.testing.assert_array_equal(first["widths"], second["widths"])
        np.testing.assert_array_equal(first["joints"], second["joints"])
        self.assertFalse(np.array_equal(first["widths"], baseline["widths"]))
        self.assertFalse(np.array_equal(first["joints"], other["joints"]))
        self.assertAlmostEqual(float(first["widths"].sum()), 1.0, places=6)
        self.assertTrue((first["widths"] > 0.09).all())

        spans = first["joints"][:, 1] - first["joints"][:, 0]
        self.assertTrue((spans > 0.30).all())
        self.assertTrue(((1.0 - spans) > 0.30).all())

    def test_plank_raster_preserves_boards_segments_and_periodic_seams(self):
        recipe = generator.load_plank_recipe(PATTERN_RECIPE_PATH)
        layout = generator.generate_plank_layout(recipe, variation=4)
        raster = generator.rasterize_plank_pattern(
            layout,
            resolution=192,
            tile_size_m=1.6,
        )

        self.assertEqual(np.unique(raster["board_id"]).size, 9)
        self.assertEqual(np.unique(raster["segment_id"]).size, 18)
        self.assertTrue(np.isfinite(raster["edge_distance"]).all())
        self.assertGreaterEqual(float(raster["edge_distance"].min()), 0.0)
        self.assertTrue(
            ((0.0 <= raster["local_u"]) & (raster["local_u"] <= 1.0)).all()
        )
        self.assertTrue(
            ((0.0 <= raster["local_v"]) & (raster["local_v"] <= 1.0)).all()
        )

        seam = _wrapped_seam_delta(raster["edge_distance"])
        interior = _interior_neighbor_delta(raster["edge_distance"])
        self.assertLess(seam, interior * 4.0 + 1.0e-7)

    def test_virtual_log_anatomy_and_knot_deformation_are_causal(self):
        material = generator.generate_material(
            resolution=256,
            seed=73129,
            tile_size_m=1.6,
            pattern_variation=0,
        )
        board = material["board_mask"] >= 0.9
        growth_flow = material["growth_flow"]
        growth_norm = np.linalg.norm(growth_flow, axis=-1)
        influence = material["knot_influence"]
        knot = material["knot_mask"]
        phase_delta = np.abs(
            material["ring_phase"] - material["ring_phase_unwarped"]
        )

        expected_fields = {
            "ring_phase",
            "ring_phase_unwarped",
            "ring_fraction",
            "earlywood_mask",
            "latewood_mask",
            "growth_flow",
            "knot_influence",
            "knot_core_mask",
            "knot_ring_mask",
            "knot_boundary_mask",
        }
        self.assertTrue(expected_fields.issubset(material))
        self.assertTrue(np.isfinite(material["ring_phase"]).all())
        self.assertTrue(np.isfinite(growth_flow).all())
        self.assertLess(
            float(np.mean(np.abs(growth_norm[board] - 1.0))),
            0.025,
        )
        self.assertGreater(float(material["earlywood_mask"][board].mean()), 0.06)
        self.assertLess(float(material["earlywood_mask"][board].mean()), 0.42)
        self.assertGreater(float(knot.mean()), 0.001)
        self.assertGreater(
            float((influence > 0.05).mean()),
            float((knot > 0.05).mean()) * 1.8,
        )
        influenced = np.logical_and(influence > 0.15, board)
        quiet = np.logical_and(influence < 0.001, board)
        self.assertGreater(
            float(phase_delta[influenced].mean()),
            float(phase_delta[quiet].mean()) + 0.01,
        )
        self.assertGreater(float(material["knot_core_mask"].mean()), 0.00003)
        self.assertGreater(float(material["knot_ring_mask"].mean()), 0.0001)
        self.assertLessEqual(
            float(
                material["knot_ring_mask"][
                    material["knot_mask"] < 0.01
                ].max(initial=0.0)
            ),
            0.02,
        )

    def test_white_oak_grain_has_owned_rings_pores_rays_and_fibres(self):
        material = generator.generate_material(
            resolution=384,
            seed=73129,
            tile_size_m=1.6,
            pattern_variation=0,
        )
        expected_fields = {
            "ring_spacing_m",
            "ring_spacing_multiplier",
            "rest_mask",
            "ray_visibility",
            "earlywood_pore_mask",
            "latewood_pore_mask",
            "vessel_mask",
            "ray_mask",
            "ray_direction",
            "fibre_bundle_mask",
            "grain_activity",
            "warm_pigment_mask",
            "cool_pigment_mask",
            "pigment_temperature",
            "shade_family_size",
            "segment_shade_palettes_linear",
            "shade_coordinate",
            "shade_index_continuous",
            "shade_drivers",
            "pigment_layer_order",
            "pigment_layers",
        }
        self.assertTrue(expected_fields.issubset(material))
        self.assertEqual(material["shade_family_size"], 20)
        shade_palettes = material["segment_shade_palettes_linear"]
        self.assertEqual(
            shade_palettes.shape,
            (material["pattern"]["segment_count"], 20, 3),
        )
        palette_luminance = np.sum(
            shade_palettes
            * np.asarray([0.2126, 0.7152, 0.0722], dtype=np.float32),
            axis=-1,
        )
        self.assertTrue(
            np.all(
                palette_luminance.max(axis=1)
                - palette_luminance.min(axis=1)
                > 0.12
            )
        )
        self.assertTrue(
            {
                "slow_pigment",
                "growth_history",
                "tissue",
            }.issubset(material["shade_drivers"])
        )
        for driver in material["shade_drivers"].values():
            self.assertEqual(driver.shape, (384, 384))
            self.assertTrue(np.isfinite(driver).all())
        self.assertEqual(
            material["pigment_layer_order"],
            (
                "heartwood_base",
                "cool_olive_wash",
                "warm_amber_wash",
                "earlywood_tone",
                "latewood_tone",
                "vessel_tone",
                "ray_figure",
                "fibre_tone",
            ),
        )
        self.assertEqual(
            tuple(material["pigment_layers"]),
            material["pigment_layer_order"],
        )
        for name, layer in material["pigment_layers"].items():
            self.assertEqual(layer["coverage"].shape, (384, 384), name)
            self.assertEqual(
                layer["color_linear"].shape,
                (384, 384, 3),
                name,
            )
            self.assertTrue(np.isfinite(layer["coverage"]).all(), name)
            self.assertTrue(np.isfinite(layer["color_linear"]).all(), name)
        board = material["board_mask"] > 0.9
        earlywood = np.logical_and(material["earlywood_mask"] > 0.55, board)
        latewood = np.logical_and(material["latewood_mask"] > 0.75, board)
        rest = np.logical_and(material["rest_mask"] > 0.65, board)
        active = np.logical_and(material["rest_mask"] < 0.05, board)

        spacing = material["ring_spacing_multiplier"][board]
        self.assertGreater(float(np.std(spacing)), 0.045)
        self.assertGreater(float(np.percentile(spacing, 95)), 1.08)
        self.assertLess(float(np.percentile(spacing, 5)), 0.94)

        earlywood_pores = material["earlywood_pore_mask"]
        self.assertGreater(float(earlywood_pores.max()), 0.45)
        self.assertGreater(
            float(earlywood_pores[earlywood].mean()),
            float(earlywood_pores[latewood].mean()) * 5.0 + 0.005,
        )
        self.assertGreater(float(material["latewood_pore_mask"].max()), 0.18)
        self.assertGreater(float(material["ray_mask"].max()), 0.30)
        self.assertGreater(float(material["fibre_bundle_mask"].max()), 0.35)
        self.assertGreater(int(rest.sum()), 24)
        self.assertGreater(
            float(material["grain_activity"][active].mean()),
            float(material["grain_activity"][rest].mean()) * 1.20,
        )
        np.testing.assert_allclose(
            np.linalg.norm(material["ray_direction"], axis=-1)[board],
            1.0,
            atol=3.0e-3,
        )

        warm = np.logical_and(material["warm_pigment_mask"] > 0.10, board)
        cool = np.logical_and(material["cool_pigment_mask"] > 0.10, board)
        self.assertGreater(int(warm.sum()), 32)
        self.assertGreater(int(cool.sum()), 32)
        growth_color = material["growth_pigment_linear"]
        self.assertGreater(
            float(
                np.linalg.norm(
                    growth_color[warm].mean(axis=0)
                    - growth_color[cool].mean(axis=0)
                )
            ),
            0.020,
        )
        focal_board = np.logical_and(material["board_id"] == 3, board)
        luminance = np.sum(
            growth_color * np.asarray([0.2126, 0.7152, 0.0722]),
            axis=-1,
        )
        self.assertGreater(
            float(
                np.percentile(luminance[focal_board], 95)
                - np.percentile(luminance[focal_board], 5)
            ),
            0.085,
        )

        focal_segment_id = 6
        focal_segment = np.logical_and(
            material["segment_id"] == focal_segment_id,
            board,
        )
        shade_coordinate = material["shade_coordinate"]
        shade_index = material["shade_index_continuous"]
        self.assertGreater(
            float(
                np.percentile(shade_coordinate[focal_segment], 95)
                - np.percentile(shade_coordinate[focal_segment], 5)
            ),
            0.78,
        )
        occupied_shades = np.unique(
            np.clip(
                np.rint(shade_index[focal_segment]),
                0,
                material["shade_family_size"] - 1,
            ).astype(np.int32)
        )
        self.assertGreaterEqual(occupied_shades.size, 16)

        horizontal_neighbors = np.logical_and(
            focal_segment[:, 1:],
            focal_segment[:, :-1],
        )
        vertical_neighbors = np.logical_and(
            focal_segment[1:, :],
            focal_segment[:-1, :],
        )
        neighbor_deltas = np.concatenate(
            [
                np.abs(
                    shade_coordinate[:, 1:]
                    - shade_coordinate[:, :-1]
                )[horizontal_neighbors],
                np.abs(
                    shade_coordinate[1:, :]
                    - shade_coordinate[:-1, :]
                )[vertical_neighbors],
            ]
        )
        self.assertLess(float(np.percentile(neighbor_deltas, 95)), 0.12)

    def test_material_layers_follow_the_wood_structure(self):
        material = generator.generate_material(
            resolution=192,
            seed=73129,
            tile_size_m=1.6,
            pattern_variation=4,
        )
        board = material["board_mask"] >= 0.9
        gap = material["board_mask"] <= 0.1

        self.assertEqual(material["base_color_linear"].shape, (192, 192, 3))
        self.assertEqual(material["normal"].shape, (192, 192, 3))
        self.assertEqual(material["orm"].shape, (192, 192, 3))
        self.assertGreater(
            float(material["height_m"][board].mean()),
            float(material["height_m"][gap].mean()) + 0.001,
        )
        self.assertGreater(
            float(material["roughness"][gap].mean()),
            float(material["roughness"][board].mean()),
        )
        self.assertGreater(float(material["knot_mask"].mean()), 0.0008)
        self.assertLess(float(material["knot_mask"].mean()), 0.08)

        growth_flow = material["growth_flow"][board]
        along_flow = float(
            np.mean(np.abs(growth_flow[:, 1]))
        )
        across_flow = float(
            np.mean(np.abs(growth_flow[:, 0]))
        )
        self.assertGreater(along_flow, across_flow * 1.15)
        np.testing.assert_allclose(
            np.linalg.norm(material["normal"], axis=-1),
            1.0,
            atol=2.0e-5,
        )

    def test_authored_damage_obeys_its_material_habitats(self):
        material = generator.generate_material(
            resolution=384,
            seed=73129,
            tile_size_m=1.6,
            pattern_variation=0,
        )
        expected_fields = {
            "knot_check_mask",
            "end_check_mask",
            "splinter_cut_mask",
            "splinter_lip_mask",
            "scratch_mask",
            "finish_loss_mask",
            "damage_height_delta_m",
        }
        self.assertTrue(expected_fields.issubset(material))
        for field in (
            "knot_check_mask",
            "end_check_mask",
            "splinter_cut_mask",
            "splinter_lip_mask",
            "scratch_mask",
        ):
            self.assertGreater(float(material[field].max()), 0.45, field)

        knot_check = material["knot_check_mask"] > 0.08
        end_check = material["end_check_mask"] > 0.08
        splinter = material["splinter_cut_mask"] > 0.08
        self.assertGreater(
            float(material["knot_influence"][knot_check].mean()),
            0.08,
        )
        self.assertLess(
            float(material["joint_edge_distance"][end_check].mean()),
            0.08,
        )
        self.assertLess(
            float(material["board_edge_distance"][splinter].mean()),
            0.03,
        )
        self.assertLess(float(material["damage_height_delta_m"].min()), -0.0002)
        self.assertGreater(float(material["damage_height_delta_m"].max()), 0.00005)
        self.assertGreater(
            float(material["finish_loss_mask"].mean()),
            float(material["scratch_mask"].mean()) * 0.35,
        )

    def test_grooves_are_layered_material_states_under_neutral_light(self):
        material = generator.generate_material(
            resolution=384,
            seed=73129,
            tile_size_m=1.6,
            pattern_variation=0,
        )
        expected_fields = {
            "heartwood_pigment_linear",
            "growth_pigment_linear",
            "finished_wood_linear",
            "finish_thickness",
            "compressed_wood_mask",
            "exposed_wood_mask",
            "dirt_deposit_mask",
            "groove_state_color_linear",
            "white_light_frontal_srgb",
            "white_light_side_srgb",
            "white_light_grazing_srgb",
        }
        self.assertTrue(expected_fields.issubset(material))
        compressed = material["compressed_wood_mask"] > 0.12
        exposed = material["exposed_wood_mask"] > 0.12
        dirt = material["dirt_deposit_mask"] > 0.12
        intact = np.logical_and(
            material["board_mask"] > 0.98,
            material["finish_loss_mask"] < 0.01,
        )
        for name, mask in (
            ("compressed wood", compressed),
            ("exposed wood", exposed),
            ("retained dirt", dirt),
            ("intact finish", intact),
        ):
            self.assertGreater(int(mask.sum()), 8, name)

        self.assertGreater(
            float(material["check_shoulder_mask"][compressed].mean()),
            0.02,
        )
        self.assertGreater(
            float(
                np.maximum(
                    material["check_fibre_lip_mask"],
                    material["splinter_lip_mask"],
                )[exposed].mean()
            ),
            0.02,
        )
        cavity_habitat = np.maximum(
            material["check_mask"],
            material["splinter_cut_mask"],
        )
        self.assertGreater(float(cavity_habitat[dirt].mean()), 0.08)
        self.assertLess(
            float(material["finish_thickness"][
                material["finish_loss_mask"] > 0.15
            ].mean()),
            float(material["finish_thickness"][intact].mean()) - 0.20,
        )

        state_color = material["groove_state_color_linear"]
        compressed_mean = state_color[compressed].mean(axis=0)
        exposed_mean = state_color[exposed].mean(axis=0)
        dirt_mean = state_color[dirt].mean(axis=0)
        self.assertGreater(
            float(np.linalg.norm(compressed_mean - exposed_mean)),
            0.035,
        )
        self.assertGreater(
            float(np.linalg.norm(exposed_mean - dirt_mean)),
            0.050,
        )

        groove = np.logical_or.reduce((compressed, exposed, dirt))
        frontal = material["white_light_frontal_srgb"]
        side = material["white_light_side_srgb"]
        grazing = material["white_light_grazing_srgb"]
        self.assertGreater(
            float(np.mean(np.abs(frontal[groove] - side[groove]))),
            0.015,
        )
        self.assertGreater(
            float(np.mean(np.abs(side[groove] - grazing[groove]))),
            0.015,
        )

    def test_generation_and_package_are_deterministic_and_complete(self):
        first = generator.generate_material(
            resolution=128,
            seed=73129,
            tile_size_m=1.6,
            pattern_variation=4,
        )
        second = generator.generate_material(
            resolution=128,
            seed=73129,
            tile_size_m=1.6,
            pattern_variation=4,
        )
        np.testing.assert_array_equal(first["height_m"], second["height_m"])
        np.testing.assert_array_equal(
            first["base_color_linear"], second["base_color_linear"]
        )

        with TemporaryDirectory() as temporary:
            output = Path(temporary)
            manifest = generator.write_material_package(first, output)
            expected = {
                "base_color",
                "normal",
                "orm",
                "height",
                "breakdown",
                "anatomy_causality",
                "grain_anatomy",
                "pigment_layers",
                "intra_board_shades",
                "damage_causality",
                "groove_specimen",
                "tiling",
                "pattern_variations",
                "book_text",
                "manifest",
            }
            self.assertEqual(set(manifest["outputs"]), expected)
            for path in manifest["outputs"].values():
                self.assertTrue((output / path).is_file(), path)
            self.assertEqual(manifest["pattern"]["variation"], 4)
            self.assertEqual(
                _png_ihdr(output / manifest["outputs"]["height"])["bit_depth"],
                16,
            )
            height_pixels = _read_gray16_png(
                output / manifest["outputs"]["height"]
            )
            self.assertEqual(int(height_pixels.min()), 0)
            self.assertEqual(int(height_pixels.max()), 65535)
            self.assertGreater(np.unique(height_pixels).size, 256)
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
            self.assertIn("# Wood Plank V2 Material Book", book_text)
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
