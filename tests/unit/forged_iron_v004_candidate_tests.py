from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004.py"
RENDER_SCRIPT = MATERIAL_ROOT / "render_connected_oxide_candidate_cycles_v004.py"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004"
BLEND = OUTPUT_ROOT / "forged_iron_connected_oxide_candidate_v004.blend"
MANIFEST = OUTPUT_ROOT / "manifest.json"
PROOF_ROOT = OUTPUT_ROOT / "cycles_actual_hinge_proof_v1"
PROOF_MANIFEST = PROOF_ROOT / "manifest.json"
BOARD = PROOF_ROOT / "forged_iron_v003_v004_cycles_comparison.png"
SOURCE = MATERIAL_ROOT / "output" / "forged_iron_v1.blend"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as handle:
        header = handle.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(f"not a PNG: {path}")
    return struct.unpack(">II", header[16:24])


class ForgedIronV004BlueprintTests(unittest.TestCase):
    def test_builder_is_exactly_the_selected_noncanonical_route(self):
        self.assertTrue(BUILD_SCRIPT.is_file(), "v004 builder is missing")
        source = BUILD_SCRIPT.read_text()
        for token in (
            "B_connected_oxide_no_luster",
            "IGGY_SH_ConnectedOxideForgedIron_v004",
            "IGGY_MAT_ConnectedOxideForgedIron_v004",
            "ShaderNodeMixShader",
            "ShaderNodeNormalMap",
            "ShaderNodeTangent",
            "EXPOSED_CONDUCTOR = 0.0",
            "save_as_mainfile",
            "reopen_candidate_contract",
        ):
            self.assertIn(token, source)
        self.assertNotIn("forged_iron_v1.blend\")", source)

    def test_renderer_codes_all_nine_paired_cycles_proofs(self):
        self.assertTrue(RENDER_SCRIPT.is_file(), "Cycles renderer is missing")
        source = RENDER_SCRIPT.read_text()
        self.assertIn('scene.render.engine = "CYCLES"', source)
        self.assertIn("default=720", source)
        self.assertIn("default=240", source)
        for panel in (
            "full_front",
            "close_neutral",
            "opposed_light_difference",
            "grazing",
            "pivot_close",
            "gameplay",
            "base_colour",
            "independent_roughness",
            "normal",
        ):
            self.assertIn(panel, source)


@unittest.skipUnless(MANIFEST.is_file(), "v004 candidate has not been built")
class ForgedIronV004ManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = json.loads(MANIFEST.read_text())

    def test_candidate_is_noncanonical_and_source_is_unchanged(self):
        self.assertEqual(
            self.payload["status"], "NONCANONICAL_CYCLES_PROOF_CANDIDATE"
        )
        self.assertTrue(self.payload["canonical_source_unchanged"])
        self.assertEqual(sha256_file(SOURCE), self.payload["source"]["sha256_after"])
        self.assertFalse(self.payload["manual_acceptance_established"])
        self.assertFalse(self.payload["unreal_parity_verified"])
        self.assertFalse(self.payload["uses_ai_generated_imagery"])

    def test_all_components_preserve_selected_physical_contract(self):
        self.assertEqual(len(self.payload["components"]), 8)
        for component in self.payload["components"].values():
            self.assertEqual(component["coverage_range"], [1.0, 1.0])
            self.assertEqual(component["metalness_maximum"], 0.0)
            self.assertEqual(component["exposure_maximum"], 0.0)
            self.assertEqual(component["oxide_surface_height_absolute_maximum_m"], 0.0)
            self.assertEqual(component["luster_amount_range"], [0.0, 0.0])
            self.assertEqual(component["material_node_counts"]["image_texture"], 4)
            self.assertTrue(component["all_images_packed"])
        self.assertEqual(self.payload["shared_group_contract"]["principled"], 2)
        self.assertEqual(self.payload["shared_group_contract"]["mix_shader"], 1)
        self.assertEqual(self.payload["shared_group_contract"]["normal_map"], 1)


@unittest.skipUnless(
    BLENDER.is_file() and BLEND.is_file(),
    "Blender or v004 candidate is unavailable",
)
class ForgedIronV004ReopenTests(unittest.TestCase):
    def test_saved_candidate_reopens_with_one_shared_group(self):
        script = r'''
import bpy, json
targets = (
    "SM_GH018_FixedLeaf", "SM_GH018_MovingLeaf_Openwork", "SM_GH018_Pintle",
    "MovingKnuckle_01", "FixedKnuckle_02", "MovingKnuckle_03",
    "FixedKnuckle_04", "MovingKnuckle_05",
)
group = bpy.data.node_groups["IGGY_SH_ConnectedOxideForgedIron_v004"]
materials = []
packed = True
for name in targets:
    obj = bpy.data.objects[name]
    material = obj.data.materials[0]
    materials.append(material.name)
    packed = packed and all(
        node.image is not None and node.image.packed_file is not None
        for node in material.node_tree.nodes
        if node.bl_idname == "ShaderNodeTexImage"
    )
payload = {
    "materials": materials,
    "shared_group_users": sum(
        node.bl_idname == "ShaderNodeGroup" and node.node_tree == group
        for material in {bpy.data.materials[name] for name in materials}
        for node in material.node_tree.nodes
    ),
    "packed": packed,
}
print("IGGY_JSON=" + json.dumps(payload))
'''
        completed = subprocess.run(
            [str(BLENDER), "--background", str(BLEND), "--python-expr", script],
            check=True,
            capture_output=True,
            text=True,
        )
        line = next(line for line in completed.stdout.splitlines() if line.startswith("IGGY_JSON="))
        payload = json.loads(line.removeprefix("IGGY_JSON="))
        self.assertEqual(len(set(payload["materials"])), 8)
        self.assertEqual(payload["shared_group_users"], 8)
        self.assertTrue(payload["packed"])


@unittest.skipUnless(PROOF_MANIFEST.is_file(), "Cycles proof has not been rendered")
class ForgedIronV004ProofTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.payload = json.loads(PROOF_MANIFEST.read_text())

    def test_proof_is_actual_asset_cycles_and_keeps_acceptance_open(self):
        self.assertEqual(self.payload["render_engine"], "CYCLES")
        self.assertEqual(self.payload["status"], "USER_NOT_ACCEPTED_PROOF_ONLY")
        self.assertTrue(self.payload["manual_acceptance_required"])
        self.assertFalse(self.payload["unreal_parity_verified"])
        self.assertEqual(len(self.payload["rows"]), 2)
        for row in self.payload["rows"].values():
            self.assertEqual(len(row["renders"]), 9)
            for proof in row["renders"].values():
                path = Path(proof["path"])
                self.assertTrue(path.is_file())
                self.assertEqual(png_dimensions(path), (720, 240))
                self.assertEqual(proof["sha256"], sha256_file(path))

    def test_board_is_hash_locked_and_reviewed(self):
        self.assertTrue(BOARD.is_file())
        self.assertEqual(self.payload["board"]["sha256"], sha256_file(BOARD))
        self.assertIn(
            self.payload["visual_review"]["result"],
            {"pending", "repair_requested", "production_candidate"},
        )


if __name__ == "__main__":
    unittest.main()
