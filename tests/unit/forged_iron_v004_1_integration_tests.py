from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004_1.py"
RENDER_SCRIPT = MATERIAL_ROOT / "render_connected_oxide_candidate_cycles_v004_1.py"
V004_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004"
SOURCE = V004_ROOT / "forged_iron_connected_oxide_candidate_v004.blend"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004_1"
CANDIDATE = OUTPUT_ROOT / "forged_iron_connected_oxide_candidate_v004_1.blend"
MANIFEST = OUTPUT_ROOT / "manifest.json"
PROOF_ROOT = OUTPUT_ROOT / "cycles_v004_v004_1_proof_v1"
PROOF_MANIFEST = PROOF_ROOT / "manifest.json"
BOARD = PROOF_ROOT / "forged_iron_v004_v004_1_cycles_comparison.png"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")


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


class ForgedIronV0041BlueprintTests(unittest.TestCase):
    def test_builder_freezes_exact_selected_c_recipe(self):
        self.assertTrue(BUILD_SCRIPT.is_file(), "v004.1 builder is missing")
        source = BUILD_SCRIPT.read_text()
        for token in (
            "C_audited_compressed_cool",
            '"#252a31"',
            '"#353b46"',
            "0.680",
            "0.035",
            "0.055",
            "0.012",
            "1.75",
            "IGGY_SH_ConnectedOxideForgedIron_v004",
            "save_as_mainfile",
            "reopen_candidate_contract",
        ):
            self.assertIn(token, source)

    def test_renderer_has_seven_paired_cycles_proofs_and_no_save(self):
        self.assertTrue(RENDER_SCRIPT.is_file(), "v004.1 proof renderer is missing")
        source = RENDER_SCRIPT.read_text()
        self.assertIn('scene.render.engine = "CYCLES"', source)
        for panel in (
            "close_neutral", "grazing", "pivot_close", "gameplay",
            "base_colour", "independent_roughness", "normal",
        ):
            self.assertIn(panel, source)
        self.assertNotIn("save_as_mainfile", source)
        self.assertNotIn("save_mainfile", source)


@unittest.skipUnless(MANIFEST.is_file(), "v004.1 has not been built")
class ForgedIronV0041ManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = json.loads(MANIFEST.read_text())

    def test_candidate_is_noncanonical_and_sources_are_unchanged(self):
        self.assertEqual(
            self.payload["status"], "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE"
        )
        self.assertTrue(self.payload["source_v004_unchanged"])
        self.assertEqual(self.payload["source_v004"]["sha256_after"], sha256_file(SOURCE))
        self.assertFalse(self.payload["manual_acceptance_established"])
        self.assertFalse(self.payload["unreal_parity_verified"])

    def test_all_eight_components_use_exact_integrated_identity(self):
        self.assertEqual(len(self.payload["components"]), 8)
        self.assertEqual(
            self.payload["integrated_recipe"]["candidate_id"],
            "C_audited_compressed_cool",
        )
        for component in self.payload["components"].values():
            self.assertEqual(component["coverage_range"], [1.0, 1.0])
            self.assertEqual(component["metalness_maximum"], 0.0)
            self.assertEqual(component["exposure_maximum"], 0.0)
            self.assertEqual(component["oxide_height_absolute_maximum_m"], 0.0)
            self.assertEqual(component["luster_amount_range"], [0.0, 0.0])
            self.assertTrue(component["all_images_packed"])


@unittest.skipUnless(BLENDER.is_file() and CANDIDATE.is_file(), "v004.1 unavailable")
class ForgedIronV0041ReopenTests(unittest.TestCase):
    def test_candidate_reopens_with_eight_users_of_unchanged_shared_group(self):
        script = r'''
import bpy, json
targets = (
"SM_GH018_FixedLeaf","SM_GH018_MovingLeaf_Openwork","SM_GH018_Pintle",
"MovingKnuckle_01","FixedKnuckle_02","MovingKnuckle_03","FixedKnuckle_04","MovingKnuckle_05")
group=bpy.data.node_groups["IGGY_SH_ConnectedOxideForgedIron_v004"]
payload={"materials":[],"users":0,"packed":True}
for name in targets:
 m=bpy.data.objects[name].data.materials[0]; payload["materials"].append(m.name)
 payload["users"] += sum(n.bl_idname=="ShaderNodeGroup" and n.node_tree==group for n in m.node_tree.nodes)
 payload["packed"] = payload["packed"] and all(n.image and n.image.packed_file for n in m.node_tree.nodes if n.bl_idname=="ShaderNodeTexImage")
print("IGGY_JSON="+json.dumps(payload))
'''
        completed = subprocess.run(
            [str(BLENDER), "--background", str(CANDIDATE), "--python-expr", script],
            check=True, capture_output=True, text=True,
        )
        line = next(x for x in completed.stdout.splitlines() if x.startswith("IGGY_JSON="))
        payload = json.loads(line.removeprefix("IGGY_JSON="))
        self.assertEqual(len(set(payload["materials"])), 8)
        self.assertEqual(payload["users"], 8)
        self.assertTrue(payload["packed"])


@unittest.skipUnless(PROOF_MANIFEST.is_file(), "v004-v004.1 proof unavailable")
class ForgedIronV0041ProofTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = json.loads(PROOF_MANIFEST.read_text())

    def test_two_rows_have_seven_hash_locked_cycles_panels(self):
        self.assertEqual(self.payload["render_engine"], "CYCLES")
        self.assertEqual(self.payload["status"], "USER_NOT_ACCEPTED_PROOF_ONLY")
        self.assertEqual(len(self.payload["rows"]), 2)
        for row in self.payload["rows"].values():
            self.assertEqual(len(row["renders"]), 7)
            for proof in row["renders"].values():
                path = Path(proof["path"])
                self.assertEqual(png_dimensions(path), (720, 240))
                self.assertEqual(proof["sha256"], sha256_file(path))

    def test_board_and_visual_decision_are_hash_locked(self):
        self.assertEqual(self.payload["board"]["sha256"], sha256_file(BOARD))
        self.assertIn(
            self.payload["visual_review"]["result"],
            {"pending", "repair_requested", "production_candidate"},
        )
        self.assertFalse(self.payload["visual_review"]["manual_acceptance_established"])


if __name__ == "__main__":
    unittest.main()
