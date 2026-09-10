# Reusable exercise roles

This is the authoring contract for a six-card sequence alongside a chapter's
numerical repetitions. It is implemented for probability and matrix row
operations. The first implementation is **Probability and Statistics
→ Finite probability: count and compare → Reason about probability**. The
sequence uses the accepted native textbook and existing multiple-choice owner.
No runtime role field, parser, renderer, save format or scoring policy is added.

## Six different decisions

| Role ID | Learner decision | Required mathematical evidence | Probability card |
| --- | --- | --- | --- |
| `read_notation` | Interpret a symbol or expression before calculating | `notation_interpretation`: the interpretation and its original domain | 01 Read the event |
| `worked_check` | Complete one part of a shown worked example | `worked_example`: supplied work and the checked missing quantity | 02 Complete a worked fraction |
| `choose_next_step` | Select a calculation meeting the stated goal and conditions | `method_condition`: the applicable condition and rejected alternatives | 03 Check the counting rule |
| `explain_step` | Choose the facts that justify a transition | `justification`: the actual relationship establishing the step | 04 Explain the complement |
| `repair_error` | Identify the first error, then carry a correction forward | `first_error`: the earliest contradiction and the corrected result | 05 Repair a counting error |
| `independent` | Solve a fresh problem with fewer supplied cues | `fresh_context`: original outcomes and independently checked result | 06 Try a fresh token problem |

Role-format version 1 supplies one card per role in this order. Numerical
repetition counts are separate. The final role uses one uncued multiple-choice
step, with optional linked reading; the name does not claim independently
written work, unassisted completion, retention or mastery. Existing linear and
matrix support levels retain their own meaning and checking contracts.

## Where to write

The probability example consists of three editable authoring sources:

- [`sequence.json`](../content/authoring/learning/probability_reasoning/sequence.json):
  stable IDs, roles, titles, objectives, prerequisites and original cases.
- [`questions.paths.md.in`](../content/authoring/learning/probability_reasoning/questions.paths.md.in):
  the actual question wording, displayed mathematics, choices, corrections,
  reached working and explanations, using the existing document grammar.
- [`lesson.md.in`](../content/authoring/learning/probability_reasoning/lesson.md.in):
  numbered definitions, worked examples and separate closed disclosures.

The JSON has only `format`, `format_version` and `questions` at its root. Each
question has exactly `id`, `role`, `title`, `objective`, `prerequisites` and
`case`. The first five are nonempty bounded strings. `case` is a nonempty object
whose fields are defined and checked by that mathematical family. It contains
original inputs, not a copied accepted-answer ID. IDs are unique; each role
appears once in teaching order. Unknown roles or missing prerequisites report
the source filename and JSON field path.

Markdown uses fields such as `{{read_notation_id}}`,
`{{read_notation_title}}`, `{{read_notation_objective}}` and `{{reading_id}}`.
The displayed mathematical examples remain readable in Markdown. Their numbers
also appear in the independent case description, so changing one without the
other must fail checking. Unknown template fields report the source file/line.
The shared `chapter_text()` uses the existing chapter template to assemble the
reading, ordered practice links and authored questions.

Draft generated documents into a fresh folder to preview Markdown edits:

```sh
python3 -B tools/export_learning.py draft build/question-batches/finite_probability_practice/2/authoring \
  --output content/authoring/drafts/probability_roles
./b/sorter --documents content/authoring/drafts/probability_roles/documents --watch-documents
```

That preview uses in-memory progress and does not publish. Never edit installed
immutable authoring/export/store files. A changed question needs a new identity
and a release retaining existing questions; a larger package version alone does
not permit changing its old content stamp.

## Checker boundary

`validate_role_sequence()` in `tools/build_question_batch.py` owns the shared
metadata contract. `verify_role_content()` owns comparison with the **actual
compiled question JSON**: identities, objectives, response mode, public givens,
reached states, step counts, choice labels, accepted values and per-choice
corrections. It requires a certificate of the role's declared evidence kind.
The final role additionally remains one step without a hint field. The document
compiler already rejects unsupported help directives in prepared-choice cards.

The mathematical family supplies independent certificates. This probability
sequence enumerates finite event members, sums exact `Fraction` probabilities,
checks complement intersection/union and enumerates physical tokens. It checks
positive outcome weights summing to one and avoids the unequal-chance example
where naive counting accidentally gives the same answer. For error repair, the
certificate identifies zero as the forbidden label and L1 as the first error.
The authored Markdown and accepted choice IDs do not produce these certificates.

