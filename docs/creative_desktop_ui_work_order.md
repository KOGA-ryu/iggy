# Creative Desktop UI - Current Claude Work Order

> This file holds one implementation batch. Build this post-state exactly.
> Do not redesign the desktop architecture, widen the batch, or start the next
> roadmap milestone. Report a STOP when a stated seam is false.

## Batch UI-4A: Functional Project Hierarchy And General Inspector

**Required baseline:** `90fc40a8` (`Add desktop logic link editing controls`) or
a direct descendant approved by the user.

**Position:** desktop shell -> semantic dispatcher -> Play mode -> Inspector
logic authoring -> **UI-4A (this batch)** -> Asset Library/History -> expanded
runtime logic.

## Baseline Truth

Verify these facts before editing:

1. `apps/iggy3d_creative/EditorDesktopPanels.cpp` still renders placeholder
   tool buttons and a placeholder `Project` panel.
2. The Inspector already owns working logic-link controls and a Play-mode
   runtime monitor. Those controls are accepted behavior, not scaffolding.
3. `EditorDesktopCommands.*` already owns typed commands for selection, focus,
   rename, visibility, locking, transforms, explicit deletion, logic links,
   and authored-asset operations.
4. Widgets append to one `CreativeDesktopCommandFrame`; `main.cpp` dispatches
   it once after menu and panel construction. Keep that order.
5. The central content viewport, pointer capture, HiDPI conversion, and scene
   scissor are already implemented. This batch does not reopen those seams.
6. Play mode permits inspection/navigation but rejects document mutation in
   the dispatcher with `stop play before editing`.
7. Current logic authoring supports TriggerZone/Switch/Lever/PressurePlate/
   Button sources, Door targets, and Toggle/Open/Close actions.

If any fact is false, stop before editing and report the exact file and symbol.

## Outcome

Deliver the first complete desktop object-editing loop:

1. `Project` becomes a searchable, deterministic object hierarchy.
2. The Inspector handles zero, single, and multi-selection and edits ordinary
   object properties through existing semantic commands.
3. Existing logic-link authoring, logic diagnostics, and runtime monitoring
   remain present beneath the general single-object Inspector.
4. Placeholder toolbar buttons are removed. The toolbar becomes a compact,
   read-only viewport header.
5. The bottom panel keeps the two real logic tabs and removes fake clickable
   tabs for features that do not exist.
6. Every document mutation still reaches the existing desktop dispatcher;
   ImGui code never mutates `Facade`, `CreativeDocument`, history, assets, or
   editor tool state directly.

## Non-Negotiable Laws

- **Widgets emit; dispatcher mutates.** No direct mutation from any ImGui TU.
- **One dispatch per frame.** Do not add a second queue or call the dispatcher
  from a widget helper.
- **Play is read-only.** Selection/navigation remains available; all document
  edits are hidden or disabled. The dispatcher remains the final authority.
- **IDs, not pointers.** Never retain `CreativeObject*`, spans, or ImGui item
  pointers across frames.
- **Revision-owned caches.** Rebuild object-derived state only when document id
  or revision changes. Search-filter changes may re-filter cached rows without
  rebuilding the hierarchy.
- **Deterministic hierarchy.** Preserve document order among roots and among
  siblings. Missing parents and cycles recover rather than hanging.
- **No recursive widget traversal and no O(n^2) child scan.** Build an id index
  and child adjacency once per revision, then flatten iteratively.
- **No branch ladder for command routing.** Use the existing enum dispatcher
  and typed payloads.
- **No false affordances.** A visible clickable control must perform a real
  command in this batch.
- **No new persistence fields.** Desktop drafts and caches are transient UI
  state only.

## Interaction Contract

### Project Panel

- Keep the dock window name exactly `Project`.
- Show a single `Scene` tab. Asset browsing is a later batch.
- Place one compact search field above the hierarchy.
- Search is ASCII case-insensitive over object name, object-kind label, and
  decimal object id.
- Empty search shows every object.
- A matching descendant keeps its ancestor path visible so the result retains
  hierarchy context.
- Flatten objects parent-before-child through `CreativeObject::parentId`.
- Preserve source document order among roots and among siblings.
- Objects with missing parents become recovered roots and display a warning
  marker with a tooltip.
- Cycles and excessive depth terminate deterministically. Append each still
  unvisited object once as a recovered root in original document order.
