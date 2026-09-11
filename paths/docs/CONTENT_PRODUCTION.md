# Content production

The user authorized four Terra production writers on 2026-09-10 and explicitly
removed solving every new problem from their responsibilities. The coordinator
owns routine mathematical, teaching, integration and regression QA. User input
is reserved for product direction and occasional assembled-product demonstrations.
No question-by-question approval or visual checklist blocks production.

This supersedes the earlier pilot-only scope and per-card user acceptance
language for the production assignments below. Historical pinned contracts
remain byte-for-byte unchanged so existing generators and evidence still work.
This changes ownership of QA; it does not label unobserved visual behavior or
unmeasured learning outcomes as verified.

## First production release

Wave 01, contract revision 2, has completed eight topic families. Each has one
canonical reading and teaching, practice and fresh-check groups of six
questions. All **144 new questions and eight readings in eight family
packages** are reviewed and published in the local Library. That release
contained 557 questions and 967 readings, up from the wave-start 413 and 959.
Exact subject release evidence is in
`build/production/wave01/{algebra,trigonometry,calculus,linear_algebra}/release.json`;
`build/production/wave01/completion.json` consolidates the final active inventory.

| Writer task | Families | Deliverable |
| --- | --- | --- |
| algebra | Signed linear equations; linear equations with brackets | 36 questions, two readings |
| trig | Sine equations in one turn; cosine equations in one turn | 36 questions, two readings |
| calc | Difference quotients; polynomial derivative rules | 36 questions, two readings |
| linear | Complete row operations; 2×2 determinants and invertibility | 36 questions, two readings |

Each subject keeps its existing task and separate source folder. The earlier
plan for 24 separately published set readings is
superseded. Six-question candidates remain internal gate evidence; the product
receives one assembled 18-question package per mathematical family. The exact family scopes, IDs and chapter bindings are in
[assignments.json](../content/authoring/production/wave01/assignments.json).
The [production ownership rules](../content/authoring/production/AGENTS.md) and
subject BRIEF.md files define writable paths and the delivery interface.
Dispatch and progress evidence live under `build/production/wave01/`. All four
existing tasks were dispatched and observed active on Terra High. Each writer
returns one completion/blocker handoff to this coordinator task, allowing review
to resume without another user prompt. No recurring monitor is installed.

The finishing pass kept published algebra and calculus sources frozen while
`trig` and `linear` completed their families. The `algebra` task independently
reviewed linear algebra, and `calc` independently reviewed trigonometry.
The coordinator checked the final deltas, source consolidation, actual
Markdown-edit regressions and import/save behavior before serial publication.
All four Wave 01 subject sources remain frozen at their published hashes.
Waves 02 through 04, described below, are also complete. All published sources
remain frozen. The latest completion is Wave 04's bounded trial using textbook
exercises, with the same four tasks and unchanged model/reasoning settings.

For an assigned independent review, reviewers may write only under the subject's
`build/production/wave01/SUBJECT/independent-review/` directory. They derive
answers from captured original givens, record exact reviewed package hashes,
and return findings to the coordinator. Changed bytes remain unreviewed until
the relevant delta is checked. Source owners make corrections; the coordinator
checks consolidation, integration and publication. This keeps review tied to
the release without duplicating maintained authoring code.

This is a defined production increment toward a viable demonstration. It is
not completion of all 1,261 original curriculum sections. A large catalogue
count alone is not evidence of complete curriculum coverage.

## Division of responsibility

**Writers** create original teaching, definitions, deliberate variations,
multiple-choice decisions, misconception feedback, bounded generators and
independent mathematical certificates. They verify every delivered route and
return immutable candidates. They fix ordinary failures without asking the
user to solve or approve their exercises. They do not publish or modify shared
runtime, tooling, reference packets, other subjects or 3D assets.

**The coordinator** reviews each new family’s mathematical assumptions,
certificate independence and explanation quality; performs aggregate catalogue,
disclosure, identity, import-limit and save checks; integrates accepted packages
serially; and records failures with source paths for the responsible writer.
Routine corrections and rechecks are already authorized. A failed candidate
stays outside the active Library while other accepted work can progress.

