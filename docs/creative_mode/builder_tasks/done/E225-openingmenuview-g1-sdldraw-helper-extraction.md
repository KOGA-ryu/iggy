# E225 - OpeningMenuView G1: SdlDraw Helper Extraction

## Status

Done.

## Context

E224 preflighted the `OpeningMenuView.cpp` split and recommended starting with
the lowest-risk pure extraction: the SDL glyph/draw helper block.

Current helpers are file-local in `src/app/iggy3d/view/OpeningMenuView.cpp`:

- `using GlyphRows = std::array<std::uint8_t, 7>;`
- `glyphFor(char)`
- `setColor(SDL_Renderer&, std::uint8_t, std::uint8_t, std::uint8_t)`
- `fillRect(SDL_Renderer&, float, float, float, float)`
- `drawText(SDL_Renderer&, std::string_view, float, float, float)`

They are renderer primitives, not menu/HUD/primitive/hit-test policy. Moving
them first gives later `ScenePrimitiveView`, `DebugHudView`, and `MenuPanelsView`
slices a shared drawing seam without changing render behavior.

## Objective

Extract only the SDL primitive helper block from `OpeningMenuView.cpp` into a
new `SdlDraw` helper under `src/app/iggy3d/view/`, preserving behavior.

## Implementation Scope

Edit only:

- `CMakeLists.txt`
- `src/app/iggy3d/view/OpeningMenuView.cpp`
- new `src/app/iggy3d/view/SdlDraw.hpp`
- new `src/app/iggy3d/view/SdlDraw.cpp`
- this task card

Add `src/app/iggy3d/view/SdlDraw.cpp` to the `iggy3d` library source list near
`OpeningMenuView.cpp`.

## Required Shape

Create `SdlDraw.hpp/.cpp` in namespace `iggy3d`.

The helper should own:

- `GlyphRows`
- `glyphFor(char)`
- `setColor(SDL_Renderer&, std::uint8_t, std::uint8_t, std::uint8_t)`
- `fillRect(SDL_Renderer&, float, float, float, float)`
- `drawText(SDL_Renderer&, std::string_view, float, float, float)`

Preserve:

- `#if defined(IGGY3D_HAS_SDL3)` behavior and SDL3 include requirements
- exact glyph row data
- `std::toupper(static_cast<unsigned char>(c))`
- unknown-glyph blank fallback
- `SDL_SetRenderDrawColor(&renderer, r, g, b, 255)`
- `SDL_FRect` construction and `SDL_RenderFillRect(&renderer, &rect)`
- text spacing at `6.0F * scale`
- fill-rect call geometry inside `drawText(...)`

`OpeningMenuView.cpp` should include `app/iggy3d/view/SdlDraw.hpp` and keep
calling the same helper names.

## Non-Scope

Do not move or edit behavior in:

- `fixedFloat(...)`
- `usesPauseMenuRows(...)`
- `menuActionOrderForFrontend(...)`
- `menuTitleForFrontend(...)`
- `selectedDraftGlyphLabel(...)`
- marker / primitive / tile / door drawing policy
- gameplay/debug HUD drawing
- room editor HUD/cursor policy beyond existing helper calls
- menu panel drawing
- hit-test/action routing
- `drawOpeningMenuView(...)`
- `OpeningMenuView.hpp`
- CMake test definitions
- receipt golden files

Do not create:

- `ScenePrimitiveView`
- `DebugHudView`
- `MenuPanelsView`
- `OpeningMenuHitTest`

No staging, commit, push, broad CTest, or window launch.

## Required Greps

Run and report:

```sh
rg -n "GlyphRows|glyphFor|setColor\\(|fillRect\\(|drawText\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/SdlDraw.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/SdlDraw.cpp
rg -n "SdlDraw\\.cpp|OpeningMenuView\\.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
git -C /Users/kogaryu/iggy3d diff --check
```

Expected classification:

- `GlyphRows` and `glyphFor(...)` definitions live only in `SdlDraw.*`.
- `setColor(...)`, `fillRect(...)`, and `drawText(...)` definitions live in
  `SdlDraw.*`; `OpeningMenuView.cpp` retains call sites only.
- `CMakeLists.txt` includes `SdlDraw.cpp` near `OpeningMenuView.cpp`.

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

- the extraction requires changing draw order, glyph data, SDL alpha, or text
  spacing
