# Learning documents: text into the app

Status: implemented for five versioned templates, the existing diagram
registry, and startup folder import. Manual visual acceptance is pending.

The producer-side [exporter and publisher](LEARNING_EXPORTS.md) packages authored
documents and activates a checked complete library. Direct folder import remains
available for authoring; both paths use the same compiler and question owners.

Reviewed source-card adaptation is implemented by
[`prepare_source_lesson.py`](../tools/prepare_source_lesson.py). The
[source lesson workflow](SOURCE_LESSON_WORKFLOW.md) supplies the current card
002 mapping, freshness checks and exact preparation/publication commands.

## Structured textbook lessons: `lesson.v2`

This template uses the same `BookBlock` data, disclosure projection and block
renderer as the native textbook. Existing `lesson.v1` Markdown documents keep
their original behavior. Use explicit prose and display-math passages:

```text
@lesson example_reading | Equality
@template lesson.v2
@block definition | equality | 1.1 | Equal values
@prose Both expressions have the same value.
@display eq1
x=x
@endblock
@block exercise | try_equality | 1.2 | Name the unknown
@prose Which symbol names the unknown in this equation?
@help hint
@prose Look for the letter.
@help answer
@display
x
@help solution
@prose The letter x names the unknown quantity.
@reference equality | Recall the definition
@endblock
@practice example_question
@end
```

Put this inside a document with `@paths 1`, subject and chapter declarations;
provide `example_question` in the same imported library. `@block` has four fields:
kind, stable local ID, display number (`-` for none), title. Kinds are
`introduction`, `definition`, `proposition`, `example`, `figure`, `exercise`,
`summary`. IDs and nonempty block numbers must be unique within the lesson.

`@prose` accepts ordinary prose on that line and following lines. `@display`
takes an optional equation number; put LaTeX on the **next** line. Inline
Markdown/LaTeX markup is not interpreted inside a prose passage. Equation
numbers are unique across the lesson, including its closed disclosures.
`@help proof|hint|answer|solution` starts one independent, initially closed
disclosure. Add passages beneath it; `@body` returns to the public body.
Each disclosure may be declared once per block and must contain a passage.
`@reference target_id | label` links to another block in this lesson.

Close every block with `@endblock` and every lesson with `@end`. Place
`@practice` links outside the inner blocks. Limits are 64 blocks per lesson,
64 passages per body/disclosure, 8192 bytes per passage and 16 references per
block, in addition to the existing file/folder limits. Unscoped text, empty
passages, duplicate numbers and unresolved references reject the whole import
with file, line and command diagnostics. Hidden help participates in the reading
fingerprint but is not flattened into public reading references or question
stamps. Opening a reading disclosure does not grade an answer or create work.

A `.paths.md` document declares its subject and chapter, supplies reading and
questions, and names a registered diagram. The importer resolves includes and
references, checks the complete folder, and feeds the existing app models.
The reader, typesetter, question session and diagram provider supply the layout
and behavior. Adding content in an existing template requires no new C++.

## Write, check, launch

Keep versioned source documents in `content/write/`. From the Paths root:

```sh
./b/sorter --check-content --documents content/write
./b/sorter --documents content/write
```

The first command validates without opening a window or reading personal saves.
The second opens the app using those source documents directly. Add or edit a
file and relaunch with the same command; no rebuild or manifest edit is needed.
Import happens at startup. There is no live folder watcher in this checkpoint.

A normal launch uses `b/learning-store/active.json` when published content is
active, or `b/content/write/` otherwise. Builds copy examples into the latter
folder. `--document-store FOLDER` selects an explicit published library, while
`--documents FOLDER` selects direct authoring; the flags are mutually exclusive.
Use the source-folder command above while authoring so rebuilds do not replace
edits to bundled examples. Publication retains a captured baseline and adds packs.

Only filenames ending in `.paths.md` are entry documents. Other `.md` files
can hold explicitly included fragments. Documents are processed in sorted
relative-path order. Reference targets may appear later in that order.

The executable can list its current supported templates, diagram names,
levels and parameter bounds without opening a window:

```sh
./b/sorter --document-capabilities
./b/sorter --inspect-documents --documents content/write
```

