# Linear textbook companion

This is the reference for teaching depth and reading presentation after the
user's inspection of the `math_lab` textbook. Compact controls must not remove
the explanations a learner needs. The older eight-card sequence remains
published with its original identities; it is not the new depth standard.

The source is `content/authoring/learning/linear_textbook/documents/chapter.paths.md`.
One `lesson.v2` contains thirteen blocks, fifteen local references and eight
independent disclosures, connected to three button-answer questions:

| Question | Purpose | Result |
| --- | --- | --- |
| 01 / Understand each move | Work through 3x + 5 = 20 | x = 5 |
| 02 / Remove a negative constant | Apply the method to 4x - 7 = 13 | x = 5 |
| 03 / Explain why subtraction is allowed | Justify 5x + 6 = 21 becoming 5x = 15 | Equal subtraction and its inverse preserve solutions |

The first two retain all four support levels; Learn and Practice are completely
solvable with buttons. The third uses the established prepared-choice route.
Question 3 checks a reason, not the completion of its numerical equation.

## What the next author must preserve

1. Introduce the purpose, prerequisites and mathematical setting.
2. Explain each new symbol or term with an example before relying on it.
3. State the rule and its conditions, including what fails outside those
   conditions. Keep a proof available separately.
4. Write each step as purpose, allowed operation, unsimplified line, arithmetic,
   resulting line and interpretation. Show the cancellation explicitly.
5. Explain a common mistake with a concrete comparison.
6. Substitute into the original equation. Distinguish checking a candidate from
   proving that every solution has been found.
7. Link exercises to definitions and examples. Keep hints, answers and full
   solutions independently selectable.

Use ordinary prose in `@prose`, LaTeX in `@display`, and stable local block IDs
in `@reference`. Put the complete textbook in `lesson.v2`. All three question
templates can attach it with `@read lesson_id`; `linear.v1` and `matrix.v1` still
require their current-step definitions and teaching. These serve the compact
solving help while the linked textbook supplies the full context.

The overview and question share `drawDocumentReading`, which delegates to the
existing `drawBookBlock` and `bookLessonView` used by `math_lab`. Reading uses a
bounded centred column, wrapped left-aligned prose, teal headings, separate
equation rows, 90–200% text size and a jump control. Wide question workspaces
place reading beside solving with a movable divider. Narrow workspaces place
it below the fixed problem and choices. No separate exercise or figure session
is constructed.

Learn opens the textbook. Changing support level or attempt closes disclosures;
Practice, Solve and Write leave it closed until requested. Closing guidance
does not erase its recorded use. The guarded `ReadReference` question action
records the kind of reading opened without selecting the active question's Help
tab, revealing that question's solution, changing working or grading an answer.
Its journal uses the existing version-2 save format with action `reference`.
Reference guidance has separate exposure bits; it does not claim that the
active question's answer was shown just because a related example was opened.
Prepared questions keep their existing grading and gain no new claim of
independent performance.

## Editing and verification

For session-only Markdown preview:

```sh
./b/sorter --documents content/authoring/learning/linear_textbook/documents --watch-documents
```

Reading-only edits, including closed proofs, update the structured reading and
retain the matching question attempt. Question teaching, answer or mathematics
edits change the stamp and start a separate preview revision. Published question
identities remain frozen; use new identities for changed question content.

The two targeted CTest entries are `paths_textbook_companion_tests` and
`paths_textbook_reading_state_tests`. They cover solving, wrong answers, save
replay/reordering, Undo, completion until Next, independent disclosures,
references, support levels and live proof edits. The reading-state check does
not initialize ImGui, fonts or a native host.

Visual acceptance belongs to the user. Open **Algebra → Linear equations: a
textbook companion → From equal values to an unknown**, then Question 1. Check
the teal headings and proof/solution controls. Keep gold Given and cyan Working
and choices visible while scrolling. Try Practice and the purple Textbook
button. Green completion should remain until Next.
