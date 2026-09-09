# P055 — Boolean Solids Lab

`boolean` is the math lab's 27th reusable object. Four presets connect editable
3D solids to set theory, Boolean logic, implicit functions and numerical
geometry. The existing object registry also makes the provider available to
authored document figures.

## Open and explore

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object boolean --level 0 --object-preset 0
```

| Preset | Construction | Useful changes |
| --- | --- | --- |
| Drilled block | Box minus a capped cylinder | Move, rotate or resize the cutter; switch to intersection to reveal the removed plug |
| Archway | Box minus an extruded arch opening | Change opening size and vertical position |
| Ball-and-socket | Box minus a sphere, with a separate matching-ball preview | Change cavity size, position and radial clearance; inspect the section |
| Blended stones | Smooth union of two spheres | Move the second sphere or adjust blend width, including zero for a sharp union |

A stays at the origin and has an editable shape and uniform size. B has its own
shape, size, position, yaw and pitch. The four primitive choices are a box,
sphere, capped cylinder and extruded arch opening. Teal identifies A; coral
identifies B and newly exposed subtraction walls. Blends mix those colours.
Pitch acts first, followed by yaw: `R_y(yaw) R_x(pitch)`.

**Solid view** selects the full solid or its section at the probe's z coordinate.
The section keeps `z <= probe z` and closes the visible cut with a gold face.
Full-solid field, truth and midpoint-volume values remain unchanged. The visible
mesh volume measures the tessellated section. **Shape only** hides construction
outlines and probes; an empty sampled view retains guides for recovery.

The matching ball appears beside the socket as a labelled preview. Its radius
is the spherical cutter radius minus **Ball radial clearance**. It is excluded
from the Boolean field and material-volume measurements; this is a geometric
fit illustration, not a collision or manufacturing-tolerance calculation.

| Layer | Linked exploration | Challenge |
| --- | --- | --- |
| 0: Inside and outside | Three defining-field curves through the probe; a three-state classification table | Locate removed material inside both inputs of a subtraction |
| 1: Sets and Boolean logic | Union, intersection or subtraction; all four truth-table rows and the current probe row | Locate a point retained by intersection |
| 2: Blends and surface normals | Positive blend width, analytic gradients and a surface-crossing normal | Locate newly blended material outside both inputs, with a regular crossing |
| 3: Sampling solids | 12–28 requested mesh cells per axis; midpoint-volume convergence and a highlighted sample cell | Make the 48/64 volume estimates agree within 3%, with at least 24 requested cells |

The nearest detected surface crossing is found on the x-directed line through
the probe. Its normal arrow appears only when the crossing has a nonzero,
well-defined gradient. A line with no detected sign crossing has no surface
marker. Dragging the field graph changes the same probe x parameter as the
sidebar. Levels and presets use the existing semantic action route.

## Mathematical contract

For primitive and composite fields, negative values mean interior, zero means
boundary and positive values mean exterior. Away from boundaries,

- Union uses `min(a,b)` and corresponds to `A OR B`.
- Intersection uses `max(a,b)` and corresponds to `A AND B`.
- Subtraction uses `max(a,-b)` and corresponds to `A AND NOT B`.

The truth table marks no binary input row within `1e-9` of either input boundary.
For smooth union it explicitly shows the hard-union baseline; a separate
readout detects material added outside both inputs. Meshes represent the sampled
negative region. Isolated zero-thickness contacts do not create material volume.

A unit box has half-extents `(1.2,1.1,0.7)`. A unit sphere has radius one.
A unit cylinder has radius one and half-length three along local z. The arch
opening is the union of a cylinder centred at local y=0.2 and a rectangular lower
opening spanning y=-2 to 0.2, both extending to z=+-3. Uniform size scales field
values and geometry consistently. Size lies in `[0.2,1.5]`; B position lies in
`[-1.5,1.5]^3`, and rotations lie in `[-180,180]` degrees.

The polynomial smooth union uses width `k` in `[0,0.6]`:

```text
h = clamp(0.5 + (b-a)/(2*k), 0, 1)
f = h*a + (1-h)*b - k*h*(1-h)
grad(f) = h*grad(a) + (1-h)*grad(b)    in the blend region
```

At zero width the implementation evaluates ordinary union directly. Composite
fields are defining functions and are not generally exact signed distances.
Gradients are analytic; sharp branch ties with unequal gradients, primitive
ridges/medial ambiguities and zero gradients are marked as undefined. Finite
mesh shading normals at such points are conventions, not mathematical claims.

The probe crossing search brackets changes of sign with 128 intervals and uses
36 bisections per crossing. It chooses the closest detected crossing in x.
Tangencies and features smaller than an interval can go undetected; this is not
a global nearest-point solver.

Midpoint volume is `occupied cell count * cell volume`, using strict negative
field membership. The table reports grids of 12, 16, 20, 24, 28, 48 and 64 cells
per axis in one fixed bounding domain. The 64-cell value is a comparison estimate,
not an exact reference or certified bound. Success at the sampling challenge
certifies the stated agreement only. Thin features and small intersections can
be missed, and error need not decrease at every resolution.

Source direction: the local [set operations](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/discrete_math.md:112>)
and [Boolean algebra](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/discrete_math.md:232>)
notes. Source notes, exercise cards and learner fields remain read-only.

## Geometry and ownership

`BooleanSolid` owns primitive preparation, field composition, gradients,
classification, surface probing and midpoint-volume estimates. `BooleanGeometry`
extracts one indexed surface into the existing `MathTriangleSurface` storage:
4,096 vertices and 24,576 indices. `MathObjects` owns actions, presets, lesson
judgments, plots and tables. `MathObjectScene`, the native host and shader retain
their existing owners and limits. No renderer extension or new persistence path
is introduced.

The surface builder follows the edge-intersection approach described in
[VTK's marching-cell documentation](https://vtk.org/doc/nightly/html/classvtkMarchingCellsContourCases.html).
This implementation is local C++; it imports no VTK code or dependency. Each
cube constructs directed contour segments on its six faces. Ambiguous square
faces use a shared centre-field decision; neighbouring cubes evaluate identical
face coordinates. Closed contours form triangles, quads or centroid fans.
Shared edge crossings are indexed once, and outward face order gives consistent
surface winding. This finite topology choice does not certify the topology of
the underlying continuous solid.

The display grid is bounded at 28 cells per axis. Fixed arrays hold grid values
and a bounded edge hash table, with no per-action heap allocation. Grid-node
values within `1e-12` of zero are canonicalized so aligned section planes share
vertices. Genuine nonzero sliver triangles are retained to preserve closure;
zero-area triangles are omitted. A full mesh budget retries at four fewer cells
per axis, down to four. Requested and actual counts are both displayed. Typical
work scales with the sampled grid and emitted surface; loops and storage have
explicit finite bounds.

Section view contours `max(F, z-sectionZ)`. The cut and the original boundary
are both sampled; its visible mesh volume is an approximation to the physical
section, rather than an exact half of the uncut mesh volume. Mesh residual is
measured against this displayed defining field.

This checkpoint provides reusable learning geometry. Arbitrary imported-mesh
Booleans, collision, UVs, export, game-engine packaging and a guaranteed topology
solver remain later capabilities.

## Verification

Release `math_lab` and `sorter` compile. Sixteen focused pure CPU suites and
62 reviewed text-only CLI routes pass: 78 checks overall. The dedicated Boolean
suite reports 2,705 numerical certificates and 377 CPU mesh states. Its largest
tested scene has 7,122 vertices and 30,936 indices, within the existing
8,192/65,536 limits.

Checks cover independent set membership, analytic box/cylinder/sphere volumes,
central-difference gradients after rotations, nonunique/zero normals, blend
limits, closed and consistently oriented mesh edges, section caps, rotated
primitive pairs, empty intersections, all presets and layers, retained settings,
truth-row boundaries, parameter extrema, atomic rejection and all four
challenges. A local Release sample averaged about 6 ms per sampling-layer
rebuild; this is CPU timing, not a rendering frame-rate claim.

Execution uses pure CPU suites and reviewed `--validate` paths that return before
native host, font or bookmark initialization. No images were generated,
captured, opened or viewed. No windows or font-atlas tests were used. The user
owns the manual visual test. Changes remain uncommitted.
