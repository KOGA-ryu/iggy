# Wave 03: the next four chapter families

The user authorized the next bounded production wave on 2026-09-10. The same
four existing writer tasks now build 12 original complete problems and one
canonical lesson each. This supersedes their completed Wave 02 standby/review
roles. Keep current model and reasoning settings. Finish the source and checks;
do not stop at a plan, skeleton, sample or request for user QA.

assignments.json reserves identities and chapter bindings. Your subject BRIEF.md
defines the mathematics and writable paths. The coordinator owns these briefs,
assignments, shared infrastructure, review and serial publication. Published
Wave 01/02 sources, earlier content, saves and 3D assets remain frozen.

## One maintained source, the accepted textbook

Read docs/LEARNING_DOCUMENTS.md, docs/QUESTION_PRACTICE_FORMAT.md,
docs/templates/QUESTION_FAMILY.md, docs/SUBJECT_PILOT_REVIEW.md and your reviewed
Wave 02 chapter. Use ordinary choices.v1 and lesson.v2 Markdown through the
existing compiler, session, native textbook and exporter. Do not copy a
generator, introduce a parser/renderer, create runtime support modes or weaken
the frozen six-role gate. choices.v1 has no @definitions, @teaching or @hint
directives and does not implement the linear/matrix four-level projection.

Keep one authoring/authoring.json and one authoring/documents/chapter.paths.md.
Beside authoring/, maintain DESIGN.md, finite cases.json, certificate_tests.py
and review.md. No generator.py or per-question scripts. Reuse existing exact
arithmetic helpers when their mathematical contract actually fits; do not
modify frozen helpers. New checking code is limited to this bounded subject's
mathematics and tests, not a general CAS, Markdown parser or receipt framework.

The lesson uses local block suffixes start, terms, rule, condition, worked,
errors, practice and summary. Define notation, prerequisites and each rule's
conditions in plain language with actual arithmetic. Give one distinct worked
example with independently closed Hint, Answer and Solution; include Proof
when useful. Keep its answer out of public blocks. All 12 questions @read this
lesson; its 12 ordered @practice links match the assignment's question order.
Keep learner-facing text free of production versions and implementation or
assessment-status commentary. Titles must not disclose the answer or method.

## Complete problems and precise decisions

Use four introductory full solves, four deliberate practice variations and
four mixed or exceptional solves. At least two final-group problems select a
method before carrying it through. Each question retains its original givens
and takes the learner through the entire requested problem and original-given
check in one workflow. Aim for four to eight meaningful decisions; three are
acceptable for a truly short exceptional case. Never pad a route to a count.

Each decision has a precise local goal, three compact symbolic choices, one
goal-correct key, reached working, ordinary-prose @why and misconception-specific
wrong feedback with the actual arithmetic. Native TeX belongs in supported
display fields and lesson blocks; history explanations stay readable prose.
Balance first-decision key positions four times each across the 12 questions;
vary later positions deterministically without detaching feedback from IDs.

Check truth AND the requested form or operation. A valid equivalent expression
may miss an explicit goal such as expanding, ordering, cancelling, or using a
specified method; feedback must explain that distinction accurately. Do not
offer equivalent spellings, reordered root sets, or the same antiderivative
family as competing answers. Preserve original domains after cancellation.
Check every earlier line, every option, all branches and completeness, not
only the final selected answer. Do not infer an identity from point samples.

## Independent evidence and handoff

Define a finite numeric/expression domain and deliberate coverage in DESIGN.md
before selecting the 12 cases. cases.json records stable IDs, original inputs,
domain, local goals and groups. Check the actual compiled givens/domain against
these inputs. Derive certificates independently from original mathematics;
authored keys, @after strings and copied expected labels are not an oracle.
certificate_tests.py accepts --routes PATH and reads the actual model's JSON.

Reject false keys, false reached states, semantically equivalent duplicate
options with different spelling, invalid inputs/domains, and a true expression
that misses an explicitly requested form. Include valid alternative-form
controls so checks do not merely require one authored string. Report the
precise finite scope of these checks; do not claim a universal verifier.
In a scratch copy, edit a real Markdown prompt and a wrong-feedback passage;
compile and prove only the intended fields changed while mathematics stayed
the same. No test-only substitution for the actual Markdown edit.

Read primary textbook references for definitions and conditions; record exact
URLs and access dates in DESIGN.md and authoring provenance. Author original
exercises and prose. The existing corpus supplies chapter topology, not an
unchecked mathematical authority. Subject briefs give starting references.

Run from /Users/kogaryu/iggy3d/paths with your assigned SUBJECT:

```sh
mkdir -p build/production/wave03/SUBJECT
b/sorter --inspect-documents --documents content/authoring/production/wave03/SUBJECT/authoring/documents > build/production/wave03/SUBJECT/inspection.json
b/paths_learning_document_tests --question-batch content/authoring/production/wave03/SUBJECT/authoring/documents > build/production/wave03/SUBJECT/routes.json
b/paths_learning_document_tests --family-lessons content/authoring/production/wave03/SUBJECT/authoring/documents > build/production/wave03/SUBJECT/lessons.json
PYTHONPATH=tools python3 -B content/authoring/production/wave03/SUBJECT/certificate_tests.py --routes build/production/wave03/SUBJECT/routes.json
```

Check each exit status. Verify ordinary metadata with export_learning.Target
and export_learning.provenance. Check binary hashes against
build/production/wave01/build-ready.json before final evidence. Do not rebuild,
publish, activate a store, write saves, open windows, initialize fonts/ImGui,
access the clipboard or take any screenshot, screen capture or image. Use
headless data checks only. No user visual acceptance is required.

Write build/production/wave03/SUBJECT/production.json: format
paths_depth_subject, format_version 1, wave wave03, subject, stage
ready_for_coordinator_review (or blocked with exact evidence), published false,
source authoring path, package_id, reading_id, question_ids, questions, readings,
steps, wrong_choices, source_sha256, evidence_sha256, target_sha256, model_sha256,
inspection/routes/lessons paths, mathematical evidence and remaining concerns.
Hash maps use names relative to your subject source/evidence folders. Cover
Markdown, metadata, DESIGN.md, cases, checker, review.md and actual evidence;
exclude production.json from its own evidence map. Record reused helper hashes.

Freeze your source at handoff. Return counts, exact receipt path, checks and
concerns in your normal final response. The coordinator retrieves files and
status directly; do not send completion callbacks, routine pings or new tasks.
The coordinator owns independent review, requested corrections, aggregate and
save checks, and serial publication. Keep work uncommitted and stop after this
assigned family; routine QA does not require the user to solve the questions.
