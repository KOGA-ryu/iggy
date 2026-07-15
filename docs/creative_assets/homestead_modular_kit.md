# Homestead Modular Kit

## Purpose

The homestead kit is a 4-meter structural building set for fast Creative map
blockout. Its modules share a bottom-center origin, meter scale, restrained
timber/plaster/stone materials, and stable asset IDs beneath
`homestead/modular/`.

The kit is generated from
`tools/blender/build_homestead_modular_kit.py`. The script is the editable
source of truth; the checked-in GLBs are runtime products. An optional generated
`.blend` file provides a gallery and assembled-house inspection scene.

Every module is below 2,000 triangles. The foundation is currently the largest
at 1,836 triangles; repeated placements use Creative's immutable asset atlas and
instanced draw path.

## Modules

| Asset ID | Role | Collision | Walkable |
|---|---|---|---|
| `homestead/modular/foundation_4x4` | Stone 4 m bay base | Bounds | Yes |
| `homestead/modular/floor_4x4` | Boarded 4 m floor | Bounds | Yes |
| `homestead/modular/ceiling_4x4` | Plaster ceiling panel | Bounds | No |
| `homestead/modular/wall_full_4x3` | Full 4 m by 3 m wall | Bounds | No |
| `homestead/modular/wall_half_2x3` | Half-width wall | Bounds | No |
| `homestead/modular/wall_low_4x1p2` | Low wall or sill course | Bounds | No |
| `homestead/modular/wall_pier_1x3` | Narrow wall pier | Bounds | No |
| `homestead/modular/lintel_2x0p6` | Door/window header | Bounds | No |
| `homestead/modular/pillar_0p5x3` | Freestanding timber post | Bounds | No |
| `homestead/modular/beam_4x0p4` | Horizontal structural beam | Bounds | No |
| `homestead/modular/door_leaf_1p1x2p2` | Hingeless door leaf | Bounds | No |
| `homestead/modular/door_frame_1p5x2p46` | Socketed timber doorway | Compound bounds (3) | No |
| `homestead/modular/window_frame_1p5x1p2` | Open timber window frame | None | No |
| `homestead/modular/gable_cap_4x1p5` | Gable infill | None | No |
| `homestead/modular/roof_shed_4x4` | Single-slope roof panel | None | No |
| `homestead/modular/roof_gable_slope_4p4x2p2` | Half-gable roof panel | None | No |
| `homestead/modular/roof_ridge_4p4` | Gable ridge cap | None | No |
| `homestead/modular/stair_straight_2x3x1p5` | Six-step straight stair | Compound bounds (6) | Yes (6) |
| `homestead/modular/porch_4x2x0p5` | Two steps and entry deck | Compound bounds (3) | Yes (3) |
| `homestead/modular/bridge_4x2` | Walkable deck with side barriers | Compound bounds (3) | Deck only |

## Assembly rules

- Snap foundations, floors, ceilings, and full walls on a 4 m bay grid.
- Place walls at foundation/floor height. Their 3 m height defines the normal
  story height.
- Place the doorway frame, then aim the door leaf at it. Its `door.frame` plug
  snaps to the frame receiver; an available receiver previews green and an
  occupied receiver previews red. The accepted leaf is stored as an attached
  child, so save/load, clipboard, undo, and authored-asset refresh preserve the
  relationship.
- For wider custom openings, continue using two separated wall piers and one
  lintel. Add an unsnapped door leaf only when the opening should be closed.
- Build a window opening from solid surrounding modules, then place the
  no-collision window frame inside the opening.
- Pair two gable-slope panels and cap their meeting edge with the ridge module.
- Use the gable cap only as visual infill. The roof family intentionally emits
  no aggregate collision until convex or mesh collision cooking exists.
- Use the stair on a 0.5 m horizontal cadence. Its six 0.25 m rises reach 1.5 m.
- Use the porch at entrances that need a 0.5 m approach. Its two steps and deck
  are separate walkable collision parts.
- Scale or rotate the bridge as one object. The deck remains walkable while the
  two parapet collision parts remain blockers.
- Preserve each module's bottom-center source origin when rotating or scaling.

Creative cooks one aggregate AABB for a `bounds` asset and up to 256 node-local
AABBs for a `compound_bounds` asset. A decorative wall mesh with a hole would
still block the entire opening when authored as one bounds asset. The separate
pier/lintel recipe is deliberate: collision follows visible solid pieces
instead of fabricating an invisible wall across doors. Window frames and
pitched roof pieces use `collision=none` because their shapes still require
convex or triangle-mesh collision.

Compound boxes follow custom object bounds, non-uniform scale, yaw, and the
asset pivot. Pitch or roll retains the blocker boxes but suppresses horizontal
walkable tops. The stair risers are deliberately 0.25 m, below the runtime's
0.35 m step-height policy. The physics movement planner uses an authored
walkable-only up-forward-down step candidate, so each stair tread can be
traversed without treating decorative or unsupported blockers as ground.

## Regeneration

Run Blender 4.5 LTS or newer in background mode:

```sh
blender --background --factory-startup \
  --python tools/blender/build_homestead_modular_kit.py -- \
  --output-root assets/creative/homestead/modular \
  --preview build/artifacts/homestead_modular_kit_preview.png \
  --assembly-preview build/artifacts/homestead_modular_kit_assembly.png \
  --blend build/artifacts/homestead_modular_kit_gallery.blend
```

The exporter writes untextured material-color GLBs without unused UV streams.
Separate clean Blender runs must produce byte-identical files, because Creative
uses the complete GLB byte hash as the asset content revision.

Verify the production importer and catalog contract with:

```sh
cmake --build build --target static_mesh_asset_tests creative_asset_room_bake_tests
ctest --test-dir build \
  -R '^(static_mesh_asset_tests|creative_asset_room_bake_tests)$' \
  --output-on-failure
```

The catalog test pins every stable ID, metadata category, collision mode,
walkability flag, finite non-degenerate bounds, collision-part/socket count,
and preview-atlas inclusion. The RoomBake test loads the checked-in stair,
porch, bridge, and doorway GLBs through the production importer and pins their
independent blocker and walkable surfaces.

## Deferred extensions

- Convex or triangle-mesh collision for pitched roofs and non-box architecture.
- Hinges, opening state, and socketed window variants.
- Textured and weathered material variants.
- Railings, corner walls, arches, chimneys, curved stairs, and trim modules.
- LODs and cooked cross-run mesh packages.
