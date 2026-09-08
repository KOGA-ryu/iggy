# Native math within Library entries

Contents → Library now presents equations within the selected entry. The title
stays gold, formulas are cyan, and prose retains the normal reading colour.
The blue Raw source / Typeset button changes presentation without changing the
entry or filters. It is on a separate compact row so navigation still fits a
360-pixel window. Controls remain 13-pixel text with 22-pixel button height;
inline math uses 16-pixel em size and display math uses 18.

The source remains authoritative. No corpus JSON, source snapshot, reviewed
adaptation, playable question or practice save is changed by typesetting.
The four separately reviewed teaching notes still open in Reviewed mode; their
Original button exposes the source entry and its presentation control.

## One rendering owner

`NativeMath` now recognizes `\(...\)`, `\[...\]`, `$...$` and `$$...$$` in a
document. It retains contiguous byte ranges covering the entire original,
then uses the existing strict MicroTeX route for each delimited expression.
Inline equations use TeX text style and share a baseline with adjacent prose;
display equations use display style on their own lines. No second parser for
the mathematics or new Vulkan rendering path is introduced.

Prose wraps between words. Single newlines act as spaces, and paragraph gaps
are bounded; the Raw source view preserves the original text and line breaks.
The layout reserves enough vertical space for tall fractions and radicals.
An equation is indivisible: when it is wider than the reading area, the area
scrolls horizontally instead of shrinking or truncating it. Its wrapping width
does not depend on horizontal scroll position.

One document is cached by source, width and prose font. Its formula geometry
is shared by value, remaining valid when the sixteen-expression cache evicts
individual entries. Current atlas UVs are resolved during drawing, as in the
original sample panel. Font resources and dependency seeds are unchanged.

`MathCorpusUi` owns presentation choice and scrolling. The existing reading
trail now retains Raw source / Typeset and horizontal position alongside
Original / Reviewed and vertical position. Back restores that bookmark.
Changing format returns to the top; moving to another title keeps the current
format preference. Paging controls move keyboard focus onward when reaching a
boundary disables the focused button, so Tab can continue into the reader.

## Explicit source fallback

Unmatched delimiters, escaped delimiter pairs and rejected expressions are
shown in amber, with the original bytes retained and a reason on hover. A
mismatched closer ends that fallback; a new opener allows later independent
math to recover. Escaped notation is displayed literally, without guessing
which backslashes the author intended. The reader never silently fixes source.

The complete 930-entry probe found:

| Rendering result | Count |
| --- | ---: |
| Typeset formulas | 2,139 |
| Entries without source fallback | 889 |
| Entries containing source fallback | 41 |
| Source fallback spans | 72 |

The report identifies each affected entry, exact byte range, source and reason
in `build/library-typesetting-evidence/corpus-compatibility.json`. Examples
include a mismatched `$p\)` in Splitting Field, escaped delimiters in Gaussian
Integers, unsupported `\Sha` in Dirac Comb and a literal `\n` command in Monte
Carlo Method. These are compatibility findings, not a mathematical fact check.
Other Markdown syntax is still literal prose; this is not a full Markdown or
arbitrary-LaTeX page renderer. Importing the separate 95 problem pages remains
outside this checkpoint.

## Verification and scope

Release target `sorter` built. The two current CTest entries
`paths_native_math_tests` and `paths_sorter_input_tests` passed. They exercise:

- Layout and actual drawing submission for all 930 source bodies, with source
  coverage, non-overlapping text/math boxes and finite declared extents.
- Fractions, radicals, matrix alignment, superscripts, inline baselines,
  delimiter variants, escaped input, error recovery and cache eviction.
- Actual Library input at 1440x860, 800x600 and 360x480, including Raw source,
  reviewed-note navigation, reading bookmarks, paging and keyboard activation.
- Horizontal wheel scrolling to the end of the real Euler-Maclaurin formula,
  without changing its size or rewrapping the document.
- Dynamic font-atlas requests with dummy texture handles, cache reuse through
  live resizing, and unchanged unfinished practice, queue and progress revision.

Verification uses CPU-only ImGui frames and an in-memory font atlas. There is
no window, Vulkan device, screenshot, capture or exported image. On-screen
appearance remains pending the user's visual check. Build logs, integrity
checks and exact counts are in `build/library-typesetting-evidence/`.

First-party production C++ adds 160 net lines across four existing UI files,
with no new production files. This is a new presentation capability, not a
cleanup. Changes remain uncommitted. Concurrent matrix-board work is preserved
and is not part of this checkpoint's verification.

## User visual check

Launch `b/sorter`, then Contents → Library. Search these exact titles:

1. Quadratic Formula: cyan powers, root and stacked fraction should sit beside
   the prose. The blue Raw source / Typeset button should keep the same entry.
2. Bayes' Theorem: cyan inline symbols and the displayed fraction should remain
   separate and readable, including in a narrow window.
3. Euler-Maclaurin Formula: horizontal scrolling should reach the final `R_p`.
4. Splitting Field: the malformed source stays amber; hovering explains why.

The next content candidate is reviewing the 41 reported entries and publishing
explicit adaptations while preserving the original source records.
