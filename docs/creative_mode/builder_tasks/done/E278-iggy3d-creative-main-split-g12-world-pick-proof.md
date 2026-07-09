# E278: iggy3d_creative Main Split G12 - World Pick Proof

## Objective

Move the world-pick proof logging block from
`apps/iggy3d_creative/main.cpp` into the existing
`CreativeEditorPickFrame.*` helper module.

This is a mechanical extraction only. Preserve capture gating, proof point
selection, pick behavior, log strings, latch behavior, click selection
behavior, object ids, frame numbers, render output, and focused test results.

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

E277 extracted visible-object pick candidate construction and point/line/path
proxy proof logs into `CreativeEditorPickFrame.*`.

Post-E277 current shape:

- `apps/iggy3d_creative/main.cpp`: 1272 lines.
- `CreativeEditorPickFrame` now carries:
  - `objectPickCandidates`
  - `haveFloorBounds`
  - `floorBoxMin`
  - `floorBoxMax`
- `main.cpp` still owns the capture-only world-pick proof block immediately
  after `buildCreativeEditorPickFrame(...)`.
- That proof block logs:
  - `WORLD_PICK_PROOF label='floor_overlap'`
  - `WORLD_PICK_PROOF label='point_proxy'`
  - `WORLD_PICK_PROOF label='line_proxy'`
  - `WORLD_PICK_PROOF label='path_proxy'`

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorPickFrame.hpp`
- `apps/iggy3d_creative/CreativeEditorPickFrame.cpp`
- this task card when moving it to `done/`

Do not edit `CMakeLists.txt`; E277 already added
`CreativeEditorPickFrame.cpp` to the `iggy3d_creative` executable source list.

## Required Helper API

Add this helper in namespace `iggy3d_creative_app`:

```cpp
void logCreativeEditorWorldPickProofFrame(
    const iggy3d::creative::Facade& facade,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    iggy3d::creative::CreativeObjectId floorObjectId,
    CreativeEditorState& editor,
    bool captureMode);
```

Expected header dependencies:

- `app/iggy3d/creative/Facade.hpp`
- `CreativeEditorState.hpp`

Expected implementation dependencies already present or needed:

- `<SDL3/SDL.h>`
- `StandalonePreviewProxies.hpp`

The helper should contain exactly the current capture-only world-pick proof
policy.

## Required `main.cpp` Migration

In `main.cpp`:

- add `using iggy3d_creative_app::logCreativeEditorWorldPickProofFrame;`;
- replace only the current `if (!capturePath.empty()) { ... WORLD_PICK_PROOF
  ... }` block with:

```cpp
logCreativeEditorWorldPickProofFrame(appState.facade,
                                     frame.camera,
                                     extent.width,
                                     extent.height,
                                     pickFrame,
                                     floorObjectId,
                                     editor,
                                     !capturePath.empty());
```

If formatting differs slightly, preserve the same argument order and semantics.

Do not move synthetic/interactive click selection, placement, capture scenario,
selection, gizmo, Move, overlay, submit, shutdown, or final logging code.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `if (!capturePath.empty())` gating, represented by `captureMode`;
- the local `logWorldPickProof` policy;
- `logged || expectedId == creative::kInvalidObjectId` early return;
- `projectPointToScreen(frame.camera.clipFromWorld, worldPoint, extent.width,
  extent.height)`;
- invalid screen-point early return;
- `worldRayFromPixel(frame.camera, screenPoint.x, screenPoint.y, extent.width,
  extent.height)`;
- `pickNearestVisualBoundsObject(objectPickCandidates, ray)`;
- exact `SDL_Log(...)` format string beginning
  `iggy3d_creative: WORLD_PICK_PROOF label='%s' ...`;
- `logged = true`;
- floor proof gate:
  `!editor.captureWorldPickFloorLogged && haveFloorBounds`;
- floor top-corner math with `0.85F`;
- `logObjectCenterPick(...)` object lookup through `appState.facade.findObject`;
- center proof through `visualBoundsCenter(visualBoundsForObject(*object))`;
- proof labels and target ids:
  - `floor_overlap` / `floorObjectId`
  - `point_proxy` / `editor.captureScript.pointTargetId`
  - `line_proxy` / `editor.captureScript.lineTargetId`
  - `path_proxy` / `editor.captureScript.pathTargetId`
- latch writes:
  - `editor.captureWorldPickFloorLogged`
  - `editor.captureWorldPickPointLogged`
  - `editor.captureWorldPickLineLogged`
  - `editor.captureWorldPickPathLogged`

Preserve:

- proof log order;
- latch behavior;
- pick-candidate source from `pickFrame.objectPickCandidates`;
- floor bounds source from `pickFrame`;
- all downstream click selection, placement, capture scenario, selection,
  gizmo, Move, overlay, submit, and final logging behavior.

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
rg -n "logCreativeEditorWorldPickProofFrame|WORLD_PICK_PROOF|captureWorldPick|floor_overlap|point_proxy|line_proxy|path_proxy|pickNearestVisualBoundsObject|projectPointToScreen|worldRayFromPixel" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorPickFrame.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorPickFrame.cpp
```

