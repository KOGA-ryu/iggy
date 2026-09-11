# Wave 03 algebra delivery review

Status: ready for coordinator review; unpublished. Package
prod03_algebra_quadratic_roots, version 1, attaches to topic_0004 /
Polynomials and Rational Expressions. Work is uncommitted. Source freezes at
the hashes in build/production/wave03/algebra/production.json.

## Delivered content

One maintained ordinary Markdown chapter contains one canonical lesson.v2
reading and twelve complete choices.v1 problems, q01 through q12. All questions
read the same lesson, and its ordered practice links cover the same twelve IDs.
There are four introductory, four practice and four mixed/exceptional problems,
50 decisions, 150 symbolic options and 100 misconception-specific wrong-choice
corrections. First-key positions are 1,2,3,2,3,1,3,1,2,1,3,2: four of each.

The mathematical pool was planned in DESIGN.md/cases.json before the chapter.
It includes nonmonic and negative-leading quadratics, denominator clearing,
one repeated root, fractional roots, two zero-root problems, three problems
with irrational real roots, a negative discriminant and an original product
equal to a nonzero constant. q09 begins by selecting common-factor
transformation; q10 begins by selecting completion of the square and carries
that method through both branches. The equivalent factor rewrite in q10 is
explicitly described as valid but missing the requested single-square goal.

## Independent original-input mathematics

The checker reads the actual model JSON supplied by --routes, not Markdown,
an answer generator, certificate output or a copied table of accepted labels.
Original expression expansion is compared to the independent coefficient
vectors in cases.json. Exact rational polynomial convolution, squarefree-radical
normalization, discriminants, completion identities and original-side substitution
derive the roots and every mathematical decision. The formula's completing-square
identity is checked coefficient by coefficient, not at sample points.

Every one of the 150 option values is classified for mathematical truth and
the local requested form. Each of 50 accepted states is independently checked.
Standardization, factorization, formula substitution, complete branches,
solution sets and original-side lists have separate checks. The final negative-D
case instead verifies the identity L(x)-R(x)=(x+1)^2+2 and strict positivity
for all real x. Both original sides are evaluated at every distinct real root
in the other eleven cases. No floats or tolerances are used.

Independent conclusions from original coefficients:

| Case | Complete real roots | Original left/right values |
|---|---|---|
| q01 | 2, 3 | (4,4), (9,9) |
| q02 | 2 only | (8,8) |
| q03 | -3/2, 1 | (3,3), (3,3) |
| q04 | 0, 2 | (0,0), (12,12) |
| q05 | -2, 4 | (0,0), (0,0) |
| q06 | -1/3, 1 | (1/6,1/6), (1/6,1/6) |
| q07 | 2-sqrt(5), 2+sqrt(5) | (1,1), (1,1) |
| q08 | -1-sqrt(2), -1+sqrt(2) | (1,1), (1,1) |
| q09 | -5/2, 0 | (0,0), (0,0) |
| q10 | 1-sqrt(3), 1+sqrt(3) | (2,2), (2,2) |
| q11 | empty | difference (x+1)^2+2 is strictly positive |
| q12 | -2, 3 | (4,4), (4,4) |

The exact scope is the twelve delivered cases plus named negative/positive
controls, not all real quadratics or all possible representations. Fixed
nonzero rational denominators, degree at most two and bounded integer
radicands are supported. The syntax adapter uses Python's bounded arithmetic
AST only for native mathematical fields; it is not a Markdown parser or
runtime solver. The unchanged question_workflow.exact helper owns rational
number construction and bounds; no helper or shared tool was copied or edited.

## Negative and valid-alternative controls

39 negative controls pass. They reject false keys, false reached work,
true-but-unexpanded standard form, true-but-unfactored expressions,
true-but-unsquared method selection, evaluated rather than explicitly
substituted formula, prematurely isolated square-root branches, lost zero
roots, lost minus roots, reordered/equivalent-radical competing sets,
duplicated repeated roots, changed real domain, nonquadratic input,
zero/variable denominators, coefficient/denominator/discriminant/core bounds,
and negative/out-of-bound radical syntax.

