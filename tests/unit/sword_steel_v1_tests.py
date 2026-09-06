from __future__ import annotations

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "assets" / "creative" / "materials" / "sword_steel_v1"
PROFILE_PATH = PACKAGE / "profiles" / "sword_steel_v1.json"
BUILDER_PATH = PACKAGE / "build_sword_steel_v1.py"
OUTPUT_ROOT = PACKAGE / "output"
MANIFEST_PATH = OUTPUT_ROOT / "sword_steel_v1_manifest.json"


class SwordSteelProfileTests(unittest.TestCase):
    def test_profile_is_minimal_clean_conductor_derivative(self):
        profile = json.loads(PROFILE_PATH.read_text())
        self.assertEqual(profile["consumer"]["blade_length_m"], 0.889)
        self.assertEqual(profile["consumer"]["base_width_m"], 0.0318)
        self.assertFalse(profile["consumer"]["actual_production_mesh_present"])
        self.assertEqual(profile["donors"]["expected_f0_linear"], [0.56, 0.57, 0.58])
        self.assertEqual(profile["donors"]["body_roughness"], 0.38)
        self.assertEqual(profile["donors"]["body_anisotropy"], 0.18)
        self.assertEqual(
            profile["finish"]["medium_grind"]["map_resolution"],
            [1024, 256],
        )
        self.assertEqual(profile["finish"]["medium_grind"]["track_count"], 47)
        self.assertEqual(
            profile["workflow_contract"]["surface_method_contract"]
            ["damage_placement_state"],
            "absent",
        )

    def test_builder_uses_one_map_one_bsdf_and_geometry_regions(self):
        self.assertTrue(BUILDER_PATH.is_file(), "sword-steel builder is missing")
        source = BUILDER_PATH.read_text()
        for required in (
            "CONDUCTOR_MANIFEST",
            "TANGENT_MANIFEST",
            "generate_macro_polish_field",
            "generate_grind_field",
            "create_analytic_blade_fixture",
            "sinc_sword_region",
            "IGGY_BladeUV",
            "build_sword_steel_material",
            "ShaderNodeBsdfPrincipled",
            "ShaderNodeTangent",
            "ShaderNodeBump",
            "macro_strength",
            "grind_strength",
            "validate_saved_blend",
            "--validate-only",
        ):
            self.assertIn(required, source)
        for prohibited in (
            'tree.nodes.new("ShaderNodeMixShader")',
            "ShaderNodeTexNoise",
            "ShaderNodeTexVoronoi",
            "ShaderNodeTexWave",
            "np.random",
            "rust_mask",
            "pitting_mask",
            "contact_polish",
        ):
            self.assertNotIn(prohibited, source)

    def test_profile_declares_three_distinct_finish_bands(self):
        profile = json.loads(PROFILE_PATH.read_text())
        finish = profile["finish"]
        self.assertEqual(finish["macro_polish"]["map_resolution"], [512, 128])
        self.assertEqual(finish["macro_polish"]["control_grid_shape"], [5, 8])
        self.assertEqual(finish["medium_grind"]["map_resolution"], [1024, 256])
        self.assertEqual(finish["medium_grind"]["track_count"], 47)
        self.assertEqual(
            finish["micro_response"]["owner"],
            "principled_anisotropy",
        )
        self.assertIsNone(finish["micro_response"]["texture_map"])
        for values in finish["regions"].values():
            self.assertIn("macro_strength", values)
            self.assertIn("grind_strength", values)


@unittest.skipUnless(MANIFEST_PATH.is_file(), "sword steel has not been built")
class SwordSteelOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST_PATH.read_text())

    def test_manifest_records_exact_donor_reuse_and_fixture_boundary(self):
        manifest = self.manifest
        self.assertEqual(manifest["status"], "DIAGNOSTIC_PRODUCTION_CANDIDATE")
        self.assertEqual(manifest["material"]["principled_count"], 1)
        self.assertEqual(manifest["material"]["mix_shader_count"], 0)
        self.assertEqual(manifest["material"]["image_texture_count"], 2)
        self.assertEqual(manifest["material"]["metalness"], 1.0)
        self.assertEqual(manifest["material"]["conductor_f0_linear"], [0.56, 0.57, 0.58])
        self.assertEqual(manifest["material"]["body_roughness"], 0.38)
        self.assertEqual(manifest["material"]["body_anisotropy"], 0.18)
        self.assertFalse(manifest["consumer"]["actual_production_mesh_present"])
        self.assertTrue(manifest["uses_damage"] is False)
        self.assertFalse(manifest["unreal_parity_verified"])
        self.assertTrue(manifest["reopen_validated"])
        self.assertEqual(
            manifest["finish"]["micro_response"]["owner"],
            "principled_anisotropy",
        )
        self.assertIsNone(
            manifest["finish"]["micro_response"]["texture_map"]
        )

    def test_map_blend_and_proofs_exist(self):
        for key in (
            "macro_polish_field",
            "grind_field",
            "saved_blend",
            "comparison_board",
        ):
            record = self.manifest["outputs"][key]
            path = Path(record["path"])
            self.assertTrue(path.is_file(), key)
            self.assertGreater(path.stat().st_size, 1024)
        for view in (
            "front",
            "grazing",
            "gameplay",
            "clay",
            "regions",
            "macro",
            "finish",
        ):
            path = Path(self.manifest["outputs"]["renders"][view]["path"])
            self.assertTrue(path.is_file(), view)
            self.assertGreater(path.stat().st_size, 1024)
        self.assertTrue(self.manifest["saved_blend_images_packed"])


if __name__ == "__main__":
    unittest.main()