The second command returns a structured report of source diagnostics, consumed
files, catalogue IDs and frozen question-stamp digests. It also supports
`--document-store FOLDER`, verifying the complete published generation before
import. Neither headless command opens a window or reads personal progress.

The four included examples are:

| Source | Visible result |
| --- | --- |
| `content/write/algebra.paths.md` | Algebra → Equations from documents; one reading and a four-level equation |
| `content/write/physics.paths.md` | Physics → Oscillations from documents; a phasor lesson with the existing 3D harmonics provider and a two-step question |
| `content/write/biology.paths.md` | Biology → Cells from documents; a reading and a question with automatically formatted text choices |
| `content/write/matrix.paths.md` | Linear Algebra → Matrix rows from documents; a reading and a matrix question with four support levels |
| `content/write/shared/equality.inc.md` | Shared teaching included by the algebra document; not a separate lesson |

These add two subjects, four chapters, four readings and four questions to
the existing Library. The combined shipped collection has 934 readings and
315 Library questions. The examples are original authoring; source text and
answer keys still need content review. The linear and matrix templates also
check their arithmetic and transitions through the exact runtime kernels.

## A readable lesson

```text
@paths 1
@subject physics | Physics
@chapter my_waves | My waves chapter

@lesson my_phasor | A rotating arrow
@template lesson.v1
The arrow's vertical projection is its sine component.

$$y=A\sin(\theta)$$

@figure harmonics 0
@parameter amplitude1 1
@parameter phase_time 0
@caption The arrow begins horizontally; the vertical projection is zero.
@end
```

The subject and chapter declarations create or reuse their ToC records. An
existing ID must keep the same title and parent. A lesson becomes a searchable
reading entry. **Open lesson** uses its shared layout: reading beside the 3D
diagram at wider sizes, or reading above it at narrower sizes. Reading scrolls
without moving the diagram. Question links remain above the reading area.

`@figure` names an existing provider and its zero-based model level. Diagram
levels are separate from question support levels. `@parameter` calls that
provider's canonical input route; its availability, bounds and snapping remain
owned by the model. `--document-capabilities` reports the model's step sizes.
Declared parameters are available under **Diagram controls**. Dragging or
resetting a diagram does not submit a question or alter question evidence.

Figures and captions are public lesson content. In `lesson.v2`, place `@figure`,
`@parameter` and `@caption` outside every `@block`, either before the first block
or after `@endblock`. These commands are rejected inside a block's body or any
`@help` section, including when expanded from an included fragment. The diagnostic
identifies the original file, line and command. A solution figure cannot yet be
attached to **Show solution**; keep withheld answers out of public diagrams.

Figure geometry is actual scene geometry. No picture, browser page or screenshot
is substituted for the provider. Biology can use the same reader and question
templates now; a biology diagram needs a registered provider before it can be
named in a document.

## Question templates

| Template | Required structure | Runtime behavior and checking |
| --- | --- | --- |
| `lesson.v1` | Reading body; optional one diagram and question links | Shared lesson layout and native math typesetting |
| `choices.v1` | Given, domain, goal, version, one or more steps with choices, answer, result, feedback and explanation | The existing symbolic-choice workflow; answers use the authored key |
| `linear.v1` | Plain `ax+b=c` equation and exactly two fully taught steps: subtract b, then divide by a | The existing Learn / Practice / Solve / Write modes and exact linear checking |
| `matrix.v1` | Plain two-row augmented matrix with a unique solution; fully taught row-addition/division steps ending at the identity coefficient block | The same four levels; exact row operations, matrix equivalence and substitution in both original equations |

One workflow is written once. The linear template derives all four support
presentations from that same workflow; authors do not maintain four cards.
The present linear family requires integer a with `2 <= abs(a) <= 9`, nonzero
integer b with `abs(b) <= 9`, a constant right side, and real x. It checks the
declared choices and resulting equations before accepting the card. The writer
supplies exact numeric choices; do not put TeX fractions in `linear.v1` inputs.

The subject name does not create a new checker. A physics or biology card can
use `choices.v1` with an authored answer key. This is structural validation and
deterministic grading against that key, not automatic scientific fact-checking.
New symbolic response types need an appropriate checker before a template can
claim their written solutions are verified.

