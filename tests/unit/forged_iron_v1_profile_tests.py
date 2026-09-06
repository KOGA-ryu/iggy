import ast
import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
)
PROFILE_PATH = MATERIAL_ROOT / "profiles" / "forged_iron_v1.json"
SAMPLES_PATH = (
    MATERIAL_ROOT
    / "references"
    / "met_468492_front_iron_palette_samples.json"
)
BUILDER_PATH = MATERIAL_ROOT / "build_forged_iron_v1.py"


class ForgedIronProfileTests(unittest.TestCase):
    def setUp(self):
        self.profile = json.loads(PROFILE_PATH.read_text())
        self.sample_document = json.loads(SAMPLES_PATH.read_text())
        self.samples = self.sample_document[
            "eyedropper_samples"
        ]["samples"]

    def test_twenty_palette_anchors_are_exact_eyedropper_samples(self):
        sampled_hex = [sample["hex"] for sample in self.samples]
        palette = self.profile["palette"][
            "observed_reference_twenty"
        ]
        self.assertEqual(len(palette), 20)
        self.assertEqual(len(set(palette)), 20)
        self.assertEqual(palette, sampled_hex)

    def test_reference_samples_separate_iron_from_oxide_and_shadow(self):
        by_role = {}
        for sample in self.samples:
            by_role.setdefault(sample["material_role"], []).append(sample)

        self.assertEqual(len(by_role["base_hue_only"]), 11)
        self.assertEqual(len(by_role["oxidation_overlay_only"]), 3)
        self.assertEqual(len(by_role["excluded"]), 6)
        self.assertTrue(
            all(
                sample["surface_class"] == "iron_face"
                for sample in by_role["base_hue_only"]
            )
        )
        self.assertTrue(
            all(
                sample["surface_class"] == "oxidized_iron"
                for sample in by_role["oxidation_overlay_only"]
            )
        )
        excluded_classes = {
            sample["surface_class"] for sample in by_role["excluded"]
        }
        self.assertTrue(
            {
                "shadowed_iron",
                "scroll_shadow_ambiguous",
                "material_ambiguous",
            }.issubset(excluded_classes)
        )
        self.assertEqual(
            self.sample_document["eyedropper_samples"]["audit"][
                "base_hue_sample_count"
            ],
            11,
        )
        self.assertFalse(
            self.sample_document["eyedropper_samples"]["audit"][
                "photograph_values_are_pbr_base_colour"
            ]
        )

    def test_reference_authority_matches_the_approved_door(self):
        authority = self.profile["reference_authority"]
        self.assertEqual(
            authority["primary_object"]["object_number"],
            "55.61.170",
        )
        self.assertEqual(
            authority["primary_object"]["view"],
            "front",
        )
        self.assertEqual(
            authority["secondary_object"]["object_number"],
            "25.120.291",
        )
        self.assertIn(
            "active corrosion",
            authority["conservation"]["rule"],
        )
        measured = authority["measured_stock"]
        self.assertEqual(
            measured["met_55_61_63"]["dimensions_m"],
            {"length": 0.349, "width": 0.038},
        )
        self.assertEqual(
            measured["met_55_61_58"]["dimensions_m"],
            {"length": 0.406, "width": 0.041},
        )
        hammer = authority["measured_forging_tool"]
        self.assertEqual(
            hammer["main_face_m"],
            [0.041275, 0.03175],
        )
        self.assertEqual(
            hammer["cross_peen_face_m"],
            [0.0365125, 0.0127],
        )
        self.assertIn("maximum footprint", hammer["use"])

    def test_shader_uses_metre_scale_and_authored_semantic_masks(self):
        coordinates = self.profile["coordinate_contract"]
        self.assertEqual(coordinates["space"], "component_local")
        self.assertEqual(coordinates["unit"], "metre")
        self.assertEqual(
            coordinates["variant_attribute"],
            "sinc_seed",
        )
        self.assertEqual(
            coordinates["edge_attribute"],
            "sinc_iron_edge_mask",
        )
        self.assertEqual(
            coordinates["longitudinal_attribute"],
            "sinc_iron_u_m",
        )
        self.assertEqual(
            coordinates["cross_stock_attribute"],
            "sinc_iron_v_m",
        )
        self.assertEqual(
            coordinates["fabrication_scale_attribute"],
            "sinc_iron_fabrication_scale",
        )
        self.assertTrue(coordinates["uv_required"])
        self.assertEqual(coordinates["tangent_uv"], "IGGY_IronUV")
        self.assertEqual(
            coordinates["fabrication_scale_presets"],
            {"human_fixture": 1.0, "giant_house_hardware": 10.0},
        )
        self.assertEqual(
            self.profile["pattern"]["physical_tile_m"],
            {"length": 1.218, "width": 0.164},
        )
        self.assertEqual(
            self.profile["pattern"]["worked_physical_tile_m"],
            {"length": 2.436, "width": 0.492},
        )
        self.assertEqual(self.profile["pattern"]["variation_count"], 4)

    def test_pbr_contract_separates_oxide_from_exposed_iron(self):
        response = self.profile["surface_response"]
        self.assertEqual(
            response["model"],
            "intact_compact_scale_with_default_off_contact_exposed_iron",
        )
        self.assertEqual(response["forge_skin_metallic"], 0.0)
        self.assertEqual(response["exposed_iron_metallic"], 1.0)
        self.assertEqual(
            response["bare_iron_base_colour_linear"],
            [0.56, 0.57, 0.58],
        )
        self.assertEqual(response["roughness_range"], [0.34, 0.82])
        self.assertEqual(
            response["surface_roughness_proxy"]["ra_m"],
            0.0000064,
        )
        self.assertEqual(
            response["surface_roughness_proxy"]["status"],
            "measured_modern_hot_forged_proxy_not_medieval_metrology",
        )
        self.assertLessEqual(
            response["height_encoding"]["maximum_m"]
            - response["height_encoding"]["minimum_m"],
            0.00036,
        )
        self.assertEqual(
            set(response["layer_height_encoding"]),
            {"macro_hammer", "scale_lip", "micro_surface"},
        )
        self.assertEqual(
            response["layer_height_encoding"]["macro_hammer"],
            {
                "minimum_m": -0.000085,
                "maximum_m": 0.000085,
                "neutral": 0.5,
            },
        )
        self.assertEqual(
            response["distance_response_m"]["micro_full_until"],
            0.65,
        )
        self.assertEqual(
            response["distance_response_m"]["micro_zero_after"],
            4.5,
        )
        self.assertEqual(
            set(response["normal_strengths"]),
            {
                "macro_hammer",
                "scale_lip",
                "micro_surface",
                "worked_surface",
                "rule",
            },
        )
        self.assertIn("Hammering never implies", response["metallic_rule"])
        self.assertEqual(response["default_metalness"], 0.0)
        self.assertEqual(response["worked_anisotropy_max"], 0.18)

    def test_damage_corrosion_and_contact_polish_are_default_off(self):
        excluded = set(self.profile["excluded_lanes"])
        self.assertTrue(
            {
                "active_orange_corrosion",
                "uniform_rust",
                "random_pitting",
                "deep_pits",
                "cracks",
                "chips",
                "scratches",
                "damage",
            }.issubset(excluded)
        )
        layers = {
            layer["id"]: layer for layer in self.profile["layers"]
        }
        self.assertEqual(
            set(layers),
            {
                "quiet_compact_forge_scale",
                "compact_scale_plate_tone",
                "finite_planishing_faces",
                "finite_cross_peen_faces",
                "micro_surface",
                "worked_surface",
                "brown_oxidation_overlay",
                "contact_polish_overlay",
            },
        )
        self.assertEqual(layers["brown_oxidation_overlay"]["amount"], 0.0)
        self.assertEqual(layers["contact_polish_overlay"]["amount"], 0.0)
        self.assertEqual(layers["worked_surface"]["height_claim_m"], 0.0)
        self.assertGreaterEqual(
            layers["quiet_compact_forge_scale"]["minimum_quiet_fraction"],
            0.35,
        )
        self.assertEqual(
            layers["quiet_compact_forge_scale"]["metallic"],
            0.0,
        )
        self.assertLessEqual(
            layers["compact_scale_plate_tone"]["maximum_tone_influence"],
            0.28,
        )

    def test_builder_has_layered_physical_responses_and_authored_maps(self):
        source = BUILDER_PATH.read_text()
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
            created_node_types.count("ShaderNodeBsdfPrincipled"),
            2,
        )
        self.assertEqual(
            created_node_types.count("ShaderNodeBump"),
            0,
        )
        self.assertEqual(
            created_node_types.count("ShaderNodeNormalMap"),
            1,
        )
        self.assertEqual(
            created_node_types.count("ShaderNodeTexImage"),
            1,
        )
        self.assertEqual(
            created_node_types.count("ShaderNodeDisplacement"),
            1,
        )
        self.assertEqual(
            created_node_types.count("ShaderNodeTexNoise"),
            0,
        )
        self.assertIn("sinc_seed", source)
        self.assertIn("sinc_iron_edge_mask", source)
        self.assertIn("sinc_iron_u_m", source)
        self.assertIn("sinc_iron_v_m", source)
        self.assertIn("sinc_iron_fabrication_scale", source)
        self.assertIn("IGGY_IronUV", source)
        self.assertIn("ShaderNodeTangent", source)
        self.assertIn("approved_wood_unchanged", source)
        self.assertNotIn("add_noise(", source)
        for group_name in (
            "IGGY_SH_IronCoordinates_v004",
            "IGGY_SH_AuthoredIronLanes_v004",
            "IGGY_SH_ForgeScaleLayer_v002",
            "IGGY_SH_HammerPlanes_v003",
            "IGGY_SH_IronMicroSurface_v002",
            "IGGY_SH_NormalCombine_v003",
            "IGGY_SH_SurfaceHeight_v003",
            "IGGY_SH_WorkedIronSurface_v001",
        ):
            self.assertIn(group_name, source)
        self.assertIn(
            "Physical_Oxide_And_Optional_Contact_Iron",
            source,
        )
        self.assertIn("Whiteout_Four_Frequency_Normal", source)
        self.assertIn("Physical_Metre_Displacement", source)
        self.assertIn(
            "obj.matrix_world @ Vector(corner)",
            source,
        )
        self.assertIn("target_span_x * 1.15", source)

    def test_builder_is_deterministic_and_preserves_approved_wood(self):
        source = BUILDER_PATH.read_text()
        syntax = ast.parse(source)
        imported_roots = {
            alias.name.split(".")[0]
            for node in ast.walk(syntax)
            if isinstance(node, (ast.Import, ast.ImportFrom))
            for alias in node.names
        }
        self.assertNotIn("random", imported_roots)
        self.assertIn(
            "structural_oak_door_v1",
            str(BUILDER_PATH.read_text()),
        )
        self.assertIn("wood_before", source)
        self.assertIn(
            "node_contract(wood.node_tree) != wood_before",
            source,
        )
        self.assertIn(
            'raise RuntimeError("Approved structural-oak material was modified")',
            source,
        )
        self.assertIn("generate_forged_iron_patterns_v1.py", source)
        self.assertIn("measured_reference_strap", source)


if __name__ == "__main__":
    unittest.main()
