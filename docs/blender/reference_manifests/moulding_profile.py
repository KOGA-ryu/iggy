"""Medieval moulding profiles as parametric curves — the lane's first geometry output.

WHY PARAMETRIC AND NOT TRACED. Paley states that these profiles are compass-and-square
constructions: hollows are arcs of stated fractions of a circle "accurately formed with the
compasses", chamfers are "generally (not invariably)" 45 degrees, and every figure is drawn in
one fixed frame - outer wall-line parallel to the bottom of the page, soffit parallel to the side.
So the profile's authored form IS a chain of circular arcs and straight runs. Tracing pixels off a
hatched 1845 engraving would produce a noisy polyline that is strictly worse than reconstructing
the construction that made it. (There is also no numpy/PIL/cv2 on this machine, but that is not
the reason - it is the second reason.)

FRAME. Paley states the convention outright, and cycle 28 found that the note previously here had
his two axes SWAPPED. His words: lay the stone so that "the wall-line, or part of the stone which
lies in the plane of the outer wall, should be parallel with the end of the paper nearest to you,
and the soffit, or inner surface at right angles with it, parallel to one side"; and in copying,
"draw the outer wall-line parallel with the bottom of the page, and the soffit parallel to the
side." So:
    the WALL-LINE is HORIZONTAL - parallel with the bottom of the page
    the SOFFIT is VERTICAL      - parallel with the side
    +x runs along the wall-line, away from the soffit
    +y runs along the soffit, away from the wall-line
    origin = the arris where the soffit plane meets the wall plane
    units  = inches, because that is what his dimension figures print

CHAIN ORDER IS WALL-LINE FIRST, not soffit-first. Paley again: "a pencil were to be carried along
the wall-line first, and afterwards in and out of each cavity and round each projection." The note
previously here claimed soffit-first "because that is the order a mason works and the order Paley
lists". He lists the opposite.

WHAT THE HATCHING MEANS. Not decoration and not solid stone generally: the figures are shaded "on
the part which represents the level surface of the flat side of the stone" - the sawn face. So the
hatched region IS the cut section, and its boundary, minus the wall-line and soffit segments where
the saw passed, IS the moulded edge. A section may therefore carry moulded edge along more than
one stretch of its outline; those stretches are one continuous edge, not rival candidates.

THREE PLANES, his terms: the wall-plane; the soffit-plane at right angles to it; and the
chamfer-plane, "generally (not invariably) done at an angle of forty-five degrees". Measured
chamfers on Plate I fig 6 are 44.2 and 40.9 degrees - the hedge is load-bearing. See fig. 10,
Plate 2, where he labels a = chamfer-plane, n = soffit-plane, c = wall-plane.

MEMBER PRIMITIVES (the observed vocabulary - see docs/blender/architecture_books_findings §3):
    fillet   flat run, square to the previous member
    chamfer  flat run at an angle, default 45 degrees
    roll     convex circular arc (bowtell); `sweep` is the fraction of a full circle exposed
    hollow   concave circular arc (cavetto); `sweep` likewise
    ogee     an S: convex arc then concave arc, radii independently settable
    quirk    a narrow deep nick, drawn as two short flats
    step     a receding order: out along x, then down along y

STOCK. Paley draws a dashed rectangle behind most figures: the uncut block the section was cut
from. `stock` records it, so a generator can model stone removal rather than extruding an outline
- which is what the section actually represents.
"""
import json
import math

# ---------------------------------------------------------------- geometry

def _arc(cx, cy, r, a0, a1, n):
    """Sample a circular arc from a0 to a1 radians (signed), n segments."""
    return [(cx + r * math.cos(a0 + (a1 - a0) * i / n),
             cy + r * math.sin(a0 + (a1 - a0) * i / n)) for i in range(n + 1)]


