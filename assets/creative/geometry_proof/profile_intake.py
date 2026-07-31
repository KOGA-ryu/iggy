"""Intake: research measurement handoffs -> draft profile specs.

The bridge between measurement threads (plate decompositions, measured
schedules, arc traces) and the proof harness. A research thread emits ONE
handoff document in its own working vocabulary - calibration, image frames,
named landmarks, scalar extents, optional traced curve samples - and this
module converts it into a DRAFT profile spec:

  - stations placed in the profile frame, with source_px in the root frame;
  - tolerance.distance derived from the calibration residual (2 x RMS);
  - parameters carrying the measured extents as claims;
  - reference/calibration blocks filled;
  - circle fits for traced curved members, with residuals reported.

What intake deliberately does NOT do: declare segments, joins, roles, arc
directions, or solid side. Those are geometry READINGS - the authoring
judgment this system exists to make explicit - so the emitted draft fails
compilation until an author completes it, and the intake report says exactly
what remains. No pixels are interpreted; only declared measurements move.
(Phase 0's "no automatic image interpretation" boundary is preserved.)

Handoff format (strict, unknown keys rejected):

{
  "format": "SINC_GeometryProof_Handoff/1",
  "id": "ellis_451a_no4",
  "source": { "citation": "...", "image": "optional/path.png", "notes": ["..."] },
  "calibration": {
    "px_per_unit": 70.401, "units": "in", "rms_residual_px": 1.66,
    "method": "...", "crosscheck": "optional"
  },
  "frames": {
    "plate": { "parent": null, "note": "upright raster, y-down" },
    "no4_grid": { "parent": "plate", "offset_px": [520, 30], "scale": 2.0,
                  "note": "crop then 2x magnify" }
  },
  "profile_frame": {
    "origin_landmark": "some_landmark_name",
    "axes": "plate-default",   // +x = +root_x, +y = -root_y (y-up flip); only mode in /1
    "note": "..."
  },
  "landmarks": [
    { "name": "wall_start", "frame": "plate", "px": [1004, 772],
      "evidence": "MEASURED", "method": "...", "note": "..." }
  ],
  "arc_traces": [
    { "name": "slip_head", "frame": "plate", "points": [[x, y], ...],
      "evidence": "MEASURED", "note": "..." }
  ],
  "extents": [
    { "measure_id": "M03", "quantity": "stile depth", "value": 1.911,
      "status": "MEASURED", "method": "...", "note": "...",
      "between": ["landmark_a", "landmark_b"] }   // optional -> distance claim
  ],
  "datum_measure_id": "M03",
  "open_questions": ["..."]
}

CLI:
  python3 profile_intake.py handoff.json --out draft_profile.json
"""

import json
import math
import sys
from pathlib import Path

sys.dont_write_bytecode = True

if __package__ is None and str(Path(__file__).resolve().parent) not in sys.path:
    sys.path.insert(0, str(Path(__file__).resolve().parent))

from profile_spec import EVIDENCE_CLASSES, SpecError  # noqa: E402

HANDOFF_FORMAT = "SINC_GeometryProof_Handoff/1"
STATUS_TO_EVIDENCE = {
    "PRINTED": "PRINTED",
    "MEASURED": "MEASURED",
    "MEASURED-CAUTION": "MEASURED",
    "FITTED": "FITTED",
    "DERIVED": "DERIVED",
    "INTERPRETATION-OPEN": "ESTIMATED",
    "ESTIMATED": "ESTIMATED",
    "AUTHORED": "AUTHORED",
}
UNIT_TO_METRES = {"in": 0.0254, "mm": 0.001, "cm": 0.01, "m": 1.0, "relative": 0.05}


def _is_num(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool)


def _is_point(v):
    return isinstance(v, list) and len(v) == 2 and all(_is_num(c) for c in v)


def _check_keys(obj, allowed, required, where, problems):
    for key in obj:
        if key not in allowed:
            problems.append(f"{where}: unknown key '{key}'")
    for key in required:
        if key not in obj:
            problems.append(f"{where}: missing required key '{key}'")


