from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
DEMANDS = PACKAGE / "CODED_DEMANDS.md"
BUILD_SCRIPT = PACKAGE / "build_connected_oxide_candidate_v004_2.py"
VERIFY_SCRIPT = PACKAGE / "verify_connected_oxide_candidate_v004_2.py"
RENDER_SCRIPT = PACKAGE / "render_connected_oxide_candidate_cycles_v004_2.py"
SOURCE_ROOT = PACKAGE / "output" / "forged_iron_connected_oxide_candidate_v004_1"
SOURCE = SOURCE_ROOT / "forged_iron_connected_oxide_candidate_v004_1.blend"
OUTPUT_ROOT = PACKAGE / "output" / "forged_iron_connected_oxide_candidate_v004_2"
CANDIDATE = OUTPUT_ROOT / "forged_iron_connected_oxide_candidate_v004_2.blend"
MANIFEST = OUTPUT_ROOT / "manifest.json"
REOPEN_CONTRACT = OUTPUT_ROOT / "reopen_contract.json"
PROOF_ROOT = OUTPUT_ROOT / "cycles_v004_1_v004_2_proof_v1"
PROOF_MANIFEST = PROOF_ROOT / "manifest.json"
BOARD = PROOF_ROOT / "forged_iron_v004_1_v004_2_cycles_comparison.png"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")
EXPECTED_TOPOLOGY = "ce47658c611f97e6b4032596b223a7016fb05bf1daf30646b89c72b04227523b"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as handle:
        header = handle.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(path)
    return struct.unpack(">II", header[16:24])


class ForgedIronV0042BlueprintTests(unittest.TestCase):
    def test_demand_precedes_three_executables(self):
        demands = DEMANDS.read_text()
        self.assertIn("DEM-PRODUCTION-014", demands)
        self.assertIn("Integrate selected clean-metal response as v004.2", demands)
        self.assertTrue(BUILD_SCRIPT.is_file())
        self.assertTrue(VERIFY_SCRIPT.is_file())
        self.assertTrue(RENDER_SCRIPT.is_file())

    def test_builder_freezes_the_one_permitted_response_change(self):
        source = BUILD_SCRIPT.read_text()
        for token in (
            'SOURCE_GROUP = "IGGY_SH_ConnectedOxideForgedIron_v004"',
            'TARGET_GROUP = "IGGY_SH_ConnectedOxideForgedIron_v004_2"',
            "DIRECTIONAL_RESPONSE_AMOUNT = 0.28",
            "DIRECTIONAL_RESPONSE_ROTATION = 0.50",
            "SOURCE_MULTIPLIER = 0.00",
            "TARGET_MULTIPLIER = 1.00",
            "group_default_differences",
            "frozen_lane_hashes_match_source",
            "save_as_mainfile",
        ):
            self.assertIn(token, source)
        self.assertNotIn("ShaderNodeTexNoise", source)
        self.assertNotIn("ShaderNodeTexVoronoi", source)

    def test_verifier_and_renderer_cannot_save_a_blend(self):
        verifier = VERIFY_SCRIPT.read_text()
        renderer = RENDER_SCRIPT.read_text()
        for source in (verifier, renderer):
            self.assertNotIn("save_as_mainfile", source)
            self.assertNotIn("save_mainfile", source)
        for panel in (
            "close_neutral",
            "grazing_left",
            "grazing_right",
            "pivot_close",
            "gameplay",
            "direction_amount",
            "source_normal",
        ):
            self.assertIn(panel, renderer)
        self.assertIn('scene.render.use_persistent_data = False', (PACKAGE / "compare_v004_1_reflection_routing_v1.py").read_text())


@unittest.skipUnless(MANIFEST.is_file(), "v004.2 has not been built")
class ForgedIronV0042ManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = json.loads(MANIFEST.read_text())

    def test_candidate_boundary_and_source_hash_hold(self):
        self.assertEqual(
            self.payload["status"],
            "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE",
        )
        self.assertTrue(self.payload["source_v004_1_unchanged"])
        self.assertEqual(
            self.payload["source_v004_1"]["blend_sha256_after"],
            sha256_file(SOURCE),
        )
        self.assertEqual(self.payload["candidate"]["blend_sha256"], sha256_file(CANDIDATE))
        self.assertFalse(self.payload["manual_acceptance_established"])
        self.assertFalse(self.payload["unreal_parity_verified"])
        self.assertFalse(self.payload["uses_ai_generated_imagery"])
        self.assertFalse(self.payload["uses_condition"])

    def test_group_topology_and_only_default_delta_are_exact(self):
        contract = self.payload["shared_group_contract"]
        self.assertEqual(contract["node_count"], 9)
        self.assertEqual(contract["link_count"], 20)
        self.assertEqual(contract["interface_socket_count"], 11)
        self.assertEqual(contract["topology_sha256"], EXPECTED_TOPOLOGY)
        self.assertEqual(
            self.payload["group_default_differences"],
            [{
                "socket": "Worked_Luster_Disabled:1:Value",
                "source": 0.0,
                "candidate": 1.0,
            }],
        )
        self.assertEqual(
            self.payload["direction_response"],
            {
                "amount": 0.28,
                "rotation": 0.5,
                "rest": 1.0,
                "source_multiplier": 0.0,
                "candidate_multiplier": 1.0,
                "visible_texture_pattern": False,
            },
        )

    def test_all_components_preserve_frozen_lanes_and_pack_uniform_control(self):
        self.assertEqual(len(self.payload["components"]), 8)
        for component in self.payload["components"].values():
            self.assertTrue(component["frozen_lane_hashes_match_source"])
            self.assertTrue(component["all_images_packed"])
            self.assertEqual(component["luster_control_resolution"], [4, 4])
            self.assertEqual(component["luster_amount_range"], [0.28, 0.28])
            self.assertEqual(component["luster_rotation_range"], [0.5, 0.5])
            self.assertEqual(component["luster_rest_range"], [1.0, 1.0])
            for lane, source_hash in component["source_frozen_lane_pixel_sha256"].items():
                self.assertEqual(component["image_pixel_sha256"][lane], source_hash)

    def test_separate_process_reopen_is_recorded(self):
        self.assertTrue(self.payload["separate_process_reopen"]["verified"])
        self.assertTrue(REOPEN_CONTRACT.is_file())
        contract = json.loads(REOPEN_CONTRACT.read_text())
        self.assertTrue(contract["verified"])
        self.assertEqual(contract["candidate_blend_sha256"], sha256_file(CANDIDATE))
        self.assertEqual(contract["unique_material_count"], 8)
        self.assertEqual(contract["candidate_group_users"], 8)
        self.assertFalse(contract["candidate_saved_by_verifier"])


