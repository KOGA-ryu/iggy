# Math diagrams and model assets — to-do list

Planning snapshot: 9 September 2026. **Six subjects, 178 chapters/topics, 930 source entries, 135 exercise cards, and 36 existing Math Lab models (updated through P065).**

This is the complete chapter-level scope for the current learning library, not a separate new engine for every chapter. Each chapter gets a concise diagram brief and an interactive-model brief. Subcategories sit within their parent chapter; repeated material shares a model or preset. The broader unread book shelf is not counted as additional chapters here.

**D = diagrams and linked plots. M = interactive models/assets.** A model can be 2D or 3D. Abstract topics use finite examples and labelled diagrams where those explain the idea better. All boxes below mean chapter/family work remains to be organized and delivered; references to existing models mean reuse or extension, not that every chapter is already integrated.

**Latest asset: [P065 — Sets and Maps Lab](P065_SETS_AND_MAPS.md).** Four layers cover editable finite maps, fibers, composition and weighted outcomes, including conditional probability. These are shared foundations for algebra, discrete math and probability; chapter integration and larger/general constructions remain open.

## Existing models to reuse

These 36 model families are present in the current source registry. The list is an implementation inventory, not a new visual-acceptance claim.

| Area | Existing models |
| --- | --- |
| Algebra and sets | Algebra blocks; Symmetry; Modular drums; Gaussian lattice; Roots of unity; Boolean Solids Lab; Sets and Maps Lab |
| Trigonometry and harmonics | Trigonometry; Harmonics; Harmonic sphere |
| Calculus and geometry | Calculus; Functions; Surfaces; Vector fields; Flux shells; Curves and sweeps; Lathe Lab; Patch Lab |
| Linear algebra | Linear algebra; Tensor blocks; Covariance cloud; Quadratic forms; PSD cone; Norm balls; Distance Geometry Lab; Polar Decomposition Lab; QR and Least Squares Lab |
| Discrete math and probability | Discrete maths; Probability network; Binomial board; Bayesian cube; Probability Simplex Lab |
| Applied dynamics | Motion; Membrane Lab; Rigid-Body Rotation Lab; Bridge & Truss Lab |

Existing matrix boards, linear-system figures, and determinant-volume figures also provide reusable foundations.

## Shared build queue

These **26 shared families** collect the work below. Several extend existing labs; each can be delivered in small batches.

- [ ] **QR and least squares** — Orthogonal frames, projections, residuals, QR stages, and rank-deficient/minimum-norm examples. **P064:** real 3x2 asset delivered; general-size extensions and chapter integration remain.
- [ ] **Matrix factorizations and numerical work** — LU/Cholesky/Schur/Jordan/Smith stages, sparsity, pivoting, roundoff, work counts, and iterative solvers.
- [ ] **Subspaces and sensitivity** — Fundamental subspaces, principal angles, conditioning, perturbations, and Procrustes alignment.
- [ ] **Sets, maps, and logic** — Finite sets, quotients, function arrows, truth tables, circuits, and proof dependencies. **P065:** finite maps, fiber partitions, competing compositions and weighted pushforwards delivered; broader logic and chapter integration remain.
- [ ] **Finite algebra** — Groups, rings, ideals, modules, finite fields, extensions, and automorphism actions.
- [ ] **Categories and homological algebra** — Commutative diagrams, exact sequences, complexes, resolutions, filtrations, and spectral-sequence pages.
- [ ] **Polynomials and complex analysis** — Roots, rational functions, complex-function views, contours, residues, and algebraic-curve examples.
- [ ] **Triangles, inverse functions, and hyperbolic geometry** — General triangles, branch restrictions, circle/hyperbola links, and spherical triangles.
- [ ] **Fourier, transforms, and special functions** — Kernels, convergence, Gibbs, sampling, convolution, time-frequency views, and Bessel/Chebyshev examples.
- [ ] **Limits, series, and quadrature** — Epsilon–delta bands, sequence families, convergence radii, integration rules, and asymptotic errors.
- [ ] **ODEs and dynamical systems** — Slope fields, initial conditions, phase portraits, equilibria, and conserved quantities.
- [ ] **Variations and weak calculus** — Path/field perturbations, action, weak derivatives, test functions, and constrained stationarity.
- [ ] **Coordinate meshes and PDEs** — Jacobian cells, heat/Green kernels, spectral grids, finite elements, and domain eigenmodes.
- [ ] **Tensors and operator geometry** — Exterior products, tensor decompositions, matrix functions, numerical ranges, and resolvent/pseudospectrum views.
- [ ] **Graphs and combinatorics** — Graph editing, trees, matching, coloring, posets, hypergraphs, symmetry counts, and random structures.
- [ ] **Recursion, automata, and algorithms** — Recurrence trees, grammars, state machines, reduction diagrams, and bounded algorithm traces.
- [ ] **Distributions and concentration** — PMF/PDF/CDF links, repeated sampling, LLN/CLT, dependency effects, and tail-bound comparisons.
- [ ] **Inference and Bayesian updating** — Likelihoods, confidence intervals, tests, estimator error, posterior updates, and robust fitting.
- [ ] **Stochastic paths and Itô calculus** — Brownian refinement, martingales, quadratic variation, stochastic sums, diffusions, jumps, and Poisson arrivals.
- [ ] **Monte Carlo and posterior sampling** — Importance weights, MCMC/Gibbs/HMC trajectories, variational approximations, and mixing diagnostics.
- [ ] **Time series and filtering** — AR/ARCH/GARCH dynamics, autocovariance, volatility, CUSUM, Bayesian filters, and Kalman tracking.
- [ ] **Convex optimization and risk** — Feasible sets, duality, constrained least squares, line search, CVaR, frontiers, and toy pricing surfaces.
- [ ] **Information, decisions, and learning** — Bregman geometry, loss/risk/regret, finite learning classes, generalization, MDPs, and policy updates.
- [ ] **Extreme values and tails** — Regular variation, order statistics, block maxima, exceedances, heavy-tail fits, and tail dependence.
- [ ] **Optimal transport** — Couplings, mass movement, dual potentials, MMD comparisons, and entropic regularization.
- [ ] **Reusable chapter diagrams** — Labelled axes, arrows, tables, staged proofs, linked plots, and chapter presets built from the same model data.

## Chapter checklist

The three-digit numbers match the suffix of the library’s stable topic IDs: 001 means `topic_0001`. Chapter names and order are preserved, including separate theorem/example/proof chapters.

### Algebra — 29 chapters

Source: [Algebra notes](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/algebra.md>).