def validate_handoff(handoff):
    problems = []
    if not isinstance(handoff, dict):
        return ["handoff root must be an object"]
    _check_keys(handoff,
                allowed={"format", "id", "source", "calibration", "frames",
                         "profile_frame", "landmarks", "arc_traces", "extents",
                         "datum_measure_id", "open_questions"},
                required={"format", "id", "source", "calibration", "frames",
                          "profile_frame", "landmarks", "extents"},
                where="handoff", problems=problems)
    if problems:
        return problems

    if handoff["format"] != HANDOFF_FORMAT:
        problems.append(f"format: expected '{HANDOFF_FORMAT}', got '{handoff['format']}'")
    if not isinstance(handoff["id"], str) or not handoff["id"].replace("_", "").isalnum():
        problems.append("id: must be a lowercase [a-z0-9_] identifier")

    src = handoff["source"]
    if isinstance(src, dict):
        _check_keys(src, allowed={"citation", "image", "notes"}, required={"citation"},
                    where="source", problems=problems)
    else:
        problems.append("source: must be an object")

    cal = handoff["calibration"]
    if isinstance(cal, dict):
        _check_keys(cal, allowed={"px_per_unit", "units", "rms_residual_px", "method", "crosscheck"},
                    required={"px_per_unit", "units", "rms_residual_px", "method"},
                    where="calibration", problems=problems)
        if "px_per_unit" in cal and (not _is_num(cal["px_per_unit"]) or cal["px_per_unit"] <= 0):
            problems.append("calibration.px_per_unit: must be a positive number")
        if "rms_residual_px" in cal and (not _is_num(cal["rms_residual_px"]) or cal["rms_residual_px"] < 0):
            problems.append("calibration.rms_residual_px: must be a non-negative number")
        if cal.get("units") not in UNIT_TO_METRES:
            problems.append(f"calibration.units: must be one of {sorted(UNIT_TO_METRES)}")
    else:
        problems.append("calibration: must be an object")

    frames = handoff["frames"]
    roots = []
    if isinstance(frames, dict) and frames:
        for name, fr in frames.items():
            where = f"frames.{name}"
            if not isinstance(fr, dict):
                problems.append(f"{where}: must be an object")
                continue
            if fr.get("parent") is None:
                _check_keys(fr, allowed={"parent", "note"}, required={"parent"},
                            where=where, problems=problems)
                roots.append(name)
            else:
                _check_keys(fr, allowed={"parent", "offset_px", "scale", "note"},
                            required={"parent", "offset_px", "scale"},
                            where=where, problems=problems)
                if fr["parent"] not in frames:
                    problems.append(f"{where}.parent: unknown frame '{fr['parent']}'")
                if "offset_px" in fr and not _is_point(fr["offset_px"]):
                    problems.append(f"{where}.offset_px: must be [x, y]")
                if "scale" in fr and (not _is_num(fr["scale"]) or fr["scale"] <= 0):
                    problems.append(f"{where}.scale: must be a positive number")
        if len(roots) != 1:
            problems.append(f"frames: exactly one root frame (parent null) required, found {roots}")
    else:
        problems.append("frames: must be a non-empty object")

    pf = handoff["profile_frame"]
    landmark_names = set()
    if isinstance(pf, dict):
        _check_keys(pf, allowed={"origin_landmark", "axes", "note"},
                    required={"origin_landmark", "axes"}, where="profile_frame", problems=problems)
        if pf.get("axes") != "plate-default":
            problems.append("profile_frame.axes: only 'plate-default' is defined in format /1")
    else:
        problems.append("profile_frame: must be an object")

    if not isinstance(handoff["landmarks"], list):
        problems.append("landmarks: must be a list")
    else:
        for i, lm in enumerate(handoff["landmarks"]):
            where = f"landmarks[{i}]"
            if not isinstance(lm, dict):
                problems.append(f"{where}: must be an object")
                continue
            _check_keys(lm, allowed={"name", "frame", "px", "evidence", "method", "note"},
                        required={"name", "frame", "px", "evidence"}, where=where, problems=problems)
            name = lm.get("name")
            if not isinstance(name, str) or not name:
                problems.append(f"{where}.name: must be a non-empty string")
            elif name in landmark_names:
                problems.append(f"{where}: duplicate landmark name '{name}'")
            else:
                landmark_names.add(name)
            if "frame" in lm and isinstance(frames, dict) and lm["frame"] not in frames:
                problems.append(f"{where}.frame: unknown frame '{lm['frame']}'")
            if "px" in lm and not _is_point(lm["px"]):
                problems.append(f"{where}.px: must be [x, y]")
            if "evidence" in lm and lm["evidence"] not in EVIDENCE_CLASSES:
                problems.append(f"{where}.evidence: must be one of {EVIDENCE_CLASSES}")

    if isinstance(pf, dict) and isinstance(pf.get("origin_landmark"), str) \
            and isinstance(handoff["landmarks"], list) \
            and pf["origin_landmark"] not in landmark_names:
        problems.append(
            f"profile_frame.origin_landmark: unknown landmark '{pf['origin_landmark']}'"
            + (" - the landmarks list is empty; deliver the 2D landmark assembly first"
               if not landmark_names else ""))

    for i, tr in enumerate(handoff.get("arc_traces", [])):
        where = f"arc_traces[{i}]"
        if not isinstance(tr, dict):
            problems.append(f"{where}: must be an object")
            continue
        _check_keys(tr, allowed={"name", "frame", "points", "evidence", "note"},
                    required={"name", "frame", "points", "evidence"}, where=where, problems=problems)
        if "frame" in tr and isinstance(frames, dict) and tr["frame"] not in frames:
            problems.append(f"{where}.frame: unknown frame '{tr['frame']}'")
        pts = tr.get("points")
        if not isinstance(pts, list) or len(pts) < 5 or not all(_is_point(p) for p in pts):
            problems.append(f"{where}.points: must be a list of at least 5 [x, y] samples")

    measure_ids = set()
    if not isinstance(handoff["extents"], list):
        problems.append("extents: must be a list")
    else:
        for i, ex in enumerate(handoff["extents"]):
            where = f"extents[{i}]"
            if not isinstance(ex, dict):
                problems.append(f"{where}: must be an object")
                continue
            _check_keys(ex, allowed={"measure_id", "quantity", "value", "status",
                                     "method", "note", "between"},
                        required={"measure_id", "quantity", "value", "status"},
                        where=where, problems=problems)
            mid = ex.get("measure_id")
            if not isinstance(mid, str) or not mid:
                problems.append(f"{where}.measure_id: must be a non-empty string")
            elif mid in measure_ids:
                problems.append(f"{where}: duplicate measure_id '{mid}'")
            else:
                measure_ids.add(mid)
            if "value" in ex and not _is_num(ex["value"]):
                problems.append(f"{where}.value: must be a number")
            if ex.get("status") not in STATUS_TO_EVIDENCE:
                problems.append(f"{where}.status: must be one of {sorted(STATUS_TO_EVIDENCE)}")
            between = ex.get("between")
            if between is not None:
                if (not isinstance(between, list) or len(between) != 2
                        or any(b not in landmark_names for b in between)):
                    problems.append(f"{where}.between: must name two known landmarks")

    if "datum_measure_id" in handoff and handoff["datum_measure_id"] not in measure_ids:
        problems.append(f"datum_measure_id: unknown measure '{handoff.get('datum_measure_id')}'")

    if "open_questions" in handoff and (not isinstance(handoff["open_questions"], list)
                                        or any(not isinstance(q, str) for q in handoff["open_questions"])):
        problems.append("open_questions: must be a list of strings")

    return problems


