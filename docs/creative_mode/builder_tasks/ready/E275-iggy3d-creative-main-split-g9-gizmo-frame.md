# E275: iggy3d_creative Main Split G9 - Gizmo Frame

## Objective

Extract the selected-object gizmo frame geometry from
`apps/iggy3d_creative/main.cpp` into an app-local helper module.

This is a mechanical extraction only. Preserve gizmo center, shaft tips/colors,
screen projections, path-handle hit boxes, gizmo anchor selection, downstream
capture logging, move behavior, object ids, frame numbers, render output, and
focused test results.

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

Post-E274 current shape:

- `apps/iggy3d_creative/main.cpp`: 1363 lines.
- `CreativeEditorSelectionFrame` now supplies `selectedId`, `selected`,
  `hasSelection`, `selBoxMin`, and `selBoxMax`.
- `main.cpp` still builds gizmo frame geometry directly after selection:
  - `gizmoCenter`
  - `gizmoShafts`
  - projected `gizmoCenterScreen`
  - projected `gizmoTipScreen`
  - `pathPointHandleHits`
  - `selectedPathHandleObjectId`
  - `selectedIsPathForHandles`
  - `gizmoAnchorS`
- The path-handle capture logging and all Move policy still belong in
  `main.cpp` for this slice.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorGizmoFrame.hpp`
- new `apps/iggy3d_creative/CreativeEditorGizmoFrame.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/CreativeEditor*.cpp` helper files.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

struct CreativeEditorGizmoFrame {
  iggy3d::Vec3 center{0.0F, 0.0F, 0.0F};
  std::array<GizmoAxisShaft, 3> shafts{};
  ScreenPoint centerScreen;
  std::array<ScreenPoint, 3> tipScreens{};
  std::vector<PathPointHandleHit> pathPointHandleHits;
  iggy3d::creative::CreativeObjectId selectedPathHandleObjectId =
      iggy3d::creative::kInvalidObjectId;
  bool selectedIsPathForHandles = false;
  iggy3d::Vec3 anchorS{0.0F, 0.0F, 0.0F};
};

[[nodiscard]] CreativeEditorGizmoFrame buildCreativeEditorGizmoFrame(
    const CreativeEditorSelectionFrame& selection,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    float axisLengthMeters);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `<array>`
- `<cstdint>`
- `<vector>`
- `app/iggy3d/creative/document/Object.hpp`
- `core/math/Vec3.hpp`
- `render/FrameInput.hpp`
- `CreativeEditorSelection.hpp`
- `StandaloneGizmo.hpp`
- `StandalonePicking.hpp`

Expected implementation dependencies:

- `app/iggy3d/creative/document/ObjectDescriptor.hpp`
- `StandalonePreviewProxies.hpp`

The implementation should move the current gizmo frame geometry and path-handle
hit data calculation exactly, adjusted only for the helper API and field names.

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorGizmoFrame.hpp`;
- add `using iggy3d_creative_app::CreativeEditorGizmoFrame;`;
- add `using iggy3d_creative_app::buildCreativeEditorGizmoFrame;`;
- replace only the current gizmo geometry, handle-hit-test data, and
  `gizmoAnchorS` setup block with a helper call and restored locals:

```cpp
const CreativeEditorGizmoFrame gizmoFrame = buildCreativeEditorGizmoFrame(
    selection, frame.camera, extent.width, extent.height, kGizmoAxisLength);
const Vec3 gizmoCenter = gizmoFrame.center;
const std::array<GizmoAxisShaft, 3>& gizmoShafts = gizmoFrame.shafts;
const ScreenPoint gizmoCenterScreen = gizmoFrame.centerScreen;
const std::array<ScreenPoint, 3>& gizmoTipScreen = gizmoFrame.tipScreens;
const std::vector<PathPointHandleHit>& pathPointHandleHits =
    gizmoFrame.pathPointHandleHits;
const creative::CreativeObjectId selectedPathHandleObjectId =
    gizmoFrame.selectedPathHandleObjectId;
const bool selectedIsPathForHandles = gizmoFrame.selectedIsPathForHandles;
const Vec3 gizmoAnchorS = gizmoFrame.anchorS;
```

