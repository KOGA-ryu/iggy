# P035: definitions and separate examples beside the problem

Status: Automated Green; user visual confirmation pending; uncommitted.

The user accepted the P034 matrix format and authorized a compact reference
panel for its three existing row-operation families. The user's separate data
parser remains their work; this checkpoint supplies a reusable content format
and a working consumer inside the game.

## Delivered experience

Select a row move, then press the violet `?` beside its selected label above
the result tiles. The existing support area shows a short definition, rule and
a separately labelled example. The three entries cover swapping rows,
dividing a row and adding a row multiple. An open reference follows a newly
selected operation family and resets its example to the beginning.

Use `<` and `>` to follow the example's x, y and right-hand columns. Cyan links
the current source column and calculation. Completed example values are green;
the example reaches its final state after all three columns. Partial example
matrices are explicitly labelled partial. For row addition, the separate
example is:

```text
R1 = [1, 2 |  4]
R2 = [3, 5 | 11]

R2 - 2R1

3  - 2 x 1 = 1
5  - 2 x 2 = 1
11 - 2 x 4 = 3

R1 = [1, 2 | 4]
R2 = [1, 1 | 3]
```

The actual gold problem, cyan working and result choices retain their
rectangles and values. Reference browsing never submits an answer, adds an
attempt, completes a problem or reveals its next result. Close or Escape
returns to inspection; Escape closes the reference before navigating away.
The actual question can be solved while the reference remains open.

At wide sizes the reference uses the right inspection pane. At narrow sizes
it uses the existing lower support area, with history available after Close.
The text body scrolls independently below pinned controls. Stepping pauses
with the game, including on focus loss. Return/resume preserves the reference
for the same question; another question or fresh run clears it. Actual Undo
continues to retain earlier solution branches.

## Owner and scope

The canonical question owner is still `LayeredQuestionSession`. Its read-only
reference lookup exposes only entries linked from the question. The existing
pure row kernel generates example before/after cells and column calculations
through the same exact transformation used for player moves. The UI keeps only
the open reference, cached projection and a bounded presentation cursor.

The existing row-operation declaration now also supplies semantic file keys
and concept IDs to the live loader and palette. Its private declaration is
replaced by one shared table. No competing mathematical judge or progression
route is introduced. Player answers still use `MathematicalMove`; example
navigation creates no attempts or assistance records. This does not change
existing authored hint or answer-reveal semantics.

`content/references/row_operations.json` is the shared authored source. The
matrix card advances to version 2 and links its three IDs. Its generated pack
selects the library using a relative path. The original question, prepared
matrices, accepted answers, 100-card study catalogue and all preceding content
retain their values. Loaded definitions include their versions and stay frozen
with their questions. Editing one library updates all linked questions on the
next load. See [the data contract](QUESTION_CONTENT_FORMAT.md#shared-row-references-p035).

Production change: eight existing C++ files, +238/-21 lines (net +217), and
one existing publisher line (net +218 total). CMake adds three deployment
lines. No production file or dependency is added. One reference data file and
this checkpoint document are new. Unrelated dirty checkout work is preserved.

## Verification

All three native targets (`sorter`, `gallery`, `paths`) build successfully.
Four focused suites derived from the current CMake graph pass:

- `paths_math_moves_tests`: independent expected cells and calculations for
  all three references, both target rows, signed exact fractions, invalid
  examples and metadata, operation-family links and unchanged player evidence.
  Existing algebra and matrix solution routes also pass.
- `paths_content_tests`: relative library resolution, two questions sharing
  one source, immutable active definitions after editing, explicit in-memory
  resolution, unknown/duplicate IDs, missing files, malformed records and
  existing prepared-content boundaries.
- `paths_sorter_input_tests`: real pointer and keyboard controls, held keys,
  backwards example steps, Close/Escape, rule switching, focus/pause, scrolling,
  live resizing, actual completion beside the reference, Undo branches,
  same-question resume and fresh-run reset. Geometry checks cover 1440x860,
  800x600 and 360x480; human visual acceptance remains separate.
- `paths_bracket_recipe_tests`: existing publishing/refusal boundaries and
  preservation of the matrix question's independently calculated prepared route.

The publisher's `--check` passes. The rebuilt sorter validates its 100-card
bundle when launched from outside the repository. Before/after hashes, scoped
diff, build/test logs and the final receipt live in `build/row-reference-evidence/`.
The initial input check exposed example stepping after focus-loss pause; that
was corrected and the affected suite passed. No screenshots, captures, visible
windows, delegation, commit or push were used.

## User visual check

Relaunch `b/sorter`, choose Linear algebra / Matrices and systems / Row
reduction, and select a row move. Look for the violet `?` and reference title.
Use `>` three times: cyan should move across the example's columns and the
finished example should become green. Select another row-operation family
while it is open; its explanation should change. Close or Escape should
restore inspection with the actual gold problem and selected results intact.
On a short window, scroll the reference body while its controls stay visible.

Remaining risk: the user must confirm visual readability. The library currently
supports only the three row-operation families, and working remains in memory.
Next candidate, after user direction: one parser-produced question and linked
reference fixture through this same loader and interface.
