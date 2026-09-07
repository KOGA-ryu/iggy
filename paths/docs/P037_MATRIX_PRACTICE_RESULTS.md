# P037 matrix practice: problems and tested routes

Status: all 12 questions are integrated into the default catalogue. Their 24
solving routes, save compatibility and native builds pass headless checks; user
visual acceptance is pending. The worker used the frozen P036 working copy and
its own build directory. See [the integrated checkpoint](P037_GROWING_MATRIX_PRACTICE.md)
for persistence, build evidence and the short colour-based visual check.

Question identities are `sorter_matrix_practice_6101` through
`sorter_matrix_practice_6112`, each at content version 1. The standalone
`content/sorter/matrix_practice_v1.json` has exactly 100 records: all 15 existing
playable questions, the 12 additions, and the first 73 sorting-only filler
records. Only tail filler leaves the catalogue. Existing questions, versions,
text, metadata and relative order are retained. Original card 6001 remains
under **Matrices and systems / Row reduction**. The new **Matrix practice**
chapter has **Integer foundations**, **Swaps and negatives**, and **Exact
fractions**, with four questions in each type.

All packs link `content/references/row_operations.json`. The question owner
still generates the live operation/result tiles and judges every move. The
accepted gold original, cyan working, green history, compact buttons, retry,
Undo and explicit Next behavior therefore use the existing implementation.

The answer column below is `(x, y)`. Route A supplies the authored prepared
working. Both routes were played using operations and results actually offered
by `mathMoveChoices()`; no typed result bypass was used.

| Card | Intended skill | Answer | Route A | Route B |
| --- | --- | --- | --- | --- |
| [6101](../content/cards/sorter_matrix_practice_6101.json) | Eliminate a multiple of a unit pivot | (3, 2) | R2 - 2R1; R1 - R2 | R1 <-> R2; R1 ÷ 2; R2 - R1; R2 ÷ (-1/2); R1 - (3/2)R2 |
| [6102](../content/cards/sorter_matrix_practice_6102.json) | Normalize a common row factor | (4, 3) | R1 ÷ 2; R2 - R1; R2 ÷ 2; R1 - R2 | R1 <-> R2; R2 - 2R1; R2 ÷ (-4); R1 - 3R2 |
| [6103](../content/cards/sorter_matrix_practice_6103.json) | Clear coefficients with two additions | (1, 3) | R2 - 3R1; R1 - 2R2 | R1 <-> R2; R1 ÷ 3; R2 - R1; R2 ÷ (-1/3); R1 - (7/3)R2 |
| [6104](../content/cards/sorter_matrix_practice_6104.json) | Divide all three columns together | (2, 3) | R1 ÷ 3; R2 - R1; R1 - 2R2 | R1 <-> R2; R2 - 3R1; R2 ÷ (-3); R1 - 3R2 |
| [6105](../content/cards/sorter_matrix_practice_6105.json) | Swap a zero pivot; retain a negative answer | (4, -3) | R1 <-> R2; R2 ÷ 2; R1 + R2 | R1 ÷ 2; R2 + R1; R1 <-> R2 |
| [6106](../content/cards/sorter_matrix_practice_6106.json) | Eliminate a negative coefficient | (-4, 3) | R1 <-> R2; R2 + 2R1; R2 ÷ 6; R1 - R2 | R1 ÷ (-2); R2 - R1; R2 ÷ 3; R1 + 2R2 |
| [6107](../content/cards/sorter_matrix_practice_6107.json) | Normalize a negative second pivot | (-7, 1) | R1 <-> R2; R2 + 3R1; R2 ÷ (-9); R1 + R2 | R1 ÷ (-3); R2 - R1; R2 ÷ (-3); R1 - 2R2 |
| [6108](../content/cards/sorter_matrix_practice_6108.json) | Swap a zero pivot; divide negative pivots | (3, -2) | R1 <-> R2; R1 ÷ (-1); R2 ÷ (-3); R1 + R2 | R1 ÷ (-3); R2 - R1; R2 ÷ (-1); R1 <-> R2 |
| [6109](../content/cards/sorter_matrix_practice_6109.json) | Carry halves to an integer answer | (3, 2) | R1 ÷ 2; R2 - R1; R2 ÷ 3/2; R1 - (1/2)R2 | R1 <-> R2; R2 - 2R1; R2 ÷ (-3); R1 - 2R2 |
| [6110](../content/cards/sorter_matrix_practice_6110.json) | Divide by a fractional pivot | (4, 2) | R1 ÷ 1/2; R2 - R1; R2 ÷ (-3); R1 - 2R2 | R1 <-> R2; R2 - (1/2)R1; R2 ÷ 3/2; R1 + R2 |
| [6111](../content/cards/sorter_matrix_practice_6111.json) | Reach a fractional x answer | (1/2, 1) | R1 ÷ 2; R2 - R1; R2 ÷ 5/2; R1 - (1/2)R2 | R1 <-> R2; R2 - 2R1; R2 ÷ (-5); R1 - 3R2 |
| [6112](../content/cards/sorter_matrix_practice_6112.json) | Keep signed fractions in both answers | (2/3, -1/3) | R1 ÷ 3/2; R2 - 2R1; R2 ÷ 13/3; R1 + (2/3)R2 | R1 <-> R2; R1 ÷ 2; R2 - (3/2)R1; R2 ÷ (-13/4); R1 - (3/2)R2 |

