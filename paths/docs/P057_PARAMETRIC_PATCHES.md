# P057 — Parametric Patch Lab

`patch` is the 28th reusable math-lab object. A 4×4 control net shapes an open
bicubic sheet. It uses the existing compact inspector, example/layer toolbar,
Graph / Values / Math / Exercise drawer and native indexed-mesh renderer.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object patch --level 0 --object-preset 0
```

| Preset | Object | Useful changes |
| --- | --- | --- |
| 0 | Canopy | Raise an interior control, lower a corner, inspect the broad influence |
| 1 | Sail | Move an edge or deepen the billow; inspect a nearly horizontal normal |
| 2 | Curved ramp | Change the rise and the approach/departure slopes |
| 3 | Saddle terrain | Lift opposite corners and compare the two bending directions |

The first control index follows u and the second follows v: P00 through P33.
Choose a point in the control dropdown or click its marker in the viewport;
edit the selected XYZ row. Each coordinate lies in [-2,2] with step 0.01.
The Probe UV row moves across [0,1]². Curve and lathe selection retain their
four- and seven-point behavior. Layer changes preserve the patch shape.

Guides shows the control net, control markers, two parameter curves and the
probe. Layer 1 onward also shows tangent directions, a framed tangent plane and
the normal when defined. Shape only hides these guides. A fully collapsed mesh
retains its controls for recovery. Gold tint in layer 0 follows the selected
control's influence; selecting a control does not change the example name.

The Sampling group sets 4–32 subdivisions per axis, in increments of four.
Profile R resets all 16 controls together, including hidden coordinate rows.
This restores canonical defaults; choosing an example restores that example.

## Four connected layers

| Layer | Model and linked views | Challenge |
| --- | --- | --- |
| 0: Control net and blending | Sixteen weights and coordinates; influence plot; v-section height | Select a corner and put the probe there, making its weight one |
| 1: Tangents and normals | Actual partial derivatives, unit normal, area density and tangent angle | Find a regular point with a nearly horizontal normal |
| 2: Curvature and metric | K, H, two principal curvatures, first/second fundamental forms, curvature traces | Find K < -0.03 at a regular saddle point |
| 3: Area and mesh refinement | Highlighted parameter cell, local area density, triangulated-area convergence and separate quadrature | At 24+ subdivisions, mesh/quadrature agreement within 0.5%, and fine/coarse quadrature agreement within 0.1% |

World y is vertical. The normal orientation follows S_u cross S_v. Tangent and
normal arrows have fixed display lengths; the Values table retains the actual
vectors. The framed tangent plane uses orthonormal directions. Positive K is
teal, negative K coral, and near-zero or undefined K grey. The regularity flag
and omitted curvature measurements distinguish singular points from flat ones.

Both shape and probe edits go through the existing semantic parameter action.
Plot scrubbing, presets, row/group resets and challenges read the same model.
The document figure registry discovers `patch` through the object catalogue;
this checkpoint supplies the model without authoring a separate textbook lesson.

## Mathematical contract

For cubic Bernstein polynomials B_i and row-major controls P_ij,

```text
S(u,v) = sum(i=0..3, j=0..3) B_i(u) B_j(v) P_ij
J = |S_u cross S_v|
N = (S_u cross S_v) / J
E = S_u dot S_u; F = S_u dot S_v; G = S_v dot S_v
e = N dot S_uu; f = N dot S_uv; g = N dot S_vv
K = (e*g - f*f) / J²
H = (e*G - 2*f*F + g*E) / (2*J²)
principal curvatures = H +/- sqrt(H²-K)
```

The implementation computes analytic first and second derivatives. It uses J²
for the metric determinant to avoid subtracting nearly equal E*G and F².
The smaller principal curvature is recovered through the product K to avoid
cancellation when the two curvatures differ greatly.

A sample is treated as numerically regular only when J > 1e-12 and
J > 1e-10*|S_u|*|S_v|. A rejected sample has no displayed normal, tangent plane,
second fundamental form or curvature value. The first fundamental form remains
available as a potentially singular metric. Curvature plots use 129 points;
a singular probe or sampled trace point removes the trace. Sampling is not a
global regularity certificate between those points.

The area comparison has two independent constructions:

- A uniform parameter grid becomes two oriented triangles per cell. Cross-product
  magnitudes at or below 1e-12 are skipped. Double-precision triangle areas supply
  the convergence graph; native mesh positions are converted to floats.
- Composite 2×2 Gauss quadrature integrates J. The lab compares 8×8 and 16×16
  parameter-cell grids; the pure kernel supports 1–32 cells per axis.

Neither successive agreement nor the quadrature result is a certified error
bound. If the comparison area is numerically zero, the table reports absolute
rather than percentage differences. A folded patch counts area with multiplicity.
The sheet has no thickness or enclosed volume, and arbitrary edits can produce
folds, intersections and singularities. These are represented as mathematical
states rather than silently repaired geometry.

The kernel uses bounded loops and fixed storage. Evaluation has a fixed 16-term
sum; both area methods and mesh generation cost O(n²). The maximum surface has
1,089 vertices and 2,048 triangles before degenerate triangles are omitted.
Control bounds keep coordinates finite. Invalid kernel/display requests are
rejected before output mutation. The existing renderer and shader are unchanged.

The construction follows the [Michigan Tech Bézier surface properties](https://pages.mtu.edu/~shene/COURSES/cs3621/NOTES/surface/bezier-properties.html):
corner interpolation, nonnegative weights, partition of unity and boundary
curves. Differential quantities use the conventions in
[Wolfram's fundamental forms reference](https://mathworld.wolfram.com/FundamentalForms.html)
and [Gaussian curvature reference](https://mathworld.wolfram.com/GaussianCurvature.html).
Implementation and tests are original; no external code or image assets were copied.

## Implementation and verification

`BezierPatch.hpp/.cpp` owns the pure mathematical kernel. `PatchGeometry.hpp/.cpp`
publishes the existing triangle-surface contract. `MathObjects` supplies 53
parameters, four presets and four model layers. The compact inspector adds a
selected XYZ row and UV row through its existing metadata.

Parameter IDs widen from 8 to 16 bits while keeping every earlier numeric value.
Preset capacity grows from 24 to 64, and the shared selectable-point array grows
from 7 to 16; consumers retain explicit counts. Existing string parameter keys
and content/learner formats are preserved.

The dedicated CPU suite compares evaluation/derivatives with an independent
power-basis expansion and boundary curves with the existing 1D evaluator. Exact
planes, quadratic graphs, saddles and a parabolic-cylinder area integral verify
normals, curvature and integration. Further checks cover orientation/scale laws,
unequal principal curvatures, singular boundaries, collapsed and folded patches,
mesh edges/winding, capacity, every control selector, atomic presets/resets,
layer retention, challenge outcomes and the older curve/lathe pickers.

The shared layout suite covers 6,480 rectangle configurations and all 100
available object/layer states. Four new `--validate` CLI cases exercise the patch
layers, including the narrow layout, before native-host/typesetter/bookmark I/O.
The installed Release `math_lab` and `sorter` both built successfully. All 86
selected CPU/text checks passed: 18 CPU suites and 68 early-return validation
commands. The first regression run passed 85; the Boolean suite stopped at its
old hard-coded 27-object count. That assertion now uses `MathObjectKind::Count`,
and the rebuilt Boolean suite passed on its targeted rerun. No production repair
was needed after the regression run.

The patch suite passed 3,121,179 assertions across 72 CPU mesh states. Its largest
published scene used 5,414 vertices and 24,240 indices, within the shared renderer
capacities. The layout suite passed 153,469 assertions across the configurations
above. All four patch challenge CLI cases passed. Installed source checksums
matched the verified files after testing.

No images, native windows or font-rasterization checks are used. Visual acceptance
of these new presets remains with the user; changes are uncommitted.
