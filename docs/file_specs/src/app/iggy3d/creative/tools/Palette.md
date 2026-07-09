# File Spec

Files: `src/app/iggy3d/creative/tools/Palette.hpp`, `src/app/iggy3d/creative/tools/Palette.cpp`

Verified at: `ff909047`

## Owns

- Creative authoring palette view derived from runtime object asset catalog data.
- Palette group labels, slot packets, status/reason strings, group counters, and disabled count.
- Mapping asset ids and editor palette groups into creative palette groups.
- Slot lookup by asset id.

## Does Not Own

- Object asset catalog storage or validation implementation.
- Creative document object descriptors.
- UI row drawing, hit testing, or command dispatch.
- Object placement or mutation execution.
- Save/load persistence of palette choices.

## Reads

- `ObjectAssetCatalog` and `ObjectAssetDefinition` rows.
- Asset editor metadata, primitive default size, placeable/rotatable/scalable flags, and collision traits.
- `validateObjectAssetDefinition(...)`.
- Built-in object asset catalog for the default palette.

## Writes / Mutates

- Builds a `PalView` with copied slot strings and counters.
- Does not mutate the catalog or asset definitions.

## Calls Out To / Wires Out To

- Calls `makeBuiltInObjectAssetCatalog()` for `buildPalView()`.
- Calls runtime object asset validation for slot enabled/reason state.
- Used by product creative UI command/catalog surfaces to expose create palette choices.

## Called By / Entry Points

- `groupName(...)`.
- `buildPalViewFromCatalog(...)`.
- `buildPalView()`.
- `findSlot(...)`.
- Grep proof: `rg -n "buildPalView|findSlot|groupName|PalView" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Palette build succeeds with explicit `palette_ready` status after scanning the catalog.
- Disabled slots remain in the slot list and increment `disabledCount`.
- Unknown groups keep slots disabled with `unknown_group`.
- Slot indexes are per group count, not global row index.
- `findSlot(...)` returns the first matching asset id or `nullptr`.

## Tests / Proof Commands

- `product_creative_palette_tests`.
- `product_creative_ui_command_frame_tests`.
- `product_creative_ui_projection_tests`.
- `rg -n "product_creative_palette_tests|buildPalViewFromCatalog|findSlot" cmake/iggy3d_tests.cmake tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/runtime/object/ObjectTraits.*` unless asset catalog schema or validation changes.
- `src/app/iggy3d/creative/bridge/UiCommandCatalog.*` unless palette consumption changes.
- `src/app/iggy3d/creative/tools/Placement.*` unless create placement policy changes.
- `src/app/iggy3d/creative/document/ObjectDescriptor.*` unless creative descriptor policy replaces asset catalog grouping.

## Update When

- Palette group mapping, slot fields, enabled/reason policy, catalog source, or slot lookup semantics change.

## Do Not Update When

- Only UI layout, create command execution, or object descriptor defaults change without changing palette view construction.
