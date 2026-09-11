# Rational equations with original restrictions

## Learning claim, prerequisites and finite bounds

Complete a real rational-equation solve: determine the original denominator
restrictions, choose an LCD or permitted factor cancellation, clear every term,
solve the resulting polynomial, filter candidates, and check the original sides.
Prerequisites are signed rational arithmetic, distribution, linear solving and
factoring a quadratic. The lesson defines rational expression, numerator,
denominator, domain, excluded set E, LCD multiplier Lambda, candidate set C,
solution set S and original evaluations L/R/V. Prepared-choice success is not
evidence of independently written solving, retention or mastery.

The delivered domain is twelve finite rational equations. Each original side
has at most two additive terms. Each numerator and each denominator is a
polynomial of degree at most two with integer coefficients of magnitude at most
30. Denominators are nonzero polynomials split over rational linear factors;
fixed constant denominators are at most 6. The union has at most two distinct
variable factors, each with multiplicity at most one in these adapted inputs.
The least integer-coefficient common denominator is formed from primitive
positive-leading linear factors and the LCM of positive denominator contents.
Cleared numerator difference has degree at most two, coefficient magnitude at
most 100 and only rational roots when nonconstant. Candidate/excluded root
numerators have magnitude at most 30 and denominators at most 6.
Intermediate arithmetic is exact. Rational cross-products may temporarily
reach degree four before polynomial gcd reduction; originals and cleared
results remain degree at most two. No inequalities, graphing, radicals, larger
original polynomials, nested rational denominators or general CAS are claimed.

Original denominators determine E BEFORE any cancellation. On the complement
of E, Lambda is nonzero and multiplying by it is reversible. A zero cleared
polynomial proves an identity on that original domain only; a nonzero constant
proves impossibility. Linear/quadratic polynomial solving derives all candidates,
then exact original denominator evaluation filters them. Every surviving finite
root is substituted into both original sides; identity/empty outcomes use
symbolic identities or exhaustive-candidate rejection, not sample points.

## Pinned source and exact adaptation map

Source id precalc_rational: David Lippman and Melonie Rasmussen, Precalculus:
An Investigation of Functions, Edition 2.3, Section 3.7 Exercises, printed page
234 / PDF page 76. Source URL:
https://www.opentextbookstore.com/precalc/2.3/Chapter%203.pdf
Registry access time: 2026-09-11T03:16:52.020541+00:00.

The source's common task asks for intercepts and asymptotes followed by a graph.
This family changes that task to explicit real equations. The given rational
functions actually seed the equations; references are not merely theorem
citations. None of Exercises 5,6,7,8,11,12,13,16 has a UW credit marker.
No NC solution manual, images or source answers are used.

| Case | Source exercise and original function | Explicit adaptation |
|---|---|---|
| q01 | 5: p(x)=(2x-3)/(x+4) | Solve p(x)=1; unchanged function. |
| q02 | 6: q(x)=(x-5)/(3x-1) | Solve q(x)=2; unchanged function. |
| q03 | 7: s(x)=4/(x-2)^2 | Change the denominator exponent from 2 to 1, then solve 4/(x-2)=2. |
| q04 | 8: r(x)=5/(x+1)^2 | Change the denominator exponent from 2 to 1, then solve 5/(x+1)=-1. |
| q05 | 11: a(x)=(x^2+2x-3)/(x^2-1) | Factor the original denominator; solve a(x)=2. |
| q06 | 12: b(x)=(x^2-x-6)/(x^2-4) | Factor the original denominator; solve b(x)=2. |
| q07 | 16: m(x)=(5-x)/(2x^2+7x+3) | Factor the denominator as (2x+1)(x+3); solve m(x)=1/3. |
| q08 | 13: h(x)=(2x^2+x-1)/(x-4) | Solve h(x)=-2; unchanged function. |
| q09 | 11, same original as q05 | Display factored numerator and denominator; solve a(x)=3/2 using safe cancellation. |
| q10 | 12, same original as q06 | Equate b(x) to its canceled expression (x-3)/(x-2), retaining both original exclusions. |
| q11 | 5, same original as q01 | Solve p(x)=2, producing a contradiction. |
| q12 | 6, same original as q02 | Add 1/(3x-1) on the left and equate the sum to 1/2. |
| worked lesson | 5: p(x)=(2x-3)/(x+4) | Solve p(x)=-1, distinct from every question. |

