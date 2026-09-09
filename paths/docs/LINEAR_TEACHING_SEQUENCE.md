# Linear equations: understand each move

Status: built and locally published as `linear_teaching_sequence` version 1.
Visual and teaching acceptance remain pending. The source is
[`chapter.paths.md`](../content/authoring/learning/linear_teaching_sequence/documents/chapter.paths.md),
with original-material attribution in its sibling `authoring.json`.

## What the learner does

Open Library → Algebra → **Linear equations: understand each move**, then
**Start here: balance, choose, explain**. Its eight exercise links share one
chapter. Numbered titles keep the existing Next navigation in the intended order.

| Card | Decision being practised | Original equation | Result |
| --- | --- | --- | --- |
| 01 Read a worked example | Follow and explain two balanced operations; Learn intentionally reveals the current step | 9x+9=36 | x=3 |
| 02 Try the two balanced steps | Use Practice to calculate with the explanation closed | 5x+4=29 | x=5 |
| 03 Explain why balance matters | Choose the equality-preserving rule, rather than calculate another number | 9x+9=36 | A justified transition to 9x=27 |
| 04 Choose a useful operation | Directly cancel the added term, then solve | 4x-7=13 | x=5 |
| 05 Find and repair a mistake | Repair a division that omitted the added constant; recognise a valid alternative route | 6x+12=30 | x+2=5, then x=3 |
| 06 Choose your next line | Recognise equivalent lines without an operation supplied | 7x-5=16 | x=3 |
| 07 Solve a fresh signed equation | Select and verify a value without step cues | -3x+6=15 | x=-3 |
| 08 Return later: an exact fraction | Revisit the skill after a break and explicitly evaluate the original left side | 4x+3=5 | x=1/2; left side equals 5 |

All cards offer multiple choice. Cards 01–02 retain Learn/Practice/Solve/Write
through `linear.v1`; Practice is the suggested level for 02, not a forced switch.
The six reasoning/transfer cards use `choices.v1` with optional Method reading.
They are distinct activities, not extra four-level solving templates. Gold Given
stays beside cyan Working. Corrections are coral; completion stays green until
Next. Supported cards retain Undo. All activities use the existing save/replay.

Card 08 is a suggestion to return later, not an automated review schedule. If it
was already completed, Again opens a fresh attempt and retains the previous run.
Completion, exposure and correctness are recorded; no mastery, retention or
transfer score is inferred. A choice of a supplied answer is still recognition,
even when a prompt has fewer cues.

## Required authoring process

1. **State one learning decision per card.** Write its objective, prerequisites,
   domain and expected result before adding choices. Separate support level from
   mathematical difficulty. A reason question can finish after a justification;
   it must not claim to have solved the entire equation.
2. **Work the mathematics independently.** Solve from the original givens using
   exact arithmetic, check each intermediate line, then substitute into the
   original equation. Explain why operations preserve the full solution set.
   An answer key is not its own verification.
3. **Choose the existing response contract.** Use `linear.v1` for its supported
   two-step numerical family. Use `choices.v1` for a bounded conceptual choice,
   method selection or error repair. Its compiler validates structure and the
   answer key, not the truth of arbitrary reasoning. Review those claims manually
   and add an independent arithmetic/logic test. Do not relabel a conceptual
   choice as independently written work or invent an unsupported checker.
4. **Start teaching with one short direction.** Explain only the definitions
   needed now, then show the actual arithmetic and why it works. Keep Terms
   neutral and Hint directional. Learn may reveal the worked step intentionally;
   do not count clicking that revealed result as an independent check. Put
   detailed typeset work in teaching/readings and concise ordinary prose in why.
5. **Design and explain each distractor.** Every wrong tile in this chapter has
   `@feedback ID | correction`. Tie it to a recognisable error, avoid claiming
   certainty about the learner's thought process, and explain how to recheck.
   A valid alternative operation must not be called mathematically invalid: make
   the requested subgoal explicit, as in 04, and acknowledge the alternative.
6. **Arrange a sequence, not only number variants.** Include worked and guided
   examples, explanation, method choice, error analysis and fewer-cue questions.
   Include signs/fractions only when prerequisites are stated. An invitation to
   return later does not establish retention or create a scheduler.
7. **Check the actual app model.** Exercise every wrong tile, the accepted route,
   unchanged working after rejection, completion waiting for Next, save/reopen
   and supported Undo. Edit feedback through live preview. Check source errors
   and verify earlier published identities/stamps remain unchanged.
8. **Obtain the user's visual and teaching check.** Keep the source preview
   editable; promote approved changes through the existing exporter. Do not
   overwrite immutable releases or silently rewrite published question identities.

The relevant design guidance is to alternate worked examples and problem solving,
ask explanatory questions, and distribute practice over time. See the
[IES practice guide](https://ies.ed.gov/ncee/wwc/PracticeGuide/1). This informs the
design; it is not evidence of learning outcomes for this app.

## Commands and checks

From the Paths root, normal saved play uses `./b/sorter`. For session-only source
preview:

```sh
./b/sorter --documents content/authoring/learning/linear_teaching_sequence/documents --watch-documents
```

The source compiles through the existing document importer. An independent
editable copy can also be created with `export_learning.py draft` before making
wording changes. Changes to published question content require new identities;
changing only the package version does not permit rewriting a saved question.

```sh
./b/sorter --inspect-documents --documents content/authoring/learning/linear_teaching_sequence/documents
./b/paths_learning_document_tests --teaching-sequence content/authoring/learning/linear_teaching_sequence/documents
python3 -B tools/export_learning.py publish content/authoring/learning/linear_teaching_sequence
```

The headless model probe uses isolated temporary saves and source copies. It
completes 16 routes, checks 44 wrong-tile corrections, preserves saves through
catalogue reorder, restores supported Undo branches and verifies live feedback
edits. Independent Fraction-based checks derive arithmetic answers from the
givens. Conceptual answer keys have explicit task/rationale checks. Direct/include
diagnostic cases reject malformed feedback at its source. Evidence is in
`build/linear-teaching-evidence/verification.json`.

Manual check: on 01, choose **36**, then **45** and compare their coral corrections.
Working must stay at the original equation. On 04, choose **Divide both sides by
4**; the correction should acknowledge that division is valid but does not
directly cancel the added term. Try 05 and 06 without Method, finish a card, and
confirm the green result stays until Next. Close and reopen once to check resume.

The next candidate is to use learner feedback from this sequence to revise one
teaching passage or distractor before generating more repetitions. A scheduler,
prerequisite diagnostic and mastery model are separate future capabilities.
