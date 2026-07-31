"""Segment proposer: measurement handoff -> compiler-verified CANDIDATE spec.

The automatable half of the authoring judgment, made safe by the referee:
every proposal must survive compile_profile, and the visual gate still
decides acceptance. The proposer NEVER invents geometry:

  - stations are connected in the handoff's declared traversal order;
  - an ARC is proposed only where a traced-and-fitted circle passes through
    BOTH endpoints of a consecutive pair within tolerance - never guessed;
  - arc direction is chosen by where the trace's own sample points lie
    around the fitted centre (measurement evidence, not preference);
  - joins upgrade to SMOOTH only where the compiled tangents already agree
    within tolerance.angle_deg - the claim is proven before it is made;
  - solid_side is resolved by compile-and-flip: the compiler's winding
    check is the authority;
  - roles and segment evidence are heuristics, LOUDLY flagged for review;
  - traces that match no station pair are reported, not dropped - they mean
    the research thread owes landmarks at the trace's tangency ends.

Output: a candidate spec that compiles (or the honest list of why not),
plus a proposal report. Human review still owns: roles, evidence classes,
datum choice, extrusion depth, and final visual acceptance.

CLI:
  python3 profile_proposer.py HANDOFF.json --out CANDIDATE.json
"""

import copy
import json
import math
import sys
from pathlib import Path

sys.dont_write_bytecode = True

if __package__ is None and str(Path(__file__).resolve().parent) not in sys.path:
    sys.path.insert(0, str(Path(__file__).resolve().parent))

from profile_compiler import compile_profile  # noqa: E402
from profile_intake import build_draft, resolved_arc_traces  # noqa: E402
from profile_spec import SpecError  # noqa: E402

TWO_PI = 2.0 * math.pi
EVIDENCE_STRENGTH = {"PRINTED": 5, "MEASURED": 4, "FITTED": 3,
                     "DERIVED": 2, "ESTIMATED": 1, "AUTHORED": 0}


def _weaker(ev_a, ev_b):
    return ev_a if EVIDENCE_STRENGTH[ev_a] <= EVIDENCE_STRENGTH[ev_b] else ev_b


def _choose_direction(a, b, centre, trace_points):
    """CLOCKWISE/COUNTERCLOCKWISE chosen by where the trace samples lie."""
    a0 = math.atan2(a[1] - centre[1], a[0] - centre[0])
    a1 = math.atan2(b[1] - centre[1], b[0] - centre[0])
    ccw_span = (a1 - a0) % TWO_PI
    cw_span = (a0 - a1) % TWO_PI
    eps = 1e-9
    in_ccw = in_cw = 0
    for p in trace_points:
        t = math.atan2(p[1] - centre[1], p[0] - centre[0])
        if ((t - a0) % TWO_PI) <= ccw_span + eps:
            in_ccw += 1
        if ((a0 - t) % TWO_PI) <= cw_span + eps:
            in_cw += 1
    if in_ccw == in_cw:
        return "COUNTERCLOCKWISE", f"AMBIGUOUS: {in_ccw} trace points on each side - REVIEW"
    direction = "COUNTERCLOCKWISE" if in_ccw > in_cw else "CLOCKWISE"
    return direction, f"{max(in_ccw, in_cw)}/{len(trace_points)} trace samples on the chosen side"


def _try_compile(spec):
    try:
        return compile_profile(spec), []
    except SpecError as err:
        return None, list(err.problems)