```text
@question my_question | My practice question
@template choices.v1
@version 1
@goal Identify the required result.
@given 2+3
@domain Work with integers.
@step 10 | Choose the sum.
@choice 11 | 4
@choice 12 | 5
@choice 13 | 6
@answer 12
@after 2+3=5
@wrong Add both quantities once.
@why Combining two and three gives five.
@end
```

Place this within a declared subject/chapter. `@choice` contains native TeX;
`@textchoice 12 | Cell membrane` formats ordinary words automatically. Step and
choice IDs are positive integers. There is one accepted answer ID per step in
this document format. `@after` is the reached working, not a preview shown before
answering. Wrong feedback and explanations are required.

In `linear.v1` and `matrix.v1`, each step also has `@definitions` and `@teaching` sections.
Their prose can span lines and include shared fragments. The complete algebra
example specifies these fields and can be copied as the first four-level
question template. General authoring standards remain in
[QUESTION_AUTHORING_WORKFLOW.md](QUESTION_AUTHORING_WORKFLOW.md).

## Matrix documents

Copy `content/write/matrix.paths.md` for a complete working example. Keep the
existing subject declaration exactly `@subject linear_algebra | Linear Algebra`.
Give each new chapter, lesson and question a permanent distinct ID.

The given and each reached matrix use ordinary text: `[a, b | c] [d, e | f]`.
Columns are the coefficients of real x and y, then the constant. The importer
formats the matrix as native mathematics; authors do not write its layout code.
One step looks like this:

```text
@step 10 | Eliminate x from row 2. Choose the multiple of row 1 to add.
@operation add_row_1_to_2
@choice 11 | 2
@choice 12 | -2
@choice 13 | -1
@answer 12
@after [1, 1 | 3] [0, -3 | -6]
@wrong Make the first entry zero: 2 plus your multiplier times 1.
@why Adding -2 times row 1 across all three entries gives [0, -3 | -6].
@definitions
Elimination cancels a coefficient. A row represents one complete equation.
@teaching
Apply the same row operation in every column, including the constant.
Adding the opposite multiple reverses the operation, preserving the solution.
```

This assumes the given is `[1, 1 | 3] [2, -1 | 0]` and needs the two later
steps in the complete example. **Choices are numeric operation operands**, not
matrix entries or preformatted labels. Fractions such as `-3/2` are exact.
The importer generates symbolic operation tiles and a corresponding Practice
blank. It checks the accepted operand against the declared reached matrix;
wrong choices must remain mathematically distinct, and equivalent duplicates
such as `-2` and `-4/2` are rejected.

| `@operation` | Meaning of the chosen number k |
| --- | --- |
| `add_row_1_to_2` | Replace row 2 by row 2 + k times row 1 |
| `add_row_2_to_1` | Replace row 1 by row 1 + k times row 2 |
| `divide_row_1` | Divide every entry of row 1 by nonzero k |
| `divide_row_2` | Divide every entry of row 2 by nonzero k |

Learn displays explanation and symbolic row-operation choices. Practice displays
the same operation with a missing multiplier or divisor. Solve requests matrix
checkpoints. Write starts with the original matrix and an empty editor. Both
written levels accept one complete matrix per line:

```text
[1, 1 | 3] [0, -3 | -6]
[1, 1 | 3] [0, 1 | 2]
[1, 0 | 1] [0, 1 | 2]
```

The exact checker requires every matrix to preserve the original unique
solution. Row swaps, other valid routes and equivalent fractions are accepted.
Completion requires coefficients `[1, 0] [0, 1]`, with the resulting x and y
checked in both original equations. A direct final matrix is a checked answer;
it does not establish that a multi-step derivation was supplied. No-solution
systems, free-variable families, larger matrices, complex entries, symbolic
row-command text and proof prose are outside this template. Unsupported syntax
retains the draft without a wrong-mathematics mark. Prepared swap-choice steps
are not part of this numeric-operand template.

Every question needs 1–32 complete taught steps, an unsolved nonsingular given,
and a final identity coefficient block; no earlier step may already finish it.
Written work is bounded at 32 lines / 8 KiB and 160 characters per complete
matrix. Existing exact-number limits apply. A failed line commits none of that
submission. Again, Undo, level changes, help exposure and saved drafts use the
same question owner and progress journal as linear questions.

