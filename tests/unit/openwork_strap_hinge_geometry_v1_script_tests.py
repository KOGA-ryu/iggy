from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
SCRIPT_PATH = PACKAGE_ROOT / "build_openwork_strap_hinge_geometry_v1.py"
PROFILE_PATH = (
    PACKAGE_ROOT
    / "profiles"
    / "openwork_strap_hinge_55_61_58_decomposition_v1.json"
)
OUTPUT_PATH = PACKAGE_ROOT / "output" / "openwork_strap_hinge_geometry_v1.blend"

SPEC = importlib.util.spec_from_file_location("openwork_hinge_builder", SCRIPT_PATH)
assert SPEC is not None and SPEC.loader is not None
hinge = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = hinge
SPEC.loader.exec_module(hinge)


class OpenworkStrapHingeGeometryScriptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.profile = hinge.load_profile()
        cls.plan = hinge.build_hinge_plan(cls.profile)

    def test_complete_catalogue_length_is_split_between_two_leaf_owners(self):
        spec = self.plan.spec
        self.assertEqual(spec.complete_length_m, 4.06)
        self.assertEqual(spec.complete_width_m, 0.41)
        self.assertEqual(spec.fixed_leaf_length_m, 0.90)
        self.assertEqual(spec.moving_leaf_length_m, 3.16)
        self.assertAlmostEqual(
            spec.fixed_leaf_length_m + spec.moving_leaf_length_m,
            spec.complete_length_m,
        )
        self.assertLess(spec.moving_leaf_length_m, spec.complete_length_m)
        self.assertEqual(self.plan.fixed_plate.owner_id, hinge.OWNER_IDS["fixed_leaf"])
        self.assertEqual(
            self.plan.moving_plate.owner_id,
            hinge.OWNER_IDS["moving_leaf"],
        )

    def test_one_cubic_quarter_completes_a_closed_leaf_without_freehand_siblings(self):
        vertices = hinge.canonical_leaf_vertices(self.profile)
        self.assertGreaterEqual(len(vertices), 60)
        self.assertGreater(hinge.polygon_signed_area([item.point for item in vertices]), 0)

        library = self.profile["quarter_shape_library"]
        self.assertEqual(library["unique_authored_curve_count"], 1)
        for shape in library["derived_shapes"].values():
            self.assertEqual(shape["source"], "Q0_leaf_quarter")

    def test_five_cells_and_four_junctions_have_exact_aperture_inventory(self):
        self.assertEqual(self.plan.spec.cell_count, 5)
        self.assertEqual(self.plan.spec.pattern_end_m, 2.465)
        self.assertEqual(
            self.plan.shape_counts,
            {
                "S1_central_lancet_aperture": 5,
                "S2_flank_leaf_aperture": 10,
                "S3_junction_petal_aperture": 16,
                "S4_rail_lune_aperture": 8,
                "fastener_hole": 5,
                "terminal_bore": 1,
            },
        )
        self.assertEqual(len(self.plan.moving_plate.apertures), 45)
        self.assertEqual(len(self.plan.fixed_plate.apertures), 2)

    def test_apertures_remain_discrete_and_inside_the_connected_plate(self):
        self.assertGreaterEqual(self.plan.minimum_aperture_gap_m, 0.025)
        moving_outer = [item.point for item in self.plan.moving_plate.outer]
        for aperture in self.plan.moving_plate.apertures:
            for vertex in aperture.vertices:
                self.assertTrue(
                    hinge.point_in_polygon(vertex.point, moving_outer),
                    aperture.name,
                )

    def test_section_schedule_changes_every_leaf_defining_lane(self):
        root = hinge.interpolate_section(self.profile, 0.0)
        throat = hinge.interpolate_section(self.profile, 0.15)
        belly = hinge.interpolate_section(self.profile, 0.52)
        tip = hinge.interpolate_section(self.profile, 1.0)

        self.assertGreater(root.width_over_nominal, throat.width_over_nominal)
        self.assertLess(belly.width_over_nominal, root.width_over_nominal)
        self.assertGreater(
            belly.bevel_width_over_local_width,
            root.bevel_width_over_local_width,
        )
        self.assertGreater(
            belly.bevel_rise_over_stock_thickness,
            root.bevel_rise_over_stock_thickness,
        )
        self.assertGreater(
            belly.crown_rise_over_stock_thickness,
            root.crown_rise_over_stock_thickness,
        )
        self.assertLess(belly.flat_land_fraction, root.flat_land_fraction)
        self.assertNotEqual(tip.width_over_nominal, belly.width_over_nominal)

    def test_front_land_is_planar_and_crown_is_localized_in_a_separate_ring(self):
        source = SCRIPT_PATH.read_text()
        compact = "".join(source.split())
        self.assertIn("aperture_crown_rings", source)
        self.assertIn(
            "front_depths.extend("
            "[spec.stock_thickness_m*0.5]*len(shoulder)"
            ")",
            compact,
        )
        self.assertNotIn(
            "front_depths.extend(reversed(shoulder_depths))",
            source,
        )
        self.assertIn("The broad front land stays planar", source)

    def test_alternating_knuckles_have_three_moving_and_two_fixed_owners(self):
        schedule = self.plan.knuckles
        self.assertEqual(len(schedule), 5)
        self.assertEqual(
            [item.owner_id for item in schedule],
            [
                hinge.OWNER_IDS["moving_leaf"],
                hinge.OWNER_IDS["fixed_leaf"],
                hinge.OWNER_IDS["moving_leaf"],
                hinge.OWNER_IDS["fixed_leaf"],
                hinge.OWNER_IDS["moving_leaf"],
            ],
        )
        for first, second in zip(schedule, schedule[1:]):
            self.assertAlmostEqual(
                second.cross_min_m - first.cross_max_m,
                self.plan.spec.knuckle_gap_m,
            )
        self.assertGreaterEqual(
            self.plan.spec.knuckle_connection_overlap_m,
            0.005,
        )
        for segment in schedule:
            seam_x = abs(
                segment.outer_radius_m
                * hinge.math.cos(
                    hinge.math.radians(segment.seam_angle_start_degrees)
                )
            )
            self.assertGreater(seam_x, self.plan.spec.moving_body_start_m)

    def test_each_open_seam_knuckle_plan_is_closed_and_manifold(self):
        for segment in self.plan.knuckles:
            mesh = hinge.compile_knuckle_mesh(segment)
            self.assertGreaterEqual(len(mesh.vertices), 292)
            report = hinge.topology_report(mesh)
            self.assertEqual(report["boundary_edges"], 0, segment.name)
            self.assertEqual(report["non_manifold_edges"], 0, segment.name)
            self.assertGreater(report["signed_volume_m3"], 0.0, segment.name)

    def test_pintle_is_a_separate_clearance_controlled_solid(self):
        spec = self.plan.spec
        self.assertEqual(spec.pintle_radius_m, 0.026)
        self.assertEqual(spec.pintle_length_m, 0.41)
        self.assertAlmostEqual(
            spec.knuckle_inner_radius_m - spec.pintle_radius_m,
            0.004,
        )
        mesh = hinge.compile_pintle_mesh(spec)
        report = hinge.topology_report(mesh)
        self.assertEqual(mesh.name, "SM_GH018_Pintle")
        self.assertEqual(report["boundary_edges"], 0)
        self.assertEqual(report["non_manifold_edges"], 0)
        self.assertGreater(report["signed_volume_m3"], 0.0)

    def test_script_declares_all_semantic_runtime_lanes(self):
        source = SCRIPT_PATH.read_text()
        required = self.profile["implementation_contract"][
            "required_semantic_attributes"
        ]
        for attribute in required:
            self.assertIn(attribute, source)
        self.assertIn("sinc_geometry_owner", source)
        self.assertIn("SOCKET_GH018_PintleAxis", source)
        self.assertIn("SOCKET_GH018_Tail", source)

    def test_script_contains_no_old_pattern_or_surface_shortcuts(self):
        source = SCRIPT_PATH.read_text()
        for forbidden in (
            "ribbon_path",
            "crossing_depth_m",
            'nodes.new("ShaderNodeTexNoise")',
            'nodes.new("ShaderNodeTexVoronoi")',
            "random.",
            "np.random",
            "bpy.ops.render",
            "bpy_module.ops.render",
            "camera_add",
            "light_add",
        ):
            self.assertNotIn(forbidden, source)

    def test_build_requires_profile_and_cli_approval_while_plan_writes_no_blend(self):
        self.assertTrue(
            self.profile["implementation_contract"]["geometry_build_authorized"]
        )
        locked_profile = json.loads(json.dumps(self.profile))
        locked_profile["implementation_contract"]["geometry_build_authorized"] = False
        with self.assertRaises(PermissionError):
            hinge.require_build_authorization(
                locked_profile,
                build_requested=True,
            )
        hinge.require_build_authorization(
            self.profile,
            build_requested=True,
        )
        hinge.require_build_authorization(
            locked_profile,
            build_requested=False,
        )

        before = OUTPUT_PATH.exists()
        completed = subprocess.run(
            [sys.executable, str(SCRIPT_PATH), "--plan-only"],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        summary = json.loads(completed.stdout)
        self.assertTrue(summary["build_authorized"])
        self.assertFalse(summary["proof_media_authored"])
        self.assertEqual(OUTPUT_PATH.exists(), before)

    def test_plan_summary_exposes_scale_pattern_and_component_ownership(self):
        summary = hinge.plan_summary(self.plan)
        self.assertEqual(
            summary["catalogued_complete_envelope_m"],
            {"length": 4.06, "width": 0.41},
        )
        self.assertEqual(summary["pattern"]["cell_count"], 5)
        self.assertEqual(summary["pattern"]["junction_count"], 4)
        self.assertEqual(summary["components"]["knuckles"]["moving_owned"], 3)
        self.assertEqual(summary["components"]["knuckles"]["fixed_owned"], 2)
        self.assertTrue(summary["build_authorized"])


if __name__ == "__main__":
    unittest.main()
