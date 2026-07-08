# E271: iggy3d_creative Main Split G5 - Command Input

## Objective

Extract the interactive command-key block from
`apps/iggy3d_creative/main.cpp` into an app-local helper module.

This is a mechanical extraction only. Preserve behavior, key latches, log
strings, save/load/delete/undo behavior, capture-mode gating, object ids, frame
numbers, render output, and focused test results.

## Current Context

E267 moved `appendWireframeBoxEdges(...)` into
`StandaloneWireframeBoxEdges.*`.

E268 introduced header-only `CreativeEditorState`.

E269 extracted frame-begin event/drawable/resize/fly-camera input into
`CreativeEditorFrameInput.*`.

E270 moved the shared `deleteSelectedObject(...)` helper into
`StandaloneDelete.*`, leaving the interactive command-key block ready to move
without also moving delete policy.

Post-E270 current shape:

- `apps/iggy3d_creative/main.cpp`: 1473 lines.
- `StandaloneDelete.hpp`: 17 lines.
- `StandaloneDelete.cpp`: 76 lines.
- The in-scope command-key block currently starts at the
  `// ---- TOOL SWITCH: '1' -> Select, '2' -> Move ------------------` comment
  and ends after assigning `editor.prevKeyF9`.
- The block currently runs only when `capturePath.empty() && keys != nullptr`.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorCommandInput.hpp`
- new `apps/iggy3d_creative/CreativeEditorCommandInput.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
`CreativeEditorFrameInput.cpp`.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

void applyCreativeEditorCommandInput(
    const bool* keys,
    bool captureMode,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `<filesystem>`
- `<string>`
- `app/iggy3d/creative/CreativeAppState.hpp`
- `CreativeEditorState.hpp`

Expected implementation dependencies include the current command-key block's
real dependencies:

- `<SDL3/SDL.h>`
- `<string>`
- creative tool/status stringification already used by the moved block
- `StandaloneBrushPalette.hpp`
- `StandaloneDelete.hpp`
- `StandalonePersistenceProof.hpp`
- `StandaloneUndo.hpp`

The helper should return immediately, without mutating latches, when
`captureMode` is true or `keys == nullptr`. This preserves the current
`if (capturePath.empty() && keys != nullptr)` behavior.

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorCommandInput.hpp`;
- add `using iggy3d_creative_app::applyCreativeEditorCommandInput;`;
- replace only the current interactive command-key block with:

```cpp
applyCreativeEditorCommandInput(
    keys, !capturePath.empty(), appState, editor, saveRoot, saveId);
```

If formatting differs slightly, preserve the same argument order and semantics.

Do not move any other SDL keyboard reads. Later selection/place/move branches
that call `SDL_GetKeyboardState(...)` must stay in `main.cpp`.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `capturePath.empty() && keys != nullptr` gating, represented as the helper's
  `captureMode` / `keys == nullptr` early return;
- scancode reads for `1`, `2`, `3`, `B`, Delete, Backspace, `Z`, F5, F6, F9;
- `SDL_GetModState()` and the GUI/Ctrl undo modifier check;
- Select key behavior:
  - `editor.placeMode = false`;
  - `appState.facade.setActiveTool(creative::Tool::Select)`;
  - `iggy3d_creative: setActiveTool(Select) accepted=%d placeMode=0`;
- Move key behavior:
  - `editor.placeMode = false`;
  - `appState.facade.setActiveTool(creative::Tool::Move)`;
  - `iggy3d_creative: setActiveTool(Move) accepted=%d placeMode=0`;
- Place key behavior:
  - `editor.placeMode = true`;
  - `iggy3d_creative: placeMode=1 brush='%s'`;
- Brush cycle behavior:
  - `editor.placeBrush = nextBrushKind(editor.brushPalette, editor.placeBrush)`;
  - `iggy3d_creative: brush cycled -> '%s'`;
- Delete/Backspace behavior:
  - `deleteSelectedObject(appState, keyDelete ? "delete_key" : "backspace_key",
     &editor.undoStack)`;
- undo behavior:
  - `undoLastSnapshot(appState, editor.undoStack, "keyboard_undo")`;
- save behavior:
  - `saveStandaloneScene(appState.facade, saveRoot, saveId)`;
  - `clearUndoStack(editor.undoStack, "save_success")` only when accepted and
    saved;
- clear behavior:
  - `clearToBlankScene(appState)`;
  - `clearUndoStack(editor.undoStack, "new_clear")`;
- load behavior:
  - `loadStandaloneScene(appState, saveRoot, saveId)`;
  - `clearUndoStack(editor.undoStack, "load_success")` only when loaded;
- all latch assignments from `editor.prevKey1` through `editor.prevKeyF9`.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

Do not move `deleteSelectedObject(...)`; E270 already owns it.
Do not move frame-begin input; E269 already owns it.
Do not move capture scenario dispatch or any capture scenario helper code.
Do not move scene construction, aim-cell resolution, picking, placement,
selection resolution, capture move script, gizmo/path policy, overlay
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
rg -n "applyCreativeEditorCommandInput|TOOL SWITCH|SAVE / LOAD keys|SDL_GetModState|SDL_SCANCODE_1|SDL_SCANCODE_F5|setActiveTool\\(Select\\)|setActiveTool\\(Move\\)|placeMode=1|brush cycled|deleteSelectedObject|keyboard_undo|save_success|new_clear|load_success|prevKeyF9" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorCommandInput.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorCommandInput.cpp
```

Expected:

- `CreativeEditorCommandInput.hpp` declares
  `applyCreativeEditorCommandInput(...)`;
- `CreativeEditorCommandInput.cpp` defines
  `applyCreativeEditorCommandInput(...)`;
- the moved interactive command-key scancodes, latch assignments, and log
  strings live in `CreativeEditorCommandInput.cpp`;
- `main.cpp` includes `CreativeEditorCommandInput.hpp`, has the `using`
  declaration, and calls `applyCreativeEditorCommandInput(...)`;
- `main.cpp` no longer owns the interactive `TOOL SWITCH` / `SAVE / LOAD keys`
  block;
- any remaining `setActiveTool(Move)` hits in `main.cpp` belong to the later
  capture move script and must remain there.

Run:

```sh
rg -n "deleteSelectedObject|StandaloneDelete|CreativeEditorFrameInput|EditorFrame|runCreativeEditorFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- E270's `StandaloneDelete.*` remains the delete helper owner;
- E269's `CreativeEditorFrameInput.*` remains the frame-begin input owner;
- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged;
- no old `appendWireframeBoxEdges(...)` helper was reintroduced.

Run:

```sh
rg -n "CreativeEditorCommandInput.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
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
- `apps/iggy3d_creative/CreativeEditorCommandInput.hpp`
- `apps/iggy3d_creative/CreativeEditorCommandInput.cpp`
- `CMakeLists.txt`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e271_final.png --frames 32 > /tmp/iggy3d_creative_e271_final.log 2>&1)
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e271_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes command-key behavior, key latch behavior, save/load
  behavior, delete/undo behavior, log strings, object ids, capture script
  behavior, or render-submit behavior;