**The 3D asset worker** retains ownership of models, geometry and controls.
Production questions use the accepted native textbook and existing symbolic
controls. A future figure integration must use the existing registered model
and disclosure contracts; content writers do not invent substitute graphics.

**The user** chooses product direction and gives optional feedback on milestone
demonstrations. They are not the content test runner or the routine acceptance
gate. A qualified external educator should review representative families and
learner sessions before claims about teaching effectiveness are made; arranging
that review does not block this authorized content production.

## Consolidation is a release requirement

One maintained family has one canonical lesson, editable Markdown questions
and one final package. Generated families keep variation in recipe data and
reuse one subject generator; directly authored families need no generator.
Shared mathematical functions stay shared. Wave 02 uses ordinary importable
Markdown and the existing compiler/exporter, with subject arithmetic confined
to its certificate-test entry point and finite cases.
Do not maintain a copied generator,
checker or lesson per set, embed full Markdown/Python programs inside generator
strings, or add an alternative parser/renderer to satisfy a content assignment.

Wave 01 reused chapter_text(), role certificates, compiler, model replay and
exporter. Its three six-question candidate gates are historical staging
evidence. Final assembly fed all 18 ordinary questions and practice links into
that same chapter wrapper with one lesson. Wave 02 uses direct multi-step
Markdown, --question-batch and --family-lessons without the six-role staging
contract or a new assembly route.
The existing model check is now `--family-lessons FOLDER`: one common required-
block and independent-disclosure check serves pilots and production families.
Its old hard-coded four-pilot mode and both consumers were replaced. Source
prose and mathematics still require coordinator review; structural checks do
not establish their clarity or truth.
No parser of generated Markdown or additional wrapper format is needed.

At intake the coordinator checks both content and ownership: where each fact,
rule, lesson and generation route is maintained; which current implementation
it reuses; and whether a superseded source or unnecessary duplicate remains.
A batch is not accepted while it leaves competing maintained routes behind.
Historical immutable receipts and published IDs are retained as evidence and
for saved progress; they are not templates for a second live implementation.

Compact means fewer maintained concepts and repeated code. It does not mean
shortened definitions, generic feedback or worked examples without arithmetic.
The public Library should present a coherent family sequence, not a list of
internal production versions. Current work is summarized in WORKSTREAMS.md;
older checkpoint narratives are isolated in WORKSTREAM_HISTORY.md.

## Quality requirements that scale

Every family completes the existing [teaching brief](templates/QUESTION_FAMILY.md)
before generating its finite, explicit cases. Conditions and notation receive
plain-language definitions. Worked solutions show the actual arithmetic and
check the original problem. Each wrong option has one specific correction and
must be mathematically distinct for the requested goal.

The six existing roles provide progression within a set. Practice and fresh
sets must change meaningful features as well as numbers. Correct answer
positions are deterministic and balanced within each set, with different
permutations across sets. Worked-example answers stay in the existing closed
disclosures and must not leak through another public block.

The existing chapter assembler, mathematical certificate boundary, compiler,
question session, native textbook and exporter remain canonical. No second
parser or runtime answer policy is introduced. Shared integration code collects
the writers' ordinary candidates; it does not decide mathematical truth.

Required evidence distinguishes:

1. Original-given mathematics, excluded cases and plausible wrong options.
2. Actual compiled routes, feedback, retained working and saved progress.
3. Prose and teaching review by the coordinator, including source attribution.
4. Native rendering observations, when available, with who observed them.
5. Learner outcomes, which are not established by automated tests.

No screenshots, images, captures, native windows, font probes or ImGui contexts
are permitted in this workflow. Data-only projection and layout checks cover
what they can actually observe. Remaining visual uncertainty is recorded at
the product milestone without making the user inspect every question.

## Integration and release evidence

