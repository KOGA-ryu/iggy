from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
SCRIPT = MATERIAL_ROOT / "audit_v004_1_component_response.py"
RESEARCH = MATERIAL_ROOT / "COMPONENT_RESPONSE_AUDIT.md"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "forged_iron_v004_1_component_response_audit_v1"
MANIFEST = OUTPUT_ROOT / "manifest.json"
BOARD = OUTPUT_ROOT / "forged_iron_v004_1_component_response_audit_board.png"
SOURCE = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_connected_oxide_candidate_v004_1"
    / "forged_iron_connected_oxide_candidate_v004_1.blend"
)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as handle:
        header = handle.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(path)
    return struct.unpack(">II", header[16:24])


class ComponentResponseAuditBlueprintTests(unittest.TestCase):
    def test_script_freezes_lineages_metrics_and_no_save(self):
        self.assertTrue(SCRIPT.is_file(), "component-response audit is missing")
        source = SCRIPT.read_text()
        for token in (
            "moving_leaf_stock",
            "fixed_leaf_stock",
            "pintle_stock",
            "rolled_eye_from_parent_leaf",
            "separate_forged_round_pin",
            "normal_angle_degrees",
            "dominant_wavelength",
            "correlation_matrix",
            "source_unchanged",
            "scene.render.engine",
        ):
            self.assertIn(token, source)
        self.assertNotIn("save_as_mainfile", source)
        self.assertNotIn("save_mainfile", source)

    def test_research_contract_names_operations_and_transfer_limits(self):
        self.assertTrue(RESEARCH.is_file(), "component-response research is missing")
        source = RESEARCH.read_text().lower()
        for token in (
            "forge work",
            "practical forging and art smithing",
            "farmers' museum",
            "food and agriculture organization",
            "rolled",
            "drift",
            "pintle",
            "prohibited inference",
        ):
            self.assertIn(token, source)


@unittest.skipUnless(MANIFEST.is_file(), "component-response audit unavailable")
class ComponentResponseAuditManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = json.loads(MANIFEST.read_text())

    def test_audit_changes_no_source_or_material_state(self):
        self.assertEqual(
            self.payload["status"], "DIAGNOSTIC_AUDIT_NO_MATERIAL_CHANGE"
        )
        self.assertTrue(self.payload["source_unchanged"])
        self.assertEqual(self.payload["source"]["sha256_after"], sha256_file(SOURCE))
        self.assertFalse(self.payload["material_candidate_created"])
        self.assertFalse(self.payload["manual_acceptance_established"])
        self.assertFalse(self.payload["unreal_parity_verified"])

    def test_all_components_have_construction_and_frequency_metrics(self):
        self.assertEqual(len(self.payload["components"]), 8)
        self.assertEqual(set(self.payload["stock_lineages"]), {
            "moving_leaf_stock", "fixed_leaf_stock", "pintle_stock",
        })
        for component in self.payload["components"].values():
            for key in (
                "construction_class",
                "stock_lineage",
                "frame",
                "field_resolution",
                "pitch_m",
                "height_m",
                "normal_angle_degrees",
                "feature_wavelength_m",
            ):
                self.assertIn(key, component)
            angles = component["normal_angle_degrees"]
            self.assertGreaterEqual(angles["maximum"], angles["p95"])
            self.assertGreaterEqual(angles["p95"], angles["median"])

    def test_correlation_and_ownership_findings_are_explicit(self):
        matrix = self.payload["correlation_matrix"]
        self.assertEqual(len(matrix["component_order"]), 8)
        self.assertEqual(len(matrix["values"]), 8)
        self.assertTrue(self.payload["findings"]["normalized_recipe_restarts"])
        self.assertTrue(self.payload["findings"]["leaf_knuckle_seed_continuity_broken"])
        self.assertEqual(
            self.payload["decision"]["result"],
            "reject_shape_only_global_normal_owner",
        )

    def test_four_hash_locked_tiles_and_board_exist(self):
        self.assertEqual(png_dimensions(BOARD), (1920, 640))
        self.assertEqual(self.payload["board"]["sha256"], sha256_file(BOARD))
        self.assertEqual(len(self.payload["diagnostic_tiles"]), 4)
        for tile in self.payload["diagnostic_tiles"].values():
            path = Path(tile["path"])
            self.assertEqual(png_dimensions(path), (960, 320))
            self.assertEqual(tile["sha256"], sha256_file(path))


if __name__ == "__main__":
    unittest.main()
