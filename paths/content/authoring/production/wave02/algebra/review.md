# Algebra depth delivery review

Date: 2026-09-10. Stage: ready for coordinator review, not published or coordinator-accepted.

## Delivered scope

One ordinary Markdown chapter supplies the canonical lesson `prod02_algebra_full_linear_r` and questions `prod02_algebra_full_linear_q01` through `q12`, attached to Algebra / Equations and Relations (`topic_0003`). Package reservation: `prod02_algebra_full_linear`, version 1. All twelve complete problems have four to six meaningful decisions, totaling **52 steps and 104 specific wrong-choice corrections**. The lesson links all twelve in introductory/practice/mixed groups of four; all questions link back to that lesson. There is no new support-level implementation.

## Independent mathematical review

Every compiled given is byte-matched to cases.json and independently expanded from its actual arithmetic with Python's standard ast machinery and an exact affine evaluator. The result must equal a separate Fraction expansion of the original term sums. The audit then derives each local target and final solution set without using the authored key or reached text as an answer oracle. All three choices in every step are evaluated and exactly one meets the local mathematical goal. Every reached-state linkage, intermediate equation, final set and original check is verified. Mathematical equivalence of the original solution set is checked after each algebraic operation; nonzero multipliers/divisors are required when used.

The independent finishing reviewer found that the initial checker normalized away unfinished distribution/combination and explicit term-order requirements, although every delivered equation satisfied its prompt. This F1 gap is now addressed by a separate goal-form check on the existing arithmetic AST for both accepted labels and reached displays. Only q12's simplification goal requires variable terms first; other prompts retain term-order flexibility. The mathematical target is still independently derived from original inputs. Each option's mathematical-target match and representation match are recorded separately in mathematics.json.

| Cases | Independently derived result | Original check |
|---|---|---|
| q01 | x=6 | Both sides 13 |
| q02 | x=-2 | Both sides 7 |
| q03 | x=7 | Both sides 9 |
| q04 | x=7 | Both sides 3 |
| q05 | x=3 | Both sides 1 |
| q06 | x=20/7 | Both sides 4 |
| q07 | Empty solution set | L(x)-R(x)=-5 for every real x |
| q08 | All real numbers | L(x)-R(x)=0 for every real x |
| q09 | x=13 | Both sides 1 |
| q10 | x=-9/11 | Both sides 30/11 |
| q11 | Empty solution set | L(x)-R(x)=-1 for every real x |
| q12 | All real numbers | L(x)-R(x)=0 for every real x |

This meets eight unique, two empty and two all-real outcomes. Fractional/negative unique answers, distribution, variables on both sides, whole-side denominator clearing and two initial method selections are present. q09 omits a redundant divide-by-one step. Degenerate cases do actual expansion/scaling and cancellation before classification and a separate original-expression completeness check; none divides by zero or invents a unique answer.

The lesson's separate original equation -2(x+1)+3=x/2-4 gives -2x+1=x/2-4, then -4x+2=x-8, -5x=-10 and x=2. Original evaluation is -3 on each side. Its givens differ from all twelve problems. Public block arithmetic, negative-distribution examples and the general unique/identity/contradiction conditions were checked as teaching prose, independently of the structural lesson gate.

## Teaching review

Every resolved calculation supplies actual operands and the applicable reversible rule; no equation of the form ax+b=cx+d is treated as automatically having a nonzero final coefficient. Terms, coefficients, signed constants, equality, real domain, denominator, solution sets, singleton/empty/all-real notation, L/R evaluation, ordered-pair order, multipliers and the general rule symbols are defined. Ordinary-prose explanations remain readable in history; extended formulas use native math fields/lesson displays. No source-status prose appears in learner content.

Wrong choices have specific correction text: omitted side changes, wrong signed coefficient subtraction, incomplete distribution, missing outside scaling, reciprocal division, omitted original-check terms, reversed differences, and confusing cancellation with a singleton zero. The method questions correctly describe multiplication by 2 or 3 as reversible but insufficient for their explicit all-denominators goal. The first-position sequence [1,2,3,2,3,1,3,1,2,1,3,2] uses each position four times; later positions vary and keep semantic IDs attached to corrections.

No title or domain states the problem's outcome. The lesson's worked given is public, but its numerical answer and solution are separately closed. The generic condition block teaches how to classify outcomes without naming a question's result. Final checks deliberately follow an already derived candidate/set; they verify it rather than pretend that the result is still hidden. Preparation and wording do not establish learner retention or transfer.

