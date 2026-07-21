# Building Closure Reference Pack V1

## Purpose

These eight generated concept sheets establish a coherent placeholder art
direction for the first production building-closure assets:

- timber frame over rough-cut stone;
- lime-plaster infill;
- split-oak shingle roofs;
- dark hand-forged iron hardware;
- weathered but maintained construction;
- simple, plausible late-medieval forms without ornamental fantasy styling.

They are visual references, not dimensional authority. The numeric, pivot,
socket, collision, walkability, and stable-ID contracts in
[`bg3_informed_asset_master_backlog.md`](../../bg3_informed_asset_master_backlog.md)
always win when an image is ambiguous.

## Sheets

| File | Modeling use |
|---|---|
| `01_anchor_homestead.png` | Overall construction culture, proportions, palette, and material balance |
| `02_roof_closure_kit.png` | Ridge, hip, valley, eave, gutter, downspout, and chimney families |
| `03_door_gate_kit.png` | Door/gate frames, identical open/closed leaves, thresholds, hinges, and latches |
| `04_window_shutter_kit.png` | Window proportions, frame depth, shutters, sills, lintels, mullions, and bars |
| `05_stair_traversal_kit.png` | Full-storey stairs, landings, rails, ramp, ladder, and scaffold language |
| `06_structural_framing_kit.png` | Posts, beams, braces, arches, buttresses, balcony, rail, and plinth junctions |
| `07_modular_building_assembly.png` | Boundary between generated shell surfaces and reusable Blender components |
| `08_material_wear_guide.png` | Material palette, grain direction, junctions, and restrained wear distribution |

## Hard Modeling Constraints

- One Blender unit is one meter.
- Grounded assets rest at Z = 0.
- Normal storeys are 3.0 m floor-to-floor.
- The standard door clear opening is 0.9 x 2.1 m.
- Open and closed state meshes share identical geometry and pivots.
- Walkable stair treads and landings are authored separately from rail/support
  collision parts.
- Generated walls, floors, roof planes, and terrain remain generated geometry.
  The modeled kit supplies inserts, closures, trim, traversal, and dressing.
- Opaque base-color-first materials are the current target. Do not infer glass,
  alpha foliage, advanced PBR, VFX, or water support from the images.

## Generation Brief

The pack was generated one sheet at a time with the built-in image-generation
tool. Each prompt required a neutral studio background, consistent materials,
orthographic or technical-product framing, buildable modular construction,
minimal shadows, no text, and no dramatic scenery. Later sheets used earlier
sheets as visual references to preserve the family language.

## Known Concept-Art Ambiguities

- The anchor house uses a substantial stone lower storey. Treat this as one
  estate variant, not a requirement that every building use that facade split.
- Downspout material and fabrication should be simplified to what the current
  material system can support.
- Switchback stair imagery is conceptual. Exact 3.0 m rise, tread rhythm,
  passage width, landing alignment, and compound collision come from the
  authored contract.
- Exploded spacing in the assembly sheet communicates ownership and does not
  prescribe origin placement.

## Acceptance Use

Before accepting a Blender asset, compare it against all three relevant truths:

1. the master backlog's numeric and semantic contract;
2. this pack's family-specific construction and material language;
3. the calibration and assembled-bay proof renders produced from the actual
   exported GLB.
