# Historical Swords Set: Geometry Study

## Result and boundary

This is a useful four-sword construction study, not a production sword pack and
not a historical measurement source. It extracts exact geometry facts from the
hash-locked donor models while keeping their materials, textures, and original
appearance out of `sword_steel_v1`.

The comparison board is
[`historical_swords_set_geometry_study_board.png`](output/historical_swords_set_study_v1/historical_swords_set_geometry_study_board.png).
Each row uses identical neutral Workbench display and contains a whole
silhouette, isolated hilt assembly, oblique blade-plane view, topology view,
and five section-plane intersections. Sections are sampled at 12%, 25%, 50%,
75%, and 88% of model blade length from its root.

## Provenance

- Source: [Historical Swords Set](https://opengameart.org/content/historical-swords-set)
- Creator: Clint Bellanger
- Selected license: [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/)
- Attribution: Historical sword models by Clint Bellanger, licensed CC BY 3.0
  via OpenGameArt.org.
- Downloaded archive: 26,102,712 bytes
- SHA-256: `b090eddf53f63cb8fd8a3bfac0c3dbe36749d934ca079595485629c2a2eb55de`
- Creator's scope: four simple "historical-ish" game models

That last phrase is decisive. These models can expose construction choices and
shader-response problems. They cannot establish a real weapon's dimensions,
mass distribution, metallurgy, finish, or historical correctness.

## Model-audited census

All dimensions below describe the downloaded meshes. Width and thickness are
the maximum local blade bounds, not museum measurements.

| Donor | Whole object W/L/T (mm) | Blade L/W/T (mm) | V/E/F | Hilt components |
| --- | ---: | ---: | ---: | --- |
| ArmingSword | 198.26 / 961.99 / 28.67 | 750.00 / 59.43 / 6.16 | 505 / 1012 / 512 | 1 guard, 1 grip, 1 pommel |
| BastardSword | 230.79 / 1085.60 / 54.59 | 832.13 / 55.73 / 5.58 | 521 / 1062 / 546 | 1 guard, 1 grip, 1 pommel |
| LongSword | 250.86 / 1220.13 / 27.40 | 896.69 / 49.60 / 6.29 | 679 / 1356 / 682 | 1 guard, 1 grip, 1 pommel |
| ClaymoreSword | 357.07 / 1385.51 / 24.73 | 1010.44 / 57.75 / 5.10 | 1106 / 2232 / 1112 | 4 guard pieces, 1 grip, 1 pommel |

`V/E/F` means source vertices, edges, and polygon faces. Every model contains a
separate blade, leather-assigned grip, and pommel. The Claymore also reveals an
assembly lesson: its guard is four disconnected pieces rather than one fused
decorative lump.

## Exact section record

Width and thickness are millimetres. `Pts` is the number of unique source-mesh
edge intersections at that station and therefore a signature of model section
complexity.

| Donor | Station | Width | Thickness | Pts |
| --- | ---: | ---: | ---: | ---: |
| ArmingSword | 12% / 25% / 50% / 75% / 88% | 55.45 / 51.92 / 46.94 / 37.60 / 27.78 | 6.01 / 5.92 / 5.37 / 3.55 / 1.94 | 8 / 8 / 8 / 8 / 8 |
| BastardSword | 12% / 25% / 50% / 75% / 88% | 54.78 / 50.80 / 43.15 / 34.40 / 27.08 | 5.14 / 4.57 / 3.46 / 2.48 / 2.02 | 12 / 12 / 12 / 8 / 8 |
| LongSword | 12% / 25% / 50% / 75% / 88% | 47.82 / 46.44 / 43.75 / 41.01 / 37.30 | 6.29 / 6.29 / 6.29 / 6.29 / 5.84 | 6 / 8 / 8 / 8 / 8 |
| ClaymoreSword | 12% / 25% / 50% / 75% / 88% | 54.23 / 50.95 / 44.52 / 38.09 / 34.74 | 5.02 / 4.97 / 4.51 / 4.06 / 4.11 | 20 / 8 / 8 / 8 / 8 |

Charts draw thickness at two times width scale so the planes remain legible.
Labelled millimetre values remain the unexaggerated model intersections.

## What each donor teaches

### ArmingSword: compound plane and paired taper

The section remains an eight-point compound or lenticular construction across
all five stations. It does not merely shrink in outline: width falls from
55.45 mm to 27.78 mm while thickness falls from 6.01 mm to 1.94 mm. The 88%
section retains 50.1% of 12% width but only 32.2% of its thickness.

Transfer: build a section rail with independently controlled half-width and
half-thickness at every station. Do not scale one root section uniformly toward
the tip. The shader should reveal authored planes, not fake them with a bright
edge mask.

### BastardSword: fuller persistence and termination

The first three stations have 12 intersections and a recessed centre on both
sides. At 75% the section becomes an eight-point compound section, so the model
carries an explicit fuller-to-point transition between 50% and 75% blade
length. Width and thickness both taper strongly.

Transfer: this is the best topology donor for a fullered blade. Author a fuller
floor, shoulder transitions, body lands, bevels, and cutting edge as real
semantic regions. Narrow and lift the fuller floor into the body before the
distal point instead of ending it with a texture mask or hard cap.

### LongSword: useful negative control

The model narrows in width, but thickness is exactly 6.293 mm at the 12%, 25%,
50%, and 75% samples and falls only to 5.835 mm at 88%. Its 88-to-12 ratio is
78.0% in width and 92.7% in thickness. In neutral proof, its broad blade faces
read unusually uniform.

Transfer: keep this as a failure control. If one of our blades behaves this way
without a documented construction reason, the generator is probably scaling
only the 2D silhouette and omitting distal taper.

### ClaymoreSword: forte complexity that resolves

The 12% station has 20 intersections: extra fluting or ridging exists at the
forte. By 25% the cross section simplifies to eight points and stays there
through distal stations. This is the clearest model of a complex root
construction deliberately resolving into a quieter distal blade.

Transfer: treat forte reinforcement, central ridges, and distal body planes as
separate station regimes with a controlled transition span. Decorative guard
terminals should also remain separate reusable components, not be baked into
the blade or material.

## Construction rules for our sword generator

1. Define root, forte, mid-blade, transition, and near-tip stations in metres.
2. At every station author half-width, half-thickness, fuller depth/width,
   body-land width, bevel start, and edge land independently.
3. Connect like semantic vertices between stations. Never ask triangulation or
   smooth shading to invent longitudinal plane flow.
4. Give fuller starts and terminations explicit transition stations. They are
   geometry events, not colour patterns.
5. Derive `body`, `fuller`, `bevel`, and `edge` attributes from authored section
   roles before modifiers or export.
6. Construct blade, guard, grip, pommel, and optional guard terminals as
   independently inspectable parts with shared datums.
7. Validate profile and distal taper numerically, then inspect neutral and
   grazing-light response. Neither test substitutes for the other.
8. Apply the existing clean-steel shader unchanged first. If all donors produce
   indistinguishable highlights, diagnose geometry and normals before inventing
   another texture pattern.

## Visual review and rejection ledger

- Accepted for study: construction separation, distinct section strategies,
  deterministic intersections, exact model-space values, and a neutral board
  that makes hilt and blade ownership inspectable.
- Repaired before registration: first oblique views rolled into a V-shaped
  composition; the revised blade view provides a stable longitudinal read.
  First charts used 4x thickness and overlapped rows; they now use 2x display
  thickness and retain exact labels.
- Weakness retained: these are low-poly game assets. Edge lands, section
  transitions, and hilt construction are simplified, and topology-wire detail
  is subordinate at full-board scale.
- Rejected use: source material slots are component clues only. No source
  colour, roughness, normal, or texture is evidence for our steel finish.
- State: `usable_reference_study_with_source_limits`. This accepts neither a
  donor as our production sword nor a material on it.

## Next decision gate

Build one cheap matched response board that applies the existing clean-steel
graph unchanged to the four isolated donor blades. The question is narrow: does
the established shader reveal different fullers, planes, and taper strategies
under the same neutral, grazing, and gameplay lights? Do not fork the shader,
modify the donors, or promote a donor into production during that gate.
