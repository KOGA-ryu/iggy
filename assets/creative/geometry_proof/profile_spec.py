"""Structural validation for SINC_GeometryProof_Profile specs.

This module checks that a spec DOCUMENT is well-formed: fields, types, name
references, loop closure by station name, and strict rejection of unknown
keys (a typo like "direciton" must be a build failure, not a silently ignored
field). Geometric validation - radius agreement, tangent continuity, winding
vs declared solid side - lives in profile_compiler.py, because it needs the
geometry.

Stdlib only. See BUILD_SPEC.md section 4 for the format this enforces.
"""

FORMAT = "SINC_GeometryProof_Profile/1"

EVIDENCE_CLASSES = ("PRINTED", "MEASURED", "FITTED", "DERIVED", "ESTIMATED", "AUTHORED")
JOIN_KINDS = ("HARD", "SMOOTH")
SEGMENT_KINDS = ("LINE", "ARC", "BEZIER")
SEGMENT_ROLES = ("MOULDED", "BOUNDING")
ARC_DIRECTIONS = ("CLOCKWISE", "COUNTERCLOCKWISE")
SOLID_SIDES = ("LEFT", "RIGHT")
UNITS = ("in", "mm", "cm", "m", "relative")
CLAIM_KINDS = ("free", "radius_of", "length_of", "angle_of", "distance", "extent")

# Colours the proof scene assigns per evidence class (linear RGB). Kept here so
# the legend and the scene cannot drift apart.
EVIDENCE_COLOURS = {
    "PRINTED":   (0.10, 0.75, 0.20, 1.0),
    "MEASURED":  (0.05, 0.45, 0.12, 1.0),
    "FITTED":    (0.15, 0.35, 0.90, 1.0),
    "DERIVED":   (0.10, 0.75, 0.75, 1.0),
    "ESTIMATED": (0.95, 0.55, 0.10, 1.0),
    "AUTHORED":  (0.85, 0.10, 0.65, 1.0),
}


class SpecError(ValueError):
    """Raised with every structural problem found, not just the first."""

    def __init__(self, problems):
        self.problems = list(problems)
        super().__init__("spec rejected:\n" + "\n".join("  - " + p for p in self.problems))


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


