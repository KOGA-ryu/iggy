# E277: iggy3d_creative Main Split G11 - Pick Frame

## Objective

Extract the visible-object pick-candidate construction block from
`apps/iggy3d_creative/main.cpp` into a new app-local
`CreativeEditorPickFrame.*` helper module.

This is a mechanical extraction only. Preserve object pick candidates, floor
bounds capture, point/line/path proxy proof logs, latch behavior, click
selection behavior, world-pick proof behavior, object ids, frame numbers,
render output, and focused test results.

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
`CreativeEditorGizmoFrame.*`, including path-handle hit data.

E276 moved path-handle capture proof logging into `CreativeEditorGizmoFrame.*`.

Post-E276 current shape:

- `apps/iggy3d_creative/main.cpp`: 1318 lines.
- The in-scope block starts at the
  `// ---- CLICK-TO-SELECT (generic over ALL objects) ------------------------`
  comment.
- The block currently:
  - builds `std::vector<ObjectVisualPickBounds> objectPickCandidates`;
  - captures floor visual bounds for the deterministic floor click;
  - logs `POINT hit proxy`, `LINE hit proxy`, and `PATH hit proxy` capture
    proof rows and sets the corresponding latches.
- `main.cpp` then uses those values for:
  - world-pick proof logging;
  - synthetic/interactive click selection;
  - downstream object picking.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorPickFrame.hpp`
- new `apps/iggy3d_creative/CreativeEditorPickFrame.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/CreativeEditor*.cpp` helper files.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

struct CreativeEditorPickFrame {
  std::vector<ObjectVisualPickBounds> objectPickCandidates;
  bool haveFloorBounds = false;
  iggy3d::Vec3 floorBoxMin{};
  iggy3d::Vec3 floorBoxMax{};
};

[[nodiscard]] CreativeEditorPickFrame buildCreativeEditorPickFrame(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    iggy3d::creative::CreativeObjectId floorObjectId,
    StandaloneCaptureScript& captureScript,
    bool captureMode);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `<cstdint>`
- `<vector>`
- `app/iggy3d/creative/document/Document.hpp`
- `app/iggy3d/creative/document/Object.hpp`
- `core/math/Vec3.hpp`
- `render/FrameInput.hpp`
- `StandaloneCaptureScript.hpp`
- `StandalonePicking.hpp`

Expected implementation dependencies:

- `<SDL3/SDL.h>`
- `StandalonePlacement.hpp`

The helper should move the current visible-object loop exactly, adjusted only
for the new helper API and field names.

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorPickFrame.hpp`;
- add `using iggy3d_creative_app::CreativeEditorPickFrame;`;
- add `using iggy3d_creative_app::buildCreativeEditorPickFrame;`;
- replace only the current visible-object pick-candidate/proxy-log loop with:

```cpp
const CreativeEditorPickFrame pickFrame = buildCreativeEditorPickFrame(
    appState.facade.document(),
    frame.camera,
    extent.width,
    extent.height,
    floorObjectId,
    editor.captureScript,
    !capturePath.empty());
const std::vector<ObjectVisualPickBounds>& objectPickCandidates =
    pickFrame.objectPickCandidates;
const bool haveFloorBounds = pickFrame.haveFloorBounds;
const Vec3 floorBoxMin = pickFrame.floorBoxMin;
const Vec3 floorBoxMax = pickFrame.floorBoxMax;
```

If formatting differs slightly, preserve the same argument order and semantics.

Keeping these local names in `main.cpp` is intentional so the world-pick proof
and click-selection code remain reviewable and unchanged.

Do not move the `logWorldPickProof` lambda, floor-corner proof, object-center
proof, synthetic/interactive click selection, placement, capture scenario,
selection, gizmo, Move, overlay, submit, shutdown, or final logging code.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- construction of `std::vector<ObjectVisualPickBounds>`;
- visible-object iteration over `appState.facade.document().objects()`;
- `if (!obj.visible) continue`;
- `buildObjectVisualPickBounds(obj, frame.camera.clipFromWorld, extent.width,
  extent.height)`;
