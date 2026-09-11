# Complete derivative-to-tangent calculations

## Learning claim and prerequisites

Learners select a rule, retain its factors, evaluate the original function and derivative separately, and construct a tangent with the correct point and slope. Prerequisites: signed integer arithmetic, polynomial expansion, positive integer powers, function notation, derivative meaning and the slope of a line. A successful route establishes these selected decisions with linked reading and correction feedback; it does not establish unaided written proof, mastery, retention or measured transfer. These are complete choices.v1 routes, not the old six roles or four runtime support levels.

## Mathematical contract

Real polynomials only. Affine coefficients and quadratic factor coefficients are integers of magnitude at most 5 with nonzero leading coefficients; powers are 2, 3 or 4; expanded degree is at most 5; x0 belongs to {-2,-1,0,1,2}. All factors are differentiable on all real inputs. No divisions by a variable, roots, transcendental functions, inverse functions or implicit equations occur. The finite tested pool is precisely the twelve cases in cases.json, not every combination in the bound.

Every route identifies a differentiation identity, computes a derivative function including each chain/product factor, simplifies that function, evaluates f(x0) and f'(x0), then chooses a tangent. Product/mixed routes separately compute the two needed factor derivatives. The final explanation checks both L(x0)=f(x0) and L'=f'(x0). There is no padded decision repeating those already-known numbers: incidence and slope are actually recomputed from the chosen line in the final explanation. Chain routes have four decisions; product/mixed routes have five. Titles and domains do not announce horizontal tangents or numerical answers.

## Deliberate finite variation

| Question | Group | Given and point | Skills and variation |
| --- | --- | --- | --- |
| q01 | introductory | (2x+1)^2, x0=1 | Square, nonunit inner derivative, point versus slope |
| q02 | introductory | (x^2+1)(x+2), x0=1 | Quadratic times linear, two product contributions |
| q03 | introductory | (-x+2)^3, x0=0 | Negative inner derivative, odd power |
| q04 | introductory | (2x^2-x+1)(x-1), x0=2 | Signed quadratic, evaluation away from zero |
| q05 | practice | (3x-1)^3, x0=1 | Negative offset, chain multiplier 3 |
| q06 | practice | (x^2-2)(-2x+1), x0=-1 | Negative linear-factor derivative and input |
| q07 | practice | (2x+2)^4, x0=-1 | Nonzero derivative function with a zero value; horizontal tangent |
| q08 | practice | (-x^2+x+2)(3x-1), x0=0 | Negative leading coefficient, nonunit derivative of the second factor |
| q09 | mixed | (x+1)^2(2x-1), x0=1 | Choose product plus chain; two unequal contributions |
| q10 | mixed | (2x-1)^3(x+1), x0=0 | Both rules, nonunit inner derivative and signed value |
| q11 | mixed | (-x+1)^2(x+2), x0=-1 | Both rules; cancellation yields horizontal tangent at nonzero height |
| q12 | mixed | (x-1)^4(-x+2), x0=0 | Both rules; degree five and negative second-factor derivative |

All four mixed problems first select the combined method before calculating. There are eight chain-rule cases, eight product-rule cases and four requiring both. The product-only factors are degree two and one. q07's zero evaluation does not collapse derivative-function choices; its value/slope pair distractors and line distractors remain different. q11 supplies a second horizontal tangent whose function value is nonzero. No two original inputs are the same.

First correct positions are 1,2,3,1,2,3,1,2,3,1,2,3. Later positions deliberately vary; semantic option IDs remain fixed. Incorrect choices target missing inner multipliers, differentiating both product factors simultaneously, omitting one product contribution, function/slope confusion, sign errors and point-slope construction errors. They are possible misconceptions, not learner diagnoses. Feedback distinguishes valid intermediate mathematics from a missed goal where relevant.

## Reading, notation and disclosure

One canonical lesson defines f(x), f'(x), f(x0), f'(x0), inner/outer function, factor, product, chain rule, line slope and tangent. It covers all required rule conditions, polynomial simplification, evaluation and two-part verification. The independent worked example is (x+2)^2(x-1) at x0=0, unlike all twelve inputs. Its full point, derivative and line are under independently closed Answer and Solution; the public worked block contains only the original problem. Proof is separate. All twelve @read references and @practice links use prod02_calc_derivative_tangent_r.

## Independent mathematical checks and teaching review

The checker reads the actual --question-batch JSON. Original coefficient arrays in cases.json are multiplied by exact convolution, then differentiated coefficient by coefficient. This is independent of the displayed factored chain/product calculations. A bounded polynomial-expression evaluator compares every compiled expression by exact coefficients, not sample points or its key. Formal rule options are compared in independent variables u, v, u', v' with the case's exponent; point and slope values are exact integers. Every key, wrong option, after-state, original given/domain and final tangent incidence/slope must agree. A changed key, changed intermediate polynomial, mathematically equivalent duplicate option and invalid domain/case must reject. Real prompt and wrong-feedback edits in evidence-only scratch Markdown must reach the corresponding compiled field without changing mathematical fields.

The document remains directly editable native Markdown. No maintained generator, template dialect or document parser is introduced. The subject checker's math-expression normalization is solely for polynomial equality on compiler-produced fields. No compiler/model, renderer, solver or six-role gate is copied or changed. Prose review checks readable arithmetic, specific feedback and no earlier disclosure of future answers. Headless checks do not establish visual appearance or learner outcomes.

## Sources and attribution

Definitions and conditions checked 2026-09-10 against primary OpenStax Calculus Volume 1 sections [3.1 Defining the Derivative](https://openstax.org/books/calculus-volume-1/pages/3-1-defining-the-derivative), [3.3 Differentiation Rules](https://openstax.org/books/calculus-volume-1/pages/3-3-differentiation-rules), and [3.6 The Chain Rule](https://openstax.org/books/calculus-volume-1/pages/3-6-the-chain-rule). These support derivative/tangent meaning and power, sum, product and chain conditions. The twelve cases, explanations and wrong options are original authoring, not copied textbook exercises. Reviewed Wave 01 calculus notation/disclosure is a read-only format reference.

## Delivery and ownership

Source: this calculus folder only, excluding coordinator-owned BRIEF.md. Evidence: build/production/wave02/calculus only. Authoring metadata and one chapter document are canonical; cases.json is an audit input. Keep published Wave 01, shared tools/binaries, other writers and 3D work untouched. Deliver one ready-for-coordinator-review production.json; freeze source after one authorized callback. Coordinator owns acceptance, immutable capture, aggregate/save review and publication. No windows, screenshots, font initialization, commits or further delegation.
