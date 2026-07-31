"""Pure-Python tests for the SINC_GeometryProof_Profile compiler.

No Blender required. The fixture is a 5x3 block whose top-right corner is cut
by a one-unit quarter arc - small enough to reason about by hand, rich enough
to exercise arcs, joins, winding, roles, and every rejection class.
"""

import copy
import math
import sys
import unittest
from pathlib import Path

sys.dont_write_bytecode = True  # keep __pycache__ out of the asset package

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "assets" / "creative" / "geometry_proof"))

from profile_compiler import compile_profile, moulded_chain_points, to_world  # noqa: E402
from profile_spec import SpecError, validate_structure  # noqa: E402


def fixture():
    """Closed outline: (0,0) -> (4,0) -arc-> (5,1) -> (5,3) -> (0,3) -> close.

    The arc is centred at (4,1), radius 1, COUNTERCLOCKWISE: a convex bulge
    through (4.707, 0.293). Traversal is counterclockwise, interior LEFT.
    """
    return {
        "format": "SINC_GeometryProof_Profile/1",
        "id": "fixture_block_v1",
        "reference": {"source": "synthetic test fixture", "scale_px_per_unit": 10.0},
        "units": "in",
        "datum": "block_width",
        "tolerance": {"distance": 0.05, "angle_deg": 2.0},
        "construction_plane": {
            "plane": "XZ",
            "profile_x": "+X",
            "profile_y": "+Z",
            "extrude_axis": "+Y",
            "scale_m_per_unit": 0.0254,
        },
        "stations": {
            "origin":   {"at": [0.0, 0.0], "source_px": [100, 400], "evidence": "MEASURED", "join": "HARD"},
            "wall_end": {"at": [4.0, 0.0], "source_px": [140, 400], "evidence": "MEASURED", "join": "HARD"},
            "arc_end":  {"at": [5.0, 1.0], "source_px": [150, 390], "evidence": "DERIVED", "join": "HARD"},
            "top_right": {"at": [5.0, 3.0], "evidence": "MEASURED", "join": "HARD"},
            "top_left": {"at": [0.0, 3.0], "evidence": "ESTIMATED", "join": "HARD"},
        },
        "segments": [
            {"name": "wall", "kind": "LINE", "from": "origin", "to": "wall_end",
             "role": "BOUNDING", "evidence": "MEASURED"},
            {"name": "corner_arc", "kind": "ARC", "from": "wall_end", "to": "arc_end",
             "centre": [4.0, 1.0], "direction": "COUNTERCLOCKWISE",
             "role": "MOULDED", "evidence": "FITTED"},
            {"name": "right", "kind": "LINE", "from": "arc_end", "to": "top_right",
             "role": "BOUNDING", "evidence": "MEASURED"},
            {"name": "top", "kind": "LINE", "from": "top_right", "to": "top_left",
             "role": "BOUNDING", "evidence": "MEASURED"},
            {"name": "left", "kind": "LINE", "from": "top_left", "to": "origin",
             "role": "BOUNDING", "evidence": "MEASURED"},
        ],
        "solid_side": "LEFT",
        "parameters": {
            "block_width": {"value": 5.0, "claim": {"kind": "extent", "axis": "width"}},
            "corner_radius": {"value": 1.0, "claim": {"kind": "radius_of", "segment": "corner_arc"}},
            "wall_length": {"value": 4.0, "claim": {"kind": "length_of", "segment": "wall"}},
            "wall_angle": {"value": 0.0, "claim": {"kind": "angle_of", "segment": "wall"}},
            "diagonal": {"value": math.hypot(5.0, 3.0),
                         "claim": {"kind": "distance", "between": ["origin", "top_right"]}},
        },
        "proof": {"extrusion_depth": 1.0},
        "notes": ["synthetic fixture for compiler tests"],
    }


def arc_points(compiled):
    return next(s for s in compiled.segments if s["name"] == "corner_arc")["points"]


