# Reusable math assets — foundation plan

Planning audit: 10 September 2026, through P068. No implementation or worker dispatch is part of this document.

The current registry contains **39 assets and 544 parameters**. The six category checklists contain **178 chapters**. The existing exercise appendix assigns **135 cards**. This plan identifies missing reusable capabilities and their build order; [MATH_ASSET_TODO.md](MATH_ASSET_TODO.md) and its six category documents remain the canonical chapter scope.

**Build a mathematical capability once, then author bounded configurations and examples against it.** Seventeen capability families below include extensions and compositions of existing assets; they do not imply seventeen mandatory new top-level labs. The first six are the recommended foundation wave.

## What is already reusable

| Existing foundation | Reuse now | Remaining distinction |
| --- | --- | --- |
| Finite maps, relations, groups and quotients | Fibers, BFS, matching, group laws, cosets, ideals and concrete witnesses | Current size limits are deliberate; general posets, category diagrams and modules are not already implemented. |
| Linear, QR, Polar, PSD, Norm, Distance and matrix figures | Vectors, decompositions, residuals, signed volumes and rank examples | A common subspace view and arbitrary factorization-stage producer are still missing. |
| Functions, Curves, Patch, Lathe, Surface and Flux | Curves, surfaces, sweeps, tangent/normal probes and existing integral constructions | The general Function lab currently exposes four fixed rules; domain-aware families and shared discretization are missing. |
| Harmonics, Membrane and Harmonic sphere | Phasors, modes, superposition and existing wave examples | General convolution, sampling conventions and transform-stage comparisons are missing. |
| Bayes, Binomial, Probability, Covariance and Simplex | Finite probability, conditioning, distributions already represented, covariance and information geometry | A common population/ensemble/CDF interface and shared stochastic histories are missing. |
| Shared geometry and controls | Indexed cells, arrows, linked plots, compact inspectors, picking and playback | These drawing primitives do not themselves supply an estimator, PDE solver, proof checker or other new mathematics. |

## Foundation order

| ID | Capability | Approach | Primary chapter routes | Minimum dependencies |
| --- | --- | --- | ---: | --- |
| F01 | Function families and domains | Extend Functions / Curves | 11 | None |
| F02 | Subspaces and operator geometry | Extend Linear / QR / Polar / Norm | 11 | None |
| F03 | Computation stages and iteration traces | New shared stage presentation; reuse matrix boards and existing playback | 14 | None |
| F04 | Signals, kernels and convolution | Extend Harmonics / Membrane | 17 | None |
| F05 | Distributions and sample ensembles | Extend Binomial / Bayes / Covariance / Simplex | 13 | None |
| F06 | Coordinate cells, local maps and slices | Extend Patch / Surface / Flux / Lathe / Tensor | 8 | None |
| F07 | Branching structures, posets and state diagrams | Extend Relations & Graphs; reuse finite maps | 17 | F03 for algorithm traces |
| F08 | Accumulation, partitions and quadrature | Extend Calculus / Functions / Lathe | 4 | F01 |
| F09 | Flows and phase portraits | Extend Vector Field / Oscillator / Curves | 2 | F03 |
| F10 | Constraints, feasible regions and loss surfaces | Extend Norm / Quadratic / PSD / Simplex | 3 | F02; F06 for mapped regions |
| F11 | Complex maps, contours and operator slices | New complex-map capability; reuse Gaussian / Roots / Curves / Surfaces | 8 | F01; F06 |
| F12 | Stochastic paths, histories and filtering | New shared path/history capability; reuse Probability / Curves / Covariance | 11 | F03; F05 |
| F13 | Triangle, hyperbola and spherical constructions | Extend Trig / Harmonic sphere | 5 | None for planar pilot; F06 for charts |
| F14 | Variations, cell fields and PDE approximations | Extend Patch / Membrane / Flux | 4 | F06; F08; F09 |
| F15 | Cell complexes and structured algebra diagrams | New typed composition capability; reuse Graph / Maps / Quotient / Tensor | 18 | F02; F03; F07 |
| F16 | Inference and estimator comparisons | Compose F05 with existing Bayes / Covariance / Quadratic | 7 | F05; F10; F12 for dependent samples or MCMC |
| F17 | Mass couplings and transport | New coupling capability; reuse Maps / Simplex / matrix cells | 1 | F05; F10 |

`R00` routes 24 chapters to an initial example using existing mathematics. Every chapter has one primary planning route in [MATH_ASSET_REUSE_MAP.tsv](MATH_ASSET_REUSE_MAP.tsv); many need additional families. Counts prioritize work, **not** percentage completion or proof coverage. Even R00 chapters remain open in the canonical checklist.

