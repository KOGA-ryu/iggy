from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "assets" / "creative" / "materials" / "sword_steel_v1"
BUILDER = PACKAGE / "build_historical_swords_clean_steel_response_v1.py"
CANONICAL = PACKAGE / "build_sword_steel_v1.py"
OUTPUT = PACKAGE / "output" / "historical_swords_clean_steel_response_v1"
MANIFEST = OUTPUT / "manifest.json"
BOARD = OUTPUT / "historical_swords_clean_steel_response_board.png"
CANONICAL_MACRO = PACKAGE / "output" / "sword_steel_v1_macro_polish.png"
CANONICAL_GRIND = PACKAGE / "output" / "sword_steel_v1_grind_field.png"
EXPECTED_SWORDS = (
    "ArmingSword",
    "BastardSword",
    "LongSword",
    "ClaymoreSword",
)
EXPECTED_VIEWS = ("neutral", "grazing", "gameplay", "regions")
EXPECTED_CANONICAL_SHA256 = (
    "76030d14d442ad2c1d4550b6d150c82caa536a8ac63ee8a94208a9e51ead7bbb"
)
EXPECTED_SOURCE_SHA256 = (
    "b090eddf53f63cb8fd8a3bfac0c3dbe36749d934ca079595485629c2a2eb55de"
)
EXPECTED_MACRO_SHA256 = (
    "dcffc56eed667746218c001503e923defcc9b9a8855b3ace371237d2cb084e35"
)
EXPECTED_GRIND_SHA256 = (
    "52c7b2f15c8a176d67d9c0d6e722f49f3b2be86c511eea7a6a438eca902f6c63"
)


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


class HistoricalSwordResponseBlueprintTests(unittest.TestCase):
    def test_canonical_shader_authority_is_frozen(self) -> None:
        self.assertEqual(sha256_file(CANONICAL), EXPECTED_CANONICAL_SHA256)
        self.assertEqual(sha256_file(CANONICAL_MACRO), EXPECTED_MACRO_SHA256)
        self.assertEqual(sha256_file(CANONICAL_GRIND), EXPECTED_GRIND_SHA256)

    def test_builder_reuses_graph_and_owns_only_geometry_adapter(self) -> None:
        self.assertTrue(BUILDER.is_file())
        source = BUILDER.read_text()
        for token in (
            "load_canonical_builder",
            "canonical.load_contracts",
            "canonical.generate_macro_polish_field",
            "canonical.generate_grind_field",
            "canonical.build_sword_steel_material",
            "author_blade_uv_and_regions",
            "classify_longitudinal_face",
            "IGGY_BladeUV",
            "sinc_sword_region",
            "DIAGNOSTIC_MULTI_GEOMETRY_RESPONSE_NOT_ACCEPTANCE",
        ):
            self.assertIn(token, source)
        for prohibited in (
            "ShaderNodeBsdfPrincipled",
            "ShaderNodeTexNoise",
            "ShaderNodeTexVoronoi",
            "ShaderNodeTexWave",
            "save_as_mainfile",
            "save_mainfile",
            "rust_mask",
            "damage_mask",
        ):
            self.assertNotIn(prohibited, source)


@unittest.skipUnless(MANIFEST.is_file(), "historical sword response board not generated")
class HistoricalSwordResponseOutputTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.payload = json.loads(MANIFEST.read_text())

    def test_boundary_and_authorities_remain_exact(self) -> None:
        payload = self.payload
        self.assertEqual(
            payload["status"],
            "DIAGNOSTIC_MULTI_GEOMETRY_RESPONSE_NOT_ACCEPTANCE",
        )
        self.assertEqual(payload["canonical_builder"]["sha256_before"], EXPECTED_CANONICAL_SHA256)
        self.assertEqual(payload["canonical_builder"]["sha256_after"], EXPECTED_CANONICAL_SHA256)
        self.assertEqual(payload["source"]["sha256_before"], EXPECTED_SOURCE_SHA256)
        self.assertEqual(payload["source"]["sha256_after"], EXPECTED_SOURCE_SHA256)
        self.assertFalse(payload["source_materials_consumed"])
        self.assertFalse(payload["source_geometry_modified"])
        self.assertFalse(payload["saved_blend_created"])
        self.assertFalse(payload["manual_acceptance_established"])
        self.assertFalse(payload["unreal_parity_verified"])
        self.assertFalse(payload["uses_ai_generated_imagery"])
        self.assertFalse(payload["uses_damage"])

    def test_one_canonical_material_and_byte_identical_maps(self) -> None:
        material = self.payload["material"]
        self.assertEqual(material["material_name"], "IGGY_MAT_SwordSteel_CleanGround_v001")
        self.assertEqual(material["principled_count"], 1)
        self.assertEqual(material["mix_shader_count"], 0)
        self.assertEqual(material["image_texture_count"], 2)
        self.assertEqual(material["tangent_count"], 1)
        self.assertEqual(material["bump_count"], 1)
        self.assertEqual(material["shared_consumer_count"], 4)
        self.assertEqual(self.payload["maps"]["macro"]["sha256"], EXPECTED_MACRO_SHA256)
        self.assertEqual(self.payload["maps"]["grind"]["sha256"], EXPECTED_GRIND_SHA256)

    def test_geometry_semantics_are_one_hot_and_bounded(self) -> None:
        self.assertEqual(list(self.payload["swords"]), list(EXPECTED_SWORDS))
        for name, sword in self.payload["swords"].items():
            counts = sword["region_polygon_counts"]
            self.assertGreater(counts["body"], 0, name)
            self.assertGreater(counts["bevel"], 0, name)
            self.assertEqual(sum(counts.values()), sword["polygon_count"])
            self.assertEqual(sword["one_hot_loop_error_count"], 0)
            self.assertEqual(sword["uv_layer"], "IGGY_BladeUV")
            self.assertEqual(sword["region_attribute"], "sinc_sword_region")
        self.assertEqual(self.payload["swords"]["ArmingSword"]["region_polygon_counts"]["fuller"], 0)
        self.assertGreater(self.payload["swords"]["BastardSword"]["region_polygon_counts"]["fuller"], 4)
        self.assertEqual(self.payload["swords"]["LongSword"]["region_polygon_counts"]["fuller"], 0)
        self.assertGreater(self.payload["swords"]["ClaymoreSword"]["region_polygon_counts"]["fuller"], 0)
        self.assertIn(
            self.payload["swords"]["ArmingSword"]["edge_lane_state"],
            {"modelled", "absent_zero_width_source_edge"},
        )

    def test_board_and_sixteen_panels_are_hash_locked(self) -> None:
        self.assertEqual(png_dimensions(BOARD), (2880, 960))
        self.assertEqual(self.payload["board"]["sha256"], sha256_file(BOARD))
        panels = self.payload["panels"]
        self.assertEqual(len(panels), 16)
        self.assertEqual(len(list((OUTPUT / "panels").glob("*.png"))), 16)
        self.assertEqual(
            set(panels),
            {f"{sword}:{view}" for sword in EXPECTED_SWORDS for view in EXPECTED_VIEWS},
        )
        for artifact in panels.values():
            path = Path(artifact["path"])
            self.assertEqual(artifact["sha256"], sha256_file(path))

    def test_visual_review_is_diagnostic_not_acceptance(self) -> None:
        review = self.payload["visual_review"]
        self.assertNotEqual(review["decision"], "pending_visual_review")
        self.assertIn("BastardSword", review["strongest_geometry_separation"])
        self.assertIn("LongSword", review["weakest_geometry_separation"])
        self.assertFalse(review["manual_acceptance_established"])


if __name__ == "__main__":
    unittest.main()
