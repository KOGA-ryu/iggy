# Reusable definitions, symbols and syntax

[The matrix corpus review](RANK_CORPUS_REVIEW.md) now generates a matrix-specific
library from the existing four matrix lessons and four reviewed teaching notes.
The thirteen matrix questions link eight lessons; other questions retain their
original notation library. The reviewed prose is authored once and shared by
Library and Symbols without changing the component or mathematical owner.

This user-authorized feature is separate from the concurrent 3D-object work
and deferred repairs. It extends the existing fixed solving workspace with
one data-driven component, without equation typing or another screen.

## Player experience

The violet **Symbols** button on the original-problem board opens the existing
support area: the right inspection pane at wide sizes or the lower support
pane at narrow sizes. The original problem, working and answers stay in place.

Each lesson presents a labelled example expression as selectable symbol tiles.
A selected tile shows its short meaning, its role at that occurrence, a
definition, an example and a reading of the whole expression. The left and
right occurrences of `R2` share a row definition but have different roles in
`R2 <- R2 - (1/2) R1`. The expression is explicitly a separate example.

Pinned controls provide Close, previous/next lesson, Try/Learn and Up/Dn text
paging. The body also scrolls normally. The paging buttons keep long text
accessible by pointer and keyboard in short windows. Try asks the player to
identify one token. It gives retry or success feedback without automatically
advancing the lesson or the actual problem.

Reference browsing and reading practice do not submit mathematical moves,
reveal prepared answers, mark questions started or alter attempts, scores,
Undo branches or saved queues. Practice activation pauses with the game.
Escape closes Symbols before leaving the question. Returning to the same
question/run retains the page; another question or fresh run resets it.
Requested hints take over the support pane immediately. Actual result tiles
and Undo remain usable while the notation page is open.

## Data contract

There is no subject switch or character-to-definition guessing in the shared
component. Authors and parsers supply explicit contexts and occurrences:

| Record | Fields | Purpose |
| --- | --- | --- |
| Term | `id`, `content_version`, `title`, `meaning`, `definition`, `example` | One shared authored definition |
| Lesson | `id`, `content_version`, `title`, `context`, `reading`, `tokens` | One expression and its contextual reading |
| Token | `text`, `term_id`, `role` | One occurrence linked to a definition |
| Optional check | `prompt`, `answer_token`, `correct`, `retry` | Identify one occurrence using its zero-based index |

The library is UTF-8 JSON with `schema_version: 1`, `terms` and `lessons`.
The complete authoring source is
[`content/references/math_notation.json`](../content/references/math_notation.json).
Identical glyphs can link to different term IDs in different contexts, such as
an augmentation separator and absolute-value bars. Repeated glyphs use their
occurrence indices as UI identities, so identical labels remain distinct.

An ordinary question pack opts in with these additional fields:

```json
{
  "notation_library": "../references/math_notation.json",
  "notation_ids": [
    "augmented_matrix",
    "row_add_syntax",
    "row_scale_syntax",
    "row_swap_syntax"
  ]
}
```

The pack's required schema, question paths and decks remain unchanged.
Library paths are relative to the pack. A nonempty card-level `notation_ids`
list overrides the pack list; empty or omitted card lists inherit it. Packs
without this metadata retain their previous behavior.

For parser imports, `parseQuestionContent()` accepts resolved notation lessons
as its fourth argument, from which a card's `notation_ids` select. New subjects
can supply different terms and token sequences without changing the renderer
or reading checker. Structural validation does not establish that definition
prose is mathematically true; imported teaching material still needs editorial
checking. This format does not implement an equation parser or LaTeX renderer.

## Owners and reuse

- `MathNotation.hpp/.cpp` owns the renderer-independent records, bounded
  structural validation and pure token-identification checker. It has no
  solver or game-session dependency.
- `QuestionContentIO` loads the library and rejects duplicate/unresolved IDs.
  It resolves tokens from the single authored term source and freezes lesson
  copies with each question. File edits affect subsequent loads.
- `LayeredQuestionContent::notation` carries the optional lessons. Direct
  model construction uses the same structural validator.
- `MathNotationUi.hpp/.cpp`, in `paths_notation_ui`, owns presentation and
  transient reading state. It depends on the pure model and ImGui, with no
  sorter, scene, 3D object or native-host dependency.
- `EquationSorterUi` mounts the same component in its mathematical-move and
  prepared-question support areas. It coordinates Symbols with the existing
  `?` row-reference panel, whose checked worked examples remain available.

To embed the component, hold `MathNotationUiState`, call `syncMathNotation()`
with a stable question/run identity, then call `drawMathNotationToggle()` and
`drawMathNotation()` in the host workspace. Supply validated lessons and
blocked/paused input state. The panel fills an existing ImGui child region.
The heading toggle expects at least 320 pixels of width, and the panel header
needs 260 pixels. The supported compact workspace has a 62-pixel support area;
its content scrolls below the header. The host retains its navigation/focus
recovery policy.

## Bounds and persistence

Libraries are limited to 1 MiB, 128 terms and 64 lessons. Questions link at
most eight lessons; each lesson contains one to sixteen tokens. IDs contain
lowercase ASCII letters, digits or underscores, with a 64-byte limit. Versions
are positive 32-bit integers. Text fields have explicit limits. Control
characters are rejected except newlines in prose. Token labels are single-line,
at most 32 bytes, and cannot contain ImGui's reserved `##` marker.

The library is data only. It cannot evaluate code or define arithmetic
operators. `checkNotation()` identifies an authored token; it does not judge
the mathematical solution or claim mastery of a definition.

All 27 current playable study questions link to the library: bracket algebra,
straight-line/system graphs and matrix practice. Eight lessons reuse 17
definitions. Existing question cards, mathematics, IDs and content versions
remain unchanged; the links are pack metadata maintained by the generators.

The existing practice-save stamp excludes reference metadata, and notation
likewise does not change mathematical identity. Pages, selected terms and
reading verdicts are transient UI state and are not saved as problem evidence.

## Verification and limits

`build/notation-feature-evidence/verification.json` records build/test results,
hashes and the scoped changes. Verification uses builds, pure tests, the
existing headless ImGui input harness and numeric rectangles. No windows,
screenshots or captures are used. Human visual acceptance remains deferred.

The initial authored coverage is algebra, matrices and graph notation. This
checkpoint supplies a reusable component and format, not a complete maths
dictionary, automatic tokenization of arbitrary equations, or integration
with the separate 3D-object builder.