The baseline contains 31 document files totaling 369,231 bytes, 413 questions,
959 readings, 190 chapters and eight subjects. Existing runtime limits include
128 input documents, 2 MiB expanded input and 1,024 questions. Wave 01 fits the
question-count budget, but its actual document counts and bytes must pass the
real compiler before publication. The former 16-question-link cap prevented an 18-question family from remaining
in one lesson. That single compiler cap is now 32. The 18-question use case, the 32-link
boundary and atomic rejection of link 33 pass through the existing native
adapter and question owner. The rebuilt Release targets were used for all
eight published families. This is one existing production file with zero net C++
lines added, plus a focused test in the existing adapter-test target. Other import limits remain
unchanged. No parallel lesson topology is introduced to work around the cap.

Publication preserves every existing question stamp and published payload.
Each production package has a new reserved identity. All packages are checked
together in an isolated store before the coordinator activates them serially.
Failures identify the package, source location and reason. Repeated publication
must be unchanged; failed imports must leave the last good Library active.

The release record should report expected versus delivered families, questions
and readings, exact build/source identities, all failed or excluded cases,
automated outcomes and outstanding product concerns. Changes remain uncommitted
unless the user requests a commit.

## Curriculum depth before demonstration packaging

The user has redirected the next work toward depth. Wave 01 completes a bounded
starter batch; its eight families do not establish complete chapter or subject
coverage. Most exercises have one decision, with two decisions in repair tasks.
Prepared choices also do not yet implement the full four-level support contract
available in the bounded linear/matrix kernels.

Next, establish complete multi-step solving with definitions, applicable rules
and conditions, intermediate arithmetic, mistake-specific corrections and a
check of the original problem. Preserve symbolic multiple choice and separate
mathematical complexity from how much guidance is shown. Extend that shared
contract through deliberate variations, exceptional cases, method selection and
mixed problems. Track completion by chapter and skill. Existing published
question payloads and saves remain stable.

The user has authorized the four existing workers to build **Wave 02: 48 full
problems and four canonical lessons**, 12 problems per subject. The exact
[shared contract](../content/authoring/production/wave02/BRIEF.md) and
[assignments](../content/authoring/production/wave02/assignments.json) define
original givens, all intermediate decisions, goal-specific corrections and a
final mathematical check. Algebra and linear algebra include unique, empty and
infinite solution sets. Trigonometry extends to shifted/frequency equations,
complete interval solutions and general solutions. Calculus combines product
and chain rules with derivative evaluation and tangent-line construction.

The bounded source uses choices.v1 and lesson.v2 directly. Four introductory,
four deliberate practice and four mixed/exceptional problems form curriculum
progression; they do not claim four runtime support modes. The old six-role
gate is frozen, not relaxed to fit full routes. The existing --question-batch
and --family-lessons gates accept the new ordinary source, with subject-specific
independent arithmetic. Writers return source and exact evidence for coordinator
capture/review. They do not publish or ask the user to solve the questions.
Current dispatch evidence belongs under build/production/wave02/.

**Wave 02 is complete:** four independently reviewed and locally published
families, 48 full problems, 233 decisions, 466 specific wrong-choice corrections
and four canonical lessons. Each family contains 12 complete routes. All 699
options and 233 reached states received independent mathematical review;
definitions, explanations, wrong-choice feedback and worked examples were read.
The writers corrected the bounded wording findings. Algebra's existing
certificate now checks requested displayed form separately from affine truth;
nine reviewer failure probes reject and four valid alternative forms pass.
Exact source/compiled deltas preserve every other mathematical field.

All 557 earlier question stamps and document/package bytes remain unchanged.
The live Library contains 605 questions, 971 readings, 1,282 sections and 36
structured lessons in 43 documents (834,240 bytes). Serial import, repeated
import, failed-import recovery, saved-progress upgrade and the pure native
adapter pass. The Release binaries remain unchanged. This wave adds four
ordinary chapters, four package metadata files, four finite case files and
four subject certificates; it adds zero runtime C++ files/lines, generators,
parsers or renderers. Published sources are frozen and changes uncommitted.

