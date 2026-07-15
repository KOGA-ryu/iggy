# Homestead Yard Kit

## Purpose

The yard kit extends the homestead set beyond the building shell with reusable
boundary, work, storage, and landmark props. Every asset uses meters, a
floor-aligned source origin, the established homestead material palette, and a
stable asset ID below `homestead/yard/`.

The editable source of truth is
`tools/blender/build_homestead_yard_kit.py`. Checked-in GLBs are runtime
products. The script also renders an isolated gallery, an assembled-yard proof,
and an optional inspection `.blend` file.

## Assets

| Asset ID | Approximate size | Collision | Sockets |
|---|---:|---|---|
| `homestead/yard/fence_2m` | 2.2 x 0.2 x 1.4 m | Bounds | None |
| `homestead/yard/fence_4m` | 4.2 x 0.2 x 1.4 m | Bounds | None |
| `homestead/yard/fence_corner_2x2` | 2.2 x 2.2 x 1.4 m | 7 compound bounds | None |
| `homestead/yard/gate_frame_1p8` | 2.0 x 0.3 x 2.1 m | 3 compound bounds | `yard_gate` receiver |
| `homestead/yard/gate_leaf_1p4` | 1.4 x 0.3 x 1.3 m | Bounds | `yard_gate_leaf` plug |
| `homestead/yard/well_roofed_1p8` | 2.1 x 2.1 x 2.7 m | Bounds | None |
| `homestead/yard/handcart_2p4` | 2.4 x 1.2 x 1.2 m | Bounds | None |
| `homestead/yard/signpost_2p2` | 1.3 x 0.2 x 2.3 m | Bounds | `lantern_hook` receiver |
| `homestead/yard/woodpile_1p5` | 1.7 x 1.3 x 0.9 m | Bounds | None |
| `homestead/yard/trough_1p6` | 1.7 x 0.8 x 0.9 m | Bounds | None |
| `homestead/yard/hay_bale_1p0` | 1.1 x 0.7 x 0.7 m | Bounds | None |

All entries use the authored catalog category `prop`. Aggregate bounds are
intentional for closed props. The corner uses one authored collision box per
post and rail so its empty interior remains available. The gate frame uses
three authored boxes, leaving the opening traversable whenever the leaf is not
attached. The leaf's bounds close that opening when present.

## Assembly Rules

- Fence origins are centered on the section. Rotate sections around Z in
  quarter turns; use the dedicated corner where two runs meet.
- The gate frame owns a `yard.gate` receiver and the gate leaf owns the matching
  plug. Snapping aligns the leaf with the opening and preserves the attachment
  through parent transforms, duplicate, clipboard, save/load, and undo.
- The gate is static in this kit. Opening, closing, hinges, latches, and
  navigation updates belong to a future interactive-object component rather
  than mesh naming conventions.
- The signpost's `decor.surface` receiver accepts the lantern from the homestead
  interior kit. This deliberately proves that compatibility families work
  across asset folders.
- The well, cart, trough, woodpile, and hay bale are static scene props. They do
  not imply water, inventory, animal feeding, harvesting, or vehicle behavior.
- Keep the well's aggregate collision in mind when placing it near narrow
  routes; precise ring collision is deferred until playtesting proves it useful.

## Regeneration

Run Blender 4.5 LTS or newer. `--factory-startup` prevents user-installed
addons from affecting background export:

```sh
blender --background --factory-startup \
  --python tools/blender/build_homestead_yard_kit.py -- \
  --output-root assets/creative/homestead/yard \
  --preview build/artifacts/homestead_yard_kit_gallery.png \
  --yard-preview build/artifacts/homestead_yard_kit_assembly.png \
  --blend build/artifacts/homestead_yard_kit_gallery.blend
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
finite non-degenerate bounds, collision-part count, socket role/count, catalog
inclusion, and preview-atlas inclusion. The placement and RoomBake targets
protect the shared imported-asset path used by preview, commit, and collision.

## Deferred Extensions

- Hinged and latched gate behavior with explicit state and navigation updates.
- Water surfaces, well interaction, trough contents, and prop inventory.
- Fence end caps, diagonal sections, damaged variants, and terrain-following
  placement rules.
- Precise compound collision for the well and handcart if aggregate bounds
  cause measurable route or interaction problems.
- Texture atlases, weathering variants, LODs, and material overrides.