## Links, includes and text rules

- `@practice question_id` inside a lesson creates a blue **Question N** button.
  It opens or resumes that exact question in the existing workspace.
- `@read lesson_id` inside a `choices.v1` question supplies its explicit Method
  reading. It can reference an imported lesson or a current Library entry.
  This does not open a separate textbook exercise or use its answer state.
- `@include shared/equality.inc.md` expands a relative fragment in place.
  It is relative to the including file. A fragment is not independently loaded.
- `@text` resumes the body of a lesson after another prose section, such as
  `@caption`. It takes no argument. The caption itself is plain text.
- Prose in a lesson or after a question text field continues until the next
  directive. Surround mathematics in reading with `$...$` or `$$...$$`, using
  the current native typesetter. Question `@given`, `@choice` and `@after` use
  TeX directly in `choices.v1`; typed templates use plain exact input as described
  above. `matrix.v1` choices are operands, with `@operation` supplying their role.
- `@end` closes each lesson or question. Subject/chapter declarations belong
  outside those blocks. Every entry document starts with exactly one `@paths 1`.
- Commands are case-sensitive. A literal line beginning with `@` can use `@@`.
  Triple-backtick code fences keep directives literal, including `@include`.
- IDs contain 1–64 lowercase letters, digits or underscores. Use permanent
  IDs, not positions or filenames. Do not reuse a lesson or question ID.

## Validation, versions and failure behavior

Import is one transaction for the chosen document folder. Syntax, templates,
includes, links, diagrams, canonical question validation and combined catalogue
capacity must all pass before anything is appended. A failed import leaves the
bundled catalogue intact, displays its diagnostic, and returns failure from
`--check-content`. Syntax errors and unresolved links include source locations.
Fix the source and relaunch. No source file or saved progress is overwritten by
the importer; there is no cached last-good document pack in this checkpoint.

Includes stay inside the chosen folder. Absolute paths, dot segments, symlinks,
cycles and unavailable files are rejected. Limits are 128 entry documents,
128 KiB per source/include, eight nested files, 2 MiB of expanded source, and
2,048 filesystem entries per folder. Existing combined limits remain: 32
subjects, 512 chapters, 4,096 readings and 1,024 loaded questions. A question
has at most 32 steps with eight choices each. A lesson has at most one diagram,
16 declared parameters and 16 practice links. Bulk curriculum loading remains
a separate capacity workstream; these limits must not be silently removed.

File renaming, sorting and added documents preserve an unchanged question's
compiled identity and save stamp. Changing the question's mathematics or frozen
support text triggers the existing original-save retention rule. `@version`
identifies authored revisions; it does not silently migrate old attempts.
Shared `@read` body edits are outside the mathematical stamp, while included
typed definitions/teaching are frozen inside it. Keep that distinction explicit.

## Ownership and extending the pipeline

`LearningDocuments` owns syntax, include resolution, registered-template
compilation, references and atomic publication. It invokes `parseMathCorpus`
and `parseCorpusStarters`; the file loader and document loader share those
canonical validators. `LayeredQuestionSession` owns answer judgments, working,
support and evidence. `CorpusPractice` owns selection and saved replay.

`DocumentLessonUi` only formats content and sends input to the existing
`MathObjects` provider. `MathObjectScene` builds its scene packet; the unchanged
native renderer consumes it. Reading, diagram exploration and question attempts
retain separate state. This work does not change the other builder's textbook,
diagram mathematics or lab exercise owners.

To add a subject using existing templates, write another document and validate
it. To add a diagram, register its named provider, parameter contracts and model
behavior once; the document capability list enumerates that registry. To add
a new answer format, first implement and test its canonical response checker,
then register a versioned template and its formatting. Do not introduce an
English-instruction evaluator, embedded script runner or a second answer judge.

The targeted gates are `paths_learning_document_tests` and
`paths_learning_document_ui_tests`. They cover source-to-app import, bad-folder
rollback, includes, reference failures, diagrams, added/renamed files, saved
drafts, old-save retention and real headless mouse/keyboard solving. Native UI
tests use 1440×860, 800×600 and 360×480 with no windows or captures. Human visual
acceptance remains separate.