- [ ] **001 — Algebraic Structures** — **D:** Sets, structure hierarchy, operation tables, and maps between groups/rings/fields. **M:** Finite-set and operation-table explorer; reuse the symmetry and Gaussian-integer labs.
- [ ] **002 — Number Systems and Divisibility** — **D:** Number-set nesting, divisibility trees, Euclidean steps, and residue cycles. **M:** Extend modular drums with GCD, prime factors, and linked remainder views.
- [ ] **003 — Equations and Relations** — **D:** Function arrows, equivalence classes, and equation-versus-identity comparisons. **M:** Mapping machine with injection, surjection, bijection, composition, and quotient settings. **Asset progress:** [P065](P065_SETS_AND_MAPS.md) supplies finite maps, injection/surjection, fibers and composition; the full chapter remains open.
- [ ] **004 — Polynomials and Rational Expressions** — **D:** Coefficient strips, factor trees, repeated roots, poles, and excluded inputs. **M:** Extend Functions with draggable polynomial roots and rational-function holes/asymptotes.
- [ ] **005 — Inequalities and Expressions** — **D:** Number-line intervals, absolute-value distances, and AM–GM rectangle comparisons. **M:** Norm-ball comparisons plus an adjustable fixed-area rectangle/box.
- [ ] **006 — Symmetry and Group Actions** — **D:** Permutation cycles, orbit graphs, stabilizers, and coset partitions. **M:** Extend Symmetry with labelled polygons, cube actions, and noncommuting moves.
- [ ] **007 — Algebraic Methodology** — **D:** Substitution chains, reversible equation steps, and contradiction dependencies. **M:** Small composition/substitution sandbox; reuse existing equation and function models.
- [ ] **008 — Theorems** — **D:** Bézout, remainder, coset, factorization, CRT, and interpolation proof figures. **M:** Reuse modular, finite-group, polynomial, and interpolation models as theorem examples.
- [ ] **009 — Worked examples** — **D:** Step-by-step Euclidean, factorization, and cyclic-subgroup traces. **M:** Presets for the preceding algebra models; no separate model family.
- [ ] **010 — Proof sketches** — **D:** Assumption-to-conclusion maps and highlighted witnesses/counterexamples. **M:** Replay the chapter's small integer, polynomial, and subgroup examples.
- [ ] **011 — Algebraic Structures and Quotients** — **D:** Ideal lattices, quotient classes, and nilpotent multiplication chains. **M:** Extend Gaussian lattice and finite-ring tables with ideals and quotient projections.
- [ ] **012 — Advanced Ring-Theoretic Terms** — **D:** Ideal chains, Euclidean descent, and maximal-ideal quotient diagrams. **M:** Integer/polynomial ring examples with division steps and quotient-field checks.
- [ ] **013 — Algebraic and Module Extensions** — **D:** Fraction/localization maps, radical membership, and factorization comparisons. **M:** Finite examples of radicals and localization; labelled algebraic-set slices where useful.
- [ ] **014 — Advanced Module Theory** — **D:** Module/submodule inclusions, homomorphisms, exactness, and tensor grids. **M:** Lattice and torsion modules plus editable kernel/image diagrams; reuse Tensor blocks.
- [ ] **015 — Homological and Categorical Additions** — **D:** Lifting/extension diagrams, free generators, and torsion decomposition. **M:** Small module examples showing projective, injective, and flat behavior under stated assumptions.
- [ ] **016 — Homological Constructions** — **D:** Short exact sequences, direct sums/products, and pullback/pushout squares. **M:** Editable finite-set and vector-space constructions with synchronized element maps. **Asset progress:** [P065](P065_SETS_AND_MAPS.md) supplies finite-set maps and composition foundations; the full chapter remains open.
- [ ] **017 — Galois and Homological Structures** — **D:** Extension towers, conjugate roots, splitting fields, and subgroup/field correspondence. **M:** Extend Roots of unity with finite Galois examples and automorphism permutations.
- [ ] **018 — Category-Theoretic Extensions** — **D:** Category arrows, functor images, naturality squares, and adjunction correspondences. **M:** Composable finite-category diagram explorer, including representable-functor examples. **Asset progress:** [P065](P065_SETS_AND_MAPS.md) supplies finite commuting-route examples; the full chapter remains open.
- [ ] **019 — Algebraic Geometry Bridges** — **D:** Prime/ideal specialization diagrams, radicals, and equations versus zero sets. **M:** Finite-ring spectrum examples and simple algebraic curves; distinguish point sets from prime spectra.
- [ ] **020 — Advanced Homological Algebra** — **D:** Split sequences, lifting squares, and extension diagrams. **M:** Small exact-sequence and resolution explorer with kernel/image overlays.
- [ ] **021 — Homological and Derived Structures** — **D:** Chain complexes, projective resolutions, homology, Ext, and Tor stages. **M:** Bounded matrix complexes with cycles, boundaries, and surviving homology classes.
- [ ] **022 — Spectral and Category Extensions** — **D:** Ascending/descending chains, composition series, and simple-factor comparisons. **M:** Finite module/subspace filtration explorer with rearrangeable composition series.
- [ ] **023 — Adjunctions and Representability** — **D:** Hom-set correspondences, adjunction units, representable functors, and torsion pairs. **M:** Finite-category examples linked to existing map and submodule diagrams.
- [ ] **024 — Topos-like Limits and Constructions** — **D:** Inverse/direct systems, compatible tuples, and limit/colimit constructions. **M:** Finite inverse-system and pullback examples; finite truncations for infinite constructions.
- [ ] **025 — Spectral Sequences and Filtrations** — **D:** Filtration stacks, associated graded pieces, exact couples, and spectral-sequence pages. **M:** Page-by-page finite spectral-sequence board with differential arrows and surviving classes.
- [ ] **026 — Derived Category Stability** — **D:** Distinguished triangles, degree truncations, hearts, and long exact sequences. **M:** Small chain-complex examples with shifts, cohomology, and truncation controls.
- [ ] **027 — Higher Categorical Constructions** — **D:** Subcategory inclusions, localization arrows, and quotient/derived-equivalence diagrams. **M:** Finite diagram examples of killed objects and changed morphisms; no literal 3D quotient claim.
- [ ] **028 — Higher Homological and Noncommutative Constructions** — **D:** Tensor and Quillen adjunction diagrams, compactness patterns, and abelianization maps. **M:** Finite group abelianization and tensor examples; schematic diagrams for general categorical claims.
- [ ] **029 — Homological Stability and Higher Algebra** — **D:** Monoidal coherence, Koszul differentials, Ext stages, and Morita correspondences. **M:** Small Koszul complexes and matrix-ring/module examples, linked to the diagram workbench.