Thus all twelve questions are source-assisted adaptations, representing eight
distinct approved exercises. cases.json repeats the full source object for
each question, including locator, original mathematical givens and changes.

## License and credit

The adapted lesson and questions are CC BY-SA 4.0:
https://creativecommons.org/licenses/by-sa/4.0/
Credit David Lippman and Melonie Rasmussen, and Chapter 3's material remixed
with permission from College Algebra, Carl Stitz and Jeff Zeager (2013).
The front matter states that the main text is CC BY-SA 4.0 and separately
identifies the authors' permission for the Chapter 3 remix. Preserve both
credits, the source URL, edition and adaptation notice in metadata and public
lesson summary. This content license makes no claim about the application's
software license. It does not rely on the less-than-ten-percent alternate-license
permission and does not reuse UW-marked or NC-manual material.

The pinned PDF, layout text, front-matter evidence and registry hashes will be
recorded in source-evidence.json and the final receipt. Only text was read.
The layout extraction detaches the powers in Exercises 7/8; an independent
pdftotext -raw extraction of page 76 confirms their square denominators.
Its output is retained as seed-page-raw.txt. Exercises 15/17/18 were not selected:
their cubic structure is outside this family's intended cleared-degree scope.
No ambiguous mathematical entry was guessed.

## Deliberate teaching sequence

q01-q04 use one variable denominator and four complete decisions each.
q05-q08 combine two factors, a canceled exclusion, an extraneous candidate,
negative roots and fractional roots. q09 chooses safe cancellation as a method;
q10 chooses an LCD for an identity; q12 chooses an LCD for an additive equation.
q11 establishes an empty set without inventing a zero candidate. The exact
planned count is 53 decisions, 159 options and 106 wrong-choice corrections.

Every first decision identifies E, before changing the original expression.
Correct first positions are 1,2,3,2,3,1,3,1,2,1,3,2; later positions rotate.
All choices are compact symbolic fields; explanatory histories use ordinary
prose. Distractors show omitted exclusions, numerator-zero confusion, partial
LCD multiplication, wrong signs, false cancellation, incomplete candidate
sets, retained denominator-zero candidates and an unrestricted identity claim.
Equivalent fractions and reordered sets are normalized for duplicate detection.

The canonical lesson has start/terms/rule/condition/worked/errors/practice/
summary and twelve ordered practice links. The distinct worked equation's
solution and arithmetic are only in independently closed Answer/Solution
disclosures. Public summary contains the readable source, license and change
notice. No task title announces an answer or solving method.

## Independent verification and boundaries

One bounded subject checker reads actual --routes model JSON, compares original
givens/domain to the finite cases and derives rational arithmetic from those
givens. Reuse the frozen Wave 03 algebra arithmetic AST/polynomial helpers
read-only where their degree-two contract fits, plus question_workflow.exact;
do not copy a parser or create a generator. New logic concerns rational-term
denominators, primitive LCD construction, candidate filtering and displayed
goal forms. Hash all such helper dependencies.

Truth and requested form are separate: selecting a denominator-clearing
multiplier is different from giving an equivalent equation; canceled fractions
must retain original exclusions; the identity's set is cofinite, not all reals.
Test all 53 states and 159 options, false keys/states, false cancellation,
omitted LCD terms, extraneous roots, wrong domains, equivalent fractions/sets,
true but goal-missing forms, and valid alternative spellings.

Run the prescribed inspection, routes and disclosure gates and exporter
Target/provenance. Make a real prompt and feedback edit in scratch Markdown,
compile it, and prove exactly those two fields changed while mathematical
certificates remain identical. Verify approved executable hashes. All evidence
goes under build/production/wave04/algebra; no previous source, shared tool,
runtime, 3D, active store or personal save is changed.

## Measurement and handoff

The observed start timestamp is 2026-09-11T03:20:16Z. Record further observed
case-design, draft-ready and checks-done timestamps in trial-timing.json.
Record extraction difficulties and actual correction rounds in review.md;
do not invent token counts, usage savings or causal speed comparisons.
Return the exact production.json and freeze the six maintained source files.
The coordinator independently reviews and publishes; no callbacks or new tasks.
