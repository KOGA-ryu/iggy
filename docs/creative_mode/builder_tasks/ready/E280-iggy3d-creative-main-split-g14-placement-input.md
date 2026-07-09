# E280: iggy3d_creative Main Split G14 - Placement Input

## Objective

Extract the interactive placement input block from
`apps/iggy3d_creative/main.cpp` into a new app-local
`CreativeEditorPlacementInput.*` helper module.

This is a mechanical extraction only. Preserve place-mode gating, Left-Alt
mouse behavior, `placeButtonDown` latch behavior, high-level placement call
semantics, object ids, frame numbers, render output, and focused test results.

## Current Context

E279 extracted synthetic/interactive click selection into
`CreativeEditorClickSelection.*`.

Post-E279 current shape:

- `apps/iggy3d_creative/main.cpp`: 1137 lines.
- The remaining placement block starts at
  `// ---- PLACE -------------------------------------------------------------`.
- It runs only when `editor.placeMode && capturePath.empty()`.
- Capture placement remains owned by `StandaloneCaptureScenario.*` and must not
  move in this slice.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorPlacementInput.hpp`
- new `apps/iggy3d_creative/CreativeEditorPlacementInput.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/CreativeEditor*.cpp` helper files.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
void applyCreativeEditorPlacementInput(
    iggy3d::SdlWindow& window,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    iggy3d::Vec3 aimCellCenter,
    bool captureMode);
```

Expected header dependencies:

- `app/iggy3d/creative/CreativeAppState.hpp`
- `app/platform/SdlWindow.hpp`
- `core/math/Vec3.hpp`
- `CreativeEditorState.hpp`

Expected implementation dependencies:

- `<SDL3/SDL.h>`
- `StandalonePlacement.hpp`

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorPlacementInput.hpp`;
- add `using iggy3d_creative_app::applyCreativeEditorPlacementInput;`;
- replace only the current placement block with:

```cpp
applyCreativeEditorPlacementInput(
    window, appState, editor, aimCellCenter, !capturePath.empty());
```

Do not move capture scenario dispatch, selection resolution, gizmo, Move,
overlay, submit, shutdown, or final logging code.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `if (editor.placeMode && capturePath.empty())`, represented through
  `editor.placeMode` and `captureMode`;
- `SDL_GetKeyboardState(nullptr)` and Left Alt check;
- `window.setRelativeMouseMode(false)` while Alt is held;
- `SDL_GetMouseState(&mx, &my)`;
- left-button mask check;
- edge-triggered `editor.placeButtonDown` behavior;
- `placeBrushObjectWithUndo(appState.facade, editor.undoStack,
  editor.placeBrush, aimCellCenter, ++editor.placedCount,
  "place_interactive")`;
- latch reset when the left button is up;
- `window.setRelativeMouseMode(true)` and `editor.placeButtonDown = false`
  when Alt is not held.

Preserve all downstream capture scenario, selection, gizmo, Move, overlay,
submit, and final logging behavior.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

Do not move capture scenario dispatch or capture placement.
Do not move capture Move behavior.
Do not move interactive Move behavior.
Do not move selection resolution, gizmo/path policy, overlay construction,
frustum culling, submit, shutdown, capture script frame numbers, object ids,
seeded Floor/Crate setup, save root setup, final capture proof strings, or
render-submit reason strings.
Do not create `EditorFrame.{hpp,cpp}`.
Do not create `runCreativeEditorFrame(...)`.
Do not change `main(int, char**)`, `--frames`, or `--capture`
parsing/default behavior.
Do not run broad CTest.
Do not launch an interactive window.
Do not require `iggy3d_creative --capture` unless an owner explicitly allows a
windowed/Vulkan capture check.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "applyCreativeEditorPlacementInput|placeBrushObjectWithUndo|placeButtonDown|SDL_GetKeyboardState|SDL_GetMouseState|setRelativeMouseMode|place_interactive" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorPlacementInput.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorPlacementInput.cpp
```

Expected:

- `CreativeEditorPlacementInput.hpp` declares
  `applyCreativeEditorPlacementInput(...)`;
- `CreativeEditorPlacementInput.cpp` defines
  `applyCreativeEditorPlacementInput(...)`;
- placement keyboard/mouse handling and `placeBrushObjectWithUndo(...)` live in
  `CreativeEditorPlacementInput.cpp`;
- `main.cpp` calls `applyCreativeEditorPlacementInput(...)`;
- capture scenario, Move, overlay, and submit remain in `main.cpp`.

Run:

```sh
rg -n "EditorFrame|runCreativeEditorFrame|CreativeEditorClickSelection|CreativeEditorPickFrame|CreativeEditorGizmoFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- existing helper ownership remains unchanged;
- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- no old `appendWireframeBoxEdges(...)` helper was reintroduced.

Run:

```sh
rg -n "CreativeEditorPlacementInput.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
```

Expected:

- the new `.cpp` is included only in the `iggy3d_creative` executable source
  list.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched source/CMake files and
this task card after moving it to `done/`.

Optional capture remains skipped unless explicitly allowed.

## Completion Brief Checklist

Report:

- files changed;
- helper API shape;
- what placement input moved and what remains in `main.cpp`;
- CMake source-list placement;
- focused build/CTest and diff/whitespace results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`, capture
  scenario move, Move policy move, later frame-stage move, tests,
  receipt/golden files, broad CTest, interactive window launch, staging,
  commit, or push was performed.
