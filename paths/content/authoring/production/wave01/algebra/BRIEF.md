# Wave 01: Algebra production — contract revision 2

Deliver **36 new questions, two canonical readings and two family packages**.
Each family below has one maintained lesson and teaching, practice and
fresh_check question groups of six existing exercise roles. The earlier
six-reading delivery is superseded; six set candidates are staging evidence
only. Report contract_revision 2 and consolidate before final delivery.
Finish both families without stopping for a pilot approval or user testing.
If one family needs an unsupported capability, complete the other and return
the precise issue to the coordinator. Never fabricate a passing receipt.

### signed_balance: Signed linear equations

Chapter `worked_linear_practice` / **Worked linear practice**. Real ax+b=c with nonzero a; signed constants and coefficients, goal-sensitive equivalence, inverse operations, exact integer and fractional solutions. Reuse the reviewed linear-family generator/certificates where valid.

### distributive_linear: Linear equations with brackets

Chapter `topic_0003` / **Equations and Relations**. Real a(x+b)=c and a(x+b)+d=e with nonzero a and bounded integer original coefficients. Teach distribution to every term, inverse order, negative multipliers and exact fractions. Expand the original expression independently and check original substitution. Exclude variable denominators and equations requiring new runtime support.

## Writable files and identities

Your source boundary is this folder only:
`/Users/kogaryu/iggy3d/paths/content/authoring/production/wave01/algebra/`.
The coordinator owns this BRIEF.md. Maintain one DESIGN.md, recipe.json,
generate.py, certificate_tests.py and review.md for the subject, with actual
editable lesson.md.in and questions.paths.md.in files per family and a shared
certificate provider where needed. One generator handles both families and
all sets using recipe data and existing helpers. Do not embed whole Markdown
documents or generated Python provider code as strings in the generator.
Remove your own superseded implementation drafts once replaced; historical
immutable build receipts are evidence, not maintained source. No runtime or
shared-tool edits. Do not reduce teaching detail to reduce code size.

Use `build/production/wave01/algebra/` for generated inputs and evidence;
temporary scratch may live there. The existing shared candidate gate also
writes immutable output under `build/parallel-authoring/algebra/`, which
is allowed. Read but never overwrite old candidates or active/exported packages.

Reserve **publishable** package IDs `prod01_algebra_FAMILY`, version 1.
Its single canonical reading ID is `prod01_algebra_FAMILY_r`, shared by
all three set checks and all 18 questions. Staging-only six-question packages
retain `prod01_algebra_FAMILY_SET`; their question IDs may keep that
prefix plus short role/case suffixes. Package/source IDs must be <=64 characters;
question/reading IDs must be <=80. Changed original problems have distinct
stable identities. Do not reuse old pilot/linear-example IDs. Bind only to the
existing chapters above. Final titles should describe the mathematics and set
progression without production, version or wave labels in learner-facing prose.

## Format and teaching quality

Read docs/templates/QUESTION_FAMILY.md, docs/SUBJECT_PILOT_REVIEW.md,
docs/QUESTION_PRACTICE_FORMAT.md and docs/LEARNING_DOCUMENTS.md. Complete the
family brief in DESIGN.md for both families before generation. Use the current
reviewed subject pilot plus content/authoring/learning/linear_family as working
examples. Do not modify those references. Reuse the existing shared chapter
wrapper, reasoning_documents(), reasoning_certificate(), verify_role_content()
and check_authoring_pilot.check_assignment(). No new parser, screen or solver.

Every set contains exactly the existing roles: read_notation, worked_check,
choose_next_step, explain_step, repair_error, independent. The repair must have
two distinct decisions: first bad line, then a correction not already supplied.
Each question has original givens, explicit necessary domain, symbolic choices,
a single goal-correct key, specific feedback for both wrong choices, reached
working and a complete explanation. Titles/domains must not reveal the answer.
Define all symbols and prerequisites; give actual intermediate arithmetic and
check the original problem. A valid move missing the stated goal is not invalid.

Readings use lesson.v2 and required local blocks start, terms, rule, condition,
worked, errors, practice, summary. A separate worked example must differ from
all eighteen questions in the family. Its original given is public; Hint, Answer and Solution
are independently closed, with Proof when useful. Do not leak the same worked
answer into another public block. Compact choices, rich readable explanations.
No implementation-status prose in the learner-facing lesson.

Keep correct answer positions balanced over each six-question set: each of the
three positions is used twice on the first decision. Choose a different stable
permutation per family/set, not the same pattern everywhere. Shuffle only
presentation; semantic option IDs must remain attached to their key/feedback.
Preserve at least one conceptual variation in each fresh_check set beyond
number substitution. State exactly what changes and what remains invariant.
Do not claim transfer, retention or mastery has been empirically established.

## Verification and delivery interface

