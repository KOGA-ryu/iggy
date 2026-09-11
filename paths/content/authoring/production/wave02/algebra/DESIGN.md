# Complete linear equations: teaching and mathematical design

## Claim and prerequisites

The learner selects a complete, equality-preserving route from an original real linear equation to its full solution set and verifies that conclusion against the original expressions. Prerequisites are signed integer arithmetic, exact fractions and multiplying a sum. The lesson defines term, coefficient, equality, equivalent equations, denominator and solution-set notation. Successful selections demonstrate these decisions with available reading and feedback, not independent written proof, retention, mastery or four-level support.

This is the Wave 02 full-route contract, not the historical six-role pilot. One native lesson.v2 reading links twelve choices.v1 problems. No generator, alternate parser, runtime change or publication is included.

## Bounded mathematical contract

The finite pool consists of exactly the twelve original equations in cases.json. Original scalar inputs are rational with reduced numerator magnitude at most 12 and positive denominator at most 6. Each side is a finite sum of constant multiples of affine expressions; divisors are nonzero constants only. Intermediate coefficients and solutions may exceed these input bounds. Excluded: variable denominators, nonlinear products, complex domains and arbitrary new cases. A zero final coefficient is permitted and must be classified before division.

The independent checker expands original terms to coefficient/constant pairs using Fraction arithmetic. Separately, it evaluates the actual compiled original arithmetic with Python's standard ast parser and a bounded affine evaluator, rejecting nonlinear products and variable/zero divisors. It derives every operation and final set without reading the authored key as an oracle. It matches the exact compiled givens and domain to the declared originals, evaluates each compiled symbolic option, checks the local goal and every reached state, and verifies preservation of the solution set. Unique cases are checked by evaluation of the unexpanded original terms; identities/contradictions use the original left-minus-right expression for all real x. The entire twelve-case pool is checked, not every value combination within the input bounds. The existing tools/question_workflow.py exact-number validator and TeX number formatter are imported unchanged; no helper implementation is copied. Its exact hash is included in the delivery evidence.

Following independent review F1, algebraic truth and requested representation are checked separately on the same arithmetic AST. Simplified equation sides must already contain at most one variable monomial and one constant atom: undistributed brackets, repeated like terms and explicit redundant zero terms fail the form goal even when their affine values match. A clearing step additionally permits no remaining displayed division. The existing finite goal plan plus one q12/simplify ordering constraint enforces its explicit variable-before-constant prompt; other simplified-equation prompts retain either term order. Neither accepted labels nor reached states can bypass this check. Expected coefficients and answers still come from the independent original-given derivation, not exact authored answer strings. No second parser or copied framework is added.

## Variation and route plan

| ID suffix | Group | Original equation | Purpose and planned route |
|---|---|---|---|
| q01 | introductory | 3x-5=x+7 | Collect variables, remove a negative constant, divide by a positive coefficient, check both sides. |
| q02 | introductory | -2x+3=x+9 | Negative collected coefficient and negative answer; signed division and original check. |
| q03 | introductory | 2(x-3)+1=9 | Distribution to both terms and combination with an outside constant before solving. |
| q04 | introductory | (x+2)/3=(x-1)/2 | Clear two constant denominators, distribute both numerators, collect a negative coefficient and solve. |
| q05 | practice | -3(x-2)+4=2x-5 | Negative distribution, an outside constant and variables on both sides. |
| q06 | practice | (2x-1)/3+(x+2)/2=4 | Multiply every term by the common denominator; combine two variable terms; exact noninteger answer. |
| q07 | practice | 4(x-1)+2=4x+3 | Actual expansion and cancellation produce a contradiction; prove no real input satisfies the original. |
| q08 | practice | 2(x+3)-4=2x+2 | Expansion and cancellation produce an identity; distinguish all reals from the singleton zero. |
| q09 | mixed | (x-1)/2-(x+2)/3=1 | Select the least positive whole-side denominator-clearing multiplier, carry a negative group through, and check. |
| q10 | mixed | 2-(3x+1)/2=x/3+3 | Select a common-denominator method, scale the outside constants too, distribute a negative multiplier, solve a negative fractional answer. |
| q11 | mixed | (x+4)/2=x/2+3 | Fractions hide a contradiction; prove original sides differ for every real input. |
| q12 | mixed | -2(x-3)+x=6-x | Negative distribution and like terms hide an identity; prove completeness without division by zero. |

