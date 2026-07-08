# E269: iggy3d_creative Main Split G3 - Frame Begin Input

## Objective

Extract the first real frame-loop stage from
`apps/iggy3d_creative/main.cpp`: frame-begin event polling, drawable/resize
handling, and fly-camera keyboard/mouse input.

This is a mechanical extraction only. Preserve behavior, loop control, camera
math, resize behavior, downstream keyboard-state use, log strings, capture
timing, object ids, frame numbers, render output, and focused test results.

## Current Context

E266 mapped `apps/iggy3d_creative/main.cpp` and identified frame input as a
real frame-stage seam, but too risky before a state shell existed.

E267 moved only `appendWireframeBoxEdges(...)` into
`StandaloneWireframeBoxEdges.*`.

E268 introduced header-only `CreativeEditorState` and moved mutable editor
locals behind `editor.<field>` without moving any frame stages.

Post-E268 current shape:

- `apps/iggy3d_creative/main.cpp`: 1583 lines.
- `CreativeEditorState.hpp`: 79 lines.
- The frame loop starts at `while (window.isOpen())`.
- The current frame-begin block performs, in order:
  1. `window.pollEvents()`;
  2. quit check;
  3. non-drawable sleep and `continue`;
  4. drawable extent read;
  5. zero-size extent sleep and `continue`;
  6. resize when extent differs from `editor.lastWidth/lastHeight`;
  7. `SDL_GetKeyboardState(nullptr)`;
  8. build `ProductCreativeFlyInput`;
  9. `SDL_GetRelativeMouseState(...)`;
  10. update `editor.yawDegrees` and `editor.pitchDegrees`;
  11. call `applyProductCreativeFlyInput(...)`;
  12. update `editor.flyPos` when applied.

