# P047 — the first interactive textbook chapter

`math_lab` now opens a textbook contents page by default. Part IV, Linear
Algebra, contains Chapter 1, **Matrices and Elimination**, with seven sections.
The remaining six parts provide the book outline and have no completed chapters
yet. The existing 22 objects remain under **Explore 3D objects**.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab
/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 1
```

| Section | Exercise reference |
| --- | --- |
| 1.1 Reading a matrix | 004, used first as a reading drill |
| 1.2 Row operations and RREF | 004 |
| 1.3 Systems and partial pivoting | 031 |
| 1.4 LU and leading blocks | 018 |
| 1.5 Recording permutations | 044, using PA=LU |
| 1.6 Banded matrices and sparsity | 001 |
| 1.7 Block elimination | 059 |

Each section has three explanatory passages, three definitions, an interactive
matrix figure, an optional worked example, an exercise prompt and its source
reference. The examples and teaching prose are newly authored; the original
problem pages remain unchanged. Reading a theorem or revealing a teaching example
does not claim a source exercise was solved or that a source help fold was opened.

## Reading and exercises

The left contents tree opens sections directly. Previous and Next move through
the chapter. The term index links 21 definitions back to their sections. Reading
and Exercise switch within the section; the exercise embeds the same reusable
matrix-board controls with its card fixed. The figure can perform a guided pivot
or open those full controls. Different printed subparts remain selectable.

`Textbook` owns navigation, text scale and per-section reading offsets. It keeps
one `MatrixBoard` per distinct card, so sections 1.1 and 1.2 share their 004 work.
Boards retain their operations and check state when the reader changes sections.
The standalone `--card` sandbox is a separate practice session. The mathematical
owner, restrictions and numerical checks from P046 are unchanged.

`MatrixBoardUi` now offers reusable grid and workspace functions as well as its
standalone window. `TextbookUi` consumes those functions. UI commands apply before
the next coherent frame snapshot; navigation never derives mathematical answers.
One `LabPage` value selects the textbook, object collection or standalone board.

## Bookmark behavior

Continue reading restores the current section and its scroll offset. Native
startup loads `reading-v1.txt` from SDL's `Paths` / `MathLab` preferences folder.
The bounded, versioned file records stable section IDs, offsets and text size.
Changes save at most once every two seconds and again at normal exit, using a
separate temporary file and rename. This is reading position, not completion.
Malformed bookmarks are rejected without partially changing reading state.

Exercise work is retained within the running session and is not saved in this
bookmark. The source Attempt/setup/solve/Run fields and sorter persistence are
untouched. `--validate` exits before preferences lookup, bookmark I/O, native host
construction or any window/image operation.

## Verification and limits

The textbook CPU tests cover section/index coverage, navigation bounds, retained
boards, shared 004 state, reading/exercise separation, scroll and text-size round
trips, malformed bookmarks, missing files and failed writes. Three textbook CLI
checks join the six board and eighteen existing object CLI checks; the matrix
board CPU suite is also retained: **29 targeted text-only tests** in this packet.
Warnings-enabled syntax checks cover all changed/new C++ production sources.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --book --section 2 --book-next --book-exercise
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --book-index
```

No images, screenshots, renders, previews or windows were taken or viewed during
implementation. Native layout, scrolling and pointer feel await the user's
manual test. This chapter is a working prototype for organizing the rest of the
book; other parts and additional exercise adapters remain future work. Changes
are uncommitted.