For form and lost-root regressions, accepted-label-only, reached-state-only,
and both-field mutations are tested. Some corrupted choices also duplicate
an existing distractor; the evidence records the precise rejection reason,
and reached-state controls exercise the mathematical/form predicate directly.
Eight positive controls accept reordered factors, harmless standard-form
parentheses, reordered root sets, equivalent radical spelling, equivalent
formula notation, extra square parentheses, reversed square-root branch
order, and a harmless positive-witness parenthesis. Root-set prompts request
exact sets, not one canonical radical spelling.

## Teaching and disclosure review

The lesson defines standard form, a/b/c, monic, factors, zero-product reasoning,
principal square root, D, both signs, repeated-root counting, S, L/R and V.
The worked equation x^2+6x=7 differs from all twelve problem givens. Its roots
-7 and 1 and original checks live inside the independently closed Answer and
Solution disclosures. Hint gives a method direction. Required local blocks
start/terms/rule/condition/worked/errors/practice/summary are present.

Each prompt identifies the next operation or displayed form; titles do not
name methods or outcomes. Wrong feedback remains attached to semantic IDs.
History explanations use ordinary prose rather than raw TeX. All factor
cross terms, denominator clearing, signed discriminants, formula substitutions,
branch solutions and exact original checks are explained after their decisions.
The repeated root is listed once. No unknown x is divided away. The no-real-root
conclusion is proved for all real inputs, not inferred from failed samples.

Primary source URLs and access date are recorded in DESIGN.md and provenance.
References are OpenStax Intermediate Algebra 2e sections 6.5, 9.2 and 9.3.
All exercises and teaching prose are original project authoring.

## Executed headless checks

All commands below returned exit status 0 from the Paths root:

~~~sh
b/sorter --inspect-documents --documents content/authoring/production/wave03/algebra/authoring/documents > build/production/wave03/algebra/inspection.json
b/paths_learning_document_tests --question-batch content/authoring/production/wave03/algebra/authoring/documents > build/production/wave03/algebra/routes.json
b/paths_learning_document_tests --family-lessons content/authoring/production/wave03/algebra/authoring/documents > build/production/wave03/algebra/lessons.json
b/paths_learning_document_tests --question-batch build/production/wave03/algebra/markdown-edit/documents > build/production/wave03/algebra/markdown-edit/routes.json
PYTHONPATH=tools python3 -B content/authoring/production/wave03/algebra/certificate_tests.py --routes build/production/wave03/algebra/routes.json --edited-routes build/production/wave03/algebra/markdown-edit/routes.json > build/production/wave03/algebra/mathematics.json
~~~

The model reports 12 routes, 100 wrong choices, successful save replay and zero
windows. The disclosure gate reports one reading, twelve questions, three
closed disclosures and three independently opened worked disclosures.
export_learning.Target.inspect(documents=...) and
export_learning.provenance(author, report["entities"]) also passed, covering
the one lesson and all twelve question IDs; evidence is provenance.json.

The scratch chapter is an actual file copy with precisely two Markdown edits:
q01 step10's prompt and its choice12 feedback. It was compiled by the real model.
The checker verifies identical mathematical certificates and, after restoring
those two JSON fields, equality of the entire compiled report. No source
template substitutions or simulated edits stand in for this proof.

executable-check.json pins the readiness record and all three recorded Release
binary hashes, plus the reused exact-number and exporter helper hashes.
Only sorter inspection and the data-only model gates ran; the UI binary was
hashed but not run. production.json covers exact source/evidence bytes and
excludes itself from its own evidence inventory.

## Remaining limits and handoff

No known mathematical, source-format or prescribed-check blocker remains.
The coordinator still owns independent review and serial publication. Prepared
choice success does not establish independent written solving or learner
mastery. Visual appearance and learner effectiveness remain unobserved.
The finite arithmetic checker deliberately refuses unsupported expressions
instead of claiming general symbolic coverage.

All writes stay inside this subject's authorized Wave 03 source/evidence
folders. No frozen Wave 01/02 sources, other subjects, runtime, shared tools,
3D assets, active stores or personal saves were altered. No builds, publication,
windows, fonts/ImGui, clipboard, images, screenshots or captures were performed.
No workers, new tasks or callback messages were sent for this Wave 03 assignment.
Stop after delivery and retain the source uncommitted and frozen for review.
