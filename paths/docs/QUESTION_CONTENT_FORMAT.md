# P010 prepared question files

The gallery reads reviewed question instructions from JSON. The loader does
not solve equations or judge display text. It produces the existing question
model; `LayeredQuestionSession` still owns correctness, attempts, collection,
working-state changes and progression.

## Pack and launch

`content/packs/gallery_foundation.json` lists five cards and four ordered
decks: `equality_sweep`, `question_relay`, `equation_chain`, and
`substitution_chain`. Mode names/titles remain in the startup table; their
question indices are no longer compiled into it.

```json
{
  "schema_version": 1,
  "questions": ["../cards/foundation_relay_add.json"],
  "decks": {
    "question_relay": [
      {"question_id": "foundation_relay_add", "content_version": 1}
    ]
  }
}
```

`questions` is the ordered catalog of card paths. Paths must be nonempty and
relative to the pack's directory; `..` is supported. A deck entry references
the exact `(question_id, content_version)` pair. Reordering the catalog does
not change the meaning of a deck. Repeating a reference in a deck is allowed
for repeated practice. Different versions of one question may coexist.

All listed cards and all present decks must load successfully, even when a
different deck is selected. A pack may define only the modes it needs; the
selected startup mode must have a deck. Empty decks and unknown references
are errors. Other deck names may be stored but do not add new startup modes.

`--content-pack PATH` selects a pack; a relative CLI path resolves from the
launch directory. Without that flag, the gallery finds
`content/packs/gallery_foundation.json` relative to SDL's executable data
directory. CMake copies the source `content/` directory there when building
`gallery`, including multi-configuration builds. Keep that directory
beside the executable when moving a build. A later build recopies source
content, so use an explicit source or authoring pack for edits you want to keep.

Files are read once, before graphics initialization. The constructed session
owns immutable content and deck snapshots. Editing files affects the next
launch, including newly added cards, without rebuilding C++. There is no
compiled gallery fallback. Workshop has no questions and rejects an explicit
`--content-pack`.

## Card schema 1

```json
{
  "schema_version": 1,
  "id": "foundation_relay_add",
  "content_version": 1,
  "equation": "7 + 5",
  "skill": "addition",
  "description": "Question Relay fixture",
  "working_states": [
    {"id": 10, "display": "7 + 5"},
    {"id": 20, "display": "7 + 5 = 12"}
  ],
  "steps": [{
    "id": 10,
    "layer_name": "ADD",
    "prompt": "What is 7 + 5?",
    "options": [
      {"id": 101, "label": "12"},
      {"id": 108, "label": "10"},
      {"id": 115, "label": "13"},
      {"id": 122, "label": "14"}
    ],
    "accepted_option_ids": [101],
    "wrong_hint": "Try another colour.",
    "explanation": "7 + 5 = 12.",
    "semantics": {
      "purpose": "calculation",
      "completion": "any_accepted",
      "before": 10,
      "after": 20
    }
  }]
}
```

Every field shown is required and has the illustrated JSON type. Unknown
additional fields are ignored in schema 1; they do not become runtime metadata.
`schema_version` is the file-format version, currently exactly `1`, on both
packs and cards. `content_version` is a positive 32-bit integer identifying
the prepared question revision. Numeric IDs also use positive 32-bit integers;
fractional, boolean, negative and overflowing values are rejected.

Question IDs must be nonempty. Step IDs are unique within the question;
option IDs are unique within a step; working-state IDs are unique within the
question. IDs may repeat across their separate scopes. Prompts and option
labels must be nonempty. Other display strings may be empty, as in the existing
model. Text is not trimmed and labels need not be unique.

`explanation` supplies the prepared reasoning for a step. Answer review shows
it below the attempts only after that step is resolved: one accepted choice
for `any_accepted`, or the full set for `all_accepted`. A wrong answer or a
partial set does not reveal it. An empty string is valid and omits the section.
Current and completed reviews use the frozen content for their question
ID/version, so editing files does not change an active game's explanations.
The text is displayed as authored; the loader does not derive or verify it.
See [P018_REVIEW_EXPLANATIONS.md](P018_REVIEW_EXPLANATIONS.md).

