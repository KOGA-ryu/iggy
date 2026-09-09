# Eigenvector directions

Status: ready implementation brief; first item in [Batch 01](BATCH_01.md).
Follow that file's ownership, no-image, validation and continuation rules.

## Learning outcome and scope

Explain why a nonzero vector can reverse direction and still be an eigenvector.
Find the invariant direction in a one-parameter family. Use section 1.10,
`matrix.eigenvector-directions`, bookmark generation 4, subject to the live
registry. Prerequisites: matrix columns, multiplication and signed scaling.
Do not introduce an eigensolver, arbitrary matrix editing or a composition lesson.

## Exact model and controls

Use existing `MathObjects::Linear`, level 1, through `ObjectLesson`:

```text
A = diag(2, 1, -1)
v(t) = (t, 0, 1),     -2 <= t <= 2, step 0.05
A v = (2t, 0, -1)
lambda_candidate = (v dot A v)/(v dot v) = (2t^2 - 1)/(t^2 + 1)
||A v - lambda_candidate v|| = 3|t|/sqrt(t^2 + 1)
```

Expose only `VectorX`, labelled `t in v = (t, 0, 1)`, initial 1. Fix VectorY=0,
VectorZ=1 and ComposeAngle=0 in every preset. Set all nine matrix parameters
to the displayed diagonal matrix; in particular Shear=0 and Scale=1.
The vector is never zero. Only t=0 gives an eigenvector in this family, with
eigenvalue -1. The matrix also has eigenvalues 2 and 1, outside this probe family.

Use metric selectors `Vector length`, `Length of A v`, `Eigenvalue candidate`,
`Eigenvector residual`; matrix selector `A`. Do not call the candidate a true
eigenvalue when its residual is nonzero. Computed readouts come from the owner.
The existing judge requires vector length >0.1, image length >0.01 and residual
<0.01. With the supported 0.05 steps, only t=0 passes; nearest nonzero t has
residual about 0.1498. Explain the numerical check separately from the definition.

The gold arrow is v and the teal arrow is Av. The violet composition uses B=I,
so BAv and the composed cage coincide with the A image. State that overlap in
the caption; do not suggest composition is being varied. The three coloured
basis arrows describe A independently of the probe.

| Preset | t | v | Av | Candidate | Residual | Check |
| --- | ---: | --- | --- | ---: | --- | --- |
| Mixed direction / default | 1 | (1,0,1) | (2,0,-1) | 1/2 | 3/sqrt(2) | Fail |
| Invariant direction | 0 | (0,0,1) | (0,0,-1) | -1 | 0 | Pass |
| Opposite horizontal component | -1 | (-1,0,1) | (-2,0,-1) | 1/2 | 3/sqrt(2) | Fail |

Reset restores t=1. Exploration presets and camera changes never check or reset
practice. Practice begins at t=1 and asks the learner to make Av parallel to the
line spanned by v, then Check. Live candidate/residual are intentionally visible.

## Reading

Include definitions of eigenvector, eigenvalue and invariant direction, the
nonzero-vector requirement, the candidate-versus-certificate distinction, the
three examples and a captioned figure. Three ungraded written checks:

1. t=0: give lambda and explain why reversal still lies on the same line. -1.
2. t=1: show no single scalar can send (1,0,1) to (2,0,-1).
3. For this diagonal A, identify two other eigenvectors and their eigenvalues:
   e1 with 2 and e2 with 1. These are written examples outside v(t).

Provide separate hints, answers and solutions, stable block references and a
summary. No proof completion or source-card result is recorded.

## Files and checks

New: `src/runtime/textbook/EigenvectorLesson.cpp` and
`tests/eigenvector_lesson_tests.cpp`. Use the P051 content/specification pattern.
Shared writes: the section factory declaration in `Textbook.hpp`, one registry
entry in `MatrixChapter.cpp`, CMake registrations, necessary reference/bookmark
tests, README, architecture/workstreams, this queue row and one checkpoint doc.
No shared UI or model implementation change should be needed.

Create `paths_eigenvector_lesson_tests`. Verify the three examples and both t
boundaries; sweep all 81 ticks using the analytic formulas above with 3e-6
absolute readout tolerance. Verify mapped-vector arrow endpoints, finite bounded
meshes, unsolved default practice, only t=0 passing, rejection atomicity,
navigation/help separation and existing bookmark migration. Reject changes to
unexposed matrix/vector parameters. Require every metric/matrix selector to
resolve in the current model; P051 validates selectors on construction.

Build/run the new target with Batch 01's baseline targets using exact names.
Register/review/run:

```sh
./b/math_lab --validate --book --section 10 --book-view together
./b/math_lab --validate --book --section 10 --book-view reading
./b/math_lab --validate --book --section 10 --book-view figure --resolution 800x600
./b/math_lab --validate --book --section 10 --book-exercise
```

Expected stable ID: `matrix.eigenvector-directions`; bookmark_io=0; practice
feedback=0 before Check. All modes begin with the prescribed t=1 example.
User launch: `/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 10`.
Visual action: move t from 1 to 0; Av and v become opposite on one line and the
residual reaches zero. Repeat in Exercise and Check. Continue to the second
queue item only after this lesson is installed and its named checks pass.
