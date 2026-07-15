# Cave Interior Kit

## Purpose

The Cave Interior Kit turns the Rock and Cliff Kit's cave mouth into playable
interior space. It provides both complete tunnel stamps for fast blocking and
separate floor, wall, corner, ceiling, ramp, and ledge modules for refinement.
Every asset uses meters, deterministic geometry, and a stable asset ID below
`cave/`.

The editable source of truth is
`tools/blender/build_cave_interior_kit.py`. Checked-in GLBs are runtime
products. The script also renders an isolated gallery, a connected cutaway
cave proof, and an optional inspection `.blend` file.

## Assets

| Asset ID | Category | Collision contract |
|---|---|---|
| `cave/floor_4x4` | `terrain` | 6 walkable floor slabs |
| `cave/ramp_4x4x1p5` | `stairs` | 8 walkable stepped slabs |
| `cave/wall_straight_4x3` | `terrain` | 8 structural rocks |
| `cave/wall_corner_inner_4x4x3` | `terrain` | 12 structural rocks |
| `cave/wall_corner_outer_4x4x3` | `terrain` | 12 structural rocks |
| `cave/ceiling_4x4` | `terrain` | 6 overhead blockers |
| `cave/tunnel_straight_4x4x3p5` | `structure` | 6 walkable + 14 structural parts |
| `cave/tunnel_turn_6x6x3p5` | `structure` | 5 walkable + 25 structural parts |
| `cave/tunnel_t_junction_6x6x3p5` | `structure` | 5 walkable + 23 structural parts |
| `cave/tunnel_dead_end_4x4x3p5` | `structure` | 6 walkable + 20 structural parts |
| `cave/chamber_8x8x4` | `structure` | 9 walkable + 31 structural parts |
| `cave/rock_column_1p4x3p2` | `structure` | 3 structural parts |
| `cave/stalactite_cluster_2x2x3` | `decor` | None |
| `cave/stalagmite_cluster_2x2x2` | `decor` | None |
| `cave/collapsed_passage_4x3` | `structure` | 9 structural parts |
| `cave/ledge_walkable_4x2` | `terrain` | 3 structural + 2 walkable parts |

Every playable shell uses authored compound AABBs. No cave asset requests
triangle-mesh or convex collision. Floor slabs and ramp treads expose only
their own top surfaces as walkable. Wall and ceiling parts remain blockers.
Stalactites and stalagmites are non-colliding dressing so creators can place
them densely without generating navigation noise.

The complete stamps share the same floor, wall, and ceiling construction
kernels as the separate modules. They are convenience compositions, not a
second geometry or collision system.

## Orientation and Assembly

- Straight tunnels run along local Y and remain open at local `-Y` and `+Y`.
- The turn accepts a tunnel from local `-Y` and exits through local `+X`.
- The T junction accepts a tunnel from local `-Y` and exits through local
  `-X` and `+X`.
- The dead end is open at local `-Y` and closed at local `+Y`.
- The chamber has a broad entrance at local `-Y` and one side opening at local
  `+X`.
- Ramps ascend toward local `+Y` from ground level to 1.5 m.
- Straight walls run along local X. Rotate wall and ceiling modules around Z
  in 90-degree increments to refine a stamped shell.
- The inner corner leaves its local negative quadrant open. The outer corner
  grows from its origin through local `+X` and `+Y`.
- Stalactites are authored for a roughly 3.2 m ceiling. Stalagmites are
  floor-aligned.
- The collapsed passage is a deliberate full blocker. Use rubble or scree from
  the Rock and Cliff Kit when the obstruction should remain traversable.

Integrated tunnel openings are wider than an ordinary player capsule and are
not bridged by aggregate collision. The assembled proof hides ceiling meshes
only for its cutaway camera; exported runtime assets retain those ceilings and
their authored blockers.

## Regeneration

Run Blender 4.5 LTS or newer. `--factory-startup` prevents user-installed
addons from affecting background export:

```sh
blender --background --factory-startup \
  --python tools/blender/build_cave_interior_kit.py -- \
  --output-root assets/creative/cave \
  --preview build/artifacts/cave_interior_kit_gallery.png \
  --cave-preview build/artifacts/cave_interior_kit_assembly.png \
  --blend build/artifacts/cave_interior_kit_gallery.blend
```

The Linux rendering worker can run the same command in a temporary checkout;
only the GLBs return to `assets/creative/`. Preview PNGs and the inspection
`.blend` stay under the ignored `build/artifacts/` directory.

Two clean Blender runs must produce byte-identical GLBs. Creative uses each
complete GLB byte hash as its content revision, so nondeterministic export
would cause false asset refreshes.

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

- Curved splines, terrain-conforming entrances, and automatic tunnel stitching
  in the editor.
- Climbing, squeezing, swimming, cave hazards, and traversal-specific AI
  semantics.
- Underground water, wet-surface rendering, fog volumes, and authored cave
  lighting.
- Mining supports, doors, rails, crystals, ruins, and biome material variants.
- Authored LODs or merged instance clusters only after map-scale profiling
  identifies a measurable bottleneck.