@unittest.skipUnless(BLENDER.is_file() and CANDIDATE.is_file(), "v004.2 unavailable")
class ForgedIronV0042DirectReopenTests(unittest.TestCase):
    def test_saved_file_exposes_eight_live_v004_2_group_consumers(self):
        expression = r'''
import bpy,json
targets=("SM_GH018_FixedLeaf","SM_GH018_MovingLeaf_Openwork","SM_GH018_Pintle","MovingKnuckle_01","FixedKnuckle_02","MovingKnuckle_03","FixedKnuckle_04","MovingKnuckle_05")
g=bpy.data.node_groups["IGGY_SH_ConnectedOxideForgedIron_v004_2"]
p={"materials":[],"users":0,"packed":True,"multiplier":float(g.nodes["Worked_Luster_Disabled"].inputs[1].default_value)}
for name in targets:
 m=bpy.data.objects[name].data.materials[0]; p["materials"].append(m.name)
 p["users"]+=sum(n.bl_idname=="ShaderNodeGroup" and n.node_tree==g for n in m.node_tree.nodes)
 p["packed"]=p["packed"] and all(n.image and n.image.packed_file for n in m.node_tree.nodes if n.bl_idname=="ShaderNodeTexImage")
print("IGGY_JSON="+json.dumps(p))
'''
        completed = subprocess.run(
            [
                str(BLENDER),
                "--background",
                str(CANDIDATE),
                "--python-expr",
                expression,
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        line = next(
            value for value in completed.stdout.splitlines() if value.startswith("IGGY_JSON=")
        )
        payload = json.loads(line.removeprefix("IGGY_JSON="))
        self.assertEqual(len(set(payload["materials"])), 8)
        self.assertEqual(payload["users"], 8)
        self.assertTrue(payload["packed"])
        self.assertEqual(payload["multiplier"], 1.0)


@unittest.skipUnless(PROOF_MANIFEST.is_file(), "v004.1-v004.2 proof unavailable")
class ForgedIronV0042ProofTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = json.loads(PROOF_MANIFEST.read_text())

    def test_two_rows_have_seven_hash_locked_cycles_panels(self):
        self.assertEqual(self.payload["render_engine"], "CYCLES")
        self.assertEqual(self.payload["cycles_samples"], 32)
        self.assertFalse(self.payload["persistent_render_data"])
        self.assertEqual(self.payload["status"], "USER_NOT_ACCEPTED_PROOF_ONLY")
        self.assertEqual(len(self.payload["rows"]), 2)
        for row in self.payload["rows"].values():
            self.assertEqual(len(row["renders"]), 7)
            for proof in row["renders"].values():
                path = Path(proof["path"])
                self.assertEqual(png_dimensions(path), (640, 220))
                self.assertEqual(proof["sha256"], sha256_file(path))

    def test_board_and_frozen_lane_proof_are_exact(self):
        self.assertEqual(png_dimensions(BOARD), (4480, 440))
        self.assertEqual(self.payload["board"]["sha256"], sha256_file(BOARD))
        frozen = self.payload["frozen_lane_contract"]
        self.assertTrue(frozen["all_component_frozen_lane_hashes_match"])
        self.assertTrue(frozen["source_normal_proof_decoded_pixels_identical"])

    def test_review_stays_below_acceptance_and_engine_parity(self):
        self.assertIn(
            self.payload["visual_review"]["decision"],
            {
                "visual_review_pending",
                "reject_v004_2",
                "prefer_v004_2_repair_candidate",
            },
        )
        self.assertFalse(self.payload["visual_review"]["manual_acceptance_established"])
        self.assertTrue(self.payload["manual_acceptance_required"])
        self.assertFalse(self.payload["unreal_parity_verified"])


if __name__ == "__main__":
    unittest.main()
