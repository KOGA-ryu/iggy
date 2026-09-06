from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest

import numpy as np


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    ROOT / "assets" / "creative" / "materials" / "forged_iron_v1"
)
GENERATOR_PATH = MATERIAL_ROOT / "generate_forged_iron_patterns_v1.py"
COMMON_PATH = (
    ROOT / "assets" / "creative" / "materials" / "pattern_lab_common.py"
)
PATTERN_PATH = (
    MATERIAL_ROOT / "patterns" / "forged_iron_face_champion_v1.json"
)


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "forged_iron_v1_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load generator from {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _load_common():
    spec = importlib.util.spec_from_file_location(
        "pattern_lab_common_preview_tests",
        COMMON_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load shared tools from {COMMON_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


generator = _load_generator()
common = _load_common()


class ConductorPreviewTests(unittest.TestCase):
    @staticmethod
    def _flat_fixture(
        *,
        height: int = 16,
        width: int = 24,
        roughness: float = 0.38,
    ) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
        normal = np.zeros((height, width, 3), dtype=np.float32)
        normal[..., 2] = 1.0
        roughness_field = np.full(
            (height, width),
            roughness,
            dtype=np.float32,
        )
        ao = np.ones((height, width), dtype=np.float32)
        return normal, roughness_field, ao

    def test_conductor_preview_has_no_diffuse_fallback(self):
        normal, roughness, ao = self._flat_fixture()
        black_f0 = np.zeros((*roughness.shape, 3), dtype=np.float32)

        preview = common.render_conductor_single_light(
            black_f0,
            normal,
            roughness,
            ao,
            direction=(0.0, 0.0, 1.0),
            environment_strength=0.0,
        )

        np.testing.assert_allclose(preview, 0.0, atol=1.0e-7)

    def test_conductor_preview_preserves_rgb_reflectance_identity(self):
        normal, roughness, ao = self._flat_fixture()
        iron_f0 = np.broadcast_to(
            np.array([0.56, 0.57, 0.58], dtype=np.float32),
            (*roughness.shape, 3),
        )

        first = common.render_conductor_preview(
            iron_f0,
            normal,
            roughness,
            ao,
        )
        second = common.render_conductor_preview(
            iron_f0,
            normal,
            roughness,
            ao,
        )

        self.assertEqual(first.shape, iron_f0.shape)
        self.assertTrue(np.isfinite(first).all())
        self.assertTrue(((0.0 <= first) & (first <= 1.0)).all())
        np.testing.assert_array_equal(first, second)
        channel_means = first.mean(axis=(0, 1))
        self.assertLess(float(channel_means[0]), float(channel_means[1]))
        self.assertLess(float(channel_means[1]), float(channel_means[2]))

    def test_conductor_roughness_controls_peak_and_spread(self):
        width = 257
        slope = np.linspace(-0.72, 0.72, width, dtype=np.float32)
        normal = np.zeros((1, width, 3), dtype=np.float32)
        normal[..., 0] = slope
        normal[..., 2] = np.sqrt(1.0 - slope * slope)
        ao = np.ones((1, width), dtype=np.float32)
        iron_f0 = np.broadcast_to(
            np.array([0.56, 0.57, 0.58], dtype=np.float32),
            (1, width, 3),
        )
        sharp = common.render_conductor_single_light(
            iron_f0,
            normal,
            np.full((1, width), 0.16, dtype=np.float32),
            ao,
            direction=(0.0, 0.0, 1.0),
            intensity=1.0,
            environment_strength=0.0,
        )[..., 1]
        broad = common.render_conductor_single_light(
            iron_f0,
            normal,
            np.full((1, width), 0.58, dtype=np.float32),
            ao,
            direction=(0.0, 0.0, 1.0),
            intensity=1.0,
            environment_strength=0.0,
        )[..., 1]

        self.assertGreater(float(sharp.max()), float(broad.max()))
        sharp_half_peak = sharp >= float(sharp.max()) * 0.5
        broad_half_peak = broad >= float(broad.max()) * 0.5
        self.assertLess(
            int(np.count_nonzero(sharp_half_peak)),
            int(np.count_nonzero(broad_half_peak)),
        )

    def test_existing_dielectric_preview_contract_is_unchanged(self):
        base_color = np.array([[[0.2, 0.3, 0.4]]], dtype=np.float32)
        normal = np.array([[[0.0, 0.0, 1.0]]], dtype=np.float32)
        roughness = np.array([[0.5]], dtype=np.float32)
        ao = np.array([[0.8]], dtype=np.float32)

        preview = common.render_material_single_light(
            base_color,
            normal,
            roughness,
            ao,
            direction=(0.2, -0.3, 0.93),
            intensity=1.4,
            color=(1.0, 0.9, 0.8),
        )

        np.testing.assert_allclose(
            preview,
            np.array(
                [[[0.45712984, 0.51031208, 0.54771590]]],
                dtype=np.float32,
            ),
            atol=1.0e-7,
        )


class ComponentTangentUvTests(unittest.TestCase):
    def test_leaf_uses_length_on_broad_faces_and_boundary_on_cut_faces(self):
        vertices = np.array(
            [
                [-2.0, 0.0, -0.5],
                [2.0, 0.0, -0.5],
                [2.0, 0.0, 0.5],
                [-2.0, 0.0, 0.5],
                [2.0, -0.1, -0.5],
                [2.0, 0.1, -0.5],
                [2.0, 0.1, 0.5],
                [2.0, -0.1, 0.5],
            ],
            dtype=np.float32,
        )
        loops = np.arange(8, dtype=np.int32)
        starts = np.array([0, 4], dtype=np.int32)
        totals = np.array([4, 4], dtype=np.int32)
        normals = np.array(
            [
                [0.0, -1.0, 0.0],
                [1.0, 0.0, 0.0],
            ],
            dtype=np.float32,
        )

        uv = common.component_tangent_uv(
            vertices,
            loops,
            starts,
            totals,
            normals,
            mode="longitudinal_leaf",
        )

        np.testing.assert_allclose(uv[:4, 0], vertices[:4, 0])
        self.assertGreater(float(np.ptp(uv[:4, 0])), 3.9)
        self.assertGreater(float(np.ptp(uv[4:, 0])), 0.9)
        self.assertGreater(float(np.ptp(uv[4:, 1])), 0.19)

    def test_barrel_unwraps_seam_and_runs_tangent_around_axis(self):
        radius = 0.4
        angles = np.radians([170.0, -170.0, -170.0, 170.0])
        z = np.array([-0.2, -0.2, 0.2, 0.2], dtype=np.float32)
        vertices = np.stack(
            [
                np.cos(angles) * radius,
                np.sin(angles) * radius,
                z,
            ],
            axis=-1,
        ).astype(np.float32)

        uv = common.component_tangent_uv(
            vertices,
            np.arange(4, dtype=np.int32),
            np.array([0], dtype=np.int32),
            np.array([4], dtype=np.int32),
            np.array([[-1.0, 0.0, 0.0]], dtype=np.float32),
            mode="circumferential_barrel",
            axis_origin=(0.0, 0.0, 0.0),
        )

        self.assertLess(float(np.ptp(uv[:, 0])), radius * 0.5)
        self.assertGreater(float(np.ptp(uv[:, 0])), radius * 0.25)
        self.assertAlmostEqual(float(np.ptp(uv[:, 1])), 0.4, places=5)

    def test_pin_runs_tangent_along_axis_on_cylindrical_wall(self):
        vertices = np.array(
            [
                [0.3, 0.0, -1.2],
                [0.0, 0.3, -1.2],
                [0.0, 0.3, 1.2],
                [0.3, 0.0, 1.2],
            ],
            dtype=np.float32,
        )

        uv = common.component_tangent_uv(
            vertices,
            np.arange(4, dtype=np.int32),
            np.array([0], dtype=np.int32),
            np.array([4], dtype=np.int32),
            np.array([[0.7, 0.7, 0.0]], dtype=np.float32),
            mode="axial_pin",
            axis_origin=(0.0, 0.0, 0.0),
        )

        np.testing.assert_allclose(uv[:, 0], vertices[:, 2])
        self.assertAlmostEqual(float(np.ptp(uv[:, 0])), 2.4, places=5)
        self.assertGreater(float(np.ptp(uv[:, 1])), 0.4)


class BroadForgingPlaneTests(unittest.TestCase):
    def test_irregular_rails_create_broad_bounded_open_planes(self):
        coordinate_x = np.linspace(0.0, 4.0, 401, dtype=np.float32)
        coordinate_y = np.linspace(-0.5, 0.5, 101, dtype=np.float32)
        x, y = np.meshgrid(coordinate_x, coordinate_y)
        rails = [
            {
                "y_m": -0.5,
                "knots_m": [
                    [0.0, 0.0],
                    [0.8, 0.0010],
                    [2.1, -0.0005],
                    [4.0, 0.0],
                ],
            },
            {
                "y_m": 0.0,
                "knots_m": [
                    [0.0, 0.0002],
                    [1.4, -0.0008],
                    [2.6, 0.0009],
                    [4.0, 0.0001],
                ],
            },
            {
                "y_m": 0.5,
                "knots_m": [
                    [0.0, 0.0],
                    [0.9, 0.0004],
                    [3.0, -0.0007],
                    [4.0, 0.0],
                ],
            },
        ]

        first = common.broad_forging_plane_field(x, y, rails)
        second = common.broad_forging_plane_field(x, y, rails)

        self.assertEqual(first.shape, x.shape)
        self.assertTrue(np.isfinite(first).all())
        np.testing.assert_array_equal(first, second)
        self.assertLessEqual(float(np.abs(first).max()), 0.00101)
        center_gradient = np.diff(first[len(coordinate_y) // 2])
        slope_sign = np.sign(center_gradient)
        sign_changes = np.count_nonzero(
            slope_sign[1:] != slope_sign[:-1]
        )
        self.assertLessEqual(int(sign_changes), 4)
        self.assertGreater(float(np.std(first)), 0.00025)

    def test_nonperiodic_normal_preserves_plane_slope_at_both_edges(self):
        coordinate_x = np.linspace(0.0, 4.0, 401, dtype=np.float32)
        coordinate_y = np.linspace(-0.5, 0.5, 101, dtype=np.float32)
        x, y = np.meshgrid(coordinate_x, coordinate_y)
        rails = [
            {
                "y_m": -0.5,
                "knots_m": [[0.0, 0.0], [2.0, 0.002], [4.0, 0.004]],
            },
            {
                "y_m": 0.5,
                "knots_m": [[0.0, 0.0], [2.0, 0.002], [4.0, 0.004]],
            },
        ]
        height = common.broad_forging_plane_field(x, y, rails)

        normal = common.height_to_normal_nonperiodic(
            height,
            meters_per_pixel_x=float(coordinate_x[1] - coordinate_x[0]),
            meters_per_pixel_y=float(coordinate_y[1] - coordinate_y[0]),
        )

        expected = np.array([-0.001, 0.0, 1.0], dtype=np.float32)
        expected /= np.linalg.norm(expected)
        np.testing.assert_allclose(
            normal[:, 0],
            np.broadcast_to(expected, normal[:, 0].shape),
            atol=2.0e-6,
        )
        np.testing.assert_allclose(
            normal[:, -1],
            np.broadcast_to(expected, normal[:, -1].shape),
            atol=2.0e-6,
        )


class ContinuousOxideLayerTests(unittest.TestCase):
    @staticmethod
    def _fixture():
        coordinate_x = np.linspace(0.0, 3.112, 381, dtype=np.float32)
        coordinate_y = np.linspace(-0.205, 0.205, 51, dtype=np.float32)
        x, y = np.meshgrid(coordinate_x, coordinate_y)
        rails = [
            {
                "y_m": -0.205,
                "knots_m": [
                    [0.0, 0.000045],
                    [0.72, 0.000062],
                    [1.80, 0.000038],
                    [3.112, 0.000053],
                ],
            },
            {
                "y_m": 0.0,
                "knots_m": [
                    [0.0, 0.000052],
                    [1.05, 0.000033],
                    [2.24, 0.000071],
                    [3.112, 0.000048],
                ],
            },
            {
                "y_m": 0.205,
                "knots_m": [
                    [0.0, 0.000049],
                    [0.84, 0.000057],
                    [2.02, 0.000041],
                    [3.112, 0.000055],
                ],
            },
        ]
        modes = [
            {"cycles": 7.3, "phase": 0.17, "amplitude_m": 0.0000030},
            {"cycles": 13.7, "phase": 0.61, "amplitude_m": 0.0000018},
        ]
        return x, y, rails, modes

    def test_intact_oxide_is_continuous_dielectric_not_relief(self):
        x, y, rails, modes = self._fixture()

        first = common.continuous_oxide_layer_fields(
            x,
            y,
            rails,
            longitudinal_modes=modes,
            minimum_thickness_m=0.000020,
            maximum_thickness_m=0.000090,
        )
        second = common.continuous_oxide_layer_fields(
            x,
            y,
            rails,
            longitudinal_modes=modes,
            minimum_thickness_m=0.000020,
            maximum_thickness_m=0.000090,
        )

        self.assertEqual(
            set(first),
            {
                "coverage",
                "thickness_m",
                "thickness_response",
                "compression_response",
                "metalness",
                "surface_height_m",
            },
        )
        for lane in first:
            np.testing.assert_array_equal(first[lane], second[lane])
            self.assertTrue(np.isfinite(first[lane]).all())
        np.testing.assert_array_equal(first["coverage"], 1.0)
        np.testing.assert_array_equal(first["metalness"], 0.0)
        np.testing.assert_array_equal(first["surface_height_m"], 0.0)
        self.assertGreaterEqual(
            float(first["thickness_m"].min()),
            0.000020,
        )
        self.assertLessEqual(
            float(first["thickness_m"].max()),
            0.000090,
        )
        self.assertGreater(
            float(np.ptp(first["thickness_m"])),
            0.000030,
        )
        self.assertGreater(float(np.std(first["thickness_m"])), 0.000005)
        self.assertTrue(
            (
                (0.0 <= first["thickness_response"])
                & (first["thickness_response"] <= 1.0)
            ).all()
        )

    def test_compression_flow_is_subordinate_to_thermal_thickness(self):
        x, y, rails, modes = self._fixture()
        base = common.continuous_oxide_layer_fields(
            x,
            y,
            rails,
            longitudinal_modes=[],
            minimum_thickness_m=0.000020,
            maximum_thickness_m=0.000090,
        )
        flowed = common.continuous_oxide_layer_fields(
            x,
            y,
            rails,
            longitudinal_modes=modes,
            minimum_thickness_m=0.000020,
            maximum_thickness_m=0.000090,
        )

        delta = np.abs(flowed["thickness_m"] - base["thickness_m"])
        self.assertLessEqual(float(delta.max()), 0.00000481)
        self.assertGreater(float(delta.mean()), 0.0000010)
        np.testing.assert_array_equal(
            flowed["coverage"],
            base["coverage"],
        )


class SurfaceLayerMixerTests(unittest.TestCase):
    @staticmethod
    def _fixture(
        *,
        height: int = 9,
        width: int = 17,
    ) -> tuple[np.ndarray, np.ndarray]:
        coordinate = np.linspace(
            0.0,
            1.0,
            height * width,
            dtype=np.float32,
        ).reshape(height, width)
        oxide = np.stack(
            [
                0.020 + coordinate * 0.015,
                0.026 + coordinate * 0.018,
                0.034 + coordinate * 0.024,
            ],
            axis=-1,
        )
        conductor = np.stack(
            [
                0.44 + coordinate * 0.06,
                0.47 + coordinate * 0.06,
                0.51 + coordinate * 0.06,
            ],
            axis=-1,
        )
        return oxide, conductor

    def test_intact_oxide_is_exact_and_invariant_to_hidden_conductor(self):
        oxide, conductor = self._fixture()
        intact = np.zeros(oxide.shape[:2], dtype=np.float32)

        first = common.compose_surface_layer_responses(
            oxide,
            conductor,
            intact,
        )
        second = common.compose_surface_layer_responses(
            oxide,
            np.full_like(conductor, 0.99),
            intact,
        )

        self.assertEqual(
            set(first),
            {
                "combined_response",
                "oxide_weight",
                "conductor_weight",
                "compiled_metalness",
            },
        )
        np.testing.assert_array_equal(first["combined_response"], oxide)
        np.testing.assert_array_equal(
            first["combined_response"],
            second["combined_response"],
        )
        np.testing.assert_array_equal(first["oxide_weight"], 1.0)
        np.testing.assert_array_equal(first["conductor_weight"], 0.0)
        np.testing.assert_array_equal(first["compiled_metalness"], 0.0)

    def test_full_exposure_reproduces_conductor_without_oxide_leak(self):
        oxide, conductor = self._fixture()
        exposed = np.ones(oxide.shape[:2], dtype=np.float32)

        result = common.compose_surface_layer_responses(
            oxide,
            conductor,
            exposed,
        )

        np.testing.assert_array_equal(
            result["combined_response"],
            conductor,
        )
        np.testing.assert_array_equal(result["oxide_weight"], 0.0)
        np.testing.assert_array_equal(result["conductor_weight"], 1.0)
        np.testing.assert_array_equal(result["compiled_metalness"], 1.0)

    def test_boundary_is_explicit_coverage_and_does_not_mutate_inputs(self):
        oxide, conductor = self._fixture(height=3, width=11)
        oxide_before = oxide.copy()
        conductor_before = conductor.copy()
        exposure = np.broadcast_to(
            np.linspace(0.0, 1.0, 11, dtype=np.float32),
            (3, 11),
        ).copy()
        exposure_before = exposure.copy()

        result = common.compose_surface_layer_responses(
            oxide,
            conductor,
            exposure,
        )

        expected = (
            oxide * (1.0 - exposure[..., np.newaxis])
            + conductor * exposure[..., np.newaxis]
        )
        np.testing.assert_allclose(
            result["combined_response"],
            expected,
            atol=1.0e-7,
        )
        np.testing.assert_array_equal(
            result["oxide_weight"] + result["conductor_weight"],
            1.0,
        )
        np.testing.assert_array_equal(
            result["compiled_metalness"],
            exposure,
        )
        np.testing.assert_array_equal(oxide, oxide_before)
        np.testing.assert_array_equal(conductor, conductor_before)
        np.testing.assert_array_equal(exposure, exposure_before)

    def test_invalid_exposure_fails_instead_of_clipping_material_identity(self):
        oxide, conductor = self._fixture(height=2, width=3)
        invalid_exposure = np.zeros((2, 3), dtype=np.float32)
        invalid_exposure[0, 0] = -0.01
        invalid_exposure[1, 2] = 1.01

        with self.assertRaisesRegex(
            ValueError,
            "exposed conductor mask",
        ):
            common.compose_surface_layer_responses(
                oxide,
                conductor,
                invalid_exposure,
            )


class ForgedIronGeneratorTests(unittest.TestCase):
    def test_champion_pattern_uses_measured_tool_envelopes(self):
        recipe = generator.load_pattern_recipe(PATTERN_PATH)

        self.assertEqual(recipe["schema"], "iggy3d.pattern.forged_iron_face.v1")
        self.assertEqual(
            recipe["physical_tile_m"],
            {"length": 1.218, "width": 0.164},
        )
        self.assertEqual(recipe["variation_count"], 4)
        self.assertEqual(
            recipe["tool_envelopes_m"]["planishing_face"],
            [0.041275, 0.03175],
        )
        self.assertEqual(
            recipe["tool_envelopes_m"]["cross_peen_face"],
            [0.0365125, 0.0127],
        )
        event_kinds = {event["kind"] for event in recipe["events"]}
        self.assertEqual(event_kinds, {"planishing_face", "cross_peen_face"})
        self.assertGreaterEqual(len(recipe["events"]), 24)
        self.assertGreaterEqual(len(recipe["colour_fields"]), 8)
        self.assertGreaterEqual(len(recipe["compact_scale_plates"]), 20)
        self.assertGreaterEqual(len(recipe["authored_micro_modes"]), 8)
        worked = recipe["worked_surface"]
        self.assertEqual(
            worked["physical_tile_m"],
            {"length": 2.436, "width": 0.492},
        )
        self.assertEqual(worked["wire_diameter_proxy_m"], [0.00030, 0.00035])
        self.assertEqual(worked["visible_bundle_spacing_m"], [0.0024, 0.0048])
        self.assertGreaterEqual(len(worked["passes"]), 12)
        self.assertTrue(
            all(
                worked["visible_bundle_spacing_m"][0]
                <= item["bundle_spacing_m"]
                <= worked["visible_bundle_spacing_m"][1]
                for item in worked["passes"]
            )
        )

    def test_variations_are_deterministic_bounded_and_not_identical(self):
        recipe = generator.load_pattern_recipe(PATTERN_PATH)
        first = generator.compile_variations(recipe)
        second = generator.compile_variations(recipe)

        self.assertEqual(len(first), 4)
        self.assertEqual(first, second)
        self.assertNotEqual(first[0], first[1])
        for variation in first:
            self.assertEqual(
                {event["kind"] for event in variation["events"]},
                {"planishing_face", "cross_peen_face"},
            )
            self.assertGreaterEqual(len(variation["events"]), 22)
            self.assertLessEqual(len(variation["events"]), 30)
            for event in variation["events"]:
                self.assertGreaterEqual(event["x_m"], 0.0)
                self.assertLess(
                    event["x_m"],
                    recipe["physical_tile_m"]["length"],
                )
                self.assertGreaterEqual(event["y_m"], 0.0)
                self.assertLess(
                    event["y_m"],
                    recipe["physical_tile_m"]["width"],
                )

    def test_material_has_quiet_fields_finite_marks_and_measured_micro_ra(self):
        recipe = generator.load_pattern_recipe(PATTERN_PATH)
        material = generator.generate_material(
            recipe,
            resolution_x=768,
            resolution_y=128,
        )

        self.assertEqual(material["base_color_linear"].shape, (512, 768, 3))
        self.assertEqual(material["roughness"].shape, (512, 768))
        self.assertEqual(material["metalness"].shape, (512, 768))
        self.assertEqual(material["height_m"].shape, (512, 768))
        self.assertEqual(material["normal"].shape, (512, 768, 3))
        self.assertEqual(material["macro_normal"].shape, (512, 768, 3))
        self.assertEqual(material["scale_normal"].shape, (512, 768, 3))
        self.assertEqual(material["micro_normal"].shape, (512, 768, 3))
        self.assertEqual(material["worked_normal"].shape, (512, 768, 3))
        self.assertEqual(material["worked_response"].shape, (512, 768, 3))
        self.assertEqual(material["macro_height_m"].shape, (512, 768))
        self.assertEqual(material["scale_height_m"].shape, (512, 768))
        self.assertEqual(material["micro_height_m"].shape, (512, 768))
        self.assertEqual(material["layer_response"].shape, (512, 768, 3))
        self.assertEqual(material["masks"].shape, (512, 768, 3))
        self.assertTrue(np.isfinite(material["height_m"]).all())
        self.assertTrue(
            ((0.34 <= material["roughness"])
             & (material["roughness"] <= 0.82)).all()
        )

        planish = material["masks"][..., 0]
        cross_peen = material["masks"][..., 1]
        scale_plate_tone = material["masks"][..., 2]
        quiet = np.maximum(planish, cross_peen) < 0.10
        self.assertGreater(float((planish > 0.10).mean()), 0.05)
        self.assertGreater(float((cross_peen > 0.10).mean()), 0.01)
        self.assertGreater(float(quiet.mean()), 0.80)
        self.assertLess(float(quiet.mean()), 0.95)
        self.assertGreater(float((scale_plate_tone > 0.10).mean()), 0.30)
        self.assertLess(float((scale_plate_tone > 0.10).mean()), 0.65)

        for variation_index in range(4):
            y0 = variation_index * 128
            y1 = y0 + 128
            variation_height = material["height_m"][y0:y1]
            center = float(np.median(variation_height))
            ra = float(np.mean(np.abs(variation_height - center)))
            self.assertGreater(ra, 0.000005)
            self.assertLess(ra, 0.000015)

        metalness = material["metalness"]
        self.assertEqual(float(metalness.min()), 0.0)
        self.assertEqual(float(metalness.max()), 0.0)
        worked = material["worked_response"][..., 0]
        direction = material["worked_response"][..., 1]
        rest = material["worked_response"][..., 2]
        self.assertGreater(float((worked > 0.10).mean()), 0.16)
        self.assertLess(float((worked > 0.10).mean()), 0.58)
        self.assertGreater(float((rest > 0.75).mean()), 0.35)
        self.assertTrue(((0.0 <= direction) & (direction <= 1.0)).all())
        self.assertGreater(
            float(np.abs(material["worked_normal"][..., :2]).max()),
            0.0001,
        )

        unique_shades = np.unique(
            np.round(material["base_color_linear"] * 255.0).astype(np.uint8)
            .reshape(-1, 3),
            axis=0,
        )
        self.assertGreaterEqual(unique_shades.shape[0], 12)
        height_roughness_correlation = np.corrcoef(
            material["height_m"].ravel(),
            material["roughness"].ravel(),
        )[0, 1]
        self.assertLess(abs(float(height_roughness_correlation)), 0.72)
        self.assertGreater(
            float(np.abs(material["macro_normal"][..., :2]).max()),
            0.001,
        )
        self.assertGreater(
            float(np.abs(material["scale_normal"][..., :2]).max()),
            0.0001,
        )
        self.assertGreater(
            float(np.abs(material["micro_normal"][..., :2]).max()),
            0.0001,
        )
        np.testing.assert_allclose(
            material["layer_response"][..., 2],
            material["metalness"],
            atol=1.0e-7,
        )

    def test_generate_writes_channel_maps_and_physical_manifest(self):
        with TemporaryDirectory() as temporary:
            manifest = generator.generate(
                Path(temporary),
                resolution_x=768,
                resolution_y=128,
            )

            self.assertEqual(
                manifest["schema"],
                "iggy-forged-iron-pattern-build/4.0",
            )
            self.assertFalse(manifest["constraints"]["uses_ai_generated_imagery"])
            self.assertFalse(manifest["constraints"]["uses_damage"])
            self.assertEqual(manifest["variation_count"], 4)
            self.assertEqual(
                manifest["physical_tile_m"],
                {"length": 1.218, "width": 0.164},
            )
            self.assertEqual(
                manifest["worked_physical_tile_m"],
                {"length": 2.436, "width": 0.492},
            )
            self.assertTrue(
                manifest["constraints"]["continuous_compact_scale_ground"]
            )
            self.assertTrue(
                manifest["constraints"][
                    "hammering_never_implies_exposed_iron"
                ]
            )
            self.assertTrue(
                manifest["constraints"][
                    "worked_surface_uses_independent_non_grid_tile"
                ]
            )
            statistics = manifest["layer_statistics"]
            self.assertGreater(
                statistics["quiet_fraction_activity_below_0_10"],
                0.80,
            )
            self.assertLess(
                statistics["conductive_fraction_above_0_90"],
                0.000001,
            )
            self.assertGreater(
                statistics["worked_fraction_above_0_10"],
                0.16,
            )
            self.assertGreater(
                statistics["protected_rest_fraction_above_0_75"],
                0.35,
            )
            self.assertEqual(
                set(manifest["outputs"]),
                {
                    "base_color",
                    "scale_color",
                    "iron_color",
                    "roughness",
                    "metalness",
                    "layer_response",
                    "height",
                    "macro_height",
                    "scale_height",
                    "micro_height",
                    "normal",
                    "macro_normal",
                    "scale_normal",
                    "micro_normal",
                    "worked_normal",
                    "worked_response",
                    "masks",
                    "proof",
                },
            )
            for output in manifest["outputs"].values():
                path = Path(output["path"])
                self.assertTrue(path.is_file())
                self.assertGreater(path.stat().st_size, 100)
            self.assertEqual(
                _png_header(Path(manifest["outputs"]["height"]["path"]))[
                    "bit_depth"
                ],
                16,
            )
            self.assertEqual(
                _png_header(Path(manifest["outputs"]["roughness"]["path"]))[
                    "bit_depth"
                ],
                16,
            )
            self.assertEqual(
                _png_header(Path(manifest["outputs"]["metalness"]["path"]))[
                    "bit_depth"
                ],
                16,
            )
            for lane in ("macro_height", "scale_height", "micro_height"):
                self.assertEqual(
                    _png_header(Path(manifest["outputs"][lane]["path"]))[
                        "bit_depth"
                    ],
                    16,
                )


def _png_header(path: Path) -> dict[str, int]:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(f"{path} is not a PNG")
    width, height, bit_depth, colour_type = struct.unpack(
        ">IIBB",
        data[16:26],
    )
    return {
        "width": width,
        "height": height,
        "bit_depth": bit_depth,
        "colour_type": colour_type,
    }


if __name__ == "__main__":
    unittest.main()
