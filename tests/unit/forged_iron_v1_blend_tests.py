from __future__ import annotations

import json
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
)
BLEND_PATH = MATERIAL_ROOT / "output" / "forged_iron_v1.blend"
MANIFEST_PATH = MATERIAL_ROOT / "output" / "forged_iron_v1_manifest.json"
PROFILE_PATH = MATERIAL_ROOT / "profiles" / "forged_iron_v1.json"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")


@unittest.skipUnless(BLENDER.is_file(), "Blender is not installed")
class ForgedIronBlendTests(unittest.TestCase):
    def test_packed_authored_maps_and_measured_specimen_contract(self):
        self.assertTrue(BLEND_PATH.is_file())
        self.assertTrue(MANIFEST_PATH.is_file())
        profile = json.loads(PROFILE_PATH.read_text())
        manifest = json.loads(MANIFEST_PATH.read_text())
        self.assertEqual(manifest["schema"], "iggy-forged-iron-build/4.0")
        self.assertEqual(
            manifest["status"],
            "LAYERED_NODE_FORGED_IRON_BUILT",
        )
        self.assertEqual(
            manifest["pattern_build"]["schema"],
            "iggy-forged-iron-pattern-build/4.0",
        )
        for constraint in (
            "uses_layered_principled_responses",
            "uses_three_frequency_normals",
            "uses_metre_displacement",
        ):
            self.assertTrue(manifest["constraints"][constraint])
        self.assertFalse(
            manifest["constraints"]["unreal_runtime_parity_verified"]
        )
        script = r'''
import bpy
import json

group = bpy.data.node_groups["IGGY_SH_ReferenceForgedIron_v003"]
material = bpy.data.materials["IGGY_MAT_ReferenceForgedIron_v003"]
coordinate_group = bpy.data.node_groups["IGGY_SH_IronCoordinates_v004"]
strap = bpy.data.objects["IGGY_IronProof_MeasuredReferenceStrap"]
trees = []
pending = [group]
seen = set()
while pending:
    tree = pending.pop()
    if tree.name in seen:
        continue
    seen.add(tree.name)
    trees.append(tree)
    pending.extend(
        node.node_tree
        for node in tree.nodes
        if node.bl_idname == "ShaderNodeGroup"
        and node.node_tree is not None
    )
nodes = [node for tree in trees for node in tree.nodes]
textures = [
    node for node in nodes
    if node.bl_idname == "ShaderNodeTexImage"
]
noise = [
    node for node in nodes
    if node.bl_idname in {"ShaderNodeTexNoise", "ShaderNodeTexVoronoi"}
]
normal_maps = [
    node for node in nodes
    if node.bl_idname == "ShaderNodeNormalMap"
]
tangents = [
    node for node in nodes
    if node.bl_idname == "ShaderNodeTangent"
]
bumps = [
    node for node in nodes
    if node.bl_idname == "ShaderNodeBump"
]
principled = [
    node for node in nodes
    if node.bl_idname == "ShaderNodeBsdfPrincipled"
]
camera_data = [
    node for node in nodes
    if node.bl_idname == "ShaderNodeCameraData"
]
displacement = [
    node for node in material.node_tree.nodes
    if node.bl_idname == "ShaderNodeDisplacement"
]
group_node = next(
    node for node in material.node_tree.nodes
    if node.bl_idname == "ShaderNodeGroup" and node.node_tree == group
)
fastener_topology = {}
for object_name in (
    "IGGY_IronProof_FacetedSquareNail",
    "IGGY_IronProof_FacetedDomedRivet",
):
    obj = bpy.data.objects[object_name]
    edge_use = {}
    for polygon in obj.data.polygons:
        vertices = tuple(polygon.vertices)
        for index, first in enumerate(vertices):
            second = vertices[(index + 1) % len(vertices)]
            edge = tuple(sorted((int(first), int(second))))
            edge_use[edge] = edge_use.get(edge, 0) + 1
    fastener_topology[object_name] = {
        "vertices": len(obj.data.vertices),
        "polygons": len(obj.data.polygons),
        "all_edges_two_manifold": all(
            count == 2 for count in edge_use.values()
        ),
    }
response_sphere = bpy.data.objects["IGGY_IronProof_ResponseSphere"]
response_cylinder = bpy.data.objects["IGGY_IronProof_ResponseCylinder"]
target_names = (
    "SM_GH018_FixedLeaf",
    "SM_GH018_MovingLeaf_Openwork",
    "SM_GH018_Pintle",
    "MovingKnuckle_01",
    "FixedKnuckle_02",
    "MovingKnuckle_03",
    "FixedKnuckle_04",
    "MovingKnuckle_05",
)
payload = {
    "node_groups": sorted(tree.name for tree in trees),
    "texture_nodes": sorted(node.name for node in textures),
    "images_packed": all(
        node.image is not None and node.image.packed_file is not None
        for node in textures
    ),
    "noise_node_count": len(noise),
    "normal_map_count": len(normal_maps),
    "tangent_uv_maps": sorted(node.uv_map for node in tangents),
    "coordinate_outputs": sorted(
        item.name
        for item in coordinate_group.interface.items_tree
        if item.item_type == "SOCKET" and item.in_out == "OUTPUT"
    ),
    "bump_count": len(bumps),
    "principled": {
        node.name: node.inputs["Metallic"].default_value
        for node in principled
    },
    "camera_data_count": len(camera_data),
    "displacement_count": len(displacement),
    "height_is_linked": (
        len(displacement) == 1
        and displacement[0].inputs["Height"].is_linked
        and displacement[0].inputs["Height"].links[0].from_node == group_node
        and displacement[0].inputs["Height"].links[0].from_socket.name
        == "Height M"
    ),
    "group_inputs": {
        socket.name: socket.default_value
        for socket in group_node.inputs
        if socket.name in {
            "Brown Oxidation Amount",
            "Contact Polish Amount",
            "Surface Strength",
            "Macro Normal Strength",
            "Scale Normal Strength",
            "Micro Normal Strength",
            "Worked Normal Strength",
            "Displacement Strength",
        }
    },
    "strap_dimensions": list(strap.dimensions),
    "strap_attributes": sorted(
        attribute.name for attribute in strap.data.attributes
    ),
    "strap_fabrication_scale": (
        strap.data.attributes["sinc_iron_fabrication_scale"].data[0].value
    ),
    "target_objects": {
        name: {
            "materials": [
                material.name for material in bpy.data.objects[name].data.materials
            ],
            "fabrication_scale": (
                bpy.data.objects[name].data.attributes[
                    "sinc_iron_fabrication_scale"
                ].data[0].value
            ),
            "uv_layers": sorted(
                layer.name for layer in bpy.data.objects[name].data.uv_layers
            ),
        }
        for name in target_names
    },
    "fastener_topology": fastener_topology,
    "response_sphere_all_smooth": all(
        polygon.use_smooth for polygon in response_sphere.data.polygons
    ),
    "response_cylinder_has_smooth_and_flat": (
        any(polygon.use_smooth for polygon in response_cylinder.data.polygons)
        and any(
            not polygon.use_smooth
            for polygon in response_cylinder.data.polygons
        )
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
            (
                item
                for item in completed.stdout.splitlines()
                if item.startswith("IGGY_JSON=")
            ),
            None,
        )
        self.assertIsNotNone(
            line,
            completed.stdout + "\nSTDERR:\n" + completed.stderr,
        )
        payload = json.loads(line.removeprefix("IGGY_JSON="))

        self.assertEqual(
            payload["node_groups"],
            [
                "IGGY_SH_AuthoredIronLanes_v004",
                "IGGY_SH_ForgeScaleLayer_v002",
                "IGGY_SH_HammerPlanes_v003",
                "IGGY_SH_IronCoordinates_v004",
                "IGGY_SH_IronMicroSurface_v002",
                "IGGY_SH_NormalCombine_v003",
                "IGGY_SH_ReferenceForgedIron_v003",
                "IGGY_SH_SurfaceHeight_v003",
                "IGGY_SH_WorkedIronSurface_v001",
            ],
        )
        self.assertEqual(
            payload["texture_nodes"],
            [
                "Iron_Color_Atlas",
                "Layer_Response_Atlas",
                "Macro_Height_Atlas",
                "Macro_Normal_Atlas",
                "Micro_Height_Atlas",
                "Micro_Normal_Atlas",
                "Scale_Color_Atlas",
                "Scale_Height_Atlas",
                "Scale_Normal_Atlas",
                "Semantic_Mask_Atlas",
                "Worked_Normal_Atlas",
                "Worked_Response_Atlas",
            ],
        )
        self.assertTrue(payload["images_packed"])
        self.assertEqual(payload["noise_node_count"], 0)
        self.assertEqual(payload["normal_map_count"], 1)
        self.assertEqual(payload["tangent_uv_maps"], ["IGGY_IronUV"])
        self.assertEqual(
            payload["coordinate_outputs"],
            [
                "Edge Mask",
                "Fabrication Scale",
                "Macro Atlas Coordinate",
                "Surface Atlas Coordinate",
                "Variation Index",
                "Worked Atlas Coordinate",
            ],
        )
        self.assertEqual(payload["bump_count"], 0)
        self.assertEqual(
            payload["principled"],
            {
                "Conductive_Exposed_Iron": 1.0,
                "Dielectric_Compact_Forge_Scale": 0.0,
            },
        )
        self.assertEqual(payload["camera_data_count"], 1)
        self.assertEqual(payload["displacement_count"], 1)
        self.assertTrue(payload["height_is_linked"])
        self.assertEqual(
            set(payload["group_inputs"]),
            {
                "Brown Oxidation Amount",
                "Contact Polish Amount",
                "Displacement Strength",
                "Macro Normal Strength",
                "Micro Normal Strength",
                "Worked Normal Strength",
                "Scale Normal Strength",
                "Surface Strength",
            },
        )
        self.assertEqual(
            payload["group_inputs"]["Brown Oxidation Amount"],
            0.0,
        )
        self.assertEqual(
            payload["group_inputs"]["Contact Polish Amount"],
            0.0,
        )
        for name, expected in (
            ("Displacement Strength", 1.0),
            ("Macro Normal Strength", 1.0),
            ("Micro Normal Strength", 0.52),
            ("Worked Normal Strength", 0.62),
            ("Scale Normal Strength", 0.78),
            ("Surface Strength", 0.85),
        ):
            self.assertAlmostEqual(
                payload["group_inputs"][name],
                expected,
                places=5,
            )
        self.assertAlmostEqual(payload["strap_dimensions"][0], 0.406, places=5)
        self.assertAlmostEqual(payload["strap_dimensions"][2], 0.041, places=5)
        self.assertAlmostEqual(
            payload["strap_dimensions"][1],
            profile["proof_geometry"]["measured_reference_strap"][
                "display_thickness_m"
            ],
            places=5,
        )
        self.assertTrue(
            {
                "sinc_iron_u_m",
                "sinc_iron_v_m",
                "sinc_iron_edge_mask",
                "sinc_seed",
                "sinc_iron_fabrication_scale",
            }.issubset(payload["strap_attributes"])
        )
        self.assertEqual(payload["strap_fabrication_scale"], 1.0)
        self.assertEqual(
            set(payload["target_objects"]),
            {
                "SM_GH018_FixedLeaf",
                "SM_GH018_MovingLeaf_Openwork",
                "SM_GH018_Pintle",
                "MovingKnuckle_01",
                "FixedKnuckle_02",
                "MovingKnuckle_03",
                "FixedKnuckle_04",
                "MovingKnuckle_05",
            },
        )
        for target in payload["target_objects"].values():
            self.assertEqual(
                target["materials"],
                ["IGGY_MAT_ReferenceForgedIron_v003"],
            )
            self.assertEqual(target["fabrication_scale"], 10.0)
            self.assertIn("IGGY_IronUV", target["uv_layers"])
        self.assertEqual(
            set(payload["fastener_topology"]),
            {
                "IGGY_IronProof_FacetedSquareNail",
                "IGGY_IronProof_FacetedDomedRivet",
            },
        )
        for topology in payload["fastener_topology"].values():
            self.assertGreater(topology["vertices"], 20)
            self.assertGreater(topology["polygons"], 20)
            self.assertTrue(topology["all_edges_two_manifold"])
        self.assertTrue(payload["response_sphere_all_smooth"])
        self.assertTrue(payload["response_cylinder_has_smooth_and_flat"])
        for proof_name in (
            "forged_iron_v1_specimen_gameplay_distance.png",
            "forged_iron_v1_specimen_normal.png",
            "forged_iron_v1_specimen_height.png",
            "forged_iron_v1_actual_hinge_clay_front.png",
            "forged_iron_v1_actual_hinge_front.png",
            "forged_iron_v1_actual_hinge_grazing.png",
            "forged_iron_v1_actual_hinge_base_colour.png",
            "forged_iron_v1_actual_hinge_worked_response.png",
        ):
            self.assertTrue((MATERIAL_ROOT / "output" / proof_name).is_file())


if __name__ == "__main__":
    unittest.main()
