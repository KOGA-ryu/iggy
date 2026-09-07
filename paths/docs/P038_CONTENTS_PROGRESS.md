# P038: current-attempt progress in Contents

Status: Automated Green; user visual confirmation pending; uncommitted.

Authority: the user approved small question-progress symbols and chapter
completion counts, derived from the existing saved working. This is one new
Contents capability. The preceding input cleanup and matrix chapter are retained.

## Player behaviour

- An empty grey circle means the current attempt has no answer or help evidence.
- A cyan dot means the current attempt is in progress. Wrong answers, help and
  partial answer collections count; opening a problem, choosing a move or
  browsing a reference does not.
- A green tick means the current attempt is complete, including assisted
  completion. It is a progress mark, not a mastery or independent-solution score.
- Each chapter shows completed / total, such as `5/12`. The denominator includes
  every playable question in every type of that chapter. Selection checkboxes,
  random previews and the active queue do not change these totals.

Undo can change a green tick back to cyan, even when it returns to the original
working. Replay resets that question to an empty circle while retaining its
archived run. Starting a new set resets the selected attempts; other questions'
marks remain. Reopening reconstructs the same marks from saved evidence.
New questions begin unstarted, and reordering preserves marks by question ID.

## Canonical owners

`LayeredQuestionSession::progress()` is the read-only classification owner. It
reads current completion, mathematical move events or prepared answer/help
records. It ignores archived runs and performs no mutation. It reads only the
current attempt, with no scan of the full saved command history.

`EquationSorterSession::view()` projects that state by question home and sums
completed/total by the existing chapter IDs. It includes retained questions
outside the active queue and selection. The resulting arrays are view data;
there is no new mutable progress store, save field or save-format change.

`EquationSorterUi` draws circles and ticks using native lines and circles.
Markers occupy 16 pixels beside the existing question checkboxes. Shapes as
well as colours distinguish states, and hovering a question gives its meaning.
Chapter titles wrap within the space reserved beside their counts. Fonts and
buttons retain their compact sizes; the solving workspace is unchanged.

Production changes: six existing C++ files, **+52/-2 lines, net +50**, with no
new production files. Two existing tests change by **+111/-3, net +108**.
All question content, generators, persistence code and build definitions retain
their pre-turn bytes. This document is the only added file.

## Verification

The current CMake graph supplies the affected targets. Release builds of
`sorter`, `gallery` and `paths` pass. Five targeted CTest entries pass:
`paths_sorter_input_tests`, `paths_sorter_solve_tests`,
`paths_practice_save_tests`, `paths_practice_save_write` and
`paths_practice_save_read`.

The model/save gate covers wrong-first answers, help-only work, partial
collection, scalar and matrix completion, Undo, repeat completion without double
counting, Replay with retained archives, selected-only restart, idle reads and
catalogue growth/reordering. It also derives progress from a real P036 save.

The actual ImGui input gate checks the visible symbols, chapter text wrapping,
counts and question labels at 1440×860, 800×600 and 360×480. It exercises prepared
completion, a fresh set and restored mathematical work through the existing
controls. It retains the prior matrix, reference, focus, held-input, Next and
fixed-workspace checks. No pixels were captured and no window was launched.

The rebuilt executable validates all 100 bundled catalogue records from
`/private/tmp`, before graphics startup. Text logs, scoped changes and a final
receipt are under `build/contents-progress-evidence/`. The initial test runs
exposed two test-assumption errors: navigation setup skipped the existing return
boundary, and a geometry assertion assumed eight pixels outside an ImGui
Selectable's expanded bounds. Both tests were corrected, and the failing gates
passed. Already-passing gates were not repeated.

## User visual check and next candidate

Launch `./b/sorter`. In Contents, find an empty grey circle and its chapter's
count. Attempt a question, then return: its dot should be cyan. Finish it and
return: its tick should be green and the chapter count should increase by one.
Undo should restore cyan; Replay should restore an empty circle. Close and reopen
once to check that the same marks and counts return.

The next candidate is a compact unfinished-question filter in Contents, using
this same read-only projection. It would change the future selection only;
the running queue would keep its existing explicit Start/Resume boundary.
Stop after this capability and leave visual acceptance to the user.
