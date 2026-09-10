# P067 — Finite Algebra Lab

Status: implemented; targeted CPU/text checks pass; user visual confirmation pending.

The 38th model uses CLI key `finite`. It supplies four layers through existing
MathObjects actions, inspector metadata, node selection, scene primitives and
indexed meshes. No textbook integration or learner-state changes are included.

## Layers

| Level | Construction | Controls and evidence |
| --- | --- | --- |
| 0 | Editable operation table | Choose a and b; compare a*b and b*a. Edit the selected row. |
| 1 | Group laws | Identity-candidate counterexamples, two-sided inverses, associativity and commutativity. Toggle the first failing pair/triple or inspect selected operands. |
| 2 | Generated subgroups and cosets | Select generator a; animate e,g,g^2,...; arrange left/right cosets; report normality. Enabled only for a verified group. |
| 3 | Rings Z/nZ | Switch modular addition/multiplication; inspect units, nonzero zero divisors and their partners. Prime moduli 2, 3, 5 are fields. |

Each table contains two to six elements. Its indexed mesh includes numeric cell
glyphs built from small quads, so results do not depend on colour recognition or
one label per cell. The teal frame marks the edited row; gold and violet outline
comparison entries. Automatic witnesses can use a different a from the edited
row. Node clicks change the shared a / row / generator selector.

## Presets and manual launch

| Index | Preset |
| --- | --- |
| 0–4 | Clock groups C2, C3, C4, C5, C6 |
| 5 | Klein four group |
| 6 | Triangle permutations S3 |
| 7 | Broken associativity |
| 8 | Ring modulo 4 |
| 9 | Ring modulo 6 |

User-run visual test, showing the zero-divisor example 2*3=0 modulo 6:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object finite --level 3 --object-preset 9
```

For cosets, choose the triangle permutations and press Play. Move Arrange cosets
and switch between Left: aH and Right: Ha:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object finite --level 2 --object-preset 6
```

For a concrete associativity counterexample:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object finite --level 1 --object-preset 7
```

The agent did not run these windowed commands. All executed CLI checks included
`--validate`, which exits before native host/font setup. No images, screenshots,
captures, native windows or font probes were generated or inspected.

## Mathematical interface

`FiniteAlgebra.hpp/.cpp` own bounded, exact finite algebra:

- `FiniteOperation`: size 2..6, 36 entries with stride six. Active outputs must
  lie in the active set; invalid input throws. Inactive entries are ignored.
- `analyzeFiniteAlgebra`: identity (-1 absent), inverse for each element (-1
  absent/undefined), law flags and first failing witnesses. A group requires
  associativity, a two-sided identity and a two-sided inverse for every element.
  Commutativity is independent of group status.
- `generatedFiniteSubgroup`: powers through the first return to identity,
  subgroup bitmask, coset partition and normality. Nongroups return undefined,
  with no invented subgroup or classes. Left cosets are aH, right cosets Ha;
  group numbering follows the smallest still-unassigned representative.
- `modularOperation` and `analyzeModularRing`: exact arithmetic in Z/nZ,
  multiplicative inverses, nonzero annihilating partners and field classification.
- `kleinFourOperation` and `trianglePermutationOperation`: explicit presets.

S3 indices use lexicographic permutation images: 0=id, 1=(12), 2=(01),
3=(012), 4=(021), 5=(02). Product a*b applies b first, then a. For example,
1*2=4 while 2*1=3. The Klein operation is bitwise XOR on 0..3.

The first three layers always analyze the editable table, including edited
presets. Shrinking the set clamps all stored outputs and operand selectors;
growing it does not restore prior entries. The ring layer always computes the
standard Z/nZ operations independently, retaining the editable table for return
to earlier layers. It does not claim that an arbitrary table defines a ring.
Zero is excluded from the zero-divisor count.

Forty-six parameters join the existing metadata registry (522 total): size,
three operands, law/witness selectors, grouping, coset side, six-step playback,
ring operation, and 36 table cells. Only the selected active row is exposed.
Unavailable constructions reject actions; edits pause playback. Between integer
steps the moving bead illustrates a route, not an intermediate group element.
Positions and grouping have no effect on the operation.

## Verification and remaining scope

Release `math_lab` and `sorter` builds pass. The existing duplicate
`libpaths_imgui.a` linker warning remains. All 13 selected checks pass: six CPU
suites and seven text-only CLI cases, including all four finite-algebra challenges.

The new suite checks 20,432 operation tables, including every table on two and
three elements, known groups, single-entry mutations, relabelled identities,
subgroup closure, coset membership, normality via conjugation, modular arithmetic
via gcd, invalid input and 1,971 fresh CPU scenes. Scene checks cover mesh/index
ownership, finite geometry, numeric table glyph counts, picker projection,
parameter bounds, dropdown labels, grouping invariance, undefined cases,
playback and layer challenges. Maximum geometry: 57 parts, 3,236 vertices,
10,404 indices. Existing layout checks cover 6,480 layouts and 140 layer states.

Algebra chapters 001, 006, 008, 009 and 010 gain direct reusable foundations.
Chapters 011 and 012 gain bounded ring examples only. General ideals, quotient
rings, modules, extension fields, automorphisms and chapter integration remain
open. Changes are uncommitted.