### Trigonometry — 29 chapters

Source: [Trigonometry notes](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/trigonometry.md>).

- [ ] **030 — Angle System** — **D:** Degree/radian arcs, coterminal turns, reference angles, and solid-angle patches. **M:** Extend unit circle and Harmonic sphere with arc-length and steradian controls.
- [ ] **031 — Right Triangle Definitions** — **D:** Similar right triangles and all six ratio constructions. **M:** Adjustable right triangle with linked side lengths, angle, and reciprocal ratios.
- [ ] **032 — Unit Circle Framework** — **D:** Unit-circle projections, quadrant signs, and exact special-angle triangles. **M:** Extend Trigonometry with a synchronized circle, triangle, and sine/cosine readout.
- [ ] **033 — Identities** — **D:** Pythagorean area layouts, angle-addition rotations, and product/sum identities. **M:** Linked rotating triangles and phasors for double-angle and identity experiments.
- [ ] **034 — Graphs and Periodic Behavior** — **D:** Amplitude, period, phase, and midline overlays. **M:** Extend Harmonics with synchronized waveform transformations and cycle markers.
- [ ] **035 — Inverse Trigonometry** — **D:** Restricted branches, reflected inverse graphs, and atan2 quadrants. **M:** Circle-and-inverse-function explorer with branch limits and discontinuity markers.
- [ ] **036 — Applications** — **D:** Sine/cosine laws, triangle area, ambiguous SSA cases, and harmonic motion. **M:** General triangle solver and adjustable rotating/spring models.
- [ ] **037 — Theorems** — **D:** Circle and triangle constructions for each identity and sine/cosine law. **M:** Reuse triangle, circle, and phasor presets; no separate theorem engine.
- [ ] **038 — Worked examples** — **D:** Exact-angle constructions and all solutions across repeated periods. **M:** Preset triangle and circle examples with synchronized solution markers.
- [ ] **039 — Proof sketches** — **D:** Identity derivations, branch assumptions, and periodicity proof sequences. **M:** Replay linked circle/triangle transformations under the stated assumptions.
- [ ] **040 — Trigonometric Identities and Extensions** — **D:** Euler correspondence, cofunctions, phase offsets, and hyperbolic area. **M:** Linked circle/hyperbola explorer; reuse phasors for complex exponentials.
- [ ] **041 — Extended Function Identities** — **D:** Hyperbolic graphs, inverse-cotangent branches, and Lissajous traces. **M:** Extend Harmonics with Lissajous frequency ratios; add linked hyperbolic controls.
- [ ] **042 — Hyperbolic and Inverse Extensions** — **D:** Inverse hyperbolic domains and sinc's removable singularity. **M:** Hyperbola and sinc explorer with valid-domain and limit controls.
- [ ] **043 — Complex-Analytic Trigonometric Extensions** — **D:** Real/imaginary slices of complex sine, cosine, and exponentials. **M:** Complex-function surfaces with linked input plane, output plane, and component views.
- [ ] **044 — Transform Applications** — **D:** Phasor sums, trig-substitution triangles, rotation maps, and Fourier coefficient bars. **M:** Reuse Harmonics and Linear maps; add linked coefficient/sample editing.
- [ ] **045 — Inverse and Parametric Identities** — **D:** Arcsec/arccsc branches, parametric circle tangents, and atan2 branch cuts. **M:** Parametric-circle and inverse-branch explorer with synchronized derivative arrows.
- [ ] **046 — Advanced Triangle and Wave Identities** — **D:** Triple-angle rotations, half-angle substitution, and travelling-wave characteristics. **M:** Linked phasor/triangle identities and an adjustable travelling string.
- [ ] **047 — Advanced Transform Identities** — **D:** Dirichlet/Fejér kernels, partial sums, energy sums, and Gibbs overshoot. **M:** Fourier convergence lab with selectable kernel, term count, and discontinuous targets.
- [ ] **048 — Operator-Based Trig Tools** — **D:** Convolution overlap, Hilbert phase shift, fractional Fourier rotation, and phase modulation. **M:** Signal-transform sandbox with paired input/output traces and time-frequency views.
- [ ] **049 — Fourier-Analytic Identities** — **D:** Fourier coefficients, Parseval energy, and harmonic mean-value circles. **M:** Extend Harmonics with energy accounting and harmonic-function averaging probes.
- [ ] **050 — Complex-Exponential Trig Synthesis** — **D:** Complex powers, root constellations, and cosine/Chebyshev correspondence. **M:** Link Roots of unity to rotating phasors and a Chebyshev polynomial explorer.
- [ ] **051 — Orthogonality and Harmonics** — **D:** Basis inner products, energy bars, Gibbs, Cesàro means, and kernel norms. **M:** Fourier convergence/orthogonality lab sharing the same coefficients and samples.
- [ ] **052 — Trigonometric Kernel Identities** — **D:** Dirichlet, Fejér, and Poisson kernels; spherical mode links. **M:** Kernel-controlled circle/disk extension plus existing Harmonic sphere modes.
- [ ] **053 — Advanced Harmonic Function Expansions** — **D:** Poisson summation, Jacobi–Anger/Bessel coefficients, and spherical triangles. **M:** Sampling-and-spectrum lab plus a draggable triangle on a sphere.
- [ ] **054 — Periodic Convolutions and Kernels** — **D:** Circular convolution, coefficient convolution, Fejér means, and band limits. **M:** Periodic signal mixer with wraparound, sample count, and filter controls.
- [ ] **055 — Asymptotic and Distributional Harmonics** — **D:** Oscillatory cancellation, sine/cosine integrals, and weighted impulse combs. **M:** Oscillatory-integral and sampling explorer; represent distributions by weighted impulses.
- [ ] **056 — Harmonic Signal Transformations** — **D:** Analytic-signal envelopes, Hilbert pairs, phase shifts, and sliding windows. **M:** Signal-analysis lab with envelope/phase controls and a movable Fourier window.
- [ ] **057 — Harmonic Kernels and Special Trigonometric Systems** — **D:** Kernel comparisons, Bessel sidebands, and Chebyshev/trigonometric basis links. **M:** Reusable special-function and Fourier-synthesis presets.
- [ ] **058 — Convergence and Gibbs-Type Phenomena** — **D:** Convergence away from jumps, Gibbs overshoot, energy, and coefficient decay. **M:** Fourier convergence lab with side-by-side partial sums and error traces.

### Calculus — 30 chapters

Source: [Calculus notes](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/calculus.md>).

