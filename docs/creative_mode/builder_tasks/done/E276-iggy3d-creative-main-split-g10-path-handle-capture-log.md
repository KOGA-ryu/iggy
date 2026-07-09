# E276: iggy3d_creative Main Split G10 - Path Handle Capture Log

## Objective

Move the path-handle capture proof logging block from
`apps/iggy3d_creative/main.cpp` into the existing
`CreativeEditorGizmoFrame.*` helper module.

This is a mechanical extraction only. Preserve capture gating, log strings,
path-handle log order, latch behavior, downstream Move behavior, object ids,
frame numbers, render output, and focused test results.

## Current Context

E267 moved `appendWireframeBoxEdges(...)` into
`StandaloneWireframeBoxEdges.*`.

E268 introduced header-only `CreativeEditorState`.

E269 extracted frame-begin event/drawable/resize/fly-camera input into
`CreativeEditorFrameInput.*`.

E270 moved the shared `deleteSelectedObject(...)` helper into
`StandaloneDelete.*`.

E271 extracted the interactive command-key block into
`CreativeEditorCommandInput.*`.

E272 extracted aim-cell calculation into `CreativeEditorAim.*`.

E273 shared raw camera-ray-to-ground-point math through `CreativeEditorAim.*`.

E274 extracted selected-object id/object/bounds resolution into
`CreativeEditorSelection.*`.

E275 extracted selected-object gizmo frame geometry into
`CreativeEditorGizmoFrame.*`, including path-handle hit data. It intentionally
left path-handle capture logging in `main.cpp`.

Post-E275 current shape:

- `apps/iggy3d_creative/main.cpp`: 1332 lines.
- `CreativeEditorGizmoFrame` now carries:
  - `selectedPathHandleObjectId`
  - `selectedIsPathForHandles`
  - `pathPointHandleHits`
- `main.cpp` still owns the path-handle capture proof block:
  - checks `!capturePath.empty()`
  - checks `!editor.captureScript.pathPointHandleLogged`
  - checks selected path handle target id
  - logs one `PATH_HANDLE hit proxy` line per handle
  - sets `editor.captureScript.pathPointHandleLogged = true`

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorGizmoFrame.hpp`
- `apps/iggy3d_creative/CreativeEditorGizmoFrame.cpp`
- this task card when moving it to `done/`

Do not edit `CMakeLists.txt`; E275 already added
`CreativeEditorGizmoFrame.cpp` to the `iggy3d_creative` executable source list.

## Required Helper API

Add this helper in namespace `iggy3d_creative_app`:

```cpp
void logCreativeEditorPathHandleCaptureFrame(
    StandaloneCaptureScript& captureScript,
    bool captureMode,
    const CreativeEditorGizmoFrame& gizmoFrame);
```

Expected header dependencies:

- `StandaloneCaptureScript.hpp`

Expected implementation dependencies:

- `<SDL3/SDL.h>`

The helper should contain exactly the current path-handle capture proof policy:

- return unless `captureMode` is true;
- return when `captureScript.pathPointHandleLogged` is already true;
- return unless `gizmoFrame.selectedIsPathForHandles` is true;
- return unless `gizmoFrame.selectedPathHandleObjectId ==
  captureScript.pathTargetId`;
- iterate `gizmoFrame.pathPointHandleHits`;
- emit the existing `iggy3d_creative: PATH_HANDLE hit proxy ...` log string and
  all current values;
- set `captureScript.pathPointHandleLogged = true` after the loop.

## Required `main.cpp` Migration

In `main.cpp`:

- add `using iggy3d_creative_app::logCreativeEditorPathHandleCaptureFrame;`;
- replace only the current path-handle capture proof block with:

```cpp
logCreativeEditorPathHandleCaptureFrame(
    editor.captureScript, !capturePath.empty(), gizmoFrame);
```

If formatting differs slightly, preserve the same argument order and semantics.

Do not move capture scenario dispatch. Do not move capture Move, interactive
Move, placement, overlay construction, dimension label logic, final logging, or
any downstream gizmo/path consumer.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `!capturePath.empty()` gating, represented by `captureMode`;
- `!editor.captureScript.pathPointHandleLogged`;
- `selectedIsPathForHandles`;
- `selectedPathHandleObjectId == editor.captureScript.pathTargetId`;
- loop over `pathPointHandleHits`;
- the exact `SDL_Log(...)` format string:
  `iggy3d_creative: PATH_HANDLE hit proxy objectId=%llu pointIndex=%zu ...`;
- `static_cast<unsigned long long>(handle.objectId)`;
- `handle.pointIndex`;
- `handle.aabb.valid ? 1 : 0`;
- `handle.position.x/y/z`;
- `handle.aabb.minX/minY/maxX/maxY`;
- `editor.captureScript.pathPointHandleLogged = true`, represented through the
  `captureScript` reference.

Preserve:

- log order;
- latch behavior;
- all downstream capture Move, interactive Move, overlay, submit, and log
  behavior.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `CMakeLists.txt`;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

Do not move capture Move behavior.
Do not move interactive Move behavior.
Do not move placement behavior.
Do not move capture scenario dispatch or any capture scenario helper code.
Do not move click selection, object pick-candidate construction, capture
pick-proof logging, capture move script, overlay construction, frustum culling,
submit, shutdown, capture script frame numbers, object ids, seeded Floor/Crate
setup, save root setup, final capture proof strings, or render-submit reason
strings.
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
rg -n "logCreativeEditorPathHandleCaptureFrame|PATH_HANDLE hit proxy|pathPointHandleLogged|pathPointHandleHits|selectedPathHandleObjectId|selectedIsPathForHandles|CreativeEditorGizmoFrame" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorGizmoFrame.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorGizmoFrame.cpp
```

