# P046 — reusable exercise matrix board

The math lab now opens a scrollable, indexed matrix board for cards 001, 004,
018, 031, 044 and 059. It is a native 2D mathematical diagram alongside the
existing twenty-two 3D objects. Use the **Exercise matrix board** button or:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --card 004
```

Select another card in the board. The printed subparts use indices 0–5 for
004 and 0–1 for 044. Generic examples support sizes 2–32; changing size,
half-bandwidth or block cut starts a fresh example. Rows and columns scroll;
cell tooltips retain complex values at greater precision. A blue region marks
the selected leading block; dark input cells mark prescribed band zeros.

| Card | Implemented behavior |
| --- | --- |
| 001 | Chosen complex banded examples, unpivoted LU, prescribed-zero mask and numerical factor/band checks |
| 004 | All six exact printed rectangular matrices, manual swap/scale/add, guided Gauss–Jordan pivots, undo and an RREF-definition check |
| 018 | Dominant complex examples plus invertible zero-first-pivot examples; leading-block selection, determinant measurement after Check, unpivoted success/breakdown |
| 031 | Chosen real systems with planted all-ones solution; partial pivoting, pivot normalization, downward elimination and synchronized RHS row operations |
| 044 | Both exact printed matrices, row-pivoted PA=LU, correct swapping of previously recorded L entries and factor reconstruction |
| 059 | Chosen complex examples, block cut, n scalar elimination steps, resulting Schur block and independent block-solve comparison |

The current 044 Notation fold explicitly states **PA=LU** and warns against
A=LUP. P046 corrects the older authoring-map note and refreshes the six reviewed
source pins. Other mapped cards remain unimplemented and are not re-certified.

## Ownership and disclosure

`paths_matrix_board` owns authored instances, operations, bounded history and
check evidence. All mutations pass through `MatrixBoard::dispatch`; a candidate
commits only after validation and finite-range checks. Invalid actions leave
state, history and feedback unchanged. The UI reads `MatrixBoardView` and sends
semantic actions; it contains no factorization or answer logic.

Setup publishes givens and dimensions, with empty working/factor arrays and no
check measurements. Choosing a pivot or manual operation begins working.
Completed traces expose their factors. Check measures existing work; it does
not solve an incomplete trace. Undo can return all the way to setup and hides
derived entries again. No answer-key reveal, source help-fold synchronization,
attempt persistence, scoring or proof grading is introduced.

History is bounded at 128 operations and generic matrices at 32 by 32. Complex
row arithmetic is supported; no arbitrary LaTeX is executed. Manual operations
are enabled on 004 and 031. The other adapters use their prescribed pivot
policy so that LU bookkeeping cannot be invalidated by unrelated operations.
The pivot cutoff is 1e-12. Checks report a 1e-10 residual tolerance. Product
residuals use maximum entry error divided by max(1, maximum reference entry);
band and planted-vector errors are absolute. RREF/shape conditions are tested
separately. A numeric pass is evidence for the current example only.

004 and 044 currently expose invariant checks, even though their source pages
request two methods. 018 supplies numerical exploration of its proof question.
These adapter checks do not assert completion of the source exercise contract.
All source pages, learner fields and other Paths modes remain unchanged.

## Text-only verification

The CPU suite covers all printed subparts against independent exact-fraction
RREF fixtures, factor shapes/reconstruction, four sizes (2, 5, 12, 32), band/cut
sweeps, complex row operations, RHS propagation, setup hiding, undo, history
bounds, invalid inputs and overflow rollback. Six native CLI checks join the
existing eighteen. All CLI checks use `--validate`, which returns before the
native host is constructed. The eight existing math/scene CPU suites also run.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --card 044 --board-case 1 --board-step 3 --board-check
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --card 059 --board-size 12 1 5 --board-step 5 --board-check
```

No window, screenshot, image generation, capture or visual inspection was used.
Native layout and pointer behavior await the user's manual test. Changes remain
uncommitted. Larger matrices, additional card families and general proof
assessment are outside this checkpoint.
