# Math corpus table of contents

**Current extension:** [the matrix review](MATRIX_CORPUS_REVIEW.md) adds optional
reviewed teaching adaptations to two entries and shares them with matrix
practice. The initial checkpoint below describes the source-only baseline;
original source text remains unreviewed, with the reviewed adaptation shown
separately. There are still 930 source entries.

This checkpoint adds a source-reading library to the existing sorter Contents.
It is the first corpus integration, separate from playable questions and the
concurrent mathematical-object work. The user explicitly identified the source
as hastily prepared and requested eventual fact-checking.

## Player route

Contents → violet **Library** opens the corpus. Subject and topic selectors,
title search and a scrollable entry list lead to a reader. The selected title
is gold. Previous/next entry and Up/Dn reading controls remain above the text.
The list follows previous/next selection. Practice or Escape returns to the
original Contents; Resume set still opens the same unfinished question.

At widths below 700 pixels the entry list sits above the reader. Wider layouts
place them side by side. Fonts are 13 pixels and main buttons are 22 pixels
high. Subject, topic, search and current reading location are transient; they
survive closing/reopening Library during the same application session.

Every imported entry is explicitly **Not yet fact-checked**. Entries display
the preserved source notes, including their original Markdown/LaTeX notation.
This checkpoint supplies navigation and source reading, not mathematical
typesetting, automatic token lessons, answer checking or verified definitions.
There are no practice/3D links inferred from matching words or titles.

## Source and publication

The seven original files are copied unchanged into
`content/source_snapshots/math_terms_v1/`. Their exact source locations, byte
counts and SHA-256 hashes are appended to `docs/MIGRATION_SEED_MANIFEST.json`.
The six subject files, rather than the incomplete README definitions index,
supply the catalogue. No runtime dependency on the Documents folder remains.

`tools/generate_corpus_toc.py` reads that pinned snapshot. It extracts every
`#### Term:` block and the third-level theorem/example/proof sections containing
direct notes. It preserves 802 term occurrences and 128 further reading entries,
for 930 entries across six subjects and 178 topics. Repeated titles remain
distinct source occurrences. Administrative “Round” headings remain in source
provenance; named topic headings supply the browser's organization. The source
classification and learning order still need editorial review.

`content/authoring/math_corpus_map.json` owns the permanent entry and topic IDs.
Initial IDs were assigned once with `--init-map`, which refuses to overwrite an
existing map. Subsequent publication resolves explicit source heading paths and
occurrences through that map. It never generates replacement IDs from edited
titles, file order or hashes. Renames/moves in a future source revision require
reconciling locators and source hashes while retaining the same IDs. Unmapped,
missing and duplicate occurrences are refused, not silently reassigned.

Run from the Paths root:

```sh
python3 tools/generate_corpus_toc.py
python3 tools/generate_corpus_toc.py --check
```

The first command deterministically publishes `content/corpus/toc.json` using
an atomic file replacement. The second compares bytes without writing. Neither
command edits the source snapshot. The JSON contract is also the boundary a
future parser can produce; Markdown is not parsed during play.

| Record | Fields |
| --- | --- |
| Catalogue | `schema_version: 1`, `review_status: "unreviewed"`, subjects, topics, entries |
| Subject | Stable `id`, `title` |
| Topic | Stable `id`, `subject` ID, `title` |
| Entry | Stable `id`, `topic` ID, `title`, `kind`, full `body`, relative source path, first/last source line |

The four source kinds are term, theorem, example and proof. They describe source
sections, not their correctness. Some entries titled “Term” are properties or
theorems; the importer preserves that classification pending editorial review.
All currently published data must carry `unreviewed`; this schema cannot claim
a verified state. A later fact-checking capability should record the reviewed
revision, evidence, assumptions and corrections before enabling promotion to
teaching or playable content. Corrected teaching text should retain the original
source and provenance rather than overwrite it.

## Owners and boundaries

- `MathCorpus` owns immutable records, bounded decoding, ID resolution and
  subject/topic/title filtering in `paths_math_corpus`. It has no game, renderer
  or persistence dependency. Search is a case-insensitive ASCII substring over
  titles; accented characters remain exact UTF-8 matches.
- `MathCorpusUi` owns only browsing state and presentation, with a clipped
  list and one reader. It emits no game commands and evaluates no source code,
  LaTeX or mathematics. Only visible rows are submitted to ImGui.
- `EquationSorterUi` mounts Library while already in Contents. Existing
  session, question, save, Symbols and mathematical-object owners are unchanged.
- Sorter startup loads the bundled catalogue before creating a native host.
  `--library FILE` selects an explicit alternate catalogue. `--check-content`
  validates both the question pack and corpus without opening a window or
  accessing personal progress. Build deployment copies `content/corpus/` beside
  the executable.

The loader bounds input to 2 MiB, 32 subjects, 512 topics and 4096 entries. Entry
bodies permit up to 32768 UTF-8 bytes, newlines and original tab whitespace.
Duplicate IDs, unresolved references, invalid kinds, source traversal, invalid
line ranges and other control bytes are rejected. The current generated file
is under 400 KiB. The library is independent of the 100-slot sorter catalogue.

## Verification

Evidence is in `build/corpus-toc-evidence/verification.json`. The affected build
and three targeted tests cover full source extraction/fidelity, stable IDs,
bounded malformed-input rejection, and actual pointer/keyboard navigation at
1440×860, 800×600 and 360×480. The input route starts a question, advances it,
returns to Contents, browses and filters the corpus, then resumes the same run.
Selection, queue and progress revision remain unchanged while browsing.

No windows, screenshots or captures are used. Human visual review remains
deferred. Structural/test success does not establish mathematical correctness.
Changes are uncommitted. The next content capability is reviewing and adapting
one small linear-algebra topic into the existing notation and practice formats.
