# Wave 04 calculus delivery review

Ready for coordinator capture and final review; **not published**. Twelve
complete problems, one lesson, 76 decisions, 228 options and 152 individual
wrong-choice corrections. The chapter is unchanged from the reviewed draft:

`addb623bd47eaafb26128aa8b33e06d8a4d2677f403f90428c6e68dda8829f8a`

The coordinator's preliminary review read the whole lesson and every decision,
option, reached state and feedback passage and reported no mathematical finding.
Its chapter hash and the source review's case hash match current bytes. This
delivery supplies the formerly missing independent finite certificates, real
Markdown edit probes and final source/evidence receipt. Prior calculus waves,
other writers, shared tools and stores remain untouched.

## Independent derivations

Here g is the selected inner function, lambda satisfies m=lambda*g' exactly,
and F=P+C is the complete family on the stated connected domain. Coefficients,
not answer labels, determine these results. Every listed primitive is checked
by symbolic differentiation against the original compiled integrand.

| Question suffix | g(x) | lambda | P(x) or definite result |
| --- | --- | --- | --- |
| q01 | 3x | 1/3 | exp(3x)/3 + C |
| q02 | 5x+1 | 1/5 | sin(5x+1)/5 + C |
| q03 | 2-7x | -1/7 | -(2-7x)^4/28 + C |
| q04 | 8-3x | -1/3 | cos(8-3x)/3 + C |
| q05 | x^2 | 1/2 | exp(x^2)/2 + C |
| q06 | 4x^2+7 | 1/8 | (4x^2+7)^3/24 + C |
| q07 | 5x^3+1 | 1/15 | ln|5x^3+1|/15 + C, x>0 |
| q08 | 11x-9 | 1/11 | ln|11x-9|/11 + C, x<0 |
| q09 | 1+4x^2 | 1/8 | u-bounds (5,17); I=(ln(17)-ln(5))/8 |
| q10 | 3-2x | -1/2 | u-bounds (3,1); I=13/3 |
| q11 | x^2 | 1/2 | u-bounds (0,1); I=(exp(1)-1)/2 |
| q12 | x^2 | 1/2 | u-bounds (0,1); I=sin(1)/2 |

q03's derivative coefficient is (-1/28)*4*(-7)=1. q04's is
(1/3)*(-1)*(-3)=1. The two negative signs serve different roles and must not
be conflated. q06's coefficient is (1/24)*3*8=1 on the remaining x factor.
Its expanded zero-constant equivalent is (8/3)x^6+14x^4+(49/2)x^2+C;
the omitted 343/24 is absorbed into arbitrary C, not lost from the derivative.

For q07, x>0 gives 5x^3+1>1. For q08, x<0 gives 11x-9<-9; ln(11x-9)
is not real, but ln|11x-9| is valid and has derivative 11/(11x-9). The
absolute value does not admit the pole 9/11 or extend the original interval.
One arbitrary real constant is complete on each stated connected interval by
the mean value theorem applied to the difference of two primitives. No
pointwise division by a vanishing inner derivative is needed in the x^2 cases.

For q09 the denominator is positive on [1,2]. Independently substitute x=2
and x=1 into P=ln|1+4x^2|/8: the values are ln(17)/8 and ln(5)/8.
For q10, P=-(3-2x)^3/6 gives P(1)=-1/6 and P(0)=-27/6; their difference
is 26/6=13/3. Keeping both the negative scale and the u-bound order (3,1)
produces the positive integral of the original square. q11 has lower primitive
value exp(0)/2=1/2, not zero. q12 has lower primitive value sin(0)/2=0,
not 1/2. All four original integrands are continuous on their closed intervals.

The distinct worked example uses g=x^2+2, g'=2x, lambda=1 and
P=-cos(x^2+2). Its derivative is exactly 2x sin(x^2+2). The original
endpoints 0 and 1 map to 2 and 3, giving cos(2)-cos(3) by both coordinate
routes. Hint, Answer and Solution are separately closed; the answer is not in
public example text. The lesson defines every requested term and rule condition,
including arbitrary constants, nonzero logarithm arguments and transformed bounds.

## Gates and adverse controls

The pinned headless binaries match build/production/wave01/build-ready.json.
The actual native parse is equal to the coordinator's hash-matched preflight:
12 routes with saved-route replay, one lesson, four closed disclosures and three
independent worked-example disclosures, zero windows. Target inspection and
exporter provenance pass. The twelve question @read links target the canonical
lesson, whose ordered practice links exactly match q01 through q12.

The new certificate checks all compiled original givens/domains, all 228 options,
all 76 reached states, exact derivative identities, completeness conditions,
ordered bounds and definite evaluations. It does not infer truth from the key.

