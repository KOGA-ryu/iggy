# File Spec

Files: `src/app/iggy3d/input/InteractionMode.hpp`, `src/app/iggy3d/input/InteractionMode.cpp`

Verified at: `5da10b9d`

## Owns

- Product interaction mode enum and input-surface enum.
- Stable string names for interaction modes and input surfaces.
- Pure controller chord toggle policy through `applyProductInteractionModeToggle(...)`.
- Allowed-surface policy for Player and Creative mode toggles.

## Does Not Own

- Frontend active-surface resolution, gamepad polling, frame-state mutation, controller action routing, creative launch/open behavior, or runtime player/creative systems.

## Reads

- Toggle request packet: current mode, input surface, chord sample, and prior chord state.
- Static policy tables for names and allowed toggle surfaces.

## Writes / Mutates

- Returns `ProductInteractionModeToggleResult`.
- Does not mutate app/window state directly.

## Calls Out To / Wires Out To

- No external calls beyond table lookup and local helpers.
- Consumed by `InteractionModeState.*` to apply per-frame state changes.

## Called By / Entry Points

- `productInteractionModeName(...)` and `productInputSurfaceName(...)` are used by receipts, routing proof, and debug surfaces.
- `applyProductInteractionModeToggle(...)` is used by `applyProductInteractionModeFrameToggle(...)` and unit tests.
- Grep proof: `rg -n "ProductInteractionMode|ProductInputSurface|applyProductInteractionModeToggle|productInputSurfaceAllowsInteractionModeToggle|toggledProductInteractionMode" src/app/iggy3d tests/unit`.

## Invariants

- Only Gameplay and RoomEditor surfaces allow controller chord toggling.
- Held chord does not retrigger a toggle.
- Partial chord reports partial status and preserves mode.
- Surface-blocked status must preserve mode.
- String names are receipt-facing proof values and must stay stable unless tests and receipts are updated.

## Tests / Proof Commands

- `rg -n "product_interaction_mode_tests|product_interaction_mode_state_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "interaction_mode_toggled|interaction_mode_chord_held|interaction_mode_surface_blocked|productInteractionModeName|productInputSurfaceName" tests/unit/product_interaction_mode_tests.cpp tests/unit/product_interaction_mode_state_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/input/InteractionModeState.*` unless frame adapter behavior changes.
- `src/app/iggy3d/menu/FrontendRouter.*` unless active-surface mapping changes.
- `src/app/iggy3d/input/ControllerActionRouting.*` unless controller action mapping consumes new mode/surface values.

## Update When

- Interaction mode values, surface values, names, toggle policy, chord policy, or allowed-surface rules change.

## Do Not Update When

- Only code that calls the existing toggle policy changes.
