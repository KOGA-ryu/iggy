"""Compile a SINC_GeometryProof_Profile spec into geometry - the ONLY producer.

Proof annotations, master curve, solid fill, extrusion, and production
consumers all read the CompiledProfile this module emits. There is no second
authoring path (BUILD_SPEC.md section 3).

Geometric validation lives here because it needs the geometry:
  - ARC centre must be equidistant from both endpoints (tolerance.distance);
  - SMOOTH joins must actually be tangent-continuous (tolerance.angle_deg);
  - the declared solid_side must agree with the outline's winding;
  - the outline must be a simple (non-self-intersecting) closed polygon;
  - source_px stations must register against the declared plate frame;
  - every parameter claim must hold against the built geometry.

All problems are collected and raised together as SpecError. Warnings (HARD
join that is actually tangent-continuous, for instance) are carried on the
result for the manifest. Stdlib only, deterministic: same spec, same floats.
"""

import math

from profile_spec import SpecError, require_valid_structure


TWO_PI = 2.0 * math.pi


class CompiledProfile:
    def __init__(self):
        self.spec = None
        self.outline = []            # [(x, y)] closed polygon, first point not repeated
        self.segments = []           # per-segment dicts, see _compile_segment
        self.stations = {}           # name -> dict(at, join, evidence, tangent_in, tangent_out)
        self.moulded_chains = []     # list of [(x, y)] runs of consecutive MOULDED segments
        self.signed_area = 0.0
        self.winding = None          # "CLOCKWISE" | "COUNTERCLOCKWISE"
        self.extents = {}            # {"width": w, "depth": d}
        self.normalized_stations = {}
        self.datum_value = None
        self.warnings = []
        self.samples_per_quarter = None
        self.source_px_mapped = {}


def _v_sub(a, b):
    return (a[0] - b[0], a[1] - b[1])


def _v_len(a):
    return math.hypot(a[0], a[1])


def _v_norm(a):
    length = _v_len(a)
    if length == 0.0:
        return None
    return (a[0] / length, a[1] / length)


def _angle_between_deg(u, v):
    dot = max(-1.0, min(1.0, u[0] * v[0] + u[1] * v[1]))
    return math.degrees(math.acos(dot))


def _sample_line(a, b, _spq):
    return [tuple(a), tuple(b)], _v_norm(_v_sub(b, a)), _v_norm(_v_sub(b, a))


def _arc_params(a, b, centre, direction, tol_dist, where, problems):
    r_from = _v_len(_v_sub(a, centre))
    r_to = _v_len(_v_sub(b, centre))
    if abs(r_from - r_to) > tol_dist:
        problems.append(
            f"{where}: centre is not equidistant from endpoints - "
            f"|centre-from| = {r_from:.4f}, |centre-to| = {r_to:.4f}, "
            f"tolerance {tol_dist}")
        return None
    radius = 0.5 * (r_from + r_to)
    if radius <= tol_dist:
        problems.append(f"{where}: radius {radius:.4f} is at or below tolerance - degenerate arc")
        return None
    a0 = math.atan2(a[1] - centre[1], a[0] - centre[0])
    a1 = math.atan2(b[1] - centre[1], b[0] - centre[0])
    if direction == "COUNTERCLOCKWISE":
        sweep = (a1 - a0) % TWO_PI
    else:
        sweep = (a0 - a1) % TWO_PI
    if sweep == 0.0:
        problems.append(f"{where}: endpoints are angularly coincident - sweep would be 0 or 360 degrees")
        return None
    if math.degrees(sweep) < 0.5:
        problems.append(
            f"{where}: sweep is {math.degrees(sweep):.4f} deg - a sliver arc. No moulding "
            "member sweeps below 0.5 deg; this is almost always a direction flip on "
            "near-coincident stations")
        return None
    return radius, a0, sweep