**Order:** preparation and one existing-asset pilot; F01–F06; then F07–F12 as dependencies allow. F13 can proceed early as an independent planar-triangle pilot. F14–F17 are specialist compositions after their required foundations. Advanced chapter content must be specified individually before production.

## Bounded foundation specifications

### F01 — Function families and domains

**Visible asset:** A curve or ribbon with movable roots, holes, poles, branches and neighborhood bands.

**First implementation:** Start with real factored polynomials of degree <=4, one rational family and explicit piecewise branches. One evaluator owns value/domain/derivatives; curves never bridge excluded inputs.

**Reuse:** Extend Functions / Curves. **Dependencies:** None.

**Independent checks:** Known factors and derivatives; excluded points stay excluded; one-sided limits checked analytically.

**Later recipe queue:** Polynomial roots and multiplicity; rational hole versus pole; inverse branches; epsilon-delta neighborhoods; partial-sum families.

**Primary chapter routes:** 0004, 0035, 0045, 0050, 0059, 0061, 0064, 0067, 0070, 0071, 0087. Full briefs remain in the category checklists.

### F02 — Subspaces and operator geometry

**Visible asset:** Linked input/output spaces containing bases, planes, fibers, residuals and singular directions.

**First implementation:** Real maps between dimensions <=3; expose four fundamental subspaces and coordinate changes using the existing numerical rank policy. Do not substitute a new rank rule inside geometry.

**Reuse:** Extend Linear / QR / Polar / Norm. **Dependencies:** None.

**Independent checks:** Rank-nullity, orthogonality and reconstruction; dependent/zero matrices; span-equivalent input bases.

**Later recipe queue:** Rank-nullity collapse; affine solution fibers; two-plane principal angle; nearly dependent basis; minimum-norm comparison.

**Primary chapter routes:** 0090, 0091, 0096, 0097, 0100, 0104, 0106, 0110, 0112, 0116, 0118. Full briefs remain in the category checklists.

### F03 — Computation stages and iteration traces

**Visible asset:** A matrix, graph or construction changes step by step while invariant/error/work readouts follow the same state.

**First implementation:** Immutable bounded stage records, current/next state and selected pivot/edge/term. First producer: small LU elimination with explicit row swaps. Each later algorithm supplies its own verified stage producer.

**Reuse:** New shared stage presentation; reuse matrix boards and existing playback. **Dependencies:** None.

**Independent checks:** P*A=L*U for the supported LU cases; stage invariants; deterministic restart; singular-pivot failure. Never animate invented intermediate mathematical states.

**Later recipe queue:** Pivoted LU; Cholesky success/failure; Householder reflection; power iteration; Euclidean-algorithm trace.

**Primary chapter routes:** 0002, 0007, 0010, 0069, 0092, 0099, 0101, 0105, 0109, 0113, 0117, 0123, 0124, 0125. Full briefs remain in the category checklists.

### F04 — Signals, kernels and convolution

**Visible asset:** Two traces slide across one another, their product forms an overlap area, and the output/spectrum updates beside them.

**First implementation:** Bounded real sample arrays, explicit sample spacing and periodic versus zero-padded boundary rules. Start with discrete convolution and a direct small DFT.

**Reuse:** Extend Harmonics / Membrane. **Dependencies:** None.

**Independent checks:** Impulse identity, direct convolution sums, circular wraparound, Parseval with the declared normalization, zero/DC signals.

**Later recipe queue:** Linear versus circular convolution; smoothing kernel; aliasing pair; Gibbs versus Fejer sums; windowed spectrum.

**Primary chapter routes:** 0034, 0041, 0044, 0046, 0047, 0048, 0049, 0051, 0052, 0054, 0055, 0056, 0057, 0058, 0072, 0073, 0155. Full briefs remain in the category checklists.

### F05 — Distributions and sample ensembles

**Visible asset:** Probability bars, a density sheet, cumulative area and repeated sample clouds share one distribution.

**First implementation:** Start with finite weighted outcomes plus named binomial/normal examples. Separate exact population truth, numerical approximation and seeded samples. Define endpoint and zero-mass behavior.

**Reuse:** Extend Binomial / Bayes / Covariance / Simplex. **Dependencies:** None.

**Independent checks:** Total mass and CDF monotonicity; known moments; exact finite enumeration; seed replay; samples cannot overwrite population parameters.

**Later recipe queue:** PMF-to-CDF sweep; sample-mean convergence; repeated confidence-interval inputs; dependence with fixed marginals; heavy-tail versus light-tail samples.

