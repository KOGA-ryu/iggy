# P028: table of contents to selected practice

Status: accepted by the user; Release builds and six targeted checks passed;
uncommitted. The user confirmed the selection experience and requested checks
work, describing the flow as opening a maths textbook and entering visuals.
They also endorsed continuing the no-screenshot workflow. No screenshots were
taken and no native window was launched by the agent.

The user authorized the table-of-contents layout, with chapter titles leading
to problem types, and requested smaller fonts and buttons. This checkpoint
connects that selection to the existing fixed solving workspace.

## Delivered flow

Normal `sorter` startup now loads the bracket practice pack and opens its
table of contents. The first live title is **Algebra → Linear equations →
Bracket equations**, with six prepared problems. Other subjects are disabled
until the loaded pack supplies prepared questions for them. Legacy packs and
explicit scripts retain their grouping entry; Groups and Contents connect
the two views without changing sorting history.

Subject, chapter and type checkboxes combine available questions. Clicking a
chapter title browses its types. All selects every available question; Random
previews a requested number without duplicates, bounded by availability;
Shuffle explicitly samples again. Pick and individual question checkboxes
allow manual selection. The selection count and Start remain visible in the
footer while long lists scroll.

Start set freezes those questions in contents order and starts fresh attempts.
Next visits only that set and is disabled at its end. Completed working stays
visible until the user acts. Back to contents pauses the active question;
Resume set restores that same question, step and frozen selection even if the
draft has since changed. Starting another set preserves previous completed
and unfinished attempts in the question owner's history.

The selection view uses 15-pixel body text, 17-pixel chapter headings and a
20-pixel gold title, with 30-pixel action buttons. Checks and the Start button
are teal/mint. Its narrow layout stacks contents above problems and retains
a separate footer. The existing solving-workspace dimensions are retained.

## Canonical owners and replaced routes

- `EquationSorterSession` derives one immutable catalogue from authored subject,
  chapter, type and form metadata. It owns title inclusion, the selection draft,
  random sampling, frozen question order and current position. The UI consumes
  its availability/count projection and sends semantic actions.
- Study Next replaces the scan over every prepared home slot with traversal of
  the frozen set. Legacy individual-card solving retains its original Next
  semantics. Both use the same `openSolve` preparation route.
- `LayeredQuestionSession::RestartQuestion` remains the history/archive owner.
  An explicit `archiveUnfinished` option is forwarded through the existing
  gallery replay/preparation route when Start set creates a new attempt.
  Ordinary replay still requires completion. An unfinished archive retains
  its original evidence and is never marked completed.
- A new set prepares copied/restarted question sessions before replacing any
  retained owner. Failure during preparation leaves the old set and evidence
  in place. Stale selection actions are revision-checked, and the existing
  queued-input and held-press guards protect the change into solving.

No new production files or alternate answer-judging routes are introduced.
The generated sorter practice pack gains classification metadata; the six
question files, answers, working, help, content versions and older sorter
packs retain their bytes.

## Verification

The current CMake graph supplied `sorter`, `gallery` and `paths` Release builds,
plus the affected input, sorter-content, sorter-solve, guided and gallery test
targets. All built. The targeted CTest result is **6/6 passed**:
`paths_sorter_input_tests`, `paths_sorter_content_tests`,
`paths_sorter_solve_tests`, `paths_bracket_recipe_tests`,
`paths_guided_tests`, and `paths_gallery_tests`.

Coverage includes actual selection controls and keyboard input at 1440×860,
800×600 and 360×480; scrolling to individual problems; editing the random
count; stable previews; empty/stale/out-of-scope refusal; synthetic multi-title
and multi-subject combinations; selected-only Next; resume after draft edits;
held Enter on Start; group preservation; and completed/unfinished history.
The existing sphere, help, replay, resize and fixed-workspace loops also pass.
The generator check and default bundled-content validation from `/private/tmp`
pass before native graphics startup. Bundled JSON matches source.

Production: **11 existing code files, +321/-24 (net +297)**. Four existing
test files change by **+194/-2 (net +192)**. One generated sorter content file
changes by **+36/-6**. Text evidence, scoped diffs and final hashes are under
`build/study-selection-evidence/`.

## User visual check and next candidate

Relaunch Sorter. Look for the gold Table of contents title, compact chapter
and question rows, and the teal Start set button. Select two questions and
confirm that Next visits only those two; return partway through and use
Resume set. Resize once to check the footer and scrolling at a smaller size.

The user has confirmed this checkpoint's experience and checks. Next candidate:
add two-step equations under Linear equations, followed by one-step types,
through this catalogue and the existing question model.
