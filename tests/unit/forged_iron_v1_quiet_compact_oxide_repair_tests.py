from __future__ import annotations

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
SCRIPT = MATERIAL_ROOT / "repair_quiet_compact_oxide_v1.py"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "quiet_compact_oxide_repair_v1"
MANIFEST = OUTPUT_ROOT / "manifest.json"


class QuietCompactOxideRepairBlueprintTests(unittest.TestCase):
    def test_repair_uses_nonperiodic_separated_material_bands(self):
        source = SCRIPT.read_text()
        self.assertIn('mode="reflect"', source)
        self.assertIn("np.fft.rfft2", source)
        self.assertIn("np.fft.irfft2", source)
        self.assertIn("FIELD_WAVELENGTHS_M = (0.240, 0.075, 0.024)", source)
        self.assertIn("finite_flow_rails", source)
        self.assertIn("FLOW_RAIL_COUNT = 3", source)
        self.assertNotIn("open_mode_field(", source)

    def test_repair_preserves_palette_and_continuous_intact_scale(self):
        source = SCRIPT.read_text()
        self.assertIn("thin_srgb=(0.090, 0.100, 0.116)", source)
        self.assertIn("thick_srgb=(0.158, 0.174, 0.202)", source)
        self.assertIn("morphology_strength = 0.16 + 0.84 * priority", source)
        self.assertIn('"preserves_cumulative_intact_palette": True', source)
        self.assertIn('"continuous_morphology_floor": 0.16', source)

    def test_grain_is_owned_only_by_roughness(self):
        source = SCRIPT.read_text()
        self.assertIn('"grain_outputs": ["roughness"]', source)
        self.assertIn('"grain_prohibited_outputs": ["base_color", "height", "normal", "metalness"]', source)
        self.assertIn("grain * priority * RECIPE.grain_roughness_amplitude", source)
        self.assertNotIn("grain[..., None]", source)
        self.assertNotIn("+ grain * RECIPE.surface_relief_amplitude_m", source)

    def test_repair_preserves_intact_identity_and_canonical_sources(self):
        source = SCRIPT.read_text()
        self.assertIn("EXPOSED_CONDUCTOR = 0.0", source)
        self.assertIn('"B_REPAIR_PROOF_NOT_CANONICAL"', source)
        self.assertIn('"uses_damage": False', source)
        self.assertIn('"uses_rust": False', source)
        self.assertIn('"uses_contact_polish": False', source)
        self.assertIn('"saved_blend": None', source)
        self.assertNotIn("save_as_mainfile", source)

    def test_repair_proof_has_six_decisive_views(self):
        source = SCRIPT.read_text()
        for view in (
            "front",
            "grazing",
            "gameplay",
            "roughness",
            "medium_morphology",
            "causal_response",
        ):
            self.assertIn(f'"{view}"', source)
        self.assertIn("build_board", source)


@unittest.skipUnless(MANIFEST.is_file(), "B-only repair has not been rendered")
class QuietCompactOxideRepairOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST.read_text())

    def test_manifest_is_intact_noncanonical_and_source_safe(self):
        self.assertEqual(self.manifest["status"], "B_REPAIR_PROOF_NOT_CANONICAL")
        self.assertTrue(self.manifest["canonical_source_unchanged"])
        self.assertFalse(self.manifest["uses_ai_generated_imagery"])
        self.assertFalse(self.manifest["uses_damage"])
        self.assertFalse(self.manifest["uses_rust"])
        self.assertFalse(self.manifest["uses_contact_polish"])
        self.assertIsNone(self.manifest["saved_blend"])
        self.assertFalse(self.manifest["unreal_parity_verified"])

    def test_manifest_preserves_physical_and_pattern_contracts(self):
        metrics = self.manifest["metrics"]
        self.assertEqual(metrics["coverage_minimum"], 1.0)
        self.assertEqual(metrics["coverage_maximum"], 1.0)
        self.assertEqual(metrics["metalness_maximum"], 0.0)
        self.assertEqual(metrics["exposure_maximum"], 0.0)
        self.assertGreater(metrics["quiet_fraction"], 0.50)
        self.assertLess(metrics["maximum_periodic_shift_correlation"], 0.85)
        self.assertEqual(self.manifest["recipe"]["finite_flow_rail_count"], 3)
        self.assertEqual(self.manifest["recipe"]["grain_outputs"], ["roughness"])
        self.assertEqual(self.manifest["recipe"]["repair_revision"], 2)
        self.assertTrue(self.manifest["recipe"]["preserves_cumulative_intact_palette"])
        self.assertAlmostEqual(self.manifest["recipe"]["continuous_morphology_floor"], 0.16)

    def test_all_decisive_proofs_exist(self):
        self.assertEqual(
            set(self.manifest["renders"]),
            {
                "front",
                "grazing",
                "gameplay",
                "roughness",
                "medium_morphology",
                "causal_response",
            },
        )
        for record in self.manifest["renders"].values():
            path = Path(record["path"])
            self.assertTrue(path.is_file())
            self.assertGreater(path.stat().st_size, 4096)
        board = Path(self.manifest["comparison_board"]["path"])
        self.assertTrue(board.is_file())
        self.assertGreater(board.stat().st_size, 4096)


if __name__ == "__main__":
    unittest.main()
