# Pilot assignment — determinants as signed volume

Status: IMPLEMENTED as [P051](../P051_DETERMINANT_VOLUME.md); user visual review
pending. This original brief records the mathematical contract. The actual
implementation uses reusable `ObjectLesson.hpp/.cpp` instead of the initially
proposed `LinearVolumeLesson` holder. The section registry now drives counts and
bookmark migration. For new work use [Batch 01](BATCH_01.md).
Repository: `/Users/kogaryu/iggy3d/paths`
Suggested worker: GPT-5.6 Terra, High reasoning, one agent, one complete lesson.
Follow [the workflow](../LESSON_AUTHORING_WORKFLOW.md). No images, screenshots,
previews, native windows, offscreen captures or font-rasterization tests.
Leave changes uncommitted; source cards, Library and motion work are read-only.

## Learning outcome and bounded scope

Build **Determinants as signed volume** in the accepted four-view shell.
The learner changes a shear and a vertical scale, predicts the volume and
orientation of the transformed unit cube, and explains why a zero determinant
can mean a collapsed plane.

Append the lesson after the current eight sections as **1.9**, with stable ID
`matrix.determinant-volume`. Preserve existing IDs, printed order and reading
positions. Recheck the live section list before reserving this location.
The next chapter, arbitrary 3x3 editing, SVD, eigenvectors, new persistence and
new numerical decomposition algorithms are out of scope.

The first object-to-textbook adapter is included in this assignment. It is a
new integration using an existing mathematical model, not a content-only task.
No new renderer, camera implementation or determinant algorithm is needed.

## Read these existing implementations

- `docs/P049_LIVE_TEXTBOOK_FIGURES.md`: shared pane/provider pattern.
- `docs/P050_SYSTEM_SOLUTION_SETS.md`: complete lesson and disclosure example.
- `src/runtime/textbook/SystemsReading.cpp`: typed reading blocks and references.
- `src/runtime/textbook/Textbook.*`: navigation, state and bookmark ownership.
- `src/ui/TextbookFigureUi.*`: current plane providers and shared viewport.
- `src/runtime/math_objects/MathObjects.*`: Linear model, level 0.
- `src/scene/MathObjectScene.*`: snapshot-to-scene adapter and camera.
- `tests/math_object_tests.cpp`: existing shear/scale determinant and rank cases.

These examples are separately authored. Do not claim they reproduce a source
card or write results into source-card records.

## Mathematical meaning

Use the existing level-0 Linear model with

```text
       [ 1  k  0 ]
A  =   [ 0  s  0 ]
       [ 0  0  1 ]

(x,y,z) -> (x + k*y, s*y, z)
det(A) = s
geometric volume = abs(s)
rank(A) = 3 for s != 0, and 2 for s = 0
```

The reference cube is `[0,1]^3`, with volume one. The grey cage is the original;
the coloured cage is its image. Basis vectors Ae1, Ae2 and Ae3 use the existing
colours. Orientation is preserved for s>0, reversed for s<0, and degenerate at
s=0. Negative determinant does not mean negative geometric volume.

Use `MathObjects::dispatch` and its snapshot as the live mathematical owner.
Do not recompute determinant/rank in UI code. Reuse existing parameter validation,
step quantization, geometry bounds and rank tolerance; this restricted family has no nonzero scale
closer to zero than 0.1, so its supported cases avoid a near-singular ambiguity.

| Control | Range / step / initial | Meaning |
| --- | --- | --- |
| Shear k | -1.2 to 1.2 / 0.1 / 0 | Slides x in proportion to y; preserves det and volume |
| Vertical scale s | -2 to 2 / 0.1 / 1 | Changes volume, orientation and collapse |
| Reset example | k=0, s=1 | Restores this example through owner actions |
| Camera controls | Existing orbit / pan / zoom | Presentation only |

## Fixed cases and independent expected results

| Case | k | s | det | volume | rank | Orientation |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| Unit cube | 0 | 1 | 1 | 1 | 3 | Preserved |
| Shear | 1 | 1 | 1 | 1 | 3 | Preserved |
| Stretch | 0 | 2 | 2 | 2 | 3 | Preserved |
| Reflection | 0.6 | -1 | -1 | 1 | 3 | Reversed |
| Collapse | 0.6 | 0 | 0 | 0 | 2 | Degenerate |

The independent oracle is the upper-triangular diagonal product `1*s*1`, with
volume `abs(s)`. Check mapped basis vectors directly from `(x+k*y,s*y,z)`.
A sheared cube need not retain right angles; do not call every result a cube.

## Reading and exercise

Use a short introduction, definitions of signed determinant and volume scaling,
an orientation explanation, three worked comparisons, the live figure, three
written checks with separate hint/answer/solution disclosures, and a summary.
Keep the distinction between a general theorem and this two-parameter example.

Written checks (ungraded):