The downstream tool-switch block still needs the same `const bool* keys`
pointer returned by `SDL_GetKeyboardState(nullptr)`.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorFrameInput.hpp`
- new `apps/iggy3d_creative/CreativeEditorFrameInput.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/*` helper sources.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

struct CreativeEditorFrameInputResult {
  bool keepRunning = true;
  bool skipFrame = false;
  iggy3d::SdlDrawableExtent extent{};
  const bool* keyboardState = nullptr;
};

CreativeEditorFrameInputResult beginCreativeEditorFrameInput(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorState& editor);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `app/platform/SdlWindow.hpp`
- `CreativeEditorState.hpp`

Forward-declare `iggy3d::VulkanBackend` in the header if practical; include
`render/vulkan/VulkanBackend.hpp` in the `.cpp`.

Expected implementation dependencies include the current frame-begin block's
real dependencies:

- `<algorithm>`
- `<chrono>`
- `<thread>`
- SDL keyboard/mouse APIs
- `app/iggy3d/creative/camera/Fly.hpp`
- `render/FrameInput.hpp`
- `render/vulkan/VulkanBackend.hpp`

Use a file-local constant for mouse sensitivity inside
`CreativeEditorFrameInput.cpp`:

```cpp
constexpr float kMouseSensitivity = 0.12F;
```

Remove the now-unused `kMouseSensitivity` local from `main.cpp`.

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorFrameInput.hpp`;
- add `using iggy3d_creative_app::beginCreativeEditorFrameInput;`;
- replace only the existing frame-begin block from `window.pollEvents()` through
  the fly-camera update with a helper call;
- preserve the `while (window.isOpen())` loop in `main.cpp`;
- preserve downstream variable names enough that later code keeps using:

```cpp
const CreativeEditorFrameInputResult frameInput =
    beginCreativeEditorFrameInput(window, *backend, editor);
if (!frameInput.keepRunning) {
  break;
}
if (frameInput.skipFrame) {
  continue;
}
const SdlDrawableExtent extent = frameInput.extent;
const bool* keys = frameInput.keyboardState;
```

If local naming differs slightly, keep behavior identical and keep the returned
extent and keyboard state explicit in `main.cpp`.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `window.pollEvents()`;
- `window.eventState().quitRequested`;
- `window.isDrawable()`;
- `std::this_thread::sleep_for(std::chrono::milliseconds(16))`;
- `window.drawableExtent()`;
- `extent.width == 0U || extent.height == 0U`;
- resize check against `editor.lastWidth` and `editor.lastHeight`;
- `RenderViewport` field writes, including `aspectRatio`;
- `backend.resize(viewport)`;
- `editor.lastWidth` / `editor.lastHeight` assignment;
- `SDL_GetKeyboardState(nullptr)`;
- WASD/Space/LCtrl/LShift fly input mapping;
- `SDL_GetRelativeMouseState(&mouseDx, &mouseDy)`;
- mouse sensitivity `0.12F`;
- pitch clamp `[-80.0F, 80.0F]`;
- `applyProductCreativeFlyInput(editor.flyConfig, flyInput, editor.flyPos)`;
- `editor.flyPos = flyResult.finalPositionMeters` only when applied.

Preserve loop control exactly:

- quit requested still exits the loop by causing `main.cpp` to `break`;
- non-drawable and zero-size drawable still sleep 16 ms and cause `main.cpp` to
  `continue`;
- downstream tool-switch/save/load/delete/undo logic still sees the same
  keyboard state pointer semantics as before.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

Do not create `EditorFrame.{hpp,cpp}`.
Do not create `runCreativeEditorFrame(...)`.
Do not move tool-switch/save/load/delete/undo key handling.
Do not move scene construction, aim-cell resolution, picking, placement,
capture scenario dispatch, selection, gizmo/path policy, overlay construction,
frustum culling, submit, shutdown, logging, capture script frame numbers,
object ids, seeded Floor/Crate setup, save root, final capture proof strings,
or render-submit reason strings.
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
rg -n "CreativeEditorFrameInputResult|beginCreativeEditorFrameInput|window\\.pollEvents|isDrawable|drawableExtent|SDL_GetKeyboardState|SDL_GetRelativeMouseState|applyProductCreativeFlyInput|kMouseSensitivity|backend->resize|backend\\.resize" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorFrameInput.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorFrameInput.cpp
```

Expected:

- `CreativeEditorFrameInputResult` is declared in
  `CreativeEditorFrameInput.hpp`;
- `beginCreativeEditorFrameInput(...)` is declared in the header, defined in
  the `.cpp`, and called from `main.cpp`;
- `window.pollEvents()`, drawable checks, resize handling, SDL keyboard/mouse
  sampling, `kMouseSensitivity`, and `applyProductCreativeFlyInput(...)` live in
  `CreativeEditorFrameInput.cpp`;
- `main.cpp` keeps the loop, handles `keepRunning`/`skipFrame`, and keeps
  explicit `extent` and `keys` locals for downstream code.

Run:

```sh
rg -n "EditorFrame|runCreativeEditorFrame|CreativeEditorState|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- E268's `CreativeEditorState` remains the state shell;
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged;
- no old `appendWireframeBoxEdges(...)` helper was reintroduced.

Run:

```sh
rg -n "CreativeEditorFrameInput.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
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
- `apps/iggy3d_creative/CreativeEditorFrameInput.hpp`
- `apps/iggy3d_creative/CreativeEditorFrameInput.cpp`
- `CMakeLists.txt`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e269_final.png --frames 32 > /tmp/iggy3d_creative_e269_final.log 2>&1)
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e269_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the extraction changes loop control, resize behavior, camera math, keyboard
  pointer semantics, log strings, frame numbers, object ids, capture script
  behavior, or render-submit behavior;
- preserving downstream key handling requires moving tool-switch/save/load
  policy into the new helper;
- compile fallout requires moving scene construction, selection, placement,
  gizmo/path policy, renderer submit, capture behavior, or any later frame
  stage;
- CMake/source-list changes affect targets other than `iggy3d_creative`;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- whether loop-control behavior stayed in `main.cpp`;
- how `extent` and `keys` are returned to downstream code;
- CMake source-list placement;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`, later
  frame-stage move, tool-switch/save/load policy move, tests, receipt/golden
  files, broad CTest, interactive window launch, staging, commit, or push was
  performed.

## Completion Brief

Files changed:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorFrameInput.hpp`
- `apps/iggy3d_creative/CreativeEditorFrameInput.cpp`
- `CMakeLists.txt`
- this task card, moved to `done/`

Helper API:

```cpp
namespace iggy3d_creative_app {

struct CreativeEditorFrameInputResult {
  bool keepRunning = true;
  bool skipFrame = false;
  iggy3d::SdlDrawableExtent extent{};
  const bool* keyboardState = nullptr;
};

CreativeEditorFrameInputResult beginCreativeEditorFrameInput(
    iggy3d::SdlWindow& window,
    iggy3d::VulkanBackend& backend,
    CreativeEditorState& editor);

}  // namespace iggy3d_creative_app
```

Extraction summary:

- Moved frame-begin event polling, quit check, drawable/zero-size sleeps,
  resize handling, SDL frame-begin keyboard/mouse sampling, mouse sensitivity,
  pitch clamp, fly input construction, and fly position update into
  `CreativeEditorFrameInput.cpp`.
- Kept the `while (window.isOpen())` loop in `main.cpp`.
- Kept loop-control decisions in `main.cpp` through
  `frameInput.keepRunning` and `frameInput.skipFrame`.
- Returned `extent` and `keyboardState` explicitly and restored them in
  `main.cpp` as `const SdlDrawableExtent extent` and `const bool* keys` for
  downstream code.

CMake:

- Added `apps/iggy3d_creative/CreativeEditorFrameInput.cpp` to the
  `iggy3d_creative` executable source list, directly after
  `apps/iggy3d_creative/main.cpp`.
- No other target source list was changed.

Required grep classifications:

- `CreativeEditorFrameInputResult` is declared in
  `CreativeEditorFrameInput.hpp`.
- `beginCreativeEditorFrameInput(...)` is declared in the header, defined in
  `CreativeEditorFrameInput.cpp`, and called from `main.cpp`.
- `window.pollEvents()`, drawable checks, resize handling, frame-begin
  `SDL_GetKeyboardState(nullptr)`, `SDL_GetRelativeMouseState(...)`,
  `kMouseSensitivity`, and `applyProductCreativeFlyInput(...)` live in
  `CreativeEditorFrameInput.cpp`.
- `main.cpp` retains the loop, `keepRunning`/`skipFrame` handling, and explicit
  `extent`/`keys` locals. Remaining `SDL_GetKeyboardState(...)` hits in
  `main.cpp` are later selection/place/move input branches left in place by
  this card.
- No `EditorFrame` or `runCreativeEditorFrame(...)` was introduced.
- E268's `CreativeEditorState` remains the state shell.
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged; no
  old `appendWireframeBoxEdges(...)` helper was reintroduced.
- `CreativeEditorFrameInput.cpp` appears only in the `iggy3d_creative`
  executable source list.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10`
  passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure`
  passed: 3/3 tests.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan passed for touched source files,
  `CMakeLists.txt`, and this card.

Optional capture:

- Skipped. No owner explicitly allowed a windowed/Vulkan capture check.

Not performed:

- No `EditorFrame`, `runCreativeEditorFrame(...)`, later frame-stage move,
  tool-switch/save/load policy move, tests, receipt/golden edits, broad CTest,
  interactive window launch, staging, commit, or push.
