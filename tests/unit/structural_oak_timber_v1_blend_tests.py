from __future__ import annotations

import json
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "structural_oak_timber_v1"
)
OUTPUT_ROOT = MATERIAL_ROOT / "output"
BLEND_PATH = OUTPUT_ROOT / "structural_oak_timber_v1_beam.blend"
MAP_MANIFEST_PATH = OUTPUT_ROOT / "structural_oak_timber_v1_manifest.json"
BLENDER_MANIFEST_PATH = (
    OUTPUT_ROOT / "structural_oak_timber_v1_blender_manifest.json"
)
BUILDER_PATH = MATERIAL_ROOT / "build_structural_oak_timber_v1.py"
README_PATH = MATERIAL_ROOT / "README.md"
RESEARCH_PATH = MATERIAL_ROOT / "RESEARCH_INTENT.md"
REVIEW_PATH = MATERIAL_ROOT / "QUALITY_REVIEW.md"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")


class StructuralOakTimberV1SourceTests(unittest.TestCase):
    def test_builder_routes_semantic_faces_without_roughness_or_damage_maps(self):
        source = BUILDER_PATH.read_text()
        for token in (
            "Timber Face Identity",
            "Side Atlas Coordinate",
            "End Atlas Coordinate",
            "Side End Color Continuity",
            "Side End Normal Continuity",
            "Chamfer Face Mask",
            "Explicit Chamfer Color",
            "Measured Timber Normal",
            "uniform_roughness",
        ):
            self.assertIn(token, source)
        self.assertNotIn("_roughness.png", source)
        self.assertNotIn("ShaderNodeTexNoise", source)
        self.assertNotIn("ShaderNodeAmbientOcclusion", source)

    def test_written_contract_records_measurement_transfer_and_repairs(self):
        written = "\n".join(
            path.read_text()
            for path in (README_PATH, RESEARCH_PATH, REVIEW_PATH)
        ).lower()
        for phrase in (
            "one pith path",
            "one annual-ring table",
            "154 ± 28",
            "finite tracks",
            "bury, fade, or flatten",
            "quiet fields",
            "subpixel",
            "no roughness texture",
            "repair passes",
            "black rails",
        ):
            self.assertIn(phrase, written)


