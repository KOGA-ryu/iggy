"""Author fixtures/worlds/framing_bay.iggy3d.save (ASSET-BLD-2).

Deterministic committed fixture author (same envelope technique as
facade_slice): a timber-framing bay proving the BLD-2 tranche — plinth run,
two posts carrying a beam with braces and an end cap, the double door with
both mirrored leaves socketed, wide/tall window frames, iron bars socketed
into the standard frame's mullion receiver, and the chained column
(base -> column -> cap, a two-deep socket parent chain).

Engine space is glTF Y-up; asset-local socket offsets converted from Blender
via (x, y, z)_blender -> (x, z, -y)_engine.

Run: python3 assets/creative/calibration/tools/make_framing_bay_fixture.py
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, *[os.pardir] * 4))
BAY = os.path.join(ROOT, "fixtures", "worlds", "calibration_bay.iggy3d.save")
OUT = os.path.join(ROOT, "fixtures", "worlds", "framing_bay.iggy3d.save")


def prop(name, asset, pos, bmin, bmax, parent=None, socket=None):
    return dict(name=name, kind="Prop", asset=asset, pos=pos,
                bmin=bmin, bmax=bmax, parent=parent, socket=socket)


def offset(base, delta):
    return tuple(round(b + d, 6) for b, d in zip(base, delta))


def bounds(pos, nmin, nmax):
    return offset(pos, nmin), offset(pos, nmax)


OBJECTS = []


def add(entry):
    OBJECTS.append(entry)
    return len(OBJECTS) - 1


add(dict(name="Framing Ground", kind="Floor", asset=None, pos=(0, -0.5, 0),
         bmin=(-10, -0.5, -10), bmax=(10, 0, 10), parent=None, socket=None))

p = (1.5, 0, 1.8)
add(prop("Scale Gauge", "calibration/human_gauge_1p8m", p,
         *bounds(p, (-0.25, 0, -0.15), (0.25, 1.8, 0.15))))

# base course behind the frame line
p = (0, 0, 0.25)
add(prop("Plinth Run", "architecture/structural/foundation_plinth_straight_4m",
         p, *bounds(p, (-2.0, 0, -0.15), (2.0, 0.5, 0.15))))
p = (2.0, 0, 0.25)
add(prop("Plinth End", "architecture/structural/foundation_plinth_end", p,
         *bounds(p, (0.0, 0, -0.15), (0.45, 0.5, 0.15))))

# post-and-beam frame with braces
for px, tag in ((-2.2, "West"), (2.2, "East")):
    p = (px, 0, 0)
    add(prop("Post %s" % tag, "architecture/structural/post_square_0p3x3m",
             p, *bounds(p, (-0.15, 0, -0.15), (0.15, 3.0, 0.15))))
p = (0, 3.0, 0)
add(prop("Frame Beam", "architecture/structural/beam_4m", p,
         *bounds(p, (-2.0, 0, -0.1), (2.0, 0.3, 0.1))))
p = (2.0, 3.0, 0)
add(prop("Beam End Cap", "architecture/structural/beam_end_cap", p,
         *bounds(p, (0.0, 0, -0.13), (0.24, 0.36, 0.13))))
p = (-2.05, 2.15, 0)
add(prop("Brace West", "architecture/structural/brace_left", p,
         *bounds(p, (0.0, 0, -0.06), (0.8, 0.85, 0.06))))
p = (2.05, 2.15, 0)
add(prop("Brace East", "architecture/structural/brace_right", p,
         *bounds(p, (-0.8, 0, -0.06), (0.0, 0.85, 0.06))))

# double door with both mirrored leaves on their named receivers
p = (0, 0, 0)
frame = add(prop("Double Door Frame",
                 "architecture/openings/door_frame_double", p,
                 *bounds(p, (-0.95, 0, -0.075), (0.95, 2.25, 0.075))))
p = (-0.9, 0, 0)
add(prop("Double Leaf Left",
         "architecture/openings/door_leaf_double_left_closed", p,
         *bounds(p, (0.0, 0, -0.025), (0.9, 2.1, 0.025)),
         parent=frame, socket="hinge.left"))
p = (0.9, 0, 0)
add(prop("Double Leaf Right",
         "architecture/openings/door_leaf_double_right_closed", p,
         *bounds(p, (-0.9, 0, -0.025), (0.0, 2.1, 0.025)),
         parent=frame, socket="hinge.right"))

# window completion: wide + tall frames, bars in the standard frame
p = (-4.5, 1.0, 0)
add(prop("Window Wide", "architecture/openings/window_frame_wide", p,
         *bounds(p, (-0.78, -0.08, -0.075), (0.78, 1.28, 0.075))))
p = (4.5, 0.6, 0)
add(prop("Window Tall", "architecture/openings/window_frame_tall", p,
         *bounds(p, (-0.48, -0.08, -0.075), (0.48, 1.88, 0.075))))
p = (-7.0, 1.0, 0)
std = add(prop("Window Barred", "architecture/openings/window_frame_standard",
               p, *bounds(p, (-0.48, -0.08, -0.075), (0.48, 1.28, 0.075))))
p = (-7.0, 1.0, 0)
add(prop("Window Bars", "architecture/openings/window_bars_standard", p,
         *bounds(p, (-0.4, 0, -0.02), (0.4, 1.2, 0.02)),
         parent=std, socket="mullion"))

# chained column: base -> column -> cap (two-deep socket parent chain)
p = (7.0, 0, 0)
base = add(prop("Column Base", "architecture/structural/column_round_base",
                p, *bounds(p, (-0.3, 0, -0.3), (0.3, 0.25, 0.3))))
p = (7.0, 0.25, 0)
col = add(prop("Column", "architecture/structural/column_round_0p5x3m", p,
               *bounds(p, (-0.25, 0, -0.25), (0.25, 3.0, 0.25)),
               parent=base, socket="column.base"))
p = (7.0, 3.25, 0)
add(prop("Column Cap", "architecture/structural/column_round_cap", p,
         *bounds(p, (-0.3, 0, -0.3), (0.3, 0.25, 0.3)),
         parent=col, socket="column.cap"))


def fmt3(v):
    def f(x):
        s = ("%.6f" % x).rstrip("0").rstrip(".")
        return s if s not in ("-0",) else "0"
    return ",".join(f(x) for x in v)


def main():
    with open(BAY) as fh:
        text = fh.read()

    lines = [ln for ln in text.splitlines()
             if not ln.startswith("creativeDocument.object.")]

    out = []
    for ln in lines:
        if ln.startswith("metadata.saveId="):
            ln = "metadata.saveId=framing_bay"
        elif ln.startswith("metadata.worldTitle="):
            ln = "metadata.worldTitle=Framing Bay"
        elif ln.startswith("metadata.saveTitle="):
            ln = "metadata.saveTitle=Framing Bay"
        elif ln.startswith("creativeDocument.name="):
            ln = "creativeDocument.name=Framing Bay"
        elif ln.startswith("creativeDocument.nextObjectId="):
            ln = "creativeDocument.nextObjectId=%d" % (len(OBJECTS) + 1)
        out.append(ln)
        if ln.startswith("creativeDocument.nextObjectId="):
            out.append("creativeDocument.object.count=%d" % len(OBJECTS))
            for i, ob in enumerate(OBJECTS):
                pre = "creativeDocument.object.%d." % i
                out.append(pre + "id=%d" % (i + 1))
                out.append(pre + "kind=" + ob["kind"])
                out.append(pre + "name=" + ob["name"])
                if ob["asset"]:
                    out.append(pre + "assetId=" + ob["asset"])
                out.append(pre + "transform.position=%s" % fmt3(ob["pos"]))
                out.append(pre + "transform.rotation=0,0,0")
                out.append(pre + "transform.scale=1,1,1")
                out.append(pre + "bounds.min=%s" % fmt3(ob["bmin"]))
                out.append(pre + "bounds.max=%s" % fmt3(ob["bmax"]))
                out.append(pre + "layerId=0")
                out.append(pre + "visible=true")
                out.append(pre + "locked=false")
                if ob["parent"] is not None:
                    out.append(pre + "hasParent=true")
                    out.append(pre + "parentId=%d" % (ob["parent"] + 1))
                    out.append(pre + "attachmentSocket=" + ob["socket"])
                else:
                    out.append(pre + "hasParent=false")
                    out.append(pre + "parentId=0")
                out.append(pre + "tag.count=0")

    with open(OUT, "w") as fh:
        fh.write("\n".join(out) + "\n")
    print("WROTE", OUT, "(%d objects)" % len(OBJECTS))


if __name__ == "__main__":
    main()
