# E272: iggy3d_creative Main Split G6 - Aim Ground Cell

## Objective

Extract the small camera-forward-to-ground-cell calculation from
`apps/iggy3d_creative/main.cpp` into an app-local helper module.

This is a mechanical extraction only. Preserve behavior, aim math, snapping,
placement preview/capture semantics, object ids, frame numbers, render output,
and focused test results.

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

Post-E271 current shape:

- `apps/iggy3d_creative/main.cpp`: 1393 lines.
- The frame loop still builds `FrameInput frame` in `main.cpp`.
- The in-scope aim block starts at the
  `// ---- AIM -> GROUND CELL (Place mode) --------------------------------`
  comment.
- The aim block casts `frame.camera.worldEye` and
  `frame.camera.worldForward` to the Y=0 ground plane, falls back to the point
  under the camera eye when the ray is near-parallel or points away, then calls
  `snapGroundToCellCenter(...)`.
- `aimCellCenter` is used later by placement, capture placement requests, path
  previews, and the green placement ghost. Those later consumers must stay in
  `main.cpp` in this slice.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorAim.hpp`
- new `apps/iggy3d_creative/CreativeEditorAim.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/CreativeEditor*.cpp` helper files.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::Vec3 resolveCreativeEditorAimCell(
    const iggy3d::RenderCameraFrame& camera,
    double placeCellSize);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `core/math/Vec3.hpp`
- `render/FrameInput.hpp`

Expected implementation dependencies:

- `<cmath>`
- `StandalonePlacement.hpp`

The implementation should move the current aim math exactly, adjusted only for
the helper API and function name.

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorAim.hpp`;
- add `using iggy3d_creative_app::resolveCreativeEditorAimCell;`;
- replace only the current aim-ground-cell block with:

```cpp
const Vec3 aimCellCenter =
    resolveCreativeEditorAimCell(frame.camera, editor.placeCellSize);
```

If formatting differs slightly, preserve the same argument order and semantics.

Do not move the `FrameInput frame` construction. Do not move any placement,
capture scenario, path preview, ghost preview, click-selection, or movement
code that consumes `aimCellCenter`.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `frame.camera.worldEye`;
- `frame.camera.worldForward`;
- fallback initialization of `aimGroundX` and `aimGroundZ` from `aimEye`;
- near-parallel threshold `1.0e-4F`;
- `std::fabs(aimFwd.y)`;
- `const float t = -aimEye.y / aimFwd.y`;
- update of `aimGroundX` and `aimGroundZ` only when `t > 0.0F`;
- `snapGroundToCellCenter(aimGroundX, aimGroundZ, placeCellSize)`.

Preserve:

- the exact ray-to-Y=0 math;
- the fallback behavior when the ray is near-parallel or points away from the
  ground plane;
- the snapping behavior and cell size source;
- all downstream `aimCellCenter` consumers;
- all placement, capture, preview, selection, movement, render-submit, and log
  behavior.

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
Do not move click selection, object pick-candidate construction, capture
pick-proof logging, capture move script, gizmo/path policy, overlay
construction, frustum culling, submit, shutdown, capture script frame numbers,
object ids, seeded Floor/Crate setup, save root setup, final capture proof
strings, or render-submit reason strings.
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
rg -n "resolveCreativeEditorAimCell|CreativeEditorAim|AIM -> GROUND CELL|aimGround|aimCellCenter|snapGroundToCellCenter|std::fabs" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorAim.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorAim.cpp
```

Expected:

- `CreativeEditorAim.hpp` declares `resolveCreativeEditorAimCell(...)`;
- `CreativeEditorAim.cpp` defines `resolveCreativeEditorAimCell(...)`;
- the moved `aimGround*`, `snapGroundToCellCenter(...)`, and aim-block
  `std::fabs(...)` logic live in `CreativeEditorAim.cpp`;
- `main.cpp` includes `CreativeEditorAim.hpp`, has the `using` declaration, and
  calls `resolveCreativeEditorAimCell(...)`;
- `main.cpp` still owns the `aimCellCenter` local and all downstream placement,
  capture, path preview, and ghost preview consumers;
- any remaining `std::fabs(...)` hit in `main.cpp` belongs to later
  movement/path drag logic and must remain there.

Run:

```sh
rg -n "CreativeEditorCommandInput|CreativeEditorFrameInput|StandaloneDelete|EditorFrame|runCreativeEditorFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- E271's `CreativeEditorCommandInput.*` remains the command-key owner;
- E269's `CreativeEditorFrameInput.*` remains the frame-begin input owner;
- E270's `StandaloneDelete.*` remains the delete helper owner;
- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged;
- no old `appendWireframeBoxEdges(...)` helper was reintroduced.

