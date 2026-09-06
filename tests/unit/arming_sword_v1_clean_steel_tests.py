from __future__ import annotations

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "assets" / "creative" / "materials" / "sword_steel_v1"
PROFILE_PATH = PACKAGE / "profiles" / "arming_sword_v1_clean_steel.json"
BUILDER_PATH = PACKAGE / "build_arming_sword_v1_clean_steel.py"
OUTPUT_ROOT = PACKAGE / "output" / "arming_sword_v1_consumer"
MANIFEST_PATH = OUTPUT_ROOT / "arming_sword_v1_clean_steel_manifest.json"


def resolve_output(record: dict[str, object]) -> Path:
    path = Path(str(record["path"]))
    return path if path.is_absolute() else OUTPUT_ROOT / path


class ArmingSwordCleanSteelContractTests(unittest.TestCase):
    def test_actual_consumer_profile_records_measured_source_without_fuller(self):
        profile = json.loads(PROFILE_PATH.read_text())
        consumer = profile["consumer"]
        geometry = profile["geometry"]
        self.assertTrue(consumer["actual_production_mesh_present"])
        self.assertEqual(consumer["source_host"], "linux-worker")
        self.assertEqual(consumer["source_object"], "SW_blade.001")
        self.assertEqual(consumer["blade_length_m"], 0.780)
        self.assertEqual(consumer["shoulder_width_m"], 0.048)
        self.assertEqual(consumer["shoulder_thickness_m"], 0.006)
        self.assertEqual(geometry["cross_section"], "lenticular_with_secondary_bevel")
        self.assertEqual(geometry["secondary_bevel_half_width_fraction"], 0.22)
        self.assertEqual(geometry["edge_land_m"], 0.0004)
        self.assertFalse(geometry["has_fuller"])
        self.assertEqual(geometry["source_zone_role"], "audit_only_not_semantic_owner")
        self.assertEqual(
            geometry["expected_region_polygon_counts"],
            {"body": 176, "fuller": 0, "bevel": 56, "edge": 36},
        )
        self.assertFalse(profile["finish"]["regions"]["fuller"]["enabled"])

    def test_actual_consumer_builder_reuses_canonical_shader_and_source_geometry(self):
        source = BUILDER_PATH.read_text()
        for required in (
            "CANONICAL_BUILDER_PATH",
            "load_canonical_builder",
            "canonical.build_sword_steel_material",
            "canonical.generate_macro_polish_field",
            "canonical.generate_grind_field",
            "execute_factory_prefix",
            "apply_factory_blade_baseline",
            "select_generated_blade",
            "author_blade_semantics",
            "IGGY_BladeUV",
            "sinc_sword_region",
            "blade_edge",
            "is_cap",
            "has_fuller",
            "validate_saved_blend",
            "--validate-only",
        ):
            self.assertIn(required, source)
        for prohibited in (
            "create_analytic_blade_fixture",
            "ShaderNodeTexNoise",
            "ShaderNodeTexVoronoi",
            "ShaderNodeTexWave",
            "np.random",
            "rust_mask",
            "pitting_mask",
            'marker = "\\nMID = 0.33\\n"',
            "zone_names(grip_ob",
        ):
            self.assertNotIn(prohibited, source)


@unittest.skipUnless(MANIFEST_PATH.is_file(), "actual sword consumer has not been built")
class ArmingSwordCleanSteelOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST_PATH.read_text())

    def test_manifest_proves_the_factory_blade_and_material_contract(self):
        manifest = self.manifest
        self.assertEqual(manifest["status"], "ACTUAL_ASSET_PRODUCTION_CANDIDATE")
        self.assertEqual(manifest["consumer"]["source_object"], "SW_blade.001")
        self.assertEqual(manifest["geometry"]["polygon_count"], 268)
        self.assertEqual(manifest["geometry"]["quad_count"], 268)
        self.assertEqual(manifest["geometry"]["dimensions_m"], [0.048, 0.006, 0.78])
        self.assertEqual(manifest["geometry"]["region_polygon_counts"]["fuller"], 0)
        self.assertEqual(manifest["material"]["principled_count"], 1)
        self.assertEqual(manifest["material"]["image_texture_count"], 2)
        self.assertTrue(manifest["saved_blend_images_packed"])
        self.assertTrue(manifest["reopen_validated"])
        self.assertFalse(manifest["uses_damage"])
        self.assertFalse(manifest["unreal_parity_verified"])

    def test_actual_asset_proof_set_exists(self):
        for key in ("saved_blend", "comparison_board", "macro_polish_field", "grind_field"):
            path = resolve_output(self.manifest["outputs"][key])
            self.assertTrue(path.is_file(), key)
            self.assertGreater(path.stat().st_size, 1024)
        for view in (
            "baseline",
            "front",
            "three_quarter",
            "grazing",
            "gameplay",
            "clay",
            "regions",
        ):
            path = resolve_output(self.manifest["outputs"]["renders"][view])
            self.assertTrue(path.is_file(), view)
            self.assertGreater(path.stat().st_size, 1024)


if __name__ == "__main__":
    unittest.main()
