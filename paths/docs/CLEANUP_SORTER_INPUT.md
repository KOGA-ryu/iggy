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

## Follow-up: shared game summary

The user requested another bounded cleanup and deferred manual review. Tracing
Pause/Resume confirmed that `GalleryPause` and `GalleryScene` already provide
one pause route. The summary they feed repeated common field assignments and
history traversal in the mathematical and prepared-question branches.

`GallerySession::view()` now assembles those shared fields and totals once,
then fills the mode-specific fields. The duplicate traversal and assignments
are removed. `GalleryScene` still owns pause state; `LayeredQuestionSession`
owns visible working and current/archived evidence. Mathematical Submit events
and prepared answer attempts keep their existing counting rules, including
retained branches and archived runs. No new state, helper or mutation route is
introduced.

One existing production file changes by **+14/-19 lines, net -5**. No production
file is added. The existing mathematical-moves test adds eight lines checking
totals through Undo branches, Replay, a second completed run and paused return
to Contents. UI layout, content and persistence code are untouched.

Targets were derived from the current CMake graph. Release builds of `sorter`
and `gallery` pass. All three targeted headless gates pass:
`paths_gallery_tests`, `paths_math_moves_tests` and `paths_sorter_input_tests`.
Together they cover prepared attempts, exact moves, retained history and the
real input route through Pause/Resume and saved practice. No windows,
screenshots or captures were used.

Evidence is in `build/session-summary-cleanup-evidence/`. Existing unrelated
changes are preserved and all work remains uncommitted. Manual review is
deferred at the user's request; the earlier checklist above is historical.
The remaining risk is a mode-specific presentation regression not covered by
the headless checks. The next cleanup candidate is prepared-answer feedback
mapping, if a focused trace reveals duplicate decisions. This checkpoint stops
after shared summary assembly.

## Follow-up: prepared-answer feedback

The user approved the next cleanup. Button choices and target hits separately
submitted answers, mapped the recorded verdict to feedback, set its expiry and
advanced resolved steps. `GallerySession::submitAnswer()` now owns that shared
handling; it replaces `submitHit()` and removes the duplicate button block.
`LayeredQuestionSession` remains the sole mathematical judge. The dispatcher
keeps input-specific validation and aiming misses. Buttons and finite practice
advance immediately, while arcade targets keep their pop interval and colour
history. Rejected input cannot refresh feedback or add attempts.

Two existing production files change by **+20/-24 lines, net -4**, with no new
production files. The existing solving test changes by **+17/-3** to cover
button feedback expiry, invalid and paused submissions, and feedback clearing
at immediate step changes. Targets came from the current CMake graph: Release
builds of `sorter` and `gallery` pass, as do `paths_gallery_tests`,
`paths_sorter_solve_tests` and `paths_sorter_input_tests`. These also preserve
partial collections, stale shots, misses and actual control input.

Evidence is in `build/answer-feedback-cleanup-evidence/`. Earlier work is
preserved and changes remain uncommitted. No windows or captures were used.
There are no unresolved failures in the targeted checks; manual visual
acceptance remains deferred at the user's request. The next cleanup candidate
is hint/reveal availability, if a focused trace finds duplicate decisions.
This checkpoint stops at shared prepared-answer handling.
