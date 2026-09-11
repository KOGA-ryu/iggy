# Wave 01 Linear Algebra family designs

## row_operations

- Observable decision: read augmented rows, complete a whole-row replacement, choose and reverse a useful replacement, repair the first omitted operation, and check a fresh ordered pair.
- Domain: exact rational two-equation systems in real `x,y`, with bounded integer augmented entries. Each complete system has nonzero determinant. Singular, inconsistent, free-variable, larger, and written-work systems are excluded.
- Contract: use the existing `MATRIX_ROLE_CHECKERS`, which independently applies exact `Fraction` arithmetic, solves the original rows by Cramer’s rule, checks substitution in both rows, verifies row-addition inverse, elimination goal, and distractor residuals. The generator adds only deterministic presentation order.
- Variations: teaching uses a signed translation and a direct cancellation; practice changes signs/row coefficients; fresh_check uses a nonunit x coefficient and the fractional elimination multiple -3/2, then retains a fresh pair test. Invariant: x, y, constant order and complete reversible operations.
- Worked reading examples are distinct from their linked question cases. Hint, Answer, and Solution stay independently closed.

The separate row lesson uses [1,2|6], [3,-1|5], with solution
(16/7,13/7). The pool retains the distinct fresh problem [1,2|5], [3,-1|4].
All six family/set answer-position patterns are balanced and distinct.

## determinants_2x2

- Observable decision: identify positions in a two by two matrix, compute `ad-bc`, distinguish zero/nonzero determinant, identify row-swap sign change, repair a mistaken plus sign, and classify a fresh matrix.
- Domain: bounded integer `2x2` real matrices. Nonsingular matrices are checked by exact inverse multiplication using `(1/(ad-bc))[d,-b;-c,a]`; singular matrices are checked by a nonzero null vector: `(b,-a)` when the first row is nonzero, `(d,-c)` when only the second row is nonzero, and `(1,0)` for the all-zero matrix. No arbitrary-size determinant, runtime solver, or claim about a particular augmented system follows from singularity alone.
- Variations: every set contains both singular and nonsingular examples; practice introduces signed entries; fresh_check introduces a zero entry and deliberate dependent rows. Invariant: determinant formula and the zero/nonzero invertibility condition.
- Wrong options: reversed diagonal subtraction, addition of diagonal products, an incorrect numerical determinant/classification pair, and a repeated instead of sign-changed determinant. They are rejected from the original entries, not an answer seed.

Numeric determinant roles are a restricted teaching pool, not all bounded
matrices: they require two distinct incorrect values computed from the listed
misconceptions. Read, criterion and independent roles try ad+bc, bc-ad, then ad;
worked and repair roles try ad+bc and ad. Candidates equal to the answer or to
an earlier candidate are excluded. If fewer than two remain, the role refuses
the case; there is no arbitrary answer+1 fallback. Thus all-zero and
zero-first-row matrices have valid null-vector evidence but cannot populate
these numeric-choice roles. The row-swap contrast separately excludes singular
matrices because its unchanged and negated options would be equivalent.

## Editable Markdown contract

The twelve existing role .md.in files own the question prompts, post-answer
explanations and both semantic wrong-option feedback messages. Python supplies
only checked arithmetic, mathematical expressions, semantic option selection
and IDs; the shared choices_text() and fill_template() own serialization and
template substitution. The displaced Python feedback/prose dispatch routes
have been removed. Role templates are shared across the three sets.

Whole-row worked and method explanations display all three destination-entry
calculations using the existing exact row owner. Inverse explanations show
all three restored entries and the complete original matrix. The determinant
row-swap explanation calculates both displayed matrices before comparing signs.
Reading introductions state learning objectives; question-count and unmeasured
mastery qualifications belong in this review documentation, not learner prose.

The source regression edits a prompt and one wrong-feedback message in scratch
copies of all twelve Markdown templates, renders and compiles both full
families, and proves both corresponding fields change in all 36 compiled
questions while every other compiled field stays identical. Live sources are
byte-checked unchanged. Additional compiled-field assertions cover all three
row calculations and both determinant calculations.

## Sources, limits, and review

Primary references checked on 2026-09-10: OpenStax, [College Algebra 2e §7.6](https://openstax.org/books/college-algebra-2e/pages/7-6-solving-systems-with-gaussian-elimination) for augmented matrices and elementary row operations; [College Algebra 2e §7.7](https://openstax.org/books/college-algebra-2e/pages/7-7-solving-systems-with-inverses) and [Intermediate Algebra 2e §4.6](https://openstax.org/books/intermediate-algebra-2e/pages/4-6-solve-systems-of-equations-using-determinants) for `ad-bc`, inverse conditions, and determinants. All examples, questions, and prose are original Paths material; no external exercise text was copied.

The declared finite pool is all 36 cards in `recipe.json`; `certificate_tests.py` checks every role case, inverse and null-vector evidence paths including the all-zero boundary, bounded-domain refusal, and compiled false-key and false-reached-working mutations. The shared candidate gate then compiles each actual source, replays its six routes, checks every wrong choice and retained working, validates provenance, and writes immutable outputs. Automation cannot establish native typesetting, learner retention, transfer, or independently written proofs. Coordinator prose review and product-level visual observation remain separate.

Closeout checks additionally replay a copied provider for all 36 captured
questions with recipe and filesystem reads disabled, check all three augmented
columns and inverse row replacements, and multiply every nonsingular matrix
and its inverse in both orders. Singular row-swap option collisions are
refused. Each repair identifies the first error without giving the second
decision's numerical answer. Package hashes use SHA-256 of the canonical JSON
file inventory (relative path, byte count and per-file SHA-256).