- [ ] **059 — Limits and Continuity** — **D:** Epsilon–delta bands, one-sided limits, discontinuities, and intermediate values. **M:** Limit challenge with adjustable neighborhoods and a linked function probe.
- [ ] **060 — Differentiation** — **D:** Secants becoming tangents, derivative rules, and composition chains. **M:** Extend Functions and Surfaces with local linearization and Hessian probes.
- [ ] **061 — Applications of Derivatives** — **D:** Critical points, monotonicity, concavity, inflection, and MVT secants. **M:** Function/terrain explorer with tangent, curvature, and extremum markers.
- [ ] **062 — Integration** — **D:** Signed area, Riemann sums, antiderivative families, and FTC correspondence. **M:** Extend Calculus and Functions with linked accumulation and moving-bound views.
- [ ] **063 — Integration Techniques** — **D:** Substitution grids, integration-by-parts areas, and improper-integral tails. **M:** Revolution/area models with variable-change and cutoff controls.
- [ ] **064 — Sequences and Series** — **D:** Sequence neighborhoods, Cauchy tails, comparison envelopes, and convergence radii. **M:** Sequence/series lab with term count, partial sums, remainder, and domain controls.
- [ ] **065 — Multivariable Calculus** — **D:** Partial derivatives, mixed changes, double-integral cells, and polar area elements. **M:** Extend Surfaces/Patch with movable cells, tangent planes, and coordinate meshes.
- [ ] **066 — Differential Equations** — **D:** Slope fields, separating variables, integrating factors, and initial conditions. **M:** First-order ODE explorer with editable initial points and solution families.
- [ ] **067 — Theorems** — **D:** FTC, MVT/Rolle, extrema, Taylor remainder, and integral-test diagrams. **M:** Reuse function, integration, and series models with theorem-specific presets.
- [ ] **068 — Worked examples** — **D:** Differentiation traces, definite-area steps, MVT witnesses, and ODE trajectories. **M:** Presets for Functions, Calculus, and the ODE explorer.
- [ ] **069 — Proof sketches** — **D:** Assumption diagrams and linked derivative/integral proof steps. **M:** Reuse preceding examples with highlighted points, intervals, and limiting stages.
- [ ] **070 — Calculus Convergence Extensions** — **D:** Uniform versus pointwise convergence, arc-length chords, and improper tails. **M:** Curve refinement, sequence families, and ODE initial-condition comparisons.
- [ ] **071 — Advanced Calculus Concepts** — **D:** Uniform continuity bands, singular integrals, absolute continuity, and path work. **M:** Extend Functions, Curves, and Vector fields with interval/path perturbations.
- [ ] **072 — Transform Methods and Asymptotics** — **D:** Fourier/Laplace transform pairs, convolution, asymptotic scales, and PDE families. **M:** Transform lab plus adjustable heat/wave examples using existing field surfaces.
- [ ] **073 — Laplace and Asymptotic Methods** — **D:** Laplace inversion, convolution, Fourier pairs, and asymptotic error comparisons. **M:** Shared transform/asymptotics lab with parameter and truncation controls.
- [ ] **074 — Complex Integral Operators** — **D:** Contours, poles, residues, deformation, and large-arc estimates. **M:** Complex-plane contour lab with editable paths and linked integral accumulation.
- [ ] **075 — Geometric and Asymptotic Analysis** — **D:** Arc-length elements, shell volumes, parameter differentiation, and dominant balance. **M:** Extend Curves and Lathe; add parameter-dependent integrals and scale comparisons.
- [ ] **076 — Special Integrals and Advanced Asymptotics** — **D:** Laplace peaks, stationary phase, saddle paths, and Gamma/Beta integrands. **M:** Integral-asymptotics lab with concentration, oscillation, and complex contour controls.
- [ ] **077 — Numerical Integration and Functional Expansions** — **D:** Rectangle/trapezoid/parabola panels, endpoint corrections, and error rates. **M:** Quadrature comparison lab with shared function, grid, and convergence plot.
- [ ] **078 — Distribution and Variational Techniques** — **D:** Test-function pairings, delta approximations, weak derivatives, and first variations. **M:** Curve/field variation lab with movable perturbations and action readouts.
- [ ] **079 — Measure and Distribution Techniques** — **D:** Measure partitions, dominating envelopes, weak derivatives, and integration order. **M:** Finite measure/integration lab with shared domains and explicit limiting sequences.
- [ ] **080 — Asymptotic Integral Methods** — **D:** Steepest-descent paths, asymptotic approximations, Mellin scaling, and remainder plots. **M:** Reuse complex-contour and quadrature labs with asymptotic method presets.
- [ ] **081 — Advanced Variational and Integral Geometry** — **D:** Variation ribbons, constraints, Euler–Lagrange stationarity, and Green boundaries. **M:** Editable path/action model plus existing Vector fields and Flux shells.
- [ ] **082 — Stokes and Divergence Framework** — **D:** Oriented boundaries, Stokes/divergence balance, and Jacobian volume cells. **M:** Extend Flux shells with linked boundary/surface/volume and coordinate changes.
- [ ] **083 — Transform Kernels and PDE Links** — **D:** Heat kernels, Green responses, convolution, and transformed PDE modes. **M:** Heat/diffusion lab with source, boundary, time, and mode controls.
- [ ] **084 — Variational PDE and Distributional Calculus** — **D:** Weak-form pairings, trial/test spaces, Sobolev contributions, and constrained stationarity. **M:** Small finite-element field model with residual and energy views.
- [ ] **085 — Geometric Measure and Operator Theory** — **D:** Level-set/coarea slices, BV jumps, moving boundaries, and Green responses. **M:** Level-set slicing lab and deformable-domain PDE examples.
- [ ] **086 — Functional-Analytic Differential Structure** — **D:** Directional versus full linearization, inverse-map neighborhoods, and Morse forms. **M:** Extend Surfaces/Patch with local derivative maps and critical-point neighborhoods.
- [ ] **087 — Measure and Geometric Integration** — **D:** Dominated families, iterated integrals, equicontinuity, and Sobolev norms. **M:** Function-family explorer with cutoff, oscillation, smoothness, and norm controls.
- [ ] **088 — Nonlinear Dynamics and PDE Invariants** — **D:** Phase portraits, invariant level sets, conservation flux, and weak-limit sequences. **M:** Dynamical-systems lab with trajectories and conserved quantities; finite PDE examples.

### Linear Algebra — 30 chapters

Source: [Linear Algebra notes](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/linear_algebra.md>).