`build/production/wave02/completion.json` binds the final active inventory,
source hashes and evidence. Each subject has a `release.json`; the current
`review-queue.json` records all four as published. Earlier candidates and
writer receipts remain historical intake evidence. Full curriculum coverage,
generic four-level support and learner outcomes remain unfinished.
Demonstration packaging stays deferred.

## Wave 03: complete and published

The user authorized another bounded four-subject increment on 2026-09-10.
The same algebra, trig, calc and linear tasks each delivered 12 complete problems
and one canonical lesson: quadratic equations; trigonometric identities with
retained domains; polynomial integration; and constructing/using 2 by 2 matrix
inverses. Their exact chapter bindings, new IDs, allowed paths and evidence
contract are in [assignments.json](../content/authoring/production/wave03/assignments.json)
and the [shared brief](../content/authoring/production/wave03/BRIEF.md).

One ordinary Markdown chapter per subject preserves the accepted native
textbook and symbolic choices. The contract explicitly checks the requested
operation/form separately from mathematical equivalence, preserves original
domains, and rejects duplicate root sets, identities or constant families.
No new generator, parser, runtime support mode or renderer is assigned.

The coordinator reviewed all 48 questions, four lessons, 237 decisions and
711 options, including 474 wrong-choice corrections. Independent arithmetic
ledgers check the original givens: quadratic discriminants and complete roots,
trigonometric identities by rational parameterization with retained domains,
polynomial primitives and ordered endpoints, and matrix inverses by elimination
with both multiplication orders and explicit singular witnesses. These bounded
checks accompany review of the compiled options, requested forms and prose;
they do not constitute a general mathematical proof engine.

Two matrix-lesson paragraphs now explicitly construct a null vector and
distinguish transpose from adjugate. That clarification changed no question
payload or lesson structure; fresh focused checks passed before acceptance.
The exact revised capture is recorded in the subject's coordinator review.

All four packages passed the combined compiler preflight, isolated publication,
repeat-import and failed-import recovery, saved-progress upgrade and pure native
textbook checks, then published serially. All 605 earlier question stamps and
document/package bytes remain unchanged, as do the Wave 01/02 source inventories.
After Wave 03 the library contained 653 questions, 975 readings, 1,286 sections and
40 structured lessons in 47 documents (1,049,318 bytes). Import limits and
Release binaries are unchanged. This increment adds four ordinary chapters,
four metadata files, four finite case files and four subject certificates,
with zero runtime C++ changes, new generators, parsers or renderers.

The [completion record](../build/production/wave03/completion.json) binds the
active generation, reviewed captures, source hashes and release evidence.
Published sources are frozen and changes remain uncommitted. Routine QA was
completed without user problem-by-problem checks, windows, screenshots, font
probes or clipboard access. Appearance and learner outcomes are unobserved.

## Completed Wave 04: trial using primary textbook exercises

The user authorized one trial to test whether sourcing exercises reduces the
work of producing full teaching cards. The four existing tasks completed
rational equations, equations using trig identities, substitution, and
span/independence/basis. Each delivered 12 complete problems and one lesson.
The [shared brief](../content/authoring/production/wave04/BRIEF.md),
[assignments](../content/authoring/production/wave04/assignments.json) and
[source registry](../content/authoring/production/wave04/SOURCES.json) form the
completed writer contract. All authoring, standby and review jobs are closed.

Pinned primary sources are Lippman and Rasmussen's Precalculus 2.3, the Active
Calculus 2024 workbook, and Hefferon's Linear Algebra fourth edition. The
selected main texts permit adaptation under their stated CC BY-SA licenses;
the registry records exact versions, URLs, source bytes, license evidence and
exclusions. Each case records an actual exercise/subpart, original givens and
changes, with at least six distinct seed prompts per subject. Existing exporter
metadata records adapted provenance; public lesson text carries attribution,
license and change notices. No new import syntax or runtime is needed.

