# Checked question batches

**Linear practice now uses this same workflow.** Package `linear_repetitions`
version 1 adds 12 questions and six readings under **Algebra → Worked linear
practice**. The library has 354 questions and 948 readings. All 342 earlier
question records and 942 reading records remain unchanged. Both families use
one compilation, model-replay and publication path; generated appearance still
awaits the user's visual check.

## Edit generated wording in live preview

The [draft command](LEARNING_EXPORTS.md#create-an-editable-preview-draft) copies
one generated authoring package into a separate editable folder:

```sh
python3 -B tools/export_learning.py draft build/question-batches/linear_repetitions/1/authoring \
  --output content/authoring/drafts/linear_practice
./b/sorter --documents content/authoring/drafts/linear_practice/documents --watch-documents
```

This linear draft is already present; launch it with the second command.
Edit its six `.paths.md` files, not the immutable files under
`build/question-batches/`. Repeating draft creation never overwrites edits.
The command prints absolute edit paths and the launch command, preserves source
attribution, and copies only the compiler's entry/include closure. Preview is
session-only; it does not publish or load personal progress. Once a wording
change is approved, it can be applied to the shared template for future batches.

## Linear equations

Build and publish from the Paths root:

```sh
cmake --build b --target sorter paths_learning_document_tests --parallel 4
python3 -B tools/build_question_batch.py --family linear --publish
./b/sorter
```

Omit `--publish` to export only. Linear defaults to package version 1 and format
version 1; the default family remains matrix. There are two cards in each group:
Positive integers, Negative coefficients, Negative offsets, Negative solutions,
**Zero as the solution**, and Fractional solutions. The zero group means a unique
answer of x=0, not an equation with no solution.

The existing `question_workflow.py` recipe supplies the bounded `ax+b=c` family:
real x, integer a with `2 <= abs(a) <= 9`, nonzero integer b with `abs(b) <= 9`,
and an exact constant c. It independently checks balance, every choice, the
solution set at each step and substitution into the original equation. The new
producer adapts those checked questions to `linear.v1`; it does not introduce
another recipe, answer checker or runtime schema. The earlier 25 linear cards
still reproduce byte for byte and retain their original identities.

Linear `--count` accepts multiples of six from 6 through 36. Selection uses a
fixed 36-question pool, interleaved across all six cases. Increasing the count
preserves earlier question identities, text and display numbers:

```sh
python3 -B tools/build_question_batch.py --family linear --count 24 --version 2 --publish
```

These are repetitions of the same two-step skill, not six independently mastered
skills. Some revisit existing recipe instances with fuller explanations under
new identities. Existing questions stay in their original packages; this linear
package does not copy them. Defaults write to
`build/question-batches/linear_repetitions/1/authoring` and its sibling `export`.

The reusable wording is
[`linear_reference/question.paths.md.in`](../content/authoring/learning/linear_reference/question.paths.md.in).
It uses the same bounded substitution as the matrix template, with these fields:

| Fields | Meaning |
| --- | --- |
| `id`, `title`, `given`, `given_tex` | Identity, readable title, plain and typeset starting equation. |
| `a`, `b`, `c`, `rhs`, `answer` | Exact typeset values; rhs is the right-hand side after removing the offset. |
| `prompt_1`, `offset_note` | A short first-step cue and the appropriate positive/negative-offset explanation. |
| `remove_left`, `remove_right`, `check` | The operation on each original side and substitution into the original equation. |
| `choices_1..2`, `after_1..2`, `equation_1..2` | Choice/answer directives, plain reached states, and typeset reached equations. |

Use spaces around template fields inside TeX groups, for example
`\frac{ {{rhs}} }{ {{a}} }`, so literal grouping is distinct from `{{field}}`.
Unknown fields name the source template and line. The audit records both recipe
and template digests. Keep Terms neutral, Hint directional and `@why` in concise
ordinary prose; detailed mathematics belongs in `@teaching`. Practice remains
multiple choice with optional typing, and all four support levels use one workflow.

The current linear evidence is `build/linear-batch-evidence/verification.json`.
Release builds and four targeted CTest entries pass, covering the shared document
gate, four-level sessions, original authoring workflow and ten-test batch suite.
The final shared uniqueness check is followed by a passing batch-suite rerun.
The 12 published linear cards pass 60 routes, 144 wrong-response checks and 384
disclosure checks. A six-card save reopens after extension to twelve with completed
work, selected question, draft, help, level and Undo branches retained. Publication
repeat, malformed source, unknown template fields and attempted removal are checked.
Both published matrix formats remain byte-identical, including their audit files.

## Matrix equations

Status: **package version 2 locally published**. The user approved applying the
[matrix reference](QUESTION_PRACTICE_FORMAT.md#editable-matrix-reference) to the
batch. Twelve worked cards join the twelve original cards. That checkpoint
brought the library to 342 questions and 942 readings, preserving every earlier
question stamp and reading record. Generated variations await the user's visual check.

## Build, generate and publish

From `/Users/kogaryu/iggy3d/paths`:

```sh
cmake --build b --target sorter paths_learning_document_tests --parallel 4
python3 -B tools/build_question_batch.py --publish
./b/sorter
```

Omit `--publish` to export without activation. In Library → Linear Algebra →
**Worked matrix practice**, choose Integer solutions, Negative solutions or
Fractional solutions. Each reading links four questions, with titles such as
**Fractional solutions · Exercise 1**. Learn opens definitions and worked
calculations. Practice keeps symbolic choices and optional typing. Terms, Hint,
Next line and Solution open separately; completion waits for Next.

**Matrix repetitions** retains the original cards. Worked cards revisit those
same systems with new content identities and separate attempts. An old save
never acquires different teaching under its frozen question identity.

The command independently checks exact mathematics, compiles documents through
`LearningDocuments`, replays all five answering routes through the real question
owner, then uses `export_learning.py` for export/publication. Failures exit
nonzero with structured JSON; document diagnostics retain file, line, field and
reason. Failed generation never falls back to a previously successful export.

## Versioning and outputs

Defaults are `--format-version 2 --version 2 --count 12`, with output in
`build/question-batches/matrix_repetitions/2/`:

| Path | Contents |
| --- | --- |
| `authoring/authoring.json` | Immutable package identity and original-generation provenance. |
| `authoring/documents/*.paths.md` | Three original documents retained byte for byte, plus three `worked_*.paths.md` documents. |
| `authoring/audit.json` | Frozen parameters, choices, answers, full working, independent checks and template digest. Authoring evidence, not learner text. |
| `export/` | Portable package, inventory and target receipt from the existing exporter. |

`--count` accepts multiples of three from 3 through 36 **per format**. Format 2
retains that many original cards and adds that many worked cards, equally split
among the three groups. Count 24 therefore produces 48 cards in this package;
upgrading from count 12 adds 12 more originals and 12 more worked cards:

```sh
python3 -B tools/build_question_batch.py --count 24 --version 3 --publish
```

Within each format, smaller batches are stable prefixes. IDs depend on givens
and format identity, not batch size or display position. Existing question text
and stamps remain identical on extension. Publishing fewer questions over a
larger batch is rejected. Teaching changes require a new question identity while
retaining the original; a higher package version alone cannot rewrite it.

Every invocation regenerates and checks before using output. Identical bytes
reuse immutable directories. Changed bytes at the same output/version reject.
Repeating publication preserves the active generation and its modification time.
Normal rebuilds retain `b/learning-store`.

`--format-version` selects the teaching format independently of package version.
Format 2 requires package version 2 or later. Format 1 reproduces the original
producer, which remains a live consumer for retained documents and old saves.
To reproduce it after upgrading the normal store, use an isolated output/store:

```sh
python3 -B tools/build_question_batch.py --format-version 1 --version 1 \
  --store build/v1-reproduction-store --output build/v1-reproduction
```

This exports only. An old package version cannot replace an upgraded store.
Frozen byte hashes protect the original twelve-card documents in the tests.
`--output`, `--store`, `--base-documents`, `--target` and `--model` also support
isolated work. There is no second store, lock or activation implementation.

## Reusable Markdown wording

The source is
[`question.paths.md.in`](../content/authoring/learning/matrix_reference/question.paths.md.in).
It derives from the approved reference and owns the step prompts, definitions,
hints, feedback, reasons and worked-teaching prose. The producer inserts bounded
fields; the app consumes ordinary `matrix.v1`/`lesson.v2` documents through the
existing compiler. This adds no runtime template engine or document parser.

`{{name}}` inserts a generated value. Available names are:

| Fields | Meaning |
| --- | --- |
| `id`, `title`, `given` | Stable identity, readable title and plain starting matrix. |
| `p`, `k`, `d`, `minus_p`, `minus_k`, `x`, `y` | Exact typeset parameters and solutions. |
| `choices_1..3` | Complete numeric choice and answer directives for each step. |
| `after_1..3` | Plain reached matrices for the mathematical owner. |
| `matrix_1..3`, `operation_1..3`, `calculation_1..3` | Typeset reached matrices, operations and all three column calculations. |
| `check_1..2` | Substitution into each original equation. |

Unknown fields name their template line. Malformed fields reject generation
before installing output. Substitution performs no evaluation or code execution.
The audit records the template digest. The original reference example remains
unchanged; this parameterized source owns future generated wording.

Keep Terms neutral and Hint directional. Learn shows the actual operation,
conditions, every column calculation, reached matrix and final substitution.
The producer supplies signs and exact fractions. It validates mathematics and
source structure, not prose quality or rendered legibility.

For individual live edits, copy generated documents to a fresh authoring folder
and use `--documents THAT_FOLDER --watch-documents`. Edit that draft, never the
immutable generated/exported files. Draft preview neither publishes nor saves
attempts. Editing a shared template does not overwrite published teaching:
changed content still needs new question identities and retained originals.

## Mathematics and verification

The bounded family starts with coefficient rows `[1,p]` and `[k,kp+d]`, for
nonzero bounded integers p and k and d in {-3,-2,2,3}. Its determinant d is nonzero.
The three operations add -k times row 1 to row 2, divide row 2 by d, then add -p
times row 2 to row 1. These are repetitions of one skill family; arithmetic groups
do not represent three separate skills or complete curriculum coverage.

Construction starts from known solutions. Independent exact arithmetic uses
Cramer's rule on the original givens, checks every intermediate row and applies
all candidate operations. Exactly one distinct choice reaches each declared
matrix. Correct positions vary deterministically. The runtime exact checker then
validates the emitted documents. No authoring report can mark an attempt solved
independently of `LayeredQuestionSession`.

Current evidence is `build/worked-batch-evidence/verification.json`. Two targeted
CTest entries pass: document/model regressions and the eight-test batch suite.
The suite covers original byte stability, independent arithmetic, export, publish,
repeat, extension, removal/stale-output rejection, template signs/structure and
unknown-field rejection. It also saves completed and partial original questions,
then reopens them against the upgraded catalogue with selection, working, draft,
help, level and Undo branches retained.

The published 24-card package completes **120 solving routes, 432 wrong-response
checks and 528 help-disclosure checks**. The full app catalogue preserves all
330 earlier question records and 939 reading records. No screenshots, images,
windows, native rendering harnesses, font probes or personal saves are used.

Manual check: one card from each arithmetic group; **gold Given** beside **cyan
Working/choices**, distinct **purple Help** disclosures, readable fractions and
matrices in Learn, and **green completion until Next**. Close/reopen once during
Practice to check normal saved progress. New families, generalized source-card
conversion, verified whole-corpus taxonomy, 3D assets and per-distractor feedback
routing remain separate work.
