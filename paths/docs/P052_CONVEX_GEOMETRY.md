# P052 — Convex geometry objects

Two new reusable `MathObjects` providers, `psd` and `norm`, add eight working
layers to the native math lab. The collection now has 24 objects. Both use
existing numerical snapshots, linked plots, tables, surfaces and scene primitives.

## Open and explore

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object psd
/Users/kogaryu/iggy3d/paths/b/math_lab --object norm
```

Choose a learning layer and an object example in the sidebar. Sliders and
scrubbable plots change one model revision; each layer has a separate challenge.
Norm balls have solid and open cross-section views. The vector-addition layer
always uses an open boundary so its construction remains visible. Orbit, pan,
zoom and Reset view use the existing camera controls.

| Provider | Layer 0 | Layer 1 | Layer 2 | Layer 3 |
| --- | --- | --- | --- | --- |
| `psd` | Matrices in the cone | Eigenvalues and quadratic directions | Convex mixtures and cone rays | Slices and supporting objectives |
| `norm` | Unit distance and shape | Triangle inequality | The limit as p grows | Dual norms and supporting planes |

## Mathematical contract

For `A=[[a,b],[b,c]]`, cone coordinates are `x=(a-c)/2`, `y=b`,
`t=(a+c)/2`. Eigenvalues are `t +/- hypot(x,y)`; PSD is equivalent to
`t >= hypot(x,y)`, or to all three principal minors `a,c,ac-b²` being
nonnegative. Numerical rank and PSD classification use an eigenvalue tolerance
of `1e-10 * max(abs(a),abs(b),abs(c))`, with no absolute floor at one.
The zero matrix is handled directly. The scene is an open, bounded guide to
an unbounded cone. This is the exact three-dimensional space of symmetric
2x2 matrices; the six-dimensional 3x3 PSD cone is not projected into it.

The second layer displays the graph `z=0.3*q(u,v)` beside the cone, translated
by `(4.6,0,0)`. Eigenvector columns are ordered maximum then minimum.
Repeated eigenvalues admit other valid bases. Negative semidefinite and
negative definite examples stay distinct from indefinite matrices.

The mixture is `s*((1-alpha)*A+alpha*B)`, with `B=2*v*v^T` for a unit
2-vector, `alpha` in `[0,1]`, and `s>=0`. Input A remains editable, so the
model explicitly reports when the PSD endpoint premise fails. B has rank one;
nonparallel rank-one endpoints with an interior mixture and positive scale
produce a positive definite matrix.

The optimization layer fixes `trace(A)=2*t`, `t>=0`. Its feasible set is the
disk `x²+y²<=t²`. For a unit objective direction `(cos(phi),sin(phi))`, the
maximum is `t`, attained at `(x,y)=t*(cos(phi),sin(phi))`. The gold matrix is
that optimum. The movable violet objective plane has value `fraction*t`;
fractions outside `[-1,1]` miss a nonzero disk. This is an analytic fixed-trace
example, not a general semidefinite solver. At `t=0` the feasible set is one point.

Norm balls live in real dimension three. Finite p is supported from 1 to 32
in quarter steps; infinity is a separate exact max operation. Powers are
computed after scaling by the largest absolute component. Finite meshes sample
the true norm-one boundary. At p=1 all triangle interiors lie in octahedron
faces; the solid infinity-mode body is an exact cube primitive. The open view
shows three principal cross-sections, with exact cube edges in infinity mode.

The limit layer keeps the vector fixed and compares
`max|x_i| <= ||x||_p <= 3^(1/p)*max|x_i|`. Infinity has no finite-p graph marker.
Zero has norm zero, but no normalized-boundary probe.

The dual layer maximizes `w dot x` on the primal unit ball, with value
`||w||_q`, where `1/p+1/q=1`. Endpoint pairs are `(1,infinity)` and
`(infinity,1)`. A maximizing point and `w/||w||_q` certify the two boundaries
and their pairing. For a tied maximum at p=1, the first maximizing coordinate
is selected deterministically; infinity uses the sign of each nonzero component.
Zero w omits the contact and supporting plane. The dual wire body is translated
by `(3.5,0,0)` for visibility; tables and numerical checks use original coordinates.

## Ownership and reuse

`MathObjects` owns the mathematical kernels, challenge judgments and fixed-size
snapshots. `MathObjectPreset` describes at most three parameter updates, applied
atomically through `MathActionKind::ObjectPreset`. Unknown or unavailable
presets reject without mutation. Reset, edits and layer changes follow existing
feedback rules. The lab matrix adapter now honors declared rows/columns, so
2x2 matrices do not acquire a misleading third row.

The existing `ObjectLessonSpec` and document diagram registry can select these
providers by their stable keys. No textbook chapter or source-card attempt is
created by selecting a diagram. Renderer, scene tessellator, question owner,
bookmarks and native font handling are unchanged.

Source direction: the local [PSD cone card](/Users/kogaryu/devil/99-red-booleans/problems/math/098_the_psd_cone_in_low_dimensions.md)
and [p-to-infinity card](/Users/kogaryu/devil/99-red-booleans/problems/math/132_the_infinity_norm_as_a_limit_of_p_norms.md).
The first object illustrates the n=2 case; the second specializes the norm
family to real dimension three and adds a duality teaching layer. Both source
cards and their learner fields remain read-only.

## Verification

The Release math lab and shared sorter UI compile. Thirteen targeted pure-model
and CPU-mesh suites and 50 CLI routes pass, including all eight new challenges.
The new suite checks 710 independently formulated mathematical certificates and
666 mesh states. Its maximum is 5,319 vertices (capacity 8,192) and 18,240 indices
(capacity 65,536).

Checks cover all principal minors versus spectral classification, trace and
determinant identities, eigenvector residuals, quadratic mesh equations,
rank-one mixtures, invalid endpoint premises, optimum attainment, exact endpoint
shapes, triangle inequalities, norm bounds, dual contacts, zero/tie cases,
maximum control settings, wire-body capacity and atomic rejection/presets.

Text-only examples:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --object psd --level 2 --object-preset 1 --check
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --object norm --level 3 --object-preset 3 --set norm_support=1 --check
```

All CLI checks use the early `--validate` return before native-host, font or
bookmark initialization. No images were generated, captured, opened or reviewed.
No font-atlas or native UI tests were executed. Changes are uncommitted; visual
acceptance remains pending the user's manual test.