All four chapters passed independent review and serial publication: **48
questions, four lessons, 276 decisions, 828 symbolic choices and 552 individual
wrong-choice corrections**. The coordinator read every lesson and question,
independently derived the original mathematics, reviewed the finite
certificates and checked bound compiler, route/save, disclosure, provenance
and real Markdown-edit evidence. The final chapters include the linear
notation definitions, concrete trig angle feedback and genuine method-choice
slots; the calculus checker separates mathematical equivalence from requested
bound order. No mathematical finding remains open for this finite batch.

The live library contains **701 questions, 979 readings, 1,290 sections and
44 structured lessons in 51 documents (1,293,858 bytes)**. All 653 earlier
question stamps, existing document/package bytes and Wave 01–03 source
inventories remain unchanged. Aggregate import, isolated-store rehearsals,
saved-progress upgrade, unchanged repeat import, rejected-import recovery and
final native data checks passed. The same Release binaries were used, with
no runtime code, parser, renderer or generator added. All four new source
inventories are frozen. The [completion](../build/production/wave04/completion.json)
binds captures, reviews, releases, source hashes and the final generation;
the [review queue](../build/production/wave04/review-queue.json) is closed.

The 48 problems use 42 distinct approved source prompts: eight algebra,
twelve trig, ten calculus and twelve linear algebra. The
[measurement record](../build/production/wave04/measurement.json) retains
observed phases, rejected seeds, extraction difficulties and correction rounds.
The [interruption history](../build/production/wave04/interruption.json) records
the original trig/calc usage-limit stops and their completed user-authorized
resumptions. All deliveries finished without changing model settings or using
a reset credit. Different topics, interruptions and coordinator work prevent
an isolated speed or cost comparison; no token or weekly-allowance saving is
claimed. No subsequent curriculum wave is assigned.

The reusable recipe capability supports four registered families as described
below. Other mathematics still requires its reviewed providers;
this does not assign another production wave.

## Recipe authoring

The common entry point is `tools/author_question_family.py`. It currently
supports `linear_balance_v1`, `sine_turn_v1`, `polynomial_derivative_v1`
and `row_operations_v1`,
using the existing reviewed teaching and native choices/textbook layout.
Work from the Paths root:

```sh
python3 -B tools/author_question_family.py init content/authoring/drafts/balance_next --family linear_balance_v1 --package balance_next
python3 -B tools/author_question_family.py check content/authoring/drafts/balance_next
python3 -B tools/author_question_family.py init content/authoring/drafts/sine_next --family sine_turn_v1 --package sine_next
python3 -B tools/author_question_family.py check content/authoring/drafts/sine_next
python3 -B tools/author_question_family.py init content/authoring/drafts/derivatives_next --family polynomial_derivative_v1 --package derivatives_next
python3 -B tools/author_question_family.py check content/authoring/drafts/derivatives_next
python3 -B tools/author_question_family.py init content/authoring/drafts/rows_next --family row_operations_v1 --package rows_next
python3 -B tools/author_question_family.py check content/authoring/drafts/rows_next
```

`init` requires a new directory. Its four editable files are:

| File | Writer responsibility |
| --- | --- |
| recipe.json | Package identity/title/version, chapter placement, source credit, bounded numerical sets and learning objectives |
| lesson.md.in | One shared lesson using the approved definitions, rules and independent worked disclosures |
| questions.paths.md.in | Prompts, explanations and specific corrections using named calculated fields |
| DESIGN.md | Learning claim, permitted cases, variation purpose and review limits |

