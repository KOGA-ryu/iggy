# E224 - OpeningMenuView Split Preflight

## Status

Done.

## Context

The render draw-kind metadata lane is now complete as E222-E223. The remaining
render organization item from `docs/complexity_audit_v0_1.md` is the large
mixed-responsibility `OpeningMenuView.cpp` file.

Current size at release:

- `src/app/iggy3d/view/OpeningMenuView.cpp`: 1654 lines
- `src/app/iggy3d/view/OpeningMenuView.hpp`: 114 lines

The complexity audit recommends splitting `OpeningMenuView.cpp` by domain into
something like `MenuPanelsView`, `DebugHudView`, `RoomEditorOverlayView`,
`ScenePrimitiveView`, and a shared SDL draw helper, while keeping
`drawOpeningMenuView(...)` as the thin entry point.

This card is only the preflight. Do not implement the split in this card.

## Objective

Read-only audit the current `OpeningMenuView` surface and produce a grounded
split plan with safe child slices.

The output should answer:

1. Which line ranges and functions belong to each domain cluster?
2. Which helpers are genuinely shared draw primitives versus domain policy?
3. Which functions are pure rendering and which are hit-test/action routing?
4. Whether `OpeningMenuView.hpp` should stay as the facade or shrink in a later
   slice.
5. Whether a shared `SdlDraw` helper is justified, and if so which exact
   functions belong there.
6. Which implementation slice should go first with the least compile/test
   fallout.

## Audit Scope

Inspect:

- `src/app/iggy3d/view/OpeningMenuView.cpp`
- `src/app/iggy3d/view/OpeningMenuView.hpp`
- focused callers found by grep, especially `drawOpeningMenuView(...)`,
  `openingMenuActionAt(...)`, and `pauseMenuActionAt(...)`
- focused tests found by grep for `OpeningMenuView`, hit tests, menu routing,
  and render bridge / viewport primitives
- `docs/complexity_audit_v0_1.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- relevant completed cards if needed for context, especially E222-E223

## Classification Requirements

Produce a table or grouped list covering at least these expected clusters:

- SDL primitive helpers:
  - `setColor(...)`
  - `fillRect(...)`
  - `drawText(...)`
  - any tiny renderer helper that is not domain policy
- scene primitive / viewport drawing:
  - marker, room tile, door marker, primitive item dispatch
  - first-person primitive viewport
  - top-down map primitive rendering
  - grid drawing
- gameplay/debug HUD drawing:
  - camera heading
  - gameplay feedback
  - interaction mode HUD
  - movement / NPC / physics debug HUDs
  - position HUD
  - movement tuning HUD
- room editor overlay drawing:
  - room editor HUD
  - room editor cursor / placement preview relationships if applicable
- menu panels:
  - starter, new-world, load-save, delete confirmation, dev tools, settings,
    gameplay panel, ASCII preview lines, panel rows, sliders
- hit-test and action routing:
  - `uiRectContains(...)`
  - `pauseMenuActionAt(...)`
  - `openingMenuDetailSurfaceFor(...)`
  - `openingMenuActionAt(...)`
- facade / orchestration:
  - `drawOpeningMenuView(...)`

For each cluster, classify:

- proposed file owner or "keep in facade"
- whether it can move in a pure extraction slice
- expected CMake/source-list impact
- focused tests that should gate the slice
- self-blockers or behavior traps

## Required Greps

Run and report:

```sh
rg -n "drawOpeningMenuView|OpeningMenuView|OpeningMenuHitTestResult|openingMenuActionAt|pauseMenuActionAt|draw[A-Z]|setColor|fillRect|drawText|ProductViewportFramedItem|ProductUiRect|TopDownMapOverlay|Hud|Panel|Primitive" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.hpp /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "OpeningMenuView|OpeningMenuView.cpp|MenuPanelsView|DebugHudView|RoomEditorOverlayView|ScenePrimitiveView|SdlDraw" /Users/kogaryu/iggy3d/docs/complexity_audit_v0_1.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/PRIORITY.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done --glob '*.md'
git -C /Users/kogaryu/iggy3d diff --check
```

Use additional `rg`, `sed`, `nl`, or `awk` reads as needed for exact line
ranges and call-site mapping.

## Deliverables

Append a completion brief to this card with:

- files inspected
- current line counts
- grouped domain/line-range table
- direct caller/test hotspot summary
- shared helper classification
- recommended child-card sequence
- safest first implementation card, or a clear "no implementation card"
  recommendation if the split is not worthwhile
- focused verification commands for the first implementation card
- stale assumptions found in `docs/complexity_audit_v0_1.md` or priority docs
- confirmation that no source, tests, CMake, receipt golden, staging, commit,
  push, or window launch was performed

## Non-Scope

Do not edit source, tests, CMake, fixtures, receipt golden files, or production
docs in this card.

Do not:

- split `OpeningMenuView.cpp`
- create `SdlDraw.hpp`
- move `drawOpeningMenuView(...)`
- change hit-test behavior
- change render output
- change draw ordering
- change receipt fields/order/values
- launch a window
- run broad CTest
- stage, commit, or push

## Self-Blockers

Stop and report if:

- the split requires a behavior decision rather than pure extraction
- line-range ownership cannot be made clear from current code
- tests do not cover the proposed first slice well enough to recommend it
- the only available split would create a mega-helper or hide domain policy

## Completion Brief

Card moved to done: yes.

Files inspected:

- `src/app/iggy3d/view/OpeningMenuView.cpp`
- `src/app/iggy3d/view/OpeningMenuView.hpp`
- `src/app/iggy3d/window/FramePresenter.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/menu/DrawList.hpp`
- `tests/unit/product_window_input_frame_tests.cpp`
- `tests/unit/product_primitive_draw_list_tests.cpp`
- `tests/unit/product_render_bridge_tests.cpp`
- `docs/complexity_audit_v0_1.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- relevant E222/E223 done-card context from the required docs grep

