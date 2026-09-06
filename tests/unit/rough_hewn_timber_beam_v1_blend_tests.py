from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
ASSET_ROOT = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "architecture"
    / "structural"
    / "rough_hewn_timber_beam_v1"
)
PROFILE_PATH = ASSET_ROOT / "profiles" / "rough_hewn_timber_beam_v1.json"
BUILDER_PATH = ASSET_ROOT / "build_rough_hewn_timber_beam_v1.py"
README_PATH = ASSET_ROOT / "README.md"
RESEARCH_PATH = ASSET_ROOT / "RESEARCH_INTENT.md"
REVIEW_PATH = ASSET_ROOT / "QUALITY_REVIEW.md"
OUTPUT_ROOT = ASSET_ROOT / "output"
BLEND_PATH = OUTPUT_ROOT / "rough_hewn_timber_beam_v1.blend"
MANIFEST_PATH = OUTPUT_ROOT / "rough_hewn_timber_beam_v1_manifest.json"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")


class RoughHewnTimberBeamV1SourceTests(unittest.TestCase):
    def test_measured_profile_is_authored_and_single_asset_scoped(self):
        profile = json.loads(PROFILE_PATH.read_text())
        self.assertEqual(
            profile["asset_id"],
            "world_asset_001_rough_hewn_timber_beam",
        )
        self.assertTrue(profile["source_policy"]["single_asset_scope"])
        self.assertTrue(profile["source_policy"]["plain_clay_acceptance_first"])
        self.assertFalse(profile["source_policy"]["ai_generated_imagery"])
        dimensions = profile["dimensions_m"]
        self.assertAlmostEqual(dimensions["length"], 4.2)
        self.assertGreater(dimensions["start_width"], dimensions["end_width"])
        self.assertGreater(
            dimensions["start_height"],
            dimensions["end_height"],
        )
        self.assertGreaterEqual(
            profile["primary_form"]["longitudinal_sections"],
            80,
        )
        faces = profile["hewing"]["faces"]
        self.assertEqual(set(faces), {"front", "back", "top", "bottom"})
        self.assertGreaterEqual(
            sum(len(face["marks"]) for face in faces.values()),
            50,
        )
        self.assertEqual(len(profile["anatomy"]["knots"]), 2)
        self.assertEqual(len(profile["anatomy"]["end_checks"]), 5)
        self.assertEqual(len(profile["joinery_reserved_zones"]), 3)

    def test_builder_and_written_contract_name_the_causal_layers(self):
        source = BUILDER_PATH.read_text()
        for token in (
            "build_hewn_volume",
            "evaluate_hewing_depth",
            "planarize_authored_hewing_regions",
            "create_end_check_cutter",
            "create_knot_socket",
            "write_geometry_attributes",
            "create_review_package",
            "validate_evaluated_master",
            "IGGY_WA001_RoughHewnTimberBeam_Master",
            "sinc_timber_u_m",
            "sinc_timber_face_id",
            "sinc_joinery_reserved",
        ):
            self.assertIn(token, source)
        written = "\n".join(
            path.read_text()
            for path in (README_PATH, RESEARCH_PATH, REVIEW_PATH)
        ).lower()
        for phrase in (
            "broad axe",
            "random adze",
            "four hewn faces",
            "joinery",
            "end check",
            "material-only",
        ):
            self.assertIn(phrase, written)


