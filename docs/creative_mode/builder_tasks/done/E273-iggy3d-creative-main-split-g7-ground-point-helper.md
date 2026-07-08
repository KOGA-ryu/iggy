# E273: iggy3d_creative Main Split G7 - Ground Point Helper

## Objective

Extend the `CreativeEditorAim.*` helper module so the remaining camera-forward
to Y=0 ground-point math in `apps/iggy3d_creative/main.cpp` is shared with the
aim-cell helper.

This is a mechanical extraction only. Preserve behavior, ground-ray math,
snapping, interactive move semantics, placement preview/capture semantics,
object ids, frame numbers, render output, and focused test results.

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

E272 extracted `resolveCreativeEditorAimCell(...)` into `CreativeEditorAim.*`.
That helper now owns the placement/capture preview snap-to-cell calculation, but
`main.cpp` still contains the same camera-forward ray to Y=0 ground-plane math
inside the interactive Move branch.

Post-E272 current shape:

- `apps/iggy3d_creative/main.cpp`: 1377 lines.
- `CreativeEditorAim.hpp`: declares `resolveCreativeEditorAimCell(...)`.
- `CreativeEditorAim.cpp`: owns the aim-cell ray-to-ground math and
  `snapGroundToCellCenter(...)` call.
- `main.cpp` still has one `std::fabs(fwd.y) > 1.0e-4F` ground-plane block in
  the interactive Move path.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorAim.hpp`
- `apps/iggy3d_creative/CreativeEditorAim.cpp`
- this task card when moving it to `done/`

Do not edit `CMakeLists.txt`; E272 already added `CreativeEditorAim.cpp` to the
`iggy3d_creative` executable source list.

## Required Helper API

Add a second helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeToolWorldPoint
resolveCreativeEditorGroundPoint(const iggy3d::RenderCameraFrame& camera);

[[nodiscard]] iggy3d::Vec3 resolveCreativeEditorAimCell(
    const iggy3d::RenderCameraFrame& camera,
    double placeCellSize);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `app/iggy3d/creative/tools/Tools.hpp`
- `core/math/Vec3.hpp`
- `render/FrameInput.hpp`

The new helper should contain the raw camera ray to Y=0 plane calculation:

- read `camera.worldEye`;
- read `camera.worldForward`;
- initialize the fallback point from `eye.x`, `0.0`, `eye.z`;
- use near-parallel threshold `1.0e-4F`;
- use `std::fabs(fwd.y)`;
- compute `const float t = -eye.y / fwd.y`;
- update `ground.x` and `ground.z` only when `t > 0.0F`;
- keep `ground.y` at `0.0`.

Then update `resolveCreativeEditorAimCell(...)` to call
`resolveCreativeEditorGroundPoint(camera)` and pass `ground.x` and `ground.z`
to `snapGroundToCellCenter(...)`.

## Required `main.cpp` Migration

In `main.cpp`:

- add `using iggy3d_creative_app::resolveCreativeEditorGroundPoint;`;
- replace only the interactive Move branch's local ground-plane math with:

```cpp
const creative::CreativeToolWorldPoint ground =
    resolveCreativeEditorGroundPoint(frame.camera);
```

If formatting differs slightly, preserve the same argument order and semantics.

Do not move the interactive Move branch. Do not move path handling, gizmo
handling, pointer packet construction, selection, placement, capture scenario,
overlay construction, or render-submit code.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `frame.camera.worldEye`;
- `frame.camera.worldForward`;
- fallback `CreativeToolWorldPoint ground{eye.x, 0.0, eye.z}`;
- near-parallel threshold `1.0e-4F`;
- `std::fabs(fwd.y)`;
- `const float t = -eye.y / fwd.y`;
- update of `ground.x` and `ground.z` only when `t > 0.0F`.

Preserve:

- exact ray-to-Y=0 math;
- fallback behavior when the ray is near-parallel or points away from the
  ground plane;
- `ground.y == 0.0`;
- `resolveCreativeEditorAimCell(...)` output;
- all downstream placement, capture, preview, path move, path-point move,
  free-move, gizmo move, render-submit, and log behavior.

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

Do not move interactive Move behavior.
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
rg -n "resolveCreativeEditorGroundPoint|resolveCreativeEditorAimCell|CreativeEditorAim|ground\\{|std::fabs|snapGroundToCellCenter" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorAim.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorAim.cpp
```

Expected:

- `CreativeEditorAim.hpp` declares both helper functions;
- `CreativeEditorAim.cpp` defines both helper functions;
- raw ground-plane `ground{...}`, `std::fabs(...)`, threshold, and positive-`t`
  logic live in `CreativeEditorAim.cpp`;
