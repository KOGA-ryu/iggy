# Wave 03 calculus — polynomial integration delivery

Date: 2026-09-10. Stage: ready_for_coordinator_review, not publication or
coordinator acceptance. Source is frozen at the accompanying production receipt.

## Delivered family

Package prod03_calc_integration version 1, attached to existing topic_0062 /
Integration. Canonical reading prod03_calc_integration_r; twelve reserved
questions prod03_calc_integration_q01 through q12.

- Four complete all-antiderivative solves, four initial-value solves and four
  signed definite integrals.
- 54 decisions, 162 symbolic options and 108 individual wrong-choice corrections.
- Four decisions per introductory route; five per initial-value route and per
  first three definite routes; three for the genuinely short equal-limit case.
- q09, q10 and q11 select and execute the Fundamental Theorem method first.
- First correct positions, zero-based: 0,1,2,1,2,0,2,0,1,0,1,2 — four each.
- One 51,416-byte directly editable Markdown chapter with the eight required
  lesson blocks, twelve practice links and twelve return reading links.

No generator, per-question script, new document parser, renderer, runtime owner
or support mode was introduced. Existing choices.v1 and lesson.v2 remain the
maintained source/presentation route. The certificate reuses only exact sparse
polynomial arithmetic/differentiation from the frozen Wave 02 calculus helper;
its added bounded math-field adapter handles this family's rational expressions,
constant-family equivalence and requested forms. It is not a general CAS.

## Exact mathematical answers and checks

P is the zero-constant primitive; every derivative identity holds on R. The
following results are independently derived from original coefficient arrays,
not from authored keys or reached strings. The machine receipt includes those
arrays, derivative checks, endpoint values and per-term signed contributions.

| Question | Complete requested result | Original-data check |
| --- | --- | --- |
| q01 | F=2x³-2x²+3x+C, C real | F'=6x²-4x+3; equal derivatives imply constant difference on R |
| q02 | F=-x³+x²-x+C, C real | F'=-3x²+2x-1; same completeness argument |
| q03 | F=(1/8)x⁴-(3/4)x²+2x+C, C real | F'=(1/2)x³-(3/2)x+2 |
| q04 | F=(1/2)x⁵-(1/2)x⁴+(1/2)x+C, C real | F'=(5/2)x⁴-2x³+1/2 |
| q05 | F=2x²-3x+3 | P(1)=-1, C=3, F(1)=2 and F'=4x-3 |
| q06 | F=-(1/2)x³+2x+1 | P(-2)=0, C=1, F(-2)=1 and F'=-(3/2)x²+2 |
| q07 | F=x³-(1/2)x²+(1/2)x+7/4 | P(1/2)=1/4, C=7/4, F(1/2)=2 and F'=3x²-x+1/2 |
| q08 | F=-(1/2)x⁴+x³-x-3/2 | P(-1)=-1/2, C=-3/2, F(-1)=-2 and F'=-2x³+3x²-1 |
| q09 | Integral from -1 to 2 = 9 | P(2)=6, P(-1)=-3; original contributions 3-3+9=9 |
| q10 | Integral from 2 to 0 = 2 | P(0)=0, P(2)=-2; original contributions 6-4=2 |
| q11 | Integral from -1 to 1 = -7 | P(1)=-3, P(-1)=4; original contributions -8+0+1=-7 |
| q12 | Integral from -1 to -1 = 0 | Both P values are -1/2; each original-coefficient endpoint difference is zero |

All family claims preserve arbitrary C, and the connected real domain permits
the constant-difference proof. A singleton primitive is not a complete family;
adding a fixed number or rescaling the arbitrary constant by a nonzero number
does not create a different family. Initial-value uniqueness uses the same
constant-difference theorem plus the given point. Both original requirements
are checked, not merely the derivative.

All polynomials are continuous on the closed interval between their limits.
The definite routes preserve displayed-limit order, including q10's decreasing
limits. q11 explicitly distinguishes negative signed accumulation from unsigned
area; its odd linear contribution cancels while the remaining terms total -7.
q12 checks equal endpoints without pretending the integrand vanishes there:
p(-1)=-1/2. Exact coefficient sums provide a second integral calculation; no
sample points or numerical quadrature are used to infer an identity.

The distinct worked example uses F'=3x²-2, F(-1)=4 and the integral from -1 to 1.
It obtains P=x³-2x, P(-1)=1, C=3, F=x³-2x+3 and I=-2. The original derivative
and point are checked, followed by endpoint subtraction and original
contributions 2-4=-2. Its public block contains only the given problem;
Hint, Answer and Solution are separately closed. The constant-difference Proof
is another independent disclosure.

## Checks and observed outcomes

Commands run from /Users/kogaryu/iggy3d/paths:

```sh
b/sorter --inspect-documents --documents content/authoring/production/wave03/calculus/authoring/documents > build/production/wave03/calculus/inspection.json
b/paths_learning_document_tests --question-batch content/authoring/production/wave03/calculus/authoring/documents > build/production/wave03/calculus/routes.json
b/paths_learning_document_tests --family-lessons content/authoring/production/wave03/calculus/authoring/documents > build/production/wave03/calculus/lessons.json
PYTHONPATH=tools python3 -B content/authoring/production/wave03/calculus/certificate_tests.py --routes build/production/wave03/calculus/routes.json
```

