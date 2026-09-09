# P062 — Distance Geometry Lab

`distance` is the 33rd Math Lab object, with four retained layers, six complete
examples and twelve compact controls. Its source is
[card 099 — Euclidean distance matrices](/Users/kogaryu/devil/99-red-booleans/problems/math/099_euclidean_distance_matrices.md).
The source card and all learner fields remain read-only.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object distance --level 3 --object-preset 5 --set distance_mix=0
```

Press Play to mix two line configurations. At an interior mixture their distance
matrix requires a plane; at either endpoint it needs only a line. Playback sweeps
t from zero to one in about 8.3 seconds. It interpolates squared distances, not
physical time or the original point coordinates. Every edit pauses playback.

| Layer | Model |
| --- | --- |
| 0 — Lengths and squared distances | Six ordinary lengths, reconstructed shape and the full symmetric 4x4 squared-distance matrix |
| 1 — Coordinates and mirror ambiguity | Anchored Gram matrix, reconstructed coordinates and a reflected counterpart |
| 2 — Dimension and impossible distances | Rank, face inequalities, Gram eigenvalues and a zero-sum impossibility witness |
| 3 — The convex cone | Nonnegative scaling and mixing of two squared-distance matrices, with live eigenvalue plots |

Examples are Regular tetrahedron, Planar square, Collinear points, Coincident
points, Impossible tetrahedron, and Dimension from mixing. The impossible
example has base edges 2 and three apex edges 1.1. Each face satisfies its
triangle inequalities, but the proposed apex is too close to all three base
vertices for a common Euclidean embedding.

## Controls and representation

Six length fields, grouped in two compact rows, edit AB, AC, AD, BC, BD, CD in
[0,4]. Inspect edge highlights the selected pair. Reconstruction side reflects
Z; with guides on, layer 1 draws the opposite embedding in violet. Lengths
cannot determine handedness. Layer changes retain inputs and settings.

Layer 3 selects a second valid distance matrix: tetrahedron, square, line,
differently spaced line, or coincident points. The live result is
D(t)=s*((1-t)*D_first+t*D_second), t in [0,1], s in [0,4]. Length controls always
edit the first matrix; the table and shape show the mixture. Scaling squared
distances by s scales ordinary lengths by sqrt(s), and volume by s^(3/2).
The convex-cone guarantee is labelled only when both input matrices are valid.
An invalid first matrix can still interpolate into a valid region.

The shape uses four labelled point markers and six edges. Only a rank-three
embedding has filled tetrahedron faces. Planar, line and point cases retain
their actual lower dimension. Coordinates are centred for scene placement;
the coordinate matrix is anchored at A=0. When no embedding exists, the model
publishes six independent length bars and no fabricated tetrahedron. Those bars
have the requested lengths but are explicitly labelled as separate segments.

## Numerical owner and contract

`DistanceGeometry.hpp/.cpp` owns one bounded four-point analysis. Its input is
six finite squared distances in [0,64]. It builds a symmetric D with zero
diagonal and the anchored 3x3 Gram matrix

G_ij=(D_Ai+D_Aj-D_ij)/2, for i,j in {B,C,D}.

D has a Euclidean embedding iff G is positive semidefinite. The minimum
embedding dimension is rank(G), at most three for four points. The kernel
normalizes by max(D), then performs at most 32 Jacobi sweeps in fixed storage.
Eigenvalues are sorted descending. PSD and numerical rank use relative
threshold 1e-10; the zero matrix is handled directly. Eigenvalues within that
rank tolerance are discarded for reconstruction, and the resulting maximum
squared-distance error is reported. This is a bounded numerical teaching model,
not an exact classifier of arbitrarily close boundary cases.

A Gram square root gives point coordinates. A deterministic point-based frame
removes arbitrary eigenvector signs and repeated-eigenvalue rotations. The first
longest residual point defines each axis; ties choose the first label. A change
of pivot can change display orientation, without changing distances. Reflection
flips the third coordinate. Volume comes from the three positive Gram
eigenvalues; rank-zero, rank-one and rank-two configurations have volume zero.

For a negative Gram eigenvector w, the kernel forms
x=(-sum(w),w_B,w_C,w_D), normalized to unit length. Its coefficients sum to zero
and x^T D x is positive, contradicting the distance-matrix condition on the
zero-sum subspace. This certificate is independently checked and plotted.
All four face triangle inequalities are tested separately: they are necessary,
but the impossible example demonstrates that they are not sufficient.

The kernel does not infer asymmetric or negative matrix entries: six
nonnegative lengths construct symmetry, diagonal zero and nonnegativity by
design. Mixing is explicitly on squared distances. No source attempt is solved
or marked complete; the finite examples illustrate card 099's general cone.
No external helper code was copied.

## Integration and verification

MathObjects remains the semantic state owner. The new object and parameters are
appended without renumbering older IDs. Existing compact controls, matrices,
plots, CPU scene tessellation and renderer capacities are reused. No renderer,
GPU, textbook catalogue, question logic or persistence route changed.

The installed CPU suite passed 1,624,592 assertions, including 4,096 exact integer
matrix comparisons and 151 scene states. An independent principal-minor oracle
checks PSD and rank. Further checks cover geometric reconstruction, determinant
volume, reflection, rigid-motion invariance, scale invariance, cone closure,
rank gain, zero scale, invalid inputs, all presets/layers, finite scene buffers,
control limits, reset, retained layers and playback.

Maximum squared-distance reconstruction error was 4.974e-14; maximum eigenpair
residual was 1.422e-14. The scene used at most 1,572 vertices and 8,640 indices.
Both Release applications, `math_lab` and `sorter`, built successfully. All 28
explicitly selected checks passed: 23 CPU suites and five early-return distance
`--validate` cases. The CLI set includes all four new challenges and zero scale.
The shared inspector suite covers all 120 object/layer states and 6,480 layouts.
Selected CTest commands were checked against current metadata and test sources;
no fixtures, native/font suites or image-producing routes were selected.
Installed file hashes matched the reviewed source, and whitespace checks passed.

No images, windows, captures, browser previews, font probes or personal saves
were used. Changes remain uncommitted. Visual appearance and pointer feel are
for the user's test.
