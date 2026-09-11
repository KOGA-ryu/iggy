# Wave 04: one trial using textbook problem sources

**Completed and published.** This brief records the finished assignment.
build/production/wave04/completion.json binds all four accepted chapters and
the 701-question live library. Subject sources are frozen; do not restart this
assignment from the historical instructions below. Both interrupted writers
completed their explicitly authorized resumptions. No later wave is assigned.

The user authorized this finite trial on 2026-09-10. The existing algebra,
trig, calc and linear tasks each deliver one canonical lesson and 12 complete
problems. Keep current model and reasoning settings. All earlier authoring,
standby and cross-review jobs are closed. Wave 03 is reviewed, published and
frozen; its completion record pins the live 653-question baseline.
Only Wave 04 was assigned by this contract. Do not create workers.

On resumption, preserve the existing draft and consult
build/production/wave04/review-queue.json and interruption.json for its actual
state. A usage-limit stop is not a completed delivery. Finish only missing
authoring/checks in this same assignment; never revive an old-wave closeout.

Read assignments.json, SOURCES.json and your subject BRIEF.md. Also read
docs/LEARNING_DOCUMENTS.md, docs/QUESTION_PRACTICE_FORMAT.md,
docs/templates/QUESTION_FAMILY.md and docs/SUBJECT_PILOT_REVIEW.md. Use the
accepted native textbook, choices.v1, lesson.v2 and the existing exporter.
The coordinator owns routine QA and publication. Do not ask the user to solve
or visually inspect your questions. Finish writing and checks, not a skeleton.

## One source and a traceable adaptation

Maintain DESIGN.md, cases.json, certificate_tests.py, review.md,
authoring/authoring.json and authoring/documents/chapter.paths.md in your own
folder. No generator.py, copied common parser, CAS, new renderer, runtime mode
or shared-tool edits. Reuse earlier exact helpers read-only when their contract
fits; hash any dependencies. New checker logic is only for the bounded family.

The registry pins downloaded primary sources and licenses. Read the local text;
PDF text extraction is allowed, images and rendering are forbidden. Restrict
seeds to your approved pool. Every question must map to an actual exercise or
worked prompt, with at least six distinct seed prompts/subparts across the 12.
Adaptation may change numbers, conditions or the requested task; record what
changed. Source-assisted does not mean merely citing a theorem while inventing
unrelated questions. Do not reuse a graph-dependent prompt without replacing
all its givens explicitly in text. Do not infer lost superscripts or matrix
entries from broken extraction: check a second text representation or choose
another clear seed. Never take a screenshot to resolve it.

Add a source object to every cases.json case with source_id, exact locator
(section, exercise and subpart), original mathematical givens in unambiguous
text/TeX, adaptation_kind and changes. DESIGN.md maps the worked example too.
Use existing authoring metadata kind adapted, exact source URL/revision,
attribution and reuse fields covering every content ID. Include the chosen CC
license URL and an adaptation notice. Keep that credit readable in the lesson's
public summary block. The content license applies to the adapted lesson and
questions; do not make claims about licensing the surrounding application.
Preserve third-party credit required by SOURCES.json. Do not copy NC manuals,
externally credited puzzles, figures or photographs. Write our own teaching,
choices and misconception feedback; independently solve the original problem.

## Complete teaching and solving

Use four introductory full solves, four deliberate variations and four mixed
or exceptional solves. At least two final-group problems choose a method and
carry it through. Keep original givens visible throughout the same workflow.
Normally four to eight meaningful decisions; a genuinely short exception may
take three. Do not pad a problem merely to hit a step count.

Each decision has a precise goal, three compact symbolic choices, one
goal-correct key, reached working, a readable @why and specific wrong feedback
showing the actual arithmetic or failed condition. TeX belongs in supported
display fields. Preserve semantic option IDs with their feedback. Balance the
first correct position four times each; vary later positions deterministically.
Neutral titles must not reveal answers or methods.

Check mathematical truth AND the explicitly requested operation/form. A true
alternative missing that goal needs accurate feedback explaining the mismatch.
Never use reordered sets, equivalent fractions or the same constant family as
competing answers. Preserve original domains after cancellation. Prove solution
completeness; point samples do not establish an identity or exhaustive roots.

The lesson has local block suffixes start, terms, rule, condition, worked,
errors, practice and summary. Define every used symbol, prerequisite and rule
condition plainly. Supply a distinct worked example with actual arithmetic,
an original-given check and separately closed Hint, Answer and Solution;
include Proof when useful. Do not leak its answer in public blocks. Every
question @reads this lesson; its 12 ordered @practice links match assignment
order. No learner-facing production versions or testing commentary.
choices.v1 has no @definitions, @teaching or @hint directives. This does not
implement four generic runtime guidance modes or change the frozen six-role gate.

## Checks and measured handoff

DESIGN.md defines finite inputs, expression domain and deliberate coverage
before cases. certificate_tests.py accepts --routes PATH and checks actual
compiled givens, every option, every reached state and the final original-given
result independently of keys or authored expected strings. Reject false keys,
false after-states, semantic duplicates, invalid domains and true-but-wrong-goal
forms. Include valid alternative-form controls. State the checker's limits.
The source answer key may cross-check results, but cannot be the oracle.

In a scratch source copy, change one real Markdown prompt and one wrong-feedback
passage. Compile and prove that only the corresponding fields changed, with
mathematics stable. Use these existing headless gates from the Paths root,
substituting your assignment's SUBJECT:

```sh
b/sorter --inspect-documents --documents content/authoring/production/wave04/SUBJECT/authoring/documents > build/production/wave04/SUBJECT/inspection.json
b/paths_learning_document_tests --question-batch content/authoring/production/wave04/SUBJECT/authoring/documents > build/production/wave04/SUBJECT/routes.json
b/paths_learning_document_tests --family-lessons content/authoring/production/wave04/SUBJECT/authoring/documents > build/production/wave04/SUBJECT/lessons.json
PYTHONPATH=tools python3 -B content/authoring/production/wave04/SUBJECT/certificate_tests.py --routes build/production/wave04/SUBJECT/routes.json
```

Check exit statuses and exporter metadata through Target/provenance. Verify
binary hashes against build/production/wave01/build-ready.json. Do not rebuild,
publish, activate stores, write saves, open windows, initialize fonts/ImGui,
access clipboard, or take screenshots/captures/images. Keep work uncommitted.

Return build/production/wave04/SUBJECT/production.json, format
paths_depth_subject/version 1, wave wave04, subject, stage
ready_for_coordinator_review (or blocked with exact reason), published false,
source, package_id, reading_id, question_ids, questions, readings, steps,
wrong_choices, source_sha256, evidence_sha256, target_sha256, model_sha256,
inspection/routes/lessons paths and remaining concerns. Hash maps cover actual
source/evidence relative to their subject folders, excluding the receipt itself.
Hash reused helpers and source snapshots. Include source_questions=12,
distinct_seed_prompts, source_registry_sha256 and selected content_license.

Record observed timestamps for start, case design, draft ready and checks done
when available; do not invent elapsed times. Record source-reading difficulties,
rejected seeds and correction rounds. Do not estimate token/weekly savings.
The coordinator will report measurements and their limits. Freeze source at
handoff and return a normal final. No callbacks, pings or further jobs. The
coordinator captures and independently reviews exact bytes, then checks aggregate
import and saved progress before any serial publication. Stop after this family.