@unittest.skipUnless(BLENDER.is_file(), "Blender is not installed")
class RoughHewnTimberBeamV1BlendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not BLEND_PATH.is_file():
            raise AssertionError(f"Missing beam blend: {BLEND_PATH}")
        if not MANIFEST_PATH.is_file():
            raise AssertionError(f"Missing beam manifest: {MANIFEST_PATH}")
        cls.manifest = json.loads(MANIFEST_PATH.read_text())

    def blender_payload(self) -> dict:
        script = r'''
import bpy
import bmesh
import json

master = bpy.data.objects["IGGY_WA001_RoughHewnTimberBeam_Master"]
clean = bpy.data.objects["IGGY_WA001_RoughHewnTimberBeam_CleanSource"]
depsgraph = bpy.context.evaluated_depsgraph_get()
evaluated = master.evaluated_get(depsgraph)
mesh = bpy.data.meshes.new_from_object(evaluated)
topology = bmesh.new()
topology.from_mesh(mesh)

def scalar_values(name):
    attribute = mesh.attributes[name]
    return [
        int(item.value) if attribute.data_type == "INT" else float(item.value)
        for item in attribute.data
    ]

payload = {
    "master_dimensions": [float(value) for value in master.dimensions],
    "master_origin": [float(value) for value in master.location],
    "source_vertices": len(master.data.vertices),
    "source_polygons": len(master.data.polygons),
    "evaluated_vertices": len(mesh.vertices),
    "evaluated_polygons": len(mesh.polygons),
    "modifiers": [
        {
            "name": modifier.name,
            "type": modifier.type,
            "show_viewport": bool(modifier.show_viewport),
            "show_render": bool(modifier.show_render),
        }
        for modifier in master.modifiers
    ],
    "clean_modifier_count": len(clean.modifiers),
    "clean_hidden_render": bool(clean.hide_render),
    "clean_hidden_viewport": bool(clean.hide_viewport),
    "attributes": {
        attribute.name: {
            "domain": attribute.domain,
            "data_type": attribute.data_type,
        }
        for attribute in mesh.attributes
    },
    "face_ids": sorted(set(scalar_values("sinc_timber_face_id"))),
    "u_values": scalar_values("sinc_timber_u_m"),
    "joinery_values": sorted(set(scalar_values("sinc_joinery_reserved"))),
    "joinery_zones": json.loads(master["iggy_joinery_reserved_zones"]),
    "knot_count": int(master["iggy_knot_count"]),
    "check_count": int(master["iggy_end_check_count"]),
    "edge_chip_count": int(master["iggy_edge_chip_count"]),
    "manifold": all(edge.is_manifold for edge in topology.edges),
    "material_names": sorted(
        slot.material.name
        for slot in master.material_slots
        if slot.material is not None
    ),
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
            entry
            for entry in completed.stdout.splitlines()
            if entry.startswith("IGGY_JSON=")
        )
        return json.loads(line.removeprefix("IGGY_JSON="))

    def test_saved_master_preserves_geometry_and_modifier_contract(self):
        payload = self.blender_payload()
        dimensions = payload["master_dimensions"]
        self.assertAlmostEqual(dimensions[0], 4.2, delta=0.035)
        self.assertAlmostEqual(dimensions[1], 0.3, delta=0.025)
        self.assertAlmostEqual(dimensions[2], 0.34, delta=0.025)
        self.assertTrue(
            all(abs(value) < 1.0e-7 for value in payload["master_origin"])
        )
        self.assertGreater(payload["source_vertices"], 3000)
        self.assertGreater(payload["source_polygons"], 3000)
        self.assertGreater(payload["evaluated_vertices"], 4000)
        self.assertGreater(payload["evaluated_polygons"], 4000)
        self.assertEqual(payload["clean_modifier_count"], 0)
        self.assertTrue(payload["clean_hidden_render"])
        self.assertTrue(payload["clean_hidden_viewport"])

        modifiers = payload["modifiers"]
        modifier_types = [entry["type"] for entry in modifiers]
        self.assertGreaterEqual(modifier_types.count("BOOLEAN"), 9)
        self.assertEqual(modifier_types[-2:], ["BEVEL", "WEIGHTED_NORMAL"])
        self.assertTrue(
            all(entry["show_viewport"] for entry in modifiers),
            modifiers,
        )
        self.assertTrue(
            all(entry["show_render"] for entry in modifiers),
            modifiers,
        )
        self.assertTrue(payload["manifold"])
        self.assertEqual(
            payload["material_names"],
            ["IGGY_MAT_WA001_NeutralClay"],
        )

    def test_attributes_anatomy_and_joinery_zones_survive_evaluation(self):
        payload = self.blender_payload()
        expected_attributes = {
            "sinc_timber_u_m",
            "sinc_timber_face_id",
            "sinc_joinery_reserved",
            "sinc_hewing_depth_m",
            "sinc_end_distance_m",
            "IGGY_TimberUV",
        }
        self.assertTrue(expected_attributes.issubset(payload["attributes"]))
        self.assertEqual(
            payload["attributes"]["sinc_timber_face_id"],
            {"domain": "FACE", "data_type": "INT"},
        )
        self.assertEqual(
            payload["attributes"]["sinc_joinery_reserved"],
            {"domain": "FACE", "data_type": "FLOAT"},
        )
        self.assertTrue(set(range(6)).issubset(payload["face_ids"]))
        self.assertEqual(payload["joinery_values"], [0.0, 1.0])
        self.assertLess(min(payload["u_values"]), 1.0e-6)
        self.assertGreater(max(payload["u_values"]), 4.19)
        self.assertEqual(len(payload["joinery_zones"]), 3)
        self.assertEqual(payload["knot_count"], 2)
        self.assertEqual(payload["check_count"], 5)
        self.assertEqual(payload["edge_chip_count"], 2)

    def test_manifest_and_review_package_are_complete(self):
        manifest = self.manifest
        self.assertEqual(
            manifest["schema"],
            "iggy3d.world_asset.rough_hewn_timber_beam.build.v1",
        )
        self.assertEqual(manifest["repair_pass_count"], 10)
        self.assertEqual(manifest["asset_scope"], "one_master_beam")
        self.assertEqual(manifest["geometry"]["hewn_face_count"], 4)
        self.assertEqual(manifest["geometry"]["knot_count"], 2)
        self.assertEqual(manifest["geometry"]["end_check_count"], 5)
        self.assertEqual(manifest["geometry"]["edge_chip_count"], 2)
        required = {
            "front",
            "back",
            "top",
            "bottom",
            "end_left",
            "end_right",
            "perspective_a",
            "perspective_b",
            "wireframe",
            "clay_silhouette",
            "toolmark_grazing",
            "end_checks_closeup",
        }
        self.assertEqual(set(manifest["renders"]), required)
        for record in manifest["renders"].values():
            path = Path(record["path"])
            self.assertTrue(path.is_file())
            self.assertGreater(path.stat().st_size, 60_000)
            self.assertEqual(record["width"], 1400)
            self.assertEqual(record["height"], 760)
        self.assertTrue(REVIEW_PATH.is_file())
        review = REVIEW_PATH.read_text().lower()
        self.assertIn("repair-pass count", review)
        self.assertIn("remaining material-only details", review)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