- [ ] **089 — Vector Foundation** — **D:** Vector components, head-to-tail sums, dot products, and norm comparisons. **M:** Extend Linear maps and Norm balls with shared vector probes.
- [ ] **090 — Vector Spaces and Subspaces** — **D:** Spans, dependence, basis coordinates, and dimension changes. **M:** Draggable vectors forming lines/planes/space, with dependent directions highlighted.
- [ ] **091 — Linear Maps** — **D:** Domain/codomain maps, kernel fibers, image, and rank-nullity. **M:** Linear-map lab with synchronized input/output spaces and collapsible dimensions.
- [ ] **092 — Matrix Algebra** — **D:** Row/column multiplication, transpose, inverse composition, and orthogonality. **M:** Reuse matrix boards, Linear maps, and Polar; add paired transformation stages.
- [ ] **093 — Determinants** — **D:** Signed areas/volumes, cofactor pieces, and multiplicative transformation stages. **M:** Reuse determinant volume figures; extend to cofactor and composition examples.
- [ ] **094 — Linear Systems** — **D:** Pivot paths, row operations, echelon structure, rank, and free variables. **M:** Reuse matrix board and plane-intersection figures with linked row-operation states.
- [ ] **095 — Orthogonality** — **D:** Projection triangles, residual orthogonality, and Gram–Schmidt stages. **M:** QR and least-squares lab with draggable columns and stepwise orthonormal frames. **Asset progress:** [P064](P064_QR_LEAST_SQUARES.md) supplies real 3x2 QR/projection/minimum-norm foundations; the full chapter remains open.
- [ ] **096 — Spectral Theory** — **D:** Invariant directions, eigenvalue multiplicity, SVD axes, and null-space collapse. **M:** Extend Linear maps with paired spectral and SVD comparisons.
- [ ] **097 — Theorems** — **D:** Rank-nullity, basis extraction, Cauchy–Schwarz, and determinant identities. **M:** Reuse vectors, determinant volumes, spectra, and subspace models as theorem presets.
- [ ] **098 — Worked examples** — **D:** Inverse, rank/nullity, dot-product, and eigenpair worked sequences. **M:** Existing linear/matrix models with frozen example data and step markers.
- [ ] **099 — Proof sketches** — **D:** Basis, rank, determinant, and spectral proof dependency diagrams. **M:** Small witness/counterexample presets using existing vector and matrix models.
- [ ] **100 — Extended Linear Concepts** — **D:** Four fundamental subspaces, affine offsets, and orthogonal complements. **M:** Subspace geometry lab with separate domain/codomain panels and solution fibers.
- [ ] **101 — Further Linear Algebra Terms** — **D:** Adjugate geometry, trace, Gram matrices, bilinear forms, and Jordan chains. **M:** Extend Quadratic/Tensor views; add a Jordan shear-and-chain model.
- [ ] **102 — Advanced Linear Operators** — **D:** QR/SVD stages, pseudoinverse solutions, and spectral-radius evolution. **M:** QR/least-squares lab plus minimum-norm and rank-deficient cases. **Asset progress:** [P064](P064_QR_LEAST_SQUARES.md) supplies real 3x2 QR/projection/minimum-norm foundations; the full chapter remains open.
- [ ] **103 — Advanced Linear Operator Extensions** — **D:** Dual vectors, adjoints, bilinear pairings, quadratic forms, and tensor products. **M:** Extend Tensor blocks and Quadratic forms with primal/dual coordinate views.
- [ ] **104 — Structured Linear Spaces** — **D:** Norm/inner-product comparisons, completion sequences, and compact-operator examples. **M:** Finite normed-space and singular-value-decay models; label infinite-dimensional analogies.
- [ ] **105 — Operator Decompositions** — **D:** LU, QR, Cholesky, SVD, and polar factor diagrams. **M:** Factorization lab sharing input matrices with existing Polar and SVD models. **Asset progress:** [P064](P064_QR_LEAST_SQUARES.md) supplies real 3x2 QR/projection/minimum-norm foundations; the full chapter remains open.
- [ ] **106 — High-Order Operator Structures** — **D:** Normal/self-adjoint/unitary comparisons, projections, and spectral functions. **M:** Operator lab with eigenvalue maps, invariant axes, and matrix-function controls.
- [ ] **107 — Spectral and Operator Analysis** — **D:** Operator norms, spectra, resolvents, and kernel/cokernel/index bookkeeping. **M:** Resolvent/pseudospectrum lab plus explicitly finite operator examples.
- [ ] **108 — Tensor, Bilinear and Multilinear Forms** — **D:** Tensor products, alternating areas/volumes, symmetric powers, and contractions. **M:** Extend Tensor blocks with wedge products, oriented cells, and symmetry controls.
- [ ] **109 — Tensor and Factorization Structures** — **D:** Kronecker blocks, tensor-rank sums, Schur complements, and pseudoinverse paths. **M:** Linked block-matrix/tensor lab with low-rank reconstruction controls.
- [ ] **110 — Operator Algebra and Geometry** — **D:** Polar factors, spectral functions, C*-identities, and CP rank-one terms. **M:** Reuse Polar and Tensor; add finite matrix-algebra and tensor-decomposition examples.
- [ ] **111 — Structured Transform Methods** — **D:** Schur forms, numerical ranges, spectral-radius maps, and matrix functions. **M:** Complex operator lab with unitary basis changes and sampled numerical-range boundaries.
- [ ] **112 — Structured Decompositions and Stability** — **D:** Factorization stability, Jordan sensitivity, singular values, and conditioning. **M:** Matrix perturbation lab comparing QR, Cholesky, eigenvectors, and SVD.
- [ ] **113 — Geometric and Structured Factorizations** — **D:** Householder mirrors, Givens rotations, Hessenberg/bidiagonal stages, and CS blocks. **M:** Stepwise orthogonal-factorization lab with linked vectors and sparsity patterns.
- [ ] **114 — Functional-Analytic Spectral Tools** — **D:** Resolvent growth, operator-norm bounds, compact spectra, and Fredholm solvability. **M:** Reuse operator/resolvent lab with finite approximations and compatibility conditions.
- [ ] **115 — Operator Perturbation and Stability** — **D:** Eigenvalue perturbation, Bauer–Fike disks, pseudospectra, and backward errors. **M:** Perturbation lab with editable disturbances and linked complex-plane contours.
- [ ] **116 — Subspace Approximation and Geometry** — **D:** Principal angles, subspace distances, perturbation bounds, and Procrustes alignment. **M:** Two-subspace alignment lab with adjustable frames and point-cloud correspondences.
- [ ] **117 — Canonical Decompositions and Matrix Geometry** — **D:** Polar/QR/Schur/Jordan/Smith forms and their allowed transformations. **M:** Shared factorization lab; exact integer matrix board for Smith normal form.
- [ ] **118 — Unitary Orbits and Operator Geometry** — **D:** Unitary orbits, congruence/equivalence, numerical range, and Rayleigh quotients. **M:** Extend Quadratic/Polar views with complex unitary transformations and orbit samples.

