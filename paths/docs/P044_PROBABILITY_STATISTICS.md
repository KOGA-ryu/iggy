# P044: probability and statistics objects

The user requested a few more batches and authorized choosing from the expanded
TOC. Its new probability/statistics section supplies this batch: three objects,
four working layers each, bringing math_lab to nineteen objects.

| Object | Level 0 | Level 1 | Level 2 | Level 3 |
| --- | --- | --- | --- | --- |
| Binomial board | Independent Bernoulli trials | Binomial mass | Mean, variance and tails | Continuity-corrected normal approximation |
| Bayesian cube | Joint probability | Conditioning | Bayesian odds | Repeated conditionally independent evidence |
| Covariance cloud | Mean and translation | Covariance and correlation | Principal components | Whitening |

## Binomial board

The pegboard has n=1..12 independent trials with fixed p=0..1 in steps of 0.05.
Next trial consumes one xorshift32 draw and records a success/failure; at most n
steps are accepted. Reset repeats the selected seed. Changes to n, p, seed or
object/level reset the path. The path is one realization; bars and tables show
the model's distribution, not empirical frequencies from that one path.

The PMF is computed by repeated Bernoulli convolution. Independent tests use
the binomial coefficient formula. Count threshold k=0..12 remains meaningful
above n, where point mass is zero and CDF is one. The upper tail P(X>k) is summed
directly to retain small probabilities. Mean and variance are n*p and n*p*(1-p).
Gold/blue markers show the mean and one standard deviation on each side.

The normal comparison uses the same mean/variance and half-count continuity
correction. Maximum CDF error includes both support boundaries. Normal mass
outside the binomial support is reported without renormalization. Zero variance
at p=0 or 1 explicitly disables the approximation. This finite-n comparison is
an illustration of approximation, not a proof of a limit theorem. Coloured bar
heights are proportional to probability; grey baselines represent mass below
1e-9, while the table retains its value.

## Bayesian cube

The left unit cube partitions two hypotheses and an event into four joint
masses. Prior and both event likelihoods range from 0..1. Conditioning on E or
its complement renormalizes the selected evidence into a second unit cube.
When the evidence probability is zero, the posterior is undefined: the second
cube and posterior column are omitted. Infinite and undefined odds/likelihood
ratios are labelled explicitly. Both posterior components are divided directly
by evidence probability, preserving small values without subtracting from one.
Pieces below 1e-9 are omitted from geometry; numerical masses remain in tables.

The final layer permits 0..8 E outcomes and 0..8 not-E outcomes. Observations
are independent conditional on each hypothesis. The likelihood is
p^successes*(1-p)^failures, with an empty sequence having likelihood one.
The plot places E observations first and stops at the first impossible prefix.
Tests accumulate evidence in the reverse order and verify the same final result.
The conditional independence assumption is part of this repeated-event model.

## Covariance cloud

Eight equally weighted cube corners are stretched, sheared, rotated and
translated. The displayed coordinates themselves determine the population mean
and covariance, using divisor 8. This is not the unbiased sample estimator.
A covariance ellipse has principal semiaxes equal to standard deviations; it
is a second-moment guide, not a Gaussian confidence boundary for this finite
non-Gaussian distribution.

Stretches range 0..2, shear -1..1, and mean coordinates -1..1. The existing SVD
of the generating transformation supplies ordered principal directions and
variances. Projected points and residual segments follow a selected principal
axis. Equal principal variances allow multiple valid orthonormal bases.
Correlation is unavailable for zero coordinate variance.

Whitening requires three positive principal variances. The left cloud is
centered input; the right is in principal coordinates. The amount control
rescales each coordinate from its original spread to unit variance, reaching
identity covariance at amount 1. Singular clouds are still valid to explore,
but their inverse, transformed cloud and whitening control are unavailable.
A uniform scene scale keeps the geometry in view; tables retain mathematical
coordinates. The mean row is an aggregate, with weight zero in the point table.

## Verification and ownership

Seven targeted CPU suites and fifteen native --validate scenarios passed:
**22 checks**. The new suite covers 252 binomial models, 4,188 Bayesian cases,
2,304 PCA cases, whitening, deterministic trial replay, atomic rejections,
all twelve challenges and 1,720 geometry states. Observed maxima were 6,834
vertices and 25,248 indices, within the unchanged renderer capacities. Earlier
P039-P043 numerical/scene regressions passed, including probability-walk replay
after extracting its shared random-number helper.

The existing MathObjects owner, snapshot structures and UI action route are
extended. No renderer or GPU changes, third-party imports, source-note edits,
scored attempts or study saves are involved. Concurrent Library, reviewed matrix
notes and CMake changes were preserved. Changes remain uncommitted.

All verification was text-only: no native host, window, rendering, screenshots
or image viewing. Visual review and pointer feel remain for the user.

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b -t math_lab paths_math_statistics_tests paths_math_shell_tensor_probability_tests paths_math_number_field_tests paths_math_exploration_tests paths_math_layers_tests paths_math_object_tests paths_scene_tests -j4
ctest --test-dir b -R '^paths_(math_statistics|math_shell_tensor_probability|math_number_field|math_exploration|math_layers|math_object|scene)_tests$|^paths_math_.*_cli$' --output-on-failure
./b/math_lab --validate --object binomial --level 3 --set trials=12 --check
./b/math_lab --validate --object bayes --level 3 --check
./b/math_lab --validate --object covariance --level 3 --check
```

For the user to open and try the batch:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object binomial
```

Use the subject/layer controls for the other objects. Trial CLI actions are
`--trial-step` and `--reset-trials`. Arguments apply in order; `--help` lists
parameters. Topic provenance is the user's source at
`/Users/kogaryu/Documents/ChatGPT/math terms and definitions/sections/probability_statistics.md`.
The source was read as text only; this packet does not mark the entire corpus
as mathematically reviewed or adapted.
