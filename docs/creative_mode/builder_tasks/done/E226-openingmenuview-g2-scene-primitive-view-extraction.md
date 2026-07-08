# E226 - OpeningMenuView G2: Scene Primitive View Extraction

## Status

Done.

## Context

E224 preflighted the `OpeningMenuView.cpp` split. E225 completed G1 by moving
the SDL glyph/draw primitives into `src/app/iggy3d/view/SdlDraw.*`.

After E225, current sizes are:

- `src/app/iggy3d/view/OpeningMenuView.cpp`: 1534 lines
- `src/app/iggy3d/view/SdlDraw.cpp`: 139 lines
- `src/app/iggy3d/view/SdlDraw.hpp`: 29 lines

The next safe domain slice is scene/viewport primitive rendering. This is still
pure render extraction; it must not touch HUD, menu panel, hit-test, or facade
behavior.

## Objective

Extract scene primitive and viewport primitive rendering helpers from
`OpeningMenuView.cpp` into a new `ScenePrimitiveView` helper under
`src/app/iggy3d/view/`, preserving behavior.

## Implementation Scope

Edit only:

- `CMakeLists.txt`
- `src/app/iggy3d/view/OpeningMenuView.cpp`
- new `src/app/iggy3d/view/ScenePrimitiveView.hpp`
- new `src/app/iggy3d/view/ScenePrimitiveView.cpp`
- this task card

Add `src/app/iggy3d/view/ScenePrimitiveView.cpp` to the `iggy3d` library source
list near `OpeningMenuView.cpp` and `SdlDraw.cpp`.

## Move Scope

Move the current scene/viewport primitive helper cluster out of
`OpeningMenuView.cpp`.

Current function starts after E225:

- `drawMarker(...)` at about line 96
- `drawPhysicsAabbDebugMarker(...)` at about line 106
- `drawPhysicsContactNormalDebugMarker(...)` at about line 124
- `drawPhysicsBroadphasePairDebugMarker(...)` at about line 140
- `drawFocusIndicator(...)` at about line 154
- `drawRoomEditorCursor(...)` at about line 163
- `drawDoorMarker(...)` at about line 181
- `drawRoomTile(...)` at about line 201
- `drawPrimitiveItem(...)` at about line 534
- `drawGrid(...)` at about line 587
- `topDownMapTitle(...)` at about line 601
- `topDownMapUsesCompactLayout(...)` at about line 617
- `topDownMapAnchorFor(...)` at about line 621
- `topDownMappedItem(...)` at about line 635
- `drawFirstPersonPrimitiveViewport(...)` at about line 655
- `drawTopDownMapPrimitives(...)` at about line 680

The new public header should expose only what `OpeningMenuView.cpp` needs:

- `void drawFirstPersonPrimitiveViewport(SDL_Renderer&, const ProductViewportFrame*);`
- `void drawTopDownMapPrimitives(SDL_Renderer&, const ProductViewportFrame*, const TopDownMapOverlay*);`

Keep the lower-level primitive functions file-local in `ScenePrimitiveView.cpp`
unless compiler fallout proves another narrow declaration is needed.

## Required Behavior Preservation

Preserve exactly:

- draw-kind dispatch in `drawPrimitiveItem(...)`
- marker, physics marker, focus, room-editor cursor, door, tile, and map-maker
  cube preview geometry
- all secondary/decorative colors kept local after E222/E223
- `drawGrid(...)` dimensions and line widths
- top-down map title text, compact/full layout dimensions, marker scale, and
  anchor selection
- null-frame/null-overlay behavior
- `frame->gridVisible` behavior
- current call order in `drawGameplayPanel(...)`

`OpeningMenuView.cpp` should include `app/iggy3d/view/ScenePrimitiveView.hpp`
and continue to call `drawFirstPersonPrimitiveViewport(...)` and
`drawTopDownMapPrimitives(...)`.

## Non-Scope

Do not move or edit behavior in:

- `drawCameraHeading(...)`
- gameplay feedback / interaction mode / movement / NPC / physics / position
  HUD functions
- `drawRoomEditorHud(...)`
- movement tuning HUD
- menu rows and panels
- `drawGameplayPanel(...)` orchestration beyond calling the moved helpers
- hit-test/action routing
- `drawOpeningMenuView(...)`
- `OpeningMenuView.hpp`
- `SdlDraw.*`
- CMake test definitions
- receipt golden files

Do not create:

- `DebugHudView`
- `MenuPanelsView`
- `OpeningMenuHitTest`

No staging, commit, push, broad CTest, or window launch.

## Required Greps

Run and report:

```sh
rg -n "drawMarker|drawPhysicsAabbDebugMarker|drawPhysicsContactNormalDebugMarker|drawPhysicsBroadphasePairDebugMarker|drawFocusIndicator|drawRoomEditorCursor|drawDoorMarker|drawRoomTile|drawPrimitiveItem|drawGrid|topDownMapTitle|topDownMapUsesCompactLayout|topDownMapAnchorFor|topDownMappedItem|drawFirstPersonPrimitiveViewport|drawTopDownMapPrimitives" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/ScenePrimitiveView.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/ScenePrimitiveView.cpp
rg -n "ScenePrimitiveView\\.cpp|SdlDraw\\.cpp|OpeningMenuView\\.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
git -C /Users/kogaryu/iggy3d diff --check
```

Expected classification:

- definitions for the moved scene/viewport primitive helpers live in
  `ScenePrimitiveView.cpp`
- `OpeningMenuView.cpp` retains only call sites for the exported viewport
  helpers
