# Substitution: finite source-assisted family

Design fixed before the case pool. Package `prod04_calc_substitution`, version 1;
one lesson, twelve editable choices.v1 problems, chapter topic_0063.

## Finite mathematical scope

An original integrand is m(x) f(g(x)), with g a nonconstant polynomial of degree
at most 3, integer coefficients of absolute value at most 11, and m a nonzero
monomial of degree at most 2 with integer coefficient of absolute value at most
2. Require m=lambda*g' as an exact polynomial identity, with nonzero rational
lambda of denominator at most 15. The outer f is exp, sin, cos, reciprocal, or
an integer power 2 or 3. No general integration or root-finding is attempted.
Polynomial primitives have degree at most 6. Endpoints are integers 0, 1 or 2.
Domains are R, x>0, x<0, or a closed interval between explicit ordered limits.
Reciprocal cases require a sign proof that g has no zero on that domain.

The field checker reuses the frozen Wave 03 rational-polynomial adapter and
its Wave 02 exact Poly dependency read-only. The new adapter admits only a sum
of polynomial multiples of exp/sin/cos/log-absolute-value/reciprocal atoms with
polynomial arguments, rational scalars and an arbitrary constant. It uses exact
chain-rule identities, polynomial equality, parity, normalized log arguments,
and constant-family equivalence. Unsupported syntax fails closed. It is not a
general CAS or document parser. Native tools remain the Markdown parser.

For an indefinite problem: choose the specified complete inner argument,
differentiate it, convert the entire integral including its scalar, integrate
in u, return a family in x, and differentiate against the original integrand.
For a definite problem: choose that substitution, differentiate, map both
ordered bounds, convert the full bounded integral, find a u-primitive, evaluate
upper minus lower, then check with an x-primitive derivative and endpoint
evaluation. The last four each include a method choice carried through.
Truth and the requested coordinate/form are separate predicates. A correct
u-expression is not a completed x-family; a legal identity substitution can
still miss the explicit complete-argument goal. Equivalent families must never
compete as distinct options.

## Exact source and adaptation map

Source id `active_substitution`: Matthew Boelkins, with David Austin and Steven
Schlicker, *Active Calculus Activities Workbook Chapters 5-8*, 2018 edition,
updated August 1, 2024. Section 5.3, printed pp.17-24 / PDF pp.27-34.
https://activecalculus.org/wp-content/uploads/2024/08/acs-activity-workbook-58-2024.pdf
CC BY-SA 4.0: https://creativecommons.org/licenses/by-sa/4.0/ .
The adapted lesson and questions retain that license; this does not license the
application. All teaching prose, choices and feedback are newly written. No
figures, photography, source answer text or learner attempts are reused.

| Question | Exact seed | Original givens | Adaptation |
| --- | --- | --- | --- |
| q01 | Preview 5.3.1(b)(i), p.17 | m(x)=exp(3x) | Retain function; structured full solve. |
| q02 | Preview 5.3.1(b)(ii), p.17 | n(x)=cos(5x+1) | Retain function; structured full solve. |
| q03 | Preview 5.3.1(b)(iv), p.17 | v(x)=(2-7x)^3 | Retain function; structured full solve. |
| q04 | Activity 5.3.2(a), p.19 | integral sin(8-3x) dx | Retain integrand; structured full solve. |
| q05 | Preview 5.3.1(c)(iii), p.18 | c(x)=x exp(x^2) | Retain function; structured full solve. |
| q06 | Preview 5.3.1(c)(ii), p.18 | b(x)=(4x+7)^11 | Replace x by x^2 in inner, exponent 11 by 2, add multiplier x. |
| q07 | Activity 5.3.3(a), p.21 | integral x^2/(5x^3+1) dx | Retain integrand; specify connected domain x>0. |
| q08 | Activity 5.3.2(c), p.19 | integral 1/(11x-9) dx | Retain integrand; specify connected domain x<0. |
| q09 | Activity 5.3.4(a), p.23 | integral from 1 to 2 of x/(1+4x^2) dx | Retain complete integral. |
| q10 | Activity 5.3.4(b), p.23 | integral from 0 to 1 of exp(-x)(2exp(-x)+3)^9 dx | Replace inner by 3-2x, multiplier exp(-x) by 1, power 9 by 2; retain bounds. |
| q11 | Preview 5.3.1(c)(iii), p.18 | c(x)=x exp(x^2) | Retain function, request definite integral from 0 to 1. |
| q12 | Preview 5.3.1(b)(ii), p.17 | n(x)=cos(5x+1) | Replace inner by x^2, add multiplier x and bounds 0 to 1. |

Ten distinct seed subparts among the twelve problems. The separate worked
example adapts Activity 5.3.3(b), p.21: original integral exp(x) sin(exp(x)) dx.
Replace inner exp(x) by x^2+2 and its multiplier exp(x) by 2x; ask for both the
antiderivative family and the integral from 0 to 1. This is a function-derivative
pair adaptation, not merely a citation to a general theorem.

The source's exp(x) inner, reciprocal inner 1/x, square-root inner and inverse
trigonometric primitives are outside this finite verifier and are not silently
certified. In particular 5.3.4(b) is deliberately simplified as recorded above;
5.3.3(c), 5.3.4(c), 5.3.1(b)(iii), 5.3.2(b,d,e,f) are not selected. The original
power 11 in 5.3.1(c)(ii) exceeds the primitive degree bound and is reduced.

## Teaching and evidence

q01-q04 introduce missing factors and two negative derivatives. q05-q08 cover
nonlinear pairs, two logarithmic domains including negative arguments, and
arbitrary constants. q09-q12 use reciprocal, polynomial, exponential and
trigonometric integrands with finite continuous intervals. q10 reverses the
u-bound order and keeps the negative scale. First keys occupy each position
four times; subsequent positions rotate deterministically.

Required evidence: actual compiled givens, all options, reached states and
original derivative identities; independent endpoint substitution; exact
duplicate semantics; adversarial keys/states/domains/mixed bounds/wrong forms;
valid equivalent controls; headless inspection, route replay, lesson disclosure
and provenance gates; actual Markdown prompt/feedback edits in scratch copies.
Source snapshots, registry, binary and frozen helper hashes are recorded.
Observed phase times, extraction difficulties and correction rounds are reported
without token/weekly savings estimates. Delivery is review-ready, not published.
