# One question, four levels of support

Status: **implemented for linear equations and bounded two-variable matrix
row reduction, with all four levels, written work, help exposure and save/resume**.
The generated linear pack has 25 questions; documents add one linear and one
matrix example. Visual acceptance is pending. Other families still need their
own checked responses. Motion and further
interactive 3D integration are deferred. Existing prepared questions retain
their current format.

The user wants fast solving and substantial explanation in one workspace,
progressing from small, explained decisions to writing the entire solution.
Every question therefore has four support levels. A separate complexity band
describes the mathematics. Changing support never changes the problem, domain,
answer, or mathematical standard of correctness.

## Shared textbook presentation standard

The user has selected the existing textbook work as the presentation standard
for equation solving. The inspected references are
[P048's section format](P048_TEXTBOOK_SECTION_FORMAT.md),
[P049's reading and figure layout](P049_LIVE_TEXTBOOK_FIGURES.md),
[P051's object binding](P051_DETERMINANT_VOLUME.md), and the
[lesson authoring workflow](LESSON_AUTHORING_WORKFLOW.md). P048 records the
formatting research that informed its hierarchy, equations and disclosures.
This decision establishes the target; it does not claim the current question
workspace already implements every textbook feature.

- Use the textbook's semantic structure: introductions, definitions,
  propositions, worked examples, captioned figures and exercises. Preserve
  stable block IDs and reference definitions directly. Printed numbers are
  labels; number an equation when the explanation refers to it.
- Keep prose left aligned, give long reading a bounded column, and scale math
  with text. The reader's reference is about 72 average character widths with
  90–200% text zoom. Preserve compact default controls; let wide equations
  scroll in their own rows instead of shrinking them or widening all prose.
- Apply that reading/figure structure inside the fixed solving workspace.
  Gold Given, cyan Working and the current response remain adjacent. Reading,
  history and an optional figure use the remaining space. Narrow layouts must
  retain the problem and response while opening supporting material in place.
  Completion remains green until Next; a step never triggers a screen change.
- Treat definitions, hints, answers, solutions and proofs as distinct material.
  The textbook's Read/Figure/Exercise views are presentation choices; they are
  not the solver's Learn/Practice/Solve/Write support levels. The question owner
  determines disclosure and records assistance for its attempt.

### Responsibility at the connection

The [current work allocation](../AGENTS.md#current-work-allocation) gives the
textbook worker responsibility for both the full teaching experience and
equation solving, including their shared formatting and figure integration.
The separate worker supplies 3D assets/models. The table below identifies code
owners within that product; it does not assign teaching to another worker.

| Responsibility | Existing owner and reuse boundary |
| --- | --- |
| Teaching structure and reference material | `BookBlock`, `BookPassage` and `Textbook` own textbook blocks, references and reading disclosures. Reuse approved material with its identity and provenance. |
| Equation-solving evidence | `LayeredQuestionSession` and its mathematical kernels own responses, accepted steps, current working, support exposure, Undo and completion. `CorpusPractice` owns the question bank and saved command replay. |
| Figure meaning and geometry | `MathObjects`, `MatrixBoard` and the relevant figure providers own their established representations. `MathObjectScene` presents their snapshots. A figure connected to a question consumes that question's given/current state through an explicit binding. |
| Layout and math rendering | `LessonSpread`, `TextbookUi`, `TextbookFigureUi` and `NativeMath` establish the reusable presentation. UI adapters dispatch commands and render approved state; they do not become answer checkers. |
| Authored input and publication | `LearningDocuments` compiles supported templates and `export_learning.py` transports their checked bytes. The source-card adapter produces that same format. |

A teaching figure may intentionally show a solution. A solving figure needs an
explicit rule for what is visible before a response, after a hint and after
completion, including labels, intersections, reduced forms and readouts. Camera
movement creates no mathematical attempt. If a figure control is an answer
action, route it through the solver; never copy a separate model challenge's
completion into the question. Bind A and b explicitly and preserve the solver's
number system, dimensions and accuracy contract. An unsupported representation
needs a supported alternative or a reported authoring gap.

The current connection is partial: textbook and question surfaces use `NativeMath`,
and prepared question help can show neutral RREF definitions/propositions and
selected chapter prose. Four-level templates currently use step
`@definitions`/`@teaching` and reject `@read`. Document lessons can embed
registered figures, but a live figure bound to a regular solving attempt and
general textbook-block export are not implemented. The full textbook block
renderer is currently internal to `TextbookUi`; do not claim it is already a
shared question component or clone it into an adapter. Establish the smallest
shared interface when the first concrete solver consumer requires it.

## The four levels

| Level | What appears before a response | What the learner supplies | Reading and feedback |
| --- | --- | --- | --- |
| **1 Learn** | Given, current working, one small goal, its rule, relevant definitions and an explanation of why the step works | A symbolic choice for the next operation or result | Current-step teaching is expanded. Reached steps retain their full explanations. One accepted response advances immediately. |
| **2 Practice** | The same given and short step cue, with a blank in the next expression | The missing value, term, symbol or operation, using a compact palette or typing | Definitions and detailed reasoning open on demand in place. A correct entry advances immediately. |
| **3 Solve** | Given, the learner's working and a meaningful checkpoint such as a factorization or resulting matrix | A complete intermediate result and their own working | No preselected method or list of possible answers. Check the submitted checkpoint; hints remain available on request. |
| **4 Independent** | Only the problem statement/equation, essential assumptions, required output and a blank working area | Their complete written solution and final answer | No step cues, examples, answer tiles, prefilled derivation or unsolicited checking. Check work submits the composed solution; help can still be requested explicitly. |

An independent proof problem still needs its claim and assumptions. A statistics
problem still needs data and units. "Equation only" means removing instruction,
not deleting information required to make the task well-defined. A small
keyboard/palette is an input aid; choosing a prewritten complete answer is not
independent written work. Handwriting recognition is not required for the first
implementation; a native multiline math editor is the written input.

Users can choose a level directly. There is no unlock sequence, forced reading
timer, or requirement to solve the same numbers four times. A new learner can
use Learn on a difficult topic; an experienced learner can use Independent on
an elementary one. Repeating one instance at four levels counts as one distinct
question, with four separate exposure conditions, not four new repetitions.

## One workspace: read, act, continue

Keep this order in the current Focus workspace:

1. A compact breadcrumb and the four level controls.
2. **Gold Given**, pinned beside **cyan Working**, directly above the active response.
3. The active step/checkpoint and its input. Enter or one tile click submits
   when the input is ready; entering a newline in the independent editor does
   not submit the solution. Use an explicit Check work button/shortcut there.
4. Current-step teaching and the reached working history. At level 1, the
   teaching is already open; levels 2–4 have a **purple Help** control.
5. The **green finished result**, retained until explicit **Next**. An optional
   review can expand in place. It never blocks the next question.

Give the response and its explanation a common alignment. On a wide window,
the reading can occupy an adjacent column; on a narrow window, it expands below
the response in the same space. Given and the active response stay visible;
long history and long explanations scroll in their own area. Retain the
existing compact font/button scale, text zoom and horizontal equation scrolling.
Do not shrink a large formula to illegibility or consume the window with a
permanent navigation sidebar. Use text labels as well as colour.

"Lots of description" belongs in the content, attached to the exact step where
it helps. Each step must have: goal; prerequisite references; definitions for
new notation; the rule and its conditions; why it applies here; one small
action; common mistakes with specific feedback; and the reached explanation.
Explain what, why and when the operation is valid. A restatement such as
"This is the correct setup" is not teaching content. Avoid an arbitrary word
quota: a beginner should not need an unstated prerequisite or unexplained jump.

Keep three disclosures separate: a general definition; a hint pointing toward
this problem's next action; and the actual next line/solution. The last two
cannot be silently included in an unassisted projection. A separate worked
example can be opened on request, with distinct givens and an exposure record.

## Fully specified example: 3x + 5 = 20

Core: solve for real x; coefficient 3 is nonzero. The reference route is
`3x+5=20 → 3x=15 → x=5`. Check `3(5)+5=20` against the original equation.
The two reversible operations also establish uniqueness. This example is part
of the bounded `linear_balance_ax_b` authoring pilot.

| Step | Goal, definition and reason | Input in Learn / Practice | Reached explanation |
| --- | --- | --- | --- |
| Remove the offset | Equality says both expressions have the same value. Subtract 5 from each side. Adding 5 back reverses this operation, so no solution is lost or gained. The left becomes 3x because 5-5=0; compute 20-5 on the right. | Learn: choose the right-hand result from 15, 20 and 25, in a shuffled order. Practice: fill `3x = [ ]`. | `3x=15`. Erasing only the left-hand 5 or adding 5 on the right would change the equation's solutions. x is still multiplied by 3. |
| Divide the coefficient | The coefficient is the multiplier attached to x. Dividing each complete side by 3 undoes multiplication; 3 is nonzero, so multiplying back recovers the prior equation. | Learn: choose x from 5, 15 and 20/3, in a shuffled order. Practice: fill `x = [ ]`. | `x=5`. Substituting in the original gives 20 on each side. The equation has one solution because its x coefficient is nonzero. |

At Solve, show `3x+5=20` and a checkpoint for x, with room for the learner's
working. At Independent, show `3x+5=20`, "Solve for x; x is real", and an empty
editor. Both can accept the valid alternative route
`x+5/3=20/3 → x=5` through the bounded written-input checker.
Do not reject a valid route because it differs from the reference solution.

The definitions and step prose live in
`content/authoring/question_layers_v1.json`. The tool emits the complete golden
question and four pre-answer projections into `build/question-format-evidence/`.
Those remain authoring artifacts. Explicit `--publish` produces the runtime
pack `content/corpus/linear_support.json`: the golden question plus 24 varied
repetitions. Search **Linear practice** under Library → Questions, then Focus.
The fourth button is labelled **4 Write**, the Independent support level.

### Linear written-input contract

The native editor accepts plain equation lines using `x`, exact numbers,
parentheses and `+ - * / =`. Fractions use `5/3`. Each submitted equation must
have exactly the original equation's single solution; `0=0` cannot pass.
Equivalent intermediate equations and a reversed final `5=x` are accepted.
Each valid submission commits atomically. A wrong or unsupported line leaves
the prior working unchanged and the complete raw draft available for editing.

```text
3x=15
x=5
check: 3*5+5=20
```

The optional `check:` line verifies both evaluated sides against the original
equation, after an isolated candidate. The owner also substitutes the candidate
itself before completion. A direct `x=5` is a checked final answer; it does not
prove that the learner supplied a multi-step derivation. Free prose, LaTeX
commands, nonlinear equations and arbitrary proofs are **Not checked yet**.
This is typed input, not handwriting recognition or a general CAS.

Enter submits the Practice blank, but inserts a newline in Solve/Write.
**Check work** submits the composed written solution. Escape leaves editing
and retains the latest draft. Drafts are bounded at 8 KiB, submitted work at
32 lines and each parsed equation at 160 characters. A run retains at most
128 working nodes and 256 submissions/Undo events; **Again** archives the run
and starts another attempt. Large curricula still require the separate bounded
loading checkpoint in the authoring workflow.

### Matrix written-input contract

[`matrix.v1`](LEARNING_DOCUMENTS.md#matrix-documents) feeds the same four-level
owner from `content/write/matrix.paths.md`. Learn chooses a symbolic row
operation; Practice fills its multiplier or divisor; Solve and Write accept
complete two-row augmented matrices, one matrix per line. Exact fractions and
alternative routes are supported. The matrix kernel checks the original unique
solution at every line and substitutes the final values in both original rows.
A redundant pair of rows is rejected even if it contains the original solution.
The shared UI takes goal and input instructions from the owner's projection;
it does not interpret row arithmetic or guess the answer family.

This first matrix family is restricted to nonsingular 2×2 real systems with
an unsolved given and a final identity coefficient block. Symbolic row commands,
arbitrary proof text and general solution families are not checked. Imported
operation metadata, step definitions and teaching are frozen in the question
stamp. The existing version-2 support journal replays matrix actions and drafts;
its wire format does not change, and earlier linear/prepared saves still load.

## Cross-subject response contracts

The shell, support levels and evidence are shared. Each question family names
its response type and a mathematical checker appropriate to that type.

| Subject/example | Typed response | Conditions the author must specify | Verification required |
| --- | --- | --- | --- |
| Algebra: solve an equation | Exact scalar or solution set; equation lines | Number domain, excluded denominators, unique/multiple/no-solution cases | Solution-set preservation for steps; original substitution and completeness of final set |
| Trigonometry: solve sin(x)=k | Angle set or periodic family | Degrees/radians, principal branch, interval and endpoints | All and only solutions in that domain; periodic equivalence |
| Calculus: antiderivative | Expression and constant/family | Interval, differentiability, singularities, meaning of C | Differentiate the candidate on the stated domain; numerical samples alone do not prove identity |
| Linear algebra: solve Ax=b | Matrix, vector or affine family | Dimensions, scalar field, coefficient vs augmented matrix | Row equivalence, residual, consistency, span and completeness; one working vector is not a whole solution set |
| Discrete mathematics: count or prove | Integer, finite set, construction or proof | Ordering, replacement, indistinguishability and quantifiers | Independent enumeration/formula for bounded counts; explicit rubric or formal support for proofs |
| Probability/statistics: compute or infer | Rational/real, distribution, interval or explanation | Model, dependence, sample/population convention, units and rounding | Exact model/reference or stated numerical tolerance; verify assumptions and interval interpretation |

Do not fit every topic into a scalar answer. For proof or modelling questions
without an adequate automatic checker, show **Submitted / compare with solution**
and an explicit self-review rubric. They can support all four presentation
levels, but must not be labelled automatically verified derivations.

## Ownership and evidence

`LayeredQuestionSession` remains the canonical owner of answers, working,
attempts, help exposure and completion. Extend its semantic command/evidence
route for support selection and typed submissions; do not start a parallel
question session in the UI. `CorpusPractice` remains the catalogue and saved
replay adapter. UI sends intent and displays a redacted projection from the
owner. Authoring tools prepare examples and verification certificates; they
are not a runtime judge. The textbook owns reusable teaching definitions and
examples; its lab exercises retain their separate state.

The earlier prepared Library Method panel remains read-only. Supported questions
use guarded semantic actions for level selection, draft editing, symbolic
choice, typed blank, written checking, help and Undo. The canonical run records
guided exposure, definitions, hints, next-line and solution reveals separately;
the UI gives a compact Guidance used label. `CorpusStarter.level`
currently means taxonomy (`subject/chapter/subcategory/practice`); do not
repurpose it. Add a separate support field. `QuestionInteraction` selects a
mechanic, and `MathObjects` levels describe lab topics; neither is this support
level. These similarly named values must remain distinct.

`CorpusPractice` writes save version 2 and reads versions 1 and 2 at the existing
save location. Original two-element prepared command records keep their
meaning. New named support records carry question/run/revision guards; replay
rebuilds checked outcomes, nodes and exposure through the same owner. Consecutive
unsubmitted draft edits are coalesced. The ID resolves independently of bank
order. A saved question with a changed content stamp is retained and reported;
this first pack freezes its inline teaching as well as its mathematical data,
so teaching edits need an explicit migration before they can reopen old runs.

Freeze the instance ID, mathematical version, parameters and answer contract
when an attempt begins. Persist the initial support level, changes of support,
definition views, hints, next-line reveals, full-solution reveals and prior
exposure to that instance. Closing reading never deletes its exposure record.
Preserve draft text, checked lines, correction attempts and branching history.
Changing presentation never regenerates a question or rewrites a submitted line.

If a learner changes levels mid-attempt, retain the same mathematical state and
draft. A prepared cue may resume only at a state the owner can match. If the
learner has taken an unsupported alternative path, offer general help while
retaining it; an explicit fresh guided attempt can archive it. Never silently
replace the learner's work with the reference route. Returning to level 4 does
not erase earlier guidance. Offer a new sibling instance for a fresh independent
attempt. Reading a general definition can be reported separately from revealing
a next step; both are observable, neither is fabricated mastery.

Record separate outcomes: answer correctness; supported line/derivation checks;
help used; first try versus corrected; and self-review. A correct final number
does not certify an unchecked derivation. Valid input in an unsupported grammar
gets **Not checked yet**, not **Wrong**. Keep raw written work. Never execute
student text, infer an answer key from display TeX, or pretend typed work was
the selection of a prepared correct tile.

## Build sequence and acceptance contract

The first implementation is one vertical slice: this exact linear family in all
four levels, with save/resume. It reuses the existing exact linear parser and
`checkMathMove` under the question owner where their contracts apply. They
already handle bounded linear expressions; they are not a general CAS, free
proof checker. The added native editor composes plain equation lines. Their check must
prove the same solution set, not merely accept any equality true at the answer.

The implemented sequence is: typed content and support projection; semantic
submissions and exposure; persisted replay and drafts; four UI presentations;
then the native input route. Old prepared content keeps its existing route
and saved identity. Only after this slice works should the second family use
the contract; matrix questions are a useful cross-subject check.

The acceptance cases for this capability are concrete:

- All four views share identical givens/domain/result; levels 3–4 contain no
  future working, answer-key IDs, hidden auto-filled values or default examples.
- Learn exposes the current definitions/why. Practice has a real blank. Solve
  accepts a full checkpoint. Independent accepts an editable multiline solution.
- The reference and valid alternative route above pass. A wrong equation,
  `0=0`, division by zero and an incorrect final result do not pass. A supported
  equivalent fraction does pass. Unsupported syntax retains the draft and
  reports the limitation without recording a mathematical error.
- Wrong answers and help preserve working. Support changes and a restart do
  not erase help already used. A fresh sibling has a distinct stable instance.
- Closing/reopening retains mode, draft, checked lines, attempts and exposure;
  old question saves migrate without rewriting their mathematics.
- Actual headless inputs at 1440×860, 800×600 and 360×480 prove the active
  response and reading controls are reachable. Typesetting checks are separate
  from mathematical correctness. The user performs any visual confirmation.
- Completed work waits for explicit Next, with no reading timer or forced review.

No screenshots, screen captures, preview images or native windows are needed
for the agent's gate. No broad repair or 3D task is part of this slice. Keep
changes uncommitted. The authoring workflow and copyable builder packets are in
[QUESTION_AUTHORING_WORKFLOW.md](QUESTION_AUTHORING_WORKFLOW.md).

### Verification of the first family

Release sorter, gallery and paths builds pass. The two new feature tests cover
100 complete model routes, every published wrong numeric option at Learn and
Practice (200 checks), valid alternate working, atomic failure, disclosure,
guarded input, history limits, archived branches and version-1/version-2 replay.
Actual mouse/keyboard tests complete the four levels at all three specified
sizes. They also check Enter, Escape, fractions, inline help, Undo, Next and
reopened editor drafts. All 250 published formulas typeset without fallback.
Nine further targeted entries cover old routes/saves, the authoring tests and
exact runtime publication. Receipt: `build/four-level-evidence/verification.json`.

```sh
cmake --build b --target sorter paths_four_level_tests paths_four_level_ui_tests -j 6
ctest --test-dir b -R '^(paths_four_level(_ui)?_tests|paths_question_workflow_tests|paths_four_level_publication)$' --output-on-failure
```

For the user's visual check: open **Linear practice 01 → Focus**. Confirm gold
Given stays beside cyan Working while the four level buttons change the input.
Purple Help should open within the same workspace. The green completion should
remain until Next. In Write, enter a partial draft, close and reopen to check its
appearance. No visual acceptance is claimed by the automated tests.