Current line counts:

- `src/app/iggy3d/view/OpeningMenuView.cpp`: 1654 lines
- `src/app/iggy3d/view/OpeningMenuView.hpp`: 114 lines

Grouped domain/line-range table:

| Cluster | Current line ranges | Functions / facts | Proposed owner | Pure extraction? | Gates / traps |
|---|---:|---|---|---|---|
| Menu policy helpers | 41-96 | `fixedFloat` forward decl, `usesPauseMenuRows`, `menuActionOrderForFrontend`, `menuTitleForFrontend`, `selectedDraftGlyphLabel` | `MenuPanelsView.cpp` for title/order/glyph; `fixedFloat` with menu/HUD formatting until split clarifies use | Mostly yes, but title/order are also used by facade and hit-test | Do not change starter/pause row order or glyph labels. |
| SDL primitive helpers | 39, 98-214 | `GlyphRows`, `glyphFor`, `setColor`, `fillRect`, `drawText` | `SdlDraw.hpp/.cpp` | Yes, safest first | Keep SDL3 guards, glyph bytes, alpha 255, text spacing, and CMake source-list update. |
| Scene primitive item drawing | 216-388, 650-705 | marker drawing, physics markers, focus indicator, room editor cursor, door marker, room tile, `drawPrimitiveItem` dispatch | `ScenePrimitiveView.cpp` | Yes after SdlDraw | Preserve draw-kind dispatch and secondary/decorative colors. E222/E223 deliberately kept those local. |
| Top-down / viewport primitives | 707-852 | `drawGrid`, top-down title/layout/anchor/mapping, `drawFirstPersonPrimitiveViewport`, `drawTopDownMapPrimitives` | `ScenePrimitiveView.cpp` | Yes after SdlDraw | Keep grid dimensions, minimap/full-map layout, marker scaling, and frame null checks. |
| Gameplay/debug HUD | 389-542, 573-648 | camera heading, feedback tone, gameplay feedback, interaction mode HUD, movement/NPC/physics/position HUDs, gameplay movement tuning HUD | `DebugHudView.cpp` | Yes after SdlDraw | No direct pixel tests; keep visibility limits, colors, row caps, and camera heading state behavior. |
| Room editor overlay | 543-571 plus primitive cursor at 283-299 and placement preview dispatch at 667-672 | `drawRoomEditorHud`; cursor/placement preview render as draw-kind primitives | Prefer `DebugHudView.cpp` for HUD now; keep cursor/placement in `ScenePrimitiveView.cpp` | Yes, but separate `RoomEditorOverlayView` is too small today | Do not split room editor cursor away from primitive dispatch unless a later owner wants a room-editor renderer. |
| Menu panels | 853-1237 | `drawMenuRow`, settings labels, `fixedFloat`, `sliderBar`, `drawMovementTuningRows`, panel rows, ASCII preview, starter/new-world/load/delete/dev/settings/gameplay panels | `MenuPanelsView.cpp` | Yes after SdlDraw | Preserve fixed coordinates and text; avoid hiding panel policy behind generic helpers. |
| Gameplay panel orchestration | 1238-1296 | `drawGameplayPanel` orchestrates viewport, map, HUDs, runtime readouts | Keep in facade at first, then maybe `DebugHudView` or `GameplayPanelView` | Later | This is orchestration across scene primitive and HUD clusters; move only after those helpers exist. |
| Hit-test/action routing | 1297-1529 | `uiRectContains`, `hitRegionActionAt`, `pauseMenuActionAt`, `openingMenuDetailSurfaceFor`, `openingMenuActionAt` | Later `OpeningMenuHitTest.hpp/.cpp` | Not first; behavior-sensitive | Pause/load/delete already consume hit regions; starter rows, new-world, dev-tools, settings still have coordinate policy. Product window input tests pin this. |
| Facade / orchestration | 1531-1651 | `drawOpeningMenuView` entry point, SDL clear/present, state receipts, menu/gameplay branch | Keep in `OpeningMenuView.cpp` | Yes as thin facade after child slices | Do not move first. Preserve `OpeningMenuViewState` receipt fields. |

