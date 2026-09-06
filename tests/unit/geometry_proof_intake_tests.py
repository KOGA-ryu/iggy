"""Tests for the research->harness intake bridge.

The load-bearing test is the fig 6 round trip: the handoff example (plate
pixels, the research thread's vocabulary) must produce the same station
coordinates the pilot spec was hand-authored with, and the emitted draft must
HONESTLY fail structural validation until segments are declared.
"""

import copy
import json
import math
import sys
import unittest
from pathlib import Path

sys.dont_write_bytecode = True  # keep __pycache__ out of the asset package

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "assets" / "creative" / "geometry_proof"
sys.path.insert(0, str(PACKAGE))

from profile_intake import (  # noqa: E402
    HANDOFF_FORMAT_V2, build_draft, fit_arc, resolve_to_root, validate_handoff)
from profile_spec import SpecError, validate_structure  # noqa: E402

EXAMPLE = PACKAGE / "handoffs" / "EXAMPLE_paley_pl1_fig6_handoff.json"
PILOT = PACKAGE / "profiles" / "paley_pl1_fig6_v1.json"


def example():
    return json.loads(EXAMPLE.read_text())


class HandoffValidationTests(unittest.TestCase):
    def test_example_is_valid(self):
        self.assertEqual(validate_handoff(example()), [])

    def test_unknown_key_rejected(self):
        h = example()
        h["landmarks"][0]["pixel"] = [1, 2]
        self.assertTrue(any("pixel" in p for p in validate_handoff(h)))

    def test_bad_status_rejected(self):
        h = example()
        h["extents"][0]["status"] = "PRETTY_SURE"
        self.assertTrue(any("status" in p for p in validate_handoff(h)))

    def test_unknown_between_landmark_rejected(self):
        h = example()
        h["extents"][1]["between"] = ["wall_start", "nonexistent"]
        self.assertTrue(any("between" in p for p in validate_handoff(h)))

    def test_two_root_frames_rejected(self):
        h = example()
        h["frames"]["other"] = {"parent": None}
        self.assertTrue(any("root frame" in p for p in validate_handoff(h)))

    def test_nondefault_axes_rejected(self):
        h = example()
        h["profile_frame"]["axes"] = "rotated-90"
        self.assertTrue(any("axes" in p for p in validate_handoff(h)))

    def test_incomplete_handoff_errors_are_the_checklist(self):
        # The 451A-stub pattern: empty landmarks + placeholder origin must
        # produce actionable errors, not a crash.
        h = example()
        h["landmarks"] = []
        h["arc_traces"] = []
        h["extents"] = [e for e in h["extents"] if not e.get("between")]
        problems = validate_handoff(h)
        self.assertTrue(any("origin_landmark" in p for p in problems))

    def test_v2_names_uncertainty_honestly_and_carries_model_assumption(self):
        h = example()
        h["format"] = HANDOFF_FORMAT_V2
        rms = h["calibration"].pop("rms_residual_px")
        h["calibration"].update({
            "control_point_uncertainty_px": rms,
            "source_projection_model": "UNIFORM_ORTHOGRAPHIC",
            "model_assumption": {
                "name": "fronto_parallel_subject",
                "evidence_class": "AUTHORED",
                "note": "catalog photograph treated as fronto-parallel",
            },
        })
        self.assertEqual(validate_handoff(h), [])
        draft, _ = build_draft(h)
        self.assertAlmostEqual(draft["tolerance"]["distance"], 0.1, places=4)
        notes = draft["reference"]["notes"]
        self.assertIn("control-point uncertainty", notes)
        self.assertIn("fronto_parallel_subject (AUTHORED)", notes)

    def test_v2_rejects_the_legacy_misnamed_uncertainty_field(self):
        h = example()
        h["format"] = HANDOFF_FORMAT_V2
        h["calibration"]["source_projection_model"] = "PLANAR_HOMOGRAPHY"
        problems = validate_handoff(h)
        self.assertTrue(any("rms_residual_px" in p and "unknown" in p for p in problems))
        self.assertTrue(any("control_point_uncertainty_px" in p and "missing" in p
                            for p in problems))