class StructureTests(unittest.TestCase):
    def test_fixture_is_structurally_valid(self):
        self.assertEqual(validate_structure(fixture()), [])

    def test_unknown_key_rejected(self):
        spec = fixture()
        spec["segments"][1]["direciton"] = "CLOCKWISE"
        problems = validate_structure(spec)
        self.assertTrue(any("direciton" in p for p in problems))

    def test_broken_loop_rejected(self):
        spec = fixture()
        spec["segments"][2]["from"] = "wall_end"
        problems = validate_structure(spec)
        self.assertTrue(any("closed loop" in p or "visited twice" in p for p in problems))

    def test_unused_station_rejected(self):
        spec = fixture()
        spec["stations"]["orphan"] = {"at": [9.0, 9.0], "evidence": "AUTHORED", "join": "HARD"}
        problems = validate_structure(spec)
        self.assertTrue(any("orphan" in p for p in problems))


class CompileTests(unittest.TestCase):
    def test_fixture_compiles(self):
        compiled = compile_profile(fixture())
        self.assertEqual(compiled.winding, "COUNTERCLOCKWISE")
        self.assertGreater(compiled.signed_area, 0.0)
        self.assertAlmostEqual(compiled.extents["width"], 5.0)
        self.assertAlmostEqual(compiled.extents["depth"], 3.0)

    def test_deterministic(self):
        self.assertEqual(compile_profile(fixture()).outline,
                         compile_profile(fixture()).outline)

    def test_hard_corners_are_exact_vertices(self):
        compiled = compile_profile(fixture())
        for name in ("origin", "wall_end", "arc_end", "top_right", "top_left"):
            at = tuple(fixture()["stations"][name]["at"])
            self.assertIn(at, compiled.outline, f"station {name} must be an exact outline vertex")

    def test_arc_geometry(self):
        compiled = compile_profile(fixture())
        seg = next(s for s in compiled.segments if s["name"] == "corner_arc")
        self.assertAlmostEqual(seg["radius"], 1.0)
        self.assertAlmostEqual(seg["sweep_deg"], 90.0, places=6)
        mx, my = seg["mid_point"]
        self.assertAlmostEqual(mx, 4.0 + math.cos(math.radians(-45.0)), places=9)
        self.assertAlmostEqual(my, 1.0 + math.sin(math.radians(-45.0)), places=9)
        for px, py in seg["points"]:
            self.assertAlmostEqual(math.hypot(px - 4.0, py - 1.0), 1.0, places=9)

    def test_direction_flip_changes_geometry(self):
        """The Phase 4 flip gate, at compiler level: CW and CCW must differ."""
        ccw = compile_profile(fixture())
        spec = fixture()
        spec["segments"][1]["direction"] = "CLOCKWISE"
        cw = compile_profile(spec)
        ccw_mid = next(s for s in ccw.segments if s["name"] == "corner_arc")["mid_point"]
        cw_mid = next(s for s in cw.segments if s["name"] == "corner_arc")["mid_point"]
        # Convex bulge vs concave scoop: midpoints mirror about the chord.
        self.assertAlmostEqual(ccw_mid[0], 4.707, places=3)
        self.assertAlmostEqual(ccw_mid[1], 0.293, places=3)
        self.assertAlmostEqual(cw_mid[0], 3.293, places=3)
        self.assertAlmostEqual(cw_mid[1], 1.707, places=3)
        cw_seg = next(s for s in cw.segments if s["name"] == "corner_arc")
        self.assertAlmostEqual(cw_seg["sweep_deg"], 270.0, places=6)
        self.assertNotEqual(ccw.outline, cw.outline)

    def test_moulded_chain_is_only_the_arc(self):
        compiled = compile_profile(fixture())
        self.assertEqual(len(compiled.moulded_chains), 1)
        self.assertEqual(compiled.moulded_chains[0], arc_points(compiled))
        self.assertEqual(moulded_chain_points(fixture()), compiled.moulded_chains)

    def test_hard_but_continuous_warns(self):
        # wall (tangent +x) meets the CCW arc (entry tangent +x): continuous,
        # yet declared HARD - that must warn, not fail.
        compiled = compile_profile(fixture())
        self.assertTrue(any("wall_end" in w and "HARD" in w for w in compiled.warnings))

    def test_normalized_stations_divide_by_datum(self):
        compiled = compile_profile(fixture())
        self.assertAlmostEqual(compiled.normalized_stations["top_right"][0], 1.0)
        self.assertAlmostEqual(compiled.normalized_stations["top_right"][1], 0.6)

    def test_to_world_maps_into_xz_metres(self):
        spec = fixture()
        self.assertEqual(to_world((1.0, 2.0), spec), (0.0254, 0.0, 0.0508))

    def test_bezier_segment_samples(self):
        spec = fixture()
        spec["segments"][3] = {
            "name": "top", "kind": "BEZIER", "from": "top_right", "to": "top_left",
            "handle_from": [3.0, 3.4], "handle_to": [2.0, 3.4],
            "role": "MOULDED", "evidence": "AUTHORED"}
        compiled = compile_profile(spec)
        seg = next(s for s in compiled.segments if s["name"] == "top")
        self.assertEqual(seg["points"][0], (5.0, 3.0))
        self.assertEqual(seg["points"][-1], (0.0, 3.0))
        self.assertGreaterEqual(len(seg["points"]), 9)


