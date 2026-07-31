"""Tests for the segment proposer.

The synthetic fixture is a 4 x 3 block whose top-right corner carries a
r=1 quarter arc, expressed as a HANDOFF (plate pixels + a trace) - so the
tests exercise the whole chain: frames -> stations -> trace fit -> segment
proposal -> direction detection -> smooth upgrade -> solid side -> compile.
The same stations with the trace on the other side of the chord must yield
the OPPOSITE arc direction: direction is chosen by measurement evidence.
"""

import json
import math
import sys
import unittest
from pathlib import Path

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "assets" / "creative" / "geometry_proof"
sys.path.insert(0, str(PACKAGE))

from profile_compiler import compile_profile  # noqa: E402
from profile_proposer import propose  # noqa: E402

# profile frame: origin at plate px (100, 400), 10 px/unit, y flipped.
PX = {"origin": [100, 400], "bed_end": [140, 400], "ovolo_spring": [140, 380],
      "ovolo_close": [130, 370], "top_left": [100, 370]}
CENTRE_PX = (130.0, 380.0)  # profile (3, 2), r = 1 (10 px)


def trace_points(angles_deg):
    return [[round(CENTRE_PX[0] + 10 * math.cos(math.radians(t)), 3),
             round(CENTRE_PX[1] - 10 * math.sin(math.radians(t)), 3)]
            for t in angles_deg]


def handoff(order=("origin", "bed_end", "ovolo_spring", "ovolo_close", "top_left"),
            angles=(15, 30, 45, 60, 75)):
    return {
        "format": "SINC_GeometryProof_Handoff/1",
        "id": "proposer_fixture",
        "source": {"citation": "synthetic proposer fixture"},
        "calibration": {"px_per_unit": 10.0, "units": "relative",
                        "rms_residual_px": 0.05, "method": "synthetic"},
        "frames": {"plate": {"parent": None}},
        "profile_frame": {"origin_landmark": "origin", "axes": "plate-default"},
        "landmarks": [
            {"name": n, "frame": "plate", "px": list(PX[n]), "evidence": "MEASURED"}
            for n in order],
        "arc_traces": [
            {"name": "corner_round", "frame": "plate", "evidence": "MEASURED",
             "points": trace_points(angles)}],
        "extents": [
            {"measure_id": "W1", "quantity": "block width", "value": 4.0,
             "status": "MEASURED", "between": ["origin", "bed_end"]}],
        "datum_measure_id": "W1",
    }


class ProposerTests(unittest.TestCase):
    def test_ccw_trace_proposes_ccw_arc_and_compiles(self):
        spec, report = propose(handoff())
        self.assertTrue(report["compile_ok"], report["problems"])
        arc = next(s for s in spec["segments"] if s["kind"] == "ARC")
        self.assertEqual(arc["from"], "ovolo_spring")
        self.assertEqual(arc["to"], "ovolo_close")
        self.assertEqual(arc["direction"], "COUNTERCLOCKWISE")
        self.assertAlmostEqual(arc["centre"][0], 3.0, places=3)
        self.assertAlmostEqual(arc["centre"][1], 2.0, places=3)
        self.assertEqual(arc["evidence"], "FITTED")
        self.assertEqual(spec["solid_side"], "LEFT")
        self.assertFalse(report["solid_side_flipped"])
        # tangent joins proven and upgraded
        self.assertIn("ovolo_spring", report["smooth_upgraded"])
        self.assertIn("ovolo_close", report["smooth_upgraded"])
        self.assertEqual(spec["stations"]["ovolo_spring"]["join"], "SMOOTH")
        self.assertNotIn("origin", report["smooth_upgraded"])
        # the candidate genuinely compiles
        compiled = compile_profile(spec)
        self.assertEqual(compiled.winding, "COUNTERCLOCKWISE")

    def test_cw_trace_flips_the_proposed_direction(self):
        # same stations, trace sampled on the OTHER side of the chord: the
        # 270-degree way around. Direction must follow the evidence.
        spec, report = propose(handoff(angles=(-30, -90, -150, 150, 120)))
        self.assertTrue(report["compile_ok"], report["problems"])
        arc = next(s for s in spec["segments"] if s["kind"] == "ARC")
        self.assertEqual(arc["direction"], "CLOCKWISE")
        compiled = compile_profile(spec)
        seg = next(s for s in compiled.segments if s["kind"] == "ARC")
        self.assertAlmostEqual(seg["sweep_deg"], 270.0, places=1)
        # concave scoop: joins at the arc are now genuinely hard
        self.assertNotIn("ovolo_spring", report["smooth_upgraded"])

    def test_reversed_traversal_flips_solid_side_and_direction(self):
        spec, report = propose(handoff(
            order=("top_left", "ovolo_close", "ovolo_spring", "bed_end", "origin")))
        self.assertTrue(report["compile_ok"], report["problems"])
        self.assertEqual(spec["solid_side"], "RIGHT")
        self.assertTrue(report["solid_side_flipped"])
        arc = next(s for s in spec["segments"] if s["kind"] == "ARC")
        # same physical quarter round traversed backward is CLOCKWISE
        self.assertEqual(arc["from"], "ovolo_close")
        self.assertEqual(arc["direction"], "CLOCKWISE")
        self.assertIn("ovolo_close", report["smooth_upgraded"])

    def test_unmatched_trace_is_reported_loudly(self):
        example = json.loads(
            (PACKAGE / "handoffs" / "EXAMPLE_paley_pl1_fig6_handoff.json").read_text())
        spec, report = propose(example)
        # the fig 6 example's landmarks exclude the bowtell's endpoints, so
        # the trace must be flagged as owed landmarks - and the remaining
        # quadrilateral still compiles
        self.assertIn("bowtell", report["unmatched_traces"])
        self.assertTrue(any("bowtell" in r and "tangency" in r for r in report["review"]))
        self.assertTrue(report["compile_ok"], report["problems"])
        self.assertTrue(all(s["kind"] == "LINE" for s in spec["segments"]))

    def test_line_evidence_inherits_the_weaker_endpoint(self):
        h = handoff()
        h["landmarks"][0]["evidence"] = "ESTIMATED"  # origin
        spec, report = propose(h)
        first = spec["segments"][0]
        self.assertEqual((first["from"], first["to"]), ("origin", "bed_end"))
        self.assertEqual(first["evidence"], "ESTIMATED")

    def test_authored_closure_pair_proposed_as_bounding(self):
        h = handoff()
        for lm in h["landmarks"]:
            if lm["name"] in ("top_left", "origin"):
                lm["evidence"] = "AUTHORED"
        spec, report = propose(h)
        closing = next(s for s in spec["segments"]
                       if (s["from"], s["to"]) == ("top_left", "origin"))
        self.assertEqual(closing["role"], "BOUNDING")


if __name__ == "__main__":
    unittest.main()