Direct caller and test hotspot summary:

- `src/app/iggy3d/window/FramePresenter.cpp:904-942` is the only production draw caller. It consumes `OpeningMenuViewState` to update camera-heading/menu text/selected-row/row-count receipt fields.
- `src/app/iggy3d/window/InputFrame.cpp:1541-1542` is the only production `openingMenuActionAt(...)` caller; `dispatchProductOpeningMenuMouseHit(...)` at `InputFrame.cpp:608-625` consumes the hit result.
- `tests/unit/product_window_input_frame_tests.cpp` is the direct hit-test and mouse-dispatch coverage hotspot:
  - `hitAt(...)`: lines 60-76
  - starter/pause rows: lines 1040-1090
  - detail surface: lines 1264-1266
  - child-panel hit areas: lines 1768-1860
  - mouse dispatch from hit result: lines 1863-1980
  - normalized click to new-world hit: lines 2015-2035
- `tests/unit/product_primitive_draw_list_tests.cpp` and `tests/unit/product_render_bridge_tests.cpp` pin upstream primitive metadata/list/bridge behavior, not SDL pixels.
- No focused SDL pixel/screenshot test for `drawOpeningMenuView(...)` was found.

Shared helper classification:

- Justified `SdlDraw` helper:
  - `GlyphRows`, `glyphFor`, `setColor`, `fillRect`, `drawText`.
  - These are renderer primitives and font data, not menu, HUD, hit-test, or draw-kind policy.
  - Recommended shape: `src/app/iggy3d/view/SdlDraw.hpp` plus `SdlDraw.cpp`, added near `OpeningMenuView.cpp` in `CMakeLists.txt`.
- Do not share yet:
  - `fixedFloat(...)` and `sliderBar(...)`: formatting/panel policy.
  - `setFeedbackToneColor(...)`: gameplay/HUD tone policy.
  - `drawPanelRow(...)` / `drawMenuRow(...)`: menu panel policy.
  - primitive dispatch and decorative colors: scene-render policy, not generic SDL helper.
  - `uiRectContains(...)` / `hitRegionActionAt(...)`: input/hit routing policy, not draw primitive support.

Header/facade conclusion:

- `OpeningMenuView.hpp` should stay the temporary facade during the first rendering extractions because both `FramePresenter.cpp` and `InputFrame.cpp` include it.
- A later hit-test slice should split `OpeningMenuHitTestResult`, `OpeningMenuHitArea`, `openingMenuActionAt(...)`, and `openingMenuDetailSurfaceFor(...)` into `OpeningMenuHitTest.hpp/.cpp`; after direct callers are repointed, `OpeningMenuView.hpp` can shrink to only `OpeningMenuViewState` and `drawOpeningMenuView(...)`.
- Do not shrink the header in the first rendering slice; that would mix render extraction with input API fallout.

Recommended child-card sequence:

1. **E225 - OpeningMenuView G1: SdlDraw Helper Extraction**
   - Add `src/app/iggy3d/view/SdlDraw.hpp`.
   - Add `src/app/iggy3d/view/SdlDraw.cpp`.
   - Move only `GlyphRows`, `glyphFor`, `setColor`, `fillRect`, and `drawText`.
   - Include the new header from `OpeningMenuView.cpp`.
   - Add `SdlDraw.cpp` to `CMakeLists.txt`.
   - Do not move any menu/HUD/primitive/hit-test policy.
2. **OpeningMenuView G2: Scene Primitive View Extraction**
   - Add `ScenePrimitiveView.hpp/.cpp`.
   - Move lines 216-388 and 650-852, including first-person and top-down primitive render helpers.
   - Keep draw-kind decorative colors local in that owner.
