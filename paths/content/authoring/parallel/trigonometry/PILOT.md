# Pilot: Find every sine solution in one turn

Status: **specified, not authored or dispatched**. Model recommendation: Terra High.
Outcome, prerequisites and original cases: [sequence.json](sequence.json).
Shared instructions: [authoring contract](../../../../docs/PARALLEL_QUESTION_AUTHORING.md).

Package: `pilot_trigonometry_sine`, version 1. Subject: `trigonometry` (Trigonometry).
Existing chapter: `topic_0032` (Unit Circle Framework).
Reading ID: `pilot_trig_sine_v1_reading`.
Question IDs are `pilot_trig_sine_v1_ROLE`, using each full role name in sequence order.
Taxonomy reference: `topic_0032` (Unit Circle Framework); this pilot reviews one
bounded skill, not every definition or prerequisite in that topic.

## Required lesson

After this lesson the learner can read a radian interval, isolate a sine value
and list every solution in one turn. Define radians, pi, theta, sine as unit-
circle height, a solution set, an integer parameter and an excluded endpoint.
Explain signs above/below the horizontal axis and equal heights at reflected
angles. Do not assume inverse-function notation has already been learned.

Separate worked example: **sin(theta)=1, 0<=theta<2pi**. The only permitted
angle is pi/2: height 1 is the top of the unit circle. Periodic copies
pi/2+2k*pi give only k=0 in this interval. Explain why a second reflected
angle coincides here and must not be counted twice. Keep Hint, Answer and
Solution separately closed. No figure is required for this textual argument.

## Six exact decisions

All angles are in radians. Use real theta in [0,2pi) wherever an interval is
specified. First choice IDs are 11, 12, 13; second-choice IDs are 21, 22, 23.

| Role | Original given and requested decision | Three choices, in order | Accepted | Reached working |
| --- | --- | --- | --- | --- |
| read_notation | theta in [0,2pi). Which inequality means the same thing? | 0<=theta<2pi; 0<theta<=2pi; 0<theta<2pi | 11 | 0<=theta<2pi |
| worked_check | sin(theta)=1/2. One angle is pi/6. Supply the other permitted angle. | pi/6; 5pi/6; 7pi/6 | 12 | theta in {pi/6,5pi/6} |
| choose_next_step | 2sin(theta)-1=0. Isolate sine without changing solutions. | sin(theta)=1; sin(theta)=-1/2; sin(theta)=1/2 | 13 | sin(theta)=1/2 |
| explain_step | alpha=pi/6, sin(alpha)=1/2. Which reflected angle has the same sine? | alpha+pi; 2pi-alpha; pi-alpha | 13 | sin(pi-alpha)=sin(alpha)=1/2 |
| repair_error | Deliberately wrong attempt below for sin(theta)=0. First wrong line, then corrected set. | L2; L1; L3, then {pi}; {0,pi,2pi}; {0,pi} | 11, then 23 | k in {0,1}; then theta in {0,pi} |
| independent | 2sin(theta)+1=0, 0<=theta<2pi. Choose all solutions. | {pi/6,5pi/6}; {7pi/6,11pi/6}; {7pi/6} | 12 | theta in {7pi/6,11pi/6} |

Incorrect attempt: L1: theta=k*pi, k an integer; L2: k in {0,1,2};
L3: theta in {0,pi,2pi}. L1 is correct for all real sine zeros.
L2 first includes the excluded endpoint k=2. L3 faithfully substitutes that
mistaken list. The second decision must include zero and omit 2pi.

In role 04 both distractors have sine -1/2 for the specified alpha. Explain
equal height under reflection across the vertical axis; saying only that the
angles are related is insufficient. The last prompt states the interval and
asks for all solutions without suggesting a quadrant or isolation step.

## Certificate and failure requirements

Represent angles by exact Fraction values for theta/pi. Derive the required
sine level by rational algebra. Use proved special-angle values plus the two
periodic solution branches; enumerate their integer shifts within the interval
and compare whole sets. Record the analytic completeness argument: the unit
circle intersects each of these horizontal levels at the stated points, and
periodicity lists their other turns. A finite angle table without this
argument proves candidate membership only, not absence of further solutions.

For sine 1/2 the branches are 1/6+2k and 5/6+2k in units of pi; for -1/2,
7/6+2k and 11/6+2k; for zero, k; for one, 1/2+2k.
Verify membership by exact symmetry/known values independently of the branch
enumerator. Never use floating-point sin(theta)==value as the only check.

Reject a missing valid solution, inclusion of 2pi, loss of zero, mixed degree/
radian assumptions, a wrong reflected sign, duplicate angles modulo the
specified interval or an unsupported sine level. Do not generalize these
special cases to arbitrary inverse trigonometry. The final correct pair gives
2(-1/2)+1=0 for both angles, and the completeness proof excludes others.
Independently check the reading's single-solution boundary as well.

## Human check after coordinator integration

Reading: inspect the definitions of the interval brackets and radians.
Exercise 05: the first error is L2; the repaired set keeps zero and drops
2pi. Exercise 06: selecting a set missing one solution gives a specific
correction and retains cyan working; green completion stays until Next.

## Primary references and evidence

- [Textbook reference 1](https://openstax.org/books/precalculus-2e/pages/7-5-solving-trigonometric-equations)

These are references for definitions and conditions; author original prose,
examples and distractor explanations. Source text checked on 2026-09-10 when
preparing the packet; the worker records the exact sections used in review.md.
Follow the shared certificate interface and provenance schema. Run:

~~~sh
PYTHONPATH=tools python3 -B content/authoring/parallel/trigonometry/certificate_tests.py
python3 -B tools/check_authoring_pilot.py --subject trigonometry
~~~

These content commands require the six worker deliverables first. Return the
candidate path and evidence; publication and manual visual acceptance are later
coordinator/user steps. Stop after this six-question pilot.
