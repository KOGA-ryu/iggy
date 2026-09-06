from __future__ import annotations

import ast
import hashlib
import json
from pathlib import Path
import struct
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
DEMANDS = PACKAGE / "CODED_DEMANDS.md"
SCRIPT = PACKAGE / "compare_v004_2_geometry_normal_control_v1.py"
SOURCE = (
    PACKAGE
    / "output"
    / "forged_iron_connected_oxide_candidate_v004_2"
    / "forged_iron_connected_oxide_candidate_v004_2.blend"
)
OUTPUT = (
    PACKAGE
    / "output"
    / "forged_iron_v004_2_geometry_normal_control_v1"
)
MANIFEST = OUTPUT / "manifest.json"
BOARD = OUTPUT / "forged_iron_v004_2_geometry_normal_control_board.png"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as handle:
        header = handle.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(path)
    return struct.unpack(">II", header[16:24])


class ForgedIronGeometryNormalBlueprintTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.script_text = SCRIPT.read_text()
        cls.demands_text = DEMANDS.read_text()
        cls.syntax = ast.parse(cls.script_text)

    def test_demand_precedes_executable(self) -> None:
        self.assertIn("DEM-PRODUCTION-015", self.demands_text)
        self.assertIn(
            "Test geometry normals against the inherited broad normal",
            self.demands_text,
        )
        self.assertTrue(SCRIPT.is_file())

    def test_binary_rows_and_single_socket_contract_are_literal(self) -> None:
        for token in (
            '"A_v004_2_control"',
            '"B_geometry_normals_only"',
            'SOURCE_GROUP = "IGGY_SH_ConnectedOxideForgedIron_v004_2"',
            'NORMAL_NODE = "Broad_Forging_Normal_Decode"',
            "CONTROL_NORMAL_STRENGTH = 1.0",
            "GEOMETRY_NORMAL_STRENGTH = 0.0",
            '"socket": "Broad_Forging_Normal_Decode:0:Strength"',
            '"source": CONTROL_NORMAL_STRENGTH',
            '"candidate": GEOMETRY_NORMAL_STRENGTH',
        ):
            self.assertIn(token, self.script_text)

        rows_assignments = [
            node
            for node in self.syntax.body
            if isinstance(node, ast.Assign)
            and any(isinstance(target, ast.Name) and target.id == "ROWS" for target in node.targets)
        ]
        self.assertEqual(len(rows_assignments), 1)
        self.assertIsInstance(rows_assignments[0].value, ast.Tuple)
        self.assertEqual(len(rows_assignments[0].value.elts), 2)

    def test_experiment_cannot_save_or_invent_a_replacement_surface(self) -> None:
        for prohibited in (
            "save_as_mainfile",
            "save_mainfile",
            "ShaderNodeTexNoise",
            "ShaderNodeTexVoronoi",
            "height_to_normal",
            "make_float_image",
            "bpy.data.images.new",
        ):
            self.assertNotIn(prohibited, self.script_text)
        for required in (
            'scene.render.use_persistent_data = False',
            '"saved_blend_created": False',
            '"uses_new_texture_or_pattern": False',
            '"applied_normal_amount"',
            '"source_normal"',
        ):
            if required == 'scene.render.use_persistent_data = False':
                self.assertIn(required, (PACKAGE / "compare_v004_1_reflection_routing_v1.py").read_text())
            else:
                self.assertIn(required, self.script_text)

    def test_all_required_actual_asset_views_are_present(self) -> None:
        for panel in (
            "close_neutral",
            "grazing_left",
            "grazing_right",
            "pivot_close",
            "gameplay",
            "applied_normal_amount",
            "source_normal",
        ):
            self.assertIn(panel, self.script_text)


@unittest.skipUnless(MANIFEST.is_file(), "geometry-normal control not generated")
class ForgedIronGeometryNormalOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.payload = json.loads(MANIFEST.read_text())

    def test_boundary_and_source_hold(self) -> None:
        self.assertEqual(
            self.payload["schema"],
            "iggy3d.forged_iron_v004_2_geometry_normal_control.v1",
        )
        self.assertEqual(
            self.payload["status"],
            "EXPLORATORY_BROAD_NORMAL_REMOVAL_NOT_CANONICAL",
        )
        self.assertEqual(
            list(self.payload["rows"]),
            ["A_v004_2_control", "B_geometry_normals_only"],
        )
        self.assertTrue(self.payload["source_hashes"]["sources_unchanged"])
        self.assertEqual(self.payload["source_hashes"]["after"], sha256_file(SOURCE))
        self.assertFalse(self.payload["saved_blend_created"])
        self.assertFalse(any(OUTPUT.rglob("*.blend")))
        self.assertFalse(self.payload["uses_ai_generated_imagery"])
        self.assertFalse(self.payload["uses_new_texture_or_pattern"])
        self.assertFalse(self.payload["uses_condition"])

    def test_one_default_changes_without_topology_change(self) -> None:
        contract = self.payload["group_contract"]
        self.assertTrue(contract["topology_matches_source"])
        self.assertEqual(
            contract["default_differences"],
            [
                {
                    "socket": "Broad_Forging_Normal_Decode:0:Strength",
                    "source": 1.0,
                    "candidate": 0.0,
                }
            ],
        )
        for key in (
            "node_count",
            "link_count",
            "interface_socket_count",
            "topology_sha256",
        ):
            self.assertEqual(
                contract["source_signature"][key],
                contract["candidate_signature"][key],
            )

    def test_frozen_images_geometry_and_direction_response_match(self) -> None:
        frozen = self.payload["frozen_lane_contract"]
        self.assertTrue(frozen["all_component_lane_hashes_match"])
        self.assertTrue(frozen["source_normal_proof_decoded_pixels_identical"])
        for component in frozen["components"].values():
            self.assertTrue(all(component["lanes"].values()))
            self.assertTrue(component["mesh_matches"])
            self.assertTrue(component["uv_layers_match"])
        for row in self.payload["rows"].values():
            self.assertEqual(row["source_group_strength_after_row"], 1.0)
            self.assertEqual(row["direction_multiplier_after_row"], 1.0)
            self.assertEqual(len(row["components"]), 8)
            for component in row["components"].values():
                self.assertEqual(component["luster_amount_range"], [0.28, 0.28])
                self.assertTrue(component["all_images_packed"])

    def test_board_and_fourteen_tiles_are_hash_locked(self) -> None:
        self.assertEqual(png_dimensions(BOARD), (4480, 440))
        self.assertEqual(self.payload["board"]["resolution"], [4480, 440])
        self.assertEqual(self.payload["board"]["sha256"], sha256_file(BOARD))
        for row in self.payload["rows"].values():
            self.assertEqual(len(row["renders"]), 7)
            for render in row["renders"].values():
                path = Path(render["path"])
                self.assertEqual(png_dimensions(path), (640, 220))
                self.assertEqual(render["sha256"], sha256_file(path))

    def test_review_makes_only_a_recipe_decision(self) -> None:
        self.assertIn(
            self.payload["visual_review"]["decision"],
            {
                "retain_v004_2_broad_normal_recipe",
                "select_geometry_normals_only_recipe",
                "reject_both_normal_ownership_recipes",
            },
        )
        self.assertFalse(self.payload["visual_review"]["manual_acceptance_established"])
        self.assertFalse(self.payload["manual_acceptance_established"])
        self.assertFalse(self.payload["unreal_parity_verified"])
        self.assertTrue(self.payload["inherited_geometry_proof"]["regenerated"] is False)


if __name__ == "__main__":
    unittest.main()