class RejectionTests(unittest.TestCase):
    def assert_rejects(self, spec, needle):
        with self.assertRaises(SpecError) as ctx:
            compile_profile(spec)
        self.assertTrue(any(needle in p for p in ctx.exception.problems),
                        f"expected a problem mentioning '{needle}', got: {ctx.exception.problems}")

    def test_bad_arc_centre_rejected(self):
        spec = fixture()
        spec["segments"][1]["centre"] = [4.0, 0.5]
        self.assert_rejects(spec, "equidistant")

    def test_fake_smooth_join_rejected(self):
        # CW arc leaves wall_end with tangent -x against the wall's +x: a
        # declared SMOOTH there is a false claim.
        spec = fixture()
        spec["segments"][1]["direction"] = "CLOCKWISE"
        spec["stations"]["wall_end"]["join"] = "SMOOTH"
        self.assert_rejects(spec, "SMOOTH")

    def test_true_smooth_join_accepted(self):
        spec = fixture()
        spec["stations"]["wall_end"]["join"] = "SMOOTH"
        compiled = compile_profile(spec)
        self.assertEqual(compiled.stations["wall_end"]["join"], "SMOOTH")

    def test_solid_side_contradiction_rejected(self):
        spec = fixture()
        spec["solid_side"] = "RIGHT"
        self.assert_rejects(spec, "solid_side")

    def test_self_intersection_rejected(self):
        spec = fixture()
        # Swap two stations to fold the rectangle into a bowtie.
        spec["stations"]["top_right"]["at"] = [0.0, 3.0]
        spec["stations"]["top_left"]["at"] = [5.0, 3.0]
        spec["stations"]["top_left"]["evidence"] = "MEASURED"
        with self.assertRaises(SpecError) as ctx:
            compile_profile(spec)
        joined = "\n".join(ctx.exception.problems)
        self.assertTrue("self-intersect" in joined or "solid_side" in joined)

    def test_claim_mismatch_rejected(self):
        spec = fixture()
        spec["parameters"]["corner_radius"]["value"] = 2.0
        self.assert_rejects(spec, "corner_radius")

    def test_source_px_registration_rejected(self):
        spec = fixture()
        spec["stations"]["arc_end"]["source_px"] = [170, 390]  # 20 px = 2 in off
        self.assert_rejects(spec, "arc_end")

    def test_degenerate_segment_rejected(self):
        spec = fixture()
        spec["stations"]["top_left"]["at"] = [0.001, 0.001]
        with self.assertRaises(SpecError):
            compile_profile(spec)

    def test_every_claim_kind_rejects_on_mismatch(self):
        for pname, bad in (("wall_length", 5.5), ("wall_angle", 30.0),
                           ("diagonal", 10.0), ("block_width", 6.0)):
            with self.subTest(claim=pname):
                spec = fixture()
                spec["parameters"][pname]["value"] = bad
                self.assert_rejects(spec, pname)

    def test_null_join_rejected_structurally(self):
        spec = fixture()
        spec["stations"]["wall_end"]["join"] = None
        problems = validate_structure(spec)
        self.assertTrue(any("wall_end" in p and "join" in p for p in problems))

    def test_sliver_arc_rejected(self):
        # two stations a hair apart on the circle, direction taking the tiny
        # way around: the bead would silently vanish without the sweep gate
        spec = fixture()
        spec["segments"][1]["centre"] = [4.0, 1.0]
        spec["stations"]["arc_end"]["at"] = [4.0000001, 0.0]
        self.assert_rejects(spec, "sliver")

    def test_colinear_doubling_back_rejected(self):
        spec = fixture()
        spec["stations"] = {
            "a": {"at": [0.0, 0.0], "evidence": "AUTHORED", "join": "HARD"},
            "b": {"at": [5.0, 0.0], "evidence": "AUTHORED", "join": "HARD"},
            "c": {"at": [1.0, 0.0], "evidence": "AUTHORED", "join": "HARD"},
            "d": {"at": [1.0, 3.0], "evidence": "AUTHORED", "join": "HARD"},
            "e": {"at": [0.0, 3.0], "evidence": "AUTHORED", "join": "HARD"},
        }
        spec["segments"] = [
            {"name": n, "kind": "LINE", "from": f, "to": t,
             "role": "BOUNDING", "evidence": "AUTHORED"}
            for n, f, t in (("s1", "a", "b"), ("s2", "b", "c"), ("s3", "c", "d"),
                            ("s4", "d", "e"), ("s5", "e", "a"))]
        spec["parameters"] = {"block_width": {"value": 5.0,
                                              "claim": {"kind": "extent", "axis": "width"}}}
        self.assert_rejects(spec, "doubles back")

    def test_improper_touch_rejected(self):
        # station d sits on the interior of edge a->b: pinched outline
        spec = fixture()
        spec["stations"] = {
            "a": {"at": [0.0, 0.0], "evidence": "AUTHORED", "join": "HARD"},
            "b": {"at": [4.0, 0.0], "evidence": "AUTHORED", "join": "HARD"},
            "c": {"at": [4.0, 2.0], "evidence": "AUTHORED", "join": "HARD"},
            "d": {"at": [2.0, 0.0], "evidence": "AUTHORED", "join": "HARD"},
            "e": {"at": [0.0, 2.0], "evidence": "AUTHORED", "join": "HARD"},
        }
        spec["segments"] = [
            {"name": n, "kind": "LINE", "from": f, "to": t,
             "role": "BOUNDING", "evidence": "AUTHORED"}
            for n, f, t in (("s1", "a", "b"), ("s2", "b", "c"), ("s3", "c", "d"),
                            ("s4", "d", "e"), ("s5", "e", "a"))]
        spec["parameters"] = {"block_width": {"value": 4.0,
                                              "claim": {"kind": "extent", "axis": "width"}}}
        self.assert_rejects(spec, "self-intersects")

    def test_bezier_smooth_join_is_verified(self):
        def with_bezier_handle(handle_from):
            spec = fixture()
            spec["segments"][3] = {
                "name": "top", "kind": "BEZIER", "from": "top_right", "to": "top_left",
                "handle_from": handle_from, "handle_to": [1.0, 3.4],
                "role": "MOULDED", "evidence": "AUTHORED"}
            spec["stations"]["top_right"]["join"] = "SMOOTH"
            return spec
        # entry tangent straight up continues the vertical fillet: SMOOTH holds
        compiled = compile_profile(with_bezier_handle([5.0, 3.5]))
        self.assertEqual(compiled.stations["top_right"]["join"], "SMOOTH")
        # entry tangent straight down reverses it: the SMOOTH claim must fail
        with self.assertRaises(SpecError) as ctx:
            compile_profile(with_bezier_handle([5.0, 2.5]))
        self.assertTrue(any("SMOOTH" in p for p in ctx.exception.problems))