- `const Vec3 boxMin = hit.bounds.min`;
- `const Vec3 boxMax = hit.bounds.max`;
- `objectPickCandidates.push_back(hit)`;
- floor id check against `floorObjectId`;
- `haveFloorBounds`, `floorBoxMin`, and `floorBoxMax` assignment;
- point proxy log gate, exact log string, values, and latch assignment;
- line proxy log gate, exact log string, values, and latch assignment;
- path proxy log gate, exact log string, values, `pathPointsSummary(...)`, and
  latch assignment.

Preserve:

- candidate order;
- floor bounds source;
- proxy proof log order;
- latch behavior;
- all downstream world-pick proof, click selection, placement, capture
  scenario, selection, gizmo, Move, overlay, submit, and final logging behavior.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

Do not move world-pick proof logging.
Do not move click selection.
Do not move placement behavior.
Do not move capture scenario dispatch or any capture scenario helper code.
Do not move capture Move behavior.
Do not move interactive Move behavior.
Do not move selection, gizmo/path policy, overlay construction, frustum
culling, submit, shutdown, capture script frame numbers, object ids, seeded
Floor/Crate setup, save root setup, final capture proof strings, or
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
rg -n "CreativeEditorPickFrame|buildCreativeEditorPickFrame|POINT hit proxy|LINE hit proxy|PATH hit proxy|pointHitProxyLogged|lineHitProxyLogged|pathHitProxyLogged|objectPickCandidates|haveFloorBounds|floorBoxMin|floorBoxMax" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorPickFrame.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorPickFrame.cpp
```

Expected:

- `CreativeEditorPickFrame.hpp` declares `CreativeEditorPickFrame` and
  `buildCreativeEditorPickFrame(...)`;
- `CreativeEditorPickFrame.cpp` defines `buildCreativeEditorPickFrame(...)`;
- visible-object candidate construction lives in `CreativeEditorPickFrame.cpp`;
- the `POINT hit proxy`, `LINE hit proxy`, and `PATH hit proxy` log strings and
  latches live in `CreativeEditorPickFrame.cpp`;
- `main.cpp` calls `buildCreativeEditorPickFrame(...)` and restores
  `objectPickCandidates`, `haveFloorBounds`, `floorBoxMin`, and `floorBoxMax`
  for downstream code;
- world-pick proof and click-selection consumers remain in `main.cpp`.

Run:

```sh
rg -n "CreativeEditorGizmoFrame|CreativeEditorSelection|CreativeEditorAim|CreativeEditorCommandInput|CreativeEditorFrameInput|StandaloneDelete|EditorFrame|runCreativeEditorFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- E276's `CreativeEditorGizmoFrame.*` remains the gizmo frame and path-handle
  capture-log owner;
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
rg -n "CreativeEditorPickFrame.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
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
- `apps/iggy3d_creative/CreativeEditorPickFrame.hpp`
- `apps/iggy3d_creative/CreativeEditorPickFrame.cpp`
- `CMakeLists.txt`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e277_final.png --frames 32 > /tmp/iggy3d_creative_e277_final.log 2>&1)
rg "POINT hit proxy|LINE hit proxy|PATH hit proxy|WORLD_PICK|FINAL frame|submit outcome" /tmp/iggy3d_creative_e277_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes candidate order, floor bounds, proxy proof logs, log
  strings, log order, latch behavior, click selection, capture behavior, object
  ids, frame numbers, render-submit behavior, or downstream pick behavior;
- preserving the pick frame requires moving world-pick proof, click selection,
  placement, capture scenario, selection, gizmo/path policy, renderer submit,
  capture behavior, or any later frame stage;
- CMake/source-list changes affect targets other than `iggy3d_creative`;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- what pick-candidate/proxy-log logic moved and what remains in `main.cpp`;
- CMake source-list placement;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`,
  world-pick proof move, click-selection move, placement move, capture scenario
  move, capture Move policy move, interactive Move policy move, later
  frame-stage move, tests, receipt/golden files, broad CTest, interactive
  window launch, staging, commit, or push was performed.
