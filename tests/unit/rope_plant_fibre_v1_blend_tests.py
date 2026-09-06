from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "rope_plant_fibre_v1"
)
BUILDER_PATH = MATERIAL_ROOT / "build_rope_plant_fibre_v1.py"
PROFILE_PATH = MATERIAL_ROOT / "profiles" / "rope_plant_fibre_v1.json"
OUTPUT_ROOT = MATERIAL_ROOT / "output"
BLEND_PATH = OUTPUT_ROOT / "rope_plant_fibre_v1.blend"
MANIFEST_PATH = OUTPUT_ROOT / "rope_plant_fibre_v1_blender_manifest.json"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")

GROUP_NAME = "IGGY_GN_RopePlantFibre_v001"
MATERIAL_NAME = "IGGY_MAT_RopePlantFibre_v001"


class RopePlantFibreV1SourceTests(unittest.TestCase):
    def test_profile_declares_measured_nested_geometry_contract(self):
        profile = json.loads(PROFILE_PATH.read_text())
        geometry = profile["geometry_source"]

        self.assertEqual(
            geometry["schema"],
            "iggy3d.geometry.rope_plant_fibre_v1.v1",
        )
        self.assertEqual(geometry["strand_count"], 3)
        self.assertEqual(geometry["yarns_per_strand"], 7)
        self.assertEqual(geometry["rope_lay_direction"], "right_hand")
        self.assertEqual(geometry["yarn_lay_direction"], "left_hand")
        self.assertEqual(
            geometry["coordinate_attributes"],
            [
                "sinc_rope_u_m",
                "sinc_rope_v_turn",
                "sinc_rope_diameter_m",
                "sinc_rope_strand_id",
                "sinc_rope_yarn_id",
                "rope_uv",
            ],
        )
        self.assertAlmostEqual(
            geometry["packing"]["strand_radius_diameter_fraction"],
            0.2320508075688773,
            places=12,
        )
        self.assertAlmostEqual(
            geometry["packing"]["strand_center_diameter_fraction"],
            0.2679491924311227,
            places=12,
        )
        self.assertLess(
            geometry["yarn_counterturn_ratio_min"],
            geometry["yarn_counterturn_ratio_default"],
        )
        self.assertGreater(
            geometry["yarn_counterturn_ratio_max"],
            geometry["yarn_counterturn_ratio_default"],
        )

    def test_builder_authors_geometry_nodes_instead_of_modifier_imitation(self):
        source = BUILDER_PATH.read_text()

        self.assertIn("GeometryNodeDuplicateElements", source)
        self.assertIn("GeometryNodeSplineParameter", source)
        self.assertIn("GeometryNodeInputTangent", source)
        self.assertIn("GeometryNodeInputNormal", source)
        self.assertIn("GeometryNodeCurveToMesh", source)
        self.assertIn("GeometryNodeStoreNamedAttribute", source)
        self.assertIn("FLOAT2", source)
        self.assertNotIn("bpy.ops.mesh.primitive_torus_add", source)
        self.assertNotIn("ShaderNodeAmbientOcclusion", source)


