# P048 — a reference format for textbook sections

Section **1.2, Row operations and RREF**, is the first section in the revised
format. Its twelve content blocks contain prerequisites, four numbered
definitions, a proposition with a proof, a worked example, a captioned
interactive figure, three practice questions and a closing synthesis. The other
six sections retain their existing content while sharing the reader controls.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 2
```

## Authoring pattern

`RrefLesson.cpp` supplies an ordered sequence of typed blocks. A block has a
stable ID, a type, an optional printed number, a title, and any number of prose
or display-math passages. Optional proof, hint, answer and solution passages are
separate fields. Internal references target IDs, so their destinations do not
depend on the block's array position or printed label. The chapter index now
contains 23 terms; the five RREF entries open their definitions directly.

The section moves from what the operations mean, to the conditions for REF and
RREF, to why operations preserve a system's solutions, and then to calculation
and investigation. The worked example reduces a separately authored two by two
matrix. Practice asks the reader to recognize a form, reduce a two by three
matrix, and explain why multiplication by zero is excluded. Every practice item
offers an independent hint, short answer and full solution. It is written or
mental practice, without an answer-entry or grading interface.

Section explanations, terms and legacy example steps no longer require exactly
three entries. The RREF sequence owns its full teaching content; its legacy
paragraph and example slots are empty. Future conversions can choose their
length and order using the same block renderer.

## Reader and ownership

The reading column uses a nominal 72 average lowercase character widths, wraps
left-aligned prose, and adds space between blocks. Contents can be hidden. Text
size ranges from 90% to 200%, with display equations scaling at the same rate.
Long equations scroll within their own rows. This is a reading-layout choice,
not a claim of complete WCAG conformance or a precisely enforced character count.

`TextbookUi` reuses `NativeMath` and its bundled resources. It keeps at most 64
shared equation layouts at the current text size so expanding a long lesson
does not thrash the typesetter's smaller cache. The shared adapter, Library,
sorter, and third-party sources are unchanged. Equation (1.2.1) is numbered
because the worked explanation refers to it; incidental displays are unnumbered.

`Textbook` resolves references and owns session disclosure choices through its
semantic dispatcher. `lessonView()` publishes help text only when that specific
disclosure is open in the active reading section. Invalid, stale or unsupported
requests reject without changing state. Following a reference does not open a
solution. Navigation retains a reader's current choices during the session.

Figure 1.2.7 presents the existing card-004 board, identifies its current printed
part and dimensions in the caption, and asks the reader to predict, operate and
explain. Its pivot, Undo and full-exercise actions use the existing board owner.
Opening or closing teaching material never changes a matrix, check or result.

The version-1 reading bookmark accepts the expanded text-size range. It still
stores only section IDs, offsets and text size. A fresh launch starts with help
closed and creates no exercise evidence. Source pages remain read-only.

## Verification

The native `math_lab` target is compiled and linked; verification runs only
pure CPU tests and `--validate`. The focused selection contains **30 tests**:
two CPU suites, four textbook CLI checks, six matrix-board CLI checks and
eighteen existing object CLI checks. The new section check reports twelve blocks,
zero open help, an untouched card-004 board and zero bookmark I/O.

Model coverage includes separate disclosures, redacted closed content, stale
requests, stable reference resolution, repeated reference jumps, retained
checked results, and a 200% bookmark round trip with no restored answers. The
earlier navigation, six retained boards and bounded bookmark tests remain.
Warnings-enabled syntax checks cover all six changed/new C++ translation units.
A separate exact-rational text audit checks both reductions, their intermediate
and final matrices, the recognition answer, and balanced braces in all eighteen
authored display formulas. It does not execute typesetting.

```sh
ctest --test-dir /Users/kogaryu/iggy3d/paths/b \
  -R '^(paths_textbook_tests|paths_matrix_board_tests|paths_math_.*_cli)$' \
  --output-on-failure
```

No images, screenshots, captures, renders, previews, font-atlas probes or native
windows are used for verification. In particular, `paths_native_math_tests` is
excluded because its harness rasterizes glyphs. Typesetting appearance, pointer
interaction, reference scrolling and narrow-window layout await the user's
manual test. Changes remain uncommitted.

## Formatting sources

The preceding research informed this pattern:

- [PreTeXt structure](https://pretextbook.org/doc/guide/html/overview-structure.html)
  and [theorem-like material](https://pretextbook.org/doc/guide/html/basics-s-thm.html)
  support meaningful divisions and distinct definitions, propositions and proofs.
- [PreTeXt mathematics](https://pretextbook.org/doc/guide/html/topic-mathematics.html)
  and [figures](https://pretextbook.org/doc/guide/html/topic-figure.html) inform
  displayed formulas, references and numbered captions.
- [PreTeXt exercises](https://pretextbook.org/doc/guide/html/overview-exercises.html)
  separates exercise statements, hints, answers and solutions.
- The [AMS Author Handbook for Monographs (2017)](https://ctan.math.illinois.edu/info/amscls-doc/Author_Handbook_Mono.pdf)
  informs mathematical hierarchy and economical use of equation numbers.
- [W3C guidance on visual presentation](https://www.w3.org/WAI/WCAG22/Understanding/visual-presentation.html)
  informs adjustable text, limited line length, nonjustified prose and spacing.
