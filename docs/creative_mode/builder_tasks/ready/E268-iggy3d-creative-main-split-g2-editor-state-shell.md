# E268: iggy3d_creative Main Split G2 - Editor State Shell

## Objective

Introduce an app-local `CreativeEditorState` shell for the mutable standalone
editor state currently scattered as free locals in
`apps/iggy3d_creative/main.cpp`.

This is a mechanical state regroup only. Do not move frame stages yet. Preserve
all behavior, log strings, capture timing, object ids, frame numbers, render
output, and focused test results.

## Current Context

E266 mapped the current `main.cpp` split target:

- `main.cpp` was 1651 lines before G1.
- `main(int, char**)` started at line 230 before G1.
- the frame loop started at line 488 before G1 and ran through line 1641.
- the target end state is eventually:
  - `CreativeEditorState` owning free locals;
  - named frame stages in `EditorFrame.{hpp,cpp}`;
  - `main()` reduced to parse/build/seed plus
    `while (open) runCreativeEditorFrame(state, deps)`.

E267 completed G1 by moving only `appendWireframeBoxEdges(...)` into
`StandaloneWireframeBoxEdges.*`. Post-G1, `main.cpp` is 1624 lines and still
owns the mutable frame-loop/editor locals directly.

This G2 should create the state shell needed for real frame-stage extraction,
but it must not extract any stages yet.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorState.hpp`
- this task card when moving it to `done/`

Do not add a `.cpp` file or touch `CMakeLists.txt` unless a compile blocker
proves a header-only state shell is not viable. If a `.cpp`/CMake change becomes
necessary, stop and report instead of widening the design.

## Required State Shape

Create `apps/iggy3d_creative/CreativeEditorState.hpp` in namespace
`iggy3d_creative_app`.

The state should use member default initializers so `main.cpp` can create it
with:

```cpp
iggy3d_creative_app::CreativeEditorState editor;
```

Expected state ownership:

```cpp
namespace iggy3d_creative_app {

struct CreativeEditorState {
  iggy3d::ProductCreativeFlyConfig flyConfig{};
  iggy3d::Vec3 flyPos{0.0F, 6.0F, 12.0F};
  float yawDegrees = 0.0F;
  float pitchDegrees = -25.0F;

  bool loggedSelection = false;

  bool prevKey1 = false;
  bool prevKey2 = false;
  bool moveDragButtonDown = false;
  bool loggedMoveBefore = false;
  bool loggedMoveAfter = false;

  iggy3d_creative_app::GizmoAxis interactiveGrabbedAxis =
      iggy3d_creative_app::GizmoAxis::None;
  iggy3d::Vec3 interactiveGrabAnchorS{0.0F, 0.0F, 0.0F};
  float interactiveGrabCursorX = 0.0F;
  float interactiveGrabCursorY = 0.0F;
  iggy3d_creative_app::ScreenPoint interactiveGrabCenterScreen;
  iggy3d_creative_app::ScreenPoint interactiveGrabTipScreen;
  bool interactivePathMoveActive = false;
  iggy3d::creative::CreativeObjectId interactivePathMoveObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeToolWorldPoint interactivePathMoveStartGround{};
  bool interactivePathPointMoveActive = false;
  iggy3d::creative::CreativeObjectId interactivePathPointMoveObjectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t interactivePathPointMoveIndex = 0U;
  iggy3d::creative::CreativeToolWorldPoint
      interactivePathPointMoveStartGround{};
  bool loggedGizmoGrab = false;

  bool placeMode = false;
  std::vector<iggy3d::creative::CreativeObjectKind> brushPalette;
  iggy3d::creative::CreativeObjectKind placeBrush =
      iggy3d::creative::CreativeObjectKind::Unknown;
  double placeCellSize = 1.0;
  bool prevKey3 = false;
  bool prevKeyB = false;
  bool placeButtonDown = false;
  std::uint64_t placedCount = 0;

  bool prevKeyF5 = false;
  bool prevKeyF6 = false;
  bool prevKeyF9 = false;
  bool prevKeyDelete = false;
  bool prevKeyBackspace = false;
  bool prevKeyZ = false;
  iggy3d_creative_app::StandaloneUndoStack undoStack;
  iggy3d_creative_app::StandaloneCaptureScript captureScript;
  bool captureWorldPickFloorLogged = false;
  bool captureWorldPickPointLogged = false;
  bool captureWorldPickLineLogged = false;
  bool captureWorldPickPathLogged = false;

