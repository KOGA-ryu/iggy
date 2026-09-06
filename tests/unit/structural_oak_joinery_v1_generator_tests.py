from __future__ import annotations

import importlib.util
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

import numpy as np


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "structural_oak_joinery_v1"
)
PROFILE_PATH = MATERIAL_ROOT / "profiles" / "structural_oak_joinery_v1.json"
PATTERN_PATH = (
    MATERIAL_ROOT / "patterns" / "structural_oak_endgrain_variants_v1.json"
)
GENERATOR_PATH = MATERIAL_ROOT / "generate_structural_oak_endgrain_v1.py"


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "structural_oak_joinery_v1_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load generator from {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class StructuralOakJoineryProfileTests(unittest.TestCase):
    def test_joint_dimensions_follow_written_rules_and_real_clearances(self):
        profile = json.loads(PROFILE_PATH.read_text())
        fixture = profile["joint_fixture_m"]
        timber = fixture["receiving_timber"]
        mortise = fixture["mortise"]
        tenon = fixture["tenon"]
        peg = fixture["peg"]

        self.assertAlmostEqual(
            mortise["width_y"],
            timber["width"] * 0.25,
        )
        self.assertAlmostEqual(
            peg["diameter"],
            mortise["width_y"] * 0.5,
            delta=0.001,
        )
        self.assertGreater(mortise["width_y"], tenon["width"])
        self.assertAlmostEqual(
            mortise["width_y"] - tenon["width"],
            tenon["side_clearance"] * 2.0,
        )
        self.assertAlmostEqual(fixture["housing"]["depth_z"], 0.0254)
        self.assertGreaterEqual(peg["drawbore_offset"], 0.0015875)
        self.assertLessEqual(peg["drawbore_offset"], 0.003175)
        self.assertEqual(peg["sides"], 8)

    def test_surface_classes_are_explicit_corner_domain_semantics(self):
        profile = json.loads(PROFILE_PATH.read_text())
        classes = profile["semantic_surface_classes"]
        self.assertEqual([entry["id"] for entry in classes], list(range(12)))
        self.assertEqual(len({entry["name"] for entry in classes}), 12)
        contract = profile["coordinate_contract"]
        self.assertEqual(contract["surface_class_domain"], "CORNER")
        self.assertEqual(contract["cut_coordinate_domain"], "CORNER")
        self.assertIn("same integer surface class", contract["interpolation_rule"])
        self.assertIn("material transfer", contract["material_boundary"])

    def test_profile_rejects_legacy_fantasy_anatomy_and_damage(self):
        profile = json.loads(PROFILE_PATH.read_text())
        excluded = set(profile["excluded_lanes"])
        self.assertTrue(
            {
                "random_noise",
                "roughness_map",
                "damage",
                "checks",
                "chips",
                "repairs",
                "triangle_interpolated_cut_masks",
                "decorative_6_to_16_mm_growth_rings",
                "stylized_1_to_4_mm_pores",
            }.issubset(excluded)
        )
        self.assertFalse(profile["representation"]["authored_roughness_map"])
        self.assertFalse(profile["representation"]["damage"])

    def test_cut_atlas_has_a_route_for_every_clean_joinery_class(self):
        pattern = json.loads(PATTERN_PATH.read_text())
        self.assertEqual(pattern["atlas_columns"], 4)
        self.assertEqual(pattern["atlas_rows"], 2)
        self.assertEqual(len(pattern["tiles"]), 8)
        self.assertEqual(
            [tile["index"] for tile in pattern["tiles"]],
            list(range(8)),
        )
        routed = {
            int(key.split("_")[0])
            for key in pattern["semantic_tile_routes"]
        }
        self.assertTrue(set(range(2, 12)).issubset(routed))


class StructuralOakJoineryGeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.generator = _load_generator()
        cls.contract = cls.generator.load_contract(PROFILE_PATH, PATTERN_PATH)

    def test_generator_reuses_the_timber_growth_table_exactly(self):
        first = self.generator.compile_shared_growth(self.contract)
        second = self.generator.compile_shared_growth(self.contract)
        self.assertEqual(first["digest"], second["digest"])
        self.assertEqual(first["ring_count"], 832)
        np.testing.assert_array_equal(
            first["boundaries_m"],
            second["boundaries_m"],
        )

    def test_tiles_are_physical_slices_with_separate_anatomical_lanes(self):
        atlas = self.generator.generate_atlas(
            self.contract,
            tile_resolution=128,
        )
        self.assertEqual(atlas["base_color_linear"].shape, (256, 512, 3))
        self.assertEqual(atlas["normal"].shape, (256, 512, 3))
        self.assertEqual(atlas["height_m"].shape, (256, 512))
        self.assertEqual(atlas["identity"].shape, (256, 512, 3))
        self.assertEqual(atlas["semantic"].shape, (256, 512, 3))
        self.assertGreater(float(atlas["identity"][..., 0].std()), 0.05)
        self.assertGreater(float(atlas["identity"][..., 1].max()), 0.02)
        self.assertGreater(float(atlas["identity"][..., 2].max()), 0.001)
        self.assertEqual(
            atlas["shared_ring_digest"],
            self.generator.compile_shared_growth(self.contract)["digest"],
        )

    def test_writer_emits_only_clean_cutface_lanes_and_manifest(self):
        with TemporaryDirectory() as temporary:
            output = Path(temporary)
            manifest = self.generator.build_outputs(
                self.contract,
                output,
                tile_resolution=128,
            )
            expected = {
                "structural_oak_joinery_v1_cut_basecolor.png",
                "structural_oak_joinery_v1_cut_normal.png",
                "structural_oak_joinery_v1_cut_height.png",
                "structural_oak_joinery_v1_cut_identity.png",
                "structural_oak_joinery_v1_cut_semantic.png",
                "structural_oak_joinery_v1_cut_atlas_book.png",
                "structural_oak_joinery_v1_atlas_manifest.json",
            }
            self.assertEqual({path.name for path in output.iterdir()}, expected)
            self.assertEqual(
                manifest["continuity"]["joinery_ring_table_digest"],
                manifest["continuity"]["timber_ring_table_digest"],
            )
            self.assertFalse(manifest["surface_response"]["roughness_map"])
            self.assertFalse(manifest["surface_response"]["damage"])


if __name__ == "__main__":
    unittest.main()
