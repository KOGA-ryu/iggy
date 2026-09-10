# Balanced equations: a complete family authoring example

This implements one bounded family from the
[problem-design research](../../../../docs/MATH_PROBLEM_DESIGN_RESEARCH.md).
It follows the reusable [teaching brief](../../../../docs/templates/QUESTION_FAMILY.md).
It does not change the four reserved subject pilots or their pinned tools.

## Learning claim and prerequisites

The learner can read the signed parts of a real linear equation, select an
operation that preserves its solution set and achieves a stated goal, explain
its inverse, locate an earlier sign error, and select an exact solution in a
fresh presentation. Required ideas: integer arithmetic, fractions as numbers,
multiplication notation, equality, ordered pairs and sequential lines.

This sequence assesses selected decisions with optional linked reading. It
does not assess independently written proofs, long-term retention, arbitrary
algebra or mastery of the chapter. The final item is uncued by intermediate
steps; the earlier items intentionally supply more help. These are the existing
six authoring roles, not six runtime support levels.

## Mathematical contract and generator

Original problems have real x and ax+b=c. A nonzero a gives exactly one solution:
subtract b from both sides and divide by a; substitution verifies existence.
If u,v both solve the equation, a(u-v)=0 implies u=v, proving uniqueness.

[recipe.json](recipe.json) supplies three finite sets. For each set, choose a
positive even a in {2,4,6,8}, positive b from 1 through 9 different from a, and
an integer seed solution from 2 through 5. The fresh solution is one of
{-3/2,-1/2,1/2,3/2}. The generator calculates c=a*s+b after applying each role's
planned coefficient/constant signs. It passes only original coefficients and
orientation to the certificate, never the intended seed solution.

The certificate independently derives (c-b)/a with exact Fraction arithmetic,
checks original substitution, and verifies each role's operation or candidate.
It rejects zero/unbounded coefficients, incorrect sign/orientation strata,
duplicate or accidentally correct options, and unsupported fresh-answer forms.
The goal question deliberately includes two equivalent equations but exactly
one that also removes the added constant. Numerical distinctness alone would
not verify that question. Symbolic option meaning is checked before formatting.

Zero coefficients, variable denominators, inequalities, quadratics and arbitrary
free-form algebra are outside this family. The reading explains zero-coefficient
boundaries; it does not generate playable boundary cases.

## Variation plan and sample answer sheet

| Role / feature | Sample original | Deliberate change and decision | Checked answer |
| --- | --- | --- | --- |
| read_notation / positive_constant | 6x+8=26 | Establish coefficient versus added constant | (a,b)=(6,8) |
| worked_check / negative_constant | 6x-8=10 | Hold coefficient and solution fixed; reverse b's sign and adjust c | 10-(-8)=18 |
| choose_next_step / negative_coefficient_and_goal | -6x+8=-10 | Against notation, hold b and solution fixed; negate a and adjust c; distinguish equivalence from the cancellation goal | -6x=-18 |
| explain_step / reversible_scaling | -6x=18 → x=-3 | Isolate the nonzero inverse rule with a negative divisor | Multiply both sides by -6 |
| repair_error / first_error | 6x-8=10 with L1: 6x=10+(-8), L2: 6x=2, L3: x=1/3 | Revisit the signed-constant case; separate first invalid operation from subsequent correct arithmetic | L1, then 18 |
| independent / reversed_fraction | 11=6x+8 | Hold positive coefficient and constant fixed; fresh exact fractional solution and reversed equality deliberately combine two previously explained features | x=1/2 |

These are purposeful comparisons across the set, not a claim that each adjacent
item changes exactly one feature. The reading demonstrates reversed equalities
and fractional solutions separately before their combined fresh application.
Its worked examples (4x-3=6, 7=4x+2) use different numbers and answers from the
final questions in all three sets. The half-integer fresh answers differ across
sets: sample 1/2, practice 3/2, fresh_check -1/2.

## Decisions and distractors

| Role | Wrong choice 1 | Wrong choice 2 | Required correction |
| --- | --- | --- | --- |
| read_notation | Negate the actual added constant | Swap coefficient and constant | Read the written sign and each term's job |
| worked_check | Leave c unchanged | Compute c+b instead of c-b | Operate on both sides; undo the signed constant |
| choose_next_step | Add b to both sides | Remove b only on the left | The first is valid but misses the goal; the second changes solutions |
| explain_step | Divide by a again | Multiply by zero | Repeated division is valid but not the requested inverse; zero loses information |
| repair_error, decision 1 | Blame L2 | Blame L3 | Those lines follow the incorrect L1 correctly; locate the first failure |
| repair_error, decision 2 | Repeat c+b | Leave c unchanged | Subtract the signed b on both complete sides |
| independent | Negate the correct candidate | Stop at ax=c-b and mistake c-b for x | Substitute the selected candidate; distinguish x from its nonunit multiple |

