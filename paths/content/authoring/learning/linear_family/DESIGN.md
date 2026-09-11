# Balanced equations: a complete family authoring example

This implements one bounded family from the
[problem-design research](../../../../docs/MATH_PROBLEM_DESIGN_RESEARCH.md).
It follows the reusable [teaching brief](../../../../docs/templates/QUESTION_FAMILY.md).
It is the first registered recipe for `tools/author_question_family.py`.
Published production waves and their pinned tools remain unchanged.

## Learning claim and prerequisites

The learner can read the signed parts of a real linear equation, select an
operation that preserves its solution set and achieves a stated goal, explain
its inverse, locate an earlier sign error, and select an exact solution in a
fresh presentation. Required ideas: integer arithmetic, fractions as numbers,
multiplication notation, equality, ordered pairs and sequential lines.

This sequence assesses selected decisions with optional linked reading. It
does not assess independently written proofs, long-term retention, arbitrary
algebra or mastery of the chapter. The final item is uncued by intermediate
steps; the earlier items intentionally supply more help. These are the existing
six authoring roles, not six runtime support levels.

## Mathematical contract and generator

Original problems have real x and ax+b=c. A nonzero a gives exactly one solution:
subtract b from both sides and divide by a; substitution verifies existence.
If u,v both solve the equation, a(u-v)=0 implies u=v, proving uniqueness.

[recipe.json](recipe.json) supplies shared package/placement/source metadata and
three finite sets under `parameters`. For each set, choose a
positive even a in {2,4,6,8}, positive b from 1 through 9 different from a, and
an integer seed solution from 2 through 5. The fresh solution is one of
{-3/2,-1/2,1/2,3/2}. The generator calculates c=a*s+b after applying each role's
planned coefficient/constant signs. It passes only original coefficients and
orientation to the certificate, never the intended seed solution.

The certificate independently derives (c-b)/a with exact Fraction arithmetic,
checks original substitution, and verifies each role's operation or candidate.
It rejects zero/unbounded coefficients, incorrect sign/orientation strata,
duplicate or accidentally correct options, and unsupported fresh-answer forms.
The goal question deliberately includes two equivalent equations but exactly
one that also removes the added constant. Numerical distinctness alone would
not verify that question. Symbolic option meaning is checked before formatting.

Zero coefficients, variable denominators, inequalities, quadratics and arbitrary
free-form algebra are outside this family. The reading explains zero-coefficient
boundaries; it does not generate playable boundary cases.

## Variation plan and sample answer sheet

| Role / feature | Sample original | Deliberate change and decision | Checked answer |
| --- | --- | --- | --- |
| read_notation / positive_constant | 6x+8=26 | Establish coefficient versus added constant | (a,b)=(6,8) |
| worked_check / negative_constant | 6x-8=10 | Hold coefficient and solution fixed; reverse b's sign and adjust c | 10-(-8)=18 |
| choose_next_step / negative_coefficient_and_goal | -6x+8=-10 | Against notation, hold b and solution fixed; negate a and adjust c; distinguish equivalence from the cancellation goal | -6x=-18 |
| explain_step / reversible_scaling | -6x=18 → x=-3 | Isolate the nonzero inverse rule with a negative divisor | Multiply both sides by -6 |
| repair_error / first_error | 6x-8=10 with L1: 6x=10+(-8), L2: 6x=2, L3: x=1/3 | Revisit the signed-constant case; separate first invalid operation from subsequent correct arithmetic | L1, then 18 |
| independent / reversed_fraction | 11=6x+8 | Hold positive coefficient and constant fixed; fresh exact fractional solution and reversed equality deliberately combine two previously explained features | x=1/2 |

These are purposeful comparisons across the set, not a claim that each adjacent
item changes exactly one feature. The reading demonstrates reversed equalities
and fractional solutions separately before their combined fresh application.
Its worked examples (4x-3=6, 7=4x+2) use different numbers and answers from the
final questions in all three sets. The half-integer fresh answers differ across
sets: sample 1/2, practice 3/2, fresh_check -1/2.

