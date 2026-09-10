# Pilot: Build a derivative from a difference quotient

Status: **specified, not authored or dispatched**. Model recommendation: Terra High.
Outcome, prerequisites and original cases: [sequence.json](sequence.json).
Shared instructions: [authoring contract](../../../../docs/PARALLEL_QUESTION_AUTHORING.md).

Package: `pilot_calculus_derivative`, version 1. Subject: `calculus` (Calculus).
Existing chapter: `topic_0060` (Differentiation).
Reading ID: `pilot_calc_derivative_v1_reading`.
Question IDs are `pilot_calc_derivative_v1_ROLE`, using each full role name in sequence order.
Taxonomy reference: `topic_0060` (Differentiation); this pilot reviews one
bounded skill, not every definition or prerequisite in that topic.

## Required lesson

After this lesson the learner can interpret a derivative at a point and follow
its derivation from a difference quotient. Define f, x, a, h, function value,
increment, average slope, instantaneous slope, limit and prime notation.
Explain that f(a+h) means evaluate the whole function at a+h. These are real
dimensionless examples; do not silently assign physical units.

Separate worked example: **f(x)=x^2+x at a=1**.
f(1)=2; f(1+h)=2+3h+h^2; for h!=0 the quotient is 3+h;
as h approaches zero it tends to 3, so f'(1)=3.
Show the expansion, subtraction, factorization, cancellation condition and
limit. Hint, Answer and Solution remain independent closed disclosures.
Explain that the simplified polynomial is continuous, which permits taking
its limit by evaluation, although the original quotient is undefined at h=0.

## Six exact decisions

Coefficient arrays in sequence.json are [A,B,C] for Ax^2+Bx+C, in descending
degree order. Inputs a and h are real; h is nonzero in a difference quotient.
First choice IDs are 11, 12, 13; a second step uses 21, 22, 23.

| Role | Original given and requested decision | Three choices, in order | Accepted | Reached working |
| --- | --- | --- | --- | --- |
| read_notation | f(x)=x^2. Identify the definition of f'(3). | lim(h->0) (f(3+h)-f(3))/h; lim(h->0) h/(f(3+h)-f(3)); f(3) | 11 | f'(3)=lim(h->0) (f(3+h)-f(3))/h |
| worked_check | f(x)=x^2, a=2; the quotient simplifies to 4+h for h!=0. Choose its limit. | 0; 4; 4+h | 12 | f'(2)=4 |
| choose_next_step | f(x)=x^2, a=3; quotient ((3+h)^2-9)/h. Factor before cancelling. | (6+h)/h; 6h+h^2; h(6+h)/h | 13 | (f(3+h)-f(3))/h=6+h, h!=0 |
| explain_step | h(6+h)/h=6+h. Which condition permits cancellation? | h=0; all real h; h!=0 | 13 | h(6+h)/h=6+h, h!=0 |
| repair_error | f(x)=2x^2+x, a=1; deliberately wrong attempt below. First wrong line, then derivative. | L2; L1; L3, then 4; 3; 5 | 11, then 23 | L2: f(1+h)=3+5h+2h^2; then f'(1)=5 |
| independent | f(x)=3x^2-2x+1. Find f'(2). | 9; 10; 12 | 12 | f'(2)=10 |

Incorrect attempt: L1: f(1+h)=2(1+2h+h^2)+1+h;
L2: f(1+h)=3+4h+2h^2;
L3: f'(1)=lim(h->0) (4h+2h^2)/h=4.
L1 expands correctly. L2 first drops the additional h from the linear term.
L3 follows that bad expansion using the correct f(1)=3. After identifying
L2, ask for the derivative from the corrected quotient 5+2h, h!=0.

Define the reciprocal choice in role 01 only as a distractor; it is not the
derivative definition. In role 03 the first choice omits a common factor in
part of the numerator, and the second loses the denominator. Role 04 must
explain why a limit can exist despite the quotient being undefined at zero.
For the final card, 9 is f(2), and 12 omits the derivative of the linear term.

## Certificate and failure requirements

For Ax^2+Bx+C, expand f(a+h)-f(a) by polynomial arithmetic and compare exact
coefficients. For h!=0 division by h yields 2Aa+B+Ah. Its limit is 2Aa+B.
Use separate coefficient differentiation [2A,B] and evaluation as the
independent check; do not have both procedures call the same derivative helper.
Bound integer coefficients to absolute value <=9 and a to absolute value <=3.
Reject unsupported degree, malformed coefficient arrays and nonfinite input.

The cancellation proposition needs the condition h!=0. Explicitly prove the
original quotient at h=0 is undefined and the nearby polynomial limit exists.
The whole-polynomial identity check must reject a wrong coefficient even if
it happens to match at one chosen test input.

Test all six decisions and the separate worked example; deliberately mutate
the compiled key, reached derivative, missing linear contribution, expansion
coefficient and cancellation condition. A few finite-difference samples or
successful multiple-choice replay alone are insufficient mathematical evidence.

## Human check after coordinator integration

Reading: inspect the distinction between f(1) and f'(1), and the separate
cancellation/limit explanation. Exercise 05: L2 is the first error and the
repaired derivative is 5. Exercise 06: choosing 9 should explain function
value versus slope, retaining cyan working until the correct choice.

## Primary references and evidence

- [Textbook reference 1](https://openstax.org/books/calculus-volume-1/pages/3-1-defining-the-derivative)

These are references for definitions and conditions; author original prose,
examples and distractor explanations. Source text checked on 2026-09-10 when
preparing the packet; the worker records the exact sections used in review.md.
Follow the shared certificate interface and provenance schema. Run:

~~~sh
PYTHONPATH=tools python3 -B content/authoring/parallel/calculus/certificate_tests.py
python3 -B tools/check_authoring_pilot.py --subject calculus
~~~

These content commands require the six worker deliverables first. Return the
candidate path and evidence; publication and manual visual acceptance are later
coordinator/user steps. Stop after this six-question pilot.
