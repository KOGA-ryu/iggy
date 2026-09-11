# Polynomial integration: teaching and mathematical contract

## Learning claim and prerequisites

The learner completes twelve original polynomial problems: all antiderivatives,
an initial-value solution, or a signed definite integral. Prerequisites are
signed rational arithmetic, powers, polynomial evaluation and differentiation.
Success establishes the requested prepared-choice decisions with available
reading/corrections. It does not establish unaided proof writing, mastery,
retention, learner outcomes or four runtime support levels.

## Mathematical contract — fixed before case selection

Inputs are nonzero real polynomials of degree zero through four, represented by
ascending exact rational coefficients. Each coefficient has absolute value at
most 6 and reduced denominator in {1,2,3,4,6}. Trailing zero coefficients are
excluded. Evaluation points and integral limits are rational, have absolute
value at most 2 and denominator 1 or 2. Initial ordinates have absolute value
at most 4 and denominator 1 or 2. All function identities have domain R;
integrals retain their original ordered endpoints, including reversed/equal
limits. Only the twelve explicit cases are delivered, not this Cartesian pool.

For p(x)=sum a_j x^j, define the zero-constant primitive P by coefficient
P_(j+1)=a_j/(j+1). All j are nonnegative integers, so j+1 is nonzero. Differentiate
the candidate coefficient by coefficient to verify P'=p as an identity, never
from samples. If F'=p on R, then (F-P)'=0; the mean value theorem makes F-P
constant on this connected interval. Thus all antiderivatives are P+C with C
arbitrary real, not just P. Initial data fix C=y0-P(x0) uniquely; check both
F'=p and F(x0)=y0. Polynomials are continuous on every closed real interval,
so the Fundamental Theorem gives I=P(b)-P(a). Signed orientation is preserved.
Independently compute I=sum a_j*(b^(j+1)-a^(j+1))/(j+1) from original inputs.

No logarithms, substitution, transcendental functions, improper integrals,
disconnected domains or general differential equations are included. The
power-rule denominator forbids exponent -1; this bounded polynomial family
never reaches it. No CAS, runtime parser, generator or renderer is assigned.

## Variation and identity plan

Reserve prod03_calc_integration version 1 in topic_0062 / Integration, reading
prod03_calc_integration_r and q01–q12. Four introductory problems find complete
antiderivative families; four practice problems add initial values, including
fractions and negative inputs; four mixed/exceptional problems compute definite
integrals. The final group includes ordinary orientation, reversed endpoints,
a negative signed result with an odd contribution cancelling, and equal limits.
At least two select and execute the theorem-based method before calculation.
Titles remain neutral. No earlier published identity or source changes.

## Decisions, forms and misconceptions

Primitive decisions explicitly request an expanded, collected polynomial with
zero constant. Mathematical identity and requested displayed form are checked
separately. Equivalent expanded term orderings and rational coefficients are
valid; a factored but true polynomial fails only the expansion goal. Actual
options do not compete as equivalent expressions. Differentiation verifies all
primitive candidates independently of the displayed answer key.

Family decisions distinguish an entire arbitrary-constant family from a single
primitive and from a family with a wrong derivative. Constant shifts and
nonzero rescalings of an arbitrary real constant describe the same family;
those cannot be separate options. Initial-value decisions retain the condition,
solve its constant, construct the particular function, then check derivative
and condition. Definite routes construct P, evaluate both endpoints in named
order and subtract upper-display value minus lower-display value. Equal limits
receive a genuinely short route, not padded method-choice duplicates.

Every decision has three symbolic choices, one goal-correct semantic ID,
reached working, ordinary-prose explanation and two specific corrections with
actual arithmetic. First positions are balanced four each; later positions
vary deterministically. Wrong feedback describes the selected expression, not
an asserted diagnosis of the learner. The old six-role framework stays frozen;
these are full choices.v1 workflows, not adaptations of that gate.

## Canonical reading and source

One directly authored authoring/documents/chapter.paths.md contains all lesson
and question wording, choices and arithmetic. One ordinary authoring.json
supplies provenance. The lesson uses lesson.v2 start, terms, rule, condition,
worked, errors, practice and summary blocks; all twelve @read links and ordered
@practice links refer to the same reading. Terms define integrand, integration
variable, primitive, derivative, arbitrary real constant, family, initial
condition, endpoints, signed integral and the Fundamental Theorem. One distinct
worked example keeps its original public and Hint, Answer, Solution separately
closed; a completeness Proof is independently closed. No new page layout or
graphics are needed.

## Verification and receipt

certificate_tests.py consumes actual compiler --question-batch JSON. Reuse the
frozen Wave 02 calculus exact sparse Poly arithmetic, differentiation and
coefficient construction read-only; record that helper's hash. Bounded math-field
normalization/AST interpretation adds rational scalar division for this finite
integration certificate, not a new document parser or universal CAS. It rejects
unsupported symbols, functions, variable denominators and unbounded expressions.
Coefficient signatures establish identity, constant-family equivalence and
semantic option distinctness. Explicit form checks operate on the same parsed
math trees, separately from their coefficient values.

Negative controls cover each key/reached state, semantically duplicated labels,
missing C, shifted-same-family duplicates, wrong endpoint order, invalid
inputs/domains and a mathematically true factored expression failing the expanded
goal. Positive controls include reordered collected terms, equivalent fractions
and constant-shifted/rescaled families. Two real scratch Markdown edits change
a prompt and one correction, then use the existing compiler/model to establish
only their intended compiled fields change and the mathematics is unchanged.

Run the specified inspection, question-batch, family-lessons and certificate
commands. Verify Release binary hashes against wave01/build-ready.json; do not
rebuild. Use export_learning.Target.inspect and provenance. Final production.json
binds DESIGN, cases, checker, review, Markdown, metadata and all actual evidence.
Compilation/replay, mathematical checks, prose review, native appearance and
learner evidence remain separate. Native appearance and learning are unobserved.

## Attribution and ownership

Original Paths exercises and prose. Definitions and conditions read 2026-09-10:
[OpenStax Calculus Volume 1, 4.10 Antiderivatives](https://openstax.org/books/calculus-volume-1/pages/4-10-antiderivatives)
(definition, complete constant family, power-rule restriction, initial values)
and [5.3 Fundamental Theorem of Calculus](https://openstax.org/books/calculus-volume-1/pages/5-3-the-fundamental-theorem-of-calculus)
(continuous integrand and antiderivative endpoint evaluation). No external
exercise wording is copied. The reference conventions do not endorse this
teaching sequence or establish learner effectiveness.

Write only this subject's allowed source files/authoring and wave03/calculus
evidence. Preserve coordinator briefs/assignments, all published content,
shared tools/binaries/runtime, personal saves, active stores and 3D work.
No rebuild, publication, callback, extra task, commit, window, screenshot,
capture, image, font/ImGui initialization or clipboard access. Freeze at one
complete family and return the receipt normally for coordinator retrieval.