- Use a fixed maximum displayed depth of 64. Deeper descendants remain rows at
  depth 64 and carry a recovered/depth warning.
- Do not retain collapsed-state data in the document. ImGui tree openness is
  UI state only.

Selection behavior:

- Plain click replaces selection with the clicked object and makes it primary.
- `Cmd` on macOS or `Ctrl` elsewhere toggles the clicked object. An added
  object becomes primary.
- `Shift` replaces selection with the inclusive range between the stored
  anchor and clicked row in the currently filtered visible order.
- `Shift` wins when both range and toggle modifiers are down.
- If the anchor is absent from the filtered rows, Shift falls back to plain
  selection.
- Plain and toggle clicks update the anchor. Range selection retains it.
- Selection commands use `CreativeDesktopCommandId::SelectObjects` with
  `CreativeDesktopSelectPayload`.
- Clicking the visibility or lock control emits only its flag command and must
  not also select the row.
- Row selection remains available during Play. Visibility and lock controls do
  not.

Row presentation:

- Use dense rows, not cards.
- Show hierarchy indentation, object name, compact kind label, and object id.
- Primary and secondary selections must be visually distinct.
- Use familiar visibility and lock controls with hover tooltips. Do not add
  text-filled decorative pills.
- No drag-reparent, rename-in-tree, context menu, layer system, thumbnails, or
  asset tree in this batch.

### Inspector

Determine valid selected object ids from current document truth every frame.
Never trust a cached id after a revision.

Zero selection:

- Show `No object selected`.
- Do not render disabled fake controls.

Multi-selection:

- Show the selected object count.
- Show tri-state visibility and lock values: On, Off, or Mixed.
- Choosing a value emits one absolute `SetObjectsVisible` or
  `SetObjectsLocked` command for the complete selected-id list.
- Provide working Duplicate and Delete buttons using the existing selected
  object commands.
- Do not show transform fields for multi-selection in this batch.

Single selection:

- Header: editable name, object-kind label, and read-only object id.
- State: visibility and lock controls.
- Transform: Position, Rotation, and Scale as three numeric triples.
- Document rotation remains radians. Display and edit degrees only at the
  panel boundary.
- Emit a transform command on Enter or `ImGui::IsItemDeactivatedAfterEdit()`;
  never emit one command per keystroke or drag sample.
- Each transform command sets exactly one component mask: position, rotation,
  or scale.
- Reject non-finite values and scale components `<= 0.0`. Retain the draft and
  show a concise inline error instead of dispatching.
- Locked objects may be unlocked, inspected, selected, and used for logic
  navigation, but name and transform edits are disabled.
- Show tags, asset id, parent id, and attachment socket as read-only metadata
  when present.
- Provide working Duplicate and Delete buttons using the current selection.
- In Play, render current values and runtime logic state but disable every
  document-editing control.

Inspector draft behavior:

- Key the draft by document id and object id.
- Refresh on inspected-object change.
- Refresh after a document revision only when no draft field is actively being
  edited.
- Never overwrite an active edit every frame.
- Escape cancels the active draft and restores current document truth.
- Name editing must use a resizable `std::string` callback or equivalent. Do
  not truncate an existing document name into a fixed buffer.

Logic preservation:

- Move the existing logic-link Inspector helpers out of
  `EditorDesktopPanels.cpp` into the new Inspector owner; do not rewrite their
  behavior.
- Preserve `Set as source`, `ACTIVE SOURCE`, source clearing, Add link,
  Toggle/Open/Close editing, removal, endpoint navigation, diagnostics, and
  the Play-mode runtime monitor.
- General object fields appear before `Logic links`.
- Logic mutation controls remain absent during Play.

### Toolbar And Bottom Panel

Toolbar:

- Keep the dock window name `Toolbar##desktop`.
- Delete the non-functional Select/Pan/Move/Rotate/Scale/Place/Erase/Measure
  buttons.
- Show a compact read-only header with document name, valid selection count,
  active control device, and `PLAY` when Play mode is active.
- Do not invent tool activation commands in this batch.

Bottom panel:

- Keep the dock window name `Diagnostics##bottom`.
- Keep the working `Diagnostics` and `Pass Status` tabs exactly functional.
- Remove `Diffs`, `Stale Outputs`, and `Proof Receipts` from the tab bar until
  those products exist.
- Preserve diagnostic row navigation and capacity-overflow reporting.

## Required Post-State Ownership

