# Reusable graphics for the 95 mathematics cards

A graphics component receives a card's declared mathematical objects and renders
linked views. Changing the card changes its data and allowed interactions.
This map is a proposed design and exact-data examples, not 95 implemented card
adapters. The existing 22 math_lab objects remain working exploration models.
No runtime behavior changes in this checkpoint.

The text-only inventory confirms 95 math pages and 91 distinct
(book, chapter, exercise) locators. `pp.py status` reports every math page as
setup not started / solve not attempted. Only `status` was run; no exercise
programs, source edits, screenshots, images or windows were used.

The machine-readable catalogue is
[`exercise_graphics_map.json`](../content/authoring/exercise_graphics_map.json).
Every page has a primary component, optional supporting components, a recipe,
source locator and SHA-256, and an explicit mapped-not-implemented state.
Local source warnings are recorded per card. Checks certify this mapping's
coverage/references, not the correctness of the cards' solution keys.

## The shared kit

The table assigns every page one primary display. Most combine it with one or
two supporting displays from the same kit. Family counts are design choices,
not claims that each family is already a reusable implementation.

| Component | Presentation | Primary cards |
| --- | --- | --- |
| Matrix board | 2D indexed cells / block views | 001, 004, 007, 018, 019, 031, 032, 037, 041, 044, 048, 059, 060, 063, 069, 071, 074, 076, 077, 086 |
| Vectors, bases and linear maps | 2D / 3D geometric scene | 006, 010, 011, 014, 015, 016, 033, 043, 046, 047, 049, 050, 051, 053, 055, 056, 057, 078, 079, 082, 089 |
| Functions, bases and samples | 2D plots + coefficient rows | 002, 003, 005, 009, 038, 054, 081 |
| Inner products and quadratic geometry | 2D contours + 3D surface | 012, 013, 035, 039, 052 |
| Spectra and the complex plane | 2D points / contours + scalar panels | 017, 024, 042, 058, 062, 068, 073 |
| Evolution and convergence | 2D traces + optional geometric probe | 022, 023, 025, 061, 066, 088, 090 |
| Precision, work and experiments | 2D plots / counters / distributions | 020, 021, 030, 064, 067, 070, 087 |
| Sampling and spectral discretization | 2D nodes / stencils / contours | 026, 028, 029, 091, 092, 093 |
| Incidence, parity and dependency graphs | 2D / 3D graph | 008, 036, 045, 065, 075 |
| Meshes, coordinates and physical fields | 2D / 3D geometry | 040, 080, 083, 084, 094, 095 |
| Assumptions and licensed steps | Text + code-native node/edge diagram | 027, 072 |
| Composition and computation flow | 2D stages + linked arrays / vectors | 034, 085 |

## Reuse what is already there

The existing scene primitives, camera controls, plots, tables, surface patches,
matrices and semantic parameter route supply useful foundations. Linear maps,
covariance and tensor views contribute arrows, frames and decomposition stages.
Functions and harmonics contribute linked curves; flux, surfaces and fields
contribute domain geometry; discrete objects contribute graph presentation.
Keep mathematical kernels reusable independently of their visual appearance.

The present matrix view stores only nine values. These cards include 3-by-5 and
4-by-5 row-reduction problems, a 15-by-11 binary encoding matrix, complex data,
60-by-60 elimination and 100-by-100 eigenvector experiments. A new matrix board
needs a bounded viewport with scrolling/aggregation and exact selected-cell
readouts. Structural zeros must be distinguished from numerical small values,
using a visible, declared threshold for the latter. Uncomputed cells remain
unknown. Complex entries need real/imaginary or magnitude/phase views; binary
entries use exact GF(2) arithmetic.

Small real maps can use 2D/3D geometry directly. Higher-dimensional objects retain
their full numeric representation, with any geometric projection labeled.
Spherical harmonics are not a solver for a disk or cube. The lopsided-drum and
cube cards need actual discretized eigenproblem results, their boundary data,
normalization, residuals and convergence evidence. Display resolution is separate
from numerical resolution. A 21-by-21 patch is a presentation limit, not a
promise of ten-digit accuracy.

## How a card configures a graphic

The proposed adapter declares:

