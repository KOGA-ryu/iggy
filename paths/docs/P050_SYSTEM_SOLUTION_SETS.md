# P050 — one, infinitely many, or no solutions

Section **1.3, One, infinitely many, or no solutions**, follows RREF in the
accepted four-view textbook format. Open it with:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 3
```

The five teaching presets show a unique point, a common line, parallel separated
planes, coincident planes, and three pairwise-intersecting planes with no common
point. Coloured equations follow the plane colours; gold appears only for a
nonempty common solution set. Right-hand-side sliders translate planes, and
coefficient sliders change their orientation. Givens move in quarter steps
between -3 and 3. Editing a given starts a new reduction. Next pivot, Undo and
manual row operations transform the whole augmented system.

The controls retain orbit, pan, zoom, original-plane overlays, normals, labels,
solution probes and grid settings. Original equations can be inspected beside
the current geometry. When a solution lies far from the origin, a labelled
uniform display scale keeps the finite grid windows useful. Grids do not bound
the actual infinite planes or solution families.

## Reading and practice

Eleven typed reading blocks introduce consistency, coefficient and augmented
rank, nullity, free variables and the affine family xp + ker(A). The translation
argument has a separately opened proof. The chapter now has eight sections and
29 index terms. Earlier section IDs remain unchanged; version-1 bookmarks from
the seven-section chapter migrate by those IDs, preserving existing positions
and initializing only the new section's scroll position.

Exercise opens three neutrally named systems. Before a prediction is submitted,
only the given equations and the learner's choices are published. A prediction
requires both an outcome and a reason. Submit reveals the explanation; Reveal
opens it without fabricating an attempted answer. Afterward, the learner can
reduce the system and check a proposed point against every original equation.
A point check establishes membership only, not completeness of a solution set.

Exploration and practice keep separate boards while switching reading modes.
Selecting or restarting a practice system resets its current work; prior
predictions remain in session history, and previous answer exposure stays
recorded for that system. No attempt or figure setting is persisted in the
reading bookmark. Source exercise cards and their existing boards retain their
previous owners and data.

## Reuse and numerical contract

The shared `LessonSpread` and native scene pipeline are reused. The new
`AffinePlanes` binding reuses `RowPlaneFigure` with an explicit right-hand side;
no second plane renderer, pane layout or camera was introduced.
`SystemLesson` owns authored givens and practice judgments. Its boards use the
existing `MatrixBoard` row-operation, transformation and Undo kernel.

`equationSpace` analyzes at most three real equations in three variables with
normalized, twice-reorthogonalized row constraints. It uses relative rank and
consistency tolerance 1e-10, constructs a minimum-norm particular solution and
an orthonormal null basis, and compares equivalent affine spaces at 1e-8.
A zero coefficient row with an exactly nonzero right-hand side is a
contradiction; neither that row nor a vacuous zero equation becomes a plane.
Nullity is always 3-rank(A), and is not presented as the dimension of an empty
solution set. This is bounded numerical teaching software, not an exact solver
for arbitrary ill-conditioned systems.

For authored systems, `MatrixBoard` clears tiny cancellation residues using
an entrywise bound from its recorded row transform E and the original inputs:
8*(operation count + 1)*machine epsilon*sum(abs(E[r,k])*abs(input[k,c])).
The same rule applies to b. A deliberately scaled nonzero row scales its bound
too. Cleanup is reported in the operation status and does not alter the printed
source-card lanes. A failing integer-system regression exposed the need for
this policy before installation.

## Text-only verification

Verification uses compilation, pure model/CPU mesh checks and `--validate`.
No window, screenshot, image, preview, font rasterization or native host is
executed. The native-math rasterizing tests are deliberately excluded.

The targeted check set contains 42 tests: the prior textbook, figure, board and
object checks, the new systems suite, and seven new systems CLI cases. The new
suite independently checks 485 systems against exact integer minors and affine
solution certificates, then inspects 516 CPU mesh states. Geometry reaches
3,628 vertices and 10,944 indices, within the existing limits. Checks include
right-hand-side-only cache invalidation, guided and manual row invariance,
contradictory zero rows, all nullities, deliberate small row scaling, redacted
practice, retained assistance and invalid-action atomicity. The prior figure
suite still checks 153 geometry states and 1,008 responsive layouts. Bookmark
migration is exercised by the existing textbook suite.

```sh
ctest --test-dir /Users/kogaryu/iggy3d/paths/b \
  -R '^(paths_system_lesson_tests|paths_textbook_figure_tests|paths_textbook_tests|paths_matrix_board_tests|paths_math_.*_cli)$' \
  --output-on-failure
```

The user accepted P049's layout and interaction before this work and
subsequently reported that P050 "looks good." This records the user
feedback; no agent visual inspection was performed. Changes remain
uncommitted. No source problem page, Library or motion-lesson file is changed.