`steps` and `options` preserve authored order. `accepted_option_ids` is a
nonempty set of option IDs in that step. Unknown and duplicate accepted IDs
are rejected. The loader converts IDs to the runtime mask only after decoding
the ordered options. Moving an option leaves its acceptance unchanged. Target
colours and their shuffled bindings remain gameplay-owned.

Allowed `purpose` values are `answer_choice`, `operation_choice`,
`calculation`, `verification`, and `graph_choice`. Allowed `completion` values are:

- `any_accepted`: one of the accepted choices completes the decision.
- `all_accepted`: every accepted choice must be collected.

The loader validates gallery content for ArcadeCollect. The existing question
constructor still checks interaction compatibility: Guided supports decisions
requiring one answer, including multiple alternatives under `any_accepted`;
it rejects `all_accepted` when more than one answer is required.

`before` and `after` reference prepared working-state IDs. Every reference must
exist. Each step's `before` must equal the preceding step's `after`. Equal
before/after values intentionally keep the same working visible. Unused
working states are permitted. The question session determines when a resolved
step advances; neither the loader nor gallery duplicates that decision.

P023 adds optional `hint` and `next_move` strings to each step. Requesting a
hint or showing the next move records assistance without an answer attempt.
Applying a prepared step records `answerShown` and advances through the shared
question owner; it does not invent a correct player answer. Missing fields
remain valid for older content. A question linked from the sorter requires
both fields on every solving decision.

P029 adds an optional `line_graph` object containing integer `rise`, `run`,
`intercept`, `x_min`, `x_max`, `y_min`, and `y_max`. Such a question uses four
`graph_choice` steps and a `graph_stage` on every working state: `grid`,
`intercept`, `run`, `rise`, `line`. Each step advances exactly one reveal stage;
the chain begins at grid and ends at line. Graph stage fields without a graph,
and graph-choice steps without a graph, are rejected. `run` is 1–8, `rise` is
−8–8, `intercept` is −12–12, and bounds are within −20–20. The axes must contain
zero and leave space around the intercept and slope triangle. These bounded
parameters define the mathematical drawing; displayed equations and answer
labels still receive independent authoring checks. The first graph presentation
is in Sorter's prepared workspace; see [P029](P029_COORDINATE_BOARD.md).

P030 permits a `second` object inside `line_graph`, with integer `rise`, `run`
and `intercept` under the same runtime bounds. Both lines share the outer axes.
For example:

```json
"line_graph": {
  "rise": 2, "run": 1, "intercept": 1,
  "x_min": -3, "x_max": 3, "y_min": -2, "y_max": 6,
  "second": { "rise": -1, "run": 1, "intercept": 4 }
}
```

A system uses exactly four `graph_choice` steps and the ordered states `grid`,
`first_line`, `both_lines`, `classified`, `system_solution`. Used states must
follow that chain; single-line states cannot substitute for system states.
Both intercepts and any unique crossing must lie strictly inside the authored
axes. The original first-line triangle bounds still apply. Parallel and
coincident systems have no unique intersection point. The question projection
provides a shared guide once both lines are visible, clips its x range so both
points remain on the board, and reveals the marked solution only at the final
state. Accepted/rejected answers remain authored options judged by the existing
question owner, with exact arithmetic checked by the recipe tests.
See [the simultaneous-equations checkpoint](P030_SIMULTANEOUS_EQUATIONS.md).

A working state may also contain up to eight `highlights`:

```json
"highlights": [
  { "offset": 0, "length": 2, "label": "Outside factor" },
  { "offset": 2, "length": 7, "label": "Whole bracket" }
]
```

Offsets and lengths are unsigned 32-bit byte counts into `display`. Length
must be positive and the label nonempty. Spans must be ordered, nonoverlapping,
in bounds and on UTF-8 character boundaries. These are authored presentation
annotations; the loader does not identify factors or derive transformations.
Omitting `highlights` preserves ordinary working-state display.

