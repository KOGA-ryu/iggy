from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
SCRIPT = MATERIAL_ROOT / "compare_connected_oxide_luster_v1.py"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "connected_oxide_luster_sweep_v1"
MANIFEST_PATH = OUTPUT_ROOT / "manifest.json"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as handle:
        header = handle.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(f"not a PNG: {path}")
    return struct.unpack(">II", header[16:24])


class ConnectedOxideLusterBlueprintTests(unittest.TestCase):
    def test_exact_rows_and_columns_are_coded(self):
        self.assertTrue(SCRIPT.is_file(), "connected oxide/luster renderer is missing")
        source = SCRIPT.read_text()
        for candidate_id in (
            "A_frozen_root_control",
            "B_connected_oxide_no_luster",
            "C_connected_oxide_continuous_luster",
            "D_connected_oxide_open_rail_luster",
        ):
            self.assertIn(candidate_id, source)
        for panel_id in (
            "full_front",
            "close_neutral",
            "opposed_light_difference",
            "gameplay",
            "connected_oxide",
            "independent_roughness",
            "luster_diagnostic",
        ):
            self.assertIn(panel_id, source)

    def test_physical_ownership_and_disposable_contract_are_coded(self):
        source = SCRIPT.read_text()
        for token in (
            "EXPOSED_CONDUCTOR = 0.0",
            "oxide_surface_height_m",
            "broad_forging_height_m",
            "aperiodic_value_field",
            "open_luster_rails",
            "ShaderNodeMixShader",
            "ShaderNodeTangent",
            "ShaderNodeNormalMap",
            "canonical_source_unchanged",
        ):
            self.assertIn(token, source)
        self.assertNotIn("save_as_mainfile", source)
        self.assertNotIn("surface_relief_amplitude", source)

    def test_renderer_is_reduced_and_has_per_row_cleanup(self):
        source = SCRIPT.read_text()
        self.assertIn("default=560", source)
        self.assertIn("default=180", source)
        self.assertIn("cleanup_candidate_resources", source)
        self.assertIn("build_comparison_board", source)


@unittest.skipUnless(MANIFEST_PATH.is_file(), "repair board has not been rendered")
class ConnectedOxideLusterOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST_PATH.read_text())

    def test_manifest_is_disposable_and_sources_are_unchanged(self):
        self.assertEqual(
            self.manifest["status"], "EXPLORATORY_COMPARISON_NOT_CANONICAL"
        )
        self.assertTrue(self.manifest["canonical_source_unchanged"])
        self.assertFalse(self.manifest["uses_ai_generated_imagery"])
        self.assertFalse(self.manifest["uses_condition"])
        self.assertFalse(self.manifest["unreal_parity_verified"])
        self.assertNotIn("saved_blend", self.manifest)

    def test_all_rows_have_seven_hash_locked_panels(self):
        expected_rows = [
            "A_frozen_root_control",
            "B_connected_oxide_no_luster",
            "C_connected_oxide_continuous_luster",
            "D_connected_oxide_open_rail_luster",
        ]
        self.assertEqual(list(self.manifest["candidates"]), expected_rows)
        for candidate in self.manifest["candidates"].values():
            self.assertEqual(len(candidate["renders"]), 7)
            for proof in candidate["renders"].values():
                path = Path(proof["path"])
                self.assertTrue(path.is_file())
                self.assertEqual(png_dimensions(path), (560, 180))
                self.assertEqual(proof["sha256"], sha256_file(path))

    def test_new_rows_preserve_intact_identity_and_zero_oxide_height(self):
        for candidate_id, candidate in self.manifest["candidates"].items():
            if candidate_id.startswith("A_"):
                continue
            self.assertEqual(candidate["coverage_range"], [1.0, 1.0])
            self.assertEqual(candidate["metalness_maximum"], 0.0)
            self.assertEqual(candidate["exposure_maximum"], 0.0)
            self.assertEqual(candidate["oxide_surface_height_absolute_maximum_m"], 0.0)
            self.assertEqual(candidate["topology"]["principled"], 2)
            self.assertEqual(candidate["topology"]["mix_shader"], 1)
            self.assertEqual(candidate["topology"]["image_texture"], 4)

    def test_board_and_manual_decision_are_recorded(self):
        board = Path(self.manifest["comparison_board"]["path"])
        self.assertTrue(board.is_file())
        self.assertEqual(
            self.manifest["comparison_board"]["sha256"], sha256_file(board)
        )
        self.assertEqual(
            self.manifest["comparison_board"]["row_order"],
            list(self.manifest["candidates"]),
        )
        self.assertEqual(len(self.manifest["comparison_board"]["column_order"]), 7)
        self.assertIn(
            self.manifest["visual_review"]["decision"],
            {"pending", "select_B", "select_C", "select_D", "reject_all"},
        )


if __name__ == "__main__":
    unittest.main()
