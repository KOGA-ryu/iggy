# E279: iggy3d_creative Main Split G13 - Click Selection

## Objective

Extract the click-selection block from `apps/iggy3d_creative/main.cpp` into a
new app-local `CreativeEditorClickSelection.*` helper module.

This is a mechanical extraction only. Preserve synthetic capture click behavior,
interactive Alt-click behavior, mouse-mode behavior, high-DPI scaling,
world-pick behavior, log strings, selection dispatch behavior, object ids, frame
numbers, render output, and focused test results.

## Current Context

E277 extracted visible-object pick candidate construction into
`CreativeEditorPickFrame.*`.

E278 moved capture-only world-pick proof logging into
`CreativeEditorPickFrame.*`.

Post-E278 current shape:

- `apps/iggy3d_creative/main.cpp` still owns click selection immediately after
  `logCreativeEditorWorldPickProofFrame(...)`.
- That block computes `clickRequested`, `clickX`, and `clickY`, then uses
  `worldRayFromPixel(...)`, `pickNearestVisualBoundsObject(...)`, logs
  `WORLD_PICK`, and dispatches a primary `PointerPress` through
  `appState.facade.dispatchToolInput(...)`.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorClickSelection.hpp`
- new `apps/iggy3d_creative/CreativeEditorClickSelection.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/CreativeEditor*.cpp` helper files.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

void applyCreativeEditorClickSelection(
    iggy3d::SdlWindow& window,
    iggy3d::creative::CreativeAppState& appState,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    CreativeEditorState& editor,
    bool captureMode);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `<cstdint>`
- `app/iggy3d/creative/CreativeAppState.hpp`
- `app/platform/SdlWindow.hpp`
- `render/FrameInput.hpp`
- `CreativeEditorPickFrame.hpp`
- `CreativeEditorState.hpp`

Expected implementation dependencies:

- `<SDL3/SDL.h>`
- `StandalonePicking.hpp`

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorClickSelection.hpp`;
- add `using iggy3d_creative_app::applyCreativeEditorClickSelection;`;
- replace only the current click-selection block, from
  `bool clickRequested = false;` through
  `(void)appState.facade.dispatchToolInput(packet);`, with:

```cpp
applyCreativeEditorClickSelection(window,
                                  appState,
                                  frame.camera,
                                  extent.width,
                                  extent.height,
                                  pickFrame,
                                  editor,
                                  !capturePath.empty());
```

If formatting differs slightly, preserve the same argument order and semantics.

Do not move placement, capture scenario dispatch, selection resolution, gizmo,
Move, overlay, submit, shutdown, or final logging code.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `clickRequested`, `clickX`, and `clickY` initialization;
- capture place-mode gate: no synthetic select-click when
  `captureMode && editor.placeMode`;
- capture synthetic floor click gate:
  `editor.frameIndex == 3U && pickFrame.haveFloorBounds`;
- floor top-corner math with `0.85F`;
- `projectPointToScreen(camera.clipFromWorld, floorTopCorner, drawableWidth,
  drawableHeight)`;
- setting `clickRequested`, `clickX`, and `clickY` only when projected point is
  valid;
- interactive gate: `!captureMode && !editor.placeMode`;
- `SDL_GetKeyboardState(nullptr)` and Left Alt check;
- `window.setRelativeMouseMode(false)` while Alt is held;
- `SDL_GetMouseState(&mx, &my)`;
- left-button mask check;
- high-DPI scaling from `window.eventState().windowWidth/windowHeight` to
  drawable dimensions;
- `window.setRelativeMouseMode(true)` when Alt is not held;
- click handling:
  - `worldRayFromPixel(camera, clickX, clickY, drawableWidth, drawableHeight)`;
  - `pickNearestVisualBoundsObject(pickFrame.objectPickCandidates, ray)`;
  - exact `WORLD_PICK` log string and values;
  - `CreativeToolInputPacket` primary pointer press;
  - target assignment only when `pickedId != creative::kInvalidObjectId`;
  - `appState.facade.dispatchToolInput(packet)`.

Preserve all downstream placement, capture scenario, selection, gizmo, Move,
overlay, submit, and final logging behavior.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

Do not move placement behavior.
Do not move capture scenario dispatch or any capture scenario helper code.
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
rg -n "applyCreativeEditorClickSelection|WORLD_PICK|clickRequested|clickX|clickY|SDL_GetKeyboardState|SDL_GetMouseState|setRelativeMouseMode|dispatchToolInput|pickNearestVisualBoundsObject|worldRayFromPixel" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorClickSelection.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorClickSelection.cpp
```

Expected:

- `CreativeEditorClickSelection.hpp` declares
  `applyCreativeEditorClickSelection(...)`;
- `CreativeEditorClickSelection.cpp` defines
  `applyCreativeEditorClickSelection(...)`;
- synthetic capture click, interactive Alt-click, mouse-mode handling,
  high-DPI scaling, `WORLD_PICK` logging, and pointer-press dispatch live in
  `CreativeEditorClickSelection.cpp`;
- `main.cpp` calls `applyCreativeEditorClickSelection(...)`;
- placement, capture scenario, Move, overlay, and submit remain in `main.cpp`.

Run:

```sh
rg -n "CreativeEditorPickFrame|CreativeEditorGizmoFrame|CreativeEditorSelection|CreativeEditorAim|CreativeEditorCommandInput|CreativeEditorFrameInput|StandaloneDelete|EditorFrame|runCreativeEditorFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- existing CreativeEditor and Standalone helper ownership remains unchanged;
- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- no old `appendWireframeBoxEdges(...)` helper was reintroduced.

Run:

```sh
rg -n "CreativeEditorClickSelection.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
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

Also run a focused trailing-whitespace scan over:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorClickSelection.hpp`
- `apps/iggy3d_creative/CreativeEditorClickSelection.cpp`
- `CMakeLists.txt`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e279_final.png --frames 32 > /tmp/iggy3d_creative_e279_final.log 2>&1)
rg "WORLD_PICK|FINAL frame|submit outcome" /tmp/iggy3d_creative_e279_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes click selection, mouse-mode behavior, high-DPI
  scaling, pick behavior, log strings, dispatch behavior, capture behavior,
  object ids, frame numbers, render-submit behavior, or downstream behavior;
- preserving click selection requires moving placement, capture scenario,
  selection, gizmo/path policy, renderer submit, capture behavior, or any later
  frame stage;
- CMake/source-list changes affect targets other than `iggy3d_creative`;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- what click-selection logic moved and what remains in `main.cpp`;
- CMake source-list placement;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`,
  placement move, capture scenario move, capture Move policy move, interactive
  Move policy move, later frame-stage move, tests, receipt/golden files, broad
  CTest, interactive window launch, staging, commit, or push was performed.
