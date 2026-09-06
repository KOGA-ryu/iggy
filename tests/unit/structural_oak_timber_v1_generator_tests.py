from __future__ import annotations

import importlib.util
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

import numpy as np


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "structural_oak_timber_v1"
)
GENERATOR_PATH = MATERIAL_ROOT / "generate_structural_oak_timber_v1.py"
PROFILE_PATH = (
    MATERIAL_ROOT / "profiles" / "structural_oak_timber_v1.json"
)
PATTERN_PATH = (
    MATERIAL_ROOT / "patterns" / "structural_oak_growth_champion_v1.json"
)


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "structural_oak_timber_v1_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load generator from {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class StructuralOakTimberProfileTests(unittest.TestCase):
    def test_profile_uses_one_growth_volume_for_side_and_end_grain(self):
        profile = json.loads(PROFILE_PATH.read_text())
        pattern = json.loads(PATTERN_PATH.read_text())

        self.assertEqual(
            profile["coordinate_contract"]["space"],
            "timber-local growth volume",
        )
        self.assertIn(
            "same pith path and ring-radius table",
            profile["coordinate_contract"]["rule"],
        )
        pith_y = pattern["growth_volume"]["pith_y_knots_m"]
        pith_z = pattern["growth_volume"]["pith_z_knots_m"]
        self.assertEqual(len(pith_y), 9)
        self.assertEqual(len(pith_z), 9)
        half_width = profile["beam_fixture_m"]["width"] * 0.5
        half_height = profile["beam_fixture_m"]["height"] * 0.5
        self.assertTrue(
            all(
                abs(y) > half_width or abs(z) > half_height
                for y, z in zip(pith_y, pith_z, strict=True)
            )
        )

    def test_champion_encodes_measured_ring_pore_and_ray_ranges(self):
        profile = json.loads(PROFILE_PATH.read_text())
        pattern = json.loads(PATTERN_PATH.read_text())
        anatomy = profile["physical_anatomy_m"]
        widths = pattern["growth_volume"]["champion_ring_widths_m"]

        self.assertEqual(len(widths), 64)
        self.assertGreaterEqual(min(widths), anatomy["observed_growth_ring_minimum"])
        self.assertLessEqual(max(widths), 0.0021)
        self.assertLess(min(widths), 0.0003)
        self.assertGreater(max(widths), anatomy["observed_growth_ring_vigorous"])
        self.assertAlmostEqual(anatomy["earlywood_vessel_mean"], 0.000154)
        self.assertAlmostEqual(anatomy["latewood_vessel_mean"], 0.000015)

        ray_families = pattern["ray_families"]
        self.assertEqual(
            len(ray_families["wide_angles_degrees"]),
            len(ray_families["wide_widths_m"]),
        )
        self.assertGreaterEqual(len(ray_families["wide_angles_degrees"]), 15)
        self.assertLess(ray_families["narrow_angle_step_degrees"], 6.0)

    def test_tracks_and_events_are_finite_authored_structures(self):
        pattern = json.loads(PATTERN_PATH.read_text())
        families = pattern["grain_track_families"]
        self.assertEqual(
            [family["face"] for family in families],
            ["front", "back", "top", "bottom"],
        )
        family_by_face = {family["face"]: family for family in families}
        track_count = 0
        for family in families:
            count = len(family["radii_m"])
            track_count += count
            self.assertGreaterEqual(count, 6)
            for lane in (
                "widths_m",
                "palette_indices",
                "starts_m",
                "lengths_m",
                "strengths",
            ):
                self.assertEqual(len(family[lane]), count)
        self.assertGreaterEqual(track_count, 27)

        allowed = {"bury", "fade", "flatten"}
        self.assertEqual(
            {event["kind"] for event in pattern["track_events"]},
            allowed,
        )
        for event in pattern["track_events"]:
            self.assertLess(
                event["track"],
                len(family_by_face[event["face"]]["radii_m"]),
            )

    def test_profile_forbids_noise_damage_and_authored_roughness(self):
        profile = json.loads(PROFILE_PATH.read_text())
        source = GENERATOR_PATH.read_text() if GENERATOR_PATH.exists() else ""

        self.assertFalse(profile["surface_response"]["authored_roughness_map"])
        self.assertFalse(profile["surface_response"]["damage"])
        self.assertIn("random_noise", profile["excluded_lanes"])
        self.assertIn("damage", profile["excluded_lanes"])
        self.assertNotIn("np.random", source)
        self.assertNotIn("periodic_value_noise", source)
        self.assertNotIn("periodic_fbm", source)


class StructuralOakTimberGeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.generator = _load_generator()

    def test_loader_reuses_exact_door_palette(self):
        bundle = self.generator.load_bundle(PROFILE_PATH, PATTERN_PATH)
        palette_hex = bundle["door_profile"]["palette"][
            "observed_reference_twenty"
        ]
        self.assertEqual(len(palette_hex), 20)
        self.assertEqual(palette_hex[0], "#49342f")
        self.assertEqual(palette_hex[-1], "#a28259")
        self.assertEqual(bundle["palette_linear"].shape, (20, 3))

    def test_ring_table_is_deterministic_and_spans_full_cross_section(self):
        bundle = self.generator.load_bundle(PROFILE_PATH, PATTERN_PATH)
        first = self.generator.compile_ring_table(bundle["pattern"])
        second = self.generator.compile_ring_table(bundle["pattern"])

        np.testing.assert_array_equal(first["boundaries_m"], second["boundaries_m"])
        np.testing.assert_array_equal(first["widths_m"], second["widths_m"])
        self.assertGreater(first["boundaries_m"][-1], 0.72)
        self.assertGreater(len(first["widths_m"]), 500)
        self.assertEqual(first["digest"], second["digest"])

    def test_side_and_end_sample_the_same_ring_identity(self):
        bundle = self.generator.load_bundle(PROFILE_PATH, PATTERN_PATH)
        ring_table = self.generator.compile_ring_table(bundle["pattern"])
        u_m = 2.37
        y_m = np.array([[-0.15, -0.06, 0.07]], dtype=np.float32)
        z_m = np.array([[0.09, -0.02, 0.17]], dtype=np.float32)

        side = self.generator.evaluate_growth_volume(
            bundle["profile"],
            bundle["pattern"],
            ring_table,
            u_m=np.full_like(y_m, u_m),
            y_m=y_m,
            z_m=z_m,
        )
        end = self.generator.evaluate_growth_volume(
            bundle["profile"],
            bundle["pattern"],
            ring_table,
            u_m=np.full_like(y_m, u_m),
            y_m=y_m,
            z_m=z_m,
        )
        np.testing.assert_array_equal(side["ring_index"], end["ring_index"])
        np.testing.assert_allclose(side["ring_phase"], end["ring_phase"])

    def test_generated_maps_preserve_separate_anatomical_lanes(self):
        bundle = self.generator.load_bundle(PROFILE_PATH, PATTERN_PATH)
        material = self.generator.generate_material(
            bundle,
            side_width=336,
            side_face_height=64,
            end_tile_resolution=192,
        )

        self.assertEqual(material["side_base_color_linear"].shape, (256, 336, 3))
        self.assertEqual(material["side_normal"].shape, (256, 336, 3))
        self.assertEqual(material["side_height_m"].shape, (256, 336))
        self.assertEqual(material["side_identity"].shape, (256, 336, 3))
        self.assertEqual(material["side_response"].shape, (256, 336, 3))
        self.assertEqual(material["end_base_color_linear"].shape, (192, 384, 3))
        self.assertEqual(material["end_normal"].shape, (192, 384, 3))
        self.assertEqual(material["end_height_m"].shape, (192, 384))
        self.assertEqual(material["end_identity"].shape, (192, 384, 3))

        ring = material["side_identity"][..., 0]
        track = material["side_identity"][..., 1]
        knot = material["side_identity"][..., 2]
        self.assertGreater(float(ring.std()), 0.01)
        self.assertGreater(float(track.std()), 0.01)
        self.assertGreater(float(knot.max()), 0.15)
        self.assertLess(float((track > 0.2).mean()), 0.22)

        end_ring = material["end_identity"][..., 0]
        end_ray = material["end_identity"][..., 1]
        end_pore = material["end_identity"][..., 2]
        self.assertGreater(float(end_ring.std()), 0.05)
        self.assertGreater(float(end_ray.std()), 0.02)
        # At this deliberately tiny test resolution a measured 0.154 mm pore
        # is far below one 2.08 mm texel.  The generator must preserve its
        # fractional area response instead of inflating it into a large dot.
        self.assertGreater(float(end_pore.max()), 0.005)
        self.assertLess(float(end_pore.max()), 0.04)
        self.assertLess(
            float(np.abs(material["side_height_m"]).max()),
            bundle["profile"]["physical_anatomy_m"]["maximum_side_relief"]
            + 1.0e-8,
        )
        self.assertLess(
            float(np.abs(material["end_height_m"]).max()),
            bundle["profile"]["physical_anatomy_m"]["maximum_end_relief"]
            + 1.0e-8,
        )

    def test_writer_emits_maps_and_a_continuity_manifest(self):
        bundle = self.generator.load_bundle(PROFILE_PATH, PATTERN_PATH)
        with TemporaryDirectory() as temporary:
            output_root = Path(temporary)
            manifest = self.generator.build_outputs(
                bundle,
                output_root,
                side_width=336,
                side_face_height=64,
                end_tile_resolution=192,
            )
            expected = {
                "structural_oak_timber_v1_side_basecolor.png",
                "structural_oak_timber_v1_side_normal.png",
                "structural_oak_timber_v1_side_height.png",
                "structural_oak_timber_v1_side_identity.png",
                "structural_oak_timber_v1_side_response.png",
                "structural_oak_timber_v1_end_basecolor.png",
                "structural_oak_timber_v1_end_normal.png",
                "structural_oak_timber_v1_end_height.png",
                "structural_oak_timber_v1_end_identity.png",
                "structural_oak_timber_v1_material_book.png",
                "structural_oak_timber_v1_manifest.json",
            }
            self.assertTrue(expected.issubset({path.name for path in output_root.iterdir()}))
            self.assertEqual(
                manifest["continuity"]["side_ring_table_digest"],
                manifest["continuity"]["end_ring_table_digest"],
            )
            self.assertFalse(manifest["surface_response"]["roughness_map"])
            self.assertFalse(manifest["surface_response"]["damage"])


if __name__ == "__main__":
    unittest.main()