The final control suite requires rejection of:

- 76 wrong-key mutations and 76 wrong reached-state mutations;
- 84 duplicate-answer mutations, including fixed constant shifts, rescaled C,
  trig parity, an expanded polynomial family, normalized logarithm arguments,
  logarithm quotient/difference identity and equivalent rational coefficients;
- 62 invalid input/domain mutations, plus a separate exact closed-interval
  pole-crossing probe;
- five mathematically true but wrong-goal forms: two alternative inner
  definitions, two unfinished u-families, and a simultaneous scale/bound reversal
  that violates the explicitly retained mapped-bound order;
- five targeted missing-C, nonreal-log, mixed-bound and false-back-substitution
  mutations.

Eleven valid equivalent replacements must pass. Of the 84 duplicate probes,
76 are per-step exact or whitespace-normalized duplicates and eight exercise
the nontrivial equivalences above. These counts do not imply universal symbolic
equivalence detection. The complete measured counts are in mathematics.json.

Two scratch probes edit actual ordinary Markdown: one q01 step prompt and one
individual q01 wrong-feedback passage. Each is compiled by the native route
tool. The resulting JSON must equal the original route JSON with precisely that
one field changed, and the full mathematics suite must still pass. The source
before/after hashes are recorded. Both edited Markdown sources, native compiled
replays and explicit one-field JSON diffs are retained in content-addressed
edit-probes folders under this subject's evidence directory. Their hashes are
included in mathematics.json and production.json; no maintained chapter is modified.

## Source, license and trial observations

All twelve cases map to actual approved prompts, with **ten distinct seeds**:
Preview 5.3.1(b)(i,ii,iv), (c)(ii,iii); Activities 5.3.2(a,c), 5.3.3(a),
5.3.4(a,b). DESIGN.md and each cases.json source object record exact original
givens and changes. The separate worked example maps to 5.3.3(b). This is
not theorem-only attribution for unrelated invented questions.

Matthew Boelkins, with David Austin and Steven Schlicker, *Active Calculus
Activities Workbook Chapters 5-8*, 2018 edition updated August 1, 2024.
Source: https://activecalculus.org/wp-content/uploads/2024/08/acs-activity-workbook-58-2024.pdf
The adapted lesson and questions use CC BY-SA 4.0:
https://creativecommons.org/licenses/by-sa/4.0/ . The metadata covers all thirteen
content IDs and the public lesson summary carries readable attribution,
adaptation notice and license. This does not make an application-license claim.
No source figures, photographs or solution text are reused.

The pinned PDF/text and registry hashes are independently checked and carried in
the receipt. Text extraction split superscripts and one reciprocal fraction.
The PDF skill's text-only extraction path was used, with no rendering or images:
pdftotext raw/bounding-box text confirmed x exp(x^2) and 1/(11x-9).
Optional pdfplumber was unavailable; no installation was necessary. The
coordinator's source review independently confirmed the same givens. The
larger power and non-polynomial inner seeds were explicitly simplified or
excluded, as recorded in DESIGN.md and phases.json.

Observed UTC milestones: assignment start 03:20:24, case design 03:26:53,
complete draft 03:38:36, resumption 04:28:53, all on 2026-09-11. The final
checks-done timestamp is observed by the checker and retained in phases.json
and production.json. The intervening usage-limit interruption is recorded in
the coordinator's interruption.json; do not interpret this wall-clock span as
uninterrupted authoring or make token/weekly-usage savings claims.

There were two certificate development failures (a shadowed evaluator name and
the frozen literal-exponent cap), both corrected only in the new checker.
A further review tightened the retained-bound-order goal control. On coordinator
request the scratch proof was retained as auditable source/replay/diff artifacts
instead of deleting its temporary copies. **Zero
chapter or case correction rounds** were required after the coordinator's
preliminary review. The final receipt binds the actual completed source and
evidence hashes, both reused helper generations, source snapshots and binaries.

## Limits and remaining authority

This is a finite elementary-expression checker, not a general integrator or
CAS. It uses exact polynomial arithmetic, elementary derivative identities,
parity, logarithmic normalization and prime-factor log constants. Domain proofs
are sufficient exact coefficient/Bernstein sign certificates; an unproved case
fails closed. It does not claim all trigonometric/exponential identities or all
coefficient combinations. Prose truth is reviewed by people/agents reading it,
not established by string-presence checks. The coordinator's reviewed prose
hash is reused rather than rewriting or redundantly reviewing a good chapter.

Final coordinator capture and certificate review, aggregate integration and
preserved progress gates remain before publication. No windows, screenshots,
captures, fonts/ImGui, clipboard, active-store writes, saves, commits, other
writers or later family were involved. Source freezes at this handoff.
