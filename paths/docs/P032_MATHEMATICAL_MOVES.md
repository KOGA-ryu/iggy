# P032: checked mathematical moves and a solution blueprint

Status: Automated Green; the user rejected typing equations in the interface.
[P033](P033_VISUAL_MATH_MOVES.md) replaces entry with visual controls while
retaining this mathematical engine and evidence. Uncommitted.

The delivered-experience and visual-check sections below describe the original
P032 checkpoint; P033 is the current interaction.

The user authorized the proposed shared mathematical move system: solve the
existing bracket equations through different valid routes, enter and check
each resulting equation, retain the working, and undo without erasing attempts.
This is one new capability, not an ownership-cleanup workstream. It extends
Paths directly; the parent Creative project and source snapshots are untouched.

## Delivered experience

All six bracket questions in the default study pack, bracket pack, and linked
original/mixed sorter packs now open mathematical entry in the fixed workspace.
Choose `a(b+c)` to expand, `=` to simplify/rewrite, or `+ - * /` for an operation
on both sides. The latter four take a number, including signed values and exact
fractions. Enter the resulting equation and click Check or press a fresh Enter.
Arithmetic may be simplified within the selected operation. An equivalent
numeric fraction or terminating decimal is accepted exactly.

For `3(x + 2) = 21`, both these routes work:

- Divide by 3: `x + 2 = 7`; subtract 2: `x = 5`.
- Expand: `3x + 6 = 21`; subtract 6: `3x = 15`; divide by 3: `x = 5`.

The gold original problem and cyan active equation stay close to the controls.
Every checked move adds a numbered, connected block below them. Checked steps
have drawn check marks; the active path is green and earlier branches are muted.
Selecting a block inspects its source equation, operation, result, justification
and attempts without changing the current working. Wide windows have a separate
inspection column inside the same workspace; narrow windows expand details in
the scrollable blueprint. The lower workspace fills the available window.

Undo moves to the immediate parent, including after completion. It retains
both accepted and incorrect attempts and the original solution branch. Solving
again from that parent adds another branch. Completion requires an isolated
`x` and a numerical answer verified by exact substitution into the original
equation. Finished working stays until Next, Undo or Play again. Next follows
the existing frozen study queue. Pause, focus loss, return/resume, per-question
sessions and fresh-set archiving preserve their existing ownership boundaries.

Progress and archives remain in memory for the running application. Durable
save/load, arbitrary symbolic proofs, variable divisors, nonlinear equations,
calculus rules and geometric transformations are separate capabilities.

## Canonical owner and mathematical contract

`LayeredQuestionSession` remains the sole correctness, progression and evidence
owner. Its `MathMoves` interaction stores an append-only `MathMoveRun` in the
existing run record and archives through the same RestartQuestion path. Each
node records its parent and checked working; each submit or Undo records its
source, destination and actual result. Incorrect submissions record evidence
without changing the active equation. Commands require the current question,
content version, run number and revision. Paused, stale and cross-question
commands cannot create an attempt. Prepared-answer and assistance commands are
unavailable in mathematical-move runs.

`LinearEquation` is a pure mathematical kernel called by that owner. It parses
each side into exact affine coefficients and preserves the entered display.
For add/subtract/multiply/divide, each side of the entry must equal that side
after the selected operation; merely sharing the final solution is insufficient.
Expansion requires a multiplied bracket and removes that structure. Simplify
permits an equivalent rewrite of each side. The UI does not evaluate expressions,
calculate answers or maintain a second history.

The grammar accepts `x`, integers, decimals, fractions, parentheses, unary signs,
`+ - * /`, and conventional implicit multiplication. It refuses variable
products/divisors even when later cancellation would remove them, unsupported
characters/functions, mismatched parentheses and zero divisors. Multiplication
or division of an equation by zero is refused. Inputs are at most 160 equation
characters and 48 operand characters; nesting is limited to 16 and parsing to
128 atoms. Reduced numerators and denominators are bounded by 1,000,000,000;
decimals allow six places. Checked 64-bit intermediate arithmetic remains within
range before reduction. A run permits 128 working nodes, including the original,
and 1024 events. Reaching a limit retains all evidence and reports the limit.
Parsing/checking is linear in bounded input length; appends can reallocate the
bounded history. The blueprint reads at most the current run's 128 nodes.

`GallerySession` forwards `MathematicalMove` to the shared owner and keeps its
pause and replay boundaries. Mathematical runs create no answer targets and
expose no prepared answer choices. Native rendering uses the existing ImGui
workspace and native strokes. The existing report adds the current run's nodes
and events; `active`, `parent`, `from` and `to` are zero-based node indices,
while each node also carries its stable working-state ID.

The content opt-in is `working_model: "linear_moves"`. Shared validation requires
a supported unsolved linear equation, with a nonzero x coefficient on the left
and a constant right side, so the available operations can reach an isolated x.
The six recipes and references advance to content version 5, generator version 3.
Their authored prepared chain remains available to the distinct prepared arcade
interaction. Graph questions retain their existing typed graph projection and
answer route. No competing mathematical-move judge or progression route is added.

Production C++ delta: 11 files touched, including 2 new pure-kernel files; +593/-8 lines (net +585). The content publisher adds +3/-2 lines and CMake adds five lines. This growth implements the authorized new capability.

## Verification

All three native Release targets (`sorter`, `gallery`, `paths`) build. The
current CMake graph selected seven focused suites, all passing:

- `paths_math_moves_tests`: independent expected arithmetic, both routes through
  all six published questions, fractions/decimals, expansion, exact equality,
  wrong arithmetic, one-sided changes, invalid input, overflow, zero operations,
  node/event limits, Undo branches, stale identities, review, replay, return,
  selection, explicit Next and archived evidence.
- `paths_sorter_input_tests`: actual ImGui typing, pointer controls, Enter and
  Tab, both routes, all six bracket questions, fractions and decimal input,
  simplified working, retained mistakes, inspection, Undo, Next, replay, held
  keys, focus loss with queued input, return/resume and live resize. Initial
  sizes are 1440x860, 800x600 and 360x480. Containment checks include the original
  problem, entry, Check and final verification. Existing graph controls pass.
- `paths_sorter_solve_tests`, `paths_guided_tests`, `paths_gallery_tests`,
  `paths_content_tests` and `paths_bracket_recipe_tests`: surviving prepared,
  graph, shared-owner, content and reproducible-authoring boundaries. Tests of
  the prepared sorter interaction explicitly use content without the new opt-in;
  the direct-move suite and its UI cases use the published content unchanged.

The publisher's `--check` passes. The bundled default content validates from
`/private/tmp` before graphics initialization. No screenshot, native capture,
visible window, image asset, delegation, commit or push was used.

Evidence and the exact scoped delta are in `build/math-moves-evidence/`, including
the turn baseline, before/after hashes, scoped diff, build/test logs and final
verification receipt. Human visual acceptance remains pending.

## User visual check

Relaunch Sorter, choose Algebra / Linear equations / Bracket equations, and open
`3(x + 2) = 21`. Look for the gold original problem, cyan working and operation
selection, and green checked blocks below. Divide by 3 and enter `x+2=7`, then
subtract 2 and enter `x=5`. Undo twice, expand to `3x+6=21`, subtract 6 to
`3x=15`, then divide by 3 to `x=5`. Select an earlier block and confirm its
working opens while the current equation stays fixed. The old branch should
remain, and the final solution should wait for Next.

Next candidate: durable save/resume of the same working and attempt records.
