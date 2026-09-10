# P068 — Quotients and Homomorphisms Lab

Status: implemented; targeted CPU/text checks pass; user visual confirmation pending.

The 39th model uses CLI key `quotient`, four layers, 22 parameters and 12 presets.
Existing actions, selected-input controls, node picking, tables and playback own
interaction. Source cards, learner state and textbook integration are unchanged.

## Layers

| Level | Construction | Mathematical condition |
| --- | --- | --- |
| 0 | Editable group maps and competing operation routes | Test f(a*b)=f(a)*f(b) on every pair; report a concrete failing pair. |
| 1 | Fibers, kernels and animated collapse | Fibers exist for every map; only a group homomorphism gets a kernel and quotient-to-image claim. |
| 2 | Subsets, left cosets and quotient multiplication | A subgroup defines cosets; normality makes all representative products agree. |
| 3 | Additive subgroups, ideals and quotient rings | Ideals additionally absorb multiplication from both sides; inspect addition and multiplication separately. |

The scene labels source elements A0..A5, target elements B0..B5, and quotient
classes C0..C5. Numeric table cells use the same indexed-mesh cell builder as
Finite Algebra. A dash marks conflicting results. It never silently chooses a
product for an ambiguous class pair. Class ordering uses the smallest unassigned
source representative; fiber ordering follows reached target element indices.

Gold and violet compare the two routes or representative choices. Automatic
witness mode changes displayed operands only, preserving the editable map and
selected input. Disable it to inspect your own pair. Alternate-member controls
index the sorted members of the classes selected by a and b.

Collapse moves source beads toward their fiber/class bead without changing the
mathematics. At full collapse one reached target/class bead represents the merged
elements; unused target elements remain small muted markers. Individual source
picking is then disabled, with the input dropdown still available. Playback takes
four seconds; it animates diagram layout, not intermediate algebraic values.

## Presets and manual launch

| Index | Preset |
| --- | --- |
| 0 | C6 to C3 by reduction; subgroup {0,3} |
| 1 | Broken reduction map |
| 2 | S3 parity map; normal subgroup {0,3,4} |
| 3 | Nonnormal S3 subgroup {0,1} |
| 4 | C4 to C2; subgroup/ideal {0,2} |
| 5 | Trivial map and whole-group/ring subset; one class |
| 6 | Klein-four projection |
| 7 | Subset fails closure |
| 8 | Coordinate ideal {0,1} in F2 x F2 |
| 9 | Diagonal {0,3} in F2 x F2; additive subgroup but not ideal |
| 10 | Ideal {0,2} in F2[e]/(e^2) |
| 11 | Identity group map and zero ideal |

User-run launch for six elements collapsing into three fibers. Press Play or
move Collapse classes:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object quotient --level 1 --object-preset 0
```

For conflicting products from a nonnormal subgroup:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object quotient --level 2 --object-preset 3
```

For an additive subgroup that fails the ideal condition, switch Quotient
operation between Addition and Multiplication:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object quotient --level 3 --object-preset 9
```

The agent did not run these windowed commands. Every executed application check
used `--validate`, which returns before native host and font setup. No images,
screenshots, captures, native windows or font probes were generated or inspected.

## Mathematical interfaces and conventions

`FiniteQuotient.hpp/.cpp` reuse `FiniteOperation` and verified group analysis:

- `analyzeFiniteHomomorphism` accepts two verified groups and a total map, checks
  every product, and returns map classification, fibers, image correspondence
  and kernel. Invalid input throws; a well-formed but nonpreserving map returns
  a failure witness and no kernel. Inactive map entries are ignored.
- `FinitePartitionOperation` records each class and representative, every possible
  result class for each product, and the first conflicting representative pair.
  Its product entries use -1 for ambiguity. One-class outputs are supported even
  though the input-group examples have two to six elements.
- `analyzeFiniteGroupQuotient` checks identity, closure and inverses for an editable
  subset, constructs left cosets only for subgroups, and tests representative
  independence. The subset witness identifies an excluded identity, inverse or
  product. Empty/invalid subsets return no classes. Nonnormal subgroups retain
  their valid set partition, with no quotient-group multiplication claim.
- `analyzeFiniteRingQuotient` checks the input unital-ring laws, additive subgroup
  conditions and two-sided absorption. Addition and multiplication have separate
  partition-operation results. The whole-ring ideal gives the zero ring.
- `finiteRingExample` supplies Z/nZ for n=2..6, F2 x F2, and F2[e]/(e^2).

Group families are C2..C6, Klein four and S3. Cn uses addition, Klein four XOR.
S3 shares P067's ordering: 0=id, 1=(12), 2=(01), 3=(012), 4=(021), 5=(02),
with a*b applying b first. F2 x F2 uses two-bit elements, XOR addition and AND
multiplication; its multiplicative identity is 3. Dual-number index a+2b means
a+b*e for a,b in F2; its identity is 1 and e*e=0.

Controls normalize operand and destination bounds when family sizes change.
Only active map inputs/subset entries contribute to analysis. Inactive subset
flags are retained. Alternate-member offsets clamp to the selected class size.
Edits pause playback, and undefined constructions reject dependent controls.
The registry now has 39 assets and 544 parameters. `SnapshotBuilder::operationCell`
shares bounded quads and numeric glyphs between P067 and P068, without a new
renderer, font path or inspector registry.

## Verification and remaining scope

Release `math_lab` and `sorter` builds pass; the existing duplicate
`libpaths_imgui.a` linker warning remains. All 19 selected checks pass: seven CPU
suites and twelve text-only CLI cases, including the four new layer challenges,
nonnormal/nonideal examples and the prior finite-algebra cases.

The new suite covers 82,238 maps (all Cn -> Cm maps for n,m=2..6 plus all
S3 -> C2 maps), all 204 subsets of the seven group examples, all 156 subsets of
the seven ring examples, and 1,793 fresh CPU scenes. Independent checks use cyclic
generators, gcd counts, conjugation, subset closure, absorption and exhaustive
representative products. Scene checks cover collapse invariance, partial images,
empty/full subsets, single-class quotients, conflict witnesses, dynamic bounds,
atomic rejected edits, picker projection, index ownership and finite geometry.

Maximum new-asset geometry: 45 parts, 3,242 vertices, 11,970 indices. The existing
Finite Algebra suite still passes its 20,432 tables and 1,971 scenes; layout
checks cover 6,480 layouts and 144 object/layer states.

These are bounded foundations for algebra chapters 001, 003, 006, 008–012.
General ideal lattices, modules, arbitrary ring-map editors, localization,
extension fields and chapter integration remain open. Changes are uncommitted.