## Headless gates and fault injection

All commands below exited zero. Evidence is under `/Users/kogaryu/iggy3d/paths/build/production/wave02/algebra/`.

1. `b/sorter --inspect-documents --documents content/authoring/production/wave02/algebra/authoring/documents` — inspection.json; one lesson, twelve questions, no diagnostics.
2. `b/paths_learning_document_tests --question-batch content/authoring/production/wave02/algebra/authoring/documents` — routes.json; twelve full prepared routes, 104 wrong-choice rejections, preserved working/feedback, isolated save replay, explicit completion/Next behavior; windows=0. This is the existing prepared route, not the old six-role gate.
3. `b/paths_learning_document_tests --family-lessons content/authoring/production/wave02/algebra/authoring/documents` — lessons.json; required local blocks pass, three disclosures begin closed and the worked Hint/Answer/Solution open independently; windows=0.
4. `PYTHONPATH=tools python3 -B content/authoring/production/wave02/algebra/certificate_tests.py --routes build/production/wave02/algebra/routes.json --edited-routes build/production/wave02/algebra/markdown-edit/routes.json` — mathematics.json; all 52 mathematical decisions pass, all 24 negative regressions reject their deliberate corruption, and four valid equivalent-form controls pass.
5. `export_learning.Target.inspect(documents=...)` followed by `export_learning.provenance(author, report['entities'])` — provenance.json; covers the lesson plus every question (13 content IDs).

The twelve original negative checks are false accepted key, false intermediate working, equivalent duplicate choice with another arithmetic spelling, changed compiled domain, changed compiled given, invalid finite-domain declaration, zero original denominator, out-of-bound original coefficient, false final set, nonlinear product, variable denominator and zero divisor. Twelve added form checks reject the reviewer's three goal-missing equivalents separately in accepted label, reached display and both together, plus uncombined constants, uncombined variable terms and a displayed denominator after a clearing goal. All form probes first confirm unchanged affine side values, so their rejection is specifically about the requested representation. Four accepted controls exercise permitted equivalent term order and notation. Detailed reasons are in mathematics.json.

The evidence-only scratch copy `markdown-edit/documents/chapter.paths.md` contains a real q01 first-prompt edit and a real wrong-feedback edit. It was recompiled through --question-batch. The checker confirms that the two requested compiled fields change, every other question field stays identical, and all twelve independent mathematical certificates stay identical. The source chapter was never used as scratch, and no other writer's files were restored or changed.

The bounded F2 correction now describes q04/step40/option43 as reversing the quotient: (-1)/(-7)=1/7 instead of the required (-7)/(-1)=7. This is the only changed compiled question field relative to the original captured candidate. The two previously approved lesson-prose deletions remain; every other learner-source byte, question value, choice/key, reached state, prompt, worked example and identity is unchanged. Exact source diffs and compiled-field comparison are in build/production/wave02/algebra/finishing-correction/delta.json. The captured candidate and independent review evidence remain untouched; the coordinator must verify/capture this corrected source before release. No callback is sent for this correction.

## Provenance and limits

The original exercises and explanations use definitions/conditions checked against OpenStax Elementary Algebra 2e sections 2.3, 2.4 and 2.5, browsed on 2026-09-10. Exact URLs and reuse statements are in DESIGN.md and authoring.json. No exercise or solution text was extracted. The checker reuses the existing exact-number and number-format helpers, with their file hash recorded alongside the executable hashes.

No screenshots, images, captures, windows, font initialization or ImGui probes were used. Structural/compiler/model checks do not verify visual appearance, teacher effectiveness or independent written work. SymPy was unavailable and was not installed; the actual certificate uses only Python's standard library plus the existing project numeric helper. No dependency or shared runtime/tooling change was needed.

The final machine-readable delivery is `build/production/wave02/algebra/production.json`, with exact source and inspected input hashes, model/target hashes matched to build-ready.json, evidence hashes, counts and remaining concerns. Wave 01, earlier sources, binaries, shared tools, stores, personal saves and 3D assets were not edited. There are no known arithmetic, compiler or disclosure blockers; native appearance and independent coordinator review remain outside this source-ready claim. Source freezes at the authorized completion handoff until a correction is assigned.