**Primary chapter routes:** 0136, 0138, 0140, 0141, 0143, 0144, 0148, 0150, 0151, 0156, 0163, 0171, 0176. Full briefs remain in the category checklists.

### F06 — Coordinate cells, local maps and slices

**Visible asset:** A small grid cell bends into a mapped patch or volume; tangents, area/volume scale and slices remain linked.

**First implementation:** Start with planar polar coordinates and a 3D affine map, then one smooth surface chart. Distinguish signed determinant, absolute volume and singular charts.

**Reuse:** Extend Patch / Surface / Flux / Lathe / Tensor. **Dependencies:** None.

**Independent checks:** Analytic Jacobians, orientation reversal, rank collapse, area/volume scaling and finite geometry at coordinate singularities.

**Later recipe queue:** Polar area cell; affine volume scaling; local linearization; oriented wedge area; level-set slices.

**Primary chapter routes:** 0060, 0063, 0065, 0082, 0085, 0086, 0103, 0108. Full briefs remain in the category checklists.

### F07 — Branching structures, posets and state diagrams

**Visible asset:** A bounded tree, layered DAG or Hasse diagram with selected paths, subsets, valuations or active states.

**First implementation:** Add reusable layouts and explicit node/edge labels first. Preserve the six-node graph contract until an independently budgeted larger representation is accepted. Start with one typed DAG/poset producer; logic, automata and counting require separate truth producers.

**Reuse:** Extend Relations & Graphs; reuse finite maps. **Dependencies:** F03 for algorithm traces.

**Independent checks:** Reachability/order consistency; no invented edges; bounded layouts; each producer has its own combinatorial oracle.

**Later recipe queue:** Recursion tree; subset lattice; truth circuit; finite automaton; conditional-expectation decision tree.

**Primary chapter routes:** 0011, 0012, 0119, 0121, 0126, 0129, 0130, 0131, 0132, 0133, 0134, 0137, 0139, 0145, 0146, 0147, 0168. Full briefs remain in the category checklists.

### F08 — Accumulation, partitions and quadrature

**Visible asset:** Intervals, cells or shells subdivide while signed contributions accumulate into a linked total.

**First implementation:** Start with midpoint/trapezoid/Simpson rules on one interval with explicit even-panel rules. Reuse F01 domains; add multidimensional cells only after F06.

**Reuse:** Extend Calculus / Functions / Lathe. **Dependencies:** F01.

**Independent checks:** Polynomial exactness where guaranteed; reversed bounds; convergence against analytic integrals; singular intervals rejected or explicitly split.

**Later recipe queue:** Quadrature comparison; integral-test rectangles; improper-tail cutoff; iterated-integration cells; finite measure partitions.

**Primary chapter routes:** 0062, 0075, 0077, 0079. Full briefs remain in the category checklists.

### F09 — Flows and phase portraits

**Visible asset:** A moving point traces a trajectory through a direction field with nullclines, equilibria and comparison solutions.

**First implementation:** Start with bounded 1D/2D autonomous systems and explicit initial conditions; distinguish analytic solutions from numerical integration. Add one integrator with controlled step size and failure reporting.

**Reuse:** Extend Vector Field / Oscillator / Curves. **Dependencies:** F03.

**Independent checks:** Known linear-system solutions, equilibrium persistence, step-refinement error, energy drift reported honestly, finite trajectory caps.

**Later recipe queue:** Exponential growth/decay; logistic flow; saddle and center; damped oscillator phase portrait; sensitivity to initial conditions.

**Primary chapter routes:** 0066, 0088. Full briefs remain in the category checklists.

### F10 — Constraints, feasible regions and loss surfaces

**Visible asset:** Constraint planes cut a bounded region while an objective surface and candidate/optimum markers show tradeoffs.

**First implementation:** Start with 2D convex quadratic objectives and linear constraints, with an optional height axis. Use analytic or enumerated small optima before adding general optimizers.

**Reuse:** Extend Norm / Quadratic / PSD / Simplex. **Dependencies:** F02; F06 for mapped regions.

**Independent checks:** Feasibility, boundary optima, empty/unbounded cases, primal residuals and independently computed optima.

**Later recipe queue:** Constrained least squares; active-set change; supporting plane; mean-variance toy frontier; robust-loss comparison.

**Primary chapter routes:** 0005, 0167, 0170. Full briefs remain in the category checklists.

### F11 — Complex maps, contours and operator slices

**Visible asset:** A complex input grid and contour map to an output grid; optional height shows a stated scalar such as magnitude.

