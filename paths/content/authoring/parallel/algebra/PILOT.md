# Pilot: Understand each balanced move

Status: **specified, not authored or dispatched**. Model recommendation: Terra High.
Outcome, prerequisites and original cases: [sequence.json](sequence.json).
Shared instructions: [authoring contract](../../../../docs/PARALLEL_QUESTION_AUTHORING.md).

Package: `pilot_algebra_balance`, version 1. Subject: `algebra` (Algebra).
Existing chapter: `worked_linear_practice` (Worked linear practice).
Reading ID: `pilot_alg_balance_v1_reading`.
Question IDs are `pilot_alg_balance_v1_ROLE`, using each full role name in sequence order.
Taxonomy reference: `topic_0003` (Equations and Relations); this pilot reviews one
bounded skill, not every definition or prerequisite in that topic.

## Required lesson

After this lesson the learner can interpret and justify the two balanced
operations in ax+b=c, repair a sign error and check the result in the original.
Define equality, solution, unknown, coefficient, added constant and inverse
operation. Explain subtraction of a negative and why division needs a nonzero
coefficient. Include 0x=0 versus 0x=1 as explanatory boundaries, not playable
cases in this unique-solution family.

Separate worked example: **4x+3=11**. Subtract 3 from both complete sides to
reach 4x=8, divide both sides by 4 to reach x=2, and check 4(2)+3=11.
Show each intermediate arithmetic operation. Answer and full solution are
separate closed disclosures. Do not reuse the last exercise as this example.

## Six exact decisions

Numbers and objectives are fixed in sequence.json. Render the equations as
LaTeX; the compact forms below specify mathematics, not a new input grammar.
First-step choice IDs are 11, 12, 13 in the listed order. A second step uses
21, 22, 23. Keep keys in the assignment/certificates, out of public prose.

| Role | Original given and requested decision | Three choices, in order | Accepted | Reached working |
| --- | --- | --- | --- | --- |
| read_notation | -4x+7=19. Identify (a,b,c) in ax+b=c. | (-4,7,19); (4,7,19); (-4,19,7) | 11 | (a,b,c)=(-4,7,19) |
| worked_check | 5x-6=9. After adding 6 to both sides, 5x=9+6. Fill the right side. | 9; 15; 3 | 12 | 5x=15 |
| choose_next_step | -3x+4=13. Remove the added constant, keeping coefficient -3 and equality. | Add 4 to both sides; subtract 4 only on the left; subtract 4 from both sides | 13 | -3x=9 |
| explain_step | -4x=12 becomes x=-3 after division by -4. Which inverse restores it? | Divide both sides by -4; multiply both sides by 0; multiply both sides by -4 | 13 | -4x=12 |
| repair_error | Deliberately wrong attempt below. Identify the first invalid line, then correct its right side. | L1; L2; L3, then 2; 7; 12 | 11, then 23 | L1: 7-(-5)=12; then 3x=12 |
| independent | -6x+5=14. Choose its solution. | x=3/2; x=-3/2; x=-9 | 12 | x=-3/2 |

For method/inverse options use short equation/operation labels with the full
meaning in the prompt and correction. Do not call adding 4 to both sides
invalid: it preserves equality but fails the goal. Subtracting on one side
fails equivalence. In the inverse card, the correct operation recovers the
original equation; the others fail that exact goal.

Incorrect attempt for repair: original 3x-5=7; L1: 3x=7-5;
L2: 3x=2; L3: x=2/3. L2 and L3 follow the preceding mistaken equation;
L1 first mishandles the sign. After choosing L1, ask for 7-(-5).
The explanation then solves x=4 and substitutes 3(4)-5=7.

## Certificate and failure requirements

Use Fraction for solutions and substitution. Derive each transformation from
the original coefficients. For a nonzero a, prove uniqueness by subtracting
two alleged solutions: a(x1-x2)=0 implies x1=x2. For balance and division,
supply the inverse algebraic operation; a single successful substitution does
not prove whole solution-set equivalence.

Check all numerical choice residuals. The final choices give residuals
-18, 0 and 45 respectively in the original equation. Check the repair's wrong
x=2/3 fails the original and x=4 satisfies it. Independently check the worked
example, not just the six graded cards.

Reject zero coefficients in the unique-solution checker, mistaken subtraction
of a negative, an operation on only one side, duplicate/equivalent answer
choices, an altered given, wrong key or reached equation. A malformed case
must fail before the shared command writes a candidate. Record the zero
coefficient boundary explanation without routing it through this checker.

## Human check after coordinator integration

Reading: inspect the signed-constant definition and open Hint, Answer and
Solution independently. Exercise 03: an incorrect choice retains cyan working
and explains whether it is invalid or merely misses the goal. Exercise 06:
the green fractional result stays until Next. The worker does not launch this.

## Primary references and evidence

- [Textbook reference 1](https://openstax.org/books/elementary-algebra-2e/pages/2-1-solve-equations-using-the-subtraction-and-addition-properties-of-equality)
- [Textbook reference 2](https://openstax.org/books/elementary-algebra-2e/pages/2-2-solve-equations-using-the-division-and-multiplication-properties-of-equality)

These are references for definitions and conditions; author original prose,
examples and distractor explanations. Source text checked on 2026-09-10 when
preparing the packet; the worker records the exact sections used in review.md.
Follow the shared certificate interface and provenance schema. Run:

~~~sh
PYTHONPATH=tools python3 -B content/authoring/parallel/algebra/certificate_tests.py
python3 -B tools/check_authoring_pilot.py --subject algebra
~~~

These content commands require the six worker deliverables first. Return the
candidate path and evidence; publication and manual visual acceptance are later
coordinator/user steps. Stop after this six-question pilot.