- `resolveCreativeEditorAimCell(...)` calls
  `resolveCreativeEditorGroundPoint(...)` and still calls
  `snapGroundToCellCenter(...)`;
- `main.cpp` uses `resolveCreativeEditorAimCell(...)` for `aimCellCenter`;
- `main.cpp` uses `resolveCreativeEditorGroundPoint(...)` for the interactive
  Move branch ground point;
- no raw `std::fabs(...)` ground-plane block remains in `main.cpp`.

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
- `apps/iggy3d_creative/CreativeEditorAim.hpp`
- `apps/iggy3d_creative/CreativeEditorAim.cpp`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e273_final.png --frames 32 > /tmp/iggy3d_creative_e273_final.log 2>&1)
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e273_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes ground math, aim-cell snapping behavior, interactive
  move behavior, placement behavior, capture behavior, object ids, log strings,
  frame numbers, render-submit behavior, or downstream preview behavior;
- preserving the ground point requires moving placement, capture scenario,
  selection, gizmo/path policy, renderer submit, capture behavior, or any later
  frame stage;
- CMake/source-list changes become necessary;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- what ground math moved and what remains in `main.cpp`;
- confirmation that `CMakeLists.txt` was unchanged;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`,
  interactive Move policy move, placement move, capture scenario move, later
  frame-stage move, tests, receipt/golden files, broad CTest, interactive
  window launch, staging, commit, or push was performed.

## Completion Brief

Files changed:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorAim.hpp`
- `apps/iggy3d_creative/CreativeEditorAim.cpp`
- this task card, moved from `ready/` to `claimed/` and then `done/`

Exact helper API shape:

```cpp
namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeToolWorldPoint
resolveCreativeEditorGroundPoint(const iggy3d::RenderCameraFrame& camera);

[[nodiscard]] iggy3d::Vec3 resolveCreativeEditorAimCell(
    const iggy3d::RenderCameraFrame& camera,
    double placeCellSize);

}  // namespace iggy3d_creative_app
```

The raw camera-forward to Y=0 ground-plane math moved into
`resolveCreativeEditorGroundPoint(...)`, including `camera.worldEye`,
`camera.worldForward`, fallback `ground{eye.x, 0.0, eye.z}`, the
`std::fabs(fwd.y) > 1.0e-4F` near-parallel gate, positive-`t` update, and
`ground.y == 0.0` preservation. `resolveCreativeEditorAimCell(...)` now calls
the ground-point helper and passes `ground.x` / `ground.z` to
`snapGroundToCellCenter(...)`.

`main.cpp` still owns the interactive Move branch, path handling, gizmo
handling, pointer packet construction, selection, placement, capture scenario,
overlay construction, render-submit, and all downstream consumers. It now uses
`resolveCreativeEditorAimCell(...)` for `aimCellCenter` and
`resolveCreativeEditorGroundPoint(...)` for the interactive Move branch ground
point.

`CMakeLists.txt` was unchanged; `git diff -- CMakeLists.txt` produced no diff.

Required grep classifications:

- `CreativeEditorAim.hpp` declares both helper functions.
- `CreativeEditorAim.cpp` defines both helper functions.
- raw ground-plane `ground{...}`, `std::fabs(...)`, threshold, and positive-`t`
  logic live in `CreativeEditorAim.cpp`.
- `resolveCreativeEditorAimCell(...)` calls
  `resolveCreativeEditorGroundPoint(...)` and still calls
  `snapGroundToCellCenter(...)`.
- `main.cpp` uses `resolveCreativeEditorAimCell(...)` for `aimCellCenter`.
- `main.cpp` uses `resolveCreativeEditorGroundPoint(...)` for the interactive
  Move branch ground point.
- no raw `std::fabs(...)` ground-plane block remains in `main.cpp`.
- `CreativeEditorCommandInput.*`, `CreativeEditorFrameInput.*`, and
  `StandaloneDelete.*` remain the command-key, frame-begin input, and delete
  helper owners.
- no `EditorFrame` or `runCreativeEditorFrame(...)` was introduced.
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged,
  and no old `appendWireframeBoxEdges(...)` helper was reintroduced.

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
- focused trailing-whitespace scan over touched source files and this task
  card passed

Optional capture:

- skipped; no owner explicitly allowed a windowed/Vulkan capture check for this
  slice

No `EditorFrame`, `runCreativeEditorFrame(...)`, interactive Move policy move,
placement move, capture scenario move, later frame-stage move, tests,
receipt/golden files, broad CTest, interactive window launch, staging, commit,
or push was performed.
