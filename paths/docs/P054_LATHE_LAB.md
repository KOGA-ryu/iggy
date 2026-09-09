# P054 — Lathe Lab

`lathe` is the math lab's 26th reusable object. A seven-point profile creates
vases, bottles, goblets and chess pawns, with hollow interiors, radial wall
thickness, partial revolutions and a viewing cutaway. Four layers share one
retained profile and its mathematical measurements.

## Open and explore

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object lathe --level 1 --object-preset 0
```

Choose **Vase**, **Bottle**, **Goblet** or **Chess pawn** in the sidebar. Click a
profile point or use **Selected profile point**, then edit its radius and
height fraction. Endpoints remain at heights 0 and 1. Interior height sliders
respect their neighbours and a minimum gap of 0.04. **Total height** scales the
object vertically. Left drag orbits, right drag pans and the wheel zooms.

| Layer | Controls and view | Mathematical evidence |
| --- | --- | --- |
| 0: Shape the profile | Seven points, mirrored outline, cavity profile | Radius/height table, tangent continuity, interior bulge and full-turn volume |
| 1: Revolve the profile | 0–360 degrees, Play/Restart, solid or hollow, cutaway | Volume proportional to the swept angle, generating radius and circular path |
| 2: Disks, washers and shells | 4–32 subdivisions, method and highlighted element | Every midpoint contribution, reference volume and convergence graph |
| 3: Surface bands and normals | Highlight a band, change subdivision count | Lateral and closure areas, frustum approximation, tangent/normal directions |

The vase, bottle and goblet start hollow; the pawn starts solid. The cavity
floor leaves material beneath the hollow region. The goblet's floor is inside
its bowl, above the stem. Wall thickness is measured **radially**, rather than
along the surface normal. Regions narrower than the wall thickness stay solid.
Edited profiles can create separate cavity pockets; the shell method counts
all occupied height intervals.

**Cutaway (% hidden)** affects visibility, while readouts retain the complete
selected revolution angle. Physical partial revolutions have two radial faces;
their area contributes to the boundary-area readout. **Shape only** hides guides
in the revolution and surface-area layers. A zero-volume state retains its
profile guides so it can be edited. The volume layer uses a wire profile and
highlights one element at a time; its table includes every subdivision.

**Play** completes a full revolution in four seconds. Edits and presets pause
playback. Layer changes retain the profile, hollow settings and angle.

## Mathematical contract

Write normalized height as `u` and physical height as `h=H*u`. The outer radius
`R(h)` joins seven nonnegative samples with six cubic Bezier segments. Their
heights are linear in each segment. Weighted harmonic PCHIP slopes preserve
monotonicity within each interval; first derivatives match at knots, while
second derivatives may jump. This follows the mathematical scheme documented
in [SciPy's PCHIP reference](https://docs.scipy.org/doc/scipy/reference/generated/scipy.interpolate.PchipInterpolator.html).
The implementation is local C++ and reuses the existing de Casteljau kernel;
SciPy is not a runtime dependency.

Radii lie in `[0,1.5]`; height lies in `[1,4]`. For a hollow object with floor
fraction `f` and radial thickness `w`, the inner radius is zero below `f*H`,
and `r(h)=max(R(h)-w,0)` above it. Width ranges from 0.03 to 0.35 and floor
fraction from 0.04 to 0.8. Sorted critical heights include profile knots, the
floor and radius crossings of `w`.

For a full revolution,

- `V = pi * integral (R(h)^2-r(h)^2) dh`.
- Equivalently, `V = 2*pi * integral q*L(q) dq`, where `L(q)` is the total
  occupied height at radius `q`, including disconnected intervals.
- Outer lateral area is `2*pi * integral R(h)*sqrt(1+R'(h)^2) dh`; the inner
  formula uses `r(h)` over the cavity intervals.

Reference volume uses four-point Gauss-Legendre integration on every critical
interval. Squared radius has degree six there, so the rule is exact up to
floating-point arithmetic and the crossing solver's tolerance. Monotone
crossings use 44 bisections. Shell spans are found from all outer and inner
crossings, then contiguous occupied spans are merged.

The displayed volume approximation samples one midpoint per equal height
interval (disks/washers) or equal radial interval (shells). A selected shell's
separate occupied intervals appear together. The table's contributions sum to
the approximation. Midpoint errors need not decrease at every individual count,
especially when a subdivision crosses the cavity floor.

Reference lateral area uses adaptive eight-point Gauss-Legendre integration,
with local comparison tolerance `1e-10` times normalized interval width and
maximum recursion depth 10. This is a numerical reference, not a certified
interval bound. Frustum bands use uniform height subdivisions plus the profile's
critical heights. The reported approximation adds the same base, rim, floor and
physical radial-face contributions to its approximate lateral areas.

Volume and rotational areas scale by `angle/360`. A physical angle strictly
between 0 and 360 additionally contributes two radial faces, each with area
`integral (R-r) dh`. At angle zero the solid is inactive, with zero volume and
boundary area. A parameterized surface normal is undefined when radius is zero;
its probe arrows are omitted. Mesh normals at collapsed axis vertices are
finite shading conventions, not a claim of regular parameterization.

Source direction: [the local revolution/shell notes](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/calculus.md:755>).
Source notes and learner fields remain read-only.

## Geometry and ownership

`LatheProfile` prepares the ordered profile and owns pure evaluation, integration
and shell-span kernels. `LatheGeometry` builds either the closed lathe surface
or the selected volume element into a fixed indexed mesh. `MathObjects` owns
semantic actions, presets, playback, challenge judgments and snapshots.

`MathTriangleSurface` adds bounded indexed geometry to the existing snapshot:
4,096 vertices and 24,576 indices. `MathObjectScene` validates it and publishes
one additional draw through the existing renderer. The old rectangular surface
patch and all primitive contracts remain available. No native-host or shader
change is needed. Profile picking now uses an explicit point count and semantic
selection parameter; existing curves keep their four-point interaction.

The smooth lathe uses 32 angular subdivisions and a height grid augmented with
knots, cavity transitions and highlighted-band boundaries. Outer/inner walls,
base, rim, cavity floor and exposed radial faces close the displayed material.
Separate face vertices preserve cap and cut-face normals. Axis-degenerate
triangles are omitted. The mesh is a finite approximation; indexed export,
vertex welding, UVs, collision meshes and game-engine packaging remain future
capabilities. The existing document/object registry can select `lathe` by key.

## Verification

Release `math_lab` and the shared sorter UI compile. Fifteen focused CPU-model
and mesh suites and 58 early-return CLI routes pass. The dedicated lathe suite
checks 1,705 numerical certificates and 243 mesh states. Its largest scene has
6,830 vertices and 28,806 indices, within the existing 8,192/65,536 limits.

Checks include analytic cylinders/cones/frustums, independent Bernstein and
Simpson calculations, monotonic interpolation, continuous slopes, shell
occupancy and disconnected spans, oriented mesh volume, hollow floors, caps,
partial revolutions, profile extrema, finite geometry, atomic rejection,
presets, retained settings, playback and all four challenges. A local Release
sample averaged about 0.11 ms per area-layer rebuild; this is CPU timing, not a
rendering frame-rate claim.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --object lathe --level 2 --object-preset 3 --set lathe_slices=32 --check
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --object lathe --level 3 --object-preset 2 --set lathe_slices=32 --check
```

Execution uses pure CPU tests or the reviewed `--validate` return before host,
font or bookmark initialization. No images were generated, captured, opened or
viewed. No native UI or font-atlas tests were run. Changes are uncommitted;
visual acceptance awaits the user's manual test.
