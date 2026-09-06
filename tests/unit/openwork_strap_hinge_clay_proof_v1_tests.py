from __future__ import annotations

import hashlib
import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
SCRIPT_PATH = PACKAGE_ROOT / "render_openwork_strap_hinge_clay_proof_v1.py"
PROFILE_PATH = (
    PACKAGE_ROOT
    / "profiles"
    / "openwork_strap_hinge_55_61_58_decomposition_v1.json"
)
GEOMETRY_PATH = (
    PACKAGE_ROOT / "output" / "openwork_strap_hinge_geometry_v1.blend"
)
PROOF_BLEND_PATH = (
    PACKAGE_ROOT / "output" / "openwork_strap_hinge_clay_proof_v1.blend"
)
MANIFEST_PATH = (
    PACKAGE_ROOT / "output" / "openwork_strap_hinge_clay_proof_v1_manifest.json"
)
VIEW_NAMES = [
    "reference_front",
    "oblique_depth",
    "cell_macro",
    "pivot_macro",
    "rear_construction",
]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class OpenworkStrapHingeClayProofContractTests(unittest.TestCase):
    def test_proof_script_has_one_presentation_only_render_route(self):
        source = SCRIPT_PATH.read_text()
        self.assertIn("proof_render_authorized", source)
        self.assertIn("geometry_mutation_authorized", source)
        self.assertIn("bpy_module.ops.wm.open_mainfile", source)
        self.assertIn("bpy_module.ops.wm.save_as_mainfile", source)
        self.assertIn("bpy_module.ops.render.render", source)
        for view_name in VIEW_NAMES:
            self.assertIn(view_name, source)
        for forbidden in (
            "ShaderNodeTexImage",
            "ShaderNodeTexNoise",
            "ShaderNodeTexVoronoi",
            "Displace",
            "Subdivision",
            "forged_iron_v1_base_color",
            "random.",
            "np.random",
        ):
            self.assertNotIn(forbidden, source)

    def test_profile_authorizes_only_the_clay_geometry_proof(self):
        profile = json.loads(PROFILE_PATH.read_text())
        contract = profile["implementation_contract"]
        self.assertTrue(contract["proof_render_authorized"])
        proof = contract["proof_render_contract"]
        self.assertEqual(proof["views"], VIEW_NAMES)
        self.assertEqual(proof["material"], "neutral_clay_only")
        self.assertFalse(proof["geometry_mutation_authorized"])
        self.assertFalse(proof["forged_iron_material_authorized"])
        self.assertFalse(proof["surface_detail_authorized"])


class OpenworkStrapHingeClayProofOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads(MANIFEST_PATH.read_text())

    def test_proof_outputs_have_current_lineage(self):
        hashes = self.manifest["source_hashes"]
        self.assertEqual(hashes["profile_sha256"], sha256(PROFILE_PATH))
        self.assertEqual(hashes["geometry_sha256"], sha256(GEOMETRY_PATH))
        self.assertEqual(hashes["proof_builder_sha256"], sha256(SCRIPT_PATH))
        self.assertEqual(hashes["proof_blend_sha256"], sha256(PROOF_BLEND_PATH))

    def test_every_required_view_is_a_substantial_png(self):
        outputs = self.manifest["outputs"]
        self.assertEqual(sorted(outputs), sorted(VIEW_NAMES))
        self.assertEqual(outputs["reference_front"]["ortho_scale_m"], 4.55)
        self.assertEqual(outputs["rear_construction"]["ortho_scale_m"], 4.55)
        for view_name in VIEW_NAMES:
            path = Path(outputs[view_name]["path"])
            self.assertTrue(path.is_file(), view_name)
            self.assertGreater(path.stat().st_size, 40_000, view_name)
            self.assertEqual(
                outputs[view_name]["sha256"],
                sha256(path),
                view_name,
            )

    def test_saved_proof_scene_is_clay_only_and_preserves_geometry(self):
        validation = self.manifest["saved_proof_validation"]
        self.assertEqual(validation["runtime_mesh_count"], 8)
        self.assertIn("SM_GH018_Pintle", validation["runtime_objects"])
        self.assertEqual(validation["runtime_geometry_hash_before"], validation["runtime_geometry_hash_after"])
        self.assertEqual(validation["material_names"], ["M_GH018_NeutralClay"])
        self.assertEqual(validation["camera_count"], 1)
        self.assertEqual(validation["light_count"], 3)
        self.assertEqual(validation["proof_views"], VIEW_NAMES)
        self.assertFalse(validation["geometry_mutated"])
        self.assertFalse(validation["surface_detail_authored"])
        self.assertFalse(validation["forged_iron_material_authored"])


if __name__ == "__main__":
    unittest.main()
