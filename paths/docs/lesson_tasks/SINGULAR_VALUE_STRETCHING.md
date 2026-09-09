# Singular-value stretching

Status: ready implementation brief; third item in [Batch 01](BATCH_01.md).
Follow that file's ownership, no-image, validation and continuation rules.

## Learning outcome and scope

Read singular values as nonnegative stretch magnitudes and explain why an SVD
stage can rotate or reflect coordinates without changing those magnitudes.
Use section 1.12, `matrix.singular-values`, bookmark generation 6, subject to
the live registry. Introduce orthogonal matrices and the unit sphere locally.
Do not write an SVD algorithm or promise continuous singular-vector choices
through repeated or zero singular values.

## Exact model and controls

Use existing `MathObjects::Linear`, level 3, through `ObjectLesson`:

```text
A(s) = diag(2,s,1),       -2 <= s <= 2, step 0.1
A^T A = diag(4,s^2,1)
singular values = {2, |s|, 1}, sorted in descending order
det(A) = 2s, geometric volume factor = 2|s|
rank(A) = 3 if s != 0, else 2
A = U Sigma V^T
stages: I; V^T; Sigma V^T; U Sigma V^T
```

Expose `Scale` labelled `Middle diagonal entry s`, initial 1, and `SvdStage`
with the existing four named choices, initial 3. Set A00=2, A22=1 and all six
off-diagonal entries to zero in every preset, including Shear=0. Keep the probe
at v=(0,0,1); it is not part of this exercise. Explicitly set all nine entries
and all vector components even when their values match model defaults.

Use metric selectors `Signed determinant`, `Volume`, `Rank`, `Singular value 1`,
`Singular value 2`, `Singular value 3`, `SVD reconstruction error`; matrix
selectors `A`, `U`, `V transpose`. Sigma is defined from the three displayed
singular values. The existing owner computes the factorization and stage mesh.

The surface is the unit sphere transformed by the selected stage. The grey
cube and teal final A-cage provide fixed references while stages change. The
stage basis arrows differ from A's final basis arrows until the last stage.
The determinant/volume/rank readouts always describe A, even at stage I; label
that distinction clearly. With s=0 the last surface is a flat ellipse.

| Preset | s | Stage | Singular values | det | Rank |
| --- | ---: | ---: | --- | ---: | ---: |
| Final map / default | 1 | 3 | (2,1,1) | 2 | 3 |
| Compress | 0.5 | 3 | (2,1,0.5) | 1 | 3 |
| Reflect | -1 | 3 | (2,1,1) | -2 | 3 |
| Collapse | 0 | 3 | (2,1,0) | 0 | 2 |
| Repeated largest stretch | 2 | 3 | (2,2,1) | 4 | 3 |

Reset restores s=1 and stage 3. Changing stage changes the illustration only;
it must not call Check. Independent practice begins at s=1, stage 3 and asks
for exactly two surviving stretch directions. The existing level-3 judge checks
rank two and third singular value <1e-7. In this family only s=0 passes. Stage
selection does not alter that judgment. Live values are intentionally visible.

At repeated singular values, individual U/V columns may change signs, order,
or basis within a repeated subspace. Do not test one preferred orientation.
Check orthogonality and reconstruction. Reversal can occur in orthogonal
factors; it does not make a singular value negative.

## Reading

Define singular value, SVD and orthogonal matrix. Explain the four stages and
the sign-versus-magnitude distinction using the fixed cases. Include a live
caption and three written checks with separate hint/answer/solution:

1. s=-0.5: give singular values (2,1,0.5), determinant -1 and volume factor 1.
2. s=0: explain why exactly one stretch vanishes and rank is two.
3. s=1: explain why repeated singular values need not determine unique U/V
   columns; the reconstructed map remains A.

## Files and checks

New: `src/runtime/textbook/SvdLesson.cpp` and `tests/svd_lesson_tests.cpp`.
Shared writes are the same narrow registry, CMake and documentation points as
the earlier items. No shared UI or numerical-kernel change is planned.
Create `paths_svd_lesson_tests`. Sweep all 41 scales and 4 stages (164 states).
Use the explicit eigenvalues of A^T A as the independent singular-value oracle;
allow 1e-9 for these diagonal singular values and 1e-5 for reconstruction/mesh
coordinates. Check U^T U=I and V^T V=I without fixing vector signs.

Check stage zero against the unit sphere, orthogonal-stage point lengths,
final-stage vertices against (2x,sy,z), finite normals and bounded meshes,
the collapsed state, repeated values and s=-2. Verify only scale changes affect
the matrix, stage changes do not grade, practice remains independent, and all
earlier reading bookmarks/lessons still work. Reject unavailable controls.

Build/run the new target with Batch 01's baseline and both completed earlier
batch targets when shared files change. Register/review/run:

```sh
./b/math_lab --validate --book --section 12 --book-view together
./b/math_lab --validate --book --section 12 --book-view reading
./b/math_lab --validate --book --section 12 --book-view figure --resolution 800x600
./b/math_lab --validate --book --section 12 --book-exercise
```

Expected stable ID: `matrix.singular-values`; bookmark_io=0; practice feedback=0
before Check. User launch:
`/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 12`.
Visual action: compare sphere and final-map stages, then move s through zero
and negative values. Singular values stay nonnegative while orientation can
reverse. Set s=0 in Exercise and Check. Finish the batch after delivery.