**First implementation:** Start with z^2, reciprocal and a branch-defined logarithm, plus piecewise-linear contours. Matrix resolvents are a later dedicated numerical producer, not supplied by a complex surface alone.

**Reuse:** New complex-map capability; reuse Gaussian / Roots / Curves / Surfaces. **Dependencies:** F01; F06.

**Independent checks:** Known mapped points; branch-cut handling; contour orientation; simple analytic integrals; no connections across poles.

**Later recipe queue:** Complex sine slices; winding contour; simple residue example; stationary-phase comparison; later resolvent/pseudospectrum presets.

**Primary chapter routes:** 0043, 0074, 0076, 0080, 0107, 0111, 0114, 0115. Full briefs remain in the category checklists.

### F12 — Stochastic paths, histories and filtering

**Visible asset:** Repeatable random paths, time slices and history branches display what is known at each step.

**First implementation:** Start with seeded random walks and coupled Brownian refinements; coarse increments must equal sums of fine increments. Add conditional information explicitly before stopping-time, SDE or filtering producers.

**Reuse:** New shared path/history capability; reuse Probability / Curves / Covariance. **Dependencies:** F03; F05.

**Independent checks:** Seed replay; coupled refinement; quadratic variation behavior; no future information in conditional readouts; independent exact random-walk cases.

**Later recipe queue:** Brownian refinement; left-point Ito sums; Poisson arrivals; AR path; later Kalman tracking and stopping-time examples.

**Primary chapter routes:** 0142, 0153, 0159, 0161, 0164, 0165, 0166, 0169, 0172, 0173, 0178. Full briefs remain in the category checklists.

### F13 — Triangle, hyperbola and spherical constructions

**Visible asset:** A labelled triangle or hyperbolic/spherical arc carries lengths, angles, area and branch choices.

**First implementation:** Start with planar SSS/SAS and the SSA ambiguity. Hyperbolic and spherical geometry need separate metric conventions; they do not share Euclidean formulas.

**Reuse:** Extend Trig / Harmonic sphere. **Dependencies:** None for planar pilot; F06 for charts.

**Independent checks:** Triangle inequalities, sine/cosine laws, degenerate cases and both valid SSA solutions.

**Later recipe queue:** General triangle; ambiguous SSA; reciprocal trig ratios; hyperbola area parameter; later spherical triangle.

**Primary chapter routes:** 0031, 0036, 0040, 0042, 0053. Full briefs remain in the category checklists.

### F14 — Variations, cell fields and PDE approximations

**Visible asset:** A mesh carries nodal values, local perturbations, boundary conditions and residual/energy contributions.

**First implementation:** Start with a 1D heat or Poisson problem with fixed boundary conditions and a small grid. Extend to trial/test functions and variations only after the discretization is independently checked.

**Reuse:** Extend Patch / Membrane / Flux. **Dependencies:** F06; F08; F09.

**Independent checks:** Known solution/manufactured solution, boundary residuals, refinement behavior, conservation or energy identities appropriate to the scheme.

**Later recipe queue:** Heat diffusion; Green response; action perturbation; weak-form cells; later finite-element and moving-domain examples.

**Primary chapter routes:** 0078, 0081, 0083, 0084. Full briefs remain in the category checklists.

### F15 — Cell complexes and structured algebra diagrams

**Visible asset:** Oriented cells and layers of spaces/maps show boundaries, kernels, images and surviving classes.

**First implementation:** Start with finite chain complexes over a declared field and d*d=0, plus typed commuting squares. Higher categorical chapters often need schematic diagrams with stated assumptions, not a literal 3D realization.

**Reuse:** New typed composition capability; reuse Graph / Maps / Quotient / Tensor. **Dependencies:** F02; F03; F07.

**Independent checks:** Boundary squared zero, image contained in kernel, homology dimensions, map typing and claimed commuting relations.

**Later recipe queue:** Triangle boundary versus filled triangle; small chain complex; short exact sequence; filtration layers; later explicitly specified spectral-sequence pages.

**Primary chapter routes:** 0013, 0014, 0015, 0016, 0017, 0018, 0019, 0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027, 0028, 0029, 0135. Full briefs remain in the category checklists.

### F16 — Inference and estimator comparisons

**Visible asset:** Data points, likelihood/posterior curves and repeated estimates share explicit population truth.

**First implementation:** Start with a normal mean with known variance and a finite-grid posterior example. Treat each estimator/test as a specified mathematical producer.

