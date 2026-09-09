# P053 — Curves and sweeps

`curve` is the math lab's 25th reusable object. Four editable control points
connect curve construction, differential geometry, distance travel and a capped
sweep. All four layers retain the same control points and shape settings.

## Open and explore

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object curve --level 3 --object-preset 0
```

Choose an example in the sidebar: **Curved pipe**, **Arched cable**, **Twisted
ribbon** or **Tapered horn**. Click a control point or use **Selected control
point**, then adjust its x/y/z sliders. Left drag orbits, right drag pans and the
wheel zooms. **Shape only** hides construction guides on a valid sweep.

| Layer | What changes | Linked evidence |
| --- | --- | --- |
| 0: Control points and interpolation | Four controls and position t | de Casteljau construction, Bernstein weights, control tetrahedron volume |
| 1: Tangents, speed and curvature | Controls and position t | Tangent/normal arrows, analytic derivatives, speed and curvature |
| 2: Travel by distance | Parameter or distance travel | Two translated curve copies, eight arc-length intervals, position mapping |
| 3: Profiles, taper and twist | Circle/square/norm profile, radius, aspect, end scale, twist | Capped surface, start/end profiles, length and regularity readouts |

**Play** traverses a complete curve in four seconds; **Restart position** returns
to zero. Editing a control or selecting a preset pauses playback. Each layer
has a measured challenge. Level 0 suggests moving a handle out of the original
plane; level 2's arched cable makes uneven parameter speed easy to compare.

## Mathematical and geometry contract

The cubic is `r(t)=sum B_i^3(t)*P_i`, `0<=t<=1`. de Casteljau interpolation
computes its position and its first two derivatives. At a regular point,
`T=r'/|r'|` and `curvature=|r' cross r''|/|r'|^3`. A zero derivative omits the
tangent/normal arrows and curvature value. A regular straight curve retains
its tangent and has zero curvature.

Controls are finite and in `[-2,2]^3`, matching the editor's coordinate domain.
The regularity tolerance is `1e-10` times the largest control-edge length.
Endpoint derivatives and the real roots of the derivative's strongest coordinate
polynomial detect stationary tangents, including roots between sample nodes.
Candidate roots are checked against the complete three-dimensional derivative.
A stationary curve stays editable but does not produce a sweep. A constant
curve also has no distance parameterization; its inverse uses t=0 as an explicit
inactive convention.

Arc length follows the local calculus notes' definition
`L=integral |r'(t)| dt`. Recursive subdivision brackets length between the chord
and control-polygon lengths. A 129-knot table uses a total subdivision tolerance
of `1e-9`, with at most 12 subdivisions per table interval. The midpoint estimate
and remaining bound width are displayed. These are floating-point geometric
bounds, with ordinary roundoff, rather than a directed-rounding interval proof.
Length inversion uses 22 bisections inside the matching table interval. Layer 2
compares **arc-length** steps, not the straight chords between markers. Its two
copies are translated by x=-2.6 and x=+2.6; the table keeps original coordinates.

The sweep's moving frame starts with the world axis least aligned to the tangent
and advances by discrete minimal rotations. Fixed parameter anchors stabilize
its orientation; straight sections do not trigger a Frenet-frame flip. At an
exact 180-degree step the previous normal is retained as a deterministic
convention. User twist rotates the two normal-plane axes by total twist times
arc-length fraction. Radius interpolates linearly from its initial value to
`initial radius * end scale` along distance. Aspect scales the second profile
axis. A norm profile reuses the max-scaled norm-ball boundary calculation;
square is the exact infinity-norm section.

The fixed surface contains 21 body rings at equal distance fractions, each with
16 profile edges and a duplicate seam vertex. Four extra rows close the ends:
centre, hard-normal rim, body, hard-normal rim, centre. The resulting 25x17 grid
uses 425 of the existing 441 surface vertices. A zero end scale collapses the
last ring to a horn tip. Coincident cap connectors and centre rows intentionally
produce degenerate triangles. Body normals use local mesh differences with a
radial fallback at collapsed tips; cap normals follow endpoint tangents.

Curves and profiles are finitely sampled. Wide or tightly bent sweeps can
self-intersect. This is editable mathematical geometry; export topology, UVs,
collision meshes and game-engine packaging are separate future capabilities.

## Ownership and reuse

`BezierCurve.hpp/.cpp` provides a pure fixed-storage kernel with no UI or native
host dependency. `MathObjects` owns controls, challenges, playback and the
complete geometry/readout snapshot. `MathObjectPreset` now has capacity for 24
parameter updates, still applied in one semantic revision. Existing presets
retain their explicit counts. All control coordinates remain available to
semantic callers; the lab sidebar displays only the selected control's XYZ.
The typed `MathCurveView` supplies pickable scene coordinates, including the
translation used in the distance layer.

The existing object registry exposes the stable key `curve` to object lessons
and document diagrams. The renderer and `MathObjectScene` use their existing
surface and primitive contracts. No source cards, learner fields, textbook
chapter content or practice-save formats are changed.

Source direction: [arc length in the local calculus notes](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/calculus.md:737>).
The Bézier construction and transported sweep extend that direction into a
reusable geometry provider.

## Verification

The Release math lab and shared sorter UI compile. Fourteen targeted pure-model
and CPU-mesh suites and 54 early-return CLI checks pass. The dedicated curve
suite checks 599 numerical certificates and 183 mesh states; the largest has
6,102 vertices (capacity 8,192) and 25,632 indices (capacity 65,536).

Independent checks use Bernstein/power formulas and adaptive Simpson integration
to verify derivatives, curvature, geometric length bounds and inverse length.
Mesh checks cover profile equations, tangent planes, centres, taper, twist,
square corners, cap/seam closure, unit normals and finite buffers. Stationary
endpoints/interior roots, straight and constant curves, all presets and layers,
control extrema, rejection, feedback, retained settings and playback are covered.
A local Release sample averaged about 1.35 ms per sweep rebuild; this is a CPU
measurement, not a rendering frame-rate claim.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --object curve --level 2 --object-preset 1 --set curve_travel=1 --check
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --object curve --level 3 --object-preset 3 --set curve_twist=90 --check
```

All execution used pure CPU tests or the reviewed `--validate` branch before
native-host, font and bookmark initialization. No images were generated,
captured, opened or viewed, and no native UI or font-atlas tests were executed.
Changes are uncommitted. Visual acceptance awaits the user's manual test.