def _sample_arc(a, b, centre, direction, radius, a0, sweep, spq):
    sign = 1.0 if direction == "COUNTERCLOCKWISE" else -1.0
    n = max(2, int(math.ceil(spq * sweep / (math.pi / 2.0))))
    pts = [tuple(a)]
    for i in range(1, n):
        ang = a0 + sign * sweep * (i / n)
        pts.append((centre[0] + radius * math.cos(ang), centre[1] + radius * math.sin(ang)))
    pts.append(tuple(b))

    def tangent(ang):
        if direction == "COUNTERCLOCKWISE":
            return (-math.sin(ang), math.cos(ang))
        return (math.sin(ang), -math.cos(ang))

    mid_ang = a0 + sign * sweep * 0.5
    mid_point = (centre[0] + radius * math.cos(mid_ang), centre[1] + radius * math.sin(mid_ang))
    return pts, tangent(a0), tangent(a0 + sign * sweep), mid_point, tangent(mid_ang)


def _sample_bezier(a, h1, h2, b, spq, where, problems):
    n = max(8, 2 * spq)
    pts = []
    for i in range(n + 1):
        t = i / n
        mt = 1.0 - t
        x = (mt ** 3 * a[0] + 3 * mt ** 2 * t * h1[0] + 3 * mt * t ** 2 * h2[0] + t ** 3 * b[0])
        y = (mt ** 3 * a[1] + 3 * mt ** 2 * t * h1[1] + 3 * mt * t ** 2 * h2[1] + t ** 3 * b[1])
        pts.append((x, y))
    pts[0] = tuple(a)
    pts[-1] = tuple(b)
    t_in = _v_norm(_v_sub(h1, a)) or _v_norm(_v_sub(h2, a)) or _v_norm(_v_sub(b, a))
    t_out = _v_norm(_v_sub(b, h2)) or _v_norm(_v_sub(b, h1)) or _v_norm(_v_sub(b, a))
    if t_in is None or t_out is None:
        problems.append(f"{where}: degenerate bezier - endpoints and handles coincide")
    return pts, t_in, t_out


def _signed_area(points):
    total = 0.0
    for i, (x0, y0) in enumerate(points):
        x1, y1 = points[(i + 1) % len(points)]
        total += x0 * y1 - x1 * y0
    return 0.5 * total


def _orient(a, b, c, eps):
    val = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])
    if abs(val) <= eps:
        return 0
    return 1 if val > 0 else -1


def _on_segment(a, b, p, eps):
    """p (already colinear with a-b) lies within the segment's bounds."""
    return (min(a[0], b[0]) - eps <= p[0] <= max(a[0], b[0]) + eps
            and min(a[1], b[1]) - eps <= p[1] <= max(a[1], b[1]) + eps)


def _segments_intersect(p1, p2, p3, p4, eps_cross, eps_dist):
    """True for proper crossings AND improper touches / colinear overlaps."""
    o1 = _orient(p1, p2, p3, eps_cross)
    o2 = _orient(p1, p2, p4, eps_cross)
    o3 = _orient(p3, p4, p1, eps_cross)
    o4 = _orient(p3, p4, p2, eps_cross)
    if o1 != o2 and o3 != o4 and 0 not in (o1, o2, o3, o4):
        return True
    if o1 == 0 and _on_segment(p1, p2, p3, eps_dist):
        return True
    if o2 == 0 and _on_segment(p1, p2, p4, eps_dist):
        return True
    if o3 == 0 and _on_segment(p3, p4, p1, eps_dist):
        return True
    if o4 == 0 and _on_segment(p3, p4, p2, eps_dist):
        return True
    return False


def _check_simple_polygon(points, problems):
    """Reject proper crossings, improper touches, colinear overlaps, and
    doubling-back - the classes the adversarial review proved slipped through
    a proper-crossing-only test. Epsilons are exact-scale: a sampled cusp
    (fig 6's bead neck, ~2 deg between adjacent chords) must pass; an exact
    colinear reversal must not."""
    n = len(points)
    diag = max(max(p[0] for p in points) - min(p[0] for p in points),
               max(p[1] for p in points) - min(p[1] for p in points)) or 1.0
    eps_cross = 1e-10 * diag * diag
    eps_dist = 1e-9 * diag
    # adjacent edges: doubling back along the shared vertex
    for i in range(n):
        a = points[(i - 1) % n]
        p = points[i]
        b = points[(i + 1) % n]
        if (_orient(a, p, b, eps_cross) == 0
                and (a[0] - p[0]) * (b[0] - p[0]) + (a[1] - p[1]) * (b[1] - p[1]) > 0):
            problems.append(
                f"outline doubles back on itself at sampled vertex {i} - "
                "adjacent edges are colinear and overlapping")
            return
    # non-adjacent edges: any contact at all is a defect
    for i in range(n):
        a1, a2 = points[i], points[(i + 1) % n]
        for j in range(i + 2, n):
            if i == 0 and j == n - 1:
                continue  # adjacent through the loop closure
            b1, b2 = points[j], points[(j + 1) % n]
            if _segments_intersect(a1, a2, b1, b2, eps_cross, eps_dist):
                problems.append(
                    f"outline self-intersects between sampled edges {i} and {j} - "
                    "the closed outline must be a simple polygon")
                return


