from __future__ import annotations

import hashlib
import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
PROFILE_PATH = (
    PACKAGE_ROOT
    / "profiles"
    / "openwork_strap_hinge_55_61_58_decomposition_v1.json"
)
DOSSIER_PATH = (
    PACKAGE_ROOT
    / "references"
    / "hinges"
    / "OPENWORK_STRAP_HINGE_55_61_58_DOSSIER.md"
)
OVERLAY_PATH = (
    PACKAGE_ROOT
    / "references"
    / "hinges"
    / "met_55_61_58_decomposition_overlay.svg"
)


class OpenworkStrapHingeDecompositionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.profile = json.loads(PROFILE_PATH.read_text())

    def test_catalogued_measurement_belongs_to_the_complete_hinged_object(self):
        source = self.profile["reference_authority"]["catalogued_object"]
        self.assertEqual(source["institution"], "The Metropolitan Museum of Art")
        self.assertEqual(source["object_number"], "55.61.58")
        self.assertEqual(
            source["catalogued_overall_m"],
            {"length": 0.406, "width": 0.041},
        )
        self.assertEqual(source["catalogued_scope"], "complete_hinged_object")

        scale = self.profile["scale_contract"]
        self.assertEqual(
            scale["catalogued_complete_hinge_giant_m"],
            {"length": 4.06, "width": 0.41},
        )
        moving = scale["paired_photo_measurement"]["moving_leaf_giant_m"]
        self.assertLess(moving["maximum"], 4.06)
        self.assertIn(
            "must never again be assigned to the moving leaf alone",
            scale["hard_rule"],
        )

    def test_public_domain_reference_files_match_frozen_hashes(self):
        images = self.profile["reference_authority"]["images"]
        for image in images.values():
            path = PACKAGE_ROOT / image["file"]
            self.assertTrue(path.is_file(), path)
            self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), image["sha256"])

    def test_pattern_is_five_connected_cells_not_a_sine_weave(self):
        anatomy = self.profile["anatomy"]["moving_leaf"]
        self.assertEqual(anatomy["side_rails"], 2)
        self.assertEqual(anatomy["full_pattern_cells"], 5)
        self.assertEqual(
            self.profile["cell_grammar"]["full_cell"]["count"],
            5,
        )
        self.assertEqual(
            self.profile["cell_grammar"]["inter_cell_junction"]["count"],
            4,
        )

        construction = self.profile["construction_read"]
        self.assertEqual(
            construction["primary_hypothesis"]["confidence"],
            "high",
        )
        rejected = " ".join(
            item["value"] for item in construction["rejected_hypotheses"]
        )
        self.assertIn("sinusoidal", rejected)
        self.assertIn("full-width rolled eye", rejected)

    def test_one_authored_quarter_derives_five_named_shapes(self):
        library = self.profile["quarter_shape_library"]
        self.assertEqual(library["unique_authored_curve_count"], 1)
        self.assertEqual(library["derived_shape_count"], 5)
        self.assertEqual(
            library["canonical_quarter"]["authored_fraction"],
            "one_quarter",
        )
        self.assertEqual(
            library["canonical_quarter"]["completion_transforms"],
            ["mirror_u_about_0.5", "mirror_v_about_0.0"],
        )
        self.assertEqual(
            set(library["derived_shapes"]),
            {
                "S1_central_lancet_aperture",
                "S2_flank_leaf_aperture",
                "S3_junction_petal_aperture",
                "S4_rail_lune_aperture",
                "S5_terminal_diamond",
            },
        )
        for shape in library["derived_shapes"].values():
            self.assertEqual(shape["source"], "Q0_leaf_quarter")
            self.assertIn("role", shape)

    def test_negative_space_inventory_is_explicit(self):
        cell = self.profile["cell_grammar"]["full_cell"]
        self.assertEqual(
            cell["negative_space_inventory"],
            {"central_lancet": 1, "flank_leaves": 2},
        )
        junction = self.profile["cell_grammar"]["inter_cell_junction"]
        self.assertEqual(
            junction["negative_space_inventory"],
            {"diagonal_petals": 4, "rail_lunes": 2},
        )
        self.assertIn("materially wider", junction["construction_rule"])

    def test_leaf_section_changes_width_bevel_height_crown_and_flat_land(self):
        stations = self.profile["section_grammar"]["pattern_web"]["stations"]
        self.assertEqual(
            [station["name"] for station in stations],
            ["root_node", "throat", "shoulder", "belly", "return", "tip_node"],
        )
        lanes = (
            "width_over_nominal",
            "bevel_width_over_local_width",
            "bevel_rise_over_stock_thickness",
            "crown_rise_over_stock_thickness",
            "flat_land_fraction",
        )
        for lane in lanes:
            values = [station[lane] for station in stations]
            self.assertGreater(len(set(values)), 1, lane)

        belly = next(item for item in stations if item["name"] == "belly")
        root = next(item for item in stations if item["name"] == "root_node")
        self.assertLess(belly["width_over_nominal"], root["width_over_nominal"])
        self.assertGreater(
            belly["bevel_width_over_local_width"],
            root["bevel_width_over_local_width"],
        )
        self.assertLess(belly["flat_land_fraction"], root["flat_land_fraction"])

    def test_unmeasured_depth_and_manufacturing_method_stay_provisional(self):
        policy = self.profile["reference_authority"]["evidence_policy"]
        self.assertIn("stock thickness", policy["not_observable_and_not_claimed"])
        self.assertIn("exact bevel height", policy["not_observable_and_not_claimed"])
        self.assertEqual(
            self.profile["construction_read"]["manufacturing_method_status"],
            "unresolved",
        )
        depth = self.profile["section_grammar"]["depth_unknowns"]
        self.assertEqual(depth["rear_face"], "keep planar in the first clay proof")
        self.assertEqual(depth["woven_crossings"], "disabled")

    def test_existing_baseline_has_seven_specific_hard_invalidations(self):
        audit = self.profile["baseline_audit"]
        self.assertEqual(audit["runtime_object_count"], 8)
        self.assertEqual(audit["runtime_vertices"], 3024)
        self.assertEqual(audit["runtime_polygons"], 3120)
        self.assertEqual(
            audit["runtime_union_extent_m"],
            {"x": 4.20222, "y": 0.283323, "z": 0.41},
        )
        invalidations = audit["hard_invalidations"]
        self.assertEqual(
            [item["id"] for item in invalidations],
            [
                "B01_scale_scope",
                "B02_pattern_topology",
                "B03_false_weave",
                "B04_constant_section",
                "B05_wrong_pivot",
                "B06_wrong_terminal",
                "B07_generic_end_pads",
            ],
        )
        for item in invalidations:
            self.assertTrue(item["finding"])
            self.assertTrue(item["required_correction"])

    def test_geometry_and_neutral_clay_proof_are_authorized(self):
        contract = self.profile["implementation_contract"]
        self.assertTrue(contract["geometry_build_authorized"])
        self.assertTrue(contract["proof_render_authorized"])
        self.assertIn("neutral-clay", contract["next_script_gate"])
        self.assertIn("forged-iron material remains blocked", contract["next_script_gate"])
        render = contract["proof_render_contract"]
        self.assertEqual(
            render["views"],
            [
                "reference_front",
                "oblique_depth",
                "cell_macro",
                "pivot_macro",
                "rear_construction",
            ],
        )
        self.assertEqual(render["material"], "neutral_clay_only")
        self.assertFalse(render["geometry_mutation_authorized"])
        self.assertFalse(render["forged_iron_material_authorized"])
        self.assertFalse(render["surface_detail_authorized"])
        self.assertIn(
            "rendering before script review",
            contract["prohibited_shortcuts"],
        )
        self.assertIn("damage", contract["prohibited_shortcuts"])
        self.assertIn("roughness used to hide weak form", contract["prohibited_shortcuts"])

    def test_dossier_and_overlay_expose_the_same_decisions(self):
        dossier = DOSSIER_PATH.read_text()
        self.assertIn("five full botanical cells", dossier)
        self.assertIn("Negative space is a first-class target", dossier)
        self.assertIn("root node", dossier)
        self.assertIn("bevel shoulder", dossier)
        self.assertIn("aperture lip", dossier)
        self.assertIn("no render before the user sees the script", dossier)

        overlay = OVERLAY_PATH.read_text()
        self.assertIn("met_55_61_58_additional_original.jpg", overlay)
        self.assertIn("Author one quarter curve", overlay)
        self.assertIn("Validate the empty spaces", overlay)
        self.assertIn("Draw the leaf with section changes", overlay)
        self.assertIn("full catalogue length assigned to moving leaf", overlay)


if __name__ == "__main__":
    unittest.main()
