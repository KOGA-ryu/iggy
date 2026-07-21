"""Author fixtures/worlds/two_storey_estate.iggy3d.save (ASSET-BLD-3).

The Batch 1 acceptance composition (backlog proof map `proof/two_storey_estate`:
"Exterior facade, per-floor partitions, stairs, roof, balconies"), authored
through the same envelope technique and placement records as the other
fixture proofs. Two storeys at 0 / 3.0 / roofline 6.0; exterior faces +Z.

Ground floor: plinth course with end + quoined corner, buttressed ends,
plastered wall piers partitioning the double door, a barred window with
sill/lintel and a wide window; porch arch flanked by two full column chains.
Vertical: quarter-turn stair to a 2x4 landing edged with the small railing
family. Upper floor: wide door opening onto a bracketed balcony deck,
tall + shuttered windows, upper piers. Roofline: parapet run with end and
corner, rear gable + ridge caps, chimney with socketed cap.

Run: python3 assets/creative/calibration/tools/make_two_storey_estate_fixture.py
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, *[os.pardir] * 4))
BAY = os.path.join(ROOT, "fixtures", "worlds", "calibration_bay.iggy3d.save")
OUT = os.path.join(ROOT, "fixtures", "worlds",
                   "two_storey_estate.iggy3d.save")


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


add(dict(name="Estate Ground", kind="Floor", asset=None, pos=(0, -0.5, 0),
         bmin=(-12, -0.5, -12), bmax=(12, 0, 12), parent=None, socket=None))
p = (2.6, 0, 2.2)
add(prop("Scale Gauge", "calibration/human_gauge_1p8m", p,
         *bounds(p, (-0.25, 0, -0.15), (0.25, 1.8, 0.15))))

# ---- ground-floor facade line (z = 0, exterior +Z) ----
for px, nm in ((-2.0, "West"), (2.0, "East")):
    p = (px, 0, 0.3)
    add(prop("Plinth %s" % nm,
             "architecture/structural/foundation_plinth_straight_4m", p,
             *bounds(p, (-2.0, 0, -0.15), (2.0, 0.5, 0.15))))
p = (-4.45, 0, 0.3)
add(prop("Plinth End", "architecture/structural/foundation_plinth_end", p,
         *bounds(p, (0.0, 0, -0.15), (0.45, 0.5, 0.15))))
p = (4.0, 0, 0.3)
add(prop("Plinth Corner",
         "architecture/structural/foundation_plinth_outer_corner", p,
         *bounds(p, (-0.15, 0, -0.5), (0.5, 0.55, 0.15))))
for px, nm in ((-4.75, "West"), (4.75, "East")):
    p = (px, 0, 0.12)
    add(prop("Buttress %s" % nm, "architecture/structural/buttress_low", p,
             *bounds(p, (-0.25, 0, 0.0), (0.25, 3.0, 0.4))))
p = (-2.5, 0, 0)
add(prop("Pier Ground Wide", "architecture/structural/wall_pier_1x3m", p,
         *bounds(p, (-0.5, 0, -0.15), (0.5, 3.0, 0.15))))
p = (2.05, 0, 0)
add(prop("Pier Ground Narrow", "architecture/structural/wall_pier_0p5x3m",
         p, *bounds(p, (-0.25, 0, -0.15), (0.25, 3.0, 0.15))))

p = (0, 0, 0)
ddoor = add(prop("Estate Door Frame",
                 "architecture/openings/door_frame_double", p,
                 *bounds(p, (-0.95, 0, -0.075), (0.95, 2.25, 0.075))))
p = (-0.9, 0, 0)
add(prop("Estate Leaf Left",
         "architecture/openings/door_leaf_double_left_closed", p,
         *bounds(p, (0.0, 0, -0.025), (0.9, 2.1, 0.025)),
         parent=ddoor, socket="hinge.left"))
p = (0.9, 0, 0)
add(prop("Estate Leaf Right",
         "architecture/openings/door_leaf_double_right_closed", p,
         *bounds(p, (-0.9, 0, -0.025), (0.0, 2.1, 0.025)),
         parent=ddoor, socket="hinge.right"))

p = (-3.5, 1.0, 0)
barred = add(prop("Window Ground Barred",
                  "architecture/openings/window_frame_standard", p,
                  *bounds(p, (-0.48, -0.08, -0.075), (0.48, 1.28, 0.075))))
p = (-3.5, 1.0, 0)
add(prop("Ground Bars", "architecture/openings/window_bars_standard", p,
         *bounds(p, (-0.4, 0, -0.02), (0.4, 1.2, 0.02)),
         parent=barred, socket="mullion"))
p = (-3.5, 0.82, 0.05)
add(prop("Ground Sill", "architecture/openings/window_sill_standard", p,
         *bounds(p, (-0.55, 0, -0.125), (0.55, 0.1, 0.125))))
p = (-3.5, 2.28, 0.05)
add(prop("Ground Lintel", "architecture/openings/window_lintel_standard", p,
         *bounds(p, (-0.6, 0, -0.1), (0.6, 0.12, 0.1))))
p = (3.3, 1.0, 0)
add(prop("Window Ground Wide", "architecture/openings/window_frame_wide", p,
         *bounds(p, (-0.78, -0.08, -0.075), (0.78, 1.28, 0.075))))

# ---- porch: arch flanked by two full column chains ----
p = (0, 0, 1.6)
add(prop("Porch Arch", "architecture/structural/arch_2m", p,
         *bounds(p, (-1.15, 0, -0.075), (1.15, 2.28, 0.075))))
for px, nm in ((-1.8, "West"), (1.8, "East")):
    p = (px, 0, 1.6)
    base = add(prop("Column Base %s" % nm,
                    "architecture/structural/column_round_base", p,
                    *bounds(p, (-0.3, 0, -0.3), (0.3, 0.25, 0.3))))
    p = (px, 0.25, 1.6)
    col = add(prop("Column %s" % nm,
                   "architecture/structural/column_round_0p5x3m", p,
                   *bounds(p, (-0.25, 0, -0.25), (0.25, 3.0, 0.25)),
                   parent=base, socket="column.base"))
    p = (px, 3.25, 1.6)
    add(prop("Column Cap %s" % nm,
             "architecture/structural/column_round_cap", p,
             *bounds(p, (-0.3, 0, -0.3), (0.3, 0.25, 0.3)),
             parent=col, socket="column.cap"))

# ---- vertical circulation: quarter-turn stair to the upper landing ----
p = (-2.0, 0, -2.5)
add(prop("Estate Stair", "architecture/traversal/stair_quarter_turn_3m", p,
         *bounds(p, (-0.5, 0, -1.75), (2.5, 3.0, 1.0))))
p = (2.5, 3.0, -2.5)
add(prop("Upper Landing", "architecture/traversal/stair_landing_2x4m", p,
         *bounds(p, (-2.0, 0, -1.0), (2.0, 0.2, 1.0))))
p = (0.54, 3.2, -1.54)
add(prop("Landing Rail Corner", "architecture/structural/railing_corner", p,
         *bounds(p, (-0.04, 0, -0.5), (0.5, 1.0, 0.04))))
p = (1.6, 3.2, -1.54)
add(prop("Landing Rail Run", "architecture/structural/railing_straight_1m",
         p, *bounds(p, (-0.5, 0, -0.04), (0.5, 1.0, 0.04))))
p = (2.2, 3.2, -1.54)
add(prop("Landing Rail End", "architecture/structural/railing_end_post", p,
         *bounds(p, (-0.055, 0, -0.055), (0.055, 1.05, 0.055))))

# ---- upper floor: balcony door, windows, partitions ----
p = (0, 3.0, 0)
udoor = add(prop("Balcony Door Frame",
                 "architecture/openings/door_frame_wide", p,
                 *bounds(p, (-0.65, 0, -0.075), (0.65, 2.25, 0.075))))
p = (-0.6, 3.0, 0)
add(prop("Balcony Door Leaf",
         "architecture/openings/door_leaf_wide_closed", p,
         *bounds(p, (0.0, 0, -0.025), (1.2, 2.1, 0.025)),
         parent=udoor, socket="hinge.left"))
p = (0, 2.85, 0.08)
add(prop("Balcony Deck", "architecture/structural/balcony_deck_2x1p5m", p,
         *bounds(p, (-1.0, -0.12, 0.0), (1.0, 0.15, 1.5))))
for px in (-0.7, 0.7):
    p = (px, 2.85, 0.08)
    add(prop("Balcony Bracket %+.1f" % px,
             "architecture/structural/balcony_bracket", p,
             *bounds(p, (-0.06, -0.55, 0.0), (0.06, 0.0, 0.5))))
p = (-3.3, 3.4, 0)
add(prop("Window Upper Tall", "architecture/openings/window_frame_tall", p,
         *bounds(p, (-0.48, -0.08, -0.075), (0.48, 1.88, 0.075))))
p = (3.3, 3.7, 0)
ushut = add(prop("Window Upper Shuttered",
                 "architecture/openings/window_frame_standard", p,
                 *bounds(p, (-0.48, -0.08, -0.075), (0.48, 1.28, 0.075))))
p = (2.9, 3.7, 0.075)
add(prop("Upper Shutter L",
         "architecture/openings/window_shutter_left_closed", p,
         *bounds(p, (0.0, 0, -0.02), (0.4, 1.2, 0.02)),
         parent=ushut, socket="shutter.left"))
p = (3.7, 3.7, 0.075)
add(prop("Upper Shutter R",
         "architecture/openings/window_shutter_right_closed", p,
         *bounds(p, (-0.4, 0, -0.02), (0.0, 1.2, 0.02)),
         parent=ushut, socket="shutter.right"))
for px in (-2.05, 2.05):
    p = (px, 3.0, 0)
    add(prop("Pier Upper %+.1f" % px,
             "architecture/structural/wall_pier_0p5x3m", p,
             *bounds(p, (-0.25, 0, -0.15), (0.25, 3.0, 0.15))))

# ---- roofline: parapet run, rear gable + ridge, chimney ----
for px, nm in ((-2.0, "West"), (2.0, "East")):
    p = (px, 6.0, 0.14)
    add(prop("Parapet %s" % nm,
             "architecture/structural/parapet_straight_4m", p,
             *bounds(p, (-2.0, 0, -0.14), (2.0, 0.88, 0.14))))
p = (-4.45, 6.0, 0.14)
add(prop("Parapet End", "architecture/structural/parapet_end", p,
         *bounds(p, (0.0, 0, -0.14), (0.45, 0.88, 0.14))))
p = (4.0, 6.0, 0.14)
add(prop("Parapet Corner", "architecture/structural/parapet_outer_corner",
         p, *bounds(p, (-0.14, 0, -0.5), (0.5, 0.95, 0.14))))
p = (0, 6.0, -4.2)
add(prop("Rear Gable", "architecture/roof/gable_cap_4m", p,
         *bounds(p, (-2.0, 0, -0.03), (2.0, 1.33470, 0.03))))
p = (0, 7.1547, -4.2)
add(prop("Rear Ridge", "architecture/roof/ridge_cap_straight_4m", p,
         *bounds(p, (-2.0, 0, -0.2), (2.0, 0.11547, 0.2))))
p = (-4.3, 6.0, -3.0)
stack = add(prop("Estate Chimney", "architecture/roof/chimney_stack_short",
                 p, *bounds(p, (-0.475, 0, -0.475), (0.475, 2.2, 0.475))))
p = (-4.3, 8.2, -3.0)
add(prop("Estate Chimney Cap", "architecture/roof/chimney_cap", p,
         *bounds(p, (-0.45, 0, -0.45), (0.45, 0.33, 0.45)),
         parent=stack, socket="cap"))


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
            ln = "metadata.saveId=two_storey_estate"
        elif ln.startswith("metadata.worldTitle="):
            ln = "metadata.worldTitle=Two Storey Estate"
        elif ln.startswith("metadata.saveTitle="):
            ln = "metadata.saveTitle=Two Storey Estate"
        elif ln.startswith("creativeDocument.name="):
            ln = "creativeDocument.name=Two Storey Estate"
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
