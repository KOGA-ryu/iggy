# First reusable object-lesson batch

Status: three implementation briefs ready. Their textbook lessons are not built.
Repository: `/Users/kogaryu/iggy3d/paths`.
Foundation: [P051 determinant lesson](../P051_DETERMINANT_VOLUME.md).
Use [the authoring workflow](../LESSON_AUTHORING_WORKFLOW.md) and `AGENTS.md`.

The user performs the pilot's visual test. An explicit request to run this batch
authorizes its three lessons and sequential continuation; do not ask again after
each successful lesson. This file alone does not launch or change a model.
Suggested first run: Terra High, one direct builder, no subagents.

## Ordered queue

| Order | Assignment | Section / stable ID | Bookmark generation | State |
| --- | --- | --- | --- | --- |
| 1 | [Eigenvector directions](EIGENVECTOR_DIRECTIONS.md) | 1.10 / `matrix.eigenvector-directions` | 4 | Ready |
| 2 | [Orthogonal projection](ORTHOGONAL_PROJECTION.md) | 1.11 / `matrix.orthogonal-projection` | 5 | Ready |
| 3 | [Singular-value stretching](SINGULAR_VALUE_STRETCHING.md) | 1.12 / `matrix.singular-values` | 6 | Ready |

Before assigning printed numbers, check the live registry. Preserve all existing
IDs, generations and ordering. If another lesson has been appended, use the next
available printed number and generation, and update the affected brief/commands.
These are separately authored examples, not solved source-card exercises.

## Shared implementation contract

Read `DeterminantLesson.cpp`, `ObjectLesson.hpp/.cpp`, and the section records in
`MatrixChapter.cpp`. Each lesson should add a content/specification `.cpp`, one
section factory declaration in `Textbook.hpp`, one factory call in the registry,
and the required CMake source/test registrations. Use `BookFigureKind::Object`
and `BookExerciseKind::Object`. Do not call `book.board()` for these lessons.

`ObjectLessonSpec` chooses a MathObjects kind/level, controls, complete presets,
relationship, conventions and challenge. Optional `metrics` and `matrices`
selectors filter presentation by existing names; they do not recalculate values.
The common UI handles step controls and labelled choices, matrices, camera,
separate practice, Check and reset. Every preset starts from model defaults;
explicitly set all nine matrix entries and all fixed vector components.

The default practice state is preset zero. Make it unsolved. Teaching presets
cannot change practice. Check is the existing MathObjects judge; do not add a
second answer owner or a new scoring/persistence system. Typed reading blocks
provide three ungraded written checks with separate hint, answer and solution.

No renderer, native host, typesetter, object numerical-kernel, Library,
regular-question, motion, source-card or parent-project changes are planned.
The existing object model owns all computed geometry, matrices and measurements.

## Verification for each lesson

Run from the Paths repository root. Configure `b-lesson-headless` with
`-DPATHS_BUILD_NATIVE=OFF -DCMAKE_BUILD_TYPE=Release`. Build and run these pure
targets plus the new target named in the current brief:

```text
paths_determinant_lesson_tests
paths_textbook_tests
paths_textbook_figure_tests
paths_system_lesson_tests
paths_matrix_board_tests
paths_math_object_tests
```

Use an anchored CTest selector containing only these exact names and the new
target. Include completed earlier batch targets when a shared file changes.
The per-lesson brief supplies its numerical certificates, input sweep and exact
four text-only CLI cases. Register those cases with assertions for the stable
section ID, independent unchecked practice, hidden scene in Reading only and
nonempty bounded geometry in Figure only/Exercise. Never remove old assertions.

Build the installed `b/math_lab`. Check current and older bookmark fixtures,
retained source-board/system work, local references and the scoped diff.
P051 already covers 1,008 spread/figure layout combinations through the existing
test; reuse that target. Do not generate another layout matrix for unchanged UI.

**No images under any circumstances.** No image generation/viewing, screenshots,
native windows, previews, offscreen captures, font-rasterization probes or
unfiltered CTest runs. Never run `paths_native_math_tests`. Review the selected
commands: every `math_lab` launch by the builder must use the early `--validate`
path, before native/typesetter/bookmark initialization. CPU model and mesh
inspection is allowed. The user alone launches the visible app.

## Sequential execution and stop rules

1. Inspect dirty ownership and record hashes for files to edit. Preserve all
   unrelated work. Use a source staging copy when that avoids shared edits during
   development; merge live drift before installation.
2. Complete queue item 1 through reading, controls, practice, build and checks.
   Mark it `Built / user visual review pending` with its checkpoint link here.
3. Continue to item 2, then item 3 under the same batch authorization. Do not
   wait for separate visual approval between those three completed lessons.
4. Stop after item 3. Return one compact delivery with the three launch commands,
   a visual action for each, checks and material limits. Leave changes uncommitted.

Resolve routine code organization, prose and local formatting choices directly.
If a check fails, diagnose and repair it; keep working while the cause is clear.
Do not weaken tests. Stop the affected item for a missing mathematical convention,
an incompatible edit owned by another worker, or a failure requiring a new
numerical method or out-of-scope architecture change. State the exact issue and
smallest resolution. Do independent authorized work when it remains useful;
never label unfinished or unchecked lessons complete.

Keep evidence in one checkpoint document per lesson and this queue's status row.
Do not create an additional diary, scheduler, generator, task or status ledger.

## Copyable start instruction

```text
Run the three lessons in /Users/kogaryu/iggy3d/paths/docs/lesson_tasks/BATCH_01.md
in order, using its linked briefs and the lesson authoring workflow. Complete,
install, build and verify each lesson before continuing to the next. You are
authorized to continue through all three without asking between successful
lessons. Work directly, preserve unrelated changes, and leave work uncommitted.
Do not take, generate, view or render images; do not launch windows or previews;
never run paths_native_math_tests. Use only the named pure checks and reviewed
--validate commands. Stop after the third lesson and give me the launch commands
and a short visual test for each.
```