# ---------------------------------------------------------------- frames

def resolve_to_root(px, frame_name, frames):
    """Point in a working frame -> root-frame pixels (child = (parent-offset)*scale)."""
    x, y = px
    name = frame_name
    seen = set()
    while frames[name].get("parent") is not None:
        if name in seen:
            raise ValueError(f"frame cycle at '{name}'")
        seen.add(name)
        fr = frames[name]
        x = fr["offset_px"][0] + x / fr["scale"]
        y = fr["offset_px"][1] + y / fr["scale"]
        name = fr["parent"]
    return (x, y)


def root_px_to_profile(px, origin_px, px_per_unit):
    """plate-default axes: +x = +root_x, +y = -root_y, origin at the origin landmark."""
    return ((px[0] - origin_px[0]) / px_per_unit,
            (origin_px[1] - px[1]) / px_per_unit)


# ---------------------------------------------------------------- arc fitting

def fit_arc(points):
    """Least-squares (Kasa) circle fit. Returns dict with centre, radius,
    rms_residual, max_residual. Pure stdlib: 3x3 normal equations."""
    n = len(points)
    sx = sum(p[0] for p in points)
    sy = sum(p[1] for p in points)
    sxx = sum(p[0] * p[0] for p in points)
    syy = sum(p[1] * p[1] for p in points)
    sxy = sum(p[0] * p[1] for p in points)
    sxz = sum(p[0] * (p[0] * p[0] + p[1] * p[1]) for p in points)
    syz = sum(p[1] * (p[0] * p[0] + p[1] * p[1]) for p in points)
    sz = sum(p[0] * p[0] + p[1] * p[1] for p in points)
    # circle x^2 + y^2 + A x + B y + C = 0
    m = [[sxx, sxy, sx, -sxz],
         [sxy, syy, sy, -syz],
         [sx, sy, float(n), -sz]]
    for col in range(3):  # gaussian elimination, partial pivot
        pivot = max(range(col, 3), key=lambda r: abs(m[r][col]))
        if abs(m[pivot][col]) < 1e-12:
            raise ValueError("degenerate arc trace - points are colinear or coincident")
        m[col], m[pivot] = m[pivot], m[col]
        for row in range(col + 1, 3):
            f = m[row][col] / m[col][col]
            for k in range(col, 4):
                m[row][k] -= f * m[col][k]
    coef = [0.0, 0.0, 0.0]
    for row in (2, 1, 0):
        coef[row] = (m[row][3] - sum(m[row][k] * coef[k] for k in range(row + 1, 3))) / m[row][row]
    a, b, c = coef
    cx, cy = -a / 2.0, -b / 2.0
    r_sq = cx * cx + cy * cy - c
    if r_sq <= 0:
        raise ValueError("degenerate arc trace - non-positive fitted radius")
    r = math.sqrt(r_sq)
    residuals = [abs(math.hypot(p[0] - cx, p[1] - cy) - r) for p in points]
    rms = math.sqrt(sum(e * e for e in residuals) / n)
    return {"centre": (cx, cy), "radius": r,
            "rms_residual": rms, "max_residual": max(residuals)}