def _check_source_px(spec, problems):
    """Validate px registration; return {station: mapped profile point}."""
    scale = spec["reference"].get("scale_px_per_unit")
    tol = spec["tolerance"]["distance"]
    stations = spec["stations"]
    order = [seg["from"] for seg in spec["segments"]]
    anchored = [name for name in order if "source_px" in stations[name]]
    if not anchored:
        return {}
    if scale is None:
        problems.append("reference.scale_px_per_unit is required when stations carry source_px")
        return {}
    anchor = anchored[0]
    ax, ay = stations[anchor]["at"]
    apx, apy = stations[anchor]["source_px"]
    mapped = {anchor: (ax, ay)}
    for name in anchored[1:]:
        px, py = stations[name]["source_px"]
        predicted = (ax + (px - apx) / scale, ay + (apy - py) / scale)
        mapped[name] = predicted
        err = _v_len(_v_sub(predicted, stations[name]["at"]))
        if err > tol:
            problems.append(
                f"stations.{name}: source_px registers {err:.3f} {spec['units']} away from 'at' "
                f"(anchor '{anchor}', scale {scale} px/{spec['units']}, tolerance {tol}) - "
                "transcription or frame error")
    return mapped


def point_in_polygon(point, polygon):
    """Ray-casting test; polygon is [(x, y)] with implied closure."""
    x, y = point
    inside = False
    n = len(polygon)
    for i in range(n):
        x0, y0 = polygon[i]
        x1, y1 = polygon[(i + 1) % n]
        if (y0 > y) != (y1 > y):
            t = (y - y0) / (y1 - y0)
            if x < x0 + t * (x1 - x0):
                inside = not inside
    return inside


def polygon_centroid(polygon):
    """Area-weighted centroid of a simple polygon."""
    a = cx = cy = 0.0
    n = len(polygon)
    for i in range(n):
        x0, y0 = polygon[i]
        x1, y1 = polygon[(i + 1) % n]
        cross = x0 * y1 - x1 * y0
        a += cross
        cx += (x0 + x1) * cross
        cy += (y0 + y1) * cross
    a *= 0.5
    if a == 0.0:
        return polygon[0]
    return (cx / (6.0 * a), cy / (6.0 * a))