Run:

```sh
rg -n "CreativeEditorAim.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
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
- `apps/iggy3d_creative/CreativeEditorAim.hpp`
- `apps/iggy3d_creative/CreativeEditorAim.cpp`
- `CMakeLists.txt`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e272_final.png --frames 32 > /tmp/iggy3d_creative_e272_final.log 2>&1)
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e272_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes aim math, snapping behavior, placement behavior,
  capture behavior, object ids, log strings, frame numbers, render-submit
  behavior, or downstream preview behavior;
- preserving `aimCellCenter` requires moving placement, capture scenario,
  selection, gizmo/path policy, renderer submit, capture behavior, or any later
  frame stage;
- CMake/source-list changes affect targets other than `iggy3d_creative`;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- what aim math moved and what remains in `main.cpp`;
- CMake source-list placement;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`,
  placement move, capture scenario move, later frame-stage move, tests,
  receipt/golden files, broad CTest, interactive window launch, staging,
  commit, or push was performed.

## Completion Brief

Files changed:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorAim.hpp`
- `apps/iggy3d_creative/CreativeEditorAim.cpp`
- `CMakeLists.txt`
- this task card, moved from `ready/` to `claimed/` and then `done/`

Exact helper API added:

```cpp
namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::Vec3 resolveCreativeEditorAimCell(
    const iggy3d::RenderCameraFrame& camera,
    double placeCellSize);

}  // namespace iggy3d_creative_app
```

The moved aim math is the camera `worldEye` / `worldForward` ray cast to the
Y=0 plane, including the `std::fabs(aimFwd.y) > 1.0e-4F` near-parallel gate,
the positive-`t` update, and `snapGroundToCellCenter(...)`. `main.cpp` still
owns the `FrameInput frame` construction, the `aimCellCenter` local, and all
downstream placement, capture, path preview, ghost preview, selection,
movement, render-submit, and logging consumers.

CMake source-list placement:

- `apps/iggy3d_creative/CreativeEditorAim.cpp` was added to the
  `iggy3d_creative` executable source list next to the other
  `CreativeEditor*.cpp` helper sources.

Required grep classifications:

- `CreativeEditorAim.hpp` declares `resolveCreativeEditorAimCell(...)`.
- `CreativeEditorAim.cpp` defines `resolveCreativeEditorAimCell(...)` and owns
  the moved `aimGround*`, aim-block `std::fabs(...)`, and
  `snapGroundToCellCenter(...)` logic.
- `main.cpp` includes `CreativeEditorAim.hpp`, has the `using` declaration,
  calls `resolveCreativeEditorAimCell(frame.camera, editor.placeCellSize)`,
  and keeps `aimCellCenter` plus all downstream consumers.
- The remaining `std::fabs(...)` hit in `main.cpp` belongs to later
  movement/path drag logic and was left in place.
- `CreativeEditorCommandInput.*`, `CreativeEditorFrameInput.*`, and
  `StandaloneDelete.*` remain the command-key, frame-begin input, and delete
  helper owners.
- No `EditorFrame` or `runCreativeEditorFrame(...)` was introduced.
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged,
  and no old `appendWireframeBoxEdges(...)` helper was reintroduced.
- `CreativeEditorAim.cpp` appears only in the `iggy3d_creative` executable
  source list.

Focused verification:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Results:

- focused build passed
- focused CTest passed, 3/3 tests
- `git diff --check` passed
- focused trailing-whitespace scan over touched source files, `CMakeLists.txt`,
  and this task card passed

Optional capture:

- skipped; no owner explicitly allowed a windowed/Vulkan capture check for this
  slice

No `EditorFrame`, `runCreativeEditorFrame(...)`, placement move, capture
scenario move, later frame-stage move, tests, receipt/golden files, broad
CTest, interactive window launch, staging, commit, or push was performed.
