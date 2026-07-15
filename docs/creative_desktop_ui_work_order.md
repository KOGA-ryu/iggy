# Creative Desktop UI - Current Claude Work Order

> **This file holds only the current Claude batch.** Replace it after Ace
> accepts the batch. The builder implements this post-state; it does not
> redesign the architecture, widen the batch, or begin the next milestone.

## Batch UI-4A: Viewport-True Outliner and Inspector

**Required baseline:** `754c7a32` (`Refactor creative batch edits to use atomic
document mutations`) or a direct descendant explicitly approved by Ace.

**Position:** Desktop shell foundation -> typed command dispatcher ->
**UI-4A (this batch)** -> Asset Library -> History/Diagnostics -> tool toolbar.

## Outcome

Replace the left and right placeholders with the first complete desktop editing
loop:

1. The 3D scene, picking proxies, path handles, and world labels all use the
   central dock node's content rectangle rather than full-window coordinates.
2. The Project panel displays a searchable object hierarchy and supports
   deterministic single, toggle, and visible-range selection.
3. The Inspector displays zero-, single-, and multi-selection states and emits
   typed commands for rename, visibility, lock, transform, duplicate, delete,
   and attachment detach.
4. Panel and menu widgets append to one command frame. `main.cpp` dispatches
   that frame once, after all widgets have emitted and before the status bar is
   drawn.
5. Widgets never mutate documents, selection, history, assets, or editor tool
   state directly.

This is one coherent milestone. Do not ship a visually populated panel whose
selection or geometry is offset from the viewport.

## Interaction Contract

### Project / Outliner

- Keep the dock window name `Project` so the existing DockBuilder layout still
  finds it.
- The panel contains a `Scene` tab. Asset browsing is not part of this batch.
- Put one search field above the hierarchy. Matching is ASCII
  case-insensitive against object name, object-kind label, and decimal object
  id. An empty query shows every row.
- Flatten `CreativeDocument::objects()` into deterministic parent-before-child
  rows using `CreativeObject::parentId`.
  - Preserve document order among roots and among siblings.
  - Do not retain `CreativeObject*` across frames.
  - Missing parents are shown as roots and flagged in the projection.
  - A cycle or excessive depth must terminate deterministically, promote the
    unresolved row to a root, and expose one non-blocking warning in the panel.
  - Do not use recursive widget traversal or an O(n^2) child scan. Build an id
    index and child adjacency once per document revision, then flatten.
- Row selection semantics:
  - Plain click: replace selection with the clicked object; it becomes primary.
  - `Cmd` on macOS or `Ctrl` elsewhere: toggle the clicked object; when added it
    becomes primary.
  - `Shift`: replace selection with the inclusive range between the stored
    anchor and clicked row in the **currently filtered visible order**. If the
    anchor is absent, fall back to plain click.
  - `Shift` wins when both shift and toggle modifiers are down.
  - The anchor changes after a plain or toggle click, not after document-driven
    selection repair.
- Clicking the visibility or lock control must emit only its flag command; it
  must not also change row selection.
- Row context menu commands: `Duplicate` and `Delete`. Use explicit object-id
  payloads. Do not temporarily rewrite the live selection to drive them.
- A selected row must be visually distinct. The primary row gets the normal
  selection color; secondary selected rows use a quieter highlight.
- Show the object-kind label and compact object id. Do not add decorative cards.
- No drag-reparent, layer editing, rename-in-tree, asset tree, or thumbnails in
  this batch.

### Inspector

- Zero selection: show `No object selected` and no disabled fake controls.
- Multi-selection:
  - Show the selected count.
  - Show tri-state visibility and lock values (`On`, `Off`, or `Mixed`). A user
    choice emits one absolute batch command for all selected object ids.
  - Provide `Duplicate` and `Delete` commands.
  - Do not show misleading transform fields; group transform editing is a later
    order.