If formatting differs slightly, preserve the same local names and semantics.
Keeping these locals in `main.cpp` is intentional so downstream capture logging,
Move, and overlay code stay unchanged and reviewable.

Do not move the path-handle capture logging block. Do not move capture Move,
interactive Move, overlay construction, dimension label logic, final logging, or
any downstream gizmo/path consumer.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `gizmoCenter` from `selBoxMin` and `selBoxMax`;
- the three `GizmoAxisShaft` entries and their axis/tip/color values;
- `projectPointToScreen(frame.camera.clipFromWorld, gizmoCenter, extent.width,
  extent.height)`;
- the loop projecting every shaft tip;
- `selectedPathHandleObjectId = static_cast<creative::CreativeObjectId>(selectedId)`;
- `selectedIsPathForHandles` predicate:
  - `hasSelection`
  - path shape descriptor check
  - `validPathPoints(selected->pathPoints)`
- `buildPathPointHandleHits(*selected, frame.camera.clipFromWorld,
  extent.width, extent.height)`;
- `gizmoAnchorS` fallback to `gizmoCenter`;
- `gizmoAnchorS = toVec3(selected->transform.position)` when `hasSelection`.

Preserve:

- shaft axis order X, Y, Z;
- shaft colors;
- axis length source from `kGizmoAxisLength`;
- path-handle hit order and values;
- anchor semantics;
- downstream capture logging, capture Move, interactive Move, overlay,
  dimension label, submit, and log behavior.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

Do not move path-handle capture logging.
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
rg -n "CreativeEditorGizmoFrame|buildCreativeEditorGizmoFrame|gizmoCenter|gizmoShafts|gizmoCenterScreen|gizmoTipScreen|pathPointHandleHits|selectedPathHandleObjectId|selectedIsPathForHandles|gizmoAnchorS|buildPathPointHandleHits|projectPointToScreen|validPathPoints" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorGizmoFrame.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorGizmoFrame.cpp
```

Expected:

- `CreativeEditorGizmoFrame.hpp` declares `CreativeEditorGizmoFrame` and
  `buildCreativeEditorGizmoFrame(...)`;
- `CreativeEditorGizmoFrame.cpp` defines `buildCreativeEditorGizmoFrame(...)`;
- gizmo center, shafts, projected center/tips, path-handle hits, path-handle
  selection predicate, and anchor fallback/selection logic live in
  `CreativeEditorGizmoFrame.cpp`;
- `main.cpp` includes `CreativeEditorGizmoFrame.hpp`, has the `using`
  declarations, calls `buildCreativeEditorGizmoFrame(...)`, and restores the
  local names for downstream code;
- path-handle capture logging remains in `main.cpp`;
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
rg -n "CreativeEditorGizmoFrame.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
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
- `apps/iggy3d_creative/CreativeEditorGizmoFrame.hpp`
- `apps/iggy3d_creative/CreativeEditorGizmoFrame.cpp`
- `CMakeLists.txt`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e275_final.png --frames 32 > /tmp/iggy3d_creative_e275_final.log 2>&1)
rg "PATH_HANDLE|GIZMO|FINAL frame|submit outcome" /tmp/iggy3d_creative_e275_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes gizmo center, shaft order, shaft colors, projected
  handle positions, path-handle hit data, selected path predicate, gizmo anchor,
  object ids, log strings, frame numbers, render-submit behavior, or downstream
  gizmo/path behavior;
- preserving the gizmo frame requires moving path-handle capture logging,
  placement, capture scenario, selection, gizmo/path policy, renderer submit,
  capture behavior, or any later frame stage;
- CMake/source-list changes affect targets other than `iggy3d_creative`;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- what gizmo frame logic moved and what remains in `main.cpp`;
- CMake source-list placement;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`,
  path-handle capture logging move, capture Move policy move, interactive Move
  policy move, placement move, capture scenario move, later frame-stage move,
  tests, receipt/golden files, broad CTest, interactive window launch, staging,
  commit, or push was performed.
