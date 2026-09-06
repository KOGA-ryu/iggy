from __future__ import annotations

import hashlib
import json
from pathlib import Path
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
PACKAGE = REPO_ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
SCRIPT = PACKAGE / "compare_v004_1_construction_response_v1.py"
DEMANDS = PACKAGE / "CODED_DEMANDS.md"
SOURCE = (
    PACKAGE
    / "output"
    / "forged_iron_connected_oxide_candidate_v004_1"
    / "forged_iron_connected_oxide_candidate_v004_1.blend"
)
OUTPUT = (
    PACKAGE
    / "output"
    / "forged_iron_v004_1_construction_response_sweep_v1"
)
MANIFEST = OUTPUT / "manifest.json"
BOARD = OUTPUT / "forged_iron_v004_1_construction_response_board.png"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


class ForgedIronConstructionResponseBlueprintTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.script_text = SCRIPT.read_text()
        cls.demands_text = DEMANDS.read_text()

    def test_coded_demand_precedes_the_executable(self) -> None:
        self.assertIn("DEM-PRODUCTION-012", self.demands_text)
        self.assertIn("Disposable construction-owned normal-response sweep", self.demands_text)
        self.assertTrue(SCRIPT.is_file())

    def test_candidate_and_measurement_boundary_is_literal(self) -> None:
        for token in (
            '"A_v004_1_control"',
            '"B_parent_rest_stock"',
            '"C_separate_axial_pintle"',
            '"D_combined_construction"',
            "HUMAN_HAMMER_ENVELOPE_M = (0.041275, 0.031750)",
            "FABRICATION_SCALE = 10.0",
            "GIANT_HAMMER_ENVELOPE_M",
            "STRAP_REVIEW_HEIGHT_P2P_M = 0.003000",
            "PINTLE_REVIEW_HEIGHT_P2P_M = 0.001000",
        ):
            self.assertIn(token, self.script_text)

    def test_live_owners_and_proofs_are_explicit(self) -> None:
        for token in (
            "moving_leaf_stock",
            "fixed_leaf_stock",
            "pintle_stock",
            "parent_rest_stock_coordinates",
            "overlapping_obliterated_work_field",
            "progressive_round_pintle_field",
            "normal_angle_degrees",
            '"isolated_height"',
            '"isolated_normal_angle"',
            '"close_neutral"',
            '"grazing"',
            '"pivot_close"',
            '"gameplay"',
        ):
            self.assertIn(token, self.script_text)

    def test_comparison_is_disposable_and_cycles_based(self) -> None:
        self.assertIn('scene.render.engine = "CYCLES"', self.script_text)
        self.assertIn("scene.render.use_persistent_data = False", self.script_text)
        self.assertIn("bpy.ops.wm.open_mainfile", self.script_text)
        self.assertNotIn("save_as_mainfile", self.script_text)
        self.assertNotIn("save_mainfile", self.script_text)

    def test_prohibited_surface_history_is_locked_out(self) -> None:
        self.assertIn("uses_ai_generated_imagery", self.script_text)
        self.assertIn("uses_condition", self.script_text)
        self.assertIn("manual_acceptance_established", self.script_text)
        self.assertNotIn("ShaderNodeTexNoise", self.script_text)
        self.assertNotIn("ShaderNodeTexVoronoi", self.script_text)


@unittest.skipUnless(MANIFEST.is_file(), "construction-response sweep not generated")
class ForgedIronConstructionResponseOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.manifest = json.loads(MANIFEST.read_text())

    def test_output_is_exploratory_and_source_is_unchanged(self) -> None:
        self.assertEqual(
            self.manifest["status"],
            "EXPLORATORY_CONSTRUCTION_RESPONSE_NOT_CANONICAL",
        )
        self.assertTrue(self.manifest["source_unchanged"])
        self.assertEqual(
            self.manifest["source"]["sha256_before"],
            self.manifest["source"]["sha256_after"],
        )
        self.assertEqual(self.manifest["source"]["sha256_after"], sha256_file(SOURCE))
        self.assertFalse(self.manifest["material_candidate_created"])

    def test_board_and_candidate_matrix_are_complete(self) -> None:
        self.assertTrue(BOARD.is_file())
        self.assertEqual(self.manifest["board"]["resolution"], [3840, 880])
        self.assertEqual(self.manifest["board"]["sha256"], sha256_file(BOARD))
        self.assertEqual(len(self.manifest["candidates"]), 4)
        for row in self.manifest["candidates"]:
            self.assertEqual(len(row["renders"]), 6)
            for render in row["renders"].values():
                path = Path(render["path"])
                self.assertTrue(path.is_file())
                self.assertEqual(render["sha256"], sha256_file(path))

    def test_host_ownership_differs_only_where_authorized(self) -> None:
        rows = {row["candidate_id"]: row for row in self.manifest["candidates"]}
        control = rows["A_v004_1_control"]["components"]
        b = rows["B_parent_rest_stock"]["components"]
        c = rows["C_separate_axial_pintle"]["components"]
        d = rows["D_combined_construction"]["components"]
        for name, baseline in control.items():
            lineage = baseline["stock_lineage"]
            if lineage == "pintle_stock":
                self.assertEqual(b[name]["normal_float_sha256"], baseline["normal_float_sha256"])
                self.assertNotEqual(c[name]["normal_float_sha256"], baseline["normal_float_sha256"])
                self.assertEqual(d[name]["normal_float_sha256"], c[name]["normal_float_sha256"])
            else:
                self.assertNotEqual(b[name]["normal_float_sha256"], baseline["normal_float_sha256"])
                self.assertEqual(c[name]["normal_float_sha256"], baseline["normal_float_sha256"])
                self.assertEqual(d[name]["normal_float_sha256"], b[name]["normal_float_sha256"])

    def test_colour_roughness_and_luster_hashes_never_change(self) -> None:
        rows = self.manifest["candidates"]
        control = rows[0]["components"]
        for row in rows[1:]:
            for name, component in row["components"].items():
                for lane in ("base_colour_source_sha256", "roughness_source_sha256", "luster_source_sha256"):
                    self.assertEqual(component[lane], control[name][lane])

    def test_rest_stock_and_pintle_seams_close(self) -> None:
        seam = self.manifest["construction_continuity"]
        for lineage in ("moving_leaf_stock", "fixed_leaf_stock"):
            self.assertLessEqual(seam[lineage]["maximum_height_error_m"], 1.0e-8)
        self.assertLessEqual(seam["pintle_stock"]["circumferential_height_error_m"], 1.0e-7)

    def test_absences_and_manual_boundary_remain_explicit(self) -> None:
        self.assertFalse(self.manifest["uses_ai_generated_imagery"])
        self.assertFalse(self.manifest["uses_condition"])
        self.assertFalse(self.manifest["manual_acceptance_established"])
        self.assertFalse(self.manifest["unreal_parity_verified"])
        self.assertNotEqual(
            self.manifest["visual_review"]["decision"],
            "visual_review_pending",
        )


if __name__ == "__main__":
    unittest.main()