### Create: Pure Desktop Model

- `apps/iggy3d_creative/EditorDesktopModel.hpp`
- `apps/iggy3d_creative/EditorDesktopModel.cpp`
- `tests/unit/creative_desktop_model_tests.cpp`

This owner is ImGui-free and mutation-free. It owns:

```cpp
enum class CreativeDesktopHierarchyRecovery : std::uint8_t {
  None,
  MissingParent,
  Cycle,
  DepthLimit,
};

struct CreativeDesktopOutlinerRow {
  CreativeObjectId objectId;
  CreativeObjectId parentObjectId;  // kInvalidObjectId when parentId is empty
  CreativeObjectKind kind;
  std::string name;
  std::uint32_t depth;
  bool hasChildren;
  bool visible;
  bool locked;
  CreativeDesktopHierarchyRecovery recovery;
};

struct CreativeDesktopOutlinerModel {
  CreativeDocumentId documentId;
  std::uint64_t documentRevision;
  std::vector<CreativeDesktopOutlinerRow> rows;
  std::size_t recoveredRowCount;
};

struct CreativeDesktopSelectionPlan {
  std::vector<CreativeObjectId> objectIds;
  CreativeObjectId primaryObjectId;
  CreativeObjectId nextAnchorObjectId;
  bool accepted;
};
```

Names may change only when needed to match an existing local convention. The
concept ownership may not move into ImGui code.

Required pure functions:

- Build/rebuild the deterministic hierarchy model from document objects.
- Filter row indices by the search contract while retaining ancestor paths.
- Plan plain, toggle, and visible-range selection.
- Resolve zero/single/multi valid selected-object ids against document truth.
- Validate and convert Inspector transform drafts, including degrees/radians.

Hierarchy implementation:

1. Build `objectId -> source index` once.
2. Build child adjacency in source order once.
3. Iteratively traverse valid roots in source order.
4. Track visit state so each object is emitted at most once.
5. Iteratively append unvisited objects as recovered roots in source order.
6. Clamp displayed depth at 64 and mark recovery rather than dropping rows.

### Create: ImGui Widget Owners

- `apps/iggy3d_creative/EditorDesktopWidgets.hpp`
- `apps/iggy3d_creative/EditorDesktopOutliner.cpp`
- `apps/iggy3d_creative/EditorDesktopInspector.cpp`

`EditorDesktopWidgets.hpp` is one small internal cluster header declaring the
two panel builders. Do not create one header per widget TU.

`EditorDesktopOutliner.cpp` owns only Project-panel rendering and conversion of
pure selection plans into typed desktop commands.

`EditorDesktopInspector.cpp` owns zero/single/multi Inspector rendering,
Inspector drafts, and all existing logic-link/runtime-monitor widgets moved
from `EditorDesktopPanels.cpp`.

Target physical sizes after formatting:

- `EditorDesktopModel.cpp`: 250-500 lines.
- `EditorDesktopOutliner.cpp`: 200-450 lines.
- `EditorDesktopInspector.cpp`: 350-650 lines, including moved logic widgets.
- `EditorDesktopPanels.cpp`: remain below 500 lines after extraction.

These are review bounds, not permission to add blank lines or compress code.
Stop if a concern cannot fit its bound without obscuring behavior.

### Modify

- `apps/iggy3d_creative/EditorDesktopUi.hpp`
  - Add the transient outliner model/cache key, search state, selection anchor,
    filtered-row storage, and Inspector draft.
  - Keep the state ImGui-free and non-persistent.
- `apps/iggy3d_creative/EditorDesktopPanels.cpp`
  - Retain menu, read-only toolbar, diagnostics, status bar, and top-level panel
    orchestration.
  - Delegate Project and Inspector to the new widget owners.
  - Remove moved duplicate helpers.
- `CMakeLists.txt`
  - Add each new app implementation exactly once.
- `cmake/iggy3d_tests.cmake`
  - Register `creative_desktop_model_tests` linked like the existing
    app-level desktop command test.

### Existing Command Mapping - Reuse Exactly