**Reuse:** Compose F05 with existing Bayes / Covariance / Quadratic. **Dependencies:** F05; F10; F12 for dependent samples or MCMC.

**Independent checks:** Analytic likelihood/posterior cases; normalization; exact definitions of intervals and coverage; seeded repetition; no mixing posterior probability with repeated-sampling coverage.

**Later recipe queue:** Likelihood profile; repeated interval coverage; bias-variance comparison; posterior update; later robust fit and MCMC diagnostics.

**Primary chapter routes:** 0152, 0157, 0158, 0160, 0162, 0174, 0175. Full briefs remain in the category checklists.

### F17 — Mass couplings and transport

**Visible asset:** Weighted beads move through a coupling matrix while costs and marginal totals remain linked.

**First implementation:** Start with exact small 1D discrete transport and a fixed cost convention. General network solvers and entropy regularization are later producers.

**Reuse:** New coupling capability; reuse Maps / Simplex / matrix cells. **Dependencies:** F05; F10.

**Independent checks:** Row/column marginal sums, mass conservation, known 1D optimum, cost accounting and zero-mass cases.

**Later recipe queue:** Two-histogram transport; alternative couplings; monotone 1D optimum; later entropic interpolation and dual potentials.

**Primary chapter routes:** 0177. Full briefs remain in the category checklists.

## First six foundations: concrete downstream queue

These are proposed asset recipes, not dispatched tasks. Each row becomes a small task after its named mathematical behavior exists and its acceptance examples are frozen.

| Foundation | First three bounded recipes | New mathematics that must exist first |
| --- | --- | --- |
| F01 | `(x-1)^2(x+2)` roots; `(x^2-1)/(x-1)` with a retained hole; `1/(x-1)` pole | Coefficients, multiplicity and explicit excluded-domain representation. |
| F02 | A plane collapsing to a line; affine fibers of a rank-one map; two planes sharing a line | Domain/codomain subspaces and a consistent numerical rank contract. |
| F03 | A 3x3 LU case needing a row swap; a positive-definite Cholesky case; an indefinite Cholesky rejection | Separate verified LU/Cholesky producers; the stage player alone does not compute either. |
| F04 | Impulse preserves a signal; a short moving average; linear/circular convolution contrast | Declared boundary rules, sample spacing and exact convolution. |
| F05 | Weighted finite PMF/CDF; binomial mean/variance; repeated standardized sample means | Population-versus-sample interface, distribution-specific formulas and seeded repetition. |
| F06 | Polar annular-sector cell; reflected affine volume cell; a singular chart at the pole | Coordinate maps, derivatives and explicit singular cases. |

## Preparation required before parallel production

1. Freeze one current source snapshot including tracked modifications and untracked asset files. A fresh worktree from HEAD alone would omit much of this implementation. Preserve the separate textbook worker's live checkout.
2. Expose the existing bounded builder to independent family modules, with one representative extraction and geometry/action regression checks. `SnapshotBuilder` is currently private to the roughly 4,300-line `MathObjects.cpp`; do not have every worker add another large switch case there. Keep semantic dispatch and registration in one integration owner.
3. Freeze one typed family input/result contract, bounds, ordinary case and failure case at a time. Follow existing separate kernels and geometry builders such as `PatchGeometry`, `QrLeastSquares` and `FiniteQuotient`. Do not design a universal theorem or geometry language.
4. Use existing `MathObjectPreset` / semantic parameter actions for the first recipe packs. No external asset-recipe loader or automatic registration pipeline was found; writing JSON files does not make them runnable. Add a loader only if the pilot proves it necessary.
5. Run one bounded Terra pilot on existing mathematics, review correctness and repair cost, then expand to two or three workers with disjoint output directories. See [MATH_ASSET_BATCH_WORKFLOW.md](MATH_ASSET_BATCH_WORKFLOW.md).

## Scope boundaries

- This is an audit and production plan, not a dispatch or a claim that the missing families are built. No model setting has been changed.
- The routing TSV contains 178 chapter routes plus 135 exercise-card routes. Card IDs and chapter IDs are separate namespaces. Card routing reuses the existing appendix groups and still requires reading each card before a production task is frozen.
- Finite pictures of infinite-dimensional, asymptotic or higher-categorical topics must state the finite example or schematic interpretation. A shared diagram cannot establish a theorem it has not checked.
- Source notes and problem pages stay read-only. The textbook owner still owns prose, curriculum, practice, disclosure and lesson integration. Asset workers deliver mathematical objects, controls, configurations and evidence.
- No images, screenshots, offscreen captures, native windows or font probes. Automated validation and the user's later visual acceptance remain separate.
