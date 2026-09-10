# Checked question batches

**Linear, matrix and finite probability now share chapter assembly as well as
compilation, model replay and publication.** Subject-specific templates supply
the teaching; the existing native textbook supplies its presentation. The
probability chapter now supplies 12 numerical repetitions, six reasoning
questions and four readings. The earlier linear, matrix and probability
format-1 documents, manifests and audits reproduce byte for byte.

**Worked matrix practice → Reason about row operations** adds the same six roles
for matrices, with a separate numbered reading. Its `matrix_reasoning_practice`
package adds six questions to the existing chapter. Matrix and probability
sequences share `reasoning_documents()` and the certificate boundary; each
supplies its own cases, Markdown and mathematical checks. The existing
probability format-2 package also reproduces byte for byte.

```sh
python3 -B tools/build_question_batch.py --family matrix-reasoning --publish
```

This command defaults to six questions, package version 1 and format version 1.
The cards use the existing prepared multiple-choice route; numerical matrix
repetitions retain their four support levels. The [matrix role contract and
answer sheet](EXERCISE_ROLES.md#matrix-row-operation-sequence) specify case bounds,
source locations, checking responsibilities and the preview workflow.

The [exercise-role contract](EXERCISE_ROLES.md) makes the six decisions reusable
across authoring families: read notation, follow a worked example, choose a
step, explain a step, repair a mistake and solve in a fresh setting. The contract
is authoring metadata; the emitted questions still use ordinary `.paths.md`.

## The reusable authoring boundary

| Write once | Supply for each mathematical family |
| --- | --- |
| `content/authoring/learning/chapter.paths.md.in`: subject/chapter, numbered lesson, practice links and question placement | Stable subject/chapter IDs, ordered groups, titles and reading IDs |
| `teaching_documents()` and `fill_template()` in `tools/build_question_batch.py`: bounded text substitution and precise unknown-field diagnostics | `lesson.md.in`, `question.paths.md.in` and the adapter that supplies their fields |
| `choices_text()`: numeric choice IDs, symbolic labels and accepted-choice directive | Checked exact values, deterministic order and one accepted index |
| Existing `LearningDocuments` compiler and `LayeredQuestionSession` | A supported response template and a mathematical certificate for every answer |
| Existing `export_learning.py`: capture, immutable package, validation and atomic activation | Original-source attribution, package identity and revision |

This is the existing `.paths.md` grammar, not another parser. Reading templates
use `lesson.v2` blocks: introduction, definition, proposition, example, exercise
and summary. Proof, Hint, Answer and Solution remain separate closed disclosures.
Layout, typography, navigation, exercise placement and 3D providers stay with
the existing native textbook. No family supplies its own screen implementation.

The shared chapter template emits the same wrapper for all three current
families. The two old reading introductions have moved out of Python into
`linear_reference/lesson.md.in` and `matrix_reference/lesson.md.in`. Their
question templates retain the previously authored explanations. Frozen matrix
format 1 remains reproducible because published packages and upgrade checks
still consume it; the shared template serves matrix format 2.

A shared template edit affects **future generation**, not already installed
immutable packages. Use an editable draft for live Markdown preview. The batch
report fingerprints the generator, recipe and template inputs and rejects edits
that occur during verification. Historical audit bytes remain unchanged.
Never overwrite a published question or silently regenerate it with new teaching
under its old identity. Changed question content needs a new identity/version
and a package retaining old questions; increasing the package number alone does
not authorize changing an old question's content stamp.

## Finite probability pilot

Open **Library → Probability and Statistics → Finite probability: count and
compare**. The readings progress through **Count an event**, **Count its
complement**, and **Impossible or certain**. Each has four linked exercises.
These are repetitions of one counting skill with three case groups, not three
independent mastery claims. There are two steps per exercise: count the event,
then choose its exact probability. Answers and worked examples use the existing
native LaTeX renderer and symbolic buttons.

The fourth reading, **Reason about probability**, adds six distinct decisions:
interpret the event, complete a worked denominator, choose a calculation for
unequal chances, justify a complement, repair the first incorrect line, and
solve an uncued token problem. The original three readings keep their original
uniform-draw scope. All six role questions remain multiple choice and include
specific corrections. The first correct button appears twice in each position
across the six cards. See [the source, certificates and authoring rules](EXERCISE_ROLES.md).

The question defines one equally likely draw from labels 1 through n. The event
selects labels divisible, or not divisible, by a given positive integer. The
recipe fixes 12 ordinary cases; the boundary variants use divisor n+1. The pool
contains 36 questions, with a stable interleaved prefix for counts 3 through 36
in multiples of three. The initial 12 include probability zero and one.

Construction uses integer division to count multiples. An independent oracle
enumerates the original labels, selects the event, and sums exact `Fraction(1,n)`
weights. It checks both step keys, all distractors and total probability one.
After the actual C++ compiler and model replay, another check compares the
compiled givens, reached states, choice labels and accepted IDs with that
certificate. Incorrect mathematics in an otherwise valid document therefore
cannot pass this producer by relying on the structural `choices.v1` compiler.

Probability uses **multiple choice only**. It does not claim the linear/matrix
written checker, four support levels, arbitrary-event validation, conditional
probability or simulation. The reasoning sequence includes one bounded example
with explicit unequal outcome weights. An arbitrary hand-edited `choices.v1`
file outside this producer still has an authored answer key; the importer does
not independently prove its science or mathematics.

The original lesson explains each symbol, cardinality, divisibility, equal
chances, the counting condition, complements, exact fractions and boundary
cases. Its worked example uses lettered tokens distinct from the numerical
practice questions. Definitions were checked against
[OpenStax, Introductory Statistics 2e, section 3.1](https://openstax.org/books/introductory-statistics-2e/pages/3-1-terminology).
No exercise text was copied. Source attribution is retained in `authoring.json`.

From the Paths root:

```sh
cmake --build b --target sorter paths_learning_document_tests --parallel 4
python3 -B tools/build_question_batch.py --family probability --publish
./b/sorter
```

The current package is `finite_probability_practice` version 2, using format 2.
It retains all twelve format-1 questions and adds the six role questions. Omit
`--publish` for export only. `--count` still counts numerical repetitions; the
six role questions are appended once. Use `--count 24 --version 3` to produce
30 questions. `--format-version 1 --version 1` reproduces the frozen original
authoring files; it cannot replace an installed version containing the roles.
Draft the published source to a fresh folder for live editing:

```sh
python3 -B tools/export_learning.py draft build/question-batches/finite_probability_practice/2/authoring \
  --output content/authoring/drafts/probability_roles
./b/sorter --documents content/authoring/drafts/probability_roles/documents --watch-documents
```

Draft creation refuses an existing destination. Saving its Markdown refreshes
the preview; the preview uses no personal progress and does not publish.

## Contract for the next family builder

For an assigned parallel subject pilot, use the
[parallel authoring packet](PARALLEL_QUESTION_AUTHORING.md). The worker authors
and checks its isolated candidate; the coordinator performs shared registration,
upgrade checks and publication in steps 5–6 below. Do not have four workers
edit this tool's registry or activate packages concurrently.

1. Name one learning outcome and the existing response checker. Supply the
   domain, exact inputs, ordinary/contrast/boundary cases, expected answers and
   original-source attribution before generating repetitions. Do not infer a
   new solver from a subject name.
2. Reuse the shared chapter wrapper and native lesson blocks. Write the family
   reading and question templates in its authoring folder. Explain notation,
   assumptions and why each operation is valid; use a separate worked example.
   Keep full solutions inside disclosures or the already accepted Learn route.
3. Supply a bounded deterministic recipe, stable IDs and a field adapter.
   Keep earlier instances and their display numbers unchanged when extending
   the pool. Every distractor must be mathematically false and have a useful
   correction; avoid numerically equivalent choices.
4. Supply a genuinely separate mathematical check from the original givens.
   Check the compiled content as well when using authored answer keys. The
   producer refusing a bad certificate is part of the release gate; runtime
   choice replay alone cannot establish mathematical correctness.
5. Register the builder and its route/check/input metadata in the existing batch
   tool. Reuse compiler, model gate and exporter. Do not add a parallel parser,
   renderer, save path or automatic promotion around them. A new 3D interaction
   requires an explicit existing-provider binding with the asset worker.
6. Run the two focused model/pipeline tests below, verify a repeat and an additive
   upgrade, then publish the bounded pilot through the existing command. Record
   preserved catalogue stamps and the exact checks. Finish with the user's
   visual/teaching check before treating the content as accepted.

```sh
ctest --test-dir b -R '^(paths_learning_document_tests|paths_question_batch_tests)$' --output-on-failure
```

These tests create no windows, images, ImGui contexts or fonts. They cover the
three-family template edit, historical byte preservation, every generated
response, wrong-answer preservation, explicit Next, save replay, reordering,
archive retention, package extension and rejected mathematical/template edits.
Actual text fit and instructional clarity still require the user's review.

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
