from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
import unittest

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "lime_plaster_masonry_v1"
)
GENERATOR_PATH = MATERIAL_ROOT / "generate_lime_plaster_masonry_v1.py"
PATTERN_PATH = MATERIAL_ROOT / "patterns" / "giant_house_rubble_v1.json"
PROFILE_PATH = MATERIAL_ROOT / "profiles" / "lime_plaster_masonry_v1.json"
INTENT_PATH = (
    MATERIAL_ROOT
    / "references"
    / "lime_plaster_masonry_material_intent_v2.json"
)


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "lime_plaster_masonry_v1_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load generator from {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


generator = _load_generator()


def _wrapped_seam_delta(value: np.ndarray) -> float:
    return float(
        max(
            np.mean(np.abs(value[0] - value[-1])),
            np.mean(np.abs(value[:, 0] - value[:, -1])),
        )
    )


def _interior_neighbor_delta(value: np.ndarray) -> float:
    return float(
        max(
            np.mean(np.abs(value[1:] - value[:-1])),
            np.mean(np.abs(value[:, 1:] - value[:, :-1])),
        )
    )


class LimePlasterMasonryV1GeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.recipe = generator.load_rubble_recipe(PATTERN_PATH)
        cls.intent = generator.load_material_intent(INTENT_PATH)
        cls.material = generator.generate_material(
            resolution=384,
            seed=73129,
            pattern_variation=0,
            recipe_path=PATTERN_PATH,
            intent_path=INTENT_PATH,
        )

    def test_written_material_intent_has_no_ai_or_pixel_dependency(self):
        intent = self.intent
        self.assertEqual(intent["schema"], generator.MATERIAL_INTENT_SCHEMA)
        self.assertEqual(
            intent["source"]["kind"],
            "written_conservation_and_authored_material_contract",
        )
        self.assertFalse(intent["source"]["uses_ai_generated_reference"])
        self.assertFalse(intent["source"]["uses_sampled_reference_pixels"])
        self.assertFalse(
            intent["production_translation"][
                "raw_pixels_used_as_runtime_texture"
            ]
        )
        self.assertFalse(
            intent["production_translation"][
                "legacy_ai_experiments_are_build_inputs"
            ]
        )
        self.assertEqual(
            set(intent["materials"]["stone"]["families"]),
            set(generator.STONE_FAMILIES),
        )
        self.assertEqual(
            generator._intent_palette(
                intent["materials"]["stone"]["common_field"],
                label="stone/common_field",
            ).shape,
            (20, 3),
        )
        for name in (
            "plaster",
            "mortar",
            "finish_coat",
            "brown_coat",
            "scratch_coat",
        ):
            self.assertEqual(
                generator._intent_palette(
                    intent["materials"][name],
                    label=name,
                ).shape,
                (20, 3),
            )
        self.assertEqual(
            intent["style_intent"]["line_color_rgb"],
            [45, 44, 42],
        )
        measurements = intent["construction_measurements_m"]
        self.assertAlmostEqual(measurements["scratch_coat"], 0.009525)
        self.assertAlmostEqual(measurements["brown_coat"], 0.009525)
        self.assertAlmostEqual(measurements["finish_coat"], 0.003175)
        self.assertAlmostEqual(
            measurements["complete_three_coat_build"],
            0.022225,
        )

    def test_recipe_owns_giant_scale_construction_and_roles(self):
        recipe = self.recipe
        self.assertEqual(recipe.schema, generator.PATTERN_SCHEMA)
        self.assertEqual(recipe.name, "giant_house_rubble_v1")
        self.assertEqual(recipe.tile_size_m, 4.0)
        self.assertEqual(len(recipe.courses), 7)
        self.assertTrue(
            all(len(champion["outline"]) >= 8 for champion in recipe.champions)
        )
        self.assertEqual(
            {entry["role"] for entry in recipe.champions},
            {"quiet", "load_bearing", "bedded", "fractured", "traversal"},
        )
        layout = generator.generate_stone_layout(recipe, variation=0)
        self.assertEqual(len(layout), 36)
        widths = np.asarray(
            [stone["x1_m"] - stone["x0_m"] for stone in layout]
        )
        heights = np.asarray(
            [stone["top_m"] - stone["bottom_m"] for stone in layout]
        )
        self.assertGreaterEqual(float(widths.min()), 0.379)
        self.assertLessEqual(float(widths.max()), 1.141)
        self.assertGreaterEqual(float(heights.min()), 0.45)
        self.assertLessEqual(float(heights.max()), 0.70)

        first = generator.generate_stone_layout(recipe, variation=4)
        second = generator.generate_stone_layout(recipe, variation=4)
        other = generator.generate_stone_layout(recipe, variation=5)
        self.assertEqual(first, second)
        self.assertEqual(
            [(s["x0_m"], s["x1_m"]) for s in first],
            [(s["x0_m"], s["x1_m"]) for s in other],
        )
        self.assertNotEqual(
            [s["phase"] for s in first],
            [s["phase"] for s in other],
        )

    def test_raster_preserves_every_authored_stone_and_mortar(self):
        masonry = self.material["masonry"]
        identities = np.unique(masonry["stone_id"])
        self.assertEqual(set(identities), set(range(37)))
        self.assertEqual(masonry["stone_count"], 36)
        self.assertGreater(float(masonry["stone_mask"].mean()), 0.72)
        self.assertLess(float(masonry["stone_mask"].mean()), 0.93)
        self.assertGreater(float(masonry["mortar_mask"].mean()), 0.07)
        self.assertTrue(np.isfinite(masonry["edge_distance_m"]).all())
        self.assertGreater(float(masonry["edge_distance_m"].max()), 0.12)

    def test_each_stone_owns_twenty_shades_and_uses_a_quiet_subset(self):
        masonry = self.material["masonry"]
        palettes = masonry["stone_shade_palettes_srgb"][1:]
        self.assertEqual(palettes.shape, (36, 20, 3))
        luminance = np.sum(
            palettes
            * np.asarray([0.2126, 0.7152, 0.0722], dtype=np.float32),
            axis=-1,
        )
        self.assertTrue(
            np.all(luminance.max(axis=1) - luminance.min(axis=1) > 0.20)
        )
        occupied_counts = []
        for stone_id in range(1, 37):
            coordinate = masonry["shade_coordinate"][
                masonry["stone_id"] == stone_id
            ]
            occupied_counts.append(
                np.unique(np.rint(coordinate * 19).astype(np.int32)).size
            )
        self.assertGreaterEqual(min(occupied_counts), 2)
        self.assertGreaterEqual(max(occupied_counts), 4)
        self.assertLessEqual(max(occupied_counts), 8)

    def test_plaster_has_broad_quiet_fields_and_sparse_surface_accents(self):
        plaster = self.material["plaster"]
        palette = plaster["shade_palette_srgb"]
        self.assertEqual(palette.shape, (20, 3))
        luminance = np.sum(
            palette
            * np.asarray([0.2126, 0.7152, 0.0722], dtype=np.float32),
            axis=-1,
        )
        authored = generator._intent_palette(
            self.intent["materials"]["plaster"],
            label="plaster",
        )
        np.testing.assert_allclose(palette, authored, atol=1.0e-7)
        self.assertEqual(plaster["authored_trowel_event_count"], 13)
        self.assertGreater(
            float((plaster["authored_trowel_event_mask"] > 0.20).mean()),
            0.01,
        )
        self.assertGreater(float(luminance.max() - luminance.min()), 0.24)
        self.assertLess(float((plaster["aggregate_mask"] > 0.10).mean()), 0.20)
        self.assertLess(float((plaster["pore_mask"] > 0.10).mean()), 0.08)
        self.assertLess(float((plaster["line_mask"] > 0.10).mean()), 0.18)
        blurred = generator.periodic_gaussian_blur(
            plaster["shade_coordinate"], 18.0
        )
        residual = plaster["shade_coordinate"] - blurred
        self.assertGreater(float(np.std(blurred)), float(np.std(residual)) * 0.8)

    def test_linework_is_selective_and_caused_by_stone_form(self):
        masonry = self.material["masonry"]
        stone = masonry["stone_mask"] > 0.5
        near_edge = np.logical_and(
            stone, masonry["edge_distance_m"] < 0.020
        )
        interior = np.logical_and(
            stone, masonry["edge_distance_m"] > 0.085
        )
        self.assertGreater(
            float(masonry["primary_line_mask"][near_edge].mean()),
            float(masonry["primary_line_mask"][interior].mean()) * 4.0,
        )
        coverage = float((masonry["ink_mask"][stone] > 0.12).mean())
        self.assertGreater(coverage, 0.015)
        self.assertLess(coverage, 0.34)
        champion_means = []
        for champion_id in np.unique(masonry["champion_id"][stone]):
            champion_means.append(
                float(
                    masonry["ink_mask"][
                        masonry["champion_id"] == champion_id
                    ].mean()
                )
            )
        self.assertGreater(max(champion_means) - min(champion_means), 0.025)

    def test_pbr_lanes_are_coherent_and_seam_safe(self):
        for surface_name in ("plaster", "masonry"):
            surface = self.material[surface_name]
            self.assertEqual(surface["base_color_srgb"].shape, (384, 384, 3))
            self.assertEqual(surface["normal"].shape, (384, 384, 3))
            self.assertEqual(surface["form_normal"].shape, (384, 384, 3))
            self.assertEqual(surface["detail_normal"].shape, (384, 384, 3))
            self.assertEqual(surface["roughness"].shape, (384, 384))
            self.assertEqual(surface["ao"].shape, (384, 384))
            self.assertTrue(np.isfinite(surface["height_m"]).all())
            np.testing.assert_allclose(
                np.linalg.norm(surface["normal"], axis=-1),
                1.0,
                atol=2.0e-4,
            )
            for lane in ("form_normal", "detail_normal"):
                np.testing.assert_allclose(
                    np.linalg.norm(surface[lane], axis=-1),
                    1.0,
                    atol=2.0e-4,
                )
                self.assertGreater(
                    float(np.std(surface[lane][..., :2])),
                    1.0e-5,
                )
            for lane in (
                surface["base_color_srgb"],
                surface["roughness"],
                surface["ao"],
            ):
                seam = _wrapped_seam_delta(lane)
                interior = _interior_neighbor_delta(lane)
                self.assertLess(seam, interior * 6.0 + 0.012, surface_name)

        masonry = self.material["masonry"]
        mortar = masonry["mortar_mask"] > 0.5
        stone = masonry["stone_mask"] > 0.5
        self.assertGreater(
            float(masonry["roughness"][mortar].mean()),
            float(masonry["roughness"][stone].mean()) + 0.08,
        )
        self.assertLess(
            float(masonry["ao"][np.logical_and(
                stone, masonry["edge_distance_m"] < 0.02
            )].mean()),
            float(masonry["ao"][np.logical_and(
                stone, masonry["edge_distance_m"] > 0.08
            )].mean()),
        )

    def test_transition_has_ordered_construction_states(self):
        states = self.material["transition_states"]
        np.testing.assert_allclose(
            np.unique(states["state"]),
            np.asarray([0.07, 0.25, 0.46, 0.68, 0.94], dtype=np.float32),
            atol=1.0e-6,
        )
        self.assertGreater(float(states["transition_edge"].max()), 0.98)
        self.assertGreater(float(states["plaster_coverage"].mean()), 0.50)
        profile = json.loads(PROFILE_PATH.read_text())
        self.assertEqual(
            [entry["name"] for entry in profile["layer_states"]],
            [
                "masonry",
                "flush_lime_mortar",
                "scratched_coarse_coat",
                "brown_float_coat",
                "lime_finish",
            ],
        )
        self.assertFalse(
            profile["source_policy"]["ai_generated_reference_capture"]
        )
        self.assertFalse(
            profile["source_policy"]["legacy_ai_experiments_are_build_inputs"]
        )
        self.assertFalse(
            profile["source_policy"]["ai_generated_runtime_texture"]
        )
        self.assertFalse(
            profile["source_policy"][
                "raw_reference_pixels_used_as_runtime_texture"
            ]
        )
        self.assertEqual(profile["default_overlays"]["damp"], 0.0)
        self.assertEqual(profile["default_overlays"]["soot"], 0.0)

    def test_writer_emits_complete_bit_depth_and_contract_package(self):
        with TemporaryDirectory() as directory:
            manifest = generator.write_material_package(
                self.material,
                output_root=directory,
                pattern_path=PATTERN_PATH,
                profile_path=PROFILE_PATH,
            )
            root = Path(directory)
            self.assertEqual(manifest["schema"], generator.MATERIAL_SCHEMA)
            self.assertEqual(manifest["stone_count"], 36)
            self.assertEqual(manifest["shade_family_size"], 20)
            self.assertFalse(
                manifest["source_policy"]["ai_generated_reference_capture"]
            )
            self.assertFalse(
                manifest["source_policy"]["ai_generated_runtime_texture"]
            )
            self.assertFalse(
                manifest["material_intent"][
                    "raw_pixels_used_as_runtime_texture"
                ]
            )
            self.assertFalse(
                manifest["material_intent"]["uses_ai_generated_reference"]
            )
            self.assertFalse(
                manifest["material_intent"]["uses_sampled_reference_pixels"]
            )
            self.assertEqual(len(manifest["outputs"]), 21)
            self.assertTrue(
                manifest["constraints"]["categorical_state_is_face_domain"]
            )
            self.assertTrue(
                manifest["constraints"][
                    "form_and_detail_normals_are_separate"
                ]
            )
            self.assertTrue(
                manifest["constraints"]["height_is_authored_in_metres"]
            )
            self.assertFalse(
                manifest["constraints"]["unreal_runtime_parity_verified"]
            )
            for relative in manifest["outputs"].values():
                self.assertTrue((root / relative).is_file(), relative)
            for name in (
                "lime_plaster_v1_height.png",
                "giant_masonry_v1_height.png",
            ):
                data = (root / name).read_bytes()
                self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
                width, height, bit_depth, color_type = struct.unpack(
                    ">IIBB", data[16:26]
                )
                self.assertEqual((width, height), (384, 384))
                self.assertEqual(bit_depth, 16)
                self.assertEqual(color_type, 0)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
