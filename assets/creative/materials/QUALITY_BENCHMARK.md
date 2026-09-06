# Material Quality Benchmark: Stone Rough V2 and Wood Plank V2

Evaluation date: 2026-07-28

## Verdict

`stone_rough_v2` and `wood_plank_v2` are structured procedural prototypes, not
high-quality finished materials. Their strongest achievement is provenance:
pattern, height, color, roughness, and damage can name their generating fields.
Their weakest achievement is the surface itself. The fields are too simple,
too uniform, and too weakly art-directed to produce convincing stone, wood, or
the requested Arcane/Borderlands-adjacent stylization.

The existing material books make this worse by describing each generated mask
as if exposing a mask proved quality. They do not compare against professional
assets, show the material under demanding lighting, inspect distance and mip
behavior, or distinguish a technically valid map from a visually persuasive
one. More explanatory panels cannot repair shallow source material.

The correct next step is to rebuild the material synthesis against external
benchmarks, then replace the proof books with adversarial review plates.

## Benchmark set

The comparison uses three kinds of reference. No third-party imagery should be
copied into the repository; the linked pages remain the visual source.

### Material-construction benchmarks

1. [Daniel Thiger — Substance Designer Stylized Stone Wall](https://www.artstation.com/artwork/68alEx)
   is the closest stone target: simplified and stylized, but with believable
   height, softened forms, varied faces, and a strong tactile read.
2. [Joe Taylor — Stone Wall](https://gamesartist.co.uk/stone-wall/) documents a
   professional wall workflow in detail. It separates pattern, repeated edge
   sculpt passes, edge wear, large and medium surfacing, deliberately quiet
   stones, per-brick directional warping, grout height, color, dirt, and
   roughness.
3. [Silvija Strončikaitė — Stylized Wood Planks](https://www.artstation.com/marketplace/p/5nax/stylized-wood-planks)
   is a 4K production-oriented Substance asset with source graph and exposed
   controls. Its renders show deeply integrated growth rings, knots, board
   deformation, damaged edges, fasteners, scratches, and a readable specular
   response.
4. [Mark Foreman — Substance Designer Tips and Tricks](https://www.adobe.com/learn/substance-3d-designer/web/mark-foreman-s-substance-3d-designer-tips-and-tricks)
   gives a particularly useful wood construction: softly distorted
   anisotropic grain, separate growth-ring and fibre families, knot-driven
   deformation, mid-scale directional noise, separately colorized grain
   layers, and related but distinct roughness.
5. [Adobe — Creating Old Wood Planks](https://www.adobe.com/learn/substance-3d-designer/web/creating-old-wood-planks-in-substance-3d-designer)
   treats wood as a sequence of pattern, plank form, height, knots, vector
   warp, knot integration, nails, roughness, color, material blending, and
   rendering. That sequence is a useful completeness checklist.

### Shipped-use benchmarks

1. [Chris Hodgson — The Last of Us Part I Materials](https://christopherhodgson.artstation.com/projects/Keed2G)
   shows herringbone wood, floorboards, tiling woods, and an old stone wall
   built for Naughty Dog's shared material library and demonstrated in actual
   environments. The important benchmark is not photorealism; it is validation
   in blends, lighting, props, and world context.
2. [Raphael Jean — Borderlands 3 Materials](https://raphjean84.artstation.com/projects/2xvDre)
   shows production tiles for modular Hyperion kits plus Substance-authored
   floor and wall experiments. It establishes that the target look is a system
   of tileables, trims, atlases, and scene use—not one isolated square map.
3. [Lucas Lanier — Borderlands 3 Environment Assets](https://zombinian.artstation.com/projects/gJLmdx)
   credits trim sheets, tileable textures, decals, and overlay inks as distinct
   contributors to the final environment. A standalone PBR tile cannot carry
   the complete Borderlands look.

### Stylization benchmarks

1. [Fortiche's description of its visual style](https://forticheprod.com/faq/)
   explicitly centers human imperfection and organic, hand-crafted texture
   rather than digital precision.
2. [Unreal Engine's Borderlands retrospective](https://www.unrealengine.com/blog/borderlands)
   describes the goal as rendering concept-art character directly in the game
   and notes that material, lighting, and shading changes were developed
   together.
3. [Illustrative Rendering in Team Fortress 2](https://www.riotgames.com/darkroom/original/87b07e8dde1ae968b72eb5e60c7ede9b%3A0ea751891424f001e471f06a521fabd8/npar07-illustrativerenderinginteamfortress2.pdf)
   remains a useful adjacent production reference: loose brushwork, controlled
   value and saturation, reduced repetition, and deliberate suppression of
   high-frequency noise preserve composition and readability.

These references point in the same direction. High-quality stylization is not
“more noise” and not a post-process outline over weak textures. It requires
strong physical form, selective simplification, controlled areas of rest,
material-specific marks, and explicit painterly or ink lanes.

## Rating scale

- **0 — absent:** the material does not attempt the requirement.
- **1 — prototype:** a basic signal exists but reads as a generator primitive.
- **2 — competent base:** structurally useful, visibly unfinished.
- **3 — production candidate:** convincing in neutral review, needs art pass.
- **4 — portfolio or shipped quality:** survives close-up, distance, and scene use.
- **5 — signature quality:** form and stylization are distinctive and memorable.

The scores below are an art-review rubric, not a claim of objective measurement.
They force each judgment to name visible evidence.

## Stone comparison

| Criterion | Professional benchmark | `stone_rough_v2` evidence | Score |
|---|---|---|---:|
| Pattern language | Stones have construction logic, varied aspect ratios, pressure, rests, and non-uniform joints. Even stylized cells feel intentionally fitted. | Thirty-one unequal cells are better than a grid, but the result still reads as a Voronoi patio. Most boundaries are long straight segments and three-way junctions have the same procedural signature. | 2 |
| Macro volume | Per-stone raise, tilt, crown, concavity, and overlap create different light behavior before surface noise. | Per-cell raise varies from 10–19 mm, but faces remain broadly parallel. At 512 px, the median interior normal deflection is only 0.60 degrees and the 90th percentile is 4.64 degrees. | 1 |
| Edge design | Multiple sculpt and wear passes create softened sections, sharp breaks, chisel terraces, broad losses, and untouched rests. | One distance-derived bevel controls nearly every edge. Chips cover only 0.29% on average and rarely alter a stone's silhouette. The normal map reads as clean colored polygon outlines. | 1 |
| Face hierarchy | Large planes establish touch; medium fractures and crumbling interrupt selected regions; micro-detail finishes the material. Per-stone warping prevents a global overlay. | A global five-cell FBM and global seventeen-cell FBM cross the entire tile. They do not rotate, stretch, or change material behavior per stone. Large and medium geological structure is missing. | 1 |
| Damage grammar | Spalls remove broad edge masses, fractures branch from stress, pitting clusters by stone type, and some stones remain quiet. | Chip, pit, and crack masks are technically separate, but cracks appear as thin wandering contours and pits as isolated dots. No broken slabs, flake planes, crushed corners, or material-dependent erosion exist. | 1 |
| Mortar | Grout has changing depth, width, bulge, aggregate, recession, contact shadows, and local deposits. It negotiates the shape of adjacent stones. | Mortar is mostly a consistently dark, smooth channel around every cell. Its width and surface response are too uniform, so it reads as an outline rather than packed material. | 1 |
| Color structure | Color follows stone type, face orientation, mineral zones, deposits, exposed breaks, and grout history while preserving broad value grouping. | The 5th–95th luminance range is only 0.312–0.425. Cell colors are close gray-browns; mineral and temperature layers barely survive. The result is dull rather than restrained. | 1 |
| Roughness and specular | Roughness is low-frequency first, then modified selectively by exposed aggregate, polished faces, moisture, dirt, and damage. A grazing render shows the breakup. | The numerical range is broad, roughly 0.708–0.911 for the 5th–95th percentiles, but the proof renderer does not reveal a persuasive reflection story. The map is not demonstrated under a specular sweep. | 2 |
| Stylized authorship | Simplification is selective: exaggerated planes and edge rhythm coexist with visible hand, brush, or ink decisions. | Polygon regularity remains digital while the painterly layer is nearly absent. No directional brush field, ink mask, highlight stroke mask, or artist-authored focal accents exist. | 1 |
| Tiling and use | The tile is reviewed at several repetitions, distances, lighting angles, and in an actual wall assembly with blends and decals. | A 2×2/4×4 repetition exists, but no mip ladder, camera-distance proof, wall-corner use, material blend, decal pass, or renderer integration exists. | 2 |

**Stone result: 1.3 / 5.** The material has a valid data pipeline and an
editable layout, but its visual vocabulary is one bevel, two global noises,
and three sparse damage masks. It is not yet a convincing stone study.

### What the benchmark does that stone does not

Joe Taylor's workflow is especially diagnostic. He layers several distinct
edge-sculpt passes, masks wear to selected bricks, varies frequency, and
preserves intentionally planar areas of rest. Surface grunge is directionally
warped per brick, so the material does not look like one cloud field laid over
many shapes. Stone angle and height vary before grout is height-blended.
Grout has its own form, and color/roughness reuse masks without becoming copies
of height.

`stone_rough_v2` currently skips most of that middle structure. It jumps from
cell diagram to rounded edge, then from rounded edge to fine procedural noise.
That missing middle is where the tactile stone identity should live.

## Wood comparison

| Criterion | Professional benchmark | `wood_plank_v2` evidence | Score |
|---|---|---|---:|
| Plank composition | Board widths, lengths, joins, fastener logic, bow, repair, and quiet areas create a believable laying history. | Seven unequal boards and fourteen cyclic segments are a useful start. The vertical arrangement is clean and readable, but every board is still perfectly straight and mechanically cut. | 2 |
| Board form | Cupping, bowing, planing, raised ends, worn shoulders, compression, and damaged corners create a strong grazing-light read. | Segment raise exists, but board interiors are almost flat: median normal deflection is 0.51 degrees and the 90th percentile is 1.44 degrees. Edges are uniform and undamaged. | 1 |
| Growth-ring construction | Rings change width, merge, pinch, open around knots, and transition between earlywood and latewood. Their rhythm is directional but not periodic. | Broad grain is a cosine with 5.5–9.5 cycles across a segment. Fine grain is another cosine at `18 + ring_count`. Even with warp, the result reads as evenly spaced barcode lines. | 1 |
| Fibres and pores | Fine fibres are anisotropic, broken, soft-edged, and layered with a separate mid-frequency family. They do not all have equal contrast. | Fine grain exists as a second mask but shares the same binary crest logic. There is no pore family, broken fibre bundle, soft vessel field, or selected region of quiet grain. | 1 |
| Knots | Knots have an eye, concentric or elliptical ring compression, surrounding grain deflection, checking, and varied life stages. | Four authored knot segments produce blurred dark dots. The phase field bends slightly, but the visual result has no ring body, pinching, branch direction, halo, or knot-specific crack system. | 1 |
| Edge and end history | Ends show saw direction, checks, compression, splinters, nail logic, and local wear. Long edges vary between fresh, rounded, chipped, and abraded. | End accents are thin dark bands and rifts are long clean curves. There are no end checks, splinters, crushed fibres, edge chips, scratches, fasteners, or repair marks. | 1 |
| Color structure | Growth rings, fibre, heartwood/sapwood, oxidation, finish, polish, dirt, and exposed cuts contribute different hue/value changes. | Segment palette assignment provides broad board variation, but color mostly follows a single brown plus a 9% macro light wash and dark grain. Ring-specific color complexity and use history are absent. | 1 |
| Roughness and specular | Roughness follows fibre direction and finish, with polished traffic lanes, porous cuts, damaged varnish, and subdued knot response. | The 5th–95th roughness range is approximately 0.605–0.896, but its directional structure is not visible in the proof. There is no finish layer, polish history, or anisotropic response. | 1 |
| Stylized authorship | The material selects and exaggerates grain sweeps, edge shadows, brush marks, and focal knots while protecting large calm shapes. | Lines are procedural and uniformly thin. There is no painterly stroke hierarchy, ink lane, highlighted fibre group, or hand-authored emphasis. | 1 |
| Tiling and use | Repetition is tested in a room-scale floor under movement, mips, lighting, props, trims, and decals. Compatible variants break landmarks. | The tile is seamless and has twelve layout variants, but only one compiled tile is shown as a repeated square. No room-scale placement, mip test, trim transition, or decal/overlay system exists. | 2 |

**Wood result: 1.2 / 5.** The layout is a credible scaffold, but the material
looks like a clean laminate diagram. The grain function, knot construction,
board relief, edge history, and finish response all need replacement or major
expansion.

### What the benchmark does that wood does not

Mark Foreman's process starts with softened anisotropic structure and introduces
only a small amount of broad distortion. Growth rings are made from varied,
flipped gradients and then warped through the wood base. Knots are not just
masks; they influence the ring field. A separate directional fibre family and
mid-size noise are blended into height. Grain and rings are colorized
separately and recombined specifically to avoid noisy contrast. Roughness uses
related shapes but a different input signal.

Silvija Strončikaitė's finished asset adds the material events that turn this
construction into a surface: strongly varied ring sweeps, broad board
deformation, worn and damaged edges, nail or hole rhythms, scratches, raised
ends, and a grazing-light response that remains readable across a large floor.

`wood_plank_v2` instead derives most visible grain from two thresholded cosine
waves. That mathematical regularity dominates every board and is the primary
reason the result feels synthetic.

## Arcane and Borderlands compatibility

The requested target is not one style.

- **Arcane/Fortiche:** painterly surfaces, organic imperfection, selective
  brush texture, restrained background detail, and authored value grouping.
- **Borderlands:** concept-art-like asset treatment combined with material and
  shading changes; production environments also use tileables, trim sheets,
  decals, and overlay inks.

The common ground is deliberate mark-making. Neither target is achieved by
adding generic grunge.

The material pipeline therefore needs explicit stylization outputs alongside
PBR data:

- `ink_mask`: structural edge, crack, fibre, and focal accent strokes;
- `highlight_stroke_mask`: sparse strokes that remain separately controllable;
- `brush_direction`: a two-channel flow field for surface-oriented paint;
- `pigment_variation`: broad authored color movement without baked scene light;
- `detail_priority`: a mask that protects calm areas and focal areas through
  mip reduction;
- normal, height, AO, and roughness derived from physical construction.

Cell shading and screen-space outlines belong to the renderer. Material-specific
ink, brush direction, and accent selection belong to the asset. Mixing these
responsibilities into base color would make the texture fight moving lights;
omitting the asset lanes entirely leaves the renderer with nothing distinctive
to stylize.

## Why the current proofs fail

The material books answer, “Which buffers exist?” They do not answer:

1. Does the material resemble excellent work of the same type?
2. Does height create convincing form under grazing light?
3. Does roughness break reflection in a material-specific way?
4. Are macro, medium, and micro frequencies independently readable?
5. Does the material preserve calm areas?
6. Does it survive 1×, 4×, 16×, and mip-distance repetition?
7. Does it work on a plane, corner, cylinder, and representative asset?
8. Does the stylized version hold under three light directions?
9. Which visible marks are procedural accidents and which are art-directed?
10. Does an in-scene render look closer to Arcane, Borderlands, neither, or a
    generic Substance exercise?

The proof books also use their own generated previews as both candidate and
judge. That circularity guarantees flattering conclusions.

## Required replacement proof system

Each material needs six adversarial plates. A chapter is accepted only when
the plate exposes failure as clearly as success.

### Plate 1 — Matched benchmark

- Link two legally referenced professional materials.
- Show the local candidate at matching crop scale and approximately matching
  light direction.
- Annotate five concrete differences in silhouette, frequency, edge, color,
  and response.
- Never embed or redistribute unlicensed benchmark maps.

### Plate 2 — Form under light

- Render a flat plane, 30-degree plane, sphere/cylinder, and corner.
- Use neutral white material light at frontal, three-quarter, and grazing
  directions.
- Show height-only clay, normal-only response, and final response.
- Include a normal-angle distribution for material faces, excluding joints.

### Plate 3 — Frequency ladder

- Pattern only.
- Macro volume only.
- Macro plus medium structure.
- Edge and damage structure.
- Micro-detail only.
- Final height and final normal.
- Include metre amplitudes and the intended viewing-distance band.

### Plate 4 — Color and stylization lanes

- Flat albedo with no AO or light.
- Broad pigment grouping.
- Material-event color.
- Ink mask.
- Highlight-stroke mask.
- Brush direction.
- Cel-ramp previews under three light directions.
- A desaturated value check and a 64-pixel thumbnail.

### Plate 5 — Reflection and packing

- Roughness strip under a moving or multi-angle grazing highlight.
- AO isolated from base color.
- ORM channels separately and packed.
- Report 5th, 50th, and 95th percentiles, but treat numbers as regression
  guards rather than artistic acceptance.

### Plate 6 — Repetition and world use

- 1×1, 4×4, and 16×16 repeats.
- Full-resolution, half-resolution, and representative mip-distance views.
- A wall corner for stone; a room-scale floor with trim transition for wood.
- Two compatible layout variants placed together.
- One decal/overlay-ink example.

## Repair order

### 1. Replace the wood grain model

This is the largest visible failure. Build a local wood coordinate system with:

- a soft anisotropic base;
- variable-width earlywood and latewood bands;
- knot bodies that deform the field;
- separate broken fibres and pores;
- a mid-frequency directional family;
- explicit areas of rest;
- separate height, pigment, and roughness interpretations of shared structure.

Do not continue polishing the current cosine grain.

### 2. Rebuild stone edges and mortar

Keep the authored layout, but replace the single uniform bevel with layered
edge classes: soft weathered shoulder, chisel plane, broad missing chunk, and
untouched edge. Give mortar its own changing depth, bulge, aggregate, recession,
and deposits. Large edge losses must alter silhouette, not only darken pixels.

### 3. Add per-element local frames and material families

Each stone needs a local orientation, planar tilt, stone family, and rotated
surface coordinates. Each board already has a local frame but needs family
parameters for cut, age, finish, ring density, and wear. Global noise may
modulate the composition; it must not be the main surface author.

### 4. Add authored stylization lanes

Generate ink, highlight strokes, brush flow, and detail priority from structural
masks, then allow hand-authored overrides. These lanes should be independently
visible and adjustable in the renderer.

### 5. Replace the proof books

The six plates above should replace celebratory chapter pages. Prose can remain
as support, but no material is accepted because its implementation is
well-described.

### 6. Validate in world context

Only after a single tile passes should compatible variations be placed across
a wall or floor. The final bar is not a clean PNG; it is a material that holds
shape, value, and authored marks inside the intended Creative scene.

## Acceptance statement

Neither material should be presented as “high quality,” “Arcane-like,” or
“Borderlands-like” in its current state. The accurate description is:

> Deterministic authored-pattern prototypes with coherent PBR map provenance,
> awaiting a professional surface, stylization, and in-scene validation pass.
