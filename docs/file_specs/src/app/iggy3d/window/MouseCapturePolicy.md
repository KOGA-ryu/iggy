# File Spec

Files: `src/app/iggy3d/window/MouseCapturePolicy.hpp`, `src/app/iggy3d/window/MouseCapturePolicy.cpp`, `src/app/iggy3d/window/MouseCaptureState.hpp`

Verified at: `b38ddff1`

## Owns

- Product mouse-capture decision policy and product mouse-capture state packet.
- Relative capture request/status/reason/mode decision for gameplay, frontend blockers, input ownership, focus, no-window mode, CreativeDocument editor pointer, and Creative Navigate mouse-look.

## Does Not Own

- SDL relative mouse API calls, SDL focus polling, active-surface resolution, creative tool selection, gameplay camera application, or receipt field writing.

## Reads

- `ProductMouseCapturePolicyRequest`: gameplay active, interaction mode, input owner, frontend gameplay block, window focus, capture support, creative document active, and creative navigate active.
- `MenuOwner` and `ProductInteractionMode`.

## Writes / Mutates

- Returns `ProductMouseCapturePolicy`.
- `ProductMouseCaptureState` is the window-state packet written by callers.
- Does not mutate SDL, product window state, frontend state, or creative state directly.

## Calls Out To / Wires Out To

- `menuOwnerName(...)` for input-owner proof strings.
- Called by window input frame and no-window loop policy recording; platform capture is applied outside this file.

## Called By / Entry Points

- `buildProductMouseCapturePolicy(...)`.
- Called from `InputFrame.cpp` and `Loop.cpp`.
- Grep proof: `rg -n "buildProductMouseCapturePolicy|ProductMouseCapturePolicy|ProductMouseCaptureState" src/app/iggy3d tests/unit`.

## Invariants

- Unsupported window capture returns no-window mode and does not request capture.
- Gameplay inactive, frontend blocked, non-gameplay input owner, non-player/non-creative mode, or unfocused window fail closed to released/no-capture.
- CreativeDocument editor defaults to free cursor for UI/pick.
- Creative Navigate tool re-enables relative capture for mouse-look.
- Policy building is pure; platform active/requested state is recorded by callers after SDL apply.

## Tests / Proof Commands

- `rg -n "product_mouse_capture_policy_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "mouse_capture_creative_editor_pointer|mouse_capture_creative_navigate_mouselook|mouse_capture_gameplay_mouselook|buildProductMouseCapturePolicy" tests/unit/product_mouse_capture_policy_tests.cpp src/app/iggy3d/window`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/window/InputFrame.*` unless policy input facts or platform application change.
- `src/app/iggy3d/window/Loop.*` unless no-window policy recording changes.
- `src/app/iggy3d/menu/FrontendRouter.*` unless active surface/input owner semantics change.

## Update When

- Mouse-capture policy inputs, reason/status strings, CreativeDocument pointer/capture behavior, Navigate capture behavior, or state packet fields change.

## Do Not Update When

- Only camera motion, creative tool command internals, SDL event polling, or receipt formatting changes without altering capture policy.
