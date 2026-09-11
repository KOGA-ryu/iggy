# Constructing and using a two by two inverse

## Learning claim and prerequisites

Construct a candidate inverse through complete reversible row operations or
through a determinant, adjugate and scalar division; verify both multiplication
orders. Use the inverse to solve a column-vector equation. Prove nonexistence
with a nonzero null vector when the determinant vanishes. Prerequisites are
signed rational arithmetic, ordered rows/columns and solving two linear
equations. Symbolic-choice success demonstrates recognition and completion of
these bounded routes, not unaided proof writing, transfer or measured mastery.

## Finite mathematical contract, fixed before case selection

Original A has exactly two rows and two columns of integers in [-5,5]. An
application right-hand side has two integer entries in [-5,5]. The field is
real; all calculations use exact rational numbers. Ten matrices are invertible;
two are singular and nonzero. No complex entries, variable coefficients,
larger matrices, approximate tolerances or rank-zero original is assigned.
Finite cases.json records original inputs, groups and explicit local operations,
not inverse answers. Intermediate rational magnitudes are bounded by 10000
in the certificate's numeric display reader. All division requires a nonzero
divisor. A zero determinant never licenses the adjugate formula.

For A=[[a,b],[c,d]], independently compute ad-bc and J=[[d,-b],[-c,a]].
For nonzero determinant derive B=J/(ad-bc); multiply original A and B in both
orders by exact row-times-column summation. On every augmented row step,
apply the specified operation across all four columns and reverse it exactly.
Also check the invariant E*A=L when current augmentation is [L|E]. The formula
certificate is independent of the authored reduction. For Ax=b, compute B*b
and substitute into original A. For singular A, derive a nonzero null vector
from the original rows, evaluate A*v exactly, and use C*A=I to derive the
contradictory v=C*(A*v)=0. No point-sampling identity arguments are used.

## Deliberate variation

- q01-q04: diagonal scaling, upper and lower triangular cancellation, and
  off-diagonal structure with a required complete row swap. Use Gauss-Jordan.
- q05-q08: dense signed matrices, positive and negative nonunit determinants,
  exact fractional inverses and adjugate sign/order. Use the formula in separate
  determinant, adjugate and evaluated-inverse decisions, then both products.
- q09-q10: choose a determinant-based construction or an existing-row pivot
  method under an explicit local goal, carry it through, then solve Ax=b and
  verify the original equation. Include a fractional solution.
- q11-q12: dependent nonzero rows and a zero row/column structure. Compute the
  determinant, select a normalized nonzero witness, multiply and finish the
  contradiction. These are proofs of nonexistence, not fabricated inverses.

The groups are four introductory, four practice and four mixed/exceptional
problems. First answer positions are 1,2,3 repeated four times. Later positions
rotate deterministically; choice IDs keep their own misconception feedback.
These groups do not alter runtime support modes. Earlier six-role packets and
all published Wave 01/02 sources remain unchanged.

## Local decisions and wrong options

Each step requests a precise operation or output. Wrong matrix options model
partial-row updates, wrong signed multiples/divisors, transpose/adjugate order,
lost determinant factors, entrywise multiplication or omitted product terms.
Explanations state the actual operands. Reversible but goal-missing operations
are described as such, not as invalid mathematics. Null-vector choices require
a nonzero vector and the stated normalization; the zero vector is insufficient
evidence even though A*0=0. Displayed matrices with equal rational entries are
semantic duplicates irrespective of spelling. An unevaluated scalar-times-
matrix expression can be true but fail an explicit entrywise-evaluation goal.

## Source and reading

One direct authoring/documents/chapter.paths.md is the maintained lesson and
question source; there is no generator. Ordinary choices.v1 and lesson.v2 flow
through the existing compiler, model and exporter. The eight local blocks are
start, terms, rule, condition, worked, errors, practice and summary. A distinct
worked example keeps Hint, Answer and Solution independently closed. Public
text defines notation and conditions without exposing that example's answer.
All twelve questions read the one lesson; its ordered links match the IDs.

Primary references read on 2026-09-10: Dan Margalit and Joseph Rabinoff,
Interactive Linear Algebra, Georgia Institute of Technology:

- https://textbooks.math.gatech.edu/ila/matrix-inverses.html (3.5: two-sided
  inverse, nonzero determinant condition, augmentation theorem, null-vector
  obstruction and unique inverse solution).
- https://textbooks.math.gatech.edu/ila/matrix-multiplication.html (3.4:
  row-column multiplication and order of factors).
- https://textbooks.math.gatech.edu/ila/row-reduction.html (1.2: whole-row
  replacement, nonzero scaling, swaps and reversibility).

All exercises, numbers and prose are original. References supply mathematical
definitions/conditions, not copied questions or claims of endorsement.

## Verification and ownership

certificate_tests.py consumes --routes from the actual headless model. It is
a finite matrix certificate and tests, not a general TeX/Markdown parser or CAS.
Reuse question_workflow.exact, build_question_batch.operate and its exact TeX
number formatter where applicable. The row helper operates on every column;
its contract fits two by four augmentation. Numeric matrix display recognition
is limited to the explicit scalar/matrix/vector and conditional method forms
in this family. It retains representation shape separately from exact values.
Controls accept equivalent rational entries and alternate matrix delimiters;
probes reject false keys/working, equivalent duplicate options, zero-divisor
misuse, partial rows, sign/order mistakes, changed domain/inputs and a true
unevaluated result that misses the requested evaluated form.

Run the existing inspection, question-batch, family-lessons, Target/provenance
checks and exact certificate. Perform a real prompt and feedback edit in an
evidence-folder Markdown copy; compile it and prove exactly those fields changed.
Verify the Release hashes in build/production/wave01/build-ready.json. No
rebuild, runtime changes, save/store writes, windows, screenshots, captures,
images, font/ImGui initialization, clipboard access, commits or publication.
Allowed source: this DESIGN.md, cases.json, certificate_tests.py, review.md and
authoring/. Evidence: build/production/wave03/linear_algebra/. The coordinator
owns briefs, assignments, independent review, integration and publication.
Stop after the complete twelve-problem family and production.json handoff;
freeze source at that point. Rendering and learner outcomes remain unobserved.
