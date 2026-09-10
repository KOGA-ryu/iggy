# Pilot: Read and justify a complete row operation

Status: **specified, not authored or dispatched**. Model recommendation: Terra High.
Outcome, prerequisites and original cases: [sequence.json](sequence.json).
Shared instructions: [authoring contract](../../../../docs/PARALLEL_QUESTION_AUTHORING.md).

Package: `pilot_linear_algebra_rows`, version 1. Subject: `linear_algebra` (Linear Algebra).
Existing chapter: `worked_matrix_practice` (Worked matrix practice).
Reading ID: `pilot_la_rows_v1_reading`.
Question IDs are `pilot_la_rows_v1_ROLE`, using each full role name in sequence order.
Taxonomy reference: `topic_0094` (Linear Systems); this pilot reviews one
bounded skill, not every definition or prerequisite in that topic.

## Required lesson

After this lesson the learner can translate a row, apply a complete row
replacement, justify its inverse and verify a pair in both original equations.
Define coefficient, constant, ordered pair, system, augmented bar, row label,
replacement arrow and the prime on a new row. Explain nonzero scaling and why
a reversible operation can still fail a particular elimination goal.

Separate worked example: **[1,1|3], [2,3|8]**.
R2<-R2-2R1 gives [0,1|2]; R1<-R1-R2 gives [1,0|1].
Thus x=1,y=2. Check 1+2=3 and 2(1)+3(2)=8.
Show all three column calculations, then put Hint, Answer and Solution in
separate closed disclosures. Explain zero-row versus contradictory-row
boundaries without claiming this pilot solves singular systems.

## Six exact decisions

The fixed cases are already within MATRIX_ROLE_CHECKERS. Use that existing
table directly in certificates.py; do not duplicate it. Its returned strings
are authoritative for exact LaTeX givens/working/options. The compact notation
below is the human answer sheet. First choice IDs are 11,12,13; the repair's
second decision uses 21,22,23.

| Role | Original given and requested decision | Three choices, in order | Accepted | Reached working |
| --- | --- | --- | --- | --- |
| read_notation | [-3,2|7]. Translate to an equation in x,y. | -3x+2y=7; -3x-2y=7; -3x-7y=2 | 11 | -3x+2y=7 |
| worked_check | [1,-1|2], [2,-1|7], R2<-R2-2R1. Complete the constant. | 7; 3; 11 | 12 | [1,-1|2], [0,1|3] |
| choose_next_step | [1,2|4], [3,1|7]. Cancel x in row 2 and retain row 1. | R2<-R2+3R1; R1<-R1-3R2; R2<-R2-3R1 | 13 | [1,2|4], [0,-5|-5] |
| explain_step | [1,-2|1], [2,-3|4] becomes [1,-2|1], [0,1|2] after R2<-R2-2R1. Restore it. | R2<-R2-2R1; R2<-0R2; R2<-R2+2R1 | 13 | [1,-2|1], [2,-3|4] |
| repair_error | [1,-1|1], [2,-1|5]. First wrong line below, then correct y. | L1; L2; L3, then 5; 4; 3 | 11, then 23 | L1: 5 must become 5-2(1); then [1,-1|1], [0,1|3], y=3 |
| independent | [3,1|5], [1,-1|1]. Choose the solution pair (x,y). | (1/2,3/2); (3/2,1/2); (3,1/2) | 12 | x=3/2,y=1/2 |

Incorrect attempt for R2<-R2-2R1:
L1: R2'=[2-2(1), -1-2(-1) | 5];
L2: R2'=[0,1|5]; L3: y=5.
L1 first omits the constant subtraction; the later lines follow that wrong
row. After the repair, x=4,y=3 checks both original equations. The wrong pair
x=6,y=5 satisfies row 1 but fails row 2. Explain that checking one equation
alone does not establish a solution of a system.

## Certificate and failure requirements

Use the existing original-input certificates, with independent determinant/
Cramer's-rule solution and substitution already supplied. Explain inverse
row addition for all common solutions; matching one solution is not the whole
equivalence argument. Keep all entries and residuals exact Fractions.

Independent test expectations for full systems in roles 02–06 are:
(5,3), (2,1), (5,2), (4,3), (3/2,1/2).
Final candidate residual pairs are (-2,-2), (0,0), (9/2,3/2).
Check these using direct original-row substitution, separately from row
transformation. Verify all reading-example operations and its pair too.

Reject a changed constant, singular system, wrong destination, missing constant
operation, wrong accepted option, altered reached row, reversed pair or an
equivalent distractor. Reuse the current certificate bounds: two 3-entry rows,
bounded integer entries with absolute value <=20, nonzero determinant, and
where supplied a nonzero integer row-addition multiplier with absolute value <=6.
The shared packet supplies ordinary cases; boundary tests prove refusal.
Never claim a generic matrix or written-work checker from this prepared sequence.

## Human check after coordinator integration

Exercise 03: adding three copies of row 1 is valid but misses the goal; feedback
should say so and retain cyan working. Exercise 05: L1 is the first error and
y becomes 3. Exercise 06: the fractional green result stays until Next.

## Primary references and evidence

- [Textbook reference 1](https://openstax.org/books/college-algebra-2e/pages/7-6-solving-systems-with-gaussian-elimination)

These are references for definitions and conditions; author original prose,
examples and distractor explanations. Source text checked on 2026-09-10 when
preparing the packet; the worker records the exact sections used in review.md.
Follow the shared certificate interface and provenance schema. Run:

~~~sh
PYTHONPATH=tools python3 -B content/authoring/parallel/linear_algebra/certificate_tests.py
python3 -B tools/check_authoring_pilot.py --subject linear_algebra
~~~

These content commands require the six worker deliverables first. Return the
candidate path and evidence; publication and manual visual acceptance are later
coordinator/user steps. Stop after this six-question pilot.
