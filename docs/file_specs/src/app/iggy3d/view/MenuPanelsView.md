# File Spec

Files: `src/app/iggy3d/view/MenuPanelsView.hpp`, `src/app/iggy3d/view/MenuPanelsView.cpp`

Verified at: `2b846f78`

## Owns

- SDL panel drawing helpers for starter details, new-world/map-builder, load-save/delete mode, delete confirmation, dev tools, settings, and gameplay movement tuning.
- Shared menu row drawing used by the opening menu SDL path.
- String formatting for movement tuning values, slider bars, selected dungeon draft glyph labels, and short ASCII room previews.

## Does Not Own

- Frontend route/action state.
- Save/delete execution or save browser model construction.
- Dungeon draft editing behavior.
- Settings mutation.
- Hit testing.
- Top-level SDL render pass/present.

## Reads

- `ProductWorldTemplate`, `ProductSaveBridgeResult`, `WorldSetupDraft`, dungeon draft proof fields, `ProductDeleteConfirmModel`, selected settings/dev-tools categories, and movement tuning descriptors.
- Builtin dungeon catalog and settings/dev-tools tab order descriptors.

## Writes / Mutates

- Draws to an `SDL_Renderer` using `SdlDraw.*`.
- Does not mutate frontend, save, settings, world setup, or gameplay state.

## Calls Out To / Wires Out To

- `setColor(...)`, `fillRect(...)`, and `drawText(...)`.
- `productBuiltinDungeonCatalog(...)`, `productBuiltinDungeonIndexForRoomId(...)`, and `settingsTabOrder(...)`.
- `devToolsCategoryOrder(...)`, `devToolsCategoryLabel(...)`, and `devToolsFunctionKeyHintLabel(...)`.
- `kProductGameplayMovementTuningFields` and movement tuning descriptors.

## Called By / Entry Points

- `OpeningMenuView.cpp` calls panel helpers from the SDL menu/gameplay draw path.
- Focused proof: `rg -n "drawNewWorldPanel|drawLoadSavePanel|drawDeleteConfirmPanel|drawSettingsPanel|drawGameplayMovementTuningHud" src/app tests`.

## Invariants

- This file draws panels only; it must not dispatch commands or mutate app state.
- Load-save panel label and footer change based on save browser mode.
- Delete confirmation panel uses the caller-provided `ProductDeleteConfirmModel`, not a static candidate string.
- New-world panel reflects draft state, builtin selection, dungeon draft cursor/tool, and ASCII preview text supplied by callers.
- Movement tuning rows are descriptor-driven from the tuning field table.

## Tests / Proof Commands

- `rg -n "drawNewWorldPanel|drawLoadSavePanel|drawDeleteConfirmPanel|drawSettingsPanel" src/app/iggy3d/view src/app/iggy3d/window tests/unit`.
- `rg -n "product_window_renderer_lifecycle_tests|product_menu_usefulness_smoke|product_window_input_frame_tests" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/ActionHandlers.*` unless panel action semantics change.
- `src/app/iggy3d/save/*` unless save browser or delete-confirm model fields change.
- `src/app/iggy3d/world/DungeonDraft.*` unless draft display fields change.
- `src/app/iggy3d/view/OpeningMenuHitTest.*` unless hit geometry changes to match panel layout.

## Update When

- SDL panel layouts, panel data inputs, movement tuning panel rows, delete-confirm text source, or load-save/new-world panel behavior changes.

## Do Not Update When

- Only the underlying save, settings, dungeon draft, or route behavior changes without changing what these panels draw.