Inspection accepts one document, one lesson and twelve questions with no
diagnostics. Model replay accepts twelve complete routes and 108 wrong choices,
with save replay. The lesson gate accepts one reading and twelve questions,
four closed disclosures and three independent worked disclosures, with zero
windows. Target.inspect and ordinary provenance validation also pass.

Exact certificates verify actual compiled givens/domains, every option,
every accepted key and every reached state. Mathematical truth is evaluated
independently of the requested form. The primitive/particular goals require
expanded collected polynomials; the condition goal requires the actual
substitution before solving; final subtraction and independent contribution
goals require their stated operations rather than a bare correct number.

Control counts:

| Control | Outcome |
| --- | --- |
| False accepted keys | 54 rejected |
| False reached working states | 54 rejected |
| Semantically equal options with changed spelling | 54 rejected specifically at semantic distinctness |
| Invalid inputs/domains | 64 rejected |
| Missing arbitrary C | 1 rejected |
| Shifted-same-family duplicate | 1 rejected |
| Wrong displayed-limit order | 1 rejected |
| True expressions missing the requested form/operation | 5 rejected |
| Valid alternative forms | 8 accepted |

The five goal-form controls are a factored primitive, a factored particular
function, an already-solved C in place of the initial substitution, a bare
correct integral value instead of endpoint subtraction, and that same value
instead of the requested original-coefficient contributions. Each control first
asserts mathematical truth separately from form failure. Positive controls
include reordered collected terms, unreduced but equivalent rational
coefficients, shifted/rescaled arbitrary-constant families, reversed written
term order in the correct theorem identity, equivalent endpoint fractions and
different grouping of the same contribution sum. Thus the gate does not merely
require one authored label.

Two real scratch Markdown edits also pass: one changes the first prompt, one
changes an individual wrong-feedback sentence. Existing compilation shows only
the intended corresponding field changed, with the mathematics and every other
compiled field unchanged. Temporary fixtures are confined to this subject's
evidence folder and removed normally. The certificate refuses stale route,
inspection or lesson evidence and checks source/helper/binary stability.

During author review, q10's final-subtraction distractors initially both gave
-2 despite illustrating different mistakes. Before compilation they were made
semantically distinct: one still gives -2 from reversed order; the other gives
3 from substituting an integrand value for P(2). No known mathematical or
compiler failure remains. All 54 explanations and 108 specific corrections
were read for the actual signed/fractional arithmetic; automated prose checks
alone establish presence and edit propagation, not natural-language truth.

## Hashes and canonical evidence

Final source/evidence manifests are in
`/Users/kogaryu/iggy3d/paths/build/production/wave03/calculus/production.json`.
The source manifest includes this review, DESIGN, cases, checker, metadata and
Markdown. Evidence hashes cover inspection.json, routes.json, lessons.json and
mathematics.json; the production receipt excludes itself from its own map.

Fixed reviewed helper:
`/Users/kogaryu/iggy3d/paths/content/authoring/production/wave02/calculus/certificate_tests.py`
SHA-256 `019ae49e9f8181602dbeba9f9f45b1cb73a54be4d7a0b422b4bf74b6f8f538eb`.

Release hashes match build/production/wave01/build-ready.json:

- sorter: `242a5a6fc0c8f3e04ae7c79bc1a02d5e0fe32663dd9969c9ea72f00391033776`
- paths_learning_document_tests: `ec98ec35bcf980527fd2e80d3b8e49e24b0e012bf9c7046420312d66c9bbd8d2`

Canonical Markdown SHA-256:
`7bf565e88c9fff59aea5a69910b43daad3558982363f830fb98eac40fd0df661`.
All source hashes are checked before and after the final certificate run.

## Attribution, ownership and limits

Definitions/conditions were read 2026-09-10 from the primary OpenStax Calculus
Volume 1 sections [4.10](https://openstax.org/books/calculus-volume-1/pages/4-10-antiderivatives)
and [5.3](https://openstax.org/books/calculus-volume-1/pages/5-3-the-fundamental-theorem-of-calculus).
Exact URLs and original-authoring attribution are in DESIGN and authoring
metadata. Exercises and prose are original; no external exercise was copied.

This tests the twelve explicit cases and stated bounded expression forms, not
all coefficient combinations, all integration methods, a complete chapter or
a general symbolic solver. Compiler/model acceptance is not native visual
confirmation. No window, font/ImGui initialization, screenshot, capture, image
or clipboard access occurred. No learner-outcome or four-level written-support
claim is made.

All source changes belong only to this assigned Wave 03 calculus folder;
all output/temporary checks belong to build/production/wave03/calculus. Earlier
published sources, coordinator briefs/assignments, shared tools/binaries/runtime,
active stores, personal saves and 3D assets remain unchanged. No rebuild,
publication, commit, extra task, worker message or callback occurred.

Coordinator independent mathematical/prose review, aggregate/save integration
and serial publication remain pending. The source is frozen at handoff. Return
the production receipt normally; stop after this one family.
