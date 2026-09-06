from __future__ import annotations

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
PROFILE_PATH = PACKAGE_ROOT / "profiles" / "forged_fasteners_v1.json"
BUILDER_PATH = PACKAGE_ROOT / "build_forged_fasteners_v1.py"


class ForgedFastenersProfileTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.profile = json.loads(PROFILE_PATH.read_text())

    def test_measured_medieval_door_nail_is_not_rescaled_or_guessed(self):
        source = self.profile["reference_authority"]["measured_door_nail"]
        self.assertEqual(source["institution"], "The Metropolitan Museum of Art")
        self.assertEqual(source["object_number"], "55.61.134")
        self.assertEqual(source["date"], "15th–16th century")
        self.assertEqual(
            source["overall_m"],
            {"width": 0.074, "depth": 0.075, "length": 0.156},
        )
        self.assertEqual(
            source["installed_m"],
            {"width": 0.074, "depth": 0.075, "projection": 0.017},
        )
        self.assertEqual(source["measurement_status"], "museum_catalogued")

    def test_rosehead_construction_follows_four_blow_square_rod_evidence(self):
        method = self.profile["reference_authority"]["rosehead_method"]
        self.assertEqual(method["institution"], "U.S. National Park Service")
        self.assertEqual(method["starting_section"], "square_rod")
        self.assertEqual(method["hammer_blow_count"], 4)
        self.assertEqual(method["result"], "shallow_pyramid")

        rose = self.profile["families"]["rosehead"]
        self.assertEqual(rose["shank_section"], "square_tapered")
        self.assertEqual(rose["dominant_planes"], 4)
        self.assertGreaterEqual(len(rose["axial_rings"]), 4)
        self.assertLess(rose["crown_plateau_ratio"], 0.35)

    def test_four_families_have_authored_topology_not_noise(self):
        families = self.profile["families"]
        self.assertEqual(
            set(families),
            {
                "rosehead",
                "faceted_square_peen",
                "domed_rivet",
                "flattened_peen",
            },
        )
        for family in families.values():
            self.assertGreaterEqual(len(family["axial_rings"]), 4)
            self.assertFalse(family["uses_noise_displacement"])
            self.assertIn(
                family["dimension_status"],
                {"museum_catalogued", "reference_bounded_authored"},
            )
            self.assertIn("quiet_face_fraction", family)

    def test_punched_seat_and_wood_seat_are_independent(self):
        seats = self.profile["seat_contract"]
        self.assertEqual(seats["wood"]["geometry"], "head_bearing_only")
        self.assertFalse(seats["wood"]["emit_iron_rim"])
        self.assertEqual(seats["iron_plate"]["geometry"], "punched_drifted_rim")
        self.assertTrue(seats["iron_plate"]["emit_iron_rim"])
        self.assertEqual(
            seats["iron_plate"]["measurement_status"],
            "historically_observed_proportion_authored",
        )

    def test_door_callers_are_role_specific(self):
        calls = self.profile["acceptance_door_calls"]
        self.assertEqual(set(calls), {"brace_nails", "hinge_rivets", "pull_plate_rivets"})
        self.assertEqual(
            calls["brace_nails"]["source_point_origin"],
            "legacy_shank_center",
        )
        self.assertEqual(
            calls["hinge_rivets"]["source_point_origin"],
            "bearing_plane",
        )
        self.assertEqual(
            calls["pull_plate_rivets"]["source_point_origin"],
            "bearing_plane",
        )
        self.assertTrue(calls["brace_nails"]["rose_head"])
        self.assertFalse(calls["brace_nails"]["punched_seat"])
        self.assertFalse(calls["hinge_rivets"]["rose_head"])
        self.assertTrue(calls["hinge_rivets"]["punched_seat"])
        self.assertTrue(calls["pull_plate_rivets"]["punched_seat"])
        for call in calls.values():
            self.assertGreater(call["head_radius_m"], call["shank_radius_m"])
            self.assertGreater(call["shank_length_m"], call["tip_length_m"])

    def test_builder_declares_reusable_node_and_semantic_contract(self):
        source = BUILDER_PATH.read_text()
        self.assertIn("IGGY_GN_ForgedFasteners_v001", source)
        self.assertIn("sinc_fastener_style", source)
        self.assertIn("sinc_fastener_surface_role", source)
        self.assertIn("sinc_iron_edge_mask", source)
        self.assertIn("Punched Seat", source)
        self.assertNotIn('nodes.new("ShaderNodeTexNoise")', source)
        self.assertNotIn('nodes.new("ShaderNodeTexVoronoi")', source)
        self.assertNotIn("random.", source)
        self.assertNotIn("np.random", source)


if __name__ == "__main__":
    unittest.main()