The generic role checker is not a natural-language fact checker. A builder must
review the prompts, assumptions, explanations and distractor corrections for
truth and teaching quality. Role names and nonempty metadata do not establish
those qualities. A different subject needs its own mathematical certificate
provider; assigning it a role does not create a solver or prove an answer.

The teaching definitions were checked against
[OpenStax 3.1, Terminology](https://openstax.org/books/introductory-statistics-2e/pages/3-1-terminology).
Adding probabilities of distinct single outcomes uses the mutually exclusive
case of the addition rule in
[OpenStax 3.3, Two Basic Rules of Probability](https://openstax.org/books/introductory-statistics-2e/pages/3-3-two-basic-rules-of-probability).
Examples, wording and exercises here are original Paths material.

## Matrix row-operation sequence

**Library → Linear Algebra → Worked matrix practice → Reason about row
operations → Exercise** contains the second six-role sequence. It is packaged
as `matrix_reasoning_practice` version 1, alongside existing matrix practice.
Its six questions use `choices.v1`; the earlier matrix questions retain their
four support levels. This sequence has seven checked decisions, because the
error-repair card first identifies the line and then repairs its value.

| Card | Decision and checked result | Accepted choice IDs |
| --- | --- | --- |
| 01 Read an augmented row | `[2, -1 \| 5]` means `2x-y=5` | 11 |
| 02 Complete the constant column | `R2 ← R2-2R1` changes `[2, 5 \| 13]` to `[0, 1 \| 3]` | 12 |
| 03 Choose the cancelling operation | Subtract three copies of row 1 from row 2; new row `[0, 4 \| 4]` | 13 |
| 04 Explain reversibility | Add two copies of the retained row 1 back; recover `[2, -1 \| 5]` | 13 |
| 05 Repair the first row error | L1 omitted the constant subtraction; corrected `y=11-2(4)=3` | 11, 23 |
| 06 Solve a fresh system | `(x,y)=(3/2,1)` satisfies both original rows | 12 |

The correct first button occurs twice in each position. Every incorrect option
has a correction addressing its actual error. An operation that fails the
requested elimination goal may still be reversible; its feedback makes that
distinction. Deliberately wrong student lines remain labelled as an attempt in
the gold givens; accepted working contains only the reached diagnosis/correction.

The reading defines coefficients, constants, the augmented bar, ordered pairs,
row labels, replacement arrows and primes. It explains whole-row arithmetic,
inverse operations, nonzero scaling, and the difference between validity and an
elimination goal. A separate example solves `x-2y=1`, `2x-3y=4`, with
`(x,y)=(5,2)`. Hint, Answer and Solution are separately closed; the reversibility
proposition has its own proof. Conventions were checked against
[OpenStax College Algebra 2e, section 7.6](https://openstax.org/books/college-algebra-2e/pages/7-6-solving-systems-with-gaussian-elimination).
All examples and prose are original; no external exercise text was copied.

Author these three files:

- [`sequence.json`](../content/authoring/learning/matrix_reasoning/sequence.json): identities, roles, objectives, prerequisites and original numeric cases.
- [`questions.paths.md.in`](../content/authoring/learning/matrix_reasoning/questions.paths.md.in): literal mathematics, wording, choices, corrections and reached working.
- [`lesson.md.in`](../content/authoring/learning/matrix_reasoning/lesson.md.in): native numbered reading blocks and optional disclosures.

The shared `reasoning_documents(root, checkers, values, filename)` supplies
manifest validation, field substitution, practice ordering and chapter assembly
to probability and matrices. `reasoning_certificate()` packages evidence and
prefixes arithmetic failures with the card ID and role. `verify_role_content()`
compares actual compiled questions to certificates. The matrix family supplies
`MATRIX_ROLE_CHECKERS`; it introduces no runtime solver or grading route.

Matrix certificates use exact rational row operations and their inverses.
Cramer's rule independently obtains the unique pair from the original system,
and substitution checks both original rows. Method candidates must meet the
destination/cancellation goal; the inverse must restore every entry. The repair
certificate identifies the omitted constant operation and checks that the
student's resulting pair fails the original system. The final card's three pairs
are checked by their residuals in both equations.

The version-1 case contract is deliberately bounded:

| Role | Original fields and additional conditions |
| --- | --- |
| `read_notation` | `row`: three integer entries, each with magnitude at most 20 |
| `worked_check` | `rows`, `multiplier`; replacement row coefficients must be `[0,1]` |
| `choose_next_step` | `rows`; both original x coefficients nonzero; multiplier derived from cancellation |
| `explain_step` | `rows`, `multiplier`; exactly the inverse restores the original matrix |
| `repair_error` | `rows`, negative `multiplier`; new coefficients `[0,1]`, source constant nonzero, omitted constant must change the solution |
| `independent` | `rows`; only one checked candidate pair satisfies both equations |

`rows` is exactly two augmented rows of three bounded integers in `(x,y,constant)`
order. A supplied multiplier is a nonzero integer with magnitude at most six.
These five system cases require a nonzero coefficient determinant. Each decision
must have three distinct choices and one correct result. Unsupported cases fail;
this is not an arbitrary matrix-question generator. New numbers must agree
between the manifest and Markdown and still support each explanation. Automatic
arithmetic checks do not establish the truth of free-form prose.

From the Paths root, verify and export with:

```sh
python3 -B tools/build_question_batch.py --family matrix-reasoning
```

Add `--publish` to activate through the existing publisher. Defaults are package
version 1, role format 1 and exactly six questions. Other counts are rejected.
Generated source is under
`build/question-batches/matrix_reasoning_practice/1/authoring`. Immutable exports
and installed store files must not be edited. Use the three authoring sources
above, or create a fresh preview with the existing draft command. Changing a
published question still requires a new identity and a package retaining it.

## Probability answer and route sheet

| Card | Checked decision/result | Accepted choice IDs |
| --- | --- | --- |
| 01 | E contains 1, 3, 5 and 7 | 11 |
| 02 | Denominator 6; 2/6 = 1/3 | 12 |
| 03 | Add 1/2 and 1/4; P(E) = 3/4, not the count-only 2/3 | 13 |
| 04 | Empty intersection and full union justify P(E complement) = 7/10 | 13 |
| 05 | L1 wrongly includes zero; repair to E = {3, 6, 9}, P(E) = 1/3 | 11, then 23 |
| 06 | Nine non-red tokens among twelve; P(not red) = 3/4 | 12 |

## Builder handoff

Assigned subject workers use the [parallel packet](PARALLEL_QUESTION_AUTHORING.md)
for exact cases, writable files and the shared headless candidate command.
The coordinator owns shared checker registration, builds and publication in
steps 4–6 below; the worker stops with checked content and a teaching review.

1. Pick one mathematical family and specify original inputs, domains,
   prerequisites and exact expected decisions for all six roles. Use the closest
   accepted lesson; keep the native layout and existing response contract.
2. Fill the six-role manifest. Write each question in ordinary `.paths.md`
   syntax, explaining wrong choices and the reason for each accepted step.
   Label deliberately incorrect work explicitly. Avoid ambiguous alternatives
   and answer-position patterns. Keep the final problem's public cue short.
3. Write separate teaching examples and neutral definitions in `lesson.v2`.
   Explain every new symbol and condition. Put optional answers and solutions
   in separate disclosures. Do not insert the final exercise's answer into its
   public prompt or hint.
4. Implement the family's certificate provider against the original inputs.
   Reuse the shared role contract, chapter wrapper, compiler, model replay and
   publisher. Add no parallel parsing, save or runtime grading route.
5. Prove malformed metadata and changed mathematics cannot reach output or
   activation. Exercise every correct/wrong choice, reopening, catalogue
   reordering, completed-state retention, repeat publication and additive upgrade.
   Verify the earlier published records remain identical.
6. Build and finish the bounded sequence. Leave changes uncommitted; take no
   images, screenshots, windows or font probes. Give the user a short visual and
   teaching check. Do not label the chapter accepted until that check occurs.

Current build and model gate:

```sh
cmake --build b --target sorter paths_learning_document_tests paths_learning_document_ui_tests --parallel 4
ctest --test-dir b -R '^(paths_learning_document_tests|paths_question_batch_tests)$' --output-on-failure
python3 -B tools/build_question_batch.py --family probability --publish
```

Probability defaults to package/format version 2: 12 original repetitions plus
six role questions. `--count 24 --version 3` adds twelve repetitions while
retaining the same six role questions. Keep the meaning of `--count` explicit
when handing off a larger batch. Source, build, publication and user acceptance
are separate evidence; the role registry is not a global coverage/mastery score.
