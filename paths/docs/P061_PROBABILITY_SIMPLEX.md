# P061 — Probability Simplex Lab

The `simplex` provider is the 32nd object in Math Lab. It adds four retained
layers and ten compact controls through the existing model, scene and inspector.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object simplex --level 2 --object-preset 1 --set simplex_mix=0
```

Press Play to sweep the mixture from P to Q. The sweep takes about 8.3 seconds;
it interpolates probabilities, not physical time or random samples. Drag the A/B
fields for each distribution, or Ctrl-click to type. C is the remaining
probability, shown in the table. Each input's upper bound follows its companion.
Layer changes retain the distributions, outcome values, function and mixture.
Every parameter edit pauses playback. Presets and paired resets are atomic.

| Layer | Representation |
| --- | --- |
| 0 — Distributions as positions | Complete probability triangle, P/Q/mixture markers, outcome probabilities and expected value |
| 1 — Entropy and variance | Height surface selected from expectation, entropy, negative entropy or variance |
| 2 — Convexity and mixtures | Endpoint chord, function trace, signed Jensen gap and linked mixture graph |
| 3 — Information divergence | KL surface against Q, forward/reverse divergence, finite support face and negative-entropy supporting-plane graph |

Five complete examples are Balanced uncertainty, Mix two certainties, Same mean /
different spread, Asymmetric information, and Missing outcome. The same-mean
example uses outcomes (-1,0,1), P=(0,1,0), Q=(1/2,0,1/2). Both means are zero;
the variances are zero and one. The asymmetric example uses P=(.9,.05,.05) and
Q=(.2,.3,.5). Missing outcome sets Q=(.5,.5,0) while P gives C positive mass.

## Numerical contract

`Simplex.hpp/.cpp` owns bounded three-outcome probability calculations. Inputs
are finite and nonnegative with sum one to 1e-12; mixture t is in [0,1]. Named
outcome values range from -3 to 3 and may coincide or be unordered. Equal-valued
categories remain distinct for entropy. The A/B constructor makes C=1-(A+B)
and clips only endpoint roundoff within its validated tolerance. Parameter
updates validate the pair before mutation; a step rounded just past an admissible
slider endpoint is clamped to that endpoint. Invalid partial resets are rejected
without changing parameters, feedback or revision.

Expectation is sum(p_i*a_i). Variance uses the centered sum
sum(p_i*(a_i-mean)^2). Entropy is -sum(p_i*log(p_i)), using natural logarithms and
0*log(0)=0. On the simplex, expectation is affine, entropy and variance are
concave, and negative entropy is convex. The reported Jensen gap is
(1-t)*f(P)+t*f(Q)-f((1-t)*P+t*Q); its sign follows that convention.

KL(P||Q)=sum(P_i*log(P_i/Q_i)). A zero P_i contributes zero; positive P_i with
zero Q_i gives positive infinity. The implementation sums generalized
relative-entropy terms whose linear components cancel for normalized inputs.
A local sixth-order series handles almost-equal components without cancelling
small positive divergences to zero. Every evaluation is O(3), with fixed storage.
For strictly positive Q, the negative-entropy supporting plane restricted to
the simplex is sum(p_i*log(Q_i)); its gap from negative entropy equals KL(p||Q).

## Geometry and finite boundaries

The equilateral triangle is an affine embedding in world XZ, with side length 4.
World Y is function value times the explicitly reported height scale. Layer 0
keeps a flat triangle. Other full surfaces use 32 subdivisions: 561 vertices and
1,024 triangles. Heights evaluate the mathematical function; vertex normals
average incident triangle normals. This is an open sampled graph, not a solid
or an exact smooth surface. Colours blend the three category colours.

Expectation uses height scale .65; entropy and negative entropy use 1.5.
Variance adapts its scale to the outcome range, and KL adapts it to Q's smallest
positive probability. Values in tables and plots are unscaled. The source
functions have finite endpoint heights even when entropy derivatives diverge.

If Q has one zero probability, finite KL exists only on its supported edge;
if Q is a vertex, only that vertex has finite KL. These cases draw the actual
finite edge curve or point, label the infinite region, and omit the 2D mesh and
full-simplex tangent. Infinite metric values are labelled rather than published
as numeric zero. Infinite plot samples are omitted. Along this affine mixture,
finite samples form the whole segment or an endpoint, so omission never joins
across a hidden interior singularity. No epsilon floor replaces zero probabilities.

## Source direction and ownership

Source cards, read as text only:

- [101 — functions on the probability simplex](/Users/kogaryu/devil/99-red-booleans/problems/math/101_functions_on_the_probability_simplex.md)
- [103 — information inequality](/Users/kogaryu/devil/99-red-booleans/problems/math/103_kullback_leibler_and_the_information_inequality.md)

This authored three-category model covers expectation, negative entropy and
variance from 101, and adds positive entropy and convex-mixture exploration.
It specializes 103's positive-vector statement to normalized probabilities and
explicitly extends it to supported boundary distributions. The other functions
on card 101 are not represented by this checkpoint. These examples and sampled
curves are not a proof or a completed learner attempt. No external helper code
was copied, and no source card, lesson catalogue, attempt or save was changed.

The existing MathObjects action owner and MathObjectLayout controls publish
all state. New object and parameter IDs are appended. MathObjectScene, renderer,
GPU capacities, native input, textbook integration and question owners are
unchanged. Changes remain uncommitted.

## Verification

The installed CPU suite passed 4,029,839 assertions over 184 scene states, reaching
3,606 vertices and 11,232 indices. Checks include an independent direct
log-ratio KL calculation, second-moment variance, the mixture variance identity,
Jensen signs, supporting-plane gaps, exact support boundaries, near-equal KL,
permutations, geometry heights/area/winding, finite buffers, draw ownership,
control bounds, atomic rejection/reset/presets, retained layers and playback.
Maximum KL oracle disagreement was 3.331e-16; maximum supporting-gap disagreement
was 4.441e-16.

Both Release applications, `math_lab` and `sorter`, built successfully. All 27
explicitly selected checks passed: 22 CPU suites covering the object library,
compact controls, matrix/textbook bindings, and five early-return simplex
`--validate` cases. Those CLI cases include all four challenges and a reference
with a missing outcome. The shared control suite covers all 116 object/layer
states. Every selected CTest command was inspected; no fixture, native/font
suite or image-producing route was included. The CLI returns before native host
creation, typesetter initialization and bookmark I/O. Installed source hashes
matched the reviewed stage, and targeted whitespace checks passed.

No images, screenshots, windows, browser previews, font probes or personal saves
were used. Visual appearance and pointer feel remain for the user's test.