### Discrete Math — 30 chapters

Source: [Discrete Math notes](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/discrete_math.md>).

- [ ] **119 — Logic and Proof** — **D:** Truth tables, logical equivalences, implication chains, and contradiction trees. **M:** Logic-gate and proof-step sandbox with selectable valuations.
- [ ] **120 — Set Theory Foundations** — **D:** Venn regions, Cartesian products, function arrows, and quotient partitions. **M:** Finite sets/maps explorer; reuse Boolean solids for spatial set examples. **Asset progress:** [P065](P065_SETS_AND_MAPS.md) supplies finite-set maps, fibers and quotient-class foundations; the full chapter remains open.
- [ ] **121 — Counting and Combinatorics** — **D:** Permutation/combination trees, Pascal layers, inclusion–exclusion, and Catalan paths. **M:** Counting lab with selectable arrangements, lattice paths, and generating counts.
- [ ] **122 — Graph Theory** — **D:** Adjacency, degree, subgraphs, Euler trails, and Hamiltonian routes. **M:** Editable graph lab extending the existing cube-route model.
- [ ] **123 — Number Theory** — **D:** Prime factors, divisor lattices, totients, and modular cycles. **M:** Extend Modular drums with exact integer number-theory presets.
- [ ] **124 — Recursion and Sequences** — **D:** Recursion trees, recurrence tables, induction ladders, and lattice dependencies. **M:** Sequence builder with stepwise recurrence and two-index examples.
- [ ] **125 — Algorithms and Complexity** — **D:** Growth curves, operation counts, and input-size comparisons. **M:** Algorithm-trace lab with measured/count-based growth views kept distinct.
- [ ] **126 — Boolean Algebra and Logic Circuits** — **D:** Gate circuits, truth tables, Boolean expression trees, and equivalence routes. **M:** Editable circuit/valuation lab linked to set and Boolean-solid truth tables.
- [ ] **127 — Theorems** — **D:** Pigeonholes, incidence counts, matching witnesses, and modular theorem examples. **M:** Reuse counting, graph, and modular models with theorem-specific presets. **Asset progress:** [P065](P065_SETS_AND_MAPS.md) supplies finite pigeonhole/collision examples; the full chapter remains open.
- [ ] **128 — Worked examples** — **D:** Occupancy, inclusion–exclusion counts, modular powers, and handshaking traces. **M:** Small presets for existing/shared counting and graph models.
- [ ] **129 — Proof sketches** — **D:** Counting proof steps, graph assumptions, and induction witnesses. **M:** Replay chapter examples and counterexamples through shared graph/set tools.
- [ ] **130 — Advanced Discrete Topics** — **D:** Bipartite matching, SCC condensation, topological order, and generating functions. **M:** Graph lab plus sequence/coefficient views with shared combinatorial examples.
- [ ] **131 — Discrete Extensions** — **D:** Trees, spanning trees, isomorphism maps, colorings, and automata states. **M:** Editable graph and automaton lab with traversal, coloring, and relabelling controls.
- [ ] **132 — Advanced Discrete Structures** — **D:** Planar embeddings, DAG orderings, pigeonholes, and language transitions. **M:** Graph/automaton lab with crossing, dependency, and accepted-word views.
- [ ] **133 — Computability and Formal Systems** — **D:** Automata, grammars, parse trees, generating functions, and reduction diagrams. **M:** Finite-machine and grammar explorer; bounded NP-completeness examples.
- [ ] **134 — Advanced Counting Structures** — **D:** Burnside fixed points, Pólya colors, Möbius inversion, and Catalan bijections. **M:** Symmetry/counting lab with recolorable necklaces, polygons, and posets.
- [ ] **135 — Finite Construction Systems** — **D:** Prüfer coding, tree counts, Euler characteristic, and graph coloring. **M:** Graph construction lab plus simple polyhedral cell decompositions.
- [ ] **136 — Randomized and Finite Methodologies** — **D:** Random graphs, walks, transition probabilities, and Monte Carlo estimates. **M:** Extend Probability network with seeded random graphs and repeated experiments.
- [ ] **137 — Advanced Counting and Complexity** — **D:** Reduction maps, counting trees, randomized approximation, and complexity relations. **M:** Small algorithm/counting experiments; schematic diagrams for complexity classes.
- [ ] **138 — Advanced Probabilistic Counting** — **D:** Expectation decomposition, probability bounds, and martingale histories. **M:** Random-experiment lab with exact small cases and empirical bound comparisons.
- [ ] **139 — Probabilistic Method in Discrete Math** — **D:** Bad-event dependency graphs, union bounds, conditional choices, and rounding. **M:** Probabilistic-method sandbox with small instances and witness construction.
- [ ] **140 — Randomized Algorithmic Counting** — **D:** Variance/covariance contributions, Chernoff envelopes, and algorithm outcome/runtime distributions. **M:** Seeded randomized-algorithm comparison with Monte Carlo and Las Vegas examples.
- [ ] **141 — Extremal and Probabilistic Structures** — **D:** Tail-bound comparisons, dependency effects, entropy counts, and concentration. **M:** Shared concentration lab with adjustable distributions and event dependencies.
- [ ] **142 — Martingale Concentration Toolkit** — **D:** Filtrations, martingale increments, stopping rules, and bounded-difference envelopes. **M:** Branching-history/path lab with conditional means and stopping-time controls.
- [ ] **143 — Large Deviations and Random Structures** — **D:** Rate functions, exponential tilting, empirical histograms, and shattering diagrams. **M:** Large-deviation lab linked to Simplex/KL; finite VC-classification examples.
- [ ] **144 — Combinatorial Probability at Scale** — **D:** Resampling influences, bounded differences, threshold events, and convex-distance illustrations. **M:** Random graph/Boolean-cube experiments with coordinate-resampling controls.
- [ ] **145 — Derandomization and Pseudorandomness** — **D:** Conditional-expectation decisions, k-wise independence, small bias, and generator graphs. **M:** Finite bit-space explorer with exact distribution tables and derandomization steps.
- [ ] **146 — Ramsey and Extremal Random Structures** — **D:** Ramsey edge colors, extremal graphs, hypergraph containers, and threshold plots. **M:** Small graph/hypergraph lab with density and random-edge controls.
- [ ] **147 — Extremal Combinatorial Theorems** — **D:** Hall neighborhoods, Sperner lattices, intersecting families, and combinatorial lines. **M:** Finite-set/poset lab with matching, antichain, and arithmetic-progression examples.
- [ ] **148 — Advanced Extremal and Probabilistic Tools** — **D:** Dependency graphs, Chernoff/Janson envelopes, Erdős–Rényi graphs, and thresholds. **M:** Reuse random graph and concentration labs with linked event-count statistics.

