from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    REPO_ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
)
SCRIPT = MATERIAL_ROOT / "render_actual_hinge_acceptance_board_v1.py"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "actual_hinge_acceptance_board_v1"
MANIFEST = OUTPUT_ROOT / "manifest.json"
BOARD = OUTPUT_ROOT / "forged_iron_v1_actual_hinge_acceptance_board.png"
SOURCE_BLEND = MATERIAL_ROOT / "output" / "forged_iron_v1.blend"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as handle:
        signature = handle.read(8)
        if signature != b"\x89PNG\r\n\x1a\n":
            raise AssertionError(f"Not a PNG: {path}")
        length = struct.unpack(">I", handle.read(4))[0]
        chunk = handle.read(4)
        if length != 13 or chunk != b"IHDR":
            raise AssertionError(f"Missing PNG IHDR: {path}")
        return struct.unpack(">II", handle.read(8))


class ForgedIronAcceptanceBoardBlueprintTests(unittest.TestCase):
    def test_script_is_proof_only_and_targets_the_current_root(self) -> None:
        source = SCRIPT.read_text()
        self.assertIn('SOURCE_BLEND = SCRIPT_ROOT / "output" / "forged_iron_v1.blend"', source)
        self.assertIn('MATERIAL_NAME = "IGGY_MAT_ReferenceForgedIron_v003"', source)
        self.assertIn('PROOF_NODE_NAME = "IGGY_IRON_PROOF_MODE"', source)
        self.assertIn('"status": "USER_NOT_ACCEPTED_PROOF_ONLY"', source)
        self.assertIn('"uses_ai_generated_imagery": False', source)
        self.assertIn('"no_saved_blend": True', source)
        self.assertIn("camera.data.ortho_scale = 16.0", source)
        self.assertNotIn("save_as_mainfile", source)
        self.assertNotIn("save_mainfile", source)

    def test_board_contract_has_twelve_named_actual_consumer_panels(self) -> None:
        source = SCRIPT.read_text()
        for panel in (
            "front",
            "three_quarter",
            "grazing",
            "gameplay",
            "close_neutral",
            "close_light_left",
            "close_light_right",
            "pivot_close",
            "base_colour",
            "roughness",
            "normal",
            "worked_response",
        ):
            self.assertIn(f'("{panel}",', source)
        for object_name in (
            "SM_GH018_FixedLeaf",
            "SM_GH018_MovingLeaf_Openwork",
            "SM_GH018_Pintle",
            "MovingKnuckle_01",
            "FixedKnuckle_02",
            "MovingKnuckle_03",
            "FixedKnuckle_04",
            "MovingKnuckle_05",
        ):
            self.assertIn(f'"{object_name}"', source)


class ForgedIronAcceptanceBoardArtifactTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if not MANIFEST.is_file():
            raise AssertionError(
                "Acceptance-board manifest is absent; run the proof renderer"
            )
        cls.payload = json.loads(MANIFEST.read_text())

    def test_manifest_keeps_manual_acceptance_open(self) -> None:
        payload = self.payload
        self.assertEqual(payload["status"], "USER_NOT_ACCEPTED_PROOF_ONLY")
        self.assertEqual(payload["visual_decision"]["result"], "repair_requested")
        self.assertTrue(payload["review_contract"]["manual_acceptance_required"])
        self.assertFalse(payload["review_contract"]["unreal_parity_verified"])
        self.assertFalse(payload["render_contract"]["uses_ai_generated_imagery"])

    def test_source_blend_is_hash_locked_and_unchanged(self) -> None:
        candidate = self.payload["candidate"]
        current_hash = sha256_file(SOURCE_BLEND)
        self.assertTrue(candidate["source_unchanged"])
        self.assertEqual(candidate["source_blend_sha256_before"], current_hash)
        self.assertEqual(candidate["source_blend_sha256_after"], current_hash)
        self.assertEqual(candidate["material"], "IGGY_MAT_ReferenceForgedIron_v003")
        self.assertEqual(candidate["public_group"], "IGGY_SH_ReferenceForgedIron_v003")

    def test_all_panels_and_board_are_present_and_hash_matched(self) -> None:
        panels = self.payload["panels"]
        self.assertEqual(len(panels), 12)
        self.assertEqual(
            [panel["id"] for panel in panels],
            [
                "front",
                "three_quarter",
                "grazing",
                "gameplay",
                "close_neutral",
                "close_light_left",
                "close_light_right",
                "pivot_close",
                "base_colour",
                "roughness",
                "normal",
                "worked_response",
            ],
        )
        for panel in panels:
            path = Path(panel["path"])
            self.assertTrue(path.is_file(), path)
            self.assertEqual(sha256_file(path), panel["sha256"])
            self.assertEqual(png_dimensions(path), (900, 360))
        self.assertTrue(BOARD.is_file())
        self.assertEqual(sha256_file(BOARD), self.payload["board"]["sha256"])
        self.assertEqual(png_dimensions(BOARD), (3600, 1080))

    def test_locked_absences_and_geometry_span_are_explicit(self) -> None:
        self.assertEqual(
            set(self.payload["locked_absences"]),
            {
                "rust",
                "damage",
                "contact_polish",
                "soot",
                "dirt",
                "blood",
                "cat_face_motifs",
                "diamond_stamps",
            },
        )
        span = self.payload["consumer"]["assembly_bounds_m"]["span"]
        self.assertAlmostEqual(span[0], 4.06, places=3)
        self.assertAlmostEqual(span[2], 0.41, places=3)


if __name__ == "__main__":
    unittest.main()
