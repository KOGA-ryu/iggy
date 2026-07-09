# File Spec

Files: `src/app/iggy3d/menu/DrawList.hpp`, `src/app/iggy3d/menu/DrawList.cpp`

Verified at: `6c79462b`

## Owns

- Product UI draw-list primitives, tones, theme structs, virtual rects, and hit-region carrier types.
- Starter/new-world/load-save/delete-confirm/settings/dev-tools draw-list construction.
- Starter UI request factory `buildProductStarterUiDrawListRequest(...)`.
- Delete-confirm model resolution from save catalog entries.
- Semantic IDs and action tags emitted for product menu UI rows and buttons.

## Does Not Own

- Pause menu draw-list layout, which lives in `PauseUi.*`.
- Creative UI draw-list layout, which lives under `src/app/iggy3d/creative/ui`.
- SDL/Vulkan rendering, font rasterization, or actual hit testing.
- Menu command execution or save catalog scanning.

## Reads

- `FrontendState`, compatible save counts, product save bridge result, optional world setup draft, and virtual dimensions.
- Starter screen model and frontend/save browser state.
- Save catalog entries for load/delete/confirm surfaces.

## Writes / Mutates

- Returns `ProductUiDrawList` packets containing primitives, hit regions, selected action, counts, status, and theme facts.
- No external state.

## Calls Out To / Wires Out To

- `buildStarterScreenModel(...)`.
- Widget helpers in `src/app/iggy3d/ui/Widget.*`.
- `resolveProductDeleteConfirmModel(...)` for confirm dialog copy used by draw and SDL paths.

## Called By / Entry Points

- `FramePresenter.cpp` converts draw lists into Vulkan frame input.
- `OpeningMenuHitTest.cpp` builds equivalent draw-list requests for hit regions.
- Product UI tests call draw-list builders directly.
- Grep proof: `rg -n "buildProductStarterUiDrawList|buildProductStarterUiDrawListRequest|resolveProductDeleteConfirmModel|UiHitRegion" src tests cmake`.

## Invariants

- Virtual dimensions and hit-region rects must match renderer and hit-test consumers.
- Semantic IDs and action tags are part of the input/routing contract.
- Delete confirm text is resolved through save catalog facts, not hard-coded guesses.
- Visual packet construction stays pure and does not mutate frontend/window/save state.

## Tests / Proof Commands

- `rg -n "product_ui_draw_list_tests|product_ui_hit_router_tests|product_vulkan_menu_frame_tests|product_window_renderer_lifecycle_tests" cmake tests`.
- `rg -n "buildProductStarterUiDrawListRequest|resolveProductDeleteConfirmModel" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/ActionHandlers.*` unless action tags or route meanings change.
- `src/app/iggy3d/view/OpeningMenuHitTest.*` unless hit-region consumption changes.
- `src/app/iggy3d/window/FramePresenter.*` unless draw-list consumption changes.

## Update When

- Product UI primitive types, tones, semantic IDs, hit-region contract, request payload, delete-confirm model, or starter/menu draw-list contents change.

## Do Not Update When

- Only menu handler behavior changes behind the same emitted actions.
- Only renderer implementation changes while consuming the same draw-list packet.
