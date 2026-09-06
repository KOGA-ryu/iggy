#!/usr/bin/env python3
"""Contract tests for the measured chevron-voussoir texture generator."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zlib

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
PACKAGE = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "cathedral_stone_trim_fracture_v1"
)
GENERATOR_PATH = PACKAGE / "generate_cathedral_stone_trim_fracture_v1.py"
AUDITOR_PATH = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "workflow"
    / "scripts"
    / "audit_material_package.py"
)
PROFILE_PATH = PACKAGE / "profiles" / "cathedral_stone_trim_fracture_v1.json"
PATTERN_PATH = PACKAGE / "patterns" / "cathedral_trim_fracture_atlas_v1.json"
CORE_OUTPUT = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "cathedral_stone_v1"
    / "output"
)


def load_module() -> object:
    spec = importlib.util.spec_from_file_location(
        "chevron_generator_under_test",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("Cannot load generator module")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_auditor() -> object:
    spec = importlib.util.spec_from_file_location(
        "material_auditor_under_test",
        AUDITOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("Cannot load material package auditor")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def read_json(path: Path) -> dict:
    return json.loads(path.read_text())


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def decode_filter_zero_rgb8(path: Path) -> np.ndarray:
    payload = path.read_bytes()
    if payload[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(f"{path} is not a PNG")
    cursor = 8
    width = height = bit_depth = color_type = None
    compressed = bytearray()
    while cursor < len(payload):
        length = struct.unpack(">I", payload[cursor : cursor + 4])[0]
        chunk_type = payload[cursor + 4 : cursor + 8]
        chunk = payload[cursor + 8 : cursor + 8 + length]
        cursor += 12 + length
        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type = struct.unpack(
                ">IIBB",
                chunk[:10],
            )
        elif chunk_type == b"IDAT":
            compressed.extend(chunk)
        elif chunk_type == b"IEND":
            break
    if (
        width is None
        or height is None
        or bit_depth != 8
        or color_type != 2
    ):
        raise AssertionError("Expected an RGB8 PNG")
    raw = zlib.decompress(bytes(compressed))
    stride = width * 3
    result = np.empty((height, width, 3), dtype=np.uint8)
    for row in range(height):
        start = row * (stride + 1)
        if raw[start] != 0:
            raise AssertionError("Expected filter-zero scanlines")
        result[row] = np.frombuffer(
            raw[start + 1 : start + 1 + stride],
            dtype=np.uint8,
        ).reshape(width, 3)
    return result


def png_header(path: Path) -> tuple[int, int, int, int]:
    payload = path.read_bytes()
    if payload[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(f"{path} is not a PNG")
    return struct.unpack(">IIBB", payload[16:26])


class ChevronGeneratorContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.generator = load_module()
        cls.auditor = load_auditor()
        cls.profile = read_json(PROFILE_PATH)
        cls.pattern = read_json(PATTERN_PATH)

    def test_source_bounded_voussoir_and_authored_completion_are_explicit(
        self,
    ) -> None:
        source = self.pattern["source_measurement"]
        self.assertEqual(source["source_id"], "S01_old_sarum_catalogue_item_55")
        self.assertEqual(source["catalogue_item"], 55)
        self.assertEqual(
            {
                "height_m": source["height_m"],
                "inner_chord_m": source["inner_chord_m"],
                "outer_chord_m": source["outer_chord_m"],
                "depth_m": source["depth_m"],
            },
            {
                "height_m": 0.2,
                "inner_chord_m": 0.14,
                "outer_chord_m": 0.18,
                "depth_m": 0.27,
            },
        )
        completion = self.pattern["authored_completion"]
        self.assertEqual(completion["count"], 16)
        self.assertAlmostEqual(completion["pitch_angle_deg"], 11.25, places=12)
        self.assertAlmostEqual(
            completion["joint_gap_centerline_m"],
            0.003,
            places=12,
        )
        self.assertIn("solved iteratively", completion["derivation"])

    def test_derived_radius_chords_and_joint_recompute_without_guessing(
        self,
    ) -> None:
        completion = self.pattern["authored_completion"]
        height = self.pattern["source_measurement"]["height_m"]
        body_angle = math.radians(completion["body_angle_deg"])
        pitch_angle = math.radians(completion["pitch_angle_deg"])
        inner_radius = completion["inner_radius_m"]
        outer_radius = completion["outer_radius_m"]
        inner_chord = 2.0 * inner_radius * math.sin(body_angle / 2.0)
        outer_chord = 2.0 * outer_radius * math.sin(body_angle / 2.0)
        mean_radius = inner_radius + height / 2.0
        gap = mean_radius * (pitch_angle - body_angle)
        self.assertAlmostEqual(inner_chord, 0.14, places=10)
        self.assertAlmostEqual(outer_chord, 0.1784852529, places=10)
        self.assertLessEqual(abs(outer_chord - 0.18), 0.005)
        self.assertAlmostEqual(gap, 0.003, places=10)
        self.assertAlmostEqual(outer_radius - inner_radius, 0.2, places=12)

    def test_moulding_is_one_centripetal_roll_hollow_roll_chevron_per_stone(
        self,
    ) -> None:
        moulding = self.pattern["moulding"]
        self.assertEqual(
            moulding["classification"],
            "centripetal lateral face chevron",
        )
        self.assertEqual(moulding["chevrons_per_voussoir"], 1)
        self.assertEqual(
            moulding["profile"],
            "quiet-roll-hollow-roll-quiet",
        )
        self.assertEqual(
            moulding["radial_zone_widths_m"],
            [0.025, 0.05, 0.05, 0.05, 0.025],
        )
        self.assertAlmostEqual(sum(moulding["radial_zone_widths_m"]), 0.2)
        self.assertEqual(moulding["roll_crest_m"], 0.012)
        self.assertEqual(moulding["hollow_depression_m"], 0.006)
        self.assertEqual(moulding["bevel_m"], 0.0)

    def test_first_capability_excludes_all_unproven_damage_and_jambs(
        self,
    ) -> None:
        self.assertEqual(
            set(self.pattern["exclusions"]),
            {
                "jamb reconstruction",
                "fracture",
                "edge damage",
                "wear",
                "weathering",
                "soot",
                "damp",
                "lichen",
                "Unreal parity",
            },
        )
        surface = self.profile["surface_contract"]
        self.assertIn("absent", surface["damage_rule"])
        self.assertEqual(surface["ao_rule"], (
            "Texture AO remains one. Only real geometry may produce "
            "contact occlusion."
        ))

    def test_texture_inventory_has_exact_scale_bit_depth_and_color_space(
        self,
    ) -> None:
        inventory = self.profile["texture_inventory"]
        self.assertEqual(
            set(inventory),
            {
                "basecolor",
                "orm",
                "body_masks",
                "body_normal",
                "body_height",
            },
        )
        for lane, entry in inventory.items():
            self.assertEqual(entry["resolution"], [1024, 1024], lane)
            self.assertAlmostEqual(
                entry["metres_per_texel"],
                entry["physical_span_m"] / 1024,
                places=12,
            )
        self.assertEqual(inventory["basecolor"]["color_space"], "sRGB")
        self.assertEqual(inventory["basecolor"]["bit_depth"], 8)
        self.assertEqual(inventory["body_height"]["bit_depth"], 16)
        for lane in ("orm", "body_masks", "body_normal", "body_height"):
            self.assertEqual(inventory[lane]["color_space"], "Non-Color")

    def test_generated_family_is_deterministic_and_preserves_core_body_bytes(
        self,
    ) -> None:
        core_files = {
            lane: CORE_OUTPUT / f"cathedral_stone_v1_stone_{lane}.png"
            for lane in ("body_masks", "body_normal", "body_height")
        }
        core_hashes_before = {
            lane: sha256(path) for lane, path in core_files.items()
        }
        with tempfile.TemporaryDirectory() as first_dir, tempfile.TemporaryDirectory() as second_dir:
            first = Path(first_dir)
            second = Path(second_dir)
            manifest_a = self.generator.generate(
                PROFILE_PATH,
                PATTERN_PATH,
                first,
            )
            manifest_b = self.generator.generate(
                PROFILE_PATH,
                PATTERN_PATH,
                second,
            )
            self.assertEqual(manifest_a["schema"], manifest_b["schema"])
            self.assertEqual(
                {
                    lane: entry["sha256"]
                    for lane, entry in manifest_a["files"].items()
                },
                {
                    lane: entry["sha256"]
                    for lane, entry in manifest_b["files"].items()
                },
            )
            for lane, core_hash in core_hashes_before.items():
                self.assertEqual(
                    sha256(
                        first
                        / f"cathedral_stone_trim_fracture_v1_{lane}.png"
                    ),
                    core_hash,
                )
        self.assertEqual(
            {
                lane: sha256(path) for lane, path in core_files.items()
            },
            core_hashes_before,
        )

    def test_basecolor_contains_broad_related_variation_not_baked_light(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            self.generator.generate(PROFILE_PATH, PATTERN_PATH, output)
            image = decode_filter_zero_rgb8(
                output / "cathedral_stone_trim_fracture_v1_basecolor.png"
            )
        luminance = (
            image[..., 0].astype(np.float32) * 0.2126
            + image[..., 1].astype(np.float32) * 0.7152
            + image[..., 2].astype(np.float32) * 0.0722
        )
        unique = np.unique(image.reshape(-1, 3), axis=0)
        self.assertGreater(len(unique), 400)
        self.assertGreater(float(luminance.std()), 4.0)
        self.assertLess(float(luminance.max() - luminance.min()), 75.0)
        self.assertGreaterEqual(int(image.min()), 80)
        self.assertLessEqual(int(image.max()), 230)

    def test_orm_is_dielectric_with_quiet_bounded_roughness(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            self.generator.generate(PROFILE_PATH, PATTERN_PATH, output)
            orm = decode_filter_zero_rgb8(
                output / "cathedral_stone_trim_fracture_v1_orm.png"
            )
        self.assertTrue(np.all(orm[..., 0] == 255))
        self.assertTrue(np.all(orm[..., 2] == 0))
        roughness = orm[..., 1].astype(np.float32) / 255.0
        self.assertGreaterEqual(float(roughness.min()), 0.66 - 1 / 255)
        self.assertLessEqual(float(roughness.max()), 0.82 + 1 / 255)
        self.assertGreater(float(roughness.std()), 0.005)

    def test_output_png_headers_match_declared_lane_formats(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            self.generator.generate(PROFILE_PATH, PATTERN_PATH, output)
            base = png_header(
                output / "cathedral_stone_trim_fracture_v1_basecolor.png"
            )
            orm = png_header(
                output / "cathedral_stone_trim_fracture_v1_orm.png"
            )
            masks = png_header(
                output / "cathedral_stone_trim_fracture_v1_body_masks.png"
            )
            normal = png_header(
                output / "cathedral_stone_trim_fracture_v1_body_normal.png"
            )
            height = png_header(
                output / "cathedral_stone_trim_fracture_v1_body_height.png"
            )
        self.assertEqual(base, (1024, 1024, 8, 2))
        self.assertEqual(orm, (1024, 1024, 8, 2))
        self.assertEqual(masks, (1024, 1024, 8, 2))
        self.assertEqual(normal, (1024, 1024, 8, 2))
        self.assertEqual(height, (1024, 1024, 16, 0))

    def test_workflow_claim_ledger_covers_every_numeric_claim_root(self) -> None:
        contract = self.profile["workflow_contract"]
        passed, detail = self.auditor.check_measurement_claims(
            contract,
            {"profile": self.profile, "pattern": self.pattern},
        )
        self.assertTrue(passed, detail["errors"])
        self.assertEqual(contract["tier"], "hero-master")
        self.assertEqual(
            contract["delivery_state"],
            "production-candidate",
        )
        self.assertGreaterEqual(detail["claim_count"], 30)

    def test_workflow_budget_and_acceptance_boundary_are_explicit(self) -> None:
        contract = self.profile["workflow_contract"]
        budget = contract["performance_budget"]
        self.assertEqual(budget["max_product_objects"], 16)
        self.assertEqual(budget["max_unique_images"], 5)
        self.assertEqual(budget["max_proof_count"], 9)
        self.assertTrue(
            contract["acceptance"][
                "actual_target_required_for_acceptance"
            ]
        )
        self.assertEqual(
            contract["acceptance"]["isolated_fixture_can_reach"],
            "production-candidate",
        )


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
