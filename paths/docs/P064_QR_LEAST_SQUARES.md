# P064 — QR and Least Squares Lab

`qr` is the 35th Math Lab object. It supplies a bounded real 3x2 model for
orthogonalization, QR reconstruction, projection, least squares, and
minimum-norm solutions. Seventeen retained controls and seven examples use
the existing compact inspector, endpoint picker, matrices, plots and scene.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object qr --level 0 --object-preset 0
```

Press Play for the six-second Gram–Schmidt construction. Click a labelled
endpoint or choose Edit vector, then drag/type its XYZ component controls.
The existing viewport drag still orbits the camera; endpoint selection does
not introduce a new world-space drag gesture. Edits pause playback.

| Layer | What to explore |
| --- | --- |
| 0 — Building an orthonormal frame | Normalize the first pivot column, subtract its shadow, normalize the remainder |
| 1 — QR and reconstruction | Build both pivoted columns from Q directions and R coefficients |
| 2 — Projection and least squares | Compare a target, its closest point in the column span, and a trial fit |
| 3 — Null directions and minimum norm | Explore the coefficient-space error bowl, trough or flat plane and its minimizing family |

Examples are Tilted plane, Orthogonal columns, Exact fit, Dependent columns,
Zero first column, Zero matrix, and Almost parallel. In Tilted plane,
coefficients (1,1) give the least-squares fit with squared residual 2.25.
In Almost parallel, the minimum is (-20,20); choosing Minimum-norm fit shows
it directly without changing the stored trial coefficients.

## Representation and controls

The original columns a0 and a1 are blue and violet; b is gold. The kernel
pivots the longer column first, with original order retained on ties, and
publishes the permutation explicitly. The drawer displays A Pi (3x2), Q
(3x2) and R (2x2), packed according to their actual rectangular dimensions.
Coefficient tables always use the ORIGINAL a0/a1 order.

The construction has four stops: 0 original pivoted columns, 1 normalize
first, 2 subtract its shadow from second, 3 normalize the remainder. Between
stops the arrows interpolate continuously. Inactive directions are zero;
a dependent column never becomes an invented perpendicular unit vector.

Layer 2 shows the best fitted point in teal and the selected trial in coral.
The gold residual segment is perpendicular to the active column span.
The shaded patch/line is a finite guide to an unbounded span. XYZ input
components range over [-3,3] in steps of 0.05; trial coefficients range over
[-64,64] in steps of 0.01. Minimum-norm fit can display a computed solution
outside that trial range. Each linked graph varies one coefficient within
three units of the selected pair, clipped to editable bounds in manual mode.

Layer 3 uses offsets delta=x-x_min over [-3,3]^2. Its right-hand surface has
height proportional to ||A delta||^2; the scale readout converts back to
squared-error units. The gold origin is x_min, and violet traces the active
null directions. Rank two gives a bowl, rank one a trough, rank zero a flat
plane. One or two null-offset controls appear only when those directions
exist. The left fitted point remains unchanged along exact null directions.
Vector scene coordinates use a common display scale; numerical values retain
their mathematical units. No renderer or scene capacity was changed.

## Numerical owner

`QrLeastSquares.hpp/.cpp` owns the pure fixed-storage kernel. Inputs are two
real three-vectors and b, each component in [-4,4]. Nonzero max(abs(A)) must
be at least 1e-100; this prevents unrepresentable coefficients in the bounded
model. There is no iterative convergence loop, heap allocation, normal-equation
solve, random sampling or external numerical-library dependency.

The kernel normalizes by max(abs(A)), chooses the longer pivot column, and
uses two-pass modified Gram–Schmidt. Rank two requires the second remainder
norm to exceed 1e-10 times the leading pivot norm. Otherwise the second
direction is explicitly truncated, with zero Q column and R diagonal.
Relative Frobenius reconstruction error records the truncation. Active Q
columns are orthonormal; the zero unused columns are not part of that claim.

At rank two, triangular back substitution solves R y=Q^T b, followed by
unpermuting the coefficients. At rank one, the remaining scalar equation
is solved in the direction of its coefficient row, producing the minimum
Euclidean coefficient norm. Its perpendicular unit vector spans the null
space. At rank zero, x_min=0 and the two coordinate axes form the null basis.
Projection uses active Q directions, and the residual is b-p.

Null families refer to this numerical-rank model. Trial/family fitted points,
normal residuals and squared errors are evaluated using the original A, so
any rank-truncation effect is still measurable. The error surface also uses
original A. For exact rank loss, x_min is perpendicular to its null space and
||x_min+N t||^2=||x_min||^2+||t||^2.

## Learning scope and source connection

This is an asset foundation for chapters 089–095, 100, 102 and the QR portion
of 105/112/117 in the chapter checklist. It is not a complete arbitrary-size
factorization system, full pseudoinverse-matrix editor, world-space endpoint
dragger, or finished chapter/exercise integration. Existing chapter boxes
remain open; progress notes identify the delivered components.

The conceptual source connection is
[card 014 — Gram–Schmidt QR](/Users/kogaryu/devil/99-red-booleans/problems/math/014_qr_by_gram_schmidt.md)
and [card 033 — column-space projection](/Users/kogaryu/devil/99-red-booleans/problems/math/033_projection_onto_a_column_space.md).
This model uses authored 3x2 examples and the explicit A Pi=QR pivot convention,
whereas the source's ordinary QR discussion uses A=QR. It does not fill the
missing row in 014(d), claim to solve the cards' other dimensions, copy helper
code, or write to source attempts. Textbook integration stays with its owner.

## Verification

The installed CPU suite passed 4,724,859 assertions across 3,053 matrix cases and
342 scene states. It exhaustively covers all 729 matrices with entries in
{-1,0,1}, with four target vectors each. Independent exact cross-product rank,
geometric projection, and coefficient formulas check the results. Further
checks cover column permutations, positive scaling, coordinate rotations,
near-dependence, the declared rank cutoff, null-space minimum norm, loss
Pythagoras, invalid requests, presets/layers, picker metadata, control extrema,
graph scrubbing bounds, reset, retained state and bounded playback.

The maximum relative QR residual was 4.472e-12 and maximum active-Q
orthogonality error 4.558e-16. The largest scene used 2,998 vertices and
12,048 indices. Release builds of `math_lab` and `sorter` passed. All 30
selected CTest checks passed (25 CPU suites and five QR text-only CLI cases)
in 4.19 seconds. The shared compact-layout suite passed 155,289 assertions
across 6,480 layouts and 128 object/layer states. All four QR challenge CLI
cases reported success; the zero-matrix case produced finite scene metadata.

Changes are uncommitted. Verification is CPU/text only: no images, screenshots,
captures, native windows or font probes. The user performs visual review.
