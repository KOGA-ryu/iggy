# E274: iggy3d_creative Main Split G8 - Selection Frame

## Objective

Extract the small selected-object resolution block from
`apps/iggy3d_creative/main.cpp` into an app-local helper module.

This is a mechanical extraction only. Preserve selection id lookup, selected
object pointer semantics, visible-selection gating, default bounds, selected
bounds, downstream gizmo/move/overlay behavior, object ids, frame numbers,
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

E273 shared the raw camera-ray-to-ground-point math between the aim-cell helper
and the remaining interactive Move ground point.

Post-E273 current shape:

- `apps/iggy3d_creative/main.cpp`: 1369 lines.
- The frame loop still resolves the current selection directly in `main.cpp`
  after capture scenario dispatch.
- The in-scope block starts at the
  `// ---- RESOLVE THE SELECTION (generic) -----------------------------------`
  comment.
- The block reads `appState.facade.selectionState().selectedTarget.value`,
  calls `appState.facade.findObject(...)` when the id is non-zero, gates
  selection with `selected != nullptr && selected->visible`, initializes
  default bounds, and replaces them with `visualBoundsForObject(*selected)` when
  `hasSelection` is true.
- Downstream gizmo geometry, handle hit-test, move, overlay, dimension label,
  and final logging code all keep using `selectedId`, `selected`,
  `hasSelection`, `selBoxMin`, and `selBoxMax`.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/CreativeEditorSelection.hpp`
- new `apps/iggy3d_creative/CreativeEditorSelection.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/CreativeEditor*.cpp` helper files.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

struct CreativeEditorSelectionFrame {
  iggy3d::creative::Id selectedId = 0;
  const iggy3d::creative::CreativeObject* selected = nullptr;
  bool hasSelection = false;
  iggy3d::Vec3 boxMin{-0.5F, 0.0F, -0.5F};
  iggy3d::Vec3 boxMax{0.5F, 1.0F, 0.5F};
};

[[nodiscard]] CreativeEditorSelectionFrame resolveCreativeEditorSelectionFrame(
    const iggy3d::creative::Facade& facade);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `app/iggy3d/creative/Facade.hpp`
- `app/iggy3d/creative/document/Object.hpp`
- `core/math/Vec3.hpp`

Expected implementation dependencies:

- `StandalonePreviewProxies.hpp`

The implementation should move the current selected-id/object/visible/bounds
logic exactly, adjusted only for the helper API and field names.

## Required `main.cpp` Migration

In `main.cpp`:

- include `CreativeEditorSelection.hpp`;
- add `using iggy3d_creative_app::CreativeEditorSelectionFrame;`;
- add `using iggy3d_creative_app::resolveCreativeEditorSelectionFrame;`;
- replace only the current selection-resolution block with:

```cpp
const CreativeEditorSelectionFrame selection =
    resolveCreativeEditorSelectionFrame(appState.facade);
const creative::Id selectedId = selection.selectedId;
const creative::CreativeObject* selected = selection.selected;
const bool hasSelection = selection.hasSelection;
const Vec3 selBoxMin = selection.boxMin;
const Vec3 selBoxMax = selection.boxMax;
```

If formatting differs slightly, preserve the same local names and semantics.
Keeping these locals in `main.cpp` is intentional so later code stays
unchanged and reviewable.

Do not move gizmo geometry, handle hit-test, move policy, overlay construction,
dimension label logic, final logging, or any downstream `selected*` consumer.

## Required Behavior Preservation

Move these statements and value expressions without behavior changes:

- `appState.facade.selectionState().selectedTarget.value`;
- non-zero selected-id check;
- `appState.facade.findObject(static_cast<creative::CreativeObjectId>(selectedId))`;
- `selected != nullptr && selected->visible`;
- default `selBoxMin{-0.5F, 0.0F, -0.5F}`;
- default `selBoxMax{0.5F, 1.0F, 0.5F}`;
- `visualBoundsForObject(*selected)`;
- `selBoxMin = selectedVisualBounds.min`;
- `selBoxMax = selectedVisualBounds.max`.

Preserve:

- selected pointer semantics, including returning the found pointer even before
  the visible-selection gate;
- `hasSelection` meaning;
- default bounds when there is no visible selection;
- selected bounds when there is a visible selection;
- all downstream selected-id, selected-object, gizmo, path, move, overlay,
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

Do not move gizmo geometry.
Do not move handle hit-test.
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
rg -n "CreativeEditorSelectionFrame|resolveCreativeEditorSelectionFrame|CreativeEditorSelection|selectionState|selectedTarget|selectedId|hasSelection|selBoxMin|selBoxMax|visualBoundsForObject" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorSelection.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/CreativeEditorSelection.cpp
```

Expected:

- `CreativeEditorSelection.hpp` declares `CreativeEditorSelectionFrame` and
  `resolveCreativeEditorSelectionFrame(...)`;
- `CreativeEditorSelection.cpp` defines `resolveCreativeEditorSelectionFrame(...)`;
- `selectionState()`, `selectedTarget`, `findObject(...)`, visible-selection
  gating, default bounds, and `visualBoundsForObject(...)` live in
  `CreativeEditorSelection.cpp`;
- `main.cpp` includes `CreativeEditorSelection.hpp`, has the `using`
  declarations, calls `resolveCreativeEditorSelectionFrame(...)`, and restores
  the local `selectedId`, `selected`, `hasSelection`, `selBoxMin`, and
  `selBoxMax` names for downstream code;
