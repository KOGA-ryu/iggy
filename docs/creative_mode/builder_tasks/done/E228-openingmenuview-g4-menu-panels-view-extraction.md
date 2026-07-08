# E228 - OpeningMenuView G4: Menu Panels View Extraction

## Status

Done.

## Context

E224 preflighted the `OpeningMenuView.cpp` split. E225 extracted the SDL glyph
and draw helpers into `SdlDraw.*`. E226 extracted scene/viewport primitive
rendering into `ScenePrimitiveView.*`. E227 extracted camera/feedback/debug,
room-editor, and position HUD drawing into `DebugHudView.*`.

After E227, current sizes are:

- `src/app/iggy3d/view/OpeningMenuView.cpp`: 951 lines
- `src/app/iggy3d/view/DebugHudView.cpp`: 224 lines
- `src/app/iggy3d/view/ScenePrimitiveView.cpp`: 400 lines
- `src/app/iggy3d/view/SdlDraw.cpp`: 169 lines

The next safe domain slice is the menu/panel rendering cluster. This card
should move child panel drawing, menu-row drawing, ASCII preview drawing, and
movement-tuning panel formatting only. It must not move hit-test routing or the
top-level `drawOpeningMenuView(...)` facade.

## Objective

Extract the render-only menu/panel helper cluster from `OpeningMenuView.cpp`
into a new `MenuPanelsView` helper under `src/app/iggy3d/view/`, preserving
behavior.

## Implementation Scope

Edit only:

- `CMakeLists.txt`
- `src/app/iggy3d/view/OpeningMenuView.cpp`
- new `src/app/iggy3d/view/MenuPanelsView.hpp`
- new `src/app/iggy3d/view/MenuPanelsView.cpp`
- this task card

Add `src/app/iggy3d/view/MenuPanelsView.cpp` to the `iggy3d` library source
list near `OpeningMenuView.cpp`, `DebugHudView.cpp`, `ScenePrimitiveView.cpp`,
and `SdlDraw.cpp`.

## Move Scope

Move these current render helpers out of `OpeningMenuView.cpp`:

- `selectedDraftGlyphLabel(...)`
- `drawGameplayMovementTuningHud(...)`
- `drawMenuRow(...)`
- `settingsTabLabel(...)`
- `fixedFloat(...)`
- `sliderBar(...)`
- `drawMovementTuningRows(...)`
- `drawPanelRow(...)`
- `drawAsciiPreviewLines(...)`
- `drawStarterDetailPanel(...)`
- `drawNewWorldPanel(...)`
- `drawLoadSavePanel(...)`
- `drawDeleteConfirmPanel(...)`
- `drawDevToolsPanel(...)`
- `drawSettingsPanel(...)`

The new public header should expose only what `OpeningMenuView.cpp` needs:

- `void drawGameplayMovementTuningHud(SDL_Renderer&, const ProductGameplayMovementTuning&, ProductGameplayMovementTuningField, bool visible);`
- `void drawMenuRow(SDL_Renderer&, std::string_view label, bool selected, bool enabled, float x, float y);`
- `void drawStarterDetailPanel(SDL_Renderer&);`
- `void drawNewWorldPanel(SDL_Renderer&, const ProductWorldTemplate&, const ProductSaveBridgeResult&, const WorldSetupDraft&, bool dungeonDraftEditMode, bool dungeonDraftModified, std::uint64_t dungeonDraftCursorRow, std::uint64_t dungeonDraftCursorColumn, const std::string& dungeonDraftSelectedGlyph, const std::string& dungeonDraftLastGlyph);`
- `void drawLoadSavePanel(SDL_Renderer&, FrontendSaveBrowserMode, const ProductSaveBridgeResult&);`
- `void drawDeleteConfirmPanel(SDL_Renderer&, const ProductDeleteConfirmModel&);`
- `void drawDevToolsPanel(SDL_Renderer&, FrontendDevToolsCategory selected);`
- `void drawSettingsPanel(SDL_Renderer&, FrontendSettingsTab selected, const ProductGameplayMovementTuning&, ProductGameplayMovementTuningField);`

Keep these helper details file-local in `MenuPanelsView.cpp`:

- `selectedDraftGlyphLabel(...)`
- `settingsTabLabel(...)`
- `fixedFloat(...)`
- `sliderBar(...)`
- `drawMovementTuningRows(...)`
- `drawPanelRow(...)`
- `drawAsciiPreviewLines(...)`

## Explicit Deferral

Do **not** move these functions in this slice:

