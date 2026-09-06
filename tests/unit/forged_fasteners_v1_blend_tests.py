from __future__ import annotations

import json
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE_ROOT = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
BLEND_PATH = PACKAGE_ROOT / "output" / "forged_fasteners_v1.blend"
MANIFEST_PATH = PACKAGE_ROOT / "output" / "forged_fasteners_v1_manifest.json"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")


@unittest.skipUnless(BLENDER.is_file(), "Blender is not installed")
class ForgedFastenersBlendTests(unittest.TestCase):
    def test_reusable_group_masters_and_exact_measured_proof(self):
        self.assertTrue(BLEND_PATH.is_file())
        manifest = json.loads(MANIFEST_PATH.read_text())
        self.assertEqual(manifest["status"], "MEASURED_FORGED_FASTENERS_BUILT")
        self.assertTrue(manifest["constraints"]["uses_authored_meshes"])
        self.assertFalse(manifest["constraints"]["uses_noise_displacement"])

        script = r'''
import bpy
import json

group = bpy.data.node_groups["IGGY_GN_ForgedFasteners_v001"]
door_group = bpy.data.node_groups["SINC_GN_DoorAssembly_v001"]
proof = bpy.data.objects["IGGY_FastenerProof_MetDoorNail_55_61_134"]
masters = sorted(
    obj.name
    for obj in bpy.data.objects
    if obj.name.startswith("IGGY_FastenerMaster_")
)
noise_nodes = [
    node.bl_idname
    for node in group.nodes
    if node.bl_idname in {"ShaderNodeTexNoise", "ShaderNodeTexVoronoi"}
]
named_attributes = sorted({
    node.inputs["Name"].default_value
    for node in group.nodes
    if node.bl_idname == "GeometryNodeStoreNamedAttribute"
})
payload = {
    "master_names": masters,
    "master_hidden": all(
        bpy.data.objects[name].hide_render
        for name in masters
    ),
    "noise_nodes": noise_nodes,
    "named_attributes": named_attributes,
    "proof_dimensions": list(proof.dimensions),
    "door_exists": "SINC_DemoDoorAssembly" in bpy.data.objects,
    "door_materials": [
        slot.material.name if slot.material else None
        for slot in bpy.data.objects["SINC_DemoDoorAssembly"].material_slots
    ],
    "brace_point_source": (
        door_group.nodes["SINC::brace_fasteners"]
        .inputs["Points"].links[0].from_node.name
    ),
    "brace_point_offset": list(
        door_group.nodes[
            "IGGY::brace_fastener_bearing_plane_adapter"
        ].inputs["Translation"].default_value
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
            item
            for item in completed.stdout.splitlines()
            if item.startswith("IGGY_JSON=")
        )
        payload = json.loads(line.removeprefix("IGGY_JSON="))
        self.assertEqual(
            payload["master_names"],
            [
                "IGGY_FastenerMaster_DomedRivet",
                "IGGY_FastenerMaster_FacetedSquarePeen",
                "IGGY_FastenerMaster_FlattenedPeen",
                "IGGY_FastenerMaster_Rosehead",
            ],
        )
        self.assertTrue(payload["master_hidden"])
        self.assertEqual(payload["noise_nodes"], [])
        self.assertTrue(
            {
                "sinc_fastener_style",
                "sinc_fastener_surface_role",
                "sinc_iron_edge_mask",
            }.issubset(payload["named_attributes"])
        )
        self.assertAlmostEqual(payload["proof_dimensions"][0], 0.074, places=4)
        self.assertAlmostEqual(payload["proof_dimensions"][1], 0.075, places=4)
        self.assertAlmostEqual(payload["proof_dimensions"][2], 0.156, places=4)
        self.assertTrue(payload["door_exists"])
        self.assertIn(
            "IGGY_MAT_ReferenceForgedIron_v001",
            payload["door_materials"],
        )
        self.assertEqual(
            payload["brace_point_source"],
            "IGGY::brace_fastener_bearing_plane_adapter",
        )
        self.assertAlmostEqual(payload["brace_point_offset"][0], 0.0, places=5)
        self.assertAlmostEqual(payload["brace_point_offset"][1], 0.105, places=5)
        self.assertAlmostEqual(payload["brace_point_offset"][2], 0.0, places=5)


if __name__ == "__main__":
    unittest.main()
