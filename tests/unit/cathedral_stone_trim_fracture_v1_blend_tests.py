#!/usr/bin/env python3
"""Saved-file tests for the measured chevron-voussoir Blender asset."""

from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import sys
import unittest

import bpy


REPO_ROOT = Path(__file__).resolve().parents[2]
PACKAGE = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "cathedral_stone_trim_fracture_v1"
)
OUTPUT = PACKAGE / "output"
PATTERN = json.loads(
    (
        PACKAGE / "patterns" / "cathedral_trim_fracture_atlas_v1.json"
    ).read_text()
)
MANIFEST_PATH = OUTPUT / "cathedral_stone_trim_fracture_v1_blender_manifest.json"
GROUP_NAME = "IGGY_SH_ChevronVoussoir_v002"
MATERIAL_NAME = "IGGY_MAT_ChevronVoussoir_v002"
PRODUCT_COLLECTION_NAME = "IGGY_ChevronVoussoirPortal_Product"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def product_objects() -> list[bpy.types.Object]:
    collection = bpy.data.collections.get(PRODUCT_COLLECTION_NAME)
    if collection is None:
        raise AssertionError("Measured chevron product collection is missing")
    return sorted(collection.objects, key=lambda obj: obj.name)


def link_exists(
    tree: bpy.types.NodeTree,
    from_node: str,
    from_socket: str,
    to_node: str,
    to_socket: str,
) -> bool:
    return any(
        link.from_node.name == from_node
        and link.from_socket.name == from_socket
        and link.to_node.name == to_node
        and link.to_socket.name == to_socket
        for link in tree.links
    )


class ChevronSavedBlendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if bpy.data.filepath == "":
            raise AssertionError("Test must run against the saved .blend file")
        cls.manifest = json.loads(MANIFEST_PATH.read_text())
        cls.objects = product_objects()
        cls.group = bpy.data.node_groups.get(GROUP_NAME)
        cls.material = bpy.data.materials.get(MATERIAL_NAME)

    def test_saved_file_and_scene_identify_the_measured_capability(self) -> None:
        self.assertEqual(
            Path(bpy.data.filepath).resolve(),
            (OUTPUT / "cathedral_stone_trim_fracture_v1.blend").resolve(),
        )
        scene = bpy.context.scene
        self.assertEqual(
            scene["iggy_asset_schema"],
            "iggy3d.asset.chevron_voussoir_portal.measured.v2",
        )
        self.assertEqual(
            scene["iggy_source_measurement"],
            "S01_old_sarum_catalogue_item_55",
        )
        self.assertEqual(scene["iggy_product_object_count"], 16)
        self.assertFalse(scene["iggy_fracture_enabled"])
        self.assertFalse(scene["iggy_damage_enabled"])
        self.assertEqual(scene["iggy_proof_set"], "all")
        self.assertEqual(
            json.loads(scene["iggy_selected_proofs"]),
            sorted(self.manifest["proofs"]),
        )

    def test_sixteen_individual_closed_wedges_replace_the_annular_ribbon(
        self,
    ) -> None:
        self.assertEqual(len(self.objects), 16)
        self.assertEqual(len({obj.data.name for obj in self.objects}), 16)
        for stone_id, obj in enumerate(self.objects):
            with self.subTest(obj=obj.name):
                self.assertEqual(obj.type, "MESH")
                self.assertEqual(obj["iggy_voussoir_id"], stone_id)
                self.assertEqual(
                    obj["iggy_role"],
                    "measured_chevron_voussoir",
                )
                self.assertEqual(
                    obj["iggy_source_id"],
                    "S01_old_sarum_catalogue_item_55",
                )
                self.assertEqual(tuple(round(v, 10) for v in obj.scale), (1.0, 1.0, 1.0))
                self.assertEqual(len(obj.modifiers), 0)
                self.assertTrue(obj.data["iggy_closed_volume"])
                self.assertEqual(obj.data["iggy_bevel_m"], 0.0)
                usage = [0] * len(obj.data.edges)
                lookup = {
                    tuple(sorted(edge.vertices)): edge.index
                    for edge in obj.data.edges
                }
                for polygon in obj.data.polygons:
                    for edge_key in polygon.edge_keys:
                        usage[lookup[tuple(sorted(edge_key))]] += 1
                self.assertTrue(all(count == 2 for count in usage))

    def test_catalogue_dimensions_and_centerline_joint_recompute(self) -> None:
        completion = PATTERN["authored_completion"]
        source = PATTERN["source_measurement"]
        body_angle = math.radians(completion["body_angle_deg"])
        pitch_angle = math.radians(completion["pitch_angle_deg"])
        inner_radius = completion["inner_radius_m"]
        outer_radius = completion["outer_radius_m"]
        inner_chord = 2.0 * inner_radius * math.sin(body_angle / 2.0)
        outer_chord = 2.0 * outer_radius * math.sin(body_angle / 2.0)
        centerline_gap = (
            inner_radius + source["height_m"] / 2.0
        ) * (pitch_angle - body_angle)
        self.assertAlmostEqual(inner_chord, 0.14, places=10)
        self.assertAlmostEqual(outer_chord, 0.1784852529, places=10)
        self.assertLessEqual(abs(outer_chord - source["outer_chord_m"]), 0.005)
        self.assertAlmostEqual(centerline_gap, 0.003, places=10)
        for obj in self.objects:
            self.assertEqual(obj["iggy_catalogue_radial_height_m"], 0.2)
            self.assertEqual(obj["iggy_catalogue_inner_chord_m"], 0.14)
            self.assertEqual(obj["iggy_catalogue_outer_chord_m"], 0.18)
            self.assertEqual(obj["iggy_catalogue_depth_m"], 0.27)
            self.assertEqual(obj["iggy_joint_gap_centerline_m"], 0.003)

    def test_front_geometry_carries_roll_hollow_quiet_and_linework_masks(
        self,
    ) -> None:
        required_point = {
            "iggy_voussoir_id": "INT",
            "iggy_material_phase": "FLOAT_VECTOR",
            "iggy_material_variant": "INT",
            "iggy_chevron_roll": "FLOAT",
            "iggy_chevron_hollow": "FLOAT",
            "iggy_chevron_quiet": "FLOAT",
            "iggy_chevron_ink": "FLOAT",
            "iggy_chevron_highlight": "FLOAT",
        }
        required_face = {
            "iggy_carved_trim": "BOOLEAN",
            "iggy_chevron_front": "BOOLEAN",
            "iggy_fracture_interior": "BOOLEAN",
        }
        for obj in self.objects:
            mesh = obj.data
            self.assertEqual(
                sorted(layer.name for layer in mesh.uv_layers),
                ["IGGY_StoneUV_A", "IGGY_StoneUV_B"],
            )
            for name, data_type in required_point.items():
                attribute = mesh.attributes.get(name)
                self.assertIsNotNone(attribute, name)
                self.assertEqual(attribute.domain, "POINT")
                self.assertEqual(attribute.data_type, data_type)
            for name, data_type in required_face.items():
                attribute = mesh.attributes.get(name)
                self.assertIsNotNone(attribute, name)
                self.assertEqual(attribute.domain, "FACE")
                self.assertEqual(attribute.data_type, data_type)
            front = mesh.attributes["iggy_chevron_front"]
            fracture = mesh.attributes["iggy_fracture_interior"]
            self.assertEqual(
                sum(bool(item.value) for item in front.data),
                mesh["iggy_front_face_count"],
            )
            self.assertTrue(all(not item.value for item in fracture.data))
            for name in (
                "iggy_chevron_roll",
                "iggy_chevron_hollow",
                "iggy_chevron_quiet",
                "iggy_chevron_ink",
                "iggy_chevron_highlight",
            ):
                values = [item.value for item in mesh.attributes[name].data]
                self.assertLessEqual(min(values), 0.001, name)
                self.assertGreaterEqual(max(values), 0.90, name)

    def test_shader_has_exact_live_texture_coordinate_and_identity_routes(
        self,
    ) -> None:
        self.assertIsNotNone(self.group)
        self.assertEqual(
            self.group["iggy_schema"],
            "iggy3d.shader.chevron_voussoir.measured_roll_hollow_roll.v2",
        )
        required_nodes = {
            "IGGY_StoneUV_A": "ShaderNodeUVMap",
            "IGGY_StoneUV_B": "ShaderNodeUVMap",
            "IGGY_BaseCoordinate_512mm": "ShaderNodeVectorMath",
            "IGGY_PrimaryCoordinate_64mm": "ShaderNodeVectorMath",
            "IGGY_SecondaryCoordinate_91mm": "ShaderNodeVectorMath",
            "IGGY_TrimBaseColor": "ShaderNodeTexImage",
            "IGGY_TrimORM": "ShaderNodeTexImage",
            "IGGY_BodyMasks64": "ShaderNodeTexImage",
            "IGGY_BodyMasks91": "ShaderNodeTexImage",
            "IGGY_BodyNormal64": "ShaderNodeTexImage",
            "IGGY_BodyHeight91": "ShaderNodeTexImage",
            "IGGY_OpenGLBodyNormal64mm": "ShaderNodeNormalMap",
            "IGGY_DecorrelatedBodyHeight91mm": "ShaderNodeBump",
            "IGGY_DetailFullAt030mZeroAt3p5m": "ShaderNodeMapRange",
            "IGGY_SelectiveChevronInk": "ShaderNodeMixRGB",
            "IGGY_SelectiveCrestHighlight": "ShaderNodeMixRGB",
        }
        for name, bl_idname in required_nodes.items():
            node = self.group.nodes.get(name)
            self.assertIsNotNone(node, name)
            self.assertEqual(node.bl_idname, bl_idname)
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_StoneUV_A",
                "UV",
                "IGGY_BaseCoordinate_512mm",
                "Vector",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_StoneUV_A",
                "UV",
                "IGGY_PrimaryCoordinate_64mm",
                "Vector",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_StoneUV_B",
                "UV",
                "IGGY_SecondaryCoordinate_91mm",
                "Vector",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_BodyNormal64",
                "Color",
                "IGGY_OpenGLBodyNormal64mm",
                "Color",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_OpenGLBodyNormal64mm",
                "Normal",
                "IGGY_DecorrelatedBodyHeight91mm",
                "Normal",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_DecorrelatedBodyHeight91mm",
                "Normal",
                "Group_Output",
                "Combined Normal",
            )
        )
        self.assertFalse(
            self.group.nodes["IGGY_DecorrelatedBodyHeight91mm"].invert
        )

    def test_every_image_is_packed_and_uses_the_declared_color_space(self) -> None:
        expected = {
            "IGGY_TrimBaseColor": "sRGB",
            "IGGY_TrimORM": "Non-Color",
            "IGGY_BodyMasks64": "Non-Color",
            "IGGY_BodyMasks91": "Non-Color",
            "IGGY_BodyNormal64": "Non-Color",
            "IGGY_BodyHeight91": "Non-Color",
        }
        for name, color_space in expected.items():
            node = self.group.nodes[name]
            self.assertIsNotNone(node.image, name)
            self.assertIsNotNone(node.image.packed_file, name)
            self.assertEqual(node.image.colorspace_settings.name, color_space)

    def test_all_declared_shader_attributes_are_live_in_the_group(self) -> None:
        expected = {
            "iggy_carved_trim",
            "iggy_chevron_front",
            "iggy_chevron_highlight",
            "iggy_chevron_hollow",
            "iggy_chevron_ink",
            "iggy_chevron_quiet",
            "iggy_chevron_roll",
            "iggy_material_phase",
            "iggy_material_variant",
            "iggy_voussoir_id",
        }
        nodes = [
            node
            for node in self.group.nodes
            if node.bl_idname == "ShaderNodeAttribute"
        ]
        self.assertEqual({node.attribute_name for node in nodes}, expected)
        for node in nodes:
            self.assertTrue(
                any(link.from_node == node for link in self.group.links),
                node.attribute_name,
            )

    def test_principled_material_consumes_group_color_roughness_and_normal(
        self,
    ) -> None:
        self.assertIsNotNone(self.material)
        tree = self.material.node_tree
        self.assertTrue(
            link_exists(
                tree,
                "Chevron_Voussoir_System",
                "Combined Color",
                "Chevron_Voussoir_BSDF",
                "Base Color",
            )
        )
        self.assertTrue(
            link_exists(
                tree,
                "Chevron_Voussoir_System",
                "Combined Roughness",
                "Chevron_Voussoir_BSDF",
                "Roughness",
            )
        )
        self.assertTrue(
            link_exists(
                tree,
                "Chevron_Voussoir_System",
                "Combined Normal",
                "Chevron_Voussoir_BSDF",
                "Normal",
            )
        )
        principled = tree.nodes["Chevron_Voussoir_BSDF"]
        self.assertEqual(principled.inputs["Metallic"].default_value, 0.0)
        self.assertAlmostEqual(principled.inputs["IOR"].default_value, 1.46)
        self.assertFalse(self.material["iggy_fracture_enabled"])
        self.assertFalse(self.material["iggy_damage_enabled"])

    def test_manifest_and_nine_proofs_survive_saved_file_reopen(self) -> None:
        self.assertEqual(
            self.manifest["schema"],
            "iggy3d.material.cathedral_stone_trim_fracture_v1.blender_manifest.v2",
        )
        self.assertEqual(self.manifest["geometry"]["object_count"], 16)
        self.assertTrue(self.manifest["geometry"]["all_meshes_manifold"])
        self.assertEqual(self.manifest["build_mode"]["proof_set"], "all")
        self.assertTrue(
            self.manifest["build_mode"]["canonical_candidate_complete"]
        )
        self.assertTrue(
            self.manifest["validation"]["complete_proof_set"]
        )
        self.assertEqual(len(self.manifest["proofs"]), 9)
        required = {
            "neutral_clay_front",
            "neutral_clay_grazing",
            "live_material_front",
            "live_material_grazing",
            "measured_close",
            "distance_read",
            "moulding_proof",
            "identity_proof",
            "wireframe_proof",
        }
        self.assertEqual(set(self.manifest["proofs"]), required)
        hashes = set()
        for proof in self.manifest["proofs"].values():
            path = OUTPUT / "proofs" / proof["filename"]
            self.assertTrue(path.is_file(), path)
            actual_hash = sha256(path)
            self.assertEqual(actual_hash, proof["sha256"])
            hashes.add(actual_hash)
        self.assertEqual(len(hashes), 9)
        self.assertTrue(all(self.manifest["validation"].values()) is False)
        for key, value in self.manifest["validation"].items():
            if key in {"damage_authored", "fracture_authored"}:
                self.assertFalse(value, key)
            elif key == "proof_count":
                self.assertEqual(value, 9)
            else:
                self.assertTrue(value, key)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
