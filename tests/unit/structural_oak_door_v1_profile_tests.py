import ast
import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "structural_oak_door_v1"
)
PROFILE_PATH = (
    MATERIAL_ROOT / "profiles" / "structural_oak_door_v1.json"
)
RECIPE_PATH = (
    MATERIAL_ROOT
    / "patterns"
    / "structural_oak_reference_fields_v1.json"
)
SAMPLES_PATH = (
    MATERIAL_ROOT
    / "references"
    / "met_468492_reverse_palette_samples.json"
)
GENERATOR_PATH = (
    MATERIAL_ROOT / "generate_structural_oak_patterns_v1.py"
)
BUILDER_PATH = MATERIAL_ROOT / "build_structural_oak_door_v1.py"


class StructuralOakDoorProfileTests(unittest.TestCase):
    def setUp(self):
        self.profile = json.loads(PROFILE_PATH.read_text())
        self.recipe = json.loads(RECIPE_PATH.read_text())
        self.sample_document = json.loads(SAMPLES_PATH.read_text())
        self.samples = self.sample_document[
            "eyedropper_samples"
        ]["samples"]

    def test_profile_is_colour_and_linework_only(self):
        excluded = set(self.profile["excluded_lanes"])
        self.assertTrue(
            {
                "roughness",
                "normal",
                "height",
                "bump",
                "damage",
                "edge_wear",
                "scratches",
                "dings",
                "scuffs",
                "cracks",
                "chips",
                "stains",
                "weathering",
                "periodic_grain",
                "random_noise",
            }.issubset(excluded)
        )
        self.assertNotIn("image_texture", excluded)
        self.assertEqual(
            self.profile["surface_response"]["model"],
            "uniform_diffuse",
        )
        self.assertFalse(
            self.profile["surface_response"]["authored_roughness"]
        )

    def test_twenty_palette_anchors_are_exact_eyedropper_samples(self):
        sampled_hex = [sample["hex"] for sample in self.samples]
        palette = self.profile["palette"][
            "observed_reference_twenty"
        ]
        self.assertEqual(len(palette), 20)
        self.assertEqual(len(set(palette)), 20)
        self.assertEqual(palette, sampled_hex)

    def test_reference_authority_is_the_door_used_for_the_model(self):
        authority = self.profile["reference_authority"]
        recipe_reference = self.recipe["reference"]
        self.assertEqual(authority["object_number"], "55.61.170")
        self.assertEqual(
            recipe_reference["object_number"],
            authority["object_number"],
        )
        self.assertEqual(
            recipe_reference["object_url"],
            authority["object_url"],
        )
        self.assertEqual(
            recipe_reference["palette_samples"],
            "../references/met_468492_reverse_palette_samples.json",
        )
        self.assertTrue(
            Path(
                recipe_reference["local_reverse_path"]
            ).is_file()
        )

    def test_every_variant_has_reference_fields_and_sparse_fibres(self):
        labels = {sample["label"] for sample in self.samples}
        variants = self.recipe["variants"]
        self.assertEqual(
            len(variants),
            self.recipe["tile_columns"]
            * self.recipe["tile_rows"],
        )
        self.assertTrue(
            self.recipe["reference_field"][
                "all_twenty_samples_influence_every_variant"
            ]
        )
        for variant in variants:
            self.assertGreaterEqual(len(variant["colour_fields"]), 3)
            self.assertGreaterEqual(len(variant["fibres"]), 4)
            self.assertIn(
                variant["photo_crop"],
                self.recipe["photo_field"]["crops"],
            )
            used_labels = {
                field["sample"]
                for field in variant["colour_fields"]
            } | {
                fibre["sample"] for fibre in variant["fibres"]
            } | set(variant["anchor_emphasis"])
            self.assertTrue(used_labels.issubset(labels))
            self.assertTrue(
                all(
                    float(fibre["ink"]) <= 0.40
                    for fibre in variant["fibres"]
                )
            )

    def test_recipe_has_no_floor_or_decorative_grain_grammar(self):
        source = RECIPE_PATH.read_text()
        self.assertNotIn("wood_plank_v2", source)
        self.assertNotIn("ring_offsets", source)
        self.assertNotIn('"mode": "cathedral"', source)
        self.assertNotIn('"mode": "rift"', source)

    def test_generator_is_deterministic_reference_reduction(self):
        source = GENERATOR_PATH.read_text()
        syntax = ast.parse(source)
        imported_roots = {
            alias.name.split(".")[0]
            for node in ast.walk(syntax)
            if isinstance(node, (ast.Import, ast.ImportFrom))
            for alias in node.names
        }
        self.assertNotIn("random", imported_roots)
        self.assertIn("OpenImageIO", source)
        self.assertIn("median_reduce", source)
        self.assertIn("project_cells_to_reference_palette", source)
        self.assertNotIn("np.sin", source)
        self.assertNotIn("np.cos", source)

    def test_profile_requires_native_direction_and_variant_attributes(self):
        contract = self.profile["coordinate_contract"]
        self.assertEqual(contract["uv_attribute"], "UVMap")
        self.assertEqual(
            contract["longitudinal_attribute"],
            "sinc_length_ratio",
        )
        self.assertEqual(contract["variant_attribute"], "sinc_seed")
        self.assertEqual(
            contract["end_grain_attribute"],
            "sinc_wood_end_mask",
        )

    def test_builder_uses_only_two_colour_texture_lanes(self):
        source = BUILDER_PATH.read_text()
        self.assertNotIn("wood_plank_v2", source)
        self.assertNotIn("sinc_wood_damage_mask", source)
        self.assertNotIn("sinc_wood_edge_mask", source)
        syntax = ast.parse(source)
        created_node_types = [
            call.args[1].value
            for call in ast.walk(syntax)
            if isinstance(call, ast.Call)
            and isinstance(call.func, ast.Name)
            and call.func.id == "add_node"
            and len(call.args) >= 2
            and isinstance(call.args[1], ast.Constant)
            and isinstance(call.args[1].value, str)
        ]
        self.assertEqual(
            created_node_types.count("ShaderNodeTexImage"),
            2,
        )
        self.assertTrue(
            {
                "ShaderNodeBump",
                "ShaderNodeNormalMap",
                "ShaderNodeBsdfPrincipled",
            }.isdisjoint(created_node_types)
        )


if __name__ == "__main__":
    unittest.main()