  std::uint64_t frameIndex = 0;
  std::uint32_t lastWidth = 0;
  std::uint32_t lastHeight = 0;
};

}  // namespace iggy3d_creative_app
```

Use the exact field names above unless a compile issue requires a smaller
rename. The point is to make the later frame-stage extraction mechanically
obvious and grep-friendly.

Expected header dependencies include only what the state needs, such as:

- `<cstddef>`
- `<cstdint>`
- `<vector>`
- `app/iggy3d/creative/camera/Fly.hpp`
- `app/iggy3d/creative/document/Object.hpp`
- `core/math/Vec3.hpp`
- `StandaloneCaptureScript.hpp`
- `StandaloneGizmo.hpp`
- `StandalonePicking.hpp`
- `StandaloneUndo.hpp`

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorState.hpp`;
- add `using iggy3d_creative_app::CreativeEditorState;`;
- replace the current free locals covered by `CreativeEditorState` with a
  single `CreativeEditorState editor;`;
- keep the same initialization values for camera, key latches, drag latches,
  capture proof latches, frame counters, and resize counters;
- initialize `editor.flyConfig` exactly where the current `flyConfig` is
  configured:
  - `enabled = true`
  - `speedMetersPerSecond = 8.0F`
  - `sprintMultiplier = 3.0F`
  - `inputStepSeconds = 1.0F / 60.0F`
- initialize `editor.brushPalette`, `editor.placeBrush`, and
  `editor.placeCellSize` using the existing expressions and keep the existing
  brush-palette `SDL_Log(...)` output;
- preserve capture-mode place initialization:
  - if `!capturePath.empty()`, set `editor.placeMode = true`;
  - reset `editor.placeBrush = firstBrushKind(editor.brushPalette)`;
- update all in-scope references in `main.cpp` to `editor.<field>`.

Do not move `floorObjectId`, `crateObjectId`, `saveRoot`, `saveId`,
`wireProjReq`, `gridConfig`, `gridSnapshot`, constants, or per-frame transient
buffers into `CreativeEditorState` in this card unless a compile blocker proves
one belongs there. These are setup/dependency values or frame-local lifetimes,
not the mutable app/editor state this card is grouping.

## Required Behavior Preservation

Preserve:

- `main(int, char**)` behavior and return codes;
- `--frames` and `--capture` parsing/default behavior;
- camera initial position and yaw/pitch;
- fly config values;
- all key latch behavior;
- place mode and brush initialization behavior;
- brush-palette log text and value order;
- capture script frame numbers and proof latches;
- undo stack lifetime;
- interactive move/gizmo/path/path-point state behavior;
- resize tracking behavior;
- `frameIndex` increment behavior and all uses in frame construction/logging;
- all existing log strings and reason strings.

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

Do not create `EditorFrame.{hpp,cpp}`.
Do not create `runCreativeEditorFrame(...)`.
Do not move frame stages.
Do not move helper implementations.
Do not change event handling, camera input, resize behavior, placement,
selection, capture scenario dispatch, gizmo/path policy, overlay construction,
frustum culling, submit, shutdown, logging, capture script frame numbers,
object ids, seeded Floor/Crate setup, save root, final capture proof strings,
or render-submit reason strings.
Do not run broad CTest.
Do not launch an interactive window.
Do not require `iggy3d_creative --capture` unless an owner explicitly allows a
windowed/Vulkan capture check.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "CreativeEditorState|editor\\.|flyConfig|flyPos|yawDegrees|pitchDegrees|prevKey|moveDragButtonDown|loggedMoveBefore|loggedMoveAfter|interactiveGrab|interactivePath|placeMode|brushPalette|placeBrush|placeCellSize|placeButtonDown|placedCount|undoStack|captureScript|captureWorldPick|frameIndex|lastWidth|lastHeight" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorState.hpp
```

Expected:

- `CreativeEditorState` is declared only in `CreativeEditorState.hpp`;
- `main.cpp` creates a single `CreativeEditorState editor;`;
- moved fields are accessed through `editor.<field>` in `main.cpp`;
- the old moved free-local declarations are gone from `main.cpp`;
- setup/dependency values such as `floorObjectId`, `saveRoot`, `saveId`,
  `wireProjReq`, `gridConfig`, and per-frame local buffers remain outside the
  state shell.

Run:

```sh
rg -n "EditorFrame|runCreativeEditorFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged;
- no old `appendWireframeBoxEdges(...)` helper was reintroduced.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorState.hpp`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e268_final.png --frames 32 > /tmp/iggy3d_creative_e268_final.log 2>&1)
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e268_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the state shell requires changing behavior, log strings, frame numbers,
  object ids, capture script behavior, or render-submit behavior;
- compile fallout requires moving a frame stage, adding a `.cpp` file, changing
  CMake, or introducing `EditorFrame`;
- `CreativeEditorState` starts absorbing setup/dependency values or per-frame
  buffers whose lifetime should remain local;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact `CreativeEditorState` field inventory;
- whether the state shell stayed header-only;
- which old free locals were removed from `main.cpp`;
- which setup/dependency values intentionally stayed outside the state;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`,
  frame-stage move, helper move, CMake/test/receipt/golden edits, broad CTest,
  interactive window launch, staging, commit, or push was performed.