# ---------------------------------------------------------------- draft emission

def _param_name(extent):
    slug = "".join(ch if ch.isalnum() else "_" for ch in extent["quantity"].lower())
    while "__" in slug:
        slug = slug.replace("__", "_")
    return f"{extent['measure_id'].lower()}_{slug.strip('_')}"[:48]


def _profile_transform(handoff):
    """(frames, origin_root, px_per_unit) for a structurally valid handoff."""
    frames = handoff["frames"]
    origin_name = handoff["profile_frame"]["origin_landmark"]
    by_name = {lm["name"]: lm for lm in handoff["landmarks"]}
    origin_root = resolve_to_root(by_name[origin_name]["px"], by_name[origin_name]["frame"], frames)
    return frames, origin_root, handoff["calibration"]["px_per_unit"]


def resolved_arc_traces(handoff):
    """[{name, points (profile frame), fit}] for a structurally VALID handoff.

    Shared by intake and the proposer so trace geometry has one source.
    """
    frames, origin_root, ppu = _profile_transform(handoff)
    out = []
    for tr in handoff.get("arc_traces", []):
        root_pts = [resolve_to_root(p, tr["frame"], frames) for p in tr["points"]]
        prof_pts = [root_px_to_profile(p, origin_root, ppu) for p in root_pts]
        out.append({"name": tr["name"], "points": prof_pts, "fit": fit_arc(prof_pts)})
    return out