- `usesPauseMenuRows(...)`
- `menuActionOrderForFrontend(...)`
- `menuTitleForFrontend(...)`
- `roundedDegrees(...)`
- `drawGameplayPanel(...)`
- `uiRectContains(...)`
- `hitRegionActionAt(...)`
- `pauseMenuActionAt(...)`
- `openingMenuDetailSurfaceFor(...)`
- `openingMenuActionAt(...)`
- `drawOpeningMenuView(...)`

Reason: `menuActionOrderForFrontend(...)` is shared by both drawing and
hit-testing. Leave it in the facade file until the later hit-test split, so this
card remains a render extraction and does not move input routing policy.

## Required Behavior Preservation

Preserve exactly:

- movement tuning HUD geometry, row cap, toggle/value formatting, and visibility
  behavior
- menu row selected/enabled colors, marker, highlight geometry, and text scale
- settings tab labels and fallback label
- `fixedFloat(...)` formatting and `sliderBar(...)` segment math
- settings gameplay tuning rows and slider output
- panel row selected/default colors and geometry
- ASCII preview line cap, cursor bracket rendering, and cursor color behavior
- starter, new-world, load-save, delete-confirm, dev-tools, and settings panel
  geometry/text/order
- current call order inside `drawGameplayPanel(...)` and `drawOpeningMenuView(...)`

`OpeningMenuView.cpp` should include `app/iggy3d/view/MenuPanelsView.hpp` and
keep calling the moved helper names.

## Non-Scope

Do not move or edit behavior in:

- hit-test/action routing
- active-surface resolution
- pause-menu model or UI routing
- menu action order selection
- menu title selection
- gameplay panel orchestration
- top-level `drawOpeningMenuView(...)` facade behavior
- `OpeningMenuView.hpp`
- `SdlDraw.*`
- `ScenePrimitiveView.*`
- `DebugHudView.*`
- CMake test definitions
- receipt golden files

Do not create:

- `OpeningMenuHitTest`
- a shared formatting helper outside `MenuPanelsView`
- a new menu/panel model
- a new widget or hit-region routing layer

No staging, commit, push, broad CTest, or window launch.

## Required Greps

Run and report:

```sh
rg -n "selectedDraftGlyphLabel|drawGameplayMovementTuningHud|drawMenuRow|settingsTabLabel|fixedFloat|sliderBar|drawMovementTuningRows|drawPanelRow|drawAsciiPreviewLines|drawStarterDetailPanel|drawNewWorldPanel|drawLoadSavePanel|drawDeleteConfirmPanel|drawDevToolsPanel|drawSettingsPanel|menuActionOrderForFrontend|openingMenuActionAt|drawOpeningMenuView" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/MenuPanelsView.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/MenuPanelsView.cpp
rg -n "MenuPanelsView\\.cpp|DebugHudView\\.cpp|ScenePrimitiveView\\.cpp|SdlDraw\\.cpp|OpeningMenuView\\.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
git -C /Users/kogaryu/iggy3d diff --check
```

Expected classification:

- definitions for the moved menu/panel helpers live in `MenuPanelsView.cpp`
- `MenuPanelsView.hpp` exposes only the eight draw functions listed above
- `selectedDraftGlyphLabel(...)`, `settingsTabLabel(...)`, `fixedFloat(...)`,
  `sliderBar(...)`, `drawMovementTuningRows(...)`, `drawPanelRow(...)`, and
  `drawAsciiPreviewLines(...)` are file-local in `MenuPanelsView.cpp`
- `OpeningMenuView.cpp` retains call sites only for moved menu/panel helpers
- `menuActionOrderForFrontend(...)`, `openingMenuActionAt(...)`, and
  `drawOpeningMenuView(...)` remain in `OpeningMenuView.cpp`
- `CMakeLists.txt` includes `MenuPanelsView.cpp` near the other view split files

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_starter_menu_action_tests product_menu_transitions_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_starter_menu_action_tests|product_menu_transitions_tests|product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- the extraction requires changing any menu/panel geometry, colors, text,
  row order, visibility checks, or call order
- moving hit-test helpers becomes necessary
- moving `menuActionOrderForFrontend(...)` becomes necessary
- a shared formatting helper outside `MenuPanelsView` becomes necessary
- compile fallout expands outside `OpeningMenuView.cpp`, `MenuPanelsView.*`, and
  CMake
- any pause-menu routing, UI hit-region routing, receipt, scene primitive, HUD,
  SDL helper, or facade behavior needs to move with the panel helper
- receipt golden output changes

## Completion Brief

When done, report:

- files changed
- exact helper/API shape created
- what stayed in `OpeningMenuView.cpp`
- CMake source-list placement
- required grep classification
- focused build/CTest result
- receipt golden diff result
- diff/whitespace checks
- confirmation that hit-test/action routing, menu action order selection,
  gameplay panel orchestration, facade behavior, `OpeningMenuView.hpp`,
  `SdlDraw.*`, `ScenePrimitiveView.*`, `DebugHudView.*`, CMake test
  definitions, staging, commit, push, and window launch were not touched

## Completion Brief

- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/view/OpeningMenuView.cpp`
  - `src/app/iggy3d/view/MenuPanelsView.hpp`
  - `src/app/iggy3d/view/MenuPanelsView.cpp`
  - this task card
- Exact helper/API shape created:
  - Added `MenuPanelsView.hpp/.cpp` under `src/app/iggy3d/view/`.
  - `MenuPanelsView.hpp` is SDL3-gated and exposes only:
    - `drawGameplayMovementTuningHud(SDL_Renderer&, const ProductGameplayMovementTuning&, ProductGameplayMovementTuningField, bool)`
    - `drawMenuRow(SDL_Renderer&, std::string_view, bool, bool, float, float)`
    - `drawStarterDetailPanel(SDL_Renderer&)`
    - `drawNewWorldPanel(SDL_Renderer&, const ProductWorldTemplate&, const ProductSaveBridgeResult&, const WorldSetupDraft&, bool, bool, std::uint64_t, std::uint64_t, const std::string&, const std::string&)`
    - `drawLoadSavePanel(SDL_Renderer&, FrontendSaveBrowserMode, const ProductSaveBridgeResult&)`
    - `drawDeleteConfirmPanel(SDL_Renderer&, const ProductDeleteConfirmModel&)`
    - `drawDevToolsPanel(SDL_Renderer&, FrontendDevToolsCategory)`
    - `drawSettingsPanel(SDL_Renderer&, FrontendSettingsTab, const ProductGameplayMovementTuning&, ProductGameplayMovementTuningField)`
  - `MenuPanelsView.cpp` owns the moved implementations.
  - `selectedDraftGlyphLabel(...)`, `settingsTabLabel(...)`, `fixedFloat(...)`,
    `sliderBar(...)`, `drawMovementTuningRows(...)`, `drawPanelRow(...)`, and
    `drawAsciiPreviewLines(...)` are file-local in `MenuPanelsView.cpp`.
- What stayed in `OpeningMenuView.cpp`:
  - `usesPauseMenuRows(...)`
  - `menuActionOrderForFrontend(...)`
  - `menuTitleForFrontend(...)`
  - `roundedDegrees(...)`
  - `drawGameplayPanel(...)`
  - `uiRectContains(...)`
  - `hitRegionActionAt(...)`
  - `pauseMenuActionAt(...)`
  - `openingMenuDetailSurfaceFor(...)`
  - `openingMenuActionAt(...)`
  - `drawOpeningMenuView(...)`
- CMake source-list placement:
  - `OpeningMenuView.cpp` at line 87
  - `MenuPanelsView.cpp` at line 88
  - `DebugHudView.cpp` at line 89
  - `ScenePrimitiveView.cpp` at line 90
  - `SdlDraw.cpp` at line 91
- Required grep classification:
  - Definitions for the moved menu/panel helpers live in `MenuPanelsView.cpp`.
  - `MenuPanelsView.hpp` exposes only the eight draw functions listed above.
  - `selectedDraftGlyphLabel(...)`, `settingsTabLabel(...)`, `fixedFloat(...)`,
    `sliderBar(...)`, `drawMovementTuningRows(...)`, `drawPanelRow(...)`, and
    `drawAsciiPreviewLines(...)` are file-local in `MenuPanelsView.cpp`.
  - `OpeningMenuView.cpp` retains call sites only for moved menu/panel helpers.
  - `menuActionOrderForFrontend(...)`, `openingMenuActionAt(...)`, and
    `drawOpeningMenuView(...)` remain in `OpeningMenuView.cpp`.
  - `CMakeLists.txt` includes `MenuPanelsView.cpp` near the other view split
    files.
- Focused build result:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_starter_menu_action_tests product_menu_transitions_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10`
    passed.
- Focused CTest result:
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_starter_menu_action_tests|product_menu_transitions_tests|product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure`
    passed, 6/6 tests.
- Receipt golden diff result:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
    was empty.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card was
    clean.
- Confirmation:
  - Hit-test/action routing, menu action order selection, gameplay panel
    orchestration, facade behavior, `OpeningMenuView.hpp`, `SdlDraw.*`,
    `ScenePrimitiveView.*`, `DebugHudView.*`, CMake test definitions, receipt
    golden, staging, commit, push, broad CTest, and window launch were not
    touched/performed.