class FrameResolutionTests(unittest.TestCase):
    FRAMES = {
        "plate": {"parent": None},
        "crop": {"parent": "plate", "offset_px": [520, 30], "scale": 2.0},
        "detail": {"parent": "crop", "offset_px": [100, 40], "scale": 3.0},
    }

    def test_chain_resolution(self):
        # detail px -> crop: 100 + x/3 ; crop -> plate: 520 + x/2
        self.assertEqual(resolve_to_root((0, 0), "detail", self.FRAMES), (570.0, 50.0))
        self.assertEqual(resolve_to_root((30, 60), "detail", self.FRAMES), (575.0, 60.0))

    def test_root_is_identity(self):
        self.assertEqual(resolve_to_root((7, 9), "plate", self.FRAMES), (7, 9))


class FitArcTests(unittest.TestCase):
    def test_recovers_synthetic_circle(self):
        pts = [(3.0 + 5.0 * math.cos(t), -2.0 + 5.0 * math.sin(t))
               for t in [0.1, 0.6, 1.1, 1.9, 2.6, 3.4, 4.2, 5.0]]
        fit = fit_arc(pts)
        self.assertAlmostEqual(fit["centre"][0], 3.0, places=9)
        self.assertAlmostEqual(fit["centre"][1], -2.0, places=9)
        self.assertAlmostEqual(fit["radius"], 5.0, places=9)
        self.assertLess(fit["rms_residual"], 1e-9)

    def test_colinear_points_rejected(self):
        with self.assertRaises(ValueError):
            fit_arc([(float(i), 2.0 * i) for i in range(8)])


class Fig6RoundTripTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.draft, cls.report = build_draft(example())
        cls.pilot = json.loads(PILOT.read_text())

    def test_stations_match_the_hand_authored_pilot(self):
        # px-exact stations must round-trip exactly; hollow_end must land on
        # the measured point, 0.021 in from the pilot's consistency-nudged one.
        for name, max_dist in (("wall_start", 1e-6), ("wall_end", 1e-3),
                               ("chamfer_top", 1e-3), ("hollow_end", 0.03)):
            drafted = self.draft["stations"][name]["at"]
            authored = self.pilot["stations"][name]["at"]
            d = math.dist(drafted, authored)
            self.assertLessEqual(
                d, max_dist,
                f"{name}: draft {drafted} vs pilot {authored} ({d:.4f} in apart)")

    def test_tolerance_derived_from_calibration(self):
        self.assertAlmostEqual(self.draft["tolerance"]["distance"], 0.1, places=4)

    def test_bowtell_refits_to_the_recorded_circle(self):
        fit = self.report["arc_fits"]["bowtell"]
        self.assertAlmostEqual(fit["centre"][0], (1190.5 - 1004) / 22.09, places=3)
        self.assertAlmostEqual(fit["centre"][1], (772 - 721.5) / 22.09, places=3)
        self.assertAlmostEqual(fit["radius"], 50.6 / 22.09, places=3)
        self.assertTrue(fit["within_tolerance"])
        self.assertEqual(fit["suggested_evidence"], "FITTED")

    def test_between_extents_become_distance_claims(self):
        params = self.draft["parameters"]
        wall = next(p for n, p in params.items() if n.startswith("wl1"))
        self.assertEqual(wall["claim"]["kind"], "distance")
        self.assertEqual(wall["claim"]["between"], ["wall_start", "wall_end"])
        self.assertEqual(self.draft["datum"], self.report["datum"])
        self.assertTrue(self.draft["datum"].startswith("sw1"))

    def test_draft_honestly_fails_structural_validation(self):
        problems = validate_structure(copy.deepcopy(self.draft))
        self.assertTrue(any("segments" in p for p in problems),
                        "the draft must not compile until segments are authored")

    def test_open_questions_carried_into_todo(self):
        self.assertTrue(any("upper-face" in t for t in self.report["todo"]))

    def test_invalid_handoff_raises_spec_error(self):
        h = example()
        del h["calibration"]
        with self.assertRaises(SpecError):
            build_draft(h)


if __name__ == "__main__":
    unittest.main()
