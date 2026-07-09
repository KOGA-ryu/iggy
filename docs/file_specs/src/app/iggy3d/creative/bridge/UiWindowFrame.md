# File Spec

Files: `src/app/iggy3d/creative/bridge/UiWindowFrame.hpp`, `src/app/iggy3d/creative/bridge/UiWindowFrame.cpp`

Verified at: `d40a476b`

## Owns

- Window-to-creative-UI frame adapter.
- Logical/drawable/fallback coordinate-space selection before creative UI frame build.
- Creative overlay input availability gate from draw-list readiness, renderer capability, drawable availability, and gameplay overlay renderability.

## Does Not Own

- Creative UI projection internals.
- Hit testing, command execution, document mutation, or renderer submission.
- Low-level SDL/window polling.

## Reads

- Window/creative pointers, drawable dimensions, logical dimensions, fallback dimensions, theme, and overlay availability inputs.
- Draw-list readiness for input-availability decisions.

## Writes / Mutates

- No external state directly.
- Returns `ProductCreativeUiFrame` and `ProductCreativeUiOverlayInputAvailability` packets.

## Calls Out To / Wires Out To

- `resolveProductCreativeWindowCoordinateSpace(...)`.
- `buildProductCreativeUiFrame(...)`.

## Called By / Entry Points

- `window/Loop.cpp` builds creative UI window frames for the live frame loop.
- Overlay tests call the window adapter and input-availability gate directly.
- Grep proof: `rg -n "buildProductCreativeUiWindowFrame|resolveProductCreativeUiOverlayInputAvailability|ProductCreativeUiOverlayInputAvailability" src tests cmake`.

## Invariants

- Virtual UI size is derived from logical/drawable/fallback coordinate space before projection.
- Overlay input is unavailable unless draw list exists, is ready, renderer can render it, the window is drawable, drawable size is nonzero, and gameplay overlay is renderable.
- This file adapts sizing/gates only; it does not mutate creative state.

## Tests / Proof Commands

- `rg -n "product_creative_ui_window_frame_tests|product_vulkan_creative_ui_overlay_tests|product_window_input_frame_tests" cmake tests`.
- `rg -n "product_creative_ui_overlay_input_available|product_creative_ui_overlay_input_renderer_unavailable" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/bridge/WindowCoordinateSpace.*` unless coordinate resolution changes.
- `src/app/iggy3d/creative/ui/UiFrame.*` unless frame projection request shape changes.
- `src/app/iggy3d/window/Loop.cpp` unless live frame ordering changes.

## Update When

- Window frame request fields, coordinate-space wiring, overlay input availability rules, or creative UI frame adapter behavior changes.

## Do Not Update When

- Only semantic command handling or document mutation changes after input is available.