- downstream selected-object consumers remain in `main.cpp`.

Run:

```sh
rg -n "CreativeEditorAim|CreativeEditorCommandInput|CreativeEditorFrameInput|StandaloneDelete|EditorFrame|runCreativeEditorFrame|appendStandaloneWireframeBoxEdges|appendWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- E273's `CreativeEditorAim.*` remains the aim/ground helper owner;
- E271's `CreativeEditorCommandInput.*` remains the command-key owner;
- E269's `CreativeEditorFrameInput.*` remains the frame-begin input owner;
- E270's `StandaloneDelete.*` remains the delete helper owner;
- no `EditorFrame` or `runCreativeEditorFrame` was introduced;
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged;
- no old `appendWireframeBoxEdges(...)` helper was reintroduced.

Run:

```sh
rg -n "CreativeEditorSelection.cpp" /Users/kogaryu/iggy3d/CMakeLists.txt
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
- `apps/iggy3d_creative/CreativeEditorSelection.hpp`
- `apps/iggy3d_creative/CreativeEditorSelection.cpp`
- `CMakeLists.txt`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e274_final.png --frames 32 > /tmp/iggy3d_creative_e274_final.log 2>&1)
rg "WORLD_PICK|GIZMO|FINAL frame|submit outcome" /tmp/iggy3d_creative_e274_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move changes selected id lookup, selected pointer semantics,
  visible-selection gating, default bounds, selected bounds, object ids, log
  strings, frame numbers, render-submit behavior, or downstream selected-object
  behavior;
- preserving the selection frame requires moving gizmo geometry, handle
  hit-test, placement, capture scenario, selection, gizmo/path policy, renderer
  submit, capture behavior, or any later frame stage;
- CMake/source-list changes affect targets other than `iggy3d_creative`;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- what selection logic moved and what remains in `main.cpp`;
- CMake source-list placement;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `EditorFrame`, `runCreativeEditorFrame(...)`, gizmo
  geometry move, handle hit-test move, interactive Move policy move, placement
  move, capture scenario move, later frame-stage move, tests, receipt/golden
  files, broad CTest, interactive window launch, staging, commit, or push was
  performed.

## Completion Brief

Files changed:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/CreativeEditorSelection.hpp`
- `apps/iggy3d_creative/CreativeEditorSelection.cpp`
- `CMakeLists.txt`
- this task card, moved from `ready/` to `claimed/` and then `done/`

Exact helper API shape:

```cpp
namespace iggy3d_creative_app {

struct CreativeEditorSelectionFrame {
  iggy3d::creative::Id selectedId = 0;
  const iggy3d::creative::CreativeObject* selected = nullptr;
  bool hasSelection = false;
  iggy3d::Vec3 boxMin{-0.5F, 0.0F, -0.5F};
  iggy3d::Vec3 boxMax{0.5F, 1.0F, 0.5F};
};

[[nodiscard]] CreativeEditorSelectionFrame resolveCreativeEditorSelectionFrame(
    const iggy3d::creative::Facade& facade);

}  // namespace iggy3d_creative_app
```

The moved selection logic is the selected id read from
`facade.selectionState().selectedTarget.value`, the non-zero `findObject(...)`
lookup, visible-selection gating, default bounds, and selected visual bounds
from `visualBoundsForObject(...)`. `main.cpp` still owns the downstream local
names and all gizmo geometry, handle hit-test, move policy, overlay,
dimension-label, final logging, submit, and selected-object consumers.

CMake source-list placement:

- `apps/iggy3d_creative/CreativeEditorSelection.cpp` was added to the
  `iggy3d_creative` executable source list next to the other
  `CreativeEditor*.cpp` helper sources.

Required grep classifications:

- `CreativeEditorSelection.hpp` declares `CreativeEditorSelectionFrame` and
  `resolveCreativeEditorSelectionFrame(...)`.
- `CreativeEditorSelection.cpp` defines
  `resolveCreativeEditorSelectionFrame(...)`.
- `selectionState()`, `selectedTarget`, `findObject(...)`,
  visible-selection gating, default bounds, and `visualBoundsForObject(...)`
  live in `CreativeEditorSelection.cpp`.
- `main.cpp` includes `CreativeEditorSelection.hpp`, has the `using`
  declarations, calls `resolveCreativeEditorSelectionFrame(...)`, and restores
  the local `selectedId`, `selected`, `hasSelection`, `selBoxMin`, and
  `selBoxMax` names for downstream code.
- downstream selected-object consumers remain in `main.cpp`.
- `CreativeEditorAim.*`, `CreativeEditorCommandInput.*`,
  `CreativeEditorFrameInput.*`, and `StandaloneDelete.*` remain the aim/ground,
  command-key, frame-begin input, and delete helper owners.
- no `EditorFrame` or `runCreativeEditorFrame(...)` was introduced.
- E267's `appendStandaloneWireframeBoxEdges(...)` helper remains unchanged,
  and no old `appendWireframeBoxEdges(...)` helper was reintroduced.
- `CreativeEditorSelection.cpp` appears in the `iggy3d_creative` executable
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

No `EditorFrame`, `runCreativeEditorFrame(...)`, gizmo geometry move, handle
hit-test move, interactive Move policy move, placement move, capture scenario
move, later frame-stage move, tests, receipt/golden files, broad CTest,
interactive window launch, staging, commit, or push was performed.