Feedback describes the selected expression, not an asserted diagnosis of the
learner. Every sample has seven decisions and fourteen specific corrections.
The accepted option's semantic ID stays with it; deterministic hashing of
question ID, step and choice ID sets presentation order. No runtime shuffle or
saved choice mutation is introduced. Stable question IDs include original case
bytes and a family version. New mathematics receives a new ID. Wording changes
still require the existing content-stamp/publication review.

## Reading and disclosure review

The existing lesson.v2 native textbook owns layout. The lesson defines unknown,
coefficient, signed constant, equality, solution and solution set before using
them. It explains both directions of each equivalence, the nonzero condition,
signed arithmetic and substitution into the original equation.

The two complete example solutions and their answers/hints are independently
closed. General explanations and useful-versus-valid contrasts are public.
Question domains state meanings and task conditions without naming the correct
option. After the first error-identification decision, cyan working shows only
L1, so the repair arithmetic remains a decision. The fresh question has no hint
or prescribed intermediate step. Its answer and checking explanation appear
only after a response through the existing session.

The coordinator reviewed the template's mathematical claims, the 6x sample
arithmetic, the deliberately incorrect attempt, the distinction between a
valid move and a requested goal, and answer disclosure. Automated template
checks cannot establish teaching clarity or correct native typesetting. Those
remain user checks. No learner effectiveness or retention result is claimed.

## Exact authoring and checking workflow

Work from `/Users/kogaryu/iggy3d/paths`:

```sh
PYTHONPATH=tools python3 -B content/authoring/learning/linear_family/tests.py
PYTHONPATH=tools python3 -B content/authoring/learning/linear_family/generate.py --set sample
```

The generator uses existing chapter assembly, certificate wrapping and
`check_authoring_pilot.check_assignment`. That shared gate compiles the actual
documents, checks original-given certificates against the compiled choices and
working, replays all six routes including wrong responses and saves, validates
provenance and checks source/binary stability. The generated source has the
existing sequence.json, lesson.md.in, questions.paths.md.in, certificates.py and
authoring.json shape. No new parser syntax or runtime math owner is added.

Each run prints its immutable `authoring` path. A source-hashed verification
receipt also lives under `build/linear-family-example/SET/SOURCE_HASH/checks/CHECK_HASH/`.
Checks have separate identities so a later build can verify identical source
content without replacing earlier evidence.
Candidate material shares the existing `build/parallel-authoring/algebra/`
content-addressed output location; package/question IDs distinguish it from the
separate pilot. Existing destinations are never overwritten.

Use the same command with `--set practice` or `--set fresh_check` to prepare
further sets. Set names describe intended teaching use, not access controls.
Keep the fresh-check set aside until the learner has studied and taken a break;
this is a manual review procedure, with no due-date scheduler or mastery score.

For an editable app preview, pass the printed authoring path to the existing
exporter. `NEW_FOLDER` must not already exist:

```sh
python3 -B tools/export_learning.py draft AUTHORING_PATH --output NEW_FOLDER
b/sorter --documents NEW_FOLDER/documents --watch-documents
```

Preview is session-only. It does not replace the active published library or
personal saved work. Publication and later scheduling remain separate work.

## Writer handoff and stop conditions

This is the concrete example for a future bounded subject assignment. Reuse the
teaching brief, existing role sequence schema, Markdown block/choice syntax and
shared gate. Define a subject-specific mathematical certificate; do not copy a
linear arithmetic oracle into trigonometry or calculus. List the exact approved
source files and output directory in each assignment, as the current pilots do.

For this family, editable originals are recipe.json, lesson.md.in,
questions.paths.md.in and this brief; coordinator-owned generate.py/tests.py
define and test the supported mathematics. Do not edit shared tools, reserved
pilot sources, generated candidates, runtime C++, personal progress or 3D assets.
Changing the checker domain requires coordinator review and new meaningful
counterexamples. Return the candidate path, receipt, answer/variation sheet,
rejected-case evidence and remaining manual checks; stop after one family.

Use original prose and generated problems. The associated research note provides
pedagogical sources and limitations; no external exercise text was imported.
