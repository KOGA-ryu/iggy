# P051 — determinant lesson and reusable object bindings

Section **1.9, Determinants as signed volume**, joins reading, a movable unit-cube
image, live matrix/readouts and separate practice in the accepted four-view shell.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 9
```

Choose Unit cube, Shear, Stretch, Reflection or Collapse. The two controls change
k and s in A=[[1,k,0],[0,s,0],[0,0,1]]. The model supplies determinant s, volume
|s| and rank three except at s=0, where two independent directions remain.
Three written checks have separate hints, answers and solutions. Exercise starts
from its own unit cube and uses the existing MathObjects collapse checker.
Reading, teaching presets and camera motion never submit that exercise.

## Repeated work removed

`ObjectLessonSpec` chooses an existing MathObjects kind/level, controls, presets,
conventions and visible measurements. `ObjectLesson` holds distinct exploration
and practice instances and sends mutations/checks through MathObjects. Presets
apply atomically; unavailable controls and invalid readout selectors fail.
The common figure UI handles live matrices, step controls, labelled discrete
choices, metrics, Check, reset and the existing viewport/camera.

`BookExerciseKind` explicitly distinguishes source boards, systems practice and
object practice. An object section cannot silently borrow a source card's board.
The registry now determines section counts, reading/help storage, contents,
navigation and CLI bounds. Stable IDs and immutable introduction generations
allow complete older bookmarks to migrate; partial/duplicate/unknown records
fail without changing live state. Seven- and eight-section fixtures are covered.

The shared scene is reset when the figure binding or practice/exploration owner
changes. Separate owners can have equal local revision numbers; this prevents
an earlier owner's mesh from being reused for the new example.

## Validation

Native `math_lab` compilation and 48 targeted text-only checks passed: six pure
suites plus 42 reviewed `--validate` CLI cases. The new determinant suite covers
five exact examples and every one of the 1,025 supported shear/scale pairs, with
independent diagonal-product, rank and mapped-basis certificates. Its meshes
stay within 1,719 vertices and 5,136 indices. It also checks independent practice,
rejection atomicity, help disclosure, navigation and a second existing model
through the same adapter. The existing RREF/system and 1,008 layout cases pass.

The Library's `paths_sorter_ui` consumer is compile-checked against the updated
textbook interface. Tests that initialize fonts/typesetting are excluded from
execution. No images, windows, previews, captures or rasterization are used.

## User visual test

1. Keep s=1 and move k: the solid slants while volume stays one.
2. Move s through positive, zero and negative: stretch, flatten and reflection
   agree with the determinant, volume and rank readouts.
3. Open Exercise, set its independent scale to zero, and Check. Return to reading
   and verify that its teaching example remains separate.

Visual/pointer acceptance remains for the user. Practice feedback lasts for this
session; reading bookmarks do not restore or manufacture exercise results.
Changes remain uncommitted. Original source pages and regular-question ownership
are unchanged.

## Ready queue

[Batch 01](lesson_tasks/BATCH_01.md) contains three filled implementation briefs:
eigenvector directions, orthogonal projection and singular-value stretching.
Each uses the new adapter and an existing Linear learning layer. Thirty-four
existing-model text-only preflight cases agree with the briefs' analytic readouts
and Check outcomes. This validates the chosen examples, not unbuilt textbook
lessons. The queue includes a copyable sequential start instruction; it does not
launch another task or change the user's model setting.