1. k=1, s=2: determine determinant and volume. Both are 2.
2. k=-1, s=-0.5: determine orientation and volume. Reversed; volume 0.5.
3. Explain why any supported k with s=0 gives a plane rather than a point.
   Ae1 and Ae3 remain independent and Ae2 is a multiple of Ae1.

The Exercise tab reuses the existing level-0 Linear manipulation challenge:
**make the image collapse into a plane**. The existing `MathObjects` Check
operation judges it. Use a distinct retained practice instance initialized to
k=0, s=1, so browsing an already collapsed teaching example does not manufacture
an exercise success. Expose the target and live metrics intentionally; this is a
manipulation exercise, not a hidden-answer prediction quiz. Changing reading
modes or the camera must not call Check. No new score/history system is needed.

## Integration scope and writable files

Original planned files (actual ownership is documented in P051):

- `src/runtime/textbook/DeterminantLesson.cpp`: typed reading content.
- `src/runtime/textbook/LinearVolumeLesson.hpp` and `.cpp`: small holder/adapter
  for existing exploration and practice MathObjects instances; no parallel math.
- `tests/determinant_lesson_tests.cpp`: behavior and independent certificates.
- One checkpoint document for the implemented lesson; select its P-number from
  the live workstream ledger when implementation starts.

Permitted existing files, for the stated purposes only:

- `src/runtime/textbook/Textbook.hpp`, `Textbook.cpp`, `MatrixChapter.cpp`:
  register the section, explicit exercise binding and retained object owners;
  update section counts and migrate existing eight-section bookmarks.
- `src/ui/TextbookUi.*`, `TextbookFigureUi.*`: route the new binding; reuse the
  same layout, viewport/camera and NativeMath adapter. Generalize the viewport
  helper to consume a supplied MathObjectSnapshot if needed.
- `app/math_lab_main.cpp`: section bounds, object-exercise startup and text-only
  validation. Do not call `book.board()` for an object-only section.
- `CMakeLists.txt`: add only the new sources and text-only test targets/cases.
- `tests/textbook_tests.cpp`: preserve bindings and expand bookmark/reference
  coverage. Do not remove assertions for existing sections or source cards.
- `README.md`, `docs/ARCHITECTURE.md`, `docs/WORKSTREAMS.md`: concise current
  documentation, preserving other workstreams' changes.

P051 replaces the previous `card == 0` special case with explicit exercise
bindings and keeps the existing board/system routes intact. Object lessons
have no source-card board.
No edits are planned to MathObjects, the scene adapter, LessonSpread, NativeMath,
the native host, shaders, third-party code, source cards or regular-question UI.
If an actual reusable-model defect prevents this bounded lesson, report the
specific defect and proposed minimal change while finishing independent work.

## Verification and delivery

Create the pure target `paths_determinant_lesson_tests`. It should verify the
five cases, control limits/rejection atomicity, snapshot-to-geometry consistency,
finite bounded geometry for every supported k/s pair, independent practice
state, Check behavior, and no automatic check from navigation. There are only
25*41=1,025 supported pairs; this is a bounded mathematical state sweep.
Do not rasterize fonts or create a native host in tests.

After adding the new target, run these commands from the Paths repository root:

```sh
cmake -S . -B b-lesson-headless -DPATHS_BUILD_NATIVE=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build b-lesson-headless --target paths_determinant_lesson_tests paths_textbook_tests paths_textbook_figure_tests paths_system_lesson_tests paths_matrix_board_tests paths_math_object_tests -j 6
ctest --test-dir b-lesson-headless -R '^(paths_determinant_lesson_tests|paths_textbook_tests|paths_textbook_figure_tests|paths_system_lesson_tests|paths_matrix_board_tests|paths_math_object_tests)$' --output-on-failure
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b --target math_lab -j 6
```

Add and review these text-only launch paths before executing them. They must
exit before native/typesetter/bookmark initialization:

```sh
./b/math_lab --validate --book --section 9 --book-view together
./b/math_lab --validate --book --section 9 --book-view reading
./b/math_lab --validate --book --section 9 --book-view figure --resolution 800x600
./b/math_lab --validate --book --section 9 --book-exercise
```

Each must identify `matrix.determinant-volume` and zero bookmark I/O. The
teaching model begins at determinant 1, volume 1, rank 3; reading-only publishes
no scene, and figure-only publishes bounded nonempty geometry. Exercise starts
from its independent unit-cube instance with Check unperformed. Use the new
pure target for the five presets and manipulation checks; do not invent object
CLI flags merely for this task. Verify an actual eight-section bookmark
migration in `paths_textbook_tests`, preserving old section IDs and positions.

Expected user launch, after implementation:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 9
```

User visual test: vary k at s=1 (shape changes, volume stays one); move s through
positive, zero and negative (stretch, collapse, reflection); switch to Exercise
and make the practice image collapse, then Check. Existing RREF and systems
lessons must retain their accepted four-view behavior.

Report what was built, exact checks passed, any material limitation, the launch
command and those visual actions. Automated completion does not claim a visual
inspection. Leave changes uncommitted and do not start the next lesson.
