# P063 — Polar Decomposition Lab

`polar` is the 34th Math Lab object. Four retained layers, two source solids,
six complete examples and fourteen controls model
[card 089 — Polar decomposition](/Users/kogaryu/devil/99-red-booleans/problems/math/089_polar_decomposition.md),
from Solomon, *Numerical Algorithms*, chapter 7, exercise 7.8.
The source and learner fields remain read-only; no helper code was copied.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object polar --level 2 --object-preset 0
```

Press Play to remove the shear. The left solid stays at A, the middle follows
X_k, and the right shows the orthogonal limit W. The iteration advances one
step every two seconds; its slider and graphs can scrub directly to a step.
All edits pause playback. Change layers without losing the matrix or settings.

| Layer | Model |
| --- | --- |
| 0 — Linear deformation | Blend I toward A, deforming a marked block or tetrahedron |
| 1 — Stretch and orientation | Compare P, W and WP = A using the same source and display scale |
| 2 — Removing shear by iteration | Apply the inverse-transpose iteration, with singular modes and orthogonality residuals |
| 3 — Reflections and collapsed dimensions | Compare proper rotations, reflections and nonunique null-space extensions |

Examples: Sheared block; Quarter-turn and stretch; Pure rotation; Mirror and
stretch; Collapsed sheet; Thin direction. The last example starts with a
singular value of 0.05, which becomes 10.025 after the first unscaled iteration
before approaching 1. Singular modes retain their initial identities in the
trace; their order can change after a step.

## Controls and geometry

Nine coefficients in [-3,3], in increments of 0.05, form a row-major matrix
acting on column vectors. Layer 0 uses three compact coefficient rows; later
layers edit the same semantic parameters through the matrix drawer. Source
solid and construction guides reuse the existing inspector. Deformation amount
is available in layer 0, iteration k in layer 2 when numerically supported,
and null-space extension in layer 3. Presets set all fourteen parameters.

Teal denotes P, violet W, coral A, and gold an asymmetric corner landmark.
The source block has unequal side lengths and face shading. Reference edges
show the undeformed source at each panel, and axis arrows use X/Y/Z colours.
Panel centres are presentation offsets. A common scale fits all currently
compared matrices, including a large first iterate; matrix values and
measurements are never scaled. Compare the scale readout when scrubbing.

Triangles have geometric normals and corrected outward winding under
reflections. Singular or visually vanishing-volume transforms retain their
exact edges and landmark without overlapping filled faces. This also covers
intermediate collapse in a straight blend between I and an invertible A.
The blend is not a rigid rotation interpolation. The existing renderer and
fixed scene capacities are unchanged.

## Numerical contract

`PolarDecomposition.hpp/.cpp` owns the bounded 3x3 kernel. It accepts finite
coefficients in [-4,4] and uses normalized, one-sided Jacobi SVD with at most
32 sweeps. It orthogonalizes A's columns directly instead of forming A^T A.
Singular values are sorted descending. Values at or below
1e-12*sigma_max are discarded, with their effect included in the relative
Frobenius reconstruction residual. This is numerical rank, not an exact
classification of arbitrarily close singularities.

For A = U Sigma V^T, the factors are W = U V^T and
P = V Sigma V^T. P is symmetric positive semidefinite; W is orthogonal and
may have determinant -1. At rank loss, deterministic Gram-Schmidt completion
chooses null directions. The alternate extension reverses one of those
null directions in W; multiplying by P gives the same reconstruction.
For an invertible input, the alternate setting leaves W unchanged. P is
unique mathematically; the orthogonal extension of a singular input is not.

The inverse-transpose iteration is available only for numerical rank three
and sigma_min >= 1e-8. Its complete 33-state trace evaluates
X_k = U diag(d_k) V^T, with d_0 = sigma and
d_(k+1) = (d_k + 1/d_k)/2. X_0 preserves the exact input bytes. No inverse
is attempted for unsupported inputs, and no replacement trace is invented.
The convergence graph plots log10(||X_k^T X_k-I||), with a display floor of
-16. Metrics retain the unfloored residual. Storage and loop bounds are fixed;
no heap allocation, physical integrator or random sampling is involved.

## Verification

The installed pure CPU suite passed 5,349,854 assertions across 838 matrix cases
and 228 scene states. It checks known polar factors, analytic shear,
independent principal-minor PSD certificates, signed determinants, repeated
and tiny singular values, null-space alternatives and positive scale
invariance. An independent cofactor inverse checks every iteration against
the inverse-transpose recurrence. Control checks cover presets, layers,
coefficient extrema, reset, rejected mutations, playback and singular gating.
Scene checks cover finite buffers, indices, winding, capacity and matrix bindings.

Maximum relative reconstruction error was 7.791e-14; maximum W orthogonality
error was 1.611e-15. The largest scene used 3,429 vertices and 10,332 indices.
Both Release applications, `math_lab` and `sorter`, built successfully. All 29
explicitly selected CTest checks passed in 3.87 seconds: 24 CPU suites and five
early-return `math_lab --validate` cases. All four polar challenges passed;
the reflection case reports det(W) = -1. The shared compact-inspector suite
passed 155,134 assertions across 6,480 layouts and all 124 object/layer states.
Current CTest commands and test sources were inspected before execution.
No native UI, font or image test was run.

Changes remain uncommitted. Textbook integration belongs to the textbook
worker. No native window, image, capture, screenshot or font probe is used;
the user performs visual review with the launch command above.