## Limits, errors and boundary

Existing model limits remain: 1–64 catalog questions, 1–32 steps per question,
2–8 options per step, and 1–33 working states per question. A pack deck contains
1–64 references. Each JSON document is limited to 1 MiB.

`QuestionContentError` provides an owned source path, JSON-pointer field and
rendered reason. For example:

```text
/my/content/cards/integral.json:/steps/0/accepted_option_ids/0: unknown accepted option ID: 999
```

An empty pointer, displayed as `<root>`, identifies a file-open or JSON syntax
failure; syntax errors also include the parser's location. Shared validation
results map back to card fields, including working states, steps and options.
A duplicate question identity points to the second card's `id` field. Missing
decks point to `/decks/<mode>` in the pack. Startup reports failure and exits.

`QuestionContentIO.hpp/.cpp` expose `loadQuestionPack(path)` and
`parseQuestionContent(jsonText, sourcePath)`. Their internal live helpers
`resolveAcceptedOptions()` and `resolveDeck()` translate file references; they
are kept private because no other consumer needs their JSON decoding context.
The loader uses pinned nlohmann/json 3.12.0 for JSON syntax. Runtime targets do
not depend on that library or on file reading.

The five starter cards preserve all data from the compiled P009 catalog,
including the integral's six decisions and exact IDs/versions. The older
`authoring/002_guided.json` and `authoring/013_guided.json` under `content/`
use a separate authoring shape and are not listed in this pack. The prepared
adaptations below retain those artifacts. See the [content folder guide](../content/README.md).
General mathematical verification, generators, scoring, persistence and
presentation changes remain separate work.

P011 now provides a [prepared 002 adaptation](P011_CARD_002_ADAPTATION.md) in
`content/packs/source_002.json`, using the same schema and the existing
`equation_chain` startup. The original `002_guided.json` remains an authoring
artifact; its source metadata and stable-ID mapping are retained separately.

P012 adds a [prepared 013 adaptation](P012_CARD_013_ADAPTATION.md) in
`content/packs/source_013.json`, also using `equation_chain`. It maps all 14
authored decisions and their working states into this existing schema. The
supplied target stays visible; completion is guided derivation practice.
Its original authoring source and explicit identity map remain separate from
runtime parsing. Neither adaptation changes the loader or introduces a second
question or answer owner.

## Checked linear moves (P032)

A question can explicitly opt into checked mathematical moves in the finite
sorter workspace with `"working_model": "linear_moves"`. An unknown value or
non-string fails at `/working_model`. The shared validator also reports that
field when the equation is unsupported: it must be unsolved, affine in x,
have a nonzero x coefficient on the left and a constant right side, and have
no `line_graph`. This field does not change the prepared arcade interaction.
The ordinary prepared steps/states are still validated for that consumer.

The equation is the mathematical input for this mode. Its bounded grammar
accepts x, integers, terminating decimals, fractions, parentheses, unary signs,
+ - * /, and implicit multiplication. Variable products/divisors and functions
are unsupported. The [P032 contract](P032_MATHEMATICAL_MOVES.md) records limits,
checking, completion and evidence. Checking compares both transformed sides
exactly; text equality and approximate point samples never establish correctness.

The six bracket recipes now publish content version 5, generator version 3,
including this opt-in. Continue editing their recipes and use the existing
publisher/version gate. The generated original equations and prepared answer
mathematics are unchanged. No solution graph is authored: the mathematical-move
run constructs it from actual checked submissions and retains branches on Undo.

## Matrix row moves (P034)

`working_model: "matrix_rows"` selects exact row reduction in the same MathMoves
interaction. The original `equation` contains exactly two augmented rows, for
example `[2, 1 | 7] [1, -1 | -1]`. Each row has two coefficients separated by a
comma, then `|` and its right-hand value. Spaces or a newline separate rows;
the sorter catalogue uses the single-line form because its card labels remain
printable ASCII. The question owner produces a padded two-line display for
working and result choices. Columns represent x, y and the right-hand value.

