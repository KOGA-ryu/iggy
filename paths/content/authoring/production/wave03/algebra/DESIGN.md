# Complete real quadratic equations

## Learning claim and prerequisites

Select a complete route from an original quadratic equation to all its real roots, then check both original sides at every root. Prerequisites are signed rational arithmetic, balanced linear operations, distribution and squaring. The lesson defines standard form, coefficients, factors, the zero-product property, discriminant, principal square root, both signs, repeated roots and solution sets. Prepared-choice success is not independent written proof, retention or mastery. The native lesson and symbolic controls are unchanged.

## Finite mathematical contract

Exactly twelve original equations form the delivered pool. Original expanded coefficients have reduced numerator magnitude at most 12 and denominator at most 6; all denominators are fixed nonzero constants. The difference of the original sides has degree exactly two. Standardization multiplies by the least common multiple of coefficient denominators, and by -1 when necessary for a positive leading coefficient; it does not divide by an unknown or discard a zero branch. Resulting integer coefficients have magnitude at most 12 and discriminants magnitude at most 100. Nonsquare radicals in the delivered roots have squarefree cores 2, 3 or 5. Displayed radical inputs in this finite checker are nonnegative integers at most 100. No complex-root, variable-denominator, degree-above-two or general CAS claim is made.

cases.json records the finite originals, independently hand-expanded coefficient vectors, groups and local operation plan, not answers or accepted labels. The checker evaluates actual compiled originals with exact polynomial convolution and compares their coefficients to that table. Roots are derived from the resulting quadratic and discriminant, not from authored answer keys. Rational square roots are reduced exactly; bounded radical values are stored by squarefree component with rational coefficients. Original expressions are evaluated at every derived root with exact arithmetic, and factor expansions/degree-two identities establish completeness rather than point sampling.

Requested form is checked separately from truth on the same bounded arithmetic AST: standard form has combined terms in descending degree and zero on the right; factored form is a product of two linear factors or a repeated linear square; formula substitution retains its actual numerator, discriminant and denominator; zero-product/square-root branches retain the full solution set. Root sets are compared without order and with equivalent radical spellings normalized. Equivalent spellings or reordered sets cannot become competing options. Valid alternative factor order, radical spelling and harmless notation are control cases. There is no Markdown parser, generator, general-purpose solver or new runtime owner.

## Variation plan

| Case | Group | Original equation | Skill and route |
|---|---|---|---|
| q01 | introductory | x^2=5x-6 | Rearrange, factor, retain both zero-product roots, check both original sides. |
| q02 | introductory | x^2+4=4x | Repeated linear factor, a singleton solution set, check the root once. |
| q03 | introductory | 2x^2+x=3 | Nonmonic factorization, signed fractional root and integer root. |
| q04 | introductory | 3x^2=6x | Common variable factor; preserve zero rather than divide by x. |
| q05 | practice | -x^2+2x+8=0 | Normalize a negative leading coefficient by a nonzero constant, then factor. |
| q06 | practice | x^2/2-x/3=1/6 | Clear every constant denominator, factor, check original fractions. |
| q07 | practice | x^2-4x=1 | Discriminant 20, formula substitution, exact roots with squarefree core 5. |
| q08 | practice | x^2+2x=1 | Signed numerator, discriminant 8, roots with squarefree core 2. |
| q09 | mixed | 2x^2+5x=0 | Select a common-factor transformation that retains zero; split both branches. |
| q10 | mixed | x^2-2x=2 | Select a single-square rewrite, use both square-root branches, core 3. |
| q11 | mixed | x^2+2x+3=0 | Coefficients, negative discriminant, empty real set, original positive-square witness. |
| q12 | mixed | (x-2)(x+1)=4 | Expand before zero-product reasoning; the nonzero original RHS prevents setting original factors to zero. |

Each route has four or five decisions (50 total), with three options each. q09/q10 select a method-specific first rewrite before carrying it through; one distractor in q10 is a valid factor rewrite that misses the explicit single-square goal. Every route ends in a solution set and original-side evidence. V denotes the list of left/right values at each root in increasing order. For q11, an exact positive expression for L(x)-R(x) proves that equality is impossible for every real x.

## Choices, teaching and disclosure

First correct positions are [1,2,3,2,3,1,3,1,2,1,3,2], four of each position; later positions rotate without moving feedback away from IDs. Distractors represent sign errors, omitted cross terms, incorrect denominator clearing, dividing away a zero root, retaining only one square-root sign, confusing a repeated root with two distinct values, using an absolute discriminant, or checking the standardized equation instead of the original sides. Correct conclusions and every intermediate option are checked independently.

The canonical lesson uses start/terms/rule/condition/worked/errors/practice/summary and twelve ordered practice links. Its separate worked original is x^2+6x=7, distinct from all questions; actual roots and calculations stay within independently closed Answer/Solution disclosures. The lesson presents factoring and completing-square explanations with concrete arithmetic and the formula's nonzero-coefficient condition. Titles are neutral quadratic problem labels, not method or outcome cues. Learner content contains no production or assessment-status commentary.

## Sources and attribution

Primary references accessed 2026-09-10 (local date). All problems, explanations and choices are original project authoring; no external exercise text is copied.

- OpenStax Intermediate Algebra 2e, 6.5 Polynomial Equations, zero-product property and standard form: https://openstax.org/books/intermediate-algebra-2e/pages/6-5-polynomial-equations
- OpenStax Intermediate Algebra 2e, 9.2 Solve Quadratic Equations by Completing the Square: https://openstax.org/books/intermediate-algebra-2e/pages/9-2-solve-quadratic-equations-by-completing-the-square
- OpenStax Intermediate Algebra 2e, 9.3 Solve Quadratic Equations Using the Quadratic Formula, formula and discriminant cases: https://openstax.org/books/intermediate-algebra-2e/pages/9-3-solve-quadratic-equations-using-the-quadratic-formula

The current native-format guidance and published Wave 02 chapter are read-only references. Existing exact-number/TeX-number helpers may be imported unchanged where their contract fits, with hashes recorded. Subject arithmetic here is bounded polynomial/radical mathematics, not a copied framework.

## Verification and handoff

Run ordinary document inspection, --question-batch, --family-lessons, exact --routes certificates and Target/provenance coverage. Regressions must reject false keys/states, equivalent duplicate root sets, zero leading coefficients, invalid domains/denominators, loss of a zero or negative-sign root, and true but goal-missing forms. Positive controls prove permissible equivalent forms remain valid. A real prompt edit and wrong-feedback edit in evidence scratch must compile to exactly those fields without a mathematical change.

Write evidence and exact source/helper/executable hashes only under build/production/wave03/algebra; verify Release hashes against the recorded readiness file. No rebuild, windows, screenshots, images, fonts/ImGui, clipboard, store activation, personal-save changes or publication. The coordinator handles independent review and serial integration. Return the normal final receipt and freeze this one family, without callbacks. Native appearance and learner effectiveness remain unobserved.