- `ScenePrimitiveView.hpp` exposes only the narrow viewport helper API unless
  compiler fallout forced a wider surface
- `CMakeLists.txt` includes `ScenePrimitiveView.cpp` near `OpeningMenuView.cpp`
  and `SdlDraw.cpp`

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files and this card.

## Self-Blockers

Stop and report instead of widening scope if:

- the extraction requires changing any render geometry, colors, draw order, or
  top-down-map layout
- compile fallout expands outside `OpeningMenuView.cpp`, `ScenePrimitiveView.*`,
  and CMake
- any HUD, menu panel, hit-test, receipt, or facade behavior needs to move with
  the scene helper
- the new header wants to expose broad internals instead of the two viewport
  helper calls currently needed by `OpeningMenuView.cpp`
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
- confirmation that HUD/menu/hit-test/facade policy, `OpeningMenuView.hpp`,
  `SdlDraw.*`, CMake test definitions, staging, commit, push, and window launch
  were not touched

## Completion Brief

Card moved to done: yes.

Files changed:

- `CMakeLists.txt`
- `src/app/iggy3d/view/OpeningMenuView.cpp`
- `src/app/iggy3d/view/ScenePrimitiveView.hpp`
- `src/app/iggy3d/view/ScenePrimitiveView.cpp`
- `docs/creative_mode/builder_tasks/done/E226-openingmenuview-g2-scene-primitive-view-extraction.md`

Exact helper/API shape created:

- Added `src/app/iggy3d/view/ScenePrimitiveView.hpp` and `src/app/iggy3d/view/ScenePrimitiveView.cpp`.
- The header is guarded by `#if defined(IGGY3D_HAS_SDL3)`, forward-declares `SDL_Renderer`, `ProductViewportFrame`, and `TopDownMapOverlay`, and exposes only:
  - `void drawFirstPersonPrimitiveViewport(SDL_Renderer&, const ProductViewportFrame*);`
  - `void drawTopDownMapPrimitives(SDL_Renderer&, const ProductViewportFrame*, const TopDownMapOverlay*);`
- `ScenePrimitiveView.cpp` owns the moved scene/viewport primitive helpers:
  - `drawMarker(...)`
  - `drawPhysicsAabbDebugMarker(...)`
  - `drawPhysicsContactNormalDebugMarker(...)`
  - `drawPhysicsBroadphasePairDebugMarker(...)`
  - `drawFocusIndicator(...)`
  - `drawRoomEditorCursor(...)`
  - `drawDoorMarker(...)`
  - `drawRoomTile(...)`
  - `drawPrimitiveItem(...)`
  - `drawGrid(...)`
  - `topDownMapTitle(...)`
  - `topDownMapUsesCompactLayout(...)`
  - `topDownMapAnchorFor(...)`
  - `topDownMappedItem(...)`
  - `drawFirstPersonPrimitiveViewport(...)`
  - `drawTopDownMapPrimitives(...)`
- All lower-level marker/tile/grid/top-down helpers remain file-local in `ScenePrimitiveView.cpp`.

What stayed in `OpeningMenuView.cpp`:

- `OpeningMenuView.cpp` now includes `app/iggy3d/view/ScenePrimitiveView.hpp`.
- It retains only the two call sites in `drawGameplayPanel(...)`:
  - `drawFirstPersonPrimitiveViewport(renderer, frame);`
  - `drawTopDownMapPrimitives(renderer, frame, topDownMapOverlay);`
- HUD functions, room-editor HUD, movement tuning HUD, menu rows/panels, hit-test/action routing, and `drawOpeningMenuView(...)` stayed in `OpeningMenuView.cpp`.

CMake source-list placement:

- Added `src/app/iggy3d/view/ScenePrimitiveView.cpp` immediately after `src/app/iggy3d/view/OpeningMenuView.cpp` and before `src/app/iggy3d/view/SdlDraw.cpp`.

Required grep classification:

- Definitions for all moved scene/viewport primitive helpers live in `ScenePrimitiveView.cpp`.
- `ScenePrimitiveView.hpp` exposes only `drawFirstPersonPrimitiveViewport(...)` and `drawTopDownMapPrimitives(...)`.
- `OpeningMenuView.cpp` retains only call sites for the two exported viewport helpers.
- `CMakeLists.txt` has:
  - `OpeningMenuView.cpp` at line 87
  - `ScenePrimitiveView.cpp` at line 88
  - `SdlDraw.cpp` at line 89

Focused build result:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10
```

Passed. The build compiled `src/app/iggy3d/view/ScenePrimitiveView.cpp` and rebuilt the listed test targets.

Focused CTest result:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
```

Passed: 4/4 tests.

Receipt golden diff result:

```sh
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
```

No diff.

Diff/whitespace checks:

```sh
git -C /Users/kogaryu/iggy3d diff --check
rg -n "[ \t]$" CMakeLists.txt src/app/iggy3d/view/OpeningMenuView.cpp src/app/iggy3d/view/ScenePrimitiveView.hpp src/app/iggy3d/view/ScenePrimitiveView.cpp docs/creative_mode/builder_tasks/done/E226-openingmenuview-g2-scene-primitive-view-extraction.md
```

Both clean; trailing-whitespace scan returned no hits.

Confirmation:

- HUD policy was not moved.
- Menu panel policy was not moved.
- Hit-test/action routing was not moved.
- Facade policy and `drawOpeningMenuView(...)` were not moved.
- `OpeningMenuView.hpp` was not changed.
- `SdlDraw.*` was not changed.
- CMake test definitions were not changed.
- Receipt golden files were not changed.
- No staging, commit, push, broad CTest, or window launch was performed.