- Single selection:
  - Header: editable name, object-kind label, and read-only object id.
  - State: visibility and lock controls.
  - Transform: Position, Rotation, and Scale as three numeric triples.
    - Document rotation remains radians. Display and edit degrees only at the
      panel boundary.
    - A field group emits one `SetObjectTransform` command on Enter or
      `ImGui::IsItemDeactivatedAfterEdit()`, with exactly one component mask.
    - Do not emit a mutation for each keystroke or drag sample.
    - Reject non-finite values and any scale component `<= 0.0`; retain the
      draft and show a short inline error instead of dispatching.
  - Locked objects: lock can be cleared, but name and transform fields are
    disabled. The dispatcher remains authoritative if state changes between
    drawing and dispatch.
  - Metadata: show tags and `assetId` read-only when present.
  - Hierarchy: show parent id and `attachmentSocket` read-only. If the object is
    attached (`parentId` present and `attachmentSocket` non-empty), show a
    `Detach` command.
  - Provide `Duplicate` and `Delete` commands.
- Inspector draft lifetime:
  - Key drafts by object id and source document revision.
  - Refresh a draft on selection change.
  - Refresh after a document revision only when none of that draft's fields is
    actively being edited.
  - Never overwrite an active edit every frame.
  - Escape cancels the active draft and restores current document truth.
  - Name editing uses a resizable `std::string` ImGui callback or an equivalent
    non-truncating adapter. Do not silently cap or truncate an existing name.

### Toolbar and Bottom Panel

- Remove the current clickable placeholder tool buttons. A button that has no
  command is a false affordance.
- Keep `Toolbar##desktop` as a compact read-only viewport header showing the
  document name, current selection count, and existing snap/grid facts. Do not
  invent tool-state mutations in this batch.
- Keep the bottom `Diagnostics##bottom` dock window, but replace the five
  repeated `coming soon` tabs with one restrained empty state:
  `No diagnostics for this document`.
- Asset Library, History, validation receipts, diffs, provenance, and a
  functional tool toolbar are later batches.

## Required Post-State Ownership

### New pure model owner

Create:

- `apps/iggy3d_creative/EditorDesktopModel.hpp`
- `apps/iggy3d_creative/EditorDesktopModel.cpp`
- `tests/unit/creative_desktop_model_tests.cpp`

`EditorDesktopModel.*` is ImGui-free and mutation-free. It owns these concepts
(names may vary only with Ace approval):

```cpp
struct CreativeDesktopOutlinerRow {
  CreativeObjectId objectId;
  CreativeObjectId parentObjectId;
  CreativeObjectKind kind;
  std::string name;
  std::uint32_t depth;
  bool hasChildren;
  bool visible;
  bool locked;
  bool recoveredHierarchy;
};

struct CreativeDesktopOutlinerProjection {
  CreativeDocumentId documentId;
  std::uint64_t documentRevision;
  std::vector<CreativeDesktopOutlinerRow> rows;
  bool recoveredHierarchy;
};

enum class CreativeDesktopSelectionGesture : std::uint8_t {
  Replace,
  Toggle,
  VisibleRange,
};

struct CreativeDesktopSelectionPlan {
  std::vector<CreativeObjectId> objectIds;
  CreativeObjectId primaryObjectId;
  CreativeObjectId nextAnchorObjectId;
  bool accepted;
};
```

It owns pure functions for:

- building the hierarchy projection from a document;
- an internal/testable hierarchy kernel that accepts document id, revision, and
  `span<const CreativeObject>` so malformed parent graphs can be pinned without
  weakening `CreativeDocument` validation;
- producing filtered row indices from a query;
- planning a selection transition from visible rows, current selected ids,
  clicked id, anchor id, and gesture;
- deriving zero/single/multi Inspector projection facts without retaining object
  pointers;
- degrees/radians conversion and validation of an Inspector transform draft.

The hierarchy projection may allocate when the document id/revision changes.
It must not rebuild or allocate its structural rows on every idle frame. Search
results may rebuild only when the query or structural projection changes.

### UI-only state owner

Modify `apps/iggy3d_creative/EditorDesktopUi.hpp`.

Add a nested `CreativeDesktopPanelState` (or one equivalently named aggregate)
to `CreativeEditorDesktopUiState`. It owns only:

