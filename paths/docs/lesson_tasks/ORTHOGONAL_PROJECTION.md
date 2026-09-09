# Orthogonal projection and residual

Status: ready implementation brief; second item in [Batch 01](BATCH_01.md).
Follow that file's ownership, no-image, validation and continuation rules.

## Learning outcome and scope

Split a vector into its projection on a plane and a perpendicular residual.
Explain why a zero residual means membership in the plane. Use section 1.11,
`matrix.orthogonal-projection`, bookmark generation 5, subject to the live
registry. Define dot product and orthogonality locally if needed. Do not add a
least-squares solver, arbitrary column editing or dependent-column algorithms.

## Exact model and controls

Use existing `MathObjects::Linear`, level 2, through `ObjectLesson`:

```text
A = [[1,0,0], [0,1,0], [1,0,0]]
S = span((1,0,1), (0,1,0)) = {(x,y,z): x=z}
v(t) = (t,0,1),       -2 <= t <= 2, step 0.05
p = projection_S(v) = ((t+1)/2, 0, (t+1)/2)
r = v-p = ((t-1)/2, 0, (1-t)/2)
||r|| = |t-1|/sqrt(2),      ||p|| = |t+1|/sqrt(2)
P = [[1/2,0,1/2], [0,1,0], [1/2,0,1/2]]
```

Expose only `VectorX`, labelled `t in v = (t, 0, 1)`, initial 0. Fix VectorY=0
and VectorZ=1 in every preset. Explicitly set all nine matrix entries: Shear=0,
Scale=1, A20=1, A22=0, with the other entries as displayed. The first two columns
are independent. The third column does not define the projection subspace.

Use metric selectors `Vector length`, `Subspace dimension`, `Residual length`,
`Projected length`, `Orthogonality error`; matrix selectors `A`, `Projection P`.
The owner uses Gram-Schmidt on the first two columns with its existing 1e-6
threshold. This fixed pair is well separated from dependence. Use analytic P
as the independent certificate. Allow 3e-6 absolute error for float projections.

The gold vector is v; the teal vector is its projection; the gold connector
from p to v is the residual. Blue lines show spanning directions, not a drawn
infinite plane. At t=-1 the projection is zero and its arrow can disappear.
The cube image is flattened by A; do not confuse that cage with the projected
probe or assert that A itself equals P.

| Preset | t | p | r | Residual length | Check |
| --- | ---: | --- | --- | --- | --- |
| Outside the plane / default | 0 | (1/2,0,1/2) | (-1/2,0,1/2) | 1/sqrt(2) | Fail |
| In the plane | 1 | (1,0,1) | (0,0,0) | 0 | Pass |
| Normal to the plane | -1 | (0,0,0) | (-1,0,1) | sqrt(2) | Fail |

Reset restores t=0. Independent practice asks the learner to put v in S, then
Check. The existing judge requires vector length >0.1 and residual <0.01. The
vector is always nonzero; with 0.05 steps, only t=1 passes. Its nearest other
tick has residual about 0.03536. Live measurements are intentionally visible.

## Reading

Define orthogonal projection, residual vector and column span. Explain
v=p+r, p in S and r perpendicular to both spanning columns. Include the three
comparisons, the distinction between A and P, a caption, and three written
checks with separate hints, answers and solutions:

1. t=0: calculate p and r from the displayed formulas.
2. Show r dot (1,0,1)=0 and r dot (0,1,0)=0 for every t.
3. t=-1: explain why projection can vanish even though v is nonzero.

The finite figure illustrates this fixed subspace. Do not call the bounded
geometry the entire infinite span or the visual probe a proof checker.

## Files and checks

New: `src/runtime/textbook/ProjectionLesson.cpp` and
`tests/projection_lesson_tests.cpp`. Shared writes are the same narrow registry,
CMake and documentation points as item 1; no shared UI or model change is planned.
Create `paths_projection_lesson_tests`. Check all 81 supported t values against
analytic P, p and r, residual orthogonality, v=p+r, finite geometry and Check.
Use actual snapshot/arrow endpoints, not the production projection routine as
its own oracle. Cover zero projection, zero residual, both boundaries, invalid
actions, independent practice, help disclosures and bookmark compatibility.

Build/run this target with Batch 01's baseline and the completed eigenvector
target when shared files change. Register/review/run:

```sh
./b/math_lab --validate --book --section 11 --book-view together
./b/math_lab --validate --book --section 11 --book-view reading
./b/math_lab --validate --book --section 11 --book-view figure --resolution 800x600
./b/math_lab --validate --book --section 11 --book-exercise
```

Expected stable ID: `matrix.orthogonal-projection`; bookmark_io=0; practice
feedback=0 before Check. User launch:
`/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 11`.
Visual action: move t from 0 to 1; the residual connector shrinks to zero. Move
to -1; the projection vanishes while v remains nonzero. Repeat t=1 in Exercise
and Check. Continue only after installation and the named checks pass.
