# Rational equations — Wave 04 delivery review

## Delivered and frozen

Package prod04_algebra_rational_equations, version 1; one canonical lesson and
12 complete problems in topic_0004. The actual compiled chapter has 53
decisions, 159 options and 106 specific wrong-choice corrections. All first
positions are balanced four times each. No generator or new shared framework
was added. The six maintained source files are complete and frozen at the
hashes in build/production/wave04/algebra/production.json.

The chapter SHA-256 is
e5d2e6d7bfc59f6e0a720f8ddb6541b488d5ea589df524110af31a2be3c26d04.
At closeout it still exactly matches the prior passing compiled inspection.
The coordinator also reported independently reviewing the entire lesson,
all 53 decisions/159 options, source exercises and certificate code, with no
mathematical correction requested. No passing gate was repeated at closeout.

## Source and teaching review

All 12 problems adapt eight distinct approved exercises: Section 3.7,
Exercises 5, 6, 7, 8, 11, 12, 13 and 16, printed page 234 / PDF page 76,
Lippman and Rasmussen, Precalculus: An Investigation of Functions, Edition 2.3.
The source's rational functions are transformed from graphing tasks into
explicit equations. cases.json records each original function and change;
DESIGN.md also maps the separate lesson example.

CC BY-SA 4.0 attribution is in metadata and the public lesson summary,
including Chapter 3's permitted remix from Stitz and Zeager, College Algebra
(2013). No NC solutions manual, UW-marked exercise, photograph or graph was
used. The PDF workflow was text-only: detached square exponents in Exercises
7/8 were checked against a second pdftotext -raw extraction, retained in
seed-page-raw.txt. No missing source entry was guessed.

Original restrictions precede simplification in every question. Correct
choices are goal-correct, not just mathematically equivalent. Histories and
feedback use ordinary readable prose; symbolic fields carry the TeX.
The separate worked example solves (2x-3)/(x+4)=-1 with x=-1/3.
Hint, Answer and Solution remain independently closed; public blocks do not
repeat its numerical answer.

| Case | Original exclusions | Complete solution set / exceptional reasoning |
| --- | --- | --- |
| q01 | -4 | {7}; original sides both 1. |
| q02 | 1/3 | {-3/5}; original sides both 2. |
| q03 | 2 | {4}; original sides both 2. |
| q04 | -1 | {-6}; original sides both -1. |
| q05 | -1, 1 | Empty: sole polynomial candidate 1 makes the original denominator zero. |
| q06 | -2, 2 | {1}; candidate -2 is excluded; original sides at 1 are both 2. |
| q07 | -3, -1/2 | {-6,1}; original sides are 1/3 at each root. |
| q08 | 4 | {-3,3/2}; original sides are -2 at each root. |
| q09 | -1, 1 | {3}; safe cancellation retains both exclusions; original sides both 3/2. |
| q10 | -2, 2 | Every real input except -2 and 2; equal cleared polynomials prove the identity on the original domain. |
| q11 | -4 | Empty: the complete original left-minus-right difference is -11/(x+4), never zero on the domain. |
| q12 | 1/3 | {-7}; both original sides are 1/2, including both additive terms on the left. |

The mixed group selects and completes methods in q09, q10 and q12. Empty
sets and the restricted identity are established symbolically, not by samples.
Two required cases explicitly reject denominator-zero candidates.

## Completed checks

All commands were run from the Paths root; their exit statuses were zero.
Evidence is under build/production/wave04/algebra/.

- sorter --inspect-documents: inspection.json accepted; one lesson and 12 questions.
- paths_learning_document_tests --question-batch: routes.json accepted;
  all 12 routes and 106 wrong choices, save replay true, windows 0.
- paths_learning_document_tests --family-lessons: lessons.json accepted;
  three closed and three independently opened worked disclosures, windows 0.
- certificate_tests.py --routes .../routes.json --edited-routes
  .../markdown-edit/routes.json, with PYTHONPATH=tools and python3 -B:
  mathematics.json covers all 53 decisions and 159 options.
- All 54 negative controls reject; all 10 valid alternative-form controls pass.
  Controls cover false keys/reached work, cancellation, omitted LCD terms,
  extraneous candidates, lost original exclusions, invalid domains, equivalent
  sets/fractions, false original evaluations and true-but-wrong-goal forms.
- A real scratch Markdown prompt and wrong-feedback edit was compiled. Exactly
  q01 step 10's prompt and option 12's feedback changed; every other compiled
  field and independent mathematical result remained unchanged.
- Existing exporter Target/provenance verified all 13 IDs and the assigned
  chapter/subject; provenance.json records them.
- Approved executable hashes match build-ready.json. Only the data-only
  inspection/model gates ran; the UI binary was hashed, not executed.
- source-evidence.json pins the source PDF, two text representations,
  front matter, source-page HTML and source registry.

## Checker independence and limits

The checker reads actual compiled route JSON, verifies each original given and
domain against the finite case record, and independently derives rational
polynomial identities, denominator restrictions, LCDs, polynomial candidates,
candidate filtering and original evaluations. Authored keys are comparisons,
not the arithmetic oracle. It separately checks truth and the requested form.
The identity is a cofinite set, not an unrestricted real solution set.

The frozen Wave 03 algebra helper is used read-only for its mathematical-field
AST adapter. question_workflow.exact and the existing exporter are reused;
their exact hashes are in executable-check.json and the receipt.
No old case keys, old certificate results or old generator are used as answers.

The bounded inputs have degree at most two, integer coefficient magnitude at
most 30, at most two additive terms per side and two distinct rational linear
denominator factors, with constant denominator contents at most 6. The
cleared polynomial has degree at most two and coefficient magnitude at most
100; root numerators are bounded by 30 and denominators by 6. Exact
cross-products may temporarily reach degree four before gcd reduction.
Repeated/irreducible variable denominator factors and higher-degree originals
are refused. This is not a general CAS, inequality or graphing checker.

## Observed timing and correction record

- Start: 2026-09-11T03:20:16Z.
- Source-page verification: 2026-09-11T03:22:28Z.
- Case design: 2026-09-11T03:26:36Z.
- Draft ready: 2026-09-11T03:34:51Z.
- Prescribed checks complete: 2026-09-11T03:43:05Z.
- Receipt closeout resumed: 2026-09-11T03:56:23Z.

One writer self-review refinement at 03:41:47Z tightened displayed
denominator-clearing form checks and added original-evaluation/uncanceled-form
controls; unsupported-input probes were aligned with their coefficient
vectors. The original chapter already compiled and passed its mathematics;
no mathematical source correction was required. Exercises 15/17/18 were
rejected as seeds because unnecessary cubic structure exceeds the chosen
degree scope. The extraction difficulty and this refinement are recorded in
trial-timing.json. These are observed phase timestamps, not uninterrupted
effort, token counts, account-usage savings or a causal speed comparison.

## Remaining concerns and handoff boundary

No known mathematical or source-provenance blocker remains. Coordinator
aggregate integration and publication remain separate; this receipt is
ready_for_coordinator_review and published false. No visual observation or
learner-retention/transfer claim is made. No rebuild, screenshot, capture,
window, clipboard operation, personal-save write, commit or publication was
performed for this family. Earlier published content stays unchanged.
Return the normal receipt handoff without a callback; stop after this family.
