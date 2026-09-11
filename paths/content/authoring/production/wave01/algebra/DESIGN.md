# Wave 01 Algebra: family designs

## Signed linear equations: `signed_balance`

- Observable decisions: identify signed terms, calculate a balanced move, choose a move that meets a stated goal, name an inverse, repair the first sign error, and select an exact solution.
- Prerequisites: signed integers, multiplication notation, equality, inverse operations, and fractions as real numbers.
- Contract: real `ax+b=c` with nonzero bounded integer `a`; every candidate is checked by `Fraction(c-b, a)` and substitution. If `x1` and `x2` solve the original equation, `a(x1-x2)=0`; nonzero `a` proves uniqueness.
- Exclusions: zero coefficients, variable denominators, inequalities, quadratics, and free-form written proof grading.
- Variation: teaching establishes signs and structure; practice changes sign combinations and integer solutions; fresh_check includes a reversed equality or fractional solution as a conceptual variation. All remain single-variable real equations.

## Linear equations with brackets: `distributive_linear`

- Observable decisions: identify multiplier and inside constant, distribute to every term, distinguish a complete expansion from an incomplete one, reverse a balanced outside-constant move, repair a distribution error, and solve a fresh grouped equation.
- Prerequisites: the distributive property, signed multiplication, equality, and fractions as real numbers.
- Contract: real `a(x+b)=e` or `a(x+b)+d=e` with nonzero bounded integer `a`. The independent certificate expands to `ax+ab+d=e`, derives `x=(e-d)/a-b`, and substitutes into the unexpanded original. The same nonzero-multiplier subtraction argument proves uniqueness.
- Exclusions: variable denominators, nonlinear terms, zero multipliers, and any response requiring a new runtime checker.
- Variation: practice changes negative multipliers and inside signs; fresh_check combines an outside constant with fractional solutions. Each fresh set retains the same distribution/inverse invariant while changing a meaningful representation feature.

## Shared decisions, sources, and limits

Revision 2 has two final canonical packages: `prod01_algebra_signed_balance` and `prod01_algebra_distributive_linear`. Each contributes one family reading linked to all 18 questions. The six teaching/practice/fresh_check rows are retained as staging evidence for their distinct seed sets, not as final readings or packages.

Every set has the six existing roles, with a two-decision repair card. First-decision answer positions are balanced twice per position and use distinct permutations: teaching `0,1,2,0,1,2`, practice `1,2,0,1,2,0`, fresh_check `2,0,1,2,0,1`. Wrong choices are checked against original givens and receive individual corrections. The 36 construction seeds are finite and explicit in `recipe.json`; nonintegral derived right sides are rejected instead of rounded. Tests cover the entire pool, changed families, zero multipliers, and duplicate/equivalent options.

OpenStax *Elementary Algebra 2e*, [section 2.1](https://openstax.org/books/elementary-algebra-2e/pages/2-1-solve-equations-using-the-subtraction-and-addition-properties-of-equality) and [section 2.2](https://openstax.org/books/elementary-algebra-2e/pages/2-2-solve-equations-using-the-division-and-multiplication-properties-of-equality), accessed 2026-09-10, were used to verify equality/inverse-operation conditions and the nonzero division condition. Section 2.2's simplification/distributive examples also informed the bracket-family condition. All Paths exercises and prose are original; no external exercise text was copied.

Automation establishes bounded arithmetic, compiler compatibility and model replay. It does not establish native typesetting quality, learner transfer, retention, mastery, or external educator review.