def validate_structure(spec):
    """Return a list of problems; empty list means structurally valid."""
    problems = []
    if not isinstance(spec, dict):
        return ["spec root must be an object"]

    _check_keys(
        spec,
        allowed={"format", "id", "reference", "units", "datum", "tolerance",
                 "construction_plane", "stations", "segments", "solid_side",
                 "parameters", "proof", "notes"},
        required={"format", "id", "reference", "units", "datum", "tolerance",
                  "construction_plane", "stations", "segments", "solid_side",
                  "parameters", "proof"},
        where="spec", problems=problems)
    if problems:
        # Missing top-level sections make the per-section checks meaningless.
        return problems

    if spec["format"] != FORMAT:
        problems.append(f"format: expected '{FORMAT}', got '{spec['format']}'")
    if not isinstance(spec["id"], str) or not spec["id"].replace("_", "").isalnum() or spec["id"] != spec["id"].lower():
        problems.append("id: must be a lowercase [a-z0-9_] identifier")

    ref = spec["reference"]
    if isinstance(ref, dict):
        _check_keys(ref, allowed={"source", "image", "scale_px_per_unit", "source_frame",
                                  "crop_px_at_origin", "crop_px_per_unit", "notes"},
                    required={"source"}, where="reference", problems=problems)
        if "scale_px_per_unit" in ref and (not _is_num(ref["scale_px_per_unit"]) or ref["scale_px_per_unit"] <= 0):
            problems.append("reference.scale_px_per_unit: must be a positive number")
        if "crop_px_at_origin" in ref and not _is_point(ref["crop_px_at_origin"]):
            problems.append("reference.crop_px_at_origin: must be [x, y]")
        if "crop_px_per_unit" in ref and (not _is_num(ref["crop_px_per_unit"]) or ref["crop_px_per_unit"] <= 0):
            problems.append("reference.crop_px_per_unit: must be a positive number")
    else:
        problems.append("reference: must be an object")

    if spec["units"] not in UNITS:
        problems.append(f"units: must be one of {UNITS}")

    tol = spec["tolerance"]
    if isinstance(tol, dict):
        _check_keys(tol, allowed={"distance", "angle_deg"}, required={"distance", "angle_deg"},
                    where="tolerance", problems=problems)
        for key in ("distance", "angle_deg"):
            if key in tol and (not _is_num(tol[key]) or tol[key] <= 0):
                problems.append(f"tolerance.{key}: must be a positive number")
    else:
        problems.append("tolerance: must be an object")

    plane = spec["construction_plane"]
    if isinstance(plane, dict):
        _check_keys(plane, allowed={"plane", "profile_x", "profile_y", "extrude_axis", "scale_m_per_unit"},
                    required={"plane", "profile_x", "profile_y", "extrude_axis", "scale_m_per_unit"},
                    where="construction_plane", problems=problems)
        if plane.get("plane") != "XZ":
            problems.append("construction_plane.plane: only 'XZ' is defined in format /1")
        if "scale_m_per_unit" in plane and (not _is_num(plane["scale_m_per_unit"]) or plane["scale_m_per_unit"] <= 0):
            problems.append("construction_plane.scale_m_per_unit: must be a positive number")
    else:
        problems.append("construction_plane: must be an object")

    stations = spec["stations"]
    if not isinstance(stations, dict) or not stations:
        problems.append("stations: must be a non-empty object")
        stations = {}
    for name, st in stations.items():
        where = f"stations.{name}"
        if not isinstance(st, dict):
            problems.append(f"{where}: must be an object")
            continue
        _check_keys(st, allowed={"at", "source_px", "evidence", "join", "note"},
                    required={"at", "evidence", "join"}, where=where, problems=problems)
        if "at" in st and not _is_point(st["at"]):
            problems.append(f"{where}.at: must be [x, y]")
        if "source_px" in st and not _is_point(st["source_px"]):
            problems.append(f"{where}.source_px: must be [x, y]")
        if "evidence" in st and st["evidence"] not in EVIDENCE_CLASSES:
            problems.append(f"{where}.evidence: must be one of {EVIDENCE_CLASSES}")
        if "join" in st and st["join"] not in JOIN_KINDS:
            problems.append(f"{where}.join: must be one of {JOIN_KINDS}")

    segments = spec["segments"]
    if not isinstance(segments, list) or len(segments) < 3:
        problems.append("segments: must be a list of at least 3 segments (a closed outline)")
        segments = []
    seen_names = set()
    for i, seg in enumerate(segments):
        where = f"segments[{i}]"
        if not isinstance(seg, dict):
            problems.append(f"{where}: must be an object")
            continue
        name = seg.get("name")
        where = f"segments[{i}] ({name})" if isinstance(name, str) else where
        base_allowed = {"name", "kind", "from", "to", "role", "evidence", "note"}
        kind = seg.get("kind")
        if kind == "ARC":
            allowed = base_allowed | {"centre", "direction"}
            required = {"name", "kind", "from", "to", "role", "evidence", "centre", "direction"}
        elif kind == "BEZIER":
            allowed = base_allowed | {"handle_from", "handle_to"}
            required = {"name", "kind", "from", "to", "role", "evidence", "handle_from", "handle_to"}
        else:
            allowed = base_allowed
            required = {"name", "kind", "from", "to", "role", "evidence"}
        _check_keys(seg, allowed=allowed, required=required, where=where, problems=problems)
        if not isinstance(name, str) or not name:
            problems.append(f"{where}: name must be a non-empty string")
        elif name in seen_names:
            problems.append(f"{where}: duplicate segment name")
        else:
            seen_names.add(name)
        if kind not in SEGMENT_KINDS:
            problems.append(f"{where}.kind: must be one of {SEGMENT_KINDS}")
        if "role" in seg and seg["role"] not in SEGMENT_ROLES:
            problems.append(f"{where}.role: must be one of {SEGMENT_ROLES}")
        if "evidence" in seg and seg["evidence"] not in EVIDENCE_CLASSES:
            problems.append(f"{where}.evidence: must be one of {EVIDENCE_CLASSES}")
        for key in ("from", "to"):
            if key in seg and seg[key] not in stations:
                problems.append(f"{where}.{key}: unknown station '{seg[key]}'")
        if kind == "ARC":
            if "centre" in seg and not _is_point(seg["centre"]):
                problems.append(f"{where}.centre: must be [x, y]")
            if "direction" in seg and seg["direction"] not in ARC_DIRECTIONS:
                problems.append(f"{where}.direction: must be one of {ARC_DIRECTIONS}")
        if kind == "BEZIER":
            for key in ("handle_from", "handle_to"):
                if key in seg and not _is_point(seg[key]):
                    problems.append(f"{where}.{key}: must be [x, y]")

    # The outline must be one closed loop in listed order, chained by name.
    if segments and not any(p.startswith("segments") for p in problems):
        for i, seg in enumerate(segments):
            nxt = segments[(i + 1) % len(segments)]
            if seg["to"] != nxt["from"]:
                problems.append(
                    f"segments: not a closed loop - segments[{i}] ({seg['name']}) ends at "
                    f"'{seg['to']}' but segments[{(i + 1) % len(segments)}] ({nxt['name']}) "
                    f"starts at '{nxt['from']}'")
        starts = [seg["from"] for seg in segments]
        if len(set(starts)) != len(starts):
            problems.append("segments: a station is visited twice - the outline must be a simple loop")
        unused = set(stations) - set(starts)
        if unused:
            problems.append(f"stations: unused stations {sorted(unused)} - remove them or wire them in")

    if spec["solid_side"] not in SOLID_SIDES:
        problems.append(f"solid_side: must be one of {SOLID_SIDES}")

    params = spec["parameters"]
    if not isinstance(params, dict):
        problems.append("parameters: must be an object")
        params = {}
    for pname, p in params.items():
        where = f"parameters.{pname}"
        if not isinstance(p, dict):
            problems.append(f"{where}: must be an object")
            continue
        _check_keys(p, allowed={"value", "claim", "note"}, required={"value", "claim"},
                    where=where, problems=problems)
        if "value" in p and not _is_num(p["value"]):
            problems.append(f"{where}.value: must be a number")
        claim = p.get("claim")
        if not isinstance(claim, dict) or claim.get("kind") not in CLAIM_KINDS:
            problems.append(f"{where}.claim.kind: must be one of {CLAIM_KINDS}")
            continue
        kind = claim["kind"]
        if kind == "free":
            _check_keys(claim, allowed={"kind"}, required={"kind"}, where=f"{where}.claim", problems=problems)
        elif kind in ("radius_of", "length_of", "angle_of"):
            _check_keys(claim, allowed={"kind", "segment"}, required={"kind", "segment"},
                        where=f"{where}.claim", problems=problems)
            if claim.get("segment") not in seen_names:
                problems.append(f"{where}.claim.segment: unknown segment '{claim.get('segment')}'")
        elif kind == "distance":
            _check_keys(claim, allowed={"kind", "between"}, required={"kind", "between"},
                        where=f"{where}.claim", problems=problems)
            between = claim.get("between")
            if (not isinstance(between, list) or len(between) != 2
                    or any(s not in stations for s in between)):
                problems.append(f"{where}.claim.between: must name two known stations")
        elif kind == "extent":
            _check_keys(claim, allowed={"kind", "axis"}, required={"kind", "axis"},
                        where=f"{where}.claim", problems=problems)
            if claim.get("axis") not in ("width", "depth"):
                problems.append(f"{where}.claim.axis: must be 'width' or 'depth'")

    if spec["datum"] not in params:
        problems.append(f"datum: '{spec['datum']}' must name a parameter")

    proof = spec["proof"]
    if isinstance(proof, dict):
        _check_keys(proof, allowed={"extrusion_depth"}, required={"extrusion_depth"},
                    where="proof", problems=problems)
        if "extrusion_depth" in proof and (not _is_num(proof["extrusion_depth"]) or proof["extrusion_depth"] <= 0):
            problems.append("proof.extrusion_depth: must be a positive number")
    else:
        problems.append("proof: must be an object")

    if "notes" in spec and (not isinstance(spec["notes"], list)
                            or any(not isinstance(n, str) for n in spec["notes"])):
        problems.append("notes: must be a list of strings")

    return problems


def require_valid_structure(spec):
    problems = validate_structure(spec)
    if problems:
        raise SpecError(problems)
