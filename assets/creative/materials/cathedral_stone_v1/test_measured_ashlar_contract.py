#!/usr/bin/env python3
"""Focused contract tests for the measured intact-ashlar construction pass."""

from __future__ import annotations

import json
import math
from pathlib import Path
import sys
import tempfile
import unittest

import numpy as np


ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from generate_cathedral_stone_v1 import (
    generate_material,
    write_material_package,
)


PATTERN = ROOT / "patterns" / "cathedral_ashlar_courses_v1.json"
PROFILE = ROOT / "profiles" / "cathedral_stone_v1.json"
CAPTURE = ROOT / "references" / "santa_marina_biocalcarenite_capture.json"
LEDGER = ROOT.parent / "reference_measurements_v1.json"


def circular_distance(a: float, b: float, period: float) -> float:
    delta = abs(a - b) % period
    return min(delta, period - delta)


class MeasuredAshlarContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.pattern = json.loads(PATTERN.read_text())
        cls.profile = json.loads(PROFILE.read_text())
        cls.capture = json.loads(CAPTURE.read_text())
        cls.ledger = json.loads(LEDGER.read_text())
        cls.stone_measurements = {
            entry["id"]: entry
            for entry in cls.ledger["measurements"]["stone"]
        }
        cls.material = generate_material(resolution=256)

    def test_recipe_names_its_measurement_authorities(self) -> None:
        self.assertEqual(
            self.pattern["schema"],
            "iggy3d.pattern.measured_cathedral_ashlar.v2",
        )
        authority = self.pattern["measurement_authority"]
        self.assertEqual(
            authority["dimension_measurement_id"],
            "cordoba_early_gothic_ashlar",
        )
        self.assertEqual(
            authority["joint_measurement_id"],
            "ashlar_joint_3mm_conservation_guide",
        )
        self.assertEqual(
            authority["tooling_measurement_id"],
            "medieval_caen_stone_tooling",
        )
        self.assertEqual(
            authority["tool_finish_measurement_id"],
            "medieval_fine_oblique_layage",
        )
        self.assertEqual(
            authority["construction_scope"],
            "single facing wythe; wall core is not inferred",
        )

    def test_every_block_is_inside_the_surveyed_dimension_envelope(self) -> None:
        measured = self.stone_measurements["cordoba_early_gothic_ashlar"]
        length_range = measured["length_m"]["observed_range"]
        height_range = measured["face_height_m"]["observed_range"]
        depth_range = measured["depth_m"]["observed_range"]
        lengths: list[float] = []
        heights: list[float] = []
        depths: list[float] = []
        for course in self.pattern["courses"]:
            heights.append(float(course["face_height_m"]))
            for block in course["blocks"]:
                lengths.append(float(block["face_length_m"]))
                depths.append(float(block["depth_m"]))
        self.assertTrue(all(length_range[0] <= value <= length_range[1] for value in lengths))
        self.assertTrue(all(height_range[0] <= value <= height_range[1] for value in heights))
        self.assertTrue(all(depth_range[0] <= value <= depth_range[1] for value in depths))
        self.assertGreaterEqual(len(lengths), 40)

    def test_courses_and_cyclic_rows_close_to_exact_module_dimensions(self) -> None:
        width = float(self.pattern["module_m"]["width"])
        height = float(self.pattern["module_m"]["height"])
        bed = float(self.pattern["mortar"]["bed_joint_m"])
        perpend = float(self.pattern["mortar"]["perpend_joint_m"])
        self.assertAlmostEqual(
            sum(float(course["face_height_m"]) + bed for course in self.pattern["courses"]),
            height,
            places=9,
        )
        for course in self.pattern["courses"]:
            self.assertAlmostEqual(
                sum(float(block["face_length_m"]) + perpend for block in course["blocks"]),
                width,
                places=9,
            )

    def test_broken_bond_places_each_joint_over_a_block_body(self) -> None:
        width = float(self.pattern["module_m"]["width"])
        perpend = float(self.pattern["mortar"]["perpend_joint_m"])
        rows: list[list[float]] = []
        for course in self.pattern["courses"]:
            cursor = float(course["bond_offset_m"]) % width
            joints = []
            for block in course["blocks"]:
                joints.append(cursor)
                cursor = (
                    cursor + float(block["face_length_m"]) + perpend
                ) % width
            rows.append(sorted(joints))
        minimum_lap = float(self.pattern["bond"]["minimum_joint_separation_m"])
        adjacent_pairs = list(zip(rows, rows[1:]))
        adjacent_pairs.append((rows[-1], rows[0]))
        for lower, upper in adjacent_pairs:
            closest = min(
                circular_distance(a, b, width)
                for a in lower
                for b in upper
            )
            self.assertGreaterEqual(closest, minimum_lap)

    def test_intact_base_has_flush_mortar_and_no_damage(self) -> None:
        self.assertEqual(float(self.pattern["mortar"]["recess_m"]), 0.0)
        self.assertEqual(self.pattern["mortar"]["profile"], "flush")
        self.assertEqual(
            self.pattern["excluded_from_intact_core"],
            [
                "chips",
                "fractures",
                "edge_loss",
                "lichen",
                "damp",
                "soot",
                "carved_ornament",
            ],
        )

    def test_tool_spacing_uses_the_measured_caen_envelope(self) -> None:
        measured = self.stone_measurements["medieval_caen_stone_tooling"]
        low, high = measured["tool_mark_spacing_m"]
        tooling = self.pattern["surface_tooling"]
        self.assertEqual(tooling["spacing_range_m"], [low, high])
        for course in self.pattern["courses"]:
            for block in course["blocks"]:
                spacing = float(block["tool_spacing_m"])
                self.assertTrue(low <= spacing <= high)
        self.assertEqual(
            tooling["depth_status"],
            "unknown; no geometric or normal depth is authored",
        )

    def test_finite_layage_marks_use_the_measured_tool_and_density_envelope(
        self,
    ) -> None:
        measured = self.stone_measurements["medieval_fine_oblique_layage"]
        tooling = self.pattern["surface_tooling"]
        self.assertEqual(
            tooling["straight_hammer_edge_length_range_m"],
            measured["straight_hammer_edge_length_range_m"],
        )
        self.assertEqual(
            tooling["fine_impact_width_range_m"],
            measured["fine_impact_width_range_m"],
        )
        self.assertEqual(
            tooling["visible_impact_density_per_m2"],
            measured["visible_impact_density_per_m2"],
        )
        self.assertEqual(tooling["detail_span_m"], 0.256)
        self.assertEqual(
            tooling["organization"],
            "oblique parallel passes with slight fan variation",
        )
        self.assertIn("not a Cordoba", measured["use_limit"])

    def test_surface_capture_uses_the_selected_church_and_retains_license(self) -> None:
        self.assertEqual(
            self.capture["schema"],
            "iggy3d.material_reference_capture.cathedral_stone.v2",
        )
        source = self.capture["source"]
        self.assertEqual(
            source["asset_id"],
            "Santa_Marina_Aguas_Santas_2024_facade",
        )
        self.assertEqual(source["license"], "CC BY-SA 4.0")
        self.assertEqual(
            source["attribution"],
            "Benjamin Smith / Wikimedia Commons",
        )
        self.assertEqual(source["resolution"], [5688, 4493])
        self.assertFalse(
            self.capture["production_translation"][
                "raw_pixels_used_as_runtime_texture"
            ]
        )
        self.assertFalse(
            self.capture["production_translation"][
                "source_pixels_retained_in_repository"
            ]
        )
        self.assertEqual(
            self.capture["photographic_color"]["measurement_limit"],
            "relative sunlit color families only; not absolute albedo, roughness, height, or normal",
        )

    def test_stone_body_is_the_measured_naranjo_biocalcarenite(self) -> None:
        measured = self.stone_measurements[
            "santa_marina_naranjo_biocalcarenite_body"
        ]
        body = self.capture["stone_body"]
        self.assertEqual(
            body["measurement_id"],
            "santa_marina_naranjo_biocalcarenite_body",
        )
        self.assertEqual(body["porosity_fraction"], measured["porosity_fraction"])
        self.assertEqual(
            body["fossil_component_fraction"],
            measured["fossil_component_fraction"],
        )
        fractions = body["nondegraded_naranjo_mineral_fractions"]
        self.assertAlmostEqual(sum(fractions.values()), 1.0, places=9)
        self.assertEqual(
            fractions,
            {
                "calcite": 0.8,
                "quartz": 0.15,
                "feldspar": 0.03,
                "clay": 0.02,
            },
        )
        self.assertEqual(
            body["active_lithotypes"],
            [
                "clastic sandy yellowish biocalcarenite",
                "sandy fine-grained biomicrite",
            ],
        )
        self.assertEqual(
            body["explicit_variant_only_lithotypes"],
            [
                "conglomeratic biocalcarenite",
                "biosparite",
            ],
        )

    def test_shader_contract_separates_architecture_and_material_body_scale(self) -> None:
        scale = self.profile["surface_scale_contract"]
        self.assertEqual(scale["construction_macro_span_m"], 4.0)
        self.assertEqual(scale["material_body_detail_span_m"], 0.064)
        self.assertEqual(
            scale["material_body_decorrelation_span_m"],
            0.091,
        )
        self.assertLessEqual(
            scale["material_body_detail_m_per_texel_at_1024"],
            0.0000625,
        )
        self.assertEqual(
            scale["material_body_channels"],
            "fossil_fragment silicate_grain visible_pore_identity",
        )
        self.assertIn(
            "per-block phase",
            scale["anti_repetition_rule"],
        )
        self.assertIn(
            "incommensurate 91 mm",
            scale["anti_repetition_rule"],
        )

    def test_intact_pbr_boundary_does_not_invent_surface_shape(self) -> None:
        pbr = self.profile["intact_pbr_contract"]
        self.assertEqual(pbr["metallic"], 0.0)
        self.assertEqual(pbr["specular"], 0.5)
        self.assertEqual(
            pbr["normal_and_height"],
            "stone-body relief uses the measured Sabucina calcarenite proxy; block planes, tool marks, arrises, mortar, and damage remain neutral until separately measured",
        )
        self.assertEqual(
            pbr["roughness_status"],
            "authored calibration under neutral and grazing light; no published optical roughness measurement",
        )
        self.assertEqual(
            pbr["base_color_rule"],
            "intrinsic stone color only; no photographed sunlight, shadow, cavity, or ambient occlusion",
        )

    def test_relief_proxy_is_scale_bearing_and_explicitly_limited(self) -> None:
        self.assertEqual(
            self.profile["schema"],
            "iggy3d.material.cathedral_stone_v1.profile.v6",
        )
        relief = self.stone_measurements[
            "sabucina_calcarenite_surface_topography_proxy"
        ]
        self.assertEqual(relief["source"], "S24_sabucina_surface_metrology")
        self.assertEqual(relief["evidence"], "laboratory_characterized")
        self.assertEqual(relief["scan_area_m"], [0.003, 0.0015])
        self.assertEqual(relief["roughness_cutoff_m"], 0.0008)
        self.assertEqual(
            relief["fresh_reference_unfiltered"]["sa_m"],
            0.00010,
        )
        self.assertEqual(
            relief["fresh_reference_unfiltered"]["sq_m"],
            0.00013,
        )
        self.assertEqual(
            relief["fresh_reference_unfiltered"]["sz_m"],
            0.00113,
        )
        self.assertIn("proxy", relief["use_limit"].lower())
        self.assertIn("not", relief["use_limit"].lower())
        contract = self.profile["surface_relief_contract"]
        self.assertEqual(
            contract["proxy_measurement_id"],
            relief["id"],
        )
        self.assertEqual(contract["body_detail_span_m"], 0.064)
        self.assertEqual(contract["roughness_cutoff_m"], 0.0008)
        self.assertFalse(contract["author_tool_mark_depth"])
        self.assertFalse(contract["author_damage_depth"])

    def test_generated_intact_surface_keeps_architecture_and_ao_neutral(self) -> None:
        self.assertTrue(np.allclose(self.material["height_m"], 0.0))
        self.assertTrue(np.allclose(self.material["normal"][..., :2], 0.0))
        self.assertTrue(np.allclose(self.material["normal"][..., 2], 1.0))
        self.assertTrue(np.allclose(self.material["ao"], 1.0))
        self.assertTrue(np.allclose(self.material["metallic"], 0.0))
        self.assertGreaterEqual(float(self.material["roughness"].min()), 0.66)
        self.assertLessEqual(float(self.material["roughness"].max()), 0.82)

    def test_tooling_is_finite_scale_bearing_linework_without_fake_depth(
        self,
    ) -> None:
        masks = self.material["tooling_detail_masks"]
        metrics = self.material["tooling_detail_metrics"]
        self.assertEqual(masks.shape, (256, 256, 3))
        self.assertEqual(self.material["tooling_detail_span_m"], 0.256)
        self.assertTrue(np.all(np.isfinite(masks)))
        self.assertTrue(np.allclose(self.material["tooling_height_m"], 0.0))
        self.assertEqual(metrics["variant_count"], 3)
        self.assertEqual(
            metrics["target_density_per_m2"],
            [720.0, 1369.0, 2420.0],
        )
        self.assertEqual(metrics["stroke_counts"], [47, 90, 159])
        self.assertGreaterEqual(metrics["stroke_length_range_m"][0], 0.060)
        self.assertLessEqual(metrics["stroke_length_range_m"][1], 0.094)
        self.assertGreaterEqual(metrics["stroke_width_range_m"][0], 0.001)
        self.assertLessEqual(metrics["stroke_width_range_m"][1], 0.002)
        self.assertGreaterEqual(metrics["intra_pass_spacing_range_m"][0], 0.001)
        self.assertLessEqual(metrics["intra_pass_spacing_range_m"][1], 0.004)
        for channel in range(3):
            coverage = float((masks[..., channel] > 0.08).mean())
            self.assertGreater(coverage, 0.02)
            self.assertLess(coverage, 0.48)

    def test_stone_body_relief_matches_the_measured_proxy_envelope(self) -> None:
        height = self.material["stone_body_height_m"]
        normal = self.material["stone_body_normal"]
        metrics = self.material["stone_body_relief_metrics"]
        self.assertEqual(height.shape, (256, 256))
        self.assertEqual(normal.shape, (256, 256, 3))
        self.assertTrue(np.all(np.isfinite(height)))
        self.assertTrue(np.all(np.isfinite(normal)))
        self.assertGreater(float(np.max(np.abs(normal[..., :2]))), 0.05)
        self.assertAlmostEqual(
            float(height.max() - height.min()),
            0.00113,
            delta=0.00003,
        )
        self.assertAlmostEqual(metrics["sa_m"], 0.00010, delta=0.000035)
        self.assertAlmostEqual(metrics["sq_m"], 0.00013, delta=0.000035)
        self.assertAlmostEqual(metrics["ssk"], -0.39, delta=0.35)
        self.assertGreaterEqual(metrics["sku"], 2.5)
        self.assertLessEqual(metrics["sku"], 5.0)
        self.assertLessEqual(metrics["height_seam_m"], 0.00015)
        self.assertEqual(
            self.material["stone_body_relief_span_m"],
            0.064,
        )

    def test_output_package_preserves_16_bit_height_and_proxy_provenance(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            manifest = write_material_package(
                self.material,
                output_root=directory,
            )
            output = Path(directory)
            body_height = output / "cathedral_stone_v1_stone_body_height.png"
            body_normal = output / "cathedral_stone_v1_stone_body_normal.png"
            tooling_detail = output / "cathedral_stone_v1_tooling_detail.png"
            self.assertTrue(body_height.is_file())
            self.assertTrue(body_normal.is_file())
            self.assertTrue(tooling_detail.is_file())
            header = body_height.read_bytes()[:25]
            self.assertEqual(header[:8], b"\x89PNG\r\n\x1a\n")
            self.assertEqual(header[24], 16)
            self.assertEqual(
                manifest["schema"],
                "iggy3d.material.cathedral_stone_v1.v6",
            )
            self.assertEqual(
                manifest["tooling_finish"]["finish_measurement_id"],
                "medieval_fine_oblique_layage",
            )
            self.assertEqual(
                manifest["tooling_finish"]["depth_status"],
                "unknown; color and roughness linework only",
            )
            stone_body = manifest["stone_body"]
            self.assertEqual(
                stone_body["relief_proxy_measurement_id"],
                "sabucina_calcarenite_surface_topography_proxy",
            )
            self.assertAlmostEqual(
                stone_body["height_amplitude_m"],
                0.00113,
                delta=0.00003,
            )
            self.assertEqual(
                stone_body["excluded_relief_lanes"],
                [
                    "block plane",
                    "tool mark",
                    "arris",
                    "mortar",
                    "damage",
                    "weathering",
                ],
            )
            self.assertFalse(
                manifest["source_policy"]["relief_proxy_is_exact_lithology"]
            )

    def test_material_body_masks_match_their_distinct_evidence_roles(self) -> None:
        masks = self.material["stone_body_masks"]
        self.assertEqual(masks.shape, (256, 256, 3))
        self.assertEqual(self.material["stone_body_detail_span_m"], 0.064)
        fossil_mean = float(self.material["fossil_fragment_mask"].mean())
        silicate_mean = float(self.material["silicate_grain_mask"].mean())
        pore_mean = float(
            self.material["visible_pore_identity_mask"].mean()
        )
        self.assertTrue(0.27 <= fossil_mean <= 0.43)
        self.assertTrue(0.13 <= silicate_mean <= 0.23)
        self.assertTrue(0.015 <= pore_mean <= 0.06)
        self.assertLess(pore_mean, 0.13)


if __name__ == "__main__":
    unittest.main()