def _check_claims(spec, compiled, problems):
    tol_dist = spec["tolerance"]["distance"]
    tol_ang = spec["tolerance"]["angle_deg"]
    seg_by_name = {s["name"]: s for s in compiled.segments}
    for pname, param in spec["parameters"].items():
        claim = param["claim"]
        kind = claim["kind"]
        value = param["value"]
        where = f"parameters.{pname}"
        if kind == "free":
            continue
        if kind == "radius_of":
            seg = seg_by_name[claim["segment"]]
            if seg["kind"] != "ARC":
                problems.append(f"{where}: radius_of targets non-ARC segment '{seg['name']}'")
            elif seg.get("radius") is not None and abs(seg["radius"] - value) > tol_dist:
                problems.append(
                    f"{where}: claims radius {value}, geometry gives {seg['radius']:.4f} "
                    f"(tolerance {tol_dist})")
        elif kind == "length_of":
            seg = seg_by_name[claim["segment"]]
            if seg["kind"] != "LINE":
                problems.append(f"{where}: length_of targets non-LINE segment '{seg['name']}'")
            else:
                length = _v_len(_v_sub(seg["points"][-1], seg["points"][0]))
                if abs(length - value) > tol_dist:
                    problems.append(
                        f"{where}: claims length {value}, geometry gives {length:.4f} "
                        f"(tolerance {tol_dist})")
        elif kind == "angle_of":
            seg = seg_by_name[claim["segment"]]
            if seg["kind"] != "LINE":
                problems.append(f"{where}: angle_of targets non-LINE segment '{seg['name']}'")
            else:
                d = _v_sub(seg["points"][-1], seg["points"][0])
                angle = math.degrees(math.atan2(d[1], d[0]))
                diff = (angle - value + 180.0) % 360.0 - 180.0
                if abs(diff) > tol_ang:
                    problems.append(
                        f"{where}: claims angle {value} deg, geometry gives {angle:.2f} deg "
                        f"(tolerance {tol_ang} deg)")
        elif kind == "distance":
            a_name, b_name = claim["between"]
            a = spec["stations"][a_name]["at"]
            b = spec["stations"][b_name]["at"]
            dist = _v_len(_v_sub(a, b))
            if abs(dist - value) > tol_dist:
                problems.append(
                    f"{where}: claims distance {value}, geometry gives {dist:.4f} "
                    f"(tolerance {tol_dist})")
        elif kind == "extent":
            actual = compiled.extents[claim["axis"]]
            if abs(actual - value) > tol_dist:
                problems.append(
                    f"{where}: claims {claim['axis']} {value}, geometry gives {actual:.4f} "
                    f"(tolerance {tol_dist})")


