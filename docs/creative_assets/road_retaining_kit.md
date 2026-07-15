# Road and Retaining Kit

## Purpose

The Road and Retaining Kit supplies modular connective infrastructure for
homesteads, yards, woodland edges, and authored terrain. Every asset uses
meters, a floor-aligned source origin, deterministic geometry, and a stable
asset ID below `infrastructure/`.

The editable source of truth is
`tools/blender/build_road_retaining_kit.py`. Checked-in GLBs are runtime
products. The script also renders an isolated gallery, a connected-road proof,
and an optional inspection `.blend` file.

## Assets

| Asset ID | Footprint | Category | Collision contract |
|---|---:|---|---|
| `infrastructure/stone_path_straight_2x2` | 2 x 2 m | `walkway` | 4 walkable stones |
| `infrastructure/stone_path_straight_4x2` | 4 x 2 m | `walkway` | 8 walkable stones |
| `infrastructure/stone_path_turn_4x4` | 4 x 4 m | `walkway` | 7 walkable stones |
| `infrastructure/stone_path_t_4x4` | 4 x 4 m | `walkway` | 10 walkable stones |
| `infrastructure/stone_path_cross_4x4` | 4 x 4 m | `walkway` | 12 walkable stones |
| `infrastructure/stone_path_end_2x2` | 2 x 2 m | `walkway` | 4 walkable stones |
| `infrastructure/doorway_threshold_2x1` | 2 x 1 m | `walkway` | 2 walkable stones |
| `infrastructure/stepping_stones_4x1p5` | 4 x 1.5 m | `walkway` | 7 walkable stones |
| `infrastructure/retaining_wall_2x1p2` | 2 x 0.5 m | `structure` | 1 wall core |
| `infrastructure/retaining_wall_4x1p2` | 4 x 0.5 m | `structure` | 1 wall core |
| `infrastructure/retaining_wall_corner_2x2x1p2` | 2 x 2 m | `structure` | 2 wall cores |
| `infrastructure/retaining_wall_end_1x1p2` | 1 x 0.5 m | `structure` | 1 wall core |
| `infrastructure/terrain_steps_2x3x1p2` | 2 x 3 m | `stairs` | 6 walkable treads |
| `infrastructure/drainage_culvert_2x2` | 2 x 2 m | `bridge` | 2 walls + 4 walkable deck stones |
| `infrastructure/roadside_marker_0p4x1p2` | 0.5 x 0.5 m | `marker` | Bounds |

Path collision follows the individual visible slabs. This prevents an L turn,
T junction, or crossroad from fabricating blockers across its empty corners.
The slabs rise roughly 8-10 cm above the placement plane, avoiding z-fighting
with terrain while remaining a small traversable step.

Retaining-wall masonry is decorative over one structural core per straight
arm. This keeps collision stable without baking every facing stone into the
physics query. Steps expose one walkable top per tread. The culvert keeps its
channel open along local X and carries a path across it along local Y.

## Orientation Rules

- Straight path pieces run along local X and are 2 m wide.
- The turn enters from local `-X` and exits through local `+Y`.
- The T junction spans local X and branches through local `+Y`.
- Rotate any path module around Z in 90-degree increments to obtain the other
  orientations. Adjacent module origins stay on the 2 m grid.
- Terrain steps ascend toward local `+Y` from 0.2 m to 1.2 m.
- Retaining straights run along local X with their dressed face toward `-Y`.
- The retaining corner pivots at the arm junction and extends through local
  `+X` and `+Y`. The end piece caps a straight run.
- The culvert channel runs through local X. Rotate it when the drainage course
  crosses the road in the opposite direction.
- These assets do not deform to arbitrary terrain. Grade transitions and
  terrain-conforming paths require an explicit authoring tool rather than
  hidden mesh mutation.

## Regeneration

Run Blender 4.5 LTS or newer. `--factory-startup` prevents user-installed
addons from affecting background export:

```sh
blender --background --factory-startup \
  --python tools/blender/build_road_retaining_kit.py -- \
  --output-root assets/creative/infrastructure \
  --preview build/artifacts/road_retaining_kit_gallery.png \
  --road-preview build/artifacts/road_retaining_kit_assembly.png \
  --blend build/artifacts/road_retaining_kit_gallery.blend
```

The Linux rendering worker can run the same command in a temporary checkout;
only the GLBs return to `assets/creative/`. Preview PNGs and the inspection
`.blend` stay under the ignored `build/artifacts/` directory.

Two clean Blender runs must produce byte-identical GLBs. Creative uses each
complete GLB byte hash as its content revision, so nondeterministic export would
cause false asset refreshes.

## Verification

```sh
cmake --build build --target i3dc static_mesh_asset_tests \
  creative_asset_room_bake_tests creative_editor_placement_tests
ctest --test-dir build \
  -R '^(static_mesh_asset_tests|creative_asset_room_bake_tests|creative_editor_placement_tests)$' \
  --output-on-failure
```

`static_mesh_asset_tests` pins every stable ID, category, collision mode,
finite non-degenerate bounds, collision-part count, walkable-part count,
catalog inclusion, and preview-atlas inclusion. The placement and RoomBake
targets protect the shared imported-asset path used by preview, commit, and
collision.

## Deferred Extensions

- Terrain-conforming path strokes, grade interpolation, and automatic junction
  selection in the editor.
- Dirt, gravel, snow, and damaged material variants.
- Curved retaining walls, ramps, switchbacks, guard rails, and larger culverts.
- Water rendering and flow semantics through drainage channels.
- LODs or merged instance groups only after map-scale profiling identifies a
  measurable bottleneck.
