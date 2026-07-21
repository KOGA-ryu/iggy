# palette_v1 - shared albedo tile set (ASSET-BLD-1 Phase 2)

Six seamless 1024px base-color tiles, procedurally generated
(deterministic numpy value noise, periodic by construction - see
generate_palette.py; regenerate with: blender -b --factory-startup
--python generate_palette.py). Source: procedural, no external downloads.

| Tile | Use | Meters/tile guide |
|---|---|---|
| oak_plank | door leaves, shutters, deck boards | 1.1 |
| oak_timber | posts, beams, rails, framing | 1.2 |
| lime_plaster | wall infill panels | 2.5 |
| stone_rough | base courses, chimneys | 1.6 |
| shingle_oak | all roof trim families | 2.0 |
| iron_forged | hardware, brackets, bars | 1.0 |

Law: palette-only materials - assets map every material to one of these
tiles; no per-asset textures. Tiling proofs (3x3) and the swatch sheet are
under proofs/. Wear is restrained per reference sheet 08; heavier wear
variants are a future palette revision, not per-asset edits.
