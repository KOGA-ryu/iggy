# E270: iggy3d_creative Main Split G4 - Delete Selected Helper

## Objective

Move the file-local `deleteSelectedObject(...)` helper out of
`apps/iggy3d_creative/main.cpp` into an app-local standalone helper module.

This is a mechanical extraction only. Preserve behavior, log strings, undo
snapshot behavior, selection invalidation behavior, call-site order, capture
scenario behavior, and focused test results.

## Current Context

E267 moved `appendWireframeBoxEdges(...)` into
`StandaloneWireframeBoxEdges.*`.

E268 introduced header-only `CreativeEditorState`.

E269 extracted the frame-begin input/resize/fly-camera stage into
`CreativeEditorFrameInput.*`.

Post-E269 current shape:

- `apps/iggy3d_creative/main.cpp`: 1534 lines.
- `CreativeEditorFrameInput.hpp`: 27 lines.
- `CreativeEditorFrameInput.cpp`: 90 lines.
- `deleteSelectedObject(...)` remains file-local in `main.cpp`.
- It currently has two call sites:
  - interactive Delete/Backspace key handling;
  - the capture scenario `deleteSelected` callback.

This helper is the shared dependency that blocks a clean command-key extraction.
Move it first, then a later card can move the interactive command-key block
without also moving delete policy.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/StandaloneDelete.hpp`
- new `apps/iggy3d_creative/StandaloneDelete.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/Standalone*.cpp` helper files.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeDocumentRemoveReceipt
deleteSelectedObject(iggy3d::creative::CreativeAppState& appState,
                     std::string_view source,
                     StandaloneUndoStack* undoStack = nullptr);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `<string_view>`
- `app/iggy3d/creative/CreativeAppState.hpp`
- `app/iggy3d/creative/document/DocumentMutation.hpp` or the narrow header
  that exposes `CreativeDocumentRemoveReceipt`
- `StandaloneUndo.hpp`

Expected implementation dependencies include whatever the current helper body
already needs:

- `<cstdint>`
- `<string>`
- SDL logging
- creative object/facade types used by the helper body
- `StandaloneUndo.hpp` for `pushUndoSnapshot(...)`

Use the existing function name `deleteSelectedObject(...)`. Do not rename it
in this card.

## Required `main.cpp` Migration

In `main.cpp`:

- include `StandaloneDelete.hpp`;
- add `using iggy3d_creative_app::deleteSelectedObject;`;
- remove the file-local `deleteSelectedObject(...)` definition from the
  anonymous namespace;
- keep both existing call sites and argument order unchanged:

```cpp
(void)deleteSelectedObject(appState,
                           keyDelete ? "delete_key" : "backspace_key",
                           &editor.undoStack);
```

```cpp
captureStep.deleteSelected = [&](std::string_view source) {
  return deleteSelectedObject(appState, source, &editor.undoStack);
};
```

Do not move the interactive command-key block in this card.

## Required Behavior Preservation

Preserve every value expression and log string from the current helper:

- selected id source:
  `appState.facade.selectionState().selectedTarget.value`;
- object count before/after reads;
- no-selection log:
  `iggy3d_creative: DELETE no selection ...`;
- missing-selection log:
  `iggy3d_creative: DELETE missing selection ...`;
- object kind capture before delete;
- undo depth capture;
- `pushUndoSnapshot(*undoStack, appState.facade, source)`;
- `appState.facade.removeDocumentObject(objectId)`;
- undo snapshot popback when delete is rejected after a push;
- undo discard log:
  `iggy3d_creative: UNDO discarded ...`;
- final delete proof log:
  `iggy3d_creative: DELETE removed ...`;
- returned `CreativeDocumentRemoveReceipt`.

Preserve both call sites:

- interactive Delete/Backspace should still pass `"delete_key"` or
  `"backspace_key"`;
- capture scenario callback should still pass through the caller-provided
  `source`.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

Do not move the interactive command-key block.
Do not create `EditorFrame.{hpp,cpp}`.
Do not create `runCreativeEditorFrame(...)`.
Do not move scene construction, aim-cell resolution, picking, placement,
capture scenario dispatch, selection resolution, gizmo/path policy, overlay
construction, frustum culling, submit, shutdown, logging outside the moved
delete helper, capture script frame numbers, object ids, seeded Floor/Crate
setup, save root, final capture proof strings, or render-submit reason strings.
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
rg -n "deleteSelectedObject|DELETE no selection|DELETE missing selection|UNDO discarded|DELETE removed|StandaloneDelete" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneDelete.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneDelete.cpp \
  /Users/kogaryu/iggy3d/CMakeLists.txt
```

Expected:

- `StandaloneDelete.hpp` declares `deleteSelectedObject(...)`;
- `StandaloneDelete.cpp` defines `deleteSelectedObject(...)`;
- the delete proof log strings live in `StandaloneDelete.cpp`;
- `main.cpp` includes `StandaloneDelete.hpp`;
- `main.cpp` has the `using` declaration and exactly two call sites;
- `CMakeLists.txt` includes `apps/iggy3d_creative/StandaloneDelete.cpp` in the
  `iggy3d_creative` executable source list.

Run:

```sh
rg -n "EditorFrame|runCreativeEditorFrame|CreativeEditorFrameInput|CreativeEditorState|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- E269's `CreativeEditorFrameInput.*` remains unchanged;
- E268's `CreativeEditorState` remains the state shell;
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
- `apps/iggy3d_creative/StandaloneDelete.hpp`
- `apps/iggy3d_creative/StandaloneDelete.cpp`
- `CMakeLists.txt`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e270_final.png --frames 32 > /tmp/iggy3d_creative_e270_final.log 2>&1)
rg "DELETE|UNDO discarded|ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e270_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes delete behavior, undo stack behavior, selection
  invalidation behavior, log strings, object ids, capture script behavior, or
  render-submit behavior;
- preserving the helper requires moving the interactive command-key block;
- compile fallout requires moving frame stages, selection, placement,
  gizmo/path policy, renderer submit, capture behavior, or any later frame
  stage;
- CMake/source-list changes affect targets other than `iggy3d_creative`;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- number of old helper definitions removed from `main.cpp`;
- number of call sites left in `main.cpp`;
- CMake source-list placement;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`,
  command-key block move, later frame-stage move, tests, receipt/golden files,
  broad CTest, interactive window launch, staging, commit, or push was
  performed.
