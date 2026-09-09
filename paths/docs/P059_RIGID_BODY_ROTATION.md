# P059 — Rigid-Body Rotation Lab

`rigid` is the 30th reusable Math Lab object. Four component assemblies share
one free-rotation model, the existing compact inspector and primitive renderer.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object rigid --level 3 --object-preset 2
```

Press **Play**. The book flips around model time 3 and points the tracked axis
backwards by time 4. Playback runs at half speed, so this takes about eight real
seconds. Time is bounded to [0,12]; Pause, scrub and Restart use the shared controls.

| Example | Body | Useful comparison |
| --- | --- | --- |
| 0: Flywheel | Solid elliptical cylinder, circular in the preset | Track body X through steady spin about Y |
| 1: Adjustable dumbbell | Two weights and a connecting bar | Change left mass share or weight separation |
| 2: Tumbling book | Two covers and a page block | Small perturbation near the intermediate Z axis produces a flip |
| 3: Satellite | Bus, two panels and two connectors | Panel extent and mass balance change inertia and free rotation |

Four layers expose orientation/frames, mass/inertia, angular motion, and
stability/conservation. Dimensions, mass, release angles, initial spin, time and
the tracked axis are retained across layers. Every edit pauses and reevaluates
the same release conditions at the selected time. Choosing an example restores
all 15 controls atomically. The body dropdown changes the construction while
retaining the other controls; example selection restores the named dimensions
and motion as well. Left mass share is available for dumbbell and satellite.

Body axes are coral X, teal Y and blue Z. Muted axes stay fixed in world space.
The gold tip and its recent trail follow the selected body axis. At the inertia
layer, gold points mark component centres of mass; the grey point marks the
original assembly origin. The whole assembly rotates about its recentered COM.
At the angular-motion and stability layers, gold L and teal omega arrows use
equal display lengths to compare directions; tables retain their true magnitudes.
Shape only hides construction guides. Thin surface stripes are massless markings.

## Mathematical contract

Lengths are metres, mass kilograms, time seconds, initial angular velocities
body-frame rad/s. There is no external torque, translation, gravity or collision.
Release angles apply fixed X, then Y, then Z rotations. The scalar-first unit
quaternion maps body coordinates to world coordinates, with right-handed active
rotation. Internally:

```text
L_body = I_body * omega_body
Ldot_body = L_body cross omega_body
qdot = q * (0, omega_body) / 2
R = R(q)
L_world = R * L_body
I_world = R * I_body * R^T
E = omega_body dot L_body / 2
```

The same component dimensions and positions supply geometry and mass properties.
Each part is homogeneous; different parts can have different densities. Component
interiors are disjoint. Box moments use m*(b²+c²)/12; the cylinder is solid,
with symmetry axis Y and possibly unequal X/Z radii. Its second moments per unit
mass are W²/16, H²/12, D²/16. Component moments are translated to the total COM
by the parallel-axis theorem. All these assemblies have diagonal body tensors.

| Body | Fractions of total mass and full dimensions |
| --- | --- |
| Flywheel | One cylinder: mass M, dimensions (W,H,D) |
| Dumbbell | Weights at x=±.35W: total .9M split by left share f, each (.3W,H,D); bar .1M, (.4W,.15H,.15D) |
| Book | Covers at y=±.47H: .1M each, (W,.06H,D); pages .8M, (W,.88H,D) |
| Satellite | Bus .6M, (.28W,H,.6D); panels at x=±.34W: total .35M split by f, each (.32W,.06H,D); connectors at x=±.16W: .025M each, (.04W,.12H,.12D) |

For distinct principal moments, free spins near the minimum and maximum moment
are stable; the intermediate-axis spin is unstable. The book starts with
omega=(.02,.02,3), near its intermediate Z axis. Alignment is the tracked body
axis dotted with the **initial** world-momentum unit vector. The flip check also
requires initial alignment above .99, a tracked intermediate axis, alignment
below -.8 and relative invariant errors below 1e-5. A negative alignment alone
is not treated as instability. Moment gaps below 1e-10 times the maximum moment
are classified as repeated; no unique intermediate axis is claimed there.

The mass properties, Euler equations and stability interpretation follow
[MIT Classical Mechanics III, Chapter 2](https://ocw.mit.edu/courses/8-09-classical-mechanics-iii-fall-2014/6fe39e8d5ce4ce746ca256dfea665eda_MIT8_09F14_Chapter_2.pdf).
Implementation and tests are original; no third-party
code or image assets were copied.

## Numerical and interface contract

`RigidBody.hpp/.cpp` owns component assembly, mass properties, quaternion
conventions and the cached free-rotation solution. `MathObjects` projects that
solution into existing primitive placements, matrices, tables, plots and model
checks. It owns the 15 appended parameter IDs and existing semantic actions.
The UI only presents those values and forwards actions. No renderer, shader,
GPU capacity, source-card, question-attempt or content-pipeline changes are made.
Authored textbook integration remains with the textbook worker.

The solver uses double-precision RK4 on the quaternion and body momentum.
Quaternion normalization preserves unit orientation; energy and momentum are
not projected back to their initial values. Signed energy error is (E-E0)/E0;
momentum error is |L_world-L0|/|L0|, including direction error. These are numerical
drift measurements, not physical dissipation or rigorous error bounds. Exact
rest bypasses integration and reports zero drift; alignment is undefined there.

An initial-condition edit builds a complete 129-checkpoint cache before replacing
it. Identical input reuses the cache. Random-access time evaluation replays only
the preceding checkpoint interval, so scrubbing is independent of frame history.
The RK4 step is at most min(1/480, .01/OmegaBound), where
OmegaBound=sqrt(2*E0/Imin). Cache construction is bounded to 262,144 microsteps;
invalid inputs or excess work reject before replacing the prior cache. Storage
is fixed. Dimensions lie in [.2,3.5], mass [.2,5], left share [.2,.8], release
angles [-180,180], and each initial spin component [-4,4].

Graphs have 129 samples over at most 8*pi/OmegaBound seconds; the recent trail
uses 64 segments spanning at most 4*pi/OmegaBound. The time slider still reaches
the full 12-second horizon. These are sampled views; native display frame rate
can limit how smoothly very fast rotations appear. Sampling does not alter the
physical state or its numerical readouts.

## Verification

The CPU suite checks mass moments against independent exact quadrature,
nonoverlapping components, centre-of-mass and scaling laws, quaternion double
cover and rotation order, principal-axis analytic rotation, symmetric-top motion,
and an independent world-space rotation-matrix ODE. It also checks conservation,
exact rest, cache replay/invalidation, public extrema, atomic presets, challenges,
control groups, retained layers, playback endpoints and published CPU geometry.

The installed CPU suite passes 1,982,160 assertions across 113 scene states;
maximum scene 3,725 vertices / 13,092 indices. Tested cache construction peaks
at 97,280 microsteps. Maximum measured relative energy error is 1.701e-12 and
world-momentum vector error 3.856e-11. The layout suite passes 154,203 assertions
across 6,480 layouts and all 108 object/layer states.

Release `math_lab` and `sorter` both built successfully. All 96 selected checks
passed: 20 CPU suites and 76 early-return text-validation commands, including
all four rigid-body challenges and the existing math, matrix and textbook
regressions. Each selected CTest command was inspected; no native font or image
tests were selected. Existing object and parameter numeric IDs are preserved.
Installed source hashes match the checked files, and the final text diff passes
whitespace review. Concurrent textbook edits are preserved.

No images, windows, font probes or personal saves are used. Visual review remains
with the user. Changes remain uncommitted.
