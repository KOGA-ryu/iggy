# Wave 02: complete solving routes

The user explicitly asked the coordinator to send the four existing workers
back to building curriculum depth on 2026-09-10. This supersedes their Wave 01
standby and cross-review assignments. Finish the bounded subject assignment;
do not stop after planning, a skeleton, one sample or a request for user QA.

Deliver **12 complete original problems and one canonical lesson per subject**:
48 problems and four lessons across this wave. This completes a depth increment,
not an entire subject. The subject BRIEF.md and assignments.json reserve the
scope, ownership, package and identities. The coordinator owns those files.

## One complete problem

Each question begins with its actual original givens and carries the learner
through the whole requested calculation and final check in one choices.v1
workflow. Do not split each intermediate line into a separate question. Aim
for four to eight meaningful decisions; a genuinely shorter exceptional case
may use three. Never pad a solution with repeated arithmetic or artificial
choices to reach a count. Report an unavoidable longer route rather than
silently truncating it. The compiler allows at most 32 steps.

Each decision has a precise local goal, three mathematically distinct symbolic
choices, one goal-correct key, its reached working, a concise ordinary-prose
explanation, and specific correction text for each wrong choice. Explain the
applicable rule and its conditions, with the actual substituted arithmetic.
Show extended mathematics in native TeX @given/@choice/@after or lesson blocks;
@why also appears in history and should remain readable ordinary prose.
A mathematically valid alternative may fail the stated local goal; feedback
must explain that distinction instead of calling valid mathematics invalid.

All problems end with a justified result and a check against the original
givens. For empty or infinite solution sets, establish the relevant contradiction
or identity and completeness; do not substitute an invented unique answer.
Every earlier line and choice must be independently checked, not just the end.

Arrange four introductory full solves, four deliberate practice variations and
four mixed or exceptional full solves in the lesson's practice order. These
are curriculum groups, not new runtime support levels. Vary mathematical
structure and misconceptions, not only coefficients. At least two final-group
problems must require selecting a method before carrying it through. State
exactly which skills each case exercises in DESIGN.md.

Keep the question title/domain free of its answer or a revealed worked route.
Teach definitions and prerequisites in the canonical lesson and give any local
reminder needed in the current prompt without disclosing the selected answer.
Choices remain compact and mathematical. Balance first-decision correct
positions four times in each of the three positions across the 12 problems;
vary later positions deterministically. Do not shuffle feedback away from its
semantic option ID or introduce equivalent choices with different TeX spelling.

## Use the current format without another generation framework

Read docs/LEARNING_DOCUMENTS.md, docs/QUESTION_PRACTICE_FORMAT.md,
docs/templates/QUESTION_FAMILY.md, docs/SUBJECT_PILOT_REVIEW.md and the reviewed
Wave 01 subject lesson as references. The native textbook remains authoritative.
Use one lesson.v2 with local block suffixes start, terms, rule, condition,
worked, errors, practice and summary. Follow the current --family-lessons gate.
Use a distinct numerical worked example that matches none of your 12 questions.
Hint, Answer and Solution stay independently closed; include Proof where useful.
Do not expose the worked answer through another public block. Link every problem
with @read to this same canonical lesson and give that lesson all 12 @practice
links. Define the notation and explain every rule condition actually needed.

This finite depth batch is written directly in ordinary importable Markdown.
Keep learner prose, arithmetic, choices, keys and corrections editable there.
The maintained source is authoring/authoring.json plus
authoring/documents/chapter.paths.md under your assigned subject folder. The
ordinary chapter document is sufficient; there is no new document syntax,
template dialect or generator.py for this wave. Optional @include fragments
are justified only by actual repeated learner passages within the same source.
Reuse the existing importer/exporter and helpers; do not copy Wave 01's generator
or create per-question/per-group scripts. Published Wave 01 sources and all
earlier pinned examples remain unchanged.

The older six-role gate deliberately requires its independent exercise to be
one uncued choice. Do not force these full problems through that contract,
relabel them as six roles, or weaken the frozen gate. Use the existing compiler
and --question-batch, which already replay complete prepared-choice routes.
choices.v1 does not currently support @definitions, @teaching or @hint, or the
linear/matrix four-level support projection. Use its supported fields and the
linked textbook lesson. Do not claim that four-level support has been added.
An unsupported presentation need belongs in the coordinator handoff, not a new
runtime implementation in a writer's folder.

