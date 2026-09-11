# Row operations: a reusable teaching family

This is the `row_operations_v1` example for the shared recipe command. It
retains the reviewed Wave 01 row-operation lesson and all six question roles.
The lesson is byte identical; the question template combines the six reviewed
role templates with prefixed field names. No new layout or runtime syntax is
introduced.

From the Paths root:

```sh
python3 -B tools/author_question_family.py init content/authoring/drafts/rows_next --family row_operations_v1 --package rows_next
python3 -B tools/author_question_family.py check content/authoring/drafts/rows_next
```

The destination must be new. Edit recipe.json, lesson.md.in,
questions.paths.md.in and this brief. Writers do not edit Python, calculate
answer keys, assign option IDs or assemble packages. Checking returns an
unpublished ordinary authoring package and immutable evidence. Publication
remains with the existing exporter and coordinator.

## Learning claim and prerequisites

Learners read augmented rows, update every entry under a row replacement,
select a stated elimination, reverse a replacement, repair an omitted constant
operation and verify a pair in both original equations. Prerequisites are
signed arithmetic, substitution and exact fractions. The lesson defines the
two coefficient columns, constant column, ordered pair and retained row.

Correct choices with available reading and corrections do not establish
unaided Gaussian elimination, proof, retention, transfer or mastery. The six
roles describe the purpose of the exercise; they are not support levels.

## Editable originals and limits

Each of sample, practice and fresh_check has one named case per role. Only
original entries and, where supplied in the question, its row multiplier are
inputs. Every row has exactly three integers in [-20,20], in x,y,constant order.

| Role | Case fields and required conditions |
| --- | --- |
| read_notation | `row: [a,b,c]`; b is nonzero and different from c and -c, so both specific sign/order corrections remain true |
| worked_check | `rows: [[a,b,c],[d,e,f]]`, `multiplier: k`; replacing row 2 must reach coefficients [0,1], with c nonzero |
| choose_next_step | `rows`; a and d are nonzero; the correct elimination multiple -d/a is calculated exactly and may be fractional |
| explain_step | `rows`, `multiplier`; c is nonzero so repeating the multiple changes the constant used in its correction |
| repair_error | `rows`, `multiplier`; k is negative, at most -2; the replacement reaches coefficients [0,1], with c nonzero |
| independent | `rows`; the unique solution has x nonzero and x different from y so doubling x and swapping coordinates give distinct wrong pairs |

Every two-row system must have a unique solution. Explicit multipliers are
nonzero integers in [-6,6]. Worked and repair coefficients are coupled: edit
the original rows together when needed to retain the requested [0,1] result.
The checker rejects a singular system, a missed target or ambiguous choices
with the original recipe location. It does not substitute arbitrary distractors.

For example, change the sample worked case to rows `[[1,-1,3],[2,-1,11]]`
with multiplier -2. The reached second row becomes [0,1|5]. The displayed
matrix, all three column calculations, answer and signed-constant corrections
change together. Only this changed mathematical question receives a new ID.

The default separate worked system `[[1,2,6],[3,-1,5]]` is reserved. Do not use
it as an active question. Its solution is (16/7,13/7). If rewriting that lesson,
the coordinator must review the replacement example and its separation from
practice. Same-role originals must differ across the three sets. Renaming a
package or reordering an equivalent system does not establish new coverage.

## Teaching and variation

The default cases retain signed entries and a fresh -3/2 elimination multiple.
The invariant is a complete reversible row operation followed by a check in
both original equations. Change one feature at a time where possible and
record why each new case belongs in its set. Integer input bounds are not a
promise that arbitrary edits form a useful lesson progression.

| Decision | Wrong choices and correction |
| --- | --- |
| Read notation | A sign change or column mixup is corrected by identifying all three original entries |
| Complete a constant | Keeping the old constant or using the opposite sign is corrected with the exact whole-row arithmetic |
| Choose elimination | The wrong destination may preserve solutions but misses the goal; the wrong sign leaves a nonzero x coefficient |
| Explain reversal | Repeating the multiple or erasing the row fails to recover all three original entries |
| Repair an omission | Identify L1 before calculating its consequence; unchanged and single-copy constants are corrected separately |
| Verify a pair | Both original left sides are evaluated for each wrong pair, then compared with their required constants |

The first repair decision identifies the first incomplete line and the
unevaluated missing operation. It must not disclose the corrected constant
before the second decision. Keep the standard lesson blocks start, terms,
rule, condition, worked, errors, practice and summary. The separate worked
example initially exposes only its given; Hint, Answer and Solution stay
independently closed. Routine teaching review belongs to the coordinator.

## Calculated fields

The common runner supplies shared placement/reading fields and role-prefixed
id, title, objective, given, choices_1 and after_1. Repair also has choices_2
and after_2. The mathematical helper supplies these prefixed fields:

| Roles | Fields |
| --- | --- |
| All | answer_1 and wrong_STEP_INDEX_id/label, where INDEX is the checker's zero-based semantic choice index |
| Notation | a, b, c, negative_b, negative_c, calculation_1 |
| Two-row roles | first_0/1/2 and second_0/1/2 |
| Row replacements | k, opposite_k, changed_constant, repeated_constant, wrong_multiple_x, constant_expression, constant_calculation, changed_row, original_row, forward_calculation, inverse_calculation |
| Independent | x, y, first_constant, second_constant, calculation_1, wrong_0_left_0/1 and wrong_2_left_0/1 |

For example, `{{worked_check_constant_calculation}}` follows numerical edits.
Use the supplied wrong-choice ID fields in feedback; display positions vary
across roles and sets while each correction remains attached to its choice.
Keep the mathematics in calculated fields instead of hardcoding an answer in
the prose.

## Verification and handoff

The existing exact matrix checkers own solution, row replacement, inverse,
residual and distractor policy. The small provider validates original inputs
and reuses the frozen Wave 01 calculated teaching fields. Its historical
recipe, ordering and packaging functions are not called by this workflow.

The common native compiler/model gate checks 18 questions, 21 decisions,
42 wrong choices, saved progress and three independent worked disclosures.
Tests compare all compiled teaching fields with the frozen reference, use
independent rational Gaussian elimination on a finite sample pool, exercise
actual Markdown/number edits and reject invalid originals, keys and working.
The sample pool is not exhaustive over all six bounded matrix entries.
Inputs, imported code and executable hashes are recorded in each receipt.

The [Wave 01 design](../../production/wave01/linear_algebra/DESIGN.md) records
its OpenStax review and original Paths authorship. This adapter claims no new
web review or copied exercise text. Published Wave 01–04 files remain frozen.
The default scaffold repeats reviewed mathematics and stays unpublished; it
demonstrates authoring reuse, not additional curriculum coverage. Arbitrary
matrices, singular systems, higher dimensions, written work and new teaching
methods require a separate reviewed provider. Visual appearance and learner
outcomes remain unobserved; checks use no windows or captures.
