# Paths workstreams

## Shared question-family authoring and finite probability

**Built and locally published; visual and teaching acceptance remain with the
user.** Linear and worked-matrix authoring now use `teaching_documents()` and
one chapter template for subject/chapter placement, lesson blocks, practice
links and questions. Their reading prose lives in family Markdown templates;
`choices_text()` writes their common choice/answer directives. The frozen
matrix format-1 route remains a live reproduction/upgrade consumer. All earlier
linear and matrix document, manifest and audit bytes remain identical.

The third-family proof is **Probability and Statistics → Finite probability:
count and compare**, with 12 questions and three readings in teaching order:
Count an event, Count its complement, Impossible or certain. The native textbook
provides definitions, a counting proposition with closed proof, an independent
letter-token example with separate help, complement/boundary explanations,
practice and summary. The questions use two steps with symbolic choices and the
existing `choices.v1` owner. They do not claim four support levels or a written
probability checker. Linear and matrix retain their existing modes.

The producer counts multiples by integer division. Its independent certificate
enumerates the original outcomes and sums exact equal weights. Before export,
the actual compiled givens, working states, choice labels and accepted IDs must
match that certificate. The shared model gate exercises supported and prepared
questions through their existing owners, including wrong feedback, save/reopen,
reordering, retained archives, additive publication and explicit Next. Source
fingerprints guard against edits during verification. The existing exporter
remains the only activation route.

The installed catalogue now has **377 questions, 953 readings, 190 chapters and
eight subjects**, exposed as **1,264 native textbook sections**. All 365 earlier
question records, 950 reading records, 189 chapters and eight subjects compare
unchanged. Local generation:
`30c769a578e8747432e4dd9915eb2b4e881d7cba19fa91a3bc8f0e9f8e12d7b9`.

Release `sorter`, the pure document-model executable and the pure textbook
adapter executable build. `paths_learning_document_tests` passes; all **21**
batch tests pass. The installed pilot passes **12 complete solving routes and
48 wrong-choice checks**. The full 36-question recipe pool passes exact
arithmetic checks. The published textbook adapter confirms all entries and
exercises are reachable, optional answers start closed and bookmarks survive
reordering/additions. No windows, images, screenshots, captures, ImGui contexts
or font probes were used. Actual typography, pointer use and teaching quality
still require the user's check.

Scope against the saved starting state: **zero runtime C++ changes and zero new
runtime files**; one existing authoring tool is +197/-47 lines (net +150), two
existing test files grow by 157 net lines, and six text/recipe authoring files
are added. No UI layout, native 3D asset, source-card, CMake or parent-repository
changes were made in this workstream. All changes remain uncommitted.

The reusable builder contract and commands are in
[QUESTION_BATCHES.md](QUESTION_BATCHES.md); the completion record is
`build/family-workflow-evidence/verification.json`. Manual check: launch
`b/sorter`, open the chapter above and try Exercise. **Gold Given** stays beside
**cyan Working/choices**; a wrong tile retains working, and **green completed
working** stays until Next. Open the token example's separate Answer/Solution
disclosures, then close/reopen once to confirm progress. The next candidate is
one additional subject family against this contract, after the user judges the
pilot's explanation depth.

## Adopt the native textbook screen for the whole Library

**Built; headless verification passed; visual acceptance remains with the user.**
The user rejected the previous hybrid layout. Library now calls the exact
`drawTextbook` used by `math_lab`, supplying content through `CorpusTextbook`.
The separate Library browser, Focus pages, standalone imported reader and
question/textbook split table are deleted, including their obsolete UI state.
There is one page-layout owner for Contents, Index, reading measure, spacing,
numbered blocks, Reading/Exercise, figures, text size and footer navigation.

The existing **950 readings and 365 questions** map to **1,261 native sections**.
Questions explicitly attached to a lesson stay in its Exercise area; the others
receive question sections in their original subject/chapter. Older native
matrix references retain their actual definitions and propositions. One
registered imported figure uses the existing model and native figure regions.
The catalogue inspection report is identical before and after this change.
No content package or question stamp was rewritten.

`Textbook` owns reading position, size, disclosures and bookmarks. Imported
bookmarks use stable identities and an explicit saved-row count: additions and
reordering preserve position, while truncated or duplicate rows fail without
mutating reading state. The native `math_lab` generation-based bookmarks retain
their existing format. Preview remaps the book by ID and resets stale disclosures.
`CorpusPractice` and `LayeredQuestionSession` retain selection, grading, attempts,
Undo and saved working. A completed question waits for explicit Next.

Release builds of **sorter and math_lab** pass, as do eight targeted model and
adapter CTest entries plus the published-catalogue adapter check. These cover
native navigation/figures, complete solving routes, wrong choices, all four
support levels, guidance evidence, save replay, Undo, independent disclosures,
references, Markdown reload and bookmark migration. Numerical page regions are
checked at 1440×860, 800×600 and 360×480. No ImGui contexts, fonts, windows,
screenshots or captures were initialized by these verification runs.

Tests tied to the deleted browser/focus/panel controls are retired; valid
question-model, notation-engine and sorter-input tests remain. The updated
notation and sorter-input binaries compile but were not run: pointer behavior,
actual text fit and appearance still require the user's visual confirmation.
Detailed evidence and the scoped delta are in
`build/textbook-adoption-evidence/verification.json`. Changes remain uncommitted.

Manual check: launch `b/sorter` and open Library. Compare the Contents tree,
Index, teal headings, A-/A+, Reading/Exercise areas and footer with `math_lab`.
Try **Algebra → Linear equations: a textbook companion → From equal values to
an unknown → Exercise**. Gold Given and cyan Working/choices should stay nearby;
wrong feedback should be coral, and completed working green until Next. Return
to Reading, scroll, then reopen to check the reading bookmark. Next candidate
after acceptance: apply the established explanation depth to the matrix reference.

Deferred project: [Standalone calculator and connected tutoring](CALCULATOR_TUTOR_PROJECT.md)
records the future independent calculator, Eigen backend, named-control guidance,
local app connection and reusable lesson bindings. It contains staged TODOs and
acceptance checks. This is planning only; implementation has not started and the
current question/audit queue is unchanged.

## A complete linear teaching sequence with specific corrections

**Built and locally published; visual and teaching acceptance await the user.**
Package `linear_teaching_sequence` version 1 adds eight questions and one textbook
overview under **Algebra → Linear equations: understand each move**. The sequence
covers a worked example, guided practice, explaining equality, choosing a useful
operation, repairing a division error, equivalent-line selection, a signed
equation and a fractional return-later exercise. Every card can be completed
with buttons. The two numerical practice cards retain the existing four support
levels; the six reasoning/transfer cards use the existing prepared-choice route.

Short teaching passages explain the current operation and actual numbers. Every
distractor has its own correction through optional `@feedback ID | prose`. A
valid alternative method is explicitly distinguished from the direct cancellation
requested by the method question. The overview states prerequisites, explains
the roles of the cards and distinguishes assisted completion from independent
evidence. Returning later is a suggested exercise, not a scheduled reminder or
mastery calculation.

`LearningDocuments` remains the compiler; `QuestionContentIO` decodes the optional
choice field; `LayeredQuestionSession` validates and projects it after rejection.
Exact linear/matrix checks still determine correctness. Prepared feedback comes
from the existing review projection, replacing the retry-only display in the
solver UI. The UI wraps prompts and displays wrong-response corrections in coral.
`CorpusPractice` retains all persistence and replay ownership. No alternate
grading, save schema, parser or runtime file was added.

Release `sorter`, `gallery`, `paths`, and the required model/content test binaries
build. Four targeted CTest entries pass: document/model, four-level sessions,
question-content loading and the shared batch suite. After strengthening the
independent test oracle to check the actual displayed givens and working, the
15-test batch suite passes again. The new sequence completes 16 answering routes
and 44 wrong-tile checks with save/reopen, catalogue reorder, supported Undo and
completion held until Next. Matrix correction reuse and live Markdown feedback
edits pass through the actual model. Twelve direct/include malformed-feedback
cases identify the original directive; JSON field and option-reorder cases pass.

The independent Fraction-based oracle checks actual givens, intermediate displays,
arithmetic choices and original substitution. The three conceptual keys have
explicit rationale/task checks; this is not an automatic verifier of arbitrary
mathematical prose. No learner retention or teaching-effectiveness study has
been performed. Human checks remain separate from functional test results.

The rebuilt app first loaded the complete old library unchanged. Publication
then added this chapter through the existing exporter. All 354 earlier question
records and 948 reading records remain identical. The library now has 362
questions, 949 readings, 188 chapters and eight subjects. Existing generated
packages and the earlier editable draft remain byte-identical. No personal save
was read or written; save checks used temporary fixtures.

Production C++ delta: **+50/-12 lines (net +38) across five existing files, zero
new runtime files**. Three test files are extended. Two original authoring files
and [one reusable authoring guide](LINEAR_TEACHING_SEQUENCE.md) are added. Existing
3D-worker changes are preserved. Everything remains uncommitted. Evidence:
`build/linear-teaching-evidence/verification.json`. No windows, screenshots,
captures, images or font probes were used.

Manual check: launch `b/sorter` and open the new chapter. On 01, choose 36 then
45: their coral corrections should differ and cyan Working should stay unchanged.
On 04, choosing Divide both sides by 4 should acknowledge a valid alternative
that does not directly cancel the added term. Try the repair and fewer-cue cards;
gold Given stays beside cyan Working, and green completion waits for Next.
Close and reopen once to check resume. Next candidate: use the user's feedback
to settle one explanation or distractor before generating more repetitions.

## Editable chapter drafts through the existing exporter

**Built; a 12-question linear draft is ready for the user's visual check.**
`export_learning.py draft SOURCE --output NEW_FOLDER` captures an authoring
package through the existing compiler, copies its entry/include closure and
prints absolute edit paths plus the shell-quoted live-preview command. It
preserves source attribution in an origin snapshot, without carrying forward a
generator audit or publication receipt. Existing drafts are never overwritten.
Source overlap, symlinks and destinations inside releases/stores are rejected.

The canonical transport owner remains `export_learning.py`; it reuses
`prepare_pack`, `Target`, capture, locking and staged directory writing.
`LearningDocuments`, `LearningDocumentPreview` and `CorpusPractice` retain all
compilation, reload, answer and session ownership. No competing parser or runtime
route was added, and no existing live route needed deletion.

The draft lives at `content/authoring/drafts/linear_practice`: six editable
Markdown documents with 12 questions, plus `draft.json` recording the original
source and attribution. It is an independent preview, not a publishable release.
Changes to teaching start separate in-memory revisions; reverting source restores
the matching preview attempt. Normal published progress is not read or written.

Release `sorter` and `paths_learning_document_tests` build. The targeted document
test passes, and the final shared batch suite passes all 13 tests. One new matrix
fixture initially supplied package version 1 for format 2; correcting that test
setup produced a passing rerun. An existing export/publish/reinstall regression
also passes. No native UI or font-rasterization test was run.

New checks cover both generated families, include closure, old-audit exclusion,
source attribution, shell quoting, edited-draft preservation, rejected source
locations, source/output boundaries, symlinks and capture consistency. The real
watcher/model verifies a saved Hint edit, retention on rejection, error recovery,
earlier choices/Undo restoration and checked completion held until Next. The
actual created linear draft also passes this probe using an isolated temporary
copy. Its on-disk source still exactly matches the generated originals.

All bytes in the active library store and both source releases remain unchanged.
The published catalogue is still 354 questions, 948 readings, 187 chapters and
8 subjects. Runtime C++ delta is zero; authoring Python changes +41/-1 lines
(net +40) in one existing file, with no new production code files. Two existing
test files gain 142 lines. Seven editable content/origin files add 1,248 lines.
Changes remain uncommitted. Evidence: `build/chapter-draft-evidence/verification.json`.
No windows, screenshots, captures, images or font probes were used.

Manual check: launch `b/sorter --documents
content/authoring/drafts/linear_practice/documents --watch-documents`, open
**Algebra → Worked linear practice → Positive integers** and Exercise 1. Edit
the first `@hint` sentence in `documents/positive_integer.paths.md` and save.
The cyan preview status should refresh, and purple Help → Hint should show the
new words. Gold Given and cyan Working/choices stay beside one another. Preview
attempts are session-only; launch normally to resume published progress.
Next candidate: use the draft to settle one wording/layout improvement, then
apply the accepted wording to the reusable template for future questions.

## Worked linear practice through the shared batch pipeline

**Built and locally published; generated appearance awaits the user's visual check.**
Package `linear_repetitions` version 1 adds 12 cards and six readings under
**Algebra → Worked linear practice**. There are two cards each for positive
integers, negative coefficients, negative offsets, negative answers, zero as
the answer, and fractions. Each has two balanced steps, neutral definitions,
separate hints, typeset arithmetic and substitution into the original equation.
Practice keeps symbolic choices and optional typing; completion waits for Next.

The existing `question_workflow.py` recipe owns linear generation and independent
arithmetic checks. One new Markdown template supplies the prose. The existing
batch producer selects a family through a table and shares its compiler, model
gate and exporter across both families. `LearningDocuments`,
`LayeredQuestionSession`, `CorpusPractice` and `export_learning.py` retain their
runtime ownership. A duplicate nested plain-equation formatter was removed;
the recipe's equation formatter now serves plain and typeset consumers. The
pure batch gate reads the compiler's canonical reached displays for either family.

Release `sorter`, `paths_learning_document_tests` and `paths_four_level_tests`
build. Four targeted CTest entries pass: document/model, four-level sessions,
original question workflow and the ten-test batch suite. After removing the
redundant matrix-only uniqueness check, the shared batch suite passes again.
The published cards pass 60 solving routes, 144 wrong-response checks and 384
help-disclosure checks. The tests cover all 36 bounded recipe instances, malformed
source, unknown fields, repeat publication, six-to-twelve-card extension and
save replay with completion, selection, drafts, help and Undo branches retained.

The library now contains 354 questions and 948 readings. All 342 earlier question
records and 942 reading records remain identical, including existing taxonomy
records. The earlier 25 linear runtime cards and both published matrix formats
reproduce byte for byte. No personal save was read or changed.

Runtime C++ delta: **zero files, zero lines**. Authoring Python changes total
+115/-36 lines (net +79 across two existing files); one 81-line Markdown template
was added. Tests and documentation use existing files. Concurrent P061 asset
documentation is preserved and is outside this workstream. Changes remain
uncommitted. Evidence: `build/linear-batch-evidence/verification.json`.
No windows, screenshots, captures, images or font probes were used.