The authoritative shared build status and executable hashes are in
`build/production/wave01/build-ready.json`. When status is ready and its hashes
match the binaries, run the final gates; do not wait on an older message.

Browse primary textbooks for definitions/conditions and cite the actual sections
and access date in DESIGN.md/authoring metadata. Write original exercises; no
unattributed extraction. Use exact arithmetic where possible. Generator seeds
are not an answer oracle: derive independent checks from original givens,
including wrong options, excluded cases and completeness/uniqueness arguments.
Test false keys, false reached work, equivalent options and domain boundaries.
Use a finite explicit pool; test the entire declared pool or explicitly bound
what was sampled. Repair any failures before delivery; do not weaken checks.

Create this exact entry point, run from the Paths root:

```sh
PYTHONPATH=tools python3 -B content/authoring/production/wave01/algebra/generate.py --target b/sorter --model b/paths_learning_document_tests
PYTHONPATH=tools python3 -B content/authoring/production/wave01/algebra/certificate_tests.py
```

Default generate.py builds all six staging sets and both consolidated families. Adapt the working linear-family
build() pattern: stage each six-role source under your build directory, supply
an assignment with subject `algebra`, source folder, reserved package ID
and chapter/reading values, then call the existing check_assignment(). The
provider must certify original cases, not parse template answer keys. The
old packet() call must still pass all its pins. Do not call `--subject` to
validate your production sources: that CLI validates the historical pilot.

Write `build/production/wave01/algebra/production.json` with exactly these
required fields (additional evidence fields are welcome):

```json
{
  "format": "paths_production_subject",
  "format_version": 1,
  "wave": "wave01",
  "contract_revision": 2,
  "subject": "algebra",
  "accepted": true,
  "published": false,
  "families": [
    {"family": "FAMILY", "authoring": "/absolute/consolidated/family/authoring", "verification": "/absolute/consolidated/family/verification.json"}
  ],
  "candidates": [
    {"family": "FAMILY", "set": "teaching", "authoring": "/absolute/candidate/authoring", "verification": "/absolute/checks/verification.json"}
  ]
}
```

There must be six staging candidate rows pointing to the unmodified shared
check_assignment receipts, and **two family rows** for the actual deliverables.
Do not publish the staging sets individually. The current compiler supports up to 32
@practice links, so the 18-question family is supported. Use the exact
check_assignment receipt at checks / sha(encoded(result)) / verification.json;
never select an arbitrary older receipt with a glob. Consolidate at the existing
assembly boundary: supply batch.chapter_text() with the canonical family lesson
blocks once, @practice links to all 18 questions, and the three sets' ordinary
question directive strings in order. Do not parse emitted Markdown back into
another format or create another chapter wrapper. All question @read references
must use the single canonical family reading ID.

Each family authoring folder has normal authoring.json plus one document with
one lesson.v2 reading and 18 choices.v1 questions. Its provenance covers every
question and its reading. Inspect it with export.Target.inspect(documents=...),
check provenance with export.provenance(), replay all 18 using the existing model
--question-batch, and compare the compiled 18 with the concatenated original
sequence records and certificates using batch.verify_role_content(). Check 21
decisions, 42 wrong choices, unchanged semantic choices and balanced first-step
positions per set. Store these results and exact source/target/model hashes in
the family's verification.json. A shared six-question receipt by itself does
not prove the assembled family.

Keep one canonical, numerical worked example per family, with a real answer
and every required intermediate calculation in Solution. Reading content must
not fork by set or expose the worked answer in another public block. Keep all
three set templates tied to the same family lesson/template source. No previous
implementation, alternate generator or staged lesson should appear as another
maintained production route.

Run the ONE shared disclosure gate for each final family:

```sh
b/paths_learning_document_tests --family-lessons /absolute/family/authoring/documents
```

Record that output, source hashes and test commands/results in review.md and
family verification. Do not duplicate this gate in a subject implementation.
It checks structural disclosure, not the truth or clarity of prose. The final
family outputs must be immutable/content-addressed; corrected generation must
not overwrite the payload behind an earlier receipt. The coordinator
independently inspects certificates/prose and verifies the aggregate, limits,
old save retention and publication. No user visual acceptance is needed to
deliver. Stop after the revision-2 receipt is complete and report exact paths,
counts, reuse/consolidation performed and remaining concerns.

## Completion handoff

After producing the final receipt (or finishing all work unaffected by an exact
blocker), send one completion handoff to the assigning coordinator task
`01a07b52-505e-7e10-822b-17f39b7f2fd1` on host `local`, using
`send_message_to_thread` without a model/thinking override. Include subject,
production.json path, counts, test results and remaining concerns, and request
coordinator review and integration under the already-authorized Wave 01 scope.
Then give your normal final report. Do not send routine progress pings or ask
the user to perform QA. This completion return is authorized task coordination,
not authorization to start more writers.