def propose(handoff):
    """Handoff dict -> (candidate_spec, report). Raises SpecError on a bad handoff."""
    draft, intake_report = build_draft(handoff)
    tol_d = draft["tolerance"]["distance"]
    tol_a = draft["tolerance"]["angle_deg"]
    names = list(draft["stations"])
    traces = resolved_arc_traces(handoff)
    report = {"handoff_id": handoff["id"], "segments": [], "unmatched_traces": [],
              "solid_side_flipped": False, "smooth_upgraded": [],
              "compile_ok": False, "problems": [], "review": list(intake_report["todo"])}

    # --- match each consecutive pair against the fitted trace circles
    pair_arc = {}
    used_traces = set()
    for i, a in enumerate(names):
        b = names[(i + 1) % len(names)]
        pa = draft["stations"][a]["at"]
        pb = draft["stations"][b]["at"]
        best = None
        for tr in traces:
            c, r = tr["fit"]["centre"], tr["fit"]["radius"]
            if (abs(math.dist(pa, c) - r) <= tol_d
                    and abs(math.dist(pb, c) - r) <= tol_d):
                if best is None or tr["fit"]["rms_residual"] < best["fit"]["rms_residual"]:
                    best = tr
        if best is not None:
            pair_arc[(a, b)] = best
            used_traces.add(best["name"])
    for tr in traces:
        if tr["name"] not in used_traces:
            report["unmatched_traces"].append(tr["name"])
            report["review"].append(
                f"arc trace '{tr['name']}' (centre {tuple(round(v, 3) for v in tr['fit']['centre'])}, "
                f"r {tr['fit']['radius']:.3f}) matches no consecutive station pair - "
                "the handoff owes landmarks at the arc's tangency ends")

    # --- build proposed segments in traversal order
    spec = copy.deepcopy(draft)
    segments = []
    for i, a in enumerate(names):
        b = names[(i + 1) % len(names)]
        ev_a = spec["stations"][a]["evidence"]
        ev_b = spec["stations"][b]["evidence"]
        role = "BOUNDING" if ev_a == "AUTHORED" and ev_b == "AUTHORED" else "MOULDED"
        seg = {"name": f"{a}_to_{b}"[:48], "from": a, "to": b, "role": role}
        if (a, b) in pair_arc:
            tr = pair_arc[(a, b)]
            fit = tr["fit"]
            direction, basis = _choose_direction(
                spec["stations"][a]["at"], spec["stations"][b]["at"],
                fit["centre"], tr["points"])
            seg.update({
                "kind": "ARC",
                "centre": [round(fit["centre"][0], 4), round(fit["centre"][1], 4)],
                "direction": direction,
                "evidence": "FITTED" if fit["rms_residual"] <= tol_d else "ESTIMATED",
                "note": f"PROPOSED from trace '{tr['name']}' (r {fit['radius']:.4f}, "
                        f"rms {fit['rms_residual']:.5f}); direction: {basis}. Confirm role.",
            })
        else:
            seg.update({
                "kind": "LINE",
                "evidence": _weaker(ev_a, ev_b),
                "note": "PROPOSED straight connection; evidence inherited from the "
                        "weaker endpoint. Confirm role.",
            })
        segments.append(seg)
        report["segments"].append({k: seg.get(k) for k in ("name", "kind", "direction", "role")})
    spec["segments"] = segments
    spec["solid_side"] = "LEFT"

    # --- compile; let the winding check resolve solid_side
    compiled, problems = _try_compile(spec)
    if compiled is None and any("solid_side" in p for p in problems):
        spec["solid_side"] = "RIGHT"
        report["solid_side_flipped"] = True
        compiled, problems = _try_compile(spec)

    # --- upgrade joins the compiled tangents already prove SMOOTH
    if compiled is not None:
        for name, st in compiled.stations.items():
            if st["join_deviation_deg"] <= tol_a:
                spec["stations"][name]["join"] = "SMOOTH"
                spec["stations"][name]["note"] = (
                    f"PROPOSED SMOOTH: tangents agree within "
                    f"{st['join_deviation_deg']:.3f} deg")
                report["smooth_upgraded"].append(name)
            else:
                spec["stations"][name]["note"] = (
                    f"PROPOSED HARD: tangent deviation "
                    f"{st['join_deviation_deg']:.1f} deg")
        compiled, problems = _try_compile(spec)

    report["compile_ok"] = compiled is not None
    report["problems"] = problems

    status = ("compiles clean - review roles/evidence, then build the proof scene"
              if compiled is not None else
              "DOES NOT COMPILE - the listed problems are the worklist")
    spec["notes"] = ([f"CANDIDATE proposed by profile_proposer from handoff "
                      f"'{handoff['id']}': {status}. Every segment, direction, join "
                      "and solid_side above is a compiler-verified PROPOSAL; roles, "
                      "evidence classes, datum and extrusion remain review items. "
                      "Final acceptance is the visual gate."]
                     + [f"REVIEW: {r}" for r in report["review"]])
    return spec, report


def main():
    args = sys.argv[1:]
    if len(args) < 1 or "--out" not in args:
        raise SystemExit("usage: python3 profile_proposer.py HANDOFF.json --out CANDIDATE.json")
    handoff = json.loads(Path(args[0]).read_text())
    out_path = Path(args[args.index("--out") + 1])
    spec, report = propose(handoff)
    out_path.write_text(json.dumps(spec, indent=1))
    print(json.dumps(report, indent=1))
    print(f"CANDIDATE written to {out_path}"
          + ("" if report["compile_ok"] else "  (does not compile yet - see problems)"))


if __name__ == "__main__":
    main()
