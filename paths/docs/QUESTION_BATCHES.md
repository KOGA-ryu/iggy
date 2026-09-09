# Checked question batches

Status: **implemented for one bounded matrix family**. The command creates
original questions, independently checks their mathematics, imports and plays
them headlessly, then exports or publishes through the existing learning
publisher. Appearance and interaction acceptance remain the user's visual check.

The [editable matrix reference](QUESTION_PRACTICE_FORMAT.md#editable-matrix-reference)
is the next format candidate: readable titles, separate hints and typeset worked
arithmetic. It is one exported, unpublished card awaiting visual acceptance.
The version-1 generator described below is unchanged. Do not regenerate it with
different prose under the same immutable output/version; adopt the accepted
format in a subsequent batch while retaining all published question stamps.

## Run a batch

From `/Users/kogaryu/iggy3d/paths`, build the app and its pure route gate:

```sh
cmake --build b --target sorter paths_learning_document_tests --parallel 4
```

Generate, verify and export the default 12-question batch:

```sh
python3 -B tools/build_question_batch.py
```

To also activate that checked batch:

```sh
python3 -B tools/build_question_batch.py --publish
```

Open Library → Linear Algebra → **Matrix repetitions**. Three reading entries
group integer, negative and fractional solutions, each with four question links.
The question list can also find them by searching **Matrix reps**. Learn supplies
the current explanation; Practice offers the same symbolic tiles with Help on
demand. Typing is optional in Practice. Completion waits for Next.

The command prints JSON containing every card's mathematical result, model-route
counts and publication state. Failures exit nonzero and print a structured error
on stderr. Compiler errors retain source file, line, field and reason. Failed
generation or validation never falls back to a previously successful export.

## Outputs and repeated use

Default output is `build/question-batches/matrix_repetitions/1/`:

| Path | Contents |
| --- | --- |
| `authoring/authoring.json` | Versioned package identity and original-generation provenance. |
| `authoring/documents/*.paths.md` | Three native textbook readings and playable questions. |
| `authoring/audit.json` | Frozen parameters, full working, choices, answers and independent mathematical checks. Authoring evidence, not public lesson text. |
| `export/` | The existing publisher's portable package, inventory and target receipt. |

Every invocation regenerates and rechecks before using these outputs. Identical
output reuses immutable directories. Changed output at the same destination is
rejected. Repeating an identical publication preserves the active generation and
its modification time. Normal rebuilds retain `b/learning-store`.

`--count` accepts multiples of three from 3 through 36, equally divided among
the three groups. Extend a batch with a new package version/output:

```sh
python3 -B tools/build_question_batch.py --count 24 --version 2 --publish
```

The smaller batch is a stable prefix of the larger one. IDs depend on givens,
not batch size or display position. Earlier question stamps remain identical.
Publishing fewer questions over a larger batch is rejected. Changed mathematics
or teaching requires a new question identity while retaining the original;
changing only the package version does not authorize rewriting it.

`--output`, `--store`, `--base-documents`, `--target` and `--model` support isolated
work. The default action exports only. `--publish` invokes the existing publisher;
this tool does not implement another store, lock or activation format.

## Mathematics and teaching

One family begins with coefficient rows `[1,p]` and `[k,kp+d]`, for bounded
nonzero integers p and k and d in {-3,-2,2,3}. Its determinant d is nonzero.
It is deliberately a three-step starting family:

1. Add -k times row 1 to row 2 to cancel x.
2. Divide row 2 by d to make its pivot one.
3. Add -p times row 2 to row 1 to cancel y.

Each question supplies givens, domain, goal, three symbolic choices per step,
accepted operand, reached matrix, definitions, a worked numerical explanation
and a step-specific correction cue. The three groups vary arithmetic; they are
repetitions of one task family, not three skills or a completed curriculum leaf.

Construction starts from known solutions. An independent pass uses Cramer's rule
on the original givens, verifies each intermediate row and applies every candidate
operation independently. Exactly one distinct choice must produce the declared
matrix. Correct positions vary deterministically; identities and the bounded
36-question arithmetic pool have regression coverage.

The C++ compiler then applies its existing exact checker. The pure model gate
exercises Learn choices, Practice choices, typed Practice, Solve and Write,
including every wrong numeric choice in guided routes, Undo, partial save/reopen
and retained completion. The generator/report cannot mark a runtime attempt
solved independently of `LayeredQuestionSession`.

## Parser and authoring consistency

The linear authoring profile now describes Practice as
`symbol_choices_optional_blank`. Its projection includes Learn's opaque choices,
retains the optional blank and omits automatic teaching and definitions. The 25
existing runtime linear cards remain byte-identical.

The matrix validator returns a static reason and affected step/choice indices.
`QuestionContentIO` preserves that result and JSON pointer; `LearningDocuments`
maps it to the original directive, including included fragments. Wrong reached
matrices, equivalent choices, an accepted zero divisor, invalid givens and
malformed reached matrices have direct/include regression cases. The document
compiler does not reimplement mathematical policy to produce these diagnostics.

## Verification and boundaries

Evidence is in `build/question-batch-evidence/verification.json`. Gates cover
authoring projections/runtime reproduction, document/model regressions and
`paths_question_batch_tests`. The latter exercises actual export, publish,
repeat, append, removal rejection, stale output rejection and error propagation
against the built app and pure model. It does not run the native UI harness.

No screenshots, images, native windows or font-rasterization probes are used.
The user checks **gold Given**, **cyan choices/Working**, **purple Help**,
save/reopen and **green completion until Next**. New families need their own
bounded recipe, checked teaching and mathematical owner. General source-card
conversion, a verified whole-corpus taxonomy, new 3D assets and per-distractor
runtime explanation routing remain separate work.
