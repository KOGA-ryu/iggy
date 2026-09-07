# P033: visual mathematical moves

Status: Visual format accepted by the user; automated checks passed; uncommitted.
The user requested the same format for linear algebra in [P034](P034_MATRIX_ROW_MOVES.md).

The user disliked typing equations in P032 and selected visual controls as the
required correction. This checkpoint replaces that input experience while
keeping the mathematical checking, alternate routes and working history.

## Delivered interaction

Click a concrete operation, then click its resulting equation. There is no
number field, equation field, or separate Check button. The current equation
supplies a compact palette: expansion, division by its coefficient, multiplication
by the reciprocal, and signed constant moves. Equivalent rewriting is offered
when earlier noncanonical working needs it. Operations use symbols and numbers.
The palette deliberately offers contextual moves rather than arbitrary operands.

For `3(x + 2) = 21`, click `÷ 3`, then `x+2 = 7`; click `- 2`, then `x = 5`.
Alternatively, expand with `a(b+c)` to `3x+6 = 21`, subtract 6 to `3x = 15`,
then divide by 3 to `x = 5`. Reciprocal multiplication and valid detours also
remain available. All six published bracket questions work through both routes.

Each move offers four distinct result tiles with one valid transformation.
Wrong results include missed distribution, one-sided operations and arithmetic
errors. A wrong click records the attempt, leaves the equation in place and
keeps the same tile order for retry. Correct positions vary across equations
and moves. Selecting an operation alone does not create answer evidence.

The gold original problem, cyan working and compact controls stay fixed.
Selected moves have a cyan fill; result tiles have teal borders. Correct moves
add green checked nodes below. Undo retains prior branches, inspection stays
read-only, and completion waits for explicit Next. The existing chapter queue,
graph exercises, pause, resume and replay behaviour remain in place. History
continues to be held only while the app is open.

## Ownership and removed routes

`LayeredQuestionSession` remains the owner of judging, progression and evidence.
Its read-only `mathMoveChoices()` projection calls `LinearEquation` for the
current equation. A shared internal transformation routine supplies both result
generation and the existing checker, so presentation does not duplicate the
rules. Generation is bounded by six move slots, four result slots and a fixed
candidate budget using the existing numeric/parser limits. Unrepresentable
moves are omitted; generation neither submits answers nor changes history.

`EquationSorterUi` caches the projection by question, run and revision, presents
its labels unchanged, and forwards the clicked entry through the existing
`MathematicalMove` command. Stale identities, pause and focus-loss guards remain
at their existing boundaries. The number/equation buffers, InputText controls,
Check action and displaced public operation-menu API are removed. No new
production file, content format, dependency or mutation route is introduced.

Production C++ delta: six existing files, +183/-75 lines (net +108); no new
production files. Two existing test files change by +120/-34 (net +86).

## Verification

All three native Release targets (`sorter`, `gallery`, `paths`) build. The current
CMake graph selected three focused suites; all pass:

- `paths_math_moves_tests`: both visual routes through all six questions,
  independently expected results, distinct distractors, stable retry order,
  original-equation completion, Undo branches, reciprocal multiplication, a
  valid detour, numeric boundaries, and the surviving exact-checker contracts.
- `paths_sorter_input_tests`: actual ImGui pointer and keyboard events, all six
  questions, wrong and correct tiles, Tab/Enter, held keys, Undo, earlier-step
  inspection, pause/focus loss, discarded queued input, replay, resume and Next.
  Controls are checked at 1440x860, 800x600 and 360x480, including live resize
  with an operation selected and with a completed solution. Existing graph and
  prepared-control checks remain green.
- `paths_sorter_solve_tests`: surviving prepared and graph solving, study queue,
  assistance, resume and explicit Next.

The first keyboard test compared navigation rectangles against the window
origin; the pinned ImGui version uses the content origin. The test now uses
ImGui's own coordinate conversion and reaches the actual result tile by Tab.
This was a test-coordinate correction; keyboard activation needed no new route.

Evidence, the exact pre-change baseline and the scoped diff are in
`build/visual-moves-evidence/`. No screenshot, capture, visible window,
delegation, commit or push was used. Human visual acceptance remains pending.

## User visual check

Relaunch `b/sorter` and choose `3(x + 2) = 21` from Bracket equations.
Look for the gold problem and cyan working. Click `÷ 3` and a wrong result:
the feedback should turn red and the working should stay. Then select
`x+2 = 7`, followed by `- 2` and `x = 5`. Green checked steps should build below,
and the finished solution should wait for Next. Undo should restore the earlier
choices while keeping the old branch. No equation typing is required.

Next candidate after this interaction is accepted: animate the mathematical
terms between checked steps in the same workspace.