def build_draft(handoff):
    """Handoff dict -> (draft_spec_dict, report dict). Raises SpecError."""
    problems = validate_handoff(handoff)
    if problems:
        raise SpecError(problems)

    cal = handoff["calibration"]
    frames = handoff["frames"]
    ppu = cal["px_per_unit"]
    tol_dist = max(round(2.0 * cal["rms_residual_px"] / ppu, 4), 1e-4)

    origin_name = handoff["profile_frame"]["origin_landmark"]
    _, origin_root, _ = _profile_transform(handoff)

    todo = []
    stations = {}
    for lm in handoff["landmarks"]:
        root = resolve_to_root(lm["px"], lm["frame"], frames)
        at = root_px_to_profile(root, origin_root, ppu)
        st = {"at": [round(at[0], 4), round(at[1], 4)],
              "source_px": [round(root[0], 2), round(root[1], 2)],
              "evidence": lm["evidence"],
              "join": "HARD",
              "note": "DRAFT: join defaulted to HARD - confirm against the reference. "
                      + lm.get("note", "")}
        stations[lm["name"]] = st
    todo.append("confirm every station's join (all drafted HARD)")

    parameters = {}
    for ex in handoff["extents"]:
        name = _param_name(ex)
        claim = {"kind": "free"}
        if ex.get("between"):
            claim = {"kind": "distance", "between": list(ex["between"])}
        note_bits = [f"{ex['measure_id']} ({ex['status']})"]
        if ex.get("method"):
            note_bits.append(ex["method"])
        if ex.get("note"):
            note_bits.append(ex["note"])
        parameters[name] = {"value": ex["value"], "claim": claim,
                            "note": "; ".join(note_bits)}
    datum = None
    if "datum_measure_id" in handoff:
        datum = _param_name(next(e for e in handoff["extents"]
                                 if e["measure_id"] == handoff["datum_measure_id"]))
    else:
        todo.append("choose a datum parameter (none declared in the handoff)")

    arc_fits = {}
    for resolved in resolved_arc_traces(handoff):
        tr, fit = resolved, resolved["fit"]
        ok = fit["rms_residual"] <= tol_dist
        arc_fits[tr["name"]] = {
            "centre": [round(fit["centre"][0], 4), round(fit["centre"][1], 4)],
            "radius": round(fit["radius"], 4),
            "rms_residual": round(fit["rms_residual"], 5),
            "max_residual": round(fit["max_residual"], 5),
            "samples": len(tr["points"]),
            "suggested_evidence": "FITTED" if ok else "ESTIMATED",
            "within_tolerance": ok,
        }
        if not ok:
            todo.append(f"arc trace '{tr['name']}' fits poorly "
                        f"(rms {fit['rms_residual']:.4f} > tolerance {tol_dist}) - "
                        "re-trace or treat as non-circular")
    if arc_fits:
        todo.append("author ARC segments from the fitted centres; DECLARE each direction")

    todo.append("declare segments (the closed outline, roles MOULDED/BOUNDING) - "
                "the draft does not compile until this is done")
    todo.append("declare solid_side (the compiler will reject a wrong declaration)")
    todo.append("set proof.extrusion_depth and review construction_plane defaults")
    for q in handoff.get("open_questions", []):
        todo.append(f"open question from research: {q}")

    units = cal["units"]
    draft = {
        "format": "SINC_GeometryProof_Profile/1",
        "id": f"{handoff['id']}_v1",
        "reference": {
            "source": handoff["source"]["citation"],
            "scale_px_per_unit": ppu,
            "source_frame": "root frame of handoff "
                            f"'{handoff['id']}', y-down; origin landmark '{origin_name}'",
            "notes": f"calibration: {cal['method']}; rms {cal['rms_residual_px']} px"
                     + (f"; crosscheck: {cal['crosscheck']}" if cal.get("crosscheck") else ""),
        },
        "units": units,
        "datum": datum if datum else "TODO_choose_datum",
        "tolerance": {"distance": tol_dist, "angle_deg": 2.0},
        "construction_plane": {
            "plane": "XZ",
            "profile_x": "+X (from the handoff root frame +x)",
            "profile_y": "+Z up (root frame y flipped)",
            "extrude_axis": "+Y",
            "scale_m_per_unit": UNIT_TO_METRES[units],
        },
        "stations": stations,
        "segments": [],
        "solid_side": "LEFT",
        "parameters": parameters,
        "proof": {"extrusion_depth": 1.0},
        "notes": (["DRAFT emitted by profile_intake from handoff "
                   f"'{handoff['id']}' - NOT compilable until segments are authored."]
                  + [f"TODO: {t}" for t in todo]),
    }
    if handoff["source"].get("image"):
        draft["reference"]["image"] = handoff["source"]["image"]

    report = {"handoff_id": handoff["id"], "stations": len(stations),
              "parameters": len(parameters), "arc_fits": arc_fits,
              "tolerance_distance": tol_dist, "datum": draft["datum"], "todo": todo}
    return draft, report


def main():
    args = sys.argv[1:]
    if len(args) < 1 or "--out" not in args:
        raise SystemExit("usage: python3 profile_intake.py HANDOFF.json --out DRAFT.json")
    handoff_path = Path(args[0])
    out_path = Path(args[args.index("--out") + 1])
    handoff = json.loads(handoff_path.read_text())
    draft, report = build_draft(handoff)
    out_path.write_text(json.dumps(draft, indent=1))
    print(json.dumps(report, indent=1))
    print(f"DRAFT written to {out_path} - complete the TODO list, then build with proof_scene.py")


if __name__ == "__main__":
    main()