There are eight unique sets (q01-q06,q09,q10), two empty sets (q07,q11) and two all-real sets (q08,q12). Unique fractional answers occur in q06 and q10; negative answers in q02 and q10. Distribution and denominator clearing each occur in several cases. q09/q10 require method selection before executing it: the precise goal is the least positive integer clearing all current denominators. Multipliers clearing only one denominator fail that goal, even though multiplication by them is reversible. These are finite curricular groups, not support levels or an implemented spaced-review schedule.

## Choices, explanations and disclosure

Every decision has three compact symbolic choices, specific correction for each wrong option, reached work and ordinary-prose reasoning with actual operands. Wrong options cover one-sided changes, sign loss, partial distribution, failure to scale a whole side, reciprocal division and mistaking cancellation for x=0. The final check decisions use L(t) and R(t), explicitly defined as evaluation of the original left/right expressions. Empty/all-real checks use L(x)-R(x) for arbitrary real x, not a sample as a completeness proof.

First-decision correct positions are [1,2,3,2,3,1,3,1,2,1,3,2], one-based, four in each position. Later positions vary deterministically. Option identities retain their feedback when presentation changes. A mathematically valid route that misses the specified local goal is identified as such. The checker tests duplicate symbolic values, false keys, false reached work and invalid inputs.

The canonical reading has the required start/terms/rule/condition/worked/errors/practice/summary blocks. Its worked equation is -2(x+1)+3=x/2-4, distinct from all twelve questions. The original given is public, while numerical Answer and complete Solution are independently closed. General identity/contradiction conditions do not expose a question-specific result. Titles/domains do not classify the current outcome. No image, figure, window or rendering acceptance is claimed.

## Primary references and provenance

Accessed 2026-09-10. These sections were browsed for definitions and conditions; all equations, choices and explanations in this batch are newly authored, not extracted exercises.

- OpenStax, Elementary Algebra 2e, section 2.3, “Solve Equations with Variables and Constants on Both Sides”: https://openstax.org/books/elementary-algebra-2e/pages/2-3-solve-equations-with-variables-and-constants-on-both-sides . Supports balancing complete sides and checking original equality.
- OpenStax, Elementary Algebra 2e, section 2.4, “Use a General Strategy to Solve Linear Equations,” especially identity/contradiction definitions: https://openstax.org/books/elementary-algebra-2e/pages/2-4-use-a-general-strategy-to-solve-linear-equations . Supports classification after cancellation.
- OpenStax, Elementary Algebra 2e, section 2.5, “Solve Equations with Fractions or Decimals”: https://openstax.org/books/elementary-algebra-2e/pages/2-5-solve-equations-with-fractions-or-decimals . Supports multiplying every term on both sides by a common nonzero denominator.

## Verification, ownership and handoff

Ordinary source lives only in authoring/authoring.json and authoring/documents/chapter.paths.md. cases.json is finite mathematical audit input; certificate_tests.py reads the existing model's --question-batch JSON, not Markdown. Inspection and provenance reuse export_learning.Target.inspect and export_learning.provenance. A real prompt edit and a real wrong-feedback edit are compiled from an evidence-only scratch copy; neither changes the mathematical projections. Negative certificate regressions deliberately corrupt key, working, duplicate choice and domain/case.

The strengthened suite retains twelve original negative tests and adds twelve form rejections. The three reviewer examples are tested in accepted-label-only, reached-only and combined mutations; three further mutations leave like terms uncombined or a denominator displayed after clearing. Every form probe first proves that affine side values are unchanged. Four positive controls retain valid freedom: constant-first q03 and q09 forms where no order is prescribed, parenthesized/explicit-coefficient q12 notation that still meets its order, and plain exact-fraction spelling for q06. The real Markdown edit proof is recompiled from the corrected lesson and feedback baseline.

Headless evidence and executable/source hashes are recorded in build/production/wave02/algebra/production.json and review.md. The coordinator owns independent review, immutable capture, aggregate/save checks and publication. No shared tools, binaries, earlier sources, stores, saves or 3D assets are writable here. After the single authorized ready handoff, source freezes pending an assigned correction. Native visual appearance and learner effectiveness remain unobserved.
