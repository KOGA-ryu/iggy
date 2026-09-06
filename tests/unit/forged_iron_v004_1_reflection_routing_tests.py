from __future__ import annotations

import hashlib
import json
from pathlib import Path
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
PACKAGE = REPO_ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
SCRIPT = PACKAGE / "compare_v004_1_reflection_routing_v1.py"
DEMANDS = PACKAGE / "CODED_DEMANDS.md"
SOURCE = (
    PACKAGE
    / "output"
    / "forged_iron_connected_oxide_candidate_v004_1"
    / "forged_iron_connected_oxide_candidate_v004_1.blend"
)
OUTPUT = PACKAGE / "output" / "forged_iron_v004_1_reflection_routing_sweep_v1"
MANIFEST = OUTPUT / "manifest.json"
BOARD = OUTPUT / "forged_iron_v004_1_reflection_routing_board.png"
CLAY_BOARD = OUTPUT / "forged_iron_v004_1_geometry_moving_strip_board.png"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


class ForgedIronReflectionRoutingBlueprintTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.script_text = SCRIPT.read_text()
        cls.demands_text = DEMANDS.read_text()

    def test_demand_precedes_executable(self) -> None:
        self.assertIn("DEM-PRODUCTION-013", self.demands_text)
        self.assertIn("Disposable clean-metal response-routing sweep", self.demands_text)
        self.assertTrue(SCRIPT.is_file())

    def test_rows_and_authored_response_constants_are_literal(self) -> None:
        for token in (
            '"A_v004_1_control"',
            '"B_construction_roughness_only"',
            '"C_directional_reflection_only"',
            '"D_roughness_plus_direction"',
            "ROUGHNESS_LONGITUDINAL_AMPLITUDE = 0.040",
            "PINTLE_ROUGHNESS_AMPLITUDE = 0.020",
            "DIRECTIONAL_RESPONSE_AMOUNT = 0.28",
            "DIRECTIONAL_RESPONSE_ROTATION = 0.50",
        ):
            self.assertIn(token, self.script_text)

    def test_open_roughness_and_uniform_direction_owners_are_explicit(self) -> None:
        for token in (
            "open_cubic_hermite_curve",
            "construction_owned_roughness",
            "parent_rest_stock_coordinates",
            "build_disposable_direction_group",
            "Worked_Luster_Disabled",
            "IGGY_IronTangentUV_v001",
            "render_geometry_moving_strip_proof",
            '"isolated_roughness"',
            '"isolated_direction_amount"',
            '"grazing_left"',
            '"grazing_right"',
        ):
            self.assertIn(token, self.script_text)

    def test_no_new_normal_or_saved_candidate_route_exists(self) -> None:
        self.assertNotIn("height_to_normal", self.script_text)
        self.assertNotIn("save_as_mainfile", self.script_text)
        self.assertNotIn("save_mainfile", self.script_text)
        self.assertNotIn("ShaderNodeTexNoise", self.script_text)
        self.assertNotIn("ShaderNodeTexVoronoi", self.script_text)
        self.assertIn('scene.render.engine = "CYCLES"', self.script_text)
        self.assertIn("scene.render.use_persistent_data = False", self.script_text)


@unittest.skipUnless(MANIFEST.is_file(), "reflection-routing sweep not generated")
class ForgedIronReflectionRoutingOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.manifest = json.loads(MANIFEST.read_text())
        cls.rows = {row["candidate_id"]: row for row in cls.manifest["candidates"]}

    def test_source_and_saved_group_remain_unchanged(self) -> None:
        self.assertEqual(
            self.manifest["status"],
            "EXPLORATORY_REFLECTION_ROUTING_NOT_CANONICAL",
        )
        self.assertTrue(self.manifest["source_unchanged"])
        self.assertEqual(self.manifest["source"]["sha256_after"], sha256_file(SOURCE))
        self.assertTrue(self.manifest["source_group_unchanged"])
        self.assertFalse(self.manifest["material_candidate_created"])

    def test_clay_and_material_boards_are_complete(self) -> None:
        self.assertTrue(CLAY_BOARD.is_file())
        self.assertTrue(BOARD.is_file())
        self.assertEqual(self.manifest["geometry_proof"]["board"]["resolution"], [3200, 220])
        self.assertEqual(self.manifest["geometry_proof"]["board"]["sha256"], sha256_file(CLAY_BOARD))
        self.assertEqual(self.manifest["board"]["resolution"], [4480, 880])
        self.assertEqual(self.manifest["board"]["sha256"], sha256_file(BOARD))
        self.assertEqual(len(self.rows), 4)
        for row in self.rows.values():
            self.assertEqual(len(row["renders"]), 7)

    def test_b_and_d_change_only_roughness_pixels(self) -> None:
        a = self.rows["A_v004_1_control"]["components"]
        b = self.rows["B_construction_roughness_only"]["components"]
        c = self.rows["C_directional_reflection_only"]["components"]
        d = self.rows["D_roughness_plus_direction"]["components"]
        for name in a:
            self.assertNotEqual(b[name]["roughness_float_sha256"], a[name]["roughness_float_sha256"])
            self.assertEqual(c[name]["roughness_float_sha256"], a[name]["roughness_float_sha256"])
            self.assertEqual(d[name]["roughness_float_sha256"], b[name]["roughness_float_sha256"])
            for frozen_lane in ("base_colour_source_sha256", "normal_source_sha256"):
                self.assertEqual(b[name][frozen_lane], a[name][frozen_lane])
                self.assertEqual(c[name][frozen_lane], a[name][frozen_lane])
                self.assertEqual(d[name][frozen_lane], a[name][frozen_lane])

    def test_c_and_d_only_enable_uniform_direction_response(self) -> None:
        a = self.rows["A_v004_1_control"]
        b = self.rows["B_construction_roughness_only"]
        c = self.rows["C_directional_reflection_only"]
        d = self.rows["D_roughness_plus_direction"]
        self.assertEqual(a["directional_response_amount"], 0.0)
        self.assertEqual(b["directional_response_amount"], 0.0)
        self.assertEqual(c["directional_response_amount"], 0.28)
        self.assertEqual(d["directional_response_amount"], 0.28)
        self.assertFalse(a["disposable_direction_group_created"])
        self.assertFalse(b["disposable_direction_group_created"])
        self.assertTrue(c["disposable_direction_group_created"])
        self.assertTrue(d["disposable_direction_group_created"])

    def test_roughness_envelope_and_mean_drift_hold(self) -> None:
        for row_id in ("B_construction_roughness_only", "D_roughness_plus_direction"):
            for component in self.rows[row_id]["components"].values():
                self.assertGreaterEqual(component["roughness"]["minimum"], 0.58)
                self.assertLessEqual(component["roughness"]["maximum"], 0.80)
                self.assertLessEqual(abs(component["roughness_mean_drift"]), 0.005)

    def test_review_and_absence_boundaries_are_explicit(self) -> None:
        self.assertNotEqual(self.manifest["visual_review"]["decision"], "visual_review_pending")
        self.assertFalse(self.manifest["uses_ai_generated_imagery"])
        self.assertFalse(self.manifest["uses_condition"])
        self.assertFalse(self.manifest["manual_acceptance_established"])
        self.assertFalse(self.manifest["unreal_parity_verified"])


if __name__ == "__main__":
    unittest.main()