Manual check: launch `b/sorter`, open **Algebra → Worked linear practice**, and
try a negative offset, zero and fractional case. Gold Given stays beside cyan
Working/choices. Purple Help should distinguish Terms, Hint, Next line and
Solution; Learn should show legible balanced calculations. Green completion
waits for Next. Close/reopen partway through Practice once to confirm your place.
Next candidate: an editable draft command that copies a generated chapter into
the live-preview folder, keeping immutable published content intact.

## Worked matrix batch from the approved reference

**Built and locally published; generated variants await the user's visual check.**
The user approved continuing with the reference format. Package
`matrix_repetitions` version 2 retains the original 12 cards and adds 12 worked
versions under **Linear Algebra → Worked matrix practice**: four integer, four
negative-number and four fractional exercises. They revisit the same systems
with readable titles, neutral Terms, directional Hint and detailed typeset Learn
explanations. Next line and Solution retain their existing disclosure behavior.

One new Markdown authoring template,
`content/authoring/learning/matrix_reference/question.paths.md.in`, owns the
reusable wording. The existing batch producer fills exact numeric/display fields
and emits ordinary learning documents. `LearningDocuments` still compiles them,
`LayeredQuestionSession` checks answers/disclosures, `CorpusPractice` owns saves,
and `export_learning.py` publishes. No competing runtime route was introduced.
The original formatter remains a live consumer for byte-stable retained cards;
package version and teaching-format version are separate. See
[batch commands and template fields](QUESTION_BATCHES.md).

Release `sorter` and `paths_learning_document_tests` build. Two selected CTest
entries pass: document/model regressions and the eight-test batch suite. The
published package completes 120 solving routes, 432 wrong-response checks and
528 disclosure checks. Independent arithmetic validates every matrix/choice;
template sign/fraction checks span the bounded 36-system pool. Frozen document
hashes protect version 1. Upgrade tests preserve completed and partial old work,
selection, draft, help, level and Undo branches; repeat and extension checks pass.

The active catalogue now contains 342 questions and 942 readings. Every one of
the earlier 330 question records and 939 reading records remains identical,
along with existing subjects/chapters. Actual version-1 → version-2 save replay
passes using temporary progress, without reading personal saves.

Runtime C++ delta: **zero files, zero lines**. The existing authoring producer
adds +95/-4 Python lines (net +91), and one 115-line Markdown template was added.
Tests and documentation use existing files. Changes remain uncommitted. Evidence:
`build/worked-batch-evidence/verification.json`. No windows, screenshots, captures,
images, native rendering harnesses or font probes were used.

Manual check: launch `b/sorter` normally. Open **Worked matrix practice** and try
one card from each group. Check gold Given beside cyan Working/choices, the
distinct purple Help disclosures, typeset fractions/matrices in Learn, and green
completion staying until Next. Close/reopen during Practice once to check saved
progress. Next candidate: carry this template-driven batch workflow to linear
equation practice through its existing exact checker.

## Editable reference: definitions, hints and worked fractions