@unittest.skipUnless(BLENDER.is_file(), "Blender is not installed")
class StructuralOakTimberV1BlendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for path in (BLEND_PATH, MAP_MANIFEST_PATH, BLENDER_MANIFEST_PATH):
            if not path.is_file():
                raise AssertionError(f"missing built material asset: {path}")
        cls.map_manifest = json.loads(MAP_MANIFEST_PATH.read_text())
        cls.blender_manifest = json.loads(BLENDER_MANIFEST_PATH.read_text())

    def blender_payload(self) -> dict:
        script = r'''
import bpy
import bmesh
import json

master = bpy.data.objects["IGGY_WA001_RoughHewnTimberBeam_Master"]
material = bpy.data.materials["IGGY_MAT_StructuralOakTimberV1"]
knot_material = bpy.data.materials["IGGY_MAT_StructuralOakKnotV1"]
evaluated = master.evaluated_get(bpy.context.evaluated_depsgraph_get())
mesh = bpy.data.meshes.new_from_object(evaluated)
topology = bmesh.new()
topology.from_mesh(mesh)
images = {
    node.name: {
        "image": node.image.name if node.image else None,
        "packed": bool(node.image and node.image.packed_file),
        "colorspace": node.image.colorspace_settings.name if node.image else None,
    }
    for node in material.node_tree.nodes
    if node.bl_idname == "ShaderNodeTexImage"
}
principled = material.node_tree.nodes["Structural Oak Principled"]
payload = {
    "dimensions": [float(value) for value in master.dimensions],
    "manifold": all(edge.is_manifold for edge in topology.edges),
    "modifier_types": [modifier.type for modifier in master.modifiers],
    "attributes": {
        attribute.name: {
            "domain": attribute.domain,
            "data_type": attribute.data_type,
        }
        for attribute in mesh.attributes
    },
    "material_nodes": sorted(material.node_tree.nodes.keys()),
    "images": images,
    "roughness": float(principled.inputs["Roughness"].default_value),
    "metallic": float(principled.inputs["Metallic"].default_value),
    "master_material": master.material_slots[0].material.name,
    "material_profile": material["iggy_material_profile"],
    "coordinate_contract": material["iggy_coordinate_contract"],
    "roughness_lane": material["iggy_roughness_lane"],
    "damage_lane": material["iggy_damage_lane"],
    "master_status": master["iggy_material_status"],
    "master_continuity": bool(master["iggy_growth_volume_continuity"]),
    "master_roughness_map": bool(master["iggy_authored_roughness_map"]),
    "master_damage": bool(master["iggy_material_damage"]),
    "knot_material": knot_material.name,
}
topology.free()
bpy.data.meshes.remove(mesh)
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
        return json.loads(line.removeprefix("IGGY_JSON="))

    def test_saved_beam_keeps_geometry_and_semantic_mapping_contract(self):
        payload = self.blender_payload()
        self.assertAlmostEqual(payload["dimensions"][0], 4.2, delta=0.035)
        self.assertTrue(payload["manifold"])
        self.assertGreaterEqual(payload["modifier_types"].count("BOOLEAN"), 9)
        self.assertEqual(
            payload["modifier_types"][-2:],
            ["BEVEL", "WEIGHTED_NORMAL"],
        )
        for attribute in (
            "sinc_timber_u_m",
            "sinc_timber_face_id",
            "sinc_end_distance_m",
            "IGGY_TimberUV",
        ):
            self.assertIn(attribute, payload["attributes"])
        self.assertEqual(
            payload["attributes"]["sinc_timber_face_id"],
            {"domain": "FACE", "data_type": "INT"},
        )
        self.assertEqual(
            payload["attributes"]["IGGY_TimberUV"],
            {"domain": "CORNER", "data_type": "FLOAT2"},
        )

    def test_material_uses_packed_side_end_maps_and_explicit_chamfer_route(self):
        payload = self.blender_payload()
        self.assertEqual(
            payload["master_material"],
            "IGGY_MAT_StructuralOakTimberV1",
        )
        required_nodes = {
            "Timber Metre UV",
            "Timber Face Identity",
            "Side Atlas Coordinate",
            "End Atlas Coordinate",
            "Side Growth Base Color",
            "End Growth Base Color",
            "Side Growth Normal",
            "End Growth Normal",
            "End Face Mask",
            "Explicit Chamfer Color",
            "Explicit Chamfer Normal",
            "Measured Timber Normal",
        }
        self.assertTrue(required_nodes.issubset(payload["material_nodes"]))
        self.assertEqual(len(payload["images"]), 4)
        self.assertTrue(all(image["packed"] for image in payload["images"].values()))
        self.assertEqual(
            payload["images"]["Side Growth Base Color"]["colorspace"],
            "sRGB",
        )
        self.assertEqual(
            payload["images"]["Side Growth Normal"]["colorspace"],
            "Non-Color",
        )
        self.assertAlmostEqual(payload["roughness"], 0.76)
        self.assertEqual(payload["metallic"], 0.0)
        self.assertEqual(payload["material_profile"], "structural_oak_timber_v1")
        self.assertEqual(payload["coordinate_contract"], "shared_growth_volume")
        self.assertEqual(payload["roughness_lane"], "uniform_constant_only")
        self.assertEqual(payload["damage_lane"], "excluded")
        self.assertEqual(payload["master_status"], "structural_oak_timber_v1")
        self.assertTrue(payload["master_continuity"])
        self.assertFalse(payload["master_roughness_map"])
        self.assertFalse(payload["master_damage"])

    def test_manifests_and_review_proofs_are_complete(self):
        continuity = self.map_manifest["continuity"]
        self.assertEqual(
            continuity["side_ring_table_digest"],
            continuity["end_ring_table_digest"],
        )
        self.assertGreaterEqual(continuity["ring_count"], 800)
        self.assertEqual(
            self.map_manifest["authored_counts"]["grain_tracks"],
            27,
        )
        self.assertEqual(
            self.map_manifest["authored_counts"]["tool_linework_events"],
            17,
        )
        self.assertFalse(
            self.map_manifest["surface_response"]["roughness_map"]
        )
        self.assertFalse(self.map_manifest["surface_response"]["damage"])

        self.assertEqual(
            self.blender_manifest["schema"],
            "iggy3d.material.structural_oak_timber.blender.v1",
        )
        self.assertEqual(
            set(self.blender_manifest["proofs"]),
            {"hero", "reverse", "end_growth", "grazing"},
        )
        for proof in self.blender_manifest["proofs"].values():
            path = Path(proof["path"])
            self.assertTrue(path.is_file())
            self.assertGreater(path.stat().st_size, 25_000)


if __name__ == "__main__":
    unittest.main()
