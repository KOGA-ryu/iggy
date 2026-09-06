from __future__ import annotations

import hashlib
import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
BLEND_PATH = PACKAGE_ROOT / "output" / "openwork_strap_hinge_geometry_v1.blend"
MANIFEST_PATH = (
    PACKAGE_ROOT
    / "output"
    / "openwork_strap_hinge_geometry_v1_manifest.json"
)
PROFILE_PATH = (
    PACKAGE_ROOT
    / "profiles"
    / "openwork_strap_hinge_55_61_58_decomposition_v1.json"
)
BUILDER_PATH = PACKAGE_ROOT / "build_openwork_strap_hinge_geometry_v1.py"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class OpenworkStrapHingeBlendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST_PATH.read_text())

    def test_generated_file_and_manifest_have_current_source_lineage(self):
        self.assertTrue(BLEND_PATH.is_file())
        self.assertGreater(BLEND_PATH.stat().st_size, 100_000)
        self.assertEqual(
            self.manifest["schema"],
            "iggy-openwork-strap-hinge-geometry-manifest/1.0",
        )
        hashes = self.manifest["source_hashes"]
        self.assertEqual(hashes["blend_sha256"], sha256(BLEND_PATH))
        self.assertEqual(hashes["profile_sha256"], sha256(PROFILE_PATH))
        self.assertEqual(hashes["builder_sha256"], sha256(BUILDER_PATH))

    def test_saved_file_reopened_at_exact_catalogued_giant_envelope(self):
        validation = self.manifest["saved_file_validation"]
        self.assertTrue(validation["reopen_validated"])
        self.assertEqual(
            validation["world_bounds_m"]["extent"],
            [4.06, 0.116, 0.41],
        )
        self.assertEqual(
            validation["world_bounds_m"]["minimum"],
            [-0.9, -0.058, -0.205],
        )
        self.assertEqual(
            validation["world_bounds_m"]["maximum"],
            [3.16, 0.058, 0.205],
        )

    def test_moving_and_fixed_plate_genus_prove_true_through_holes(self):
        validation = self.manifest["saved_file_validation"]
        reports = validation["mesh_reports"]
        moving = reports["SM_GH018_MovingLeaf_Openwork"]
        fixed = reports["SM_GH018_FixedLeaf"]
        self.assertEqual(moving["genus_total"], 45)
        self.assertEqual(moving["euler_characteristic"], -88)
        self.assertEqual(moving["components"], 1)
        self.assertEqual(moving["boundary_edges"], 0)
        self.assertEqual(moving["non_manifold_edges"], 0)
        self.assertGreater(moving["semantic_face_counts"]["sinc_aperture_wall"], 0)

        self.assertEqual(fixed["genus_total"], 2)
        self.assertEqual(fixed["euler_characteristic"], -2)
        self.assertEqual(fixed["components"], 1)
        self.assertEqual(fixed["boundary_edges"], 0)
        self.assertEqual(fixed["non_manifold_edges"], 0)

    def test_five_knuckles_reopen_as_closed_open_seam_solids(self):
        reports = self.manifest["saved_file_validation"]["mesh_reports"]
        names = [
            "MovingKnuckle_01",
            "FixedKnuckle_02",
            "MovingKnuckle_03",
            "FixedKnuckle_04",
            "MovingKnuckle_05",
        ]
        for name in names:
            report = reports[name]
            self.assertEqual(report["components"], 1)
            self.assertEqual(report["genus_total"], 0)
            self.assertEqual(report["boundary_edges"], 0)
            self.assertEqual(report["non_manifold_edges"], 0)
            self.assertGreater(report["signed_volume_m3"], 0.0)
            self.assertEqual(report["modifiers"], [])

    def test_pintle_reopens_as_a_separate_closed_solid(self):
        report = self.manifest["saved_file_validation"]["mesh_reports"][
            "SM_GH018_Pintle"
        ]
        self.assertEqual(report["components"], 1)
        self.assertEqual(report["genus_total"], 0)
        self.assertEqual(report["boundary_edges"], 0)
        self.assertEqual(report["non_manifold_edges"], 0)
        self.assertEqual(report["dimensions_m"], [0.052, 0.052, 0.41])
        self.assertGreater(report["signed_volume_m3"], 0.0)
        self.assertEqual(report["modifiers"], [])

    def test_every_runtime_mesh_preserves_all_semantic_lanes(self):
        required = {
            "sinc_hinge_zone",
            "sinc_cell_index",
            "sinc_shape_id",
            "sinc_web_station",
            "sinc_bevel_land",
            "sinc_aperture_wall",
            "sinc_front_face",
            "sinc_rear_face",
            "sinc_interface_id",
        }
        reports = self.manifest["saved_file_validation"]["mesh_reports"]
        for name, report in reports.items():
            self.assertTrue(required.issubset(report["attributes"]), name)

    def test_geometry_checkpoint_contains_no_presentation_or_texture_route(self):
        validation = self.manifest["saved_file_validation"]
        self.assertEqual(validation["camera_or_light_objects"], [])
        self.assertFalse(validation["proof_media_authored"])
        self.assertTrue(validation["render_authorized"])
        self.assertEqual(
            validation["material_nodes"],
            ["ShaderNodeBsdfPrincipled", "ShaderNodeOutputMaterial"],
        )
        self.assertEqual(
            validation["datum_objects"],
            ["SOCKET_GH018_PintleAxis", "SOCKET_GH018_Tail"],
        )

    def test_plan_records_real_knuckle_overlap_and_safe_aperture_web(self):
        plan = self.manifest["plan"]
        self.assertGreaterEqual(
            plan["components"]["knuckles"]["leaf_connection_overlap_m"],
            0.005,
        )
        self.assertGreaterEqual(
            plan["pattern"]["minimum_aperture_gap_m"],
            0.025,
        )
        self.assertEqual(plan["pattern"]["cell_count"], 5)
        self.assertEqual(plan["pattern"]["junction_count"], 4)


if __name__ == "__main__":
    unittest.main()
