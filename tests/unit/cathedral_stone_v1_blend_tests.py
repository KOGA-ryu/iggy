from __future__ import annotations

import json
import math
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
    / "cathedral_stone_v1"
)
OUTPUT_ROOT = MATERIAL_ROOT / "output"
BLEND_PATH = OUTPUT_ROOT / "cathedral_stone_v1.blend"
MANIFEST_PATH = OUTPUT_ROOT / "cathedral_stone_v1_blender_manifest.json"
MATERIAL_NAME = "IGGY_MAT_CathedralStone_v006"
GROUP_NAME = "IGGY_SH_CathedralStone_v006"


class CathedralStoneV1BlendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not BLEND_PATH.is_file():
            raise AssertionError(f"Missing cathedral-stone blend: {BLEND_PATH}")
        if Path(bpy.data.filepath).resolve() != BLEND_PATH.resolve():
            bpy.ops.wm.open_mainfile(filepath=str(BLEND_PATH))
        cls.manifest = json.loads(MANIFEST_PATH.read_text())

    def test_canonical_v6_material_contains_the_full_visual_hierarchy(self):
        self.assertIn(MATERIAL_NAME, bpy.data.materials)
        self.assertIn(GROUP_NAME, bpy.data.node_groups)
        material = bpy.data.materials[MATERIAL_NAME]
        group = bpy.data.node_groups[GROUP_NAME]
        self.assertEqual(
            material["iggy_material_schema"],
            "iggy3d.material.cathedral_stone_v1.v6",
        )
        self.assertEqual(
            material["iggy_relief_proxy_measurement_id"],
            "sabucina_calcarenite_surface_topography_proxy",
        )
        self.assertTrue(
            {
                "IGGY_ObjectCoordinateAnchor",
                "IGGY_WorldXZTileCoordinate",
                "IGGY_64mmBodyDetailCoordinate",
                "IGGY_91mmBodyDetailCoordinate",
                "IGGY_StoneBodyMasks",
                "IGGY_StoneBodyMasksDecorrelated",
                "IGGY_StoneBodyHeight",
                "IGGY_StoneBodyHeightDecorrelated",
                "IGGY_Blend64mmAnd91mmBodyHeight",
                "IGGY_256mmToolingCoordinate",
                "IGGY_ToolingDetail",
                "IGGY_SelectToolingDensityVariant",
                "IGGY_ToolingColorLinework",
                "IGGY_ToolingRoughnessLinework",
                "IGGY_SeparateStylizationLanes",
                "IGGY_CameraDistance",
                "IGGY_DetailFullAt030mZeroAt3p5m",
                "IGGY_DistanceFadedStoneBodyHeight",
                "IGGY_DistanceFadedTooling",
                "IGGY_SelectiveStructuralInk",
                "IGGY_SelectiveArrisHighlight",
            }.issubset(group.nodes.keys())
        )
        self.assertNotIn("IGGY_WorldCoordinateAnchor", group.nodes)
        self.assertEqual(
            len(
                [
                    node
                    for node in group.nodes
                    if node.bl_idname == "ShaderNodeCameraData"
                ]
            ),
            1,
        )
        self.assertFalse(
            [
                node
                for node in group.nodes
                if node.bl_idname
                in {"ShaderNodeTexNoise", "ShaderNodeTexVoronoi"}
            ]
        )
        object_coordinate = group.nodes["IGGY_ObjectCoordinateAnchor"]
        self.assertEqual(object_coordinate.bl_idname, "ShaderNodeTexCoord")
        self.assertTrue(
            any(
                link.from_node == object_coordinate
                and link.from_socket.name == "Object"
                and link.to_node.name == "IGGY_WorldXZMetres"
                for link in group.links
            )
        )
        self.assertAlmostEqual(
            float(material["iggy_detail_full_distance_m"]),
            0.30,
            places=8,
        )
        self.assertAlmostEqual(
            float(material["iggy_detail_zero_distance_m"]),
            3.5,
            places=8,
        )
        self.assertTrue(material["iggy_stylization_lanes_live"])
        self.assertFalse(material["iggy_unmeasured_tool_depth_authored"])
        self.assertEqual(
            float(material["iggy_tooling_detail_span_m"]),
            0.256,
        )
        bump = material.node_tree.nodes["Metre_Scale_Stone_Height"]
        self.assertAlmostEqual(
            float(bump.inputs["Strength"].default_value),
            1.0,
            places=8,
        )
        self.assertTrue(
            math.isclose(
                float(bump.inputs["Distance"].default_value),
                0.00113,
                abs_tol=1.0e-9,
            )
        )

    def test_fixture_is_sixty_nine_closed_measured_stone_volumes(self):
        collection = bpy.data.collections["IGGY_CathedralStoneProof"]
        blocks = [
            obj
            for obj in collection.objects
            if obj.name.startswith("IGGY_MeasuredAshlar_C")
        ]
        mortars = [
            obj
            for obj in collection.objects
            if obj.name.startswith("IGGY_MeasuredAshlarMortar_")
        ]
        self.assertEqual(len(blocks), 69)
        self.assertEqual(len(mortars), 68)
        self.assertEqual(
            {int(obj["iggy_source_design_index"]) for obj in blocks},
            set(range(1, 41)),
        )
        for obj in blocks:
            self.assertEqual(len(obj.data.polygons), 6)
            self.assertFalse(obj.modifiers)
            self.assertTrue(
                0.78 - 1.0e-6
                <= float(obj.dimensions.x)
                <= 1.13 + 1.0e-6
            )
            self.assertTrue(
                0.17 - 1.0e-6
                <= float(obj.dimensions.y)
                <= 0.30 + 1.0e-6
            )
            self.assertTrue(
                0.32 - 1.0e-6
                <= float(obj.dimensions.z)
                <= 0.43 + 1.0e-6
            )

    def test_geometry_attributes_exist_and_unsupported_lanes_default_off(self):
        expected = set(self.manifest["validation"]["geometry_attributes"])
        collection = bpy.data.collections["IGGY_CathedralStoneProof"]
        blocks = [
            obj
            for obj in collection.objects
            if obj.name.startswith("IGGY_MeasuredAshlar_C")
        ]
        for obj in blocks:
            self.assertTrue(expected.issubset(obj.data.attributes.keys()))
            for attribute_name in (
                "iggy_fracture_interior",
                "iggy_lichen_mask",
                "iggy_damp_mask",
                "iggy_traversal_id",
            ):
                self.assertTrue(
                    all(
                        float(entry.value) == 0.0
                        for entry in obj.data.attributes[
                            attribute_name
                        ].data
                    )
                )
            for attribute_name in (
                "iggy_tool_angle_deg",
                "iggy_tool_spacing_m",
                "iggy_tool_density_variant",
            ):
                self.assertIn(attribute_name, obj.data.attributes)
        runtime = self.manifest["validation"]["runtime_material"]
        self.assertTrue(runtime["measured_stone_body_relief_authored"])
        self.assertFalse(runtime["unmeasured_relief_authored"])

    def test_maps_are_packed_at_final_resolution_and_proofs_exist(self):
        group = bpy.data.node_groups[GROUP_NAME]
        for node_name in (
            "IGGY_BaseColor",
            "IGGY_ORM",
            "IGGY_StoneBodyMasks",
            "IGGY_StoneBodyHeight",
            "IGGY_StoneBodyHeightDecorrelated",
            "IGGY_ToolingDetail",
        ):
            image = group.nodes[node_name].image
            self.assertIsNotNone(image)
            self.assertIsNotNone(image.packed_file)
            self.assertEqual(tuple(image.size), (1024, 1024))
        self.assertEqual(
            self.manifest["schema"],
            "iggy3d.material.cathedral_stone_v1.blender.v6",
        )
        self.assertEqual(
            self.manifest["texture_manifest"]["schema"],
            "iggy3d.material.cathedral_stone_v1.v6",
        )
        self.assertTrue(
            self.manifest["constraints"]["uses_object_space_metre_coordinates"]
        )
        self.assertTrue(
            self.manifest["constraints"]["uses_live_ink_and_highlight_lanes"]
        )
        self.assertFalse(
            self.manifest["constraints"]["unreal_runtime_parity_verified"]
        )
        render_groups = self.manifest["renders"]
        self.assertEqual(
            set(render_groups),
            {"neutral_geometry", "layered_surface"},
        )
        render_count = 0
        for group_manifest in render_groups.values():
            for render in group_manifest.values():
                path = Path(render["path"])
                self.assertTrue(path.is_file())
                self.assertGreater(path.stat().st_size, 50_000)
                render_count += 1
        self.assertEqual(render_count, 12)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
