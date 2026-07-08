# E225 - OpeningMenuView G1: SdlDraw Helper Extraction

## Status

Ready.

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