The shared recipe fields are `format: paths_question_family`, `format_version: 1`,
`family`, `package`, `placement`, `source` and `parameters`. Package has id,
version and title; placement has subject/subject_title and chapter/chapter_title.
Source has kind, title, uri, revision, attribution and reuse. The registered
provider defines parameters. All use three named finite sets and six role
descriptions. The [linear example](../content/authoring/learning/linear_family/DESIGN.md)
derives signed linear cases from bounded coefficients and solutions. The
[sine example](../content/authoring/learning/sine_family/DESIGN.md) specifies a
height, coefficient sign, offset and known branch for each named role, with
strict limits that keep its feedback and questions meaningful.
The [polynomial derivative example](../content/authoring/learning/polynomial_family/DESIGN.md)
uses four bounded integer coefficients and a fixed evaluation point per role.
It reserves the independent lesson example and rejects cases without two
distinct named numerical distractors.
The [row-operation example](../content/authoring/learning/row_family/DESIGN.md)
uses bounded augmented rows and explicit multipliers where the question supplies
one. It checks unique solutions, whole-row targets, reversible arithmetic and
distinct misconceptions, including conditions that keep the written corrections
accurate. Its calculated elimination multiplier may be fractional.
No Python path or program is accepted as a recipe field.

Sine, calculus and row operations use the same named-case assembly: sets and
role metadata, case identities and repeated-question rejection live in the
shared runner.
Providers supply only mathematical input validation/construction and calculated
fields. The linear recipe retains its different grouped-seed mathematics.
Adding another named-case provider does not require copying those shared steps.

`check` generates stable question IDs from the registered family/version,
package namespace, role, set and original mathematics. It fills existing
Markdown templates, numbers all 18 questions, labels their sets, balances
first-answer positions across roles and sets while preserving feedback IDs,
and assembles one lesson with ordered practice links. It creates
standard metadata and source coverage automatically. The linear provider's
old standalone packaging and receipt implementation was removed. The sine
and calculus adapters call the frozen Wave 01 exact mathematical functions;
the row adapter uses the canonical batch matrix checkers and frozen calculated
corrections. None calls the old recipe, choice-ordering or packaging routes.
Imported mathematical code is included in the shared source-change checks
and verification hashes.

The actual document compiler, independent finite certificates, complete model
replay, wrong-choice checks, saved-work replay and shared disclosure gate must
pass. Invalid parameters and missing template fields identify the input file
and field/line. Corrupt mathematical keys or working are rejected even if the
document is otherwise playable. Source/tool/binary changes during checks also
reject. A failed run does not create an accepted package or activate the library.

Successful commands print a short handoff with counts, authoring path and
verification path. Detailed compiled content, mathematical evidence, source and
tool hashes, attribution and exact input snapshots remain in that immutable
verification directory. This keeps repeated tool output small without removing
review evidence. Generated output is ordinary authoring.json plus one
documents/chapter.paths.md; it contains no Python or new runtime syntax.

Edit the source Markdown or recipe and rerun the same command to produce an
updated candidate; no C++ rebuild is required. The existing exporter handles
editable app previews and coordinator publication. Wording changes preserve
question IDs but can change content stamps; unchanged save compatibility is
not inferred merely from an unchanged ID. Routine teaching review remains the
coordinator's responsibility. Visual quality and learning outcomes remain
separate observations. Neither checking nor initialization publishes content.

Only these four families' documented finite parameters are supported at this
checkpoint. The sine, calculus and row scaffolds deliberately start with reviewed
Wave 01 mathematics; publishing them unchanged would duplicate that practice.
New IDs or package names do not establish new mathematical coverage. The
examples remain unpublished. A number edit still needs review for purposeful
variation.

Adding another subject means registering its reviewed generator/certificates
and calculated fields with the same runner, while reusing the packaging and
checking path. It does not mean trusting answer keys or copying this linear
oracle into unrelated mathematics. The single targeted
`paths_question_family_tests` CTest entry covers all four providers; their tests
are maintained in `tests/question_family_tests.py`.

## Deferred demonstration packaging

The next product milestone combines the completed content with a short,
repeatable demonstration: open the Contents, enter an appropriate lesson,
solve with symbolic controls, recover from an error, consult a definition,
complete a problem and resume saved work. Existing 3D examples can demonstrate
the visual direction without claiming every new question has an interactive
model. Packaging, clean-start instructions and a known-limit list belong to
that milestone. Funding material must describe the actual completed slice and
roadmap honestly; it must not present all listed curriculum entries as finished.
