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
| `homestead/modular/window_frame_1p5x1p2` | Open timber window frame | None | No |
| `homestead/modular/gable_cap_4x1p5` | Gable infill | None | No |
| `homestead/modular/roof_shed_4x4` | Single-slope roof panel | None | No |
| `homestead/modular/roof_gable_slope_4p4x2p2` | Half-gable roof panel | None | No |
| `homestead/modular/roof_ridge_4p4` | Gable ridge cap | None | No |

## Assembly rules

- Snap foundations, floors, ceilings, and full walls on a 4 m bay grid.
- Place walls at foundation/floor height. Their 3 m height defines the normal
  story height.
- Build a physically open doorway from two separated wall piers and one lintel.
  Add the door leaf only when the opening should be closed.
- Build a window opening from solid surrounding modules, then place the
  no-collision window frame inside the opening.
- Pair two gable-slope panels and cap their meeting edge with the ridge module.
- Use the gable cap only as visual infill. The roof family intentionally emits
  no aggregate collision until convex or mesh collision cooking exists.
- Preserve each module's bottom-center source origin when rotating or scaling.

Creative currently cooks one aggregate AABB for a `bounds` asset. A decorative
wall mesh with a hole would therefore still block the entire opening. The
separate pier/lintel recipe is deliberate: collision follows visible solid
pieces instead of fabricating an invisible wall across doors. Window frames and
pitched roof pieces use `collision=none` for the same reason.

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
cmake --build build --target static_mesh_asset_tests
ctest --test-dir build -R '^static_mesh_asset_tests$' --output-on-failure
```

The catalog test pins every stable ID, metadata category, collision mode,
walkability flag, finite non-degenerate bounds, and preview-atlas inclusion.

## Deferred extensions

- Convex or triangle-mesh collision for pitched roofs and non-box architecture.
- Hinges, sockets, opening state, and attachment metadata for doors/windows.
- Textured and weathered material variants.
- Stair, railing, corner-wall, arch, chimney, and trim modules.
- LODs and cooked cross-run mesh packages.
