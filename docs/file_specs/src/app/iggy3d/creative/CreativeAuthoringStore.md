# File Spec

File: `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`

Verified at: `cd6c9663`

## Owns

- Product-window authoring mirror packet for world setup, ASCII room authoring, legacy room editor, CreativeDocument UI, viewport pick, baked-room, and wireframe diagnostics.
- Aggregation of receipt/status fields that still live under `ProductAppWindowState::creativeAuthoring`.
- App-facing mirrors used by receipts and no-window tests.

## Does Not Own

- Authoritative creative facade/document state.
- Runtime simulation or save serialization.
- UI command execution.
- Room editor algorithms.
- Creative viewport pick, wireframe, or bake implementation.

## Reads

- No data directly; this header declares a state aggregate populated by other systems.
- Includes authoring, room editor, world setup, creative UI, and diagnostic packet types.

## Writes / Mutates

- No functions here; callers mutate fields directly.
- Field writers include automation, menu action handlers, window input, creative UI frame/command/viewport/wireframe flows, receipt builders, and transition reset code.

## Calls Out To / Wires Out To

- Embedded in `ProductAppWindowState`.
- Read by receipt fields, frontend routing, tests, and product frame/window paths.
- Covered by god-struct ownership coverage to ensure the store remains an assigned ownership bucket.

## Called By / Entry Points

- Aggregate field access through `window.creativeAuthoring`.
- Grep proof: `rg -n "CreativeAuthoringStore|creativeAuthoring\\.|creativeViewportPick|creativeWireframe|creativeBakedRoomStale|creativeNavigateActive" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- This store is mirror/diagnostic app state, not source-of-truth runtime or document state.
- Status and reason-code defaults must be explicit "not requested" or "none" baselines.
- Creative navigation, UI, viewport pick, wireframe, and baked-room fields are off-save mirrors unless separately promoted.
- Do not move runtime kernels or product behavior into this aggregate.

## Tests / Proof Commands

- `product_god_struct_ownership_coverage_tests`.
- `product_creative_world_launch_tests`.
- `product_creative_ui_input_frame_tests`.
- `product_creative_wireframe_frame_tests`.
- `product_room_editor_action_controller_tests`.
- `rg -n "product_god_struct_ownership_coverage_tests|CreativeAuthoringStore|creativeAuthoring\\." cmake/iggy3d_tests.cmake tests/unit src/app/iggy3d`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ProductAppWindowState.hpp` unless the embedding or god-struct member changes.
- `src/app/iggy3d/receipt/*` unless receipt field mirrors change.
- `src/app/iggy3d/creative/CreativeAppState.hpp` unless authoritative creative app state changes.
- `src/app/iggy3d/menu/Transitions.*` unless reset baselines change.

## Update When

- Creative authoring mirror fields, defaults, embedded packet ownership, receipt mirror boundaries, or app-window authoring ownership changes.

## Do Not Update When

- Only underlying algorithms change without adding/removing/changing mirror fields or their ownership meaning.
