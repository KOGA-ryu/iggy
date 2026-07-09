# File Spec

Files: `src/app/iggy3d/view/SdlDraw.hpp`, `src/app/iggy3d/view/SdlDraw.cpp`

Verified at: `2b846f78`

## Owns

- Minimal SDL drawing primitives used by product SDL view code.
- Built-in 5-by-7 bitmap glyph rows for supported uppercase text, digits, and selected punctuation.
- Color, filled rectangle, and bitmap text rendering helpers.

## Does Not Own

- UI layout.
- Menu or gameplay panel composition.
- Font loading.
- Vulkan rendering.
- SDL window lifecycle.
- Hit testing.

## Reads

- Input text characters for `drawText(...)`.
- Current renderer target through `SDL_Renderer`.

## Writes / Mutates

- Calls SDL draw APIs to set renderer color and fill rectangles.
- `drawText(...)` advances only local cursor state.
- Does not mutate app model state.

## Calls Out To / Wires Out To

- `SDL_SetRenderDrawColor(...)`.
- `SDL_RenderFillRect(...)`.
- Callers in view/HUD/panel files use `setColor(...)`, `fillRect(...)`, and `drawText(...)`.

## Called By / Entry Points

- `OpeningMenuView.cpp`, `MenuPanelsView.cpp`, `DebugHudView.cpp`, and `ScenePrimitiveView.cpp`.
- Focused proof: `rg -n "setColor\\(|fillRect\\(|drawText\\(" src/app/iggy3d/view`.

## Invariants

- Text rendering is deterministic bitmap drawing with no external font dependency.
- Unsupported glyphs render as blank rows.
- Space advances the cursor without drawing pixels.
- Color helper always sets full alpha.
- This file is SDL-only behind `IGGY3D_HAS_SDL3`.

## Tests / Proof Commands

- `rg -n "drawText\\(|glyphFor\\(|SdlDraw" src/app tests`.
- `rg -n "product_window_renderer_lifecycle_tests|product_menu_usefulness_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/view/MenuPanelsView.*` unless panel text/layout changes.
- `src/app/iggy3d/view/OpeningMenuView.*` unless top-level draw composition changes.
- `src/app/iggy3d/view/DebugHudView.*` unless HUD text drawing changes.
- `src/app/platform/SdlWindow.*` unless SDL window lifecycle changes.

## Update When

- Glyph tables, text spacing, color alpha policy, rectangle drawing, or SDL-only drawing helper behavior changes.

## Do Not Update When

- Only higher-level UI layout, panel text, hit testing, or renderer backend behavior changes.