## Mathematical evidence and ready-to-review delivery

Maintain one DESIGN.md, one finite cases.json, one certificate_tests.py and one
review.md beside authoring/. cases.json lists the 12 stable identities, original
inputs, necessary domain, local goals and coverage groups; it is an authoring
audit input, not a runtime format. Define your bounded numeric domain before
choosing values. Every delivered case and every choice is checked. Do not claim
all combinations in a larger domain were tested unless they actually were.

certificate_tests.py reads the existing model's actual --question-batch JSON
using --routes PATH. Derive correct results from original inputs independently
of the authored @answer and @after text. Verify that those inputs match the
actual compiled givens/domain, and compare every reached state, choice and key
with the mathematical certificate. Reuse existing exact arithmetic and subject
helpers where valid. Keep new code to subject mathematics and tests; no copied
compiler, document parser, generic six-role checker, exporter or UI logic.
Reading the compiler's structured output does not require parsing Markdown.

Test rejection of a false key, false intermediate working, an equivalent
duplicate option and an invalid domain/case, as well as all 12 genuine cases.
At least one real Markdown prompt edit and one wrong-feedback edit in a scratch
copy must change the corresponding compiled fields while leaving mathematics
unchanged. Record exact inspected input and executable hashes and test results;
never label a failed or skipped check passed. Restore no files in another task's
ownership. Browse primary textbook sections for definitions and conditions;
record the exact references and access date in DESIGN.md and provenance. Author
original exercises and explanations.

Run from /Users/kogaryu/iggy3d/paths, replacing SUBJECT with your assigned key:

```sh
mkdir -p build/production/wave02/SUBJECT
b/sorter --inspect-documents --documents content/authoring/production/wave02/SUBJECT/authoring/documents > build/production/wave02/SUBJECT/inspection.json
b/paths_learning_document_tests --question-batch content/authoring/production/wave02/SUBJECT/authoring/documents > build/production/wave02/SUBJECT/routes.json
b/paths_learning_document_tests --family-lessons content/authoring/production/wave02/SUBJECT/authoring/documents > build/production/wave02/SUBJECT/lessons.json
PYTHONPATH=tools python3 -B content/authoring/production/wave02/SUBJECT/certificate_tests.py --routes build/production/wave02/SUBJECT/routes.json
```

Inspect each command's exit status before using its output. The three binaries
are ready Release builds recorded in build/production/wave01/build-ready.json;
verify their hashes before final evidence. Do not rebuild or modify them.
Use export_learning.Target.inspect() and export_learning.provenance() for the
ordinary authoring.json coverage check. Do not publish, export to an existing
release location, activate a store, alter saves, open a window, initialize
fonts/ImGui, or take any screenshot/capture/image. No user visual approval is
required for this source delivery.

Write build/production/wave02/SUBJECT/production.json with format
paths_depth_subject, format_version 1, wave wave02, subject, stage
ready_for_coordinator_review (or blocked with exact failed evidence),
published false, source authoring path, package_id, reading_id, question_ids,
questions, steps, wrong_choices, source_sha256 (a relative-file hash map covering
the delivered Markdown, metadata, cases, checker and DESIGN.md), target_sha256,
model_sha256, inspection/routes/lessons evidence paths, mathematical evidence,
and remaining concerns. Do not claim coordinator acceptance. Source remains
editable until handoff; after handoff freeze it until a correction is assigned.
The coordinator captures immutable candidate bytes once, checks the mathematics
and teaching, then checks aggregate import and saved progress before serial
publication. Writers need no separate transport or receipt framework.

After delivery, send one completion or exact-blocker handoff to coordinator task
01a07b52-505e-7e10-822b-17f39b7f2fd1, host local, using send_message_to_thread
without model/thinking overrides. Include production.json, counts, tests and
concerns. This callback is explicitly authorized coordination. Do not send
routine progress pings, create more workers, or ask the user to test the cards.
Keep all changes uncommitted and stop after your assigned depth family.