@unittest.skipUnless(BLENDER.is_file(), "Blender is not installed")
class RopePlantFibreV1BlendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not BLEND_PATH.is_file():
            raise AssertionError(f"Missing rope blend: {BLEND_PATH}")
        if not MANIFEST_PATH.is_file():
            raise AssertionError(f"Missing rope manifest: {MANIFEST_PATH}")
        cls.manifest = json.loads(MANIFEST_PATH.read_text())

    def test_blend_contains_shared_measured_geometry_and_material_contract(self):
        script = rf'''
import bpy
import json

group = bpy.data.node_groups["{GROUP_NAME}"]
material = bpy.data.materials["{MATERIAL_NAME}"]
required_objects = [
    "IGGY_Rope_FineLashing",
    "IGGY_Rope_UtilityHero",
    "IGGY_Rope_HeavyHawser",
    "IGGY_Rope_UtilityStraight",
]
objects = [bpy.data.objects[name] for name in required_objects]
interface_inputs = sorted(
    item.name
    for item in group.interface.items_tree
    if item.item_type == "SOCKET" and item.in_out == "INPUT"
)
modifier_groups = sorted({{
    modifier.node_group.name
    for obj in objects
    for modifier in obj.modifiers
    if modifier.type == "NODES"
}})

straight = bpy.data.objects["IGGY_Rope_UtilityStraight"]
evaluated = straight.evaluated_get(bpy.context.evaluated_depsgraph_get())
mesh = bpy.data.meshes.new_from_object(evaluated)
attributes = {{
    attribute.name: {{
        "domain": attribute.domain,
        "data_type": attribute.data_type,
    }}
    for attribute in mesh.attributes
}}

def scalar_values(name):
    attribute = mesh.attributes[name]
    if attribute.data_type == "INT":
        return [item.value for item in attribute.data]
    return [float(item.value) for item in attribute.data]

payload = {{
    "group_nodes": sorted(group.nodes.keys()),
    "material_nodes": sorted(material.node_tree.nodes.keys()),
    "interface_inputs": interface_inputs,
    "modifier_groups": modifier_groups,
    "material_schema": material["iggy_material_schema"],
    "material_uses_baked_ao_in_base_colour": bool(
        material["iggy_uses_baked_ao_in_base_colour"]
    ),
    "objects": {{
        obj.name: {{
            "diameter_m": float(obj["sinc_rope_diameter_m"]),
            "lay_length_m": float(obj["sinc_rope_lay_length_m"]),
            "rope_lay_sign": int(obj["sinc_rope_lay_sign"]),
            "yarn_lay_sign": int(obj["sinc_yarn_lay_sign"]),
        }}
        for obj in objects
    }},
    "straight_dimensions": list(straight.dimensions),
    "vertex_count": len(mesh.vertices),
    "polygon_count": len(mesh.polygons),
    "attributes": attributes,
    "strand_ids": sorted(set(scalar_values("sinc_rope_strand_id"))),
    "yarn_ids": sorted(set(scalar_values("sinc_rope_yarn_id"))),
    "diameter_values": scalar_values("sinc_rope_diameter_m"),
    "u_values": scalar_values("sinc_rope_u_m"),
}}
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
            item
            for item in completed.stdout.splitlines()
            if item.startswith("IGGY_JSON=")
        )
        payload = json.loads(line.removeprefix("IGGY_JSON="))

        self.assertEqual(
            payload["modifier_groups"],
            [GROUP_NAME],
        )
        self.assertTrue(
            {
                "Geometry",
                "Diameter",
                "Lay Length",
                "Yarn Counterturn",
                "Sample Length",
                "Profile Resolution",
                "Material",
            }.issubset(payload["interface_inputs"])
        )
        self.assertTrue(
            {
                "IGGY_ResampleByMetres",
                "IGGY_PathLengthMetres",
                "IGGY_Duplicate21Yarns",
                "IGGY_RopeAngleFromMetres",
                "IGGY_OpposedYarnAngle",
                "IGGY_ParallelTransportBinormal",
                "IGGY_StoreLongitudinalMetres",
                "IGGY_StoreProfileTurn",
                "IGGY_StoreFaceCornerUV",
                "IGGY_CurveToYarnMesh",
                "IGGY_Duplicate3StrandHulls",
                "IGGY_CurveToStrandHull",
                "IGGY_JoinStrandHullAndYarns",
            }.issubset(payload["group_nodes"])
        )
        self.assertTrue(
            {
                "IGGY_Colour_BroadPassages",
                "IGGY_Colour_YarnVariation",
                "IGGY_FibreAtlas_LongShortSpawn",
                "IGGY_FibreAtlasVariationRow",
                "IGGY_AuthoredFibreBaseColourAtlas",
                "IGGY_AuthoredAndProceduralColour",
                "IGGY_Fibre_LongitudinalRidges",
                "IGGY_FibreColourLinework",
                "IGGY_IndependentRoughness",
                "IGGY_FibreSheen",
            }.issubset(payload["material_nodes"])
        )
        self.assertEqual(
            payload["material_schema"],
            "iggy3d.material.rope_plant_fibre_v1.v1",
        )
        self.assertFalse(payload["material_uses_baked_ao_in_base_colour"])

        expected_attributes = {
            "sinc_rope_u_m",
            "sinc_rope_v_turn",
            "sinc_rope_diameter_m",
            "sinc_rope_strand_id",
            "sinc_rope_yarn_id",
            "rope_uv",
        }
        self.assertTrue(
            expected_attributes.issubset(payload["attributes"])
        )
        self.assertEqual(
            payload["attributes"]["rope_uv"],
            {"domain": "CORNER", "data_type": "FLOAT2"},
        )
        self.assertEqual(payload["strand_ids"], [0, 1, 2])
        self.assertEqual(payload["yarn_ids"], list(range(7)))
        self.assertGreater(payload["vertex_count"], 12_000)
        self.assertGreater(payload["polygon_count"], 12_000)
        self.assertGreater(max(payload["u_values"]), 0.42)
        self.assertLess(min(payload["u_values"]), 1.0e-6)
        self.assertTrue(
            all(abs(value - 0.032) < 1.0e-6
                for value in payload["diameter_values"])
        )
        self.assertAlmostEqual(
            payload["straight_dimensions"][1],
            0.032,
            delta=0.0035,
        )
        self.assertAlmostEqual(
            payload["straight_dimensions"][2],
            0.032,
            delta=0.0035,
        )

        for values in payload["objects"].values():
            ratio = values["lay_length_m"] / values["diameter_m"]
            self.assertGreaterEqual(ratio, 2.79)
            self.assertLessEqual(ratio, 2.84)
            self.assertEqual(values["rope_lay_sign"], 1)
            self.assertEqual(values["yarn_lay_sign"], -1)

    def test_manifest_records_hierarchy_coordinates_and_visual_proofs(self):
        self.assertEqual(
            self.manifest["schema"],
            "iggy3d.material.rope_plant_fibre_blender_build.v1",
        )
        self.assertEqual(
            self.manifest["hierarchy"],
            {
                "strand_count": 3,
                "yarns_per_strand": 7,
                "total_yarns": 21,
                "rope_lay_direction": "right_hand",
                "yarn_lay_direction": "left_hand",
            },
        )
        self.assertEqual(
            self.manifest["coordinate_contract"]["attributes"],
            [
                "sinc_rope_u_m",
                "sinc_rope_v_turn",
                "sinc_rope_diameter_m",
                "sinc_rope_strand_id",
                "sinc_rope_yarn_id",
                "rope_uv",
            ],
        )
        self.assertEqual(
            set(self.manifest["renders"]),
            {
                "hero_oblique",
                "construction_side",
                "grazing_response",
                "scale_family",
            },
        )
        for render in self.manifest["renders"].values():
            path = Path(render["path"])
            self.assertTrue(path.is_file())
            self.assertGreater(path.stat().st_size, 50_000)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