def compile_profile(spec, samples_per_quarter=24):
    """Spec dict -> CompiledProfile. Raises SpecError listing every problem."""
    require_valid_structure(spec)

    problems = []
    warnings = []
    tol_dist = spec["tolerance"]["distance"]
    tol_ang = spec["tolerance"]["angle_deg"]
    stations = spec["stations"]

    compiled = CompiledProfile()
    compiled.spec = spec
    compiled.samples_per_quarter = samples_per_quarter

    # --- per-segment sampling -------------------------------------------------
    for seg in spec["segments"]:
        name = seg["name"]
        where = f"segments.{name}"
        a = tuple(stations[seg["from"]]["at"])
        b = tuple(stations[seg["to"]]["at"])
        out = {"name": name, "kind": seg["kind"], "role": seg["role"],
               "evidence": seg["evidence"], "from": seg["from"], "to": seg["to"],
               "points": None, "tangent_in": None, "tangent_out": None}
        if _v_len(_v_sub(a, b)) <= tol_dist and seg["kind"] != "ARC":
            problems.append(f"{where}: endpoints coincide within tolerance - degenerate segment")
            compiled.segments.append(out)
            continue
        if seg["kind"] == "LINE":
            pts, t_in, t_out = _sample_line(a, b, samples_per_quarter)
            out.update({"points": pts, "tangent_in": t_in, "tangent_out": t_out})
        elif seg["kind"] == "ARC":
            centre = tuple(seg["centre"])
            params = _arc_params(a, b, centre, seg["direction"], tol_dist, where, problems)
            if params is None:
                compiled.segments.append(out)
                continue
            radius, a0, sweep = params
            pts, t_in, t_out, mid_point, mid_tangent = _sample_arc(
                a, b, centre, seg["direction"], radius, a0, sweep, samples_per_quarter)
            out.update({"points": pts, "tangent_in": t_in, "tangent_out": t_out,
                        "centre": centre, "radius": radius,
                        "sweep_deg": math.degrees(sweep), "direction": seg["direction"],
                        "mid_point": mid_point, "mid_tangent": mid_tangent})
        else:  # BEZIER
            pts, t_in, t_out = _sample_bezier(
                a, tuple(seg["handle_from"]), tuple(seg["handle_to"]), b,
                samples_per_quarter, where, problems)
            out.update({"points": pts, "tangent_in": t_in, "tangent_out": t_out})
        compiled.segments.append(out)

    if problems:
        raise SpecError(problems)

    # --- joins ----------------------------------------------------------------
    n_seg = len(compiled.segments)
    for i, seg in enumerate(compiled.segments):
        prev = compiled.segments[(i - 1) % n_seg]
        st_name = seg["from"]
        st = stations[st_name]
        t_in = prev["tangent_out"]
        t_out = seg["tangent_in"]
        deviation = _angle_between_deg(t_in, t_out)
        if st["join"] == "SMOOTH" and deviation > tol_ang:
            problems.append(
                f"stations.{st_name}: declared SMOOTH but tangents disagree by "
                f"{deviation:.2f} deg ({prev['name']} -> {seg['name']}, tolerance {tol_ang} deg)")
        if st["join"] == "HARD" and deviation <= tol_ang:
            warnings.append(
                f"stations.{st_name}: declared HARD but tangents are continuous "
                f"({deviation:.2f} deg) - confirm this corner is real")
        compiled.stations[st_name] = {
            "at": tuple(st["at"]), "join": st["join"], "evidence": st["evidence"],
            "tangent_in": t_in, "tangent_out": t_out,
            "source_px": tuple(st["source_px"]) if "source_px" in st else None,
            "join_deviation_deg": deviation,
        }

    # --- closed outline -------------------------------------------------------
    outline = []
    for seg in compiled.segments:
        outline.extend(seg["points"][:-1])  # each segment's last point is the next one's first
    compiled.outline = outline

    compiled.signed_area = _signed_area(outline)
    if abs(compiled.signed_area) <= tol_dist * tol_dist:
        problems.append(
            f"outline encloses (near-)zero area ({compiled.signed_area:.6g}) - "
            "winding and solid side are undefined for a degenerate outline")
    compiled.winding = "COUNTERCLOCKWISE" if compiled.signed_area > 0 else "CLOCKWISE"
    xs = [p[0] for p in outline]
    ys = [p[1] for p in outline]
    compiled.extents = {"width": max(xs) - min(xs), "depth": max(ys) - min(ys)}

    interior_side = "LEFT" if compiled.winding == "COUNTERCLOCKWISE" else "RIGHT"
    if spec["solid_side"] != interior_side:
        problems.append(
            f"solid_side: declared {spec['solid_side']} but the outline winds "
            f"{compiled.winding}, which puts the interior on the {interior_side} "
            "of travel - the declaration contradicts the geometry")

    _check_simple_polygon(outline, problems)
    compiled.source_px_mapped = _check_source_px(spec, problems)

    # --- moulded chains -------------------------------------------------------
    runs = []
    current = None
    for seg in compiled.segments:
        if seg["role"] == "MOULDED":
            if current is None:
                current = list(seg["points"])
            else:
                current.extend(seg["points"][1:])
        else:
            if current is not None:
                runs.append(current)
                current = None
    if current is not None:
        # A run may wrap through the loop closure into the first run.
        if runs and compiled.segments[0]["role"] == "MOULDED":
            runs[0] = current[:-1] + runs[0]
        elif current[0] == current[-1]:
            # every segment is MOULDED: the chain IS the closed outline -
            # return it in outline convention, first point not repeated
            runs.append(current[:-1])
        else:
            runs.append(current)
    compiled.moulded_chains = runs

    # --- datum + normalization ------------------------------------------------
    compiled.datum_value = spec["parameters"][spec["datum"]]["value"]
    if compiled.datum_value <= 0:
        problems.append(f"datum: parameter '{spec['datum']}' must be positive")
    else:
        compiled.normalized_stations = {
            name: (st["at"][0] / compiled.datum_value, st["at"][1] / compiled.datum_value)
            for name, st in compiled.stations.items()}

    _check_claims(spec, compiled, problems)

    if problems:
        raise SpecError(problems)

    compiled.warnings = warnings
    return compiled


def to_world(point, spec):
    """Profile (x, y) -> Blender world (X, Y, Z) in metres, construction plane XZ."""
    s = spec["construction_plane"]["scale_m_per_unit"]
    return (point[0] * s, 0.0, point[1] * s)


def moulded_chain_points(spec, samples_per_quarter=24):
    """Production entry point: the moulded profile runs, compiled and validated.

    Asset scripts consume THIS. They do not recreate profile geometry.

    Conventions: each chain is an OPEN polyline with distinct endpoints. The
    one exception is a profile whose every segment is MOULDED - then the
    single chain is the closed outline itself, in outline convention (first
    point not repeated at the end).
    """
    return compile_profile(spec, samples_per_quarter).moulded_chains