| UI action | Command | Payload |
|---|---|---|
| Select rows | `SelectObjects` | `CreativeDesktopSelectPayload` |
| Clear selection | `ClearSelection` | monostate |
| Rename | `RenameObject` | `CreativeDesktopRenamePayload` |
| Visibility | `SetObjectsVisible` | `CreativeDesktopObjectFlagPayload` |
| Lock | `SetObjectsLocked` | `CreativeDesktopObjectFlagPayload` |
| Position/rotation/scale | `SetObjectTransform` | `CreativeDesktopTransformPayload` |
| Duplicate selected | `DuplicateSelection` | monostate |
| Delete selected | `DeleteSelection` | monostate |
| Logic source/link edits | existing four logic command ids | existing typed logic payload |

No new command id or payload alternative is expected. If a required
interaction cannot be represented by this table, stop and report the missing
semantic command instead of bypassing the dispatcher.

## Scope Firewall

Do not modify:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/EditorDesktopCommands.cpp`
- `apps/iggy3d_creative/EditorDesktopCommands.hpp`
- `apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp`
- `apps/iggy3d_creative/EditorLogicLinks.*`
- `apps/iggy3d_creative/EditorPlayMode.*`
- renderer, Vulkan, frame-input, viewport, picking, camera, or pointer code
- Creative document/mutation/history/persistence code
- terrain, placement, assets, controller routing, or save format code
- existing capture outputs or golden files

Include-only changes outside the allowed list require a STOP and a named
reason before editing.

## Headless Proof

### New Model Tests

`creative_desktop_model_tests.cpp` must pin:

1. Empty document.
2. Parent-before-child flattening with sibling source order preserved.
3. Missing-parent recovery.
4. Self-cycle and multi-object-cycle recovery with every object emitted once.
5. Depth-limit recovery.
6. ASCII case-insensitive name/kind/id filtering.
7. Matching descendants retain ancestor paths.
8. Plain selection replacement and primary id.
9. Toggle add/remove and primary behavior.
10. Shift visible-range selection in both directions.
11. Shift precedence over toggle.
12. Missing filtered anchor falls back to plain selection.
13. Stale selected ids are discarded during zero/single/multi resolution.
14. Degree/radian conversion parity.
15. Non-finite and non-positive-scale draft rejection.

### Existing Regression Tests

Build and run:

- `creative_desktop_model_tests`
- `creative_desktop_ui_command_tests`
- `creative_viewport_layout_tests`
- `creative_editor_logic_link_tests`
- `creative_editor_play_mode_tests`
- `creative_logic_link_tests`

Required commands:

```sh
cmake -S . -B build
cmake --build build --target i3dc creative_desktop_model_tests creative_desktop_ui_command_tests creative_viewport_layout_tests creative_editor_logic_link_tests creative_editor_play_mode_tests creative_logic_link_tests -j 4
ctest --test-dir build -R '^(creative_desktop_model_tests|creative_desktop_ui_command_tests|creative_viewport_layout_tests|creative_editor_logic_link_tests|creative_editor_play_mode_tests|creative_logic_link_tests)$' --output-on-failure
git diff --check
```

Do not run a broad CTest loop and do not launch a window automatically.

## Manual Visual Checkpoint

After headless verification, stop and ask the user to launch `./build/i3dc`.
The user should verify:

1. Project hierarchy is readable and search behaves naturally.
2. Plain/Cmd-or-Ctrl/Shift selection feels correct.
3. Visibility and lock controls do not accidentally select rows.
4. Inspector drafts do not reset while typing or dragging.
5. Logic controls still work beneath general properties.
6. Play mode is inspectable but document controls are read-only.
7. No placeholder toolbar or fake diagnostics tabs remain.
8. Fly-look capture and the central viewport still behave exactly as before.

Do not commit an interactive repair until the user reports the observed issue.

## Stop Conditions

Stop without improvising when:

- baseline facts do not match;
- a requested UI action lacks an existing semantic command;
- preserving logic controls would require changing their kernel contract;
- hierarchy projection cannot guarantee one row per object deterministically;
- any widget would need direct document/history mutation;
- Play-mode editing can reach a document mutation;
- a required include creates an ownership cycle;
- a new TU cannot fit the stated concern/line bound;
- configure, build, a focused test, or `git diff --check` fails.

## Completion Brief

Report:

- exact commit hash, or state explicitly that work remains uncommitted;
- files created and modified;
- final line counts for the four implementation owners;
- hierarchy algorithm and recovery behavior;
- selection modifier behavior;
- Inspector draft lifetime behavior;
- confirmation that logic authoring and Play read-only behavior were preserved;
- build/test commands and exact results;
- visual verification still required;
- deviations, remaining risks, and any STOP encountered.
