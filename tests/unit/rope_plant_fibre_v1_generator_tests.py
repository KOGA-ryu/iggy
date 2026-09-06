from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory
import unittest

import numpy as np


ROOT = Path(__file__).resolve().parents[2]
MATERIAL_ROOT = ROOT / "assets" / "creative" / "materials" / "rope_plant_fibre_v1"
GENERATOR_PATH = MATERIAL_ROOT / "generate_rope_plant_fibre_v1.py"
PATTERN_PATH = MATERIAL_ROOT / "patterns" / "three_strand_regular_lay_v1.json"
BUNDLE_PATTERN_PATH = (
    MATERIAL_ROOT / "patterns" / "rope_bundle_tracks_champion_v2.json"
)
FIBRE_PATTERN_PATH = (
    MATERIAL_ROOT / "patterns" / "rope_fibre_tracks_champion_v2.json"
)


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "rope_plant_fibre_v1_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load generator from {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


generator = _load_generator()


class RopePlantFibreGeneratorTests(unittest.TestCase):
    def test_fibre_champion_uses_finite_authored_ribbons_and_staples(self):
        recipe = generator.load_fibre_recipe(FIBRE_PATTERN_PATH)

        self.assertEqual(
            recipe["schema"],
            "iggy3d.pattern.rope_fibre_tracks.v2",
        )
        self.assertEqual(len(recipe["strand_transforms"]), 3)
        self.assertGreaterEqual(len(recipe["long_ribbons"]), 14)
        self.assertGreaterEqual(len(recipe["short_fibres"]), 15)
        long_lengths = [
            float(element["length"]) for element in recipe["long_ribbons"]
        ]
        short_lengths = [
            float(element["length"]) for element in recipe["short_fibres"]
        ]
        self.assertGreaterEqual(max(long_lengths) / min(long_lengths), 6.0)
        self.assertGreaterEqual(max(short_lengths) / min(short_lengths), 10.0)
        self.assertTrue(
            any(element["geometry_spawn"] for element in recipe["short_fibres"])
        )

        expected_counts = {
            "fine_lashing": (15, 6),
            "utility_line": (28, 11),
            "heavy_hawser": (34, 15),
        }
        for preset_id, (long_count, short_count) in expected_counts.items():
            compiled = generator.compile_fibre_elements(
                recipe,
                preset_id=preset_id,
                variation_index=2,
            )
            repeated = generator.compile_fibre_elements(
                recipe,
                preset_id=preset_id,
                variation_index=2,
            )
            self.assertEqual(compiled, repeated)
            self.assertEqual(len(compiled["long_ribbons"]), long_count * 3)
            self.assertEqual(len(compiled["short_fibres"]), short_count * 3)

    def test_bundle_champion_is_authored_as_tracks_not_a_frequency(self):
        recipe = generator.load_bundle_recipe(BUNDLE_PATTERN_PATH)

        self.assertEqual(
            recipe["schema"],
            "iggy3d.pattern.rope_bundle_tracks.v2",
        )
        self.assertEqual(len(recipe["strand_profiles"]), 3)
        self.assertEqual(len(recipe["track_templates"]), 11)
        self.assertEqual(
            {track["width_class"] for track in recipe["track_templates"]},
            {"fine", "narrow", "medium", "wide", "broad"},
        )
        event_tracks = [
            track for track in recipe["track_templates"] if track["events"]
        ]
        self.assertGreaterEqual(len(event_tracks), 8)
        event_kinds = {
            event["kind"]
            for track in recipe["track_templates"]
            for event in track["events"]
        }
        self.assertEqual(
            event_kinds,
            {"burial", "split", "merge", "flatten", "fade"},
        )

    def test_compiled_bundle_tracks_vary_by_preset_strand_and_variant(self):
        recipe = generator.load_bundle_recipe(BUNDLE_PATTERN_PATH)
        expected_counts = {
            "fine_lashing": 5,
            "utility_line": 8,
            "heavy_hawser": 11,
        }
        champions = {}
        for preset_id, expected in expected_counts.items():
            tracks = generator.compile_bundle_tracks(
                recipe,
                preset_id=preset_id,
                variation_index=0,
            )
            champions[preset_id] = tracks
            self.assertEqual(len(tracks), expected * 3)
            self.assertEqual(
                {track["strand"] for track in tracks},
                {0, 1, 2},
            )
            for strand in range(3):
                strand_tracks = [
                    track for track in tracks if track["strand"] == strand
                ]
                self.assertEqual(len(strand_tracks), expected)
                self.assertGreaterEqual(
                    len({round(track["width"], 4) for track in strand_tracks}),
                    min(expected, 4),
                )

        second = generator.compile_bundle_tracks(
            recipe,
            preset_id="heavy_hawser",
            variation_index=1,
        )
        repeated = generator.compile_bundle_tracks(
            recipe,
            preset_id="heavy_hawser",
            variation_index=1,
        )
        self.assertEqual(second, repeated)
        self.assertNotEqual(second, champions["heavy_hawser"])

    def test_recipe_has_three_regular_lay_scale_presets(self):
        recipe = generator.load_pattern_recipe(PATTERN_PATH)

        self.assertEqual(
            recipe["schema"],
            "iggy3d.pattern.rope_plant_fibre.v1",
        )
        self.assertEqual(recipe["strand_count"], 3)
        self.assertEqual(recipe["lay_direction"], "right_hand")
        self.assertEqual(recipe["yarn_direction"], "left_hand")
        self.assertEqual(recipe["variation_count"], 4)
        self.assertEqual(
            [preset["id"] for preset in recipe["presets"]],
            ["fine_lashing", "utility_line", "heavy_hawser"],
        )
        for preset in recipe["presets"]:
            ratio = preset["lay_length_m"] / preset["diameter_m"]
            self.assertGreaterEqual(ratio, 2.79)
            self.assertLessEqual(ratio, 2.84)
            self.assertGreaterEqual(
                preset["represented_yarn_groups_per_strand"],
                7,
            )

    def test_material_preserves_separate_strand_yarn_and_fibre_layers(self):
        recipe = generator.load_pattern_recipe(PATTERN_PATH)
        materials = generator.generate_material(
            recipe,
            resolution_x=384,
            resolution_y=128,
        )

        self.assertEqual(set(materials), {
            "fine_lashing",
            "utility_line",
            "heavy_hawser",
        })
        for preset_id, material in materials.items():
            with self.subTest(preset=preset_id):
                self.assertEqual(
                    material["base_color_linear"].shape,
                    (512, 384, 3),
                )
                self.assertEqual(material["normal"].shape, (512, 384, 3))
                self.assertEqual(material["roughness"].shape, (512, 384))
                self.assertEqual(material["height_m"].shape, (512, 384))
                self.assertEqual(material["identity_masks"].shape, (512, 384, 3))
                self.assertEqual(
                    material["strand_identity_masks"].shape,
                    (512, 384, 3),
                )
                self.assertEqual(
                    material["fibre_detail_masks"].shape,
                    (512, 384, 3),
                )
                self.assertEqual(
                    material["response_masks"].shape,
                    (512, 384, 3),
                )
                self.assertEqual(
                    material["bundle_event_masks"].shape,
                    (512, 384, 3),
                )
                self.assertEqual(material["stylization"].shape, (512, 384, 3))
                self.assertEqual(
                    set(material["height_layers"]),
                    {
                        "strand_lobes",
                        "yarn_groups",
                        "long_fibre_ribbons",
                        "short_fibres",
                    },
                )
                strand = material["identity_masks"][..., 0]
                yarn = material["identity_masks"][..., 1]
                fibre = material["identity_masks"][..., 2]
                self.assertGreater(float(strand.std()), 0.18)
                self.assertGreater(float(yarn.std()), 0.08)
                self.assertGreater(float(fibre.std()), 0.015)
                self.assertLess(
                    abs(float(np.corrcoef(yarn.ravel(), fibre.ravel())[0, 1])),
                    0.92,
                )
                long_fibres = material["fibre_detail_masks"][..., 0]
                short_fibres = material["fibre_detail_masks"][..., 1]
                flyaway_spawn = material["fibre_detail_masks"][..., 2]
                self.assertGreater(float(long_fibres.std()), 0.01)
                self.assertGreater(float(short_fibres.std()), 0.002)
                self.assertLess(float((short_fibres > 0.12).mean()), 0.12)
                self.assertTrue((flyaway_spawn <= short_fibres + 1.0e-6).all())

    def test_cross_section_has_three_macro_strand_crowns(self):
        recipe = generator.load_pattern_recipe(PATTERN_PATH)
        material = generator.generate_material(
            recipe,
            resolution_x=384,
            resolution_y=192,
        )["utility_line"]
        strand = material["identity_masks"][:192, :, 0]

        for column in (35, 143, 278):
            profile = strand[:, column]
            peaks = _count_circular_peaks(profile, threshold=0.78)
            self.assertEqual(peaks, 3)
            crown_widths = _circular_run_widths(profile > 0.72)
            self.assertEqual(len(crown_widths), 3)
            self.assertGreaterEqual(max(crown_widths) - min(crown_widths), 2)

    def test_variations_are_seam_safe_nonidentical_and_have_quiet_passages(self):
        recipe = generator.load_pattern_recipe(PATTERN_PATH)
        material = generator.generate_material(
            recipe,
            resolution_x=512,
            resolution_y=128,
        )["heavy_hawser"]

        variations = np.split(material["base_color_linear"], 4, axis=0)
        for variation in variations:
            x_seam = float(np.mean(np.abs(variation[:, 0] - variation[:, -1])))
            y_seam = float(np.mean(np.abs(variation[0] - variation[-1])))
            x_internal = float(
                np.mean(np.abs(variation[:, 1:] - variation[:, :-1]))
            )
            y_internal = float(
                np.mean(np.abs(variation[1:] - variation[:-1]))
            )
            self.assertLess(x_seam, max(0.012, x_internal * 1.35))
            self.assertLess(y_seam, max(0.015, y_internal * 1.35))
        self.assertGreater(
            float(np.mean(np.abs(variations[0] - variations[1]))),
            0.002,
        )

        quiet = material["stylization"][..., 2]
        quiet_fraction = float((quiet > 0.72).mean())
        self.assertGreater(quiet_fraction, 0.25)
        self.assertLess(quiet_fraction, 0.48)

    def test_colour_and_response_are_layered_but_bounded(self):
        recipe = generator.load_pattern_recipe(PATTERN_PATH)
        material = generator.generate_material(
            recipe,
            resolution_x=384,
            resolution_y=128,
        )["utility_line"]

        self.assertTrue(np.isfinite(material["height_m"]).all())
        self.assertTrue(np.isfinite(material["normal"]).all())
        self.assertTrue(
            ((0.66 <= material["roughness"])
             & (material["roughness"] <= 0.86)).all()
        )
        unique_colours = np.unique(
            np.rint(material["base_color_srgb"] * 255.0)
            .astype(np.uint8)
            .reshape(-1, 3),
            axis=0,
        )
        self.assertGreaterEqual(unique_colours.shape[0], 20)
        height_roughness_correlation = np.corrcoef(
            material["height_m"].ravel(),
            material["roughness"].ravel(),
        )[0, 1]
        self.assertLess(abs(float(height_roughness_correlation)), 0.72)

        diameter = 0.032
        maximum_relief = float(material["height_m"].max() - material["height_m"].min())
        self.assertGreater(maximum_relief, diameter * 0.025)
        self.assertLess(maximum_relief, diameter * 0.10)

    def test_generate_writes_all_texture_channels_without_geometry(self):
        with TemporaryDirectory() as temporary:
            manifest = generator.generate(
                Path(temporary),
                resolution_x=320,
                resolution_y=96,
            )

            self.assertEqual(
                manifest["schema"],
                "iggy-rope-plant-fibre-build/1.0",
            )
            self.assertFalse(manifest["constraints"]["uses_ai_generated_imagery"])
            self.assertFalse(manifest["constraints"]["generates_geometry"])
            self.assertEqual(set(manifest["presets"]), {
                "fine_lashing",
                "utility_line",
                "heavy_hawser",
            })
            for preset in manifest["presets"].values():
                self.assertEqual(
                    set(preset["outputs"]),
                    {
                        "base_color",
                        "normal",
                        "height",
                        "orm",
                        "identity_masks",
                        "strand_identities",
                        "fibre_detail_masks",
                        "response_masks",
                        "bundle_event_masks",
                        "stylization",
                        "proof",
                        "cylinder_proof",
                    },
                )
                for output in preset["outputs"].values():
                    path = Path(output["path"])
                    self.assertTrue(path.is_file())
                    self.assertGreater(path.stat().st_size, 100)
                self.assertEqual(
                    _png_header(Path(preset["outputs"]["height"]["path"]))[
                        "bit_depth"
                    ],
                    16,
                )


def _count_circular_peaks(values: np.ndarray, threshold: float) -> int:
    before = np.roll(values, 1)
    after = np.roll(values, -1)
    return int(((values > before) & (values >= after) & (values > threshold)).sum())


def _circular_run_widths(mask: np.ndarray) -> list[int]:
    values = np.asarray(mask, dtype=bool)
    if values.all():
        return [len(values)]
    start = int(np.flatnonzero(~values)[0])
    rotated = np.roll(values, -start)
    widths: list[int] = []
    active = 0
    for value in rotated:
        if value:
            active += 1
        elif active:
            widths.append(active)
            active = 0
    if active:
        widths.append(active)
    return widths


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
