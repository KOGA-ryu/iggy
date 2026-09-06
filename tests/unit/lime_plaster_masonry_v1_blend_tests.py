from __future__ import annotations

import json
from pathlib import Path
import sys
import unittest

import bpy


REPO_ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "lime_plaster_masonry_v1"
)
OUTPUT_ROOT = MATERIAL_ROOT / "output"
PROFILE_PATH = MATERIAL_ROOT / "profiles" / "lime_plaster_masonry_v1.json"
MANIFEST_PATH = OUTPUT_ROOT / "lime_plaster_masonry_v1_blender_manifest.json"
PUBLIC_GROUP = "IGGY_SH_LimePlasterMasonry_v001"
SURFACE_GROUP = "IGGY_SH_PlasterMasonrySurfaceData_v002"
NORMAL_GROUP = "IGGY_SH_PlasterMasonryNormalCombine_v002"
HEIGHT_GROUP = "IGGY_SH_PlasterMasonryHeight_v002"


def _nodes_of_type(tree, *node_types):
    return [
        node
        for node in tree.nodes
        if node.bl_idname in set(node_types)
    ]


class LimePlasterMasonryV1BlendTests(unittest.TestCase):
    def test_geometry_owns_every_transition_and_overlay_lane(self):
        wall = bpy.data.objects["IGGY_GiantHouse_PlasterMasonrySurface"]
        expected = {
            "iggy_material_phase",
            "iggy_material_variant",
            "iggy_plaster_coverage",
            "iggy_plaster_layer_state",
            "iggy_transition_edge",
            "iggy_traversal_id",
            "iggy_damp_mask",
            "iggy_soot_mask",
        }
        self.assertTrue(expected.issubset(wall.data.attributes.keys()))
        state = wall.data.attributes["iggy_plaster_layer_state"]
        self.assertEqual(state.domain, "FACE")
        values = [float(entry.value) for entry in state.data]
        self.assertLessEqual(min(values), 0.071)
        self.assertGreaterEqual(max(values), 0.939)
        for entry in json.loads(PROFILE_PATH.read_text())["layer_states"]:
            lower, upper = entry["range"]
            self.assertTrue(
                any(lower <= value <= upper for value in values),
                entry["name"],
            )
        for name in ("iggy_damp_mask", "iggy_soot_mask"):
            attribute = wall.data.attributes[name]
            self.assertTrue(
                all(float(entry.value) == 0.0 for entry in attribute.data)
            )

    def test_shader_reads_face_state_in_object_metres(self):
        group = bpy.data.node_groups[SURFACE_GROUP]
        attributes = {
            node.attribute_name
            for node in group.nodes
            if node.bl_idname == "ShaderNodeAttribute"
        }
        profile = json.loads(PROFILE_PATH.read_text())
        coordinate = profile["coordinate_contract"]
        expected = {
            coordinate["phase_attribute"],
            coordinate["plaster_coverage_attribute"],
            coordinate["layer_state_attribute"],
            coordinate["transition_edge_attribute"],
            coordinate["traversal_id_attribute"],
            coordinate["damp_attribute"],
            coordinate["soot_attribute"],
        }
        self.assertTrue(expected.issubset(attributes))
        coordinate_node = group.nodes["Object_Metre_Position"]
        self.assertEqual(coordinate_node.bl_idname, "ShaderNodeTexCoord")
        self.assertTrue(
            any(
                link.from_node == coordinate_node
                and link.from_socket.name == "Object"
                and link.to_node.name == "Wall_XZ_Metres"
                for link in group.links
            )
        )
        self.assertIn("Four_Metre_Tile_X", group.nodes)
        self.assertIn("Four_Metre_Tile_Z", group.nodes)
        self.assertEqual(
            group.nodes["Four_Metre_Tile_X"].inputs[1].default_value,
            0.25,
        )
        material = bpy.data.materials["IGGY_MAT_LimePlasterMasonry_v001"]
        self.assertEqual(material["iggy_tile_size_m"], 4.0)
        self.assertEqual(material["iggy_default_damp"], 0.0)
        self.assertEqual(material["iggy_default_soot"], 0.0)
        self.assertFalse(material["iggy_ai_reference_used"])

    def test_shader_has_separate_physical_responses_and_pbr_lanes(self):
        expected_groups = {
            PUBLIC_GROUP,
            SURFACE_GROUP,
            NORMAL_GROUP,
            HEIGHT_GROUP,
        }
        self.assertTrue(expected_groups.issubset(bpy.data.node_groups.keys()))
        public = bpy.data.node_groups[PUBLIC_GROUP]
        principled = _nodes_of_type(public, "ShaderNodeBsdfPrincipled")
        self.assertEqual(
            {node.name for node in principled},
            {
                "Stone_Dielectric_Response",
                "Mortar_Dielectric_Response",
                "Plaster_Dielectric_Response",
            },
        )
        self.assertTrue(
            all(
                node.inputs["Metallic"].default_value == 0.0
                for node in principled
            )
        )
        self.assertEqual(
            len(_nodes_of_type(public, "ShaderNodeNormalMap")),
            1,
        )
        surface = bpy.data.node_groups[SURFACE_GROUP]
        self.assertEqual(
            len(_nodes_of_type(surface, "ShaderNodeTexImage")),
            12,
        )
        normal = bpy.data.node_groups[NORMAL_GROUP]
        self.assertEqual(
            len(_nodes_of_type(normal, "ShaderNodeCameraData")),
            1,
        )
        self.assertIn(
            "Detail_Full_At_One_Metre_Zero_At_Eight",
            normal.nodes,
        )
        material = bpy.data.materials["IGGY_MAT_LimePlasterMasonry_v001"]
        self.assertEqual(
            len(_nodes_of_type(material.node_tree, "ShaderNodeDisplacement")),
            1,
        )
        self.assertEqual(
            len(_nodes_of_type(material.node_tree, "ShaderNodeBump")),
            0,
        )
        for tree_name in expected_groups:
            tree = bpy.data.node_groups[tree_name]
            self.assertEqual(
                len(
                    _nodes_of_type(
                        tree,
                        "ShaderNodeTexNoise",
                        "ShaderNodeTexVoronoi",
                    )
                ),
                0,
                tree_name,
            )

    def test_traversal_readability_is_supported_by_projecting_geometry(self):
        footholds = sorted(
            (
                obj
                for obj in bpy.data.objects
                if obj.name.startswith("IGGY_TraversalStone_")
                and not obj.name.endswith("_Mesh")
            ),
            key=lambda obj: obj.name,
        )
        self.assertEqual(len(footholds), 5)
        for foothold in footholds:
            self.assertEqual(
                foothold["iggy_surface_role"],
                "projecting_traversal_stone",
            )
            attribute = foothold.data.attributes["iggy_traversal_id"]
            self.assertTrue(
                all(float(entry.value) == 1.0 for entry in attribute.data)
            )
            self.assertGreater(foothold.dimensions.y, 0.10)

    def test_approved_prior_material_systems_remain_present(self):
        for name in (
            "IGGY_SH_ReferenceStructuralOak_v001",
            "IGGY_SH_ReferenceForgedIron_v001",
        ):
            self.assertIn(name, bpy.data.node_groups)
        for name in (
            "IGGY_MAT_ReferenceStructuralOakDoor_v001",
            "IGGY_MAT_ReferenceForgedIron_v001",
        ):
            self.assertIn(name, bpy.data.materials)
        manifest = json.loads(MANIFEST_PATH.read_text())
        self.assertTrue(
            manifest["validation"]["prior_contracts_unchanged"]
        )

    def test_final_maps_are_packed_and_visual_proofs_exist(self):
        packed = {
            image.name: tuple(image.size)
            for image in bpy.data.images
            if image.packed_file is not None
            and (
                image.name.startswith("lime_plaster")
                or image.name.startswith("giant_masonry")
            )
        }
        self.assertGreaterEqual(len(packed), 12)
        self.assertTrue(
            all(size == (1536, 1536) for size in packed.values()),
            packed,
        )
        manifest = json.loads(MANIFEST_PATH.read_text())
        self.assertFalse(
            manifest["source_policy"]["ai_generated_reference_capture"]
        )
        self.assertFalse(
            manifest["source_policy"]["ai_generated_runtime_texture"]
        )
        self.assertFalse(
            manifest["source_policy"][
                "raw_reference_pixels_used_as_runtime_texture"
            ]
        )
        self.assertEqual(
            set(manifest["renders"]),
            {
                "door_hero",
                "transition_close",
                "gameplay_distance",
                "grazing",
                "layer_states",
                "traversal",
                "normal",
                "height",
                "game_light",
            },
        )
        for render in manifest["renders"].values():
            path = Path(render["path"])
            self.assertTrue(path.is_file())
            self.assertGreater(path.stat().st_size, 100_000)
        validation = manifest["validation"]
        self.assertEqual(
            validation["coordinate_space"],
            "object_position_xz_metres",
        )
        self.assertEqual(validation["material_state_domain"], "FACE")
        self.assertEqual(validation["principled_count"], 3)
        self.assertEqual(validation["image_texture_count"], 12)
        self.assertEqual(validation["normal_map_count"], 1)
        self.assertEqual(validation["camera_distance_count"], 1)
        self.assertEqual(validation["displacement_count"], 1)
        self.assertEqual(validation["bump_count"], 0)
        self.assertEqual(validation["generic_noise_count"], 0)
        constraints = manifest["constraints"]
        self.assertTrue(constraints["uses_three_physical_dielectrics"])
        self.assertTrue(constraints["uses_form_and_detail_normals"])
        self.assertTrue(constraints["uses_distance_faded_detail"])
        self.assertTrue(constraints["uses_metre_displacement"])
        self.assertTrue(constraints["categorical_state_is_face_domain"])
        self.assertTrue(constraints["uses_object_space_metre_coordinates"])
        self.assertFalse(constraints["unreal_runtime_parity_verified"])


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