## Decisions and distractors

| Role | Wrong choice 1 | Wrong choice 2 | Required correction |
| --- | --- | --- | --- |
| read_notation | Negate the actual added constant | Swap coefficient and constant | Read the written sign and each term's job |
| worked_check | Leave c unchanged | Compute c+b instead of c-b | Operate on both sides; undo the signed constant |
| choose_next_step | Add b to both sides | Remove b only on the left | The first is valid but misses the goal; the second changes solutions |
| explain_step | Divide by a again | Multiply by zero | Repeated division is valid but not the requested inverse; zero loses information |
| repair_error, decision 1 | Blame L2 | Blame L3 | Those lines follow the incorrect L1 correctly; locate the first failure |
| repair_error, decision 2 | Repeat c+b | Leave c unchanged | Subtract the signed b on both complete sides |
| independent | Negate the correct candidate | Stop at ax=c-b and mistake c-b for x | Substitute the selected candidate; distinguish x from its nonunit multiple |

Feedback describes the selected expression, not an asserted diagnosis of the
learner. Each six-question set has seven decisions and fourteen specific
corrections. The complete package has 18 questions, 21 decisions and 42
corrections, with one common lesson. Identical originals for the same role in
different sets are rejected; a reserved fresh check must be a different problem.

The shared runner keeps each semantic option ID attached to its feedback and
rotates presentation positions deterministically. Each first-answer position
occurs twice per set; later decisions vary by step. Titles receive continuous
numbering and a sample/practice/fresh-check label. Stable question IDs combine
the registered family/version, package namespace, set, role and original-case hash. Package version and
wording do not independently change those IDs. New mathematics receives a new
ID. Wording changes still require existing content-stamp/publication review.

## Reading and disclosure review

The existing lesson.v2 native textbook owns layout. The lesson defines unknown,
coefficient, signed constant, equality, solution and solution set before using
them. It explains both directions of each equivalence, the nonzero condition,
signed arithmetic and substitution into the original equation.

The two complete example solutions and their answers/hints are independently
closed. General explanations and useful-versus-valid contrasts are public.
Question domains state meanings and task conditions without naming the correct
option. After the first error-identification decision, cyan working shows only
L1, so the repair arithmetic remains a decision. The fresh question has no hint
or prescribed intermediate step. Its answer and checking explanation appear
only after a response through the existing session.

The coordinator reviewed the template's mathematical claims, the 6x sample
arithmetic, the deliberately incorrect attempt, the distinction between a
valid move and a requested goal, and answer disclosure. Automated template
checks cannot establish teaching clarity or correct native typesetting. The
coordinator owns routine content review; user feedback is optional at product
milestones. No learner effectiveness or retention result is claimed.

## Exact authoring and checking workflow

Work from `/Users/kogaryu/iggy3d/paths`:

```sh
python3 -B tools/author_question_family.py check content/authoring/learning/linear_family
```

The common command reuses `fill_template`, `choices_text`, `chapter_text`,
`reasoning_certificate`, `verify_role_content`, the real document compiler,
`--question-batch`, `--family-lessons` and exporter provenance/immutable writing.
It checks all 18 actual compiled routes, wrong responses, saves and independent
worked disclosures. It checks source and tool/binary stability before recording
acceptance. The provider retains only finite mathematical generation, exact
certificates and calculated teaching fields; its old standalone build and
receipt route was removed. The frozen pilot checker remains historical tooling.

The command prints a compact handoff with counts, immutable authoring path and
verification path. Full evidence and the exact four authoring inputs are under
`build/question-families/PACKAGE/CONTENT_HASH/checks/CHECK_HASH/`. Generated
authoring consists of metadata and one ordinary `.paths.md` chapter. No Python
is copied into it, and no author-supplied Python path can select a provider.
Checks have separate identities so changed tools can verify identical content
without replacing historical evidence. No package is published by this command.

