from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
)
ASSEMBLY_SCRIPT = (
    MATERIAL_ROOT / "build_cumulative_intact_hinge_preview_v1.py"
)


class CumulativeIntactHingeBlueprintTests(unittest.TestCase):
    def test_selected_donors_have_one_explicit_cumulative_owner(self):
        self.assertTrue(
            ASSEMBLY_SCRIPT.is_file(),
            "the cumulative intact-hinge assembly builder is not implemented",
        )
        source = ASSEMBLY_SCRIPT.read_text()
        for required_owner in (
            "component_tangent_uv",
            "broad_forging_plane_field",
            "height_to_normal_nonperiodic",
            "continuous_oxide_layer_fields",
            "ShaderNodeMixShader",
            "EXPOSED_CONDUCTOR = 0.0",
            "CONDUCTOR_F0_LINEAR = (0.56, 0.57, 0.58)",
            "CONDUCTOR_ROUGHNESS = 0.38",
            "ANISOTROPY = 0.18",
            "GIANT_LEAF_FORGING_AMPLITUDE_M = 0.006",
            '"field_coordinate_mode": field_coordinate_mode',
            "span_x * 1.12",
        ):
            self.assertIn(required_owner, source)

    def test_intact_assembly_prohibits_legacy_damage_and_motif_owners(self):
        self.assertTrue(ASSEMBLY_SCRIPT.is_file())
        source = ASSEMBLY_SCRIPT.read_text()
        for prohibited in (
            "scale_plates",
            "_irregular_plate",
            "contact_polish",
            "edge_wear",
            "rust_mask",
            "damage_mask",
        ):
            self.assertNotIn(prohibited, source)


if __name__ == "__main__":
    unittest.main()
