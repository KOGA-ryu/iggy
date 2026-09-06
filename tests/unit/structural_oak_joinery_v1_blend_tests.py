from __future__ import annotations

import json
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "structural_oak_joinery_v1"
)
OUTPUT_ROOT = MATERIAL_ROOT / "output"
BLEND_PATH = OUTPUT_ROOT / "structural_oak_joinery_v1.blend"
ATLAS_MANIFEST_PATH = OUTPUT_ROOT / "structural_oak_joinery_v1_atlas_manifest.json"
BLENDER_MANIFEST_PATH = OUTPUT_ROOT / "structural_oak_joinery_v1_manifest.json"
BUILDER_PATH = MATERIAL_ROOT / "build_structural_oak_joinery_v1.py"
README_PATH = MATERIAL_ROOT / "README.md"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")


class StructuralOakJoineryV1SourceTests(unittest.TestCase):
    def test_builder_uses_exact_material_transfer_then_corner_semantics(self):
        source = BUILDER_PATH.read_text()
        for token in (
            'modifier.solver = "EXACT"',
            'modifier.material_mode = "TRANSFER"',
            "_tag_transferred_faces",
            "surface_class_attribute",
            "cut_uv_attribute",
            'domain="CORNER"',
            "temporary Boolean transfer marker still has datablock users",
        ):
            self.assertIn(token, source)
        self.assertNotIn("ShaderNodeTexNoise", source)
        self.assertNotIn("_roughness.png", source)

    def test_written_contract_records_physical_joinery_and_excluded_lanes(self):
        written = " ".join(
            README_PATH.read_text().lower().replace("`", "").split()
        )
        for phrase in (
            "one quarter of stock thickness",
            "0.0024 m toward the shoulder",
            "all 832 annual-ring boundaries",
            "cylindrical sample",
            "corner-domain taxonomy",
            "one closed manifold mesh",
            "no damage",
            "no random source",
            "no baked lighting",
        ):
            self.assertIn(phrase, written)