To create another editable instance of this family:

```sh
python3 -B tools/author_question_family.py init content/authoring/drafts/balance_next --family linear_balance_v1 --package balance_next
python3 -B tools/author_question_family.py check content/authoring/drafts/balance_next
```

The destination must be new. Edit its recipe, lesson, question template and
design brief; rerun `check` after an edit. Unknown fields report the source
template and line; invalid parameters report the recipe field. Rejected inputs
produce no accepted package. The full check is also registered as
`paths_question_family_tests` in CTest and covers the complete 512-seed domain,
real text/number edits, corrupt keys/working and source changes during a check.

All three sets are assembled together. Their names describe intended teaching
use, not access controls; no due-date scheduler or mastery score is implemented.

### Template fields for writers

Use `{{field_name}}` through the existing template filler. The complete question
template supplies examples of every mathematical field. Each question field
starts with its role, for example `{{worked_check_rhs}}`.

| Field suffix | Supplied value |
| --- | --- |
| id, title, objective | Generated identity/numbered title and the recipe's learning objective |
| given | The original problem in the family's mathematical notation |
| a, b, c | Signed coefficients from that role's original ax+b=c |
| opposite_b, twice_b, c_plus_b, rhs | Exact -b, 2b, c+b and c-b calculations |
| solution | Exact solution derived from the original equation, as integer or fraction text |
| neg_candidate_lhs, undivided_lhs | Original-expression evaluations for the two fresh-question mistakes |
| choices_1, after_1 | Complete choice/answer directives and reached working for decision one |
| choices_2, after_2 | The second decision of repair_error only |

Role prefixes are read_notation, worked_check, choose_next_step, explain_step,
repair_error and independent. Shared fields are subject, subject_title, chapter,
chapter_title, reading_id and reading_title; questions also receive set_title.
The lesson uses shared fields and its own distinct worked examples.

Keep `{{ROLE_choices_1}}` as a complete directive block. Feedback 12 and 13
describes the first and second mathematical mistakes, regardless of button
position; the correct semantic ID is 11. For the repair's second decision,
those IDs are 22/23 and 21. The runner changes display order while retaining
those associations. Do not replace calculated fields with guessed answers.
New variables or mathematical operations require an addition to the reviewed
provider; arbitrary expressions are not evaluated inside template fields.

For an editable app preview, pass the printed authoring path to the existing
exporter. `NEW_FOLDER` must not already exist:

```sh
python3 -B tools/export_learning.py draft AUTHORING_PATH --output NEW_FOLDER
b/sorter --documents NEW_FOLDER/documents --watch-documents
```

Preview is session-only. It does not replace the active published library or
personal saved work. Publication and later scheduling remain separate work.

## Writer handoff and stop conditions

This is the concrete example for a future bounded subject assignment. Reuse the
teaching brief, existing role sequence schema, Markdown block/choice syntax and
shared gate. Define a subject-specific mathematical certificate; do not copy a
linear arithmetic oracle into trigonometry or calculus. List the exact approved
source files and output directory in each assignment, as the current pilots do.

For this family, editable originals are recipe.json, lesson.md.in,
questions.paths.md.in and this brief; coordinator-owned generate.py/tests.py
define and test the supported mathematics. The shared recipe schema, provider
registration and pipeline belong to tools/author_question_family.py.
Do not edit shared tools, reserved
pilot sources, generated candidates, runtime C++, personal progress or 3D assets.
Changing the checker domain requires coordinator review and new meaningful
counterexamples. Return the candidate path, receipt, answer/variation sheet,
rejected-case evidence and remaining manual checks; stop after one family.

Use original prose and generated problems. The associated research note provides
pedagogical sources and limitations; no external exercise text was imported.
