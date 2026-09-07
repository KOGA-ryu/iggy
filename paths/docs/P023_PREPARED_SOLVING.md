# P023: one complete prepared solving sequence

Status: implemented; targeted model and actual-input checks passed; native
offscreen views reviewed; uncommitted. Human pointer/swapchain acceptance is open.

The sorter now opens a prepared solving sequence for card **1012**,
`-2(x + 1) = 22`, from either the original algebra pack or the mixed pack.
The goal is **find x**. The first working highlights the outside factor and
the whole bracket it multiplies. Operation choices are buttons; arithmetic
answers use the gallery's moving coloured spheres. The original equation and
current working remain visible above the active decision.

| Decision | Accepted move or result | Next working |
| --- | --- | --- |
| Remove the outside factor | Divide both sides by -2 | `x + 1 = 22 / (-2)` |
| Calculate the division | -11 | `x + 1 = -11` |
| Remove the addition | Subtract 1 from both sides | `x = -11 - 1` |
| Calculate the answer | -12 | `x = -12` |

Completion displays the substitution check:
`-2(-12 + 1) = -2(-11) = 22`. This finite practice waits for **Play again** or
**Back to groups**. Replay preserves the completed run as prior exposure.

The wording explicitly follows the division-first method. Expanding first is
identified as valid mathematics; this first prepared sequence does not provide
that branch or offer a general "What can you do next?" question with a single
accepted method.

## Help and progression

Every decision has three separate controls:

- **Hint** reveals the prepared clue for the current mathematical decision.
- **Show next move** reveals the next transformation or calculation without
  submitting an answer or changing the working.
- **Do this step** applies that prepared step and continues. It records
  assistance, with no fabricated correct attempt or score.

A wrong choice produces the existing brief feedback and leaves that decision
active. Correct operations and arithmetic advance immediately through the
question owner. Prepared arithmetic targets start active, and progression
does not wait for a pop animation. The existing continuous gallery retains
its target lifecycle and endless progression.

Back to groups and Escape retain memberships, inspection, frozen inventory
slots, scroll and sorter Undo. **Resume** reopens the saved decision. Focus
loss clears queued input and pauses the active solve; Resume continues it.
If a focused operation or control disappears, keyboard focus recovers to the
persistent Back control without activating it. Holding a key is not a second
mathematical answer.

## Canonical owners and content

- `EquationSorterSession` selects the linked problem and retains one solving
  session. `OpenSolve` and `ReturnToSorter` do not rewrite grouping history.
  A shortcut offers the prepared example when no card is inspected; inspecting
  an unprepared card does not silently select another equation.
- `LayeredQuestionSession` owns judging, working-state progression, and help
  evidence (`hintRequested`, `nextMoveRequested`, `answerShown`). The new help
  commands reuse its existing resolution/Continue boundary.
- `GallerySession` composes the shared question owner with operation choices,
  sphere hits, help, and finite replay. Presented challenge/frame identities
  reject stale shots and help after a transition. `stopAfterQuestion` selects
  this prepared flow; ordinary gallery defaults remain unchanged.
- `GalleryScene` owns target lifecycle, picking and camera framing.
  `FrameTargets` reuses the existing camera-fit kernel around preset targets
  and their motion extents. Prepared solving reframes on viewport changes,
  keeping targets usable even at 360x480; UI tests require at least 24 pixels
  of projected target width.
- `EquationSorterUi` presents these owners and forwards real controls to their
  dispatchers. It does not evaluate equation strings or maintain another answer
  key, working-state index, or attempt history.

The sorter record's optional relative `solve_pack` points to
`content/sorter/linear_bracket_pack.json`, whose `solve` deck contains
`content/cards/sorter_linear_bracket.json`. The existing loaders resolve and
validate the linked question before creating a native host. The question must
match the displayed card and supply prepared help for every operation/calculation
step. Both bundled sorter packs include this link; legacy unlinked cards remain
valid. Only this algebra question has a solving sequence in P023.

Question schema 1 now reads optional `hint` and `next_move` strings on steps,
and optional `highlights` on working states. Each highlight supplies a byte
`offset`, positive `length`, and nonempty `label`. At most eight ordered,
nonoverlapping spans may refer into the display, on UTF-8 character boundaries.
The shared validator covers loaded and directly constructed content; rendering
does not infer mathematical structure. Rich typesetting remains outside this
checkpoint.

## Build and play

From the Paths root:

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b -t sorter -j4
./b/sorter
```

Click **Solve: -2(x + 1) = 22**, or inspect that card and use its Solve button.
Choose the requested operation, then shoot the ball whose colour/letter matches
the arithmetic answer. Help is optional at each decision. To use the mixed pack:

```sh
./b/sorter --content content/sorter/mixed_foundations_v1.json
```

## Verification and checkpoint

The resumed work retains the existing linked-content, help, script and solve
session implementation, and finishes structure highlighting, immediate
progression, visible checking, target framing, focus recovery, and selection
of the intended problem.

Six targeted pure suites passed with `PATHS_BUILD_NATIVE=OFF`:
`paths_sorter_tests`, `paths_sorter_solve_tests`, `paths_gallery_tests`,
`paths_scene_tests`, `paths_guided_tests`, and `paths_content_tests`.
The solve/scene/gallery subset was rerun after the final framing change.
The existing deterministic fixture generator's `--check` also passed.

`paths_sorter`, `paths_sorter_input_tests`, and `paths_gallery` rebuilt.
The actual ImGui input suite passes operation buttons, real projected sphere
clicks, wrong choices, all three help levels, focus loss, return/resume,
completion, replay, and scrolling at 1440x900, 800x600 and 360x480, retaining the
earlier sorter layout coverage. It verifies immediate arithmetic availability,
target size, keyboard recovery, and a visible completion check.

Native offscreen operation, arithmetic, narrow arithmetic, narrow help,
completed, assisted, and returned views were reviewed. Reports verify four
player answers with two retries, four assisted steps with zero invented
attempts, and unchanged groups/slots/Undo on return. The native scenario
`tests/equation_sorter_solve.script` completes the sequence without timing waits.
Evidence and hashes are under `build/solve-evidence/resumed/`.

Against the resumed working state, ten existing C++ production files change by
**+121/-23 lines (net +98)**; no production files are added. The existing solve
and input tests add **+58/-13 lines (net +45)**, and the native scenario removes
six timing waits. The linked question gains authored highlights and clearer
method wording; earlier unrelated work is checked against saved file hashes.

Vulkan captures used approved host GPU access. No visible window was opened.
Changes remain uncommitted and earlier unrelated work is preserved. The next
checkpoint is the user's playtest of this one complete question; other maths
subjects need their own learner prompts and prepared solving sequences.
