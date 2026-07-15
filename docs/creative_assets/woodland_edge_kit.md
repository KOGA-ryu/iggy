# Woodland Edge Kit

## Purpose

The Woodland Edge Kit provides solid low-poly vegetation and ground dressing
for framing homesteads, roads, rivers, and authored terrain. Every asset uses
meters, a floor-aligned source origin, deterministic geometry, and a stable
asset ID below `woodland/`.

The editable source of truth is
`tools/blender/build_woodland_edge_kit.py`. Checked-in GLBs are runtime
products. The script also renders an isolated gallery, a composed woodland-edge
proof, and an optional inspection `.blend` file.

## Assets

| Asset ID | Nominal size | Category | Collision |
|---|---:|---|---|
| `woodland/broadleaf_young_4p7m` | 4.7 m tall | `tree` | One trunk box |
| `woodland/broadleaf_mature_7p3m` | 7.3 m tall | `tree` | One trunk box |
| `woodland/broadleaf_ancient_9p5m` | 9.5 m tall | `tree` | One trunk box |
| `woodland/pine_medium_6p3m` | 6.3 m tall | `tree` | One trunk box |
| `woodland/pine_tall_9p3m` | 9.3 m tall | `tree` | One trunk box |
| `woodland/dead_tree_snag_6m` | 6.0 m tall | `tree` | One trunk box |
| `woodland/tree_stump_0p8m` | 0.8 m tall | `log` | Bounds |
| `woodland/fallen_branch_pile_2m` | 2.0 m wide | `log` | None |
| `woodland/shrub_low_1p2m` | 1.2 m wide | `shrub` | None |
| `woodland/shrub_dense_1p8m` | 1.8 m wide | `shrub` | None |
| `woodland/fern_cluster_1p2m` | 1.2 m wide | `foliage` | None |
| `woodland/reed_grass_clump_1p4m` | 1.4 m tall | `foliage` | None |
| `woodland/rock_cluster_small_1p5m` | 1.5 m wide | `rock` | None |

The six trees use compound collision with exactly one authored part: the
trunk. Branches and opaque foliage remain visual geometry, so a broad canopy
does not create an invisible wall. The stump is a deliberate low obstacle.
Shrubs, fern, reeds, fallen branches, and small rocks are non-colliding dressing
that can be placed densely without producing movement snags.

## Visual Contract

- Foliage is solid geometry. The current renderer deliberately has no alpha
  blend/mask material path, so the kit does not depend on billboard cards or
  hidden transparency assumptions.
- Broadleaf crowns use overlapping low-poly leaf masses with age-specific color
  variation. Pine crowns use nested solid conical tiers.
- Tree rotation around Z is encouraged. Non-uniform scale remains available,
  but large distortion changes the apparent trunk-to-canopy proportion.
- Floor-aligned origins keep placement predictable on authored terrain. These
  assets do not automatically conform, scatter, or tilt to a surface normal.
- Repeated copies use the existing imported-mesh instance path. Duplicating a
  tree changes transforms rather than expanding its mesh into room geometry.
- Nothing in the mesh names implies harvesting, wind, damage, growth, or AI
  cover. Those behaviors require explicit systems and authored data later.

## Regeneration

Run Blender 4.5 LTS or newer. `--factory-startup` prevents user-installed
addons from affecting background export:

```sh
blender --background --factory-startup \
  --python tools/blender/build_woodland_edge_kit.py -- \
  --output-root assets/creative/woodland \
  --preview build/artifacts/woodland_edge_kit_gallery.png \
  --woodland-preview build/artifacts/woodland_edge_kit_assembly.png \
  --blend build/artifacts/woodland_edge_kit_gallery.blend
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
finite non-degenerate bounds, collision-part count, catalog inclusion, and
preview-atlas inclusion. The placement and RoomBake targets protect the shared
imported-asset path used by preview, commit, and collision.

## Deferred Extensions

- Terrain-aware scatter, density masks, deterministic seeds, exclusion zones,
  and editor-side instance painting.
- LOD meshes, GPU-driven visibility, and distant impostors after profiling
  proves the current instance path insufficient.
- Alpha-masked leaf cards only after the material and sorting contracts exist.
- Wind animation, seasonal palettes, snow variants, damage, and harvesting.
- Larger boulder families and deliberate climbable rock collision.
