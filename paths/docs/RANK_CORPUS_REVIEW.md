# Rank, nullity and consistency in Library and Symbols

This extends the [first matrix review](MATRIX_CORPUS_REVIEW.md) with two teaching
adaptations, shared by all thirteen existing matrix questions. The same
authoring file, publisher, definition model and reading controls handle both.
No new runtime code or solver capability is needed.

## Source findings

| Source record | Finding | Added explanation |
| --- | --- | --- |
| `corpus_00513`, Rank | Correct column-space definition, too brief for solving | Coefficient pivots, row rank, augmented rank and consistency |
| `corpus_00514`, Nullity | Correct for a finite-dimensional linear map, with that assumption unstated | Domain dimension, nonpivot variables, kernel directions and solution families |

The source has no standalone linear-system Free Variables or Consistency term.
The Consistency entry in probability/statistics concerns estimators and retains
its original meaning and unreviewed status. The two new lesson titles are
**Rank and consistency** and **Nullity and free variables**; their definitions
remain bound to the original Rank and Nullity occurrence IDs.

The source comparison was performed on 2026-09-07 using:

- [MIT 18.700: Gaussian elimination](https://ocw.mit.edu/courses/18-700-linear-algebra-fall-2013/b144082f6883d02faeec26d7f708c63e_MIT18_700F13_gauss.pdf),
  Definition 2.6, Propositions 2.7 and 2.11, Section 5: rank, kernel, pivots and free variables over a field.
- [Georgia Tech ILA §2.9](https://textbooks.math.gatech.edu/ila/rank-thm.html):
  rank and nullity definitions and the rank theorem.
- [Georgia Tech ILA §1.3](https://textbooks.math.gatech.edu/ila/parametric-form.html):
  free variables, parametric form and possible numbers of solutions.
- [Georgia Tech ILA §2.3](https://textbooks.math.gatech.edu/ila/matrix-equations.html):
  consistency as membership of the right-hand vector in the column span.

The equality-of-ranks consistency criterion is a deduction from the cited
column-span criterion: appending b preserves the span dimension exactly when
b already belongs to the column space. This reasoning is recorded in the
published correction note. Infinite-solution statements explicitly require
consistency and an infinite field such as the rationals or reals.

## Player content

The new green Library entries open reviewed adaptations; the selected title
is gold. Original / Reviewed preserves access to the unchanged source.
The catalogue now has four reviewed adaptations and 926 entries without one.
The original 930-entry source catalogue retains its unreviewed status.

Each matrix question now links eight Symbols lessons: four base notation
lessons, the earlier elimination/RREF notes, then these two new notes.
Selectable tiles identify coefficient rank, augmented rank, nullity and a free
variable. They explain separate examples within the existing workspace and
do not answer or complete the active question. These are reading lessons;
singular and inconsistent systems have not been added as playable questions.

The authored examples are:

```text
[1,2|3] [2,4|7]
R2 <- R2 - 2 R1  -> [1,2|3] [0,0|1]
rank(A)=1, rank([A|b])=2: no solution.

Replacing 7 with 6 makes both ranks 1:
(x,y)=(3,0)+t(-2,1).

[1,0,2|5] [0,1,-1|1]
rank(A)=2, nullity(A)=3-2=1.
(x,y,z)=(5,1,0)+t(-2,1,1).
```

The last example has three unknowns; its fourth column is b, not a fourth
unknown. Inconsistent systems still have a coefficient-matrix nullity but no
solution family. Even zero nullity does not itself guarantee consistency.

## Ownership and verification

`content/authoring/matrix_corpus_review.json` remains the single authoring
source. New records have version 1 and SHA-256 pins to their exact source
occurrences. Existing reviewed notes, base definitions and lessons remain
unchanged. The two fixture generators only append explicit lesson IDs to the
thirteen matrix packs. All question-card bytes, mathematics and versions stay
unchanged, as do all seven original corpus snapshot files.

The targeted checks cover publication/source fidelity, all thirteen live
definition bindings, seven exact rank/consistency cases, and input at
1440×860, 800×600 and 360×480. Independent rational minor determinants check
ranks; independent kernel vectors and a particular solution certify complete
affine families. Inconsistent cases have a row-combination witness with zero
coefficients and a nonzero right-hand side. Boundaries include rectangular
matrices, zero rank, fractions, nonleading pivots, multiple free variables and
full coefficient rank with an inconsistent right-hand side.

Four- and six-lesson predecessor content configurations exercise save migration
to the eight-lesson content with byte-identical saved working. This checks
metadata growth through the existing persistence owner, without claiming a
new frozen historical executable fixture. The existing P036 fixture remains
covered by the same save test target.

`build/rank-review-evidence/verification.json` records the build, named test
results, startup check and preserved-input hashes. No windows, screenshots,
captures or images are used; UI checks inspect memory-only input and layout
data. Human visual review remains deferred. Changes remain uncommitted.

The next candidate is prerequisite links between reviewed notes, so a player
can move from nullity to rank or row reduction within the reading panel.
That navigation is outside this checkpoint.