- cached outliner projection and filtered row indices;
- last applied search query and a search edit buffer;
- selection anchor object id;
- Inspector name and transform drafts, their source object id/revision, dirty
  and active-edit flags, and validation message;
- no document objects, object pointers, Facade, history, or asset catalog.

`EditorDesktopUi.cpp` continues to own only ImGui frame/dockspace lifecycle and
pointer-capture policy. Do not move panel behavior into it.

### Widget adapter owner

Modify `apps/iggy3d_creative/EditorDesktopPanels.hpp/.cpp`.

Change the panel entry point to receive the active app state, editor state, and
the shared command frame:

```cpp
void buildCreativeEditorDesktopPanels(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorState& editor,
    const iggy3d::creative::CreativeAppState& activeAppState,
    CreativeDesktopCommandFrame& commands);
```

This owner may:

- read document, selection, history availability, and current editor settings;
- update UI-only drafts/caches;
- render ImGui widgets;
- append typed desktop commands.

It may not call Facade mutation methods, `EditorEdits` kernels, history
transactions, document mutation APIs, or asset mutation APIs.

### Dispatcher owner

Modify:

- `apps/iggy3d_creative/EditorDesktopCommands.hpp/.cpp`
- `apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp`
- `tests/unit/creative_desktop_ui_command_tests.cpp`

Add `CreativeDesktopCommandId::DetachAttachment` and a typed single-object-id
payload. Dispatch it to the existing
`detachObjectWithUndo(...)` kernel in `EditorEdits.*`. Do not duplicate the
detach mutation or transaction discipline.

Make a multi-command frame result composable. Preserve the last-command message
for the status bar, while accumulating at least:

- processed count;
- accepted count;
- failed count;
- any-change;
- total affected object count;
- `documentReplaced` via logical OR;
- successful document-save via logical OR plus the exact document revision at
  the moment the last successful save in that frame completed.

Do not let a later selection command erase an earlier save/replacement fact in
the same frame. Do not set `lastSavedRevision` from the post-dispatch document:
a later mutation may already have made that saved revision stale. Update
existing command tests to pin this behavior.

### Main-loop owner

Modify `apps/iggy3d_creative/main.cpp` so desktop ordering is exactly:

1. create one `CreativeDesktopCommandFrame`;
2. build Project/Inspector panels into it;
3. build the menu bar into the same frame;
4. dispatch once if non-empty;
5. apply aggregate save/cache/status consequences;
6. draw the status bar from post-dispatch state.

Panels precede the menu intentionally: clicking Save while leaving an edited
Inspector field queues the field's deactivation commit before Save, so the Save
includes the edit. The dispatch result's exact saved revision keeps dirty-state
tracking correct for every other multi-command ordering.

There must be one call to `dispatchCreativeDesktopCommands` in the frame loop.
Do not dispatch once for menus and once for panels.

## Viewport Coordinate Closure

This checkpoint precedes widget implementation. The Vulkan scene viewport is
already correct; remaining CPU-side screen projections still use full drawable
dimensions.

Modify:

- `src/app/iggy3d/creative/render/CreativeScreenProjection.hpp/.cpp`
- `apps/iggy3d_creative/EditorPicking.hpp/.cpp`
- `apps/iggy3d_creative/EditorFrame.hpp`
- `apps/iggy3d_creative/EditorPickFrame.cpp`
- `apps/iggy3d_creative/EditorGizmo.hpp/.cpp`
- `apps/iggy3d_creative/EditorOverlayHud.cpp`
- `apps/iggy3d_creative/EditorInteraction.hpp`
- `apps/iggy3d_creative/EditorInteractionOverlay.cpp`
- `apps/iggy3d_creative/main.cpp`
- `tests/unit/creative_screen_projection_tests.cpp`
- `tests/unit/creative_viewport_layout_tests.cpp`

Required shape:

- Add `RenderContentViewport` overloads to point and bounds screen projection.
  Keep the width/height forms as full-frame wrappers for existing callers.
