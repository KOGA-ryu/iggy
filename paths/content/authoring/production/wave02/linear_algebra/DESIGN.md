# Complete two-equation solves and classification

## Learning claim and prerequisites

Read an augmented matrix, choose and execute reversible whole-row operations,
classify its real solution set, and verify the complete result in the original
equations. Prerequisites are signed arithmetic, exact fractions, ordered pairs
and equality. The lesson defines pivots, rank, consistency and free parameters.
Choosing these prepared steps is observable guided solving, not an unaided
written proof or evidence of retention, transfer or mastery.

## Mathematical contract

The finite pool in cases.json has twelve two-row, three-column augmented
matrices. Original entries are integers of magnitude at most 12; the coefficient
matrix has rank at least one. Unknowns and parameters are real. Intermediate
arithmetic uses exact rational numbers without rounding. There are exactly
eight unique, two empty and two one-parameter solution sets. Larger systems,
complex inputs, the all-zero coefficient matrix and arbitrary written work are
excluded. Only these twelve cases, not all bounded combinations, are certified.

Swapping complete rows is its own inverse. Row replacement uses the old source
and destination entries in every column and is undone by the opposite multiple.
Division of a complete row requires a nonzero divisor and is undone by
multiplication. A valid alternative can fail a stated local goal without
changing the solution set. Method choices distinguish these two judgments.

Independent evidence derives the determinant and Cramer numerators from the
original inputs for unique cases, and coefficient/augmented rank from exact
minors for the other cases. All row moves are checked for arithmetic, inverses
and unchanged rank/solution sets. Parameter families are affine polynomials:
constant and parameter coefficients of both original residuals must vanish
identically, not merely at sampled values. One free coordinate and the pivot
equation prove completeness. Inconsistency is also checked directly as two
different required constants for proportional original left sides.

## Variation plan and local decisions

| ID suffix | Group | Structure and skills |
| --- | --- | --- |
| q01 | introductory | Unit x pivot; negative second pivot; complete integer solve and original checks |
| q02 | introductory | Negative y coefficient; positive second pivot; add rather than subtract to clear y |
| q03 | introductory | Zero first x entry; required initial row swap; preserve both constants |
| q04 | introductory | Unit first pivot but two fractional solution coordinates |
| q05 | practice | Negative nonunit first pivot; fractional elimination multiple and negative solution coordinate |
| q06 | practice | Positive nonunit first pivot; negative second pivot; final division of the x row |
| q07 | practice | Proportional coefficient rows with incompatible constants; coefficient rank one versus augmented rank two |
| q08 | practice | Dependent complete equations; y free; normalize pivot and describe every solution |
| q09 | mixed | Choose an initial method for a zero x pivot, then retain fractions through a nonunit x pivot |
| q10 | mixed | Choose a unit pivot without introducing fractions; distinguish valid but goal-missing alternatives |
| q11 | mixed | Entire x column zero; y pivot and contradictory constants; an absent x pivot does not imply consistency |
| q12 | mixed | Entire x column zero; use a y pivot; x free rather than y |

All routes retain x,y,constant order and end with original-system evidence.
The q07 contradiction needs three meaningful decisions; the other routes use
four to six. This exception is not padded with a repeated final answer.
Local goals and operation operands are recorded in cases.json, not a runtime
format. Every arithmetic decision has two distinct concrete misconception
results: unchanged constant, reversed sign, incomplete swap, wrong coordinate
sign, missing contribution, incompatible rank or incomplete parameter family.
The actual editable corrections remain in chapter.paths.md.

First-correct positions (zero-based) are 0,1,2,1,2,0,2,0,1,0,1,2:
four in each slot. Later positions cycle deterministically from each first
position. Choice IDs retain their semantic role when displayed in a new order.

## Reading and disclosure

One ordinary lesson.v2 defines all terms and gives the three solution-set
criteria with their conditions. Its numerical example [1,-1|2], [2,1|7]
differs from all twelve cases. Hint, Answer and Solution are independently
closed, and no public block repeats that example's answer. A separate Proof
disclosure explains completeness. All twelve questions link to this lesson,
whose practice order is introductory, practice, then mixed. No figure, layout,
new template dialect, generator or four-level-support claim is introduced.

## Verification and source ownership

Only authoring/authoring.json, authoring/documents/chapter.paths.md, cases.json,
certificate_tests.py, DESIGN.md and review.md are maintained in this new
subject folder. The checker reads actual compiled --question-batch JSON via
--routes. It does not parse Markdown or trust authored answer/reached fields.
It uses shared exact matrix/TeX helpers where applicable, with independent
subject mathematics for the expanded scope. It checks source-bound original
givens/domain, every choice/key, every working state and final completeness.
Tests reject false keys, false intermediate states, semantically duplicate
options and invalid inputs/domains. Scratch Markdown prompt and wrong-feedback
edits must change the real compiled fields while leaving mathematics unchanged.

Run the exact inspector, question-batch and family-lessons commands from the
shared BRIEF; verify pinned executable hashes and ordinary exporter provenance.
Return production.json with counts, source/executable hashes, checks and limits
to the named coordinator; freeze source after handoff. No commits, publication,
store activation, screenshots, windows, fonts/ImGui or other workers' changes.
Coordinator teaching/integration acceptance and visual/learner evidence remain
separate from these headless writer checks.

## Primary references

Definitions and conditions checked on 2026-09-10 against:

- OpenStax, College Algebra 2e, [7.6 Solving Systems with Gaussian Elimination](https://openstax.org/books/college-algebra-2e/pages/7-6-solving-systems-with-gaussian-elimination).
- OpenStax, College Algebra 2e, [7.8 Solving Systems with Cramer's Rule](https://openstax.org/books/college-algebra-2e/pages/7-8-solving-systems-with-cramers-rule).
- Margalit and Rabinoff, Interactive Linear Algebra, [1.2 Row Reduction](https://textbooks.math.gatech.edu/ila/row-reduction.html) and [1.3 Parametric Form](https://textbooks.math.gatech.edu/ila/parametric-form.html).

All lesson prose, numerical problems and corrections are original Paths work;
no textbook exercise or passage is copied. These sources establish conventions,
not endorsement or measured effectiveness of this curriculum sequence.
