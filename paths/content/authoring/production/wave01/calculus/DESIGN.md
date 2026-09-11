# Wave 01 calculus family design

## Learning claim and boundary

Both families ask a learner to select one justified symbolic decision at a time: read derivative notation, complete arithmetic, choose a goal-sensitive move, state a condition, repair the first error, then evaluate a fresh case. Prerequisites are real-number substitution, collecting terms and introductory limits. Success records checked multiple-choice decisions with available reading and feedback; it does not establish unaided proof, retention, transfer or mastery.

`difference_quotient` uses real `Ax^2+Bx+C`, bounded integer coefficients and integer `a`. It derives the h coefficient by expanding `f(a+h)-f(a)` and uses separate coefficient differentiation as a check. `polynomial_rules` uses real degree-at-most-three polynomials. It recovers the h coefficient by binomial expansion at `a+h` and separately applies sum, constant and power rules. In both, h is nonzero for cancellation and tends to zero only in the limit. Constant/linear fresh cases deliberately test that the formula remains meaningful when a higher-degree coefficient is zero.

The finite `recipe.json` pool contains all 36 delivered cases and is tested in full. Coefficients are integers in -9..9 and points in -3..3; every target and distractor is checked for distinctness. Error repair has two decisions. False keys, working changes, equivalent options, malformed degree and h=0 cancellation are rejection tests.

## Variation and reading plan

Each family has teaching, practice and fresh_check sets. The invariant is the derivative rule; signs, zero/nonzero lower-degree coefficients, negative points and degree are varied. Fresh difference-quotient cases include linear functions; fresh polynomial-rule cases include cubic, quadratic and linear forms. These are conceptual variations beyond number substitution, not claims of measured transfer. First-decision answer positions are balanced twice per position in every set, with six different deterministic permutations across the six sets.

Every generated `lesson.v2` has start, terms, rule, condition, worked, errors, practice and summary blocks. Its worked polynomial is separate from its six questions and keeps Hint, Answer and Solution independently closed. Each active question supplies original givens, domain, two specific corrections, reached working and an explanation with arithmetic plus an independent check.

## Source check, scope and handoff

Definitions and conditions were checked on 2026-09-10 against OpenStax, *Calculus Volume 1*, [3.1 Defining the Derivative](https://openstax.org/books/calculus-volume-1/pages/3-1-defining-the-derivative) (difference quotient, derivative definition and h-based cancellation examples) and [3.3 Differentiation Rules](https://openstax.org/books/calculus-volume-1/pages/3-3-differentiation-rules) (constant, sum and power rules). All prose, examples and exercises are original Paths material; no external exercise wording is copied. No runtime calculus parser, free-form grading, transcendental derivative or visual figure is in scope. Writer-owned files are this design, recipe, generator, certificate tests, review and generated build evidence. The coordinator owns integration, publication and aggregate QA.
