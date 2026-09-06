# wood_plank_v2

`wood_plank_v2` is the second authored-pattern material. It replaces the
equal-width, uninterrupted, sine-grain construction of the palette prototype
with:

1. nine physically sized, hand-proportioned wide-plank rows;
2. two cyclic segments per row so joints can be staggered without breaking
   the texture seam;
3. bounded deterministic variation of widths and joints;
4. a virtual-log node that projects flat-sawn ring history into each segment;
5. authored saw angle and pith drift, plus slowly changing ring spacing;
6. ring-owned earlywood and latewood pore families, cut-dependent medullary
   rays, three broken long-axis fibre families, and protected rest regions;
7. a twenty-shade brown family owned by every individual segment, sampled
   continuously by slow pigment drift, growth history, and tissue;
8. an eight-layer pigment stack: heartwood, cool wash, warm wash, earlywood,
   latewood, vessels, rays, and fibres;
9. four authored branch intersections with core, rings, boundary, influence,
   and coordinate warp;
10. shared structural masks for base color, height, normals, AO, and roughness.

The current acceptance milestone is oak grain and pigment without damage,
finish, semantic ink, or directional light hiding it. Damage is a separate
library concern: intrinsic checks and splinters, usage marks, and
geometry-aware placement must not be treated as one generic tile layer. The
older damage diagnostics remain available for later extraction, but are not
evidence that scene-context placement is solved.

## Authored layout

The canonical composition is
`patterns/aged_oak_champion_v1.json`. Variation `0` preserves the authored
physical widths, joint positions, saw histories, rest regions, and branch
records. Positive variation indices currently change widths and joints within
the recipe's limits while keeping the result deterministic and seamless.

The pattern sheet shows variations `0` through `11`; the compiled choice has a
gold outline.

## Generate

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/wood_plank_v2/generate_wood_plank_v2.py \
  -- \
  --resolution 2048 \
  --pattern-variation 0
```

The default tile represents `1.6 m × 1.6 m`. Generated outputs are:

- `wood_plank_v2_basecolor.png`;
- `wood_plank_v2_normal.png`;
- `wood_plank_v2_orm.png`;
- `wood_plank_v2_height.png`, as 16-bit linear data;
- `wood_plank_v2_manifest.json`;
- `proofs/wood_plank_v2_breakdown.png`, the visual index for the eight-chapter
  material book;
- `proofs/wood_plank_v2_anatomy_causality.png`, the required unwarped,
  branch-only, coordinate-warp, growth-flow, and knot-component comparison;
- `proofs/wood_plank_v2_grain_anatomy.png`, the damage-free ring-spacing,
  earlywood/latewood, pores, rays, fibre, rest, color, and white-light proof;
- `proofs/wood_plank_v2_pigment_layers.png`, the cumulative eight-layer color
  stack before finish, knots, damage, dirt, ink, or light;
- `proofs/wood_plank_v2_intra_board_shades.png`, a single-segment proof of its
  twenty control shades, actual shade occupancy, continuous palette
  coordinate, three color drivers, unlit result, and neutral-light result;
- `proofs/wood_plank_v2_damage_causality.png`, the typed check, splinter,
  scratch, habitat, raised-lip, finish-loss, and height diagnostic retained
  for the future damage-library workstream;
- `proofs/wood_plank_v2_book/*.png`, with full-page studies of layout, joints,
  grain, character, height, color, response, and repetition;
- `wood_plank_v2_material_book.md`, the long-form companion;
- `proofs/wood_plank_v2_tiling.png`;
- `proofs/wood_plank_v2_pattern_variations.png`.

This package does not yet replace `palette_v1/oak_plank.png` for existing
Creative assets. That integration should happen only after a variation is
accepted and the renderer/material binding route is selected.

## Focused test

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python tests/unit/wood_plank_v2_generator_tests.py
```

The test covers authored-recipe validation, bounded deterministic variation,
board and segment survival, periodic seams, ring-spacing variation,
ring-porous vessel ownership, cut-dependent rays, broken fibre families,
protected rests, the explicit pigment-layer order, sparse characters,
coherent PBR layers, and the packaged 16-bit height contract. It also verifies
that each individual segment owns twenty related brown controls, that one
isolated segment uses at least sixteen shade neighborhoods, that transitions
remain continuous, and that all eight material-book chapters and the written
companion are present.
