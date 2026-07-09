# E281: iggy3d_creative Main Split G15 - Capture Scenario Frame

## Objective

Extract the `--capture` scenario request assembly block from
`apps/iggy3d_creative/main.cpp` into a new app-local
`CreativeEditorCaptureScenario.*` helper module.

This is a mechanical extraction only. Preserve capture gating, frame index
semantics, request fields, delete callback behavior, object ids, frame numbers,
log strings, render output, and focused test results.

## Current Context

E279 extracted synthetic/interactive click selection into
`CreativeEditorClickSelection.*`.

E280 extracted interactive placement input into
`CreativeEditorPlacementInput.*`.

Post-E280 current shape:

- `apps/iggy3d_creative/main.cpp`: 1108 lines.
- Capture placement remains owned by `StandaloneCaptureScenario.*`.
- The remaining capture scenario block starts at
  `// ---- CAPTURE SCENARIO (--capture) --------------------------------------`.
- The block only assembles `StandaloneCaptureScenarioStepRequest`, installs the
  delete callback, and calls `runStandaloneCaptureScenarioStep(...)`.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorCaptureScenario.hpp`
- new `apps/iggy3d_creative/CreativeEditorCaptureScenario.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/CreativeEditor*.cpp` helper files.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
void runCreativeEditorCaptureScenarioFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId,
    bool captureMode);
```

Expected header dependencies:

- `<filesystem>`
- `<string>`
- `app/iggy3d/creative/CreativeAppState.hpp`
- `CreativeEditorState.hpp`

Expected implementation dependencies:

- `StandaloneCaptureScenario.hpp`
- `StandaloneDelete.hpp`
- `StandaloneGizmo.hpp`

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorCaptureScenario.hpp`;
- add `using iggy3d_creative_app::runCreativeEditorCaptureScenarioFrame;`;
- replace only the current capture-scenario block with:

```cpp
runCreativeEditorCaptureScenarioFrame(
    appState, editor, saveRoot, saveId, !capturePath.empty());
```

Do not move selection resolution, gizmo frame construction, capture Move,
interactive Move, overlay, submit, shutdown, or final logging code.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `if (!capturePath.empty())`, represented through `captureMode`;
- `StandaloneCaptureScenarioStepRequest captureStep;`;
- `captureStep.enabled = true`;
- `captureStep.frameIndex = editor.frameIndex`;
- every pointer/value assignment to `captureStep`;
- `captureStep.moveHeldAxisForX = heldAxisForGrabbedAxis(GizmoAxis::X)`;
- `captureStep.moveHeldAxisForZ = heldAxisForGrabbedAxis(GizmoAxis::Z)`;
- delete callback body:
  `return deleteSelectedObject(appState, source, &editor.undoStack);`
- `runStandaloneCaptureScenarioStep(captureStep)`.

Preserve all downstream selection, gizmo, capture Move, interactive Move,
overlay, submit, shutdown, and final logging behavior.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

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
rg -n "runCreativeEditorCaptureScenarioFrame|StandaloneCaptureScenarioStepRequest|runStandaloneCaptureScenarioStep|deleteSelectedObject|moveHeldAxisForX|moveHeldAxisForZ|captureStep" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorCaptureScenario.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorCaptureScenario.cpp
```

Expected:

- `CreativeEditorCaptureScenario.hpp` declares
  `runCreativeEditorCaptureScenarioFrame(...)`;
- `CreativeEditorCaptureScenario.cpp` defines
  `runCreativeEditorCaptureScenarioFrame(...)`;
- `StandaloneCaptureScenarioStepRequest`, all `captureStep` assignments,
  delete callback setup, and `runStandaloneCaptureScenarioStep(...)` live in
  `CreativeEditorCaptureScenario.cpp`;
- `main.cpp` calls `runCreativeEditorCaptureScenarioFrame(...)`;
- capture Move, interactive Move, overlay, and submit remain in `main.cpp`.

Run:

```sh
rg -n "EditorFrame|runCreativeEditorFrame|CreativeEditorPlacementInput|CreativeEditorClickSelection|CreativeEditorPickFrame|CreativeEditorGizmoFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- existing helper ownership remains unchanged;
- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- no old `appendWireframeBoxEdges(...)` helper was reintroduced.

Run:

```sh
rg -n "CreativeEditorCaptureScenario.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
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
- what capture scenario request assembly moved and what remains in `main.cpp`;
- CMake source-list placement;
- focused build/CTest and diff/whitespace results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`, capture
  Move policy move, interactive Move policy move, later frame-stage move,
  tests, receipt/golden files, broad CTest, interactive window launch, staging,
  commit, or push was performed.

## Completion Brief

Files changed:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorCaptureScenario.hpp`
- `apps/iggy3d_creative/CreativeEditorCaptureScenario.cpp`
- `CMakeLists.txt`
- moved this task card from `ready/` to `done/`

Helper API shape:

```cpp
void runCreativeEditorCaptureScenarioFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId,
    bool captureMode);
```

What moved:

- `!capturePath.empty()` capture gating, represented as `captureMode`
- `StandaloneCaptureScenarioStepRequest captureStep` construction
- all `captureStep` pointer/value assignments
- `frameIndex = editor.frameIndex`
- `moveHeldAxisForX` and `moveHeldAxisForZ` assignment through
  `heldAxisForGrabbedAxis(...)`
- delete callback setup using `deleteSelectedObject(appState, source,
  &editor.undoStack)`
- `runStandaloneCaptureScenarioStep(captureStep)`

What remains in `main.cpp`:

- selection resolution
- gizmo frame construction
- capture Move policy
- interactive Move policy
- overlay construction
- submit, shutdown, and final logging

CMake source-list placement:

- added `apps/iggy3d_creative/CreativeEditorCaptureScenario.cpp` to the
  `iggy3d_creative` executable source list next to the other
  `CreativeEditor*.cpp` helpers.

Required grep classifications:

- `runCreativeEditorCaptureScenarioFrame(...)` is declared in
  `CreativeEditorCaptureScenario.hpp`, defined in
  `CreativeEditorCaptureScenario.cpp`, and called from `main.cpp`.
- `StandaloneCaptureScenarioStepRequest`, all `captureStep` assignments,
  delete callback setup, `moveHeldAxisForX`, `moveHeldAxisForZ`,
  `deleteSelectedObject(...)`, and `runStandaloneCaptureScenarioStep(...)` live
  in `CreativeEditorCaptureScenario.cpp`.
- capture Move, interactive Move, overlay, and submit remain in `main.cpp`.
- existing helper ownership remains unchanged; no `EditorFrame`,
  `runCreativeEditorFrame(...)`, or old `appendWireframeBoxEdges(...)` helper
  was introduced.
- `CreativeEditorCaptureScenario.cpp` appears in `CMakeLists.txt` only in the
  `iggy3d_creative` executable source list.

Focused verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10`
  passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure`
  passed: 3/3 tests.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- focused trailing-whitespace scan over touched source/CMake files and this
  card passed.

Optional capture:

- skipped; no owner explicitly allowed a windowed/Vulkan capture check.

Confirmed not performed:

- no `EditorFrame`
- no `runCreativeEditorFrame(...)`
- no capture Move policy move
- no interactive Move policy move
- no later frame-stage move
- no tests, receipt files, or golden files edited
- no broad CTest
- no interactive window launch
- no staging, commit, or push
