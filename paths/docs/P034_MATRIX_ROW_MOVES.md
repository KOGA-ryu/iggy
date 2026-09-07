# P034: a linear algebra question with visual row moves

Status: Automated Green; user visual confirmation pending; uncommitted.

The user accepted the visual format and asked for the same format on a linear
algebra question. This checkpoint adds one question, solving `2x + y = 7` and
`x - y = -1` through an augmented matrix in the existing workspace.

## Delivered experience

Contents now includes **Linear algebra / Matrices and systems / Row reduction**.
The original matrix stays gold above cyan current working. Click a compact row
operation, then click its resulting matrix from four tiles. Correct steps add
green checked nodes. Wrong clicks retain the matrix and result order for retry.
There is no typing. The controls, Undo, inspection and completion use the same
format as the scalar algebra questions; matrix panels reserve two text rows.

One route is:

```text
               [2,  1 |  7]       [1, -1 | -1]
R1 <-> R2      [1, -1 | -1]   ->  [2,  1 |  7]

R2 - 2R1                          [1, -1 | -1]
                                  [0,  3 |  9]

R2 ÷ 3                            [1, -1 | -1]
                                  [0,  1 |  3]

R1 + R2                           [1,  0 |  2]
                                  [0,  1 |  3]
```

Dividing R1 by 2 first offers an alternate path through fractions. Both routes
finish at `x = 2, y = 3`, verified in both original equations. Undo keeps the
earlier branch; replay archives it. A matrix-only study set contains exactly
one question and cannot advance to unrelated questions. Completed working
stays until an explicit action. Retention remains in memory while the app runs.

## Canonical owner and scope

`LayeredQuestionSession` still owns progression, judging, attempts and archives.
Its existing mathematical nodes now hold a typed scalar equation or augmented
matrix. The same dispatcher visits this value to obtain choices, check a move
and verify the result. No matrix-specific session, answer judge in the UI, or
parallel progression route is introduced.

The existing pure kernel shares its exact rational arithmetic. A declared
`matrix_rows` working model prepares a nonsingular, unsolved 2x2 system. Row
swaps, nonzero row division, and adding a multiple of the other row must match
across all six cells. Wrong result tiles include unchanged coefficients or
right-hand values and arithmetic errors. Choosing the right final answer with
an unrelated row operation fails. Completion requires an identity coefficient
block and exact substitution into both original equations.

The contextual palette is bounded by six move slots and four result slots.
It offers pivot normalization, elimination and row swaps using a fixed candidate
budget. Parser, numeric, node and attempt limits remain those of the shared
mathematical interaction. This first exercise covers two equations in two
variables; larger matrices and singular-system classifications are later work.

`content/cards/sorter_matrix_rows.json` is the authored source. The publisher
links it through `matrix_rows_pack.json` and inserts sorter ID 6001 after the
existing fourteen study questions. Those questions keep their identity, content
and order. One sorting-only filler slot leaves the fixed 100-card default pack.

Production change: six existing C++ files, +238/-43 lines (net +195), plus the
existing publisher +23/-1 (net +22). No production file or dependency is added.
The obsolete scalar-only choice/verification names are replaced at their live
callers; both working forms share the same interface.

## Verification

Release builds pass for `sorter`, `gallery` and `paths`. Five focused suites
derived from the current CMake graph pass:

- `paths_math_moves_tests`: both matrix routes, independent expected matrices,
  distinct choices, wrong/stale/cross-domain commands, zero division, invalid
  shapes and singular content, exact fractions, original-system verification,
  Undo branches, replay archives and matrix-only study selection. Existing
  scalar mathematical checks also pass.
- `paths_sorter_input_tests`: real pointer and keyboard input through Contents,
  the new subject/chapter, both solution routes, wrong choices, held Enter,
  pause/resume, Undo, replay and live resizing. Matrices and their result tiles
  pass geometry checks at 1440x860, 800x600 and 360x480. Existing algebra and
  graph controls pass.
- `paths_bracket_recipe_tests`: independently checked authored matrix states,
  prepared answers, catalogue insertion, unchanged preceding study entries,
  reproducibility and the publisher's existing refusal boundaries.
- `paths_sorter_solve_tests` and `paths_content_tests`: surviving graph,
  prepared-solving, study-queue and content-validation boundaries.

The publisher's `--check` passes. The executable validates its bundled 100-card
pack when launched from outside the repository, and all 52 bundled JSON files
match their source bytes. Evidence, before/after hashes and the scoped diff are
in `build/matrix-moves-evidence/`. No screenshot, capture, visible
window, delegation, commit or push was used. Human visual acceptance is pending.

## User visual check

Relaunch `b/sorter`. Uncheck Bracket equations in Contents and select **Linear
algebra / Matrices and systems / Row reduction**, then Start set. Look for the
gold original matrix and cyan row-move controls. Try the four operations shown
above, selecting each resulting matrix. Green working should end at
`[1, 0 | 2] [0, 1 | 3]` and show `x = 2, y = 3`. A wrong choice should leave
working in place; Undo should retain the earlier branch.

Next candidate after the visual check: another 2x2 matrix question with a
fractional final solution, using this same interaction.
