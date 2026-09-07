# P025: one persistent solving workspace

Status: implemented; Release build and targeted automated checks passed;
uncommitted. User feedback received: bring the question closer to the choices
and use symbolic operation labels. [P026](P026_COMPACT_SYMBOL_CHOICES.md) records
that follow-up; this is not blanket acceptance of visual polish. No screenshots
were taken.

The user approved the first implementation checkpoint from the study-flow
discussion: give the six existing prepared bracket questions a fixed workspace,
then build subject/chapter/question selection as a later capability. They chose
to retain finished working until pressing Next. Individual accepted solving
steps still advance immediately.

## Delivered behaviour

- An amber/gold original-problem board stays at the top for the whole question.
  Its question number counts prepared cards in the loaded pack, not completion.
- Current working has its own fixed strip directly below, marked in mint.
- The same scene viewport contains operation buttons, moving arithmetic targets
  with attached answer labels, and the completion controls. Its placement depends
  on window size, never on the active mathematical step or help visibility.
  The six prepared questions retain identical camera framing throughout.
- Hints, next moves, reached working and the final check share a fixed support
  panel beside the activity, or below it in narrow windows. Long support text
  scrolls within that panel. It cannot move the problem or activity.
- Finished working waits indefinitely. Next opens the following prepared card
  in pack home order while staying inside the workspace. It resumes that card's
  existing progress if previously opened. It skips cards without a prepared
  solution and stops at the last prepared card without wrapping or restarting.
- Replay, Back to groups, Resume, assistance and grouping Undo retain their
  original meaning. Returning after Next offers the last active question.

Opening a later card directly begins there; Next follows the remaining pack
order. This checkpoint does not create a chapter catalog, selected-question
queue, shuffle policy, or new mathematical content. The ordinal and last-card
message do not claim that earlier cards have been completed.

## Ownership and input boundary

`EquationSorterUi::solveLayout()` is the presentation-only layout calculation.
One ImGui root contains fixed child regions with shared keyboard navigation.
The old operation-versus-arithmetic body placement and separate answer legend
are replaced by the shared activity area and labels projected from the existing
scene bindings. Text fits or wraps without parsing mathematics.

`EquationSorterSession` owns Next eligibility and prepared-card order. Open and
Next call one preparation boundary; allocation succeeds before the previous
session is paused. Next requires the current question to be complete and a
current sorter revision. The existing per-card sessions retain all evidence.
`LayeredQuestionSession` still owns judging, reached working, assistance and
completion; `GallerySession` still composes question actions with scene picking.
Working history comes from the existing read-only review projection.

Native scripts expose the same Next action as `next_problem`. UI context changes
consume queued help/shots before another card can receive them. Since shooting
is judged on mouse press, the UI also consumes that held press until release:
an operation that appears immediately beneath the pointer cannot receive a
second answer from the same physical click.

## Verification

Derived from the current CMake graph, `sorter`, `paths_sorter_solve_tests` and
`paths_sorter_input_tests` built in the Release `b` directory. The two targeted
suites passed; no broad CTest run or graphical host was launched.

- The model gate walks all six questions through actual operation commands and
  projected sphere hits. It covers explicit Next, reversed input-file order,
  partially completed destination cards, stale/repeated Next, a minute of ticks
  with finished working retained, terminal refusal, answer evidence and Undo.
- Real ImGui input covers 1440×900, 800×600 and 360×480. It verifies the actual
  child-window rectangles stay fixed, the camera stays fixed, hints cannot move
  operation buttons, sphere labels stay inside the activity area, and targets
  remain at least 24 pixels wide. Long final checks remain reachable by scrolling
  only support. It also covers mouse release after a correct shot, keyboard Next,
  held Enter, stale queued help, focus loss, Replay and per-card Resume.
- The rebuilt executable validated the bundled bracket pack from `/private/tmp`
  with `--check-content`, which exits before graphics startup.

The first input checks exposed undersized narrow-window targets, a mouse release
reaching a newly appeared operation, and keyboard navigation between separate
root panels. The delivered layout, consumed-press handling and shared root fix
those cases. Final input evidence is `build/fixed-workspace-evidence/input-tests.log`.
The model pass is in `build/fixed-workspace-evidence/targeted-tests.log`; that
earlier combined invocation still records its subsequently repaired UI failure.
The final receipt distinguishes these gate results and records current hashes.

Five existing production C++ files change by **+220/-87 lines (net +133)**;
no production code files are added. Two existing test files change by
**+164/-12 lines (net +152)**. The existing sorter-grid UI, mathematical content,
gallery/scene owners and unrelated build-name changes retain their prior bytes.

## User visual checklist

Launch `./b/sorter --content content/sorter/bracket_practice_v1.json` from Paths.

- Open the first amber/gold card. The gold original-problem board should stay
  fixed while the working below its mint heading changes.
- Operation choices and labelled spheres should occupy the same activity space.
  Opening Hint or Show next move should leave that space and the problem still.
- Finish a question, wait, then press Next. The next equation should appear in
  the same workspace. Next is disabled after the final prepared question.
- In a narrow window, confirm the equations and sphere labels remain readable;
  scroll the support panel to read a long check without moving the problem.

Next candidate: subject and chapter selection with all, random or specific
questions feeding this workspace, following the recorded study-flow direction.