class MouldedChainTests(unittest.TestCase):
    def test_wrap_through_loop_closure_merges_runs(self):
        spec = fixture()
        spec["segments"][0]["role"] = "MOULDED"   # wall
        spec["segments"][4]["role"] = "MOULDED"   # left (last segment)
        compiled = compile_profile(spec)
        self.assertEqual(len(compiled.moulded_chains), 1)
        chain = compiled.moulded_chains[0]
        self.assertEqual(chain[0], (0.0, 3.0), "chain must start where 'left' starts")
        self.assertEqual(chain[-1], (5.0, 1.0), "chain must end where the arc ends")
        for a, b in zip(chain, chain[1:]):
            self.assertNotEqual(a, b, "no zero-length edges from bad junction dedup")

    def test_all_moulded_profile_returns_outline_convention(self):
        spec = fixture()
        for seg in spec["segments"]:
            seg["role"] = "MOULDED"
        compiled = compile_profile(spec)
        self.assertEqual(len(compiled.moulded_chains), 1)
        self.assertEqual(compiled.moulded_chains[0], compiled.outline,
                         "fully moulded chain IS the outline, first point not repeated")
        self.assertNotEqual(compiled.moulded_chains[0][0], compiled.moulded_chains[0][-1])


class ProfilesDirectoryTests(unittest.TestCase):
    """Every committed profile spec must compile - the vocabulary gate."""

    PROFILES = ROOT / "assets" / "creative" / "geometry_proof" / "profiles"

    def load(self, name):
        import json
        return json.loads((self.PROFILES / name).read_text())

    def test_every_profile_compiles(self):
        for path in sorted(self.PROFILES.glob("*.json")):
            if path.name == "TEMPLATE.json":
                continue
            with self.subTest(profile=path.name):
                compiled = compile_profile(self.load(path.name))
                self.assertGreater(len(compiled.outline), 3)

    def test_ovolo_has_validated_smooth_joins(self):
        compiled = compile_profile(self.load("ovolo_v1.json"))
        smooth = [n for n, st in compiled.stations.items() if st["join"] == "SMOOTH"]
        self.assertEqual(sorted(smooth), ["ovolo_close", "ovolo_spring"])

    def test_ogee_carries_both_arc_directions(self):
        compiled = compile_profile(self.load("ogee_brandon_pl9_v1.json"))
        directions = {s["direction"] for s in compiled.segments if s["kind"] == "ARC"}
        self.assertEqual(directions, {"CLOCKWISE", "COUNTERCLOCKWISE"})
        radii = sorted(round(s["radius"], 2) for s in compiled.segments if s["kind"] == "ARC")
        self.assertEqual(radii, [0.9, 1.2])
        self.assertEqual(compiled.stations["inflection"]["join"], "SMOOTH")

    def test_stepped_is_straight_only_and_right_solid(self):
        compiled = compile_profile(self.load("stepped_paley_pl1_fig7_v1.json"))
        self.assertEqual([s for s in compiled.segments if s["kind"] == "ARC"], [])
        self.assertEqual(compiled.winding, "CLOCKWISE")
        self.assertTrue(all(st["join"] == "HARD" for st in compiled.stations.values()))


