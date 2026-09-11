# Polynomial derivatives: a reusable teaching family

This is the `polynomial_derivative_v1` example for the common recipe command.
It reuses the reviewed Wave 01 degree-at-most-three polynomial questions and
lesson. The lesson is unchanged. The question template changes only local
placeholder names and feedback IDs to the common convention; its teaching,
symbolic choices and mathematical checks retain the reviewed source.

From the Paths root:

```sh
python3 -B tools/author_question_family.py init content/authoring/drafts/derivatives_next --family polynomial_derivative_v1 --package derivatives_next
python3 -B tools/author_question_family.py check content/authoring/drafts/derivatives_next
```

The destination must be new. Writers edit recipe.json, lesson.md.in,
questions.paths.md.in and this design brief. No Python or new runtime syntax
belongs in a writer folder. Checking returns an unpublished ordinary authoring
package and immutable evidence; the existing exporter owns publication.

## Learning claim and prerequisites

Learners identify derivative notation, complete derivative arithmetic, choose
the requested factored quotient, state the condition for cancellation, locate
the first error and evaluate a fresh derivative. Prerequisites are real-number
substitution, signed polynomial arithmetic, collecting terms and introductory
limits. The lesson defines coefficients, powers, function values, derivatives,
the fixed input a and the increment h.

Success records the requested symbolic choices with available reading and
corrections. It does not establish unaided proof, retention, transfer, mastery
or fluency with arbitrary functions. The six roles are exercise purposes,
not six support levels.

## Editable cases and numerical limits

Each of `sample`, `practice` and `fresh_check` has one named case for each of
the six roles. Each case has exactly two fields:

| Field | Meaning and permitted values |
| --- | --- |
| coefficients | Four integers `[A, B, C, D]`, each from -5 through 5, describing `A x^3 + B x^2 + C x + D` |
| at | The fixed real input a, here an integer from -3 through 3 |

Use four entries even for quadratic or linear polynomials; leading zeroes
express the lower degree. At least one of A, B or C must be nonzero. Fractions,
other functions, extra powers and changing the real/nonzero-h domain require
a different reviewed provider. These bounds are a tested subset of the
frozen checker's -9..9 coefficient contract.

For example, changing the sample worked_check point from 2 to 1 changes the
same polynomial's derivative calculation from -12+12+2=2 to -3+6+2=5.
The displayed point, working, answer, misconception values and explanations
are recalculated together. Only that question receives a new identity.

The numeric roles worked_check, repair_error and independent need two distinct
wrong computations, chosen in this reviewed order: the function value, the
derivative with the constant incorrectly retained, then the negative derivative.
Results equal to the correct derivative or a previously selected result are
skipped. If two distinct wrong results cannot be formed, the case is rejected
with its exact recipe location. A writer must change its polynomial or point;
the tool does not invent arbitrary numerical filler.

The default lesson's worked case, coefficients `[1,-2,1,4]` at 1, is reserved
and cannot be an active question. Keep the worked example separate when
editing the lesson. Each set must use a different original for the same role.
Changing only a title or a package name does not create new mathematics.

## Teaching and variation

The default 18 cases retain the reviewed progression through signed cubic,
quadratic and linear cases, including zero coefficients and negative points.
Each set contains these decisions in order:

| Role | Decision and correction |
| --- | --- |
| read_notation | Select the difference-quotient limit; distinguish it from an inverted ratio or the function output |
| worked_check | Evaluate the supplied derivative expression; corrections show the actual mistaken calculation |
| choose_next_step | Show h factored above and below the fraction bar; the already-cancelled expression is valid but misses this request |
| explain_step | State h != 0; a limit does not make the original quotient defined at h = 0 |
| repair_error | Identify L2's changed h coefficient, then calculate the corrected derivative |
| independent | Evaluate the derivative at the original input in one uncued decision |

The first repair decision identifies the line only. Its reached working and
explanation must not supply the corrected derivative before the second decision.
Every wrong choice retains its own correction when the shared runner varies
answer position. Prompts, domain statements and explanations remain in Markdown.

Keep the standard lesson block IDs start, terms, rule, condition, worked,
errors, practice and summary. The separate worked example exposes only its
given initially; Hint, Answer and Solution are independently closed.
Routine mathematical and prose review belongs to the coordinator.

## Calculated fields and template convention

Shared fields are subject, subject_title, chapter, chapter_title, reading_id
and reading_title. All question fields have a role prefix, such as
`{{worked_check_derivative_expression}}`. The runner supplies id, title,
objective, given, choices_1 and after_1, plus choices_2 and after_2 for repair_error.

The mathematical fields are a, d, f, expanded, numerator, hp, quotient,
definition, factored, derivative_expression, value_expression and function_value.
Here d is the derivative value, expanded is f(a+h), numerator is its difference
from f(a), and hp is the polynomial quotient valid for nonzero h.

For numeric roles, error_a_name/expression/value and error_b_name/expression/value
describe the two selected wrong computations. The frozen mathematical helper
owns these named calculations. Feedback 12 and 13 belongs to the two wrong
choices in each first decision; repair's second decision uses 22 and 23.
The common runner moves display positions while retaining these semantic IDs.
Do not replace calculated fields with answers that stop following number edits.

## Verification, source and handoff

The frozen mathematical implementation compares coefficient differentiation
with exact binomial expansion at a+h. Only original coefficients and the
evaluation point cross that boundary; the recipe contains no answer key.
Independent tests expand by repeated polynomial multiplication using Horner's
method, checking higher powers as well as the derivative coefficient.

The common pipeline compiles one chapter, replays all 18 questions, 21 decisions
and 42 wrong choices, compares keys and reached working with exact certificates,
checks the closed worked disclosures and replays saved progress. Number/Markdown
edits, false keys, altered working, missing evaluation points, invalid domains
and degenerate numeric cases are covered by the shared targeted test entry.

Question IDs bind package, family version, set, role and original mathematics.
Prose edits retain IDs but may change stamps. Inputs, imported mathematical
code and executable hashes are bound to each immutable receipt; changes during
checking reject the candidate. No C++ rebuild is needed for recipe/Markdown edits.

The [Wave 01 design](../../production/wave01/calculus/DESIGN.md) records its
OpenStax Calculus Volume 1 source review and original Paths authorship. No new
external exercise text was copied or new source review claimed here. Published
Wave 01–04 sources remain frozen. Their historical recipe and packaging code
is retained for reproduction and is not called by this workflow.

The default scaffold repeats reviewed mathematics and remains unpublished;
it demonstrates a reusable authoring path, not additional curriculum coverage.
Visual appearance and learner outcomes remain unobserved. These checks use
no windows, screenshots, fonts, ImGui or clipboard access.
