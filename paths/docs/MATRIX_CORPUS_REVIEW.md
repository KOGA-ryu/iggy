# Reviewed matrix notes connected to practice

This records the original two-note checkpoint. The subsequent
[rank/nullity extension](RANK_CORPUS_REVIEW.md) brings the current total to four.

The user authorized a small matrix/row-reduction fact-check after identifying
the corpus as hastily prepared. This checkpoint reviews and adapts two entries,
then shares those adaptations with the thirteen existing matrix questions.

## Scope and findings

| Source entry | Finding | Adaptation |
| --- | --- | --- |
| `corpus_00511`, Gaussian Elimination, source lines 190–192 | Broadly correct summary, insufficient as a teaching definition | Specifies permitted whole-row operations, nonzero scaling, preservation of solutions, and elimination/reduction stages |
| `corpus_00512`, Reduced Row Echelon Form, source lines 193–195 | Incomplete: no defining conditions or explanation of uniqueness | Defines the matrix form and distinguishes its uniqueness from the number of solutions |

The original source file is
`content/source_snapshots/math_terms_v1/sections/linear_algebra.md`.
Its bytes, entry titles, bodies, stable IDs and source locations are unchanged.
Each green marker applies to a **reviewed teaching adaptation**, not blanket
approval of the original source. The remaining 928 entries have no reviewed
adaptation. The original 930-entry source catalogue retains `unreviewed` status.

The factual comparison used these primary teaching references on 2026-09-07:

- [MIT 18.700, Gaussian elimination (2013)](https://ocw.mit.edu/courses/18-700-linear-algebra-fall-2013/b144082f6883d02faeec26d7f708c63e_MIT18_700F13_gauss.pdf),
  Definition 2.10, Definition 3.2, Proposition 3.4 and Section 4: matrix forms,
  allowed operations, field assumptions and preservation of solutions.
- [Georgia Tech, Interactive Linear Algebra §1.2](https://textbooks.math.gatech.edu/ila/row-reduction.html),
  Echelon Forms and the Row Reduction Algorithm: RREF conditions and uniqueness.

The review also checked the existing row-reference restrictions: swap whole
rows; divide by a nonzero scalar; add a multiple of a different row while
retaining the source row. Those references required no changes. Terminology
varies: both cited treatments use Gaussian elimination for a process that can
continue through RREF. The adaptation explicitly describes the stages.

The examples were authored for this checkpoint and checked with exact rational
arithmetic. They use a separate system from the active 6001 question:

```text
[1,2|4] [3,5|11]
R2 <- R2 - 3 R1      -> [1,2|4] [0,-1|-1]
R2 <- R2 / -1       -> [1,2|4] [0,1|1]
R1 <- R1 - 2 R2     -> [1,0|2] [0,1|1]
```

Here x=2 and y=1 satisfy every displayed row. The reviewed explanation also
distinguishes an inconsistent augmented row from a consistent system with a
free variable. This is a source comparison and tested teaching adaptation,
not a formal proof-verification system or an external expert certification.

## Player experience

Library lists the two reviewed entries in green, with a gold selected title.
The reader opens the reviewed note with its conditions, example, review
explanation, date and references. The pinned **Original / Reviewed** control
switches between the preserved source and adaptation. Original source text
continues to say **Not yet fact-checked**. Filtered counts include only reviewed
notes in the current results. Unreviewed entries cannot inherit the review view.

The same definitions appear through Symbols in every matrix question, after
the four existing notation lessons: **Why row elimination works** and
**What reduced form means**. Select the example tile to read the explanation.
These are reading lessons without an authored token quiz. They do not judge
working, reveal the active answer, complete a problem, or change saved evidence.
The original problem/choices stay in the fixed solving workspace.

## Single authoring source and publication

`content/authoring/matrix_corpus_review.json` owns the reviewed prose, conditions,
corrections, source citations, review date, examples, stable entry IDs and
positive content versions. It pins the original entry's ID/title/kind/body/path
with a SHA-256 digest. An altered source requires explicit review reconciliation.

`tools/generate_corpus_toc.py` publishes two projections from this authoring file:

1. Optional `review` metadata on the two entries in `content/corpus/toc.json`.
2. `content/references/matrix_notation.json`, containing the four existing matrix
   lessons with their required definitions plus the two reviewed lessons.

Existing base notation comes from `math_notation.json`; generated copies are
not separate authoring sources. The corpus and notation definitions must have
identical meaning, definition, example and version fields. Their tests compare
those values through the live content loaders for all thirteen matrix cards.

The existing sorter/matrix generators now link matrix packs to the generated
notation library and its six explicit lesson IDs. All question-card bytes,
mathematics, IDs and versions remain unchanged. To publish or check:

```sh
python3 tools/generate_corpus_toc.py
python3 tools/generate_sorter_fixture.py
python3 tools/generate_corpus_toc.py --check
python3 tools/generate_sorter_fixture.py --check
python3 tools/generate_matrix_practice.py --check
```

Publication refuses duplicate/missing/changed source bindings, missing citations,
invalid review dates, out-of-bounds teaching text and nonpositive versions.
Editing a published note, generated definition or lesson requires an increased
content version. Deleting an adaptation withdraws it; it does not approve the
original. All outputs are prepared before any output is replaced.

## Runtime owners and checks

`MathCorpus` decodes optional review metadata, checks its pinned source text and
title against the entry, validates the definition with the existing pure
`MathNotation` validator, and resolves bounded citation data. Catalogue-level
source status remains unreviewed. Metadata structure alone does not establish
truth; factual evidence is the review record above.

`MathCorpusUi` presents the appropriate view. Existing `QuestionContentIO` and
`MathNotationUi` load/present the generated practice lessons without new routes.
`LayeredQuestionSession` continues to own mathematical truth and evidence.
No production solver, gameplay or persistence implementation changed. There
are no new C++ files or competing definition-authoring routes.

`build/matrix-review-evidence/verification.json` records the targeted build and
test results. They cover exact example arithmetic independently and through
the existing row kernel; binding/refusal and publication checks; unchanged
source content; all thirteen live practice links; pointer/keyboard reading,
Original/Reviewed switching and citation paging at 1440×860, 800×600, 360×480;
and byte-identical matrix save migration after adding the new explanations.

Changes remain uncommitted. No windows, screenshots or captures are used;
human visual review remains deferred. The next candidate is a small
rank/free-variable/consistency topic using the same reviewed-note contract.
