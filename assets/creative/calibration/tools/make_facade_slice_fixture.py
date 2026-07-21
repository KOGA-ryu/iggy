"""Author fixtures/worlds/facade_slice.iggy3d.save (ASSET-BLD-1 phase 4).

Deterministic, committed fixture author: reuses the calibration_bay save as
the envelope template (same schema the app writes) and replaces the document
identity and object list with the facade-slice composition required by the
work order: wide door with leaf, two standard windows with shutters, sills,
lintels and mullions, the eave/gutter/downspout/fascia line, and the chimney
stack + cap — socketed children attach through parentId + attachmentSocket
exactly like the bay fixture.

Engine space is glTF Y-up. Asset-local socket offsets below are the
generator's Blender socket locations converted via (x, y, z)_blender ->
(x, z, -y)_engine.

Run: python3 assets/creative/calibration/tools/make_facade_slice_fixture.py
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, *[os.pardir] * 4))
BAY = os.path.join(ROOT, "fixtures", "worlds", "calibration_bay.iggy3d.save")
OUT = os.path.join(ROOT, "fixtures", "worlds", "facade_slice.iggy3d.save")

# (name, kind, assetId, position, bounds_min, bounds_max, parent_index,
#  attachment_socket) — parent_index refers into this list (None = unparented)
G = None  # ground uses explicit bounds; props get bounds from nominal + pos


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


# ground slab (kind Floor, like the bay)
add(dict(name="Facade Ground", kind="Floor", asset=None, pos=(0, -0.5, 0),
         bmin=(-10, -0.5, -10), bmax=(10, 0, 10), parent=None, socket=None))

# scale gauge (Law 6)
p = (1.8, 0, 1.5)
add(prop("Scale Gauge", "calibration/human_gauge_1p8m", p,
         *bounds(p, (-0.25, 0, -0.15), (0.25, 1.8, 0.15))))

# wide door + leaf on the hinge receiver (frame local hinge.left at
# blender (-0.6, 0, 0) -> engine (-0.6, 0, 0))
p = (0, 0, 0)
door = add(prop("Wide Door Frame", "architecture/openings/door_frame_wide", p,
                *bounds(p, (-0.65, 0, -0.075), (0.65, 2.25, 0.075))))
p = (-0.6, 0, 0)
add(prop("Wide Door Leaf", "architecture/openings/door_leaf_wide_closed", p,
         *bounds(p, (0.0, 0, -0.025), (1.2, 2.1, 0.025)),
         parent=door, socket="hinge.left"))

# window 1 (with shutters + mullion); frame origin at clear-opening bottom
# center, placed at sill height 1.0. Shutter receivers at blender
# (-/+0.4, -0.075, 0) -> engine (-/+0.4, 0, 0.075); mullion at origin.
for wx, tag, with_shutters in ((-3.0, "West", True), (3.0, "East", False)):
    p = (wx, 1.0, 0)
    win = add(prop("Window %s" % tag,
                   "architecture/openings/window_frame_standard", p,
                   *bounds(p, (-0.48, -0.08, -0.075), (0.48, 1.28, 0.075))))
    if with_shutters:
        p = (wx - 0.4, 1.0, 0.075)
        add(prop("Window %s Shutter L" % tag,
                 "architecture/openings/window_shutter_left_closed", p,
                 *bounds(p, (0.0, 0, -0.02), (0.4, 1.2, 0.02)),
                 parent=win, socket="shutter.left"))
        p = (wx + 0.4, 1.0, 0.075)
        add(prop("Window %s Shutter R" % tag,
                 "architecture/openings/window_shutter_right_closed", p,
                 *bounds(p, (-0.4, 0, -0.02), (0.0, 1.2, 0.02)),
                 parent=win, socket="shutter.right"))
    p = (wx, 1.0, 0)
    add(prop("Window %s Mullion" % tag,
             "architecture/openings/window_mullion_cross", p,
             *bounds(p, (-0.4, 0, -0.025), (0.4, 1.2, 0.025)),
             parent=win, socket="mullion"))
    p = (wx, 0.82, 0.05)
    add(prop("Window %s Sill" % tag,
             "architecture/openings/window_sill_standard", p,
             *bounds(p, (-0.55, 0, -0.125), (0.55, 0.1, 0.125))))
    p = (wx, 2.28, 0.05)
    add(prop("Window %s Lintel" % tag,
             "architecture/openings/window_lintel_standard", p,
             *bounds(p, (-0.6, 0, -0.1), (0.6, 0.12, 0.1))))

# fascia line: 4m eave trim, outer corner, finished end
p = (0, 2.9, 0.2)
add(prop("Eave Trim", "architecture/roof/eave_trim_4m", p,
         *bounds(p, (-2.0, 0, -0.17), (2.0, 0.3, 0.03))))
p = (2.0, 2.9, 0.2)
add(prop("Eave Outer Corner", "architecture/roof/eave_outer_corner", p,
         *bounds(p, (-0.03, 0, -0.6), (0.6, 0.3, 0.03))))
p = (-2.3, 2.9, 0.2)
add(prop("Fascia End", "architecture/roof/fascia_end", p,
         *bounds(p, (-0.05, 0, -0.05), (0.3, 0.3, 0.05))))

# drainage: gutter above the fascia, downspout to the ground outlet
p = (0, 3.25, 0.38)
add(prop("Gutter", "architecture/roof/gutter_straight_2m", p,
         *bounds(p, (-1.0, 0, -0.09), (1.0, 0.14, 0.09))))
p = (1.4, 0.5, 0.38)
add(prop("Downspout", "architecture/roof/downspout_3m", p,
         *bounds(p, (-0.05, 0, -0.05), (0.05, 3.0, 0.05))))
p = (1.4, 0, 0.38)
add(prop("Downspout Outlet", "architecture/roof/downspout_outlet", p,
         *bounds(p, (-0.05, 0, -0.05), (0.05, 0.5, 0.3))))

# chimney with socketed cap (stack receiver at engine (0, 2.2, 0))
p = (-5.0, 0, -0.8)
stack = add(prop("Chimney Stack", "architecture/roof/chimney_stack_short", p,
                 *bounds(p, (-0.475, 0, -0.475), (0.475, 2.2, 0.475))))
p = (-5.0, 2.2, -0.8)
add(prop("Chimney Cap", "architecture/roof/chimney_cap", p,
         *bounds(p, (-0.45, 0, -0.45), (0.45, 0.33, 0.45)),
         parent=stack, socket="cap"))


def main():
    with open(BAY) as fh:
        text = fh.read()

    # keep the envelope; drop the bay's object block entirely
    lines = [ln for ln in text.splitlines()
             if not ln.startswith("creativeDocument.object.")]

    out = []
    for ln in lines:
        if ln.startswith("metadata.saveId="):
            ln = "metadata.saveId=facade_slice"
        elif ln.startswith("metadata.worldTitle="):
            ln = "metadata.worldTitle=Facade Slice"
        elif ln.startswith("metadata.saveTitle="):
            ln = "metadata.saveTitle=Facade Slice"
        elif ln.startswith("creativeDocument.name="):
            ln = "creativeDocument.name=Facade Slice"
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


def fmt3(v):
    def f(x):
        s = ("%.6f" % x).rstrip("0").rstrip(".")
        return s if s not in ("-0",) else "0"
    return ",".join(f(x) for x in v)


if __name__ == "__main__":
    main()
