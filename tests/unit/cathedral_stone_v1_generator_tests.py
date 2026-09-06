from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
import unittest

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "cathedral_stone_v1"
)
GENERATOR_PATH = MATERIAL_ROOT / "generate_cathedral_stone_v1.py"
PATTERN_PATH = (
    MATERIAL_ROOT / "patterns" / "cathedral_ashlar_courses_v1.json"
)
PROFILE_PATH = MATERIAL_ROOT / "profiles" / "cathedral_stone_v1.json"
CAPTURE_PATH = (
    MATERIAL_ROOT
    / "references"
    / "santa_marina_biocalcarenite_capture.json"
)
LEDGER_PATH = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "reference_measurements_v1.json"
)


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "cathedral_stone_v1_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load generator from {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class CathedralStoneV1GeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.generator = _load_generator()
        cls.recipe = cls.generator.load_ashlar_recipe(PATTERN_PATH)
        cls.capture = cls.generator.load_reference_capture(CAPTURE_PATH)
        cls.relief_proxy = cls.generator.load_relief_proxy(LEDGER_PATH)
        cls.material = cls.generator.generate_material(
            resolution=384,
            seed=823451,
            pattern_variation=0,
            recipe_path=PATTERN_PATH,
            capture_path=CAPTURE_PATH,
            ledger_path=LEDGER_PATH,
        )

    def test_measured_construction_recipe_uses_current_authorities(self):
        recipe = self.recipe
        self.assertEqual(
            recipe.schema,
            "iggy3d.pattern.measured_cathedral_ashlar.v2",
        )
        self.assertEqual(recipe.tile_size_m, 4.0)
        self.assertEqual(len(recipe.courses), 10)
        self.assertEqual(
            sum(len(course["blocks"]) for course in recipe.courses),
            40,
        )
        self.assertEqual(recipe.tooling, ("diagonal_drove",))
        self.assertEqual(recipe.edge_profiles, ("square_true",))
        self.assertEqual(
            recipe.measurement_authority["dimension_measurement_id"],
            "cordoba_early_gothic_ashlar",
        )
        self.assertEqual(float(recipe.mortar["recess_m"]), 0.0)

    def test_selected_color_capture_and_relief_proxy_are_distinct_evidence(self):
        self.assertEqual(
            self.capture["source"]["asset_id"],
            "Santa_Marina_Aguas_Santas_2024_facade",
        )
        self.assertEqual(self.capture["source"]["license"], "CC BY-SA 4.0")
        self.assertFalse(
            self.capture["production_translation"][
                "raw_pixels_used_as_runtime_texture"
            ]
        )
        self.assertEqual(
            self.relief_proxy["id"],
            "sabucina_calcarenite_surface_topography_proxy",
        )
        self.assertEqual(
            self.relief_proxy["scan_area_m"],
            [0.003, 0.0015],
        )
        self.assertEqual(
            self.relief_proxy["roughness_cutoff_m"],
            0.0008,
        )
        self.assertIn(
            "not the Santa Marina or Naranjo stone",
            self.relief_proxy["use_limit"],
        )

    def test_raster_retains_block_local_frames_and_measured_body_roles(self):
        material = self.material
        self.assertEqual(material["block_count"], 40)
        self.assertEqual(set(np.unique(material["block_id"])), set(range(41)))
        self.assertGreater(float(material["stone_mask"].mean()), 0.97)
        self.assertLess(float(material["mortar_mask"].mean()), 0.03)
        self.assertEqual(
            material["stone_body_masks"].shape,
            (384, 384, 3),
        )
        self.assertTrue(np.isfinite(material["local_u"]).all())
        self.assertTrue(np.isfinite(material["local_v"]).all())
        self.assertTrue(
            0.27
            <= float(material["fossil_fragment_mask"].mean())
            <= 0.43
        )
        self.assertLess(
            float(material["visible_pore_identity_mask"].mean()),
            0.06,
        )

    def test_pbr_construction_stays_neutral_while_body_relief_is_measured(self):
        material = self.material
        self.assertTrue(np.allclose(material["height_m"], 0.0))
        self.assertTrue(np.allclose(material["normal"][..., :2], 0.0))
        self.assertTrue(np.allclose(material["ao"], 1.0))
        self.assertTrue(np.allclose(material["metallic"], 0.0))
        height = material["stone_body_height_m"]
        normal = material["stone_body_normal"]
        metrics = material["stone_body_relief_metrics"]
        self.assertEqual(height.shape, (384, 384))
        self.assertEqual(normal.shape, (384, 384, 3))
        self.assertAlmostEqual(float(np.ptp(height)), 0.00113, delta=0.00003)
        self.assertAlmostEqual(metrics["sa_m"], 0.00010, delta=0.000035)
        self.assertAlmostEqual(metrics["sq_m"], 0.00013, delta=0.000035)
        self.assertAlmostEqual(metrics["ssk"], -0.39, delta=0.35)
        self.assertGreaterEqual(metrics["sku"], 2.5)
        self.assertLessEqual(metrics["sku"], 5.0)
        self.assertLessEqual(metrics["height_seam_m"], 0.00015)

    def test_measured_tooling_finish_is_finite_and_has_no_depth(self):
        material = self.material
        masks = material["tooling_detail_masks"]
        metrics = material["tooling_detail_metrics"]
        self.assertEqual(masks.shape, (384, 384, 3))
        self.assertEqual(material["tooling_detail_span_m"], 0.256)
        self.assertEqual(metrics["stroke_counts"], [47, 90, 159])
        self.assertEqual(
            metrics["target_density_per_m2"],
            [720.0, 1369.0, 2420.0],
        )
        self.assertGreater(float(masks.max()), 0.75)
        self.assertTrue(np.allclose(material["tooling_height_m"], 0.0))

    def test_graphic_value_hierarchy_uses_quantized_shades_and_selective_lines(
        self,
    ):
        material = self.material
        stone = material["stone_mask"] > 0.5
        shade = material["shade_coordinate"][stone]
        self.assertTrue(np.allclose(shade * 19.0, np.round(shade * 19.0)))
        self.assertGreaterEqual(len(np.unique(shade)), 7)
        self.assertEqual(material["shade_family_size"], 20)
        for block_id in range(1, material["block_count"] + 1):
            block_shades = np.unique(
                material["shade_coordinate"][
                    material["block_id"] == block_id
                ]
            )
            self.assertGreaterEqual(len(block_shades), 4)
            self.assertLessEqual(len(block_shades), 6)
        priorities = {
            round(float(block["line_priority"]), 2)
            for block in material["layout"]
        }
        self.assertEqual(priorities, {0.18, 0.30, 0.48, 0.62})
        ink_coverage = float((material["ink_mask"][stone] > 0.12).mean())
        highlight_coverage = float(
            (material["highlight_mask"][stone] > 0.10).mean()
        )
        self.assertGreater(ink_coverage, 0.002)
        self.assertLess(ink_coverage, 0.28)
        self.assertGreater(highlight_coverage, 0.002)
        self.assertLess(highlight_coverage, 0.40)

    def test_writer_emits_v6_maps_and_sixteen_bit_body_height(self):
        with TemporaryDirectory() as directory:
            manifest = self.generator.write_material_package(
                self.material,
                output_root=directory,
                pattern_path=PATTERN_PATH,
                profile_path=PROFILE_PATH,
            )
            root = Path(directory)
            self.assertEqual(
                manifest["schema"],
                "iggy3d.material.cathedral_stone_v1.v6",
            )
            self.assertEqual(manifest["block_count"], 40)
            self.assertEqual(
                manifest["stone_body"]["relief_proxy_measurement_id"],
                "sabucina_calcarenite_surface_topography_proxy",
            )
            self.assertFalse(
                manifest["source_policy"]["relief_proxy_is_exact_lithology"]
            )
            self.assertEqual(
                manifest["tooling_finish"]["finish_measurement_id"],
                "medieval_fine_oblique_layage",
            )
            self.assertTrue(
                manifest["constraints"][
                    "uses_authored_graphic_plane_values"
                ]
            )
            self.assertTrue(
                manifest["constraints"][
                    "stone_body_detail_requires_runtime_distance_fade"
                ]
            )
            self.assertFalse(
                manifest["constraints"]["unreal_runtime_parity_verified"]
            )
            self.assertTrue(
                (root / "cathedral_stone_v1_tooling_detail.png").is_file()
            )
            for relative in manifest["outputs"].values():
                self.assertTrue((root / relative).is_file(), relative)
            height = (
                root / "cathedral_stone_v1_stone_body_height.png"
            ).read_bytes()
            width, image_height, bit_depth, color_type = struct.unpack(
                ">IIBB",
                height[16:26],
            )
            self.assertEqual((width, image_height), (384, 384))
            self.assertEqual(bit_depth, 16)
            self.assertEqual(color_type, 0)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
