from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "assets" / "creative" / "materials" / "sword_steel_v1"
PROVENANCE = PACKAGE / "references" / "historical_swords_set_v1.json"
BUILDER = PACKAGE / "build_historical_swords_set_study_v1.py"
OUTPUT = PACKAGE / "output" / "historical_swords_set_study_v1"
SOURCE = OUTPUT / "source" / "historical_swords_set.blend"
MANIFEST = OUTPUT / "manifest.json"
BOARD = OUTPUT / "historical_swords_set_geometry_study_board.png"
EXPECTED_OBJECTS = (
    "ArmingSword",
    "BastardSword",
    "LongSword",
    "ClaymoreSword",
)
EXPECTED_STATIONS = [0.12, 0.25, 0.5, 0.75, 0.88]
EXPECTED_PANELS = (
    "whole_front",
    "hilt_oblique",
    "blade_oblique",
    "blade_topology",
)
EXPECTED_SOURCE_SHA256 = (
    "b090eddf53f63cb8fd8a3bfac0c3dbe36749d934ca079595485629c2a2eb55de"
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


class HistoricalSwordDonorBlueprintTests(unittest.TestCase):
    def test_provenance_is_licensed_hash_locked_and_bounded(self) -> None:
        record = json.loads(PROVENANCE.read_text())
        self.assertEqual(record["schema"], "iggy3d.sword-reference-donor.v1")
        self.assertEqual(record["creator"], "Clint Bellanger")
        self.assertEqual(record["selected_license"]["id"], "CC-BY-3.0")
        self.assertTrue(record["selected_license"]["commercial_use_allowed"])
        self.assertTrue(record["selected_license"]["attribution_required"])
        self.assertEqual(record["source_archive"]["bytes"], 26102712)
        self.assertEqual(record["source_archive"]["sha256"], EXPECTED_SOURCE_SHA256)
        self.assertEqual(record["study_contract"]["source_objects"], list(EXPECTED_OBJECTS))
        self.assertEqual(record["study_contract"]["section_stations_from_blade_root"], EXPECTED_STATIONS)
        self.assertEqual(
            [record["study_contract"][key] for key in ("blade_axis", "width_axis", "thickness_axis")],
            ["+Y", "X", "Z"],
        )
        self.assertIn("historically_measured_specimen", record["prohibited_claims"])
        self.assertFalse(record["study_contract"]["uses_ai_generated_imagery"])

    def test_builder_is_non_destructive_and_material_free(self) -> None:
        self.assertTrue(BUILDER.is_file())
        source = BUILDER.read_text()
        for token in (
            "fetch_hash_locked_source",
            "connected_components",
            "section_intersections",
            "render_geometry_panel",
            "write_section_chart_svg",
            "source_unchanged",
            '"REFERENCE_DONOR_STUDY_NOT_PRODUCTION_ASSET"',
        ):
            self.assertIn(token, source)
        for prohibited in (
            "save_as_mainfile",
            "save_mainfile",
            "ShaderNodeTexNoise",
            "ShaderNodeTexVoronoi",
            "ShaderNodeTexWave",
            "Principled BSDF",
            "material.accepted",
        ):
            self.assertNotIn(prohibited, source)


@unittest.skipUnless(MANIFEST.is_file(), "historical sword study not generated")
class HistoricalSwordDonorOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.payload = json.loads(MANIFEST.read_text())

    def test_source_and_boundary_are_exact(self) -> None:
        self.assertEqual(
            self.payload["status"],
            "REFERENCE_DONOR_STUDY_NOT_PRODUCTION_ASSET",
        )
        self.assertTrue(self.payload["source_unchanged"])
        self.assertEqual(self.payload["source"]["sha256"], EXPECTED_SOURCE_SHA256)
        self.assertEqual(sha256_file(SOURCE), EXPECTED_SOURCE_SHA256)
        self.assertEqual(self.payload["source"]["bytes"], 26102712)
        self.assertEqual(self.payload["source"]["selected_license"], "CC-BY-3.0")
        self.assertFalse(self.payload["runtime_material_created"])
        self.assertFalse(self.payload["manual_acceptance_established"])
        self.assertFalse(self.payload["unreal_parity_verified"])
        self.assertFalse(self.payload["uses_ai_generated_imagery"])

    def test_four_swords_have_components_and_exact_sections(self) -> None:
        self.assertEqual(list(self.payload["swords"]), list(EXPECTED_OBJECTS))
        for sword in self.payload["swords"].values():
            self.assertEqual(set(sword["construction_components"]), {"blade", "guard", "grip", "pommel"})
            self.assertEqual([section["station_fraction"] for section in sword["sections"]], EXPECTED_STATIONS)
            self.assertGreater(sword["blade"]["length_m"], 0.70)
            self.assertGreater(sword["blade"]["maximum_width_m"], 0.04)
            self.assertGreater(sword["blade"]["maximum_thickness_m"], 0.004)
            for section in sword["sections"]:
                self.assertGreaterEqual(len(section["points_m"]), 6)
                self.assertEqual(len(section["points_m"]), len(section["normalized_points"]))
                self.assertGreater(section["width_m"], 0.02)
                self.assertGreater(section["thickness_m"], 0.001)

    def test_board_and_artifacts_are_hash_locked(self) -> None:
        self.assertEqual(png_dimensions(BOARD), (2400, 2240))
        self.assertEqual(self.payload["board"]["sha256"], sha256_file(BOARD))
        self.assertEqual(len(self.payload["artifacts"]["geometry_panels"]), 16)
        self.assertEqual(len(list((OUTPUT / "panels").glob("*.png"))), 16)
        self.assertEqual(len(list((OUTPUT / "sections").glob("*.png"))), 4)
        self.assertEqual(len(list((OUTPUT / "sections").glob("*.svg"))), 4)
        self.assertEqual(
            set(self.payload["artifacts"]["geometry_panels"]),
            {
                f"{sword_name}:{panel}"
                for sword_name in EXPECTED_OBJECTS
                for panel in EXPECTED_PANELS
            },
        )
        self.assertEqual(len(self.payload["artifacts"]["section_charts"]), 4)
        for artifact in (
            list(self.payload["artifacts"]["geometry_panels"].values())
            + list(self.payload["artifacts"]["section_charts"].values())
        ):
            path = Path(artifact["path"])
            self.assertTrue(path.is_file())
            self.assertEqual(artifact["sha256"], sha256_file(path))

    def test_review_identifies_transfer_and_negative_controls(self) -> None:
        review = self.payload["visual_review"]
        self.assertNotEqual(review["decision"], "pending_visual_review")
        self.assertIn("BastardSword", review["best_fuller_study"])
        self.assertIn("LongSword", review["negative_control"])
        self.assertFalse(review["manual_acceptance_established"])


if __name__ == "__main__":
    unittest.main()