3. **OpeningMenuView G3: Debug HUD View Extraction**
   - Add `DebugHudView.hpp/.cpp`.
   - Move gameplay/debug HUD helpers from 389-648 plus `drawRoomEditorHud(...)`.
   - Keep movement tuning panel rows out of this slice.
4. **OpeningMenuView G4: Menu Panels View Extraction**
   - Add `MenuPanelsView.hpp/.cpp`.
   - Move menu rows, panel rows, ASCII preview, settings/dev/new-world/load/delete/starter panel drawing.
   - Leave `drawOpeningMenuView(...)` as orchestration.
5. **OpeningMenuView G5: Hit-Test Split**
   - Add `OpeningMenuHitTest.hpp/.cpp`.
   - Move hit-test result types and `openingMenuActionAt(...)` / `openingMenuDetailSurfaceFor(...)`.
   - Repoint `InputFrame.cpp` and `product_window_input_frame_tests.cpp`.
   - Consider a later behavior card to route remaining hand-derived starter/new-world/settings/dev-tools rectangles through `ProductUiDrawList` hit regions.
6. **OpeningMenuView G6: Facade Include Cleanup**
   - Shrink `OpeningMenuView.hpp` after draw and hit-test callers are separated.

Safest first implementation card:

**E225 - OpeningMenuView G1: SdlDraw Helper Extraction**

Scope:

- Edit only `CMakeLists.txt`, `src/app/iggy3d/view/OpeningMenuView.cpp`, new `src/app/iggy3d/view/SdlDraw.hpp`, new `src/app/iggy3d/view/SdlDraw.cpp`, and the task card.
- Move only the renderer primitive helper block: `GlyphRows`, `glyphFor`, `setColor`, `fillRect`, `drawText`.
- Keep all function bodies byte-equivalent except for namespace/header qualification needed to call the new helper.
- Keep `OpeningMenuView.hpp`, `drawOpeningMenuView(...)`, hit-test APIs, draw-kind dispatch, menu/HUD/panel logic, CMake test definitions, receipt golden, and renderer/window behavior unchanged.

Focused verification for E225:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_window_input_frame_tests|product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

E225 self-blockers:

- Moving the helper requires changing draw order, glyph data, SDL color alpha, or text spacing.
- CMake/source fallout expands outside `OpeningMenuView.cpp` and the new helper.
- Any hit-test, menu panel, HUD, primitive dispatch, or receipt behavior has to move with the helper.
- Receipt golden changes.

Stale assumptions / docs notes:

- `docs/complexity_audit_v0_1.md` is line-count stale by one line for this file (`1653` in the audit versus `1654` now), but its split recommendation remains valid.
- The broader draw-kind metadata part of the older roadmap is now complete as recorded in `PRIORITY.md`: E222-E223 closed the remaining primary metadata handoff, so no metadata child card should be mixed into the `OpeningMenuView` split.
- The current hit-test state is partly newer than older handoff wording: pause-menu routing uses `buildProductPauseUiDrawList(...)` and `routeProductUiHit(...)`; load-save/delete-confirm buttons use draw-list hit regions; starter rows, new-world buttons, settings tabs, and dev-tools categories still have coordinate policy in `OpeningMenuView.cpp`.

Commands run:

```sh
wc -l /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.hpp
rg -n "drawOpeningMenuView|OpeningMenuView|OpeningMenuHitTestResult|openingMenuActionAt|pauseMenuActionAt|draw[A-Z]|setColor|fillRect|drawText|ProductViewportFramedItem|ProductUiRect|TopDownMapOverlay|Hud|Panel|Primitive" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.hpp /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "OpeningMenuView|OpeningMenuView.cpp|MenuPanelsView|DebugHudView|RoomEditorOverlayView|ScenePrimitiveView|SdlDraw" /Users/kogaryu/iggy3d/docs/complexity_audit_v0_1.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/PRIORITY.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done --glob '*.md'
rg -n "^[A-Za-z_][A-Za-z0-9_:<>*& ]+ [A-Za-z_][A-Za-z0-9_]*\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.hpp
rg -n "#include \"app/iggy3d/view/OpeningMenuView.hpp\"|drawOpeningMenuView\\(|openingMenuActionAt\\(|pauseMenuActionAt\\(|openingMenuDetailSurfaceFor\\(" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "PauseUi|buildProductPauseUiDrawList|ProductUiHitRoute|hitRegions|hand-derived hit rects|opening-menu hit-test" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/docs/ui /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp' --glob '*.md'
git -C /Users/kogaryu/iggy3d diff --check
```

Read-only confirmation:

- No source files edited.
- No test files edited.
- No CMake files edited.
- No fixture or receipt golden files edited.
- No staging, commit, push, broad CTest, or window launch performed.