### Probability and Statistics — 30 chapters

Source: [Probability and Statistics notes](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/probability_statistics.md>).

- [ ] **149 — Probability Foundations** — **D:** Sample spaces, event intersections, conditioning, and Bayes trees. **M:** Reuse Bayesian cube; add general finite-event partitions and weighted outcomes. **Asset progress:** [P065](P065_SETS_AND_MAPS.md) supplies weighted finite outcomes and conditioning on fibers; the full chapter remains open.
- [ ] **150 — Random Variables** — **D:** Outcome-to-value maps, PMF/PDF/CDF links, moments, and generating functions. **M:** Distribution lab with linked probability bars/density, cumulative area, and moment controls. **Asset progress:** [P065](P065_SETS_AND_MAPS.md) supplies finite outcome-to-value maps and pushforward mass; the full chapter remains open.
- [ ] **151 — Distribution Theory** — **D:** Bernoulli/binomial/normal comparisons, sample means, LLN, and CLT panels. **M:** Extend Binomial board with repeated sampling and standardized-sum convergence.
- [ ] **152 — Statistical Inference** — **D:** Likelihood profiles, estimator sampling distributions, and null/tail regions. **M:** Inference lab with sample size, parameters, estimator, and test-statistic controls.
- [ ] **153 — Stochastic Processes** — **D:** Markov transitions, stationary mass, Brownian paths, Itô sums, and measure changes. **M:** Extend Probability network; add stochastic paths with linked time slices and histograms.
- [ ] **154 — Foundational Probability and Inference** — **D:** Random walks, covariance ellipses, Poisson counts, and entropy bars. **M:** Reuse Covariance/Simplex; add Poisson arrivals and repeated-walk distributions.
- [ ] **155 — Risk and Learning Core Terms** — **D:** Variance decompositions, covariance directions, and characteristic-function traces. **M:** Extend Covariance cloud with complex characteristic functions and distribution comparisons.
- [ ] **156 — Decision and Concentration** — **D:** Conditional partitions, conditional means, total probability, and Chebyshev tails. **M:** Finite-event/conditional-expectation lab with synchronized cubes and bound plots. **Asset progress:** [P065](P065_SETS_AND_MAPS.md) supplies finite event/fiber conditioning and renormalization; the full chapter remains open.
- [ ] **157 — Estimation and Testing Extensions** — **D:** Repeated confidence intervals, distributional convergence, and KL comparisons. **M:** Sampling/inference lab linked to Simplex's relative-entropy geometry.
- [ ] **158 — Bayesian Foundations** — **D:** Prior × likelihood → posterior, evidence normalization, and MAP markers. **M:** Extend Bayesian cube into finite-grid and conjugate posterior update examples.
- [ ] **159 — Time Series and Risk** — **D:** Autocovariance lags, AR dependence, quantile tails, and expected shortfall. **M:** Time-series/risk lab with lag, coefficient, sample, and tail-level controls.
- [ ] **160 — Learning Geometry** — **D:** Log-likelihood curvature, Fisher information, Cramér–Rao bounds, and Bregman gaps. **M:** Likelihood-surface lab linked to convex tangent-plane geometry.
- [ ] **161 — Computational Methods** — **D:** Monte Carlo errors, importance weights, Markov proposals, and mixing diagnostics. **M:** Sampling lab with target/proposal controls and repeatable chains.
- [ ] **162 — Estimator Quality** — **D:** Bias/variance/MSE decomposition, consistency plots, and transformed expectations. **M:** Estimator comparison lab with repeated samples and known population truth.
- [ ] **163 — Dependence and Tail Risk** — **D:** Joint versus marginal ranks, copula surfaces, and joint-tail events. **M:** Dependence lab with adjustable copula parameters and simultaneous extremes.
- [ ] **164 — Stochastic Calculus Core** — **D:** Filtrations, martingale increments, quadratic variation, and left-point stochastic sums. **M:** Brownian/Itô path lab with shared increments and refinement controls.
- [ ] **165 — Mathematical Finance Mechanics** — **D:** GBM paths, drift changes, replication flows, and Black–Scholes surfaces. **M:** Toy pricing/diffusion model with explicit assumptions and linked payoff/PDE views.
- [ ] **166 — Inferential Learning for Time-Dependent Data** — **D:** Information revealed over time, density reweighting, cross-entropy, and transport comparisons. **M:** Finite filtered-probability examples linked to information and transport labs.
- [ ] **167 — Decision and Estimation Tradeoffs** — **D:** Loss tables, utility curves, Bayes-risk envelopes, and cumulative regret. **M:** Decision sandbox with adjustable actions, outcomes, costs, and repeated choices.
- [ ] **168 — Sequential and Online Learning** — **D:** MDP graphs, Bellman backups, value tables, and temporal-difference errors. **M:** Small gridworld/graph lab with policy, reward, discount, and transition controls.
- [ ] **169 — Modern Risk Measures** — **D:** Peak-to-trough drawdowns, equity paths, and return/risk comparisons. **M:** Synthetic-path risk lab with drawdown, Sharpe, and Calmar readouts.
- [ ] **170 — Optimization and Risk Measures** — **D:** Acceptance sets, entropic/CVaR objectives, and mean–variance frontiers. **M:** Convex portfolio/risk model linked to PSD, Simplex, and optimization views.
- [ ] **171 — Information and Generalization** — **D:** Hypothesis shattering, Rademacher signs, PAC-Bayes terms, and information tradeoffs. **M:** Finite learning-class lab with sample size, noise, prior, and complexity controls.
- [ ] **172 — Financial Time-Series Dynamics** — **D:** ARCH/GARCH variance recursion, volatility clustering, and scaling plots. **M:** Synthetic time-series lab with conditional variance and roughness comparisons.
- [ ] **173 — Detection and Filtering** — **D:** CUSUM paths, likelihood ratios, recursive updates, and Kalman covariance ellipses. **M:** Tracking/filtering lab with noisy motion, sensor, and process controls.
- [ ] **174 — Robustness and Misspecification** — **D:** Influence curves, outlier leverage, Huber loss, and decision margins. **M:** Robust-fit lab with draggable outliers and linked loss/fit changes.
- [ ] **175 — Bayesian Computation** — **D:** HMC trajectories, Gibbs coordinate steps, variational contours, and autocorrelation/ESS. **M:** Posterior-sampling lab with target density, step size, and sampler controls.
- [ ] **176 — Extreme-Value Theory** — **D:** Block maxima, threshold exceedances, tail fits, and extreme-value scaling. **M:** Extreme-value lab with sample, block/threshold, tail-index, and fit controls.
- [ ] **177 — Optimal Transport and Generative Learning** — **D:** Transport couplings, mass movement, dual potentials, MMD, and entropic smoothing. **M:** Optimal-transport lab between point clouds/histograms with cost and regularization controls.
- [ ] **178 — Stochastic Models for Finance and AI** — **D:** Jump paths, diffusion versus Lévy increments, time changes, and policy-gradient estimates. **M:** Jump-process and small-policy experiments sharing stochastic-path/MDP components.