- compile fallout expands outside `OpeningMenuView.cpp`, `SdlDraw.*`, and CMake
- any menu, HUD, primitive dispatch, hit-test, receipt, or panel behavior needs
  to move with the helper
- receipt golden output changes
- the helper cannot cleanly preserve the existing `IGGY3D_HAS_SDL3` behavior

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
- confirmation that scene/HUD/menu/hit-test policy, `OpeningMenuView.hpp`,
  CMake test definitions, staging, commit, push, and window launch were not
  touched

## Completion Brief

Card moved to done: yes.

Files changed:

- `CMakeLists.txt`
- `src/app/iggy3d/view/OpeningMenuView.cpp`
- `src/app/iggy3d/view/SdlDraw.hpp`
- `src/app/iggy3d/view/SdlDraw.cpp`
- `docs/creative_mode/builder_tasks/done/E225-openingmenuview-g1-sdldraw-helper-extraction.md`

Exact helper/API shape created:

- Added `src/app/iggy3d/view/SdlDraw.hpp` and `src/app/iggy3d/view/SdlDraw.cpp` in namespace `iggy3d`.
- `SdlDraw.hpp` is guarded by `#if defined(IGGY3D_HAS_SDL3)`, forward-declares `SDL_Renderer`, and declares:
  - `using GlyphRows = std::array<std::uint8_t, 7>;`
  - `GlyphRows glyphFor(char c);`
  - `void setColor(SDL_Renderer&, std::uint8_t, std::uint8_t, std::uint8_t);`
  - `void fillRect(SDL_Renderer&, float, float, float, float);`
  - `void drawText(SDL_Renderer&, std::string_view, float, float, float);`
- `SdlDraw.cpp` owns the moved glyph table and helper definitions under the same SDL3 guard.
- Preserved exact glyph row data, `std::toupper(static_cast<unsigned char>(c))`, blank fallback, alpha `255`, `SDL_FRect` construction, `SDL_RenderFillRect(&renderer, &rect)`, text spacing, and text fill-rect geometry.

What stayed in `OpeningMenuView.cpp`:

- All menu, HUD, scene primitive, room-editor, panel, hit-test, and `drawOpeningMenuView(...)` policy stayed in place.
- `OpeningMenuView.cpp` now includes `app/iggy3d/view/SdlDraw.hpp` and keeps calling `setColor(...)`, `fillRect(...)`, and `drawText(...)` by the same helper names.
- Removed only the local `GlyphRows`, `glyphFor(...)`, `setColor(...)`, `fillRect(...)`, and `drawText(...)` definitions plus the no-longer-needed `<cctype>` include.

CMake source-list placement:

- Added `src/app/iggy3d/view/SdlDraw.cpp` immediately after `src/app/iggy3d/view/OpeningMenuView.cpp` in the `iggy3d` library source list.

Required grep classification:

- `GlyphRows` and `glyphFor(...)` live in `SdlDraw.hpp/.cpp`; `OpeningMenuView.cpp` has no remaining `GlyphRows`/`glyphFor(...)` hits.
- `setColor(...)`, `fillRect(...)`, and `drawText(...)` definitions live in `SdlDraw.cpp`, with declarations in `SdlDraw.hpp`.
- `OpeningMenuView.cpp` retains call sites only for `setColor(...)`, `fillRect(...)`, and `drawText(...)`.
- `CMakeLists.txt` has `OpeningMenuView.cpp` at line 87 and `SdlDraw.cpp` at line 88.

Focused build result:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_window_input_frame_tests product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10
```

Passed. The build compiled `src/app/iggy3d/view/SdlDraw.cpp` and rebuilt the listed test targets.

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
rg -n "[ \t]$" CMakeLists.txt src/app/iggy3d/view/OpeningMenuView.cpp src/app/iggy3d/view/SdlDraw.hpp src/app/iggy3d/view/SdlDraw.cpp docs/creative_mode/builder_tasks/done/E225-openingmenuview-g1-sdldraw-helper-extraction.md
```

Both clean; the trailing-whitespace scan returned no hits.

Confirmation:

- Scene primitive policy was not moved.
- Gameplay/debug HUD policy was not moved.
- Menu panel policy was not moved.
- Hit-test/action routing was not moved.
- `drawOpeningMenuView(...)` was not moved.
- `OpeningMenuView.hpp` was not changed.
- CMake test definitions were not changed.
- Receipt golden files were not changed.
- No staging, commit, push, broad CTest, or window launch was performed.
