# Linear algebra: solve and classify complete systems

Resume building in task `linear`, 01a08b7f-444b-7703-a228-d817abcd6afd.
Your Wave 01 standby is superseded. Read ../BRIEF.md for the common depth
contract. Finish 12 complete questions and one canonical lesson; do not stop
for a pilot or user testing.

Write only in content/authoring/production/wave02/linear_algebra/ and
build/production/wave02/linear_algebra/. The coordinator owns both BRIEF.md
files and assignments.json. Read reviewed Wave 01 linear-algebra sources as
references; do not edit published sources or shared tools.

Reserve package `prod02_linear_full_systems`, version 1, lesson
`prod02_linear_full_systems_r`, questions `prod02_linear_full_systems_q01`
through `prod02_linear_full_systems_q12`. Bind to subject `linear_algebra` /
Linear Algebra, chapter `worked_matrix_practice` / Worked matrix practice.
Learner titles describe the mathematics and omit production/wave labels.

Solve two real linear equations in ordered unknowns x,y from their original
augmented matrix through elimination, classification and original-system
verification. Original integer entries have magnitude at most 12. Use exact
fractions during elimination. Eight cases have unique solutions, two have no
solution and two have infinitely many solutions with one free parameter.
At least two unique cases require an initial row swap and two have a fractional
solution coordinate. Requirements may overlap. Avoid the all-zero rank-zero
system in this bounded wave.

Carry each entire row operation through all three columns. Classify using
pivots and the augmented row: a zero coefficient row with a nonzero constant
is a contradiction; a zero row with zero constant allows a free variable when
one pivot remains. Singular does not mean no solution. For infinitely many
solutions, give an explicit parameterized pair, define its real parameter and
verify it in both original equations for every parameter value.

Use independent original determinant/Cramer checks for unique solutions and
exact coefficient-versus-augmented rank/residual checks for the other cases.
Verify each row operation, its nonzero-divisor condition and reversibility.
Check actual intermediate matrices and all distractors, not just the final
pair. Teach row/column order, augmented constant, pivot, rank, consistency and
free variable using the current numerical system. Existing MATRIX_ROLE_CHECKERS
may be reused only where their bounded contract genuinely fits; they do not
certify this entire new scope merely because it is linear algebra.