1. Source identity, subpart, revision, exact givens and any labeled repairs.
2. Domain and dimensions: real, complex, finite field, function space or mesh.
3. Local roles: given value, unknown, parameter, index and function input.
4. Component IDs, numeric/structural payloads, labels and allowed semantic actions.
5. Disclosure stage for every derived datum, label and geometric feature.
6. Check kind, measured quantity, tolerance, parameter sweep, and what the check establishes.

The domain/session owner computes once and publishes a bounded snapshot. The
3D view, matrix cells, diagram, plot and numerical table consume the same facts.
Inputs route back through that owner. Factorization convention, source/target
basis order, conjugation convention and boundary conditions are data, not UI
assumptions. Numerical kernels must not be inferred by parsing arbitrary LaTeX.
Author explicit adapters and retain pointers to the unchanged quoted question.

Large experiments can supply bounded presentation data from an independently
validated computation. Bind that result to source revision, inputs, seed, method,
precision and tolerance. Never substitute a small demo for the requested size
while labeling it the original exercise. Timings must come from actual runs;
operation counts and extrapolations are labeled separately.

The catalogue includes four concrete example bindings: 002 and 003 share the
same fitting component with different basis functions and sample nodes; 015
uses the supplied 3-by-2 map; 001 uses the supplied symbolic band mask. None
contains a solved answer or changes the learner's page.

## Respect the card's learning stages

The existing four exploration levels are subject progression. These exercise
cards additionally need disclosure stages: setup, help, working, check and
explicit solution reveal. They are different concepts.

At setup, show givens, shape, roles and constraints. Unknown factors, rank,
optimal fits or requested eigenvalues stay unknown. A fitted curve can reveal
its coefficients without printing them; a collapsed shape can reveal rank.
Therefore filter geometry and derived labels as well as text. Revealing a Method
fold permits only that fold's information. A learner-submitted operation can
produce its own working result without opening the answer key. The existing
ungated explorers remain usable as sandboxes; they are not automatically safe
exercise embeds.

Keep the four check kinds from `problems/README.md`: library, invariant, two
methods and planted. The future check panel shows actual measurements and the
stated tolerance. Sampled agreement does not prove a universally quantified
statement. Proof and setup cards keep their argument, dependency and review
state; a graph is an aid to reading the argument. Never write to the source
Attempt, setup, solve or Run fields from this graphic integration.

## Repeated sources and gaps

Four source exercises appear twice: 012/052, 022/061, 024/062 and 037/077.
They can share recipes while retaining separate page identities and learner
state. Matching locators suggest reuse; local givens and adaptations still need
comparison. 049 and 050 are a useful counterexample: their S matrices differ in
the sign of one entry, so a similar title is not permission to merge inputs.

Some binding blockers are local to one subpart: 014(d) has a missing final row;
043(a) has missing candidate vectors while the unit-square part has a usable
matrix. Card 009 explicitly retains multiple readings of missing signs. Card
040 contains a textual reconstruction of a missing figure; that provenance
must stay visible. Other pages resolve dependencies in Notation/Method folds.
Do not guess an absent figure or silently treat a reconstructed formula as the
verbatim original. The JSON lists these review notes without changing sources.

## First build

Implemented in [P046](P046_MATRIX_BOARD.md): six adapters, bounded native 2D
board, explicit working actions and text-only checks. Current generic sizes are
2–32; the larger experiments below remain future adapters.

Start with the variable-size matrix board and six card adapters: 001, 004, 018,
031, 044 and 059. These exercise band masks, rectangular row operations,
leading blocks, pivoting and block elimination. Keep the domain action owner
separate from the drawing component; use explicit matrix and permutation
conventions. Display input data during setup, learner operations during working,
and numerical/structural checks only in their allowed stages.

Then connect the common fitting view to 002/003/038 and the rectangular
vector-map view to 011/014(a-c)/015. This sequence creates reusable components
around real card data before adding the larger numerical experiments.

Validate the first board with pure CPU state and scene tests: invalid edits
are atomic, rectangular shapes stay intact, a row operation updates the matching
right-hand side, permutations use the declared convention, and structural masks
and disclosure stages cannot leak a solution. Use at least three instance sizes
where parameters vary. The user performs the visual test; no images or window
launches are authorized for the agent.

Source direction: `/Users/kogaryu/devil/99-red-booleans/problems/README.md`,
`TEMPLATE-math.md`, `NOTATION.md`, `ORDER.md`, `EXTRACT.md`, `work/pp.py`, and all
95 `math/*.md` questions. This catalogue is not loaded by CMake or the app yet.
