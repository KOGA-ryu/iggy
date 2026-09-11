# Linear algebra: constructing and using an inverse

Read ../BRIEF.md and ../assignments.json. Only write your own DESIGN.md,
cases.json, certificate_tests.py, review.md and authoring/ under this folder,
plus build/production/wave03/linear_algebra/. Leave this coordinator-owned brief
and all older/shared sources unchanged. Your existing task is linear.

Package prod03_linear_matrix_inverse version 1; lesson
prod03_linear_matrix_inverse_r; questions with suffixes _q01 through _q12.
Bind all content to topic_0092 / Matrix Algebra. Use neutral learner titles.

Build complete inverse problems for 2 by 2 matrices with exact rational
arithmetic. Choose finite integer input bounds (magnitude at most five is a
reasonable starting point) and bounded right-hand sides before writing cases.
q01-q04 introduce distinct diagonal, triangular and off-diagonal structures;
q05-q08 practise dense matrices, negative/nonunit determinants and fractions;
q09-q10 compute an inverse then solve Ax=b; q11-q12 prove a singular original
matrix has no inverse. At least two final-group problems select a method and
carry it through. Include a structure requiring a row swap. Do not reduce an
inverse construction to one formula-choice step.

Represent both the adjugate formula with its nonzero determinant condition and
Gauss-Jordan reduction of [A|I] in the family. Define matrix entry order,
identity, row-times-column multiplication, inverse, determinant, augmentation
and each used elementary row operation. Show actual intermediate matrices.
Keep every augmented operation on the entire row. The existing native TeX
fields handle the mathematics; report presentation needs instead of adding a
renderer or 3D implementation.

For every invertible case, independently verify both AB=I and BA=I against the
original A. Application routes also check the original Ax=b after finding x.
For each singular case show a nonzero vector v with Av=0, explain why an
inverse would force v=0, and connect that contradiction to the computed
determinant. Never divide by zero or present a fabricated inverse matrix.

Use exact matrix arithmetic derived from original entries, independent of
authored keys. Check every intermediate row operation, determinant, formula,
matrix product, vector and local goal. Include zero-determinant misuse,
partial-row updates, adjugate sign/order errors, equivalent fractional matrix
options and a true but goal-missing form as rejected probes. Include valid
equivalent controls. Avoid copied general parsers or redundant frameworks.

Starting primary reference: Georgia Tech Interactive Linear Algebra,
[3.5 Matrix Inverses](https://textbooks.math.gatech.edu/ila/matrix-inverses.html),
with its linked Matrix Multiplication section as needed. Read the conditions
and proofs and record access dates. Write original exercises and explanations.