- Region-aware projection outputs absolute drawable coordinates:
  `region.x + localX`, `region.y + localY`.
- `insideViewport` / `intersectsViewport` compare against the region bounds,
  not `(0,0,width,height)`.
- Add region-aware overloads for `buildObjectVisualPickBounds` and
  `buildPathPointHandleHits`; keep full-frame wrappers only where tests or
  capture proofs still require them.
- `buildCreativeEditorPickFrame` and `buildCreativeEditorGizmoFrame` receive the
  effective content region, not a loose width/height pair.
- In `main.cpp`, compute `effectiveContentViewport(frame)` once after frame
  construction and pass that same value to picking, world interaction, gizmo,
  and overlay requests.
- Volume and selection world labels project through the effective content
  region but still clip glyphs against the full drawable.
- Crosshair already uses the content region; preserve it.
- For gamepad legacy HUD, center the hotbar/held label inside the content region
  and place it above the region's bottom edge. Keep modal overlays full-window.
- Under `--capture`, the shell remains disabled and the effective region is the
  full drawable. Capture output and proof coordinates must stay byte-identical.

Do not change world math, camera math, selection bounds, reach distance, scene
geometry, or render signatures. This checkpoint only closes the drawable-space
to content-space mapping.

## Exact File Scope

### Create

- `apps/iggy3d_creative/EditorDesktopModel.hpp`
- `apps/iggy3d_creative/EditorDesktopModel.cpp`
- `tests/unit/creative_desktop_model_tests.cpp`

### Modify

- `CMakeLists.txt`
- `cmake/iggy3d_tests.cmake`
- `apps/iggy3d_creative/EditorDesktopUi.hpp`
- `apps/iggy3d_creative/EditorDesktopPanels.hpp`
- `apps/iggy3d_creative/EditorDesktopPanels.cpp`
- `apps/iggy3d_creative/EditorDesktopCommands.hpp`
- `apps/iggy3d_creative/EditorDesktopCommands.cpp`
- `apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp`
- `apps/iggy3d_creative/EditorFrame.hpp`
- `apps/iggy3d_creative/EditorPickFrame.cpp`
- `apps/iggy3d_creative/EditorPicking.hpp`
- `apps/iggy3d_creative/EditorPicking.cpp`
- `apps/iggy3d_creative/EditorGizmo.hpp`
- `apps/iggy3d_creative/EditorGizmo.cpp`
- `apps/iggy3d_creative/EditorOverlayHud.cpp`
- `apps/iggy3d_creative/EditorInteraction.hpp`
- `apps/iggy3d_creative/EditorInteractionOverlay.cpp`
- `apps/iggy3d_creative/main.cpp`
- `src/app/iggy3d/creative/render/CreativeScreenProjection.hpp`
- `src/app/iggy3d/creative/render/CreativeScreenProjection.cpp`
- `tests/unit/creative_desktop_ui_command_tests.cpp`
- `tests/unit/creative_screen_projection_tests.cpp`
- `tests/unit/creative_viewport_layout_tests.cpp`

### Scope firewall

- Do not modify `Facade*`, `Document*`, `Mutation*`, `EditorEdits.*`, history,
  save/load, renderer/Vulkan, asset import, Blender tooling, terrain/voxel
  kernels, controls persistence, or capture scripts.
- Do not add drag/drop, reparenting, asset browsing, diagnostics data,
  validation, history rows, keyboard rebinding, or tool-toolbar commands.
- Do not change capture constants or update a golden image/hash.
- Do not edit this work-order file in the implementation commit.
- If an unlisted production file is genuinely required, STOP and report the
  symbol and caller that require it. Do not silently widen the batch.

## Headless Proof

### New pure model tests

`creative_desktop_model_tests.cpp` must pin:

- roots/siblings retain document order;
- children follow parents with correct depth;
- missing parent recovery;
- cycle/depth recovery terminates and reports recovery;
- empty, name, kind, and decimal-id search;
- plain, toggle, and filtered visible-range selection;
- missing anchor range fallback;
- primary and next-anchor facts;
- zero/single/multi Inspector projection;
- mixed visibility/lock facts;
- radians-to-degrees-to-radians tolerance;
- non-finite and non-positive-scale draft rejection;
- projection cache keys are document id + revision, not object pointers.

