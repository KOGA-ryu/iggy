# File Spec

Files: `src/app/iggy3d/menu/PauseUi.hpp`, `src/app/iggy3d/menu/PauseUi.cpp`

Verified at: `6c79462b`

## Owns

- Pause menu action labels for product UI rows.
- Journal-themed pause menu draw-list construction.
- Pause UI hit regions and semantic IDs produced through widget helpers.
- Missing-model failure packet for pause UI draw-list generation.

## Does Not Own

- Pause menu model construction and action enablement.
- Pause command execution.
- Starter/load/save/delete/settings/dev-tools draw lists.
- Renderer presentation or hit-test routing.

## Reads

- `ProductPauseUiRequest`.
- `PauseMenuModel` rows and selected action.
- Frontend action names for semantic IDs and fallback labels.

## Writes / Mutates

- Returns a `ProductUiDrawList` packet.
- No external state.

## Calls Out To / Wires Out To

- Widget emit helpers in `src/app/iggy3d/ui/Widget.*`.
- Shared product UI primitive/tone/theme/hit-region types from `DrawList.*`.

## Called By / Entry Points

- `FramePresenter.cpp` appends pause overlay draw lists.
- `OpeningMenuHitTest.cpp` uses pause draw-list hit regions.
- Tests call `buildProductPauseUiDrawList(...)` directly.
- Grep proof: `rg -n "buildProductPauseUiDrawList|productPauseActionLabel" src tests cmake`.

## Invariants

- Missing model returns `ready=false` with pause-specific reason/status.
- Pause theme remains `Journal`.
- Row hit regions carry frontend actions so hit-test and menu dispatch can agree.
- This file stays presentation-only and does not mutate frontend/window state.

## Tests / Proof Commands

- `rg -n "product_pause_ui_draw_list_tests|product_vulkan_pause_overlay_tests|product_window_input_frame_tests" cmake tests`.
- `rg -n "buildProductPauseUiDrawList|pause.row" src tests`.

## Nearby Files Usually Not Touched

- `src/app/frontend/PauseMenu.*` unless model shape changes.
- `src/app/iggy3d/menu/ActionHandlers.*` unless pause actions change meaning.
- `src/app/iggy3d/window/FramePresenter.*` unless pause overlay consumption changes.

## Update When

- Pause labels, theme, hit-region IDs, request/model consumption, or draw-list packet behavior changes.

## Do Not Update When

- Only pause action execution changes behind the same row actions.