The first four authored routes keep every cell integral. Cards 6105 and 6108
start with a zero first pivot. Every fraction-group problem uses fractional
working; 6111 and 6112 have fractional final answers. Every original system
has a nonzero determinant. Independent Python determinant arithmetic and
independent C++ rational row arithmetic both verify the final values in both
original equations.

The first-example gate ran before generating 6102–6112: only 6101 and its pack
were generated with `--first-example`, then `matrix_practice_tests
--first-example` completed both routes through `GallerySession` and
`LayeredQuestionSession`. That milestone was reported before batch publication.

Final worker checks pass: 7 Python tests and the C++ chapter test covering 24
complete routes, 92 accepted moves, and 92 deliberately wrong tile submissions.
Every offered operation at every visited route state has four distinct tiles
and exactly one independently correct tile. Authored symbols, all working
states and accepted options agree with the live operation. Wrong submissions
retain working and result order; duplicate inputs are stale; Undo retains both
branches; return/resume holds completed work; replay archives both routes.
Completed problems also remain complete after 120 ticks.

The widest live matrix/result encountered across these states is on **6112**:
**41 ASCII characters total, 20 characters per row**. This includes unchosen
operations and wrong result tiles. One such tile is:

```text
[3/2, -13/4 | 13/12]
[  2,     3 |   1/3]
```

The recipe source is [matrix_practice_recipes.json](../content/authoring/matrix_practice_recipes.json).
The existing publisher now composes the chapter into both 100-card catalogues.
Regenerate and verify from the Paths project root:

```sh
python3 tools/generate_sorter_fixture.py
python3 tools/generate_sorter_fixture.py --check
python3 tools/generate_matrix_practice.py --check
cmake -S . -B build-chapter -DPATHS_BUILD_NATIVE=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-chapter --target paths_matrix_practice_tests -j 4
ctest --test-dir build-chapter -R '^paths_matrix_practice(_recipe)?_tests$' --output-on-failure
```

The new generator exports `matrix_practice_outputs(recipe_document,
base_catalogue)`: twelve cards, twelve packs and a standalone catalogue. It has
no import of the existing publisher. Assembly is idempotent; duplicate identities,
insufficient filler capacity and same-version content changes fail before writing.
The worker's exact 30-file transfer manifest and SHA-256 receipt are retained in
`build/matrix-practice-evidence/worker/`.

Two tested routes per problem establish playable coverage; other possible
detours remain governed by the existing bounded exact checker. The widest case
is included in the integrated headless input gate. No screenshots, captures or
visible windows were used.
