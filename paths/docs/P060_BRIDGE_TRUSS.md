# P060 — Bridge & Truss Lab

`truss` is the 31st reusable Math Lab object. Four six-joint structures share a
bounded planar equilibrium solver and the existing solid-primitive renderer.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object truss --level 0 --object-preset 1
```

Press **Play** to sweep the gold load along the structure. Select a joint by
clicking its marker or choosing A-F, then edit the selected joint's XY offsets.
Span, height and crown shift adjust the template underneath those offsets.

| Example | Construction | Try |
| --- | --- | --- |
| 0: Triangular support | Subdivided triangle with three internal members | Layer 1, M3: inspect compression |
| 1: Bridge | Triangulated span with a lower load path | Layer 3: raise height from .7 to 1.4 m |
| 2: Crane boom | Cantilever triangles attached to a vertical wall | Follow the horizontal support-reaction pair |
| 3: Roof truss | Sloping chords, central post and two struts | Move a roof joint and inspect its force polygon |

All four structures start with nine active bars and three support reactions.
The two reaction components at A are independent ideal constraints. B supplies
a vertical reaction for the floor-supported structures, a horizontal reaction
for the crane. These supports constrain motion in both senses; lift-off/contact
and friction are not simulated. Extra support adds B's horizontal restraint or
a vertical restraint at crane joint F. Release removes the original B restraint.

**Test member** removes M8 for Triangle/Roof, M7 for Bridge, or M5 for Crane.
It is independent of **Inspect member**, which only chooses a readout and gold
endpoint markers. Removed members appear as thin dashed guides; Shape only
hides the guides. Solid joints, support symbols and the load marker remain.

Twenty-six parameters and four atomic examples use the existing compact controls.
Only the selected joint's two offsets are exposed. Profile reset restores every
joint's offsets, including hidden ones. Each layer retains geometry, supports,
loads and load position. Every edit pauses the sweep. Playback advances .12 of
the normalized path per second and stops at 1; Restart begins again at 0.
This is a sequence of equilibrium states, not vehicle or structural dynamics.

## Four connected layers

| Layer | Views and measurements | Check |
| --- | --- | --- |
| 0: Geometry, loads and supports | Joint coordinates, transferred loads, reaction traces, global force/moment residuals | Balance nonzero horizontal and vertical loading |
| 1: Tension and compression | Member lengths, signed forces, utilization, force-vs-position trace and signed stems | Inspect a member carrying over .25 kN compression |
| 2: Joint equilibrium | Actual member/load/reaction vectors, head-to-tail force polygon, member endpoint matrix | Inspect a loaded joint with at least three incident members and small residual |
| 3: Rank and force limits | Rank, motion/force freedoms, compatibility, condition and full-path utilization | Carry at least 1 kN along the whole path within specified caps no greater than 2 kN |

Tension is positive and teal; compression is negative and coral. Grey members
are zero-force or lack a unique force solution. Applied arrows are gold and
reactions blue. At layer 2 the selected joint also shows member-force arrows.
Arrows share a bounded scale within the view, while all numerical forces retain
kN. This scale does not change the mathematics. The gold cube is a load-location
symbol, without its own mass. Crossings connect only at declared joints.

## Mathematical contract

The model uses weightless straight bars, frictionless pin joints and axial
member forces. Geometry lies in XY; rods and joints have display thickness in Z.
Loads act at the joints. For a moving load between path joints A and B, the
fractions (1-t) and t go to A and B respectively. This preserves the resultant
force and its moment at the interpolated position. Position is normalized
arclength along the piecewise-linear path, recomputed after geometry edits.

For a member with unit direction n from its start to end, positive tension N
acts as +N*n at the start and -N*n at the end. Support columns are unit vectors
in the constrained coordinate directions. The complete equilibrium system is

```text
E * [active member forces; support reactions] = -applied joint loads
```

There are two equations at each of six joints. Complete row/column pivoting
finds numerical rank, retaining a row transformation and column permutation.
Load samples reuse that factorization; they do not solve a new matrix each frame.
The coefficients are dimensionless and the rank pivot threshold is 1e-10.

- Rank 12 with 12 unknowns: determinate, with unique member/support forces.
- Rank 12 with more unknowns: force indeterminacy; stiffness data would be needed.
- Rank below 12: infinitesimal motion remains unconstrained. Load compatibility
  is reported separately; a compatible load or zero load does not certify stability.
- Coincident joints: degenerate geometry; rank, forces and capacity are unavailable.

The mechanism and force-freedom counts are 12-rank(E) and unknowns-rank(E).
They can both be nonzero. No arbitrary particular solution is displayed as a
physical force result. Only a determinate, load-compatible system publishes
forces. In particular, releasing a support and adding one elsewhere can change
which structure is determinate; the four-support crane with M5 removed is tested.

Each member has the same specified tension/compression force caps. Utilization
is N/tensionLimit for N>=0, and -N/compressionLimit otherwise. These are axial
force constraints, without material stress, elastic deflection, bending, buckling
or fracture. Changing a force cap does not change equilibrium or rod thickness.

Between path joints, every force is affine in load position, and utilization is
a convex piecewise-linear function. The worst utilization over the entire path
therefore occurs at a path vertex. `inspectTrussSweep` checks those vertices and
returns a worst-position/member witness, independently of the 129-sample plot.
The design check uses this full-path maximum, with at least 1 kN of resultant
load, both caps at most 2 kN, and reciprocal condition above 1e-5. The bridge at
span 4.2 m, height .7 m and vertical load -2 kN exceeds the caps; at height 1.4 m
its worst full-path utilization is 1 with both caps set to 2 kN.

Global residuals use only applied loads and reactions, separately from member
balance at each joint. The reported reciprocal infinity-norm condition number
is 1/(norm(E)*norm(inverse(E))); it concerns numerical sensitivity, not strength.
Compatibility uses an infinity-norm residual threshold of
1e-8*max(1, magnitude of the applied resultant in kN).

The force conventions and ideal-truss assumptions follow
[Engineering Statics: Method of Joints](https://engineeringstatics.org/method-of-joints.html)
and its [equilibrium summary](https://engineeringstatics.org/Chapter_06-summary.html).
The library is relevant to source card 040, but its four authored geometries are
separate examples and do not reproduce that card's four-node figure. Source
cards and learner Attempt/setup/solve fields are unchanged. Code and tests are
original; no external implementation or image assets were copied.

## Interfaces and verification

`Truss.hpp/.cpp` owns the templates, edited joint coordinates, support/member
assembly, matrix factorization, load sampling and whole-path limit calculation.
Preparation is bounded O(12^3); a load sample is O(12^2), and the whole-path check
uses at most five samples. Storage is fixed: six joints, nine bars, at most four
support reactions, twelve equations and thirteen unknowns.

Inputs are finite: span 1–5 m, height 0–3 m, crown shift -.15–.15 of span,
joint offsets -.5–.5 m, load components -5–5 kN, force caps .25–10 kN, and
normalized position 0–1. Zero height and coincident-joint edits are retained as
inspectable invalid configurations. Old object and parameter IDs are preserved.

`MathObjects` projects the prepared analysis through existing parts, joint
selection, plots, tables and matrix views. The shared layout exposes the current
joint's offset tuple. Both applications retain their renderer, shader and
content-owner contracts. Textbook/game integration remains with the textbook
worker. No scored attempts, source cards or personal saves are touched.

The installed CPU suite passes 3,578,528 assertions across 210 scene states. The
largest scene has 1,990 vertices and 8,112 indices. Checks include independent
Cholesky stiffness solutions, analytic reactions and triangular-member forces,
force superposition, uniform scaling/translation, cap scaling, full-path maxima,
mechanisms and indeterminacy, coincident/edited joints, extreme aspect ratios,
invalid inputs, compact selectors, atomic presets, retained layers, playback
endpoints, force polygons and geometry ownership/capacity. Maximum joint residual
is 8.527e-14 kN; maximum difference from the independent stiffness formulation is
1.819e-12 kN. The layout suite passes 154,581 assertions over 6,480 layouts and
all 112 object/layer states.

Release `math_lab` and `sorter` both built successfully. All 101 selected checks
passed: 21 CPU suites and 80 early-return text-validation commands, including
all four truss checks and the existing math-object, matrix and textbook
regressions. Each selected command was inspected before execution; no image,
native-host, font or fixture tests were selected. All 13 installed file hashes
match the checked files, and the final diff passes whitespace review. Concurrent
textbook changes in CMake and shared documents are preserved.

All checks use CPU/text paths with no images, windows or font rasterization.
Visual review remains with the user. Changes remain uncommitted.