- preserving the command block requires moving capture scenario dispatch,
  selection, placement, gizmo/path policy, renderer submit, capture behavior,
  or any later frame stage;
- CMake/source-list changes affect targets other than `iggy3d_creative`;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- command-key block moved and what remains in `main.cpp`;
- CMake source-list placement;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`, delete
  helper move, frame-begin input move, capture scenario move, later frame-stage
  move, tests, receipt/golden files, broad CTest, interactive window launch,
  staging, commit, or push was performed.

## Completion Brief

Files changed:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorCommandInput.hpp`
- `apps/iggy3d_creative/CreativeEditorCommandInput.cpp`
- `CMakeLists.txt`
- this task card, moved to `done/`

Helper API:

```cpp
namespace iggy3d_creative_app {

void applyCreativeEditorCommandInput(
    const bool* keys,
    bool captureMode,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId);

}  // namespace iggy3d_creative_app
```

Extraction summary:

- Moved the interactive command-key block into
  `CreativeEditorCommandInput.cpp`.
- Preserved the capture-mode/null-keyboard early return semantics.
- Preserved scancode reads, latch reads/writes, modifier check, tool-switch
  calls, place-mode/brush-cycle behavior, delete/undo behavior, save/new/load
  behavior, and all moved log strings.
- `main.cpp` now calls:

```cpp
applyCreativeEditorCommandInput(
    keys, !capturePath.empty(), appState, editor, saveRoot, saveId);
```

What remains in `main.cpp`:

- frame-begin input remains delegated to `CreativeEditorFrameInput.*`;
- the capture scenario dispatch and capture callback remain in `main.cpp`;
- the separate capture callback still calls `deleteSelectedObject(...)`;
- later selection/place/move keyboard reads remain in `main.cpp`.

CMake:

- Added `apps/iggy3d_creative/CreativeEditorCommandInput.cpp` to the
  `iggy3d_creative` executable source list, directly before
  `apps/iggy3d_creative/CreativeEditorFrameInput.cpp`.
- No other target source list was changed.

Required grep classifications:

- `CreativeEditorCommandInput.hpp` declares
  `applyCreativeEditorCommandInput(...)`.
- `CreativeEditorCommandInput.cpp` defines
  `applyCreativeEditorCommandInput(...)`.
- The moved command-key scancodes, latch assignments, modifier check, and log
  strings live in `CreativeEditorCommandInput.cpp`.
- `main.cpp` includes `CreativeEditorCommandInput.hpp`, has the `using`
  declaration, and calls `applyCreativeEditorCommandInput(...)`.
- `main.cpp` no longer owns the interactive `TOOL SWITCH` / `SAVE / LOAD keys`
  block. The remaining `setActiveTool(Move)` hit in `main.cpp` belongs to the
  later capture move script and remains there.
- E270's `StandaloneDelete.*` remains the delete helper owner.
- E269's `CreativeEditorFrameInput.*` remains the frame-begin input owner.
- No `EditorFrame` or `runCreativeEditorFrame(...)` was introduced.
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged; no
  old `appendWireframeBoxEdges(...)` helper was reintroduced.
- `CreativeEditorCommandInput.cpp` appears only in the `iggy3d_creative`
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

- No `EditorFrame`, `runCreativeEditorFrame(...)`, delete helper move,
  frame-begin input move, capture scenario move, later frame-stage move, tests,
  receipt/golden edits, broad CTest, interactive window launch, staging,
  commit, or push.
