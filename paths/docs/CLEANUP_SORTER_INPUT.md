# Cleanup: one pending input action

Authority: the user approved a bounded code cleanup while the separate matrix
chapter task continued. This work changes queued control input only.

## Ownership and removal

`EquationSorterUiState` now holds one optional action: either a sorter action
or a command for the active question. `beginEquationSorterFrame` remains the
single consumer, dispatching to the existing sorter or gallery owner and then
clearing the action. The question owner still judges the mathematics.

The separate `pendingGame` slot is removed, along with its second dispatch
block, repeated clearing and duplicated checks for two occupied slots. The
interface can no longer retain navigation and a question command together.
Existing navigation replacement, first-action guards, focus-loss cancellation,
consumed mouse presses and explicit Next retain their previous behaviour.
Scene picking still follows its existing mouse-press route.

Two existing production files change by **+13/-17 lines, net -4**. No production
file is added. The existing input test changes by **+6/-6** to exercise the
single pending action. Concurrent chapter additions to that test are preserved
and excluded from this cleanup's counts. This document is the only added file.

## Verification

Targets were derived from the current Paths CMake graph. Release builds of
`sorter`, `gallery` and `paths` completed in a separate build directory.

The frozen P036 source plus this cleanup passed both `paths_sorter_input_tests`
and `paths_sorter_solve_tests`. This independently checks the cleanup before
the concurrently edited chapter is integrated. The bundled catalogue also
validated from `/private/tmp` using `--check-content`, which exits before
graphics startup.

The combined chapter and cleanup build passes both gates too. The headless
input test includes matrix fraction controls at 1440×860, 800×600 and 360×480.
The first integration attempts encountered incomplete chapter content and an
old assertion expecting four Contents types. The chapter task completed its
content and corrected that assertion to seven. Only the failing solving gate
was rerun after the fixture correction; it passed. Bundled content validation
also passed with the integrated chapter.

The input gate covers real ImGui control events and layout measurements without
a window or captured image: queued-action cancellation, repeated/held input,
mathematical moves, Undo, Resume, reference browsing and explicit Next.

Text evidence and exact manifests are in `build/input-cleanup-evidence/`.
`scoped-diff.patch` contains only this cleanup; concurrent test changes are
recorded separately in `preserved-concurrent-tests.patch`. All work remains
uncommitted. No screenshots, captures or windows were used.

The tested integrated executable is `build/input-cleanup-native/sorter`.
Its build and copied inputs are isolated from the other task's build directory.

## Remaining risk and next candidate

Human visual confirmation remains separate from the automated gates. The layout
and colours are unchanged. Open an algebra or matrix question, make one move,
then return to Contents and Resume. The gold problem should stay fixed, the cyan
working should return unchanged, and the green checked history should remain.

The next cleanup candidate is the Pause/Resume input route after the chapter
work settles, if tracing it reveals duplicate decisions that can be removed.
This checkpoint stops after the pending-action consolidation.