def build(profile, seg=24):
    """Profile dict -> list of (x, y) points, in inches, soffit-first.

    Pure Python, no dependencies. `seg` is arc segments per quarter-turn.
    """
    pts = [(0.0, 0.0)]
    hd = 0.0  # heading in radians; 0 = +x (outward along the wall face)
    for m in profile["members"]:
        x, y = pts[-1]
        t = m["type"]
        if t in ("fillet", "chamfer"):
            # Paley states chamfers are "generally (not invariably)" 45 degrees. That is an
            # angle to the DRAWING FRAME - to the wall face - not to the previous member's
            # tangent. So a straight member may declare `absolute: true` and set its heading
            # outright, discarding whatever rotation the preceding arcs accumulated. Chaining
            # a flat off a 240-degree bowtell's exit tangent is meaningless; chaining it off
            # the wall face is what the engraver actually drew.
            a = math.radians(m.get("angle", 0.0 if t == "fillet" else -45.0))
            ang = a if m.get("absolute", False) else hd + a
            L = m["length"]
            pts.append((x + L * math.cos(ang), y + L * math.sin(ang)))
            if m.get("absolute", False) or m.get("turn", False):
                hd = ang
        elif t in ("roll", "hollow"):
            r = m["radius"]
            # COMPASS FORM. Paley's arcs are struck with the compasses: the engraver set a
            # centre and swept between two angles. For an ordinary member the tangent-chained
            # form below is equivalent and easier to author. But a near-full-round bowtell is
            # attached at a NECK - the outline enters and leaves at nearly the same place - and
            # then `sweep` alone cannot say where it leaves, so the next member lands wrong.
            # There the drawn fact is the centre, so let a member give it outright.
            # Same lesson as `absolute` on straight members: where the plate states a position,
            # take the position, don't derive it from an accumulated heading. (Cycle 24.)
            if "centre" in m:
                cx, cy = m["centre"]
                a0 = math.radians(m["a_start"])
                a1 = math.radians(m["a_end"])
                n = max(4, int(seg * abs(a1 - a0) / (math.pi / 2)))
                pts += _arc(cx, cy, r, a0, a1, n)
                hd = a1 + (-math.pi / 2 if t == "roll" else math.pi / 2)
                continue
            sweep = m.get("sweep", 0.5) * 2 * math.pi
            sign = 1.0 if t == "roll" else -1.0
            # Centre sits perpendicular to the heading, on the side the curve bends TOWARD:
            # a convex roll traversed soffit-first bulges away from its centre, so the centre
            # is to the RIGHT of the heading (hd - 90); a concave hollow cups toward its
            # centre, so the centre is to the LEFT (hd + 90). Hence `hd - sign*90`.
            # Getting this backwards puts every arc on the wrong side, so the chain doubles
            # back on itself and the profile builds far short of its stock. (Cycle 24.)
            cx = x + r * math.cos(hd - sign * math.pi / 2)
            cy = y + r * math.sin(hd - sign * math.pi / 2)
            a0 = math.atan2(y - cy, x - cx)
            n = max(4, int(seg * sweep / (math.pi / 2)))
            pts += _arc(cx, cy, r, a0, a0 + sign * -sweep, n)[1:]
            hd -= sign * sweep
        elif t == "ogee":
            for sub, rad in (("roll", m["radius_out"]), ("hollow", m["radius_in"])):
                x, y = pts[-1]
                sign = 1.0 if sub == "roll" else -1.0
                sweep = m.get("sweep", 0.25) * 2 * math.pi
                cx = x + rad * math.cos(hd - sign * math.pi / 2)
                cy = y + rad * math.sin(hd - sign * math.pi / 2)
                a0 = math.atan2(y - cy, x - cx)
                n = max(4, int(seg * sweep / (math.pi / 2)))
                pts += _arc(cx, cy, rad, a0, a0 + sign * -sweep, n)[1:]
                hd -= sign * sweep
        elif t == "quirk":
            w, d = m.get("width", 0.15), m["depth"]
            pts += [(x, y - d), (x + w, y - d), (x + w, y)]
        elif t == "step":
            pts += [(x + m["out"], y), (x + m["out"], y - m["down"])]
        else:
            raise ValueError("unknown member type: " + t)
    return pts


def to_svg(profile, path, scale=28.0, pad=14):
    """Write the profile as an SVG so a reconstruction can be eyeballed against the plate."""
    pts = build(profile)
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    st = profile.get("stock")
    if st:
        xs += [0, st["width"]]
        ys += [0, -st["height"]]
    w = (max(xs) - min(xs)) * scale + 2 * pad
    h = (max(ys) - min(ys)) * scale + 2 * pad
    def X(v): return (v - min(xs)) * scale + pad
    def Y(v): return h - ((v - min(ys)) * scale + pad)
    body = []
    if st:
        body.append(f'<rect x="{X(0):.1f}" y="{Y(0):.1f}" width="{st["width"]*scale:.1f}" '
                    f'height="{st["height"]*scale:.1f}" fill="none" stroke="#999" '
                    f'stroke-dasharray="5 4"/>')
    d = "M " + " L ".join(f"{X(x):.2f},{Y(y):.2f}" for x, y in pts)
    body.append(f'<path d="{d}" fill="none" stroke="#000" stroke-width="2"/>')
    open(path, "w").write(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w:.0f}" height="{h:.0f}" '
        f'viewBox="0 0 {w:.0f} {h:.0f}"><rect width="100%" height="100%" fill="#fff"/>'
        + "".join(body) +
        f'<text x="{pad}" y="{h-4:.0f}" font="11px sans-serif" font-size="11" fill="#444">'
        f'{profile.get("id","")} — {profile.get("source","")}</text></svg>')
    return path


def load(path):
    return json.load(open(path))


def extent(profile):
    """(width, depth) of the finished section in inches - for checking against printed dims."""
    pts = build(profile)
    return (max(p[0] for p in pts) - min(p[0] for p in pts),
            max(p[1] for p in pts) - min(p[1] for p in pts))