Expected:

- `CreativeEditorPickFrame.hpp` declares
  `logCreativeEditorWorldPickProofFrame(...)`;
- `CreativeEditorPickFrame.cpp` defines
  `logCreativeEditorWorldPickProofFrame(...)`;
- the `WORLD_PICK_PROOF` log string, proof labels, target-id checks, floor
  top-corner math, object-center proof, pick call, and latch writes live in
  `CreativeEditorPickFrame.cpp`;
- `main.cpp` calls `logCreativeEditorWorldPickProofFrame(...)`;
- click-selection consumers remain in `main.cpp`.

Run:

```sh
rg -n "CreativeEditorGizmoFrame|CreativeEditorSelection|CreativeEditorAim|CreativeEditorCommandInput|CreativeEditorFrameInput|StandaloneDelete|EditorFrame|runCreativeEditorFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- E277's `CreativeEditorPickFrame.*` remains the pick-frame owner;
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
- `apps/iggy3d_creative/CreativeEditorPickFrame.hpp`
- `apps/iggy3d_creative/CreativeEditorPickFrame.cpp`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e278_final.png --frames 32 > /tmp/iggy3d_creative_e278_final.log 2>&1)
rg "WORLD_PICK_PROOF|WORLD_PICK|FINAL frame|submit outcome" /tmp/iggy3d_creative_e278_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes proof point selection, pick behavior, log strings, log
  order, latch behavior, click selection, capture behavior, object ids, frame
  numbers, render-submit behavior, or downstream pick behavior;
- preserving the world-pick proof requires moving click selection, placement,
  capture scenario, selection, gizmo/path policy, renderer submit, capture
  behavior, or any later frame stage;
- CMake/source-list changes become necessary;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- what world-pick proof logic moved and what remains in `main.cpp`;
- confirmation that `CMakeLists.txt` was unchanged;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`,
  click-selection move, placement move, capture scenario move, capture Move
  policy move, interactive Move policy move, later frame-stage move, tests,
  receipt/golden files, broad CTest, interactive window launch, staging,
  commit, or push was performed.

## Completion Brief

Files changed:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorPickFrame.hpp`
- `apps/iggy3d_creative/CreativeEditorPickFrame.cpp`
- `docs/creative_mode/builder_tasks/done/E278-iggy3d-creative-main-split-g12-world-pick-proof.md`

Exact helper API added:

```cpp
void logCreativeEditorWorldPickProofFrame(
    const iggy3d::creative::Facade& facade,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    iggy3d::creative::CreativeObjectId floorObjectId,
    CreativeEditorState& editor,
    bool captureMode);
```

Moved world-pick proof logic:

- Capture-mode gating, the local world-pick proof policy, floor top-corner
  proof, object-center proofs, proof labels, target-id checks, pick call, exact
  `WORLD_PICK_PROOF` log string, and latch writes now live in
  `CreativeEditorPickFrame.cpp`.
- `main.cpp` now calls `logCreativeEditorWorldPickProofFrame(...)`.
- Synthetic/interactive click selection, placement, capture scenario dispatch,
  selection, gizmo, Move, overlay, submit, shutdown, and final logging remain in
  `main.cpp`.

`CMakeLists.txt` was unchanged.

Required grep classifications:

- `CreativeEditorPickFrame.hpp` declares
  `logCreativeEditorWorldPickProofFrame(...)`.
- `CreativeEditorPickFrame.cpp` defines
  `logCreativeEditorWorldPickProofFrame(...)`.
- The `WORLD_PICK_PROOF` log string, proof labels, floor top-corner math,
  object-center proof, pick call, and capture-world latch writes live in
  `CreativeEditorPickFrame.cpp`.
- `main.cpp` calls `logCreativeEditorWorldPickProofFrame(...)`.
- Click-selection consumers remain in `main.cpp`.
- Existing ownership remains unchanged for `CreativeEditorPickFrame.*`,
  `CreativeEditorGizmoFrame.*`, `CreativeEditorSelection.*`,
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

No `EditorFrame`, `runCreativeEditorFrame(...)`, click-selection move,
placement move, capture scenario move, capture Move policy move, interactive
Move policy move, later frame-stage move, tests, receipt/golden files, broad
CTest, interactive window launch, staging, commit, or push was performed.