### Existing focused tests

Extend focused tests to pin:

- region point projection: NDC center maps to the region center including x/y
  offset;
- region bounds projection and region intersection;
- full-frame wrapper parity;
- central content center produces the camera forward ray;
- object screen AABB and path handles carry the region offset;
- crosshair and gamepad hotbar center on the content region;
- Detach command success, failure, one history entry, undo, and redo;
- one command frame containing save/replacement plus a later selection retains
  aggregate save/replacement facts;
- Inspector commit followed by Save records the resulting revision as saved;
  Save followed by a later mutation leaves the document dirty;
- panel command payloads never require direct document mutation.

### Commands

Use a dedicated build directory for this batch so another checkout cannot race
the active one:

```sh
cmake -S . -B build-ui4a -DIGGY3D_BUILD_TESTS=ON -DIGGY3D_BUILD_TOOLS=ON
cmake --build build-ui4a --target \
  i3dc \
  creative_desktop_model_tests \
  creative_desktop_ui_command_tests \
  creative_screen_projection_tests \
  creative_viewport_layout_tests \
  render_content_viewport_tests \
  creative_editor_placement_tests \
  creative_editor_attachment_tests
ctest --test-dir build-ui4a -R \
  '^(creative_desktop_model_tests|creative_desktop_ui_command_tests|creative_screen_projection_tests|creative_viewport_layout_tests|render_content_viewport_tests|creative_editor_placement_tests|creative_editor_attachment_tests)$' \
  --output-on-failure
ctest --test-dir build-ui4a -R '^creative_capture_stability_smoke$' \
  --output-on-failure
git diff --check
```

Do **not** run broad `-L unit` or `-L smoke` labels for this batch. Several smoke
harnesses create transient windows, and the focused list above covers the
changed contracts. Run only the named capture-stability smoke. Do not launch
`i3dc` automatically.

The capture-stability hash must remain the accepted T-0 value already pinned by
the test. Do not copy a hash from this order into code.

## Visual Checkpoint

After headless proof passes, STOP and ask Ace to launch `./build-ui4a/i3dc` once.
Report that no visual check was performed by the builder.

Ace checks:

1. Scene is framed only inside the central viewport; it is not stretched or
   shifted behind Project/Inspector.
2. Crosshair, selection, path handles, and world labels agree spatially.
3. Clicking an Outliner row updates world selection and Inspector immediately.
4. World selection updates Outliner and Inspector immediately.
5. Search, Cmd/Ctrl toggle, Shift range, visibility, lock, rename, one transform
   component, duplicate, delete, and detach behave as specified.
6. Clicking or editing a panel never captures fly-look or places/removes world
   content. Clicking the viewport still captures; Esc still releases.
7. Resizing keeps Project, Inspector, toolbar, status, and central viewport from
   overlapping.
8. No clickable placeholder controls remain.

Do not begin Asset Library or History/Diagnostics while waiting for this check.

## Shared-Worktree Rules

- Work in a dedicated clean worktree/branch rooted at the approved baseline.
- Record `BASE=$(git rev-parse HEAD)` before edits.
- The two `.claude/worktrees/*` gitlinks in Ace's checkout are user-owned. Do
  not stage, reset, remove, or commit them.
- No repo-wide recon agents are needed; this order names the owners and seams.
- Never run `git add -A`. Stage only the explicit files in this order.
- If a scoped file changes externally after work begins, STOP. Do not sweep or
  overwrite another worker's hunks.
- Produce one coherent implementation commit only after all headless gates pass.

## Completion Brief

Report:

- baseline and commit hash;
- files created/modified;
- final ownership boundaries;
- exact selection, draft, and viewport semantics implemented;
- tests and results;
- capture-stability result;
- confirmation that no window was launched;
- visual checkpoint still required;
- deviations or residual risks.
