# P065 — Sets and Maps Lab

`maps` is the 36th Math Lab model: total maps between nonempty finite sets
of one to four elements, with 25 controls and nine complete presets.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object maps --level 2 --object-preset 4
```

Press Play to follow the selected input through A, B and C over four seconds.
Gold takes the composed route; the raised violet/coral arc shows the direct
route for that same input. Edit a bead's destination through the inspector.
Click an A bead or use its dropdown to select it; camera dragging still orbits.
The B selector chooses which input of g to edit. Destination numbers match the
zero-based labels A0, B0, C0, etc. Edits pause playback.

| Layer | Construction |
| --- | --- |
| 0 — Maps and destinations | Editable f: A -> B; injection, surjection, bijection and unused outputs |
| 1 — Fibers and partitions | Highlight a chosen B preimage; group A by common output without changing f |
| 2 — Composition and competing routes | Editable f, g and h; animated g after f versus h; compare every input |
| 3 — Weighted outcomes | Normalize nonnegative weights, push mass onto B, and condition on one fiber |

In layer 1, expand Display to use Group by common output. In layer 3, edit
weights in Sampling and select an event with Inspect output B. Probability view
switches between the whole population and conditioning on f(A)=selected B.
Positive bead volumes encode the displayed probability; zero-mass outputs use
small muted markers. The two plots and table supply the numerical values.

Presets: 0 Permutation; 1 Injection with unused outputs; 2 Surjection with a
collision; 3 Grouped fibers; 4 Commuting routes; 5 One disagreeing route;
6 Weighted outcomes; 7 Zero-mass event; 8 Zero total weight.

## Reusable contract

`FiniteMapsInput` and `analyzeFiniteMaps` in `FiniteMaps.hpp/.cpp` own total-map
validation, fiber masks/sizes, compact quotient-class indices, composition,
injection/surjection, route agreement, normalized source/output/composed mass,
and conditional source/output mass. Fixed arrays hold four slots; inactive
entries are ignored. Fiber class indices follow reached B indices in order.
The geometric spacing of trays and intermediate animation positions carry no
extra mathematical meaning. The quotient has one class per nonempty fiber.

Source and destination cardinalities are explicit. Shrinking B/C clamps every
stored destination into its remaining codomain; shrinking A hides its extra
inputs. Selectors are clamped as needed. Restoring a larger codomain does not
restore removed destinations. Reset restores the full default permutation.
`MathObjects::parameterMaximum` supplies active bounds to validation and the
inspector through one binding table; dropdowns honor those bounds.

All-zero weights produce no probability law. A zero-weight fiber produces no
conditional law, even if it contains elements. Undefined probability columns
and requested probability plots are omitted; the model never substitutes a
uniform distribution. Weight normalization uses maximum scaling to avoid sum
overflow; positive-event weights are normalized separately. UI weights are
bounded by 12 with step 0.1. The kernel also accepts finite nonnegative doubles;
a probability below representable double range can underflow while its separate
conditional law remains defined.

This supplies reusable foundations for Algebra 003/016/018, Discrete 120/127,
and Probability 149/150/156. Chapter integration, empty sets, arbitrary-size
relations, operation-preserving algebraic maps, and general category machinery
remain separate work. No learner state or source exercise card is modified.

## Verification

Both Release applications (`math_lab`, `sorter`) build. All 14 selected CTest
checks pass: five CPU suites and nine text-only CLI cases. The new suite checks
133,300 exhaustive pairs of finite maps, independent classification and fiber
partition oracles, weighted pushforward, conditioning, zero/invalid/extreme
weights, resizing and atomic rejection, selected-field wiring, grouping,
playback and all four challenges. Its 860 CPU scenes stay within the buffers
(maximum 2,573 vertices / 11,868 indices), with finite geometry and usable input
picker positions. Existing layout checks cover 6,480 layouts and 132 object/layer
states. The shared plot contracts now cover 410 states and 515 plots.

The linker retains its duplicate `libpaths_imgui.a` warning. Verification is
CPU/text only: no native windows, images, captures, screenshots or font probes.
Visual review remains with the user. Changes are uncommitted.
