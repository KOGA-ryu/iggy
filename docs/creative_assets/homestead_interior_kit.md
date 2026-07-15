# Homestead Interior Kit

## Purpose

The interior kit fills the homestead structural shell with reusable room-scale
props. Every asset uses meters, a floor-aligned source origin, the established
homestead material palette, and a stable asset ID below
`homestead/interior/`.

The editable source of truth is
`tools/blender/build_homestead_interior_kit.py`. Checked-in GLBs are runtime
products. The script also renders a gallery, a furnished-room proof, and an
optional inspection `.blend` file.

## Assets

| Asset ID | Approximate size | Collision | Sockets |
|---|---:|---|---|
| `homestead/interior/bed_single_2p1x1p1` | 2.1 x 1.1 x 1.2 m | Bounds | None |
| `homestead/interior/chair_ladderback_0p56` | 0.56 x 0.55 x 1.33 m | Bounds | None |
| `homestead/interior/bench_rustic_1p6` | 1.6 x 0.48 x 0.61 m | Bounds | None |
| `homestead/interior/bookshelf_1p1x2p1` | 1.18 x 0.46 x 2.11 m | Bounds | `top_center` receiver |
| `homestead/interior/dresser_1p3` | 1.34 x 0.62 x 1.11 m | Bounds | `top_center` receiver |
| `homestead/interior/storage_chest_1p1` | 1.12 x 0.70 x 0.80 m | Bounds | `top_center` receiver |
| `homestead/interior/hearth_stone_1p8` | 1.82 x 0.86 x 1.70 m | Bounds | `mantel_center` receiver |
| `homestead/interior/barrel_oak_0p8` | 0.78 x 0.78 x 0.87 m | Bounds | None |
| `homestead/interior/crate_wood_0p6` | 0.63 x 0.63 x 0.66 m | Bounds | stack plug + receiver |
| `homestead/interior/lantern_iron_0p7` | 0.38 x 0.38 x 0.67 m | None | `base` plug |

All entries use the authored catalog category `prop`. Bounds collision is a
deliberate coarse blocker for furniture: characters should not walk through a
bed, shelf, chest, or hearth, while small openings beneath legs do not need
navigation precision. The lantern has no collision so a decorative placement
cannot create an invisible movement snag.

## Placement Rules

- Place every prop on the floor from its source origin. Rotation and uniform or
  non-uniform scale use the normal Creative transform path.
- Keep shelves, dressers, and hearths close to a wall, but do not bury their
  rear face in the wall collision volume.
- The lantern's `decor.surface` plug snaps to the matching receiver on the
  bookshelf, dresser, storage chest, or hearth. The accepted lantern becomes an
  attached child and follows parent transform, duplicate, clipboard, save/load,
  and undo.
- The crate's bottom plug and top receiver share the same compatibility family,
  allowing deterministic vertical crate stacks. One receiver accepts one child.
- The lantern's warm material is visual only; it does not create a runtime
  point light in this batch.
- The chest, dresser, and fireplace are static props. Openable storage,
  drawers, fire state, and loot semantics require explicit gameplay components
  later rather than hidden behavior in the mesh.

## Regeneration

Run Blender 4.5 LTS or newer. `--factory-startup` prevents user-installed
addons from affecting background export:

```sh
blender --background --factory-startup \
  --python tools/blender/build_homestead_interior_kit.py -- \
  --output-root assets/creative/homestead/interior \
  --preview build/artifacts/homestead_interior_kit_gallery.png \
  --room-preview build/artifacts/homestead_interior_kit_room.png \
  --blend build/artifacts/homestead_interior_kit_gallery.blend
```

The Linux rendering worker can run the same command in a temporary checkout;
only the GLBs return to `assets/creative/`. Preview PNGs and the inspection
`.blend` stay under the ignored `build/artifacts/` directory.

Two clean Blender runs must produce byte-identical GLBs. Creative uses each
complete GLB byte hash as its content revision, so nondeterministic export would
cause false asset refreshes.

## Verification

```sh
cmake --build build --target static_mesh_asset_tests \
  creative_asset_room_bake_tests creative_editor_placement_tests
ctest --test-dir build \
  -R '^(static_mesh_asset_tests|creative_asset_room_bake_tests|creative_editor_placement_tests)$' \
  --output-on-failure
```

`static_mesh_asset_tests` pins every stable ID, category, collision mode,
finite non-degenerate bounds, socket role/count, catalog inclusion, and preview
atlas inclusion. The placement and RoomBake targets protect the shared imported
asset path used by Creative preview, commit, and collision baking.

## Deferred Extensions

- A small decor family that can use the furniture receiver sockets: candle,
  crockery, books, flowers, and tabletop tools.
- Interactive chest, drawer, door, and fire state components.
- Material variants, texture atlases, LODs, and weathering.
- Wall-mounted shelves, paintings, curtains, rugs, and room dividers.
- Compound collision only where playtesting proves aggregate bounds too coarse.
