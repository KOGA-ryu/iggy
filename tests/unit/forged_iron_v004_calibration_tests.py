from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
SCRIPT = MATERIAL_ROOT / "compare_v004_optical_response_calibration.py"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "forged_iron_v004_optical_calibration_v1"
MANIFEST = OUTPUT_ROOT / "manifest.json"
SOURCE = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_connected_oxide_candidate_v004"
    / "forged_iron_connected_oxide_candidate_v004.blend"
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
        raise AssertionError(f"not a PNG: {path}")
    return struct.unpack(">II", header[16:24])


class ForgedIronV004CalibrationBlueprintTests(unittest.TestCase):
    def test_exact_candidates_and_audited_anchors_are_coded(self):
        self.assertTrue(SCRIPT.is_file(), "v004 optical calibration renderer is missing")
        source = SCRIPT.read_text()
        for candidate_id in (
            "A_current_v004_control",
            "B_audited_dark_half",
            "C_audited_compressed_cool",
            "D_audited_cool_to_warm",
        ):
            self.assertIn(candidate_id, source)
        for anchor in ("#252a31", "#41464f", "#353b46", "#292e36", "#574f4b"):
            self.assertIn(anchor, source)

    def test_scope_is_only_existing_palette_roughness_and_normal_owners(self):
        source = SCRIPT.read_text()
        for token in (
            "thermal_response",
            "connected_medium",
            "aggregate_grain",
            "broad_forging_height_m",
            "height_to_normal_nonperiodic",
            "EXPOSED_CONDUCTOR = 0.0",
            "cleanup_candidate_resources",
            'scene.render.engine = "CYCLES"',
            "default=640",
            "default=220",
        ):
            self.assertIn(token, source)
        self.assertNotIn("save_as_mainfile", source)
        self.assertNotIn("save_mainfile", source)


@unittest.skipUnless(MANIFEST.is_file(), "calibration board has not been rendered")
class ForgedIronV004CalibrationOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = json.loads(MANIFEST.read_text())

    def test_manifest_is_disposable_and_source_is_hash_locked(self):
        self.assertEqual(self.payload["status"], "EXPLORATORY_CALIBRATION_NOT_CANONICAL")
        self.assertTrue(self.payload["source_unchanged"])
        self.assertEqual(self.payload["source"]["sha256_after"], sha256_file(SOURCE))
        self.assertFalse(self.payload["uses_ai_generated_imagery"])
        self.assertFalse(self.payload["uses_condition"])
        self.assertFalse(self.payload["unreal_parity_verified"])
        self.assertNotIn("saved_blend", self.payload)

    def test_all_four_rows_have_seven_cycles_proofs_and_intact_identity(self):
        self.assertEqual(len(self.payload["candidates"]), 4)
        for candidate in self.payload["candidates"].values():
            self.assertEqual(len(candidate["renders"]), 7)
            self.assertEqual(candidate["coverage_range"], [1.0, 1.0])
            self.assertEqual(candidate["metalness_maximum"], 0.0)
            self.assertEqual(candidate["exposure_maximum"], 0.0)
            self.assertEqual(candidate["oxide_height_absolute_maximum_m"], 0.0)
            self.assertEqual(candidate["luster_amount_range"], [0.0, 0.0])
            for proof in candidate["renders"].values():
                path = Path(proof["path"])
                self.assertTrue(path.is_file())
                self.assertEqual(png_dimensions(path), (640, 220))
                self.assertEqual(proof["sha256"], sha256_file(path))

    def test_board_and_bounded_visual_decision_are_recorded(self):
        board = Path(self.payload["board"]["path"])
        self.assertTrue(board.is_file())
        self.assertEqual(self.payload["board"]["sha256"], sha256_file(board))
        self.assertIn(
            self.payload["visual_review"]["decision"],
            {"pending", "select_B", "select_C", "select_D", "reject_all"},
        )
        self.assertFalse(self.payload["visual_review"]["manual_acceptance_established"])


if __name__ == "__main__":
    unittest.main()