## Exercise-card coverage — all 135 current cards

This compact appendix covers the numerical/book exercise lane as well as the six-subject library. Every current card is assigned once below; many will reuse supporting diagrams from other rows. Cards 001–095 retain the existing graphics-map grouping; cards 096–135 extend the scope here. These are model/diagram tasks, not claims that card adapters or solutions have been implemented.

The earlier [exercise graphics plan](/Users/kogaryu/iggy3d/paths/docs/EXERCISE_GRAPHICS.md) contains more detail for its original 95-card snapshot.

- [ ] **Matrix boards and factorization diagrams** — Extend indexed/block matrices, sparsity, pivots, permutations, packed storage, and factor stages. **Cards:** 001, 004, 007, 018, 019, 031, 032, 037, 041, 044, 048, 059, 060, 063, 069, 071, 074, 076, 077, 086.
- [ ] **Vectors, bases, and linear maps** — Reuse linear/polar geometry; add QR, coordinate changes, pseudoinverses, and rank-one updates. **Cards:** 006, 010, 011, 014, 015, 016, 033, 043, 046, 047, 049, 050, 051, 053, 055, 056, 057, 078, 079, 082, 089, 130.
- [ ] **Functions, bases, and fits** — Add editable samples, polynomial/trigonometric bases, function-space constraints, and least-squares comparisons. **Cards:** 002, 003, 005, 009, 038, 054, 081.
- [ ] **Metrics, quadratic geometry, and distance** — Reuse norms, quadratic forms, PSD cone, distance geometry, and covariance; extend constraints and dual norms. **Cards:** 012, 013, 035, 039, 052, 096, 097, 098, 099, 131, 132, 133.
- [ ] **Spectra and complex-plane diagrams** — Add resolvents, matrix functions, characteristic polynomials, localization, and perturbation views. **Cards:** 017, 024, 042, 058, 062, 068, 073, 125, 126, 127, 128, 129, 134, 135.
- [ ] **Iteration and convergence** — Extend stepwise matrix evolution, transient growth, power/inverse iteration, and convergence traces. **Cards:** 022, 023, 025, 061, 066, 088, 090.
- [ ] **Precision and computational work** — Add number-line rounding, growth factors, backward error, and labelled work/error experiments. **Cards:** 020, 021, 030, 064, 067, 070, 087.
- [ ] **Spectral discretization** — Add sinc/Chebyshev nodes, derivative matrices, interpolation error, and boundary-value convergence. **Cards:** 026, 028, 029, 091, 092, 093.
- [ ] **Incidence, parity, and dependency graphs** — Add parity-code circuits, path counts, dependency schedules, and graph/matrix links. **Cards:** 008, 036, 045, 065, 075.
- [ ] **Meshes, coordinates, and physical fields** — Reuse Truss/Rigid/Membrane; add coordinate metrics, nonlinear waves, and actual cube/drum eigenproblems. **Cards:** 040, 080, 083, 084, 094, 095.
- [ ] **Proof and assumption diagrams** — Build explicit assumptions, implications, and licensed derivation steps for these proof cards. **Cards:** 027, 072.
- [ ] **Composition and differentiation diagrams** — Link operation stages to arrays, products, and derivative perturbations. **Cards:** 034, 085.
- [ ] **Convexity, information, and optimization** — Reuse Simplex/PSD/Quadratic; add feasible sets, dual bounds, constrained fits, and line search. **Cards:** 100, 101, 102, 103, 104, 105, 106, 107, 108, 109.
- [ ] **Brownian motion, SDEs, and arrivals** — Add shared path refinements, Itô sums, moments, generators, GBM, pricing-PDE examples, and Poisson thinning. **Cards:** 110, 111, 112, 113, 114, 115, 116, 117, 118, 124.
- [ ] **Tail asymptotics and order statistics** — Add ratio-limit plots, slow/regular variation, ordered samples, and Rényi/exponential constructions. **Cards:** 119, 120, 121, 122, 123.

## Finish a chapter package

- [ ] Select or extend the shared model, and add the chapter’s labelled diagram/presets.
- [ ] Keep diagrams, geometry, plots, and readouts tied to the same mathematical inputs.
- [ ] Check the mathematics and model behavior; keep visual review with the user.
- [ ] Hand the finished asset to the textbook owner for reading/exercise integration and answer disclosure.

The imported notes are marked unreviewed. Advanced diagrams are illustrations, not substitutes for proofs; finite models must state when they illustrate a broader or infinite-dimensional construction. Source exercises and learner attempts stay unchanged.

## Source scope

- [Live subject TOC](</Users/kogaryu/Documents/ChatGPT/math terms and definitions/README.md>) and its six subject files. All six match the imported source snapshot at this planning checkpoint.
- [Current chapter catalogue](/Users/kogaryu/iggy3d/paths/content/corpus/toc.json): 178 chapter/topic IDs and 930 source entries.
- [Math exercise cards](/Users/kogaryu/devil/99-red-booleans/problems/math) and [book order](/Users/kogaryu/devil/99-red-booleans/problems/ORDER.md): 135 current cards.
- [Existing graphics map](/Users/kogaryu/iggy3d/paths/content/authoring/exercise_graphics_map.json): the earlier 95-card grouping.
- [Current model registry](/Users/kogaryu/iggy3d/paths/src/runtime/math_objects/MathObjects.cpp): 35 registered model families.

Planning only: no application code, source notes, exercise state, images, or other worker’s files are changed.