**Built and headlessly verified; user approved applying it to the batch above.** One original
fractional-matrix reference now lives in
`content/authoring/learning/matrix_reference/documents/reference.paths.md`.
It has a neutral `lesson.v2` reading, a readable exercise title and three fully
explained `matrix.v1` steps. The [field contract](QUESTION_PRACTICE_FORMAT.md#editable-matrix-reference)
defines the reusable format and the gate before generating more cards.

`LearningDocuments` connects optional `@hint` to the existing step hint field.
`LayeredQuestionSession` separates Hint from worked teaching; the old
Hint-to-teaching projection is removed. Terms supplies definitions, Next line
one reached matrix, and Solution the full route with concise explanations.
Learn expands definitions and detailed typeset teaching. Practice retains
symbolic tiles and optional typing. The UI shows the existing catalogue title
above the level controls and reuses its current math-document renderer.
No checker, save schema, question schema, renderer or 3D asset model was added.

Release `sorter`, `paths_learning_document_tests` and `paths_four_level_tests`
build. Three selected CTest entries pass: documents, four-level sessions and the
six-test question-batch suite. Reference evidence covers 48 disclosures across
four levels, five solving routes, 18 wrong responses, Undo, hint/save replay,
live hint edits and eight direct/include source-error cases. Independent exact
arithmetic checks the givens, three reached matrices and all six distractors.
TeX delimiters/grouping pass structural checks; appearance remains unverified.

The existing exporter accepts the candidate in
`build/reference-card-evidence/export`; **it is not published**. The active
catalogue report remains byte-identical: 330 questions and 939 readings, including
all existing stamps. The earlier 12-question generator is unchanged.

Production C++ delta: +16/-5 lines, **net +11 across three existing files**, with
no new runtime files. Two source authoring files were added; tests and docs use
existing files. Changes remain uncommitted. Evidence:
`build/reference-card-evidence/verification.json`. No windows, screenshots,
captures, images, font probes or personal progress files were used.

Manual check: launch `b/sorter --documents
content/authoring/learning/matrix_reference/documents --watch-documents` and open
**Linear Algebra → Matrix reference → Fractional solutions**, then its question.
Check the gold Given beside cyan Working/choices. In purple Help, Terms, Hint,
Next line and Solution should reveal progressively different material. Close
help and select Learn to inspect the fractional calculations and matrices.
Completion remains green until Next. Preview attempts are session-only. Next
candidate: apply the visually accepted reference to a new batch version while
preserving every published question identity and stamp.

## Live authoring: save Markdown and refresh the open app

**Built and headlessly verified; user visual acceptance pending.** Launch
`b/sorter --documents content/write --watch-documents`. The explicit source
preview opens Library, watches document/include bytes and compiles settled edits
through `LearningDocuments`. Valid changes refresh the existing renderer and
invalid changes keep the last accepted catalogue with a coral source error.
The cyan preview status is visible in browsing, lessons and solving. Long errors
have a bounded scroll area so they do not consume the entire workspace.

`LearningDocumentPreview` owns capture/debounce inside the existing compiler
module. One shared directory enumerator replaces the old inline scan.
`CorpusPractice` remains the attempt/persistence adapter and reuses the original
question sessions. Exact ID/stamp matches retain their journals, drafts, levels,
Undo branches and completion. Changed questions start fresh at the selected level;
up to 32 earlier attempted revisions are retained in memory and restored when
their source returns. The owner rejects attachment to persistence-backed sessions.
Live preview neither loads nor writes personal progress nor publishes packages.

The UI remaps selected subjects, chapters, readings and bookmarks by stable ID.
It retains reading position where possible and text scale, closes removed
selections, refreshes lists and invalidates old lesson/figure bindings and input
buffers. The existing source-keyed math renderer handles new text and equations.
No renderer, mathematical checker, save schema or 3D asset model was added.

Release sorter, document/model, starter/save and UI targets build. Four selected
CTest entries pass: document/model regressions, starter/save regressions, batch
publication and the new data-only UI reconciliation entry. After compacting the
status/error presentation, the two directly affected entries pass again. The
reconciliation entry creates no ImGui context, font atlas or window; the native
rendering harness was not run. Five CLI cases verify valid source inspection,
required source selection, rejection of saved/store modes and invalid-source
diagnostics. The published catalogue report is byte-for-byte structurally
identical: 330 questions and 939 readings.

Production C++ delta: +160/-14 lines, **net +146 across eight existing files**.
No production files were added. Tests, build registration and documentation use
existing files. Changes remain uncommitted. Evidence:
`build/live-authoring-evidence/verification.json`. No windows, screenshots,
captures, images, font probes or personal progress were used.

Manual check: open **Linear Algebra → Matrix rows from documents**, then its
question in **2 Practice**. Change the `@step 10` prompt in the source Markdown
and save: the cyan preview number and prompt should update in place beside the
gold Given. A deliberately invalid `@after` should show a coral file/line error
while preserving the prior question; undo that edit to recover. Next candidate:
use this preview loop to finish one reference card's hint/solution separation,
typeset explanations and readable titles before expanding production.

## Checked question batches: generate, verify and publish

**Implemented and locally published; user visual acceptance pending.** The
[batch command](QUESTION_BATCHES.md) generates one bounded three-step matrix
family through existing `matrix.v1` and `lesson.v2` documents. Version 1 adds
12 questions, evenly split among integer, negative and fractional solutions,
and three readings under **Linear Algebra → Matrix repetitions**. Library now
has 330 questions and 939 readings. All 318 earlier question identities/stamps,
936 readings and existing subject/chapter records remain unchanged. Repeating
the production command reports `unchanged: true` for the same generation.

`LayeredQuestionSession` remains the mathematical owner; `LearningDocuments`
compiles the documents and `export_learning.py` owns publication. The new
authoring producer checks original givens independently with exact arithmetic,
then requires the built app and pure model to accept every question before
calling the existing exporter. No runtime checker, store or activation route
was duplicated. Practice's authoring projection now includes symbolic choices
and an optional blank; the existing 25 linear runtime cards reproduce unchanged.

Release sorter and both affected model targets build. Three selected CTest
entries pass, including ten direct/include matrix-error location regressions,
existing linear/matrix model and save coverage, and five batch tests for
arithmetic, export, publish, repeat, extension, rejection and stale-output
handling. The authoring suite's eight tests and runtime reproduction check also
pass. The published batch completes 60 solving routes, 216 wrong-choice checks,
Undo and partial/completed save replay. Matrix errors now carry the owner's
reason back to the offending given, reached matrix or numeric choice line.

Production C++ delta: +81/-38 lines, **net +43 across five existing files**;
no runtime source files added. One authoring producer, one Python test file and
one workflow document were added. Evidence:
`build/question-batch-evidence/verification.json`. Changes remain uncommitted;
no windows, screenshots, captures, images, font probes or personal saves were
used. The user's remaining check is the gold Given, cyan choices/Working,
purple Help and green completion until Next. The next candidate is a second
reviewed mathematical family through this same pipeline.

## Practice choices: less guidance with the same symbolic controls

**Automated model/save checks passed; user visual and interaction check pending.**
`2 Practice` now offers the existing linear and two-variable matrix answer tiles
with teaching behind Help. Optional typing lives in a collapsed **Type an answer
(optional)** section; an existing draft opens it on first display. Gold Given
and cyan Working remain adjacent, wrong choices preserve both working and draft,
and completion stays green until Next.

`LayeredQuestionSession` remains the sole answer/working owner. Its existing
`Choose` action now accepts Practice responses as well as Learn responses, with
the same exact checker, stable option IDs and revision/anchor guards. Correct
Practice tiles clear the completed step's draft. Submission action and level
already distinguish choices from typed blanks, and the existing journal retains
that distinction through Undo, branches and reopen. Written levels reject choices.
No parallel checker, new question stamp or persistence schema was added.

The new regression failed against the previous model. Release `sorter`,
`paths_four_level_tests`, `paths_learning_document_tests` and the updated native
UI test target compile. Both selected pure CTest entries pass: 125 complete
linear routes with 300 wrong-response checks, 80 matrix routes across 16 parameter
sets (10 with fractional answers), mixed choice/typed history, help disclosure,
stale-input rejection, Undo branches, reordered saves and retained completion.
The old typed Practice routes and v1/v2 progress checks remain covered. The native
UI test was compiled only; it was not run because its harness uses font atlases.
The rebuilt app also passes `--check-content --no-progress` without a native host.

Production delta: +13/-6 C++ lines, **net +7 across three existing files**, with
no new production files. Tests and documentation were updated in existing files.
Evidence: `build/practice-choice-evidence/verification.json`. Changes remain
uncommitted; no windows, captures, images, font probes or personal saves were used.

Manual check: launch `b/sorter`, open a Linear practice or Document matrix
question, and choose **2 Practice**. Use the **cyan symbolic tiles** beneath the
**gold question**; a wrong tile must retain working. **Purple Help** should open
teaching in place. Try Undo, close/reopen partway through, and confirm a **green
finished result** remains until Next. The next repair candidate is precise
source locations and reasons for rejected matrix-card steps.

## Audit repair: Library autosave recovery

`CorpusPractice` remains the owner of Library progress. It now creates an
exclusive temporary file for each write, preserving abandoned files instead of
letting the old fixed `.tmp` path disable saving. A short OS-managed lock
serializes app writers; original-byte checks retain another window's saved work.
Only failed-load validation remains permanently blocked. Write failures retain
the newest draft and retry after two seconds, and the existing app-close call
requests one immediate retry. The journal, question stamps and save format are
unchanged. [Recovery rules](CORPUS_STARTERS.md#canonical-owners-and-persistence) describe
the retained temporary files and persistent lock file.

The orphan regression failed against the original writer. Release `sorter`,
`paths_corpus_starter_tests` and `paths_four_level_tests` builds now pass, as do
both targeted CTest entries. They cover three orphan forms, first-save recovery,
lock contention, automatic and closing retries, latest-draft replay, repaired
destinations, competing writers, symbolic paths and invalid-load preservation.
Existing coverage retains 278 prepared routes and 100 four-level routes, with
1,034 wrong-response checks, frozen identities, archived working and v1/v2 replay.
Default startup and published-store inspection pass with the same 936 readings
and 318 question stamps as the preceding checkpoint.

Production C++ changes are +53/-16 lines (net +37) across three existing files;
no production source files were added. Tests and documents use existing files.
Changes remain uncommitted. No windows, images, captures, font probes or personal
progress files were used. Evidence: `build/autosave-recovery-evidence/verification.json`.
Manual close/reopen confirmation remains with the user. The remaining audit
candidate is precise source locations and reasons for invalid matrix steps.

## Audit repair: public lesson figure placement

`LearningDocuments` now rejects `@figure`, `@parameter` and `@caption` inside
any `lesson.v2` textbook block, including its hint, answer, solution and proof
sections. One compiler scope check closes the route that placed protected
figure metadata in the public lesson. Diagnostics identify the original file,
line and command, including included fragments, and explain that lesson figures
are always public. Public diagrams remain valid outside `@block`; scoped figure
disclosures are not implemented by this repair. The existing model and renderer
owners are unchanged. [Authoring rules](LEARNING_DOCUMENTS.md#a-readable-lesson)
record where the directives belong.

Release `sorter` and `paths_learning_document_tests` builds pass. The new
regression failed on the original importer, then passed with the scope check:
30 invalid placements, four valid public arrangements, original source locations
and atomic rejection. The same pure test retains its 64 matrix routes and
save/replay checks. The rebuilt app rejects a solution figure at its exact
directive, passes default startup validation, and imports the published store
with the identical catalogue: 936 readings and 318 unchanged question stamps.

Production change: two added C++ lines in one existing file; no new production
files. Tests and authoring documentation use existing files. Changes remain
uncommitted. No windows, images, captures, font probes or personal saves were
used. Evidence: `build/lesson-disclosure-evidence/verification.json`.
Autosave recovery is recorded above; precise matrix-step diagnostics remain.

## Source lesson 002: reviewed text into the existing textbook and solving game

The [source lesson workflow](SOURCE_LESSON_WORKFLOW.md) is implemented and its
first package, `source_002_textbook` version 1, is published in the normal
`b/learning-store`. Library has **936 readings and 318 questions**. All prior
316 Library question identities and stamps remain unchanged. The lesson reuses
the current Claude 002 source, existing 13-step guided route and row-operation
definitions. It adds nine textbook blocks and an explicitly separate four-level
2x2 reduction exercise; the full polynomial problem remains a prepared route.

`LearningDocuments` is the compiler, `BookBlock`/`bookLessonView` owns the shared
reading structure and redaction, and both applications use `drawBookBlock` from
one `paths_textbook_ui` library. Existing mathematical/checking and progress
owners are unchanged. The producer uses the existing parser's public lossless
audit with explicit reviewed selections and SHA-256 pins. Changed source,
prepared route, definitions, presentation or template inputs fail before output
installation. Unmapped source files and learner Attempt/Run fields stay outside
runtime content. The raw page and full audit remain in authoring evidence.

Release sorter and math_lab builds pass. The document model gate passes its
existing 64 matrix routes plus thirteen new malformed textbook-block cases;
the native textbook model passes 513 assertions. Source integration proves all
13 prepared steps and 39 distractors, all four matrix levels, independent exact
fraction arithmetic and the determinant, seven stale/draft/version rejections,
deterministic output, actual publisher/startup integration, tamper rejection,
saved drafts and retained Undo branches. The delivered default store also passes
the old-save regression and all new source routes. No images, font probes,
windows or personal progress files were used. The user accepted the format
before requesting the repository audit.

Production C++ is +207/-88 lines (net +119) across nine files, including one new
40-line shared reading-data header. One 145-line Python producer was added;
the existing publisher, answer kernels and 3D assets were preserved. Changes
remain uncommitted. Evidence is `build/source-lesson-evidence/verification.json`.
The next candidate is one more reviewed source card using this same format.

## Current allocation: full textbook and learning application

The user clarified that this worker owns the entire textbook and application
learning experience. Contents, lessons, definitions, formatting, navigation,
questions, support levels, checks, progress, the content pipeline and figure
integration belong together. The other worker is strictly responsible for 3D
assets/models. [AGENTS.md](../AGENTS.md#current-work-allocation) is the current
allocation; this supersedes the earlier equation-only assignment. Existing
mathematical owners and state boundaries remain in force.

The first complete Claude-card lesson adaptation is recorded above: readable
teaching and definitions plus a checked solving route in the established format.
Ownership of the wider product does not start the separate lesson batch queue.

## Equation solving: shared textbook presentation standard

The user selected the existing researched textbook formatting and
interactive models as the reference for equation solving. Source inspection
traced typed reading blocks and disclosures through `TextbookUi`, the shared
`LessonSpread`, and the row-plane, affine-system and object figure bindings.
`NativeMath` is already shared; prepared question help has a limited connection
to neutral textbook material. Four-level textbook references and figures bound
to solving attempts remain future integration work.

The textbook also has board bindings for source cards 001, 004, 018, 031, 044
and 059, with explicit distinctions between printed data and chosen examples.
The earlier two-card adaptation count referred only to regular question packs
002/013. Board examples and invariant checks do not certify every source claim.

The adopted [question presentation contract](QUESTION_PRACTICE_FORMAT.md#shared-textbook-presentation-standard)
records layout, disclosure and ownership. The textbook worker now owns teaching,
formatting, playable questions, four support levels, judgments, saved work and
figure integration. The asset worker supplies 3D models; the existing code
owners still connect through explicit bindings. Camera/reading actions cannot grade an
answer, and an illustrative figure cannot silently expose a withheld solution.

Four question/export/workstream documents record the decision, and the earlier
source-card report now distinguishes regular packs from board bindings.
Production code, cards and the other worker's implementation are unchanged;
no build or visual test was needed for this source inspection. Changes remain uncommitted. The
next candidate is one Claude equation-card mapping into the existing document
pipeline using this standard and a supported mathematical checker.

## Learning exports: authored chapter into a published library

[LEARNING_EXPORTS.md](LEARNING_EXPORTS.md) is implemented for authored document
packages on macOS/Linux. `tools/export_learning.py publish` captures a chapter,
asks the app to validate it, exports the exact include closure with provenance,
and activates a complete immutable library generation. The first package,
`matrix_foundations` version 1, is published in `b/learning-store`; a normal
sorter launch discovers it. Library now has 935 readings and 316 questions.

`LearningDocuments` remains the compiler and import authority. It reports exact
file hashes, declarations and canonical question-stamp digests, and verifies
published generations before progress loads. The Python publisher owns locking,
transport and atomic activation. Every existing question ID/stamp is retained;
renamed files and reading-only updates remain compatible. Existing direct folder
import is a live authoring route through the same compiler. No competing math,
UI or save route was added or replaced. OpenSSL Crypto supplies SHA-256.

Release sorter, gallery and paths builds pass. Three targeted CTest entries pass,
including 16 publication regressions: exact/reproducible export, unchanged
activation, source edits during export, diagnostics, bad mathematics, unsupported
capabilities, provenance coverage, tampering, forged claims, cross-package links,
collisions, combined capacity, symlinks, frozen-content removal/change rejection,
interrupted activation and competing publishers. The published reader restores
an earlier draft, help exposure and Undo branches without rewriting its save.
Twelve actual headless UI routes solve the exported matrix in four modes at
1440×860, 800×600 and 360×480, with no notation fallback. Existing document gates
also pass: 64 matrix model routes and 30 document UI routes. The delivered store
passed default-startup inspection and a fresh saved-draft/answer regression.

Production C++ is +183/-33 lines (net +150) across three existing files. One
497-line Python publisher was added. The new authoring package has a metadata
file, a chapter and a shared include; one new Python test and two existing C++
tests cover the handoff. This is a new capability, with no new production C++
files. Existing source documents/cards/corpus and other owned areas are retained;
concurrent P053 math-object/CMake/documentation changes are preserved. Evidence:
`build/learning-export-evidence/verification.json`.

Changes remain uncommitted. No window, screenshot or capture was used. Visual
acceptance is pending: open the blue question link in **Matrix foundations: two
equations**, check gold Given beside cyan Working, then the green `x=2, y=3`
result waiting for Next. The next candidate is a Paths authoring adapter consuming
immutable source bytes and the existing parser's audit to emit this same format.
Prose-to-question generation, arbitrary new answer checkers, archives, live reload,
general save migration and rollback after play are outside this checkpoint.

## Shared linked-plot construction

The user approved consolidating sampled plots after the inspector migration.
`SnapshotBuilder::linkedPlot` now creates a plot with its first sampled series,
scrubbing binding and explicit mathematical marker. Forty construction sites
across 17 families use this shared call, including Function, Surface, Harmonics,
Oscillator, VectorField, Flux, Spherical, PSD, Norm, Curve, Lathe, Patch, Membrane,
Truss, Simplex, Distance and QR. Existing comparison-series calls, domain bounds,
read-only states and special/discrete traces remain intact. No new numerical
formula, renderer behavior, input route or public asset identifier is introduced.

The before/after Release traces are byte-identical across all 35 assets: 5,678
states, 8,157 plots and 1,374,426 sampled points. The trace checks presets,
discrete choices, continuous-control bounds and playback, recording every point,
marker, label, colour, series flag and scrub binding. The persistent plot-contract
suite covers 370 states, 499 plots and 81,884 samples, checking capacities, finite
values and parameter ownership. Release builds of `math_lab` and `sorter`
passed, along with all 19 selected CTest checks (eight CPU suites and eleven
text-only CLI cases). The installed 44,250,074-byte plot trace also matches the
original Release trace byte-for-byte.

One production file changes, with no new production files and 12 fewer lines.
Changes stay uncommitted. All verification is CPU/text only; no images, native
windows, screenshots, captures or font probes are used. Vector and projection
construction is the next shared-component candidate.

## Shared math asset control bindings

The user approved finishing the inspector consolidation across the library.
All 409 parameters in 35 assets now declare static editor bindings beside their
limits and defaults in `MathParameterSpec::control`. This checkpoint migrates the
remaining 356 parameters, including 64 coordinate tuples and 88 fields filtered
by point/mode selection. The inspector's parallel metadata, tuple and selected-
range registries and their fallback branches are deleted. Playback continues to
use the registry's constexpr lookup. Existing numerical predicates, dynamic
ranges, semantic actions, calculations and geometry are unchanged.

The expanded before/after trace is byte-identical across 3,592 sampled states:
control labels/groups/rows, parameter availability, presets, discrete selections,
playback, preset titles, group-open defaults, changed counts and group-reset
membership. The trace contains 4,836,355 bytes. The installed layout suite passed
157,338 assertions across 6,480 layouts and 128 object/layer states. Binding
validity checks now cover the complete registry without a legacy exemption.
Release builds of `math_lab` and `sorter` passed. All 19 selected CTest
checks passed (nine CPU suites and ten text-only CLI cases), as did the installed
before/after parity comparison. Verification opened no native windows and
produced no screenshots, captures or font probes.

Three production files change, with no new production files and 56 fewer lines.
No images or windows are used. Changes stay uncommitted; textbook integration,
source cards and learner state remain with their existing owners. The linked-plot
consolidation is recorded in the checkpoint above.

## P064: QR and least-squares assets

The user approved building the next shared family from the six category
checklists. [QR and Least Squares Lab](P064_QR_LEAST_SQUARES.md) is the 35th
object, with four layers, seven examples and seventeen controls. It connects
orthonormal frames, factor reconstruction, closest fits and coefficient-space
null families through a fixed real 3x2 kernel. Checklist notes record partial
chapter coverage; chapter integration and broader matrix sizes remain open.

This checkpoint is one asset family. Existing source cards and learner state
remain untouched, and textbook integration stays with its owner. Changes are
uncommitted; only CPU/text checks are used, with visual review by the user.

## P063: polar decomposition assets

The user authorized continuing the math assets after Distance Geometry Lab.
[Polar Decomposition Lab](P063_POLAR_DECOMPOSITION.md), based on card 089, is
the 34th object. Four layers connect matrix deformation, positive stretch,
rotation/reflection, inverse-transpose iteration and singular extensions.
A marked block and tetrahedron share six complete examples. The packet records
the numerical, geometry and verification contracts.

This checkpoint is one asset family. Textbook integration remains with its
current owner. Source cards and learner state stay read-only. Work remains
uncommitted, with CPU/text checks only and visual review left to the user.

## P062: distance geometry assets

The user authorized continuing the math assets after the simplex.
[Distance Geometry Lab](P062_DISTANCE_GEOMETRY.md), based on card 099, is the
33rd object. Four layers connect six lengths, the distance matrix, reconstructed
coordinates, reflection, rank, feasibility and convex-cone mixing. Six examples
include valid faces that cannot assemble into a tetrahedron and two line
configurations whose mixture needs a plane. The packet records the bounded
numerical contract and verification.

This is one asset family. Textbook integration remains with its current owner;
source cards and learner state are unchanged. Work stays uncommitted and checks
use CPU/text only, with no images or windows.

## P061: probability simplex assets

The user approved [Probability Simplex Lab](P061_PROBABILITY_SIMPLEX.md) from
math cards 101 and 103. The 32nd object has four retained layers and five
examples covering distributions, expectation, entropy, variance, mixtures and
KL divergence. Ten appended parameters use paired probability controls and
the shared playback route. Boundary references expose their actual finite
support face. The packet records scope, numerical contracts and verification.

This is one asset family; textbook integration remains with its current owner.
Source cards and learner state are unchanged. Verification is CPU/text only;
no images or windows. Changes remain uncommitted.

## P060: bridge and truss assets

The user approved Bridge & Truss Lab as the next asset checkpoint.
[P060_BRIDGE_TRUSS.md](P060_BRIDGE_TRUSS.md) adds triangular support, bridge,
crane boom and roof truss through the 31st object, `truss`. Six editable joints,
a moving static load, support choices and a removable member connect geometry,
signed forces, joint equilibrium and rank/force limits. A bounded factorization
serves every load position; the design check covers the complete load path.
The packet records the model contract and build/check evidence.

This is one asset family. Textbook/game integration remains with its current
owner. Source cards and learner attempts are unchanged; changes remain
uncommitted. Checks use CPU/text only, with no images, windows or font probes.
The user performs visual review.

## P059: rigid-body rotation assets

The user approved free-rotation assets after the vibrating membrane.
[P059_RIGID_BODY_ROTATION.md](P059_RIGID_BODY_ROTATION.md) adds flywheel,
adjustable dumbbell, tumbling book and satellite through the 30th object, `rigid`.
Four retained layers connect orientation/quaternions, component inertia, angular
motion and intermediate-axis stability. Geometry and physics share one component
assembly; existing compact controls, playback and renderer contracts are reused.
The packet records the numerical contract and build/check evidence. This is one
asset checkpoint; textbook integration remains with the textbook worker.

Checks use only CPU/text paths. No images, windows, font probes or personal
saves are used. Changes remain uncommitted; the user performs visual review.

## P058: vibrating membrane assets

The user visually accepted Patch Lab and approved a vibrating membrane next.
[P058_VIBRATING_MEMBRANE.md](P058_VIBRATING_MEMBRANE.md) adds drumhead,
divided membrane, interference and damped pluck as the 29th object, `membrane`.
Four editable mode slots connect displacement, nodal lines, superposition and
energy/damping, using the existing compact controls, playback and indexed mesh.
The document records the model contract and final build/check evidence.

This is one asset checkpoint. Geometry and motion stay in the asset owner;
textbook integration remains with the textbook worker. Source cards and learner
attempts are preserved. Checks use CPU/text paths with no images, windows or
font rasterization; changes remain uncommitted. The user performs visual review.

## P057: parametric patch assets

The user accepted the compact controls and approved the next asset batch.
[P057_PARAMETRIC_PATCHES.md](P057_PARAMETRIC_PATCHES.md) adds canopy, sail,
curved ramp and saddle terrain through the 28th object, `patch`. Sixteen
selectable controls share compact XYZ rows and a UV probe. Four layers expose
Bernstein influence, tangents/normals, curvature/fundamental forms and surface
area convergence. The existing document-figure registry discovers the provider;
lesson integration remains with the textbook worker.

The P057 document records the kernel contract and final build/check evidence.
This is one asset checkpoint; membrane, rigid-body and simplex ideas remain
future work. Source cards, teaching pipelines and learner attempts are preserved.
No images or native windows are used for verification, and changes remain
uncommitted. The user performs visual acceptance of the new patch presets.

## P056: compact model workspace

The user approved a control-density pass after visually accepting the Boolean
Solids Lab. [P056_COMPACT_MODEL_WORKSPACE.md](P056_COMPACT_MODEL_WORKSPACE.md)
adds searchable object/example/layer selection, compact shared scalar controls,
grouped coordinate fields, remembered collapsible groups, a resizable inspector
and a resizable Graph / Values / Math / Exercise drawer. A few key metrics stay
above the model; optional labels use hover/selection. All 27 objects retain their
existing mathematical owners and actions. The textbook reuses the scalar widget
inside its existing authored object controls and learning modes.

Validation status and the manual launch command are in the P056 document.
The pass uses pure layout/model checks and reviewed text-only commands. Visual
acceptance of this new layout remains with the user; changes are uncommitted.

## P055: Boolean Solids Lab

[P055_BOOLEAN_SOLIDS.md](P055_BOOLEAN_SOLIDS.md) adds the 27th object: drilled
block, archway, ball-and-socket and blended stones. Four retained layers link
inside/outside fields, Boolean truth tables, smooth blends/normals and numerical
volume. Position, scale, rotation, operation, section and sampling controls use
the existing semantic route. Pure kernels produce the same bounded indexed mesh;
no renderer, source-card or learner-evidence changes are needed.

Release math_lab and sorter compile. Sixteen focused CPU suites and 62 reviewed
text-only CLI routes pass. The dedicated Boolean suite checks 2,705 numerical
certificates and 377 mesh states, including rotated pairs, manifold edges,
section caps, analytic volumes, gradients, truth rows and atomic actions. Its
largest tested scene has 7,122 vertices and 30,936 indices. No images, windows
or font-atlas tests were used. Changes remain uncommitted; the user will perform
the visual check.

## P054: Lathe Lab

[P054_LATHE_LAB.md](P054_LATHE_LAB.md) adds the 26th math-lab object with four
retained layers: editable profile, revolution, disk/washer/shell volume and
surface bands/normals. Vase, bottle, goblet and pawn presets include solid or
hollow interiors, radial wall thickness and a viewing cutaway. Pure kernels
reuse the existing Bezier evaluator; one bounded indexed surface extends the
existing scene contract without changing the native host or shader.

Release math_lab and the shared sorter UI compile. Fifteen focused CPU suites
and 58 text-only CLI routes pass. The lathe suite checks 1,705 numerical
certificates and 243 mesh states, including independent volume/area integrals,
disconnected shells, hollow floors, oriented volume and atomic editing.
No windows, images or font-atlas tests were used. Changes are uncommitted;
the user confirmed that the lathe works in the following manual test.

## P053: curves and sweeps

[P053_CURVES_AND_SWEEPS.md](P053_CURVES_AND_SWEEPS.md) adds the 25th math-lab
object with four retained layers: Bézier construction, tangent/curvature,
equal-distance travel and capped profile sweeps. Pipe, cable, ribbon and horn
presets share editable controls, with profile, thickness, aspect, taper and
twist. The existing model, scene and semantic actions remain the owners.

Release math_lab and the shared sorter UI compile. Fourteen targeted pure-model
suites and 54 text-only CLI checks pass. The curve suite verifies 599 numerical
certificates and 183 mesh states, including independent arc integration,
stationary points, transported sections, seams/caps and atomic control actions.
No windows, images or font-atlas tests were used. Changes are uncommitted;
visual acceptance awaits the user's manual test.

## P052: convex geometry objects

[P052_CONVEX_GEOMETRY.md](P052_CONVEX_GEOMETRY.md) adds the PSD cone and morphing
norm balls to the math lab: 24 objects overall and eight new learning layers.
Both use the existing model/action/snapshot/scene route. The cone connects
matrix coefficients, eigenvalues, quadratic surfaces, convex mixtures and
fixed-trace optimization. Norm balls connect distance, vector addition,
finite-p convergence and dual supporting planes, with exact endpoint shapes.

The Release math lab and shared sorter UI compile. Thirteen focused model and
CPU-mesh suites and 50 text-only CLI routes pass. The dedicated suite checks
principal-minor and spectral certificates, mesh equations, rank boundaries,
convex closure, objective attainment, triangle inequalities, dual contacts,
zero/tie cases and atomic rejection. No windows, images or font-atlas tests
were used. Changes are uncommitted; visual acceptance is pending the user's
manual test. The existing textbook queue remains available for later work.

## Matrix documents through four support levels

`content/write/matrix.paths.md` adds **Linear Algebra → Matrix rows from
documents**: one reading and one three-step question. `@template matrix.v1`
uses ordinary augmented-matrix text, numeric choices and a named `@operation`.
The [authoring contract](LEARNING_DOCUMENTS.md#matrix-documents) lists the four
supported row-addition/division commands and required teaching fields. The
shipped Library now has 934 readings and 315 questions; four document templates
are available. Further matrix content requires a document and relaunch.

Learn presents explained symbolic row operations; Practice fills a multiplier
or divisor; Solve and Write accept complete matrices, one per line. Given stays
gold beside cyan working, and completion stays green until Next. The same
support owner projects goals, input syntax and disclosures for both families.
No new question session, UI solver or save format was added. Linear-only support
nodes now use the existing typed `MathWorkingValue`; numeric TeX formatting is
shared. The existing exact row kernel owns operations and matrix equivalence,
with final substitution in both original equations.

Release builds of sorter, gallery and paths pass. Seven targeted CTest entries
and the document UI executable pass: 64 matrix model routes over 16 parameter
sets (ten with fractional answers), 12 matrix UI routes within 30 document
routes at 1440×860, 800×600 and 360×480, all without notation fallback. The
existing linear, prepared/guided, row-move and progress checks remain green.
Matrix checks cover wrong operands, zero divisors, exact arithmetic bounds,
nonunique/changed solution sets, alternative written routes, atomic bad-line
rejection, retained drafts, Undo branches, renamed documents and original-save
protection. The final teaching text also passed the headless UI route.

Four CLI checks confirm source/bundled import, a newly dropped matrix document
without rebuilding, and rollback of a mathematically wrong authored answer key.
Production C++: +231/-43 lines, net +188 across seven existing files, with no
new production C++ files. One source document was added; three existing tests
were extended. All 148 checked pre-existing content, textbook, matrix-board,
object and scene files retain their pre-work bytes. Evidence:
`build/matrix-document-evidence/verification.json`.

Changes remain uncommitted; no windows or captures were used. Manual visual
acceptance is pending. This first matrix template handles nonsingular 2×2 real
systems; larger/complex matrices and general solution families need distinct
response contracts. Written input is matrix notation, not symbolic row-command
or proof prose. A direct final matrix is a verified answer, not evidence of a
multi-step derivation. The next candidate is a small matrix repetition set
authored through this same document template.

## Learning documents: folder into the app

[LEARNING_DOCUMENTS.md](LEARNING_DOCUMENTS.md) defines `.paths.md` source,
includes, ToC declarations, stable links and three versioned templates:
`lesson.v1`, `choices.v1` and `linear.v1`. The executable imports the complete
chosen folder at startup. Authors add supported content without rebuilding;
`--check-content --documents content/write` validates it headlessly, and
`--document-capabilities` lists the templates and 22 registered diagram providers.

`LearningDocuments` owns bounded parsing and atomic import, compiling through
the existing corpus and question validators. `DocumentLessonUi` formats the
result and passes diagram inputs to `MathObjects`; the existing scene and native
renderer supply geometry. `LayeredQuestionSession` keeps all answer judgments,
and `CorpusPractice` keeps question progress. The old file parser now delegates
to the same question parser used by documents; no second answer route was added.

Algebra, physics and biology examples add two subjects, three chapters, three
readings and three questions: 933 readings and 314 Library questions overall.
The algebra question uses all four support levels. The physics reading calls
the existing harmonics provider; biology demonstrates ordinary text choices.
Blue question links open the fixed solving workspace; explicit Method links
read imported teaching. Completed questions remain until navigation.

Release builds of sorter, gallery and paths pass. Eight targeted CTest entries
pass, including six imported model routes, 18 real headless UI solving routes,
three diagram-orbit routes at 1440×860, 800×600 and 360×480, plus the existing
corpus, prepared, matrix and four-level regressions. A built-executable check
discovers an additional lesson and question from a newly placed file without
changing the binary. A broken template reports its file/line and commits no
partial import. Renaming files, adding content and restoring written drafts
pass; incompatible frozen content retains the original save.

Production C++: +474/-16 lines, net +458, across nine existing files and four
new files. This is new capability code. Two C++ tests, three entry documents
and one shared include were added. All 55 existing files in the checked cards,
corpus, references and source-snapshot roots retain their pre-work bytes.
Concurrent work is preserved. Evidence: `build/document-pipeline-evidence/verification.json`.

Changes remain uncommitted; no windows or captures were used. Manual visual
acceptance remains pending. Imports require relaunch; arbitrary new response
checkers and diagram providers still need implementation, and authored science
answer keys are not automatically fact-checked. The next candidate is a matrix
template through the same document pipeline and four-level question owner.

## Four support levels: first linear family implemented

[QUESTION_PRACTICE_FORMAT.md](QUESTION_PRACTICE_FORMAT.md) defines Learn,
Practice, Solve and Independent around one unchanged mathematical question.
[QUESTION_AUTHORING_WORKFLOW.md](QUESTION_AUTHORING_WORKFLOW.md) defines
authoring, source provenance, independent verification, four-view disclosure
and concrete implementation/content task packets. The golden example plus 24
varied repetitions now run in the Library's existing workspace: explained
symbols, a typed blank, equation checkpoints, and a blank multiline solution.
Search **Linear practice**, then Focus. The fourth button is **4 Write**.

`LayeredQuestionSession` remains the canonical owner. Supported questions reject
the prepared answer-tile command route; typed submissions have their own guarded
events. Exact linear checking accepts alternative equivalent routes and verifies
the original substitution. Unsupported syntax retains the draft without a
wrong-math mark. Help exposure, level changes, draft edits, Undo branches and
archived runs survive `CorpusPractice` replay. Version-1 prepared saves still
load; version 2 adds the named support records. Original incompatible saves remain.

Release builds of sorter, gallery and paths pass. Eleven targeted CTest entries
pass across the feature and regression gates: 100 model routes, 200 wrong
responses, 12 actual headless UI routes at 1440×860, 800×600 and 360×480,
250 native formulas without fallback, eight Python authoring tests, save
compatibility and surviving prepared/guided/mathematical-move routes.
Evidence: `build/four-level-evidence/verification.json`.

Production C++ adds 515 net lines across ten existing files, with no new
production C++ file; one generated runtime JSON pack and two C++ test files
were added. The authoring ledger counts 25 integrated instances in one family,
with the remaining 228 leaves unfilled in this four-level format. Taxonomy and
visual acceptance remain pending. This checker handles bounded linear equation
lines; general proofs, nonlinear work and free prose are not automatically
verified. The inline teaching is frozen with its saved question stamp.

The next candidate is one matrix family through the same four support levels,
after the user's format check. Further question-linked 3D and Motion development
are deferred. Changes remain uncommitted; no windows or captures were used.

## Regular questions with shared method reading

[MATRIX_REASONING.md](MATRIX_REASONING.md) adds eight questions and 23 symbolic
decisions: row operations, REF/RREF, fractional pivots and system solution sets.
Focus keeps gold Given and cyan Working above the choices; purple Method reads
existing textbook definitions without changing attempts or disclosing the
textbook's exercise answers. The question session remains the answer owner.
Release sorter and four targeted checks pass, including independent exact
arithmetic, 24 UI routes at three sizes and unchanged-question save migration.
Evidence: `build/matrix-reasoning-evidence/verification.json`. No windows or
captures were used. Changes remain uncommitted; visual review is deferred until
the user is ready. A later candidate is a read-only figure linked to the active
question's canonical working, using the boundary documented in the checkpoint.

## Shelved: Drive the velocity graph, chapters 1–5

[MOTION_LESSONS.md](MOTION_LESSONS.md) introduces five playable questions from
position to acceleration and braking. An actual 3D cart, editable graph,
symbolic plan tiles, working and prior attempts share the fixed workspace.
Release sorter and three targeted headless tests pass, including 1,021
independently checked plans, 20 real UI solving routes, and in-flight save/resume.
Evidence: `build/motion-lessons-evidence/verification.json`. No windows or
captures were used. Changes remain uncommitted. The user saw the direction but
asked to shelve the design; this is not full visual acceptance. Further chapters
and design decisions are deferred. The prototype and saved work remain available.

## Representative questions across the Library

[CORPUS_STARTERS.md](CORPUS_STARTERS.md) adds one starting question for every
subject, chapter and named subcategory: 278 total. Each uses two symbolic
decisions, native notation, retained working and explicit Next. The existing
question session remains the only answer owner. New progress is saved by
stable identity in a separate file, with command replay and protection of
incompatible originals. The [coverage sheet](CORPUS_STARTER_COVERAGE.md) records
all assignments. Evidence lives in `build/corpus-starters-evidence/`; visual
acceptance of this new capability remains pending. The next candidate is
subject-specific formatting informed by the user's checks.

## Accepted Library typesetting

[LIBRARY_TYPESETTING.md](LIBRARY_TYPESETTING.md) puts native equations inside
the actual Library entries. Cyan math shares lines with prose or occupies its
own display line; Raw source remains available and amber notation explains
fallbacks. All 930 entries passed the headless layout/drawing probe, with
2,139 formulas typeset and 72 source fallbacks across 41 entries. These are
rendering results, not mathematical review. The Release sorter build and two
targeted tests passed, including live resizing, source navigation and exact
unfinished-practice preservation. Evidence is in
`build/library-typesetting-evidence/verification.json`. Changes are uncommitted;
the user subsequently reported “heyyy it looks great.” A future content workstream can review
the reported source defects while retaining the pinned originals.

## Native equation panel

[NATIVE_EQUATION_PANEL.md](NATIVE_EQUATION_PANEL.md) establishes native
typesetting through the existing ImGui/Vulkan path. Library → Equations opens
four coloured samples with text-size controls and retained LaTeX source.
Release builds and targeted headless checks passed, including dynamic font
atlas requests and three viewport sizes. Evidence is in
`build/native-equation-evidence/verification.json`. Changes are uncommitted;
the user's visual check remains pending. The Library integration above follows
this rendering checkpoint.

## Linked reviewed notes

[LINKED_CORPUS_NOTES.md](LINKED_CORPUS_NOTES.md) connects the four reviewed notes
with seven explicit reading links. Cyan Related notes controls stay within the
Library reader; Back and Escape restore the previous location and reading mode.
The exact search and subject/topic filters survive linked reading, as does
unfinished practice. Evidence: `build/linked-notes-evidence/verification.json`.
Changes are uncommitted; the user subsequently reported that the Library work
was good and identified LaTeX display as its remaining visible issue.

## Rank, nullity and consistency

[RANK_CORPUS_REVIEW.md](RANK_CORPUS_REVIEW.md) extends the reviewed-note contract
with Rank and Nullity, including consistency and free-variable explanations.
All thirteen matrix questions share eight Symbols lessons; the Library has
four reviewed adaptations and 926 entries still without one. Original sources,
earlier notes and playable mathematics are unchanged. Evidence is in
`build/rank-review-evidence/verification.json`. Visual review remains deferred
and changes are uncommitted.

## Reviewed matrix teaching notes

[MATRIX_CORPUS_REVIEW.md](MATRIX_CORPUS_REVIEW.md) records source comparison,
corrections, exact examples and the shared publication contract for Gaussian
elimination and RREF. Two reviewed adaptations are available in Library and
matrix Symbols; 928 corpus entries remain without a reviewed adaptation.
Original sources and playable mathematics are unchanged. Evidence is in
`build/matrix-review-evidence/verification.json`; visual review remains deferred
and changes are uncommitted.

## Corpus table of contents

The [corpus TOC](MATH_CORPUS_TOC.md) connects the pinned six-subject source to
Contents → Library, with subject/topic filters, title search and source notes.
All 930 entries are marked not yet fact-checked. This is navigation and reading;
mathematical review and adaptation to playable lessons remain a later capability.
Evidence: `build/corpus-toc-evidence/verification.json`. The existing practice
queue and unfinished question are preserved. Visual review remains deferred,
and changes are uncommitted.

## Definitions, symbols and syntax

The user authorized a reusable notation feature while another builder owns the
3D mathematical objects. The [design and content contract](MATH_NOTATION_DESIGN.md)
covers a shared Symbols panel, contextual definitions, selectable occurrences
and optional reading practice, linked to the 27 existing playable study
questions. The long-history repair remains deferred. Build and headless results
are recorded in `build/notation-feature-evidence/verification.json`. Visual
review remains deferred, and changes are uncommitted.

## Workstream history

| ID | User-visible capability | Delivery | Required boundary |
| --- | --- | --- | --- |
| P001 | Standalone Paths app with title screen, Guided Questions, Quick Hunt, and session stats | In Progress; current session owns implementation after cross-chat failure | Paths-only copy builds/runs without the parent repo; carried models and captured title/modes agree |
| P002 | Paths Workshop: choose objectives/scoring and compare a completed run under another rule | User-authorized; next checkpoint after P001 | Frozen run config; one canonical scoring owner; comparisons preserve evidence |
| P003 | Choose and complete source card 002 in Guided Questions | Authored; P011 gallery adaptation available; multi-card Guided grid integration pending | Catalog routing and evidence isolated per question/version |
| P004 | Work through source card 013 as a guided derivation | Authored; P012 gallery adaptation available; Guided grid integration pending | General justification retained; selection practice not labelled independent proof |
| P005 | Port the iggy3d gallery engine foundation into a standalone Paths startup | Automated Green; Manual Test Needed; committed locally as `2308995` | Independent native mesh rendering, camera/primitive/picking/patrol actions, and headless evidence |
| P006 | Shared coloured-ball target/question foundation with Sweep, Relay, and Chain fixtures | Automated Green; Manual Test Needed; committed locally as `2308995` | One mesh-pick-to-verdict route, collection, atomic colour/question reset, stale-shot guards, common motion/pop clock |
| P007 | Shared structural validation for question content and catalogs | Automated Green; committed locally as `b5078b9` | Structured first-error locations, constructor reuse, existing identity/mask/capacity/interaction contracts preserved |
| P008 | Explicit step completion and prepared mathematical working | Automated Green; committed locally as `2308995` | One answer-rule owner; validated working chain; Continue controls working visibility; gallery commits remain atomic |
| P009 | Playtest the six-decision substitution integral | Automated Green; offscreen reviewed; committed locally as `2308995` | Compiled fixture uses the same validated question owner and gallery route; future content parsing stays separate |
| P010 | Load editable question cards and mode decks from JSON | Automated Green; offscreen reviewed; committed locally as `2308995` | Shared validation, stable-ID resolution, frozen run content, source/field startup errors, no compiled gallery fallback |
| P011 | Play source card 002 through the JSON gallery | Automated Green; offscreen reviewed; committed locally as `2308995` | Thirteen authored decisions, explicit ID mapping, preserved working/attempts, existing executable and question owner |
| P012 | Play source card 013 as a gallery derivation | Automated Green; offscreen reviewed; committed locally as `2308995` | Fourteen authored decisions, supplied target and general justification retained, stable evidence, existing executable and question owner |
| P013 | Choose and resume question packs inside the gallery | Automated Green; offscreen reviewed; committed locally as `2308995` | Shared Play/CLI launch, paused sessions retained per pack and practice type, recoverable loading errors, scene changes between frames |
| P014 | Choose movement pattern and speed before Play | Automated Green; offscreen reviewed; committed locally | Shared config validation, pending setup, immutable per-game settings on Resume |
| P015 | Separate playable questions from retained authoring material | Automated Green; committed locally | Byte-preserved authoring moves, updated validation consumers and provenance destinations, unchanged playable packs |
| P016 | Start a fresh game for a previously started set/practice type | Automated Green; offscreen reviewed; uncommitted | Cancellable replacement draft, shared preparation, preserved old run on failure and isolated replacement |
| P017 | Review reached steps and submitted answers while paused | Automated Green; offscreen reviewed; uncommitted | Frozen question-owner projection, ordered attempts, current/completed history, no future answer disclosure, paused return |
| P018 | Read prepared explanations for resolved steps in review | Automated Green; offscreen reviewed; uncommitted | Existing resolution predicate gates frozen authored text; partial sets and fresh runs remain hidden |
| P019 | Keep shooting with brief feedback and totals only while stopped | Automated Green; offscreen reviewed; uncommitted | Wrong clicks retain the active question and moving targets; half-second signal never gates input; Stop/Resume uses the existing pause owner |
| P020 | Standalone native equation sorter with stable slots and reversible grouping | Automated Green; offscreen reviewed; uncommitted; Manual Test Needed | File-loaded 100-card pack; one owner per equation; two activations; fixed slots; atomic Undo; separate from shooting |
| P021 | Mixed-subject sample pack through the existing sorter | Automated Green; offscreen reviewed; uncommitted; Manual Test Needed | 80 retained algebra cards plus 20 checked examples across four subjects; same loader/session; stable card width from the first inventory frame |
| P022 | Auto sort by subject with optional hints and next-action guidance | Automated Green; offscreen reviewed; uncommitted; Manual Test Needed | Prepared metadata; A–E plus manual Dump; one automatic transaction and Undo; guidance before, during and after grouping |
| P023 | Solve one selected algebra card through operations, arithmetic and checking | Automated Green; offscreen reviewed; uncommitted; Manual Test Needed | Shared question progression and help evidence; immediate working updates; real sphere answers; preserved sorter state on return |
| P024 | Generate and play six bracket equations from compact recipes | Automated Green; user-confirmed loop and controls; uncommitted; formatting next | Exact prepared arithmetic, shared question format, stable identities, independent per-card resume and unchanged grouping history |
| P025 | Keep prepared solving and explicit Next in one fixed workspace | Automated Green; uncommitted; user spacing/notation feedback addressed by P026 | Fixed problem, working, activity and help regions; one click per step; retained per-card evidence and terminal Next |
| P026 | Bring the equation and compact symbolic choices together | Automated Green; uncommitted; user confirmed improved visibility; window sizing addressed by P027 | Authored symbolic labels with unchanged mathematics; compact stable layout; supported glyphs |
| P027 | Fit the solving workspace to the available window | Automated Green; uncommitted; user visual confirmation pending | Problem, notation and choices grow together; live resize preserves the current question and shared scene viewport |
| P028 | Table of contents, problem types and selected practice sets | Accepted by user; automated checks passed; uncommitted | Authored catalogue; all/random/specific draft; frozen selected-only Next; preserved completed and unfinished attempts |
| P029 | Coordinate board, animated slope triangle and movable point | Accepted by user; automated checks passed; uncommitted | Four prepared line questions; question-owned graph projection and judging; fixed board, replayable motion and explicit Next |
| P030 | Simultaneous equations with two lines and a shared comparison guide | Accepted by user; automated checks passed; uncommitted | Four prepared systems; shared graph/answer owner; staged solution reveal; fixed workspace and explicit Next |
| P031 | Linked numerical substitution, graph and value table | Automated Green; user visual confirmation pending; uncommitted | Same question-owned projection and x guide; selectable samples; fixed problem and unchanged attempt evidence |
| P032 | Check mathematical moves and construct an inspectable solution blueprint | Automated Green; typing interface rejected by user; replaced by P033; uncommitted | Exact per-side transformations; multiple valid routes; append-only attempts and Undo branches; same question owner and explicit Next |
| P033 | Solve mathematical moves using symbols and result tiles | Visual format accepted by user; automated checks passed; uncommitted | No equation typing; same exact checker, retained branches and explicit Next; mouse and keyboard controls |
| P034 | Solve one linear algebra question with row moves and matrix tiles | Accepted by user; automated checks passed; uncommitted | Same question owner and visual format; exact row operations; alternate routes, retained branches and original-system verification |
| P035 | Read linked row-operation definitions and step through separate examples beside the problem | Automated Green; user visual confirmation pending; uncommitted | One shared library and exact row kernel; immutable linked definitions; fixed workspace, no player attempts from reference browsing |
| P036 | Save practice and resume the same checked working after restarting | Automated Green; user visual confirmation pending; uncommitted | Canonical command replay; exact drafts and frozen queue; preserved attempts, help, branches and archived runs; device-local atomic file replacement |
| P037 | Add twelve matrix problems while keeping existing saved practice compatible | Automated Green; user visual confirmation pending; uncommitted | Frozen P036 worker copy; 24 checked routes; both 100-card catalogues integrated; stable-ID save dependencies; preserved drafts, queue order and history across catalogue growth |
| P038 | Show current-attempt progress and chapter completion counts in Contents | Automated Green; user visual confirmation pending; uncommitted | Question-owned progress; selection-independent totals; Undo, Replay and saved evidence remain canonical; compact native symbols |
| P039 | Explore five native 3D mathematical objects with live parameters and challenges | Build and headless tests passed; visual/interactive review unperformed; uncommitted | One mathematical owner; existing SceneFrame/Vulkan pipeline; bounded meshes and separate exploration state |
| P040 | Explore linked function, matrix and surface learning layers | Native build and six targeted text-only checks passed; visual/interactive review unperformed; uncommitted | One mathematical owner; analytic readouts; linked plots/matrices/contours; bounded geometry; no images |
| P041 | Explore symmetry, harmonic synthesis and spring/pendulum dynamics | Native build and ten targeted text-only checks passed; user reported successful visual try; uncommitted | Exact cube rotations; Fourier coefficients; deterministic motion and spring convolution; existing renderer; no images |
| P042 | Explore modular arithmetic, Gaussian integers and vector fields | Native build and fourteen targeted text-only checks passed; user visual review pending; uncommitted | Exact integer kernels; bounded tables and walks; linked paths and integrals; existing renderer; no images |
| P043 | Explore flux shells, tensor blocks and probability networks | Native build and eighteen targeted text-only checks passed; user reported successful try; uncommitted | Oriented surface/volume integrals; tensor contractions and basis invariants; stochastic matrices and bounded walk evidence; no images |
| P044 | Explore binomial trials, Bayesian conditioning and covariance geometry | Native build and twenty-two targeted text-only checks passed; user visual review pending; uncommitted | Exact finite probabilities; explicit undefined cases; moments from actual points; PCA/whitening; no images |
| P045 | Explore spherical harmonics, quadratic forms and roots of unity | Native build and twenty-six targeted text-only checks passed; user visual review pending; uncommitted | Real orthonormal modes; heat diffusion; signed/singular forms; Rayleigh bounds; exact cyclotomic action; no images |
| P046 | [Reusable matrix board](P046_MATRIX_BOARD.md) for six exercise cards | Native build and 33 targeted text-only checks passed; user visual review pending; uncommitted | Complex/rectangular matrices; row operations; pivot and block traces; setup disclosure; read-only source cards; no images |
| P047 | [Interactive textbook](P047_INTERACTIVE_TEXTBOOK.md): Matrices and Elimination | Native build and 29 targeted text-only tests passed; user visual review pending; uncommitted | Seven sections; 21-term index; retained board work; reading bookmarks separate from exercise evidence; no images |
| P048 | [Textbook section format](P048_TEXTBOOK_SECTION_FORMAT.md): Row operations and RREF | Native build and 30 targeted text-only tests passed; reading format accepted by user; uncommitted | Typed numbered blocks; stable references; typeset equations; separate redacted help; existing exercise evidence retained; no images |
| P049 | [Live textbook figures](P049_LIVE_TEXTBOOK_FIGURES.md): reusable spread and RREF equation planes | Native build and 34 targeted text-only tests passed; layout and interaction accepted by user; uncommitted | One reusable shell; synchronized board and 3D geometry; all nullities; responsive panes; no images |
| P050 | [System solution sets](P050_SYSTEM_SOLUTION_SETS.md): affine planes and prediction practice | Native build and 42 targeted text-only checks passed; user reported "looks good"; uncommitted | Reused four-view shell; explicit rhs; shared row kernel; all solution outcomes; redacted practice; bookmark migration; no images |
| P051 | [Determinants as signed volume](P051_DETERMINANT_VOLUME.md) and reusable object lessons | Native build and 48 targeted text-only checks passed; user visual review pending; uncommitted | Five examples; independent practice; registry-driven navigation/bookmarks; generic controls/readouts; three ready batch briefs; no images |

Local commit `2308995` records the standalone project foundation through P013;
no push was made. Historical checkpoint sections below retain their original
commit-state descriptions.
P014 and P015 are recorded together in local commit `6a9f1c8`.

The current native targets are `math_lab`, `sorter`, `gallery`, and `paths`; Release builds
use `b`. See the [README](../README.md) for current commands. Historical
checkpoint sections retain the target names and build paths used for their evidence.

The current sorter checkpoint is P038; the standalone gallery remains at P019. The preceding
[P006_TARGET_FOUNDATION.md](P006_TARGET_FOUNDATION.md) implements the
target/question boundary proposed in
[TARGET_FOUNDATION_DESIGN.md](TARGET_FOUNDATION_DESIGN.md). It adds four
production files, extends the existing scene/question owners and startup, and
reuses P005's native renderer. The twelve motion patterns are now available;
source cards now play through the separate P011/P012 packs. Guided grid
integration, scoring/bonus rules and saved profiles remain later
capabilities. This does not mark P001 through P004 complete.

## Current priority

The regular-question priority is the [four-level format](QUESTION_PRACTICE_FORMAT.md)
and its [authoring workflow](QUESTION_AUTHORING_WORKFLOW.md), with the question
session owning answers, working and help exposure. The matrix questions remain
available. The teaching workstream continues independently below; its existing
definitions are reusable from the question workspace. Interactive 3D follows
the question foundation, and Motion remains shelved.

[P051](P051_DETERMINANT_VOLUME.md) is the current textbook checkpoint: section
1.9 connects the existing linear cube to reading and separate practice. Its
registry and object adapter support [Batch 01](lesson_tasks/BATCH_01.md), three
prepared lessons with an explicit sequential start instruction. The pilot awaits
the user's visual test; the queued lessons are not yet implemented. This teaching
work remains separate from the regular-question workstream.

[P050](P050_SYSTEM_SOLUTION_SETS.md) adds section 1.3 after RREF: one, infinitely
many, or no solutions. It reuses the user-accepted four-view shell with movable
affine equation planes, synchronized row operations, solution probes, and
prediction practice. Existing section IDs and seven-section reading bookmarks
remain usable. Verification is text-only; the user subsequently reported
that P050 "looks good."

The [lesson authoring workflow](LESSON_AUTHORING_WORKFLOW.md) describes the
repetitive work and reusable task template. P051 implements its determinant
pilot; Batch 01 contains the next three implementation briefs. Queue execution
starts from the user's batch instruction.

The user prohibits taking or viewing images. Do not capture, render, open
image files or launch previews as part of this work. Use the pure-model/scene
tests and `math_lab --validate` for text-only verification. Appearance and
interaction acceptance are recorded per checkpoint from the user's feedback;
agent checks do not establish them. Keep changes uncommitted.

## Previous cleanup checkpoint

The user requested another bounded code cleanup and deferred manual review.
The [prepared-answer cleanup](CLEANUP_SORTER_INPUT.md#follow-up-prepared-answer-feedback)
merges duplicate submission, feedback and advancement in
`GallerySession::submitAnswer()`. It removes four net production lines across
two existing files. Release builds of `sorter` and `gallery` and three targeted
headless checks pass. The earlier shared-summary cleanup is preserved. This
checkpoint stops at answer handling; no new visual check is requested.

[P038 Contents progress](P038_CONTENTS_PROGRESS.md) remains implemented with
five targeted headless checks and all three native builds passed. Its visual
confirmation and earlier visual gates retain their own pending status. The
P037 chapter, prior input cleanup and current UI format are preserved. Use no
screenshots, captures or windows. The user's parser and notes remain separate.
The unfinished-question filter remains a later feature candidate, using the
existing progress projection without changing an active queue.

Keep the build focused on answering questions quickly and continuously for
long sessions. Before adding systems, check rapid repeated input, automatic
question changes and responsiveness as attempt history accumulates. Then
format small prepared packs for different levels of maths through the existing
question model, ready for the user's playtesting.

Visual equation transformations and effects that build with progress are a
recorded later direction in [P019_CONTINUOUS_PLAY.md](P019_CONTINUOUS_PLAY.md).
They must respond to accepted mathematical steps without delaying the next
shot. Countdown modes, additional review features and richer effects are
deferred while the basic loop and content format are established. This priority
takes precedence over historical next-candidate notes below.

The user subsequently authorized building the separate [P020 sorter](P020_EQUATION_SORTER_SPEC.md).
Its native prototype and checked algebra fixture are now implemented. It remains
an independent startup. This checkpoint establishes sorting, content loading and
recovery; it did not yet supply other maths subjects or a gallery connection.
The resumed P020 check repaired keyboard navigation across the inventory/header
boundary and focus after cards or confirmation controls disappear. The expanded
input gate passed and a fresh native offscreen inventory was inspected. See the
resumed checkpoint in the P020 spec.

The authorized [P021 mixed pack](P021_MIXED_MATH_PACK.md) now supplies five
prepared examples each of trig, calculus, linear algebra and discrete maths,
interleaved with 80 retained algebra cards. Mathematical checks read the new
strings outside the runtime, and native previews establish readable plain-text
notation. A one-line UI correction reserves scrollbar space from the first
frame so inventory cards do not shift when clicking begins. Human playtesting
of grouping and notation remains next; broader subject coverage and gallery
integration are separate later capabilities.

The user then requested Auto sort and a way to ask for help at each step,
choosing grouping by maths subject. [P022](P022_SORTER_ASSISTANCE.md) supplies
Auto sort for remaining cards, prepared per-card hints, and state-specific next
actions through the existing sorter owner. All five subjects have their own
groups; existing manual choices are preserved and one Undo reverses the batch.
The four targeted sorter suites passed, including actual controls at four sizes,
and native offscreen screens were inspected. Human playtesting remains next.
Future interactive solving stages should likewise provide optional hints and a
clear next action. This checkpoint covers the sorter; it does not add those
future solving stages or alter the continuous shooting loop.

The user's subsequent request is now completed in [P023](P023_PREPARED_SOLVING.md):
card 1012 opens the prepared division-first solution of `-2(x + 1) = 22`.
Its highlighted structure, operation choices, arithmetic targets, immediate
working changes, three levels of help, and final substitution check share the
existing question/gallery owners. Returning preserves the sorter and resumes
the same decision. The resumed checkpoint completed missing presentation/input
details and verified the retained in-progress implementation. Player feedback
on this single complete question was the proposed next step before the user
authorized P024's bounded algebra family.

[P024](P024_BRACKET_RECIPES.md) now generates six bracket questions from compact
recipes through the existing authoring tool and question format. The separate
practice pack puts the prepared cards first, with visible Solve labels. Positive,
negative, zero and fractional cases share the operation/arithmetic/help flow.
The sorter retains each card's session when switching, including attempts and
assistance; grouping and Undo remain separate. The original hand-authored
bracket card is replaced by generated version 2. The original two sorter files
retain their bytes and shared IDs keep the same meaning across packs.

At the user's direction, complete code, builds and automated checks first,
then give a short visual checklist at the end of the brief. Do not capture
screenshots; the user performs visual confirmation. This supersedes the earlier
capture workflow for subsequent work. On 2026-09-07 the user confirmed testing
the whole P024 loop and that all buttons do what they should. Formatting and
sequencing remain open; this is not blanket acceptance of presentation polish.

### Study selection and a persistent solving workspace: design discussion

The user now wants to discuss sequencing and time spent on each screen before
changing formatting. Their requested flow is:

- Select one or multiple maths subjects, such as algebra, calculus and trig.
- Go deeper into chapters organized like a textbook, with room to follow the
  learner's current book as well as a general chapter sequence.
- Select all questions, random questions or specific problems from the chosen
  chapters, then begin the resulting selection of questions.
- Keep the original problem in one easy-to-identify place for its entire solve.
- Keep methods and games in a single 3D activity area. Changing solving steps
  must not switch screens or relocate the problem and activity space.
- On completion, keep the finished problem and working visible until the user
  chooses Next. The user explicitly selected this pacing on 2026-09-07;
  individual accepted solving steps still advance immediately.

Proposed defaults, still under discussion: expand subjects, chapters and
problem choices within one setup screen; show the selected question count;
keep the original problem separate from the changing working; use one stable
camera and activity footprint; keep help in a consistent place. Question
selection and question order are separate choices. Random selection should
produce a fixed set for the session, without duplicate selections. Next should
load the following problem into the same board and activity area. The end of
the selected set should remain in that workspace as well.

The existing sorter classifies cards by subject; it has no chapter-selection
catalog or finite study-selection flow. Its solve UI changes the activity
layout between operation and arithmetic steps. `LayeredQuestionSession` remains
the owner of judging, working and evidence, with `GallerySession` composing
the activities. Any later selection flow must reuse those owners. The gallery's
existing repeating deck is not evidence of a finished finite study session.
The user subsequently authorized the first implementation checkpoint:
[P025](P025_FIXED_SOLVING_WORKSPACE.md) now gives the six prepared questions a
fixed solving workspace and explicit Next in pack order. The original problem
is marked amber/gold, the current working has a mint heading, and operation
choices and labelled targets share one scene viewport. Help has a fixed panel;
Next retains the previous question and stops at the last prepared card. The
Release build, model sequence and actual input checks passed. User visual
confirmation remains pending; no screenshots were taken. Subject/chapter
selection and all/random/specific question queues remain the next capability.

The user's P025 feedback requested less distance between the question and
choices and symbolic operation labels. [P026](P026_COMPACT_SYMBOL_CHOICES.md)
centres compact symbol buttons beneath the equation and working, moves controls
above the problem, and bounds the activity height. The recipe generator supplies
the symbols and shared both-sides instruction as versioned prepared content;
mathematical answers, working and help retain their values. The Release build,
three targeted tests, generation check and bundled content validation passed.
The user performs the remaining visual check; no screenshots were taken.

The user confirmed that P026's layout made the question easier to see, but
reported a large unused band at the bottom of a roughly 1440×860 window.
[P027](P027_RESPONSIVE_SOLVING_WORKSPACE.md) replaces the fixed activity cap
with a workspace that grows and centres within the available space, including
its maths and answer controls. The Release build and targeted input suite
passed, including live resizing and the solving loop at the reported size.
The user performs the remaining visual proportion check; no screenshots were taken.

The user then authorized a table of contents leading from chapter titles to
problem types, with smaller text and buttons. [P028](P028_STUDY_SELECTION.md)
now opens that selection on normal Sorter startup. All, random and individual
selection use the six prepared bracket equations; the catalogue supports
multiple authored subjects, chapters and types as more questions are prepared.
Start freezes the set, Next follows only its selected questions, and Resume
keeps the active step even while the next selection is edited. The existing
question owner archives completed and unfinished attempts before a new set.
All three native Release builds and six targeted tests passed; no screenshots
were taken. The user subsequently confirmed the experience and requested
checks work, likening the flow to opening a maths textbook and entering visuals.
They endorsed continuing the no-screenshot workflow. After discussing maths
visuals, the user authorized the coordinate board and slope triangle as the
next capability. [P029](P029_COORDINATE_BOARD.md) adds four prepared graph
questions under Straight lines, using the same answer, assistance and selection
owners. Release builds, mathematical checks and actual headless input checks
pass. The user subsequently reported that the feature works throughout and
believes they tested all four graph questions. P029 is accepted; no screenshot
or agent-launched window was used. The user then authorized the proposed two-line
set. [P030](P030_SIMULTANEOUS_EQUATIONS.md) now adds four systems: integer and
fractional crossings, parallel lines, and equivalent equations. Both original
equations stay on the gold board, teal and violet lines share an exploratory x
guide, and completed solutions are marked mint. The existing question owner
supplies the two-line projection and handles all answers and help. All three
native builds and six focused suites pass without screenshots. The user
subsequently confirmed that the visual check passed. The cross-section
discussion recommended linked equation/graph/value tables first, followed by
parameter controls, earlier-step inspection and direct answer placement.
The user authorized continuing with that first recommendation.

[P031](P031_LINKED_VALUES.md) now supplies three selectable sample rows and
an amber live row beside each existing graph. The current input appears in
numerical substitutions above it. All values come from the existing question
projection; pointer, keyboard and slider input change one transient x without
altering answers. The native builds and two focused suites pass, covering all
eight graph questions at three sizes. User visual confirmation remains pending;
no screenshots or agent-launched windows were used.

P001's migration brief is [P001_BUILD_PACKET.md](P001_BUILD_PACKET.md). It
supersedes the unsent FM002-R1 repair packet for the parent engine. Adopt those
specific teaching/input/layout repairs only in the copied Paths files.

The parent FM002 implementation was reported green by its builder with a
five-test gate. Planner review confirmed its supplied first-try/retry/reveal
JSON counts, and identified text-scale, context-layout, input, and smoke-mode
gaps. The parent remains the recorded baseline during Paths migration; no
further feature work is assigned there.

Source-card artifact checks passed: seven pinned snapshots, two authored cards
with 27 layers, exact coefficients and determinant for 002, twelve rational
013 instances and sixty distance-gap comparisons. These are content/planning
checks, not Paths engine acceptance. Interactive swapchain behavior and human
pointer feel remain separate from offscreen evidence.

## P007: question-content structural validation

`src/runtime/first_move/LayeredQuestionSession.hpp` and `.cpp` remain the owner.
`validateQuestion(content, interaction)` and `validateCatalog(span, interaction)`
read the existing content types without changing them. They return a
`QuestionValidationResult` with an error code, static field name, and optional
zero-based question/step/option indices. Success has no field or indices;
catalog-size and interaction errors have no content location. A standalone
question's content errors use question index zero. Results borrow no strings
from the supplied content.

Validation returns the first error in the existing constructor order:
question fields, duplicate catalog identity, then that question's steps and
options in order. The constructor now consumes this shared result and retains
its original `std::invalid_argument` reasons. The initial-question-index check
stays in the constructor. Its former content-validation loop is removed.

The retained contract allows 1–64 questions, 1–32 steps per question, and 2–8
options per step. Question identity is the `(id, version)` pair; step IDs are
unique within their question and option IDs within their step. Required values
remain nonempty question IDs, prompts and option labels, and nonzero versions,
step IDs and option IDs. Other text remains optional; there is no trimming,
label-uniqueness requirement, or mathematical check. Accepted masks must be
nonzero and reference existing options. Guided requires exactly one accepted
option; ArcadeCollect permits any nonempty subset, including every option.
Judgment, progression, and player attempts remain in the question session.

The current CMake graph supplies `paths_guided_tests` through `paths_model`,
and the existing `paths_gallery_tests` covers its gallery consumer. Both built
and passed in `build/question-validation` with `PATHS_BUILD_NATIVE=OFF`:
`ctest --test-dir build/question-validation -R '^paths_(guided|gallery)_tests$' --output-on-failure`.
The focused additions cover valid single/multiple answers, exact diagnostic
locations, duplicates and failure order, required values, invalid masks,
capacity boundaries, mode compatibility, and the separate initial-index check.
Existing gameplay tests also passed. No window was launched.

Production change: two existing files, +109/-24 lines (net +85), no new
production files. Tests add 210 lines in the existing question-model test file;
this workstream record is the only documentation change. The four checkpoint
files were committed locally as `b5078b9` on 2026-09-06. Because Paths had no
prior commit, other existing project files remain untracked. No unresolved
contract decisions remain for this checkpoint.
The accepted-ID conversion helper awaits an importer with a live consumer;
importing, mathematical verification, schema/metadata, generation, scoring,
persistence, and presentation remain separate specifications.

## P008: step completion and prepared working

The user-supplied step-semantics design extends the existing question owner.
`WorkingStateId`, `WorkingState`, `StepPurpose`, `CompletionRule`, and
`StepSemantics` live in `LayeredQuestionSession.hpp`, with their implementation
in the matching source. The existing immutable catalog now holds numbered
working snapshots. Each step references its before and after states. The
former content `workingLine` field and the Guided UI's hardcoded completed
working are removed; the compiled fixtures provide the prepared text.

`validateQuestion()` calls the private `validateStepChain()` in the same
source. It checks snapshot count and unique nonzero IDs, known purposes and
completion rules, resolvable before/after references, and the join
`steps[i].after == steps[i + 1].before`. Failures retain structured field and
question/step locations, with `workingStateIndex` identifying invalid snapshot
definitions. IDs are local to each question and independent of vector order.

The new bounded collection allows 1–33 snapshots, enough for the existing
32-step linear-chain capacity. Empty prepared displays and unused snapshots
within that limit are allowed. Every step must reference existing snapshots;
there is no fallback to the removed field. Display strings are never evaluated
as mathematical code. These references describe a prepared continuation, not
evidence that a transformation is mathematically correct.

`answerSetComplete()` and `requiredAnswerCount()` apply the explicit rule:
AnyAccepted needs one accepted option; AllAccepted needs every accepted option
and remains the default. Step purpose does not determine this rule. Guided
retains select/check/recovery and requires one answer to finish a step, so
multiple alternatives with AnyAccepted are now valid; a multiple-answer
AllAccepted step still requires ArcadeCollect. This deliberately extends the
P007 Guided constraint while retaining its accepted-mask and identity rules.

`judgeOption()` still owns attempts, correctness, and collection. It asks the
shared rule whether the step is resolved. Continue calls the private
`advanceResolvedStep()`. `visibleWorkingId()` and `visibleWorking()` derive
the display from the existing step index and completed flag; no mutable copy
of the current equation was added. Wrong answers, partial sets, repeated hits,
and resolved steps awaiting Continue cannot advance the working. Guided answer
reveals keep their assistance record and established Continue behaviour.

Both the Guided view and gallery board read the session's working. The gallery
also reads the shared required count. Its existing pop delay, pause clock,
frame/challenge guards, and prepare/commit transaction remain intact, so the
next prompt, working, and target assignment become visible together. Endless
play still archives the completed run and resets to the next question.

The current CMake graph provided the focused verification targets. Both
`paths_guided_tests` and `paths_gallery_tests` built and passed in
`build/step-semantics` with `PATHS_BUILD_NATIVE=OFF`. The existing
`paths_input_tests` also passed in `build/gallery-port`, and both `paths` and
`paths_gallery` built successfully there. No visible window or broad test loop
was used. Regression cases cover alternate accepted choices in both modes,
AllAccepted partial sets, operation/calculation working boundaries, assisted
Continue, restart, invalid state definitions/references/joins, the maximum
chain, and gallery publication during popping and after stale shots.

P008 changes six existing production files: +157/-42 lines (net +115), with no
new production files. Two existing test files and this record are also
updated. These new changes remain uncommitted; unrelated files are preserved.
No unresolved contract decisions remain for this checkpoint. Branching routes,
mathematical verification, importing and its accepted-ID conversion helper,
generators, scoring, and persistence remain separate work.

## P009: compiled substitution-integral playtest

The user authorized hardcoded mathematical questions for testing and specified
that other equations will use dedicated files and parsing. The existing
compiled catalog now includes `foundation_substitution_integral_6x`, version 1,
with six player decisions for `Integral of 6x(x^2 + 1)^2 dx`:

| Decision | Accepted answer | Working after Continue |
| --- | --- | --- |
| Choose the substitution | `u = x^2 + 1` | Original integral |
| Calculate `du/dx` | `2x` | Original integral |
| Find the multiplier in `6x dx = ? du` | `3` | `3 * integral u^2 du` |
| Integrate in `u` | `u^3 + C` | `u^3 + C` |
| Substitute back | `(x^2 + 1)^3 + C` | `(x^2 + 1)^3 + C` |
| Differentiate to check | `6x(x^2 + 1)^2` | Keep the final answer; complete |

Each step supplies four options and one accepted answer. The prompts state the
information established by preceding decisions. The four immutable working
states retain the original integral through substitution and differentiation;
the transformed integral appears only after the multiplier step advances.
No fixture-specific judging, progression, parsing, or target code was added.

The existing variation table exposes this fixture as `substitution_chain`.
Launch it with `./build/gallery-port/paths_gallery --start-mode substitution_chain`.
CLI help now reads that table too. Existing presets keep their catalog entries.
Both interaction modes are exercised through the question session; the new
gallery startup uses ArcadeCollect and the existing endless restart.

The dedicated content/parsing boundary is recorded in `ARCHITECTURE.md`.
Future file readers produce the existing content types and call shared
validation before constructing a session. No content format, loader, accepted-ID
conversion helper, or mathematical-expression parser is introduced here.

Verification followed the current CMake graph: `paths_guided_tests` and
`paths_gallery_tests` both built and passed in `build/step-semantics`. The
question test checks the exact six accepted answers, purposes, prepared
working, wrong derivative/multiplier retries, and final verification in both
interaction modes. The gallery test shoots the real presented target meshes
through all six decisions, including a wrong derivative, then verifies the
completed evidence and endless restart.

An independent exact-coefficient check confirmed the derivative of
`(x^2 + 1)^3` is `6x + 12x^3 + 6x^5 = 6x(x^2 + 1)^2`, along with `du/dx = 2x`,
the multiplier 3, and the integration power rule. This checks the authored
fixture; it does not add a general mathematical verifier to the game.

`paths_gallery` built successfully and its help lists the new startup. A
1440x900 offscreen preview was inspected at
`build/substitution-fixture/start.png`; `start.json` records the ready
four-target challenge and the correct question identity. As recorded for P005,
MoltenVK required an unsandboxed headless process on this Mac. No visible
window was opened; interactive pointer acceptance remains separate.

This checkpoint changes four existing production files: +37/-3 lines, net
+34, with no new production files. Two existing test files, README, architecture,
and this record are updated. Changes remain uncommitted and unrelated changes
are preserved. The next content-loading capability awaits its own format and
parsing specification.

## P010: editable question packs through gallery startup

The five gallery fixtures have moved to prepared question JSON, preserving
every original field, question/content version, numeric ID, acceptance set and
working-state transition. A temporary export of the pre-change compiled
catalog was compared field for field with the loaded model serialized back to
the same shape; all five matched exactly. The older `002_guided.json` and
`013_guided.json` remain byte-for-byte unchanged and are not in the new pack.

`QuestionContentIO.hpp/.cpp` is the authorized new boundary. It reads bounded
files with the pinned nlohmann/json library, resolves accepted option IDs only
after option order is established, resolves deck entries by `(id, version)`,
and invokes the existing shared validators. The live startup consumes its
catalog and selected deck before graphics initialization. The compiled gallery
catalog and its mode table's fixed question indices have been deleted.

`GallerySession` receives and freezes its deck; the existing question session
validates and freezes its catalog. The question owner retains judgments,
attempts, completion and working transitions. No renderer, scoring, persistence,
presentation, mathematical solver or generator changes are part of P010.

`--content-pack` selects the authoring pack. The default bundled pack is
deployed beside the executable and resolves independently of launch directory;
card references resolve relative to the selected pack. Editing or adding cards
and changing a deck affects the next launch of the same binary. An active run
keeps its original data. Loading errors identify a source and JSON-pointer
field and terminate startup, with no fallback to compiled gallery content.
The [format contract](QUESTION_CONTENT_FORMAT.md) documents schema version 1,
required fields, enum tokens, existing capacity/interaction rules, deck bounds,
repeat references, optional mode decks and version resolution. No contract
decisions remain unresolved for this checkpoint.

Verification followed the affected current CMake graph:

- Debug `PATHS_BUILD_NATIVE=OFF`: `paths_content_tests`, `paths_gallery_tests`
  and `paths_guided_tests` built and passed in `build/question-content`.
  They cover decoding/structural diagnostics, identity and version resolution,
  reordered options/catalogs, the eighth mask bit and full accepted set,
  invalid deck injection, a frozen active run, and a newly added card/deck in
  the same test executable. Existing gallery tests now load real JSON and
  complete the equality set and all six integral decisions, including a wrong
  derivative retry that preserves the working state.
- Release `paths_gallery` built in `build/gallery-port`.
  `paths_content_startup_tests` passed seven real startup rejection cases:
  missing pack, unsupported pack schema, missing selected deck, missing card,
  unsupported card schema, unknown accepted option ID and malformed JSON.
  These tests run from another directory and reject before requesting graphics.
- A native offscreen run from `build/question-content-evidence/elsewhere`
  found the default bundled pack and completed the integral's six correct
  decisions plus one wrong retry, with zero misses and one completed question.
  It then published the ready endless restart. Evidence: `integral.script`,
  `integral.json`, and `integral.png` under `build/question-content-evidence`.
- After building, a separate editable pack gained `p010_added_after_build` and
  a changed `question_relay` deck. The same executable loaded that relative
  pack path from another directory, completed the new card, and restarted it.
  `new-card.json` records one correct hit, no wrong hits/misses, and a completed
  question with the added identity. `new-card.png` displays its new prompt.
  The executable's before/after SHA-256 is identical:
  `08b6af5378b03fc6c6f808d7c9468d59b1f2b2b8a3b6853d9d4a5399ced25133`.
- Both native screenshots were inspected. MoltenVK used the previously
  documented unsandboxed offscreen route; no visible window was opened.
  Interactive swapchain and human pointer acceptance remain separate.

Changed-file map for this checkpoint:

| Area | Files |
| --- | --- |
| New loader | `src/content/QuestionContentIO.hpp`, `src/content/QuestionContentIO.cpp` |
| Runtime/startup | `src/runtime/first_move/LayeredQuestionSession.hpp`, `src/runtime/first_move/LayeredQuestionSession.cpp`, `src/runtime/gallery/GallerySession.hpp`, `src/runtime/gallery/GallerySession.cpp`, `app/gallery_main.cpp` |
| Content | `content/cards/foundation_equality_24.json`, `content/cards/foundation_relay_add.json`, `content/cards/foundation_relay_notation.json`, `content/cards/foundation_chain_2x_plus_3.json`, `content/cards/foundation_substitution_integral_6x.json`, `content/packs/gallery_foundation.json` |
| Build/tests | `CMakeLists.txt`, `tests/question_content_tests.cpp`, `tests/question_content_startup.cmake`, `tests/gallery_session_tests.cpp`, `tests/first_move_layered_question_tests.cpp` |
| Documentation | `README.md`, `docs/ARCHITECTURE.md`, `docs/QUESTION_CONTENT_FORMAT.md`, `docs/WORKSTREAMS.md`, `docs/MIGRATION_SEED_MANIFEST.json` |
| Dependency | `third_party/nlohmann/json.hpp`, `third_party/nlohmann/LICENSE.MIT`, `third_party/nlohmann/README.md` |

Owned production C++ changes seven files, with two new files: **+316/-91
physical lines, net +225**. This is the requested new file-loading capability,
not a cleanup extraction. Separately, the unmodified vendored JSON header adds
25,526 lines; its MIT license, upstream version and matching release hash are
recorded. CMake, tests, JSON data and docs are excluded from that C++ LOC count.
The P010 comparison uses the pre-change snapshot because much of this
independent repository is still untracked; earlier P008/P009 work is not
included in the delta. All changes remain uncommitted and unrelated files are
preserved. The next candidate is adapting an existing authored source card to
this prepared format under a separate specification.

## P011: card 002 through the existing gallery

The [adaptation record](P011_CARD_002_ADAPTATION.md) describes the prepared
question, source hashes, stable-ID map and answer-exposure review. Card 002,
"Three points, one formula," now has its own runtime JSON and test pack:

```sh
./build/gallery-port/paths_gallery --start-mode equation_chain \
  --content-pack content/packs/source_002.json
```

The runtime keeps the authored question ID and content version 1. Its 13
prompts, four choices per step, correct answers, layer labels, explanations,
recovery text and before-workspaces are preserved. Fixed numeric IDs map the
authored string identities into the existing model. Each step uses
`any_accepted`; its explicit before/after chain leads to the next authored
workspace. The terminal state retains the source's ordered solution lines.
The board displays the problem, guided-adaptation label and Meckes & Meckes
chapter/exercise attribution. The original authoring and source files remain
unchanged.

`paths_content_tests` and `paths_gallery_tests` were derived from the current
CMake graph, built, and passed in `build/question-content`. The former compares
prepared content to the authoring source and checks the final solution state.
The latter completes all 13 decisions via presented meshes, with a wrong/correct
pair in every step. It checks preserved working during retries and popping,
stale shots, stable evidence identities and clean endless restart. Existing
foundation regressions in those targets also passed. An independent
exact-rational solve verified `(a,b,c)=(3/2,1/2,0)`, determinant `-2`, and the
three observations, agreeing with the earlier planning receipt.

The native gallery target built with content deployment only. The executable
is byte-identical to the pre-P011 binary. From another working directory, it
loaded the separate pack and completed a 70-frame offscreen run: 13 correct,
13 wrong, zero misses, one completed question, then a ready endless restart.
`build/card-002-evidence/` contains scripts, reports, screenshots, the exact
arithmetic receipt and executable hash. Captures at steps 1, 3, 8, 11 and 13
were reviewed at 1440×900; all choices and current working were readable.
MoltenVK used the established unsandboxed offscreen route. No visible window
was opened, and interactive acceptance remains separate.

Changed files:

- New `content/cards/source_002_quadratic_three_points.json` and
  `content/packs/source_002.json`.
- Extended `tests/question_content_tests.cpp` and `tests/gallery_session_tests.cpp`.
- New `docs/P011_CARD_002_ADAPTATION.md`; updated `README.md`,
  `docs/ARCHITECTURE.md`, `docs/QUESTION_CONTENT_FORMAT.md`, this workstream
  record and `content/SOURCE_CARD_ARCHITECTURE.md`.

Production C++ delta: **+0/-0 lines, zero files added or changed**. Gameplay,
loader, schema and build wiring remain unchanged. No conflicting production
route was introduced. The original 002/013 authoring cards, foundation pack
and unrelated files are preserved. Changes remain uncommitted; no contract
decisions remain unresolved. The next candidate is a prepared card 013
adaptation. P003's multi-card Guided navigation, scoring/timing and saved
profiles remain separate capabilities.

## P012: card 013 guided derivation through the file-loaded gallery

[P012_CARD_013_ADAPTATION.md](P012_CARD_013_ADAPTATION.md) records the source
hashes, explicit ID map, answer exposure, mathematical argument and native
evidence. The new `source_013.json` pack selects
`source_013_closest_point_line.json` through the existing `equation_chain`
startup. It preserves all 14 authored prompts, choices, accepted answers,
working blocks and explanations under the original question ID/version.

The problem already supplies its target coordinates. The persistent board
retains them and labels the exercise guided derivation. The player selects
valid parameter roles, projection conditions, calculations and the argument
for closest and unique in Euclidean distance. Completion does not claim
independent proof writing; finite examples do not substitute for the general
perpendicular-residual and nonnegative-distance-gap justification.

The existing question session remains the correctness, working and evidence
owner. Shared test-only checks now exercise both source cards, with explicit
expectations for each. The CMake-derived `paths_content_tests` and
`paths_gallery_tests` targets built and passed in `build/question-content`.
They cover the preserved authoring, final solution state, all 14 wrong/correct
pairs, unchanged working during retry/pop, stale shots, archives and clean
endless restart. Existing 002 and foundation regressions passed as well.

An exact-rational check agreed with all 12 prior planning examples and passed
60 direct squared-distance comparisons. It includes zero/negative slopes,
the origin and points already on the line. The general real-parameter proof
is retained in the adaptation record and authoring artifact.

`paths_gallery` built with content deployment; its executable is byte-identical
to the pre-P012 binary. From another working directory it completed a 75-frame
offscreen run: **14 correct, 14 wrong, zero misses, one completed question**,
then a ready endless restart. `build/card-013-evidence/` contains scripts,
reports, screenshots, exact-arithmetic results and the executable hash.
Captures for steps 1, 5, 7, 11, 12 and 14 were reviewed at 1440×900; all choices,
formulas, current working and attribution fit. No visible window was opened.
Human pointer and interactive swapchain acceptance remain separate.

Changed files:

- New `content/cards/source_013_closest_point_line.json` and `content/packs/source_013.json`.
- Extended `tests/question_content_tests.cpp` and `tests/gallery_session_tests.cpp`, sharing the source-card test routines.
- New `docs/P012_CARD_013_ADAPTATION.md`; updated `README.md`, `docs/ARCHITECTURE.md`,
  `docs/QUESTION_CONTENT_FORMAT.md`, this record and `content/SOURCE_CARD_ARCHITECTURE.md`.

Production C++ delta: **+0/-0 lines, zero files added or changed**. The loader,
schema, gameplay and build wiring are unchanged; no competing route is added.
Both original authoring cards, snapshots, the foundation/002 packs and unrelated
work are preserved. Changes are uncommitted, with no unresolved contract
decisions for this checkpoint. The next candidate is a gallery pack picker
so these sets can be selected without launch arguments. P003's Guided grid,
scoring/timing and saved profiles remain separate capabilities.

## P013: choose questions in the gallery

[P013_QUESTION_MENU.md](P013_QUESTION_MENU.md) records the controls, ownership,
verification and limits. The default gallery startup now offers Starter
questions, Three points, one formula and The closest point on a line. The
starter's available practice types come from its loaded decks. Play starts a
new game or resumes the selected pack/type. Pause → Choose questions returns
to the menu with that session frozen. The developer workshop remains reachable
from the menu and by its explicit startup argument.

`GalleryMenu` is the app owner of selection, screen and session lifetime.
Its shared `launch()` prepares a session for both CLI startup and the Play
action; the former independent constructor block in `gallery_main.cpp` is
removed. The JSON loader, gallery and question model remain their respective
content, gameplay and evidence owners. UI navigation is applied between native
frames to keep each scene pointer and its visible board consistent.

The new `paths_gallery_menu_tests` target and existing `paths_gallery_tests`
passed with native dependencies disabled. They verify all three packs, starter
practice types, retained partial collection and wrong attempts, separate
sessions, pause/resume, invalid selections, recoverable broken/missing files,
repaired-file retry, frozen content and direct custom-pack startup. Existing
source-card and foundation regressions passed. The native
`paths_content_startup_tests` also passed, retaining the seven pre-graphics
content-error cases.

The rebuilt native executable passed ten bounded offscreen scenarios, including
the default menu at 1440×900 and 800×600, each question set, switching/resuming
all three sets, returning to the menu and workshop, the prior full 14-step
direct launch and the existing workshop script. A copied executable without
content displayed a recoverable error screen. Captures of the menu, Resume,
resumed game, workshop return and error screen were reviewed. The switching
script preserves a partial collection and an unfinished pop, then completes
the starter and resumes both source cards at step 2 with their wrong attempts.
Evidence is in `build/menu-evidence/`; the repeatable scenario is
`tests/gallery_menu.script`. No visible window was opened.

Production C++ delta: **+292/-40 lines (net +252)** across the existing startup
and two new app-menu files. Build wiring adds the pure menu/test targets. New
tests add 126 C++ lines and one native script. README, architecture, this ledger
and the new checkpoint record document the feature. This is an explicitly
requested UI capability, not an ownership cleanup; the new module has live UI
and CLI consumers. Content files, gameplay owners, renderer and unrelated work
are unchanged. Changes are uncommitted. No contract decisions remain unresolved
for this checkpoint; human pointer and interactive swapchain acceptance remain
separate from these checks.

The next candidate is movement-pattern and speed controls beside Play, using
the existing route/configuration owners. Saved profiles and scoring remain
separate capabilities.


## P014: movement setup before Play

[P014_MOVEMENT_SETUP.md](P014_MOVEMENT_SETUP.md) records the complete contract
and evidence. The menu exposes thirteen existing gameplay presets and a speed
slider. CLI values initialize the draft. Play freezes its choices into the
new GallerySession; Resume presents that session's settings with editing
locked. A new set/type continues using the latest pending draft.

`GalleryMenu` owns that draft and dispatches `SelectMotion`/`SetPace`.
`validateGalleryConfig()` in the existing gallery module is shared with game
construction and delegates route limits to TargetMotion. The constructor-only
preset check is replaced; no parallel route calculator or judgment path was
added. Existing question content, gameplay evidence and renderer remain intact.

The current build graph selected `paths_gallery_menu_tests`,
`paths_gallery_tests` and the native `paths_gallery` build. All passed. Focused
tests exercise every preset with actual movement and judged clicks, speed
boundaries, invalid inputs, seeded assignment compatibility and exact Resume
state. Eleven successful native scenarios and two deliberate rejection checks
passed their report/log assertions. Menu and Resume controls fit at 800×600;
1440×900 selection and the moving answer board were also reviewed. Evidence:
`build/menu-motion-evidence/verification.json`. No visible window was launched.

Production C++: **+89/-21 lines, net +68**, five existing production files,
zero added. Tests: **+102/-2 C++ lines, net +100**, plus one native script.
README, architecture and the new checkpoint document the feature. P014 remains
uncommitted; no unresolved contract decisions remain. Interactive pointer feel
and swapchain acceptance are still separate. The next candidate is an explicit
New game action to choose another setup for an already-started set/type.

## P015: content folder organization

[content/README.md](../content/README.md) is the editing guide. Playable JSON
belongs in `content/cards/`, explicit file lists and decks in `content/packs/`,
and the older 002/013 authoring cards now live in `content/authoring/`.
Source snapshots retain their read-only provenance role. The two moved files
preserve their original bytes; snapshot paths inside them still resolve from
the content root. No duplicate cards remain at their old source paths.

The planning validator and existing authored-to-playable content tests now
read from `authoring/`. Adaptation links, research/proposal references, the
source-card architecture and migration destination entries follow the move.
Migration source paths and hashes remain the original seed evidence. The main
README and architecture folder map point to the current editing path and
describe the gallery menu, gameplay and workshop startup accurately.

Verification: the current CMake graph identified `paths_content_tests`, which
rebuilt and passed, including both source-card mappings and loading/progression
checks. The existing planning validator passed seven snapshot hashes, 27
authored decisions and its specified arithmetic/reference cases; its receipt
was refreshed. `paths_gallery_content` refreshed the local native bundle, with
the two unchanged obsolete authoring copies removed from its old cards folder.
All three deployed pack lists resolve to unchanged playable cards. File hashes,
references and local documentation links passed checks. No native window or
broader test loop was needed for this data-path move.

Changed: two authoring cards moved; one path each in the Python validator and
C++ content test changed; the planning receipt, migration destinations, folder
guide and affected documentation were updated. Production C++ delta for P015:
**+0/-0 lines, zero production files added or removed**. Test C++ and the
planning script each change one line with zero net LOC. Playable content,
packs, snapshots and all existing P014 production changes are byte-preserved.
Evidence is under `build/content-organization-evidence/`.

Changes remain uncommitted. No contract decisions remain unresolved. The next
gameplay candidate is still New game for another setup on a started set/type.

## P016: New game

[P016_NEW_GAME.md](P016_NEW_GAME.md) records the completed contract and evidence.
New game opens an editable copy of the selected session's settings. Cancel
preserves the original run. Start new game explicitly replaces its progress
and answer history, after the current pack and fresh session prepare
successfully. Other pack/practice slots are preserved. Failed loading keeps
the old run and replacement draft available for cancellation or retry.

`GalleryMenu` owns this state and reuses one private preparation route for
initial games and replacements. The startup presents the controls and forwards
actions between frames. Question, scene, motion and renderer owners are
unchanged. Production C++: **+75/-29 lines, net +46**, three existing files,
zero new production files. The menu test adds **+90/-1 lines, net +89**;
one native script and documentation record the feature.

The current build graph selected `paths_gallery_menu_tests` and the native
`paths_gallery` target. Both built; the focused tests passed. Six native
offscreen scenarios passed, including small/large setup, small Resume,
cancellation, replacement with another game retained and the existing
three-pack progression script. JSON assertions and reviewed screenshots are
under `build/new-game-evidence/`. Controls fit at 800×600. No visible window
was opened; human pointer and swapchain acceptance remain separate.

Changes are uncommitted. No contract decisions remain unresolved. A useful
next candidate is an in-game answer-review screen using the existing attempt
records; scoring and persistence still need their separate specifications.

## P017: Answer review

[P017_ANSWER_REVIEW.md](P017_ANSWER_REVIEW.md) records the completed contract and
evidence. Pause → Answer review opens the current question's reached steps.
Expandable rows retain their names when collapsed, show working and submitted
answers in order, and distinguish in-progress, first-try and retried results.
Wrong clicks and steps needing another try have separate counts. Completed
questions from this game are available in the selector. Unreached steps and
unsubmitted answer keys are not exposed. Back to game retains the pause.

`LayeredQuestionSession::review()` projects the existing frozen content and
attempt records. `GalleryMenu` owns selection and navigation; the startup
renders the projection and blocks gameplay input until review closes. No
parallel judging, history store, scoring or persistence route was introduced.
New game clears the replaced game's history through the existing session
replacement boundary. Five existing production C++ files change; zero are
added. P017 contributes **net +162 production lines** after the documented
P016 net +46, for **net +208 combined** against `6a9f1c8`.

This checkpoint resumes the builder's unfinished P017 changes and finishes
the collapsed-row names, verification and documentation. The current CMake
graph selected `paths_guided_tests`, `paths_gallery_menu_tests` and native
`paths_gallery`. Both focused tests passed and the native target rebuilt.
Five offscreen captures/reports and two deliberate input-rejection cases
passed. Reports verify partial collection, completed history, reached-step
filtering and identical game/target state after Back. The 1440×900 multi-step
screen and 800×600 screen were visually reviewed. Evidence and repeatable
report assertions are in `build/answer-review-evidence/resumed/`.

Changes remain uncommitted, including the retained P016 work. No visible window
was launched; interactive pointer and swapchain acceptance remain separate.
The next candidate is prepared explanations for resolved steps in review,
with explicit rules for unfinished and multiple-answer steps.

## P018: explanations in Answer review

[P018_REVIEW_EXPLANATIONS.md](P018_REVIEW_EXPLANATIONS.md) records the completed
contract and evidence. Resolved steps now display their prepared Explanation
below the submitted attempts. All 39 playable steps already contain this text.
Wrong answers, partial answer sets and fresh runs do not unlock it; resolved
earlier steps and completed questions retain it. Empty authored strings remain
valid and omit the section.

`LayeredQuestionSession::review()` reuses `layeredQuestionStepResolved()` to
expose the frozen question/version's explanation. The UI renders that view and
the native report records availability. No alternate completion, content or
history route was added. Production C++: **+5/-2 lines, net +3**, three existing
files and zero new production files, measured against the saved P017 working
state. C++ tests: **+32/-3 lines, net +29** across two existing files, plus one
repeatable native script.

The current CMake graph selected `paths_guided_tests`,
`paths_gallery_menu_tests` and native `paths_gallery`. Both tests passed and
the native executable rebuilt. Four bounded offscreen scenarios verify hidden
partial-set text, completed-history explanations, and an earlier resolved step
beside an unfinished step at 1440×900 and 800×600. Existing question evidence,
targets and clocks match the corresponding P017 reports. Both layouts were
visually reviewed; the smaller panel keeps overflow inside its scroll area.
Evidence is under `build/answer-explanation-evidence/`.

Changes remain uncommitted; P016/P017 and unrelated work are preserved. No
visible window was opened. Interactive pointer and swapchain acceptance remain
separate. The next candidate is a focused retry round for missed questions,
with fresh attempts recorded separately from the original completed runs.

## P019: continuous arcade play

[P019_CONTINUOUS_PLAY.md](P019_CONTINUOUS_PLAY.md) records the user's corrected
priority: keep clicking, receive a small wrong-answer signal, and continue the
same question. Errors must not interrupt the game or create correction work.
Totals belong after stopping; detailed review is optional. This supersedes the
missed-question retry candidate recorded after P018.

The existing gallery already kept wrong answers active. This change removes
its persistent correction message and live totals, adds a half-second hit
indicator, and makes Stop/Esc reveal the accumulated totals. Resume continues
the same game and hides the numbers. The mode remains endless. No timer or
new point formula is implied by this checkpoint.

`GallerySession` owns typed feedback and its expiry against the existing scene
clock. The question model keeps judging and progression; `GalleryPause` keeps
Stop/Resume. The UI presents the signal and stopped totals. Persistent coaching
strings and the unused per-question display clock are deleted. Production C++:
**+38/-20 lines, net +18**, three existing files, zero new production files,
measured against the saved P018 working state. The gallery test adds 37 lines;
one native script captures repeated errors followed by uninterrupted completion.

The current CMake graph selected `paths_gallery_tests` and native
`paths_gallery`. Both rebuilt and the focused tests passed, including moving
targets through repeated errors, immediate subsequent hits, feedback expiry,
Stop/Resume and the existing question/collection regressions. Five offscreen
scenarios passed for wrong feedback, expiry, stopped totals, resumed play and
800×600 results. Active and stopped layouts were visually reviewed. Evidence:
`build/continuous-play-evidence/verification.json`.

Changes remain uncommitted and previous work is preserved. No visible window
was launched; interactive feel remains for a player to assess. The user's
subsequent direction is to establish fast, sustained endless play, then format
different levels of maths for playtesting. A whole-run countdown and richer
visuals remain later options. Forced correction rounds are not part of this loop.