@unittest.skipUnless(BLENDER.is_file(), "Blender is not installed")
class StructuralOakJoineryV1BlendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for path in (
            BLEND_PATH,
            ATLAS_MANIFEST_PATH,
            BLENDER_MANIFEST_PATH,
        ):
            if not path.is_file():
                raise AssertionError(f"missing built joinery asset: {path}")
        cls.atlas_manifest = json.loads(ATLAS_MANIFEST_PATH.read_text())
        cls.blender_manifest = json.loads(BLENDER_MANIFEST_PATH.read_text())
        script = r'''
import bpy
import bmesh
import json

names = [
    "IGGY_Joinery_ReceivingTimber",
    "IGGY_Joinery_EnteringTenon",
    "IGGY_Joinery_OctagonalDrawPeg",
]
class_name = "sinc_wood_surface_class"
uv_name = "sinc_joinery_uv"

objects = {}
for name in names:
    obj = bpy.data.objects[name]
    mesh = obj.data
    topology = bmesh.new()
    topology.from_mesh(mesh)
    attribute = mesh.attributes[class_name]
    objects[name] = {
        "manifold": all(edge.is_manifold for edge in topology.edges),
        "classes": sorted({int(item.value) for item in attribute.data}),
        "class_domain": attribute.domain,
        "class_type": attribute.data_type,
        "uv_domain": mesh.attributes[uv_name].domain if uv_name in mesh.attributes else None,
        "materials": [
            slot.material.name
            for slot in obj.material_slots
            if slot.material is not None
        ],
        "vertices": len(mesh.vertices),
        "polygons": len(mesh.polygons),
    }
    topology.free()

cut = bpy.data.materials["IGGY_MAT_StructuralOakJoineryCutsV1"]
principled = cut.node_tree.nodes["Clean Joinery Principled"]
images = {
    node.name: {
        "packed": bool(node.image and node.image.packed_file),
        "colorspace": node.image.colorspace_settings.name if node.image else None,
    }
    for node in cut.node_tree.nodes
    if node.bl_idname == "ShaderNodeTexImage"
}
payload = {
    "objects": objects,
    "cut_nodes": sorted(cut.node_tree.nodes.keys()),
    "images": images,
    "roughness": float(principled.inputs["Roughness"].default_value),
    "metallic": float(principled.inputs["Metallic"].default_value),
    "damage": bool(cut["iggy_damage"]),
    "roughness_map": bool(cut["iggy_roughness_map"]),
    "marker_present": (
        "IGGY_MAT_JoineryBooleanTransferMarker" in bpy.data.materials
    ),
    "cutter_objects": sorted(
        obj.name for obj in bpy.data.objects if obj.name.startswith("IGGY_CUT_")
    ),
    "packed_file_count": sum(
        1 for image in bpy.data.images if image.packed_file is not None
    ),
}
print("IGGY_JSON=" + json.dumps(payload))
'''
        completed = subprocess.run(
            [
                str(BLENDER),
                "--background",
                str(BLEND_PATH),
                "--python-expr",
                script,
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        line = next(
            value
            for value in completed.stdout.splitlines()
            if value.startswith("IGGY_JSON=")
        )
        cls.payload = json.loads(line.removeprefix("IGGY_JSON="))

    def test_saved_geometry_is_manifold_and_keeps_discrete_face_meanings(self):
        expected = {
            "IGGY_Joinery_ReceivingTimber": [0, 1, 3, 4, 5, 6, 7],
            "IGGY_Joinery_EnteringTenon": [0, 1, 2, 7, 8, 9],
            "IGGY_Joinery_OctagonalDrawPeg": [10, 11],
        }
        self.assertEqual(set(self.payload["objects"]), set(expected))
        for name, classes in expected.items():
            obj = self.payload["objects"][name]
            self.assertTrue(obj["manifold"])
            self.assertEqual(obj["classes"], classes)
            self.assertEqual(obj["class_domain"], "CORNER")
            self.assertEqual(obj["class_type"], "INT")
        self.assertEqual(
            self.payload["objects"]["IGGY_Joinery_ReceivingTimber"]["uv_domain"],
            "CORNER",
        )
        self.assertEqual(
            self.payload["objects"]["IGGY_Joinery_EnteringTenon"]["uv_domain"],
            "CORNER",
        )
        self.assertGreater(
            self.payload["objects"]["IGGY_Joinery_ReceivingTimber"]["vertices"],
            10_000,
        )
        self.assertEqual(
            self.payload["objects"]["IGGY_Joinery_OctagonalDrawPeg"]["polygons"],
            18,
        )

    def test_saved_material_has_only_clean_packed_cut_lanes(self):
        self.assertTrue(
            {
                "Explicit Joinery Cut UV",
                "Explicit Corner Surface Class",
                "Physical Cut Base Color Atlas",
                "Physical Cut Normal Atlas",
                "Measured Cut Face Normal",
                "Clean Joinery Principled",
            }.issubset(self.payload["cut_nodes"])
        )
        self.assertEqual(len(self.payload["images"]), 2)
        self.assertTrue(
            all(item["packed"] for item in self.payload["images"].values())
        )
        self.assertEqual(
            self.payload["images"]["Physical Cut Base Color Atlas"]["colorspace"],
            "sRGB",
        )
        self.assertEqual(
            self.payload["images"]["Physical Cut Normal Atlas"]["colorspace"],
            "Non-Color",
        )
        self.assertAlmostEqual(self.payload["roughness"], 0.76)
        self.assertEqual(self.payload["metallic"], 0.0)
        self.assertFalse(self.payload["damage"])
        self.assertFalse(self.payload["roughness_map"])
        self.assertGreaterEqual(self.payload["packed_file_count"], 6)

    def test_temporary_boolean_state_is_absent_from_saved_asset(self):
        self.assertFalse(self.payload["marker_present"])
        self.assertEqual(self.payload["cutter_objects"], [])
        contract = self.blender_manifest["boolean_contract"]
        self.assertEqual(contract["solver"], "EXACT")
        self.assertEqual(contract["material_mode"], "TRANSFER")
        self.assertEqual(len(contract["cuts"]), 4)
        self.assertFalse(contract["marker_material_present_after_build"])

    def test_atlas_measurements_and_proof_package_are_complete(self):
        self.assertEqual(self.atlas_manifest["atlas_resolution"], [3072, 1536])
        self.assertEqual(self.atlas_manifest["continuity"]["ring_count"], 832)
        self.assertEqual(
            self.atlas_manifest["continuity"]["joinery_ring_table_digest"],
            self.atlas_manifest["continuity"]["timber_ring_table_digest"],
        )
        fixture = self.blender_manifest["measurements_m"]
        self.assertAlmostEqual(fixture["mortise"]["width_y"], 0.075)
        self.assertAlmostEqual(fixture["tenon"]["width"], 0.071)
        self.assertAlmostEqual(fixture["peg"]["drawbore_offset"], 0.0024)
        self.assertEqual(fixture["peg"]["sides"], 8)
        self.assertEqual(
            set(self.blender_manifest["proofs"]),
            {
                "assembly_exploded",
                "mortise_housing_bore",
                "tenon_shoulder_bore",
                "cutface_closeup",
            },
        )
        for proof in self.blender_manifest["proofs"].values():
            path = Path(proof["path"])
            self.assertTrue(path.is_file())
            self.assertEqual((proof["width"], proof["height"]), (1440, 810))
            self.assertGreater(path.stat().st_size, 25_000)


if __name__ == "__main__":
    unittest.main()