Expected:

- `CreativeEditorGizmoFrame.hpp` declares
  `logCreativeEditorPathHandleCaptureFrame(...)`;
- `CreativeEditorGizmoFrame.cpp` defines
  `logCreativeEditorPathHandleCaptureFrame(...)`;
- the `PATH_HANDLE hit proxy` log string lives in
  `CreativeEditorGizmoFrame.cpp`;
- path-handle capture gating and `pathPointHandleLogged` assignment live in
  `CreativeEditorGizmoFrame.cpp`;
- `main.cpp` calls `logCreativeEditorPathHandleCaptureFrame(...)`;
- capture Move, interactive Move, and overlay consumers remain in `main.cpp`.

Run:

```sh
rg -n "CreativeEditorSelection|CreativeEditorAim|CreativeEditorCommandInput|CreativeEditorFrameInput|StandaloneDelete|EditorFrame|runCreativeEditorFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- E274's `CreativeEditorSelection.*` remains the selection-frame owner;
- E273's `CreativeEditorAim.*` remains the aim/ground helper owner;
- E271's `CreativeEditorCommandInput.*` remains the command-key owner;
- E269's `CreativeEditorFrameInput.*` remains the frame-begin input owner;
- E270's `StandaloneDelete.*` remains the delete helper owner;
- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged;
- no old `appendWireframeBoxEdges(...)` helper was reintroduced.

Run:

```sh
git -C /Users/kogaryu/iggy3d diff -- CMakeLists.txt
```

Expected:

- no diff. `CMakeLists.txt` should not change in this slice.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorGizmoFrame.hpp`
- `apps/iggy3d_creative/CreativeEditorGizmoFrame.cpp`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e276_final.png --frames 32 > /tmp/iggy3d_creative_e276_final.log 2>&1)
rg "PATH_HANDLE|GIZMO|FINAL frame|submit outcome" /tmp/iggy3d_creative_e276_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes path-handle logging, log strings, log order, latch
  behavior, capture behavior, object ids, frame numbers, render-submit
  behavior, or downstream gizmo/path behavior;
- preserving the logging block requires moving capture Move, interactive Move,
  placement, capture scenario, selection, gizmo/path policy, renderer submit,
  capture behavior, or any later frame stage;
- CMake/source-list changes become necessary;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- what path-handle capture logging moved and what remains in `main.cpp`;
- confirmation that `CMakeLists.txt` was unchanged;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`, capture
  Move policy move, interactive Move policy move, placement move, capture
  scenario move, later frame-stage move, tests, receipt/golden files, broad
  CTest, interactive window launch, staging, commit, or push was performed.

## Completion Brief

Files changed:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorGizmoFrame.hpp`
- `apps/iggy3d_creative/CreativeEditorGizmoFrame.cpp`
- `docs/creative_mode/builder_tasks/done/E276-iggy3d-creative-main-split-g10-path-handle-capture-log.md`

Exact helper API added:

```cpp
void logCreativeEditorPathHandleCaptureFrame(
    StandaloneCaptureScript& captureScript,
    bool captureMode,
    const CreativeEditorGizmoFrame& gizmoFrame);
```

Moved path-handle capture logging:

- The capture-mode gate, `pathPointHandleLogged` latch gate, selected path
  target gate, `PATH_HANDLE hit proxy` log loop, and latch assignment now live
  in `CreativeEditorGizmoFrame.cpp`.
- `main.cpp` now calls
  `logCreativeEditorPathHandleCaptureFrame(editor.captureScript,
  !capturePath.empty(), gizmoFrame)`.
- Capture Move policy, interactive Move policy, placement, capture scenario
  dispatch, overlay construction, submit, shutdown, and downstream
  path/gizmo consumers remain in `main.cpp`.

`CMakeLists.txt` was unchanged.

Required grep classifications:

- `logCreativeEditorPathHandleCaptureFrame(...)` is declared in
  `CreativeEditorGizmoFrame.hpp`, defined in `CreativeEditorGizmoFrame.cpp`,
  and called from `main.cpp`.
- The `PATH_HANDLE hit proxy` log string, capture gating, and
  `pathPointHandleLogged` assignment live in `CreativeEditorGizmoFrame.cpp`.
- `main.cpp` still owns capture Move, interactive Move, and overlay consumers.
- Existing ownership remains unchanged for `CreativeEditorSelection.*`,
  `CreativeEditorAim.*`, `CreativeEditorCommandInput.*`,
  `CreativeEditorFrameInput.*`, `StandaloneDelete.*`, and
  `StandaloneWireframeBoxEdges.*`.
- No `EditorFrame` or `runCreativeEditorFrame(...)` was introduced.
- No old `appendWireframeBoxEdges(...)` helper was reintroduced.

Focused verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10`
  passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure`
  passed: 3/3 tests.
- `git -C /Users/kogaryu/iggy3d diff -- CMakeLists.txt` produced no diff.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over touched source files and this card
  passed.

Optional capture was skipped because no windowed/Vulkan capture check was
explicitly allowed.

No `EditorFrame`, `runCreativeEditorFrame(...)`, capture Move policy move,
interactive Move policy move, placement move, capture scenario move, later
frame-stage move, tests, receipt/golden files, broad CTest, interactive window
launch, staging, commit, or push was performed.
