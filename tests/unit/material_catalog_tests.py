from __future__ import annotations

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = ROOT / "assets" / "creative" / "materials"
CATALOG_PATH = MATERIAL_ROOT / "MATERIAL_CATALOG.json"


class MaterialCatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.catalog = json.loads(CATALOG_PATH.read_text())
        cls.views = cls.catalog["lineage_views"]
        cls.lineage = [item for view in cls.views.values() for item in view]
        cls.by_id = {item["id"]: item for item in cls.lineage}

    def test_catalog_is_a_generated_surface_foundry_snapshot(self):
        self.assertEqual(
            self.catalog["schema"],
            "surface-foundry.workbench-catalog.v2",
        )
        snapshot = self.catalog["snapshot"]
        self.assertEqual(snapshot["source"], "sf:catalog/materials.json")
        self.assertEqual(snapshot["consumer_policy"], "read_only_generated_snapshot")
        self.assertEqual(len(snapshot["source_sha256"]), 64)

    def test_every_local_material_package_is_registered_or_explicit_support(self):
        packages = {
            path.name
            for path in MATERIAL_ROOT.iterdir()
            if path.is_dir() and path.name not in {"__pycache__", "workflow"}
        }
        registered = {
            item["id"]
            for item in self.lineage
            if item["id"] in packages
        }
        self.assertEqual(packages - registered, {"palette_v1"})

    def test_three_lifecycle_views_are_disjoint(self):
        self.assertEqual(set(self.views), {"current", "workbench", "archive"})
        ids_by_lane = {
            lane: {item["id"] for item in records}
            for lane, records in self.views.items()
        }
        self.assertFalse(ids_by_lane["current"] & ids_by_lane["workbench"])
        self.assertFalse(ids_by_lane["current"] & ids_by_lane["archive"])
        self.assertFalse(ids_by_lane["workbench"] & ids_by_lane["archive"])
        for lane, records in self.views.items():
            self.assertTrue(all(item["lane"] == lane for item in records))

    def test_current_scopes_have_one_owner(self):
        scopes = [
            (item["material_id"], item["scope"])
            for item in self.views["current"]
        ]
        self.assertEqual(len(scopes), len(set(scopes)))

    def test_only_bound_user_rulings_claim_acceptance(self):
        ruled = {
            item["id"]: item
            for item in self.views["current"]
            if item["status"] in {"user_accepted", "user_not_accepted"}
        }
        self.assertEqual(
            {item_id for item_id, item in ruled.items() if item["status"] == "user_accepted"},
            {"structural_oak_timber_v1", "structural_oak_joinery_v1"},
        )
        self.assertEqual(
            {item_id for item_id, item in ruled.items() if item["status"] == "user_not_accepted"},
            {"structural_oak_door_v1", "forged_iron_v1"},
        )
        self.assertTrue(all(item["decision"]["passed"] for item in ruled.values()))

    def test_forged_iron_current_owner_is_not_a_nested_prototype(self):
        iron = self.by_id["forged_iron_v1"]
        self.assertEqual(iron["lane"], "current")
        self.assertEqual(iron["status"], "user_not_accepted")
        self.assertTrue(
            iron["proof"]["display_path"].endswith(
                "actual_hinge_acceptance_board_v1/"
                "forged_iron_v1_actual_hinge_acceptance_board.png"
            )
        )
        archive = {item["id"] for item in self.views["archive"]}
        self.assertIn("forged_iron_cumulative_hinge_preview_v1", archive)
        self.assertIn("forged_iron_quiet_oxide_repair_v1", archive)

    def test_sword_family_includes_cross_repository_master_sword_work(self):
        master = self.by_id["master_sword_blade_palette_v1"]
        self.assertEqual(master["lane"], "current")
        self.assertEqual(master["status"], "component_candidate")
        self.assertTrue(master["proof"]["display_path"].startswith("sf:"))
        self.assertTrue(
            master["proof"]["display_path"].endswith("master_sword_strategy_board.png")
        )

    def test_all_lineage_proofs_existed_when_snapshot_was_generated(self):
        for item in self.lineage:
            with self.subTest(lineage=item["id"]):
                self.assertTrue(item["proof"]["exists"])
                self.assertIsNotNone(item["proof"]["url"])


if __name__ == "__main__":
    unittest.main()
