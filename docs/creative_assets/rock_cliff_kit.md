# Rock and Cliff Kit

## Purpose

The Rock and Cliff Kit supplies modular natural terrain for cave approaches,
ridge lines, overlooks, elevation changes, and map-edge dressing. Every asset
uses meters, a floor-aligned source origin, deterministic geometry, and a
stable asset ID below `rock_cliff/`.

The editable source of truth is
`tools/blender/build_rock_cliff_kit.py`. Checked-in GLBs are runtime products.
The script also renders an isolated gallery, an assembled cliff proof, and an
optional inspection `.blend` file.

## Assets

| Asset ID | Category | Collision contract |
|---|---|---|
| `rock_cliff/boulder_medium_1p8m` | `rock` | Aggregate bounds |
| `rock_cliff/boulder_large_3m` | `rock` | Aggregate bounds |
| `rock_cliff/rock_outcrop_low_3x2` | `terrain` | 4 structural parts |
| `rock_cliff/rock_outcrop_tall_3x3` | `terrain` | 5 structural parts |
| `rock_cliff/cliff_face_2x2` | `terrain` | 4 structural parts |
| `rock_cliff/cliff_face_4x3` | `terrain` | 12 structural parts |
| `rock_cliff/cliff_corner_inner_2x2x2` | `terrain` | 8 structural parts |
| `rock_cliff/cliff_corner_outer_2x2x2` | `terrain` | 8 structural parts |
| `rock_cliff/cliff_cap_walkable_4x2` | `terrain` | 4 structural parts + 2 walkable caps |
| `rock_cliff/natural_stone_steps_2x3x1p5` | `stairs` | 6 walkable treads |
| `rock_cliff/cave_mouth_arch_4x3p5` | `structure` | 9 structural parts around an open mouth |
| `rock_cliff/scree_pile_3x2` | `rock` | None |
| `rock_cliff/rubble_cluster_2x1p5` | `rock` | None |
| `rock_cliff/terrain_transition_left_4x2x2` | `terrain` | 4 structural parts |
| `rock_cliff/terrain_transition_right_4x2x2` | `terrain` | 4 structural parts |
| `rock_cliff/overlook_ledge_4x2` | `terrain` | 3 structural parts + 2 walkable caps |

Collision is intentionally limited to aggregate bounds or authored compound
AABBs. No asset requests triangle-mesh or convex collision. Scree and rubble
are non-colliding dressing so dense placement does not create noisy blockers.
The cave arch uses separate left, right, and crown parts; its opening remains
physically clear.

Only the cliff cap, stone steps, and overlook expose walkable surfaces. Their
walkable parts follow the visible flat caps or treads instead of fabricating
one large aggregate floor over irregular rock.

## Orientation Rules

- Cliff faces run along local X, with their dressed face toward local `-Y`.
- The outer corner pivots at its junction and extends through local `+X` and
  `+Y`.
- The inner corner places its two walls at the far local `+X` and `+Y` edges,
  leaving the inside of the corner open.
- Stone steps ascend toward local `+Y`.
- The left transition rises toward local `+X`; the right transition falls
  toward local `+X`. Pair them with cliff faces to enter and leave a ridge.
- The cliff cap and overlook are the only intentionally walkable cliff-top
  modules. Their flat surfaces project toward local `-Y` when attached to a
  face.
- The cave mouth faces local `-Y`. Keep its central opening free of overlapping
  collision assets when assembling a passable cave entrance.
- Rotate modules around Z to change heading. Scaling changes render geometry
  and authored bounds together, but large non-uniform scale should be treated
  as a deliberate map-authoring choice rather than terrain deformation.

## Regeneration

Run Blender 4.5 LTS or newer. `--factory-startup` prevents user-installed
addons from affecting background export:

```sh
blender --background --factory-startup \
  --python tools/blender/build_rock_cliff_kit.py -- \
  --output-root assets/creative/rock_cliff \
  --preview build/artifacts/rock_cliff_kit_gallery.png \
  --cliff-preview build/artifacts/rock_cliff_kit_assembly.png \
  --blend build/artifacts/rock_cliff_kit_gallery.blend
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

- Triangle-mesh or convex collision cooking, only after the physics runtime has
  a bounded production contract for those modes.
- Climbing, mantling, ledge traversal, and explicit cave-interior semantics.
- Terrain blending, conforming cliff bases, erosion masks, and material
  variants for biome or weather changes.
- Authored LODs, occlusion groups, or merged instance clusters only after
  map-scale profiling identifies a measurable bottleneck.
