from __future__ import annotations

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
)
SWEEP_SCRIPT = MATERIAL_ROOT / "compare_intact_oxide_layers_v1.py"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "intact_oxide_layer_sweep_v1"
MANIFEST_PATH = OUTPUT_ROOT / "manifest.json"


class IntactOxideSweepBlueprintTests(unittest.TestCase):
    def test_sweep_has_four_deliberate_layer_strategies(self):
        self.assertTrue(
            SWEEP_SCRIPT.is_file(),
            "the reduced-resolution intact-oxide sweep is not implemented",
        )
        source = SWEEP_SCRIPT.read_text()
        for candidate_id in (
            "A_smooth_control",
            "B_quiet_compact_scale",
            "C_heavy_forged_scale",
            "D_compressed_directional_scale",
        ):
            self.assertIn(candidate_id, source)
        for causal_owner in (
            "thermal_macro_response",
            "compression_flow_response",
            "medium_morphology_response",
            "micro_grain_response",
            "detail_priority",
            "combined_surface_height_m",
        ):
            self.assertIn(causal_owner, source)

    def test_sweep_preserves_intact_physical_layer_contract(self):
        self.assertTrue(SWEEP_SCRIPT.is_file())
        source = SWEEP_SCRIPT.read_text()
        self.assertIn("EXPOSED_CONDUCTOR = 0.0", source)
        self.assertIn('"Metallic",), 0.0', source)
        self.assertIn("ShaderNodeMixShader", source)
        self.assertIn("ShaderNodeTangent", source)
        self.assertNotIn("save_as_mainfile", source)
        for prohibited in (
            "scale_plates",
            "contact_polish",
            "edge_wear",
            "rust_mask",
            "damage_mask",
            "scratch_mask",
            "pitting",
        ):
            self.assertNotIn(prohibited, source)

    def test_sweep_is_reduced_resolution_and_releases_each_candidate(self):
        self.assertTrue(SWEEP_SCRIPT.is_file())
        source = SWEEP_SCRIPT.read_text()
        self.assertIn('default=560', source)
        self.assertIn('default=180', source)
        self.assertIn('scene.render.engine = "BLENDER_EEVEE_NEXT"', source)
        self.assertIn("cleanup_candidate_resources", source)
        self.assertIn("build_comparison_board", source)


@unittest.skipUnless(MANIFEST_PATH.is_file(), "sweep has not been rendered")
class IntactOxideSweepOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST_PATH.read_text())

    def test_manifest_is_exploratory_and_canonical_sources_are_unchanged(self):
        self.assertEqual(
            self.manifest["status"],
            "EXPLORATORY_COMPARISON_NOT_CANONICAL",
        )
        self.assertTrue(self.manifest["canonical_source_unchanged"])
        self.assertFalse(self.manifest["uses_damage"])
        self.assertFalse(self.manifest["unreal_parity_verified"])
        self.assertNotIn("saved_blend", self.manifest)

    def test_every_candidate_has_matching_proofs_and_intact_identity(self):
        candidates = self.manifest["candidates"]
        self.assertEqual(
            list(candidates),
            [
                "A_smooth_control",
                "B_quiet_compact_scale",
                "C_heavy_forged_scale",
                "D_compressed_directional_scale",
            ],
        )
        for candidate_id, candidate in candidates.items():
            with self.subTest(candidate=candidate_id):
                self.assertEqual(candidate["coverage_minimum"], 1.0)
                self.assertEqual(candidate["coverage_maximum"], 1.0)
                self.assertEqual(candidate["metalness_maximum"], 0.0)
                self.assertEqual(candidate["exposure_maximum"], 0.0)
                self.assertGreaterEqual(candidate["quiet_fraction"], 0.0)
                self.assertLessEqual(candidate["quiet_fraction"], 1.0)
                for view in (
                    "front",
                    "grazing",
                    "gameplay",
                    "layer_response",
                ):
                    proof = Path(candidate["renders"][view]["path"])
                    self.assertTrue(proof.is_file())
                    self.assertGreater(proof.stat().st_size, 1024)

    def test_comparison_board_exists(self):
        board = Path(self.manifest["comparison_board"]["path"])
        self.assertTrue(board.is_file())
        self.assertGreater(board.stat().st_size, 4096)


if __name__ == "__main__":
    unittest.main()
