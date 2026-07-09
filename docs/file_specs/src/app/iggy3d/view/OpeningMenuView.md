# File Spec

Files: `src/app/iggy3d/view/OpeningMenuView.hpp`, `src/app/iggy3d/view/OpeningMenuView.cpp`

Verified at: `d5abfcf8`

## Owns

- SDL draw bridge for product opening menu, pause menu, settings/dev-tools/load-save/delete-confirm/new-world detail panels, and gameplay panel fallback when frontend screen is gameplay.
- `OpeningMenuViewState` proof fields for menu text, selected row, camera heading, and row count.
- Manual drawing composition that combines frontend state, save state, world setup draft state, gameplay debug HUDs, primitive frame, and camera heading into the SDL renderer.

## Does Not Own

- Frontend route/action decisions.
- Menu draw-list model generation.
- Hit testing or input routing.
- Save/delete execution.
- Gameplay simulation, camera controller mutation, or primitive projection.
- Vulkan rendering backend resources.

## Reads

- `ProductAppOptions`, `ProductWorldTemplate`, `FrontendState`, selected settings tab, movement tuning, world setup draft, dungeon draft proof fields, gameplay active flag, runtime state hash, viewport frame, gameplay feedback, debug HUD packets, debug projection, camera yaw/pitch, save bridge result, and delete candidate id.
- Active-surface resolution from the frontend router.

## Writes / Mutates

- Draws to an `SDL_Renderer` and calls `SDL_RenderPresent(...)`.
- Returns `OpeningMenuViewState`.
- Does not mutate frontend, window, save, gameplay, or runtime state.

## Calls Out To / Wires Out To

- SDL draw helpers in `SdlDraw.*`.
- Panel draw helpers in `MenuPanelsView.*`.
- Primitive scene draw helpers in `ScenePrimitiveView.*`.
- HUD draw helpers in `DebugHudView.*`.
- `openingMenuActionOrderForFrontend(...)`, `openingMenuUsesPauseRows(...)`, `resolveProductActiveSurface(...)`, and `resolveProductDeleteConfirmModel(...)`.
- Option display helpers from `Options.*`.

## Called By / Entry Points

- `FramePresenter.cpp` calls `drawOpeningMenuView(...)` when presenting through the SDL path.
- Focused proof: `rg -n "drawOpeningMenuView|OpeningMenuViewState|menuTextDrawn|opening_menu_text_ready" src/app tests`.

## Invariants

- Gameplay screen uses the gameplay panel path and returns after presenting it.
- Non-gameplay screens draw main action rows from `openingMenuActionOrderForFrontend(...)`.
- Continue is disabled on starter rows when no compatible save exists; pause rows use pause-row policy.
- Detail panel selection follows child screen or active surface, not ad hoc row indexes.
- This file draws; it must not execute frontend actions or mutate save/session state.

## Tests / Proof Commands

- `rg -n "drawOpeningMenuView|OpeningMenuViewState" src/app tests/unit`.
- `rg -n "product_window_renderer_lifecycle_tests|product_window_input_frame_tests|product_menu_usefulness_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "opening_menu_visible|opening_menu_text_ready|menuTextDrawn" src/app/iggy3d/receipt src/app/iggy3d/window tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/ActionHandlers.*` unless route/action semantics change.
- `src/app/iggy3d/menu/InputRouter.*` unless input routing changes.
- `src/app/iggy3d/view/OpeningMenuHitTest.*` unless hit-region geometry changes.
- `src/app/iggy3d/window/FramePresenter.*` unless the presentation request changes.
- `src/render/*` unless backend rendering contracts change.

## Update When

- SDL menu/gameplay panel draw contract, inputs to `drawOpeningMenuView(...)`, proof fields, detail-panel routing, or renderer presentation behavior changes.

## Do Not Update When

- Only frontend route semantics, hit tests, runtime gameplay, or Vulkan backend internals change without changing this draw bridge.
