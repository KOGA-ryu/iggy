# Matrix reasoning in the regular question workspace

Eight follow-up questions add 23 decisions to **Contents → Library → Questions**.
Search **Matrix reasoning**, select a question, then choose **Focus**. The
original 278 starters retain their content and saved mathematics.

The new layout keeps **gold Given** and **cyan Working** together above the
symbolic choices. Given remains fixed throughout the problem. Working advances
after a correct choice; a wrong choice records an attempt and retains the
current state. At completion, working turns **green** and stays until **Next**
or **Again**. Again archives the completed run before starting a fresh attempt.
Focus hides browsing controls; **Browse** or Escape restores them without
changing the question. Fonts and buttons retain the compact native style.

The **purple Method** button opens existing textbook definitions below the
choices. Its label becomes **Working** to return to the reached explanations.
Given, current working and choices remain in place. Method changes no answer,
step, journal, score or textbook exercise state. Its small selector switches
between relevant definitions. Method is optional and is not a solving step.

## Question and result sheet

All questions have stable IDs `matrix_reasoning_01` through
`matrix_reasoning_08`, at content version 1. They appear under Linear algebra,
topic `topic_0094`, with `level: practice`. Matrices without an augmented bar
are explicitly coefficient matrices. Augmented systems use a visible bar and
carry the right-hand side through every row operation.

| ID suffix | Question | Decisions | Intended result |
| --- | --- | ---: | --- |
| 01 | Clear below, then above | 2 | RREF of `[1,2; 3,7]` is the 2 by 2 identity |
| 02 | Swap and scale | 3 | Swap rows, scale by -1/2, then clear above; RREF is the identity |
| 03 | Fractional pivots | 3 | RREF of `[2,1,3; 0,3,6]` is `[1,0,1/2; 0,1,2]` |
| 04 | REF is not always RREF | 3 | `[1,3,0; 0,1,2]` is REF; clear entry b12=3 to obtain `[1,0,-6; 0,1,2]` |
| 05 | Two free variables | 3 | For A=`[1,2,-1; 2,4,-2]`, Ax=0 has x=`(-2s+t,s,t)` |
| 06 | Carry the right-hand side | 3 | From `[1,1|5; 2,3|12]`, obtain `(x,y)=(3,2)` |
| 07 | Detect no solution | 3 | `[1,2|3; 2,4|8]` reduces to a contradictory row, 0=2 |
| 08 | One free variable | 3 | `[1,-1|2; 3,-3|6]` has `(x,y)=(t+2,t)` |

Parameters s and t range over the real numbers. Operation prompts specify a
local goal; choosing a different valid elementary operation need not meet that
goal. This is guided symbolic practice, not arbitrary row-operation input or
independent proof assessment.

Question 01 adapts the existing textbook's `rref.example` matrix. The remaining
questions are newly authored follow-ups. Method references use only the neutral
definitions and propositions, not that worked example. Existing source problem
pages and the pinned corpus were not edited.

## Roles of responsibility

| Area | Canonical owner | Boundary |
| --- | --- | --- |
| Regular question answers and working | `LayeredQuestionSession` | Judges selected tiles, commits prepared working, records attempts and publishes reached explanations |
| Question catalogue and saved progress | `CorpusPractice` | Loads stable question identities, routes commands and replays saved journals |
| Question presentation | `CorpusPracticeUi` / `MathCorpusUi` | Draws Given, Working, choices and reading; Focus and Method affect presentation only |
| Teaching material and reading navigation | Textbook authored blocks / `Textbook` | Defines concepts, worked examples, proofs and reading disclosures/bookmarks |
| Existing lab exercises | `MatrixBoard` / `SystemLesson` | Retain their own operations, exploration and practice judgments in the lab |
| Figures | `RowPlaneFigure` and its scene/UI adapters | Project a supplied mathematical state into equation planes and solution geometry |
| Typesetting and rendering | `NativeMath` / native Vulkan host | Lay out notation and draw existing scene/UI data; do not judge answers |

The other worker's side already contains both teaching and lab practice. This
checkpoint reuses teaching text while leaving that practice under its current
owners. `paths_sorter_ui` links the existing `paths_textbook` target; it does
not instantiate a textbook, board or system exercise to open Method.

The six shared reading IDs are `matrix.entries`, `rref.operations`,
`rref.equivalence`, `rref.echelon`, `rref.reduced` and `rref.preservation`.
The projection accepts Definition/Proposition bodies and a section's general
explanation. It does not read help disclosures, examples, exercise answers,
solution fields, board state or automatic figure results. A missing reference
shows an unavailable message while retaining the active question.

The lab's current plane provider supports both Ax=0 and explicit Ax=b. A future
question-linked figure should receive the question owner's current coefficient
matrix and explicit right-hand side, together with question/version/step
identity. It must respect the question's current answer disclosure. Its camera
and visual settings can be independent; its mathematical state cannot drift
into a different exercise. Do not parse display TeX or use test-only check
records as runtime truth, and do not add a second independently mutable board
whose Check/Reveal overrides the regular question's result.

## Authoring and persistence

`tools/generate_matrix_reasoning.py` produces
`content/corpus/matrix_reasoning.json`. The native app appends this pack to the
existing starter bank through the same loader and question owner. Combined
catalogues reject duplicate stable IDs and remain bounded at 1,024 questions.
Optional `reading_refs` are bounded and unique within a question; reference
metadata is outside the mathematical save stamp.

Existing question journals therefore survive adding or reordering this pack.
New questions save through the existing journal route, including an unfinished
multi-step attempt and wrong choices. Editing a reading link does not invalidate
unchanged mathematics. No persistence schema or runtime answer checker was added.

This capability changes six existing production C++ files: **+98/-12 lines,
net +86**, with zero new production C++ files. It adds one authoring generator,
one generated content pack and two focused test files. No conflicting answer
route was introduced or removed. The other worker's concurrent textbook and
solution-set changes are outside this delta and are preserved.

## Verification and deferred visual check

The Release `sorter` build and four targeted CTest entries pass:

- Exact independent arithmetic checks: 14 row operations, 28 alternative
  operations, REF/RREF conditions and four complete solution-set checks.
- All eight model routes, 23 decisions and 46 wrong tiles; prior-bank save
  migration, new unfinished-question replay and reading-only state preservation.
- 24 complete headless UI routes: all eight questions at 1440×860, 800×600 and
  360×480, with actual Focus, Method, answer, Next and Browse input.
- 108 new equation/working/tile instances and all six reading references typeset
  without fallback. Existing starter checks retain 278 complete routes, 834
  wrong choices and 2,502 notation instances across the same three window sizes.

`sorter --check-content` validates 100 sortable cards, 930 Library entries,
278 starters and eight follow-ups before constructing a native host. The
generator reproduces its JSON exactly. See
`build/matrix-reasoning-evidence/verification.json` and the adjacent build and
CTest logs. Headless UI checks use in-memory font atlas data; no windows,
screenshots, captures or image artifacts were produced.

When ready, launch `/Users/kogaryu/iggy3d/paths/b/sorter` and search the Questions
view for **Matrix reasoning**. In Focus, look for **gold Given beside cyan
Working**, both near the choices. Open **purple Method** and confirm it keeps
your place. After finishing, the **green result** should wait for Next. Visual
acceptance and pointer feel remain for the user; no review is requested now.

Motion is shelved, with its code and progress untouched. A later candidate is
one question-linked figure following the ownership boundary above. This
checkpoint stops at regular questions and shared method reading. Changes remain
uncommitted.