Shared preparation requires a nonsingular 2x2 coefficient block that is not
already the identity. Singular systems, malformed shapes, unsupported cells,
overflow, and graph/move combinations fail at `/working_model`. Cells use the
existing exact arithmetic and input bounds. Division by zero and operations
that leave the matrix unchanged cannot produce a checked node.

Row swaps, nonzero row division and adding a multiple of the other row are
checked across all six cells. Result shape or solution equivalence alone is
insufficient. Completion requires an identity coefficient block and exact
substitution of both resulting values into both original equations. The same
question/version/run/revision guards, attempts, Undo branches and archives
apply. The UI carries the selected operation, operand and matrix text through
the existing command; it does not parse or judge matrix arithmetic.

The first card is `sorter_matrix_rows`, sorter ID 6001 (introduced at version 1;
version 2 adds P035 reference links). Its four-step
prepared route remains valid for prepared consumers, while the finite sorter
uses contextual row moves and four result choices per move. The chapter is
Linear algebra / Matrices and systems / Row reduction. This checkpoint covers
two equations in two variables; larger matrices and singular-system outcomes
are not supplied as exercises.

## Shared row references (P035)

A question pack may select one shared library using a nonempty relative path:

```json
"reference_library": "../references/row_operations.json"
```

A question opts into specific entries, in display-independent ID order:

```json
"concept_ids": ["row_swap", "row_scaling", "row_addition"]
```

The library has `schema_version: 1` and a `references` array. One entry is:

```json
{
  "id": "row_addition",
  "content_version": 1,
  "title": "Add a row multiple",
  "definition": "Add a multiple of one row to another. Keep the source row unchanged.",
  "rule": "Ri' = Ri + k Rj, i != j. Include the right-hand value.",
  "example": {
    "operation": "add_row_1_to_2",
    "operand": "-2",
    "before": "[1, 2 | 4] [3, 5 | 11]"
  }
}
```

All shown fields are required. Entries have positive unsigned 32-bit content
versions. Nonempty titles, definitions and rules are limited to 64, 400 and
160 bytes respectively. Library and question link arrays allow at most 16
entries; duplicate IDs and unresolved question links fail. An ID must match
the operation's declared family:

| Concept ID | Example operation keys |
| --- | --- |
| `row_swap` | `swap_rows` |
| `row_scaling` | `divide_row_1`, `divide_row_2` |
| `row_addition` | `add_row_1_to_2`, `add_row_2_to_1` |

`before` follows the bounded two-row matrix grammar. `operand` is an exact
number for scaling/addition and must be empty for a swap. Zero divisors,
unchanged transformations, unsupported shapes and arithmetic overflow fail.
Examples may use either target row, negative operands and exact fractions.
The kernel computes the result and each column calculation through the same
row transformation used in play. Definition and rule prose remain authored
text; the loader does not interpret that prose as mathematics.

`loadQuestionPack()` loads the selected library before resolving the cards.
`parseQuestionContent()` accepts an explicit reference span for in-memory
imports; its default empty span continues to support unlinked cards and
rejects unresolved links. Every resolved question owns immutable copies,
including reference versions. Editing a shared file affects subsequent loads;
existing sessions and archived attempts retain their original content.
Errors preserve the source library/card and JSON pointer. A declared library
must load successfully, even if an individual card uses only a subset.

This first consumer is the matrix move reference panel. The operation palette
supplies concept IDs from the canonical row-operation table. The UI looks up
the linked reference through the question owner and displays its separate
example. Its local column cursor is presentation state: partial example rows
are labelled as partial and never submitted as actual working. Reading or
stepping an example creates no attempts or assistance records. Existing
question hints and answer-reveal evidence keep their separate semantics.

The user's external parser is not part of this checkpoint. It can eventually
emit this content shape for supported concepts; adding a new mathematical
interaction still requires its domain operations and checking.