class Fig6MigrationTests(unittest.TestCase):
    """The Phase 4 migration gate: the pilot spec must reproduce the corrected
    cavetto where the old workflow was right, and its divergences must be
    exactly the documented old-workflow defects."""

    OLD_MANIFESTS = ROOT / "docs" / "blender" / "reference_manifests"
    PILOT = ROOT / "assets" / "creative" / "geometry_proof" / "profiles" / "paley_pl1_fig6_v1.json"

    @classmethod
    def setUpClass(cls):
        import json
        sys.path.insert(0, str(cls.OLD_MANIFESTS))
        import moulding_profile
        old_profiles = json.loads(
            (cls.OLD_MANIFESTS / "moulding_profiles_v1.json").read_text())
        fig6 = next(p for p in old_profiles if p["id"] == "paley_pl1_fig6")
        cls.old_pts = moulding_profile.build(fig6, seg=24)
        cls.spec = json.loads(cls.PILOT.read_text())
        cls.compiled = compile_profile(cls.spec)

    @staticmethod
    def dist_to_polyline(p, pts, closed=False):
        def seg_dist(p, a, b):
            ax, ay = a
            vx, vy = b[0] - ax, b[1] - ay
            L2 = vx * vx + vy * vy
            t = 0.0 if L2 == 0 else max(0.0, min(1.0, ((p[0] - ax) * vx + (p[1] - ay) * vy) / L2))
            return math.hypot(p[0] - (ax + t * vx), p[1] - (ay + t * vy))
        n = len(pts)
        pairs = range(n if closed else n - 1)
        return min(seg_dist(p, pts[i], pts[(i + 1) % n]) for i in pairs)

    def test_shape_parity_where_old_workflow_was_right(self):
        # Everything except the old chain's final point (its displaced upper
        # chamfer end) must lie within measurement tolerance of the new outline.
        for i, p in enumerate(self.old_pts[:-1]):
            d = self.dist_to_polyline(p, self.compiled.outline, closed=True)
            self.assertLessEqual(
                d, 0.1, f"old point {i} {p} is {d:.3f} in from the new outline")

    def test_old_endpoint_defect_is_corrected(self):
        # Old defect: the heading-chained hollow stopped short, displacing the
        # upper chamfer; the old final point pokes through the soffit plane.
        old_final = self.old_pts[-1]
        stock_right = max(p[0] for p in self.compiled.outline)
        self.assertGreater(old_final[0], stock_right + 0.15,
                           "expected the old endpoint to overshoot the soffit plane")
        d = self.dist_to_polyline(old_final, self.compiled.outline, closed=True)
        self.assertGreater(d, 0.15, "old endpoint divergence is the documented defect")
        # The new chamfer top sits exactly on the soffit plane instead.
        top = self.compiled.stations["chamfer_top"]["at"]
        self.assertAlmostEqual(top[0], stock_right, places=6)

    def test_pilot_validates_with_no_errors(self):
        self.assertEqual(self.compiled.winding, "COUNTERCLOCKWISE")
        self.assertEqual(len(self.compiled.moulded_chains), 1,
                         "the moulded edge is one continuous run")
        bowtell = next(s for s in self.compiled.segments if s["name"] == "bowtell")
        self.assertAlmostEqual(bowtell["sweep_deg"], 267.2, delta=0.3)
        hollow = next(s for s in self.compiled.segments if s["name"] == "bead_hollow")
        self.assertAlmostEqual(hollow["sweep_deg"], 190.8, delta=0.3)

    def test_hollow_end_stays_within_tolerance_of_measurement(self):
        # The consistency repair moved hollow_end 0.021 in off the measured
        # pixel point - it must stay well inside measurement tolerance.
        at = self.compiled.stations["hollow_end"]["at"]
        measured = ((1175 - 1004) / 22.09, (772 - 649) / 22.09)
        self.assertLessEqual(math.dist(at, measured), 0.05)


if __name__ == "__main__":
    unittest.main()
