# File Spec

Files: `src/app/iggy3d/debug/TopDownMapOverlay.hpp`, `src/app/iggy3d/debug/TopDownMapOverlay.cpp`, `src/app/iggy3d/debug/TopDownMapState.hpp`

Verified at: `e4855124`

## Owns

- Top-down map overlay packet, request, and product window-state mirror.
- Overlay visibility/status/purpose/size decision from renderer request, interaction mode, gameplay-active, room-editing-ready, creative-world-active, and item count facts.
- Item count carry-through for receipt/render consumers.

## Does Not Own

- Scene item generation, primitive drawing, or map geometry.
- Runtime/player/editor state.
- Vulkan/SDL presentation layout beyond the packet consumed elsewhere.

## Reads

- `TopDownMapOverlayRequest`.
- Renderer request, interaction mode, gameplay active state, room editor readiness, creative world active flag, and item count.

## Writes / Mutates

- Returns `TopDownMapOverlay`; no external state.
- `ProductTopDownMapState` defines the window-state mirror populated elsewhere.

## Calls Out To / Wires Out To

- Output is copied by projection refresh and consumed by scene primitive view, opening menu view, Vulkan frame presenter, and receipt fields.

## Called By / Entry Points

- `ProjectionRefresh.cpp` builds/copies top-down overlay facts.
- `ScenePrimitiveView.cpp` and `FramePresenter.cpp` consume it for draw/presentation.
- Grep proof: `rg -n "buildTopDownMapOverlay|TopDownMapOverlay|ProductTopDownMapState|appendTopDownMapOverlayUi|drawScenePrimitives" src tests cmake`.

## Invariants

- Overlay visibility depends on product surface/mode gates, not runtime map data alone.
- Status/reason must explain hidden versus visible overlay states.
- Creative world and room editor paths remain explicit in the request.
- This file returns facts only and does not draw or mutate state.

## Tests / Proof Commands

- `rg -n "product_top_down_map_overlay_tests|product_vulkan_room_frame_tests" cmake tests`.
- `rg -n "buildTopDownMapOverlay|copyTopDownMapOverlay" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless request/copy behavior changes.
- `src/app/iggy3d/view/ScenePrimitiveView.*` unless presentation changes.
- `src/app/iggy3d/window/FramePresenter.*` unless Vulkan overlay consumption changes.

## Update When

- Overlay request/fields, visibility policy, status/purpose/size mapping, or state mirror changes.

## Do Not Update When

- Scene primitive generation changes without changing overlay packet semantics.
